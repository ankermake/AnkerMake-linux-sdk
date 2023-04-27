#include <linux/spinlock.h>
#include <linux/slab.h>
#include <linux/delay.h>
#include <asm/io.h>
#include <linux/gpio.h>
#include <mach/jzsnd.h>
#include "hw.h"
#include "x1830_codec.h"

static int adc_hpf_en = 1;
module_param(adc_hpf_en, int, 0644);

#define SPEAKER_AWAYS_ON

struct icodec {
	struct mutex	io_mutex;		/*codec hw io lock, Note codec cannot opt in irq context*/
	void * __iomem	mapped_base;		/*vir addr*/
	int hpout_vol;
	int mic_vol;
	int dac_enabled:1;
	int adc_enabled:1;
};

static struct icodec *icodec = NULL;

/*
 *	REG ACCESS
 */
static int icodec_writel(unsigned int reg, unsigned int value)
{
	unsigned int val1;
	int timeout = 0x5;
	writel(value, icodec->mapped_base + reg);
recheck:
	val1 = readl(icodec->mapped_base + reg);
	if (value != val1) {
		if (!(--timeout)) {
			printk("icodec write 0x%p:(0x%02x->0x%02x) failed\n",
					icodec->mapped_base + reg, value, val1);
			return 0;
		}
		goto recheck;
	}
#ifdef DEBUG
	printk("icodec write 0x%p:(0x%02x->0x%02x) ok\n",
			icodec->mapped_base + reg, value, val1);
#endif
	return 0;
}

static unsigned int icodec_readl(unsigned int reg)
{
	unsigned int val;
	val = readl(icodec->mapped_base + reg);
	return val;
}

static unsigned int icodec_update(unsigned int reg,
		unsigned int mask, unsigned int value)
{
	unsigned int val;
	val = icodec_readl(reg);
	val &= ~mask;
	val |= (value & mask);
	return icodec_writel(reg, val);
}

int codec_set_dac_vol(int val)
{
	mutex_lock(&icodec->io_mutex);
	icodec->hpout_vol = val;
	if (icodec->dac_enabled)
		icodec_update(ICDC_CHCR, ICDC_CHCR_HPOUTGAIN_MSK, ICDC_CHCR_HPOUTGAIN(val));
	mutex_unlock(&icodec->io_mutex);
	return 0;
}

int codec_get_dac_vol(void)
{
	return icodec->hpout_vol;
}

int codec_set_adc_vol(int val)
{
	mutex_lock(&icodec->io_mutex);
	icodec->mic_vol = val;
	if (icodec->adc_enabled)
		icodec_update(ICDC_CACR2, ICDC_CACR2_ALCGAIN_MSK,
				ICDC_CACR2_ALCGAIN(val));
	mutex_unlock(&icodec->io_mutex);
	return 0;
}

int codec_get_adc_vol(void)
{
	return icodec->mic_vol;
}

static int __codec_dac_power(bool on)
{
	mutex_lock(&icodec->io_mutex);
	if (on && !icodec->dac_enabled) {
		icodec_update(ICDC_CAACR, ICDC_CACCR_ADCSRCEN, ICDC_CACCR_ADCSRCEN);
		msleep(10);
		icodec_update(ICDC_CAR, ICDC_CAR_DACREFVOLBUIFEN, ICDC_CAR_DACREFVOLBUIFEN);
		msleep(10);
		icodec_update(ICDC_CAR, ICDC_CAR_DACREFVOLEN, ICDC_CAR_DACREFVOLEN);
		msleep(10);
		icodec_update(ICDC_CAR, ICDC_CAR_DACCLKEN, ICDC_CAR_DACCLKEN);
		msleep(10);
		icodec_update(ICDC_CAR, ICDC_CAR_DACEN, ICDC_CAR_DACEN);
		msleep(10);
		icodec_update(ICDC_CAR, ICDC_CAR_DACINIT_N, ICDC_CAR_DACINIT_N);
		msleep(10);
		icodec_update(ICDC_CHCR, ICDC_CHCR_HPOUTPOP_MSK, ICDC_CHCR_HPOUTPOP_WORK);
		msleep(10);
		icodec_update(ICDC_CHR, ICDC_CHR_HPOUTEN, ICDC_CHR_HPOUTEN);
		msleep(10);
		icodec_update(ICDC_CHR, ICDC_CHR_HPOUTINIT_N, ICDC_CHR_HPOUTINIT_N);
		msleep(10);
		icodec_update(ICDC_CMICCR, ICDC_CMICCR_MICMUTE_N, ICDC_CMICCR_MICMUTE_N);
		msleep(10);
		icodec_update(ICDC_CHCR, ICDC_CHCR_HPOUTGAIN_MSK, ICDC_CHCR_HPOUTGAIN(icodec->hpout_vol));
		msleep(10);
#ifndef SPEAKER_AWAYS_ON
		jz_speaker_enable(true);
#endif
		icodec_update(ICDC_CHR, ICDC_CHR_HPOUTMUTE_N, ICDC_CHR_HPOUTMUTE_N);
		msleep(10);
		icodec->dac_enabled = 1;
	} else if (!on && icodec->dac_enabled) {
		jz_speaker_enable(false);
		msleep(10);
		icodec_update(ICDC_CHR, ICDC_CHR_HPOUTMUTE_N, 0);
		msleep(10);
		icodec_update(ICDC_CAR, ICDC_CAR_DACEN|ICDC_CAR_DACCLKEN|ICDC_CAR_DACREFVOLEN|ICDC_CAR_DACINIT_N, 0);
		msleep(10);
		icodec->dac_enabled = 0;
	}
	mutex_unlock(&icodec->io_mutex);
	return 0;
}

