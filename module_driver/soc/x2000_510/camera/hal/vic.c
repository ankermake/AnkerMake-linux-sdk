/*
 * Copyright (C) 2020 Ingenic Semiconductor Co., Ltd.
 *
 * Camera Driver for the Ingenic VIC controller
 *
 */
#include <linux/module.h>
#include <linux/miscdevice.h>
#include <linux/interrupt.h>
#include <linux/ioctl.h>
#include <linux/list.h>
#include <linux/clk.h>
#include <linux/irq.h>
#include <linux/slab.h>
#include <linux/dma-mapping.h>
#include <linux/delay.h>
#include <linux/clk-provider.h>

#include <common.h>
#include <bit_field.h>
#include <utils/clock.h>

#include "camera_gpio.h"
#include "vic.h"
#include "csi.h"
#include "../isp/isp.h"
#include "dsys.h"
#include "../isp/vic_channel_tiziano.h"
#include "../vic/vic_channel_mem.h"
#include "../cim/cim.h"


struct jz_camera_data {
    int index;
    int is_enable;
    int is_finish;
    int mclk_io;                    /* MCLK输出管脚选择: PC15(3.3V) / PE24(1.8V) */

    int is_isp_enable;              /* ISP Enable功能
                                     * =1, VIC ---> ISP
                                     * =0, VIC ---> DDR
                                     */
    const char *cim_gate_clk_name;  /* CIM */
    const char *isp_power_clk_name;
    const char *isp_gate_clk_name;
    const char *isp_div_clk_name;   /* ISP0/1 共用 */

    struct clk *mclk_div;           /* VIC/CIM 可共用 */
    struct clk *cim_gate_clk;       /* CIM */
    struct clk *isp_div;
    struct clk *isp_power_clk;
    struct clk *isp_gate_clk;
    unsigned long isp_clk_rate;

    /* Camera Device */
    char *device_name;              /* 设备节点名字 */
    unsigned int cam_mem_cnt;       /* 循环buff个数(针对VIC MEM有效, 经过ISP该参数无效) */
    struct camera_device camera;

    struct mutex lock;
    struct spinlock spinlock;
};

static struct jz_camera_data jz_camera_dev[3] = {
    /* VIC0 */
    {
        .index                  = 0,
        .is_enable              = 0,
        .mclk_io                = -1,
        .isp_div_clk_name       = "div_isp",
        .isp_power_clk_name     = "power_isp0",
        .isp_gate_clk_name      = "gate_isp0",
        .is_isp_enable          = 0,
        .cam_mem_cnt            = 2,
    },
    /* VIC1 */
    {
        .index                  = 1,
        .is_enable              = 0,
        .mclk_io                = -1,
        .isp_div_clk_name       = "div_isp",
        .isp_power_clk_name     = "power_isp1",
        .isp_gate_clk_name      = "gate_isp1",
        .is_isp_enable          = 0,
        .cam_mem_cnt            = 2,
    },

    /*************************************************/
    /* CIM information */
    {
        .index                  = 2,
        .is_enable              = 0,
        .mclk_io                = -1,
        .cim_gate_clk_name      = "gate_cim",
        .cam_mem_cnt            = 2,
    },
};

/* VIC Controller */
module_param_named(vic0_is_enable,      jz_camera_dev[0].is_enable,    int, 0644);
module_param_named(vic0_is_isp_enable,  jz_camera_dev[0].is_isp_enable,int, 0444);
module_param_named(vic0_frame_nums,     jz_camera_dev[0].cam_mem_cnt,  int, 0644);
module_param_gpio_named(vic0_mclk_io,   jz_camera_dev[0].mclk_io, 0644);

module_param_named(vic1_is_enable,      jz_camera_dev[1].is_enable,    int, 0644);
module_param_named(vic1_is_isp_enable,  jz_camera_dev[1].is_isp_enable,int, 0444);
module_param_named(vic1_frame_nums,     jz_camera_dev[1].cam_mem_cnt,  int, 0644);
module_param_gpio_named(vic1_mclk_io,   jz_camera_dev[1].mclk_io, 0644);

/* CIM Controller */
module_param_named(cim_is_enable,       jz_camera_dev[2].is_enable,    int, 0644);
module_param_named(cim_frame_nums,      jz_camera_dev[2].cam_mem_cnt,  int, 0644);
module_param_gpio_named(cim_mclk_io,    jz_camera_dev[2].mclk_io, 0644);



/*
 * 输入为raw8时控制器输出会变成raw16，当我们想得到raw8时，
 * 我们就把raw8数据当yuv数据来处理，将输入输出格式均设为yuv422，
 * 因为输入输出yuv422格式时不会改变原数据，这样我们就可以得到原封不动的raw8数据了。
*/
unsigned int is_output_y8(int index, struct sensor_attr *attr)
{
    if (attr->dbus_type == SENSOR_DATA_BUS_DVP)
        return ( !jz_camera_dev[index].is_isp_enable && \
                attr->dvp.data_fmt == DVP_RAW8 &&       \
                (sensor_fmt_is_8BIT(attr->sensor_info.fmt)) );

    if (attr->dbus_type == SENSOR_DATA_BUS_MIPI)
        return ( !jz_camera_dev[index].is_isp_enable && \
                attr->mipi.data_fmt == MIPI_RAW8 &&     \
                (sensor_fmt_is_8BIT(attr->sensor_info.fmt)) );

    return 0;
}

/*
 * 输入为yuv422时 DMA控制器可以重新排列输出的顺序,
 * 所以YUV422输入可以选择输出NV12/NV21/Grey格式
*/
unsigned int is_output_yuv422(int index, struct sensor_attr *attr)
{
    if (attr->dbus_type == SENSOR_DATA_BUS_DVP)
        return ( !jz_camera_dev[index].is_isp_enable &&             \
                (sensor_fmt_is_YUV422(attr->sensor_info.fmt)) &&    \
                (attr->dvp.data_fmt == DVP_YUV422 || attr->dvp.data_fmt == DVP_YUV422_8BIT) );

    if (attr->dbus_type == SENSOR_DATA_BUS_MIPI)
        return ( !jz_camera_dev[index].is_isp_enable &&     \
                attr->mipi.data_fmt == MIPI_YUV422 &&       \
                (sensor_fmt_is_YUV422(attr->sensor_info.fmt)) );

    return 0;
}

static void vic_start(int index)
{
    /* start vic 控制器 */
    vic_set_bit(index, VIC_CONTROL, VIC_START, 1);
}

void vic_reset(unsigned int index)
{
    /* reset vic 控制器 */
    vic_set_bit(index, VIC_CONTROL, VIC_GLB_RST, 1);
}

static void vic_dma_reset(int index)
{
    /* reset dma */
    vic_write_reg(index, VIC_DMA_RESET, 1);
}

static void vic_register_enable(int index)
{
    /* VIC 开始初始化 */
    vic_set_bit(index ,VIC_CONTROL, VIC_REG_ENABLE, 1);
#if 0
    int timeout = 3000;
    while (vic_get_bit(index ,VIC_CONTROL, VIC_REG_ENABLE)) {
        if (--timeout == 0) {
            printk(KERN_ERR "timeout while wait vic_reg_enable: %x\n", vic_read_reg(index, VIC_CONTROL));
            break;
        }
    }
#endif
}

static void vic_data_path_select_route(int index, int route)
{
    if (route) {
        /* ISP Route */
        vic_set_bit(index, VIC_CONTROL_DMA_ROUTE, VC_DMA_ROUTE_dma_out, 0);
        vic_set_bit(index, VIC_CONTROL_TIZIANO_ROUTE, VC_TIZIANO_ROUTE_isp_out, 1);
    } else {
        /* DMA Route */
        vic_set_bit(index, VIC_CONTROL_TIZIANO_ROUTE, VC_TIZIANO_ROUTE_isp_out, 0);
        vic_set_bit(index, VIC_CONTROL_DMA_ROUTE, VC_DMA_ROUTE_dma_out, 1);
    }
}


