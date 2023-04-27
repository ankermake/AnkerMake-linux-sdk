#include <common.h>
#include <linux/list.h>
#include <linux/clk.h>
#include <linux/miscdevice.h>
#include <linux/gpio.h>
#include <linux/module.h>
#include <linux/ioctl.h>
#include <linux/fs.h>

#include <soc/base.h>
#include <soc/irq.h>
#include <linux/interrupt.h>
#include <linux/irq.h>
#include <linux/kernel.h>
#include <linux/slab.h>

#include <linux/spinlock.h>
#include <linux/mutex.h>
#include <linux/wait.h>
#include <linux/io.h>
#include <linux/dma-mapping.h>
#include <linux/string.h>
#include <linux/delay.h>
#include <linux/sched.h>
#include <bit_field.h>


#include "vic_sensor.h"
#include "camera.h"
#include "vic_regs.h"
#include "csi_regs.h"

#define VIC_ALIGN_SIZE                  64

int dvp_init_mclk_gpio(void);
void dvp_deinit_mclk_gpio(void);

void vic_init_sensors(void);


/*
 * DVP operation
 */
#define VIC_ADDR(reg)    ((volatile unsigned long *)CKSEG1ADDR(ISP_VIC_IOBASE + reg))

static inline void vic_write_reg(unsigned int reg, int val)
{
    *VIC_ADDR(reg) = val;
}

static inline unsigned int vic_read_reg(unsigned int reg)
{
    return *VIC_ADDR(reg);
}

static inline void vic_set_bit(unsigned int reg, int start, int end, unsigned int val)
{
    set_bit_field(VIC_ADDR(reg), start, end, val);
}

static inline unsigned int vic_get_bit(unsigned int reg, int start, int end)
{
    return get_bit_field(VIC_ADDR(reg), start, end);
}

/*
 * MIPI operation
 */
#define CSI_ADDR(reg)    ((volatile unsigned long *)CKSEG1ADDR(MIPI_CSI_IOBASE + reg))

static inline void csi_write_reg(unsigned int reg, int val)
{
    *CSI_ADDR(reg) = val;
}

static inline unsigned int csi_read_reg(unsigned int reg)
{
    return *CSI_ADDR(reg);
}

static inline void csi_set_bit(unsigned int reg, int start, int end, unsigned int val)
{
    set_bit_field(CSI_ADDR(reg), start, end, val);
}

static inline unsigned int csi_get_bit(unsigned int reg, int start, int end)
{
    return get_bit_field(CSI_ADDR(reg), start, end);
}

struct camera_device {
    struct vic_sensor_config *sensor;
    unsigned int is_power_on;
    unsigned int is_stream_on;
};

static DEFINE_MUTEX(lock);
static DEFINE_SPINLOCK(spinlock);

static struct camera_device m_camera;

struct frm_data {
    struct list_head link;
    void *address;
    unsigned int is_user;
};

struct vic_data {
    struct clk *mclk;
    struct clk *isp_clk;
    struct clk *cgu_isp_clk;
    struct clk *csi_clk;
    wait_queue_head_t waiter;

    unsigned long isp_clk_rate;

    void *mem;
    unsigned int mem_cnt;
    unsigned int frm_size;
    unsigned int uv_data_offset;

    volatile unsigned int frame_counter;
    volatile unsigned int dma_err;
    volatile unsigned int wait_timeout;
    unsigned int dma_index;

    struct list_head free_list;
    struct list_head usable_list;
    struct list_head user_list;

    struct frm_data *t[2];

    struct frm_data *frms;
};

static struct vic_data vic_data;

static int report_error_if_dma_err = 0;

static int m_mem_cnt;
module_param_named(frame_nums, m_mem_cnt, int, 0644);

static unsigned long rmem_start = 0;
module_param(rmem_start, ulong, 0644);

static unsigned int rmem_size = 0;
module_param(rmem_size, uint, 0644);

static inline void vic_debug_remain_lines(void)
{
    unsigned int line = vic_get_bit(VIC_LINE_CNT, REMAIN_LINE);

    while (1) {
        printk("%d\n", line);
        while (line == vic_get_bit(VIC_LINE_CNT, REMAIN_LINE));
        line = vic_get_bit(VIC_LINE_CNT, REMAIN_LINE);
    }
}

static unsigned int dma_addr[][2] = {
    {DMA_Y_CH_BANK0_ADDR, DMA_UV_CH_BANK0_ADDR},
    {DMA_Y_CH_BANK1_ADDR, DMA_UV_CH_BANK1_ADDR},
    {DMA_Y_CH_BANK2_ADDR, DMA_UV_CH_BANK2_ADDR},
    {DMA_Y_CH_BANK3_ADDR, DMA_UV_CH_BANK3_ADDR},
    {DMA_Y_CH_BANK4_ADDR, DMA_UV_CH_BANK4_ADDR},
};

static void vic_set_dma_addr(unsigned long y_addr, unsigned long uv_addr, int index)
{
    vic_write_reg(dma_addr[index][0], y_addr);
    vic_write_reg(dma_addr[index][1], uv_addr);
}

static inline void vic_get_dma_addr(unsigned long *y_addr, unsigned long *uv_addr, int index)
{
    *y_addr = vic_read_reg(dma_addr[index][0]);
    *uv_addr = vic_read_reg(dma_addr[index][1]);
}

static void set_dma_addr(struct frm_data *frm, int index)
{
    unsigned long address = virt_to_phys(frm->address);
    vic_set_dma_addr(address, address + vic_data.uv_data_offset, index);
}

static void add_to_free_list(struct frm_data *frm)
{
    list_add_tail(&frm->link, &vic_data.free_list);
}

static struct frm_data *get_free_frm(void)
{
    if (list_empty(&vic_data.free_list))
        return NULL;

    struct frm_data *frm = list_first_entry(&vic_data.free_list, struct frm_data, link);
    list_del(&frm->link);
    return frm;
}

static void add_to_usable_list(struct frm_data *frm)
{
    vic_data.frame_counter++;
    list_add_tail(&frm->link, &vic_data.usable_list);
}

static struct frm_data *get_usable_frm(void)
{
    if (list_empty(&vic_data.usable_list))
        return NULL;

    vic_data.frame_counter--;

    struct frm_data *frm = list_first_entry(&vic_data.usable_list, struct frm_data, link);
    list_del(&frm->link);
    return frm;
}

static void init_frm_lists(void)
{
    INIT_LIST_HEAD(&vic_data.free_list);
    INIT_LIST_HEAD(&vic_data.usable_list);
    vic_data.dma_index = 0;
    vic_data.frame_counter = 0;
    vic_data.dma_err = 0;
    vic_data.wait_timeout = 0;

    int i;
    for (i = 0; i < vic_data.mem_cnt; i++) {
        struct frm_data *frm = &vic_data.frms[i];
        frm->address = vic_data.mem + i * vic_data.frm_size;
        frm->is_user = 0;
        add_to_free_list(frm);
    }
}

static void reset_frm_lists(void)
{
    int i;
    for (i = 0; i < vic_data.mem_cnt; i++) {
        struct frm_data *frm = &vic_data.frms[i];
        frm->is_user = 0;
        if (vic_data.t[0] != frm && vic_data.t[1] != frm)
            list_del(&frm->link);
        add_to_free_list(frm);
    }

    vic_data.dma_index = 0;
    vic_data.frame_counter = 0;
    vic_data.dma_err = 0;
    vic_data.wait_timeout = 0;
}

