#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/delay.h>
#include <linux/gpio.h>
#include <linux/miscdevice.h>
#include <linux/clk.h>
#include <linux/slab.h>
#include <linux/irq.h>
#include <linux/interrupt.h>
#include <linux/fb.h>
#include <linux/dma-mapping.h>
#include <linux/platform_device.h>
#include <linux/mutex.h>
#include <linux/wait.h>
#include <linux/spinlock.h>
#include <linux/mm.h>
#include <linux/list.h>
#include <linux/string.h>
#include <linux/delay.h>
#include <linux/vmalloc.h>
#include <linux/sched.h>
#include <linux/ioctl.h>
#include <common.h>
#include <soc/base.h>

#include "fb_layer_mixer.h"
#include "dpu_hal.h"

#define CMD_SET_INPUT_LAYER              _IOWR('M', 82, struct fb_layer_mixer_layer *)
#define CMD_SET_OUTPUT_FRAME              _IOWR('M', 83, struct fb_layer_mixer_output_cfg *)
#define CMD_WORK_OUT                  _IO('M', 84)
#define CMD_DELETE_MIXER              _IO('M', 85)


#define State_writeback_start 0
#define State_writeback_end   1

struct lcdc_frame {
    struct framedesc *framedesc;
    struct layerdesc *layers[4];
};

struct fb_layer_mixer_layer {
    struct lcdc_layer cfg;
    int layer_id;
};

struct fb_layer_mixer_info {
    int xres;
    int yres;
    unsigned int bytes_per_line;
    unsigned int bytes_per_frame;
    enum fb_fmt format;
};

struct fb_layer_mixer_dev {
    struct device *dev;
    struct lcdc_frame frame;
    struct lcdc_layer layer_cfg[4];
    struct fb_layer_mixer_output_cfg mixer_cfg;
    struct fb_layer_mixer_info info;
    int set_input_count;
};

struct fb_layer_mixer_platform_data {
    struct device *dev;
    struct miscdevice misc_dev;
};

struct {
    struct clk *clk;
    int frame_state;
    int is_enable;
    int irq;
    int fb_is_srdma;
} layer_mixer;


module_param_named(fb_is_srdma, layer_mixer.fb_is_srdma, int, 0644);

static DEFINE_MUTEX(lock);
static DECLARE_WAIT_QUEUE_HEAD(waiter);


static inline int bytes_per_line(int xres, enum fb_fmt fmt)
{
    int len;
    int bytes;

    if(fmt == fb_fmt_ARGB8888 || fmt == fb_fmt_RGB888)
        bytes = 4;
    else
        bytes = 2;

    len = xres * bytes;
    return ALIGN(len, 8);
}

static inline int bytes_per_frame(int line_len, int yres)
{
    unsigned int frame_size = line_len * yres;

    return frame_size;
}

static inline void *m_dma_alloc_coherent(struct device *dev, int size)
{
    dma_addr_t dma_handle;
    void *mem = dma_alloc_coherent(dev, size, &dma_handle, GFP_KERNEL);
    assert(mem);

    return (void *)CKSEG0ADDR(mem);
}

static inline void m_dma_free_coherent(struct device *dev, void *mem, int size)
{
    dma_addr_t dma_handle = virt_to_phys(mem);
    dma_free_coherent(dev, size, (void *)CKSEG1ADDR(mem), dma_handle);
}

static inline void m_cache_sync_write(void *mem, int size)
{
    dma_cache_wback((unsigned long)mem, size);
}

static inline void m_cache_sync_read(void *mem, int size)
{
    dma_cache_inv((unsigned long)mem, size);
}

static void init_lcdc(void)
{
    unsigned long intc = dpu_read(INTC);
    set_bit_field(&intc, EOW_MSK, 1);
    dpu_write(INTC, intc);

    dpu_write(CLR_ST, dpu_read(INT_FLAG));

    unsigned long com_cfg = dpu_read(COM_CFG);
    set_bit_field(&com_cfg, BURST_LEN_BDMA, 3);
    set_bit_field(&com_cfg, BURST_LEN_RDMA, 3);
    set_bit_field(&com_cfg, CH_SEL, 0);
    dpu_write(COM_CFG, com_cfg);
}

static int wb_format(enum fb_fmt format)
{
    switch (format)
    {
        case fb_fmt_RGB565: return 1;
        case fb_fmt_RGB555: return 2;
        case fb_fmt_RGB888: return 6;
        case fb_fmt_ARGB8888: return 0;
        default:
            panic("writeback not support this format:%d\n", format);
    }
}