static void vic_init_dvp_timing(int index, struct sensor_attr *attr)
{
    unsigned long vic_input_dvp = vic_read_reg(index, VIC_INPUT_DVP);
    unsigned long yuv_data_order = attr->dvp.yuv_data_order;

    if (is_output_y8(index, attr))
        set_bit_field(&vic_input_dvp, DVP_DATA_FORMAT, 6);
    else if (attr->dvp.data_fmt <= DVP_RAW12)
        set_bit_field(&vic_input_dvp, DVP_DATA_FORMAT, attr->dvp.data_fmt);
    else if (attr->dvp.data_fmt == DVP_YUV422)
        set_bit_field(&vic_input_dvp, DVP_DATA_FORMAT, 6); // YUV422(8bit IO)

    if (is_output_y8(index, attr))  // 不改变四个raw8的先后顺序
        yuv_data_order = order_1_2_3_4;

    set_bit_field(&vic_input_dvp, YUV_DATA_ORDER, yuv_data_order);
    set_bit_field(&vic_input_dvp, DVP_TIMING_MODE, attr->dvp.timing_mode);
    set_bit_field(&vic_input_dvp, HSYNC_POLAR, attr->dvp.hsync_polarity);
    set_bit_field(&vic_input_dvp, VSYNC_POLAR, attr->dvp.vsync_polarity);
    set_bit_field(&vic_input_dvp, INTERLACE_EN, attr->dvp.img_scan_mode);

    if (attr->dvp.gpio_mode == DVP_PA_HIGH_8BIT ||
            attr->dvp.gpio_mode == DVP_PA_HIGH_10BIT)
        set_bit_field(&vic_input_dvp, DVP_RAW_ALIGN, 1);
    else
        set_bit_field(&vic_input_dvp, DVP_RAW_ALIGN, 0);

    vic_write_reg(index, VIC_INPUT_DVP, vic_input_dvp);

    unsigned long vic_ctrl_delay;
    set_bit_field(&vic_ctrl_delay, VC_CONTROL_delay_hdelay, 1);
    set_bit_field(&vic_ctrl_delay, VC_CONTROL_delay_vdelay, 1);
    vic_write_reg(index, VIC_CONTROL_DELAY, vic_ctrl_delay);
}


static void vic_init_mipi_timing(int index, struct sensor_attr *attr)
{
    unsigned long horizontal_resolution = attr->sensor_info.width;

    if (is_output_y8(index, attr)) {
        vic_write_reg(index, VIC_INPUT_MIPI, MIPI_YUV422);
        horizontal_resolution /= 2;
    } else
        vic_write_reg(index, VIC_INPUT_MIPI, attr->mipi.data_fmt);

    int width_4byte;
    int pixel_wdith;

    switch (attr->mipi.data_fmt) {
    case MIPI_RAW8:
        pixel_wdith = 8;
        break;
    case MIPI_RAW10:
        pixel_wdith = 10;
        break;
    case MIPI_RAW12:
        pixel_wdith = 12;
        break;
    default:
        pixel_wdith = 8;
        break;
    }

    /* 每行前有0个无效像素, 每行之后有0个无效像素 */
    width_4byte = ((horizontal_resolution + 0 + 0) * pixel_wdith + 31) / 32;
    vic_write_reg(index, MIPI_ALL_WIDTH_4BYTE, width_4byte);

    unsigned long hcrop_ch0;
    set_bit_field(&hcrop_ch0, MIPI_HCROP_CHO_all_image_width, horizontal_resolution);
    set_bit_field(&hcrop_ch0, MIPI_HCROP_CHO_start_pixel, 0);
    vic_write_reg(index, MIPI_HCROP_CH0, hcrop_ch0);

    unsigned long vic_ctrl_delay;
    set_bit_field(&vic_ctrl_delay, VC_CONTROL_delay_hdelay, 10);
    set_bit_field(&vic_ctrl_delay, VC_CONTROL_delay_vdelay, 10);
    vic_write_reg(index, VIC_CONTROL_DELAY, vic_ctrl_delay);
}

static void vic_init_common_setting(int index, struct sensor_attr *attr)
{
    unsigned long resolution = 0;
    unsigned long horizontal_resolution = attr->sensor_info.width;

    /*
     * sensor输出的图像数据是raw8的，但我们是使用yuv422的格式输入和输出的，
     * 因为raw8一个像素1个字节 yuv422一个像素占2个字节。
     * 所以填入寄存器的像素点为raw8像素点的1/2。
    */
    if (is_output_y8(index, attr))
        horizontal_resolution /= 2;

    set_bit_field(&resolution, HORIZONTAL_RESOLUTION, horizontal_resolution);
    set_bit_field(&resolution, VERTICAL_RESOLUTION, attr->sensor_info.height);
    vic_write_reg(index ,VIC_RESOLUTION, resolution);

    int vic_interface = 0;
    switch (attr->dbus_type) {
    case SENSOR_DATA_BUS_BT656:
        vic_interface = 0;
        break;
    case SENSOR_DATA_BUS_BT601:
        vic_interface = 1;
        break;
    case SENSOR_DATA_BUS_MIPI:
        vic_interface = 2;
        break;
    case SENSOR_DATA_BUS_DVP:
        vic_interface = 3;
        break;
    case SENSOR_DATA_BUS_BT1120:
        vic_interface = 4;
        break;
    default:
        printk(KERN_ERR "vic%d unknown dbus_type: %d\n",  index, attr->dbus_type);
    }
    vic_write_reg(index ,VIC_INPUT_INTF, vic_interface);
}

static void init_dvp_dma(int index, struct sensor_attr *attr)
{
    struct jz_camera_data *drv = &jz_camera_dev[index];
    unsigned long dma_resolution = 0;
    unsigned long horizontal_resolution = attr->sensor_info.width;

    if (is_output_y8(index, attr))
        horizontal_resolution /= 2;

    set_bit_field(&dma_resolution, DMA_HORIZONTAL_RESOLUTION, horizontal_resolution);
    set_bit_field(&dma_resolution, DMA_VERTICAL_RESOLUTION, attr->sensor_info.height);
    vic_write_reg(index ,VIC_DMA_RESOLUTION, dma_resolution);

    unsigned int base_mode = 0;
    unsigned int y_stride = 0;
    unsigned int uv_stride = 0;
    unsigned int horizon_time = 0;

    switch (attr->dvp.data_fmt) {
    case DVP_RAW8:
    case DVP_RAW10:
    case DVP_RAW12:
        base_mode = 0;
        y_stride = attr->sensor_info.width * 2;
        horizon_time = attr->sensor_info.width;
        if (is_output_y8(index, attr)) {
            base_mode = 3;
            y_stride = attr->sensor_info.width;
        }
        break;

    case DVP_YUV422:
        if (attr->info.data_fmt == CAMERA_PIX_FMT_GREY) {
            base_mode = 6;
            y_stride = attr->sensor_info.width;
        } else if (attr->info.data_fmt == CAMERA_PIX_FMT_NV12) {
            base_mode = 6;
            uv_stride = attr->sensor_info.width;
            y_stride = attr->sensor_info.width;
        } else if (attr->info.data_fmt == CAMERA_PIX_FMT_NV21) {
            base_mode = 7;
            uv_stride = attr->sensor_info.width;
            y_stride = attr->sensor_info.width;
        } else {
            base_mode = 3;
            y_stride = attr->sensor_info.width * 2;
        }

        horizon_time = attr->sensor_info.width * 2;
        break;

    default:
        break;
    }

    vic_set_bit(index, VIC_IN_HOR_PARA0, HACT_NUM, horizon_time);
    vic_write_reg(index, VIC_DMA_Y_CH_LINE_STRIDE, y_stride);
    vic_write_reg(index, VIC_DMA_UV_CH_LINE_STRIDE, uv_stride);

    unsigned long dma_configure = vic_read_reg(index ,VIC_DMA_CONFIGURE);
    //set_bit_field(&dma_configure, Dma_en, 1);
    set_bit_field(&dma_configure, Buffer_number, 2 - 1);
    set_bit_field(&dma_configure, Base_mode, base_mode);
    set_bit_field(&dma_configure, Yuv422_order, 2);
    vic_write_reg(index ,VIC_DMA_CONFIGURE, dma_configure);

    /* default DMA Route */
    vic_data_path_select_route(index, drv->is_isp_enable);
}

