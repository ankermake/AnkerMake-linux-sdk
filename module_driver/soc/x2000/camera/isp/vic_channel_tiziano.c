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

#include "csi.h"
#include "isp.h"
#include "dsys.h"
#include "vic.h"
#include "drivers/rmem_manager/rmem_manager.h"

struct jz_vic_tiziano_data {
    int index;
    int is_enable;
    int is_finish;

    int irq;
    const char *irq_name;

    struct camera_device camera;
    struct mutex lock;

    unsigned int vic_frd_c;     /* frame done cnt */
    unsigned int vic_fre_c;     /* frame err cnt */
    unsigned int vic_frov_c;    /* frame overflow cnt */

#ifdef SOC_CAMERA_DEBUG
    struct kobject dsysfs_parent_kobj;
    struct kobject dsysfs_kobj;
    struct completion snap_raw_comp;
#endif
};


static struct jz_vic_tiziano_data jz_vic_tiziano_dev[2] = {
    {
        .index                  = 0,
        .is_enable              = 0,
        .irq                    = IRQ_INTC_BASE + IRQ_VIC0, /* BASE + 19 */
        .irq_name               = "VIC0",
    },

    {
        .index                  = 1,
        .is_enable              = 0,
        .irq                    = IRQ_INTC_BASE + IRQ_VIC1, /* BASE + 18 */
        .irq_name               = "VIC1",
    },
};


static irqreturn_t vic_irq_isp_handler(int irq, void *data)
{
    struct jz_vic_tiziano_data *drv = (struct jz_vic_tiziano_data *)data;
    int index = drv->index;
    volatile unsigned long state, pending, mask;

    state = vic_read_reg(index, VIC_INT_STA);
    vic_write_reg(index, VIC_INT_CLR, state);

    mask = vic_read_reg(index, VIC_INT_MASK);
    pending = state & (~mask);

    if (get_bit_field(&pending, VIC_HVRES_ERR)) {
        vic_reset(index); /* 复位vic后，下帧重头开始取（防裂屏） */

        drv->vic_fre_c++;
        printk(KERN_ERR "## VIC WARN status = 0x%08lx\n", pending);
    }
    if (get_bit_field(&pending, VIC_FIFO_OVF)) {
        drv->vic_frov_c++;
    }
    if (get_bit_field(&pending, DMA_FRD)) {
#ifdef SOC_CAMERA_DEBUG
        complete(&drv->snap_raw_comp);
#endif
    }
    if (get_bit_field(&pending, VIC_FRD)) {
        drv->vic_frd_c++;
    }

    return IRQ_HANDLED;
}



int vic_tiziano_stream_on(int index, struct sensor_attr *attr)
{
    struct jz_vic_tiziano_data *drv = &jz_vic_tiziano_dev[index];

    int ret = vic_stream_on(index, attr);
    if (ret) {
        printk(KERN_ERR "vic%d(tiziano) : vic stream on failed\n", index);
        return ret;
    }

    enable_irq(drv->irq);

    return 0;
}


void vic_tiziano_stream_off(int index, struct sensor_attr *attr)
{
    struct jz_vic_tiziano_data *drv = &jz_vic_tiziano_dev[index];

    disable_irq(drv->irq);

    vic_stream_off(index, attr);
}

int vic_tiziano_power_on(int index)
{
    return vic_power_on(index);
}

void vic_tiziano_power_off(int index)
{
    vic_power_off(index);
}

#ifdef SOC_CAMERA_DEBUG
static ssize_t dsysfs_vic_tiziano_show_frame_cnt(struct device *dev, struct device_attribute *attr, char *buf)
{
    struct jz_vic_tiziano_data *drv = container_of((struct kobject *)dev, struct jz_vic_tiziano_data, dsysfs_kobj);
    char *p = buf;

    p += sprintf(p, "vic frame done: %u\n", drv->vic_frd_c);
    p += sprintf(p, "vic frame error: %u\n", drv->vic_fre_c);
    p += sprintf(p, "vic frame overflow: %u\n", drv->vic_frov_c);
    return p - buf;
}

static ssize_t dsysfs_vic_tiziano_dump_reg(struct device *dev, struct device_attribute *attr, char *buf)
{
    struct jz_vic_tiziano_data *drv = container_of((struct kobject *)dev, struct jz_vic_tiziano_data, dsysfs_kobj);
    int index = drv->index;
    char *p = buf;

    p += dsysfs_vic_dump_reg(index, p);

    return p - buf;
}

