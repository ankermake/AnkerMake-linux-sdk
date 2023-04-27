/*
 * Copyright (C) 2020 Ingenic Semiconductor Co., Ltd.
 *
 * Mulitiple Channel Scaler Driver
 *
 */

#include <linux/module.h>
#include <linux/miscdevice.h>
#include <linux/interrupt.h>
#include <linux/ioctl.h>
#include <linux/list.h>
#include <linux/clk.h>
#include <linux/gpio.h>
#include <linux/irq.h>
#include <linux/slab.h>
#include <linux/dma-mapping.h>
#include <linux/delay.h>
#include <linux/clk-provider.h>

#include <common.h>
#include <bit_field.h>
#include <utils/clock.h>

#include "mscaler_regs.h"
#include "mscaler.h"
#include "isp.h"
#include "vic.h"
#include "drivers/rmem_manager/rmem_manager.h"


#define MSCALER_ALIGN_SIZE              8

enum mscaler_channel_num {
    ISP_MSCALER_CHANNEL_NUM0        = 0,
    ISP_MSCALER_CHANNEL_NUM1,
    ISP_MSCALER_CHANNEL_NUM2,
    ISP_MSCALER_CHANNEL_MAX,
};

struct mscaler_data;

struct mscaler_channel {
    int index;
    int channel;
    int is_init;
    int is_power_on;
    int is_stream_on;     /* 0: stopped, 1: streaming */

    unsigned int wait_timeout;
    volatile unsigned int dma_index;
    wait_queue_head_t waiter;

    void *mem;
    unsigned int mem_cnt;
    unsigned int frm_size;
    unsigned int uv_data_offset;
    volatile unsigned int frame_counter;

    struct list_head free_list;
    struct list_head usable_list;
    struct frame_data *t[2];
    struct frame_data *frames;

    struct mscaler_data *drv;
    struct miscdevice mscaler_mdev;
    char device_name[16];   /* 设备节点名字 */

    struct mutex lock;
    struct spinlock spinlock;

    struct camera_info info;                /* 返回信息,应用保存映射地址 */
    struct frame_image_format output_fmt;   /* 输出信息 */

    unsigned int chn_done_cnt;      /* 通道处理完成帧计数 */
    unsigned int chn_output_cnt;    /* 通道输出完成帧计数 */

    unsigned int vic_ts_cnt_1s;     /* vic ts counter in 1s */
};

struct mscaler_data {
    int index;
    int is_enable;
    int is_finish;

    /* Camera Device */
    char *device_name;              /* 设备名字 */
    struct camera_device camera;
    struct mscaler_channel mscaler_ch[MSCALER_MAX_CH];

    struct mutex lock;

#ifdef SOC_CAMERA_DEBUG
    struct kobject *dsysfs_parent_kobj;
    struct kobject dsysfs_kobj;
#endif
};

static struct mscaler_data jz_mscaler_dev[2] = {
    {
        .index                  = 0,
        .device_name            = "mscaler0",
    },

    {
        .index                  = 1,
        .device_name            = "mscaler1",
    },
};


/*
 * MScaler Operation
 */
static const unsigned long mscaler_iobase[] = {
        KSEG1ADDR(MSCALER0_IOBASE),
        KSEG1ADDR(MSCALER1_IOBASE),
};

#define MSCALER_ADDR(index, reg)        ((volatile unsigned long *)((mscaler_iobase[index]) + (reg)))

static inline void mscaler_write_reg(int index, unsigned int reg, unsigned int val)
{
    *MSCALER_ADDR(index, reg) = val;
}

static inline unsigned int mscaler_read_reg(int index, unsigned int reg)
{
    return *MSCALER_ADDR(index, reg);
}

static inline void mscaler_set_bit(int index, unsigned int reg, unsigned int start, unsigned int end, unsigned int val)
{
    set_bit_field(MSCALER_ADDR(index, reg), start, end, val);
}

static inline unsigned int mscaler_get_bit(int index, unsigned int reg, unsigned int start, unsigned int end)
{
    return get_bit_field(MSCALER_ADDR(index, reg), start, end);
}

static inline void mscaler_module_clock_disable(int index)
{
    mscaler_write_reg(index, MSCA_CLK_DIS, 1);
}

static inline void mscaler_module_clock_enable(int index)
{
    mscaler_write_reg(index, MSCA_CLK_DIS, 0);
}

static void add_to_free_list(struct mscaler_channel *data, struct frame_data *frm)
{
    frm->status = frame_status_free;
    list_add_tail(&frm->link, &data->free_list);
}

static struct frame_data *get_free_frm(struct mscaler_channel *data)
{
    if (list_empty(&data->free_list))
        return NULL;

    struct frame_data *frm = list_first_entry(&data->free_list, struct frame_data, link);

    list_del(&frm->link);

    return frm;
}

static void add_to_usable_list(struct mscaler_channel *data, struct frame_data *frm)
{
    frm->status = frame_status_usable;
    data->frame_counter++;
    list_add_tail(&frm->link, &data->usable_list);
}

static struct frame_data *get_usable_frm(struct mscaler_channel *data)
{
    if (list_empty(&data->usable_list))
        return NULL;

    data->frame_counter--;

    struct frame_data *frm = list_first_entry(&data->usable_list, struct frame_data, link);

    list_del(&frm->link);

    return frm;
}

enum mscaler_algo_mode {
    //一般放大和轻微缩小双三次效果好，缩小较多均值效果好
    MSCALER_ALGO_AVG,                   /* 均值插值 */
    MSCALER_ALGO_BICUBIC,               /* 双三次插值 */
};

struct mscaler_algo_coef {
    //均值插值系数，四个像素的权重（和为512）
    short pw1, pw2, pw3, pw4;
};

struct mscaler_algo_attr {
    enum mscaler_algo_mode mode;        /* 插值算法 */
    struct mscaler_algo_coef avg_coef;  /* 均值插值系数 */
};

static int mscaler_set_scale_algo(struct mscaler_channel *data, struct mscaler_algo_attr *attr)
{
    int index = data->index;
    int channel = data->channel;
    struct frame_image_format *output_fmt = &data->output_fmt;
    unsigned int val = 0;
    unsigned int value0, value1;

    if (!data->is_stream_on) {
        printk(KERN_ERR "%s fail ,please stream on!\n", data->device_name);
        return -EPERM;
    }
    if (!output_fmt->scaler.enable)
        return 0;

    mutex_lock(&data->lock);

    val = mscaler_read_reg(index, MSCA_CH_EN);
    if (attr->mode == MSCALER_ALGO_AVG) {
        if ((attr->avg_coef.pw1 + attr->avg_coef.pw2 + attr->avg_coef.pw3 + attr->avg_coef.pw4) != 512) {
            printk(KERN_ERR "%s avg coef err, 4 pw sum must equal 512!\n", data->device_name);
            mutex_unlock(&data->lock);
            return -EINVAL;
        }

        value0 = (attr->avg_coef.pw1 & 0x7ff) << 11 | (attr->avg_coef.pw2 & 0x7ff);
        value1 = (attr->avg_coef.pw3 & 0x7ff) << 11 | (attr->avg_coef.pw4 & 0x7ff);
        mscaler_write_reg(index, COE_ZERO_VRSZ_H(channel), value0);
        mscaler_write_reg(index, COE_ZERO_VRSZ_L(channel), value1);
        mscaler_write_reg(index, COE_ZERO_HRSZ_H(channel), value0);
        mscaler_write_reg(index, COE_ZERO_HRSZ_L(channel), value1);

        val |= ((1 << 8) | (1 << 11)) << channel;
    } else if (attr->mode == MSCALER_ALGO_BICUBIC) {
        value0 = (0 << 11) | 512;
        value1 = (0 << 11) | 0;
        mscaler_write_reg(index, COE_ZERO_VRSZ_H(channel), value0);
        mscaler_write_reg(index, COE_ZERO_VRSZ_L(channel), value1);
        mscaler_write_reg(index, COE_ZERO_HRSZ_H(channel), value0);
        mscaler_write_reg(index, COE_ZERO_HRSZ_L(channel), value1);

        val &= ~(((1 << 8) | (1 << 11)) << channel);
    }
    mscaler_write_reg(index, MSCA_CH_EN, val);

    mutex_unlock(&data->lock);

    return 0;
}