static inline void vic_dump_reg(void)
{
    printk("==========dump vic reg============\n");
    printk("BT_DVP_CFG      : 0x%08x\n",    vic_read_reg(VIC_INPUT_DVP));
    printk("INTERFACE_TYPE  : 0x%08x\n",    vic_read_reg(VIC_INPUT_INTF));
    printk("IDI_TYPE_TYPE   : 0x%08x\n",    vic_read_reg(VIC_INPUT_MIPI));
    printk("RESOLUTION      : 0x%08x\n",    vic_read_reg(VIC_RESOLUTION));
    printk("AB_VALUE        : 0x%08x\n",    vic_read_reg(0x18)); /* invalid */
    printk("GLOBAL_CONFIGURE: 0x%08x\n",    vic_read_reg(VIC_OUTPUT_CFG));
    printk("CONTROL         : 0x%08x\n",    vic_read_reg(VIC_CONTROL));
    printk("PIXEL           : 0x%08x\n",    vic_read_reg(VIC_PIXEL_CNT));
    printk("LINE            : 0x%08x\n",    vic_read_reg(VIC_LINE_CNT));
    printk("VIC_STATE       : 0x%08x\n",    vic_read_reg(VIC_STATE));
    printk("OFIFO_COUNT     : 0x%08x\n",    vic_read_reg(VIC_OUT_FIFO_CNT));
    printk("0x3c            : 0x%08x\n",    vic_read_reg(0x3c));
    printk("0x40            : 0x%08x\n",    vic_read_reg(0x40));
    printk("0x44            : 0x%08x\n",    vic_read_reg(0x44));
    printk("0x54            : 0x%08x\n",    vic_read_reg(0x54));
    printk("0x58            : 0x%08x\n",    vic_read_reg(0x58));
    printk("0x58            : 0x%08x\n",    vic_read_reg(0x5c));
    printk("0x60            : 0x%08x\n",    vic_read_reg(0x60));
    printk("0x64            : 0x%08x\n",    vic_read_reg(0x64));
    printk("0x68            : 0x%08x\n",    vic_read_reg(0x68));
    printk("0x6c            : 0x%08x\n",    vic_read_reg(0x6c));
    printk("0x70            : 0x%08x\n",    vic_read_reg(0x70));
    printk("0x74            : 0x%08x\n",    vic_read_reg(0x74));
    printk("0x04            : 0x%08x\n",    vic_read_reg(0x04));
    printk("0x08            : 0x%08x\n",    vic_read_reg(0x08));
    printk("VIC_EIGHTH_CB   : 0x%08x\n",    vic_read_reg(VIC_EIGHTH_CB));
    printk("CB_MODE0        : 0x%08x\n",    vic_read_reg(VIC_BACK_CB_CTRL));
}

static inline void mipi_csi_dphy_test_data_in(unsigned char data)
{
    csi_write_reg(MIPI_PHY_TST_CTRL1, data);
}

static inline void mipi_csi_dphy_test_en(unsigned char on_falling_edge)
{
    csi_set_bit(MIPI_PHY_TST_CTRL1, CSI_PHY_test_ctrl1_testen, on_falling_edge);
}

static inline void mipi_csi_dphy_test_clock(int value)
{
    csi_set_bit(MIPI_PHY_TST_CTRL0, CSI_PHY_test_ctrl0_testclk, value);
}

static inline void mipi_csi_dphy_test_clear(int value)
{
    csi_set_bit(MIPI_PHY_TST_CTRL0, CSI_PHY_test_ctrl0_testclr, value);
}

static inline void mipi_csi_dphy_test_data_out(void)
{
    /* TODO */
}

static inline void mipi_csi_dphy_set_lanes(int lanes)
{
    csi_set_bit(MIPI_N_LANES, CSI_PHY_n_lanes, lanes-1);
}

static inline void mipi_csi_dphy_phy_shutdown(int enable)
{
    csi_set_bit(MIPI_PHY_SHUTDOWNZ, CSI_PHY_phy_shutdown, enable);
}

static inline void mipi_csi_dphy_dphy_reset(int state)
{
    csi_set_bit(MIPI_DPHY_RSTZ, CSI_PHY_dphy_reset, state);
}

static inline void mipi_csi_dphy_csi2_reset(int state)
{
    csi_set_bit(MIPI_CSI2_RESETN, CSI_PHY_csi2_reset, state);
}

static inline int mipi_csi_dphy_get_state(void)
{
    return csi_read_reg(MIPI_PHY_STATE);
}

static unsigned char mipi_csi_event_disable(unsigned int  mask, unsigned char err_reg_no)
{
    switch (err_reg_no) {
    case CSI_ERR_MASK_REGISTER1:
        csi_write_reg(MIPI_MASK1, mask | csi_read_reg(MIPI_MASK1));
        break;
    case CSI_ERR_MASK_REGISTER2:
        csi_write_reg(MIPI_MASK2, mask | csi_read_reg(MIPI_MASK2));
        break;
    default:
        return CSI_ERR_OUT_OF_BOUND;
    }

    return 0;
}

static int mipi_csi_phy_ready(int lanes)
{
    int ready;
    int ret = 0;

    ready = mipi_csi_dphy_get_state();
    ret = ready & (1 << CSI_PHY_STATE_CLK_LANE_STOP );

    switch (lanes) {
    case 2:
        ret = ret && (ready & (1 << CSI_PHY_STATE_DATA_LANE1));
    case 1:
        ret = ret && (ready & (1 << CSI_PHY_STATE_DATA_LANE0));
        break;
    default:
        break;
    }

    return !!ret;
}

static inline void mipi_csi_dphy_test_loop(unsigned char address, unsigned char *data, unsigned char data_length)
{
    unsigned i = 0;

    if (data != 0) {
        /* set the TESTCLK input high in preparation to latch in the desired test mode */
        mipi_csi_dphy_test_clock(1);
        /* set the desired test code in the input 8-bit bus TESTDIN[7:0] */
        mipi_csi_dphy_test_data_in(address);
        /* set TESTEN input high */
        mipi_csi_dphy_test_en(1);
        /* drive the TESTCLK input low; the falling edge captures the chosen test code into the transceiver */
        mipi_csi_dphy_test_clock(0);
        /* set TESTEN input low to disable further test mode code latching  */
        mipi_csi_dphy_test_en(0);
        /* start writing MSB first */
        for (i = data_length; i > 0; i--) {
            /* set TESTDIN[7:0] to the desired test data appropriate to the chosen test mode */
            mipi_csi_dphy_test_data_in(data[i - 1]);
            /* pulse TESTCLK high to capture this test data into the macrocell; repeat these two steps as necessary */
            mipi_csi_dphy_test_clock(1);

            mipi_csi_dphy_test_clock(0);
        }
    }
    mipi_csi_dphy_test_data_out();
}

static inline void mipi_csi_dump_regs(void)
{
    printk(KERN_ERR "==========dump csi reg============\n");
    printk(KERN_ERR "CSI_VERSION     : %08x\n", csi_read_reg(MIPI_PHY_VERSION));
    printk(KERN_ERR "N_LANES         : %08x\n", csi_read_reg(MIPI_N_LANES));
    printk(KERN_ERR "PHY_SHUTDOWNZ   : %08x\n", csi_read_reg(MIPI_PHY_SHUTDOWNZ));
    printk(KERN_ERR "DPHY_RSTZ       : %08x\n", csi_read_reg(MIPI_DPHY_RSTZ));
    printk(KERN_ERR "CSI2_RESETN     : %08x\n", csi_read_reg(MIPI_CSI2_RESETN));
    printk(KERN_ERR "PHY_STATE       : %08x\n", csi_read_reg(MIPI_PHY_STATE));
    printk(KERN_ERR "DATA_IDS_1      : %08x\n", csi_read_reg(MIPI_DATA_IDS_1));
    printk(KERN_ERR "DATA_IDS_2      : %08x\n", csi_read_reg(MIPI_DATA_IDS_2));
    printk(KERN_ERR "ERR1            : %08x\n", csi_read_reg(MIPI_ERR1));
    printk(KERN_ERR "ERR2            : %08x\n", csi_read_reg(MIPI_ERR2));
    printk(KERN_ERR "MASK1           : %08x\n", csi_read_reg(MIPI_MASK1));
    printk(KERN_ERR "MASK2           : %08x\n", csi_read_reg(MIPI_MASK2));
}