static ssize_t dsysfs_vic_tiziano_show_sensor_info(struct device *dev, struct device_attribute *attr, char *buf)
{
    struct jz_vic_tiziano_data *drv = container_of((struct kobject *)dev, struct jz_vic_tiziano_data, dsysfs_kobj);
    int index = drv->index;
    char *p = buf;

    p += dsysfs_vic_show_sensor_info(index, p);

    return p - buf;
}

static int dsysfs_vic_tiziano_snap_raw(struct jz_vic_tiziano_data *drv)
{
    struct sensor_attr *sensor = drv->camera.sensor;
    int index = drv->index;
    unsigned int image_width;
    unsigned int image_height;

    if ( (sensor->dbus_type == SENSOR_DATA_BUS_MIPI) && (sensor->mipi.mipi_crop.enable) ) {
        image_width = sensor->mipi.mipi_crop.output_width;
        image_height = sensor->mipi.mipi_crop.output_height;
    } else {
        image_width = sensor->sensor_info.width;
        image_height = sensor->sensor_info.height;
    }

    unsigned int lineoffset = image_width * 2;
    unsigned int imagesize = lineoffset * image_height;
    void *snap_vaddr;
    unsigned int snap_paddr;

    struct file *fd = NULL;
    mm_segment_t old_fs;
    loff_t *pos;
    int ret = 0;

    snap_vaddr = rmem_alloc_aligned(imagesize, PAGE_SIZE);
    if (NULL == snap_vaddr){
        printk(KERN_ERR "not enough mem space for vic dma~~~\n");
        return -1;
    }
    snap_paddr = virt_to_phys(snap_vaddr);
    if (snap_paddr){
        vic_set_bit(index, VIC_DMA_CONFIGURE, Dma_en, 0);

        vic_set_bit(index, VIC_CONTROL_DMA_ROUTE, VC_DMA_ROUTE_dma_out, 1);
        vic_write_reg(index, VIC_DMA_RESET, 0x01);
        vic_write_reg(index, VIC_DMA_RESOLUTION, image_width << 16 | image_height);
        vic_write_reg(index, VIC_DMA_Y_CH_LINE_STRIDE, lineoffset);
        vic_write_reg(index, VIC_DMA_Y_CH_BUF0_ADDR, snap_paddr);
        vic_write_reg(index, VIC_DMA_CONFIGURE, 0x1<<31);

        ret = wait_for_completion_interruptible_timeout(&drv->snap_raw_comp, msecs_to_jiffies(2000));
        vic_set_bit(index, VIC_DMA_CONFIGURE, Dma_en, 0);
        if (ret > 0)
            ret = 0;
        else {
            printk(KERN_ERR "snapraw timeout\n");
            ret = -1;
            goto exit;
        }

        /* save raw */
        fd = filp_open("/tmp/snap.raw", O_CREAT | O_WRONLY | O_TRUNC, 0766);
        if (fd < 0) {
            printk(KERN_ERR "Failed to open /tmp/snap.raw\n");
            ret = -1;
            goto exit;
        }
        old_fs = get_fs();
        set_fs(KERNEL_DS);
        pos = &(fd->f_pos);
        vfs_write(fd, snap_vaddr, imagesize, pos);
        filp_close(fd, NULL);
        set_fs(old_fs);
        printk(KERN_ERR "snapraw sucess\n");
    }

exit:
    if (snap_vaddr)
        rmem_free(snap_vaddr, imagesize);

    return ret;
}

static ssize_t dsysfs_vic_tiziano_ctrl(struct file *file, struct kobject *kobj, struct bin_attribute *attr, char *buf, loff_t pos, size_t count)
{
    struct jz_vic_tiziano_data *drv = container_of(kobj, struct jz_vic_tiziano_data, dsysfs_kobj);
    int index = drv->index;

    if (!strncmp(buf, "snapraw", sizeof("snapraw")-1)) {
        if(!vic_stream_state(index)) {
            printk(KERN_ERR "%s sensor doesn't work, please stream on\n", __func__);
            goto exit;
        }
        dsysfs_vic_tiziano_snap_raw(drv);

    } else {
        dsysfs_vic_ctrl(index, buf);
    }

exit:
    return count;
}


