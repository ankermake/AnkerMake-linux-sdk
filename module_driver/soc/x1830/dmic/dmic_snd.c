#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/platform_device.h>
#include <linux/moduleparam.h>

#include "dmic_snd.h"
#include "dmic_hal.c"
#include "dmic_dma.c"

#define PERIOD_BYTES_MIN 1024
#define PERIODS_MIN 3
#define PERIODS_MAX 128
#define MAX_DMA_BUFFERSIZE (128*1024)

#define DMIC_BUSY 1
#define DMIC_IDLE 0

#define MAX_CAPTURE_VOLUME 0x1f

#define DMIC_GPIO_CLK  GPIO_PC(8)
#define DMIC_GPIO_DAT0 GPIO_PC(25)
#define DMIC_GPIO_DAT1 GPIO_PC(26)

static struct clk *clk;
static struct dmic_data dmic;
static int gpio_state = 0;
static int dmic_gain = 13;

static const struct snd_pcm_hardware dmic_pcm_hardware = {
    .info   = SNDRV_PCM_INFO_INTERLEAVED |
            SNDRV_PCM_INFO_MMAP |
            SNDRV_PCM_INFO_MMAP_VALID |
            SNDRV_PCM_INFO_BATCH,
    .formats = SNDRV_PCM_FMTBIT_S16_LE,
    .channels_min   = 1,
    .channels_max   = 4,
    .buffer_bytes_max = MAX_DMA_BUFFERSIZE,
    .period_bytes_min   = PERIOD_BYTES_MIN,
    .period_bytes_max   = MAX_DMA_BUFFERSIZE,
    .periods_min        = PERIODS_MIN,
    .periods_max        = PERIODS_MAX,
    .fifo_size      = 0,
};

static inline int init_gpio(int gpio, const char *name)
{
    int ret;

    ret = gpio_request(gpio, name);
    if (ret < 0) {
        char buf[10];
        gpio_to_str(gpio, buf);
        printk(KERN_ERR "DMIC: %s gpio %s request failed!\n", name, buf);
        return ret;
    }

    gpio_set_func(gpio, GPIO_FUNC_2);

    return 0;
}

static int dmic_gpio_request(int channels)
{
    int ret = 0;

    if (!(gpio_state & BIT(0))) {
        ret = init_gpio(DMIC_GPIO_CLK, "dmic-clk");
        if (ret < 0)
            return ret;
        gpio_state |= BIT(0);
    }

    if (!(gpio_state & BIT(1))) {
        ret = init_gpio(DMIC_GPIO_DAT0, "dmic-dat0");
        if (ret < 0)
            goto err_gpio_dat0;

        gpio_state |= BIT(1);
    }

    if (channels > 2 && !(gpio_state & BIT(2))) {
        ret = init_gpio(DMIC_GPIO_DAT1, "dmic-dat1");
        if (ret < 0)
            goto err_gpio_dat1;
        gpio_state |= BIT(2);
    }

    return ret;
err_gpio_dat1:
    if (gpio_state & BIT(1))
        gpio_free(DMIC_GPIO_DAT0);
err_gpio_dat0:
    gpio_free(DMIC_GPIO_CLK);

    gpio_state = 0;

    return ret;
}

static void dmic_gpio_release(void)
{
    if (gpio_state & BIT(0))
        gpio_free(DMIC_GPIO_CLK);

    if (gpio_state & BIT(1))
        gpio_free(DMIC_GPIO_DAT0);

    if (gpio_state & BIT(2))
        gpio_free(DMIC_GPIO_DAT1);

    gpio_state = 0;
}

static unsigned int dmic_get_readable_size(void)
{
    struct dma_param *dma = dmic.dma;
    struct dma_chan *dma_chan = dma->dma_chan;
    unsigned int buffer_size = dmic.buffer_size;
    int pdmic_addr_start = virt_to_phys(dmic.buffer);

    dma_addr_t dmic_dst = get_dma_addr(dma_chan, pdmic_addr_start, dmic.buffer_size);
    unsigned int pos = dmic_dst - pdmic_addr_start;
    unsigned int size = sub_pos(buffer_size, pos, dmic.dma_pos);
    unsigned int unit_size = dmic.unit_size;

    dmic.dma_pos = pos;
    dmic.data_size += size;

    if (dmic.data_size > (buffer_size - unit_size)) {
        unsigned int align_pos = ALIGN(pos, unit_size);
        dmic.read_pos = add_pos(buffer_size, align_pos, unit_size);
        dmic.data_size = buffer_size - sub_pos(buffer_size, dmic.read_pos, pos);
    }

    return dmic.data_size > unit_size ? dmic.data_size - unit_size : 0;
}