static inline int mipi_csi_phy_configure(int lanes)
{
    mipi_csi_dphy_test_clear(1);
    mdelay(10);

    mipi_csi_dphy_set_lanes(lanes);

    /*reset phy*/
    mipi_csi_dphy_phy_shutdown(0);
    mipi_csi_dphy_dphy_reset(0);
    mipi_csi_dphy_csi2_reset(0);
#if 0
    /* test phy loop */
    unsigned char data[4];

    udelay(15);

    mipi_csi_dphy_test_clear(0);

    mipi_csi_dphy_test_data_in(0);

    mipi_csi_dphy_test_en(0);
    mipi_csi_dphy_test_clock(0);

    data[0]=0x00;
    mipi_csi_dphy_test_loop(0x44,data, 1);

    data[0]=0x1e;
    mipi_csi_dphy_test_loop(0xb0,data, 1);

    data[0]=0x1;
    mipi_csi_dphy_test_loop(0xb1,data, 1);
#endif
    mdelay(10);
    mipi_csi_dphy_phy_shutdown(1);
    mipi_csi_dphy_dphy_reset(1);
    mipi_csi_dphy_csi2_reset(1);
    mipi_csi_event_disable(0xffffffff, CSI_ERR_MASK_REGISTER1);
    mipi_csi_event_disable(0xffffffff, CSI_ERR_MASK_REGISTER2);
    mdelay(10);

    //mipi_csi_dump_regs();
    /* wait for phy ready */
    return 0;
}

static int mipi_csi_phy_initialization(struct mipi_csi_bus_info *mipi_info)
{
    int retries = 30;
    int ret = 0;
    int i;

    mipi_csi_phy_configure(mipi_info->lanes);

    /* wait for phy ready */
    for (i = 0; i < retries; i++) {
        if (mipi_csi_phy_ready(mipi_info->lanes))
            break;

        udelay(2);
    }

    if (i >= retries) {
        ret = -1;
        printk(KERN_ERR "csi init failure!\n");
        return ret;
    }

    return ret;
}


/* 输入为raw8时控制器输出会变成raw16，当我们想得到raw8时，
 * 我们就把raw8数据当yuv数据来处理，将输入输出格式均设为yuv422，
 * 因为输入输出yuv422格式时不会改变原数据，这样我们就可以得到原封不动的raw8数据了。
*/
static unsigned int is_output_y8(struct vic_sensor_config *cfg)
{
    if (cfg->vic_interface == VIC_dvp)
        return (cfg->dvp_cfg_info.dvp_data_fmt == DVP_RAW8 && \
                sensor_fmt_is_8BIT(cfg->sensor_info.fmt) );
    if (cfg->vic_interface == VIC_mipi_csi)
        return (cfg->mipi_cfg_info.data_fmt == MIPI_RAW8 && \
                sensor_fmt_is_8BIT(cfg->sensor_info.fmt));

    return 0;
}

/*
 * 输入为yuv422时 DMA控制器可以重新排列输出的顺序,
 * 所以YUV422输入可以选择输出NV12/NV21/Grey格式
*/
static unsigned int is_output_yuv422(struct vic_sensor_config *cfg)
{
    if (cfg->vic_interface == VIC_dvp)
        return (cfg->dvp_cfg_info.dvp_data_fmt == DVP_YUV422 && \
                sensor_fmt_is_YUV422(cfg->sensor_info.fmt));

    if (cfg->vic_interface == VIC_mipi_csi)
        return (cfg->mipi_cfg_info.data_fmt == MIPI_YUV422 && \
                sensor_fmt_is_YUV422(cfg->sensor_info.fmt));

    return 0;
}

static void vic_init_dvp_timing(struct vic_sensor_config *cfg)
{
    unsigned long vic_input_dvp = vic_read_reg(VIC_INPUT_DVP);
    unsigned long yuv_data_order = cfg->dvp_cfg_info.dvp_yuv_data_order;

    if (is_output_y8(cfg))
        set_bit_field(&vic_input_dvp, DVP_DATA_FORMAT, 6);
    else if (cfg->dvp_cfg_info.dvp_data_fmt <= DVP_RAW12)
        set_bit_field(&vic_input_dvp, DVP_DATA_FORMAT, cfg->dvp_cfg_info.dvp_data_fmt);
    else if (cfg->dvp_cfg_info.dvp_data_fmt == DVP_YUV422)
        set_bit_field(&vic_input_dvp, DVP_DATA_FORMAT, 6); // YUV422(8bit IO)

    if (is_output_y8(cfg))  // 不改变四个raw8的先后顺序
        yuv_data_order = order_1_2_3_4;

    set_bit_field(&vic_input_dvp, YUV_DATA_ORDER, yuv_data_order);
    set_bit_field(&vic_input_dvp, DVP_TIMING_MODE, cfg->dvp_cfg_info.dvp_timing_mode);
    set_bit_field(&vic_input_dvp, HSYNC_POLAR, cfg->dvp_cfg_info.dvp_hsync_polarity);
    set_bit_field(&vic_input_dvp, VSYNC_POLAR, cfg->dvp_cfg_info.dvp_vsync_polarity);
    set_bit_field(&vic_input_dvp, INTERLACE_EN, cfg->dvp_cfg_info.dvp_img_scan_mode);

    if (cfg->dvp_cfg_info.dvp_gpio_mode == DVP_PA_HIGH_8BIT ||
        cfg->dvp_cfg_info.dvp_gpio_mode == DVP_PA_HIGH_10BIT)
        set_bit_field(&vic_input_dvp, DVP_RAW_ALIGN, 1);
    else
        set_bit_field(&vic_input_dvp, DVP_RAW_ALIGN, 0);

    vic_write_reg(VIC_INPUT_DVP, vic_input_dvp);
}

static void vic_init_mipi_format(struct vic_sensor_config *cfg)
{
    if (is_output_y8(cfg))
        vic_write_reg(VIC_INPUT_MIPI, MIPI_YUV422);
    else
        vic_write_reg(VIC_INPUT_MIPI, cfg->mipi_cfg_info.data_fmt);
}

static void vic_start(void)
{
    /* reset vic 控制器 */
    vic_set_bit(VIC_CONTROL, VIC_START, 1);
}

static void vic_reset(void)
{
    /* reset vic 控制器 */
    vic_set_bit(VIC_CONTROL, VIC_RESET, 1);
}

static void dma_reset(void)
{
    /* reset dma */
    vic_write_reg(DMA_RESET, 1);
}

static void vic_reg_enable(void)
{
    int timeout = 3000;
    vic_set_bit(VIC_CONTROL, VIC_REG_ENABLE, 1);
    while (vic_get_bit(VIC_CONTROL, VIC_REG_ENABLE)) {
        if (--timeout == 0) {
            printk(KERN_ERR "timeout while wait vic_reg_enable: %x\n", vic_read_reg(VIC_CONTROL));
            break;
        }
    }
}

