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

static int icodec_board_i2s_hw_params(struct snd_pcm_substream *substream, struct snd_pcm_hw_params *params)
{
    struct snd_soc_pcm_runtime *rtd = substream->private_data;
    struct snd_soc_dai *cpu_dai = rtd->cpu_dai;

    int ret = snd_soc_dai_set_sysclk(cpu_dai, SELECT_INNER_CODEC, 0, 0);
    if (ret)
        return ret;

    return 0;
}

static struct snd_soc_ops i2s_ops = {
    .hw_params = icodec_board_i2s_hw_params,
};

static int asoc_dai_link_init(struct snd_soc_pcm_runtime *rtd)
{
    return 0;
}

static struct snd_soc_dai_link icodec_board_dais[] = {
    [0] = {
        .name = "x1830 icodec",       /* 仅表示声卡信息，无实质意义 */
        .stream_name = "x1830 icodec pcm",  /* 仅表示声卡信息，无实质意义 */
        .platform_name = "ingenic-aic",
        .cpu_dai_name = "ingenic-aic",
        .init = asoc_dai_link_init,
        .codec_dai_name = "internal-codec",
        .codec_name = "ingenic-icodec",
        .dai_fmt = SND_SOC_DAIFMT_I2S|SND_SOC_DAIFMT_CBM_CFM,
        .ops = &i2s_ops,
    },
};

static struct snd_soc_card icodec_snd_card = {
    .name = "icodec-sound-card",
    .owner = THIS_MODULE,
    .dai_link = icodec_board_dais,
    .num_links = ARRAY_SIZE(icodec_board_dais),
};

static int icodec_board_probe(struct platform_device *pdev)
{
    int ret = 0;

    icodec_snd_card.dev = &pdev->dev;

    ret = snd_soc_register_card(&icodec_snd_card);
    if (ret)
        panic("icodec-sound-card: ASOC register card failed !\n");

    return ret;
}

static int icodec_board_remove(struct platform_device *pdev)
{
    snd_soc_unregister_card(&icodec_snd_card);

    platform_set_drvdata(pdev, NULL);

    return 0;
}

static struct platform_driver icodec_board_driver = {
    .probe = icodec_board_probe,
    .remove = icodec_board_remove,
    .driver = {
        .owner = THIS_MODULE,
        .name = "ingenic-icodec-board",
        .pm = &snd_soc_pm_ops,
    },
};

/* stop no dev release warning */
static void icodec_board_dev_release(struct device *dev){}

struct platform_device icodec_board_device = {
    .name = "ingenic-icodec-board",
    .dev  = {
        .release = icodec_board_dev_release,
    },
};

static int __init icodec_board_init(void)
{
    int ret = platform_device_register(&icodec_board_device);
    if (ret)
        return ret;

    return platform_driver_register(&icodec_board_driver);
}
module_init(icodec_board_init);

static void icodec_board_exit(void)
{
    platform_device_unregister(&icodec_board_device);

    platform_driver_unregister(&icodec_board_driver);
}
module_exit(icodec_board_exit);
MODULE_LICENSE("GPL");