static DSYSFS_DEV_ATTR(show_frm_cnt, S_IRUGO|S_IWUSR, dsysfs_vic_tiziano_show_frame_cnt, NULL);
static DSYSFS_DEV_ATTR(dump_vic_reg, S_IRUGO|S_IWUSR, dsysfs_vic_tiziano_dump_reg, NULL);
static DSYSFS_DEV_ATTR(show_sensor_info, S_IRUGO|S_IWUSR, dsysfs_vic_tiziano_show_sensor_info, NULL);
static struct attribute *dsysfs_vic_dev_attrs[] = {
    &dsysfs_dev_attr_show_frm_cnt.attr,
    &dsysfs_dev_attr_dump_vic_reg.attr,
    &dsysfs_dev_attr_show_sensor_info.attr,
    NULL,
};

static DSYSFS_BIN_ATTR(ctrl, S_IRUGO|S_IWUSR, NULL, dsysfs_vic_tiziano_ctrl, 0);
static struct bin_attribute *dsysfs_vic_bin_attrs[] = {
    &dsysfs_bin_attr_ctrl,
    NULL,
};

static const struct attribute_group dsysfs_vic_attr_group = {
    .attrs  = dsysfs_vic_dev_attrs,
    .bin_attrs = dsysfs_vic_bin_attrs,
};

struct kobject *dsysfs_get_root_dir(int index)
{
    return &(jz_vic_tiziano_dev[index].dsysfs_parent_kobj);
}
#endif


