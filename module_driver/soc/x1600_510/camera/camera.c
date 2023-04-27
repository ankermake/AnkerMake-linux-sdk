#include <common.h>
#include <linux/list.h>
#include <linux/clk.h>
#include <linux/miscdevice.h>
#include <linux/gpio.h>
#include <linux/module.h>
#include <linux/ioctl.h>
#include <linux/interrupt.h>
#include <linux/slab.h>
#include <linux/dma-mapping.h>
#include <linux/sched.h>
#include <bit_field.h>
#include <linux/platform_device.h>

#include "camera_sensor.h"
#include "camera_gpio.h"
#include "cim_regs.h"
#include "csi.h"
#include "drivers/rmem_manager/rmem_manager.h"

#define CAMERA_DMA_DESC_COUNT           2  /* 不修正为其他值 */
#define CAMERA_LUMINANCE_SIZE           (9 * 4)
#define MEM_ALIGN(x, n)                 (((x) + (n) - 1) - ((x) + (n) - 1) % (n))
/*
 * DMA description 描述
 */
typedef union desc_intc {
    unsigned int d32;
    struct {
        unsigned int reserved0:1;
        unsigned int eof_mask:1;
        unsigned int sof_mask:1;
        unsigned int reserved3_31:29;
    } data;
} desc_intc_t;

typedef union desc_cfg {
    unsigned int d32;
    struct {
        unsigned int desc_end:1;    /* =0: not the last one
                                     * =1: this desc is the last one of the chain */
        unsigned int reserved1_15:15;
        unsigned int write_back_format:3;
        unsigned int reserved19_25:7;
        unsigned int id:6;
    } data;
} desc_cfg_t;

typedef union desc_lumi_cfg {
    unsigned int d32;
    struct {
        unsigned int reserved0_29:30;
        unsigned int lumi_en:1;
        unsigned int reserved31:1;
    } data;
} desc_lumi_cfg_t;

struct frame_desc {
    unsigned long next_desc_addr;
    unsigned long write_back_address;  /* pixel size aligned */
    unsigned long write_back_stride;
    desc_intc_t desc_intc_t;
    desc_cfg_t  desc_cfg_t;
    desc_lumi_cfg_t desc_lumi_cfg_t;
    unsigned long lumi_write_back_addr;  /* address 4Byte aligned */
    unsigned long sf_write_back_addr;
} __attribute__ ((aligned (8)));

struct lumi_area {
    int enable;

    /* area offset point1 */
    int x1;
    int y1;

    /* area offset point2 */
    int x2;
    int y2;

    /* 存放luminance占空间大小 */
    int size;
};


struct jz_cim_data {
    struct device *dev;
    int is_enable;
    int is_finish;
    int mclk_io; /* MCLK输出管脚选择: PA24(频率可调整) / PC25(频率不可调整24M) */

    int irq;
    const char *irq_name;

    struct mutex lock;
    struct spinlock spinlock;

    struct clk *cim_gate_clk;
    const char *cim_gate_clk_name;  /* GATE_CIM */
    struct clk *cim_mclk;
    const char *cim_mclk_name;      /* MCLK */

    wait_queue_head_t waiter;

    void *mem;
    unsigned int mem_cnt;   /* 申请循环buff个数 */
    unsigned int frm_size;  /* frame + luminance 申请空间(对齐)大小 */
    unsigned int frame_align_size;  /* frame 申请空间(对齐)大小,仅用作驱动DMA地址的偏移和raw10/raw12软件扩展大小 */
    struct frame_desc *frame_descs;

    volatile unsigned int frame_counter;
    volatile unsigned int free_frame_counter;
    unsigned int wait_timeout;

    /* Camera Device */
    struct lumi_area lumi_info;
    int color_in_order;      /* 控制器接收的像素数据是否和sensor发送的一致，主要是raw10/raw12扩展为raw16=0.顺序不一直 其他顺序一致=1 */
    char *device_name;       /* 设备节点名字 */
    unsigned int cam_mem_cnt;/* 应用传递循环buff个数 */
    struct camera_device camera;
    struct miscdevice cim_mdev;

    struct list_head free_list;
    struct list_head usable_list;
    struct frame_data *frames;
    struct frame_data **trans_list;

    unsigned int cim_frm_done;
};

static int is_frame_size_check = 1;
static int is_enable_snapshot = 0;

static struct jz_cim_data jz_cim_dev = {
    .is_enable              = 1,
    .mclk_io                = -1,
    .irq                    = IRQ_INTC_BASE + IRQ_CIM, /* BASE + 30 */
    .irq_name               = "CIM",
    .cim_mclk_name          = "div_cim",
    .cim_gate_clk_name      = "gate_cim",
    .cam_mem_cnt            = 2,
    .device_name            = "camera",
};

module_param_named(frame_nums,      jz_cim_dev.cam_mem_cnt,  int, 0644);
module_param_gpio_named(mclk_io,    jz_cim_dev.mclk_io, 0644);


#define CAMERA_ADDR(reg)                ((volatile unsigned long *)CKSEG1ADDR(CIM_IOBASE + reg))

static inline void cim_write_reg(unsigned int reg, unsigned int val)
{
    *CAMERA_ADDR(reg) = val;
}

static inline unsigned int cim_read_reg(unsigned int reg)
{
    return *CAMERA_ADDR(reg);
}

static inline void cim_set_bit(unsigned int reg, unsigned int start, unsigned int end, unsigned int val)
{
    set_bit_field(CAMERA_ADDR(reg), start, end, val);
}

static inline unsigned int cim_get_bit(unsigned int reg, unsigned int start, unsigned int end)
{
    return get_bit_field(CAMERA_ADDR(reg), start, end);
}

static inline void cim_dump_regs(void)
{
    printk(KERN_ERR "Dump CIM regs\n");

    printk(KERN_ERR "GLB_CFG                  :0x%08x\n", cim_read_reg(CIM_GLB_CFG));
    printk(KERN_ERR "CROP_SIZE                :0x%08x\n", cim_read_reg(CIM_CROP_SIZE));
    printk(KERN_ERR "CROP_SITE                :0x%08x\n", cim_read_reg(CIM_CROP_SITE));
    printk(KERN_ERR "SCAN_CFG                 :0x%08x\n", cim_read_reg(CIM_SCAN_CFG));
    printk(KERN_ERR "DLY_CFG                  :0x%08x\n", cim_read_reg(CIM_DLY_CFG));
    printk(KERN_ERR "QOS_CTRL                 :0x%08x\n", cim_read_reg(CIM_QOS_CTRL));
    printk(KERN_ERR "QOS_CFG                  :0x%08x\n", cim_read_reg(CIM_QOS_CFG));
    printk(KERN_ERR "LUMI_POINT_1             :0x%08x\n", cim_read_reg(CIM_LUMI_POINT_1));
    printk(KERN_ERR "LUMI_POINT_2             :0x%08x\n", cim_read_reg(CIM_LUMI_POINT_2));
    printk(KERN_ERR "RGB_COEF                 :0x%08x\n", cim_read_reg(CIM_RGB_COEF));
    printk(KERN_ERR "RGB_BIAS                 :0x%08x\n", cim_read_reg(CIM_RGB_BIAS));
    printk(KERN_ERR "DES_ADDR                 :0x%08x\n", cim_read_reg(CIM_DES_ADDR));
    printk(KERN_ERR "CTRL                     :0x%08x\n", cim_read_reg(CIM_CTRL));
    printk(KERN_ERR "ST                       :0x%08x\n", cim_read_reg(CIM_ST));
    printk(KERN_ERR "CLR_ST                   :0x%08x\n", cim_read_reg(CIM_CLR_ST));
    printk(KERN_ERR "INTC                     :0x%08x\n", cim_read_reg(CIM_INTC));
    printk(KERN_ERR "INT_FLAG                 :0x%08x\n", cim_read_reg(CIM_INT_FLAG));
    printk(KERN_ERR "FRM_ID                   :0x%08x\n", cim_read_reg(CIM_FRM_ID));
    printk(KERN_ERR "ACT_SIZE                 :0x%08x\n", cim_read_reg(CIM_ACT_SIZE));
    printk(KERN_ERR "DBG_CGC                  :0x%08x\n", cim_read_reg(CIM_DBG_CGC));
}

static inline void cim_dump_debug_desc(void)
{
    printk(KERN_ERR "Dump CIM debug DMA desc\n");
    printk(KERN_ERR "DBG next_desc_addr       :0x%08x\n", cim_read_reg(CIM_DBG_DES));
    printk(KERN_ERR "DBG write_back_address   :0x%08x\n", cim_read_reg(CIM_DBG_DES));
    printk(KERN_ERR "DBG write_back_stride    :0x%08x\n", cim_read_reg(CIM_DBG_DES));
    printk(KERN_ERR "DBG desc_intc_t          :0x%08x\n", cim_read_reg(CIM_DBG_DES));
    printk(KERN_ERR "DBG desc_cfg_t           :0x%08x\n", cim_read_reg(CIM_DBG_DES));
    printk(KERN_ERR "DBG desc_hist_cfg_t      :0x%08x\n", cim_read_reg(CIM_DBG_DES));
    printk(KERN_ERR "hist_write_back_addr     :0x%08x\n", cim_read_reg(CIM_DBG_DES));
    printk(KERN_ERR "sf_write_back_addr       :0x%08x\n", cim_read_reg(CIM_DBG_DES));
    printk(KERN_ERR "DBG current DMA addr     :0x%08x\n", cim_read_reg(CIM_DBG_DMA));
}

static inline void cim_dump_desc(void)
{
    struct jz_cim_data *drv = &jz_cim_dev;
    struct frame_desc *descs = drv->frame_descs;
    int mem_cnt = CAMERA_DMA_DESC_COUNT;

    printk(KERN_ERR "Dump CIM DMA desc\n");

    int i = 0;
    for (i = 0; i < mem_cnt; i++) {
        printk(KERN_ERR "desc index = %d\n", i);
        printk(KERN_ERR "next_desc_addr       :0x%08lx\n", descs[i].next_desc_addr);
        printk(KERN_ERR "write_back_address   :0x%08lx\n", descs[i].write_back_address);
        printk(KERN_ERR "write_back_stride    :0x%08lx\n", descs[i].write_back_stride);
        printk(KERN_ERR "desc_intc_t          :0x%08x\n", descs[i].desc_intc_t.d32);
        printk(KERN_ERR "desc_cfg_t           :0x%08x\n", descs[i].desc_cfg_t.d32);
        printk(KERN_ERR "desc_lumi_cfg_t      :0x%08x\n", descs[i].desc_lumi_cfg_t.d32);
        printk(KERN_ERR "lumi_write_back_addr :0x%08lx\n", descs[i].lumi_write_back_addr);
        printk(KERN_ERR "sf_write_back_addr   :0x%08lx\n", descs[i].sf_write_back_addr);
        printk(KERN_ERR "\n");
    }
}

static void add_to_free_list(struct frame_data *frm)
{
    struct jz_cim_data *drv = &jz_cim_dev;

    frm->status = frame_status_free;
    drv->free_frame_counter++;
    list_add_tail(&frm->link, &drv->free_list);
}