static void init_mipi_dma(int index, struct sensor_attr *attr)
{
    struct jz_camera_data *drv = &jz_camera_dev[index];
    unsigned long dma_resolution = 0;
    unsigned long horizontal_resolution = attr->sensor_info.width;

    if (is_output_y8(index, attr))
        horizontal_resolution /= 2;

    set_bit_field(&dma_resolution, DMA_HORIZONTAL_RESOLUTION, horizontal_resolution);
    set_bit_field(&dma_resolution, DMA_VERTICAL_RESOLUTION, attr->sensor_info.height);
    vic_write_reg(index ,VIC_DMA_RESOLUTION, dma_resolution);

    unsigned int base_mode = 0;
    unsigned int y_stride = 0;
    unsigned int uv_stride = 0;
    unsigned int yuv422_order_mode = 0;

    switch (attr->mipi.data_fmt) {
    case MIPI_RAW8:
    case MIPI_RAW10:
    case MIPI_RAW12:
        base_mode = 0;
        y_stride = horizontal_resolution * 2;

        if (is_output_y8(index, attr)) {
            base_mode = 3;          /* YUV422 packey */
            yuv422_order_mode = 3;  /* =3, U1Y1V1Y2 */
            y_stride = horizontal_resolution * 2;
        }
        break;
    case MIPI_YUV422:
        if (attr->info.data_fmt == CAMERA_PIX_FMT_GREY) {
            base_mode = 6;
            y_stride = attr->info.width;
        } else if (attr->info.data_fmt == CAMERA_PIX_FMT_NV12) {
            base_mode = 7; //maybe spec is err
            uv_stride = attr->info.width;
            y_stride = attr->info.width;
        } else if (attr->info.data_fmt == CAMERA_PIX_FMT_NV21) {
            base_mode = 6;
            uv_stride = attr->info.width;
            y_stride = attr->info.width;
        } else {
            base_mode = 3;
            y_stride = attr->info.width * 2;
        }

    default:
        break;
    }

    vic_write_reg(index ,VIC_DMA_Y_CH_LINE_STRIDE, y_stride);
    vic_write_reg(index ,VIC_DMA_UV_CH_LINE_STRIDE, uv_stride);

    unsigned long dma_configure = vic_read_reg(index, VIC_DMA_CONFIGURE);
    //set_bit_field(&dma_configure, Dma_en, 1);
    set_bit_field(&dma_configure, Buffer_number, 2 - 1);
    set_bit_field(&dma_configure, Base_mode, base_mode);
    set_bit_field(&dma_configure, Yuv422_order, yuv422_order_mode);

    vic_write_reg(index ,VIC_DMA_CONFIGURE, dma_configure);

    /* default DMA Route */
    vic_data_path_select_route(index, drv->is_isp_enable);
}

static void init_dvp_irq(int index)
{
    unsigned long vic_int_mask = 0;

    set_bit_field(&vic_int_mask, VIC_FRM_START, 1);
    set_bit_field(&vic_int_mask, VIC_FRM_RST, 1);

    vic_write_reg(index, VIC_INT_CLR, vic_int_mask);
    vic_write_reg(index, VIC_INT_MASK, vic_int_mask);
}

static void init_mipi_irq(int index)
{
    unsigned long vic_int_mask = 0xFFFFF;

    set_bit_field(&vic_int_mask, VIC_DONE, 0);
    set_bit_field(&vic_int_mask, MIPI_VCOMP_ERR_CH0, 0);
    set_bit_field(&vic_int_mask, MIPI_HCOMP_ERR_CH0, 0);
    set_bit_field(&vic_int_mask, DMA_FRD, 0);
    set_bit_field(&vic_int_mask, VIC_HVRES_ERR, 0);
    set_bit_field(&vic_int_mask, VIC_FRM_START, 0);
    set_bit_field(&vic_int_mask, VIC_FRD, 0);

    vic_write_reg(index, VIC_INT_CLR, vic_int_mask);
    vic_write_reg(index, VIC_INT_MASK, vic_int_mask);
}

static int vic_isp_div_clock_enable(int index)
{
    struct jz_camera_data *drv = &jz_camera_dev[index];

    if ( !__clk_is_enabled(drv->isp_div) ) {
        clk_set_rate(drv->isp_div, drv->isp_clk_rate);

    } else  if (drv->isp_clk_rate != clk_get_rate(drv->isp_div)) {
        printk(KERN_ERR "vic%d already enable isp clock(%ld) not change to %ld\n",  \
                !index, clk_get_rate(drv->isp_div), drv->isp_clk_rate);
    }

    clk_enable(drv->isp_div);

    return 0;
}

static int vic_isp_div_clock_disable(int index)
{
    struct jz_camera_data *drv = &jz_camera_dev[index];

    clk_disable(drv->isp_div);

    return 0;
}

