#include <linux/module.h>
#include <linux/moduleparam.h>
#include <linux/kernel.h>
#include <linux/init.h>
#include <linux/delay.h>
#include <linux/pm.h>
#include <linux/platform_device.h>
#include <linux/vmalloc.h>
#include <linux/slab.h>
#include <linux/ctype.h>
#include <linux/io.h>
#include <sound/core.h>
#include <sound/pcm.h>
#include <sound/pcm_params.h>
#include <sound/soc.h>
#include <sound/jack.h>
#include <sound/soc-dapm.h>
#include <sound/initval.h>
#include <sound/tlv.h>
#include <asm/div64.h>
#include <sound/soc-dai.h>
#include <linux/spinlock.h>
#include <utils/gpio.h>

#include "../aic/aic.h"

#define ICODEC_FORMATS (SNDRV_PCM_FMTBIT_S16_LE | SNDRV_PCM_FMTBIT_S24_LE)
#define ICODEC_RATE SNDRV_PCM_RATE_8000_192000

struct icodec_data {
    int ref_count;
    int adc_is_enable;
    int dac_is_enable;

    spinlock_t lock;

    struct mutex mutex;
};

struct icodec_data icodec;

#define MAX_MIC_VOLUME 0x1f

static unsigned int max_hpout_volume = 0x1f;
static unsigned int min_hpout_volume = 0;
static int hpout_gain = 16;
static int mic_alc_gain = 26;
static int mic_in_gain = 3;
static int hpout_mute = 0;
static int spk_gpio1 = -1;
static int spk_gpio2 = -1;
static int speaker_gpio1_level = 1;
static int speaker_gpio2_level = 1;
static int bias_enable = 1;
static int bias_level = 7;
static int speaker_need_delay_ms = 20;

module_param(max_hpout_volume, int, 0644);
module_param(min_hpout_volume, int, 0644);

module_param_gpio_named(speaker_gpio1, spk_gpio1, 0644);
module_param(speaker_gpio1_level, int, 0644);

module_param_gpio_named(speaker_gpio2, spk_gpio2, 0644);
module_param(speaker_gpio2_level, int, 0644);

module_param(speaker_need_delay_ms, int, 0644);

module_param(bias_enable, int, 0644);
module_param(bias_level, int, 0644);

module_param(mic_in_gain, int, 0644);

#include "icodec_hal.c"

static inline int m_gpio_request(int gpio, const char *name)
{
    if (gpio < 0)
        return 0;

    int ret = gpio_request(gpio, name);
    if (ret) {
        char buf[20];
        printk(KERN_ERR "ICODEC: failed to request %s: %s\n", name, gpio_to_str(gpio, buf));
        return ret;
    }

    return 0;
}

static inline void m_gpio_free(int gpio)
{
    if (gpio >= 0)
        gpio_free(gpio);
}

static inline void m_gpio_direction_output(int gpio, int value)
{
    if (gpio >= 0)
        gpio_direction_output(gpio, value);
}

static int speaker_gpio_request(void)
{
    int ret = 0;

    ret = m_gpio_request(spk_gpio1, "spk_gpio1");
    if(ret < 0)
        return ret;


    ret = m_gpio_request(spk_gpio2, "spk_gpio2");
    if (ret < 0) {
        m_gpio_free(spk_gpio1);
        return ret;
    }

    m_gpio_direction_output(spk_gpio1, !speaker_gpio1_level);
    m_gpio_direction_output(spk_gpio2, !speaker_gpio2_level);


    return 0;
}

static void speaker_gpio_release(void)
{
    m_gpio_free(spk_gpio1);
    m_gpio_free(spk_gpio2);
}

static inline void speaker_enable(void)
{
    m_gpio_direction_output(spk_gpio1, speaker_gpio1_level);
    m_gpio_direction_output(spk_gpio2, speaker_gpio2_level);

    if (speaker_need_delay_ms) {
        int us = speaker_need_delay_ms * 1000;
        usleep_range(us, us);
    }
}

