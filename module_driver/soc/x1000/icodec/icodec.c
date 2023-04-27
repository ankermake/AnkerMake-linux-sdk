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

#define ICODEC_FORMATS (SNDRV_PCM_FMTBIT_S16_LE | SNDRV_PCM_FMTBIT_S24_LE)
#define ICODEC_RATE (SNDRV_PCM_RATE_8000 | SNDRV_PCM_RATE_16000 | \
        SNDRV_PCM_RATE_32000 | SNDRV_PCM_RATE_44100 | SNDRV_PCM_RATE_48000 | \
        SNDRV_PCM_RATE_96000 | SNDRV_PCM_RATE_176400 | SNDRV_PCM_RATE_192000)

#define ENABLE 1
#define DISABLE 0

struct icodec_data {
    struct device *dev;

    int is_bais;
    int is_init;
    int dac_rate;
    int adc_rate;
    int dac_format;
    int adc_format;
    int adc_is_enable;
    int dac_is_enable;

    spinlock_t lock;

    struct mutex mutex;
};

struct icodec_data icodec;

#define MAX_MIC_VOLUME 0x1f

static unsigned int max_hpout_volume = 0x1f;
static unsigned int min_hpout_volume = 0;
static int spk_gpio = -1;
static int hpout_gain = 17;
static int mic_gain = 12;
static int hpout_mute = 0;

module_param_named(is_bais, icodec.is_bais, int, 0644);
module_param_named(max_hpout_volume, max_hpout_volume, int, 0644);
module_param_named(min_hpout_volume, min_hpout_volume, int, 0644);
module_param_gpio_named(speaker_gpio, spk_gpio, 0644);

#include "icodec_hal.c"

static int spkeaker_gpio_request(void)
{
    int ret = 0;
    char buf[10];

    if (spk_gpio >= 0) {
        ret = gpio_request(spk_gpio, "spk_gpio");
        if (ret < 0)
            printk(KERN_ERR "ICODEC: speaker gpio failed to request %s.\n", gpio_to_str(spk_gpio, buf));
    }

    return ret;
}

static void spkeaker_gpio_release(void)
{
    if (spk_gpio >= 0)
       gpio_free(spk_gpio);
}

static void spkeaker_enable(int enable)
{
    if (spk_gpio >= 0)
        gpio_direction_output(spk_gpio, enable);
}

static int icodec_hw_params(struct snd_pcm_substream *substream,
        struct snd_pcm_hw_params *params, struct snd_soc_dai *dai)
{
    int sample_rate[] = {8000, 11025, 12000, 16000,22050, 24000,
            32000, 44100, 48000, 88200, 96000, 176400, 192000};

    int count = ARRAY_SIZE(sample_rate);
    int rate = params_rate(params);
    int format = params_format(params);
    int i;

    for (i = 0; i < count; i++) {
        if (rate == sample_rate[i])
            break;
    }

    if (i >= count) {
        printk(KERN_ERR "ICODEC: No support rate %d\n", rate);
        return -EINVAL;
    }

    mutex_lock(&icodec.mutex);

    if (substream->stream == SNDRV_PCM_STREAM_PLAYBACK) {
        icodec.dac_rate = i;
        icodec.dac_format = format;

        if (!icodec.dac_is_enable) {
            spkeaker_enable(ENABLE);
            icodec_enable_dac();
            icodec.dac_is_enable = 1;
        }
    } else {
        icodec.adc_rate = i;
        icodec.adc_format = format;

        if (!icodec.adc_is_enable) {
            icodec_enable_adc();
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

    if (substream->stream == SNDRV_PCM_STREAM_PLAYBACK)
        icodec.dac_is_enable = 0;
    else
        icodec.adc_is_enable = 0;

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
        break;
    case SNDRV_PCM_TRIGGER_STOP:
    case SNDRV_PCM_TRIGGER_SUSPEND:
    case SNDRV_PCM_TRIGGER_PAUSE_PUSH:
        break;
    }
    return 0;
}

static int icodec_probe(struct snd_soc_codec *codec)
{
    int ret = 0;

    if (max_hpout_volume > 0x1f)
        max_hpout_volume = 0x1f;

    if (min_hpout_volume > 0x1f)
        min_hpout_volume = 0x1f;

    mutex_init(&icodec.mutex);

    ret = spkeaker_gpio_request();
    if (ret < 0)
        return ret;

    icodec_hal_init();

    icodec_power_on();

    return 0;
}

static int icodec_remove(struct snd_soc_codec *codec)
{
    icodec_power_off();

    spkeaker_gpio_release();

    return 0;
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
        uctl->value.integer.value[0] = icodec_playback_pcm_get_volume();
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

    if (icodec.dac_is_enable)
        icodec_playback_pcm_set_volume(vol);

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
        uctl->value.integer.value[0] = icodec_capture_pcm_get_volume();
    else
        uctl->value.integer.value[0] = mic_gain;

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
        icodec_playback_pcm_set_volume(vol);

    mic_gain = vol;

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
        uctl->value.integer.value[0] = icodec_playback_pcm_get_mute();
    else
        uctl->value.integer.value[0] = hpout_mute;

    mutex_unlock(&icodec.mutex);

    return 0;
}

static int mute_set(struct snd_kcontrol *kctrl,
            struct snd_ctl_elem_value *uctl)
{
    int mute;

    mutex_lock(&icodec.mutex);

    mute = uctl->value.integer.value[0];

    if (icodec.dac_is_enable)
        icodec_playback_pcm_set_mute(mute);

    hpout_mute = mute;

    mutex_unlock(&icodec.mutex);

    return 0;
}

static struct snd_soc_dai_ops icodec_dai_ops = {
    .hw_params = icodec_hw_params,
    .hw_free = icodec_hw_free,
    .trigger = icodec_trigger,
};

static const DECLARE_TLV_DB_SCALE(hpout_tlv, -3100, 100, 0);
static const DECLARE_TLV_DB_SCALE(alc_tlv, -3100, 100, 0);

static struct snd_kcontrol_new icodec_controls[] = {
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
        .name   = "Mute",
        .access = SNDRV_CTL_ELEM_ACCESS_READWRITE,
        .info   = mute_info,
        .get    = mute_get,
        .put    = mute_set,
    },
};

static struct snd_soc_codec_driver icodec_codec = {
    .probe = icodec_probe,
    .remove = icodec_remove,
    .controls = icodec_controls,
    .num_controls = ARRAY_SIZE(icodec_controls),
};

static struct snd_soc_dai_driver icodec_dai = {
    .name = "internal-codec",
    .playback = {
        .stream_name = "playback",
        .channels_min = 1,
        .channels_max = 2,
        .rates = ICODEC_RATE,
        .formats = ICODEC_FORMATS,
    },
    .capture = {
        .stream_name = "Capture",
        .channels_min = 1,
        .channels_max = 1,
        .rates = ICODEC_RATE,
        .formats = ICODEC_FORMATS,
    },
    .symmetric_rates = 1,
    .ops = &icodec_dai_ops,
};

static int icodec_plat_probe(struct platform_device *pdev)
{
    int ret;

    ret = snd_soc_register_codec(&pdev->dev, &icodec_codec, &icodec_dai, 1);
    if (ret)
        panic("ICODEC: asoc register icodec failed ret = %d\n", ret);

    return 0;
}

static int icodec_plat_remove(struct platform_device *pdev)
{
    snd_soc_unregister_codec(&pdev->dev);

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
