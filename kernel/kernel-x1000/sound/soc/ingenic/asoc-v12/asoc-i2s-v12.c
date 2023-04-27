/*
 *  sound/soc/ingenic/asoc-i2s.c
 *  ALSA Soc Audio Layer -- ingenic i2s (part of aic controller) driver
 *
 *  Copyright 2014 Ingenic Semiconductor Co.,Ltd
 *	cli <chen.li@ingenic.com>
 *
 *  This program is free software; you can redistribute  it and/or modify it
 *  under  the terms of  the GNU General  Public License as published by the
 *  Free Software Foundation;  either version 2 of the  License, or (at your
 *  option) any later version.
 */

#include <linux/init.h>
#include <linux/module.h>
#include <linux/device.h>
#include <linux/delay.h>
#include <linux/clk.h>
#include <sound/core.h>
#include <sound/pcm.h>
#include <sound/pcm_params.h>
#include <sound/initval.h>
#include <sound/soc.h>
#include <sound/soc-dai.h>
#include <linux/slab.h>
#include "asoc-aic-v12.h"
#include "aic_new.h"

static int jz_i2s_debug = 0;
module_param(jz_i2s_debug, int, 0644);
#define I2S_DEBUG_MSG(msg...)			\
	do {					\
		if (jz_i2s_debug)		\
			printk(KERN_DEBUG"I2S: " msg);	\
	} while(0)

struct jz_i2s {
	struct device *aic;
#define I2S_WRITE 0x1
#define I2S_READ  0x2
	int i2s_mode;
	struct jz_pcm_dma_params tx_dma_data;
	struct jz_pcm_dma_params rx_dma_data;
	unsigned int dai_fmt;
	unsigned int clk_div;
	unsigned int clk_freq;
	unsigned int clk_id;
	unsigned int clk_dir;
};

#define I2S_RFIFO_DEPTH 32
#define I2S_TFIFO_DEPTH 64
#define JZ_I2S_FORMATS (SNDRV_PCM_FMTBIT_S8 |  SNDRV_PCM_FMTBIT_S16_LE |	\
		SNDRV_PCM_FMTBIT_S18_3LE | SNDRV_PCM_FMTBIT_S20_3LE |	\
		SNDRV_PCM_FMTBIT_S24_LE)
#define JZ_I2S_RATE (SNDRV_PCM_RATE_8000_192000&(~SNDRV_PCM_RATE_64000))

static void dump_registers(struct device *aic)
{
	struct jz_aic *jz_aic = dev_get_drvdata(aic);

	pr_info("AIC_FR\t\t%p : 0x%08x\n", (jz_aic->vaddr_base+AICFR),jz_aic_read_reg(aic, AICFR));
	pr_info("AIC_CR\t\t%p : 0x%08x\n", (jz_aic->vaddr_base+AICCR),jz_aic_read_reg(aic, AICCR));
	pr_info("AIC_I2SCR\t%p : 0x%08x\n",(jz_aic->vaddr_base+I2SCR),jz_aic_read_reg(aic, I2SCR));
	pr_info("AIC_SR\t\t%p : 0x%08x\n", (jz_aic->vaddr_base+AICSR),jz_aic_read_reg(aic, AICSR));
	pr_info("AIC_I2SSR\t%p : 0x%08x\n",(jz_aic->vaddr_base+I2SSR),jz_aic_read_reg(aic, I2SSR));
	pr_info("AIC_I2SDIV\t%p : 0x%08x\n",(jz_aic->vaddr_base+I2SDIV),jz_aic_read_reg(aic, I2SDIV));
	pr_info("AIC_DR\t%p : 0x%08x\n",(jz_aic->vaddr_base+AICDR),jz_aic_read_reg(aic, AICDR));
	pr_info("AIC_I2SCDR\t 0x%08x\n",*(volatile unsigned int*)0xb0000060);
	return;
}