static void speaker_disable(void)
{
    m_gpio_direction_output(spk_gpio1, !speaker_gpio1_level);
    m_gpio_direction_output(spk_gpio2, !speaker_gpio2_level);
}

static int to_data_bits(int format)
{
    if (format == SNDRV_PCM_FORMAT_S16_LE)
        return 16;
    if (format == SNDRV_PCM_FORMAT_S24_LE)
        return 24;
    if (format == SNDRV_PCM_FORMAT_S32_LE)
        return 32;
    return 16;
}

static int icodec_hw_params(struct snd_pcm_substream *substream,
        struct snd_pcm_hw_params *params, struct snd_soc_dai *dai)
{
    int format = params_format(params);
    int data_bits = to_data_bits(format);
    unsigned long flags;

    mutex_lock(&icodec.mutex);

    if (substream->stream == SNDRV_PCM_STREAM_PLAYBACK) {
        if (!icodec.dac_is_enable) {
            if (icodec.ref_count++ == 0) {
                icodec_power_on();
                usleep_range(10 *1000, 10 *1000);
            }

            speaker_enable();

            spin_lock_irqsave(&icodec.lock, flags);
            icodec_disable_dac();
            icodec_config_dac(data_bits);

            icodec_enable_dac(hpout_mute, hpout_gain);

            spin_unlock_irqrestore(&icodec.lock, flags);

            icodec.dac_is_enable = 1;
        }
    } else {
        if (!icodec.adc_is_enable) {
            if (icodec.ref_count++ == 0) {
                icodec_power_on();
                usleep_range(10 *1000, 10 *1000);
            }

            spin_lock_irqsave(&icodec.lock, flags);
            icodec_config_adc(data_bits);
            icodec_enable_adc(bias_enable, bias_level, mic_in_gain, mic_alc_gain);
            spin_unlock_irqrestore(&icodec.lock, flags);

            icodec.adc_is_enable = 1;
        }
    }
    mutex_unlock(&icodec.mutex);

    return 0;
}

static int icodec_hw_free(struct snd_pcm_substream *substream,
        struct snd_soc_dai *dai)
{
    mutex_lock(&icodec.mutex);

    if (substream->stream == SNDRV_PCM_STREAM_PLAYBACK) {
        if (icodec.dac_is_enable) {

            icodec_disable_dac();
            usleep_range(30*1000, 30*1000);

            speaker_disable();

            icodec.dac_is_enable = 0;

            if (--icodec.ref_count == 0)
                icodec_power_off();
        }
    } else {
        if (icodec.adc_is_enable) {
            icodec_disable_adc();
            icodec.adc_is_enable = 0;

            if (--icodec.ref_count == 0)
                icodec_power_off();
        }
    }

    mutex_unlock(&icodec.mutex);

    return 0;
}

static int icodec_trigger(struct snd_pcm_substream * stream, int cmd,
        struct snd_soc_dai *dai)
{
    switch (cmd) {
    case SNDRV_PCM_TRIGGER_START:
    case SNDRV_PCM_TRIGGER_RESUME:
    case SNDRV_PCM_TRIGGER_PAUSE_RELEASE:
        if (stream->stream == SNDRV_PCM_STREAM_PLAYBACK)
            aic_mute_icodec(hpout_mute);
        break;
    case SNDRV_PCM_TRIGGER_STOP:
    case SNDRV_PCM_TRIGGER_SUSPEND:
    case SNDRV_PCM_TRIGGER_PAUSE_PUSH:
        break;
    }
    return 0;
}

static int icodec_probe(struct snd_soc_component *component)
{
    int ret = 0;

    if (max_hpout_volume > 0x1f)
        max_hpout_volume = 0x1f;

    if (min_hpout_volume > max_hpout_volume)
        min_hpout_volume = max_hpout_volume;

    if (!hpout_gain) {
        hpout_gain = min_hpout_volume +
            (max_hpout_volume - min_hpout_volume) * 7 / 10;
    }

    mutex_init(&icodec.mutex);

    spin_lock_init(&icodec.lock);

    ret = speaker_gpio_request();
    if (ret < 0)
        return ret;

    return 0;
}