static void dmic_do_read_buffer(void *mem, unsigned int bytes)
{
    unsigned int buffer_size = dmic.buffer_size;
    unsigned int pos = dmic.read_pos;
    void *src = dmic.buffer;

    if (pos + bytes <= buffer_size) {
        dma_cache_sync(NULL, src+pos, bytes, DMA_DEV_TO_MEM);
        memcpy(mem, src+pos, bytes);
    } else {
        unsigned int size1 = buffer_size - pos;
        dma_cache_sync(NULL, src+pos, size1, DMA_DEV_TO_MEM);
        memcpy(mem, src+pos, size1);
        dma_cache_sync(NULL, (src), bytes-size1, DMA_DEV_TO_MEM);
        memcpy(mem+size1, src, bytes-size1);
    }

    dmic.read_pos = add_pos(buffer_size, pos, bytes);
    dmic.data_size -= bytes;
}

static void dmic_wait_event(unsigned long *flags)
{
    spin_unlock_irqrestore(&dmic.lock, *flags);
    wait_event(dmic.wait_queue, !dmic.working_flag);
    spin_lock_irqsave(&dmic.lock, *flags);
}

static void dmic_read_frame(struct dmic_data *dmic, void *mem, int frames)
{
    int bytes = frames * dmic->frame_size;
    unsigned int len = 0;
    unsigned long flags;

    spin_lock_irqsave(&dmic->lock, flags);

    while (bytes) {
        if (!dmic_get_readable_size()) {
            dmic->working_flag = 1;
            dmic_wait_event(&flags);
        }

        unsigned int n = dmic_get_readable_size();
        if (!n) {
            if (!len) {
                printk(KERN_ERR "DMIC: dmic_read_frame timeout\n");
            }
        }

        if (n > bytes)
            n = bytes;
        if (n > 512)
            n = 512;

        dmic_do_read_buffer(mem, n);
        len += n;
        mem += n;
        bytes -= n;

        spin_unlock_irqrestore(&dmic->lock, flags);
        spin_lock_irqsave(&dmic->lock, flags);

    }

    spin_unlock_irqrestore(&dmic->lock, flags);
}

static enum hrtimer_restart dmic_hrtimer_callback(struct hrtimer *hrtimer)
{
    unsigned int n;
    unsigned long flags;

    hrtimer_forward(hrtimer, hrtimer->_softexpires, ns_to_ktime(dmic.period_time));

    spin_lock_irqsave(&dmic.lock, flags);

    n = dmic_get_readable_size();
    if (n) {
        snd_pcm_period_elapsed(dmic.substream);
        dmic.working_flag = 0;
        wake_up(&dmic.wait_queue);
    }

    spin_unlock_irqrestore(&dmic.lock, flags);

    return HRTIMER_RESTART;
}

static void dmic_hrtimer_start(struct dmic_data *dmic)
{
    hrtimer_forward_now(&dmic->hrtimer,
            ns_to_ktime(dmic->period_time));
    hrtimer_start_expires(&dmic->hrtimer, HRTIMER_MODE_ABS);
}

static void dmic_hrtimer_stop(struct dmic_data *dmic)
{
    hrtimer_cancel(&dmic->hrtimer);
}

static void dmic_hrtimer_init(struct dmic_data *dmic)
{
    hrtimer_init(&dmic->hrtimer, CLOCK_MONOTONIC, HRTIMER_MODE_REL);
    dmic->hrtimer.function = dmic_hrtimer_callback;
}

static void dmic_pcm_capture_start(void)
{
    clk_enable(clk);

    dmic_enable(dmic.channels, dmic.rate, dmic_gain);

    dmic_start();

    dmic_dma_submit_cyclic(&dmic);

    dmic_hrtimer_start(&dmic);

    dmic.is_enable = DMIC_BUSY;
}

static void dmic_pcm_capture_stop(void)
{
    dmic_hrtimer_stop(&dmic);

    dmic_dma_terminate(&dmic);

    dmic_disable();

    clk_disable(clk);

    dmic.is_enable = DMIC_IDLE;
}

static int dmic_info_volume(struct snd_kcontrol *kctrl,
            struct snd_ctl_elem_info *uinfo)
{
    uinfo->type = SNDRV_CTL_ELEM_TYPE_INTEGER;
    uinfo->count = 1;
    uinfo->value.integer.min = 0;
    uinfo->value.integer.max = MAX_CAPTURE_VOLUME;

    return 0;
}

static int dmic_get_volume(struct snd_kcontrol *kctrl,
            struct snd_ctl_elem_value *uctl)
{
    unsigned long flags;

    spin_lock_irqsave(&dmic.lock, flags);

    if (dmic.is_enable)
        uctl->value.integer.value[0] = dmic_capture_get_volume();
    else
        uctl->value.integer.value[0] = dmic_gain;