static int jz_do_set_dai_fmt(struct snd_soc_dai *dai, unsigned int fmt)
{
	struct jz_i2s *jz_i2s = dev_get_drvdata(dai->dev);
	struct device *aic = jz_i2s->aic;
    unsigned int interface = fmt & SND_SOC_DAIFMT_FORMAT_MASK;
    unsigned int clk_dir = fmt & SND_SOC_DAIFMT_MASTER_MASK;

	I2S_DEBUG_MSG("enter %s dai fmt %x\n", __func__, fmt);

    switch (interface) {
    case SND_SOC_DAIFMT_I2S:
        aic_select_i2s_fmt();
        break;
    case SND_SOC_DAIFMT_MSB:
        aic_select_i2s_msb_fmt();
        break;
    default:
        printk("aic: fmt error: %x", interface);
        return -EINVAL;
    }

    switch (clk_dir) {
    case SND_SOC_DAIFMT_CBM_CFM:
        aic_bclk_input();
        aic_frame_sync_input();
        break;
    case SND_SOC_DAIFMT_CBS_CFS:
        aic_bclk_output();
        aic_frame_sync_output();
        break;
    default:
        printk("aic: clk dir error: %x", clk_dir);
        return -EINVAL;
    }

	return 0;
}

static int jz_set_dai_fmt(struct snd_soc_dai *dai, unsigned int fmt)
{
	struct jz_i2s *jz_i2s = dev_get_drvdata(dai->dev);
    unsigned int interface = fmt & SND_SOC_DAIFMT_FORMAT_MASK;
    unsigned int clk_dir = fmt & SND_SOC_DAIFMT_MASTER_MASK;

	I2S_DEBUG_MSG("enter %s dai fmt %x\n", __func__, fmt);

    switch (interface) {
    case SND_SOC_DAIFMT_I2S:
    case SND_SOC_DAIFMT_MSB:
        break;
    default:
        printk("aic: fmt error: %x", interface);
        return -EINVAL;
    }

    switch (clk_dir) {
    case SND_SOC_DAIFMT_CBM_CFM:
    case SND_SOC_DAIFMT_CBS_CFS:
        break;
    default:
        printk("aic: clk dir error: %x", clk_dir);
        return -EINVAL;
    }

	jz_i2s->dai_fmt = fmt;

	return 0;
}

static int jz_set_sysclk(struct snd_soc_dai *dai, int clk_id,
		unsigned int freq, int dir)
{
	struct jz_i2s *jz_i2s = dev_get_drvdata(dai->dev);
	struct device *aic = jz_i2s->aic;

	I2S_DEBUG_MSG("enter %s clk_id %d req %d clk dir %d\n", __func__,
			clk_id, freq, dir);

	jz_i2s->clk_id = clk_id;
	jz_i2s->clk_dir = dir;
	jz_i2s->clk_freq = freq;

	return 0;
}

static int jz_set_clkdiv(struct snd_soc_dai *dai, int div_id, int div)
{
	struct jz_i2s *jz_i2s = dev_get_drvdata(dai->dev);
	struct device *aic = jz_i2s->aic;

	I2S_DEBUG_MSG("enter %s div_id %d div %d\n", __func__, div_id , div);

	/*BIT CLK fix 64FS*/
	/*SYS_CLK is 256, 384, 512, 768*/
	if (div != 256 && div != 384 &&
			div != 512 && div != 768)
		return -EINVAL;

	jz_i2s->clk_div = div;

	return 0;
}

static int jz_i2s_startup(struct snd_pcm_substream *substream,
		struct snd_soc_dai *dai)
{
	struct jz_i2s *jz_i2s = dev_get_drvdata(dai->dev);

	I2S_DEBUG_MSG("enter %s, substream = %s\n",
			__func__,
			(substream->stream == SNDRV_PCM_STREAM_PLAYBACK) ? "playback" : "capture");

	return 0;
}

static int jz_i2s_hw_params(struct snd_pcm_substream *substream,
				struct snd_pcm_hw_params *params, struct snd_soc_dai *dai)
{
	int channels = params_channels(params);
	int fmt_width = snd_pcm_format_width(params_format(params));
	struct jz_i2s *jz_i2s = dev_get_drvdata(dai->dev);
	struct device *aic = jz_i2s->aic;
	enum dma_slave_buswidth buswidth;
	int trigger;

