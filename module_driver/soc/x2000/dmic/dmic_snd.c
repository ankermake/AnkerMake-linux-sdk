#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/platform_device.h>
#include <linux/moduleparam.h>
#include <linux/clk.h>
#include <linux/dma-mapping.h>
#include <linux/dmaengine.h>
#include <linux/interrupt.h>
#include <linux/irq.h>
#include <linux/slab.h>
#include <linux/cdev.h>
#include <linux/device.h>
#include <linux/io.h>
#include <linux/memory.h>
#include <linux/mm.h>
#include <linux/spinlock.h>
#include <linux/kthread.h>
#include <linux/delay.h>
#include <linux/workqueue.h>
#include <assert.h>
#include <sound/core.h>
#include <sound/pcm.h>
#include <sound/pcm_params.h>
#include <sound/soc.h>
#include <sound/soc-dpcm.h>

#include <utils/gpio.h>

#include "dmic_hal.c"
#include "../audio_dma/audio.h"

#define ASOC_DMIC_RATE (SNDRV_PCM_RATE_8000 | SNDRV_PCM_RATE_16000 | \
                        SNDRV_PCM_RATE_48000 | SNDRV_PCM_RATE_96000)

#define DMIC_BUSY 1
#define DMIC_IDLE 0

#define MAX_CAPTURE_VOLUME 0x1f

#define DMIC_GPIO_CLK GPIO_PC(20)
#define DMIC_GPIO_IN0 GPIO_PC(21)
#define DMIC_GPIO_IN1 GPIO_PC(22)
#define DMIC_GPIO_IN2 GPIO_PC(23)
#define DMIC_GPIO_IN3 GPIO_PC(24)

struct dmic_data {
    spinlock_t lock;

    struct clk *dmic_clk;
    struct clk *mux_dmic;

    unsigned int channels;
    unsigned int dma_channels;
    unsigned int sample_rate;
    unsigned int dma_frame_size;
    unsigned int frame_size;
    void *dma_buffer;

    unsigned int data_offset[8];

    unsigned int is_enable;
    int fmt_width;
};

struct dmic_data dmic_dev;

static int gpio_state = 0;
static int dmic_gain = 4;

static int dmic_int0 = 0;
static int dmic_int1 = 0;
static int dmic_int2 = 0;
static int dmic_int3 = 0;

module_param_named(dmic_int0, dmic_int0, int, 0644);
module_param_named(dmic_int1, dmic_int1, int, 0644);
module_param_named(dmic_int2, dmic_int2, int, 0644);
module_param_named(dmic_int3, dmic_int3, int, 0644);

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

    gpio_set_func(gpio, GPIO_FUNC_0);

    return 0;
}

static int dmic_gpio_request(void)
{
    int ret = 0;

    if (!(gpio_state & BIT(0))) {
        ret = init_gpio(DMIC_GPIO_CLK, "dmic-clk");
        if (ret < 0)
            return ret;
        gpio_state |= BIT(0);
    }

    if (dmic_int0 && !(gpio_state & BIT(1))) {
        ret = init_gpio(DMIC_GPIO_IN0, "dmic-in0");
        if (ret < 0)
            goto err_gpio_in0;
        gpio_state |= BIT(1);
    }

    if (dmic_int1 && !(gpio_state & BIT(2))) {
        ret = init_gpio(DMIC_GPIO_IN1, "dmic-in1");
        if (ret < 0)
            goto err_gpio_in1;
        gpio_state |= BIT(2);
    }

    if (dmic_int2 && !(gpio_state & BIT(3))) {
        ret = init_gpio(DMIC_GPIO_IN2, "dmic-in2");
        if (ret < 0)
            goto err_gpio_in2;
        gpio_state |= BIT(3);
    }

    if (dmic_int3 && !(gpio_state & BIT(4))) {
        ret = init_gpio(DMIC_GPIO_IN3, "dmic-in3");
        if (ret < 0)
            goto err_gpio_in3;
        gpio_state |= BIT(4);
    }

    return ret;

err_gpio_in3:
    if (gpio_state & BIT(3))
        gpio_free(DMIC_GPIO_IN2);
err_gpio_in2:
    if (gpio_state & BIT(2))
        gpio_free(DMIC_GPIO_IN1);
err_gpio_in1:
    if (gpio_state & BIT(1))
        gpio_free(DMIC_GPIO_IN0);
err_gpio_in0:
    gpio_free(DMIC_GPIO_CLK);

    gpio_state = 0;

    return ret;
}