static int mscaler_hal_stream_on(struct mscaler_channel *data)
{
    int index = data->index;
    int channel = data->channel;
    struct mscaler_data *drv = data->drv;
    int ret = 0;

    struct sensor_attr *attr = drv->camera.sensor;
    struct frame_image_format *output_fmt = &data->output_fmt;

    unsigned int val = 0;
    unsigned int fmt = 0;
    int input_width;
    int input_height;

    if ( (attr->dbus_type == SENSOR_DATA_BUS_MIPI) && (attr->mipi.mipi_crop.enable) ) {
        input_width = attr->mipi.mipi_crop.output_width;
        input_height = attr->mipi.mipi_crop.output_height;
    } else {
        input_width = attr->sensor_info.width;
        input_height = attr->sensor_info.height;
    }

    unsigned int output_width = input_width;
    unsigned int output_height = input_height;
    unsigned int output_format = output_fmt->pixel_format;

    switch (output_format) {
    case CAMERA_PIX_FMT_NV21:
        fmt = OUT_FMT_NV21;
        break;

    case CAMERA_PIX_FMT_NV12:
    case CAMERA_PIX_FMT_YVU420:
    case CAMERA_PIX_FMT_JZ420B:
        fmt = OUT_FMT_NV12;
        break;

    case CAMERA_PIX_FMT_GREY:
        fmt = OUT_FMT_NV12;
        fmt |= OUT_FMT_Y_OUT_ONLY;
        break;

    default:
        printk("%s : output format not supported!\n", data->device_name);
        return -EINVAL;
    }

    /*
     * 1. mscaler_hw_configure
     */

    /* scaler */
    if (output_fmt->scaler.enable) {
        output_width = output_fmt->scaler.width;
        output_width = ALIGN(output_width, MSCALER_ALIGN_SIZE);
        output_height = output_fmt->scaler.height;
    }

    unsigned int step_w = 0, step_h = 0;
    step_w = input_width * 512 / output_width;
    step_h = input_height * 512 / output_height;

    unsigned long resize_step = 0;
    set_bit_field(&resize_step, RESIZW_STEP_WIDTH, step_w);
    set_bit_field(&resize_step, RESIZW_STEP_HEIGHT, step_h);
    mscaler_write_reg(index, RSZ_STEP(channel), resize_step);


    unsigned long resize_output_image = 0;
    set_bit_field(&resize_output_image, RESIZE_OSIZE_WIDTH, output_width);
    set_bit_field(&resize_output_image, RESIZE_OSIZE_HEIGHT, output_height);
    mscaler_write_reg(index, RSZ_OSIZE(channel), resize_output_image);

    /* set scaler coef */
    if (output_fmt->scaler.enable) {
        if ((input_width > output_fmt->scaler.width || input_height > output_fmt->scaler.height)) {
            if (output_fmt->scaler.width >= 1280 && output_fmt->scaler.height >= 720) {
                mscaler_write_reg(index, COE_ZERO_VRSZ_H(channel), (0 << 11) | 512);
                mscaler_write_reg(index, COE_ZERO_VRSZ_L(channel), (0 << 11) | 0);
                mscaler_write_reg(index, COE_ZERO_HRSZ_H(channel), (0 << 11) | 512);
                mscaler_write_reg(index, COE_ZERO_HRSZ_L(channel), (0 << 11) | 0);
            } else if (output_fmt->scaler.width >= 960 && output_fmt->scaler.height >= 540) {
                mscaler_write_reg(index, COE_ZERO_VRSZ_H(channel), (0 << 11) | 256);
                mscaler_write_reg(index, COE_ZERO_VRSZ_L(channel), (0 << 11) | 256);
                mscaler_write_reg(index, COE_ZERO_HRSZ_H(channel), (0 << 11) | 256);
                mscaler_write_reg(index, COE_ZERO_HRSZ_L(channel), (0 << 11) | 256);
            } else {
                mscaler_write_reg(index, COE_ZERO_VRSZ_H(channel), (128 << 11) | 128);
                mscaler_write_reg(index, COE_ZERO_VRSZ_L(channel), (128 << 11) | 128);
                mscaler_write_reg(index, COE_ZERO_HRSZ_H(channel), (128 << 11) | 128);
                mscaler_write_reg(index, COE_ZERO_HRSZ_L(channel), (128 << 11) | 128);
            }
        } else {
            mscaler_write_reg(index, COE_ZERO_VRSZ_H(channel), (0 << 11) | 512);
            mscaler_write_reg(index, COE_ZERO_VRSZ_L(channel), (0 << 11) | 0);
            mscaler_write_reg(index, COE_ZERO_HRSZ_H(channel), (0 << 11) | 512);
            mscaler_write_reg(index, COE_ZERO_HRSZ_L(channel), (0 << 11) | 0);
        }
    }

    /* crop */
    unsigned long crop_pos = 0;
    if (output_fmt->crop.enable) {
        output_width = output_fmt->crop.width;
        output_width = ALIGN(output_width, MSCALER_ALIGN_SIZE);
        output_height = output_fmt->crop.height;

        set_bit_field(&crop_pos, CROP_OPOS_START_X, output_fmt->crop.left);
        set_bit_field(&crop_pos, CROP_OPOS_START_Y, output_fmt->crop.top);
    }

    mscaler_write_reg(index, CROP_OPOS(channel), crop_pos);

    unsigned long crop_size = 0;
    set_bit_field(&crop_size, CROP_OSIZE_WIDTH, output_width);
    set_bit_field(&crop_size, CROP_OSIZE_HEIGHT, output_height);
    mscaler_write_reg(index, CROP_OSIZE(channel), crop_size);

    /* output image stride */
    mscaler_write_reg(index, DMAOUT_Y_STRI(channel), output_width);
    mscaler_write_reg(index, DMAOUT_UV_STRI(channel), output_width);


    /* uv pixel re-sample mode  */
    val = mscaler_read_reg(index, MSCA_DMAOUT_ARB);
    val |= 1 << (channel + 1);
    mscaler_write_reg(index, MSCA_DMAOUT_ARB, val);

    /* output format set */
    mscaler_write_reg(index, OUT_FMT(channel), fmt);

    /*
     * 2. mscaler_hw_start
     */
    val = mscaler_read_reg(index, MSCA_CH_EN);
    val |= 1 << channel;            /* resize enable */
    val |= (1 << 8) << channel;     /* vertical resize enable */
    val |= (1 << 11) << channel;    /* horizontal resize_enable */
    mscaler_write_reg(index, MSCA_CH_EN, val);

    return ret;
}

static int mscaler_hal_stream_off(struct mscaler_channel *data)
{
    int index = data->index;
    int channel = data->channel;

    /* resize disable */
    unsigned int timeout = 0xffffff;
    unsigned int val = 0;
    val = mscaler_read_reg(index, MSCA_CH_EN);
    val &= ~(1 << channel);
    mscaler_write_reg(index, MSCA_CH_EN, val);

    /* polling status to make sure channel stopped */
    do {
        val = mscaler_read_reg(index, MSCA_CH_STA);
    } while(!!(val & (1<<channel)) && --timeout);

    if(!timeout)
        printk(KERN_ERR "%s : [Warning] mscaler disable timeout!\n", data->device_name);

    /* clear fifo */
    mscaler_write_reg(index, DMAOUT_Y_ADDR_CLR(channel), 1);
    mscaler_write_reg(index, DMAOUT_UV_ADDR_CLR(channel), 1);
    mscaler_write_reg(index, DMAOUT_Y_LAST_ADDR_CLR(channel), 1);
    mscaler_write_reg(index, DMAOUT_UV_LAST_ADDR_CLR(channel), 1);

    return 0;
}

static inline void set_mscaler_dma_addr(struct mscaler_channel *data, struct frame_data *frm)
{
    unsigned int fifo_full = 0;
    unsigned int y_fifo_status = 0, uv_fifo_status = 0;
    int index = data->index;
    int channel = data->channel;

    y_fifo_status = mscaler_read_reg(index, Y_ADDR_FIFO_STA(channel));
    uv_fifo_status = mscaler_read_reg(index, UV_ADDR_FIFO_STA(channel));

    if ((y_fifo_status & MSCA_CHx_Y_ADDR_FIFO_STA_FULL) ||
            (uv_fifo_status & MSCA_CHx_UV_ADDR_FIFO_STA_FULL)) {
        fifo_full = 1;
    }

    /* Set DMA Address */
    if (fifo_full)
        panic("%s : failed to set dma addr\n", data->device_name);

    dma_addr_t y_addr = 0;
    dma_addr_t uv_addr = 0;

    frm->status = frame_status_trans;
    /*Y*/
    y_addr = frm->info.paddr;
    mscaler_write_reg(index, DMAOUT_Y_ADDR(channel), y_addr);

    /*UV*/
    uv_addr = y_addr + data->uv_data_offset;
    mscaler_write_reg(index, DMAOUT_UV_ADDR(channel), uv_addr);
}

/*
 * In irq context
 */
static int mscaler_irq_notify_ch_done(int index, int channel)
{
    struct mscaler_data *drv = &jz_mscaler_dev[index];
    struct mscaler_channel *data = &drv->mscaler_ch[channel];

    unsigned int y_last_addr = 0;
    unsigned int uv_last_addr = 0;
    unsigned int y_frame_num = 0;
    unsigned int uv_frame_num = 0;

    unsigned long flags = 0;

    spin_lock_irqsave(&data->spinlock, flags);

    data->chn_done_cnt++;

    /* 由于mscaler stop stream时, ISP 可能还会产生中断,这个时候不用处理 */
    if(data->is_stream_on == 0) {
        spin_unlock_irqrestore(&data->spinlock, flags);
        return 0;
    }

    /* 读取出当前数据的地址 */
    y_last_addr = mscaler_read_reg(index, DMAOUT_Y_LAST_ADDR(channel));
    y_frame_num = mscaler_read_reg(index, DMAOUT_Y_LAST_STATS_NUM(channel));

    uv_last_addr = mscaler_read_reg(index, DMAOUT_UV_LAST_ADDR(channel));
    uv_frame_num = mscaler_read_reg(index, DMAOUT_UV_LAST_STATS_NUM(channel));

    struct frame_data *frm, *usable_frm = NULL;

    volatile int frame_index = data->dma_index;
    data->dma_index = !frame_index;

    /*
     * 当传输列表只有一帧的时候，不能用这一帧
     */
    if (data->t[0] != data->t[1]) {
        usable_frm = data->t[frame_index];
    }

    /*
     * 1 优先从空闲列表中获取新的帧加入传输
     * 2 如果空闲列表没有帧，那么传输完成列表中获取
     * 3 如果完成列表也没有，那么用下一帧做保底
     */
    frm = get_free_frm(data);
    if (!frm)
        frm = get_usable_frm(data);
    if (!frm)
        frm = data->t[!frame_index];

    data->t[frame_index] = frm;

    set_mscaler_dma_addr(data, frm);

    if (usable_frm) {
        usable_frm->info.sequence = data->chn_done_cnt;
        usable_frm->info.timestamp = get_time_us();

        add_to_usable_list(data, usable_frm);
        wake_up_all(&data->waiter);
    }

    spin_unlock_irqrestore(&data->spinlock, flags);

    return 0;
}