static void init_writeback_desc(struct fb_layer_mixer_dev *dev)
{

    struct lcdc_frame *frame = &dev->frame;
    struct framedesc *desc = frame->framedesc;

    desc->FrameCfgAddr = virt_to_phys(desc);

    desc->FrameCtrl = 0;
    set_bit_field(&desc->FrameCtrl, f_DirectEn, 0);
    set_bit_field(&desc->FrameCtrl, f_WriteBack, 1);
    set_bit_field(&desc->FrameCtrl, f_Change2RDMA, 0);
    set_bit_field(&desc->FrameCtrl, f_stop, 1);
    set_bit_field(&desc->FrameCtrl, f_WB_DitherAuto, 1);
    set_bit_field(&desc->FrameCtrl, f_WB_DitherEn, 0);

    desc->Layer0CfgAddr = virt_to_phys(frame->layers[0]);
    desc->Layer1CfgAddr = virt_to_phys(frame->layers[1]);
    desc->Layer2CfgAddr = virt_to_phys(frame->layers[2]);
    desc->Layer3CfgAddr = virt_to_phys(frame->layers[3]);

    set_bit_field(&desc->LayerCfgScaleEn, f_layer0order, 0);
    set_bit_field(&desc->LayerCfgScaleEn, f_layer1order, 1);
    set_bit_field(&desc->LayerCfgScaleEn, f_layer2order, 2);

    set_bit_field(&desc->LayerCfgScaleEn, f_layer3order, 3);

    desc->InterruptControl = 0;
    set_bit_field(&desc->InterruptControl, f_EOW_MSK, 1);

    m_cache_sync_write(frame->framedesc, sizeof(frame->framedesc));
}

static void config_writeback_desc(struct fb_layer_mixer_dev *dev)
{
    struct lcdc_frame *frame = &dev->frame;

    struct framedesc *desc = frame->framedesc;

    set_bit_field(&desc->FrameSize, f_Width, dev->mixer_cfg.xres);
    set_bit_field(&desc->FrameSize, f_Height, dev->mixer_cfg.yres);

    set_bit_field(&desc->FrameCtrl, f_WB_Format, wb_format(dev->mixer_cfg.format));

    desc->WritebackBufferAddr = virt_to_phys(dev->mixer_cfg.dst_mem);
    desc->WritebackStride = dev->mixer_cfg.xres;

    m_cache_sync_write(frame->framedesc, sizeof(frame->framedesc));
}

static void fb_layer_mixer_enable(void)
{
    if (layer_mixer.is_enable++ != 0)
        return;

    clk_prepare_enable(layer_mixer.clk);

    init_lcdc();
}

static void fb_layer_mixer_disable(void)
{
    if (--layer_mixer.is_enable != 0)
        return;

    layer_mixer.is_enable = 0;

    clk_disable(layer_mixer.clk);
}

static irqreturn_t fb_layer_mixer_irq_handler(int irq, void *data)
{
    unsigned long flags = dpu_read(INT_FLAG);

    if (get_bit_field(&flags, WDMA_END)) {
        layer_mixer.frame_state = State_writeback_end;
        dpu_write(CLR_ST, bit_field_val(CLR_WDMA_END, 1));
        wake_up_all(&waiter);
        return IRQ_HANDLED;
    }

    return IRQ_HANDLED;
}

void fb_layer_mixer_set_output_frame(struct fb_layer_mixer_dev *mixer, struct fb_layer_mixer_output_cfg *mixer_cfg)
{
    assert(mixer);
    assert(mixer_cfg);

    mutex_lock(&lock);

    mixer->mixer_cfg = *mixer_cfg;

    mixer->info.xres = mixer->mixer_cfg.xres;
    mixer->info.yres = mixer->mixer_cfg.yres;
    mixer->info.format = mixer->mixer_cfg.format;

    mixer->info.bytes_per_line = bytes_per_line(mixer->mixer_cfg.xres, mixer->mixer_cfg.format);
    mixer->info.bytes_per_frame = ALIGN(bytes_per_frame(mixer->info.bytes_per_line, mixer->info.yres), PAGE_SIZE);

    config_writeback_desc(mixer);

    mutex_unlock(&lock);
}

struct fb_layer_mixer_dev *fb_layer_mixer_create(struct device *dev)
{
    if (!layer_mixer.fb_is_srdma) {
        printk(KERN_ERR "fb is composer mode, can not use fb_layer_mixer\n");
        return NULL;
    }

    mutex_lock(&lock);

    int i = 0;