static void dmic_gpio_release(void)
{
    if (gpio_state & BIT(0))
        gpio_free(DMIC_GPIO_CLK);

    if (gpio_state & BIT(1))
        gpio_free(DMIC_GPIO_IN0);

    if (gpio_state & BIT(2))
        gpio_free(DMIC_GPIO_IN1);

    if (gpio_state & BIT(3))
        gpio_free(DMIC_GPIO_IN2);

    if (gpio_state & BIT(4))
        gpio_free(DMIC_GPIO_IN3);

    gpio_state = 0;
}

#include "dmic_dma.c"

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

    spin_lock_irqsave(&dmic_dev.lock, flags);

    if (dmic_dev.is_enable)
        uctl->value.integer.value[0] = dmic_capture_get_volume();
    else
        uctl->value.integer.value[0] = dmic_gain;

    spin_unlock_irqrestore(&dmic_dev.lock, flags);

    return 0;
}

static int dmic_put_volume(struct snd_kcontrol *kctrl,
            struct snd_ctl_elem_value *uctl)
{
    int vol;
    unsigned long flags;

    spin_lock_irqsave(&dmic_dev.lock, flags);

    vol = uctl->value.integer.value[0];
    if (vol < 0)
        vol = 0;
    if (vol > MAX_CAPTURE_VOLUME)
        vol = MAX_CAPTURE_VOLUME;

    if (dmic_dev.is_enable)
        dmic_capture_set_volume(vol);

    dmic_gain = vol;

    spin_unlock_irqrestore(&dmic_dev.lock, flags);

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

static void dmic_pcm_start(void)
{
    unsigned long flags;

    spin_lock_irqsave(&dmic_dev.lock, flags);

    dmic_config(dmic_dev.dma_channels, dmic_dev.sample_rate, dmic_gain, dmic_dev.fmt_width);

    dmic_enable();

    dmic_dev.is_enable = DMIC_BUSY;

    spin_unlock_irqrestore(&dmic_dev.lock, flags);
}

static void dmic_pcm_stop(void)
{
    unsigned long flags;

    spin_lock_irqsave(&dmic_dev.lock, flags);

    dmic_disable();

    dmic_dev.is_enable = DMIC_IDLE;

    spin_unlock_irqrestore(&dmic_dev.lock, flags);
}

static int dmic_trigger(struct snd_pcm_substream *substream, int cmd,
        struct snd_soc_dai *dai)
{
    switch (cmd) {
    case SNDRV_PCM_TRIGGER_START:
    case SNDRV_PCM_TRIGGER_RESUME:
    case SNDRV_PCM_TRIGGER_PAUSE_RELEASE:
        dmic_pcm_start();
        break;
    case SNDRV_PCM_TRIGGER_STOP:
    case SNDRV_PCM_TRIGGER_SUSPEND:
    case SNDRV_PCM_TRIGGER_PAUSE_PUSH:
        dmic_pcm_stop();
        break;
    default:
        return -EINVAL;
    }