static inline void mscaler_dump_reg(int index)
{
    printk("MSCA_CTRL                       :0x%08x\n", mscaler_read_reg(index, MSCA_CTRL));
    printk("MSCA_CH_EN                      :0x%08x\n", mscaler_read_reg(index, MSCA_CH_EN));
    printk("MSCA_CH_STA                     :0x%08x\n", mscaler_read_reg(index, MSCA_CH_STA));
    printk("MSCA_DMAOUT_ARB                 :0x%08x\n", mscaler_read_reg(index, MSCA_DMAOUT_ARB));
    printk("MSCA_CLK_GATE_EN                :0x%08x\n", mscaler_read_reg(index, MSCA_CLK_GATE_EN));
    printk("MSCA_CLK_DIS                    :0x%08x\n", mscaler_read_reg(index, MSCA_CLK_DIS));
    printk("MSCA_SRC_IN                     :0x%08x\n", mscaler_read_reg(index, MSCA_SRC_IN));
    printk("MSCA_GLO_RSZ_COEF_WR            :0x%08x\n", mscaler_read_reg(index, MSCA_GLO_RSZ_COEF_WR));
    printk("MSCA_SYS_PRO_CLK_EN             :0x%08x\n", mscaler_read_reg(index, MSCA_SYS_PRO_CLK_EN));
    printk("MSCA_DS0_CLK_NUM                :0x%08x\n", mscaler_read_reg(index, MSCA_DS0_CLK_NUM));
    printk("MSCA_DS1_CLK_NUM                :0x%08x\n", mscaler_read_reg(index, MSCA_DS1_CLK_NUM));
    printk("MSCA_DS2_CLK_NUM                :0x%08x\n", mscaler_read_reg(index, MSCA_DS2_CLK_NUM));
    printk("\n");
    printk("\n");

    int channel = 0;
    for (channel = 0; channel < MSCALER_MAX_CH; channel++) {
        printk("RSZ_OSIZE(%d)               :0x%08x\n", channel, mscaler_read_reg(index, RSZ_OSIZE(channel)));
        printk("RSZ_STEP(%d)                :0x%08x\n", channel, mscaler_read_reg(index, RSZ_STEP(channel)));
        printk("CROP_OPOS(%d)               :0x%08x\n", channel, mscaler_read_reg(index, CROP_OPOS(channel)));
        printk("CROP_OSIZE(%d)              :0x%08x\n", channel, mscaler_read_reg(index, CROP_OSIZE(channel)));
        printk("FRA_CTRL_LOOP(%d)           :0x%08x\n", channel, mscaler_read_reg(index, FRA_CTRL_LOOP(channel)));
        printk("FRA_CTRL_MASK(%d)           :0x%08x\n", channel, mscaler_read_reg(index, FRA_CTRL_MASK(channel)));
        printk("MS0_POS(%d)                 :0x%08x\n", channel, mscaler_read_reg(index, MS0_POS(channel)));
        printk("MS0_SIZE(%d)                :0x%08x\n", channel, mscaler_read_reg(index, MS0_SIZE(channel)));
        printk("MS0_VALUE(%d)               :0x%08x\n", channel, mscaler_read_reg(index, MS0_VALUE(channel)));
        printk("MS1_POS(%d)                 :0x%08x\n", channel, mscaler_read_reg(index, MS1_POS(channel)));
        printk("MS1_SIZE(%d)                :0x%08x\n", channel, mscaler_read_reg(index, MS1_SIZE(channel)));
        printk("MS1_VALUE(%d)               :0x%08x\n", channel, mscaler_read_reg(index, MS1_VALUE(channel)));
        printk("MS2_POS(%d)                 :0x%08x\n", channel, mscaler_read_reg(index, MS2_POS(channel)));
        printk("MS2_SIZE(%d)                :0x%08x\n", channel, mscaler_read_reg(index, MS2_SIZE(channel)));
        printk("MS2_VALUE(%d)               :0x%08x\n", channel, mscaler_read_reg(index, MS2_VALUE(channel)));
        printk("MS3_POS(%d)                 :0x%08x\n", channel, mscaler_read_reg(index, MS3_POS(channel)));
        printk("MS3_SIZE(%d)                :0x%08x\n", channel, mscaler_read_reg(index, MS3_SIZE(channel)));
        printk("MS3_VALUE(%d)               :0x%08x\n", channel, mscaler_read_reg(index, MS3_VALUE(channel)));
        printk("OUT_FMT(%d)                 :0x%08x\n", channel, mscaler_read_reg(index, OUT_FMT(channel)));
        printk("DMAOUT_Y_ADDR(%d)           :0x%08x\n", channel, mscaler_read_reg(index, DMAOUT_Y_ADDR(channel)));
        printk("Y_ADDR_FIFO_STA(%d)         :0x%08x\n", channel, mscaler_read_reg(index, Y_ADDR_FIFO_STA(channel)));

        printk("Y_LAST_ADDR_FIFO_STA(%d)    :0x%08x\n", channel, mscaler_read_reg(index, Y_LAST_ADDR_FIFO_STA(channel)));
        printk("DMAOUT_Y_STRI(%d)           :0x%08x\n", channel, mscaler_read_reg(index, DMAOUT_Y_STRI(channel)));
        printk("DMAOUT_UV_ADDR(%d)          :0x%08x\n", channel, mscaler_read_reg(index, DMAOUT_UV_ADDR(channel)));
        printk("UV_ADDR_FIFO_STA(%d)        :0x%08x\n", channel, mscaler_read_reg(index, UV_ADDR_FIFO_STA(channel)));

        printk("UV_LAST_ADDR_FIFO_STA(%d)   :0x%08x\n", channel, mscaler_read_reg(index, UV_LAST_ADDR_FIFO_STA(channel)));
        printk("DMAOUT_UV_STRI(%d)          :0x%08x\n", channel, mscaler_read_reg(index, DMAOUT_UV_STRI(channel)));
        printk("DMAOUT_Y_ADDR_CLR(%d)       :0x%08x\n", channel, mscaler_read_reg(index, DMAOUT_Y_ADDR_CLR(channel)));
        printk("DMAOUT_UV_ADDR_CLR(%d)      :0x%08x\n", channel, mscaler_read_reg(index, DMAOUT_UV_ADDR_CLR(channel)));
        printk("DMAOUT_Y_LAST_ADDR_CLR(%d)  :0x%08x\n", channel, mscaler_read_reg(index, DMAOUT_Y_LAST_ADDR_CLR(channel)));
        printk("DMAOUT_UV_LAST_ADDR_CLR(%d) :0x%08x\n", channel, mscaler_read_reg(index, DMAOUT_UV_LAST_ADDR_CLR(channel)));
        printk("DMAOUT_Y_ADDR_SEL(%d)       :0x%08x\n", channel, mscaler_read_reg(index, DMAOUT_Y_ADDR_SEL(channel)));
        printk("DMAOUT_UV_ADDR_SEL(%d)      :0x%08x\n", channel, mscaler_read_reg(index, DMAOUT_UV_ADDR_SEL(channel)));
        printk("\n");
    }
}

static void init_frm_lists(struct mscaler_channel *data)
{
    INIT_LIST_HEAD(&data->free_list);
    INIT_LIST_HEAD(&data->usable_list);
    data->dma_index = 0;
    data->frame_counter = 0;
    data->wait_timeout = 0;

    int i;
    for (i = 0; i < data->mem_cnt; i++) {
        struct frame_data *frm = &data->frames[i];
        frm->addr = data->mem + i * data->frm_size;

        frm->info.index = i;
        frm->info.width = data->output_fmt.width;
        frm->info.height = data->output_fmt.height;
        frm->info.pixfmt = data->output_fmt.pixel_format;
        frm->info.size = data->output_fmt.frame_size;
        frm->info.paddr = virt_to_phys(frm->addr);
        add_to_free_list(data, frm);
    }
}

static void reset_frm_lists(struct mscaler_channel *data)
{
    int i;

    for (i = 0; i < data->mem_cnt; i++) {
        struct frame_data *frm = &data->frames[i];
        if (frm->status == frame_status_user)
            continue;

        if (data->t[0] != frm && data->t[1] != frm)
            list_del(&frm->link);
        add_to_free_list(data, frm);
    }

    data->dma_index = 0;
    data->frame_counter = 0;
    data->wait_timeout = 0;
}

static inline void m_cache_sync(void *mem, int size)
{
    dma_cache_sync(NULL, mem, size, DMA_FROM_DEVICE);
}

static int mscaler_alloc_mem(struct mscaler_channel *data)
{
    struct mscaler_data *drv = data->drv;
    struct frame_image_format *output_fmt = &data->output_fmt;

    int mem_cnt = output_fmt->frame_nums + 1;
    int frame_size;     /* 帧对齐之后大小 */
    int line_size;      /* 行对齐之后大小 */
    int uv_data_offset;
    int output_width;
    int output_height;
    int frame_align_size;

    assert(mem_cnt >= 3);
    assert(data->mem == NULL);

    /*
     * 数据输出分辨率判断顺序(用于申请buf):
     * 首先crop使能, 则为 crop输出分辨率
     * 其次scaler使能,则为scaler输出分辨率
     * 最后,上述功能均是禁止,则输出分辨率与输入分辨率相同
     */
    if (output_fmt->crop.enable) {
        output_width = output_fmt->crop.width;
        output_height = output_fmt->crop.height;
    } else if (output_fmt->scaler.enable) {
        output_width = output_fmt->scaler.width;
        output_height = output_fmt->scaler.height;
    } else {
        if ( (drv->camera.sensor->dbus_type == SENSOR_DATA_BUS_MIPI) && (drv->camera.sensor->mipi.mipi_crop.enable) ) {
            output_width = drv->camera.sensor->mipi.mipi_crop.output_width;
            output_height = drv->camera.sensor->mipi.mipi_crop.output_height;
        } else {
            output_width = drv->camera.sensor->sensor_info.width;
            output_height = drv->camera.sensor->sensor_info.height;
        }
    }

    output_width = ALIGN(output_width, MSCALER_ALIGN_SIZE);

    if (camera_fmt_is_NV12(output_fmt->pixel_format)) {
        line_size = output_width;
        frame_size = line_size * output_height;
        uv_data_offset = frame_size;
        frame_size += frame_size / 2;

        output_fmt->frame_size = output_width * output_height * 3 / 2;

    } else if (camera_fmt_is_8BIT(output_fmt->pixel_format)) {
        line_size = output_width;
        frame_size = line_size * output_height;
        uv_data_offset = 0;

        output_fmt->frame_size = output_width * output_height;

    } else {
        line_size = output_width * 2;
        frame_size = line_size * output_height;
        uv_data_offset = 0;

        output_fmt->frame_size = output_width * output_height * 2;
    }

    //frame_align_size = ALIGN(frame_size, PAGE_SIZE);
    frame_align_size = ALIGN((frame_size + sizeof(unsigned long long)), PAGE_SIZE); /* sizeof(unsigned long long) is vic timestamp size */
    data->mem = rmem_alloc_aligned(frame_align_size * mem_cnt, PAGE_SIZE);
    if (data->mem == NULL) {
        printk(KERN_ERR "%s : failed to alloc mem: %u\n", data->device_name, frame_align_size * mem_cnt);
        return -ENOMEM;
    }

    data->frm_size = frame_align_size;
    data->mem_cnt = mem_cnt;
    data->uv_data_offset = uv_data_offset;

    data->frames = kzalloc(data->mem_cnt * sizeof(data->frames[0]), GFP_KERNEL);
    assert(data->frames);

    init_frm_lists(data);

    /* 计算一帧时间 :单位us */
    drv->camera.sensor->info.fps =
            (drv->camera.sensor->sensor_info.fps >> 16) / (drv->camera.sensor->sensor_info.fps & 0xFFFF);
    /* 设置输出的info信息 */
    memcpy(&data->info, &drv->camera.sensor->info, sizeof(data->info));
    data->info.width = output_width;
    data->info.height = output_height;
    data->info.frame_size = frame_size;
    data->info.frame_align_size = frame_align_size;
    data->info.frame_nums = mem_cnt;
    data->info.line_length = line_size;
    data->info.data_fmt   = output_fmt->pixel_format;
    data->info.phys_mem = virt_to_phys(data->mem);

    return 0;
}