static void icodec_remove(struct snd_soc_component *component)
{
    speaker_gpio_release();
}

static int hpout_info_volume(struct snd_kcontrol *kctrl,
            struct snd_ctl_elem_info *uinfo)
{
    uinfo->type = SNDRV_CTL_ELEM_TYPE_INTEGER;
    uinfo->count = 1;
    uinfo->value.integer.min = min_hpout_volume;
    uinfo->value.integer.max = max_hpout_volume;
    return 0;
}

static int hpout_get_volume(struct snd_kcontrol *kctrl,
            struct snd_ctl_elem_value *uctl)
{
    mutex_lock(&icodec.mutex);

    if (icodec.dac_is_enable)
        uctl->value.integer.value[0] = icodec_get_dac_gain();
    else
        uctl->value.integer.value[0] = hpout_gain;

    mutex_unlock(&icodec.mutex);

    return 0;
}

static int hpout_put_volume(struct snd_kcontrol *kctrl,
            struct snd_ctl_elem_value *uctl)
{
    int vol;

    mutex_lock(&icodec.mutex);

    vol = uctl->value.integer.value[0];
    if (vol < min_hpout_volume)
        vol = min_hpout_volume;
    if (vol > max_hpout_volume)
        vol = max_hpout_volume;

    if (vol == min_hpout_volume)
        hpout_mute = 1;
    else
        hpout_mute = 0;

    if (icodec.dac_is_enable) {
        icodec_set_dac_gain(vol);

        aic_mute_icodec(hpout_mute);
    }

    hpout_gain = vol;

    mutex_unlock(&icodec.mutex);

    return 0;
}

static int mic_info_volume(struct snd_kcontrol *kctrl,
            struct snd_ctl_elem_info *uinfo)
{
    uinfo->type = SNDRV_CTL_ELEM_TYPE_INTEGER;
    uinfo->count = 1;
    uinfo->value.integer.min = 0;
    uinfo->value.integer.max = MAX_MIC_VOLUME;
    return 0;
}

static int mic_get_volume(struct snd_kcontrol *kctrl,
            struct snd_ctl_elem_value *uctl)
{
    mutex_lock(&icodec.mutex);

    if (icodec.adc_is_enable)
        uctl->value.integer.value[0] = icodec_get_adc_gain();
    else
        uctl->value.integer.value[0] = mic_alc_gain;

    mutex_unlock(&icodec.mutex);

    return 0;
}

static int mic_put_volume(struct snd_kcontrol *kctrl,
            struct snd_ctl_elem_value *uctl)
{
    int vol;

    mutex_lock(&icodec.mutex);

    vol = uctl->value.integer.value[0];
    if (vol < 0)
        vol = 0;
    if (vol > MAX_MIC_VOLUME)
        vol = MAX_MIC_VOLUME;

    if (icodec.adc_is_enable)
        icodec_set_adc_gain(vol);

    mic_alc_gain = vol;

    mutex_unlock(&icodec.mutex);

    return 0;
}

static int mute_info(struct snd_kcontrol *kctrl,
            struct snd_ctl_elem_info *uinfo)
{
    uinfo->type = SNDRV_CTL_ELEM_TYPE_INTEGER;
    uinfo->count = 1;
    uinfo->value.integer.min = 0;
    uinfo->value.integer.max = 1;
    return 0;
}

static int mute_get(struct snd_kcontrol *kctrl,
            struct snd_ctl_elem_value *uctl)
{
    mutex_lock(&icodec.mutex);

    if (icodec.dac_is_enable)
        uctl->value.integer.value[0] = icodec_get_dac_mute();
    else
        uctl->value.integer.value[0] = hpout_mute;

    mutex_unlock(&icodec.mutex);

    return 0;
}

/* 由于使用icodec的mute功能无法达到完全静音的效果，
 * 所以这里需要借助 aic_mute_icodec 来实现。
 * */
static int mute_set(struct snd_kcontrol *kctrl,
            struct snd_ctl_elem_value *uctl)
{
    int mute;

    mutex_lock(&icodec.mutex);

    mute = uctl->value.integer.value[0];

