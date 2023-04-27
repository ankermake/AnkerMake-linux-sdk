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
#include "vic_regs.h"
#include "csi.h"
#include "dsys.h"
#include "vic.h"
#include "drivers/rmem_manager/rmem_manager.h"

#define VIC_ALIGN_SIZE                  8

struct jz_vic_mem_data {
    int index;
    int is_enable;
    int is_finish;

    int irq;
    const char *irq_name;

    wait_queue_head_t waiter;

    void *mem;
    unsigned int mem_cnt;
    unsigned int frm_size;
    unsigned int uv_data_offset;

    volatile unsigned int frame_counter;
    volatile unsigned int wait_timeout;
    unsigned int dma_index;

    /* Camera Device */
    char *device_name;              /* 设备节点名字 */
    unsigned int cam_mem_cnt;       /* 循环buff个数 */
    struct miscdevice vic_mdev;
    struct camera_device camera;
    struct list_head free_list;
    struct list_head usable_list;

    struct frame_data *t[2];
    struct frame_data *frames;

    struct mutex lock;
    struct mutex sensor_mlock;
    struct spinlock spinlock;

    unsigned int vic_frd_c;     /* frame done cnt, no reset */
    unsigned int vic_fre_c;     /* frame err cnt */
    unsigned int vic_frov_c;    /* frame overflow cnt */

    unsigned int vic_frm_done;  /* frame done cnt */
    unsigned int vic_frm_output;/* frame output(dqbuf) */
#ifdef SOC_CAMERA_DEBUG
    struct kobject dsysfs_parent_kobj;
    struct kobject dsysfs_kobj;
#endif
};

static struct jz_vic_mem_data jz_vic_mem_dev[2] = {
    {
        .index                  = 0,
        .is_enable              = 0,
        .irq                    = IRQ_INTC_BASE + IRQ_VIC0, /* BASE + 19 */
        .irq_name               = "VIC0",
        .cam_mem_cnt            = 2,
        .device_name            = "vic0",
    },

    {
        .index                  = 1,
        .is_enable              = 0,
        .irq                    = IRQ_INTC_BASE + IRQ_VIC1, /* BASE + 18 */
        .irq_name               = "VIC1",
        .cam_mem_cnt            = 2,
        .device_name            = "vic1",
    },
};

static int reset_user_frame_when_stream_on = 0;
module_param_named(reset_user_frame_when_stream_on, reset_user_frame_when_stream_on, int, 0644);

static unsigned int vic_dma_addr[][2] = {
    {VIC_DMA_Y_CH_BUF0_ADDR, VIC_DMA_UV_CH_BUF0_ADDR},
    {VIC_DMA_Y_CH_BUF1_ADDR, VIC_DMA_UV_CH_BUF1_ADDR},
    {VIC_DMA_Y_CH_BUF2_ADDR, VIC_DMA_UV_CH_BUF2_ADDR},
    {VIC_DMA_Y_CH_BUF3_ADDR, VIC_DMA_UV_CH_BUF3_ADDR},
    {VIC_DMA_Y_CH_BUF4_ADDR, VIC_DMA_UV_CH_BUF4_ADDR},
};


static void vic_set_dma_addr(int index, unsigned long y_addr, unsigned long uv_addr, int frame_index)
{
    vic_write_reg(index, vic_dma_addr[frame_index][0], y_addr);
    vic_write_reg(index, vic_dma_addr[frame_index][1], uv_addr);
}

static inline void vic_get_dma_addr(unsigned int index, unsigned long *y_addr, unsigned long *uv_addr, int frame_index)
{
    *y_addr = vic_read_reg(index, vic_dma_addr[frame_index][0]);
    *uv_addr = vic_read_reg(index, vic_dma_addr[frame_index][1]);
}

static void set_dma_addr(int index, struct frame_data *frm, int frame_index)
{
    struct jz_vic_mem_data *drv = &jz_vic_mem_dev[index];

    frm->status = frame_status_trans;
    unsigned long address = frm->info.paddr;
    vic_set_dma_addr(index, address, address + drv->uv_data_offset, frame_index);
}

static void add_to_free_list(int index, struct frame_data *frm)
{
    struct jz_vic_mem_data *drv = &jz_vic_mem_dev[index];

    frm->status = frame_status_free;
    list_add_tail(&frm->link, &drv->free_list);
}

static struct frame_data *get_free_frm(int index)
{
    struct jz_vic_mem_data *drv = &jz_vic_mem_dev[index];

    if (list_empty(&drv->free_list))
        return NULL;

    struct frame_data *frm = list_first_entry(&drv->free_list, struct frame_data, link);
    list_del_init(&frm->link);

    return frm;
}

static void add_to_usable_list(int index, struct frame_data *frm)
{
    struct jz_vic_mem_data *drv = &jz_vic_mem_dev[index];

    frm->status = frame_status_usable;
    drv->frame_counter++;
    list_add_tail(&frm->link, &drv->usable_list);
}

static struct frame_data *get_usable_frm(int index)
{
    struct jz_vic_mem_data *drv = &jz_vic_mem_dev[index];

    if (list_empty(&drv->usable_list))
        return NULL;

    drv->frame_counter--;

    struct frame_data *frm = list_first_entry(&drv->usable_list, struct frame_data, link);
    list_del_init(&frm->link);

    return frm;
}

static void init_frm_lists(int index)
{
    struct jz_vic_mem_data *drv = &jz_vic_mem_dev[index];

    INIT_LIST_HEAD(&drv->free_list);
    INIT_LIST_HEAD(&drv->usable_list);
    drv->dma_index = 0;
    drv->frame_counter = 0;
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
        add_to_free_list(index, frm);
    }
}

static void reset_frm_lists(int index)
{
    struct jz_vic_mem_data *drv = &jz_vic_mem_dev[index];
    int i;
    for (i = 0; i < drv->mem_cnt; i++) {
        struct frame_data *frm = &drv->frames[i];

        /*
        * reset_user_frame_when_stream_on
        * 该参数用于选择是否判断在reset_frm_lists时:
        * 强制将复位所有帧或者选择未被应用使用的帧才复位
        * 0 :未被应用使用的帧才复位
        * 1 :强制将复位所有帧
        * 默认为0
        */
        if (!reset_user_frame_when_stream_on)
            if (frm->status == frame_status_user)
                continue;

        if (drv->t[0] != frm && drv->t[1] != frm)
            list_del_init(&frm->link);
        add_to_free_list(index, frm);
    }

    drv->dma_index = 0;
    drv->frame_counter = 0;
    drv->wait_timeout = 0;
}