    return 0;
}

static int dmic_hw_params(struct snd_pcm_substream *substream,
        struct snd_pcm_hw_params *hw_params, struct snd_soc_dai *dai)
{
    return 0;
}

static int dmic_startup(struct snd_pcm_substream *substream, struct snd_soc_dai *dai)
{
    clk_prepare_enable(dmic_dev.mux_dmic);

    clk_prepare_enable(dmic_dev.dmic_clk);

    return 0;
}

static void dmic_shutdown(struct snd_pcm_substream *substream, struct snd_soc_dai *dai)
{
    clk_disable_unprepare(dmic_dev.dmic_clk);

    clk_disable_unprepare(dmic_dev.mux_dmic);
}

static int dmic_hw_free(struct snd_pcm_substream *substream, struct snd_soc_dai *dai)
{
    return 0;
}

static struct snd_soc_dai_ops dmic_dai_ops = {
    .startup    = dmic_startup,
    .shutdown   = dmic_shutdown,
    .trigger    = dmic_trigger,
    .hw_params  = dmic_hw_params,
    .hw_free    = dmic_hw_free,
};

static int dmic_dai_probe(struct snd_soc_dai *dai)
{
    return 0;
}

static int dmic_dai_remove(struct snd_soc_dai *dai)
{
    return 0;
}

static struct snd_soc_dai_driver dmic_dai = {
    .probe = dmic_dai_probe,
    .remove = dmic_dai_remove,
    .name = "ingenic-dmic",
    .suspend = NULL,
    .resume = NULL,
    .capture = {
        .channels_min = 1,
        .channels_max = 8,
        .rates = ASOC_DMIC_RATE,
        .formats = SNDRV_PCM_FMTBIT_S16_LE | SNDRV_PCM_FMTBIT_S24_LE,
    },
    .ops = &dmic_dai_ops,
};

static void dmic_clk_init(void)
{
    struct clk *parent;

    dmic_dev.mux_dmic = clk_get(NULL, "mux_dmic");
    if (IS_ERR_OR_NULL(dmic_dev.mux_dmic))
        panic("DMIC: get clock mux_dmic failed\n");

    parent = clk_get(NULL, "ext");
    clk_set_parent(dmic_dev.mux_dmic, parent);
    clk_put(parent);

    dmic_dev.dmic_clk = clk_get(NULL, "gate_dmic");
    if (IS_ERR_OR_NULL(dmic_dev.dmic_clk))
        panic("DMIC: get clock gate_dmic failed\n");

    clk_set_rate(dmic_dev.mux_dmic, 24000000);
}

static struct snd_soc_component_driver dmic_component = {
    .name = "ingenic-dmic-component",
    .controls = &capture_controls,
    .num_controls = 1,
};

static void calculate_data_offset(void)
{
    int channels = 0;
    if (dmic_int0) {
        dmic_dev.data_offset[channels++] = 0;
        dmic_dev.data_offset[channels++] = 1;
    }

    if (dmic_int1) {
        dmic_dev.data_offset[channels++] = 2;
        dmic_dev.data_offset[channels++] = 3;
    }

    if (dmic_int2) {
        dmic_dev.data_offset[channels++] = 4;
        dmic_dev.data_offset[channels++] = 5;
    }

    if (dmic_int3) {
        dmic_dev.data_offset[channels++] = 6;
        dmic_dev.data_offset[channels++] = 7;
    }

    return;
}

static int dmic_probe(struct platform_device *pdev)
{
    int ret;

    spin_lock_init(&dmic_dev.lock);

    dmic_clk_init();

    dmic_dai.capture.channels_max = (dmic_int0 + dmic_int1 + dmic_int2 + dmic_int3) * 2;

    ret = snd_soc_register_component(&pdev->dev, &dmic_component, &dmic_dai, 1);
    if (ret)
        panic("DMIC: failed to register snd component: %d\n", ret);

    ret = dmic_dma_driver_init(pdev);
    if (ret)
        panic("DMIC: failed to register snd platform: %d\n", ret);

    dmic_dev.dma_buffer = m_dma_alloc_coherent(MAX_DMA_BUFFERSIZE);
    if (!dmic_dev.dma_buffer)
        panic("DMIC: failed to alloc dma buffer.\n");

    calculate_data_offset();

    return 0;
}

static int dmic_remove(struct platform_device *pdev)
{
    m_dma_free_coherent(dmic_dev.dma_buffer, MAX_DMA_BUFFERSIZE);

    dmic_dma_driver_exit(pdev);

    snd_soc_unregister_component(&pdev->dev);

    clk_put(dmic_dev.dmic_clk);

    clk_put(dmic_dev.mux_dmic);

    dmic_gpio_release();

    return 0;
}

/* stop no dev release warning */
static void dmic_device_release(struct device *dev){}

struct platform_device dmic_platform_device = {
    .name           = "ingenic-dmic",
    .id             = -1,
    .dev            = {
        .release = dmic_device_release,
    },
};

static struct platform_driver dmic_platform_driver = {
    .probe = dmic_probe,
    .remove = dmic_remove,
    .driver = {
        .name = "ingenic-dmic",
        .owner = THIS_MODULE,
    },
};

int dmic_board_init(void);
void dmic_board_exit(void);

static int __init dmic_init(void)
{
    int ret = platform_device_register(&dmic_platform_device);
    if (ret) {
        printk(KERN_ERR "DMIC: Failed to register mic dev: %d\n", ret);
        return ret;
    }

    ret = platform_driver_register(&dmic_platform_driver);

    return dmic_board_init();
}
module_init(dmic_init);

static void dmic_exit(void)
{
    dmic_board_exit();

    platform_device_unregister(&dmic_platform_device);

    platform_driver_unregister(&dmic_platform_driver);
}
module_exit(dmic_exit);

MODULE_LICENSE("GPL");