    struct fb_layer_mixer_dev *mixer = kmalloc(sizeof(struct fb_layer_mixer_dev), GFP_KERNEL);
    mixer->dev = dev;
    mixer->frame.framedesc = m_dma_alloc_coherent(dev, sizeof(struct framedesc));

    for(i = 0; i < 4; i++) {
        mixer->frame.layers[i] = m_dma_alloc_coherent(dev, sizeof(struct layerdesc));
        memset(mixer->frame.layers[i], 0, sizeof(struct layerdesc));
    }

    init_writeback_desc(mixer);

    fb_layer_mixer_enable();

    mutex_unlock(&lock);

    return mixer;
}


void fb_layer_mixer_set_input_layer(struct fb_layer_mixer_dev *mixer, int layer_id, struct lcdc_layer *cfg)
{
    assert(layer_id < 4 && layer_id >= 0);
    assert(mixer);

    mutex_lock(&lock);
    mixer->set_input_count++;

    struct lcdc_frame *frame = &mixer->frame;
    struct layerdesc *layer = frame->layers[layer_id];
    struct lcdc_layer *layer_cfg = &mixer->layer_cfg[layer_id];

    *layer_cfg = *cfg;

    dpu_init_layer_desc(layer, layer_cfg);

    dpu_enable_layer(frame->framedesc, layer_id, !!layer_cfg->layer_enable);
    dpu_enable_layer_scaling(frame->framedesc, layer_id, !!layer_cfg->scaling.enable);
    dpu_set_layer_order(frame->framedesc, layer_id, layer_cfg->layer_order);

    m_cache_sync_write(layer, sizeof(*layer));
    m_cache_sync_write(frame->framedesc, sizeof(*frame->framedesc));

    mutex_unlock(&lock);
}


void fb_layer_mixer_enable_layer(struct fb_layer_mixer_dev *mixer, unsigned int layer_id, int enable)
{
    assert(layer_id < 4 && layer_id >= 0);
    assert(mixer);

    struct lcdc_frame *frame = &mixer->frame;

    mutex_lock(&lock);

    dpu_enable_layer(frame->framedesc, layer_id, !!enable);
    m_cache_sync_write(frame->framedesc, sizeof(*frame->framedesc));

    mutex_unlock(&lock);
}

void fb_layer_mixer_work_out_one_frame(struct fb_layer_mixer_dev *mixer)
{
    assert(mixer);

    mutex_lock(&lock);

    if (!mixer->set_input_count) {
        printk(KERN_ERR "must set fb layer mixer input layer\n");
        mutex_unlock(&lock);
        return;
    }

    struct lcdc_frame *frame = &mixer->frame;

    fast_iob();

    dpu_write(FRM_CFG_ADDR, virt_to_phys(frame->framedesc));

    layer_mixer.frame_state = State_writeback_start;
    dpu_start_composer();

    wait_event_timeout(waiter, layer_mixer.frame_state == State_writeback_end, 300);
    if (layer_mixer.frame_state != State_writeback_end)
        printk(KERN_ERR "frame write back timeout %d\n", layer_mixer.frame_state);

    mutex_unlock(&lock);
}

void fb_layer_mixer_delete(struct fb_layer_mixer_dev *mixer)
{
    assert(mixer);

    int i;
    mutex_lock(&lock);

    fb_layer_mixer_disable();

    for(i = 0; i < 4; i++)
        m_dma_free_coherent(mixer->dev, mixer->frame.layers[i], sizeof(struct layerdesc));

    m_dma_free_coherent(mixer->dev, mixer->frame.framedesc, sizeof(struct framedesc));

    kfree(mixer);

    mutex_unlock(&lock);
}


static int fb_layer_mixer_open(struct inode *inode, struct file *file)
{
    struct miscdevice *dev = file->private_data;
    struct fb_layer_mixer_platform_data *platform_data = container_of(dev, struct fb_layer_mixer_platform_data, misc_dev);

    struct fb_layer_mixer_dev *mixer = fb_layer_mixer_create(platform_data->dev);
    if(!mixer)
        return -1;

    file->private_data = mixer;

    return 0;
}

static int fb_layer_mixer_release(struct inode *inode, struct file *file)
{
    struct fb_layer_mixer_dev *mixer = (struct fb_layer_mixer_dev *)file->private_data;

    fb_layer_mixer_delete(mixer);

    return 0;
}

