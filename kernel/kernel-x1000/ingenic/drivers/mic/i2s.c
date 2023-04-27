/**
 * xb_snd_i2s.c
 *
 * cli <chen.li@ingenic.cn>
 *
 * 2018
 *
 */
/*#define DEBUG*/
#include <linux/init.h>
#include <linux/delay.h>
#include <linux/workqueue.h>
#include <linux/sched.h>
#include <linux/interrupt.h>
#include <linux/platform_device.h>
#include <linux/kernel.h>
#include <linux/proc_fs.h>
#include <linux/clk.h>
#include <linux/vmalloc.h>
#include <linux/string.h>
#include <linux/sound.h>
#include <linux/slab.h>
#include <sound/core.h>
#include <linux/switch.h>
#include <linux/dma-mapping.h>
#include <linux/soundcard.h>
#include <linux/wait.h>
#include "i2s.h"
#include "mic.h"
#include "dma.h"

static DEFINE_SPINLOCK(i2s_lock);

struct i2s_device {
	struct clk * i2s_clk;
	struct clk * aic_clk;
	void __iomem * iomem;
};
static struct i2s_device *g_i2s_dev = NULL;
static void inline i2s_writel(unsigned int reg, unsigned int val)
{
	writel(val, g_i2s_dev->iomem + reg);
}

static unsigned int inline i2s_readl(unsigned int reg)
{
	return readl(g_i2s_dev->iomem + reg);
}

static void inline i2s_update(unsigned int reg, unsigned int mask, unsigned int val)
{
	unsigned int value = readl(g_i2s_dev->iomem + reg);
	value &= ~mask;
	value |= (val & mask);
	writel(value, g_i2s_dev->iomem + reg);
}

void dump_i2s_reg(void)
{
	int i;
	unsigned int reg_addr[] = {
		AICFR,AICCR,I2SCR,AICSR,I2SSR,I2SDIV
	};
	for (i = 0; i < ARRAY_SIZE(reg_addr); i++) {
		printk("##### i2s reg0x%08x = 0x%08x.\n",
				reg_addr[i], i2s_readl(reg_addr[i]));
	}
}
EXPORT_SYMBOL_GPL(dump_i2s_reg);

void codec_icdc_reset(void)
{
	unsigned long flags;
	spin_lock_irqsave(&i2s_lock, flags);
	i2s_update(AICFR, AICFR_ICDC_RST, AICFR_ICDC_RST);
	mdelay(10);
	i2s_update(AICFR, AICFR_ICDC_RST, 0);
	spin_unlock_irqrestore(&i2s_lock, flags);
}
EXPORT_SYMBOL_GPL(codec_icdc_reset);

static int __i2s_clk_switch(bool on) {
	static int enable_count = 0;
	if (on) {
		enable_count++;
		if (enable_count != 1)
			return 0;
		clk_enable(g_i2s_dev->i2s_clk);
		clk_enable(g_i2s_dev->aic_clk);
		mdelay(1);
		i2s_update(AICFR, AICFR_ENB, AICFR_ENB);
		pr_debug("i2s enable\n");
	} else {
		enable_count--;
		if (enable_count)
			return 0;
		i2s_update(AICFR, AICFR_ENB, 0);
		clk_disable(g_i2s_dev->aic_clk);
		clk_disable(g_i2s_dev->i2s_clk);
		pr_debug("i2s disable\n");
	}
	return 0;
}

int i2s_clk_switch(bool on)
{
	unsigned long flags;
	int ret;
	spin_lock_irqsave(&i2s_lock, flags);
	ret = __i2s_clk_switch(on);
	spin_unlock_irqrestore(&i2s_lock, flags);
	return ret;
}
EXPORT_SYMBOL_GPL(i2s_clk_switch);

void i2s_enable_playback_dma(void)
{
	unsigned long flags;
	unsigned long i, timeout = 150000;

	spin_lock_irqsave(&i2s_lock, flags);
	__i2s_clk_switch(1);
	for (i = 0; i < 16; i++) {
		i2s_writel(AICDR, 0);
		i2s_writel(AICDR, 0);
	}
	i2s_update(AICSR, AICSR_TUR, 0);
	i2s_update(AICCR, AICCR_ERPL, AICCR_ERPL);
	while (!(i2s_readl(AICSR) & AICSR_TUR) && (--timeout));
	i2s_update(AICCR, AICCR_TDMS, AICCR_TDMS);
	spin_unlock_irqrestore(&i2s_lock, flags);
}
EXPORT_SYMBOL_GPL(i2s_enable_playback_dma);