static struct frame_data *get_free_frm(void)
{
    struct jz_cim_data *drv = &jz_cim_dev;

    if (list_empty(&drv->free_list))
        return NULL;

    drv->free_frame_counter--;
    struct frame_data *frm = list_first_entry(&drv->free_list, struct frame_data, link);
    list_del(&frm->link);

    return frm;
}

static void add_to_usable_list(struct frame_data *frm)
{
    struct jz_cim_data *drv = &jz_cim_dev;

    frm->status = frame_status_usable;
    drv->frame_counter++;
    list_add_tail(&frm->link, &drv->usable_list);
}

static struct frame_data *get_usable_frm(void)
{
    struct jz_cim_data *drv = &jz_cim_dev;

    if (list_empty(&drv->usable_list))
        return NULL;

    drv->frame_counter--;
    struct frame_data *frm = list_first_entry(&drv->usable_list, struct frame_data, link);
    list_del(&frm->link);

    return frm;
}

static void set_desc_addr(int id, struct frame_data *frm)
{
    struct jz_cim_data *drv = &jz_cim_dev;
    struct frame_desc *desc = &drv->frame_descs[id];

    desc->write_back_address = virt_to_phys(frm->addr);
    desc->lumi_write_back_addr = desc->write_back_address + drv->frame_align_size;
    dma_cache_wback((unsigned long)desc, sizeof(*desc));
}

static void set_transfer_frame(int index, struct frame_data *frm)
{
    struct jz_cim_data *drv = &jz_cim_dev;

    frm->status = frame_status_trans;
    drv->trans_list[index] = frm;
    set_desc_addr(index, frm);
}


static void init_frm_lists(void)
{
    struct jz_cim_data *drv = &jz_cim_dev;

    INIT_LIST_HEAD(&drv->free_list);
    INIT_LIST_HEAD(&drv->usable_list);
    drv->frame_counter = 0;
    drv->free_frame_counter = 0;
    drv->wait_timeout = 0;

    int i;
    for (i = 0; i < drv->mem_cnt; i++) {
        struct frame_data *frm = &drv->frames[i];
        frm->addr = drv->mem + i * drv->frm_size;

        frm->info.index = i;
        frm->info.width = drv->camera.sensor->info.width;
        frm->info.height = drv->camera.sensor->info.height;
        frm->info.pixfmt = drv->camera.sensor->info.data_fmt;
        frm->info.size = drv->frm_size;
        frm->info.paddr = virt_to_phys(frm->addr);
        add_to_free_list(frm);
    }
}

static void reset_frm_lists(void)
{
    struct jz_cim_data *drv = &jz_cim_dev;
    int i;

    drv->frame_counter = 0;
    drv->free_frame_counter = 0;
    drv->wait_timeout = 0;

    for (i = 0; i < drv->mem_cnt; i++) {
        struct frame_data *frm = &drv->frames[i];
        if (frm->status != frame_status_trans)
            list_del(&frm->link);

        add_to_free_list(frm);
    }
}

static void cim_set_luminance_area_point1(int x, int y)
{
    cim_set_bit(CIM_LUMI_POINT_1, CIM_LUMI_point_1_x, x);
    cim_set_bit(CIM_LUMI_POINT_1, CIM_LUMI_point_1_y, y);
}

static void cim_set_luminance_area_point2(int x, int y)
{
    cim_set_bit(CIM_LUMI_POINT_2, CIM_LUMI_point_2_x, x);
    cim_set_bit(CIM_LUMI_POINT_2, CIM_LUMI_point_2_y, y);
}

static void cim_start(void)
{
    cim_set_bit(CIM_CTRL, CTRL_START, 1);
}

static void cim_soft_reset(void)
{
    /* reset cim 控制器 */
    cim_set_bit(CIM_CTRL, CTRL_SOFT_RESET, 1);

    int timeout = 3000;
    while (!cim_get_bit(CIM_ST, ST_SRA)) {
        udelay(100);
        if (--timeout == 0) {
            printk(KERN_ERR "cim reset wait finish timeout: %x\n", cim_read_reg(CIM_ST));
            break;
        }
    }

    cim_set_bit(CIM_CLR_ST, CLR_ST_SOFT_RESET, 1);
}

static void cim_clear_status_all(void)
{
    /* 清除所有状态标志 */
    cim_write_reg(CIM_CLR_ST, 0xFF);
}

static void cim_set_dma_desc_addr(int index)
{
    struct jz_cim_data *drv = &jz_cim_dev;
    unsigned long des_addr_phy = virt_to_phys(&drv->frame_descs[index]);

    cim_write_reg(CIM_DES_ADDR, des_addr_phy);
}

static void cim_init_dma_desc(void)
{
    struct jz_cim_data *drv = &jz_cim_dev;
    struct frame_data *frm;

    frm = get_free_frm();
    set_transfer_frame(0, frm);

    frm = get_free_frm();
    if (!frm)
        frm = drv->trans_list[0];
    set_transfer_frame(1, frm);
}



static irqreturn_t cim_irq_handler(int irq, void *data)
{
    struct jz_cim_data *drv = (struct jz_cim_data *)data;
    unsigned long status;

    status = cim_read_reg(CIM_INT_FLAG);

    if (get_bit_field(&status, INT_FLAG_EOW) || get_bit_field(&status, INT_FLAG_EOF)) {
        cim_set_bit(CIM_CLR_ST, CLR_ST_EOW, 1);
        cim_set_bit(CIM_CLR_ST, CLR_ST_EOF, 1);

        struct frame_data *frm;
        struct frame_data *usable_frm = NULL;
        int index = cim_read_reg(CIM_FRM_ID); /* 当前已传输完成帧 */
        int id = !index;/* 正在接收的帧 */

        drv->cim_frm_done++;

        if (drv->trans_list[0] != drv->trans_list[1]) {
            usable_frm = drv->trans_list[index];
        }

        /*
         * 1. 优先从空闲列表中获取新的帧, 加入到接收列表中
         * 2. 如果空闲列表没有帧, 则从传输完成列表中获取
         * 3. 如果传输完成列表也没有帧,就用当前传输帧作为保底
         */
        frm = get_free_frm();
        if (!frm)
            frm =  get_usable_frm();

        if (!frm)
            frm =  drv->trans_list[id];

        set_transfer_frame(index, frm);

        if (usable_frm) {
            usable_frm->info.sequence = drv->cim_frm_done;
            usable_frm->info.timestamp = get_time_us();

            add_to_usable_list(usable_frm);
            wake_up_all(&drv->waiter);
        }

        /* re-start next dma frame. */
        if (get_bit_field(&status, INT_FLAG_EOW)) {
            cim_set_dma_desc_addr(id);
            cim_start();
        }
        goto out;
    }

    if (get_bit_field(&status, INT_FLAG_SZ_ERR)) {
        cim_set_bit(CIM_CLR_ST, CLR_ST_SIZE_ERR, 1);

        unsigned int active_val = cim_read_reg(CIM_ACT_SIZE);
        unsigned int width = active_val & 0x3ff;
        unsigned int height = (active_val >> 16) & 0x3ff;
        printk(KERN_ERR "cim size err width=%d height=%d\n", width, height);
        goto out;
    }

    if (get_bit_field(&status, INT_FLAG_OVER)) {
        cim_set_bit(CIM_CLR_ST, CLR_ST_OVER, 1);
        printk(KERN_ERR "cim overflow\n");

        cim_dump_debug_desc();
        cim_dump_regs();
        goto out;
    }

    if (get_bit_field(&status, INT_FLAG_GSA)) {
        cim_set_bit(CIM_CLR_ST, CLR_ST_GSA, 1);
        printk(KERN_ERR "cim general stop\n");
        goto out;
    }

    if (get_bit_field(&status, INT_FLAG_SOF)) {
        cim_set_bit(CIM_CLR_ST, CLR_ST_SOF, 1);
        //printk(KERN_ERR "cim start of frame\n");
        goto out;
    }

    printk(KERN_ERR "can not handler rawRGB errror status=0x%lx\n", status);
    cim_write_reg(CIM_CLR_ST, status);
out:
    return IRQ_HANDLED;
}

static int init_frame_desc(struct frame_desc *desc, int index, void *addr,
        struct frame_desc *next, struct sensor_attr *sensor)
{
    struct jz_cim_data *drv = &jz_cim_dev;
    int pixel_fmt = sensor->info.data_fmt;
    int dma_output_y_mode = sensor->dma_mode;

    switch (pixel_fmt) {
    case CAMERA_PIX_FMT_GREY:
    case CAMERA_PIX_FMT_SBGGR8:
    case CAMERA_PIX_FMT_SGBRG8:
    case CAMERA_PIX_FMT_SGRBG8:
    case CAMERA_PIX_FMT_SRGGB8:
        if (dma_output_y_mode == SENSOR_DATA_DMA_MODE_GREY)
            desc->desc_cfg_t.data.write_back_format = FRM_WRBK_FMT_Y8;
        else
            desc->desc_cfg_t.data.write_back_format = FRM_WRBK_FMT_MONO_RAWRGB;
        break;

    case CAMERA_PIX_FMT_SBGGR16:
    case CAMERA_PIX_FMT_SGBRG16:
    case CAMERA_PIX_FMT_SGRBG16:
    case CAMERA_PIX_FMT_SRGGB16:
        /* 不支持Y8提取 */
        desc->desc_cfg_t.data.write_back_format = FRM_WRBK_FMT_MONO_RAWRGB;
        break;

    case CAMERA_PIX_FMT_UYVY:
    case CAMERA_PIX_FMT_VYUY:
    case CAMERA_PIX_FMT_YUYV:
    case CAMERA_PIX_FMT_YVYU:
        if (dma_output_y_mode == SENSOR_DATA_DMA_MODE_GREY)
            desc->desc_cfg_t.data.write_back_format = FRM_WRBK_FMT_Y8;
        else
            desc->desc_cfg_t.data.write_back_format = FRM_WRBK_FMT_YUV422;
        break;

    case CAMERA_PIX_FMT_RGB565:
        if (dma_output_y_mode == SENSOR_DATA_DMA_MODE_GREY)
            desc->desc_cfg_t.data.write_back_format = FRM_WRBK_FMT_Y8;
        else
            desc->desc_cfg_t.data.write_back_format = FRM_WRBK_FMT_RGB565;
        break;

    case CAMERA_PIX_FMT_RGB24:
    case CAMERA_PIX_FMT_RBG24:
    case CAMERA_PIX_FMT_BGR24:
    case CAMERA_PIX_FMT_GBR24:
        if (dma_output_y_mode == SENSOR_DATA_DMA_MODE_GREY)
            desc->desc_cfg_t.data.write_back_format = FRM_WRBK_FMT_Y8;
        else
            desc->desc_cfg_t.data.write_back_format = FRM_WRBK_FMT_RGB888;
        break;

    default:
        printk(KERN_ERR "cim NOT support format %x\n", pixel_fmt);
        return -EINVAL;
    }