void vic_dump_reg(int index)
{
    printk("==========dump vic%d register============\n", index);

    printk("VIC_CONTROL                 : 0x%08x\n", vic_read_reg(index, VIC_CONTROL));
    printk("VIC_RESOLUTION              : 0x%08x\n", vic_read_reg(index, VIC_RESOLUTION));
    printk("VIC_FRM_ECC                 : 0x%08x\n", vic_read_reg(index, VIC_FRM_ECC));
    printk("VIC_INPUT_INTF              : 0x%08x\n", vic_read_reg(index, VIC_INPUT_INTF));
    printk("VIC_INPUT_DVP               : 0x%08x\n", vic_read_reg(index, VIC_INPUT_DVP));
    printk("VIC_INPUT_MIPI              : 0x%08x\n", vic_read_reg(index, VIC_INPUT_MIPI));
    printk("VIC_IN_HOR_PARA0            : 0x%08x\n", vic_read_reg(index, VIC_IN_HOR_PARA0));
    printk("VIC_IN_HOR_PARA1            : 0x%08x\n", vic_read_reg(index, VIC_IN_HOR_PARA1));
    printk("VIC_BK_CB_CTRL              : 0x%08x\n", vic_read_reg(index, VIC_BK_CB_CTRL));
    printk("VIC_BK_CB_BLK               : 0x%08x\n", vic_read_reg(index, VIC_BK_CB_BLK));
    printk("VIC_IN_VER_PARA0            : 0x%08x\n", vic_read_reg(index, VIC_INPUT_VPARA0));
    printk("VIC_IN_VER_PARA1            : 0x%08x\n", vic_read_reg(index, VIC_INPUT_VPARA1));
    printk("VIC_IN_VER_PARA2            : 0x%08x\n", vic_read_reg(index, VIC_INPUT_VPARA2));
    printk("VIC_IN_VER_PARA3            : 0x%08x\n", vic_read_reg(index, VIC_INPUT_VPARA3));
    printk("VIC_VLD_LINE_SAV            : 0x%08x\n", vic_read_reg(index, VIC_VLD_LINE_SAV));
    printk("VIC_VLD_LINE_EAV            : 0x%08x\n", vic_read_reg(index, VIC_VLD_LINE_EAV));
    printk("VIC_VLD_FRM_SAV             : 0x%08x\n", vic_read_reg(index, VIC_VLD_FRM_SAV));
    printk("VIC_VLD_FRM_EAV             : 0x%08x\n", vic_read_reg(index, VIC_VLD_FRM_EAV));
    printk("VIC_VC_CONTROL_FSM          : 0x%08x\n", vic_read_reg(index, VIC_VC_CONTROL_FSM));
    printk("VIC_VC_CONTROL_CH0_PIX      : 0x%08x\n", vic_read_reg(index, VIC_VC_CONTROL_CH0_PIX));
    printk("VIC_VC_CONTROL_CH1_PIX      : 0x%08x\n", vic_read_reg(index, VIC_VC_CONTROL_CH1_PIX));
    printk("VIC_VC_CONTROL_CH2_PIX      : 0x%08x\n", vic_read_reg(index, VIC_VC_CONTROL_CH2_PIX));
    printk("VIC_VC_CONTROL_CH3_PIX      : 0x%08x\n", vic_read_reg(index, VIC_VC_CONTROL_CH3_PIX));
    printk("VIC_VC_CONTROL_CH0_LINE     : 0x%08x\n", vic_read_reg(index, VIC_VC_CONTROL_CH0_LINE));
    printk("VIC_VC_CONTROL_CH1_LINE     : 0x%08x\n", vic_read_reg(index, VIC_VC_CONTROL_CH1_LINE));
    printk("VIC_VC_CONTROL_CH2_LINE     : 0x%08x\n", vic_read_reg(index, VIC_VC_CONTROL_CH2_LINE));
    printk("VIC_VC_CONTROL_CH3_LINE     : 0x%08x\n", vic_read_reg(index, VIC_VC_CONTROL_CH3_LINE));
    printk("VIC_VC_CONTROL_FIFO_USE     : 0x%08x\n", vic_read_reg(index, VIC_VC_CONTROL_FIFO_USE));
    printk("VIC_CB_1ST                  : 0x%08x\n", vic_read_reg(index, VIC_CB_1ST));
    printk("VIC_CB_2ND                  : 0x%08x\n", vic_read_reg(index, VIC_CB_2ND));
    printk("VIC_CB_3RD                  : 0x%08x\n", vic_read_reg(index, VIC_CB_3RD));
    printk("VIC_CB_4TH                  : 0x%08x\n", vic_read_reg(index, VIC_CB_4TH));
    printk("VIC_CB_5TH                  : 0x%08x\n", vic_read_reg(index, VIC_CB_5TH));
    printk("VIC_CB_6TH                  : 0x%08x\n", vic_read_reg(index, VIC_CB_6TH));
    printk("VIC_CB_7TH                  : 0x%08x\n", vic_read_reg(index, VIC_CB_7TH));
    printk("VIC_CB_8TH                  : 0x%08x\n", vic_read_reg(index, VIC_CB_8TH));
    printk("VIC_CB2_1ST                 : 0x%08x\n", vic_read_reg(index, VIC_CB2_1ST));
    printk("VIC_CB2_2ND                 : 0x%08x\n", vic_read_reg(index, VIC_CB2_2ND));
    printk("VIC_CB2_3RD                 : 0x%08x\n", vic_read_reg(index, VIC_CB2_3RD));
    printk("VIC_CB2_4TH                 : 0x%08x\n", vic_read_reg(index, VIC_CB2_4TH));
    printk("VIC_CB2_5TH                 : 0x%08x\n", vic_read_reg(index, VIC_CB2_5TH));
    printk("VIC_CB2_6TH                 : 0x%08x\n", vic_read_reg(index, VIC_CB2_6TH));
    printk("VIC_CB2_7TH                 : 0x%08x\n", vic_read_reg(index, VIC_CB2_7TH));
    printk("VIC_CB2_8TH                 : 0x%08x\n", vic_read_reg(index, VIC_CB2_8TH));
    printk("MIPI_ALL_WIDTH_4BYTE        : 0x%08x\n", vic_read_reg(index, MIPI_ALL_WIDTH_4BYTE));
    printk("MIPI_VCROP_DEL01            : 0x%08x\n", vic_read_reg(index, MIPI_VCROP_DEL01));
    printk("MIPI_SENSOR_CONTROL         : 0x%08x\n", vic_read_reg(index, MIPI_SENSOR_CONTROL));
    printk("MIPI_HCROP_CH0              : 0x%08x\n", vic_read_reg(index, MIPI_HCROP_CH0));
    printk("MIPI_VCROP_SHADOW_CFG       : 0x%08x\n", vic_read_reg(index, MIPI_VCROP_SHADOW_CFG));
    printk("VIC_CONTROL_LIMIT           : 0x%08x\n", vic_read_reg(index, VIC_CONTROL_LIMIT));
    printk("VIC_CONTROL_DELAY           : 0x%08x\n", vic_read_reg(index, VIC_CONTROL_DELAY));
    printk("VIC_CONTROL_TIZIANO_ROUTE   : 0x%08x\n", vic_read_reg(index, VIC_CONTROL_TIZIANO_ROUTE));
    printk("VIC_CONTROL_DMA_ROUTE       : 0x%08x\n", vic_read_reg(index, VIC_CONTROL_DMA_ROUTE));
    printk("VIC_INT_STA                 : 0x%08x\n", vic_read_reg(index, VIC_INT_STA));
    printk("VIC_INT_MASK                : 0x%08x\n", vic_read_reg(index, VIC_INT_MASK));
    printk("VIC_INT_CLR                 : 0x%08x\n", vic_read_reg(index, VIC_INT_CLR));

    printk("VIC_DMA_CONFIGURE           : 0x%08x\n", vic_read_reg(index, VIC_DMA_CONFIGURE));
    printk("VIC_DMA_RESOLUTION          : 0x%08x\n", vic_read_reg(index, VIC_DMA_RESOLUTION));
    printk("VIC_DMA_RESET               : 0x%08x\n", vic_read_reg(index, VIC_DMA_RESET));
    printk("DMA_Y_CH_LINE_STRIDE        : 0x%08x\n", vic_read_reg(index, VIC_DMA_Y_CH_LINE_STRIDE));
    printk("VIC_DMA_Y_CH_BUF0_ADDR      : 0x%08x\n", vic_read_reg(index, VIC_DMA_Y_CH_BUF0_ADDR));
    printk("VIC_DMA_Y_CH_BUF1_ADDR      : 0x%08x\n", vic_read_reg(index, VIC_DMA_Y_CH_BUF1_ADDR));
    printk("VIC_DMA_Y_CH_BUF2_ADDR      : 0x%08x\n", vic_read_reg(index, VIC_DMA_Y_CH_BUF2_ADDR));
    printk("VIC_DMA_Y_CH_BUF3_ADDR      : 0x%08x\n", vic_read_reg(index, VIC_DMA_Y_CH_BUF3_ADDR));
    printk("VIC_DMA_Y_CH_BUF4_ADDR      : 0x%08x\n", vic_read_reg(index, VIC_DMA_Y_CH_BUF4_ADDR));
    printk("VIC_DMA_UV_CH_LINE_STRIDE   : 0x%08x\n", vic_read_reg(index, VIC_DMA_UV_CH_LINE_STRIDE));
    printk("VIC_DMA_UV_CH_BUF0_ADDR     : 0x%08x\n", vic_read_reg(index, VIC_DMA_UV_CH_BUF0_ADDR));
    printk("VIC_DMA_UV_CH_BUF1_ADDR     : 0x%08x\n", vic_read_reg(index, VIC_DMA_UV_CH_BUF1_ADDR));
    printk("VIC_DMA_UV_CH_BUF2_ADDR     : 0x%08x\n", vic_read_reg(index, VIC_DMA_UV_CH_BUF2_ADDR));
    printk("VIC_DMA_UV_CH_BUF3_ADDR     : 0x%08x\n", vic_read_reg(index, VIC_DMA_UV_CH_BUF3_ADDR));
    printk("VIC_DMA_UV_CH_BUF4_ADDR     : 0x%08x\n", vic_read_reg(index, VIC_DMA_UV_CH_BUF4_ADDR));
    printk("=========================================\n");
}

static void vic_dvp_init(int index, struct sensor_attr *attr)
{
    assert_range(attr->sensor_info.width, 1, 2048);
    assert_range(attr->sensor_info.height, 1, 2048);
    assert_range(attr->dvp.data_fmt, DVP_RAW8, DVP_YUV422);
    assert(attr->dbus_type == SENSOR_DATA_BUS_DVP);
    assert(attr->dvp.timing_mode == DVP_HREF_MODE);

    vic_reset(index);

    vic_init_common_setting(index, attr);

    vic_init_dvp_timing(index, attr);

    vic_dma_reset(index);

    init_dvp_dma(index, attr);

    init_dvp_irq(index);

    vic_register_enable(index);

    vic_start(index);

    //vic_dump_reg(index);
}

static void vic_mipi_init(int index, struct sensor_attr *attr)
{
    assert_range(attr->sensor_info.width, 1, 3840);
    assert_range(attr->sensor_info.height, 1, 2560);
    assert_range(attr->mipi.data_fmt, MIPI_RAW8, MIPI_YUV422);
    assert_range(attr->mipi.lanes, 1, 4);
    assert(attr->dbus_type == SENSOR_DATA_BUS_MIPI);

    vic_reset(index);

    vic_init_common_setting(index, attr);

    vic_init_mipi_timing(index, attr);

    vic_dma_reset(index);

    init_mipi_dma(index, attr);

    init_mipi_irq(index);

    int csi_ret = mipi_csi_phy_initialization(index, &attr->mipi);
    assert(csi_ret >= 0);

    vic_register_enable(index);

    vic_start(index);

    //vic_dump_reg(index);
}

static void vic_hal_power_on(int index, struct sensor_attr *attr)
{
    if (attr->dbus_type == SENSOR_DATA_BUS_DVP)
        vic_dvp_init(index, attr);
    else if (attr->dbus_type == SENSOR_DATA_BUS_MIPI)
        vic_mipi_init(index, attr);
}

static void vic_hal_power_off(int index, struct sensor_attr *attr)
{
    vic_reset(index);
    usleep_range(1000, 1000);

    if (attr->dbus_type == SENSOR_DATA_BUS_MIPI)
        mipi_csi_phy_stop(index);
}