void i2s_disable_playback_dma(void)
{
	unsigned long flags, timeout_jiffies;
	spin_lock_irqsave(&i2s_lock, flags);
	i2s_update(AICSR, AICSR_TUR, 0);
	i2s_update(AICCR, AICCR_TDMS, 0);
	timeout_jiffies = jiffies + msecs_to_jiffies((1000 * I2S_TX_FIFO_DEPTH) / 8000) * 110 / 100;
	while (!(i2s_readl(AICSR) & AICSR_TUR) && time_before(jiffies, timeout_jiffies));
	i2s_update(AICCR, AICCR_TFLUSH, AICCR_TFLUSH);
	mdelay(1);
	i2s_update(AICCR, AICCR_ERPL, 0);
	i2s_update(AICSR, AICSR_TUR, 0);
	__i2s_clk_switch(0);
	spin_unlock_irqrestore(&i2s_lock, flags);
}
EXPORT_SYMBOL_GPL(i2s_disable_playback_dma);

void i2s_enable_capture_dma(void)
{
	unsigned long flags, aicsr;
	spin_lock_irqsave(&i2s_lock, flags);
	__i2s_clk_switch(1);
	aicsr = i2s_readl(AICSR);
	if (AICSR_RFL(aicsr)) {
		pr_debug("before AICSR %lu(%lx)\n", AICSR_RFL(aicsr),aicsr);
		i2s_update(AICCR, AICCR_RFLUSH, AICCR_RFLUSH);
		udelay(21); /*at 48k sample rate*/
		aicsr = i2s_readl(AICSR);
		if (AICSR_RFL(aicsr))
			printk("after AICSR %lu(%lx)\n", AICSR_RFL(aicsr),aicsr);
	}
	i2s_update(AICCR, AICCR_EREC|AICCR_RDMS, AICCR_EREC|AICCR_RDMS);
	spin_unlock_irqrestore(&i2s_lock, flags);
}
EXPORT_SYMBOL_GPL(i2s_enable_capture_dma);

void i2s_disable_capture_dma(void)
{
	unsigned long flags, timeout_jiffies;
	spin_lock_irqsave(&i2s_lock, flags);
	i2s_update(AICSR, AICSR_ROR, 0);
	i2s_update(AICCR, AICCR_RDMS, 0);
	timeout_jiffies = jiffies + msecs_to_jiffies((1000 * I2S_RX_FIFO_DEPTH) / 8000 / 2) * 110 / 100;
	while (!(i2s_readl(AICSR) & AICSR_ROR) && time_before(jiffies, timeout_jiffies));
	i2s_update(AICCR, AICCR_EREC, 0);
	i2s_update(AICCR, AICCR_RFLUSH, AICCR_RFLUSH);
	udelay(21);	/*at 48k sample rate*/
	__i2s_clk_switch(0);
	spin_unlock_irqrestore(&i2s_lock, flags);
}
EXPORT_SYMBOL_GPL(i2s_disable_capture_dma);