    desc->desc_cfg_t.data.id = index;
    desc->desc_cfg_t.data.desc_end = 0; /* =0:countinue, not the last one */
    desc->desc_intc_t.data.eof_mask = 1;/* =1:end of frame generate interrupt */
    desc->desc_intc_t.data.sof_mask = 0;/* =1:start of frame generate interrupt， =0:disable */

    desc->next_desc_addr = virt_to_phys(next);
    desc->write_back_address = virt_to_phys(addr);

    /* luminance disable */
    if (!drv->lumi_info.enable) {
        desc->desc_lumi_cfg_t.d32 = 0;
        desc->lumi_write_back_addr = 0;
    } else {
        desc->desc_lumi_cfg_t.data.lumi_en = 1;
        desc->lumi_write_back_addr = desc->write_back_address + drv->frame_align_size;
    }

#if 0
    /* (调试ITU模式时验证) ITU656 interlace mode */
    desc->write_back_stride = sensor->info.line_length * 2;
    desc->sf_write_back_addr = desc->write_back_address + desc->write_back_stride;
#else
    /* progressive mode */
    desc->write_back_stride = sensor->info.line_length;
    desc->sf_write_back_addr = 0;
#endif

    return 0;
}

static inline void *m_dma_alloc_coherent(struct device *dev, int size)
{
    dma_addr_t dma_handle;
    void *mem = dma_alloc_coherent(dev, size, &dma_handle, GFP_KERNEL);
    if (!mem)
        return mem;

    return (void *)CKSEG0ADDR(mem);
}

static inline void m_dma_free_coherent(struct device *dev, void *mem, int size)
{
    dma_addr_t dma_handle = virt_to_phys(mem);
    dma_free_coherent(dev, size, (void *)CKSEG1ADDR(mem), dma_handle);
}

static inline void m_cache_sync(void *mem, int size)
{
    dma_cache_inv((unsigned long)mem, size);
}


static int cim_alloc_mem(struct sensor_attr *sensor)
{
    struct jz_cim_data *drv = &jz_cim_dev;
    unsigned int line_length;   /* 一行存放的数据长度, 即:stride长度 */
    unsigned int pixel_bytes;   /* 每个像素由多少字节组成 */
    sensor_pixel_fmt sensor_fmt = sensor->sensor_info.fmt;
    int dma_output_y_mode = sensor->dma_mode;

    switch (sensor_fmt) {
    case SENSOR_PIXEL_FMT_Y8_1X8:
    case SENSOR_PIXEL_FMT_Y10_1X10:  /* 控制器丢弃低数据位 */
    case SENSOR_PIXEL_FMT_Y12_1X12:  /* 控制器丢弃低数据位 */
    case SENSOR_PIXEL_FMT_SBGGR8_1X8:
    case SENSOR_PIXEL_FMT_SGBRG8_1X8:
    case SENSOR_PIXEL_FMT_SGRBG8_1X8:
    case SENSOR_PIXEL_FMT_SRGGB8_1X8:
    case SENSOR_PIXEL_FMT_SBGGR10_ALAW8_1X8: /* 控制器丢弃低数据位 */
    case SENSOR_PIXEL_FMT_SGBRG10_ALAW8_1X8: /* 控制器丢弃低数据位 */
    case SENSOR_PIXEL_FMT_SGRBG10_ALAW8_1X8: /* 控制器丢弃低数据位 */
    case SENSOR_PIXEL_FMT_SRGGB10_ALAW8_1X8: /* 控制器丢弃低数据位 */
    case SENSOR_PIXEL_FMT_SBGGR12_ALAW8_1X8: /* 控制器丢弃低数据位 */
    case SENSOR_PIXEL_FMT_SGBRG12_ALAW8_1X8: /* 控制器丢弃低数据位 */
    case SENSOR_PIXEL_FMT_SGRBG12_ALAW8_1X8: /* 控制器丢弃低数据位 */
    case SENSOR_PIXEL_FMT_SRGGB12_ALAW8_1X8: /* 控制器丢弃低数据位 */
        drv->color_in_order = 1;
        line_length = sensor->sensor_info.width;
        pixel_bytes = 1;
        break;

    case SENSOR_PIXEL_FMT_SBGGR10_1X10:
    case SENSOR_PIXEL_FMT_SGBRG10_1X10:
    case SENSOR_PIXEL_FMT_SGRBG10_1X10:
    case SENSOR_PIXEL_FMT_SRGGB10_1X10:
        if (sensor->sensor_info.width % 4) {
            printk(KERN_ERR "sensor format(%x) is raw10 but width(%d) is not align: 4\n",
                    sensor->info.data_fmt, sensor->sensor_info.width);
            return -EINVAL;
        }

        if (sensor->dbus_type == SENSOR_DATA_BUS_DVP) {
            /* DVP接口，只能处理输入8bit数据,接收的一行数据量为sensor的宽度 */
            line_length = sensor->sensor_info.width;
        } else {
            /* mipi接口，按照协议可以接收raw10数据,接收模式为raw，接收的一行数据量raw10 sensor的宽度 */
            line_length = sensor->sensor_info.width * 5 / 4;
        }

        /*
         * sensor发送顺序和控制接收数据顺序不同，控制器无法提取Y和统计luminance值
         * 为简化配置，DVP模式可以提取Y也不做区分
         */
        drv->color_in_order = 0;
        pixel_bytes = 1;
        break;

    case SENSOR_PIXEL_FMT_SBGGR12_1X12:
    case SENSOR_PIXEL_FMT_SGBRG12_1X12:
    case SENSOR_PIXEL_FMT_SGRBG12_1X12:
    case SENSOR_PIXEL_FMT_SRGGB12_1X12:
        if (sensor->sensor_info.width % 2) {
            printk(KERN_ERR "sensor format(%x) is raw12 but width(%d) is not align: 2\n",
                    sensor->info.data_fmt, sensor->sensor_info.width);
            return -EINVAL;
        }

        if (sensor->dbus_type == SENSOR_DATA_BUS_DVP) {
            /* DVP接口，只能处理输入8bit数据,接收的一行数据量为sensor的宽度 */
            line_length = sensor->sensor_info.width;
        } else {
            /* mipi接口，按照协议可以接收raw10数据,接收模式为raw，接收的一行数据量raw12 sensor的宽度 */
            line_length = sensor->sensor_info.width * 3 / 2;
        }

        /*
         * sensor发送顺序和控制接收数据顺序不同，控制器无法提取Y和统计luminance值
         * 为简化配置，DVP模式可以提取Y也不做区分
         */
        drv->color_in_order = 0;
        pixel_bytes = 1;
        break;

    case SENSOR_PIXEL_FMT_UYVY8_2X8:
    case SENSOR_PIXEL_FMT_VYUY8_2X8:
    case SENSOR_PIXEL_FMT_YUYV8_2X8:
    case SENSOR_PIXEL_FMT_YVYU8_2X8:
    case SENSOR_PIXEL_FMT_RGB565_2X8_BE:
    case SENSOR_PIXEL_FMT_RGB565_2X8_LE:
        drv->color_in_order = 1;
        line_length = sensor->sensor_info.width;
        /* DMA输出GREY(Y8)数据，重新调整每个像素占用字节个数 */
        if (dma_output_y_mode == SENSOR_DATA_DMA_MODE_GREY)
            pixel_bytes = 1;
        else
            pixel_bytes = 2;

        break;

    case SENSOR_PIXEL_FMT_RGB888_1X24:
    case SENSOR_PIXEL_FMT_RBG888_1X24:
    case SENSOR_PIXEL_FMT_BGR888_1X24:
    case SENSOR_PIXEL_FMT_GBR888_1X24:
        drv->color_in_order = 1;
        line_length = sensor->sensor_info.width;
        /* DMA输出GREY(Y8)数据，重新调整每个像素占用字节个数 */
        if (dma_output_y_mode == SENSOR_DATA_DMA_MODE_GREY)
            pixel_bytes = 1;
        else
            pixel_bytes = 3;
        break;

    default:
        printk(KERN_ERR "cim alloc mem NOT support format %x\n", sensor_fmt);
        return -EINVAL;
    }


    /* raw10/raw12 需扩展存储空间 */
    unsigned int line_pixel_bytes = MEM_ALIGN(line_length, sensor->sensor_info.width) * pixel_bytes;
    unsigned int real_frame_size = line_pixel_bytes * sensor->sensor_info.height;
    unsigned int frame_align_size = MEM_ALIGN(real_frame_size, 4);  /* luminance 4Byte对齐 */

    /* 保存luminance值 */
    unsigned int frame_lumi_size = frame_align_size + CAMERA_LUMINANCE_SIZE;
    unsigned int frame_lumi_alloc_size = ALIGN(frame_lumi_size, 4096);

    unsigned int mem_cnt = drv->cam_mem_cnt + 1;

    assert(mem_cnt >= 2);

    void *mem = rmem_alloc_aligned(mem_cnt * frame_lumi_alloc_size, PAGE_SIZE);
    if (mem == NULL) {
        printk(KERN_ERR "camera: failed to alloc mem: %u\n", frame_lumi_alloc_size * mem_cnt);
        return -ENOMEM;
    }

    drv->mem = mem;
    drv->mem_cnt = mem_cnt;

    drv->frames = kzalloc(mem_cnt * sizeof(drv->frames[0]), GFP_KERNEL);
    assert(drv->frames);

    drv->trans_list = kmalloc(CAMERA_DMA_DESC_COUNT * sizeof(drv->trans_list[0]), GFP_KERNEL);
    assert(drv->trans_list);

    drv->frm_size = frame_lumi_alloc_size;   /* 申请内存空间大小(包含luminance 空间大小) */
    drv->frame_align_size = frame_align_size; /* luminance DMA偏移地址和raw10/raw12软件扩展大小 */
    sensor->info.fps = (sensor->sensor_info.fps >> 16) / (sensor->sensor_info.fps & 0xFFFF);
    sensor->info.frame_size = real_frame_size; /* sensor存放有效数据大小 */
    sensor->info.frame_align_size = frame_lumi_alloc_size; /* 用户空间获取mmap申请大小 */
    sensor->info.line_length = line_length;
    sensor->info.frame_nums = mem_cnt;
    sensor->info.phys_mem = virt_to_phys(drv->mem);

    init_frm_lists();

    /* DMA描述符 */
    drv->frame_descs = m_dma_alloc_coherent(drv->dev, CAMERA_DMA_DESC_COUNT * sizeof(drv->frame_descs[0]));
    assert(drv->frame_descs);
    init_frame_desc(&drv->frame_descs[0], 0, drv->frames[0].addr, &drv->frame_descs[1], sensor);
    init_frame_desc(&drv->frame_descs[1], 1, drv->frames[1].addr, &drv->frame_descs[0], sensor);

    dma_cache_wback((unsigned long)drv->frame_descs, CAMERA_DMA_DESC_COUNT * sizeof(drv->frame_descs[0]));
    //cim_dump_desc();

    return 0;
}

