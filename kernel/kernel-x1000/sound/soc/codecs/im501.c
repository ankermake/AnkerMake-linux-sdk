#include <linux/platform_device.h>
#include <linux/slab.h>
#include <linux/module.h>
#include <sound/core.h>
#include <sound/pcm.h>
#include <sound/soc.h>
#include <sound/soc-dapm.h>

#if 0
/* im501 dapm widgets */
static const struct snd_soc_dapm_widget im501_dapm_widgets[] = {
	/* ak4951 dapm route */
	SND_SOC_DAPM_ADC("ADC", "Capture", SND_SOC_NOPM, 0, 0),
	SND_SOC_DAPM_INPUT("ADC IN"),
};

static const struct snd_soc_dapm_route im501_audio_map[] = {
	/* ADC */
	{"ADC", NULL, "ADC IN"},
};
#endif


static struct snd_soc_dai_driver im501_dai = {
	.name = "im501-hifi",
	.capture = {
		.stream_name = "Capture",
		.channels_min = 2,
		.channels_max = 2,
		.rates = SNDRV_PCM_RATE_16000 | SNDRV_PCM_RATE_48000,
		.formats = SNDRV_PCM_FMTBIT_S16_LE,
	},
	
};

static struct snd_soc_codec_driver soc_im501 = {
	//.controls =     ak4951_snd_controls,
	//.num_controls = ARRAY_SIZE(ak4951_snd_controls),
	//.dapm_widgets = im501_dapm_widgets,
	//.num_dapm_widgets = ARRAY_SIZE(im501_dapm_widgets),
	//.dapm_routes = im501_audio_map,
	//.num_dapm_routes = ARRAY_SIZE(im501_audio_map),
};



static int im501_dev_probe(struct platform_device *pdev)
{
	int res = snd_soc_register_codec(&pdev->dev,
			&soc_im501, &im501_dai, 1);
	pr_err("im501_dev_probe, register codec = %d\n", res);
	return res;
}

static int im501_dev_remove(struct platform_device *pdev)
{
	snd_soc_unregister_codec(&pdev->dev);
	return 0;
}

static struct platform_driver im501_driver = {
	.driver = {
		.name = "im501-codec",
		.owner = THIS_MODULE,
	},
	.probe = im501_dev_probe,
	.remove = im501_dev_remove,
};

module_platform_driver(im501_driver);
MODULE_DESCRIPTION("Generic IM501 driver");
MODULE_AUTHOR("Liam Girdwood <lrg@slimlogic.co.uk>");
MODULE_LICENSE("GPL");