static void vic_init_common_setting(struct vic_sensor_config *cfg)
{
    unsigned long resolution = 0;
    unsigned long horizontal_resolution = cfg->info.width;

    /* sensor输出的图像数据是raw8的，但我们是使用yuv422的格式输入和输出的，
     * 因为raw8一个像素1个字节 yuv422一个像素占2个字节。
     * 所以填入寄存器的像素点为raw8像素点的1/2。
    */
    if (is_output_y8(cfg))
        horizontal_resolution /= 2;

    set_bit_field(&resolution, HORIZONTAL_RESOLUTION, horizontal_resolution);
    set_bit_field(&resolution, VERTICAL_RESOLUTION, cfg->info.height);
    vic_write_reg(VIC_RESOLUTION, resolution);

    vic_write_reg(VIC_INPUT_INTF, cfg->vic_interface);

    unsigned long vic_out_cfg = vic_read_reg(VIC_OUTPUT_CFG);
    set_bit_field(&vic_out_cfg, ISP_PORT_MOD, 0);
    set_bit_field(&vic_out_cfg, VCKE_ENA_BLE, 0);
    set_bit_field(&vic_out_cfg, BLANK_ENABLE, 0);
    set_bit_field(&vic_out_cfg, AB_MODE_SELECT, 0);
    vic_write_reg(VIC_OUTPUT_CFG, vic_out_cfg);

    unsigned long isp_clk_gate = vic_read_reg(ISP_CLK_GATE);
    set_bit_field(&isp_clk_gate, DMA_CLK_GATE_EN, 0);
    set_bit_field(&isp_clk_gate, VPCLK_GATE_EN, 0);
    vic_write_reg(ISP_CLK_GATE, isp_clk_gate);
}

static void init_dvp_dma(struct vic_sensor_config *cfg)
{
    unsigned long dma_resolution = 0;
    unsigned long horizontal_resolution = cfg->info.width;

    if (is_output_y8(cfg))
        horizontal_resolution /= 2;

    set_bit_field(&dma_resolution, DMA_HORIZONTAL_RESOLUTION, horizontal_resolution);
    set_bit_field(&dma_resolution, DMA_VERTICAL_RESOLUTION, cfg->info.height);
    vic_write_reg(DMA_RESOLUTION, dma_resolution);

    unsigned int base_mode = 0;
    unsigned int y_stride = 0;
    unsigned int uv_stride = 0;

    switch (cfg->dvp_cfg_info.dvp_data_fmt) {
    case DVP_RAW8:
    case DVP_RAW10:
    case DVP_RAW12:
        base_mode = 0;
        y_stride = cfg->info.width * 2;
        if (is_output_y8(cfg)) {
            base_mode = 3;
            y_stride = cfg->info.width;
        }
        break;

    case DVP_YUV422:
        if (cfg->info.data_fmt == CAMERA_PIX_FMT_GREY) {
            base_mode = 6;
            y_stride = cfg->info.width;
        } else if (cfg->info.data_fmt == CAMERA_PIX_FMT_NV12) {
            base_mode = 6;
            uv_stride = cfg->info.width;
            y_stride = cfg->info.width;
        } else if (cfg->info.data_fmt == CAMERA_PIX_FMT_NV21) {
            base_mode = 7;
            uv_stride = cfg->info.width;
            y_stride = cfg->info.width;
        } else {
            base_mode = 3;
            y_stride = cfg->info.width * 2;
        }
        break;

    default:
        break;
    }

    vic_write_reg(DMA_Y_CH_LINE_STRIDE, y_stride);
    vic_write_reg(DMA_UV_CH_LINE_STRIDE, uv_stride);

    unsigned long dma_configure = vic_read_reg(DMA_CONFIGURE);
    set_bit_field(&dma_configure, Dma_en, 1);
    set_bit_field(&dma_configure, Buffer_number, 2 - 1);
    set_bit_field(&dma_configure, Base_mode, base_mode);
    set_bit_field(&dma_configure, Yuv422_order, 2);
    vic_write_reg(DMA_CONFIGURE, dma_configure);
}

static void init_mipi_dma(struct vic_sensor_config *cfg)
{
    unsigned long dma_resolution = 0;
    unsigned long horizontal_resolution = cfg->info.width;

    if (is_output_y8(cfg))
        horizontal_resolution /= 2;

    set_bit_field(&dma_resolution, DMA_HORIZONTAL_RESOLUTION, horizontal_resolution);
    set_bit_field(&dma_resolution, DMA_VERTICAL_RESOLUTION, cfg->info.height);
    vic_write_reg(DMA_RESOLUTION, dma_resolution);

    unsigned int base_mode = 0;
    unsigned int y_stride = 0;
    unsigned int uv_stride = 0;

    switch (cfg->mipi_cfg_info.data_fmt) {
    case MIPI_RAW8:
    case MIPI_RAW10:
    case MIPI_RAW12:
        base_mode = 0;
        y_stride = cfg->info.width * 2;
        if (is_output_y8(cfg)) {
            base_mode = 3;
            y_stride = cfg->info.width;
        }
        break;

    case MIPI_YUV422:
        if (cfg->info.data_fmt == CAMERA_PIX_FMT_GREY) {
            base_mode = 6;
            y_stride = cfg->info.width;
        } else if (cfg->info.data_fmt == CAMERA_PIX_FMT_NV12) {
            base_mode = 6;
            uv_stride = cfg->info.width;
            y_stride = cfg->info.width;
        } else if (cfg->info.data_fmt == CAMERA_PIX_FMT_NV21) {
            base_mode = 7;
            uv_stride = cfg->info.width;
            y_stride = cfg->info.width;
        } else {
            base_mode = 3;
            y_stride = cfg->info.width * 2;
        }
        break;

    default:
        break;
    }

    vic_write_reg(DMA_Y_CH_LINE_STRIDE, y_stride);
    vic_write_reg(DMA_UV_CH_LINE_STRIDE, uv_stride);

    unsigned long dma_configure = vic_read_reg(DMA_CONFIGURE);
    set_bit_field(&dma_configure, Dma_en, 1);
    set_bit_field(&dma_configure, Buffer_number, 2 - 1);
    set_bit_field(&dma_configure, Base_mode, base_mode);
    set_bit_field(&dma_configure, Yuv422_order, 3);
    vic_write_reg(DMA_CONFIGURE, dma_configure);
}

static void init_mipi_irq(void)
{
    unsigned long irq_en = 0;
    set_bit_field(&irq_en, DMA_DVP_OVF, 1);
    set_bit_field(&irq_en, DMA_FRD, 1);
    set_bit_field(&irq_en, CORE_FIFO_FAIL, 1);
    set_bit_field(&irq_en, VIC_HVF, 1);
    set_bit_field(&irq_en, VIC_IMVE, 1);
    set_bit_field(&irq_en, VIC_IMHE, 1);
    set_bit_field(&irq_en, VIC_MIPI_OVF, 1);
    set_bit_field(&irq_en, VIC_FRD, 1);
    vic_write_reg(ISP_IRQ_EN, irq_en);

    vic_write_reg(ISP_IRQ_CNTCA, vic_read_reg(ISP_IRQ_SRC));
}