static void init_dma_addr(int index)
{
    struct jz_vic_mem_data *drv = &jz_vic_mem_dev[index];
    struct frame_data *frm;

    frm = get_free_frm(index);
    drv->t[0] = frm;
    set_dma_addr(index, frm, 0);

    frm = get_free_frm(index);
    if (!frm)
        frm = drv->t[0];

    drv->t[1] = frm;
    set_dma_addr(index, frm, 1);
}

static irqreturn_t vic_irq_dma_handler(int irq, void *data)
{
    struct jz_vic_mem_data *drv = (struct jz_vic_mem_data *)data;
    int index = drv->index;
    volatile unsigned long state, pending, mask;

    mask = vic_read_reg(index, VIC_INT_MASK);
    state = vic_read_reg(index, VIC_INT_STA);
    pending = state & (~mask);
    vic_write_reg(index, VIC_INT_CLR, pending);

    if (get_bit_field(&pending, DMA_FRD)) {
        struct frame_data *frm, *usable_frm = NULL;
        int frame_index = drv->dma_index;
        drv->dma_index = !frame_index;

        drv->vic_frm_done++;

        /*
         * 当传输列表只有一帧的时候，不能用这一帧
         */
        if (drv->t[0] != drv->t[1])
            usable_frm = drv->t[frame_index];

        /*
         * 1 优先从空闲列表中获取新的帧加入传输
         * 2 如果空闲列表没有帧，那么传输完成列表中获取
         * 3 如果完成列表也没有，那么用下一帧做保底
         */
        frm = get_free_frm(index);
        if (!frm)
            frm = get_usable_frm(index);
        if (!frm)
            frm = drv->t[!frame_index];
        drv->t[frame_index] = frm;
        set_dma_addr(index, frm, frame_index);

        if (usable_frm) {
            usable_frm->info.sequence = drv->vic_frm_done;
            usable_frm->info.timestamp = get_time_us();

            add_to_usable_list(index, usable_frm);
            wake_up_all(&drv->waiter);
        }
    }

    if (get_bit_field(&pending, VIC_HVRES_ERR)) {
        /* 复位vic后，下帧重头开始取（防裂屏），dma地址会从第一个地址开始往后写 */
        vic_reset(index);
        if (drv->t[drv->dma_index] != drv->t[!drv->dma_index]) {
            add_to_free_list(index, drv->t[drv->dma_index]);
            add_to_free_list(index, drv->t[!drv->dma_index]);
        } else {
            add_to_free_list(index, drv->t[drv->dma_index]);
        }
        drv->dma_index = 0;
        init_dma_addr(index);

        drv->vic_fre_c++;
        printk(KERN_ERR "## VIC WARN status = 0x%08lx\n", pending);
    }
    if (get_bit_field(&pending, VIC_FIFO_OVF)) {
        drv->vic_frov_c++;
    }

    if (get_bit_field(&pending, VIC_FRM_START)) {
        if(drv->camera.sensor->ops.frame_start_callback) {
            if(drv->camera.sensor->ops.frame_start_callback() >= 1) {
                struct frame_data *current_frm = drv->t[drv->dma_index];
                current_frm->info.shutter_count = drv->camera.sensor->ops.frame_start_callback();
            }
        }
    }

    if (get_bit_field(&pending, VIC_FRD)) {
        drv->vic_frd_c++;
    }

    return IRQ_HANDLED;
}

static inline void m_cache_sync(void *mem, int size)
{
    dma_cache_sync(NULL, mem, size, DMA_FROM_DEVICE);
}

static int vic_alloc_mem(int index, struct sensor_attr *attr)
{
    struct jz_vic_mem_data *drv = &jz_vic_mem_dev[index];
    int mem_cnt = drv->cam_mem_cnt + 1;
    int frm_size, uv_data_offset;
    int line_length;
    int frame_align_size;
    int alloc_width;
    int alloc_height;

    assert(mem_cnt >= 2);

    if ( (attr->dbus_type == SENSOR_DATA_BUS_MIPI ) && (attr->mipi.mipi_crop.enable) ){
        alloc_width = attr->mipi.mipi_crop.output_width;
        alloc_height = attr->mipi.mipi_crop.output_height;
    } else {
        alloc_width = attr->sensor_info.width;
        alloc_height = attr->sensor_info.height;
    }

    if (camera_fmt_is_NV12(attr->info.data_fmt) ) {
        line_length = ALIGN(alloc_width, 16);
        frm_size = line_length * alloc_height;
        if (frm_size % VIC_ALIGN_SIZE) {
            printk(KERN_ERR "frm_size not aligned\n");
            return -EINVAL;
        }
        uv_data_offset = frm_size;
        frm_size += frm_size / 2;

    } else if (is_output_y8(index, attr) || camera_fmt_is_8BIT(attr->info.data_fmt)){
        line_length = ALIGN(alloc_width, 8);
        frm_size = line_length * alloc_height;
        if (frm_size % VIC_ALIGN_SIZE) {
            printk(KERN_ERR "frm_size not aligned\n");
            return -EINVAL;
        }
        uv_data_offset = 0;

    } else {
        line_length = ALIGN(alloc_width, 8) * 2;
        frm_size = line_length * alloc_height;
        if (frm_size % VIC_ALIGN_SIZE) {
            printk(KERN_ERR "frm_size not aligned\n");
            return -EINVAL;
        }

        uv_data_offset = 0;
    }

    frame_align_size = ALIGN(frm_size, PAGE_SIZE);
    drv->uv_data_offset = uv_data_offset;

    drv->mem = rmem_alloc_aligned(frame_align_size * mem_cnt, PAGE_SIZE);
    if (drv->mem == NULL) {
        printk(KERN_ERR "vic%d : camera failed to alloc mem: %u\n", index, frame_align_size * mem_cnt);
        return -ENOMEM;
    }

    drv->mem_cnt = mem_cnt;
    drv->frm_size = frame_align_size;
    attr->info.fps = (attr->sensor_info.fps >> 16) / (attr->sensor_info.fps & 0xFFFF);
    attr->info.line_length = line_length;
    attr->info.phys_mem = virt_to_phys(drv->mem);
    attr->info.frame_size = frm_size;
    attr->info.frame_align_size = frame_align_size;
    attr->info.frame_nums = mem_cnt;

    drv->frames = kzalloc(drv->mem_cnt * sizeof(drv->frames[0]), GFP_KERNEL);
    assert(drv->frames);

    init_frm_lists(index);

    return 0;
}