unsigned int vic_enable_ts(int index, int ms_ch, unsigned int offset)
{
    struct jz_camera_data *drv = &jz_camera_dev[index];
    unsigned int ts_enbale;
    unsigned int ts_counter = 250;
    unsigned long isp_clk_rate;

    assert_range(ms_ch, 0, 2);

    /*
     * 1.counter max 255
     * 2.offset need 64bit align
     * 3.vic timestamp 64bit, hight 32bit timestamp, low 32bit vic status
     * 4.vic1 timestamp is from vic0
     */

    ts_enbale = vic_read_reg(index, VIC_TS_ENABLE);
    ts_enbale |= TS_COUNTER_EN;

    vic_write_reg(index, VIC_TS_COUNTER, ts_counter - 1);

    switch(ms_ch) {
        case 0:
            vic_write_reg(index, VIC_TS_MS_CH0_OFFSET, offset);
            ts_enbale |= TS_MS_CH0_EN;
            vic_write_reg(index, VIC_TS_ENABLE, ts_enbale);
            break;
        case 1:
            vic_write_reg(index, VIC_TS_MS_CH1_OFFSET, offset);
            ts_enbale |= TS_MS_CH1_EN;
            vic_write_reg(index, VIC_TS_ENABLE, ts_enbale);
            break;
        case 2:
            vic_write_reg(index, VIC_TS_MS_CH2_OFFSET, offset);
            ts_enbale |= TS_MS_CH2_EN;
            vic_write_reg(index, VIC_TS_ENABLE, ts_enbale);
            break;
    }

    isp_clk_rate = clk_get_rate(drv->isp_div);

    /* 返回每秒计数数 */
    return isp_clk_rate / ts_counter;
}

void vic_disable_ts(int index)
{
    vic_write_reg(index,  VIC_TS_COUNTER, 0);
    vic_write_reg(index,  VIC_TS_ENABLE, 0);
}

int vic_stream_on(int index, struct sensor_attr *attr)
{
    struct jz_camera_data *drv = &jz_camera_dev[index];
    int ret = 0;

    /* 当前仅支持MIPI DVP */
    assert_range(attr->dbus_type, SENSOR_DATA_BUS_MIPI, SENSOR_DATA_BUS_DVP);

    mutex_lock(&drv->lock);

    if (!drv->camera.is_power_on) {
        printk(KERN_ERR "vic%d can't stream on when not power on\n", index);
        ret = -EINVAL;
        goto out;
    }

    /* vic stream on */

    /* sensor stream on */
    ret = attr->ops.stream_on();
    if (ret)
        goto out;

    drv->camera.is_stream_on = 1;

out:
    mutex_unlock(&drv->lock);
    return ret;
}

void vic_stream_off(int index, struct sensor_attr *attr)
{
    struct jz_camera_data *drv = &jz_camera_dev[index];

    mutex_lock(&drv->lock);

    if (!drv->camera.is_stream_on) {
        printk(KERN_ERR "vic%d is already steam off\n", index);
        goto unlock;
    }

    /* sensor stream off */
    attr->ops.stream_off();

    /* vic stream off */

    /* disable timestamp */
    vic_disable_ts(index);

    drv->camera.is_stream_on = 0;

unlock:
    mutex_unlock(&drv->lock);
}

int vic_enable_clk(int index)
{
    struct jz_camera_data *drv = &jz_camera_dev[index];
    struct sensor_attr *attr = drv->camera.sensor;

    /* enable clock */
    if (attr->isp_clk_rate)
        drv->isp_clk_rate = attr->isp_clk_rate;
    vic_isp_div_clock_enable(index);
    clk_enable(drv->isp_gate_clk);
    clk_enable(drv->isp_power_clk);
    usleep_range(1500, 1500);

    return 0;
}

int vic_power_on(int index)
{
    struct jz_camera_data *drv = &jz_camera_dev[index];
    struct sensor_attr *attr = drv->camera.sensor;
    int ret = 0;

    mutex_lock(&drv->lock);

    if (drv->camera.is_power_on) {
        printk(KERN_ERR "vic%d is already power on, no need power on again\n", index);
        goto unlock;
    }

    vic_enable_clk(index);

    /* sensor power on */
    ret = attr->ops.power_on();
    if (ret != 0)
        goto unlock;

    /* vic power on, put it back of sensor power_on to ensure mipi phy ready */
    vic_hal_power_on(index, attr);

    drv->camera.is_power_on = 1;

unlock:
    mutex_unlock(&drv->lock);

    return ret;
}

void vic_power_off(int index)
{
    struct jz_camera_data *drv = &jz_camera_dev[index];
    struct sensor_attr *attr = drv->camera.sensor;

    if (!drv->camera.is_power_on) {
        printk(KERN_ERR "vic%d is already power off\n", index);
        return ;
    }

    if (drv->camera.is_stream_on)
        vic_stream_off(index, drv->camera.sensor);

    mutex_lock(&drv->lock);

    /* sensor power off */
    attr->ops.power_off();

    /* vic power off */
    vic_hal_power_off(index, attr);

    /* disable clock */
    clk_disable(drv->isp_power_clk);
    clk_disable(drv->isp_gate_clk);
    vic_isp_div_clock_disable(index);

    drv->camera.is_power_on = 0;

    mutex_unlock(&drv->lock);
}

/*
 * VIC && device power state
 * return =1: is power on
 *        =0: is power off
 */
int vic_power_state(int index)
{
    struct jz_camera_data *drv = &jz_camera_dev[index];

    return drv->camera.is_power_on;
}

/*
 * VIC && device stream state
 * return =1: is stream on
 *        =0: is stream off
 */
int vic_stream_state(int index)
{
    struct jz_camera_data *drv = &jz_camera_dev[index];

    return drv->camera.is_stream_on;
}


#ifdef SOC_CAMERA_DEBUG