static int mscaler_free_mem(struct mscaler_channel *data)
{
    if (data->mem) {
        rmem_free(data->mem, data->mem_cnt * data->frm_size);
        kfree(data->frames);
        data->t[0] = NULL;
        data->t[1] = NULL;
        data->mem = NULL;
        data->frames = NULL;
    }

    return 0;
}

static void init_dma_addr(struct mscaler_channel *data)
{
    struct frame_data *frm;

    frm = get_free_frm(data);
    data->t[0] = frm;
    set_mscaler_dma_addr(data, frm);

    frm = get_free_frm(data);
    if (!frm)
        frm = data->t[0];
    data->t[1] = frm;

    set_mscaler_dma_addr(data, frm);
}

int mscaler_interrupt_service_routine(int index, unsigned int status)
{
    int ret = 0;

    if(status & (1 << MSCA_CH0_FRM_DONE_INT)) {
        ret = mscaler_irq_notify_ch_done(index, 0);
    }

    if(status & (1 << MSCA_CH1_FRM_DONE_INT)) {
        ret = mscaler_irq_notify_ch_done(index, 1);
    }

    if(status & (1 << MSCA_CH2_FRM_DONE_INT)) {
        ret = mscaler_irq_notify_ch_done(index, 2);
    }

    /*TODO: add handler.*/
    if(status & (1 << MSCA_CH0_CROP_ERR_INT)) {

    }
    if(status & (1 << MSCA_CH1_CROP_ERR_INT)) {

    }
    if(status & (1 << MSCA_CH2_CROP_ERR_INT)) {

    }

    return ret;
}

static unsigned int mscaler_output_res[][2] = {
    {MSCALER_OUTPUT0_MAX_WIDTH, MSCALER_OUTPUT0_MAX_HEIGHT},
    {MSCALER_OUTPUT1_MAX_WIDTH, MSCALER_OUTPUT1_MAX_HEIGHT},
    {MSCALER_OUTPUT2_MAX_WIDTH, MSCALER_OUTPUT2_MAX_HEIGHT},
};

static void camera_stream_off(struct mscaler_channel *data)
{
    struct mscaler_data *drv = data->drv;
    int index = data->index;

    mutex_lock(&drv->lock);

    if (--drv->camera.is_stream_on == 0) {
        isp_stream_off(index, drv->camera.sensor);
    }

    mutex_unlock(&drv->lock);
}

static int camera_stream_on(struct mscaler_channel *data)
{
    struct mscaler_data *drv = data->drv;
    int index = data->index;
    int ret = 0;

    mutex_lock(&drv->lock);

    if (drv->camera.is_stream_on++ == 0) {
        isp_stream_on(index,  drv->camera.sensor);
    }

    mutex_unlock(&drv->lock);

    return ret;
}

static int camera_power_on(struct mscaler_channel *data)
{
    struct mscaler_data *drv = data->drv;
    int ret = 0;

    mutex_lock(&drv->lock);

    if (drv->camera.is_power_on == 0)
        ret = isp_power_on(drv->index);

    if (!ret)
        drv->camera.is_power_on++;

    mutex_unlock(&drv->lock);

    return ret;
}

static void camera_power_off(struct mscaler_channel *data)
{
    struct mscaler_data *drv = data->drv;

    mutex_lock(&drv->lock);

    drv->camera.is_power_on--;
    if (drv->camera.is_power_on == 0)
        isp_power_off(drv->index);

    mutex_unlock(&drv->lock);
}

static void mscaler_stream_off_nolock(struct mscaler_channel *data)
{
    unsigned long flags = 0;

    if (!data->is_stream_on)
        return;

    spin_lock_irqsave(&data->spinlock, flags);

    mscaler_hal_stream_off(data);

    data->is_stream_on = 0;

    spin_unlock_irqrestore(&data->spinlock, flags);

    camera_stream_off(data);
}

static int mscaler_stream_on_nolock(struct mscaler_channel *data)
{
    int ret = 0;
    unsigned long flags = 0;

    if (data->is_stream_on)
        return 0;


    reset_frm_lists(data);

    init_dma_addr(data);

    spin_lock_irqsave(&data->spinlock, flags);

    ret = mscaler_hal_stream_on(data);
    if (ret < 0) {
        spin_unlock_irqrestore(&data->spinlock, flags);
        printk(KERN_ERR "%s : failed to stream on mscaler, %d\n", data->device_name, ret);
        return ret;
    }

    spin_unlock_irqrestore(&data->spinlock, flags);

    ret = camera_stream_on(data);
    if (ret) {
        printk(KERN_ERR "%s : failed to stream on camera, %d\n", data->device_name, ret);

        spin_lock_irqsave(&data->spinlock, flags);
        mscaler_hal_stream_off(data);
        spin_unlock_irqrestore(&data->spinlock, flags);
        return ret;
    }

    data->vic_ts_cnt_1s = vic_enable_ts(data->index, data->channel, data->output_fmt.frame_size);

    data->is_stream_on = 1;

    return 0;
}

static void mscaler_power_off_nolock(struct mscaler_channel *data)
{
    if (!data->is_power_on)
        return;

    mscaler_stream_off_nolock(data);

    camera_power_off(data);

    data->is_power_on = 0;
}

static int mscaler_power_on_nolock(struct mscaler_channel *data)
{
    if (data->is_power_on)
        return 0;

    int ret = camera_power_on(data);
    if (ret) {
        printk(KERN_ERR "%s : failed to power on camera, %d\n", data->device_name, ret);
        return ret;
    }

    data->is_power_on = 1;

    return ret;
}

static inline void camera_dump_frame_format(struct frame_image_format *fmt)
{
    if (fmt == NULL) {
        printk(KERN_ERR "fmt is NULL");
        return ;
    }

    printk(KERN_ERR "========dump frame image format=======\n");

    printk(KERN_ERR "width    = %d\n", fmt->width);
    printk(KERN_ERR "height    = %d\n", fmt->height);
    printk(KERN_ERR "pixel_format    = %d\n", fmt->pixel_format);
    printk(KERN_ERR "frame_size      = %d\n", fmt->frame_size);

    printk(KERN_ERR "scaler_enable   = %d\n", fmt->scaler.enable);
    printk(KERN_ERR "scaler_width    = %d\n", fmt->scaler.width);
    printk(KERN_ERR "scaler_height   = %d\n", fmt->scaler.height);

    printk(KERN_ERR "crop_enable     = %d\n", fmt->crop.enable);
    printk(KERN_ERR "crop_top        = %d\n", fmt->crop.top);
    printk(KERN_ERR "crop_left       = %d\n", fmt->crop.left);
    printk(KERN_ERR "crop_width      = %d\n", fmt->crop.width);
    printk(KERN_ERR "crop_height     = %d\n", fmt->crop.height);

    printk(KERN_ERR "frame_nums      = %d\n", fmt->frame_nums);
}

static inline void camera_dump_frame_info(struct frame_info *info)
{
    if (info == NULL) {
        printk(KERN_ERR "info is NULL");
        return ;
    }

    printk(KERN_ERR "========dump frame info =======\n");

    printk(KERN_ERR "index          = %u\n", info->index);
    printk(KERN_ERR "sequence       = %u\n", info->sequence);

    printk(KERN_ERR "width          = %u\n", info->width);
    printk(KERN_ERR "height         = %u\n", info->height);
    printk(KERN_ERR "pixfmt         = %c%c%c%c\n", (char)(info->pixfmt >> 0), (char)(info->pixfmt >> 8),
                                                (char)(info->pixfmt >> 16), (char)(info->pixfmt >> 24));
    printk(KERN_ERR "size           = %u\n", info->size);
    printk(KERN_ERR "vaddr          = %p\n", info->vaddr);
    printk(KERN_ERR "paddr          = 0x%lx\n", info->paddr);

    printk(KERN_ERR "timestamp      = %llu\n", info->timestamp);

    printk(KERN_ERR "isp_timestamp  = %u\n", info->isp_timestamp);
}

static inline int is_camera_frame_parameter_valid(struct mscaler_channel *data, struct frame_image_format *fmt)
{
    struct mscaler_data *drv = data->drv;
    struct sensor_attr *sensor = drv->camera.sensor;
    unsigned int output_width, output_height;
    int channel = data->channel;
    unsigned int mscaler_outpt_max_width = mscaler_output_res[channel][0];
    unsigned int mscaler_outpt_max_height = mscaler_output_res[channel][1];

    /* mscaler分辨率检查 */
    if (fmt->width < MSCALER_OUTPUT_MIN_WIDTH || fmt->height < MSCALER_OUTPUT_MIN_HEIGHT ||
        fmt->width > mscaler_outpt_max_width || fmt->height > mscaler_outpt_max_height) {
        printk(KERN_ERR "%s : width(%d) or height(%d) < 8 or > 4095\n", data->device_name, fmt->width, fmt->height);
        return -EINVAL;
    }
    if ( (sensor->dbus_type == SENSOR_DATA_BUS_MIPI) && (sensor->mipi.mipi_crop.enable) ) {
        output_width = sensor->mipi.mipi_crop.output_width;
        output_height = sensor->mipi.mipi_crop.output_height;
    } else {
        output_width = sensor->sensor_info.width;
        output_height = sensor->sensor_info.height;
    }

    /* note: scaler up need increase isp clk */
    if (fmt->scaler.enable) {
        if ((fmt->scaler.width > mscaler_outpt_max_width) ||
            (fmt->scaler.height > mscaler_outpt_max_height)) {
            printk(KERN_ERR "%s : scaler(%d X %d) out of range(%d X %d)\n", data->device_name,
                fmt->scaler.width, fmt->scaler.height,
                mscaler_outpt_max_width, mscaler_outpt_max_height);
            return -EINVAL;
        }

        output_width = fmt->scaler.width;
        output_height = fmt->scaler.height;
    }

    if (fmt->crop.enable) {
        /* compare with input */
        if (fmt->scaler.enable) {
            if (((fmt->crop.left + fmt->crop.width) > fmt->scaler.width) ||
                ((fmt->crop.top + fmt->crop.height) > fmt->scaler.height)) {
                printk(KERN_ERR "%s : crop(%d+%d X %d+%d) out of scaler range(%d X %d)\n", data->device_name,
                    fmt->crop.left, fmt->crop.width, fmt->crop.top, fmt->crop.height,
                    fmt->scaler.width, fmt->scaler.height);
                return -EINVAL;
            }
        } else {
            if (((fmt->crop.left + fmt->crop.width) > sensor->sensor_info.width) ||
                ((fmt->crop.top + fmt->crop.height) > sensor->sensor_info.height)) {
                printk(KERN_ERR "%s : crop(%d+%d X %d+%d) out of sensor output range(%d X %d)\n", data->device_name,
                    fmt->crop.left, fmt->crop.width, fmt->crop.top, fmt->crop.height,
                    sensor->sensor_info.width, sensor->sensor_info.height);
                return -EINVAL;
            }
        }

        output_width = fmt->crop.width;
        output_height = fmt->crop.height;
    }

    if (output_width != fmt->width || output_height != fmt->height) {
        printk(KERN_ERR "%s : mscaler resulation config error, check sensor resulotion and mscaler channel config\n", data->device_name);
        printk(KERN_ERR "%s : sensor ouput: %d X %d\n", data->device_name, sensor->sensor_info.width, sensor->sensor_info.height);
        camera_dump_frame_format(fmt);
        return -EINVAL;
    }

    return 0;
}