static long fb_layer_mixer_ioctl(struct file *file, unsigned int cmd, unsigned long arg)
{
    struct fb_layer_mixer_dev *mixer = (struct fb_layer_mixer_dev *)file->private_data;
    struct fb_layer_mixer_output_cfg mixer_cfg;
    struct fb_layer_mixer_layer *layer;

    switch (cmd) {
    case CMD_SET_OUTPUT_FRAME :
        if (copy_from_user(&mixer_cfg, (struct fb_layer_mixer_output_cfg *)arg, sizeof(struct fb_layer_mixer_output_cfg)))
            return -EINVAL;

        if (mixer_cfg.dst_mem >= (void *)(512*1024*1024) || !mixer_cfg.dst_mem) {
            printk(KERN_ERR "fb_layer_mixer: dst_mem must be phyaddr : %p\n", mixer_cfg.dst_mem);
            return -EINVAL;
        }

        if (mixer_cfg.format > fb_fmt_ARGB8888) {
            printk(KERN_ERR "fb_layer_mixer: invalid fmt: %d\n", mixer_cfg.format);
            return -EINVAL;
        }

        if (!mixer_cfg.dst_mem_virtual) {
            printk(KERN_ERR "fb_layer_mixer:pls set dst_mem_virtual\n");
            return -EINVAL;
        }

        mixer_cfg.dst_mem = (void *)CKSEG0ADDR(mixer_cfg.dst_mem);

        fb_layer_mixer_set_output_frame(mixer, &mixer_cfg);
        break;

    case CMD_SET_INPUT_LAYER :
        layer = (struct fb_layer_mixer_layer *)arg;
        if (!layer)
            return -EINVAL;

        struct lcdc_layer *user_cfg = &layer->cfg;

        if (user_cfg->layer_order > lcdc_layer_3) {
            printk(KERN_ERR "fb_layer_mixer: invalid order: %d\n", user_cfg->layer_order);
            return -EINVAL;
        }
        if (user_cfg->alpha.value >= 256) {
            printk(KERN_ERR "fb_layer_mixer: invalid alpha: %x\n", user_cfg->alpha.value);
            return -EINVAL;
        }
        if (user_cfg->fb_fmt > fb_fmt_NV21) {
            printk(KERN_ERR "fb_layer_mixer: invalid fmt: %d\n", user_cfg->fb_fmt);
            return -EINVAL;
        }
        if (user_cfg->fb_fmt < fb_fmt_NV12) {
            if (user_cfg->rgb.mem >= (void *)(512*1024*1024)) {
                printk(KERN_ERR "fb_layer_mixer: must be phys address, %p\n", user_cfg->rgb.mem);
                return -EINVAL;
            }
        } else {
            if (user_cfg->y.mem >= (void *)(512*1024*1024)) {
                printk(KERN_ERR "fb_layer_mixer: must be phys address, %p\n", user_cfg->y.mem);
                return -EINVAL;
            }
            if (user_cfg->uv.mem >= (void *)(512*1024*1024)) {
                printk(KERN_ERR "fb_layer_mixer: must be phys address, %p\n", user_cfg->uv.mem);
                return -EINVAL;
            }
        }

        if (!user_cfg->scaling.enable) {
            if (user_cfg->xres + user_cfg->xpos > mixer->mixer_cfg.xres) {
                printk(KERN_ERR "fb_layer_mixer: invalid xres: %d %d\n", user_cfg->xres, user_cfg->xpos);
                return -EINVAL;
            }
            if (user_cfg->yres + user_cfg->ypos > mixer->mixer_cfg.yres) {
                printk(KERN_ERR "fb_layer_mixer: invalid yres: %d %d\n", user_cfg->yres, user_cfg->ypos);
                return -EINVAL;
            }
        } else {
            if (user_cfg->scaling.xres + user_cfg->xpos > mixer->mixer_cfg.xres) {
                printk(KERN_ERR "fb_layer_mixer: invalid scaling xres: %d %d\n", user_cfg->scaling.xres, user_cfg->xpos);
                return -EINVAL;
            }
            if (user_cfg->scaling.yres + user_cfg->ypos > mixer->mixer_cfg.yres) {
                printk(KERN_ERR "fb_layer_mixer: invalid scaling yres: %d %d\n", user_cfg->scaling.yres, user_cfg->ypos);
                return -EINVAL;
            }
        }

        mutex_lock(&lock);

        struct lcdc_layer *cfg = &mixer->layer_cfg[layer->layer_id];
        *cfg = *user_cfg;

        if (cfg->fb_fmt < fb_fmt_NV12) {
            cfg->rgb.mem = (void *)CKSEG0ADDR(cfg->rgb.mem);
        } else {
            cfg->y.mem = (void *)CKSEG0ADDR(cfg->y.mem);
            cfg->uv.mem = (void *)CKSEG0ADDR(cfg->uv.mem);
        }

        mutex_unlock(&lock);

        fb_layer_mixer_set_input_layer(mixer, layer->layer_id, cfg);
        break;

    case CMD_WORK_OUT :
        fb_layer_mixer_work_out_one_frame(mixer);
        m_cache_sync_read(mixer->mixer_cfg.dst_mem_virtual, mixer->info.bytes_per_frame);
        break;

    case CMD_DELETE_MIXER:
        fb_layer_mixer_delete(mixer);
        break;

    default:
        printk(KERN_ERR "FB_LAYER_MIXER: not support this cmd: %d\n", cmd);
        return -1;
    }

    return 0;
}