static void cim_free_mem(void)
{
    struct jz_cim_data *drv = &jz_cim_dev;

    kfree(drv->frames);
    kfree(drv->trans_list);

    rmem_free(drv->mem, drv->frm_size * drv->mem_cnt);
    m_dma_free_coherent(drv->dev, drv->frame_descs, CAMERA_DMA_DESC_COUNT * sizeof(drv->frame_descs[0]));

    drv->mem = NULL;
    drv->frames = NULL;
    drv->trans_list = NULL;
    drv->frame_descs = NULL;
}

static inline void cim_init_common_settint_format_error(struct sensor_attr *attr)
{
    sensor_pixel_fmt source_sensor_fmt = attr->sensor_info.fmt; /* sensor的格式 */
    int pixel_data_fmt = attr->info.data_fmt;   /* 暴露给应用的像素数据存储格式 */
    char pixel_fmt[4];
    pixel_fmt[0] = pixel_data_fmt >> 0;
    pixel_fmt[1] = pixel_data_fmt >> 8;
    pixel_fmt[2] = pixel_data_fmt >> 16;
    pixel_fmt[3] = pixel_data_fmt >> 24;
    printk(KERN_ERR "cim init commont setting output pixel fom %c%c%c%c NOT support source format %x\n", \
                pixel_fmt[0], pixel_fmt[1], pixel_fmt[2], pixel_fmt[3], source_sensor_fmt);
}

static void cim_init_common_setting(struct sensor_attr *attr)
{
    struct jz_cim_data *drv = &jz_cim_dev;
    unsigned long glb_cfg = cim_read_reg(CIM_GLB_CFG);
    unsigned int resolution_height = 0;
    unsigned int resolution_width = 0;

    /* 接口类型 */
    switch (attr->dbus_type) {
    case SENSOR_DATA_BUS_MIPI:
        set_bit_field(&glb_cfg, GLB_CFG_DAT_IF_SEL, 1);
        break;

    case SENSOR_DATA_BUS_DVP:
        set_bit_field(&glb_cfg, GLB_CFG_DAT_IF_SEL, 0);
        break;

    default:
        printk(KERN_ERR "cim unknown dbus_type: %d\n", attr->dbus_type);
        assert(0);
        return;
    }

    sensor_pixel_fmt source_sensor_fmt = attr->sensor_info.fmt; /* sensor的格式 */
    int pixel_data_fmt = attr->info.data_fmt;   /* 暴露给应用的像素数据存储格式 */

    /* Frame format */
    switch (pixel_data_fmt) {
    case CAMERA_PIX_FMT_GREY:
        resolution_height = attr->info.height;
        resolution_width = attr->info.line_length;

        /* sensor输入源格式选择,10-bits / 12-bits MONO数据的低有效位被丢弃 */
        if (source_sensor_fmt == SENSOR_PIXEL_FMT_Y8_1X8)
            set_bit_field(&glb_cfg, GLB_CFG_FRM_FORMAT, CIM_FRAME_FMT_MONO_8);
        else if (source_sensor_fmt == SENSOR_PIXEL_FMT_Y10_1X10)
            set_bit_field(&glb_cfg, GLB_CFG_FRM_FORMAT, CIM_FRAME_FMT_MONO_10);
        else if (source_sensor_fmt == SENSOR_PIXEL_FMT_Y12_1X12)
            set_bit_field(&glb_cfg, GLB_CFG_FRM_FORMAT, CIM_FRAME_FMT_MONO_12);
        else {
            cim_init_common_settint_format_error(attr);
            assert(0);
        }
        break;

    case CAMERA_PIX_FMT_SBGGR8:
    case CAMERA_PIX_FMT_SGBRG8:
    case CAMERA_PIX_FMT_SGRBG8:
    case CAMERA_PIX_FMT_SRGGB8:
        resolution_height = attr->info.height;
        resolution_width = attr->info.line_length;
        /* 配置RGB顺序 */
        if (pixel_data_fmt == CAMERA_PIX_FMT_SBGGR8)
            set_bit_field(&glb_cfg, GLB_CFG_COLOR_ORDER, CIM_FRAME_ORDER_BGGR);
        else if (pixel_data_fmt == CAMERA_PIX_FMT_SGBRG8)
            set_bit_field(&glb_cfg, GLB_CFG_COLOR_ORDER, CIM_FRAME_ORDER_GBRG);
        else if (pixel_data_fmt == CAMERA_PIX_FMT_SGRBG8)
            set_bit_field(&glb_cfg, GLB_CFG_COLOR_ORDER, CIM_FRAME_ORDER_GRBG);
        else if (pixel_data_fmt == CAMERA_PIX_FMT_SRGGB8)
            set_bit_field(&glb_cfg, GLB_CFG_COLOR_ORDER, CIM_FRAME_ORDER_RGGB);

        /*
         * DVP接口, sensor输出的是8/10/12-bits 数据， 接口丢弃低有效数据位输出8-bits数据
         * MIPI接收，sensor输出的是8/10/12-bits 数据， 控制器丢弃低有效数据位输出8-bits数据
         */
        if ((source_sensor_fmt == SENSOR_PIXEL_FMT_SBGGR8_1X8) ||
            (source_sensor_fmt == SENSOR_PIXEL_FMT_SGBRG8_1X8) ||
            (source_sensor_fmt == SENSOR_PIXEL_FMT_SGRBG8_1X8) ||
            (source_sensor_fmt == SENSOR_PIXEL_FMT_SRGGB8_1X8) ) {
            set_bit_field(&glb_cfg, GLB_CFG_FRM_FORMAT, CIM_FRAME_FMT_RAW_RGB_8);

        } else if ((source_sensor_fmt == SENSOR_PIXEL_FMT_SBGGR10_ALAW8_1X8) ||
            (source_sensor_fmt == SENSOR_PIXEL_FMT_SGBRG10_ALAW8_1X8) ||
            (source_sensor_fmt == SENSOR_PIXEL_FMT_SGRBG10_ALAW8_1X8) ||
            (source_sensor_fmt == SENSOR_PIXEL_FMT_SRGGB10_ALAW8_1X8)) {
            set_bit_field(&glb_cfg, GLB_CFG_FRM_FORMAT, CIM_FRAME_FMT_RAW_RGB_10);

        } else if ((source_sensor_fmt == SENSOR_PIXEL_FMT_SBGGR12_ALAW8_1X8) ||
            (source_sensor_fmt == SENSOR_PIXEL_FMT_SGBRG12_ALAW8_1X8) ||
            (source_sensor_fmt == SENSOR_PIXEL_FMT_SGRBG12_ALAW8_1X8) ||
            (source_sensor_fmt == SENSOR_PIXEL_FMT_SRGGB12_ALAW8_1X8) ) {
            set_bit_field(&glb_cfg, GLB_CFG_FRM_FORMAT, CIM_FRAME_FMT_RAW_RGB_12);

        } else {
            cim_init_common_settint_format_error(attr);
            assert(0);
        }
        break;


    case CAMERA_PIX_FMT_SBGGR16:
    case CAMERA_PIX_FMT_SGBRG16:
    case CAMERA_PIX_FMT_SGRBG16:
    case CAMERA_PIX_FMT_SRGGB16:
        resolution_height = attr->info.height;
        resolution_width = attr->info.line_length;

        /* 配置RGB顺序 */
        if (pixel_data_fmt == CAMERA_PIX_FMT_SBGGR16)
            set_bit_field(&glb_cfg, GLB_CFG_COLOR_ORDER, CIM_FRAME_ORDER_BGGR);
        else if (pixel_data_fmt == CAMERA_PIX_FMT_SGBRG16)
            set_bit_field(&glb_cfg, GLB_CFG_COLOR_ORDER, CIM_FRAME_ORDER_GBRG);
        else if (pixel_data_fmt == CAMERA_PIX_FMT_SGRBG16)
            set_bit_field(&glb_cfg, GLB_CFG_COLOR_ORDER, CIM_FRAME_ORDER_GRBG);
        else if (pixel_data_fmt == CAMERA_PIX_FMT_SRGGB16)
            set_bit_field(&glb_cfg, GLB_CFG_COLOR_ORDER, CIM_FRAME_ORDER_RGGB);

        /*
         * sensor输出10-bits / 12-bits数据， 控制器接收10-bits / 12-bits数据(软件扩展为16-bits)
         * DVP模式 只接收8-bits数据，格式检查不通过
         * MIPI模式下按照8-bits协议接收10-bits / 12-bits 数据，设置接收宽度大小为sensor的width*5/4，或width*3/2
         * MIPI模式下,接收数据顺序被调整，控制器无法提取出Y，以计算luminance值
         */
        set_bit_field(&glb_cfg, GLB_CFG_FRM_FORMAT, CIM_FRAME_FMT_RAW_RGB_8);
        break;

    case CAMERA_PIX_FMT_YUYV:
        resolution_height = attr->info.height;
        resolution_width = attr->info.width;
        set_bit_field(&glb_cfg, GLB_CFG_FRM_FORMAT, CIM_FRAME_FMT_YUV422);
        set_bit_field(&glb_cfg, GLB_CFG_COLOR_ORDER, CIM_FRAME_ORDER_YUYV);
        break;

    case CAMERA_PIX_FMT_YVYU:
        resolution_height = attr->info.height;
        resolution_width = attr->info.width;
        set_bit_field(&glb_cfg, GLB_CFG_FRM_FORMAT, CIM_FRAME_FMT_YUV422);
        set_bit_field(&glb_cfg, GLB_CFG_COLOR_ORDER, CIM_FRAME_ORDER_YVYU);
        break;

    case CAMERA_PIX_FMT_UYVY:
        resolution_height = attr->info.height;
        resolution_width = attr->info.width;
        set_bit_field(&glb_cfg, GLB_CFG_FRM_FORMAT, CIM_FRAME_FMT_YUV422);
        set_bit_field(&glb_cfg, GLB_CFG_COLOR_ORDER, CIM_FRAME_ORDER_UYVY);
        break;

    case CAMERA_PIX_FMT_VYUY:
        resolution_height = attr->info.height;
        resolution_width = attr->info.width;
        set_bit_field(&glb_cfg, GLB_CFG_FRM_FORMAT, CIM_FRAME_FMT_YUV422);
        set_bit_field(&glb_cfg, GLB_CFG_COLOR_ORDER, CIM_FRAME_ORDER_VYUY);
        break;

    case CAMERA_PIX_FMT_RGB565:
        resolution_height = attr->info.height;
        resolution_width = attr->info.width;
        set_bit_field(&glb_cfg, GLB_CFG_FRM_FORMAT, CIM_FRAME_FMT_RGB565);
        set_bit_field(&glb_cfg, GLB_CFG_COLOR_ORDER, CIM_FRAME_ORDER_RGB);
        break;

    case CAMERA_PIX_FMT_RGB24:
    case CAMERA_PIX_FMT_RBG24:
    case CAMERA_PIX_FMT_BGR24:
    case CAMERA_PIX_FMT_GBR24:
        resolution_height = attr->info.height;
        resolution_width = attr->info.width;
        set_bit_field(&glb_cfg, GLB_CFG_FRM_FORMAT, CIM_FRAME_FMT_RGB888);

        /* 配置RGB顺序 */
        if (pixel_data_fmt == CAMERA_PIX_FMT_RGB24)
            set_bit_field(&glb_cfg, GLB_CFG_COLOR_ORDER, CIM_FRAME_ORDER_RGB);
        else if (pixel_data_fmt == CAMERA_PIX_FMT_RBG24)
            set_bit_field(&glb_cfg, GLB_CFG_COLOR_ORDER, CIM_FRAME_ORDER_RBG);
        else if (pixel_data_fmt == CAMERA_PIX_FMT_BGR24)
            set_bit_field(&glb_cfg, GLB_CFG_COLOR_ORDER, CIM_FRAME_ORDER_BGR);
        else if (pixel_data_fmt == CAMERA_PIX_FMT_GBR24)
            set_bit_field(&glb_cfg, GLB_CFG_COLOR_ORDER, CIM_FRAME_ORDER_GBR);

        break;

    default:
        printk(KERN_ERR "default: ");
        cim_init_common_settint_format_error(attr);
        assert(0);
        return;
    }

    /* set dma burst length */
    set_bit_field(&glb_cfg, GLB_CFG_BURST_LEN, CIM_DMA_BURST_LEN_32);

    /* enable auto recovery */
    set_bit_field(&glb_cfg, GLB_CFG_AUTO_RECOVERY, 1);

    /* frame size check enable/disable */
    if (is_frame_size_check)
        set_bit_field(&glb_cfg, GLB_CFG_SIZE_CHK, 1);
    else
        set_bit_field(&glb_cfg, GLB_CFG_SIZE_CHK, 0);

    /* disable snapshot */
    if (is_enable_snapshot) {
        unsigned char expo_width = 0x08;
        set_bit_field(&glb_cfg, GLB_CFG_SNAPSHOT_MODE, 1);
        set_bit_field(&glb_cfg, GLB_CFG_EXPO_WIDTH, expo_width - 1);

        unsigned int delay_num = 0x08; /* 该值目前随机 */
        unsigned long delay_cfg;
        set_bit_field(&delay_cfg, DLY_CFG_EN, 1);
        set_bit_field(&delay_cfg, DLY_CFG_MD, 1);
        set_bit_field(&delay_cfg, DLY_CFG_NUM, delay_num);

        cim_write_reg(CIM_DLY_CFG, delay_cfg);
    } else {
        cim_write_reg(CIM_DLY_CFG, 0);
    }

    /* luminance area */
    struct lumi_area *lumi_info = &drv->lumi_info;
    if (lumi_info->enable) {
        cim_set_luminance_area_point1(lumi_info->x1, lumi_info->y1);
        cim_set_luminance_area_point2(lumi_info->x2, lumi_info->y2);
    }

    /* image no resize */
    unsigned long resolution = 0;
#if 0
    /* (调试ITU模式时验证) ITU656 interlace mode */
    set_bit_field(&resolution, CROP_SIZE_HEIGHT, resolution_height / 2);
#else
    /* progressive mode */
    set_bit_field(&resolution, CROP_SIZE_HEIGHT, resolution_height);
#endif

    set_bit_field(&resolution, CROP_SIZE_WIDTH,  resolution_width);

    cim_write_reg(CIM_CROP_SIZE, resolution);
    cim_write_reg(CIM_CROP_SITE, 0);

    /* interrupt setting */
    unsigned long interrupt = 0;
    /* 停止正常工作帧中断 */
    set_bit_field(&interrupt, INTC_MSK_EOW,  1);

    /*
     * 一帧采集结束中断
     * Work together with DES_INTC.EOF_MSK
     * only when both are 1 will generate interrupt
     */
    set_bit_field(&interrupt, INTC_MSK_EOF,  1);

    //set_bit_field(&interrupt, INTC_MSK_SOF,  1);

    /* enable general stop of frame interrupt */
    set_bit_field(&interrupt, INTC_MSK_GSA,  1);

    /* enable rxfifo overflow interrupt */
    set_bit_field(&interrupt, INTC_MSK_OVER,  1);

    /* enable size check err */
    if (is_frame_size_check)
        set_bit_field(&interrupt, INTC_MSK_SZ_ERR,  1);
    else
        set_bit_field(&interrupt, INTC_MSK_SZ_ERR,  0);

    cim_write_reg(CIM_DBG_CGC, 0x00);
    cim_write_reg(CIM_GLB_CFG, glb_cfg);
    cim_write_reg(CIM_INTC, interrupt);
    cim_write_reg(CIM_QOS_CTRL, 0);
}