static int mscaler_request_buffer(struct mscaler_channel *data, unsigned long arg)
{
    struct frame_image_format *output_fmt = &data->output_fmt;
    int i, ret = 0;

    struct frame_image_format fmt;

    mutex_lock(&data->lock);

    ret = copy_from_user(&fmt, (void __user *)arg, sizeof(fmt));
    if(ret){
        printk(KERN_ERR "%s : Failed to copy from user\n", data->device_name);
        ret = -ENOMEM;
        goto unlock;
    }

    if (fmt.frame_nums < 2) {
        printk(KERN_ERR "%s : buffer count too small, fix to 2\n", data->device_name);
        fmt.frame_nums = 2;
    }

    if (is_camera_frame_parameter_valid(data, output_fmt) < 0) {
        printk(KERN_ERR "%s : frame parameter(%d) is invalid\n", data->device_name, fmt.pixel_format);
        ret = -EINVAL;
        goto unlock;
    }

    output_fmt->frame_nums = fmt.frame_nums;

    if (data->is_init) {
        for (i = 0; i < data->mem_cnt; i++) {
            unsigned int timeout_cnt = 100;
            struct frame_data *frm = &data->frames[i];

            if (frm->status != frame_status_user)
                continue;

            while(timeout_cnt--) {
                if (frm->status != frame_status_user)
                    break;

                mutex_unlock(&data->lock);
                usleep_range(1*1000, 1*1000);
                mutex_lock(&data->lock);
            }

            if (!timeout_cnt) {
                printk(KERN_ERR "%s : Failed to request buffer\n", data->device_name);
                ret = -EBUSY;
                goto unlock;
            }
        }

        mscaler_power_off_nolock(data);

        mscaler_free_mem(data);
    }

    //camera_dump_frame_format(output_fmt);

    ret = mscaler_alloc_mem(data);
    if (0 == ret)
        data->is_init = 1;

unlock:
    mutex_unlock(&data->lock);

    return ret;
}

static int mscaler_free_buffer(struct mscaler_channel *data)
{
    int ret;

    mutex_lock(&data->lock);

    if (data->is_init)
        ret = mscaler_free_mem(data);

    data->is_init = 0;

    mutex_unlock(&data->lock);

    return ret;
}

static int mscaler_get_frames_format(struct mscaler_channel *data, struct frame_image_format *fmt)
{
    struct frame_image_format fmt_ = data->output_fmt;
    int ret = 0;

    mutex_lock(&data->lock);

    ret = copy_to_user((void __user *)fmt, &fmt_, sizeof(*fmt));
    if (ret)
        printk(KERN_ERR "%s : camera can't get frames format\n", data->device_name);

    mutex_unlock(&data->lock);

    return ret;
}

static int mscaler_set_frames_format(struct mscaler_channel *data, unsigned long arg)
{
    struct frame_image_format *output_fmt = &data->output_fmt;
    int ret;

    struct frame_image_format fmt;

    mutex_lock(&data->lock);

    ret = copy_from_user(&fmt, (void __user *)arg, sizeof(struct frame_image_format));
    if(ret){
        printk(KERN_ERR "%s : Failed to copy from user\n", data->device_name);
        return -ENOMEM;
    }

    if (is_camera_frame_parameter_valid(data, &fmt) < 0) {
        printk(KERN_ERR "%s : frame parameter(%d) is invalid\n", data->device_name, fmt.pixel_format);
        return -EINVAL;
    }

    output_fmt->width = fmt.width;
    output_fmt->height = fmt.height;
    output_fmt->pixel_format = fmt.pixel_format;

    /* ingenic: 分辨率 */
    output_fmt->crop.enable    = fmt.crop.enable;
    output_fmt->crop.top       = fmt.crop.top;
    output_fmt->crop.left      = fmt.crop.left;
    output_fmt->crop.width     = fmt.crop.width;
    output_fmt->crop.height    = fmt.crop.height;

    output_fmt->scaler.enable = fmt.scaler.enable;
    output_fmt->scaler.width  = fmt.scaler.width;
    output_fmt->scaler.height = fmt.scaler.height;

    output_fmt->frame_nums        = fmt.frame_nums;

//    camera_dump_frame_format(output_fmt);

    mutex_unlock(&data->lock);

    return 0;
}

static void mscaler_skip_frames(struct mscaler_channel *data, unsigned int frames)
{
    unsigned long flags;

    spin_lock_irqsave(&data->spinlock, flags);

    while (frames--) {
        struct frame_data *frm = get_usable_frm(data);
        if (frm == NULL)
            break;
        add_to_free_list(data, frm);
    }

    spin_unlock_irqrestore(&data->spinlock, flags);
}

static unsigned int mscaler_get_available_frame_count(struct mscaler_channel *data)
{
    return data->frame_counter;
}

static int check_frame_mem(struct mscaler_channel *data, void *mem, void *base)
{
    unsigned int size = mem - base;

    if (mem < base)
        return -1;

    if (size % data->frm_size)
        return -1;

    if (size / data->frm_size >= data->mem_cnt)
        return -1;

    return 0;
}

static inline struct frame_data *mem_2_frame_data(struct mscaler_channel *data, void *mem)
{
    unsigned int size = mem - data->mem;
    unsigned int count = size / data->frm_size;
    struct frame_data *frm = &data->frames[count];

    assert(!(size % data->frm_size));
    assert(count < data->mem_cnt);

    return frm;
}

static int check_frame_info(struct mscaler_channel *data, struct frame_info *info)
{
    struct frame_data *frm = &data->frames[info->index];

    if (info->index >= data->mem_cnt)
        return -1;

    if (frm->info.paddr != info->paddr)
        return -1;

    return 0;
}

static inline struct frame_data *frame_info_2_frame_data(struct mscaler_channel *data, struct frame_info *info)
{
    if (!check_frame_info(data, info))
        return &data->frames[info->index];
    else {
        camera_dump_frame_info(info);
        return NULL;
    }
}

static void mscaler_put_frame(struct mscaler_channel *data, struct frame_data *frm)
{
    unsigned long flags;

    mutex_lock(&data->lock);

    spin_lock_irqsave(&data->spinlock, flags);

    if (frm->status != frame_status_user) {
        printk(KERN_ERR "%s : camera double free frame index %d\n", data->device_name, frm->info.index);
    } else {
        list_del(&frm->link);
        add_to_free_list(data, frm);
    }

    spin_unlock_irqrestore(&data->spinlock, flags);

    mutex_unlock(&data->lock);
}

static int mscaler_get_frame(struct mscaler_channel *data, struct list_head *list, struct frame_data **frame, unsigned int timeout_ms)
{
    struct frame_data *frm;
    unsigned long flags;
    int ret = -EAGAIN;

    mutex_lock(&data->lock);

    spin_lock_irqsave(&data->spinlock, flags);
    frm = get_usable_frm(data);
    spin_unlock_irqrestore(&data->spinlock, flags);
    if (!frm && timeout_ms) {
        ret = wait_event_interruptible_timeout(data->waiter, data->frame_counter, msecs_to_jiffies(timeout_ms));

        spin_lock_irqsave(&data->spinlock, flags);
        frm = get_usable_frm(data);
        spin_unlock_irqrestore(&data->spinlock, flags);
        if (ret <= 0 && !frm) {
            ret = -ETIMEDOUT;
            printk(KERN_ERR "%s : get frame time out\n", data->device_name);
        }
    }

    if (frm) {
        ret = 0;
        frm->status = frame_status_user;
        list_add_tail(&frm->link, list);
        *frame = frm;
    }

    mutex_unlock(&data->lock);

    return ret;
}

static int mscaler_stream_off(struct mscaler_channel *data)
{
    mutex_lock(&data->lock);

    mscaler_stream_off_nolock(data);

    mutex_unlock(&data->lock);

    return 0;
}

static int mscaler_stream_on(struct mscaler_channel *data)
{
    int ret = 0;

    mutex_lock(&data->lock);

    if (!data->is_power_on) {
        printk(KERN_ERR "%s : camera can't stream on when not power on\n", data->device_name);
        ret = -EINVAL;
        goto unlock;
    }

    ret = mscaler_stream_on_nolock(data);

unlock:
    mutex_unlock(&data->lock);

    return ret;
}

static int mscaler_power_on(struct mscaler_channel *data)
{
    int ret = 0;

    mutex_lock(&data->lock);

    ret = mscaler_power_on_nolock(data);

    mutex_unlock(&data->lock);

    return ret;
}

static void mscaler_power_off(struct mscaler_channel *data)
{
    mutex_lock(&data->lock);

    mscaler_power_off_nolock(data);

    mutex_unlock(&data->lock);
}

static int mscaler_get_info(struct mscaler_channel *data, struct camera_info *info)
{
    struct camera_info info_ = data->info;
    int ret = 0;

    mutex_lock(&data->lock);

    ret = copy_to_user((void __user *)info, &info_, sizeof(*info));
    if (ret)
        printk(KERN_ERR "%s : camera can't get camera infor\n", data->device_name);

    mutex_unlock(&data->lock);

    return ret;
}

static int mscaler_get_sensor_info(struct mscaler_channel *data, struct camera_info *info)
{
    struct camera_info info_ = data->drv->camera.sensor->info;

    int ret = copy_to_user((void __user *)info, &info_, sizeof(*info));
    if (ret)
        printk(KERN_ERR "%s : camera can't get sensor infor\n", data->device_name);

    return ret;
}