int codec_dac_power(bool on)
{
#ifndef SPEAKER_AWAYS_ON
	return __codec_dac_power(on);
#else
	if (on)
		jz_speaker_enable(true);
	return 0;
#endif
}

static int __codec_adc_power(bool on)
{
	mutex_lock(&icodec->io_mutex);
	if (on && !icodec->adc_enabled) {
		icodec_update(ICDC_CACR, ICDC_CACR_ADCEN, ICDC_CACR_ADCEN);
		msleep(10);
		icodec_update(ICDC_CAACR, ICDC_CACCR_ADCSRCEN, ICDC_CACCR_ADCSRCEN);
		msleep(10);
		/* datasheet said set 0 enable, but the source code set 1*/
		icodec_update(ICDC_CAMPCR, ICDC_CAMPCR_ADCREFVOLEN, ICDC_CAMPCR_ADCREFVOLEN);
		msleep(10);
		icodec_update(ICDC_CAACR, BIT(4), BIT(4));
		msleep(10);
		icodec_update(ICDC_CMICCR, ICDC_CMICCR_ALCMUTE_N, ICDC_CMICCR_ALCMUTE_N);	/*enable alc mode*/
		msleep(10);
		icodec_update(ICDC_CAMPCR, ICDC_CAMPCR_ADCLCKEN, ICDC_CAMPCR_ADCLCKEN);	/* enable ADC clk*/
		msleep(10);
		icodec_update(ICDC_CAACR, BIT(3), BIT(3));
		msleep(10);
		icodec_update(ICDC_CAMPCR, BIT(4), BIT(4));
		msleep(10);
		icodec_update(ICDC_CMICCR, ICDC_CMICCR_MICEN, ICDC_CMICCR_MICEN);	/*enable BST mode*/
		msleep(10);
		icodec_update(ICDC_CACR2, ICDC_CACR2_ALCSEL_SIGNAL_ENDED, 0);
		msleep(10);
		icodec_update(ICDC_CMICCR, BIT(3), BIT(3));
		msleep(10);
		icodec_update(ICDC_CMICCR, (0x3 << 1), (0x1 << 1));
		msleep(10);
		icodec_update(ICDC_CACR2, ICDC_CACR2_ALCGAIN_MSK, ICDC_CACR2_ALCGAIN(icodec->mic_vol));
		msleep(10);
		if (adc_hpf_en) {
			icodec_update(ICDC_CGAINR, ICDC_CGAINR_ADC_HPF_EN_N, 0);
			msleep(10);
		}
		icodec->adc_enabled = 1;
	} else if (!on && icodec->adc_enabled) {
		//icodec_update(ICDC_CACR2, ICDC_CACR2_ALCGAIN_MSK, ICDC_CACR2_ALCGAIN(0xc));
		//icodec_update(ICDC_CAACR, ICDC_CACCR_MICBAISCTR_MSK, 0);
		//icodec_update(ICDC_CAACR, ICDC_CACCR_MICBIASEN, 0);
		//icodec_update(ICDC_CMICCR, ICDC_CMICCR_MICEN, 0);
		icodec_update(ICDC_CAMPCR, ICDC_CAMPCR_ADCREFVOLEN|ICDC_CAMPCR_ADCLCKEN|ICDC_CAMPCR_ADCLAMPEN, 0);
		//icodec_update(ICDC_CAMPCR, BIT(4), 0);
		//icodec_update(ICDC_CACR, ICDC_CACR_ADCEN, 0);
		msleep(10);
		icodec->adc_enabled = 0;
	}
	mutex_unlock(&icodec->io_mutex);
	return 0;
}

