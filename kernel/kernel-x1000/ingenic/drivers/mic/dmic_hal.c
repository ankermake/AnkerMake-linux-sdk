#include <linux/kernel.h>
#include "dmic_hal.h"

#ifdef DEBUG
static void *dmic_iomem;
#endif

void dmic_set_gain(struct mic_dev *mic_dev, int gain)
{
	struct mic *dmic = &mic_dev->dmic;
	printk("entry: %s, set: %d\n", __func__, gain);
	if (gain < 0)
		gain = 0;
	if (gain > 15 * 3)
		gain = 15 * 3;
	dmic->gain = gain;
	gain /= 3;
	__dmic_set_gcr(mic_dev, gain);
}

void dmic_get_gain_range(struct gain_range *range)
{
	range->min_dB = 0;
	range->max_dB = 15 * 3;
}

int dmic_init(struct mic_dev *mic_dev) {

	struct clk *dmic_clk;

#ifdef DEBUG
	dmic_iomem = mic_dev->dmic.dma->iomem;
#endif
	dmic_clk = clk_get(mic_dev->dev,"dmic");
	if (IS_ERR(dmic_clk)) {
		dev_err(mic_dev->dev, "Failed to get clk dmic\n");
		return -ENODEV;
	}
	clk_enable(dmic_clk);

	__dmic_reset(mic_dev);
	while(__dmic_get_reset(mic_dev));
	dmic_disable(mic_dev);

	/**
	 * format
	 */
	__dmic_set_sr_16k(mic_dev);
	__dmic_set_chnum(mic_dev, 3);
	__dmic_unpack_msb(mic_dev);
	__dmic_unpack_dis(mic_dev);
	__dmic_enable_pack(mic_dev);

	__dmic_enable_hpf1(mic_dev);
	__dmic_mask_all_int(mic_dev);
	/**
	 * for dma request
	 */
	__dmic_enable_rdms(mic_dev);
	__dmic_set_request(mic_dev, 32);
	__dmic_set_gcr(mic_dev, 10);
	__dmic_disable_sw_lr(mic_dev);
	__dmic_enable_lp(mic_dev);
	mic_dev->dmic.dma->dma_addr = mic_dev->dmic.dma->io_res->start + DMICDR;
	mic_dev->dmic.dma->rx_maxburst = 128;
	mic_dev->dmic.dma->rx_buswidth = DMA_SLAVE_BUSWIDTH_4_BYTES;
	dmic_enable(mic_dev);

	return 0;
}

void dmic_enable(struct mic_dev *mic_dev)
{
	__dmic_enable(mic_dev);
}

void dmic_disable(struct mic_dev *mic_dev)
{
	__dmic_disable(mic_dev);
}


#ifdef DEBUG
/*
 * DEBUG
 */
static void *dmic_iomem = NULL;

static unsigned long dmic_readl(unsigned long reg)
{
	return readl(dmic_iomem + reg);
}
static void dmic_writel(unsigned long reg, unsigned long val)
{
	writel(val, dmic_iomem + reg);
}

static unsigned long dmic_reg_attr[] = {
	DMICCR0, DMICGCR, DMICIMR ,DMICINTCR,
	DMICTRICR, DMICTHRH, DMICTHRL, DMICTRIMMAX,
	DMICTRINMAX, DMICDR, DMICFTHR,DMICFSR,
	DMICCGDIS
};

static int dmic_regs_set(const char *val, const struct kernel_param *kp)
{
	const char* reg = NULL;
	int i;
	unsigned long dmicreg, value;
	reg = val;
	val = strstr(val, ",");
	if (val == NULL)
		return 0;
	val++;
	dmicreg = simple_strtoul(reg, NULL, 16);
	for (i = 0; i < ARRAY_SIZE(dmic_reg_attr); i++)
		if (dmicreg == dmic_reg_attr[i])
			break;
	if (i == ARRAY_SIZE(dmic_reg_attr))
		return 0;
	value = simple_strtoul(val, NULL, 16);
	dmic_writel(dmicreg, value);
	return 0;
}

static int dmic_regs_get(char *buffer, const struct kernel_param *kp)
{
	int i, pos = 0;
	for (i = 0; i < ARRAY_SIZE(dmic_reg_attr); i++)
		pos += snprintf(buffer + pos, 4096 - pos,
				"#####i2s reg0x%02x = 0x%08x.\n",
				(unsigned)dmic_reg_attr[i], (unsigned)dmic_readl(dmic_reg_attr[i]));
	return pos;
}

static struct kernel_param_ops dmic_regs_param_ops = {
	.set = dmic_regs_set,
	.get = dmic_regs_get,
};
module_param_cb(dmic_reg, &dmic_regs_param_ops, NULL, 0644);
#endif