static void vic_free_mem(int index)
{
    struct jz_vic_mem_data *drv = &jz_vic_mem_dev[index];

    rmem_free(drv->mem, drv->mem_cnt * drv->frm_size);
    kfree(drv->frames);
    drv->mem = NULL;
    drv->frames = NULL;
}

static int vic_mem_stream_on(int index, struct sensor_attr *attr)
{
    struct jz_vic_mem_data *drv = &jz_vic_mem_dev[index];
    int ret = 0;

    ret = vic_stream_on(index, attr);
    if (ret) {
        printk(KERN_ERR "vic%d(mem) : vic stream on failed\n", index);
        return ret;
    }

    enable_irq(drv->irq);

    drv->camera.is_stream_on = 1;

    return 0;
}

static void vic_mem_stream_off(int index, struct sensor_attr *attr)
{
    struct jz_vic_mem_data *drv = &jz_vic_mem_dev[index];

    disable_irq(drv->irq);

    vic_stream_off(index, attr);

    drv->camera.is_stream_on = 0;
}

static int vic_mem_power_on(int index)
{
    return vic_power_on(index);
}

static void vic_mem_power_off(int index)
{
    vic_power_off(index);
}

/******************************************************************************
 *
 * ioctl 函数实现
 *
 *****************************************************************************/
static int camera_stream_on(int index)
{
    struct jz_vic_mem_data *drv = &jz_vic_mem_dev[index];
    int ret = 0;

    mutex_lock(&drv->lock);

    if (!drv->camera.is_power_on) {
        printk(KERN_ERR "vic%d : camera can't stream on when not power on\n", index);
        ret = -EINVAL;
        goto unlock;
    }

    if (drv->camera.is_stream_on)
        goto unlock;

    reset_frm_lists(index);
    init_dma_addr(index);
    vic_set_bit(index, VIC_DMA_CONFIGURE, Dma_en, 1);

    ret = vic_mem_stream_on(index, drv->camera.sensor);

unlock:
    mutex_unlock(&drv->lock);

    return ret;
}

static void camera_stream_off(int index)
{
    struct jz_vic_mem_data *drv = &jz_vic_mem_dev[index];

    mutex_lock(&drv->lock);

    if (drv->camera.is_stream_on) {
        vic_mem_stream_off(index, drv->camera.sensor);
        wake_up_all(&drv->waiter);
    }

    mutex_unlock(&drv->lock);
}

static int camera_power_on(int index)
{
    struct jz_vic_mem_data *drv = &jz_vic_mem_dev[index];
    int ret = 0;

    mutex_lock(&drv->lock);

    if (!drv->camera.is_power_on) {
        ret = vic_mem_power_on(index);
        if (!ret) {
            drv->camera.is_power_on = 1;

            drv->vic_frm_done = 0;
            drv->vic_frm_output = 0;
        }
    }

    mutex_unlock(&drv->lock);

    return ret;
}

static void camera_power_off(int index)
{
    struct jz_vic_mem_data *drv = &jz_vic_mem_dev[index];

    mutex_lock(&drv->lock);

    if (drv->camera.is_stream_on)
        vic_mem_stream_off(index, drv->camera.sensor);

    if (drv->camera.is_power_on) {
        vic_mem_power_off(index);
        drv->camera.is_power_on = 0;
    }

    mutex_unlock(&drv->lock);
}

static inline void camera_dump_frame_data(struct frame_data *frm, int length)
{
    unsigned char *data;
    int i;

    if (frm) {
        data = frm->addr;
        for (i = 0; i < length; i++) {
            if ((i != 0) && (i % 16 == 0))
                printk("\n");

            printk("%02x:", data[i]);
        }
        printk("\n");
    } else {
        printk("frm is NULL\n");
    }
}

static int check_frame_mem(int index, void *mem, void *base)
{
    struct jz_vic_mem_data *drv = &jz_vic_mem_dev[index];
    unsigned int size = mem - base;

    if (mem < base)
        return -1;

    if (size % drv->frm_size)
        return -1;

    if (size / drv->frm_size >= drv->mem_cnt)
        return -1;

    return 0;
}

static inline struct frame_data *mem_2_frame_data(int index, void *mem)
{
    struct jz_vic_mem_data *drv = &jz_vic_mem_dev[index];
    unsigned int size = mem - drv->mem;
    unsigned int count = size / drv->frm_size;
    struct frame_data *frm = &drv->frames[count];

    assert(!(size % drv->frm_size));
    assert(count < drv->mem_cnt);

    return frm;
}

static int check_frame_info(int index, struct frame_info *info)
{
    struct jz_vic_mem_data *drv = &jz_vic_mem_dev[index];
    struct frame_data *frm = &drv->frames[info->index];

    if (info->index >= drv->mem_cnt)
        return -1;

    if (frm->info.paddr != info->paddr)
        return -1;

    return 0;
}

static inline struct frame_data *frame_info_2_frame_data(int index, struct frame_info *info)
{
    struct jz_vic_mem_data *drv = &jz_vic_mem_dev[index];

    if (!check_frame_info(index, info))
        return &drv->frames[info->index];
    else {
        //camera_dump_frame_info(info);
        return NULL;
    }
}

static void camera_put_frame(int index, struct frame_data *frm)
{
    struct jz_vic_mem_data *drv = &jz_vic_mem_dev[index];
    unsigned long flags;

    mutex_lock(&drv->lock);

    spin_lock_irqsave(&drv->spinlock, flags);

    if (frm->status != frame_status_user) {
        printk(KERN_ERR "vid%d camera double free of vic frame index %u\n", index, frm->info.index);
    } else {
        list_del_init(&frm->link);
        add_to_free_list(index, frm);
    }

    spin_unlock_irqrestore(&drv->spinlock, flags);

    mutex_unlock(&drv->lock);
}