    hpout_mute = mute;

    if (icodec.dac_is_enable) {
        icodec_set_dac_mute(mute);

        aic_mute_icodec(mute);
    }

    mutex_unlock(&icodec.mutex);

    return 0;
}

static struct snd_soc_dai_ops icodec_dai_ops = {
    .hw_params = icodec_hw_params,
    .hw_free = icodec_hw_free,
    .trigger = icodec_trigger,
};

static const DECLARE_TLV_DB_SCALE(hpout_tlv, -3900, 150, 0);
static const DECLARE_TLV_DB_SCALE(alc_tlv, -1800, 150, 0);

static const struct snd_kcontrol_new icodec_controls[] = {
    [0] = {
        .iface  = SNDRV_CTL_ELEM_IFACE_MIXER,
        .name   = "Master Playback Volume",
        .access = SNDRV_CTL_ELEM_ACCESS_TLV_READ | SNDRV_CTL_ELEM_ACCESS_READWRITE,
        .info   = hpout_info_volume,
        .get    = hpout_get_volume,
        .put    = hpout_put_volume,
        .tlv.p  = hpout_tlv,
    },
    [1] = {
        .iface  = SNDRV_CTL_ELEM_IFACE_MIXER,
        .name   = "Mic Volume",
        .access = SNDRV_CTL_ELEM_ACCESS_TLV_READ | SNDRV_CTL_ELEM_ACCESS_READWRITE,
        .info   = mic_info_volume,
        .get    = mic_get_volume,
        .put    = mic_put_volume,
        .tlv.p  = alc_tlv,
    },
    [2] = {
        .iface  = SNDRV_CTL_ELEM_IFACE_MIXER,
        .name   = "Playback Mute",
        .access = SNDRV_CTL_ELEM_ACCESS_READWRITE,
        .info   = mute_info,
        .get    = mute_get,
        .put    = mute_set,
    },
};

static const struct snd_soc_component_driver soc_component_dev_icodec = {
    .probe = icodec_probe,
    .remove = icodec_remove,
    .controls = icodec_controls,
    .num_controls = ARRAY_SIZE(icodec_controls),
};

static struct snd_soc_dai_driver icodec_dai = {
    .name = "internal-codec",
    .playback = {
        .stream_name = "Playback",
        .channels_min = 1,
        .channels_max = 1,
        .rates = ICODEC_RATE,
        .formats = ICODEC_FORMATS,
    },
    .capture = {
        .stream_name = "Capture",
        .channels_min = 1,
        .channels_max = 2,
        .rates = ICODEC_RATE,
        .formats = ICODEC_FORMATS,
    },
    .symmetric_rates = 1,
    .ops = &icodec_dai_ops,
};

static int icodec_plat_probe(struct platform_device *pdev)
{
    int ret;

    ret = snd_soc_register_component(&pdev->dev, &soc_component_dev_icodec, &icodec_dai, 1);
    if (ret)
        panic("icodec: asoc register icodec failed ret = %d\n", ret);

    return 0;
}

static int icodec_plat_remove(struct platform_device *pdev)
{
    snd_soc_unregister_component(&pdev->dev);

    return 0;
}

/* stop no dev release warning */
static void asoc_icodec_dev_release(struct device *dev){}

static struct platform_device icodec_device = {
    .id   = -1,
    .name = "ingenic-icodec",
    .dev  = {
        .release = asoc_icodec_dev_release,
    },
};

static struct platform_driver icodec_driver = {
    .driver = {
        .name = "ingenic-icodec",
        .owner = THIS_MODULE,
    },
    .probe = icodec_plat_probe,
    .remove = icodec_plat_remove,
};

static int icodec_init(void)
{
    int ret;

    ret = platform_device_register(&icodec_device);
    if (ret)
        return ret;

    return platform_driver_register(&icodec_driver);
}
module_init(icodec_init);

static void icodec_exit(void)
{
    platform_device_unregister(&icodec_device);
    platform_driver_unregister(&icodec_driver);
}
module_exit(icodec_exit);
MODULE_LICENSE("GPL");