ssize_t dsysfs_vic_dump_reg(int index, char *buf)
{
    char *p = buf;

    p += sprintf(p, "VIC REG:\n");
    p += sprintf(p, "\t VIC_CONTROL               :0x%08x\n", vic_read_reg(index, VIC_CONTROL));
    p += sprintf(p, "\t VIC_RESOLUTION            :0x%08x\n", vic_read_reg(index, VIC_RESOLUTION));
    p += sprintf(p, "\t VIC_FRM_ECC               :0x%08x\n", vic_read_reg(index, VIC_FRM_ECC));
    p += sprintf(p, "\t VIC_INPUT_INTF            :0x%08x\n", vic_read_reg(index, VIC_INPUT_INTF));
    p += sprintf(p, "\t VIC_INPUT_DVP             :0x%08x\n", vic_read_reg(index, VIC_INPUT_DVP));
    p += sprintf(p, "\t VIC_INPUT_MIPI            :0x%08x\n", vic_read_reg(index, VIC_INPUT_MIPI));
    p += sprintf(p, "\t VIC_IN_HOR_PARA0          :0x%08x\n", vic_read_reg(index, VIC_IN_HOR_PARA0));
    p += sprintf(p, "\t VIC_IN_HOR_PARA1          :0x%08x\n", vic_read_reg(index, VIC_IN_HOR_PARA1));
    p += sprintf(p, "\t VIC_BK_CB_CTRL            :0x%08x\n", vic_read_reg(index, VIC_BK_CB_CTRL));
    p += sprintf(p, "\t VIC_BK_CB_BLK             :0x%08x\n", vic_read_reg(index, VIC_BK_CB_BLK));
    p += sprintf(p, "\t VIC_IN_VER_PARA0          :0x%08x\n", vic_read_reg(index, VIC_INPUT_VPARA0));
    p += sprintf(p, "\t VIC_IN_VER_PARA1          :0x%08x\n", vic_read_reg(index, VIC_INPUT_VPARA1));
    p += sprintf(p, "\t VIC_IN_VER_PARA2          :0x%08x\n", vic_read_reg(index, VIC_INPUT_VPARA2));
    p += sprintf(p, "\t VIC_IN_VER_PARA3          :0x%08x\n", vic_read_reg(index, VIC_INPUT_VPARA3));
    p += sprintf(p, "\t VIC_VLD_LINE_SAV          :0x%08x\n", vic_read_reg(index, VIC_VLD_LINE_SAV));
    p += sprintf(p, "\t VIC_VLD_LINE_EAV          :0x%08x\n", vic_read_reg(index, VIC_VLD_LINE_EAV));
    p += sprintf(p, "\t VIC_VLD_FRM_SAV           :0x%08x\n", vic_read_reg(index, VIC_VLD_FRM_SAV));
    p += sprintf(p, "\t VIC_VLD_FRM_EAV           :0x%08x\n", vic_read_reg(index, VIC_VLD_FRM_EAV));
    p += sprintf(p, "\t VIC_VC_CONTROL_FSM        :0x%08x\n", vic_read_reg(index, VIC_VC_CONTROL_FSM));
    p += sprintf(p, "\t VIC_VC_CONTROL_CH0_PIX    :0x%08x\n", vic_read_reg(index, VIC_VC_CONTROL_CH0_PIX));
    p += sprintf(p, "\t VIC_VC_CONTROL_CH1_PIX    :0x%08x\n", vic_read_reg(index, VIC_VC_CONTROL_CH1_PIX));
    p += sprintf(p, "\t VIC_VC_CONTROL_CH2_PIX    :0x%08x\n", vic_read_reg(index, VIC_VC_CONTROL_CH2_PIX));
    p += sprintf(p, "\t VIC_VC_CONTROL_CH3_PIX    :0x%08x\n", vic_read_reg(index, VIC_VC_CONTROL_CH3_PIX));
    p += sprintf(p, "\t VIC_VC_CONTROL_CH0_LINE   :0x%08x\n", vic_read_reg(index, VIC_VC_CONTROL_CH0_LINE));
    p += sprintf(p, "\t VIC_VC_CONTROL_CH1_LINE   :0x%08x\n", vic_read_reg(index, VIC_VC_CONTROL_CH1_LINE));
    p += sprintf(p, "\t VIC_VC_CONTROL_CH2_LINE   :0x%08x\n", vic_read_reg(index, VIC_VC_CONTROL_CH2_LINE));
    p += sprintf(p, "\t VIC_VC_CONTROL_CH3_LINE   :0x%08x\n", vic_read_reg(index, VIC_VC_CONTROL_CH3_LINE));
    p += sprintf(p, "\t VIC_VC_CONTROL_FIFO_USE   :0x%08x\n", vic_read_reg(index, VIC_VC_CONTROL_FIFO_USE));
    p += sprintf(p, "\t VIC_CB_1ST                :0x%08x\n", vic_read_reg(index, VIC_CB_1ST));
    p += sprintf(p, "\t VIC_CB_2ND                :0x%08x\n", vic_read_reg(index, VIC_CB_2ND));
    p += sprintf(p, "\t VIC_CB_3RD                :0x%08x\n", vic_read_reg(index, VIC_CB_3RD));
    p += sprintf(p, "\t VIC_CB_4TH                :0x%08x\n", vic_read_reg(index, VIC_CB_4TH));
    p += sprintf(p, "\t VIC_CB_5TH                :0x%08x\n", vic_read_reg(index, VIC_CB_5TH));
    p += sprintf(p, "\t VIC_CB_6TH                :0x%08x\n", vic_read_reg(index, VIC_CB_6TH));
    p += sprintf(p, "\t VIC_CB_7TH                :0x%08x\n", vic_read_reg(index, VIC_CB_7TH));
    p += sprintf(p, "\t VIC_CB_8TH                :0x%08x\n", vic_read_reg(index, VIC_CB_8TH));
    p += sprintf(p, "\t VIC_CB2_1ST               :0x%08x\n", vic_read_reg(index, VIC_CB2_1ST));
    p += sprintf(p, "\t VIC_CB2_2ND               :0x%08x\n", vic_read_reg(index, VIC_CB2_2ND));
    p += sprintf(p, "\t VIC_CB2_3RD               :0x%08x\n", vic_read_reg(index, VIC_CB2_3RD));
    p += sprintf(p, "\t VIC_CB2_4TH               :0x%08x\n", vic_read_reg(index, VIC_CB2_4TH));
    p += sprintf(p, "\t VIC_CB2_5TH               :0x%08x\n", vic_read_reg(index, VIC_CB2_5TH));
    p += sprintf(p, "\t VIC_CB2_6TH               :0x%08x\n", vic_read_reg(index, VIC_CB2_6TH));
    p += sprintf(p, "\t VIC_CB2_7TH               :0x%08x\n", vic_read_reg(index, VIC_CB2_7TH));
    p += sprintf(p, "\t VIC_CB2_8TH               :0x%08x\n", vic_read_reg(index, VIC_CB2_8TH));
    p += sprintf(p, "\t MIPI_ALL_WIDTH_4BYTE      :0x%08x\n", vic_read_reg(index, MIPI_ALL_WIDTH_4BYTE));
    p += sprintf(p, "\t MIPI_VCROP_DEL01          :0x%08x\n", vic_read_reg(index, MIPI_VCROP_DEL01));
    p += sprintf(p, "\t MIPI_SENSOR_CONTROL       :0x%08x\n", vic_read_reg(index, MIPI_SENSOR_CONTROL));
    p += sprintf(p, "\t MIPI_HCROP_CH0            :0x%08x\n", vic_read_reg(index, MIPI_HCROP_CH0));
    p += sprintf(p, "\t MIPI_VCROP_SHADOW_CFG     :0x%08x\n", vic_read_reg(index, MIPI_VCROP_SHADOW_CFG));
    p += sprintf(p, "\t VIC_CONTROL_LIMIT         :0x%08x\n", vic_read_reg(index, VIC_CONTROL_LIMIT));
    p += sprintf(p, "\t VIC_CONTROL_DELAY         :0x%08x\n", vic_read_reg(index, VIC_CONTROL_DELAY));
    p += sprintf(p, "\t VIC_CONTROL_TIZIANO_ROUTE :0x%08x\n", vic_read_reg(index, VIC_CONTROL_TIZIANO_ROUTE));
    p += sprintf(p, "\t VIC_CONTROL_DMA_ROUTE     :0x%08x\n", vic_read_reg(index, VIC_CONTROL_DMA_ROUTE));
    p += sprintf(p, "\t VIC_INT_STA               :0x%08x\n", vic_read_reg(index, VIC_INT_STA));
    p += sprintf(p, "\t VIC_INT_MASK              :0x%08x\n", vic_read_reg(index, VIC_INT_MASK));
    p += sprintf(p, "\t VIC_INT_CLR               :0x%08x\n", vic_read_reg(index, VIC_INT_CLR));

    p += sprintf(p, "\t VIC_DMA_CONFIGURE         :0x%08x\n", vic_read_reg(index, VIC_DMA_CONFIGURE));
    p += sprintf(p, "\t VIC_DMA_RESOLUTION        :0x%08x\n", vic_read_reg(index, VIC_DMA_RESOLUTION));
    p += sprintf(p, "\t VIC_DMA_RESET             :0x%08x\n", vic_read_reg(index, VIC_DMA_RESET));
    p += sprintf(p, "\t DMA_Y_CH_LINE_STRIDE      :0x%08x\n", vic_read_reg(index, VIC_DMA_Y_CH_LINE_STRIDE));
    p += sprintf(p, "\t VIC_DMA_Y_CH_BUF0_ADDR    :0x%08x\n", vic_read_reg(index, VIC_DMA_Y_CH_BUF0_ADDR));
    p += sprintf(p, "\t VIC_DMA_Y_CH_BUF1_ADDR    :0x%08x\n", vic_read_reg(index, VIC_DMA_Y_CH_BUF1_ADDR));
    p += sprintf(p, "\t VIC_DMA_Y_CH_BUF2_ADDR    :0x%08x\n", vic_read_reg(index, VIC_DMA_Y_CH_BUF2_ADDR));
    p += sprintf(p, "\t VIC_DMA_Y_CH_BUF3_ADDR    :0x%08x\n", vic_read_reg(index, VIC_DMA_Y_CH_BUF3_ADDR));
    p += sprintf(p, "\t VIC_DMA_Y_CH_BUF4_ADDR    :0x%08x\n", vic_read_reg(index, VIC_DMA_Y_CH_BUF4_ADDR));
    p += sprintf(p, "\t VIC_DMA_UV_CH_LINE_STRIDE :0x%08x\n", vic_read_reg(index, VIC_DMA_UV_CH_LINE_STRIDE));
    p += sprintf(p, "\t VIC_DMA_UV_CH_BUF0_ADDR   :0x%08x\n", vic_read_reg(index, VIC_DMA_UV_CH_BUF0_ADDR));
    p += sprintf(p, "\t VIC_DMA_UV_CH_BUF1_ADDR   :0x%08x\n", vic_read_reg(index, VIC_DMA_UV_CH_BUF1_ADDR));
    p += sprintf(p, "\t VIC_DMA_UV_CH_BUF2_ADDR   :0x%08x\n", vic_read_reg(index, VIC_DMA_UV_CH_BUF2_ADDR));
    p += sprintf(p, "\t VIC_DMA_UV_CH_BUF3_ADDR   :0x%08x\n", vic_read_reg(index, VIC_DMA_UV_CH_BUF3_ADDR));
    p += sprintf(p, "\t VIC_DMA_UV_CH_BUF4_ADDR   :0x%08x\n", vic_read_reg(index, VIC_DMA_UV_CH_BUF4_ADDR));

    p += sprintf(p, "\t VIC_TS_ENABLE             :0x%08x\n", vic_read_reg(index, VIC_TS_ENABLE));
    p += sprintf(p, "\t VIC_TS_COUNTER            :0x%08x\n", vic_read_reg(index, VIC_TS_COUNTER));
    p += sprintf(p, "\t VIC_TS_DMA_OFFSET         :0x%08x\n", vic_read_reg(index, VIC_TS_DMA_OFFSET));
    p += sprintf(p, "\t VIC_TS_MS_CH0_OFFSET      :0x%08x\n", vic_read_reg(index, VIC_TS_MS_CH0_OFFSET));
    p += sprintf(p, "\t VIC_TS_MS_CH1_OFFSET      :0x%08x\n", vic_read_reg(index, VIC_TS_MS_CH1_OFFSET));
    p += sprintf(p, "\t VIC_TS_MS_CH2_OFFSET      :0x%08x\n", vic_read_reg(index, VIC_TS_MS_CH2_OFFSET));

    p += sprintf(p, "MIPI REG:\n");
    p += dsysfs_mipi_dump_reg(index, p);

    return p - buf;
}

