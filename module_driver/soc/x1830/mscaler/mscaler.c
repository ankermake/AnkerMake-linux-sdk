/*
 * Copyright (c) 2020 Ingenic Semiconductor Co., Ltd.
 *              http://www.ingenic.com/
 *
 * Input file for Ingenic mscaler driver
 *
 * This  program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 *
 */

#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/errno.h>
#include <linux/clk.h>
#include <linux/mutex.h>
#include <linux/poll.h>
#include <linux/wait.h>
#include <linux/fs.h>
#include <linux/irq.h>
#include <linux/mm.h>
#include <linux/ctype.h>
#include <linux/dma-mapping.h>
#include <linux/interrupt.h>
#include <linux/platform_device.h>
#include <linux/io.h>
#include <linux/memory.h>
#include <linux/suspend.h>
#include <linux/miscdevice.h>
#include <linux/proc_fs.h>
#include <linux/delay.h>
#include <linux/completion.h>
#include <linux/time.h>
#include <asm/cacheflush.h>
#include <soc/irq.h>
#include <assert.h>
#include "mscaler.h"
#include "mscaler_hal.c"

#define MSCALER_ALIGN_SIZE       16
#define WAIT_TIMEOUT             3000
#define error_if(_cond) \
    do { \
        if (_cond) { \
            printk(KERN_ERR "mscaler:  failed to check: %s\n",#_cond); \
            return -1; \
        } \
    } while (0)

struct mscaler_dev {
    struct clk *clk;
    struct miscdevice mdev;

    unsigned int status;
    struct mutex mutex;
    wait_queue_head_t wait_queue;
};

static struct mscaler_dev mscaler_device;

static int mscaler_fmt_config(struct mscaler_param *ms_param)
{
    /*1.Set source image size , data address and stride (only DMAIN),format and work mode(DMAIN or OnTheFly);*/
    mscaler_hal_src_size(ms_param->src->xres, ms_param->src->yres);
    mscaler_hal_src_y_addr(ms_param->src->y.phys_addr);
    mscaler_hal_src_uv_addr(ms_param->src->uv.phys_addr);
    mscaler_hal_src_y_stride(ms_param->src->y.stride);
    mscaler_hal_src_uv_stride(ms_param->src->uv.stride);
    mscaler_hal_src_in(FROM_DRAM, ms_param->src->fmt);

    /*2.Set output image size , resize step ,address and stride, resize coefficient, crop position and size ,
    mask position and size, frame rate control and channel enable ;*/
    mscaler_hal_resize_step(MSCALER_CHANNEL, ms_param->src->xres, ms_param->src->yres, ms_param->dst->xres, ms_param->dst->yres);
    mscaler_hal_resize_osize(MSCALER_CHANNEL, ms_param->dst->xres, ms_param->dst->yres);
    int64_t record_time_ms = jiffies_to_msecs(jiffies);
    /*judge fifo is full, 1 full 0 empty*/
    while (mscaler_hal_judge_chx_y_addr_fifo_full(MSCALER_CHANNEL)) {
        if (jiffies_to_msecs(jiffies) - record_time_ms > WAIT_TIMEOUT) {
            return -1;
        }
    };

    mscaler_hal_dst_y_addr(MSCALER_CHANNEL, ms_param->dst->y.phys_addr);
    mscaler_hal_dst_uv_addr(MSCALER_CHANNEL, ms_param->dst->uv.phys_addr);


    mscaler_hal_dst_y_stride(MSCALER_CHANNEL, ms_param->dst->y.stride);
    mscaler_hal_dst_uv_stride(MSCALER_CHANNEL, ms_param->dst->uv.stride);

    mscaler_hal_csc_normal_configure();
    mscaler_hal_csc_offset_parameter();

    mscaler_hal_crop_opos(MSCALER_CHANNEL);
    mscaler_hal_crop_osize(MSCALER_CHANNEL, ms_param->dst->xres, ms_param->dst->yres);

    mscaler_hal_chx_framerate_ctrl(MSCALER_CHANNEL, 31);//frame kept deep 0-31
    /* set output format */
    mscaler_hal_dst_fmt(MSCALER_CHANNEL, ms_param->dst->fmt);
    return 0;
}

static int mscaler_start(struct mscaler_param *ms_param)
{
    int ret = 0;

    mscaler_hal_disable_all_clkgate();

    ret = mscaler_fmt_config(ms_param);
    if (ret < 0) {
        printk(KERN_ERR "mscaler: mscaler_fmt_config dst_yaddr is full timeout err %d\n", ret);
        mscaler_dump_regs(MSCALER_CHANNEL);
        return ret;
    }

    mscaler_hal_clr_allirq();
    mscaler_hal_clr_allmask();
    mscaler_hal_dma_done_mode(0);// 1 dma depend on axi
    mscaler_hal_enable_channel(MSCALER_CHANNEL);

    dma_cache_sync(NULL, ms_param->src->y.mem, ms_param->src->y.mem_size, DMA_TO_DEVICE);
    dma_cache_sync(NULL, ms_param->src->uv.mem, ms_param->src->uv.mem_size, DMA_TO_DEVICE);

    enable_irq(IRQ_MSCALER);
    mscaler_hal_softreset();
    //mscaler_dump_regs(MSCALER_CHANNEL);

    mscaler_device.status = 1;
    ret = wait_event_interruptible_timeout(mscaler_device.wait_queue,!mscaler_device.status, msecs_to_jiffies(WAIT_TIMEOUT));
    if (mscaler_device.status == 1) {
        ret = -1;
        printk(KERN_ERR "mscaler: done_mscaler wait_for_completion_interruptible_timeout error %d\n", ret);
        mscaler_dump_regs(MSCALER_CHANNEL);
    } else
        ret = 0;

    dma_cache_sync(NULL, ms_param->dst->y.mem, ms_param->dst->y.mem_size, DMA_FROM_DEVICE);
    if (ms_param->dst->uv.mem)
        dma_cache_sync(NULL, ms_param->dst->uv.mem, ms_param->dst->uv.mem_size, DMA_FROM_DEVICE);
    mscaler_hal_stop();
    mscaler_hal_disable_channel(MSCALER_CHANNEL);
    disable_irq(IRQ_MSCALER);
    return ret;
}

int mscaler_check(struct mscaler_param *ms_param)
{
    int src_alignsize_y = 0;
    int src_alignsize_uv = 0;
    int dst_alignsize_y = 0;
    int dst_alignsize_uv = 0;

    if (IS_ERR(ms_param))
        return -EINVAL;

    if (IS_ERR(ms_param->src))
        return -EINVAL;

    if (IS_ERR(ms_param->dst))
        return -EINVAL;

    struct mscaler_frame *src = ms_param->src;
    struct mscaler_frame *dst = ms_param->dst;

    error_if(src->fmt != MSCALER_FORMAT_NV12 && src->fmt != MSCALER_FORMAT_NV21);
    error_if((src->y.phys_addr % MSCALER_ALIGN_SIZE) ||
        (src->uv.phys_addr % MSCALER_ALIGN_SIZE) ||
        (dst->y.phys_addr % MSCALER_ALIGN_SIZE) ||
        (dst->uv.phys_addr % MSCALER_ALIGN_SIZE));
    error_if(src->y.phys_addr == 0 || src->y.mem == NULL ||
        src->uv.phys_addr == 0 || src->uv.mem == NULL ||
        dst->y.phys_addr == 0 || dst->y.mem == NULL);
    error_if(src->xres > MSCALER_INPUT_MAX_WIDTH || src->xres < MSCALER_INPUT_MIN_WIDTH);
    error_if(src->yres > MSCALER_INPUT_MAX_HEIGHT || src->yres < MSCALER_INPUT_MIN_HEIGHT);
    error_if(dst->xres > MSCALER_OUTPUT0_MAX_WIDTH || dst->xres < MSCALER_OUTPUT_MIN_WIDTH);
    error_if(dst->yres > MSCALER_OUTPUT0_MAX_WIDTH || dst->yres < MSCALER_OUTPUT_MIN_WIDTH);

    error_if(src->y.stride % MSCALER_ALIGN_SIZE || src->y.stride < src->xres);
    src_alignsize_y = src->y.stride * src->yres;
    src_alignsize_y = ALIGN(src_alignsize_y, L1_CACHE_BYTES);
    error_if(src->y.mem_size < src_alignsize_y);

    error_if(src->uv.stride % MSCALER_ALIGN_SIZE || src->uv.stride < src->xres);
    src_alignsize_uv = src->uv.stride * src->yres / 2;
    src_alignsize_uv = ALIGN(src_alignsize_uv, L1_CACHE_BYTES);
    error_if(src->uv.mem_size < src_alignsize_uv);

    switch (dst->fmt) {
        case MSCALER_FORMAT_NV12:
        case MSCALER_FORMAT_NV21:
            error_if(dst->uv.phys_addr == 0 || dst->uv.mem == NULL);

            error_if(dst->y.stride % MSCALER_ALIGN_SIZE || dst->y.stride < dst->xres);
            dst_alignsize_y = dst->y.stride * dst->yres;
            dst_alignsize_y = ALIGN(dst_alignsize_y, L1_CACHE_BYTES);
            error_if(dst->y.mem_size < dst_alignsize_y);

            error_if(dst->uv.stride % MSCALER_ALIGN_SIZE || dst->uv.stride < dst->xres);
            dst_alignsize_uv = dst->uv.stride * dst->yres / 2;
            dst_alignsize_uv = ALIGN(dst_alignsize_uv, L1_CACHE_BYTES);
            error_if(dst->uv.mem_size < dst_alignsize_uv);
            break;

        case MSCALER_FORMAT_BGRA_8888:
        case MSCALER_FORMAT_GBRA_8888:
        case MSCALER_FORMAT_RBGA_8888:
        case MSCALER_FORMAT_BRGA_8888:
        case MSCALER_FORMAT_GRBA_8888:
        case MSCALER_FORMAT_RGBA_8888:
        case MSCALER_FORMAT_ABGR_8888:
        case MSCALER_FORMAT_AGBR_8888:
        case MSCALER_FORMAT_ARBG_8888:
        case MSCALER_FORMAT_ABRG_8888:
        case MSCALER_FORMAT_AGRB_8888:
        case MSCALER_FORMAT_ARGB_8888:
            error_if(dst->y.stride % MSCALER_ALIGN_SIZE || dst->y.stride < dst->xres * 4);
            dst_alignsize_y = dst->y.stride * dst->yres;
            dst_alignsize_y = ALIGN(dst_alignsize_y, L1_CACHE_BYTES);
            error_if(dst->y.mem_size < dst_alignsize_y);
            break;

        case MSCALER_FORMAT_BGR_565:
        case MSCALER_FORMAT_GBR_565:
        case MSCALER_FORMAT_RBG_565:
        case MSCALER_FORMAT_BRG_565:
        case MSCALER_FORMAT_GRB_565:
        case MSCALER_FORMAT_RGB_565:
            error_if(dst->y.stride % MSCALER_ALIGN_SIZE || dst->y.stride < dst->xres * 2);
            dst_alignsize_y = dst->y.stride * dst->yres;
            dst_alignsize_y = ALIGN(dst_alignsize_y, L1_CACHE_BYTES);
            error_if(dst->y.mem_size < dst_alignsize_y);
            break;

         default:
            printk(KERN_ERR "mscaler: dst_fmt not support\n");
            return -1;
    }

    dst_alignsize_y = ALIGN(dst_alignsize_y, L1_CACHE_BYTES);
    error_if(dst->y.mem_size < dst_alignsize_y);

    return 0;
}

int mscaler_convert(struct mscaler_param *ms_param)
{
    int ret = 0;

    ret = mscaler_check(ms_param);
    if (ret < 0) {
        printk(KERN_ERR "mscaler: mscaler_check error: ret = %d\n", ret);
        return ret;
    }

    ret = mscaler_start(ms_param);
    if (ret < 0) {
        printk(KERN_ERR "mscaler: mscaler_start error: ret = %d\n", ret);
    }

    return ret;
}

static int mem_mmap(struct file *filp, struct vm_area_struct *vma)
{
    struct mscaler_info *ms_info = filp->private_data;

    if (ms_info == NULL)
        return -EINVAL;

    if (vma->vm_pgoff != 0)
        return -EINVAL;

    if ((vma->vm_end - vma->vm_start) != ms_info->alloc_alignsize)
        return -EINVAL;

    pgprot_val(vma->vm_page_prot) &= ~_CACHE_MASK;
    /* 0: cachable,write through (cache + cacheline对齐写穿)*/
    pgprot_val(vma->vm_page_prot) |= _CACHE_CACHABLE_NO_WA;

    vma->vm_pgoff = (unsigned long)ms_info->phys_addr >> PAGE_SHIFT;
    vma->vm_flags |= VM_IO;

    if (io_remap_pfn_range(vma,vma->vm_start,
           vma->vm_pgoff,
           vma->vm_end - vma->vm_start,
           vma->vm_page_prot))
        return -EAGAIN;

    return 0;
}

static int ioctl_alloc_size(struct file *filp, unsigned long arg)
{
    int ret = 0;
    struct mscaler_info info;

    if (copy_from_user(&info, (void __user *) arg, sizeof(struct mscaler_info)))
        return -EFAULT;

    mutex_lock(&mscaler_device.mutex);
    if (filp->private_data) {
        printk(KERN_ERR "mscaler: can't double malloc, you must free it\n");
        ret = -EBUSY;
        goto unlock;
    }

    struct mscaler_info *ms_info = kmalloc(sizeof(struct mscaler_info), GFP_KERNEL);
    if (ms_info == NULL) {
        printk(KERN_ERR "mscaler: can't malloc mscaler_info: %d\n", sizeof(struct mscaler_info));
        ret = -ENOMEM;
        goto unlock;
    }

    info.alloc_alignsize = ALIGN(info.alloc_size, PAGE_SIZE);
    void *mem = kzalloc(info.alloc_alignsize, GFP_KERNEL);
    if (mem == NULL) {
        printk(KERN_ERR "mscaler: can't malloc mem: %ld\n", info.alloc_alignsize);
        ret = -ENOMEM;
        goto free_ms_info;
    }

    ms_info->alloc_alignsize = info.alloc_alignsize;
    ms_info->phys_addr = (void *)virt_to_phys(mem);

    if (copy_to_user((void __user *) arg, ms_info,  sizeof(struct mscaler_info))) {
        ret = -EFAULT;
        goto free_ms_mem;
    }

    filp->private_data = ms_info;

    goto unlock;
free_ms_mem:
    kfree(mem);
free_ms_info:
    kfree(ms_info);
unlock:
    mutex_unlock(&mscaler_device.mutex);

    return ret;
}

static long mscaler_ioctl(struct file *filp, unsigned int cmd, unsigned long arg)
{
    int err = 0;
    int ret = 0;
    struct mscaler_param ms_param;
    if (_IOC_TYPE(cmd) != JZMSCALER_IOC_MAGIC) {
        printk(KERN_ERR "mscaler: invalid cmd!\n");
        return -EFAULT;
    }
    if (_IOC_DIR(cmd) & _IOC_READ)
        err = !access_ok(VERIFY_WRITE,
            (void __user *)arg, _IOC_SIZE(cmd));
    if (_IOC_DIR(cmd) & _IOC_WRITE)
        err = !access_ok(VERIFY_READ,
            (void __user *)arg, _IOC_SIZE(cmd));
    if (err)
        return -EFAULT;

    switch (cmd) {
        case IOCTL_MSCALER_CONVERT:
            if (copy_from_user(&ms_param, (void __user *) arg, sizeof(struct mscaler_param)))
                return -EFAULT;

            mutex_lock(&mscaler_device.mutex);
            ret = mscaler_convert(&ms_param);
            if (ret)
                printk(KERN_ERR "mscaler: error mscaler_convert ret = %d\n", ret);

            mutex_unlock(&mscaler_device.mutex);
            break;

        case IOCTL_MSCALER_ALIGN_SIZE:
            __put_user(MSCALER_ALIGN_SIZE, (__u32 __user *)arg);
            break;

        case IOCTL_L1CACHE_ALIGN_SIZE:
            __put_user(L1_CACHE_BYTES, (__u32 __user *)arg);
            break;

        case IOCTL_MSCALER_ALLOC_ALIGN_SIZE:
            ret = ioctl_alloc_size(filp, arg);
            break;

        case IOCTL_MSCALER_FREE_ALIGN_SIZE:
            mutex_lock(&mscaler_device.mutex);
            struct mscaler_info *ms_info = filp->private_data;

            if (ms_info) {
                if(ms_info->phys_addr)
                    kfree(phys_to_virt((unsigned long)ms_info->phys_addr));

                kfree(ms_info);
                filp->private_data = NULL;
            }

            mutex_unlock(&mscaler_device.mutex);
            break;

        default:
            printk(KERN_ERR "mscaler: invalid command: 0x%08x\n", cmd);
            ret = -EINVAL;
    }

    return ret;
}

static int mscaler_open(struct inode *inode, struct file *filp)
{
    filp->private_data = NULL;
    return 0;
}

static int mscaler_release(struct inode *inode, struct file *filp)
{
    mutex_lock(&mscaler_device.mutex);
    struct mscaler_info *ms_info = filp->private_data;

    if (ms_info) {
        if(ms_info->phys_addr)
            kfree(phys_to_virt((unsigned long)ms_info->phys_addr));

        kfree(ms_info);
        filp->private_data = NULL;
    }

    mutex_unlock(&mscaler_device.mutex);
    return 0;
}

static struct file_operations mscaler_ops = {
    .owner          = THIS_MODULE,
    .open           = mscaler_open,
    .release        = mscaler_release,
    .unlocked_ioctl = mscaler_ioctl,
    .mmap           = mem_mmap,
};

static irqreturn_t mscaler_irq_handler(int irq, void *data)
{
    unsigned int pending = 0;
    pending = mscaler_hal_get_irqstate() & (~ mscaler_hal_get_irqmask());
    unsigned int irq_bit = __ffs(pending);
    switch (irq_bit) {
        case MS_IRQ_CH2_DONE_BIT:
            mscaler_hal_clr_irq(1 << irq_bit);
            mscaler_device.status = 0;
            wake_up_all(&mscaler_device.wait_queue);
            break;

        case MS_IRQ_CH1_DONE_BIT:
            mscaler_hal_clr_irq(1 << irq_bit);
            mscaler_device.status = 0;
            wake_up_all(&mscaler_device.wait_queue);
            break;

        case MS_IRQ_CH0_DONE_BIT:
            mscaler_hal_clr_irq(1 << irq_bit);
            mscaler_device.status = 0;
            wake_up_all(&mscaler_device.wait_queue);
            break;

        case MS_IRQ_OVF_BIT:
            mscaler_hal_clr_irq(1 << irq_bit);
            break;

        case MS_IRQ_CSC_BIT:
            mscaler_hal_clr_irq(1 << irq_bit);
            break;

        case MS_IRQ_FRM_BIT:
            mscaler_hal_clr_irq(1 << irq_bit);
            break;

        case MS_IRQ_CH2_CROP_BIT:
            mscaler_hal_clr_irq(1 << irq_bit);
            break;

        case MS_IRQ_CH1_CROP_BIT:
            mscaler_hal_clr_irq(1 << irq_bit);
            break;

        case MS_IRQ_CH0_CROP_BIT:
            mscaler_hal_clr_irq(1 << irq_bit);
            break;

        default:
            break;
    }
    return IRQ_HANDLED;
}

struct miscdevice mscaler_mdevice = {
    .minor  = MISC_DYNAMIC_MINOR,
    .name   = "jz_mscaler",
    .fops   = &mscaler_ops,
};

static void __exit mscaler_exit(void)
{
    int ret = 0;
    free_irq(IRQ_MSCALER, NULL);

    clk_disable(mscaler_device.clk);
    clk_put(mscaler_device.clk);

    ret = misc_deregister(&mscaler_mdevice);
    if (ret < 0) {
        panic("mscaler: %s deregister misc device failed!\n",__func__);
    }
}

static int __init mscaler_init(void)
{
    int ret = 0;
    mscaler_device.clk = clk_get(NULL, "mscaler");
    clk_enable(mscaler_device.clk);

    mutex_init(&mscaler_device.mutex);
    init_waitqueue_head(&mscaler_device.wait_queue);

    ret = request_irq(IRQ_MSCALER, mscaler_irq_handler, IRQF_ONESHOT, mscaler_mdevice.name, NULL);
    if (ret < 0) {
        clk_put(mscaler_device.clk);
        printk(KERN_ERR "mscaler: request_irq failed!\n");
        return ret;
    }

    disable_irq(IRQ_MSCALER);
    mscaler_device.mdev = mscaler_mdevice;

    ret = misc_register(&mscaler_mdevice);
    if (ret < 0)
        panic("mscaler: register misc device failed!\n");

    return 0;
}

module_init(mscaler_init);
module_exit(mscaler_exit);

MODULE_DESCRIPTION("JZ mscaler driver");
MODULE_AUTHOR("yichun zhou     <yichun.zhou@ingenic.com>");
MODULE_LICENSE("GPL");
