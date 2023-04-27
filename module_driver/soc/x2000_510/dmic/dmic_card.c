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

static int asoc_dai_link_init(struct snd_soc_pcm_runtime *rtd)
{
    return 0;
}

SND_SOC_DAILINK_DEFS(dmic,
	DAILINK_COMP_ARRAY(COMP_CPU("ingenic-dmic")),
	DAILINK_COMP_ARRAY(COMP_CODEC("snd-soc-dummy", "snd-soc-dummy-dai")),
	DAILINK_COMP_ARRAY(COMP_PLATFORM("ingenic-dmic")));

static struct snd_soc_dai_link dmic_board_dais[] = {

     [0] = {
        .name = "x2000 DMIC",       /* 仅表示声卡信息，无实质意义 */
        .stream_name = "x2000 DMIC",  /* 仅表示声卡信息，无实质意义 */
        .init = asoc_dai_link_init,
        SND_SOC_DAILINK_REG(dmic),
    },
};

static struct snd_soc_card dmic_snd_card = {
    .name = "dmic-sound-card",
    .owner = THIS_MODULE,
    .dai_link = dmic_board_dais,
    .num_links = ARRAY_SIZE(dmic_board_dais),
};

static int dmic_board_probe(struct platform_device *pdev)
{
    int ret = 0;

    dmic_snd_card.dev = &pdev->dev;

    ret = snd_soc_register_card(&dmic_snd_card);
    if (ret)
        panic("******dmic-sound-card: ASOC register card failed !\n");

    return ret;
}

static int dmic_board_remove(struct platform_device *pdev)
{
    snd_soc_unregister_card(&dmic_snd_card);

    platform_set_drvdata(pdev, NULL);

    return 0;
}

static struct platform_driver dmic_board_driver = {
    .probe = dmic_board_probe,
    .remove = dmic_board_remove,
    .driver = {
        .owner = THIS_MODULE,
        .name = "ingenic-dmic-board",
        .pm = &snd_soc_pm_ops,
    },
};

/* stop no dev release warning */
static void dmic_board_dev_release(struct device *dev){}

struct platform_device dmic_board_device = {
    .name = "ingenic-dmic-board",
    .dev  = {
        .release = dmic_board_dev_release,
    },
};

int dmic_board_init(void)
{
    int ret = platform_device_register(&dmic_board_device);
    if (ret)
        return ret;

    return platform_driver_register(&dmic_board_driver);
}

void dmic_board_exit(void)
{
    platform_device_unregister(&dmic_board_device);

    platform_driver_unregister(&dmic_board_driver);
}