#include <linux/init.h>
#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/platform_device.h>
#include <linux/moduleparam.h>
#include <assert.h>

#include <linux/kernel.h>

#include <sound/core.h>
#include <sound/pcm.h>
#include <sound/pcm_params.h>
#include <sound/soc.h>
#include <sound/soc-dpcm.h>

#include <ingenic_asla_sound_card.h>
#include <utils/gpio.h>

static int codec_en = GPIO_PB(30);

static void enable_adc_dac_codec(void)
{
    char buf[10];

    int ret = gpio_request(codec_en, "codec(adc-dac) enable");
    if (ret) {
        printk(KERN_ERR "x1600 dummy snd card: failed to request %s gpio: %s\n", "codec(adc-dac) enable", gpio_to_str(codec_en, buf));
        return;
    }
    gpio_direction_output(codec_en, 1);
}

static void disable_adc_dac_codec(void)
{
    gpio_free(codec_en);
}

static int codec_board_i2s_hw_params(struct snd_pcm_substream *substream, struct snd_pcm_hw_params *params)
{
    struct snd_soc_pcm_runtime *rtd = substream->private_data;
    struct snd_soc_dai *cpu_dai = rtd->cpu_dai;

    int ret = snd_soc_dai_set_sysclk(cpu_dai, SELECT_ENABLE_MCLK, 0, 0);
    if (ret)
        return ret;

    int fmt = SND_SOC_DAIFMT_CBS_CFS | SND_SOC_DAIFMT_NB_NF | SND_SOC_DAIFMT_I2S;
    ret = snd_soc_dai_set_fmt(cpu_dai, fmt);
    if (ret)
        return ret;

    return 0;
}

static int codec_board_i2s_hw_free(struct snd_pcm_substream *substream)
{
    struct snd_soc_pcm_runtime *rtd = substream->private_data;
    struct snd_soc_dai *cpu_dai = rtd->cpu_dai;

    return snd_soc_dai_set_sysclk(cpu_dai, SELECT_DISABLE_MCLK, 0, 0);
};

static struct snd_soc_ops i2s_ops = {
    .hw_params = codec_board_i2s_hw_params,
    .hw_free   = codec_board_i2s_hw_free,
};

static int asoc_dai_link_init(struct snd_soc_pcm_runtime *rtd)
{
    return 0;
}

static struct snd_soc_dai_link codec_board_dais[] = {
    [0] = {
        .name = "x1600 i2s",             /* 仅表示声卡信息，无实质意义 */
        .stream_name = "x1600 i2s pcm",  /* 仅表示声卡信息，无实质意义 */
        .platform_name = "ingenic-aic",
        .cpu_dai_name = "ingenic-aic",
        .init = asoc_dai_link_init,
        .codec_dai_name = "snd-soc-dummy-dai",
        .codec_name = "snd-soc-dummy",
        .dai_fmt = SND_SOC_DAIFMT_I2S|SND_SOC_DAIFMT_CBS_CFS,
        .ops = &i2s_ops,
    },
};

static struct snd_soc_card codec_snd_card = {
    .name = "dummy-sound-card",
    .owner = THIS_MODULE,
    .dai_link = codec_board_dais,
    .num_links = ARRAY_SIZE(codec_board_dais),
};

static int codec_board_probe(struct platform_device *pdev)
{
    int ret = 0;

    codec_snd_card.dev = &pdev->dev;

    ret = snd_soc_register_card(&codec_snd_card);
    if (ret)
        panic("dummy-sound-card: ASOC register card failed !\n");

    enable_adc_dac_codec();

    return ret;
}

static int codec_board_remove(struct platform_device *pdev)
{
    snd_soc_unregister_card(&codec_snd_card);

    platform_set_drvdata(pdev, NULL);

    disable_adc_dac_codec();

    return 0;
}

static struct platform_driver codec_board_driver = {
    .probe = codec_board_probe,
    .remove = codec_board_remove,
    .driver = {
        .owner = THIS_MODULE,
        .name = "ingenic-dummy-board",
        .pm = &snd_soc_pm_ops,
    },
};

/* stop no dev release warning */
static void codec_board_dev_release(struct device *dev){}

struct platform_device codec_board_device = {
    .name = "ingenic-dummy-board",
    .dev  = {
        .release = codec_board_dev_release,
    },
};

static int __init codec_board_init(void)
{
    int ret = platform_device_register(&codec_board_device);
    if (ret)
        return ret;

    return platform_driver_register(&codec_board_driver);
}
module_init(codec_board_init);

static void codec_board_exit(void)
{
    platform_device_unregister(&codec_board_device);

    platform_driver_unregister(&codec_board_driver);
}
module_exit(codec_board_exit);
MODULE_LICENSE("GPL");