	I2S_DEBUG_MSG("enter %s, substream = %s\n", __func__,
		      (substream->stream == SNDRV_PCM_STREAM_PLAYBACK) ? "playback" : "capture");
	if (!((1 << params_format(params)) & JZ_I2S_FORMATS) ||
			channels > 2) {
		dev_err(dai->dev, "hw params not inval channel %d params %x\n",
				channels, params_format(params));
		return -EINVAL;
	}

	// if (jz_i2s->i2s_mode & (1 << substream->stream)) {
	// 	printk("aic: can't config it again: %d\n", substream->stream);
    //     return -EBUSY;
	// }

	if (!jz_i2s->i2s_mode) {
		if (!jz_i2s->clk_freq) {
			printk("aic: must set clk_freq\n");
        	return -EINVAL;
		}

		if (!jz_i2s->clk_div) {
			printk("aic: must set clk_div\n");
        	return -EINVAL;
		}

        aic_init_registers();
		aic_set_div(jz_i2s->clk_div / 64);
		aic_disable_sysclk_output();
        aic_disable_clk(aic);
        msleep(20);

        aic_disable_gate_clk(aic);

        msleep(20);
        aic_set_rate(aic, jz_i2s->clk_freq);
        aic_enable_clk(aic);

        aic_enable_gate_clk(aic);

        aic_init_registers();

		aic_stop_bit_clk();

		if (jz_i2s->clk_id == JZ_I2S_INNER_CODEC) {
			aic_internal_codec_master_mode();
			aic_select_internal_codec();
			aic_bclk_input();
			aic_bclk_output();
		} else {
			aic_internal_codec_slave_mode();
			aic_select_external_codec();
			jz_do_set_dai_fmt(dai, jz_i2s->dai_fmt);
		}

		if (jz_i2s->clk_dir == SND_SOC_CLOCK_OUT)
			aic_enable_sysclk_output();

		aic_start_bit_clk();

        msleep(20);
	}

	if (substream->stream == SNDRV_PCM_STREAM_PLAYBACK) {
		enum aic_oss oss;

        if (fmt_width == 8) {
            buswidth = DMA_SLAVE_BUSWIDTH_1_BYTE;
            oss = aic_oss_8bit;
        } else if (fmt_width == 16) {
            if (channels == 1)
                buswidth = DMA_SLAVE_BUSWIDTH_2_BYTES;
            else
                buswidth = DMA_SLAVE_BUSWIDTH_4_BYTES;
            oss = aic_oss_16bit;
        } else if (fmt_width == 20) {
            buswidth = DMA_SLAVE_BUSWIDTH_4_BYTES;
            oss = aic_oss_20bit;
        } else {
            buswidth = DMA_SLAVE_BUSWIDTH_4_BYTES;
            oss = aic_oss_24bit;
        }

        if (fmt_width == 16) {
            if (channels == 1)
                aic_enable_m2s();
            else
                aic_disable_m2s();

            aic_enable_pack16();
            aic_set_tx_channel(2);
        } else {
            aic_disable_m2s();
            aic_disable_pack16();
            aic_set_tx_channel(channels);
        }

        aic_set_tx_sample_bit(oss);
        aic_set_tx_trigger_level(I2S_TFIFO_DEPTH / 2);

        aic_clear_tx_underrun();

		jz_i2s->tx_dma_data.buswidth = buswidth;
		jz_i2s->tx_dma_data.max_burst = (I2S_TFIFO_DEPTH * buswidth)/2;
		snd_soc_dai_set_dma_data(dai, substream, (void *)&jz_i2s->tx_dma_data);
	} else {
		enum aic_iss iss;

        if (fmt_width == 8) {
            buswidth = DMA_SLAVE_BUSWIDTH_1_BYTE;
            iss = aic_iss_8bit;
        } else if (fmt_width == 16) {
            buswidth = DMA_SLAVE_BUSWIDTH_2_BYTES;
            iss = aic_iss_16bit;
        } else if (fmt_width == 20) {
            buswidth = DMA_SLAVE_BUSWIDTH_4_BYTES;
            iss = aic_iss_20bit;
        } else {
            buswidth = DMA_SLAVE_BUSWIDTH_4_BYTES;
            iss = aic_iss_24bit;
        }

        aic_set_rx_sample_bit(iss);
        aic_set_rx_trigger_level(I2S_RFIFO_DEPTH / 4);

		jz_i2s->rx_dma_data.buswidth = buswidth;
		jz_i2s->rx_dma_data.max_burst = (I2S_RFIFO_DEPTH * buswidth)/2/2 /*To reduce overrun happen*/;
		snd_soc_dai_set_dma_data(dai, substream, (void *)&jz_i2s->rx_dma_data);
	}