static int camera_get_frame(int index, struct list_head *list, struct frame_data **frame, unsigned int timeout_ms)
{
    struct jz_vic_mem_data *drv = &jz_vic_mem_dev[index];
    struct frame_data *frm;
    unsigned long flags;
    int ret = -EAGAIN;

    mutex_lock(&drv->lock);

    if (!drv->camera.is_stream_on) {
        ret = -EINVAL;
        goto unlock;
    }

    spin_lock_irqsave(&drv->spinlock, flags);
    frm = get_usable_frm(index);
    spin_unlock_irqrestore(&drv->spinlock, flags);
    if (!frm && timeout_ms) {
        ret = wait_event_interruptible_timeout(
            drv->waiter, drv->frame_counter && drv->camera.is_stream_on, msecs_to_jiffies(timeout_ms));

        spin_lock_irqsave(&drv->spinlock, flags);
        frm = get_usable_frm(index);
        spin_unlock_irqrestore(&drv->spinlock, flags);
        if (ret <= 0 && !frm) {
            ret = -ETIMEDOUT;
            printk(KERN_ERR "camera: wait frame time out\n");
        }
    }

    if (frm) {
        ret = 0;
        frm->status = frame_status_user;
        list_add_tail(&frm->link, list);
        *frame = frm;
    }

unlock:
    mutex_unlock(&drv->lock);

    return ret;
}

static unsigned int camera_get_available_frame_count(int index)
{
    struct jz_vic_mem_data *drv = &jz_vic_mem_dev[index];

    return drv->frame_counter;
}

static void camera_skip_frames(int index, unsigned int frames)
{
    struct jz_vic_mem_data *drv = &jz_vic_mem_dev[index];
    unsigned long flags;

    spin_lock_irqsave(&drv->spinlock, flags);

    while (frames--) {
        struct frame_data *frm = get_usable_frm(index);
        if (frm == NULL)
            break;
        add_to_free_list(index, frm);
    }

    spin_unlock_irqrestore(&drv->spinlock, flags);
}

static int camera_get_info(int index, struct camera_info *info)
{
    struct jz_vic_mem_data *drv = &jz_vic_mem_dev[index];

    int ret = copy_to_user((void __user *)info, &drv->camera.sensor->info, sizeof(struct camera_info));
    if (ret)
        printk(KERN_ERR "%s : camera can't get sensor infor\n", drv->device_name);
    return 0;
}

static int camera_set_sensor_reg(int index, struct sensor_dbg_register *reg)
{
    struct jz_vic_mem_data *drv = &jz_vic_mem_dev[index];
    struct sensor_attr *sensor = drv->camera.sensor;
    struct sensor_dbg_register dbg_reg;
    int ret;

    if (copy_from_user(&dbg_reg, (void __user *)reg, sizeof(struct sensor_dbg_register))) {
        printk(KERN_ERR "%s copy from user error\n", __func__);
        return -EFAULT;
    }

    if (sensor->ops.set_register) {
        mutex_lock(&drv->sensor_mlock);
        ret = sensor->ops.set_register(&dbg_reg);
        mutex_unlock(&drv->sensor_mlock);
        if (ret < 0) {
            printk(KERN_ERR "%s set_register fail\n", __func__);
            return ret;
        }
    } else {
        printk(KERN_ERR "sensor->ops.set_register is NULL!\n");
        return -EINVAL;
    }

    return 0;
}

static int camera_get_sensor_reg(int index, struct sensor_dbg_register *reg)
{
    struct jz_vic_mem_data *drv = &jz_vic_mem_dev[index];
    struct sensor_attr *sensor = drv->camera.sensor;
    struct sensor_dbg_register dbg_reg;
    int ret;

    if (copy_from_user(&dbg_reg, (void __user *)reg, sizeof(struct sensor_dbg_register))) {
        printk(KERN_ERR "%s copy from user error\n", __func__);
        return -EFAULT;
    }

    if (sensor->ops.get_register) {
        mutex_lock(&drv->sensor_mlock);
        ret = sensor->ops.get_register(&dbg_reg);
        mutex_unlock(&drv->sensor_mlock);
        if (ret < 0) {
            printk(KERN_ERR "%s get_register fail\n", __func__);
            return ret;
        }
    } else {
        printk(KERN_ERR "sensor->ops.get_register is NULL!\n");
        return -EFAULT;
    }

    if (copy_to_user((void __user *)reg, &dbg_reg, sizeof(struct sensor_dbg_register))) {
        printk(KERN_ERR "%s copy from user error\n", __func__);
        return -EFAULT;
    }

    return 0;
}

static int camera_get_fps(int index, int *fps)
{
    struct jz_vic_mem_data *drv = &jz_vic_mem_dev[index];
    struct sensor_attr *sensor = drv->camera.sensor;

    if (copy_to_user((void __user *)fps, &sensor->sensor_info.fps, sizeof(int))) {
        printk(KERN_ERR "%s copy from user error\n", __func__);
        return -EFAULT;
    }
    return 0;
}

static int camera_set_fps(int index, int fps)
{
    struct jz_vic_mem_data *drv = &jz_vic_mem_dev[index];
    struct sensor_attr *sensor = drv->camera.sensor;
    int ret;

    if (sensor->ops.set_fps) {
        mutex_lock(&drv->sensor_mlock);
        ret = sensor->ops.set_fps(fps);
        mutex_unlock(&drv->sensor_mlock);
        if (ret) {
            printk(KERN_ERR "%s fail\n", __func__);
            return ret;
        }
    } else {
        printk(KERN_ERR "sensor->ops.set_fps is NULL!\n");
        return -EFAULT;
    }

    return 0;
}

static int camera_get_hflip(int index, camera_ops_mode *mode)
{
    struct jz_vic_mem_data *drv = &jz_vic_mem_dev[index];
    struct sensor_attr *sensor = drv->camera.sensor;
    camera_ops_mode tmp;
    int ret;

    if (sensor->ops.get_hflip) {
        mutex_lock(&drv->sensor_mlock);
        ret = sensor->ops.get_hflip(&tmp);
        mutex_unlock(&drv->sensor_mlock);
        if (ret) {
            printk(KERN_ERR "%s fail\n", __func__);
            return ret;
        }
    } else {
        printk(KERN_ERR "sensor->ops.get_hflip is NULL!\n");
        return -EFAULT;
    }

    if (copy_to_user((void __user *)mode, &tmp, sizeof(camera_ops_mode))) {
        printk(KERN_ERR "%s copy from user error\n", __func__);
        return -EFAULT;
    }

    return 0;
}

