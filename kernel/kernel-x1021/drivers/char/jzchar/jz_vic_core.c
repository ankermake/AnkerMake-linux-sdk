/*
 * Ingenic x1021 camera (CIM) host driver only for x1021
 *
 * Copyright (C) 2019, Ingenic Semiconductor Inc. xiaoyan.zhang@ingenic.com
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 *
 */

#include <linux/clk.h>
#include <linux/delay.h>
#include <linux/device.h>
#include <linux/dma-mapping.h>
#include <linux/errno.h>
#include <linux/fs.h>
#include <linux/init.h>
#include <linux/interrupt.h>
#include <linux/io.h>
#include <linux/kernel.h>
#include <linux/mm.h>
#include <linux/module.h>
#include <linux/moduleparam.h>
#include <linux/mutex.h>
#include <linux/platform_device.h>
#include <linux/sched.h>
#include <linux/slab.h>
#include <linux/time.h>
#include <linux/version.h>
#include <linux/spinlock.h>
#include <linux/miscdevice.h>
#include <asm/dma.h>
#include <linux/gpio.h>
#include <soc/gpio.h>
#include <soc/vic.h>
#include <soc/cpm.h>

#define CIM_IMG_DEFAULT_WIDTH         640
#define CIM_IMG_DEFAULT_HEIGHT        480
#define ISP_CLOCK_DEFAULT             90000000
#define CIM_CLOCK_DEFAULT             24000000
#define CIM_FRAME_BUF_CNT             4

static int debug = 3;
module_param(debug, int, 0644);