#define error_if(_cond)                                                 \
    do {                                                                \
        if (_cond) {                                                    \
            printk(KERN_ERR "vic(tiziano): failed to check: %s\n", #_cond);\
            ret = -1;                                                   \
            goto unlock;                                                \
        }                                                               \
    } while (0)


/*
 * 格式信息转换 转换后的格式提供给ISP
 * sensor format : Sensor格式在sensor driver中根据setting指定
 * camera format : Camera格式在camera driver中使用,并暴露给应用
 *
 *    sensor格式                    Camera 格式
 * [成员0 sensor format]  <--->  [成员1 camera format]
 *
 */
struct fmt_pair {
    sensor_pixel_fmt sensor_fmt;
    camera_pixel_fmt camera_fmt;
};

static struct fmt_pair fmts[] = {
    {SENSOR_PIXEL_FMT_SBGGR8_1X8,       CAMERA_PIX_FMT_SBGGR8},
    {SENSOR_PIXEL_FMT_SGBRG8_1X8,       CAMERA_PIX_FMT_SGBRG8},
    {SENSOR_PIXEL_FMT_SGRBG8_1X8,       CAMERA_PIX_FMT_SGRBG8},
    {SENSOR_PIXEL_FMT_SRGGB8_1X8,       CAMERA_PIX_FMT_SRGGB8},
    {SENSOR_PIXEL_FMT_SBGGR10_1X10,     CAMERA_PIX_FMT_SBGGR10},
    {SENSOR_PIXEL_FMT_SGBRG10_1X10,     CAMERA_PIX_FMT_SGBRG10},
    {SENSOR_PIXEL_FMT_SGRBG10_1X10,     CAMERA_PIX_FMT_SGRBG10},
    {SENSOR_PIXEL_FMT_SRGGB10_1X10,     CAMERA_PIX_FMT_SRGGB10},
    {SENSOR_PIXEL_FMT_SBGGR12_1X12,     CAMERA_PIX_FMT_SBGGR12},
    {SENSOR_PIXEL_FMT_SGBRG12_1X12,     CAMERA_PIX_FMT_SGBRG12},
    {SENSOR_PIXEL_FMT_SGRBG12_1X12,     CAMERA_PIX_FMT_SGRBG12},
    {SENSOR_PIXEL_FMT_SRGGB12_1X12,     CAMERA_PIX_FMT_SRGGB12},
};

static int sensor_attribute_check_init(int index, struct sensor_attr *sensor)
{
    int ret = -EINVAL;

    error_if(!sensor->device_name);
    error_if(sensor->sensor_info.width < 128 || sensor->sensor_info.width > 2048);
    error_if(sensor->sensor_info.height < 128);
    error_if(!sensor->ops.power_on);
    error_if(!sensor->ops.power_off);
    error_if(!sensor->ops.stream_on);
    error_if(!sensor->ops.stream_off);

    memset(sensor->info.name, 0x00, sizeof(sensor->info.name));
    memcpy(sensor->info.name, sensor->device_name, strlen(sensor->device_name));

    if ( (sensor->dbus_type == SENSOR_DATA_BUS_MIPI) && (sensor->mipi.mipi_crop.enable) ) {
        sensor->info.width =  sensor->mipi.mipi_crop.output_width;
        sensor->info.height =  sensor->mipi.mipi_crop.output_height;
    } else {
        sensor->info.width =  sensor->sensor_info.width;
        sensor->info.height =  sensor->sensor_info.height;
    }

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

    sensor->info.data_fmt = fmts[i].camera_fmt;

    if (sensor->dbus_type == SENSOR_DATA_BUS_DVP) {
        switch (sensor->dvp.gpio_mode) {
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
            if (sensor->dvp.data_fmt < DVP_YUV422){
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

int vic_register_sensor_route_tiziano(int index, struct sensor_attr *sensor)
{
    assert(index < 2);

    struct jz_vic_tiziano_data *drv = &jz_vic_tiziano_dev[index];
    int ret = 0;

    assert(drv->is_finish > 0);
    assert(!drv->camera.sensor);

    ret = sensor_attribute_check_init(index, sensor);
    assert(ret == 0);

    mutex_lock(&drv->lock);

    ret = isp_component_bind_sensor(index, sensor);
    if (!ret)
        drv->camera.sensor = sensor;

    mutex_unlock(&drv->lock);

    return ret;
}

void vic_unregister_sensor_route_tiziano(int index, struct sensor_attr *sensor)
{
    assert(index < 2);

    struct jz_vic_tiziano_data *drv = &jz_vic_tiziano_dev[index];
    assert(drv->is_finish > 0);
    assert(drv->camera.sensor);
    assert(sensor == drv->camera.sensor);

    mutex_lock(&drv->lock);

    isp_component_unbind_sensor(index, sensor);

    drv->camera.sensor = NULL;

    mutex_unlock(&drv->lock);
}


int jz_vic_tiziano_drv_init(int index)
{
    assert(index < 2);

    struct jz_vic_tiziano_data *drv = &jz_vic_tiziano_dev[index];
    int ret;

    mutex_init(&drv->lock);

    ret = request_irq(drv->irq, vic_irq_isp_handler, 0, drv->irq_name, (void *)drv);
    if (ret) {
        printk(KERN_ERR "camera: vic%d(tiziano) failed to request irq\n", index);
        goto error_request_irq;
    }
    disable_irq(drv->irq);

#ifdef SOC_CAMERA_DEBUG
    char dsysfs_root_dir_name[16];

    sprintf(dsysfs_root_dir_name ,"isp%d", drv->index);
    ret = dsysfs_create_dir(&drv->dsysfs_parent_kobj, NULL, dsysfs_root_dir_name);
    if (ret) {
        printk(KERN_ERR "isp%d dsysfs create root dir fail\n", index);
        goto error_dsys_create_root;
    }

    ret = dsysfs_create_group(&drv->dsysfs_kobj, &drv->dsysfs_parent_kobj, "vic", &dsysfs_vic_attr_group);
    if (ret) {
        printk(KERN_ERR "isp%d dsysfs create sub dir vic fail\n", index);
        goto error_dsys_create_vic;
    }

    init_completion(&drv->snap_raw_comp);
#endif

    ret = jz_isp_drv_init(index);
    if (ret) {
        printk(KERN_ERR "camera: failed to init isp%d resources\n", index);
        goto error_isp_drv_init;
    }

    drv->is_finish = 1;

    printk(KERN_DEBUG "vic%d(tiziano) register successfully\n", index);

    return 0;

error_isp_drv_init:
#ifdef SOC_CAMERA_DEBUG
    dsysfs_remove_group(&drv->dsysfs_kobj, &dsysfs_vic_attr_group);
error_dsys_create_vic:
    dsysfs_remove_dir(&drv->dsysfs_parent_kobj);
error_dsys_create_root:
#endif
    free_irq(drv->irq, drv);

error_request_irq:
    return ret;
}

void jz_vic_tiziano_drv_deinit(int index)
{
    assert(index < 2);

    struct jz_vic_tiziano_data *drv = &jz_vic_tiziano_dev[index];

    if (!drv->is_finish)
        return ;

    drv->is_finish = 0;

    free_irq(drv->irq, drv);

    jz_isp_drv_deinit(index);

#ifdef SOC_CAMERA_DEBUG
    dsysfs_remove_group(&drv->dsysfs_kobj, &dsysfs_vic_attr_group);
    dsysfs_remove_dir(&drv->dsysfs_parent_kobj);
#endif
}