int dsysfs_vic_show_sensor_info(int index, char *buf)
{
    struct jz_camera_data *drv = &jz_camera_dev[index];
    struct sensor_attr *sensor = drv->camera.sensor;
    char *p = buf;

    char *sensor_data_bus_type_to_str(sensor_data_bus_type type)
    {
        static char *type_str[SENSOR_DATA_BUS_BUTT] = {
            [SENSOR_DATA_BUS_MIPI] = "SENSOR_DBUS_MIPI",
            [SENSOR_DATA_BUS_DVP] = "SENSOR_DBUS_DVP",
            [SENSOR_DATA_BUS_BT601] = "SENSOR_DBUS_601",
            [SENSOR_DATA_BUS_BT656] = "SENSOR_DBUS_BT656",
            [SENSOR_DATA_BUS_BT1120] = "SENSOR_DBUS_BT1120",
        };

        if (type < SENSOR_DATA_BUS_BUTT)
            return type_str[type];
        return NULL;
    }

    p += sprintf(p, "name: %s\n", sensor->device_name);
    p += sprintf(p, "cbus addr: 0x%x\n", sensor->cbus_addr);
    p += sprintf(p, "dbus type: %s\n", sensor_data_bus_type_to_str(sensor->dbus_type));
    if (SENSOR_DATA_BUS_MIPI == sensor->dbus_type) {
        p += sprintf(p, "\t lanes: %d\n", sensor->mipi.lanes);
        p += sprintf(p, "\t clk_settle_time: %d ns\n", sensor->mipi.clk_settle_time);
        p += sprintf(p, "\t data_settle_time: %d ns\n", sensor->mipi.data_settle_time);
    } else if (SENSOR_DATA_BUS_DVP == sensor->dbus_type) {
        p += sprintf(p, "\t gpio_mode: %d\n", sensor->dvp.gpio_mode);
        p += sprintf(p, "\t timing_mode: %d\n", sensor->dvp.timing_mode);
        p += sprintf(p, "\t hsync_polarity: %d\n", sensor->dvp.hsync_polarity);
        p += sprintf(p, "\t vsync_polarity: %d\n", sensor->dvp.vsync_polarity);
        p += sprintf(p, "\t img_scan_mode: %d\n", sensor->dvp.img_scan_mode);
    } else {
        //TODO
    }

    p += sprintf(p, "mclk_rate: %ld\n", clk_get_rate(drv->mclk_div));
    p += sprintf(p, "isp_clk_rate: %ld\n", clk_get_rate(drv->isp_div));

    p += sprintf(p, "sensor info:\n");
    p += sprintf(p, "\t width: %d\n", sensor->sensor_info.width);
    p += sprintf(p, "\t height: %d\n", sensor->sensor_info.height);
    p += sprintf(p, "\t fmt: %d\n", sensor->sensor_info.fmt);
    p += sprintf(p, "\t fps: %d << 16 / %d\n", sensor->sensor_info.fps >> 16, sensor->sensor_info.fps & 0xffff);
    p += sprintf(p, "\t total_width: %d\n", sensor->sensor_info.total_width);
    p += sprintf(p, "\t total_height: %d\n", sensor->sensor_info.total_height);
    p += sprintf(p, "\t integration_time: %d\n", sensor->sensor_info.integration_time);
    p += sprintf(p, "\t min_integration_time: %d\n", sensor->sensor_info.min_integration_time);
    p += sprintf(p, "\t max_integration_time: %d\n", sensor->sensor_info.max_integration_time);
    p += sprintf(p, "\t one_line_expr_in_us: %d\n", sensor->sensor_info.one_line_expr_in_us);
    //p += sprintf(p, "\t again: %d\n", sensor->sensor_info.again);
    p += sprintf(p, "\t max_again: %d\n", sensor->sensor_info.max_again);
    //p += sprintf(p, "\t dgain: %d\n", sensor->sensor_info.dgain);
    p += sprintf(p, "\t max_dgain: %d\n", sensor->sensor_info.max_dgain);


    return p - buf;
}

int dsysfs_vic_ctrl(int index, char *buf)
{
    struct jz_camera_data *drv = &jz_camera_dev[index];
    struct sensor_attr *sensor = drv->camera.sensor;
    int ret;

    if (!strncmp(buf, "r_sen_reg", sizeof("r_sen_reg")-1)) {
        if(!vic_power_state(index)){
            printk(KERN_ERR "%s sensor doesn't work, please power on\n", __func__);
            goto exit;
        }

        struct sensor_dbg_register reg;
        reg.reg = simple_strtoull(buf+sizeof("r_sen_reg"), NULL, 0);
        if (sensor->ops.get_register) {
            ret = sensor->ops.get_register(&reg);
            if (ret < 0) {
                printk(KERN_ERR "%s get_register fail\n", __func__);
                goto exit;
            }
            printk(KERN_ERR "r_sen_reg: 0x%llx 0x%llx \n", reg.reg, reg.val);
        } else {
            printk(KERN_ERR "sensor->ops.get_register is NULL!\n");
            ret = -EINVAL;
            goto exit;
        }
    } else if (!strncmp(buf, "w_sen_reg", sizeof("w_sen_reg")-1)) {
        if(!vic_power_state(index)){
            printk(KERN_ERR "%s sensor doesn't work, please power on\n", __func__);
            goto exit;
        }

        struct sensor_dbg_register reg;
        char *p = 0;
        reg.reg = simple_strtoull(buf+sizeof("w_sen_reg"), &p, 0);
        reg.val = simple_strtoull(p+1, NULL, 0);
        if (sensor->ops.set_register) {
            ret = sensor->ops.set_register(&reg);
            if (ret < 0) {
                printk(KERN_ERR "%s set_register fail\n", __func__);
                return ret;
            }
            printk(KERN_ERR "w_sen_reg: 0x%llx 0x%llx \n", reg.reg, reg.val);
        } else {
            printk(KERN_ERR "sensor->ops.get_register is NULL!\n");
            ret = -EINVAL;
            goto exit;
        }
    } else if (!strncmp(buf, "r_vic_reg", sizeof("r_vic_reg")-1)) {
        if(!vic_power_state(index)){
            printk(KERN_ERR "%s sensor doesn't work, please power on\n", __func__);
            goto exit;
        }

        unsigned int reg = simple_strtoul(buf+sizeof("r_vic_reg"), NULL, 0);
        printk(KERN_ERR "r_vic_reg: 0x%x 0x%x \n", reg, vic_read_reg(index, reg));
    } else if (!strncmp(buf, "w_vic_reg", sizeof("w_vic_reg")-1)) {
        if(!vic_power_state(index)){
            printk(KERN_ERR "%s sensor doesn't work, please power on\n", __func__);
            goto exit;
        }

        char *p = 0;
        unsigned int reg = simple_strtoul(buf+sizeof("w_vic_reg"), &p, 0);
        unsigned int val = simple_strtoul(p+1, NULL, 0);
        vic_write_reg(index, reg, val);
        printk(KERN_ERR "w_vic_reg: 0x%x 0x%x \n", reg, val);
    } else {
        printk(KERN_ERR "%s, unknow cmd: %s\n", __func__, buf);
        printk(KERN_ERR "usage:\n");
        printk(KERN_ERR "1. snap raw\n");
        printk(KERN_ERR "\t\t echo \"snapraw\" > /sys/ispX/vic/ctrl\n");
        printk(KERN_ERR "2. rw sensor reg(rw reg val)\n");
        printk(KERN_ERR "\t\t echo \"r_sen_reg 0x55\" > /sys/ispX/vic/ctrl(/sys/vicX/ctrl)\n");
        printk(KERN_ERR "\t\t echo \"w_sen_reg 0x55 0xaa\" > /sys/ispX/vic/ctrl(/sys/vicX/ctrl)\n");
    }

exit:
    return ret;
}