static void init_dvp_irq(void)
{
    unsigned long irq_en = 0;
    set_bit_field(&irq_en, DMA_DVP_OVF, 1);
    set_bit_field(&irq_en, DMA_FRD, 1);
    set_bit_field(&irq_en, CORE_FIFO_FAIL, 1);
    set_bit_field(&irq_en, VIC_HVF, 1);
    set_bit_field(&irq_en, VIC_IMVE, 1);
    set_bit_field(&irq_en, VIC_IMHE, 1);
    set_bit_field(&irq_en, VIC_DVP_OVF, 1);
    set_bit_field(&irq_en, VIC_FRD, 1);
    vic_write_reg(ISP_IRQ_EN, irq_en);

    vic_write_reg(ISP_IRQ_CNTCA, vic_read_reg(ISP_IRQ_SRC));
}

static void init_dma_addr(void)
{
    struct frm_data *frm;

    frm = get_free_frm();
    vic_data.t[0] = frm;
    set_dma_addr(frm, 0);

    frm = get_free_frm();
    vic_data.t[1] = frm;
    set_dma_addr(frm, 1);
}

static void vic_dvp_init_setting(struct vic_sensor_config *cfg)
{
    assert_range(cfg->info.width, 1, 2560);
    assert_range(cfg->info.height, 1, 4096);
    assert_range(cfg->dvp_cfg_info.dvp_data_fmt, DVP_RAW8, DVP_YUV422);
    assert(cfg->vic_interface == VIC_dvp);
    assert(cfg->dvp_cfg_info.dvp_timing_mode == DVP_href_mode);

    vic_reset();

    vic_init_common_setting(cfg);

    vic_init_dvp_timing(cfg);

    vic_reg_enable();

    dma_reset();

    init_dma_addr();

    init_dvp_dma(cfg);

    init_dvp_irq();

    vic_start();

    //vic_dump_reg();
}

static int vic_mipi_init_setting(struct vic_sensor_config *cfg)
{
    assert_range(cfg->info.width, 1, 2560);
    assert_range(cfg->info.height, 1, 4096);
    assert_range(cfg->mipi_cfg_info.data_fmt, MIPI_RAW8, MIPI_YUV422);

    assert(cfg->vic_interface == VIC_mipi_csi);

    vic_reset();

    vic_init_common_setting(cfg);

    vic_init_mipi_format(cfg);

    vic_reg_enable();

    dma_reset();

    init_dma_addr();

    init_mipi_dma(cfg);

    init_mipi_irq();

    int csi_ret = mipi_csi_phy_initialization(&cfg->mipi_cfg_info);
    if (csi_ret < 0)
        return -1;

    vic_start();
    //vic_dump_reg();

    return 0;
}

static int vic_frm_done = 0;

static irqreturn_t vic_irq_handler(int irq, void *data)
{
    unsigned long irq_src = vic_read_reg(ISP_IRQ_SRC);

    if (get_bit_field(&irq_src, DMA_FRD)) {
        struct frm_data *frm, *usable_frm = NULL;

        vic_write_reg(ISP_IRQ_CNTCA, bit_field_val(DMA_FRD, 1));

        if (get_bit_field(&irq_src, VIC_FRD)) {
            vic_write_reg(ISP_IRQ_CNTCA, bit_field_val(VIC_FRD, 1));
            vic_frm_done = 1;
        }

        int dma_state = vic_read_reg(VIC_DMA_OFF + 0xa0);
        int vic_state = vic_read_reg(VIC_STATE);
        int pixel_cnt = vic_read_reg(VIC_PIXEL_CNT);
        int line_cnt = vic_read_reg(VIC_LINE_CNT);
        int index = vic_data.dma_index;
        vic_data.dma_index = !index;

        /* 当传输列表只有一帧的时候，不能
         */
        if (vic_data.t[0] != vic_data.t[1])
            usable_frm = vic_data.t[index];

        /* 1 优先从空闲列表中获取新的帧加入传输
         * 2 如果空闲列表没有帧，那么传输完成列表中获取
         * 3 如果完成列表也没有，那么用下一帧做保底
         */
        frm = get_free_frm();
        if (!frm)
            frm = get_usable_frm();
        if (!frm)
            frm = vic_data.t[!index];
        vic_data.t[index] = frm;
        set_dma_addr(frm, index);

        if (report_error_if_dma_err && (dma_state != 0 || !vic_frm_done)) {
            printk(KERN_ERR "dma_state: %d %d %d %d %d\n", vic_frm_done, dma_state, vic_state, pixel_cnt, line_cnt);
            vic_data.dma_err = 1;
        }

        /* 如果dma state 不是 0,或者dma frame done 比vic frame done 先来
         * 那么重新复位 vic dma, dma 可能已经出错了
         * dma 出错无迹可寻,暂时靠这个判断
         */
        if (!report_error_if_dma_err && (dma_state != 0 || !vic_frm_done)) {
            vic_set_bit(DMA_CONFIGURE, Dma_en, 0);

            set_dma_addr(vic_data.t[0], 0);
            set_dma_addr(vic_data.t[1], 1);
            vic_data.dma_index = 0;

            vic_write_reg(DMA_RESET, 1);

            while (vic_read_reg(DMA_RESET));

            vic_set_bit(DMA_CONFIGURE, Dma_en, 1);

            printk(KERN_ERR "dma_state: %d %d %d %d %d\n", vic_frm_done, dma_state, vic_state, pixel_cnt, line_cnt);
        }

        vic_frm_done = 0;

        if (usable_frm) {
            add_to_usable_list(usable_frm);
            wake_up_all(&vic_data.waiter);
        }

        return IRQ_HANDLED;
    }

    if (get_bit_field(&irq_src, VIC_FRD)) {
        vic_frm_done = 1;
        vic_write_reg(ISP_IRQ_CNTCA, bit_field_val(VIC_FRD, 1));
        return IRQ_HANDLED;
    }

    printk(KERN_ERR "vic err:%lx\n", irq_src);

    vic_write_reg(ISP_IRQ_CNTCA, irq_src);

    return IRQ_HANDLED;
}

void vic_enable_sensor_mclk(unsigned long clk_rate)
{
    clk_set_rate(vic_data.mclk, clk_rate);
    clk_enable(vic_data.mclk);
}

void vic_disable_sensor_mclk(void)
{
    clk_disable(vic_data.mclk);
    assert(!clk_is_enabled(vic_data.mclk));
}

static inline void *m_dma_alloc_coherent(int size)
{
    if (rmem_size >= size && rmem_start)
        return (void *)CKSEG0ADDR(rmem_start);

    dma_addr_t dma_handle;
    void *mem = dma_alloc_coherent(NULL, size, &dma_handle, GFP_KERNEL);
    if (!mem)
        return mem;

    return (void *)CKSEG0ADDR(mem);
}

static inline void m_dma_free_coherent(void *mem, int size)
{
    dma_addr_t dma_handle = virt_to_phys(mem);
    if (dma_handle != rmem_start)
        dma_free_coherent(NULL, size, (void *)CKSEG1ADDR(mem), dma_handle);
}

static inline void m_cache_sync(void *mem, int size)
{
    dma_cache_sync(NULL, mem, size, DMA_FROM_DEVICE);
}

static int vic_stream_on(struct vic_sensor_config *cfg)
{
    assert_range(cfg->vic_interface, VIC_mipi_csi, VIC_dvp);

    if (cfg->isp_clk_rate)
        vic_data.isp_clk_rate = cfg->isp_clk_rate;

    clk_enable(vic_data.isp_clk);
    clk_set_rate(vic_data.cgu_isp_clk, vic_data.isp_clk_rate);
    clk_enable(vic_data.cgu_isp_clk);

    reset_frm_lists();

    if (cfg->vic_interface == VIC_dvp) {
        vic_dvp_init_setting(cfg);

    } else if (cfg->vic_interface == VIC_mipi_csi) {
        clk_enable(vic_data.csi_clk);
        if (vic_mipi_init_setting(cfg) < 0) {
            clk_disable(vic_data.isp_clk);
            clk_disable(vic_data.cgu_isp_clk);
            return -1;
        }
    }

    enable_irq(IRQ_ISP);

    return 0;
}