static struct file_operations fb_layer_mixer_fops = {
    .open = fb_layer_mixer_open,
    .release = fb_layer_mixer_release,
    .unlocked_ioctl = fb_layer_mixer_ioctl,
};

static struct miscdevice fb_layer_mixer_mdev = {
    .minor = MISC_DYNAMIC_MINOR,
    .name = "fb_layer_mixer",
    .fops = &fb_layer_mixer_fops,
};

static int fb_layer_mixer_probe(struct platform_device *pdev)
{
    int ret;
    struct fb_layer_mixer_platform_data *platform_data;
    layer_mixer.clk = clk_get(NULL, "gate_lcd");
    assert(layer_mixer.clk);

    layer_mixer.irq = IRQ_INTC_BASE + 31;

    platform_data = (struct fb_layer_mixer_platform_data *)kzalloc(sizeof(*platform_data), GFP_KERNEL);
    if (!platform_data) {
        dev_err(&pdev->dev, "alloc fb_layer_mixer_platform_data failed!\n");
        return -ENOMEM;
    }

    platform_data->dev = &pdev->dev;
    platform_data->misc_dev.minor = MISC_DYNAMIC_MINOR;
    platform_data->misc_dev.name = "fb_layer_mixer";
    platform_data->misc_dev.fops = &fb_layer_mixer_fops;

    ret = request_irq(layer_mixer.irq, fb_layer_mixer_irq_handler, IRQF_SHARED, "fb_layer_mixer", &layer_mixer);
    if (ret < 0)
        goto irq_err;

    ret = misc_register(&platform_data->misc_dev);
    if (ret < 0)
        goto misc_err;

    return 0;

misc_err:
    disable_irq(layer_mixer.irq);
    free_irq(layer_mixer.irq, &layer_mixer);

irq_err:
    clk_put(layer_mixer.clk);

    return ret;
}

static int fb_layer_mixer_remove(struct platform_device *pdev)
{
    disable_irq(layer_mixer.irq);
    free_irq(layer_mixer.irq, &layer_mixer);

    clk_put(layer_mixer.clk);
    misc_deregister(&fb_layer_mixer_mdev);

    return 0;
}

static struct platform_driver fb_layer_mixer_driver = {
    .probe = fb_layer_mixer_probe,
    .remove = fb_layer_mixer_remove,
    .driver = {
        .owner = THIS_MODULE,
        .name = "fb-layer-mixer",
    },
};

/* stop no dev release warning */
static void fb_layer_mixer_dev_release(struct device *dev){}

struct platform_device fb_layer_mixer_device = {
    .name = "fb-layer-mixer",
    .dev  = {
        .release = fb_layer_mixer_dev_release,
    },
};

static int __init fb_layer_mixer_init(void)
{
    int ret = platform_device_register(&fb_layer_mixer_device);
    if (ret)
        return ret;

    return platform_driver_register(&fb_layer_mixer_driver);
}
module_init(fb_layer_mixer_init);

static void __exit fb_layer_mixer_module_exit(void)
{
    platform_device_unregister(&fb_layer_mixer_device);

    platform_driver_unregister(&fb_layer_mixer_driver);
}
module_exit(fb_layer_mixer_module_exit);

EXPORT_SYMBOL_GPL(fb_layer_mixer_create);
EXPORT_SYMBOL_GPL(fb_layer_mixer_set_output_frame);
EXPORT_SYMBOL_GPL(fb_layer_mixer_set_input_layer);
EXPORT_SYMBOL_GPL(fb_layer_mixer_work_out_one_frame);
EXPORT_SYMBOL_GPL(fb_layer_mixer_delete);

MODULE_DESCRIPTION("Ingenic Soc FB layer mixer driver");
MODULE_LICENSE("GPL");