int codec_adc_power(bool on)
{
#if 0
	return __codec_adc_power(on);
#else
	return 0;
#endif
}

static int codec_init(void *iomem)
{
	static bool icodec_inited = false;
	int i;

	if (icodec_inited)
		return 0;
	icodec = kzalloc(sizeof(*icodec), GFP_KERNEL);
	if (!icodec)
		return -ENOMEM;
	icodec->mapped_base = iomem + 0x1000;
	icodec->mic_vol = 0x10;
	icodec->hpout_vol = 0x18;
	mutex_init(&icodec->io_mutex);

	jz_speaker_init(NULL);
	msleep(10);
	i2s_clk_switch(true);
	msleep(10);
	codec_icdc_reset();
	msleep(10);
	icodec_update(ICDC_CGR, ICDC_CGR_DCRST_N|ICDC_CGR_SRST_N, 0);
	msleep(10);
	icodec_update(ICDC_CGR, ICDC_CGR_DCRST_N|ICDC_CGR_SRST_N,
			ICDC_CGR_DCRST_N|ICDC_CGR_SRST_N);
	msleep(10);
	icodec_writel(ICDC_CMCR, ICDC_CMCR_MASTEREN|ICDC_CMCR_ADCDAC_MASTER|ICDC_CMCR_ADCDATALEN_32BIT|ICDC_CMCR_ADCI2SRST_N);
	icodec_writel(ICDC_CDC2R, ICDC_CDCR2_DACDATALEN_32BIT|ICDC_CDCR2_DACI2SRST_N);
	icodec_writel(ICDC_CDC1R, ICDC_CDCR1_DACVALDALEN_16BIT|ICDC_CDCR1_DACI2SMODE_I2S);
	icodec_writel(ICDC_CACR, ICDC_CACR_ADCVALDALEN_16BIT|ICDC_CACR_ADCI2SMODE_I2S|ICDC_CACR_ADCSWAP_EN|ICDC_CACR_ADCINTF_MONO);
	msleep(10);
	icodec_update(ICDC_CHCR, ICDC_CHCR_HPOUTPOP_MSK, ICDC_CHCR_HPOUTPOP_PERCHARGE);
	msleep(10);
	/*datasheet said 0 is precharge, but the actual situation is the opposite*/
	icodec_update(ICDC_CCR, ICDC_CCR_DACPERCHARGE_N, ICDC_CCR_DACPERCHARGE_N);
	msleep(10);
	for (i = 1; i <= 6; i++) {
		icodec_writel(ICDC_CCR, ICDC_CCR_DACCHAGESEL((0x3f >> i)) | ICDC_CCR_DACPERCHARGE_N);
		usleep_range(20000, 30000);
	}
	icodec_update(ICDC_CCR, ICDC_CCR_DACCHAGESEL_MSK, ICDC_CCR_DACCHAGESEL(0x1f));
	msleep(10);
	icodec_update(ICDC_CSRR, ICDC_CSRR_SAMPLERATE_MSK, ICDC_CSRR_SAMPLERATE(1));
	msleep(10);
	__codec_adc_power(true);
#ifdef SPEAKER_AWAYS_ON
	__codec_dac_power(true);
#endif
	icodec_inited = true;
	return 0;
}

int codec_adc_init(void *iomem, struct codec_params* params)
{
	codec_init(iomem);

	params->vol_step_DB = 150;
	params->vol_min_DB = -1800;
	params->vol_mute = 0;
	params->vol_max_val = 0x1f;
	params->vol_min_val = 0x0;
	return 0;
}

int codec_dac_init(void *iomem, struct codec_params* params)
{
	codec_init(iomem);

	params->vol_step_DB = 150;
	params->vol_min_DB = -3900;
	params->vol_mute = 0;
	params->vol_max_val = 0x1f;
	params->vol_min_val = 0x0;
	return 0;
}