	if (!jz_i2s->i2s_mode)
		aic_enable();

	jz_i2s->i2s_mode |= (1 << substream->stream);

	return 0;
}

static void jz_i2s_start_substream(struct snd_pcm_substream *substream,
		struct snd_soc_dai *dai)
{
	struct jz_i2s *jz_i2s = dev_get_drvdata(dai->dev);
	struct device *aic = jz_i2s->aic;
	int timeout = 150000;
	I2S_DEBUG_MSG("enter %s, substream = %s\n",
			__func__,
			(substream->stream == SNDRV_PCM_STREAM_PLAYBACK) ? "playback" : "capture");

	if (substream->stream == SNDRV_PCM_STREAM_PLAYBACK) {
		unsigned int i;
		unsigned long flags;

		aic_clear_tx_underrun();

		// aic_print_state();
		// aic_print_state();

		for (i = 0; i < 32; i++) {
			if (aic_get_tx_fifo_nums() < I2S_TFIFO_DEPTH)
				aic_write_fifo(0);
			// printk("aic: sr %d\n", aic_get_tx_fifo_nums());
		}

		// aic_print_state();

		if (aic_get_tx_fifo_nums() % 2)
			aic_write_fifo(0);

		int tx_num = aic_get_tx_fifo_nums();

		// aic_print_state();

		aic_clear_tx_underrun();
		aic_enable_replay();

		int timeout = 15000;
		while (aic_get_tx_fifo_nums()) {
			udelay(10);
			if (timeout-- == 0) {
				printk("aic: replay is not work %d %d\n", tx_num, aic_get_tx_fifo_nums());
				dump_registers(aic);
				return -EBUSY;
			}
		}
		aic_clear_tx_underrun();
		aic_enable_tx_dma();
	} else {
		aic_flush_rx_fifo();
		mdelay(1);
		aic_enable_record();
		aic_enable_rx_dma();
	}

	return;
}

static void jz_i2s_stop_substream(struct snd_pcm_substream *substream,
		struct snd_soc_dai *dai)
{
	struct jz_i2s *jz_i2s = dev_get_drvdata(dai->dev);
	struct device *aic = jz_i2s->aic;
	unsigned long timeout_jiffies;
	I2S_DEBUG_MSG("enter %s, substream = %s\n",
			__func__,
			(substream->stream == SNDRV_PCM_STREAM_PLAYBACK) ? "playback" : "capture");