static void vic_stream_off(void)
{
    disable_irq(IRQ_ISP);

    vic_write_reg(VIC_CONTROL, bit_field_val(VIC_GLB_SAFE_RST, 1));
    usleep_range(1000, 1000);

    clk_disable(vic_data.isp_clk);
    clk_disable(vic_data.cgu_isp_clk);
}

static int vic_alloc_mem(struct vic_sensor_config *cfg)
{
    int mem_cnt = m_mem_cnt + 1;
    int frm_size, uv_data_offset;
    int line_length;
    int frame_align_size;

    assert(mem_cnt >= 2);

    if (camera_fmt_is_NV12(cfg->info.data_fmt) ) {
        line_length = ALIGN(cfg->info.width, 16);
        frm_size = line_length * cfg->info.height;
        if (frm_size % VIC_ALIGN_SIZE) {
            printk(KERN_ERR "frm_size not aligned\n");
            return -EINVAL;
        }
        uv_data_offset = frm_size;
        frm_size += frm_size / 2;

    } else if (is_output_y8(cfg) || camera_fmt_is_8BIT(cfg->info.data_fmt) ) {
        line_length = ALIGN(cfg->info.width, 8);
        frm_size = line_length * cfg->info.height;
        if (frm_size % VIC_ALIGN_SIZE) {
            printk(KERN_ERR "frm_size not aligned\n");
            return -EINVAL;
        }
        uv_data_offset = 0;
    } else {
        line_length = ALIGN(cfg->info.width, 8) * 2;
        frm_size = line_length * cfg->info.height;
        if (frm_size % VIC_ALIGN_SIZE) {
            printk(KERN_ERR "frm_size not aligned\n");
            return -EINVAL;
        }
        uv_data_offset = 0;
    }

    cfg->info.line_length = line_length;
    frame_align_size = ALIGN(frm_size, PAGE_SIZE);

    vic_data.uv_data_offset = uv_data_offset;

    vic_data.mem = m_dma_alloc_coherent(frame_align_size * mem_cnt);
    if (vic_data.mem == NULL) {
        printk(KERN_ERR "camera: failed to alloc mem: %u\n", frame_align_size * mem_cnt);
        return -ENOMEM;
    }

    vic_data.frm_size = frame_align_size;
    vic_data.mem_cnt = mem_cnt;
    cfg->info.fps = (cfg->sensor_info.fps >> 16) / (cfg->sensor_info.fps & 0xFFFF);
    cfg->info.frame_size = frm_size;
    cfg->info.frame_align_size = frame_align_size;
    cfg->info.frame_nums = mem_cnt;
    cfg->info.phys_mem = virt_to_phys(vic_data.mem);

    vic_data.frms = kmalloc(vic_data.mem_cnt * sizeof(vic_data.frms[0]), GFP_KERNEL);
    assert(vic_data.frms);

    init_frm_lists();

    return 0;
}

static void vic_free_mem(void)
{
    m_dma_free_coherent(vic_data.mem, vic_data.mem_cnt * vic_data.frm_size);
    kfree(vic_data.frms);
    vic_data.mem = NULL;
    vic_data.frms = NULL;
}

static int camera_power_on(void)
{
    int ret = 0;

    mutex_lock(&lock);

    if (!m_camera.is_power_on) {
        ret = m_camera.sensor->ops.power_on();
        if (!ret)
            m_camera.is_power_on = 1;
    }

    mutex_unlock(&lock);

    return ret;
}

static void camera_power_off(void)
{
    mutex_lock(&lock);

    if (m_camera.is_stream_on) {
        vic_stream_off();
        m_camera.sensor->ops.stream_off();
        m_camera.is_stream_on = 0;
    }

    if (m_camera.is_power_on) {
        m_camera.sensor->ops.power_off();
        m_camera.is_power_on = 0;
    }

    mutex_unlock(&lock);
}

static int camera_stream_on(void)
{
    int ret = 0;

    mutex_lock(&lock);

    if (!m_camera.is_power_on) {
        printk(KERN_ERR "camera: can't stream on when not power on\n");
        ret = -EINVAL;
        goto unlock;
    }

    if (m_camera.is_stream_on)
        goto unlock;

    if (m_camera.sensor->vic_interface == VIC_dvp) {

        ret = m_camera.sensor->ops.stream_on();
        if (ret)
            goto unlock;

        ret = vic_stream_on(m_camera.sensor);
        if (ret) {
            m_camera.sensor->ops.stream_off();
            goto unlock;
        }
    } else {

        ret = vic_stream_on(m_camera.sensor);
        if (ret)
            goto unlock;

        ret = m_camera.sensor->ops.stream_on();
        if (ret) {
            vic_stream_off();
            goto unlock;
        }
    }

    m_camera.is_stream_on = 1;

unlock:
    mutex_unlock(&lock);

    return ret;
}

static void camera_stream_off(void)
{
    mutex_lock(&lock);

    if (m_camera.is_stream_on) {
        vic_stream_off();
        m_camera.sensor->ops.stream_off();
        m_camera.is_stream_on = 0;
    }

    mutex_unlock(&lock);
}

static int camera_wait_frame(struct list_head *list, void **mem_p)
{
    struct frm_data *frm;
    void *mem = NULL;
    unsigned long flags;
    int ret;

    mutex_lock(&lock);

    spin_lock_irqsave(&spinlock, flags);
    frm = get_usable_frm();
    if (!frm) {
        spin_unlock_irqrestore(&spinlock, flags);
        ret = wait_event_interruptible_timeout(vic_data.waiter, vic_data.frame_counter, msecs_to_jiffies(3000));

        spin_lock_irqsave(&spinlock, flags);
        frm = get_usable_frm();
        if (ret <= 0 && !frm) {
            ret = -ETIMEDOUT;
            printk(KERN_ERR "camera: wait frame time out\n");
        }
    }
    spin_unlock_irqrestore(&spinlock, flags);

    if (frm) {
        ret = 0;
        frm->is_user = 1;
        mem = frm->address;
        list_add_tail(&frm->link, list);
        *mem_p = mem;
    }

    mutex_unlock(&lock);

    return ret;
}

static void camera_put_frame(void *mem)
{
    unsigned int size = mem - vic_data.mem;
    unsigned int index = size / vic_data.frm_size;
    struct frm_data *frm = &vic_data.frms[index];

    assert(!(size % vic_data.frm_size));
    assert(index < vic_data.mem_cnt);

    mutex_lock(&lock);

    unsigned long flags;

    spin_lock_irqsave(&spinlock, flags);

    if (frm->is_user != 1) {
        printk(KERN_ERR "camera: double free of vic frame %p\n", mem);
    } else {
        frm->is_user = 0;
        list_del(&frm->link);
        add_to_free_list(frm);
    }

    spin_unlock_irqrestore(&spinlock, flags);

    mutex_unlock(&lock);
}

static int check_frame_mem(void *mem, void *base)
{
    unsigned int size = mem - base;

    if (size % vic_data.frm_size)
        return -1;

    if (size / vic_data.frm_size >= vic_data.mem_cnt)
        return -1;

    return 0;
}

static unsigned int camera_get_available_frame_count(void)
{
    return vic_data.frame_counter;
}