static int camera_set_hflip(int index, camera_ops_mode mode)
{
    struct jz_vic_mem_data *drv = &jz_vic_mem_dev[index];
    struct sensor_attr *sensor = drv->camera.sensor;
    int ret;

    if (sensor->ops.set_hflip) {
        mutex_lock(&drv->sensor_mlock);
        ret = sensor->ops.set_hflip(mode);
        mutex_unlock(&drv->sensor_mlock);
        if (ret < 0) {
            printk(KERN_ERR "%s fail\n", __func__);
            return ret;
        }
    } else {
        printk(KERN_ERR "sensor->ops.set_hflip is NULL!\n");
        return -EINVAL;
    }

    return 0;
}

static int camera_get_vflip(int index, camera_ops_mode *mode)
{
    struct jz_vic_mem_data *drv = &jz_vic_mem_dev[index];
    struct sensor_attr *sensor = drv->camera.sensor;
    camera_ops_mode tmp;
    int ret;

    if (sensor->ops.get_vflip) {
        mutex_lock(&drv->sensor_mlock);
        ret = sensor->ops.get_vflip(&tmp);
        mutex_unlock(&drv->sensor_mlock);
        if (ret) {
            printk(KERN_ERR "%s fail\n", __func__);
            return ret;
        }
    } else {
        printk(KERN_ERR "sensor->ops.get_vflip is NULL!\n");
        return -EFAULT;
    }

    if (copy_to_user((void __user *)mode, &tmp, sizeof(camera_ops_mode))) {
        printk(KERN_ERR "%s copy from user error\n", __func__);
        return -EFAULT;
    }

    return 0;
}

static int camera_set_vflip(int index, camera_ops_mode mode)
{
    struct jz_vic_mem_data *drv = &jz_vic_mem_dev[index];
    struct sensor_attr *sensor = drv->camera.sensor;
    int ret;

    if (sensor->ops.set_vflip) {
        mutex_lock(&drv->sensor_mlock);
        ret = sensor->ops.set_vflip(mode);
        mutex_unlock(&drv->sensor_mlock);
        if (ret < 0) {
            printk(KERN_ERR "%s fail\n", __func__);
            return ret;
        }
    } else {
        printk(KERN_ERR "sensor->ops.set_vflip is NULL!\n");
        return -EINVAL;
    }

    return 0;
}