#endif


static int jz_vic_resources_init(int index)
{
    struct jz_camera_data *drv = &jz_camera_dev[index];
    int ret;


    ret = camera_mclk_gpio_init(drv->mclk_io);
    if(ret < 0) {
        printk(KERN_ERR "vic%d driver mclk gpio init failed\n", index);
        return ret;
    }

    drv->mclk_div = clk_get(NULL, "div_cim");   /* VIC/CIM 可共用 */
    assert(!IS_ERR(drv->mclk_div));
    assert(!clk_prepare(drv->mclk_div));

    drv->isp_div = clk_get(NULL, drv->isp_div_clk_name);
    assert(!IS_ERR(drv->isp_div));
    assert(!clk_prepare(drv->isp_div));

    drv->isp_gate_clk = clk_get(NULL, drv->isp_gate_clk_name);
    assert(!IS_ERR(drv->isp_gate_clk));
    assert(!clk_prepare(drv->isp_gate_clk));

    drv->isp_power_clk = clk_get(NULL, drv->isp_power_clk_name);
    assert(!IS_ERR(drv->isp_power_clk));
    assert(!clk_prepare(drv->isp_power_clk));

    drv->isp_clk_rate = 90 * 1000 * 1000;

    mutex_init(&drv->lock);

    if (drv->is_isp_enable) {
        ret = jz_vic_tiziano_drv_init(index);
    } else {
        ret = jz_vic_mem_drv_init(index);
    }

    if (ret) {
        printk(KERN_ERR "camera: failed to init vic%d resources\n", index);
        goto error_vic_resources_init;
    }

    drv->is_finish = 1;

    printk(KERN_DEBUG "vic%d register successfully\n", index);

    return 0;

error_vic_resources_init:
    clk_put(drv->mclk_div);
    clk_put(drv->isp_div);
    clk_put(drv->isp_power_clk);
    clk_put(drv->isp_gate_clk);
    camera_mclk_gpio_deinit(drv->mclk_io);

    return ret;
}

static void jz_vic_resources_deinit(int index)
{
    struct jz_camera_data *drv = &jz_camera_dev[index];

    drv->is_finish = 0;

    if (drv->is_isp_enable)
        jz_vic_tiziano_drv_deinit(index);
    else
        jz_vic_mem_drv_deinit(index);

    clk_put(drv->mclk_div);
    clk_put(drv->isp_div);
    clk_put(drv->isp_power_clk);
    clk_put(drv->isp_gate_clk);
    camera_mclk_gpio_deinit(drv->mclk_io);
}

static int jz_cim_resources_init(struct device *dev, int index)
{
    struct jz_camera_data *drv = &jz_camera_dev[index];
    int ret;

    ret = camera_mclk_gpio_init(drv->mclk_io);
    if(ret < 0) {
        printk(KERN_ERR "cim driver mclk gpio init failed\n");
        return ret;
    }

    drv->mclk_div = clk_get(NULL, "div_cim");   /* VIC/CIM 可共用 */
    assert(!IS_ERR(drv->mclk_div));
    assert(!clk_prepare(drv->mclk_div));

    drv->cim_gate_clk = clk_get(NULL, drv->cim_gate_clk_name);
    assert(!IS_ERR(drv->cim_gate_clk));
    assert(!clk_prepare(drv->cim_gate_clk));

    mutex_init(&drv->lock);

    ret = jz_cim_drv_init(dev);

    if (ret) {
        printk(KERN_ERR "camera: failed to init cim resources\n");
        goto error_cim_resources_init;
    }

    drv->is_finish = 1;

    printk(KERN_DEBUG "cim resources register successfully\n");

    return 0;

error_cim_resources_init:
    clk_put(drv->mclk_div);
    clk_put(drv->cim_gate_clk);
    camera_mclk_gpio_deinit(drv->mclk_io);

    return ret;
}

static void jz_cim_resources_deinit(int index)
{
    struct jz_camera_data *drv = &jz_camera_dev[index];

    drv->is_finish = 0;

    jz_cim_drv_deinit();

    clk_put(drv->mclk_div);
    clk_put(drv->cim_gate_clk);
    camera_mclk_gpio_deinit(drv->mclk_io);
}


/*
 * VIC/CIM 模块公用mclk, 使用index参数方便获取mclk的变量
 */
void camera_enable_sensor_mclk(int index, unsigned long clk_rate)
{
    struct jz_camera_data *drv = &jz_camera_dev[index];
    unsigned long rate = 0;

    if ( !__clk_is_enabled(drv->mclk_div) ) {
        clk_set_rate(drv->mclk_div, clk_rate);
        clk_enable(drv->mclk_div);
        return ;
    }

    rate = clk_get_rate(drv->mclk_div);
    if (rate != clk_rate) {
        printk(KERN_ERR "mclk already enabled rate=%ld, not change to %ld\n", rate, clk_rate);
    }

    clk_enable(drv->mclk_div);
}

void camera_disable_sensor_mclk(int index)
{
    struct jz_camera_data *drv = &jz_camera_dev[index];

    clk_disable(drv->mclk_div);
}

int camera_register_sensor(int index, struct sensor_attr *sensor)
{
    assert(index < 3);

    struct jz_camera_data *drv = &jz_camera_dev[index];
    assert(drv->is_finish > 0);
    assert(!drv->camera.sensor);

    int ret = -EINVAL;

    switch (index) {
    case 0:
    case 1:
        /* VIC */
        if (drv->is_isp_enable)
            ret = vic_register_sensor_route_tiziano(index, sensor);
        else
            ret = vic_register_sensor_route_mem(index, drv->cam_mem_cnt, sensor);

        break;

    case 2:
        /* CIM */
        ret = cim_register_sensor_route(index, drv->cam_mem_cnt, sensor);
        break;

    default:
        printk(KERN_ERR "camera register index(%d) is invalid\n", index);
        break;
    }

    if (!ret) {
        drv->camera.sensor = sensor;
        drv->camera.is_power_on = 0;
        drv->camera.is_stream_on = 0;
    }

    return ret;
}

void camera_unregister_sensor(int index, struct sensor_attr *sensor)
{
    assert(index < 3);

    struct jz_camera_data *drv = &jz_camera_dev[index];
    assert(drv->is_finish > 0);
    assert(drv->camera.sensor);
    assert(sensor == drv->camera.sensor);

    switch (index) {
    case 0:
    case 1:
        /* VIC */
        if (drv->is_isp_enable)
            vic_unregister_sensor_route_tiziano(index, sensor);
        else
            vic_unregister_sensor_route_mem(index, sensor);

        break;

    case 2:
        /* CIM */
        cim_unregister_sensor_route(index, sensor);
        break;

    default:
        printk(KERN_ERR "camera unregister index(%d) is invalid\n", index);
        break;
    }

    drv->camera.sensor = NULL;
}

int jz_arch_vic_init(struct device *dev)
{
    /* VIC */
    if (jz_camera_dev[0].is_enable)
        jz_vic_resources_init(jz_camera_dev[0].index);

    if (jz_camera_dev[1].is_enable)
        jz_vic_resources_init(jz_camera_dev[1].index);

    /* CIM */
    if (jz_camera_dev[2].is_enable)
        jz_cim_resources_init(dev, jz_camera_dev[2].index);

    return 0;
}

void jz_arch_vic_exit(void)
{
    /* VIC */
    if (jz_camera_dev[0].is_finish)
        jz_vic_resources_deinit(jz_camera_dev[0].index);

    if (jz_camera_dev[1].is_finish)
        jz_vic_resources_deinit(jz_camera_dev[1].index);

    /* CIM */
    if (jz_camera_dev[2].is_finish)
        jz_cim_resources_deinit(jz_camera_dev[2].index);
}


EXPORT_SYMBOL(camera_enable_sensor_mclk);
EXPORT_SYMBOL(camera_disable_sensor_mclk);

EXPORT_SYMBOL(camera_register_sensor);
EXPORT_SYMBOL(camera_unregister_sensor);