#ifndef CONFIG_JZ_MICCHAR_I2S_DEF_MCLK_RATE
#define CONFIG_JZ_MICCHAR_I2S_DEF_MCLK_RATE (48000 * 256)
#endif
static int i2s_device_init(struct dma_param *dma)
{
	int rx_trigger, tx_trigger;
	unsigned long flags;
	static int i2s_inited = false;
	struct clk *i2s_clk = clk_get(NULL, "cgu_i2s");
	struct clk *aic_clk;

	if (IS_ERR(i2s_clk)) {
		pr_err("cgu i2s clk get failed! %ld\n", PTR_ERR(i2s_clk));
		return PTR_ERR(i2s_clk);
	}
	aic_clk = clk_get(NULL, "aic");
	if (IS_ERR(aic_clk)) {
		clk_put(i2s_clk);
		pr_err("aic clk get failed! %ld\n", PTR_ERR(aic_clk));
		return PTR_ERR(aic_clk);
	}


	spin_lock_irqsave(&i2s_lock, flags);
	if (i2s_inited) {
		spin_unlock_irqrestore(&i2s_lock, flags);
		clk_put(i2s_clk);
		clk_put(aic_clk);
		return 0;
	}

	g_i2s_dev = kzalloc(sizeof(*g_i2s_dev), GFP_ATOMIC);
	if(!g_i2s_dev) {
		pr_err("failed to alloc i2s device\n");
		spin_unlock_irqrestore(&i2s_lock, flags);
		clk_put(i2s_clk);
		clk_put(aic_clk);
		return -ENOMEM;
	}
	g_i2s_dev->i2s_clk = i2s_clk;
	g_i2s_dev->aic_clk = aic_clk;
	clk_set_rate(g_i2s_dev->i2s_clk, CONFIG_JZ_MICCHAR_I2S_DEF_MCLK_RATE);
	g_i2s_dev->iomem = dma->iomem;
	dma->tx_buswidth = DMA_SLAVE_BUSWIDTH_2_BYTES;
	dma->rx_buswidth = DMA_SLAVE_BUSWIDTH_2_BYTES;
	dma->tx_maxburst = I2S_TX_FIFO_DEPTH * dma->tx_buswidth / 2;
	dma->rx_maxburst = I2S_RX_FIFO_DEPTH * dma->rx_buswidth / 2;
	dma->dma_addr = dma->io_res->start + AICDR;
	tx_trigger = I2S_TX_FIFO_DEPTH - (dma->tx_maxburst/dma->tx_buswidth)/2;
	rx_trigger = (dma->rx_maxburst/dma->tx_buswidth/2) - 1;

	__i2s_clk_switch(1);
	i2s_update(AICFR, AICFR_RST|AICFR_ENB, AICFR_RST);
	mdelay(1);
	i2s_update(I2SCR, I2SCR_STPBK, I2SCR_STPBK);
	if (IS_ENABLED(CONFIG_JZ_MICCHAR_I2S_ICDC)) {
		i2s_writel(AICFR, AICFR_ICDC|AICFR_CDC_MASTER|AICFR_AUSEL|AICFR_LSMP|	\
				AICFR_TFTH(tx_trigger)|AICFR_RFTH(rx_trigger));
		i2s_writel(I2SCR, I2SCR_RFIRST|I2SCR_STPBK);
	} else {
		if (IS_ENABLED(CONFIG_JZ_MICCHAR_I2S_MASTER))
			i2s_writel(AICFR, AICFR_AUSEL|AICFR_LSMP|AICFR_SYNCD|	\
					AICFR_BCKD|AICFR_TFTH(tx_trigger)| \
					AICFR_RFTH(rx_trigger));
		else
			i2s_writel(AICFR, AICFR_AUSEL|AICFR_LSMP|	\
					AICFR_TFTH(tx_trigger)| AICFR_RFTH(rx_trigger));
		i2s_writel(I2SCR, I2SCR_RFIRST|I2SCR_ESCLK|I2SCR_STPBK);
	}
	i2s_writel(AICCR, AICCR_CHANNEL_STEREO|AICCR_OSS_16BIT|AICCR_ISS_16BIT);
	i2s_update(I2SCR, I2SCR_STPBK, 0);
	__i2s_clk_switch(0);
	i2s_inited = true;
	spin_unlock_irqrestore(&i2s_lock, flags);
	return 0;
}

int i2s_init(struct platform_device *pdev, struct mic *mic)
{
	return i2s_device_init(mic->dma);
}
EXPORT_SYMBOL_GPL(i2s_init);

#ifdef DEBUG

/*
 * DEBUG
 */
static unsigned int i2s_reg_addr[] = {
	AICFR,AICCR,I2SCR,AICSR,I2SSR,I2SDIV
};

static int i2s_regs_set(const char *val, const struct kernel_param *kp)
{
	const char* reg = NULL;
	unsigned long i2sreg, value;
	int i;
	reg = val;
	val = strstr(val, ",");
	if (val == NULL)
		return 0;
	val++;
	i2sreg = simple_strtoul(reg, NULL, 16);
	for (i = 0; i < ARRAY_SIZE(i2s_reg_addr); i++)
		if (i2sreg == i2s_reg_addr[i])
			break;
	if (i == ARRAY_SIZE(i2s_reg_addr))
		return 0;
	value = simple_strtoul(val, NULL, 16);
	i2s_writel(i2sreg, value);
	return 0;
}

static int i2s_regs_get(char *buffer, const struct kernel_param *kp)
{
	int i, pos = 0;
	for (i = 0; i < ARRAY_SIZE(i2s_reg_addr); i++)
		pos += snprintf(buffer + pos, 4096 - pos,
				"#####i2s reg0x%02x = 0x%08x.\n",
				i2s_reg_addr[i], i2s_readl(i2s_reg_addr[i]));
	return pos;
}

static struct kernel_param_ops i2s_regs_param_ops = {
	.set = i2s_regs_set,
	.get = i2s_regs_get,
};
module_param_cb(i2s_reg, &i2s_regs_param_ops, NULL, 0644);
#endif