static int mscaler_get_max_scaler_size(struct mscaler_channel *data, unsigned int arg)
{
    int channel = data->channel;
    unsigned int scaler_size[2];

    scaler_size[0] = mscaler_output_res[channel][0];
    scaler_size[1] = mscaler_output_res[channel][1];

    int ret = copy_to_user((void __user *)arg, scaler_size, sizeof(scaler_size));
    if (ret)
        printk(KERN_ERR "%s : camera can't get max scaler size\n", data->device_name);

    return ret;
}

static int mscaler_get_line_align_size(struct mscaler_channel *data, unsigned int arg)
{
    int align_size = MSCALER_ALIGN_SIZE;

    int ret = copy_to_user((void __user *)arg, &align_size, sizeof(align_size));
    if (ret)
        printk(KERN_ERR "%s : camera can't get line align size\n", data->device_name);

    return ret;
}

static int mscaler_get_sensor_reg(struct mscaler_channel *data, struct sensor_dbg_register *reg)
{
    struct sensor_attr *sensor = data->drv->camera.sensor;
    struct sensor_dbg_register dbg_reg;
    int ret;

    if (copy_from_user(&dbg_reg, (void __user *)reg, sizeof(struct sensor_dbg_register))) {
        printk(KERN_ERR "%s copy from user error\n", __func__);
        return -EFAULT;
    }

    if (sensor->ops.get_register) {
        ret = sensor->ops.get_register(&dbg_reg);
        if (ret < 0) {
            printk(KERN_ERR "%s get_register fail\n", __func__);
            return ret;
        }
    } else {
        printk(KERN_ERR "sensor->ops.get_register is NULL!\n");
        return -EINVAL;
    }

    if (copy_to_user((void __user *)reg, &dbg_reg, sizeof(struct sensor_dbg_register))) {
        printk(KERN_ERR "%s copy from user error\n", __func__);
        return -EFAULT;
    }

    return 0;
}

static int mscaler_set_sensor_reg(struct mscaler_channel *data, struct sensor_dbg_register *reg)
{
    struct sensor_attr *sensor = data->drv->camera.sensor;
    struct sensor_dbg_register dbg_reg;
    int ret;

    if (copy_from_user(&dbg_reg, (void __user *)reg, sizeof(struct sensor_dbg_register))) {
        printk(KERN_ERR "%s copy from user error\n", __func__);
        return -EFAULT;
    }

    if (sensor->ops.set_register) {
        ret = sensor->ops.set_register(&dbg_reg);
        if (ret < 0) {
            printk(KERN_ERR "%s set_register fail\n", __func__);
            return ret;
        }
    } else {
        printk(KERN_ERR "sensor->ops.get_register is NULL!\n");
        return -EINVAL;
    }

    return 0;
}



#define ISP_CMD_MAGIC                   'C'
#define ISP_CMD_MAGCIC_NR               120

#define CMD_get_info                    _IOWR(ISP_CMD_MAGIC, ISP_CMD_MAGCIC_NR + 0, struct camera_info)
#define CMD_power_on                    _IO(ISP_CMD_MAGIC,   ISP_CMD_MAGCIC_NR + 1)
#define CMD_power_off                   _IO(ISP_CMD_MAGIC,   ISP_CMD_MAGCIC_NR + 2)
#define CMD_stream_on                   _IO(ISP_CMD_MAGIC,   ISP_CMD_MAGCIC_NR + 3)
#define CMD_stream_off                  _IO(ISP_CMD_MAGIC,   ISP_CMD_MAGCIC_NR + 4)
#define CMD_set_format                  _IOWR(ISP_CMD_MAGIC, ISP_CMD_MAGCIC_NR + 5, struct frame_image_format)
#define CMD_get_format                  _IO(ISP_CMD_MAGIC,   ISP_CMD_MAGCIC_NR + 6)
#define CMD_request_buffer              _IOWR(ISP_CMD_MAGIC, ISP_CMD_MAGCIC_NR + 7, struct frame_image_format)
#define CMD_free_buffer                 _IO(ISP_CMD_MAGIC,   ISP_CMD_MAGCIC_NR + 8)
#define CMD_get_max_scaler_size         _IOWR(ISP_CMD_MAGIC, ISP_CMD_MAGCIC_NR + 9, unsigned int)
#define CMD_get_line_align_size         _IOWR(ISP_CMD_MAGIC, ISP_CMD_MAGCIC_NR + 10, unsigned int)
#define CMD_set_scaler_algo             _IOWR(ISP_CMD_MAGIC, ISP_CMD_MAGCIC_NR + 11, struct mscaler_algo_attr)

#define CMD_wait_frame                  _IO(ISP_CMD_MAGIC,   ISP_CMD_MAGCIC_NR + 20)
#define CMD_get_frame                   _IO(ISP_CMD_MAGIC,   ISP_CMD_MAGCIC_NR + 21)
#define CMD_put_frame                   _IO(ISP_CMD_MAGIC,   ISP_CMD_MAGCIC_NR + 22)
#define CMD_dqbuf                       _IO(ISP_CMD_MAGIC,   ISP_CMD_MAGCIC_NR + 23)
#define CMD_dqbuf_wait                  _IO(ISP_CMD_MAGIC,   ISP_CMD_MAGCIC_NR + 24)
#define CMD_qbuf                        _IO(ISP_CMD_MAGIC,   ISP_CMD_MAGCIC_NR + 25)
#define CMD_skip_frames                 _IO(ISP_CMD_MAGIC,   ISP_CMD_MAGCIC_NR + 26)
#define CMD_get_frame_count             _IO(ISP_CMD_MAGIC,   ISP_CMD_MAGCIC_NR + 27)

#define CMD_get_sensor_info             _IOWR(ISP_CMD_MAGIC, ISP_CMD_MAGCIC_NR + 30, struct camera_info)
#define CMD_get_sensor_reg              _IOWR(ISP_CMD_MAGIC, ISP_CMD_MAGCIC_NR + 31, struct sensor_dbg_register)
#define CMD_set_sensor_reg              _IOWR(ISP_CMD_MAGIC, ISP_CMD_MAGCIC_NR + 32, struct sensor_dbg_register)



struct m_private_data {
    struct mscaler_channel *data;
    struct list_head list;
    void *map_mem;
};

static long mscaler_ioctl(struct file *filp, unsigned int cmd, unsigned long arg)
{
    struct m_private_data *pdata = filp->private_data;
    struct list_head *list = &pdata->list;
    void *map_mem = pdata->map_mem;
    struct frame_data *frm;
    unsigned int timeout_ms = 0;
    int ret;

    struct mscaler_channel *data = pdata->data;

    switch (cmd) {
    case CMD_get_info:{
        struct camera_info *info = (void *)arg;
        if (!info)
            return -EINVAL;
        return mscaler_get_info(data, info);
    }

    case CMD_get_sensor_info:{
        if (arg)
            return mscaler_get_sensor_info(data, (struct camera_info *)arg);
        else
            return -EINVAL;
    }

    case CMD_get_max_scaler_size:{
        if (arg)
            return mscaler_get_max_scaler_size(data, arg);
        else
            return -EINVAL;
    }

    case CMD_set_scaler_algo:{
        struct mscaler_algo_attr attr;
        ret = copy_from_user(&attr, (void __user *)arg, sizeof(struct mscaler_algo_attr));
        if(ret){
            printk(KERN_ERR "%s: CMD_set_scaler_algo copy_from_user fail\n", data->device_name);
            return ret;
        }
        return mscaler_set_scale_algo(data, &attr);
    }

    case CMD_get_line_align_size:{
        if (arg)
            return mscaler_get_line_align_size(data, arg);
        else
            return -EINVAL;
    }

    case CMD_request_buffer:
        return mscaler_request_buffer(data, arg);

    case CMD_free_buffer:
        return mscaler_free_buffer(data);

    case CMD_power_on:
        return mscaler_power_on(data);

    case CMD_power_off:
        mscaler_power_off(data);
        return 0;

    case CMD_stream_on:
        return mscaler_stream_on(data);

    case CMD_stream_off:
        mscaler_stream_off(data);
        return 0;

    case CMD_wait_frame:
        timeout_ms = 3000;
    case CMD_get_frame:{
        if (!map_mem) {
            printk(KERN_ERR "%s : please mmap first\n", data->device_name);
            return -EINVAL;
        }

        void **mem_p = (void *)arg;
        if (!mem_p)
            return -EINVAL;

        ret = mscaler_get_frame(data, list, &frm, timeout_ms);
        if (0 == ret) {
            frm->info.vaddr = map_mem + (frm->addr - data->mem);
            m_cache_sync(frm->addr, frm->info.size);
        } else
            return ret;

        *mem_p = frm->info.vaddr;
        data->chn_output_cnt++;
        return 0;
    }

    case CMD_put_frame: {
        void *mem = (void *)arg;
        if (check_frame_mem(data, mem, map_mem))
            return -EINVAL;

        mem = data->mem + (mem - map_mem);
        frm = mem_2_frame_data(data, mem);
        m_cache_sync(frm->addr, frm->info.size);
        mscaler_put_frame(data, frm);
        return 0;
    }

    case CMD_dqbuf_wait:
        timeout_ms = 3000;
    case CMD_dqbuf: {
        if (!map_mem) {
            printk(KERN_ERR "%s : please mmap first\n", data->device_name);
            return -EINVAL;
        }

        ret = mscaler_get_frame(data, list, &frm, timeout_ms);
        if (0 == ret) {
            frm->info.vaddr = map_mem + (frm->addr - data->mem);

            /* vic_ts_cnt to us */
            unsigned int vic_ts = *((unsigned int *)(frm->addr + frm->info.size) + 1);
            unsigned long long vic_ts_us = (unsigned long long)vic_ts * USEC_PER_SEC;
            do_div(vic_ts_us, data->vic_ts_cnt_1s);
            frm->info.isp_timestamp = (unsigned int)vic_ts_us;

            m_cache_sync(frm->addr, frm->info.size);
        } else
            return ret;

        ret = copy_to_user((void __user *)arg, &frm->info, sizeof(struct frame_info));
        if (ret) {
            printk(KERN_ERR "%s: CMD_dqbuf copy_to_user fail\n", data->device_name);
            return ret;
        }

        data->chn_output_cnt++;
        return 0;
    }

    case CMD_qbuf: {
        struct frame_info info;
        ret = copy_from_user(&info, (void __user *)arg, sizeof(struct frame_info));
        if(ret){
            printk(KERN_ERR "%s: CMD_qbuf copy_from_user fail\n", data->device_name);
            return ret;
        }

        frm = frame_info_2_frame_data(data, &info);
        if (!frm) {
            printk(KERN_ERR "%s: CMD_qbuf err frame info\n", data->device_name);
            return -EINVAL;
        }

        m_cache_sync(frm->addr, frm->info.size);
        mscaler_put_frame(data, frm);
        return 0;
    }

    case CMD_get_frame_count: {
        unsigned int *count_p = (void *)arg;
        if (!count_p)
            return -EINVAL;

        *count_p = mscaler_get_available_frame_count(data);
        return 0;
    }

    case CMD_skip_frames: {
        unsigned int frames = arg;
        mscaler_skip_frames(data, frames);
        return 0;
    }

    case CMD_set_format:
        return mscaler_set_frames_format(data, arg);

    case CMD_get_format: {
        struct frame_image_format *fmt = (void *)arg;
        if (!fmt)
            return -EINVAL;

        return mscaler_get_frames_format(data, fmt);
    }

    case CMD_get_sensor_reg:{
         if (arg)
            return mscaler_get_sensor_reg(data, (struct sensor_dbg_register *)arg);
        else
            return -EINVAL;
    }
    case CMD_set_sensor_reg:{
         if (arg)
            return mscaler_set_sensor_reg(data, (struct sensor_dbg_register *)arg);
        else
            return -EINVAL;
    }

    default:
        printk(KERN_ERR "%s : %x not support this cmd\n", data->device_name, cmd);
        return -EINVAL;
    }

    return 0;
}