    spin_unlock_irqrestore(&dmic.lock, flags);

    return 0;
}

static int dmic_put_volume(struct snd_kcontrol *kctrl,
            struct snd_ctl_elem_value *uctl)
{
    int vol;
    unsigned long flags;

    spin_lock_irqsave(&dmic.lock, flags);

    vol = uctl->value.integer.value[0];
    if (vol < 0)
        vol = 0;
    if (vol > MAX_CAPTURE_VOLUME)
        vol = MAX_CAPTURE_VOLUME;

    if (dmic.is_enable)
        dmic_capture_set_volume(vol);

    dmic_gain = vol;

    spin_unlock_irqrestore(&dmic.lock, flags);

    return 0;
}
static struct snd_kcontrol_new capture_controls = {
    .iface  = SNDRV_CTL_ELEM_IFACE_MIXER,
    .name   = "Master Capture Volume",
    .index  = 0,
    .access = SNDRV_CTL_ELEM_ACCESS_READWRITE,
    .info   = dmic_info_volume,
    .get    = dmic_get_volume,
    .put    = dmic_put_volume,
};

static int dmic_pcm_open(struct snd_pcm_substream *substream)
{
    struct snd_pcm_runtime *runtime = substream->runtime;

    snd_soc_set_runtime_hwparams(substream, &dmic_pcm_hardware);

    runtime->hw.rate_min = 8000;
    runtime->hw.rate_max = 48000;
    runtime->hw.formats = SNDRV_PCM_FMTBIT_S16_LE;
    runtime->hw.channels_min = 1;
    runtime->hw.channels_max = 4;

    snd_pcm_hw_constraint_integer(runtime, SNDRV_PCM_HW_PARAM_PERIODS);

    return 0;
}

static int dmic_pcm_close(struct snd_pcm_substream *substream)
{
    return 0;
}

static int dmic_pcm_hw_params(struct snd_pcm_substream *substream,
                struct snd_pcm_hw_params *hw_params)
{
    int ret, channels, period_ms;

    struct dmic_data *dmic = snd_pcm_substream_chip(substream);
    ret = snd_pcm_lib_malloc_pages(substream, params_buffer_bytes(hw_params));
    if (ret < 0)
        return ret;

    dmic->rate = params_rate(hw_params);
    if ((dmic->rate != 8000) && (dmic->rate != 16000) && (dmic->rate != 48000))
        return -EINVAL;
    channels = params_channels(hw_params);
    if ((channels != 1) && (channels != 2) && (channels != 4))
        return -EINVAL;

    ret = dmic_gpio_request(channels);
    if (ret < 0)
        return ret;

    dmic->channels = channels;
    dmic->buffer_size = params_buffer_bytes(hw_params);
    if (dmic->buffer_size % 128) {
        printk(KERN_ERR "DMIC: capture buffer size no align 128!\n");
        return -EINVAL;
    }

    dmic->period_size = params_period_size(hw_params);
    dmic->frame_size = snd_pcm_format_physical_width(params_format(hw_params)) * channels / 8;/* 帧＝采样位数*通道 */
    dmic->buffer = (unsigned long *)CKSEG0ADDR(substream->dma_buffer.area);

    period_ms = 1000 * dmic->period_size / dmic->rate / dmic->frame_size;
    dmic->period_time = period_ms * 1000 * 1000;

    dmic->dma_pos = 0;
    dmic->read_pos = 0;
    dmic->data_size = 0;
    dmic->unit_size = 128;

    return 0;
}

static int dmic_pcm_hw_free(struct snd_pcm_substream *substream)
{
    return snd_pcm_lib_free_pages(substream);
}

static int dmic_pcm_trigger(struct snd_pcm_substream *substream, int cmd)
{
    int ret = 0;
    unsigned long flags;

    spin_lock_irqsave(&dmic.lock, flags);
    switch (cmd) {
    case SNDRV_PCM_TRIGGER_START:
    case SNDRV_PCM_TRIGGER_RESUME:
        dmic.substream = substream;
        dmic_pcm_capture_start();
        break;
    case SNDRV_PCM_TRIGGER_STOP:
    case SNDRV_PCM_TRIGGER_SUSPEND:
        dmic.substream = NULL;
        dmic_pcm_capture_stop();
        break;
    default:
        ret = -EINVAL;
    }
    spin_unlock_irqrestore(&dmic.lock, flags);

    return ret;
}

static snd_pcm_uframes_t dmic_pcm_pointer(struct snd_pcm_substream *substream)
{
    return bytes_to_frames(substream->runtime, dmic.dma_pos);
}

static int dmic_pcm_copy(struct snd_pcm_substream *substream,
                int channel, /* not used (interleaved data) */
                snd_pcm_uframes_t pos,
                void __user *dst,
                snd_pcm_uframes_t count)
{
    dmic_read_frame(&dmic, dst, count);

    return 0;
}