	if (substream->stream == SNDRV_PCM_STREAM_PLAYBACK) {
		if (aic_tx_dma_is_enabled()) {
			/*step 1: stop dma request now*/
			aic_disable_tx_dma();
			timeout_jiffies = jiffies + msecs_to_jiffies((1000 * I2S_TFIFO_DEPTH) / 8000) * 110 / 100;
			I2S_DEBUG_MSG("i2s replay timeout %lu timeout_jiffies %lu jiffies %lu\n",
					msecs_to_jiffies((1000 * I2S_TFIFO_DEPTH) / 8000) * 110 / 100,
					timeout_jiffies, jiffies);
			/*step 2: clear tur flag*/
			aic_clear_tx_underrun();
			/*step 3: make sure no data transfer on ahb2 bus*/
			while((!aic_get_tx_underrun()) && time_before(jiffies, timeout_jiffies));
			I2S_DEBUG_MSG("i2s replay stop ok\n");
			/*step 4: safely stop replay function*/
			aic_disable_replay();
			/*step 5: clear tur flag finally*/
			aic_clear_tx_underrun();
		}
	} else {
		if (aic_rx_dma_is_enabled()) {
			/*step 1: stop dma request now*/
			aic_disable_rx_dma();
			timeout_jiffies = jiffies + msecs_to_jiffies((1000 * I2S_RFIFO_DEPTH)/ 2/ 8000) * 110 / 100;
			I2S_DEBUG_MSG("i2s record timeout %lu timeout_jiffies %lu jiffies %lu\n",
					msecs_to_jiffies((1000 * I2S_RFIFO_DEPTH) / 2/ 8000) * 110 / 100,
					timeout_jiffies, jiffies);
			/*step 2: clear ror flag*/
			aic_clear_rx_overrun();
			/*step 3: make sure no data transfer on ahb2 bus*/
			while(!aic_get_rx_overrun() && time_before(jiffies, timeout_jiffies));
			I2S_DEBUG_MSG("i2s record stop ok\n");
			/*step 4: safely stop record function*/
			aic_disable_record();
			/*step 5: clear ror flag finally*/
			aic_clear_rx_overrun();
		}
	}

	return;
}

static int jz_i2s_trigger(struct snd_pcm_substream *substream, int cmd, struct snd_soc_dai *dai)
{
#ifndef CONFIG_SND_ASOC_DMA_HRTIMER
	struct jz_pcm_runtime_data *prtd = substream->runtime->private_data;
#endif
	I2S_DEBUG_MSG("enter %s, substream = %s cmd = %d\n",
		      __func__,
		      (substream->stream == SNDRV_PCM_STREAM_PLAYBACK) ? "playback" : "capture",
		      cmd);
	switch (cmd) {
	case SNDRV_PCM_TRIGGER_START:
	case SNDRV_PCM_TRIGGER_RESUME:
	case SNDRV_PCM_TRIGGER_PAUSE_RELEASE:
#ifndef CONFIG_SND_ASOC_DMA_HRTIMER
		if (atomic_read(&prtd->stopped_pending))
			return -EPIPE;
#endif
		jz_i2s_start_substream(substream, dai);
		break;
	case SNDRV_PCM_TRIGGER_STOP:
	case SNDRV_PCM_TRIGGER_SUSPEND:
	case SNDRV_PCM_TRIGGER_PAUSE_PUSH:
#ifndef CONFIG_SND_ASOC_DMA_HRTIMER
		if (atomic_read(&prtd->stopped_pending))
			return 0;
#endif
		jz_i2s_stop_substream(substream, dai);
		break;
	}
	return 0;
}

static void jz_i2s_shutdown(struct snd_pcm_substream *substream,
		struct snd_soc_dai *dai) {
	struct jz_i2s *jz_i2s = dev_get_drvdata(dai->dev);
	struct device *aic = jz_i2s->aic;
	I2S_DEBUG_MSG("enter %s, substream = %s\n",
			__func__,
			(substream->stream == SNDRV_PCM_STREAM_PLAYBACK) ? "playback" : "capture");

	jz_i2s_stop_substream(substream, dai);

	if (substream->stream == SNDRV_PCM_STREAM_PLAYBACK)
		jz_i2s->i2s_mode &= ~I2S_WRITE;
	else
		jz_i2s->i2s_mode &= ~I2S_READ;

	jz_i2s->i2s_mode &= ~(1 << substream->stream);

	if (!jz_i2s->i2s_mode)
		aic_disable();

	return;
}

static int jz_i2s_probe(struct snd_soc_dai *dai)
{
	struct jz_i2s *jz_i2s = dev_get_drvdata(dai->dev);
	struct device *aic = jz_i2s->aic;
	I2S_DEBUG_MSG("enter %s\n", __func__);

	return 0;
}

static struct snd_soc_dai_ops jz_i2s_dai_ops = {
	.startup	= jz_i2s_startup,
	.trigger 	= jz_i2s_trigger,
	.hw_params 	= jz_i2s_hw_params,
	.shutdown	= jz_i2s_shutdown,
	.set_fmt	= jz_set_dai_fmt,
	.set_sysclk	= jz_set_sysclk,
	.set_clkdiv	= jz_set_clkdiv,
};

