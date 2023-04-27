#ifndef __ASOC_AIC_X2500_H__
#define __ASOC_AIC_X2500_H__

#include <linux/dmaengine.h>
#include <linux/platform_device.h>
#include <linux/clk.h>
#include <linux/spinlock.h>
#include <linux/types.h>

/* For AICFR */
#define AICFR_ICDC_BIT		(5)
#define AICFR_ICDC_MASK		(1 << AICFR_ICDC_BIT)
#define AICFR_CDC_SLV_BIT	(7)
#define AICFR_CDC_SLV_MASK	(1 << AICFR_CDC_SLV_BIT)
#define	AICFR_ISYNCD_BIT	(9)
#define	AICFR_ISYNCD_MASK	(1 << AICFR_ISYNCD_BIT)
#define AICFR_IBCKD_BIT		(10)
#define AICFR_IBCKD_MASK	(1 << AICFR_IBCKD_BIT)
#define AICFR_SYSCLKD_BIT	(11)
#define AICFR_SYSCLKD_MASK	(1 << AICFR_SYSCLKD_BIT)

/* For AICCR */
#define AICCR_ASVTSU_BIT	(9)
#define AICCR_ASVTSU_MASK	(1 << AICCR_ASVTSU_BIT)
#define AICCR_M2S_BIT		(11)
#define AICCR_M2S_MASK		(1 << AICCR_M2S_BIT)

/* For I2SCR */
#define	I2SCR_ESCLK_BIT		(4)
#define	I2SCR_ESCLK_MASK	(1 << I2SCR_ESCLK_BIT)
#define	I2SCR_STPBK_BIT		(12)
#define	I2SCR_STPBK_MASK	(1 << I2SCR_STPBK_BIT)
#define	I2SCR_ISTPBK_BIT	(13)
#define	I2SCR_ISTPBK_MASK	(1 << I2SCR_ISTPBK_BIT)

/* For AICFR */
#define __aic_select_internal_codec(parent)	\
	ingenic_aic_set_reg(parent, AICFR, 1, AICFR_ICDC_MASK, AICFR_ICDC_BIT)

#define __aic_select_external_codec(parent)	\
	ingenic_aic_set_reg(parent, AICFR, 0, AICFR_ICDC_MASK, AICFR_ICDC_BIT)

#define __i2s_codec_slave(parent)	\
	ingenic_aic_set_reg(parent, AICFR, 1, AICFR_CDC_SLV_MASK, AICFR_CDC_SLV_BIT)

#define __i2s_codec_master(parent)	\
	ingenic_aic_set_reg(parent, AICFR, 0, AICFR_CDC_SLV_MASK, AICFR_CDC_SLV_BIT)

#define __i2s_isync_input(parent)            \
	ingenic_aic_set_reg(parent, AICFR , 0, AICFR_ISYNCD_MASK, AICFR_ISYNCD_BIT)

#define __i2s_ibclk_input(parent)            \
	ingenic_aic_set_reg(parent, AICFR, 0, AICFR_IBCKD_MASK, AICFR_IBCKD_BIT)

#define __i2s_select_sysclk_output(parent)	\
	ingenic_aic_set_reg(parent, AICFR, 0, AICFR_SYSCLKD_MASK, AICFR_SYSCLKD_BIT)

#define __i2s_select_sysclk_input(parent)	\
	ingenic_aic_set_reg(parent, AICFR, 1, AICFR_SYSCLKD_MASK, AICFR_SYSCLKD_BIT)


/* For AICCR */
#define __i2s_asvtsu_enable(parent)	\
	ingenic_aic_set_reg(parent, AICCR, 1, AICCR_ASVTSU_MASK, AICCR_ASVTSU_BIT)

#define __i2s_asvtsu_disable(parent)	\
	ingenic_aic_set_reg(parent, AICCR, 0, AICCR_ASVTSU_MASK, AICCR_ASVTSU_BIT)

#define __i2s_m2s_enable(parent)	\
	ingenic_aic_set_reg(parent, AICCR, 1, AICCR_M2S_MASK, AICCR_M2S_BIT)

#define __i2s_m2s_disable(parent)	\
	ingenic_aic_set_reg(parent, AICCR, 0, AICCR_M2S_MASK, AICCR_M2S_BIT)

/* For I2SCR */
#define __i2s_enable_sysclk_output(parent)   \
	ingenic_aic_set_reg(parent, I2SCR, 1, I2SCR_ESCLK_MASK, I2SCR_ESCLK_BIT)

#define __i2s_disable_sysclk_output(parent)  \
	ingenic_aic_set_reg(parent, I2SCR, 0, I2SCR_ESCLK_MASK, I2SCR_ESCLK_BIT)

#define __i2s_stop_bitclk(parent)            \
	ingenic_aic_set_reg(parent, I2SCR, 1, I2SCR_STPBK_MASK, I2SCR_STPBK_BIT)

#define __i2s_start_bitclk(parent)		\
	ingenic_aic_set_reg(parent, I2SCR, 0, I2SCR_STPBK_MASK, I2SCR_STPBK_BIT)





#endif  /*__ASOC_AIC_X2500_H__*/