static void cim_init_dvp_timming(struct sensor_attr *attr)
{
    unsigned long glb_cfg = cim_read_reg(CIM_GLB_CFG);

    /* PCLK Polarity */
    if (attr->dvp.pclk_polarity == POLARITY_SAMPLE_RISING)
        set_bit_field(&glb_cfg, GLB_CFG_EDGE_PCLK, CIM_PCLK_SAMPLE_RISING);
    else
        set_bit_field(&glb_cfg, GLB_CFG_EDGE_PCLK, CIM_PCLK_SAMPLE_FALLING);

    /* VSYNC Polarity */
    if (attr->dvp.vsync_polarity == POLARITY_HIGH_ACTIVE)
        set_bit_field(&glb_cfg, GLB_CFG_LEVEL_VSYNC, CIM_VSYNC_ACTIVE_HIGH);
    else
        set_bit_field(&glb_cfg, GLB_CFG_LEVEL_VSYNC, CIM_VSYNC_ACTIVE_LOW);

    /* HSYNC Polarity */
    if (attr->dvp.hsync_polarity == POLARITY_HIGH_ACTIVE)
        set_bit_field(&glb_cfg, GLB_CFG_LEVEL_HSYNC, CIM_HSYNC_ACTIVE_HIGH);
    else
        set_bit_field(&glb_cfg, GLB_CFG_LEVEL_HSYNC, CIM_HSYNC_ACTIVE_LOW);

    cim_write_reg(CIM_GLB_CFG, glb_cfg);
}


static void cim_dvp_init_setting(struct sensor_attr *attr)
{
    assert_range(attr->sensor_info.width, 1, 2047);
    assert_range(attr->sensor_info.height, 1, 2047);
    assert(attr->dbus_type == SENSOR_DATA_BUS_DVP);

    cim_init_common_setting(attr);

    cim_init_dvp_timming(attr);

    cim_init_dma_desc();

    cim_set_dma_desc_addr(0);

    cim_clear_status_all();
}

static void cim_init_mipi_timming(struct sensor_attr *attr)
{
    unsigned long glb_cfg = cim_read_reg(CIM_GLB_CFG);

    /* PCLK Polarity */
    set_bit_field(&glb_cfg, GLB_CFG_EDGE_PCLK, CIM_PCLK_SAMPLE_RISING);

    /* VSYNC Polarity */
    set_bit_field(&glb_cfg, GLB_CFG_LEVEL_VSYNC, CIM_VSYNC_ACTIVE_HIGH);

    /* HSYNC Polarity */
    set_bit_field(&glb_cfg, GLB_CFG_LEVEL_HSYNC, CIM_HSYNC_ACTIVE_HIGH);

    cim_write_reg(CIM_GLB_CFG, glb_cfg);
}

static void cim_mipi_init_setting(struct sensor_attr *attr)
{
    assert_range(attr->sensor_info.width, 1, 2047);
    assert_range(attr->sensor_info.height, 1, 2047);
    assert_range(attr->mipi.lanes, 1, 2);
    assert(attr->dbus_type == SENSOR_DATA_BUS_MIPI);

    cim_init_common_setting(attr);

    cim_init_mipi_timming(attr);

    cim_init_dma_desc();

    cim_set_dma_desc_addr(0);

    int csi_ret = mipi_csi_phy_initialization(&attr->mipi);
    assert(csi_ret >= 0);

    cim_clear_status_all();
}

static void cim_init_setting(struct sensor_attr *attr)
{
    struct jz_cim_data *drv = &jz_cim_dev;

    clk_enable(drv->cim_gate_clk);

    reset_frm_lists();

    /* ITU656 暂未支持 */
    if (attr->dbus_type == SENSOR_DATA_BUS_DVP)
        cim_dvp_init_setting(attr);
    else if (attr->dbus_type == SENSOR_DATA_BUS_MIPI)
        cim_mipi_init_setting(attr);
    else
        panic("camera: not support this type now: %d\n", attr->dbus_type);
}

static void cim_deinit_setting(struct sensor_attr *attr)
{
    struct jz_cim_data *drv = &jz_cim_dev;

    if (attr->dbus_type == SENSOR_DATA_BUS_MIPI)
        mipi_csi_phy_stop(&attr->mipi);

    clk_disable(drv->cim_gate_clk);
}

static void cim_stream_on(struct sensor_attr *attr)
{
    struct jz_cim_data *drv = &jz_cim_dev;

    cim_soft_reset();

    cim_start();

    //cim_dump_regs();

    enable_irq(drv->irq);
}

static void cim_stream_off(struct sensor_attr *attr)
{
    struct jz_cim_data *drv = &jz_cim_dev;

    disable_irq(drv->irq);

    cim_set_bit(CIM_CTRL, CTRL_QUICK_STOP, 1);
    cim_write_reg(CIM_CLR_ST, 0xFF);

    cim_soft_reset();

    if (attr->dbus_type == SENSOR_DATA_BUS_MIPI)
        mipi_csi_phy_stop(&attr->mipi);

    clk_disable(drv->cim_gate_clk);
}



static void camera_skip_frames(unsigned int frames)
{
    struct jz_cim_data *drv = &jz_cim_dev;
    unsigned long flags;

    mutex_lock(&drv->lock);

    spin_lock_irqsave(&drv->spinlock, flags);

    while (frames--) {
        struct frame_data *frm = get_usable_frm();
        if (frm == NULL)
            break;
        add_to_free_list(frm);
    }

    spin_unlock_irqrestore(&drv->spinlock, flags);

    mutex_unlock(&drv->lock);
}

static unsigned int camera_get_available_frame_count(void)
{
    struct jz_cim_data *drv = &jz_cim_dev;

    return drv->frame_counter;
}

static int check_frame_mem(void *mem, void *base)
{
    struct jz_cim_data *drv = &jz_cim_dev;
    unsigned int size = mem - base;

    if (mem < base)
        return -1;

    if (size % drv->frm_size)
        return -1;

    if (size / drv->frm_size >= drv->mem_cnt)
        return -1;

    return 0;
}

static inline struct frame_data *mem_2_frame_data(void *mem)
{
    struct jz_cim_data *drv = &jz_cim_dev;
    unsigned int size = mem - drv->mem;
    unsigned int count = size / drv->frm_size;
    struct frame_data *frm = &drv->frames[count];

    assert(!(size % drv->frm_size));
    assert(count < drv->mem_cnt);

    return frm;
}

static int check_frame_info(struct frame_info *info)
{
    struct jz_cim_data *drv = &jz_cim_dev;
    struct frame_data *frm = &drv->frames[info->index];

    if (info->index >= drv->mem_cnt)
        return -1;

    if (frm->info.paddr != info->paddr)
        return -1;

    return 0;
}