static int mscaler_mmap(struct file *file, struct vm_area_struct *vma)
{
    unsigned long len;
    unsigned long start;
    unsigned long off;
    struct m_private_data *pdata = file->private_data;

    struct mscaler_channel *data = pdata->data;

    off = vma->vm_pgoff << PAGE_SHIFT;
    if (off) {
        printk(KERN_ERR "%s : camera offset must be 0\n", data->device_name);
        return -EINVAL;
    }

    len = data->frm_size * data->mem_cnt;

    if ((vma->vm_end - vma->vm_start) != len) {
        printk(KERN_ERR "%s : camera size must be equal total size\n", data->device_name);
        return -EINVAL;
    }

    /* frame buffer memory */
    start = virt_to_phys(data->mem);
    off += start;

    vma->vm_pgoff = off >> PAGE_SHIFT;
    vma->vm_flags |= VM_IO;

    /*
     * 0: cachable,write through (cache + cacheline对齐写穿)
     * 1: uncachable,write Acceleration (uncache + 硬件写加速)
     * 2: uncachable
     * 3: cachable
     */
    pgprot_val(vma->vm_page_prot) &= ~_CACHE_MASK;
    pgprot_val(vma->vm_page_prot) |= _CACHE_CACHABLE_NO_WA;

    if (io_remap_pfn_range(vma, vma->vm_start, off >> PAGE_SHIFT,
                           vma->vm_end - vma->vm_start, vma->vm_page_prot)) {
        return -EAGAIN;
    }

    pdata->map_mem = (void *)vma->vm_start;

    return 0;
}

static int mscaler_open(struct inode *inode, struct file *filp)
{
    struct m_private_data *pdata = kmalloc(sizeof(*pdata), GFP_KERNEL);
    struct mscaler_channel *data = container_of(filp->private_data,
            struct mscaler_channel, mscaler_mdev);

    mutex_lock(&data->lock);

    data->chn_done_cnt = 0;
    data->chn_output_cnt = 0;

    INIT_LIST_HEAD(&pdata->list);

    pdata->data = data;

    filp->private_data = pdata;

    mutex_unlock(&data->lock);

    return 0;
}

static int mscaler_release(struct inode *inode, struct file *filp)
{
    unsigned long flags;
    struct list_head *pos, *n;
    struct m_private_data *pdata = filp->private_data;

    struct list_head *list = &pdata->list;

    struct mscaler_channel *data = pdata->data;

    mutex_lock(&data->lock);

    spin_lock_irqsave(&data->spinlock, flags);

    if (data->is_init) {
        list_for_each_safe(pos, n, list) {
            list_del(pos);
            struct frame_data *frm = list_entry(pos, struct frame_data, link);
            add_to_free_list(data, frm);
        }
    }

    spin_unlock_irqrestore(&data->spinlock, flags);

    kfree(pdata);

    mutex_unlock(&data->lock);

    return 0;
}


static struct file_operations mscaler_misc_fops = {
    .open           = mscaler_open,
    .release        = mscaler_release,
    .mmap           = mscaler_mmap,
    .unlocked_ioctl = mscaler_ioctl,
};

#ifdef SOC_CAMERA_DEBUG
static int dsysfs_mscaler_dump_reg(struct device *dev, struct device_attribute *attr, char *buf)
{
    struct mscaler_data *drv = container_of((struct kobject *)dev, struct mscaler_data, dsysfs_kobj);
    char *p = buf;

    p += sprintf(p, "MSCA_CTRL           :0x%08x\n", mscaler_read_reg(drv->index, MSCA_CTRL));
    p += sprintf(p, "MSCA_CH_EN          :0x%08x\n", mscaler_read_reg(drv->index, MSCA_CH_EN));
    p += sprintf(p, "MSCA_CH_STA         :0x%08x\n", mscaler_read_reg(drv->index, MSCA_CH_STA));
    p += sprintf(p, "MSCA_DMAOUT_ARB     :0x%08x\n", mscaler_read_reg(drv->index, MSCA_DMAOUT_ARB));
    p += sprintf(p, "MSCA_CLK_GATE_EN    :0x%08x\n", mscaler_read_reg(drv->index, MSCA_CLK_GATE_EN));
    p += sprintf(p, "MSCA_CLK_DIS        :0x%08x\n", mscaler_read_reg(drv->index, MSCA_CLK_DIS));
    p += sprintf(p, "MSCA_SRC_IN         :0x%08x\n", mscaler_read_reg(drv->index, MSCA_SRC_IN));
    p += sprintf(p, "MSCA_GLO_RSZ_COEF_WR:0x%08x\n", mscaler_read_reg(drv->index, MSCA_GLO_RSZ_COEF_WR));
    p += sprintf(p, "MSCA_SYS_PRO_CLK_EN :0x%08x\n", mscaler_read_reg(drv->index, MSCA_SYS_PRO_CLK_EN));
    p += sprintf(p, "MSCA_DS0_CLK_NUM    :0x%08x\n", mscaler_read_reg(drv->index, MSCA_DS0_CLK_NUM));
    p += sprintf(p, "MSCA_DS1_CLK_NUM    :0x%08x\n", mscaler_read_reg(drv->index, MSCA_DS1_CLK_NUM));
    p += sprintf(p, "MSCA_DS2_CLK_NUM    :0x%08x\n", mscaler_read_reg(drv->index, MSCA_DS2_CLK_NUM));

    int chn = 0;
    for (chn = 0; chn < MSCALER_MAX_CH; chn++) {
        p += sprintf(p, "chn%d:\n", chn);
        p += sprintf(p, "RSZ_OSIZE            :0x%08x\n", mscaler_read_reg(drv->index, RSZ_OSIZE(chn)));
        p += sprintf(p, "RSZ_STEP             :0x%08x\n", mscaler_read_reg(drv->index, RSZ_STEP(chn)));
        p += sprintf(p, "CROP_OPOS            :0x%08x\n", mscaler_read_reg(drv->index, CROP_OPOS(chn)));
        p += sprintf(p, "CROP_OSIZE           :0x%08x\n", mscaler_read_reg(drv->index, CROP_OSIZE(chn)));
        p += sprintf(p, "FRA_CTRL_LOOP        :0x%08x\n", mscaler_read_reg(drv->index, FRA_CTRL_LOOP(chn)));
        p += sprintf(p, "FRA_CTRL_MASK        :0x%08x\n", mscaler_read_reg(drv->index, FRA_CTRL_MASK(chn)));
        p += sprintf(p, "MS0_POS              :0x%08x\n", mscaler_read_reg(drv->index, MS0_POS(chn)));
        p += sprintf(p, "MS0_SIZE             :0x%08x\n", mscaler_read_reg(drv->index, MS0_SIZE(chn)));
        p += sprintf(p, "MS0_VALUE            :0x%08x\n", mscaler_read_reg(drv->index, MS0_VALUE(chn)));
        p += sprintf(p, "MS1_POS              :0x%08x\n", mscaler_read_reg(drv->index, MS1_POS(chn)));
        p += sprintf(p, "MS1_SIZE             :0x%08x\n", mscaler_read_reg(drv->index, MS1_SIZE(chn)));
        p += sprintf(p, "MS1_VALUE            :0x%08x\n", mscaler_read_reg(drv->index, MS1_VALUE(chn)));
        p += sprintf(p, "MS2_POS              :0x%08x\n", mscaler_read_reg(drv->index, MS2_POS(chn)));
        p += sprintf(p, "MS2_SIZE             :0x%08x\n", mscaler_read_reg(drv->index, MS2_SIZE(chn)));
        p += sprintf(p, "MS2_VALUE            :0x%08x\n", mscaler_read_reg(drv->index, MS2_VALUE(chn)));
        p += sprintf(p, "MS3_POS              :0x%08x\n", mscaler_read_reg(drv->index, MS3_POS(chn)));
        p += sprintf(p, "MS3_SIZE             :0x%08x\n", mscaler_read_reg(drv->index, MS3_SIZE(chn)));
        p += sprintf(p, "MS3_VALUE            :0x%08x\n", mscaler_read_reg(drv->index, MS3_VALUE(chn)));
        p += sprintf(p, "OUT_FMT              :0x%08x\n", mscaler_read_reg(drv->index, OUT_FMT(chn)));
        p += sprintf(p, "DMAOUT_Y_ADDR        :0x%08x\n", mscaler_read_reg(drv->index, DMAOUT_Y_ADDR(chn)));
        p += sprintf(p, "Y_ADDR_FIFO_STA      :0x%08x\n", mscaler_read_reg(drv->index, Y_ADDR_FIFO_STA(chn)));
        p += sprintf(p, "Y_LAST_ADDR_FIFO_STA :0x%08x\n", mscaler_read_reg(drv->index, Y_LAST_ADDR_FIFO_STA(chn)));
        p += sprintf(p, "DMAOUT_Y_STRI        :0x%08x\n", mscaler_read_reg(drv->index, DMAOUT_Y_STRI(chn)));
        p += sprintf(p, "DMAOUT_UV_ADDR       :0x%08x\n", mscaler_read_reg(drv->index, DMAOUT_UV_ADDR(chn)));
        p += sprintf(p, "UV_ADDR_FIFO_STA     :0x%08x\n", mscaler_read_reg(drv->index, UV_ADDR_FIFO_STA(chn)));
        p += sprintf(p, "UV_LAST_ADDR_FIFO_STA:0x%08x\n", mscaler_read_reg(drv->index, UV_LAST_ADDR_FIFO_STA(chn)));
        p += sprintf(p, "DMAOUT_UV_STRI       :0x%08x\n", mscaler_read_reg(drv->index, DMAOUT_UV_STRI(chn)));
        p += sprintf(p, "DMAOUT_Y_ADDR_CLR    :0x%08x\n", mscaler_read_reg(drv->index, DMAOUT_Y_ADDR_CLR(chn)));
        p += sprintf(p, "DMAOUT_UV_ADDR_CLR   :0x%08x\n", mscaler_read_reg(drv->index, DMAOUT_UV_ADDR_CLR(chn)));
        p += sprintf(p, "DMAOUT_Y_LAST_ADDR_CLR:0x%08x\n", mscaler_read_reg(drv->index, DMAOUT_Y_LAST_ADDR_CLR(chn)));
        p += sprintf(p, "DMAOUT_UV_LAST_ADDR_CLR:0x%08x\n", mscaler_read_reg(drv->index, DMAOUT_UV_LAST_ADDR_CLR(chn)));
        p += sprintf(p, "DMAOUT_Y_ADDR_SEL    :0x%08x\n", mscaler_read_reg(drv->index, DMAOUT_Y_ADDR_SEL(chn)));
        p += sprintf(p, "DMAOUT_UV_ADDR_SEL   :0x%08x\n", mscaler_read_reg(drv->index, DMAOUT_UV_ADDR_SEL(chn)));
        p += sprintf(p, "COE_ZERO_VRSZ_H      :0x%08x\n", mscaler_read_reg(drv->index, COE_ZERO_VRSZ_H(chn)));
        p += sprintf(p, "COE_ZERO_VRSZ_L      :0x%08x\n", mscaler_read_reg(drv->index, COE_ZERO_VRSZ_L(chn)));
        p += sprintf(p, "COE_ZERO_HRSZ_H      :0x%08x\n", mscaler_read_reg(drv->index, COE_ZERO_HRSZ_H(chn)));
        p += sprintf(p, "COE_ZERO_HRSZ_L      :0x%08x\n", mscaler_read_reg(drv->index, COE_ZERO_HRSZ_L(chn)));
    }

    return p - buf;
}