static void camera_skip_frames(unsigned int frames)
{
    unsigned long flags;

    spin_lock_irqsave(&spinlock, flags);

    while (frames--) {
        struct frm_data *frm = get_usable_frm();
        if (frm == NULL)
            break;
        add_to_free_list(frm);
    }

    spin_unlock_irqrestore(&spinlock, flags);
}

static void camera_get_info(struct camera_info *info)
{
    mutex_unlock(&lock);

    *info = m_camera.sensor->info;

    mutex_unlock(&lock);
}

#define CMD_get_info                _IOWR('C', 120, struct camera_info)
#define CMD_power_on                _IO('C', 121)
#define CMD_power_off               _IO('C', 122)
#define CMD_stream_on               _IO('C', 123)
#define CMD_stream_off              _IO('C', 124)
#define CMD_wait_frame              _IO('C', 125)
#define CMD_put_frame               _IO('C', 126)
#define CMD_get_frame_count         _IO('C', 127)
#define CMD_skip_frames             _IO('C', 128)

struct m_private_data {
    struct list_head list;
    void *map_mem;
};

static long vic_ioctl(struct file *filp, unsigned int cmd, unsigned long arg)
{
    int ret;
    struct m_private_data *data = filp->private_data;
    void *map_mem = data->map_mem;

    switch (cmd) {
    case CMD_get_info:{
        struct camera_info *info = (void *)arg;
        if (!info)
            return -EINVAL;
        camera_get_info(info);
        return 0;
    }

    case CMD_power_on:
        return camera_power_on();

    case CMD_power_off:
        camera_power_off();
        return 0;

    case CMD_stream_on:
        return camera_stream_on();

    case CMD_stream_off:
        camera_stream_off();
        return 0;

    case CMD_wait_frame: {
        struct list_head *list = &data->list;
        void **mem_p = (void *)arg;
        if (!mem_p)
            return -EINVAL;

        if (!map_mem) {
            printk(KERN_ERR "camera: please mmap first\n");
            return -EINVAL;
        }

        void *mem;
        ret = camera_wait_frame(list, &mem);
        if (ret)
            return ret;

        mem = map_mem + (mem - vic_data.mem);
        m_cache_sync(mem, vic_data.frm_size);
        *mem_p = mem;
        return 0;
    }

    case CMD_put_frame: {
        void *mem = (void *)arg;
        if (!mem)
            return -EINVAL;
        if (check_frame_mem(mem, map_mem))
            return -EINVAL;

        m_cache_sync(mem, vic_data.frm_size);
        mem = vic_data.mem + (mem - map_mem);
        camera_put_frame(mem);
        return 0;
    }

    case CMD_get_frame_count: {
        unsigned int *count_p = (void *)arg;
        if (!count_p)
            return -EINVAL;
        *count_p = camera_get_available_frame_count();
        return 0;
    }

    case CMD_skip_frames: {
        unsigned int frames = arg;
        camera_skip_frames(frames);
        return 0;
    }

    default:
        printk(KERN_ERR "camera: %x not support this cmd\n", cmd);
        return -EINVAL;
    }

    return 0;
}

static int vic_mmap(struct file *file, struct vm_area_struct *vma)
{
    unsigned long start;
    unsigned long off;
    u32 len;

    off = vma->vm_pgoff << PAGE_SHIFT;

    if (off) {
        printk(KERN_ERR "camera: off must be 0\n");
        return -EINVAL;
    }

    len = vic_data.frm_size * vic_data.mem_cnt;
    if ((vma->vm_end - vma->vm_start) != len) {
        printk(KERN_ERR "camera: size must be total size\n");
        return -EINVAL;
    }

    /* frame buffer memory */
    start = virt_to_phys(vic_data.mem);
    off += start;

    vma->vm_pgoff = off >> PAGE_SHIFT;
    vma->vm_flags |= VM_IO;

    /* 0: cachable,write through (cache + cacheline对齐写穿)
     * 1: uncachable,write Acceleration (uncache + 硬件写加速)
     * 2: uncachable
     * 3: cachable
     */
    pgprot_val(vma->vm_page_prot) &= ~_CACHE_MASK;
    pgprot_val(vma->vm_page_prot) |= _CACHE_CACHABLE_NO_WA;

    if (io_remap_pfn_range(vma, vma->vm_start, off >> PAGE_SHIFT,
                           vma->vm_end - vma->vm_start, vma->vm_page_prot))
    {
        return -EAGAIN;
    }

    struct m_private_data *data = file->private_data;
    data->map_mem = (void *)vma->vm_start;

    return 0;
}

static int vic_open(struct inode *inode, struct file *filp)
{
    struct m_private_data *data = kmalloc(sizeof(*data), GFP_KERNEL);

    INIT_LIST_HEAD(&data->list);
    filp->private_data = data;
    return 0;
}

static int vic_release(struct inode *inode, struct file *filp)
{
    unsigned long flags;
    struct list_head *pos, *n;
    struct m_private_data *data = filp->private_data;
    struct list_head *list = &data->list;

    mutex_lock(&lock);

    spin_lock_irqsave(&spinlock, flags);
    list_for_each_safe(pos, n, list) {
        list_del(pos);
        struct frm_data *frm = list_entry(pos, struct frm_data, link);
        add_to_free_list(frm);
    }
    spin_unlock_irqrestore(&spinlock, flags);

    mutex_unlock(&lock);

    kfree(data);

    return 0;
}

static struct file_operations vic_misc_fops = {
    .open = vic_open,
    .release = vic_release,
    .mmap = vic_mmap,
    .unlocked_ioctl = vic_ioctl,
};

static struct miscdevice vic_mdev = {
    .minor = MISC_DYNAMIC_MINOR,
    .name = "camera",
    .fops = &vic_misc_fops,
};

/*
 * 格式信息转换
 * sensor format : Sensor格式在sensor driver中根据setting指定
 * camera format : Camera格式在camera driver中使用,并暴露给应用
 *
 *    sensor格式                    Camera 格式                  Camera格式(VIC扩展为16bit)
 * [成员0 sensor format]  <--->  [成员1 camera format]  <--->  [成员2 camera format]
 *
 */
struct fmt_pair {
    sensor_pixel_fmt sensor_fmt;
    camera_pixel_fmt camera_fmt;
};

static struct fmt_pair fmts[] = {
    {SENSOR_PIXEL_FMT_Y8_1X8,           CAMERA_PIX_FMT_GREY},
    {SENSOR_PIXEL_FMT_UYVY8_2X8,        CAMERA_PIX_FMT_UYVY},
    {SENSOR_PIXEL_FMT_VYUY8_2X8,        CAMERA_PIX_FMT_VYUY},
    {SENSOR_PIXEL_FMT_YUYV8_2X8,        CAMERA_PIX_FMT_YUYV},
    {SENSOR_PIXEL_FMT_YVYU8_2X8,        CAMERA_PIX_FMT_YVYU},