static inline struct frame_data *frame_info_2_frame_data(struct frame_info *info)
{
    struct jz_cim_data *drv = &jz_cim_dev;

    if (!check_frame_info(info))
        return &drv->frames[info->index];
    else {
        return NULL;
    }
}

static void camera_do_put_frame(struct frame_data *frm)
{
    struct jz_cim_data *drv = &jz_cim_dev;
    unsigned long flags;

    spin_lock_irqsave(&drv->spinlock, flags);

    list_del(&frm->link);

    if (drv->trans_list[0] == drv->trans_list[1]) {
        int index = cim_read_reg(CIM_FRM_ID);

        set_transfer_frame(index, frm);
    } else {
        add_to_free_list(frm);
    }

    spin_unlock_irqrestore(&drv->spinlock, flags);
}

static void camera_put_frame(struct frame_data *frm)
{
    struct jz_cim_data *drv = &jz_cim_dev;

    mutex_lock(&drv->lock);

    if (frm->status != frame_status_user) {
        printk(KERN_ERR "%s camera: double free frame index %d\n", drv->device_name, frm->info.index);
        goto unlock;
    }

    camera_do_put_frame(frm);

unlock:
    mutex_unlock(&drv->lock);
}

static int camera_get_frame(struct list_head *list, struct frame_data **frame, unsigned int timeout_ms)
{
    struct jz_cim_data *drv = &jz_cim_dev;
    struct frame_data *frm;
    unsigned long flags;
    int ret = -EAGAIN;

    mutex_lock(&drv->lock);

    spin_lock_irqsave(&drv->spinlock, flags);
    frm = get_usable_frm();
    spin_unlock_irqrestore(&drv->spinlock, flags);
    if (!frm && timeout_ms) {
        ret = wait_event_interruptible_timeout(drv->waiter, drv->frame_counter, msecs_to_jiffies(timeout_ms));

        spin_lock_irqsave(&drv->spinlock, flags);
        frm = get_usable_frm();
        spin_unlock_irqrestore(&drv->spinlock, flags);
        if (ret <= 0 && !frm) {
            ret = -ETIMEDOUT;
            printk(KERN_ERR "%s : wait frame time out\n", drv->device_name);
        }
    }

    if (frm) {
        ret = 0;
        frm->status = frame_status_user;
        list_add_tail(&frm->link, list);
        *frame = frm;
    }

    mutex_unlock(&drv->lock);

    return ret;
}

/*
 * 从内存中的最后有效数据向前排列
 * 小端排序, 低6位补0
 * mipi raw10 格式排序转换为raw16
 * raw10在内存中的排列顺序
 * MSB                                               LSB
 *   8bit         8bit       8bit         8bit
 * [d9 ~ d2]   [c9 ~ c2]   [b9 ~ b2]   [a9 ~ a2]
 * [ ....  ]   [ ....  ]   [ ....  ]   [d1d0c1c0b1b0a1a0]
 *
 *
 *
 * 转换后raw16在内存中的排列顺序
 *   8bit         8bit         8bit         8bit
 * [b9 ~ b2]   [b1b0------] [a9 ~ a2]   [a1a0------]
 * [d9 ~ d2]   [d1d0------] [c9 ~ c2]   [c1c0------]
 *
 */
static void frame_convert_raw10_padding_raw16_forward(unsigned char *src, unsigned short *dst, int length)
{
    while (length) {
        src -= 5;
        unsigned short a = (src[0] << 8) | ((src[4] << 6) & 0xC0);
        unsigned short b = (src[1] << 8) | ((src[4] << 4) & 0xC0);
        unsigned short c = (src[2] << 8) | ((src[4] << 2) & 0xC0);
        unsigned short d = (src[3] << 8) | ((src[4] << 0) & 0xC0);

        dst -= 4;
        dst[0] = a;
        dst[1] = b;
        dst[2] = c;
        dst[3] = d;

        length -= 5;
    }
}

/*
 * 从内存中的最后有效数据向前排列
 * 小端排序, 低4位补0
 *
 * mipi raw12 格式排序转换为raw16
 * raw12在内存中的排列顺序
 * MSB                                                    LSB
 *   8bit                8bit            8bit         8bit
 * [ ....  ]     [b3b2b1b0a3a2a1a0]   [b11 ~ b4]   [a11 ~ a4]
 *
 *
 *
 * 转换后raw16在内存中的排列顺序
 *   8bit             8bit               8bit         8bit
 * [b11 ~ b4]    [b3b2b1b0----]       [a11 ~ a4]   [a3a2a1a0----]
 *
 */
static void frame_convert_raw12_padding_raw16_forward(unsigned char *src, unsigned char *dst, int length)
{
    while (length) {
        src -= 3;
        unsigned short a = (src[0] << 8) | ((src[2] << 4) & 0xF0);
        unsigned short b = (src[1] << 8) | ((src[2] << 0) & 0xF0);

        dst -= 2;
        dst[0] = a;
        dst[1] = b;

        length -= 3;
    }
}

static void frame_format_convert(struct jz_cim_data *drv, void *address)
{
    struct sensor_attr *attr = drv->camera.sensor;
    sensor_pixel_fmt sensor_fmt = attr->sensor_info.fmt;

    if (attr->dbus_type == SENSOR_DATA_BUS_DVP) {
        /* DVP只能输入8bit数据 */
        return ;
    }

    /* CSI实际接收到的有效数据大小 */
    unsigned int real_frame_size = attr->info.line_length * attr->info.height;

    switch (sensor_fmt) {
    case SENSOR_PIXEL_FMT_SBGGR10_1X10:
    case SENSOR_PIXEL_FMT_SGBRG10_1X10:
    case SENSOR_PIXEL_FMT_SGRBG10_1X10:
    case SENSOR_PIXEL_FMT_SRGGB10_1X10:
        /* 从后向前将数据扩展为raw16格式 */
        frame_convert_raw10_padding_raw16_forward(
                address + real_frame_size,
                address + drv->frame_align_size,
                real_frame_size);
        break;

    case SENSOR_PIXEL_FMT_SBGGR12_1X12:
    case SENSOR_PIXEL_FMT_SGBRG12_1X12:
    case SENSOR_PIXEL_FMT_SGRBG12_1X12:
    case SENSOR_PIXEL_FMT_SRGGB12_1X12:
        /* 从后向前将数据扩展为raw16格式 */
        frame_convert_raw12_padding_raw16_forward(
                address + real_frame_size,
                address + drv->frame_align_size,
                real_frame_size);
        break;

    default:
        break;
    }
}

static int camera_stream_on(void)
{
    struct jz_cim_data *drv = &jz_cim_dev;
    struct sensor_attr *attr = drv->camera.sensor;
    int ret = 0;

    mutex_lock(&drv->lock);

    if (!drv->camera.is_power_on) {
        printk(KERN_ERR "camera can't stream on when not power on\n");
        ret = -EINVAL;
        goto unlock;
    }

    if (drv->camera.is_stream_on)
        goto unlock;

    cim_init_setting(attr);

    ret = attr->ops.stream_on();
    if (ret) {
        cim_deinit_setting(attr);
        goto unlock;
    }

    cim_stream_on(attr);

    drv->camera.is_stream_on = 1;

unlock:
    mutex_unlock(&drv->lock);

    return ret;
}

static void camera_stream_off(void)
{
    struct jz_cim_data *drv = &jz_cim_dev;
    struct sensor_attr *attr = drv->camera.sensor;

    mutex_lock(&drv->lock);

    if (drv->camera.is_stream_on) {
        cim_stream_off(attr);
        attr->ops.stream_off();
        drv->camera.is_stream_on = 0;
    }

    mutex_unlock(&drv->lock);
}

static void camera_power_off(void)
{
    struct jz_cim_data *drv = &jz_cim_dev;
    struct sensor_attr *attr = drv->camera.sensor;

    mutex_lock(&drv->lock);

    if (drv->camera.is_stream_on) {
        cim_stream_off(attr);
        attr->ops.stream_off();
        drv->camera.is_stream_on = 0;
    }

    if (drv->camera.is_power_on) {
        attr->ops.power_off();
        drv->camera.is_power_on = 0;
    }

    mutex_unlock(&drv->lock);
}


static int camera_power_on(void)
{
    struct jz_cim_data *drv = &jz_cim_dev;
    struct sensor_attr *attr = drv->camera.sensor;
    int ret = 0;

    mutex_lock(&drv->lock);

    if (drv->camera.is_power_on) {
        printk(KERN_ERR "cim is already power on, no need power on again\n");
        goto unlock;
    }

    /* device(sensor) power on */
    ret = attr->ops.power_on();
    if (!ret) {
        drv->camera.is_power_on = 1;

        drv->cim_frm_done = 0;
    }

unlock:
    mutex_unlock(&drv->lock);

    return ret;
}

static int camera_get_info(struct camera_info *info)
{
    struct jz_cim_data *drv = &jz_cim_dev;
    int ret = 0;

    mutex_lock(&drv->lock);

    ret = copy_to_user((void __user *)info, &drv->camera.sensor->info, sizeof(*info));
    if (ret)
        printk(KERN_ERR "%s : camera can't get camera info\n", drv->device_name);

    mutex_unlock(&drv->lock);

    return ret;
}

static int camera_get_sensor_reg(struct sensor_dbg_register *reg)
{
    struct jz_cim_data *drv = &jz_cim_dev;
    struct sensor_attr *attr = drv->camera.sensor;
    struct sensor_dbg_register dbg_reg;
    int ret = 0;

    if (copy_from_user(&dbg_reg, (void __user*)reg, sizeof(struct sensor_dbg_register))) {
        printk(KERN_ERR "%s copy from user error\n", __func__);
        return -EFAULT;
    }

    if (attr->ops.get_register) {
        ret = attr->ops.get_register(&dbg_reg);
        if (ret < 0) {
            printk(KERN_ERR "%s get_register fail\n", __func__);
            return ret;
        }
    } else {
        printk(KERN_ERR "attr->ops.get_register is NULL!\n");
        return -EFAULT;
    }

    if (copy_to_user((void __user *)reg, &dbg_reg, sizeof(struct sensor_dbg_register))) {
        printk(KERN_ERR "%s copy from user error\n", __func__);
        return -EFAULT;
    }

    return 0;
}