#define jz_i2s_suspend	NULL
#define jz_i2s_resume	NULL
static struct snd_soc_dai_driver jz_i2s_dai = {
		.probe   = jz_i2s_probe,
		.suspend = jz_i2s_suspend,
		.resume  = jz_i2s_resume,
		.playback = {
			.channels_min = 1,
			.channels_max = 2,
			.rates = JZ_I2S_RATE,
			.formats = JZ_I2S_FORMATS,
		},
		.capture = {
			.channels_min = 2,
			.channels_max = 2,
			.rates = JZ_I2S_RATE,
			.formats = JZ_I2S_FORMATS,
		},
		.ops = &jz_i2s_dai_ops,
};

static ssize_t jz_i2s_regs_show(struct device *dev,
		struct device_attribute *attr, char *buf)
{
	struct jz_i2s *jz_i2s = dev_get_drvdata(dev);
	dump_registers(jz_i2s->aic);
	return 0;
}

static struct device_attribute jz_i2s_sysfs_attrs[] = {
	__ATTR(i2s_regs, S_IRUGO, jz_i2s_regs_show, NULL),
};

static const struct snd_soc_component_driver jz_i2s_component = {
	.name		= "jz-i2s",
};

static int jz_i2s_platfrom_probe(struct platform_device *pdev)
{
	struct jz_aic_subdev_pdata *pdata = dev_get_platdata(&pdev->dev);
	struct jz_i2s *jz_i2s;
	int i = 0, ret;

	jz_i2s = devm_kzalloc(&pdev->dev, sizeof(struct jz_i2s), GFP_KERNEL);
	if (!jz_i2s)
		return -ENOMEM;

	jz_i2s->aic = pdev->dev.parent;
	jz_i2s->i2s_mode = 0;
	jz_i2s->tx_dma_data.dma_addr = pdata->dma_base + AICDR;
	jz_i2s->rx_dma_data.dma_addr = pdata->dma_base + AICDR;
	platform_set_drvdata(pdev, (void *)jz_i2s);

	for (; i < ARRAY_SIZE(jz_i2s_sysfs_attrs); i++) {
		ret = device_create_file(&pdev->dev, &jz_i2s_sysfs_attrs[i]);
		if (ret)
			dev_warn(&pdev->dev,"attribute %s create failed %x",
					attr_name(jz_i2s_sysfs_attrs[i]), ret);
	}

	ret = snd_soc_register_component(&pdev->dev, &jz_i2s_component,
					 &jz_i2s_dai, 1);
	if (!ret)
		dev_info(&pdev->dev, "i2s platform probe success\n");
	return ret;
}

static int jz_i2s_platfom_remove(struct platform_device *pdev)
{
	int i;
	for (i = 0; i < ARRAY_SIZE(jz_i2s_sysfs_attrs); i++)
		device_remove_file(&pdev->dev, &jz_i2s_sysfs_attrs[i]);
	platform_set_drvdata(pdev, NULL);
	snd_soc_unregister_component(&pdev->dev);
	return 0;
}

static struct platform_driver jz_i2s_plat_driver = {
	.probe  = jz_i2s_platfrom_probe,
	.remove = jz_i2s_platfom_remove,
	.driver = {
		.name = "jz-asoc-aic-i2s",
		.owner = THIS_MODULE,
	},
};

static int jz_i2s_init(void)
{
        return platform_driver_register(&jz_i2s_plat_driver);
}
module_init(jz_i2s_init);

static void jz_i2s_exit(void)
{
	platform_driver_unregister(&jz_i2s_plat_driver);
}
module_exit(jz_i2s_exit);

MODULE_AUTHOR("cli <chen.li@ingenic.com>");
MODULE_DESCRIPTION("JZ AIC I2S SoC Interface");
MODULE_LICENSE("GPL");
MODULE_ALIAS("platform:jz-aic-i2s");
