#include <linux/init.h>
#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/platform_device.h>
#include <linux/moduleparam.h>
#include <assert.h>

#include <sound/core.h>
#include <sound/pcm.h>
#include <sound/pcm_params.h>
#include <sound/soc.h>
#include <sound/soc-dpcm.h>

#include <ingenic_asla_sound_card.h>

static int ecodec_board_i2s_hw_params(struct snd_pcm_substream *substream, struct snd_pcm_hw_params *params)
{
    struct snd_soc_pcm_runtime *rtd = substream->private_data;
    struct snd_soc_dai *cpu_dai = rtd->cpu_dai;
    int rate = params_rate(params);

    int ret = snd_soc_dai_set_sysclk(cpu_dai, SELECT_EXT_CODEC, rate * 256, 0);
    if (ret)
        return ret;

    return 0;
}

static struct snd_soc_ops i2s_ops = {
    .hw_params = ecodec_board_i2s_hw_params,
};

static int asoc_dai_link_init(struct snd_soc_pcm_runtime *rtd)
{
    return 0;
}

static struct snd_soc_dai_link ecodec_board_dais[] = {

     [0] = {
        .name = "x1830 ecodec",       /* 仅表示声卡信息，无实质意义 */
        .stream_name = "x1830 ecodec pcm",  /* 仅表示声卡信息，无实质意义 */
        .platform_name = "jz-asoc-aic-i2s",
        .cpu_dai_name = "jz-asoc-aic-i2s",
        .init = asoc_dai_link_init,
        .codec_dai_name = "ES8388 HiFi",
        .codec_name = "ES8388.1-0011",
        .dai_fmt = SND_SOC_DAIFMT_I2S|SND_SOC_DAIFMT_CBS_CFS,
        .ops = &i2s_ops,
    },
};

static struct snd_soc_card ecodec_snd_card = {
    .name = "ecodec-sound-card",
    .owner = THIS_MODULE,
    .dai_link = ecodec_board_dais,
    .num_links = ARRAY_SIZE(ecodec_board_dais),
};

static int ecodec_board_probe(struct platform_device *pdev)
{
    int ret = 0;

    ecodec_snd_card.dev = &pdev->dev;

    ret = snd_soc_register_card(&ecodec_snd_card);
    if (ret)
        panic("ecodec-sound-card: ASOC register card failed !\n");

    return ret;
}

static int ecodec_board_remove(struct platform_device *pdev)
{
    snd_soc_unregister_card(&ecodec_snd_card);

    platform_set_drvdata(pdev, NULL);

    return 0;
}

static struct platform_driver ecodec_board_driver = {
    .probe = ecodec_board_probe,
    .remove = ecodec_board_remove,
    .driver = {
        .owner = THIS_MODULE,
        .name = "ingenic-ecodec-board",
        .pm = &snd_soc_pm_ops,
    },
};

/* stop no dev release warning */
static void ecodec_board_dev_release(struct device *dev){}

struct platform_device ecodec_board_device = {
    .name = "ingenic-ecodec-board",
    .dev  = {
        .release = ecodec_board_dev_release,
    },
};

static int __init ecodec_board_init(void)
{
    int ret = platform_device_register(&ecodec_board_device);
    if (ret)
        return ret;

    return platform_driver_register(&ecodec_board_driver);
}
module_init(ecodec_board_init);

static void ecodec_board_exit(void)
{
    platform_device_unregister(&ecodec_board_device);

    platform_driver_unregister(&ecodec_board_driver);
}
module_exit(ecodec_board_exit);
MODULE_LICENSE("GPL");