static int camera_set_sensor_reg(struct sensor_dbg_register *reg)
{
    struct jz_cim_data *drv = &jz_cim_dev;
    struct sensor_attr *attr = drv->camera.sensor;
    struct sensor_dbg_register dbg_reg;
    int ret = 0;

    if (copy_from_user(&dbg_reg, (void __user*)reg, sizeof(struct sensor_dbg_register))) {
        printk(KERN_ERR "%s copy from user error\n", __func__);
        return -EFAULT;
    }

    if (attr->ops.set_register) {
        ret = attr->ops.set_register(&dbg_reg);
        if (ret < 0) {
            printk(KERN_ERR "%s set_register fail\n", __func__);
            return ret;
        }
    } else {
        printk(KERN_ERR "attr->ops.set_register is NULL!\n");
        return -EINVAL;
    }

    return 0;
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
#define CMD_get_sensor_reg          _IO('C', 129)
#define CMD_set_sensor_reg          _IO('C', 130)
#define CMD_get_frame               _IO('C', 131)
#define CMD_dqbuf                   _IO('C', 132)
#define CMD_dqbuf_wait              _IO('C', 133)
#define CMD_qbuf                    _IO('C', 134)


struct m_private_data {
    struct list_head list;
    void *map_mem;
};

static long cim_ioctl(struct file *filp, unsigned int cmd, unsigned long arg)
{
    struct jz_cim_data *drv = &jz_cim_dev;
    struct m_private_data *data = filp->private_data;
    struct list_head *list = &data->list;
    void *map_mem = data->map_mem;
    struct frame_data *frm;
    unsigned int timeout_ms = 0;
    int ret;

    switch (cmd) {
    case CMD_get_info:{
        struct camera_info *info = (void *)arg;
        if (!info)
            return -EINVAL;

        ret = camera_get_info(info);
        return ret;
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

    case CMD_wait_frame:
        timeout_ms = 3000;
    case CMD_get_frame: {
        if (!map_mem) {
            printk(KERN_ERR "camera: please mmap first\n");
            return -EINVAL;
        }

        void **mem_p = (void *)arg;
        if (!mem_p)
            return -EINVAL;

        ret = camera_get_frame(list, &frm, timeout_ms);
        if (0 == ret) {
            frm->info.vaddr = map_mem + (frm->addr - drv->mem);
            m_cache_sync(frm->addr, frm->info.size);
            frame_format_convert(drv, frm->addr);
        } else
            return ret;

        *mem_p = frm->info.vaddr;
        return 0;
    }

    case CMD_put_frame: {
        void *mem = (void *)arg;
        if (check_frame_mem(mem, map_mem))
            return -EINVAL;

        mem = drv->mem + (mem - map_mem);
        frm = mem_2_frame_data(mem);
        m_cache_sync(frm->addr, frm->info.size);
        camera_put_frame(frm);
        return 0;
    }

    case CMD_dqbuf_wait:
        timeout_ms = 3000;
    case CMD_dqbuf: {
        if (!map_mem) {
            printk(KERN_ERR "camera: please mmap first\n");
            return -EINVAL;
        }

        ret = camera_get_frame(list, &frm, timeout_ms);
        if (0 == ret) {
            frm->info.vaddr = map_mem + (frm->addr - drv->mem);
            m_cache_sync(frm->addr, frm->info.size);
            frame_format_convert(drv, frm->addr);
        } else
            return ret;

        ret = copy_to_user((void __user *)arg, &frm->info, sizeof(struct frame_info));
        if (ret) {
            printk(KERN_ERR "%s: CMD_dqbuf copy_to_user fail\n", drv->device_name);
            return ret;
        }
        return 0;
    }

    case CMD_qbuf: {
        struct frame_info info;
        ret = copy_from_user(&info, (void __user *)arg, sizeof(struct frame_info));
        if(ret){
            printk(KERN_ERR "%s: CMD_qbuf copy_from_user fail\n", drv->device_name);
            return ret;
        }
        frm = frame_info_2_frame_data(&info);
        if (!frm) {
            printk(KERN_ERR "%s: CMD_qbuf err frame info\n", drv->device_name);
            return -EINVAL;
        }

        camera_put_frame(frm);
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

    case CMD_get_sensor_reg: {
        if (arg)
            return camera_get_sensor_reg((struct sensor_dbg_register *)arg);
        else
            return -EINVAL;
    }

    case CMD_set_sensor_reg: {
        if (arg)
            return camera_set_sensor_reg((struct sensor_dbg_register *)arg);
        else
            return -EINVAL;
    }

    default:
        printk(KERN_ERR "camera: %x not support this cmd\n", cmd);
        return -EINVAL;

    }

    return 0;
}


static int cim_mmap(struct file *file, struct vm_area_struct *vma)
{
    struct jz_cim_data *drv = &jz_cim_dev;
    unsigned long start;
    unsigned long offset;
    unsigned long len;

    offset = vma->vm_pgoff << PAGE_SHIFT;
    if (offset) {
        printk(KERN_ERR "camera: cim mmap offset must be 0\n");
        return -EINVAL;
    }

    len = drv->frm_size * drv->mem_cnt;
    if ((vma->vm_end - vma->vm_start) != len) {
        printk(KERN_ERR "camera: cim mmap size must be total size\n");
        return -EINVAL;
    }
    /* frame buffer memory */
    start = virt_to_phys(drv->mem);
    offset += start;

    vma->vm_pgoff = offset >> PAGE_SHIFT;
    vma->vm_flags |= VM_IO;


    /* 0: cachable,write through (cache + cacheline对齐写穿)
     * 1: uncachable,write Acceleration (uncache + 硬件写加速)
     * 2: uncachable
     * 3: cachable
     */
    pgprot_val(vma->vm_page_prot) &= ~_CACHE_MASK;
    pgprot_val(vma->vm_page_prot) |= _CACHE_CACHABLE_NO_WA;

    if (io_remap_pfn_range(vma, vma->vm_start, offset >> PAGE_SHIFT,
                           vma->vm_end - vma->vm_start, vma->vm_page_prot))
    {
        return -EAGAIN;
    }

    struct m_private_data *data = file->private_data;
    data->map_mem = (void *)vma->vm_start;

    return 0;
}

static int cim_open(struct inode *inode, struct file *filp)
{
    struct m_private_data *data = kmalloc(sizeof(*data), GFP_KERNEL);

    INIT_LIST_HEAD(&data->list);
    filp->private_data = data;

    return 0;
}

static int cim_release(struct inode *inode, struct file *filp)
{
    struct jz_cim_data *drv = &jz_cim_dev;
    struct list_head *pos, *n;
    struct m_private_data *data = filp->private_data;
    struct list_head *list = &data->list;

    mutex_lock(&drv->lock);

    list_for_each_safe(pos, n, list) {
        struct frame_data *frm = list_entry(pos, struct frame_data, link);
        camera_do_put_frame(frm);
    }

    mutex_unlock(&drv->lock);

    kfree(data);

    return 0;
}


static struct file_operations cim_misc_fops = {
    .open           = cim_open,
    .release        = cim_release,
    .mmap           = cim_mmap,
    .unlocked_ioctl = cim_ioctl,
};



#define error_if(_cond)                                     \
    do {                                                    \
        if (_cond) {                                        \
            printk(KERN_ERR "cim: failed to check: %s\n", #_cond);   \
            ret = -1;                                       \
            goto unlock;                                    \
        }                                                   \
    } while (0)


/*
 * 格式信息转换 转换后的格式经过CIM DMA 到DDR
 * sensor format : Sensor格式在sensor driver中根据setting指定
 * camera format : Camera格式在camera driver中使用,并暴露给应用
 *
 *    sensor格式                  Camera格式
 * [成员0 sensor format]   <--->  [成员1 camera format]
 */
struct format_mapping {
    sensor_pixel_fmt sensor_fmt;
    camera_pixel_fmt camera_fmt;
};


/*
 * DMA支持的格式转换
 * 输入格式            输出格式
 * YUV422             YUV422
 *                    Y8(直接提取)
 *
 * RGB888             RGB888
 *                    Y8 (计算公式)
 *
 * RGB565             RGB565
 *                    Y8 (计算公式)
 *
 * MONO-8bits         MONO-8bit
 *                    Y8 (直接提取)
 *
 * MONO-10bits        MONO-8bit(丢弃低有效位)
 *                    Y8 (丢弃低有效位 提取)
 *
 * MONO-12bits        MONO-8bit(丢弃低有效位)
 *                    Y8 (丢弃低有效位 提取)
 *
 * rawRGB-8bits       rawRGB-8bits
 *                    Y8 (使用RGB元素转换为Y)
 *
 * rawRGB-10bits      rawRGB-8bits(丢弃低有效位)
 *                    Y8 (使用rawRGB-8bits使用RGB元素转换为Y)
 *                    rawRGB-16bits(仅mipi接口支持)(按照raw格式收取,接收数据量扩大1.25倍,再通过软件扩展为rawRGB-16bits)
 *
 * rawRGB-12bits      rawRGB-8bits(丢弃低有效位)
 *                    Y8 (使用rawRGB-8bits使用RGB元素转换为Y)
 *                    rawRGB-16bits(仅mipi接口支持)(按照raw格式收取,接收数据量扩大1.5倍,再通过软件扩展为rawRGB-16bits)
 */
static struct format_mapping sensor_camera_format_map[] = {
    {SENSOR_PIXEL_FMT_Y8_1X8,            CAMERA_PIX_FMT_GREY},
    {SENSOR_PIXEL_FMT_Y10_1X10,          CAMERA_PIX_FMT_GREY},      /* 控制器裁剪为8-bits */
    {SENSOR_PIXEL_FMT_Y12_1X12,          CAMERA_PIX_FMT_GREY},      /* 控制器裁剪为8-bits */

    {SENSOR_PIXEL_FMT_SBGGR8_1X8,        CAMERA_PIX_FMT_SBGGR8},
    {SENSOR_PIXEL_FMT_SGBRG8_1X8,        CAMERA_PIX_FMT_SGBRG8},
    {SENSOR_PIXEL_FMT_SGRBG8_1X8,        CAMERA_PIX_FMT_SGRBG8},
    {SENSOR_PIXEL_FMT_SRGGB8_1X8,        CAMERA_PIX_FMT_SRGGB8},

    {SENSOR_PIXEL_FMT_SBGGR10_1X10,      CAMERA_PIX_FMT_SBGGR16},   /* 软件扩展为16-bits */
    {SENSOR_PIXEL_FMT_SGBRG10_1X10,      CAMERA_PIX_FMT_SGBRG16},   /* 软件扩展为16-bits */
    {SENSOR_PIXEL_FMT_SGRBG10_1X10,      CAMERA_PIX_FMT_SGRBG16},   /* 软件扩展为16-bits */
    {SENSOR_PIXEL_FMT_SRGGB10_1X10,      CAMERA_PIX_FMT_SRGGB16},   /* 软件扩展为16-bits */

    {SENSOR_PIXEL_FMT_SBGGR10_ALAW8_1X8, CAMERA_PIX_FMT_SBGGR8},    /* 控制器裁剪为8-bits */
    {SENSOR_PIXEL_FMT_SGBRG10_ALAW8_1X8, CAMERA_PIX_FMT_SGBRG8},    /* 控制器裁剪为8-bits */
    {SENSOR_PIXEL_FMT_SGRBG10_ALAW8_1X8, CAMERA_PIX_FMT_SGRBG8},    /* 控制器裁剪为8-bits */
    {SENSOR_PIXEL_FMT_SRGGB10_ALAW8_1X8, CAMERA_PIX_FMT_SRGGB8},    /* 控制器裁剪为8-bits */

    {SENSOR_PIXEL_FMT_SBGGR12_1X12,      CAMERA_PIX_FMT_SBGGR16},   /* 软件扩展为16-bits */
    {SENSOR_PIXEL_FMT_SGBRG12_1X12,      CAMERA_PIX_FMT_SGBRG16},   /* 软件扩展为16-bits */
    {SENSOR_PIXEL_FMT_SGRBG12_1X12,      CAMERA_PIX_FMT_SGRBG16},   /* 软件扩展为16-bits */
    {SENSOR_PIXEL_FMT_SRGGB12_1X12,      CAMERA_PIX_FMT_SRGGB16},   /* 软件扩展为16-bits */

    {SENSOR_PIXEL_FMT_SBGGR12_ALAW8_1X8, CAMERA_PIX_FMT_SBGGR8},    /* 控制器裁剪为8-bits */
    {SENSOR_PIXEL_FMT_SGBRG12_ALAW8_1X8, CAMERA_PIX_FMT_SGBRG8},    /* 控制器裁剪为8-bits */
    {SENSOR_PIXEL_FMT_SGRBG12_ALAW8_1X8, CAMERA_PIX_FMT_SGRBG8},    /* 控制器裁剪为8-bits */
    {SENSOR_PIXEL_FMT_SRGGB12_ALAW8_1X8, CAMERA_PIX_FMT_SRGGB8},    /* 控制器裁剪为8-bits */

    {SENSOR_PIXEL_FMT_UYVY8_2X8,         CAMERA_PIX_FMT_UYVY},
    {SENSOR_PIXEL_FMT_VYUY8_2X8,         CAMERA_PIX_FMT_VYUY},
    {SENSOR_PIXEL_FMT_YUYV8_2X8,         CAMERA_PIX_FMT_YUYV},
    {SENSOR_PIXEL_FMT_YVYU8_2X8,         CAMERA_PIX_FMT_YVYU},

    {SENSOR_PIXEL_FMT_RGB565_2X8_BE,     CAMERA_PIX_FMT_RGB565},
    {SENSOR_PIXEL_FMT_RGB565_2X8_LE,     CAMERA_PIX_FMT_RGB565},

    {SENSOR_PIXEL_FMT_RGB888_1X24,       CAMERA_PIX_FMT_RGB24},
    {SENSOR_PIXEL_FMT_RBG888_1X24,       CAMERA_PIX_FMT_RBG24},
    {SENSOR_PIXEL_FMT_BGR888_1X24,       CAMERA_PIX_FMT_BGR24},
    {SENSOR_PIXEL_FMT_GBR888_1X24,       CAMERA_PIX_FMT_GBR24},
};


static int sensor_attribute_check_init(struct sensor_attr *sensor)
{
    int ret = -EINVAL;

    error_if(!sensor->device_name);
    error_if(sensor->sensor_info.width < 64 || sensor->sensor_info.width > 2047);
    error_if(sensor->sensor_info.height < 64 || sensor->sensor_info.height > 2047);
    error_if(!sensor->ops.power_on);
    error_if(!sensor->ops.power_off);
    error_if(!sensor->ops.stream_on);
    error_if(!sensor->ops.stream_off);

    memset(sensor->info.name, 0x00, sizeof(sensor->info.name));
    memcpy(sensor->info.name, sensor->device_name, strlen(sensor->device_name));

    sensor->info.width =  sensor->sensor_info.width;
    sensor->info.height =  sensor->sensor_info.height;

    int i = 0;
    int size = ARRAY_SIZE(sensor_camera_format_map);
    for (i = 0; i < size; i++) {
        if (sensor->sensor_info.fmt == sensor_camera_format_map[i].sensor_fmt)
            break;
    }

    if (i >= size) {
        printk(KERN_ERR "attribute check: sensor data_fmt(0x%x) is NOT support.\n", sensor->sensor_info.fmt);
        goto unlock;
    }

    sensor->info.data_fmt = sensor_camera_format_map[i].camera_fmt;

    if (sensor->dbus_type == SENSOR_DATA_BUS_DVP) {
        /* DVP接口，只能处理输入8bit数据,不做默认调整 */
        if  ((sensor->info.data_fmt == CAMERA_PIX_FMT_SBGGR16) ||
            (sensor->info.data_fmt == CAMERA_PIX_FMT_SGBRG16) ||
            (sensor->info.data_fmt == CAMERA_PIX_FMT_SGRBG16) ||
            (sensor->info.data_fmt == CAMERA_PIX_FMT_SRGGB16) ) {
            printk(KERN_ERR "attribute check: DVP Unsupported > 8-bits interface. Please check sensor attribute config\n");
            goto unlock;
        }
    }

    if (sensor->dbus_type == SENSOR_DATA_BUS_MIPI) {
        int dma_output_y_mode = sensor->dma_mode;

        /* MIPI接口软件扩展为rawRGB-16bit,不支持Y8格式提取 */
        if  (dma_output_y_mode == SENSOR_DATA_DMA_MODE_GREY &&
            ((sensor->info.data_fmt == CAMERA_PIX_FMT_SBGGR16) ||
            (sensor->info.data_fmt == CAMERA_PIX_FMT_SGBRG16) ||
            (sensor->info.data_fmt == CAMERA_PIX_FMT_SGRBG16) ||
            (sensor->info.data_fmt == CAMERA_PIX_FMT_SRGGB16)) ) {
            printk(KERN_ERR "attribute check: MIPI Unsupported 10/12-bits interface padding to 16-bits but DMA out Y8\n");
            printk(KERN_ERR "Please check sensor attribute config\n");
            goto unlock;
        }
    }

    return 0;

unlock:
    return ret;
}

/******************************************************************************
 *
 * sensor调用用接口
 *
 *****************************************************************************/
int camera_register_sensor(struct sensor_attr *sensor)
{
    struct jz_cim_data *drv = &jz_cim_dev;
    int ret;

    assert(drv->is_finish > 0);
    assert(!drv->camera.sensor);

    ret = sensor_attribute_check_init(sensor);
    assert(ret == 0);

    mutex_lock(&drv->lock);

    if (drv->cam_mem_cnt < 1) {
        printk(KERN_ERR "camera device :mem cnt from(%d) fix to (2)\n", drv->cam_mem_cnt);
        drv->cam_mem_cnt = 2;
    }

    drv->camera.sensor = sensor;
    ret = cim_alloc_mem(sensor);
    if (ret) {
        drv->camera.sensor = NULL;
        goto unlock;
    }

    ret = misc_register(&drv->cim_mdev);
    assert(!ret);

    drv->camera.is_power_on = 0;
    drv->camera.is_stream_on = 0;

unlock:
    mutex_unlock(&drv->lock);

    return ret;
}

void camera_unregister_sensor(struct sensor_attr *sensor)
{
    struct jz_cim_data *drv = &jz_cim_dev;

    assert(drv->is_finish > 0);
    assert(drv->camera.sensor);
    assert(sensor == drv->camera.sensor);

    mutex_lock(&drv->lock);

    if (drv->camera.is_stream_on) {
        printk(KERN_ERR "cim : failed to unregister, when camera stream on!\n");
        return ;
    }

    if (drv->camera.is_power_on) {
        struct sensor_attr *attr = drv->camera.sensor;
        attr->ops.power_off();

        drv->camera.is_power_on = 0;
    }
    drv->camera.sensor = NULL;

    misc_deregister(&drv->cim_mdev);

    cim_free_mem();

    mutex_unlock(&drv->lock);
}

/*
 * 如果mclk_io的输出选择PC25，即24MHz旁路输出，则此处调整频率输出并不会生效
 */
void camera_enable_sensor_mclk(unsigned long clk_rate)
{
    struct jz_cim_data *drv = &jz_cim_dev;

    if (drv->mclk_io == GPIO_PC(25)) {
        if (clk_rate != 24 * 1000 * 1000) {
            printk(KERN_DEBUG "Warning:mclk io(GPIO_PC25) is exclk bypass,the freq is fixed 24MHz\n");
        }
    }

    clk_set_rate(drv->cim_mclk, clk_rate);
    clk_enable(drv->cim_mclk);
}

void camera_disable_sensor_mclk(void)
{
    struct jz_cim_data *drv = &jz_cim_dev;

    clk_disable(drv->cim_mclk);
}

static int jz_arch_camera_probe(struct platform_device *pdev)
{
    struct jz_cim_data *drv = &jz_cim_dev;
    int ret;

    drv->dev = &pdev->dev;
    ret = camera_mclk_gpio_init(drv->mclk_io);
    if (ret < 0) {
        printk(KERN_ERR "camera mclk gpio init failed\n");
        return -EIO;
    }

    drv->cim_mclk = clk_get(NULL, drv->cim_mclk_name);
    assert(!IS_ERR(drv->cim_mclk));
    assert(!clk_prepare(drv->cim_mclk));

    drv->cim_gate_clk = clk_get(NULL, drv->cim_gate_clk_name);
    assert(!IS_ERR(drv->cim_gate_clk));
    assert(!clk_prepare(drv->cim_gate_clk));

    memset(&drv->cim_mdev, 0x00, sizeof(struct miscdevice));
    drv->cim_mdev.minor = MISC_DYNAMIC_MINOR;
    drv->cim_mdev.name  = drv->device_name;
    drv->cim_mdev.fops  = &cim_misc_fops;


    init_waitqueue_head(&drv->waiter);

    mutex_init(&drv->lock);
    spin_lock_init(&drv->spinlock);

    ret = request_irq(drv->irq, cim_irq_handler, 0, drv->device_name, drv);
    assert(!ret);

    disable_irq(drv->irq);

    drv->is_finish = 1;

    return 0;
}

static int jz_arch_camera_remove(struct platform_device *pdev)
{
    struct jz_cim_data *drv = &jz_cim_dev;

    if (!drv->is_finish)
        return 0;

    drv->is_finish = 0;

    free_irq(drv->irq, drv);
    clk_put(drv->cim_mclk);
    clk_put(drv->cim_gate_clk);
    camera_mclk_gpio_deinit(drv->mclk_io);

    return 0;
}

static struct platform_driver jz_arch_camera_driver = {
    .probe = jz_arch_camera_probe,
    .remove = jz_arch_camera_remove,
    .driver = {
        .owner = THIS_MODULE,
        .name = "jz-arch-camera",
    },
};

/* stop no dev release warning */
static void jz_arch_camera_dev_release(struct device *dev){}

struct platform_device jz_arch_camera_device = {
    .name = "jz-arch-camera",
    .dev  = {
        .release = jz_arch_camera_dev_release,
    },
};

static int __init jz_arch_camera_init(void)
{
    int ret = platform_device_register(&jz_arch_camera_device);
    if (ret)
        return ret;

    return platform_driver_register(&jz_arch_camera_driver);
}
module_init(jz_arch_camera_init);

static void __exit jz_arch_camera_exit(void)
{
    platform_device_unregister(&jz_arch_camera_device);

    platform_driver_unregister(&jz_arch_camera_driver);
}
module_exit(jz_arch_camera_exit);

EXPORT_SYMBOL(camera_register_sensor);
EXPORT_SYMBOL(camera_unregister_sensor);

EXPORT_SYMBOL(camera_enable_sensor_mclk);
EXPORT_SYMBOL(camera_disable_sensor_mclk);

MODULE_DESCRIPTION("JZ x1600 cim driver");
MODULE_LICENSE("GPL");
