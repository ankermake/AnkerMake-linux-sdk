#include <linux/interrupt.h>
#include <linux/err.h>
#include "mic.h"
#include "dmic_hal.h"
#include "i2s.h"
#include "snd.h"

int mic_resource_init(struct platform_device *pdev, struct mic_dev *mic_dev, int type)
{
	int res_no = (type == DMIC) ? 0 : 1;
	int chan_cnt = (type == DMIC) ? 1 : 2;
	struct mic *mic = (type == DMIC) ? &mic_dev->dmic : &mic_dev->amic;

	mic->mic_dev = mic_dev;
	mic->type = type;

	mic->dma = dma_param_init(pdev, res_no, chan_cnt);
	if (IS_ERR_OR_NULL(mic->dma))
		return mic->dma ? PTR_ERR(mic->dma) : -ENOMEM;
	if (type == DMIC)
		mic->dma->dma_chan[1] = mic->dma->dma_chan[0];
	return 0;
}

static int mic_pdata_init(struct mic_dev *mic_dev, struct platform_device *pdev)
{
	int ret;

	ret = mic_resource_init(pdev, mic_dev, DMIC);
	if (ret < 0)
		return ret;

	ret = mic_resource_init(pdev, mic_dev, AMIC);
	if (ret < 0)
		return ret;

	ret = dmic_init(mic_dev);
	if (ret < 0)
		return ret;

	ret = amic_init(mic_dev);
	if (ret < 0)
		return ret;
	return 0;
}

static int mic_probe(struct platform_device *pdev)
{
	int ret;
	struct mic_dev *mic_dev;

	mic_dev = kzalloc(sizeof(struct mic_dev), GFP_KERNEL);
	if(!mic_dev) {
		printk("voice dev alloc failed\n");
		return -ENOMEM;
	}
	mic_dev->dev = &pdev->dev;
	dev_set_drvdata(mic_dev->dev, mic_dev);
	mic_dev->pdev = pdev;
	mutex_init(&mic_dev->buf_lock);
	init_waitqueue_head(&mic_dev->wait_queue);

	ret = mic_pdata_init(mic_dev, pdev);
	if (ret < 0) {
		kfree(mic_dev);
		return ret;
	}

	mic_hrtimer_init(mic_dev);

	mic_sys_init(mic_dev);

	mic_set_periods_ms(NULL, mic_dev, MIC_DEFAULT_PERIOD_MS);

#ifdef CONFIG_JZ_MICCHAR_WITH_SND_I2S
	snd_codec_init(mic_dev->amic.dma);
#endif
	return 0;
}

static struct platform_driver mic_platform_driver = {
	.probe = mic_probe,
	.driver = {
		.name = "mic",
		.owner = THIS_MODULE,
	},
};

static int __init mic_init(void)
{
	return platform_driver_register(&mic_platform_driver);
}
module_init(mic_init);