static int dmic_pcm_null(struct snd_pcm_substream *substream)
{
    return 0;
}

static struct snd_pcm_ops dmic_pcm_ops = {
    .open = dmic_pcm_open,
    .close = dmic_pcm_close,
    .ioctl = snd_pcm_lib_ioctl,
    .hw_params = dmic_pcm_hw_params,
    .hw_free = dmic_pcm_hw_free,
    .trigger = dmic_pcm_trigger,
    .pointer = dmic_pcm_pointer,
    .copy   = dmic_pcm_copy,
    .prepare = dmic_pcm_null,
};

static int snd_dmic_init(struct dmic_data *dmic)
{
    int ret;
    struct snd_card *card;
    struct snd_pcm *pcm;

    ret = snd_card_create(-1, NULL, THIS_MODULE, 0, &card);
    if (ret < 0)
        panic("DMIC: snd_card_create failed %d\n", ret);

    dmic->card = card;

    strcpy(dmic->card->mixername, "DMIC Mixer");
    ret = snd_ctl_add(card, snd_ctl_new1(&capture_controls, dmic));
    if (ret < 0)
        panic("DMIC: snd_ctl_add failed %d\n", ret);

    ret = snd_pcm_new(dmic->card, "DMIC PCM", 0, 0, 1, &pcm);
    if (ret < 0)
        panic("DMIC: snd_pcm_new failed %d\n", ret);

    strcpy(pcm->name, "DMIC PCM");
    pcm->private_data = dmic;

    dmic->pcm = pcm;

    snd_pcm_set_ops(pcm, SNDRV_PCM_STREAM_CAPTURE, &dmic_pcm_ops);

    strcpy(card->driver, "DMIC_PCM");
    strcpy(card->shortname, "DMIC_PCM");
    strcpy(card->longname, "Ingenic x1830 DMIC_PCM");

    snd_card_set_dev(card, &dmic->pdev->dev);

    snd_pcm_lib_preallocate_pages_for_all(pcm, SNDRV_DMA_TYPE_DEV,
        &dmic->pdev->dev, MAX_DMA_BUFFERSIZE, MAX_DMA_BUFFERSIZE);

    ret = snd_card_register(card);
    if (ret < 0)
        panic("DMIC: snd_card_register failed %d\n", ret);

    return 0;
}

static irqreturn_t dmic_irq_handler(int irq, void *data)
{
    printk(KERN_ERR "dmic err: %x\n", dmic_read_reg(DMIC_ICR));
    dmic_write_reg(DMIC_ICR, dmic_read_reg(DMIC_ICR));
    return IRQ_HANDLED;
}

static int dmic_probe(struct platform_device *pdev)
{
    int ret;

    dmic.dev = &pdev->dev;
    dev_set_drvdata(dmic.dev, NULL);
    dmic.pdev = pdev;

    dmic.dma = dma_param_init(pdev);
    BUG_ON(!dmic.dma);

    snd_dmic_init(&dmic);
    spin_lock_init(&dmic.lock);
    init_waitqueue_head(&dmic.wait_queue);
    dmic_hrtimer_init(&dmic);

    clk = clk_get(NULL, "dmic");
    BUG_ON(IS_ERR(clk));

    dmic.irq = IRQ_DMIC;
    ret = request_irq(IRQ_DMIC, dmic_irq_handler, 0, "dmic_irq", &dmic);
    BUG_ON(ret);
    disable_irq(IRQ_DMIC);

    return 0;
}

static int dmic_remove(struct platform_device *pdev)
{
    snd_pcm_lib_preallocate_free_for_all(dmic.pcm);

    snd_card_free(dmic.card);

    dma_release_channel(dmic.dma->dma_chan);

    dmic_gpio_release();

    clk_put(clk);

    free_irq(IRQ_DMIC, &dmic);

    return 0;
}

static const  struct platform_device_id dmic_id_table[] = {
        { .name = "dmic", },
        {},
};

static struct platform_driver dmic_platform_driver = {
    .probe = dmic_probe,
    .remove = dmic_remove,
    .driver = {
        .name = "dmic",
        .owner = THIS_MODULE,
    },
    .id_table = dmic_id_table,
};

int dmic_platform_device_init(void);
void dmic_platform_device_exit(void);

static int __init dmic_init(void)
{
    int ret;

    ret = dmic_platform_device_init();
    if (ret)
        return ret;

    return platform_driver_register(&dmic_platform_driver);
}
module_init(dmic_init);

static void dmic_exit(void)
{
    dmic_platform_device_exit();

    platform_driver_unregister(&dmic_platform_driver);
}
module_exit(dmic_exit);

MODULE_LICENSE("GPL");