    {SENSOR_PIXEL_FMT_SBGGR8_1X8,       CAMERA_PIX_FMT_SBGGR8},
    {SENSOR_PIXEL_FMT_SGBRG8_1X8,       CAMERA_PIX_FMT_SGBRG8},
    {SENSOR_PIXEL_FMT_SGRBG8_1X8,       CAMERA_PIX_FMT_SGRBG8},
    {SENSOR_PIXEL_FMT_SRGGB8_1X8,       CAMERA_PIX_FMT_SRGGB8},
    {SENSOR_PIXEL_FMT_SBGGR10_1X10,     CAMERA_PIX_FMT_SBGGR16},
    {SENSOR_PIXEL_FMT_SGBRG10_1X10,     CAMERA_PIX_FMT_SGBRG16},
    {SENSOR_PIXEL_FMT_SGRBG10_1X10,     CAMERA_PIX_FMT_SGRBG16},
    {SENSOR_PIXEL_FMT_SRGGB10_1X10,     CAMERA_PIX_FMT_SRGGB16},
    {SENSOR_PIXEL_FMT_SBGGR12_1X12,     CAMERA_PIX_FMT_SBGGR16},
    {SENSOR_PIXEL_FMT_SGBRG12_1X12,     CAMERA_PIX_FMT_SGBRG16},
    {SENSOR_PIXEL_FMT_SGRBG12_1X12,     CAMERA_PIX_FMT_SGRBG16},
    {SENSOR_PIXEL_FMT_SRGGB12_1X12,     CAMERA_PIX_FMT_SRGGB16},
};

#define error_if(_cond) \
    do { \
        if (_cond) { \
            printk(KERN_ERR "camera: failed to check: %s\n", #_cond); \
            ret = -1; \
            goto unlock; \
        } \
    } while (0)

static int sensor_attribute_check_init(struct vic_sensor_config *sensor)
{
    int ret = -1;

    error_if (m_camera.sensor);
    error_if(!sensor->device_name);
    error_if(sensor->sensor_info.width < 128 || sensor->sensor_info.width > 2048);
    error_if(sensor->sensor_info.height < 128 || sensor->sensor_info.height > 2048);
    error_if(!sensor->ops.power_on);
    error_if(!sensor->ops.power_off);
    error_if(!sensor->ops.stream_on);
    error_if(!sensor->ops.stream_off);

    memset(sensor->info.name, 0x00, sizeof(sensor->info.name));
    memcpy(sensor->info.name, sensor->device_name, strlen(sensor->device_name));

    sensor->info.width =  sensor->sensor_info.width;
    sensor->info.height =  sensor->sensor_info.height;
    sensor->info.data_fmt = 0;

    int i = 0;
    int size = ARRAY_SIZE(fmts);
    for (i = 0; i < size; i++) {
        if (sensor->sensor_info.fmt == fmts[i].sensor_fmt)
            break;
    }

    if (i >= size) {
        printk(KERN_ERR "sensor format(0x%x) is not Support\n", sensor->sensor_info.fmt);
        goto unlock;
    }

    /* 当sensor输出为8BIT时, 其他控制条件必须同时满足为8bit */
    if (sensor_fmt_is_8BIT(sensor->sensor_info.fmt) && !is_output_y8(sensor)) {
        if (sensor->vic_interface == VIC_dvp) {
            printk(KERN_ERR "Please check sensor format and VIC data format\n");
            printk(KERN_ERR "sensor format is 8BIT(0x%x)\n", sensor->sensor_info.fmt);
            printk(KERN_ERR "VIC inteface(DVP) data format is not 8BIT(0x%x)\n", sensor->dvp_cfg_info.dvp_data_fmt);
            goto unlock;
        }

        if (sensor->vic_interface == VIC_mipi_csi) {
            printk(KERN_ERR "Please check sensor format and VIC data format\n");
            printk(KERN_ERR "sensor format is 8BIT(0x%x)\n", sensor->sensor_info.fmt);
            printk(KERN_ERR "VIC inteface(MIPI) data format is not 8BIT(0x%x)\n", sensor->mipi_cfg_info.data_fmt);
            goto unlock;
        }

        printk(KERN_ERR "Now dbus type(%d) not support\n", sensor->vic_interface);
        goto unlock;
    }

    camera_pixel_fmt data_fmt = fmts[i].camera_fmt;

    /* 当sensor输出格式为YUV422时,可通过VIC DMA重新排列输出格式 */
    if (is_output_yuv422(sensor) && camera_fmt_is_YUV422(data_fmt)) {
        switch (sensor->dma_mode) {
        case SENSOR_DATA_DMA_MODE_NV12:
            data_fmt = CAMERA_PIX_FMT_NV12;
            break;
        case SENSOR_DATA_DMA_MODE_NV21:
            data_fmt = CAMERA_PIX_FMT_NV21;
            break;
        case SENSOR_DATA_DMA_MODE_GREY:
            data_fmt = CAMERA_PIX_FMT_GREY;
            break;
        default:
            break;
        }
    }

    sensor->info.data_fmt = data_fmt;
    return 0;

unlock:
    return ret;
}

int vic_register_sensor(struct vic_sensor_config *sensor)
{
    int ret = 0;

    mutex_lock(&lock);

    ret = sensor_attribute_check_init(sensor);
    assert(ret == 0);

    ret = vic_alloc_mem(sensor);
    if (ret)
        goto unlock;

    ret = misc_register(&vic_mdev);
    assert(!ret);

    m_camera.sensor = sensor;
    m_camera.is_power_on = 0;
    m_camera.is_stream_on = 0;

unlock:
    mutex_unlock(&lock);
    return ret;
}

void vic_unregister_sensor(struct vic_sensor_config *sensor)
{
    int ret;

    mutex_lock(&lock);

    assert(m_camera.sensor);
    assert(sensor == m_camera.sensor);

    ret = misc_deregister(&vic_mdev);
    assert(!ret);

    if (m_camera.is_power_on)
        vic_stream_off();

    vic_free_mem();

    m_camera.sensor = NULL;

    mutex_unlock(&lock);
}

static int vic_init(void)
{
    if (m_mem_cnt < 1) {
        m_mem_cnt = 2;
        printk(KERN_ERR "camera: mem cnt fix to 2\n");
    }

    if (dvp_init_mclk_gpio())
        return -1;

    vic_data.mclk = clk_get(NULL, "cgu_cim");
    assert(!IS_ERR(vic_data.mclk));

    vic_data.isp_clk = clk_get(NULL, "isp");
    assert(!IS_ERR(vic_data.isp_clk));

    vic_data.cgu_isp_clk = clk_get(NULL, "cgu_isp");
    assert(!IS_ERR(vic_data.cgu_isp_clk));

    vic_data.csi_clk = clk_get(NULL, "csi");
    assert(!IS_ERR(vic_data.csi_clk));

    vic_data.isp_clk_rate = 90 * 1000 * 1000;

    init_waitqueue_head(&vic_data.waiter);

    int ret = request_irq(IRQ_ISP, vic_irq_handler, 0, "vic", NULL);
    if (ret) {
        printk(KERN_ERR "camera: failed to request irq\n");
        clk_put(vic_data.mclk);
        clk_put(vic_data.isp_clk);
        clk_put(vic_data.cgu_isp_clk);
        clk_put(vic_data.csi_clk);

        dvp_deinit_mclk_gpio();
        return ret;
    }

    disable_irq(IRQ_ISP);

    return 0;
}

static void vic_exit(void)
{
    free_irq(IRQ_ISP, NULL);

    clk_put(vic_data.mclk);
    clk_put(vic_data.isp_clk);
    clk_put(vic_data.cgu_isp_clk);
    clk_put(vic_data.csi_clk);

    dvp_deinit_mclk_gpio();
}

module_init(vic_init);

module_exit(vic_exit);

EXPORT_SYMBOL(vic_register_sensor);
EXPORT_SYMBOL(vic_unregister_sensor);

EXPORT_SYMBOL(vic_enable_sensor_mclk);
EXPORT_SYMBOL(dvp_init_low10bit_gpio);
EXPORT_SYMBOL(vic_disable_sensor_mclk);
EXPORT_SYMBOL(dvp_deinit_gpio);

MODULE_DESCRIPTION("JZ x1830 vic driver");
MODULE_LICENSE("GPL");