#define dprintk(level, fmt, arg...)                    \
    do {                                \
        if (debug >= level)                    \
            printk("x1021-camera: " fmt, ## arg);    \
    } while (0)


#define DRIVER_NAME "tx-isp"

/*
 * ioctl commands
 */
#define IOCTL_SET_IMG_FORMAT    8    //arg type:enum imgformat
#define IOCTL_SET_TIMING_PARAM  9    // arg type: timing_param_t *
#define IOCTL_SET_IMG_PARAM     10    // arg type: img_param_t *
#define IOCTL_GET_FRAME         11
#define IOCTL_GET_FRAME_PHY     12
#define IOCTL_START_DMA         13
#define IOCTL_GET_INFO          14
#define IOCTL_STOP_DMA         15

/*
 * Structures
 */
struct x1021_cim_dma_desc {
    dma_addr_t next;
    unsigned int id;
    unsigned int buf;
    unsigned int cmd;
    /* only used when SEP = 1 */
    unsigned int cb_frame;
    unsigned int cb_len;
    unsigned int cr_frame;
    unsigned int cr_len;
} __attribute__ ((aligned (32)));

/* timing parameters */
typedef struct
{
    unsigned int mclk_freq;
    unsigned int ispclk_freq;
    unsigned char pclk_active_direction;//o for rising edge, 1 for falling edge, same with cim controller
    unsigned char hsync_active_level;
    unsigned char vsync_active_level;
} timing_param_t;

typedef void (*frame_receive)(unsigned char* buf, unsigned int pixelformat, unsigned int width, unsigned int height, unsigned int seq);
typedef struct{
    unsigned int width;         // Resolution width(x)
    unsigned int height;        // Resolution height(y)
    unsigned int size;
    unsigned int bpp;           // Bits Per Pixels
    unsigned int nbuf;          // buf queue
    frame_receive fr_cb;
    uint8_t *buf;
} capt_param_t;


/* image format */
typedef enum{
    RAW8,
    RAW10,
    RAW12,
    YUV422,
    RSV4,
    RSV5,
    YUV422_8BIT,
} img_format_t;

struct cim_device {
    int id;        /* CIM0 or CIM1 */
        int buf_flag[5];
    unsigned int irq;
    struct clk *clk;
    struct clk *mclk;
    struct clk *ispclk;
    struct resource    *res;
    void __iomem *base;

    struct miscdevice misc_dev;
    struct device *dev;

    unsigned char *framebuf_vaddr;
    unsigned char *framebuf_paddr;
    void *desc_vaddr;
    struct x1021_cim_dma_desc *dma_desc_paddr;

    void *shared_mem;    //app get frame from shared_mem

    timing_param_t timing;
    capt_param_t img;
    img_format_t img_format;
    unsigned int frame_size;
    int dma_started;
    int read_flag;
    int buf_cnt;
    spinlock_t lock;
    atomic_t opened;

    char name[8];
    char clkname[8];
    char cguclkname[16];

    struct timeval latest;
    struct timeval now;
};

struct camera_driver_info { /* should compatible with user space */
    capt_param_t parameters;
    img_format_t format;
    unsigned int physical_address;
};

static void x1021_cim_dump_reg(struct cim_device *cim)
{
    printk("=========================================\n");
#if 1
    printk("VIC_CONTROL(%x)     = 0x%08x\n", VIC_CONTROL, tx_isp_vic_readl(cim->base + VIC_CONTROL));
    printk("VIC_RESOLUTION(%x)    = 0x%08x\n", VIC_RESOLUTION, tx_isp_vic_readl(cim->base + VIC_RESOLUTION));
    printk("VIC_FRM_ECC(%x)    = 0x%08x\n", VIC_FRM_ECC, tx_isp_vic_readl(cim->base + VIC_FRM_ECC));
    printk("VIC_INTF_TYPE(%x)   = 0x%08x\n", VIC_INTF_TYPE, tx_isp_vic_readl(cim->base + VIC_INTF_TYPE));
    printk("VIC_DB_CFG(%x)   = 0x%08x\n", VIC_DB_CFG, tx_isp_vic_readl(cim->base + VIC_DB_CFG));
    printk("VIC_GLOBAL_CFG(%x)     = 0x%08x\n", VIC_GLOBAL_CFG, tx_isp_vic_readl(cim->base + VIC_GLOBAL_CFG));
    printk("VIC_OUT_ABVAL(%x)      = 0x%08x\n", VIC_OUT_ABVAL, tx_isp_vic_readl(cim->base + VIC_OUT_ABVAL));
    printk("VIC_INPUT_HSYNC_BLANKING(%x)      = 0x%08x\n", VIC_INPUT_HSYNC_BLANKING, tx_isp_vic_readl(cim->base + VIC_INPUT_HSYNC_BLANKING));
    printk("VIC_INT_STA(%x)     = 0x%08x\n", VIC_INT_STA, tx_isp_vic_readl(cim->base + VIC_INT_STA));
    printk("VIC_INT_MASK(%x)     = 0x%08x\n", VIC_INT_MASK, tx_isp_vic_readl(cim->base + VIC_INT_MASK));
    printk("VIC_INT_CLR(%x)    = 0x%08x\n", VIC_INT_CLR, tx_isp_vic_readl(cim->base + VIC_INT_CLR));
    printk("VIC_DMA_CONFIG(%x)  = 0x%08x\n", VIC_DMA_CONFIG, tx_isp_vic_readl(cim->base + VIC_DMA_CONFIG));
    printk("VIC_DMA_RESOLUTION(%x)      = 0x%08x\n", VIC_DMA_RESOLUTION, tx_isp_vic_readl(cim->base + VIC_DMA_RESOLUTION));
    printk("VIC_DMA_RESET(%x)     = 0x%08x\n", VIC_DMA_RESET, tx_isp_vic_readl(cim->base + VIC_DMA_RESET));
    printk("VIC_DMA_Y_STRID(%x)    = 0x%08x\n", VIC_DMA_Y_STRID, tx_isp_vic_readl(cim->base + VIC_DMA_Y_STRID));
    printk("VIC_DMA_UV_STRID(%x)    = 0x%08x\n", VIC_DMA_UV_STRID, tx_isp_vic_readl(cim->base + VIC_DMA_UV_STRID));
    printk("VIC_DMA_Y_BUF0(%x)    = 0x%08x\n", VIC_DMA_Y_BUF0, tx_isp_vic_readl(cim->base + VIC_DMA_Y_BUF0));
    printk("VIC_DMA_Y_BUF1(%x)    = 0x%08x\n", VIC_DMA_Y_BUF1, tx_isp_vic_readl(cim->base + VIC_DMA_Y_BUF1));
    printk("VIC_DMA_Y_BUF2(%x)    = 0x%08x\n", VIC_DMA_Y_BUF2, tx_isp_vic_readl(cim->base + VIC_DMA_Y_BUF2));
    printk("VIC_DMA_Y_BUF3(%x)    = 0x%08x\n", VIC_DMA_Y_BUF3, tx_isp_vic_readl(cim->base + VIC_DMA_Y_BUF3));
    printk("VIC_DMA_Y_BUF4(%x)    = 0x%08x\n", VIC_DMA_Y_BUF4, tx_isp_vic_readl(cim->base + VIC_DMA_Y_BUF4));
    printk("VIC_DMA_UV_BUF0(%x)    = 0x%08x\n", VIC_DMA_UV_BUF0, tx_isp_vic_readl(cim->base + VIC_DMA_UV_BUF0));
    printk("VIC_DMA_UV_BUF1(%x)    = 0x%08x\n", VIC_DMA_UV_BUF1, tx_isp_vic_readl(cim->base + VIC_DMA_UV_BUF1));
    printk("VIC_DMA_UV_BUF2(%x)    = 0x%08x\n", VIC_DMA_UV_BUF2, tx_isp_vic_readl(cim->base + VIC_DMA_UV_BUF2));
    printk("VIC_DMA_UV_BUF3(%x)    = 0x%08x\n", VIC_DMA_UV_BUF3, tx_isp_vic_readl(cim->base + VIC_DMA_UV_BUF3));
    printk("VIC_DMA_UV_BUF4(%x)    = 0x%08x\n", VIC_DMA_UV_BUF4, tx_isp_vic_readl(cim->base + VIC_DMA_UV_BUF4));
#else
    int i;
    for(i = 0; i < 0x304; i+=4)
    {
         printk("(%x)    = 0x%08x\n", i, tx_isp_vic_readl(cim->base + i));
    }
#endif
    printk("=========================================\n");
}

static void x1021_cim_enable_mclk(struct cim_device *cim)
{
    int ret = -1;
    if(cim->mclk) {
            printk("[%s][%d]  cim->timing.mclk_freq:%ld\n", __FUNCTION__,__LINE__, cim->timing.mclk_freq);
        ret = clk_set_rate(cim->mclk, cim->timing.mclk_freq);
        ret = clk_enable(cim->mclk);
    }
    if(ret)
        dprintk(3, "enable mclock failed!\n");
    printk("got cim->mclk:%dM \n", clk_get_rate(cim->mclk)/1000000);
    mdelay(10);
}

static void x1021_cim_enable_ispclk(struct cim_device *cim)
{
    int ret = -1;
        if(cim->ispclk) {
            printk("[%s][%d]  cim->timing.ispclk_freq:%ld\n", __FUNCTION__,__LINE__, cim->timing.ispclk_freq);
        ret = clk_set_rate(cim->ispclk, cim->timing.ispclk_freq);
        ret = clk_enable(cim->ispclk);
    }
    if(ret)
        dprintk(3, "enable ispclock failed!\n");
    printk("got cim->ispclk:%dM \n", clk_get_rate(cim->ispclk)/1000000);
    mdelay(10);
}

static void x1021_cim_disable_mclk(struct cim_device *cim)
{
    if(cim->mclk)
        clk_disable(cim->mclk);
}

static void x1021_cim_disable_ispclk(struct cim_device *cim)
{
    if(cim->ispclk)
        clk_disable(cim->ispclk);
}

static void x1021_cim_enable_clk(struct cim_device *cim)
{
    int ret = -1;
    dprintk(7, "Activate device\n");

    if(cim->clk)
        ret = clk_enable(cim->clk);

    if(ret)
        dprintk(3, "enable clock failed!\n");
    mdelay(10);
}

static void x1021_cim_disable_clk(struct cim_device *cim)
{
    if(cim->clk)
        clk_disable(cim->clk);
}

static void x1021_cim_activate(struct cim_device *cim)
{
    cim->timing.ispclk_freq = ISP_CLOCK_DEFAULT;
    cim->timing.mclk_freq = CIM_CLOCK_DEFAULT;

    x1021_cim_enable_clk(cim);
    x1021_cim_enable_mclk(cim);
    x1021_cim_enable_ispclk(cim);

    tx_isp_vic_writel(0x1ff, cim->base + VIC_INT_MASK);
    tx_isp_vic_writel(0x1ff, cim->base + VIC_INT_CLR);
}

static void x1021_cim_deactivate(struct cim_device *cim)
{
    unsigned long temp = 0;

    /* disable DVP_OVF interrupt */
    temp = tx_isp_vic_readl(cim->base + VIC_INT_MASK);
    temp |= DMA_DVP_OVF_MSK;
    tx_isp_vic_writel(temp, cim->base + VIC_INT_MASK);

    /* disable DMA_FRD interrupt */
    temp = tx_isp_vic_readl(cim->base + VIC_INT_MASK);
    temp |= DMA_FRD_MSK;
    tx_isp_vic_writel(temp, cim->base + VIC_INT_MASK);

    /* disable rx overflow interrupt */
    temp = tx_isp_vic_readl(cim->base + VIC_INT_MASK);
    temp |= VIC_VRES_ERR_MSK;
    tx_isp_vic_writel(temp, cim->base + VIC_INT_MASK);

    /* disable rx overflow interrupt */
    temp = tx_isp_vic_readl(cim->base + VIC_INT_MASK);
    temp |= VIC_HRES_ERR_MSK;
    tx_isp_vic_writel(temp, cim->base + VIC_INT_MASK);

        /* disable rx overflow interrupt */
    temp = tx_isp_vic_readl(cim->base + VIC_INT_MASK);
    temp |= VIC_FIFO_OVF_MSK;
    tx_isp_vic_writel(temp, cim->base + VIC_INT_MASK);

    temp = tx_isp_vic_readl(cim->base + VIC_CONTROL);
    temp |= GLB_RST;
    tx_isp_vic_writel(temp, cim->base + VIC_CONTROL);

    x1021_cim_disable_mclk(cim);
    x1021_cim_disable_clk(cim);

    dprintk(7, "Deactivate device\n");
}

static void x1021_cim_start_dma(struct cim_device *cim)
{
    unsigned int temp;
    unsigned long timeout = 30000;

    tx_isp_vic_writel(REG_ENABLE, cim->base + VIC_CONTROL);
    while(tx_isp_vic_readl(cim->base + VIC_CONTROL)) {
            if(--timeout == 0) {
                printk("#################### %s,%d (zxy: x1021_cim_start_dma temeout!!!!!!!!! ) \n", __FUNCTION__,__LINE__);
                break;
            }
        }
    tx_isp_vic_writel(VIC_START, cim->base + VIC_CONTROL);

    temp = tx_isp_vic_readl(cim->base + VIC_DMA_CONFIG);
    temp |= DMA_EN;
    tx_isp_vic_writel(temp, cim->base + VIC_DMA_CONFIG);
}

static void x1021_cim_stop_dma(struct cim_device *cim)
{
    unsigned int temp, i;
    unsigned long timeout = 30000;

    temp = tx_isp_vic_readl(cim->base + VIC_DMA_CONFIG);
    temp &= (~DMA_EN);
    tx_isp_vic_writel(temp, cim->base + VIC_DMA_CONFIG);
    tx_isp_vic_writel(DMA_RESET, cim->base + VIC_DMA_RESET);
    tx_isp_vic_writel(0, cim->base + VIC_CONTROL);
    tx_isp_vic_writel(GLB_RST, cim->base + VIC_CONTROL);
    cim->id = -1;
    for(i = 0; i < cim->buf_cnt; i++)
    {
        cim->buf_flag[i] = 0;
    }
    memset(cim->framebuf_vaddr, 0, cim->buf_cnt * cim->frame_size);
}


static irqreturn_t x1021_cim_irq_handler(int irq, void *data)
{
    unsigned long flags;
    unsigned int status = 0;
    unsigned int temp = 0;
    unsigned int cnt = 5;

    struct cim_device *cim = (struct cim_device *)data;

    /* read interrupt status register */
    status = tx_isp_vic_readl(cim->base + VIC_INT_STA);

    if (status & DMA_FIFO_OVF)
        dprintk(3, "DMA's sync fifo overflow error occurs interrupt!\n");
    if (status & VIC_HVF_ERR)
        dprintk(3, "VIC BT656/1120 hvf error occurs interrupt!\n");
    if (status & VIC_VRES_ERR)
        dprintk(3, "VIC image vertical error interrupt!\n");
    if (status & VIC_HRES_ERR)
    {
        dprintk(3, "VIC image horizon error interrupt! \n");
        do{
            temp = tx_isp_vic_readl(cim->base + 0x308);
            dprintk(3, "0x308 reg = 0x%x\n", temp);
        }while(cnt-- > 0);
        tx_isp_vic_writel(VIC_HRES_ERR, cim->base + VIC_INT_MASK);
        tx_isp_vic_writel(VIC_HRES_ERR, cim->base + VIC_INT_CLR);
    }
    if (status & VIC_FIFO_OVF)
        dprintk(3, "VIC async FIFO overflow interrupt!\n");

    if(status & DMA_FRD) {
        dprintk(7, "vic got one frame \n");
        spin_lock_irqsave(&cim->lock, flags);
        cim->id ++;
        if(cim->id == cim->buf_cnt)
            cim->id = 0;
        cim->buf_flag[cim->id] = 1;
        spin_unlock_irqrestore(&cim->lock, flags);
        tx_isp_vic_writel(DMA_FRD, cim->base + VIC_INT_CLR);
    }

    /* clear status register */
    tx_isp_vic_writel(0x1ff, cim->base + VIC_INT_CLR);

    return IRQ_HANDLED;
}

static void x1021_cim_set_timing(struct cim_device *cim)
{
    timing_param_t *timing = &cim->timing;
    unsigned int temp = 0;

    if (timing->mclk_freq != CIM_CLOCK_DEFAULT)
    {
        x1021_cim_disable_mclk(cim);
        x1021_cim_disable_clk(cim);
        x1021_cim_enable_clk(cim);
        x1021_cim_enable_mclk(cim);
    }
    else if(timing->mclk_freq == 0)
    {
        x1021_cim_disable_mclk(cim);
        x1021_cim_disable_clk(cim);
    }

    if (timing->ispclk_freq != ISP_CLOCK_DEFAULT)
    {
        x1021_cim_disable_ispclk(cim);
        x1021_cim_disable_clk(cim);
        x1021_cim_enable_clk(cim);
        x1021_cim_enable_ispclk(cim);
    }
    else if(timing->ispclk_freq == 0)
    {
        x1021_cim_disable_ispclk(cim);
        x1021_cim_disable_clk(cim);
    }

    temp = tx_isp_vic_readl(cim->base + VIC_DB_CFG);
    if (timing->hsync_active_level == 0)
        temp |= HSYN_POLAR;
    else
        temp &= (~HSYN_POLAR);

    if (timing->vsync_active_level == 0)
        temp |= VSYN_POLAR;
    else
        temp &= (~VSYN_POLAR);
    tx_isp_vic_writel(temp, cim->base + VIC_DB_CFG);

    tx_isp_vic_writel(0x1ff, cim->base + VIC_INT_CLR);
    x1021_cim_stop_dma(cim);
    cim->dma_started = 0;
}

static void x1021_cim_set_img(struct cim_device *cim, unsigned int width, unsigned int height)
{
    cim->img.width    = width;
    cim->img.height    = height;
    if(cim->img_format == RAW8)
        cim->img.bpp = 8;
    else
        cim->img.bpp    = 16;
    cim->frame_size    = cim->img.width * cim->img.height * (cim->img.bpp / 8);
}

static int x1021_cim_fb_alloc(struct cim_device *cim)
{
    cim->framebuf_paddr = NULL;

    //framebuf paddr aligned to 32-word boundary, bit[6:0] = 0;
    cim->framebuf_vaddr = dma_alloc_coherent(cim->dev,
                    cim->frame_size * cim->buf_cnt + 128,
                    (dma_addr_t *)&cim->framebuf_paddr, GFP_KERNEL);
    if (!cim->framebuf_paddr)
        return -ENOMEM;

    if ((unsigned int)cim->framebuf_paddr & 0x3f) {//not aligned
        cim->framebuf_paddr += ((unsigned int)cim->framebuf_paddr / 128 + 1) * 128 - (unsigned int)cim->framebuf_paddr;
        cim->framebuf_vaddr += ((unsigned int)cim->framebuf_vaddr / 128 + 1) * 128 - (unsigned int)cim->framebuf_vaddr;
    }
    return 0;
}

static void x1021_cim_fb_free(struct cim_device *cim)
{
    dma_free_coherent(cim->dev, cim->frame_size * cim->buf_cnt + 128,
                cim->framebuf_vaddr, GFP_KERNEL);
}


#define set_vic_dma_buf(NO)    \
    tx_isp_vic_writel(cim->framebuf_paddr + NO * cim->frame_size, cim->base + VIC_DMA_Y_BUF##NO); \
        tx_isp_vic_writel(cim->framebuf_paddr + NO * cim->frame_size, cim->base + VIC_DMA_UV_BUF##NO);

void set_img_format(struct cim_device* cim) {
    unsigned int y_stride, uvstride, base_mode;
    unsigned int db_cfg, dma_cfg;

    db_cfg = tx_isp_vic_readl(cim->base + VIC_DB_CFG);
    db_cfg &= ~DVP_DATA_FMT_MASK;
    db_cfg |= (cim->img_format << DVP_DATA_TYPE);
    tx_isp_vic_writel(db_cfg, cim->base + VIC_DB_CFG);

    switch (cim->img_format) {
        case RAW8:
            db_cfg &= ~YUV_DATA_ORDER_MASK;
            db_cfg |= YUYV;
            db_cfg &= ~DVP_DATA_FMT_MASK;
            db_cfg |= DVP_YUV422_16BIT;
            tx_isp_vic_writel(db_cfg, cim->base + VIC_DB_CFG);
            y_stride = cim->img.width;
            uvstride = 0;
            base_mode = DMA_OUT_FMT_NV12;
            break;
        case YUV422_8BIT:
            y_stride = cim->img.width * 2;
            uvstride = 0;
            base_mode = DMA_OUT_FMT_YUV422P;
            break;
        default:
            return;
    }

    tx_isp_vic_writel(y_stride, cim->base + VIC_INPUT_HSYNC_BLANKING);
    tx_isp_vic_writel(y_stride, cim->base + VIC_DMA_Y_STRID);
    tx_isp_vic_writel(uvstride, cim->base + VIC_DMA_UV_STRID);
    dma_cfg = tx_isp_vic_readl(cim->base + VIC_DMA_CONFIG);
    dma_cfg &= ~DMA_OUT_FMT_MASK;
    dma_cfg |= base_mode;
    tx_isp_vic_writel(dma_cfg, cim->base + VIC_DMA_CONFIG);
}

void set_dma_buf(struct cim_device* cim)
{
    unsigned int i;
    for(i = 0; i < cim->buf_cnt; i++)
    {
        if(i == 0)
            set_vic_dma_buf(0);
        if(i == 1)
            set_vic_dma_buf(1);
        if(i == 2)
            set_vic_dma_buf(2);
        if(i == 3)
            set_vic_dma_buf(3);
        if(i == 4)
            set_vic_dma_buf(4);

        cim->buf_flag[i] = 0;
    }
}

static void x1021_cim_reg_default_init(struct cim_device *cim)
{
    unsigned int ctrl_reg = 0;
    unsigned int res_reg = 0;
    unsigned int dma_cfg = 0;
    unsigned int timeout = 3000;

    res_reg = cim->img.width << H_RESOLUTION_OFF | cim->img.height << V_RESOLUTION_OFF;
    tx_isp_vic_writel(res_reg, cim->base + VIC_RESOLUTION);

    tx_isp_vic_writel(INTF_TYPE_DVP, cim->base + VIC_INTF_TYPE);

    tx_isp_vic_writel(1 << A_VALUE_OFF, cim->base + VIC_OUT_ABVAL);
    tx_isp_vic_writel(ISP_PRESET_MODE2 |VCKE_EN | (1 << AB_MODE_SELECT), cim->base + VIC_GLOBAL_CFG);
    tx_isp_vic_writel(FRAME_ECC_MODE | FRAME_ECC_EN, cim->base + VIC_FRM_ECC);
    tx_isp_vic_writel(GLB_RST | REG_ENABLE, cim->base + VIC_CONTROL);

    do {
        ctrl_reg = tx_isp_vic_readl(cim->base + VIC_CONTROL);
    } while (ctrl_reg && timeout-- > 0);

    if(timeout <= 0)
        printk("@@@@@@@@@@@@@@@@@@@check vic control reg timeout ... \n");

    tx_isp_vic_writel(cim->img.width << H_RES_OFF | cim->img.height << V_RES_OFF, cim->base + VIC_DMA_RESOLUTION);
    set_img_format(cim);
    set_dma_buf(cim);
    dma_cfg = tx_isp_vic_readl(cim->base + VIC_DMA_CONFIG);
    dma_cfg |= DMA_EN | BUF_NUM(cim->buf_cnt);
    tx_isp_vic_writel(dma_cfg, cim->base + VIC_DMA_CONFIG);

    tx_isp_vic_writel(0x1ff, cim->base + VIC_INT_CLR);
    tx_isp_vic_writel(0x10, cim->base + VIC_INT_MASK);
    //tx_isp_vic_writel(VIC_START, cim->base + VIC_CONTROL);
    cim->id = -1;

}

static int x1021_cim_lowlevel_init(struct cim_device *cim)
{
    int err;

    err = x1021_cim_fb_alloc(cim);
    if (err)
        goto exit;
    return 0;

exit:
    return err;
}

static void x1021_cim_lowlevel_deinit(struct cim_device *cim)
{
    x1021_cim_fb_free(cim);
}



static int x1021_cim_open(struct inode *inode, struct file *file)
{
    struct miscdevice *dev = file->private_data;
    struct cim_device *cim = container_of(dev, struct cim_device, misc_dev);

    if (atomic_inc_and_test(&cim->opened)){
        printk("init at the first open\n");
        x1021_cim_activate(cim);
        x1021_cim_reg_default_init(cim);
        cim->read_flag = 0;
    }
    //x1021_cim_dump_reg(cim);

    return 0;
}

static int x1021_cim_release(struct inode *inode, struct file *file)
{
    struct miscdevice *dev = file->private_data;
    struct cim_device *cim = container_of(dev, struct cim_device, misc_dev);

    if(atomic_sub_return(1, &cim->opened) == -1) {
        printk("release at the last close\n");
        x1021_cim_deactivate(cim);
        x1021_cim_lowlevel_deinit(cim);
        cim->dma_started = 0;
        cim->read_flag = 0;
    }
    return 0;
}

static ssize_t x1021_cim_read(struct file *file, char *buf, size_t size, loff_t *l)
{
    int ret;
    struct miscdevice *dev = file->private_data;
    struct cim_device *cim = container_of(dev, struct cim_device, misc_dev);
//    long diff_s;

    if(cim->id == -1)
    {
        dprintk(7, "no vic dma done occur !!!!!!!!!!!\n");
        return -EFAULT;
    }
    if(cim->buf_flag[cim->id] == 1)
    {
//        printk("%d \n", cim->id);
        cim->buf_flag[cim->id] = 0;
//        do_gettimeofday(&cim->latest);
        ret = copy_to_user(buf, cim->framebuf_vaddr + cim->id * cim->frame_size, size<cim->frame_size?size:cim->frame_size);
        if (ret) {
            printk("ERROR: copy_to_user failed at %s %d \n", __FUNCTION__, __LINE__);
            return -EFAULT;
        }
#if 0
        do_gettimeofday(&cim->now);
        diff_s = (cim->now.tv_sec*1000 + cim->now.tv_usec / 1000) -
            (cim->latest.tv_sec*1000 + cim->latest.tv_usec / 1000);
        printk("!!!!!!!!!!!!!!!!ms: %d\n", diff_s);
#endif
        ret = min(size, cim->frame_size);
        return ret;
//        dma_cache_sync(NULL, cim->framebuf_paddr, cim->frame_size * CIM_FRAME_BUF_CNT, DMA_TO_DEVICE);
    }
    else
    {
//        printk("cim->buf_flag[cim->id] == 0  error \n");
        return -EFAULT;
    }
}

static ssize_t x1021_cim_write(struct file *file, const char *buf, size_t size, loff_t *l)
{
    printk("cim error: write is not implemented\n");
    return -1;
}

static long x1021_cim_ioctl(struct file *file, unsigned int cmd, unsigned long arg)
{
    int err = 0;
    capt_param_t img;
    void __user *argp = (void __user *)arg;
    struct miscdevice *dev = file->private_data;
    struct cim_device *cim = container_of(dev, struct cim_device, misc_dev);

    switch (cmd) {
    case IOCTL_SET_IMG_FORMAT: // 1
        if (copy_from_user(&cim->img_format, (void *)arg, sizeof(img_format_t)))
            return -EFAULT;
        set_img_format(cim);
        printk("** img format = %d **\n", cim->img_format);
        break;

    case IOCTL_SET_TIMING_PARAM: // 3
        if (copy_from_user(&cim->timing, (void *)arg, sizeof(timing_param_t)))
            return -EFAULT;
        x1021_cim_set_timing(cim);
        break;

    case IOCTL_SET_IMG_PARAM: // 2
        if (copy_from_user(&img, (void *)arg, sizeof(capt_param_t)))
            return -EFAULT;
        if(img.width * img.height * (img.bpp / 8) > cim->frame_size)
        {
            x1021_cim_lowlevel_deinit(cim);
            x1021_cim_set_img(cim, img.width, img.height);
            err = x1021_cim_lowlevel_init(cim);
            if (err < 0) {
                printk("ERROR: x1021_cim_lowlevel_init failed %s %d\n", __FUNCTION__, __LINE__);
                return -EFAULT;
            }
        }else {
            x1021_cim_set_img(cim, img.width, img.height);
        }
        tx_isp_vic_writel((cim->img.width << H_RESOLUTION_OFF) | (cim->img.height  << V_RESOLUTION_OFF), cim->base + VIC_RESOLUTION);
        tx_isp_vic_writel((cim->img.width << H_RES_OFF) | (cim->img.height << V_RES_OFF),cim->base + VIC_DMA_RESOLUTION);
        set_img_format(cim);
        set_dma_buf(cim);
        break;
    case IOCTL_GET_FRAME_PHY:
#if 0
        offset = x1021_cim_get_frame(cim);
        if (offset == (unsigned int)(~0))
            return -1;
        offset -= (unsigned int)cim->framebuf_paddr;
        return copy_to_user(argp, &offset, sizeof(offset)) ? -EFAULT : 0;
#else
            break;
#endif
    case IOCTL_GET_FRAME:
#if 0
        if (cim->id == 0)
            err = x1021_cim_copy_frame(cim);
        else if (cim->id == 1)
            err = x1021_cim_get_frame(cim);
        if (err < 0){
            return -1;
        }
        offset = 0;
        return copy_to_user(argp, &offset, sizeof(offset)) ? -EFAULT : 0;
#else
            break;
#endif
    case IOCTL_START_DMA:
        if (cim->dma_started == 0) {
            tx_isp_vic_writel(0x1ff, cim->base + VIC_INT_CLR);
//            memset(cim->framebuf_vaddr, 0, cim->frame_size * CIM_FRAME_BUF_CNT);
            x1021_cim_start_dma(cim);
            cim->dma_started = 1;
        }
        break;

    case IOCTL_STOP_DMA:
        if (cim->dma_started == 1) {
            x1021_cim_stop_dma(cim);
            tx_isp_vic_writel(0x1ff, cim->base + VIC_INT_CLR);
            cim->dma_started = 0;
        }
        break;

    case IOCTL_GET_INFO:
    {
        struct camera_driver_info cameraInfo;

        cameraInfo.physical_address = (unsigned int)cim->framebuf_paddr;
        memcpy(&cameraInfo.parameters, &cim->img, sizeof(cim->img));
        cameraInfo.format = cim->img_format;
        return copy_to_user(argp, &cameraInfo, sizeof(cameraInfo)) ? -EFAULT : 0;
    }
    default:
        printk("Not supported command: 0x%x\n", cmd);
        return -EINVAL;
    }

    return 0;
}

static int x1021_cim_mmap(struct file *file, struct vm_area_struct *vma)
{
    unsigned long start = 0;
    unsigned long off;
    struct miscdevice *dev = file->private_data;
    struct cim_device *cim = container_of(dev, struct cim_device, misc_dev);

    if (cim->id == 0 && cim->shared_mem == NULL){
        printk("==>%s L%d: no mem\n", __func__, __LINE__);
        return -ENOMEM;
    }

    off = vma->vm_pgoff << PAGE_SHIFT;    //only surport one share buffer, offset is zero
    if (cim->id == 0)
        start = (unsigned int)virt_to_phys(cim->shared_mem);
    else if (cim->id == 1)
        start = (unsigned int)cim->framebuf_paddr;
    start &= PAGE_MASK;
    off += start;

    vma->vm_flags |= VM_IO;
    if (cim->id == 1)
        vma->vm_page_prot = pgprot_noncached(vma->vm_page_prot);
    if (remap_pfn_range(vma, vma->vm_start, off>>PAGE_SHIFT, vma->vm_end-vma->vm_start, vma->vm_page_prot))
        return -EAGAIN;

    return 0;
}

static struct file_operations x1021_cim_fops = {
    .owner        = THIS_MODULE,
    .open        = x1021_cim_open,
    .release    = x1021_cim_release,
    .read        = x1021_cim_read,
    .write        = x1021_cim_write,
    .unlocked_ioctl    = x1021_cim_ioctl,
    .mmap        = x1021_cim_mmap,
};

static int x1021_cim_probe(struct platform_device *pdev)
{
    struct cim_device *cim;
    struct resource *res;
    void __iomem *base;
    unsigned int irq;
    int err = 0;

    res = platform_get_resource(pdev, IORESOURCE_MEM, 0);
    irq = platform_get_irq(pdev, 0);
    if (!res || (int)irq <= 0) {
        err = -ENODEV;
        goto exit;
    }

    cim = kzalloc(sizeof(*cim), GFP_KERNEL);
    if (!cim) {
        printk("Could not allocate cim\n");
        err = -ENOMEM;
        goto exit;
    }

    sprintf(cim->name, "isp");
    cim->irq = irq;
    cim->res = res;
    cim->dev = &pdev->dev;
    cim->clk = clk_get(&pdev->dev, "isp");
    if (IS_ERR(cim->clk)) {
        printk(KERN_ERR "%s: get isp clk failed\n", __FUNCTION__);
        err = PTR_ERR(cim->clk);
        goto exit_kfree;
    }

    cim->mclk = clk_get(&pdev->dev, "cgu_cim");
    if (IS_ERR(cim->mclk)) {
        printk(KERN_ERR "%s: get isp mclk failed\n", __FUNCTION__);
        err = PTR_ERR(cim->mclk);
        goto exit_put_clk_cim;
    }

    cim->ispclk = clk_get(&pdev->dev, "cgu_isp");
    if (IS_ERR(cim->ispclk)) {
        printk(KERN_ERR "%s: get isp ispclk failed\n", __FUNCTION__);
        err = PTR_ERR(cim->ispclk);
        goto exit_put_clk_cgumcim;
    }

    /*
     * Request the regions.
     */
    if (!request_mem_region(res->start, resource_size(res), DRIVER_NAME)) {
        err = -EBUSY;
        goto exit_put_clk_cguispcim;
    }

    base = ioremap(res->start, resource_size(res));
    if (!base) {
        err = -ENOMEM;
        goto exit_release;
    }
    cim->base = base;

    /* request irq */
    err = request_irq(cim->irq, x1021_cim_irq_handler, IRQF_DISABLED,
                        dev_name(&pdev->dev), cim);
    if(err) {
        printk("request irq failed!\n");
        goto exit_iounmap;
    }

    spin_lock_init(&cim->lock);
    atomic_set(&cim->opened, -1);

    cim->misc_dev.minor = MISC_DYNAMIC_MINOR;
    cim->misc_dev.name = cim->name;
    cim->misc_dev.fops = &x1021_cim_fops;
    err = misc_register(&cim->misc_dev);
    if (err < 0)
        goto exit_free_irq;

    cim->buf_cnt = CIM_FRAME_BUF_CNT;
    cim->img_format = RAW8;
    x1021_cim_set_img(cim, CIM_IMG_DEFAULT_WIDTH, CIM_IMG_DEFAULT_HEIGHT);
    err = x1021_cim_lowlevel_init(cim);
    if (err < 0) {
        printk("ERROR: x1021_cim_lowlevel_init failed %s %d\n", __FUNCTION__, __LINE__);
        return -EFAULT;
    }

    cim->shared_mem = kmalloc(cim->frame_size, GFP_KERNEL);
    if (cim->shared_mem == NULL){
        printk(KERN_ERR "%s: isp kmalloc failed\n", __FUNCTION__);
        return -ENOMEM;
    }

    platform_set_drvdata(pdev, cim);

    printk("ingenic isp driver loaded\n");
    return 0;

exit_free_irq:
    free_irq(cim->irq, cim);
exit_iounmap:
    iounmap(base);
exit_release:
    release_mem_region(res->start, resource_size(res));
exit_put_clk_cguispcim:
    clk_put(cim->ispclk);
exit_put_clk_cgumcim:
    clk_put(cim->mclk);
exit_put_clk_cim:
    clk_put(cim->clk);
exit_kfree:
    kfree(cim);
exit:
    return err;
}

static int x1021_cim_remove(struct platform_device *pdev)
{
    struct cim_device *cim = platform_get_drvdata(pdev);
    struct resource *res;

    if (cim->shared_mem)
        kfree(cim->shared_mem);

    x1021_cim_lowlevel_deinit(cim);

    misc_deregister(&cim->misc_dev);

    free_irq(cim->irq, cim);

    clk_put(cim->clk);
    clk_put(cim->mclk);

    iounmap(cim->base);

    res = cim->res;
    release_mem_region(res->start, resource_size(res));

    kfree(cim);

    printk("ZK Camera driver unloaded\n");

    return 0;
}

static struct platform_driver x1021_cim_driver = {
    .driver     = {
        .name    = DRIVER_NAME,
        .owner    = THIS_MODULE,
    },
    .probe        = x1021_cim_probe,
    .remove        = x1021_cim_remove,
};

static int __init x1021_cim_init(void)
{
    return platform_driver_register(&x1021_cim_driver);
}

static void __exit x1021_cim_exit(void)
{
    platform_driver_unregister(&x1021_cim_driver);
}

module_init(x1021_cim_init);
module_exit(x1021_cim_exit);

MODULE_DESCRIPTION("Camera Host driver");
MODULE_AUTHOR("Zhang Xiaoyan <xiaoyan.zhang@ingenic.com>");
MODULE_LICENSE("GPL");
MODULE_ALIAS("platform:" DRIVER_NAME);
