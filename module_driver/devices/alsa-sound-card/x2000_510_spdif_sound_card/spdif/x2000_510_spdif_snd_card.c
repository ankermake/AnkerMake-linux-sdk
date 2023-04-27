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

#include <spdif.h>

static int spdif_hw_params(struct snd_pcm_substream *substream, struct snd_pcm_hw_params *params)
{
    struct snd_soc_pcm_runtime *rtd = asoc_substream_to_rtd(substream);
    struct snd_soc_dai *cpu_dai = asoc_rtd_to_cpu(rtd, 0);
    int is_capture = substream->stream == SNDRV_PCM_STREAM_CAPTURE;
    int clk_ratio = substream->stream ? 1280 : 32 * 2 * params_channels(params);  //fixed 2channels
    int clk_id = is_capture ? SPDIF_IN : SPDIF_OUT;
    int ret = 0;
    uint spdif_out_clk_div = 0, spdif_in_clk_div = 0;
    int freq = params_rate(params) * 768;
    spdif_out_clk_div = (freq + ((params_rate(params) * clk_ratio) - 1))
        / (params_rate(params) * clk_ratio);
    spdif_out_clk_div &= ~0x1;
    spdif_in_clk_div = (freq + ((params_rate(params) * clk_ratio) - 1))
        / (params_rate(params) * clk_ratio);
    spdif_in_clk_div &= ~0x1;

    ret = snd_soc_dai_set_sysclk(cpu_dai, clk_id, freq, SPDIF_OUTPUT);
    if (ret)
        return ret;
    ret = snd_soc_dai_set_clkdiv(cpu_dai, substream->stream,!(substream->stream) ? spdif_out_clk_div : spdif_in_clk_div);
    if (ret)
        return ret;

    return 0;
};

static struct snd_soc_ops spdif_ops = {
    .hw_params = spdif_hw_params,
};

static int asoc_dai_link_init(struct snd_soc_pcm_runtime *rtd)
{
    return 0;
}

SND_SOC_DAILINK_DEFS(link1,
	DAILINK_COMP_ARRAY(COMP_CPU("ingenic-spdif")),
	DAILINK_COMP_ARRAY(COMP_CODEC("snd-soc-dummy", "snd-soc-dummy-dai")),
	DAILINK_COMP_ARRAY(COMP_PLATFORM("ingenic-spdif")));

static struct snd_soc_dai_link spdif_board_dais[] = {

     [0] = {
        .name = "x2000 SPDIF",       /* 仅表示声卡信息，无实质意义 */
        .stream_name = "x2000 SPDIF",  /* 仅表示声卡信息，无实质意义 */
        .init = asoc_dai_link_init,
        .dai_fmt = SND_SOC_DAIFMT_I2S|SND_SOC_DAIFMT_CBS_CFS,
        .ops = &spdif_ops,
		SND_SOC_DAILINK_REG(link1),
    },
};

static struct snd_soc_card spdif_snd_card = {
    .name = "spdif-sound-card",
    .owner = THIS_MODULE,
    .dai_link = spdif_board_dais,
    .num_links = ARRAY_SIZE(spdif_board_dais),
};

static int spdif_board_probe(struct platform_device *pdev)
{
    int ret = 0;

    spdif_snd_card.dev = &pdev->dev;

    ret = snd_soc_register_card(&spdif_snd_card);
    if (ret)
        panic("******spdif-sound-card: ASOC register card failed !\n");

    return ret;
}

static int spdif_board_remove(struct platform_device *pdev)
{
    snd_soc_unregister_card(&spdif_snd_card);

    platform_set_drvdata(pdev, NULL);

    return 0;
}

static struct platform_driver spdif_board_driver = {
    .probe = spdif_board_probe,
    .remove = spdif_board_remove,
    .driver = {
        .owner = THIS_MODULE,
        .name = "ingenic-spdif-board",
        .pm = &snd_soc_pm_ops,
    },
};

/* stop no dev release warning */
static void spdif_board_dev_release(struct device *dev){}

struct platform_device spdif_board_device = {
    .name = "ingenic-spdif-board",
    .dev  = {
        .release = spdif_board_dev_release,
    },
};

static int __init spdif_board_init(void)
{
    int ret = platform_device_register(&spdif_board_device);
    if (ret)
        return ret;

    return platform_driver_register(&spdif_board_driver);
}
module_init(spdif_board_init);

static void spdif_board_exit(void)
{
    platform_device_unregister(&spdif_board_device);

    platform_driver_unregister(&spdif_board_driver);
}
module_exit(spdif_board_exit);
MODULE_LICENSE("GPL");