static int dsysfs_mscaler_show_info(struct device *dev, struct device_attribute *attr, char *buf)
{
    struct mscaler_data *drv = container_of((struct kobject *)dev, struct mscaler_data, dsysfs_kobj);
    struct mscaler_channel *mscaler_ch;
    char *p = buf;

    int chn;
    for(chn = 0; chn < MSCALER_MAX_CH; chn++) {
        p += sprintf(p ,"############## mscaler chn%d ###############\n", chn);
        mscaler_ch = &drv->mscaler_ch[chn];
        p += sprintf(p ,"chan status: %s\n", mscaler_ch->is_stream_on ? "stream on" : "stream off");
        if(!mscaler_ch->is_stream_on)
            continue;

        p += sprintf(p ,"output resolution: %d * %d\n", mscaler_ch->output_fmt.width, mscaler_ch->output_fmt.height);
        p += sprintf(p ,"output pixformat: 0x%x\n", mscaler_ch->output_fmt.pixel_format);
        p += sprintf(p ,"scaler: %s\n", mscaler_ch->output_fmt.scaler.enable ? "enable" : "disable");
        if(mscaler_ch->output_fmt.scaler.enable){
            p += sprintf(p ,"\t scaler width: %d\n", mscaler_ch->output_fmt.scaler.width);
            p += sprintf(p ,"\t scaler height: %d\n", mscaler_ch->output_fmt.scaler.height);
        }
        p += sprintf(p ,"crop: %s\n", mscaler_ch->output_fmt.crop.enable ? "enable" : "disable");
        if(mscaler_ch->output_fmt.crop.enable){
            p += sprintf(p ,"\t crop top: %d\n", mscaler_ch->output_fmt.crop.top);
            p += sprintf(p ,"\t crop left: %d\n", mscaler_ch->output_fmt.crop.left);
            p += sprintf(p ,"\t crop width: %d\n", mscaler_ch->output_fmt.crop.width);
            p += sprintf(p ,"\t crop height: %d\n", mscaler_ch->output_fmt.crop.height);
        }

        p += sprintf(p ,"buf cnt: %d(%d+1)\n", mscaler_ch->mem_cnt, mscaler_ch->output_fmt.frame_nums);
        p += sprintf(p ,"buf size: %d\n", mscaler_ch->output_fmt.frame_size);
        p += sprintf(p ,"buf align size: %d\n", mscaler_ch->frm_size);

        p += sprintf(p ,"chn done cnt: %u\n", mscaler_ch->chn_done_cnt);
        p += sprintf(p ,"chn output cnt: %u\n", mscaler_ch->chn_output_cnt);
        p += sprintf(p ,"chn frm rate ctrl: loop(%d) mask(0x%x)\n",
            (mscaler_read_reg(mscaler_ch->index, FRA_CTRL_LOOP(mscaler_ch->channel))&0x1f) + 1,
            mscaler_read_reg(mscaler_ch->index, FRA_CTRL_MASK(mscaler_ch->channel)));
    }

    return p - buf;
}


static DSYSFS_DEV_ATTR(dump_mscaler_reg, S_IRUGO|S_IWUSR, dsysfs_mscaler_dump_reg, NULL);
static DSYSFS_DEV_ATTR(show_mscaler_info, S_IRUGO|S_IWUSR, dsysfs_mscaler_show_info, NULL);
static struct attribute *dsysfs_mscaler_dev_attrs[] = {
    &dsysfs_dev_attr_dump_mscaler_reg.attr,
    &dsysfs_dev_attr_show_mscaler_info.attr,
    NULL,
};

static const struct attribute_group dsysfs_mscaler_attr_group = {
    .attrs  = dsysfs_mscaler_dev_attrs,
};
#endif

#define error_if(_cond)                                                 \
    do {                                                                \
        if (_cond) {                                                    \
            printk(KERN_ERR "mscaler: failed to check: %s\n", #_cond);  \
            ret = -1;                                                   \
            goto unlock;                                                \
        }                                                               \
    } while (0)

int mscaler_register_sensor(int index, struct sensor_attr *sensor)
{
    struct mscaler_data *drv = &jz_mscaler_dev[index];
    int ret = 0;

    assert(drv->is_finish > 0);
    assert(!drv->camera.sensor);

    mutex_lock(&drv->lock);

    error_if (drv->camera.sensor);
    error_if(sensor->sensor_info.width < 8 || sensor->sensor_info.width > 2048);
    error_if(sensor->sensor_info.height < 8 || sensor->sensor_info.height > 2048);

    /* 由应用程序申请buffer */

    /* 申请mscaler的多个通道设备节点 */
    int channel;
    for(channel = 0; channel < MSCALER_MAX_CH; channel++) {
        struct miscdevice *mscaler_mdev = &drv->mscaler_ch[channel].mscaler_mdev;
        ret = misc_register(mscaler_mdev);
        assert(!ret);
    }

    drv->camera.sensor = sensor;
    drv->camera.is_power_on = 0;
    drv->camera.is_stream_on = 0;

unlock:
    mutex_unlock(&drv->lock);

    return ret;
}

void mscaler_unregister_sensor(int index, struct sensor_attr *sensor)
{
    struct mscaler_data *drv = &jz_mscaler_dev[index];

    assert(drv->is_finish > 0);
    assert(drv->camera.sensor);
    assert(sensor == drv->camera.sensor);

    mutex_lock(&drv->lock);
    int channel;

    if (drv->camera.is_stream_on)
        panic("mscaler%d : failed to unregister, when camera stream on!\n", index);

    if (drv->camera.is_power_on) {
        isp_power_off(drv->index);
        drv->camera.is_power_on = 0;
    }

    drv->camera.sensor = NULL;

    /* 释放mscaler 多个通道节点 */
    for(channel = 0; channel < MSCALER_MAX_CH; channel++) {
        struct mscaler_channel *data = &drv->mscaler_ch[channel];
        struct miscdevice *mscaler_mdev = &data->mscaler_mdev;

        misc_deregister(mscaler_mdev);

        mscaler_free_mem(data);
    }

    mutex_unlock(&drv->lock);
}

int jz_mscaler_drv_init(int index)
{
    struct mscaler_data *drv = &jz_mscaler_dev[index];

    mutex_init(&drv->lock);

    int ch;
    for(ch = 0; ch < MSCALER_MAX_CH; ch++) {
        struct miscdevice *mscaler_mdev = &drv->mscaler_ch[ch].mscaler_mdev;
        drv->mscaler_ch[ch].index = index;
        drv->mscaler_ch[ch].channel = ch;
        drv->mscaler_ch[ch].drv = drv;

        mutex_init(&drv->mscaler_ch[ch].lock);

        spin_lock_init(&drv->mscaler_ch[ch].spinlock);

        init_waitqueue_head(&drv->mscaler_ch[ch].waiter);

        memset(drv->mscaler_ch[ch].device_name, 0x00, sizeof(drv->mscaler_ch[ch].device_name));
        sprintf(drv->mscaler_ch[ch].device_name, "%s-ch%d", drv->device_name, ch);

        mscaler_mdev->minor = MISC_DYNAMIC_MINOR;
        mscaler_mdev->name  = drv->mscaler_ch[ch].device_name;
        mscaler_mdev->fops  = &mscaler_misc_fops;
    }

#ifdef SOC_CAMERA_DEBUG
    int ret = dsysfs_create_group(&drv->dsysfs_kobj, dsysfs_get_root_dir(index), "mscaler", &dsysfs_mscaler_attr_group);
    if (ret) {
        printk(KERN_ERR "isp%d dsysfs create sub dir mscaler fail\n", index);
        goto error_dsys_create_mscaler;
    }
#endif

    drv->is_finish = 1;

    printk(KERN_DEBUG "mscaler%d initialization successfully\n", index);

    return 0;

#ifdef SOC_CAMERA_DEBUG
error_dsys_create_mscaler:
    return ret;
#endif
}

void jz_mscaler_drv_deinit(int index)
{
    struct mscaler_data *drv = &jz_mscaler_dev[index];

    if (!drv->is_finish)
        return ;

    drv->is_finish = 0;
    /* Nothing TODO */

#ifdef SOC_CAMERA_DEBUG
    dsysfs_remove_group(&drv->dsysfs_kobj, &dsysfs_mscaler_attr_group);
#endif

    return ;
}