static int camera_reset_fmt(int index, struct camera_info *info)
{
    struct jz_vic_mem_data *drv = &jz_vic_mem_dev[index];
    struct sensor_attr *sensor = drv->camera.sensor;
    int ret;

    if (drv->camera.is_power_on || drv->camera.is_stream_on) {
        printk(KERN_ERR "%s fail, please stream off and power off\n", __func__);
        return -EPERM;
    }

    if (sensor->ops.reset_fmt) {
        mutex_lock(&drv->sensor_mlock);
        ret = sensor->ops.reset_fmt(info);
        mutex_unlock(&drv->sensor_mlock);
        if (ret < 0) {
            printk(KERN_ERR "%s fail\n", __func__);
            return ret;
        }

        //re request buff
        mutex_lock(&drv->lock);
        vic_free_mem(index);
        ret = vic_alloc_mem(index, sensor);
        mutex_unlock(&drv->lock);
        if (ret)
            return ret;
    } else {
        printk(KERN_ERR "sensor->ops.reset_fmt is NULL!\n");
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
#define CMD_get_fps                 _IO('C', 135)
#define CMD_set_fps                 _IO('C', 136)
#define CMD_get_hflip               _IO('C', 137)
#define CMD_set_hflip               _IO('C', 138)
#define CMD_get_vflip               _IO('C', 139)
#define CMD_set_vflip               _IO('C', 140)
#define CMD_reset_fmt               _IO('C', 141)


struct m_private_data {
    unsigned int index;
    struct list_head list;
    void *map_mem;
};

static long vic_ioctl(struct file *filp, unsigned int cmd, unsigned long arg)
{
    struct m_private_data *data = filp->private_data;
    unsigned int index = data->index;
    struct jz_vic_mem_data *drv = &jz_vic_mem_dev[index];
    void *map_mem = data->map_mem;
    struct list_head *list = &data->list;
    struct frame_data *frm;
    unsigned int timeout_ms = 0;
    int ret;

    switch (cmd) {
    case CMD_get_info:{
         if (arg)
            return camera_get_info(index, (struct camera_info *)arg);
        else
            return -EINVAL;
    }

    case CMD_power_on:
        return camera_power_on(index);

    case CMD_power_off:
        camera_power_off(index);
        return 0;

    case CMD_stream_on:
        return camera_stream_on(index);

    case CMD_stream_off:
        camera_stream_off(index);
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

        ret = camera_get_frame(index, list, &frm, timeout_ms);
        if (0 == ret) {
            frm->info.vaddr = map_mem + (frm->addr - drv->mem);
            m_cache_sync(frm->addr, frm->info.size);
        } else
            return ret;

        *mem_p = frm->info.vaddr;
        drv->vic_frm_output++;
        return 0;
    }

    case CMD_put_frame: {
        void *mem = (void *)arg;
        if (check_frame_mem(index, mem, map_mem))
            return -EINVAL;

        mem = drv->mem + (mem - map_mem);
        frm = mem_2_frame_data(index, mem);
        m_cache_sync(frm->addr, frm->info.size);
        camera_put_frame(index, frm);
        return 0;
    }

    case CMD_dqbuf_wait:
        timeout_ms = 3000;
    case CMD_dqbuf: {
        if (!map_mem) {
            printk(KERN_ERR "camera: please mmap first\n");
            return -EINVAL;
        }

        ret = camera_get_frame(index, list, &frm, timeout_ms);
        if (0 == ret) {
            frm->info.vaddr = map_mem + (frm->addr - drv->mem);
            m_cache_sync(frm->addr, frm->info.size);
        } else
            return ret;

        ret = copy_to_user((void __user *)arg, &frm->info, sizeof(struct frame_info));
        if (ret) {
            printk(KERN_ERR "%s: CMD_dqbuf copy_to_user fail\n", drv->device_name);
            return ret;
        }

        drv->vic_frm_output++;
        return 0;
    }

    case CMD_qbuf: {
        struct frame_info info;
        ret = copy_from_user(&info, (void __user *)arg, sizeof(struct frame_info));
        if(ret){
            printk(KERN_ERR "%s: CMD_qbuf copy_from_user fail\n", drv->device_name);
            return ret;
        }
        frm = frame_info_2_frame_data(index, &info);
        if (!frm) {
            printk(KERN_ERR "vic_frame_info_2_frame_data fail\n");
            return -EINVAL;
        }
        m_cache_sync(frm->addr, frm->info.size);
        camera_put_frame(index, frm);
        return 0;
    }

    case CMD_get_frame_count: {
        unsigned int *count_p = (void *)arg;
        if (!count_p)
            return -EINVAL;

        *count_p = camera_get_available_frame_count(index);
        return 0;
    }

    case CMD_skip_frames: {
        unsigned int frames = arg;
        camera_skip_frames(index, frames);
        return 0;
    }

    case CMD_get_sensor_reg: {
        if (arg)
            return camera_get_sensor_reg(index, (struct sensor_dbg_register *)arg);
        else
            return -EINVAL;
    }

    case CMD_set_sensor_reg: {
        if (arg)
            return camera_set_sensor_reg(index, (struct sensor_dbg_register *)arg);
        else
            return -EINVAL;
    }

    case CMD_get_fps: {
        if (arg)
            return camera_get_fps(index, (int *)arg);
        else
            return -EINVAL;
    }
    case CMD_set_fps: {
        return camera_set_fps(index, (int)arg);
    }

    case CMD_get_hflip: {
        if (arg)
            return camera_get_hflip(index, (camera_ops_mode *)arg);
        else
            return -EINVAL;
    }
    case CMD_set_hflip: {
        return camera_set_hflip(index, (camera_ops_mode)arg);
    }

    case CMD_get_vflip: {
        if (arg)
            return camera_get_vflip(index, (camera_ops_mode *)arg);
        else
            return -EINVAL;
    }
    case CMD_set_vflip: {
        return camera_set_vflip(index, (camera_ops_mode)arg);
    }

    case CMD_reset_fmt: {
        struct camera_info info;
        if (arg) {
            ret = copy_from_user(&info, (void __user *)arg, sizeof(struct camera_info));
            if(ret){
                printk(KERN_ERR "%s: CMD_reset_fmt copy_from_user fail\n", drv->device_name);
                return ret;
            }
        } else
            printk(KERN_INFO "%s: CMD_reset_fmt param is NULL\n", drv->device_name);
        return camera_reset_fmt(index, &info);
    }

    default:
        printk(KERN_ERR "camera: %x not support this cmd\n", cmd);
        return -EINVAL;
    }

    return 0;
}

static int vic_mmap(struct file *file, struct vm_area_struct *vma)
{
    unsigned long len;
    unsigned long start;
    unsigned long off;
    struct m_private_data *data = file->private_data;
    int index = data->index;
    struct jz_vic_mem_data *drv = &jz_vic_mem_dev[index];

    off = vma->vm_pgoff << PAGE_SHIFT;

    if (off) {
        printk(KERN_ERR "vic%d : camera offset must be 0\n", index);
        return -EINVAL;
    }

    len = drv->frm_size * drv->mem_cnt;
    if ((vma->vm_end - vma->vm_start) != len) {
        printk(KERN_ERR "vic%d : camera size must be total size\n", index);
        return -EINVAL;
    }

    /* frame buffer memory */
    start = virt_to_phys(drv->mem);
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

    data->map_mem = (void *)vma->vm_start;

    return 0;
}

static int vic_open(struct inode *inode, struct file *filp)
{
    struct m_private_data *data = kmalloc(sizeof(*data), GFP_KERNEL);
    struct jz_vic_mem_data *drv = container_of(filp->private_data,
            struct jz_vic_mem_data, vic_mdev);

    data->index = drv->index;
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
    int index = data->index;
    struct jz_vic_mem_data *drv = &jz_vic_mem_dev[index];

    mutex_lock(&drv->lock);

    spin_lock_irqsave(&drv->spinlock, flags);
    list_for_each_safe(pos, n, list) {
        list_del_init(pos);
        struct frame_data *frm = list_entry(pos, struct frame_data, link);
        add_to_free_list(index, frm);
    }
    spin_unlock_irqrestore(&drv->spinlock, flags);

    kfree(data);

    mutex_unlock(&drv->lock);

    return 0;
}

static struct file_operations vic_misc_fops = {
    .open           = vic_open,
    .release        = vic_release,
    .mmap           = vic_mmap,
    .unlocked_ioctl = vic_ioctl,
};

#ifdef SOC_CAMERA_DEBUG
static ssize_t dsysfs_vic_mem_show_frame_cnt(struct device *dev, struct device_attribute *attr, char *buf)
{
    struct jz_vic_mem_data *drv = container_of((struct kobject *)dev, struct jz_vic_mem_data, dsysfs_kobj);
    char *p = buf;

    p += sprintf(p, "vic frame done: %u\n", drv->vic_frd_c);
    p += sprintf(p, "vic frame error: %u\n", drv->vic_fre_c);
    p += sprintf(p, "vic frame overflow: %u\n", drv->vic_frov_c);
    p += sprintf(p, "vic frame output: %u\n", drv->vic_frm_output);
    return p - buf;
}

static ssize_t dsysfs_vic_mem_dump_reg(struct device *dev, struct device_attribute *attr, char *buf)
{
    struct jz_vic_mem_data *drv = container_of((struct kobject *)dev, struct jz_vic_mem_data, dsysfs_kobj);
    int index = drv->index;
    char *p = buf;

    p += dsysfs_vic_dump_reg(index, p);

    return p - buf;
}

static ssize_t dsysfs_vic_mem_show_sensor_info(struct device *dev, struct device_attribute *attr, char *buf)
{
    struct jz_vic_mem_data *drv = container_of((struct kobject *)dev, struct jz_vic_mem_data, dsysfs_kobj);
    int index = drv->index;
    char *p = buf;

    p += dsysfs_vic_show_sensor_info(index, p);

    return p - buf;
}


static ssize_t dsysfs_vic_mem_ctrl(struct file *file, struct kobject *kobj, struct bin_attribute *attr, char *buf, loff_t pos, size_t count)
{
    struct jz_vic_mem_data *drv = container_of(kobj, struct jz_vic_mem_data, dsysfs_kobj);
    int index = drv->index;

    dsysfs_vic_ctrl(index, buf);

    return count;
}


static DSYSFS_DEV_ATTR(show_frm_cnt, S_IRUGO|S_IWUSR, dsysfs_vic_mem_show_frame_cnt, NULL);
static DSYSFS_DEV_ATTR(dump_vic_reg, S_IRUGO|S_IWUSR, dsysfs_vic_mem_dump_reg, NULL);
static DSYSFS_DEV_ATTR(show_sensor_info, S_IRUGO|S_IWUSR, dsysfs_vic_mem_show_sensor_info, NULL);
static struct attribute *dsysfs_vic_dev_attrs[] = {
    &dsysfs_dev_attr_show_frm_cnt.attr,
    &dsysfs_dev_attr_dump_vic_reg.attr,
    &dsysfs_dev_attr_show_sensor_info.attr,
    NULL,
};

static DSYSFS_BIN_ATTR(ctrl, S_IRUGO|S_IWUSR, NULL, dsysfs_vic_mem_ctrl, 0);
static struct bin_attribute *dsysfs_vic_bin_attrs[] = {
    &dsysfs_bin_attr_ctrl,
    NULL,
};

static const struct attribute_group dsysfs_vic_attr_group = {
    .attrs  = dsysfs_vic_dev_attrs,
    .bin_attrs = dsysfs_vic_bin_attrs,
};
#endif


#define error_if(_cond)                                                 \
    do {                                                                \
        if (_cond) {                                                    \
            printk(KERN_ERR "vic(mem): failed to check: %s\n", #_cond);   \
            ret = -1;                                                   \
            goto unlock;                                                \
        }                                                               \
    } while (0)



/*
 * 格式信息转换 转换后的格式经过VIC DMA 到DDR
 * sensor format : Sensor格式在sensor driver中根据setting指定
 * camera format : Camera格式在camera driver中使用,并暴露给应用
 *
 *    sensor格式                  Camera格式(VIC扩展为16bit)
 * [成员0 sensor format]   <--->  [成员1 camera format]
 *
 */
struct fmt_pair {
    sensor_pixel_fmt sensor_fmt;
    camera_pixel_fmt camera_fmt;
};

static struct fmt_pair fmts[] = {
    {SENSOR_PIXEL_FMT_Y8_1X8,           CAMERA_PIX_FMT_GREY},   /* 其他条件必须满足 8bit要求 */
    {SENSOR_PIXEL_FMT_UYVY8_2X8,        CAMERA_PIX_FMT_UYVY},
    {SENSOR_PIXEL_FMT_VYUY8_2X8,        CAMERA_PIX_FMT_VYUY},
    {SENSOR_PIXEL_FMT_YUYV8_2X8,        CAMERA_PIX_FMT_YUYV},
    {SENSOR_PIXEL_FMT_YVYU8_2X8,        CAMERA_PIX_FMT_YVYU},

    {SENSOR_PIXEL_FMT_SBGGR8_1X8,       CAMERA_PIX_FMT_SBGGR8}, /* 其他条件必须满足 8bit要求 */
    {SENSOR_PIXEL_FMT_SGBRG8_1X8,       CAMERA_PIX_FMT_SGBRG8}, /* 其他条件必须满足 8bit要求 */
    {SENSOR_PIXEL_FMT_SGRBG8_1X8,       CAMERA_PIX_FMT_SGRBG8}, /* 其他条件必须满足 8bit要求 */
    {SENSOR_PIXEL_FMT_SRGGB8_1X8,       CAMERA_PIX_FMT_SRGGB8}, /* 其他条件必须满足 8bit要求 */
    {SENSOR_PIXEL_FMT_SBGGR10_1X10,     CAMERA_PIX_FMT_SBGGR16},
    {SENSOR_PIXEL_FMT_SGBRG10_1X10,     CAMERA_PIX_FMT_SGBRG16},
    {SENSOR_PIXEL_FMT_SGRBG10_1X10,     CAMERA_PIX_FMT_SGRBG16},
    {SENSOR_PIXEL_FMT_SRGGB10_1X10,     CAMERA_PIX_FMT_SRGGB16},
    {SENSOR_PIXEL_FMT_SBGGR12_1X12,     CAMERA_PIX_FMT_SBGGR16},
    {SENSOR_PIXEL_FMT_SGBRG12_1X12,     CAMERA_PIX_FMT_SGBRG16},
    {SENSOR_PIXEL_FMT_SGRBG12_1X12,     CAMERA_PIX_FMT_SGRBG16},
    {SENSOR_PIXEL_FMT_SRGGB12_1X12,     CAMERA_PIX_FMT_SRGGB16},
};

static int sensor_attribute_check_init(int index, struct sensor_attr *sensor)
{
    int ret = -EINVAL;

    error_if(!sensor->device_name);
    error_if(sensor->sensor_info.width < 64 || sensor->sensor_info.width > 3840);
    error_if(sensor->sensor_info.height < 64 || sensor->sensor_info.height > 2560);
    error_if(!sensor->ops.power_on);
    error_if(!sensor->ops.power_off);
    error_if(!sensor->ops.stream_on);
    error_if(!sensor->ops.stream_off);

    memset(sensor->info.name, 0x00, sizeof(sensor->info.name));
    memcpy(sensor->info.name, sensor->device_name, strlen(sensor->device_name));

    sensor->info.width =  sensor->sensor_info.width;
    sensor->info.height =  sensor->sensor_info.height;

    int i = 0;
    int size = ARRAY_SIZE(fmts);
    for (i = 0; i < size; i++) {
        if (sensor->sensor_info.fmt == fmts[i].sensor_fmt)
            break;
    }

    if (i >= size) {
        printk(KERN_ERR "attribute check: sensor data_fmt(0x%x) is NOT support.\n", sensor->sensor_info.fmt);
        goto unlock;
    }

    /* 当sensor输出为8BIT时, 其他控制条件必须同时满足为8bit */
    if (sensor_fmt_is_8BIT(sensor->sensor_info.fmt)
        && !is_output_y8(index, sensor)) {
        if (sensor->dbus_type == SENSOR_DATA_BUS_DVP) {
            printk(KERN_ERR "Please check sensor format and VIC data format\n");
            printk(KERN_ERR "sensor format is 8BIT(0x%x)\n", sensor->sensor_info.fmt);
            printk(KERN_ERR "VIC inteface(DVP) data format is not 8BIT(0x%x)\n", sensor->dvp.data_fmt);
            goto unlock;
        }

        if (sensor->dbus_type == SENSOR_DATA_BUS_MIPI) {
            printk(KERN_ERR "Please check sensor format and VIC data format\n");
            printk(KERN_ERR "sensor format is 8BIT(0x%x)\n", sensor->sensor_info.fmt);
            printk(KERN_ERR "VIC inteface(MIPI) data format is not 8BIT(0x%x)\n", sensor->mipi.data_fmt);
            goto unlock;
        }

        printk(KERN_ERR "Now dbus type(%d) not support\n", sensor->dbus_type);
        goto unlock;
    }

    camera_pixel_fmt data_fmt = fmts[i].camera_fmt;

    /* 当sensor输出格式为YUV422时,可通过VIC DMA重新排列输出格式 */
    if (is_output_yuv422(index, sensor) && camera_fmt_is_YUV422(data_fmt)) {
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

    if (sensor->dbus_type == SENSOR_DATA_BUS_DVP) {
        switch(sensor->dvp.gpio_mode) {
        case DVP_PA_LOW_10BIT:
        case DVP_PA_HIGH_10BIT:
            if (sensor->dvp.data_fmt > DVP_RAW10) {
                printk(KERN_ERR "attribute check: data_fmt set error,should be less than DVP_RAW12.\n");
                goto unlock;
            }
            break;

        case DVP_PA_12BIT:
            break;

        case DVP_PA_LOW_8BIT:
        case DVP_PA_HIGH_8BIT:
            if (sensor->dvp.data_fmt < DVP_YUV422) {
                if (sensor->dvp.data_fmt > DVP_RAW8) {
                    printk(KERN_ERR "attribute check: data_fmt set error,should be DVP_RAW8.\n");
                    goto unlock;
                }
            }
            break;

        default:
            printk(KERN_ERR "attribute check: Unsupported this format.\n");
            goto unlock;
        }
    }
    return 0;

unlock:
    return ret;
}

int vic_register_sensor_route_mem(int index, int mem_cnt, struct sensor_attr *sensor)
{
    assert(index < 2);

    struct jz_vic_mem_data *drv = &jz_vic_mem_dev[index];
    int ret = 0;

    assert(drv->is_finish > 0);
    assert(!drv->camera.sensor);

    ret = sensor_attribute_check_init(index, sensor);
    assert(ret == 0);

    mutex_lock(&drv->lock);

    drv->cam_mem_cnt = mem_cnt;
    if (drv->cam_mem_cnt < 1) {
        drv->cam_mem_cnt = 2;
        printk(KERN_ERR "vic%d device(camera): mem cnt fix to 2\n", index);
    }

    drv->camera.sensor = sensor;
    ret = vic_alloc_mem(index, sensor);
    if (ret) {
        drv->camera.sensor = NULL;
        goto unlock;
    }

    ret = misc_register(&drv->vic_mdev);
    assert(!ret);

    drv->camera.is_power_on = 0;
    drv->camera.is_stream_on = 0;

unlock:
    mutex_unlock(&drv->lock);

    return ret;
}

void vic_unregister_sensor_route_mem(int index, struct sensor_attr *sensor)
{
    assert(index < 2);

    struct jz_vic_mem_data *drv = &jz_vic_mem_dev[index];

    assert(drv->is_finish > 0);
    assert(drv->camera.sensor);
    assert(sensor == drv->camera.sensor);

    mutex_lock(&drv->lock);

    if (drv->camera.is_stream_on)
        panic("vic%d(mem) : failed to unregister, when camera stream on!\n", index);

    if (drv->camera.is_power_on) {
        struct sensor_attr *attr = drv->camera.sensor;
        attr->ops.power_off();

        drv->camera.is_power_on = 0;
    }
    drv->camera.sensor = NULL;

    misc_deregister(&drv->vic_mdev);

    vic_free_mem(index);

    mutex_unlock(&drv->lock);
}


int jz_vic_mem_drv_init(int index)
{
    assert(index < 2);

    struct jz_vic_mem_data *drv = &jz_vic_mem_dev[index];
    int ret;

    memset(&drv->vic_mdev, 0, sizeof(struct miscdevice));

    drv->vic_mdev.minor = MISC_DYNAMIC_MINOR;
    drv->vic_mdev.name  = drv->device_name;
    drv->vic_mdev.fops  = &vic_misc_fops;

    init_waitqueue_head(&drv->waiter);
    mutex_init(&drv->lock);
    mutex_init(&drv->sensor_mlock);
    spin_lock_init(&drv->spinlock);

    ret = request_irq(drv->irq, vic_irq_dma_handler, 0, drv->irq_name, (void *)drv);
    if (ret) {
        printk(KERN_ERR "camera: vic%d(mem) failed to request irq\n", index);
        goto error_request_irq;
    }
    disable_irq(drv->irq);

#ifdef SOC_CAMERA_DEBUG
    char dsysfs_root_dir_name[16];
    sprintf(dsysfs_root_dir_name ,"vic%d", drv->index);
    ret = dsysfs_create_group(&drv->dsysfs_kobj, NULL, dsysfs_root_dir_name, &dsysfs_vic_attr_group);
    if (ret)
        printk(KERN_ERR "vic%d(mem) dsysfs create fail\n", index);
#endif

    drv->is_finish = 1;

    printk(KERN_DEBUG "vic(mem)%d register successfully\n", index);

    return 0;

error_request_irq:
    return ret;
}

void jz_vic_mem_drv_deinit(int index)
{
    assert(index < 2);

    struct jz_vic_mem_data *drv = &jz_vic_mem_dev[index];

    if (!drv->is_finish)
        return ;

    drv->is_finish = 0;

    free_irq(drv->irq, drv);
#ifdef SOC_CAMERA_DEBUG
    dsysfs_remove_group(&drv->dsysfs_kobj, &dsysfs_vic_attr_group);
#endif
}
