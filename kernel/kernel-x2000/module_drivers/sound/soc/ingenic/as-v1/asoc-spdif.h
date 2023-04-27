#ifndef __ASOC_AIC_SPDIF_H__
#define __ASOC_AIC_SPDIF_H__

#include <linux/dmaengine.h>
#include <linux/platform_device.h>
#include <linux/clk.h>
#include <linux/spinlock.h>
#include <linux/types.h>

/* For SPDIF */
#define SPENA		(0x80)
#define SPCTRL		(0x84)
#define SPSTATE		(0x88)
#define SPCFG1		(0x8c)
#define SPCFG2		(0x90)
#define SPFIFO		(0x94)

/* For SPENA */
#define	SPENA_SPEN_BIT		(0)
#define SPENA_SPEN_MASK		(1 << SPENA_SPEN_BIT)

/* For SPCTRL */
#define SPCTRL_M_FFUR_BIT	(0)
#define SPCTRL_M_FFUR_MASK	(1 << SPCTRL_M_FFUR_BIT)
#define SPCTRL_M_TRIG_BIT	(1)
#define SPCTRL_M_TRIG_MASK	(1 << SPCTRL_M_TRIG_BIT)
#define SPCTRL_SPDIF_I2S_BIT	(10)
#define SPCTRL_SPDIF_I2S_MASK	(1 << SPCTRL_SPDIF_I2S_BIT)
#define SPCTRL_SFT_RST_BIT	(11)
#define SPCTRL_SFT_RST_MASK	(1 << SPCTRL_SFT_RST_BIT)
#define SPCTRL_INVALID_BIT	(12)
#define SPCTRL_INVALID_MASK	(1 << SPCTRL_INVALID_BIT)
#define SPCTRL_SIGN_N_BIT	(13)
#define SPCTRL_SIGN_N_MASK	(1 << SPCTRL_SIGN_N_BIT)
#define SPCTRL_D_TYPE_BIT	(14)
#define SPCTRL_D_TYPE_MASK	(1 << SPCTRL_D_TYPE_BIT)
#define SPCTRL_DMA_EN_BIT	(15)
#define SPCTRL_DMA_EN_MASK	(1 << SPCTRL_DMA_EN_BIT)

/* For SPSTATE */
#define SPSTATE_F_FFUR_BIT	(0)
#define SPSTATE_F_FFUR_MASK	(1 << SPSTATE_F_FFUR_BIT)
#define SPSTATE_F_TRIG_BIT	(1)
#define SPSTATE_F_TRIG_MASK	(1 << SPSTATE_F_TRIG_BIT)
#define SPSTATE_BUSY_BIT	(7)
#define SPSTATE_BUSY_MASK	(1 << SPSTATE_BUSY_BIT)
#define SPSTATE_FIFO_LVL_BIT	(8)
#define SPSTATE_FIFO_LVL_MASK	(0x7f << SPSTATE_FIFO_LVL_BIT)

/* For SPCFG1 */
#define SPCFG1_CH2_NUM_BIT	(0)
#define SPCFG1_CH2_NUM_MASK	(0xf << SPCFG1_CH2_NUM_BIT)
#define SPCFG1_CH1_NUM_BIT	(4)
#define SPCFG1_CH1_NUM_MASK	(0xf << SPCFG1_CH1_NUM_BIT)
#define SPCFG1_SRC_NUM_BIT	(8)
#define SPCFG1_SRC_NUM_MASK	(0xf << SPCFG1_SRC_NUM_BIT)
#define SPCFG1_TRIG_BIT		(12)
#define SPCFG1_TRIG_MASK	(0x3 << SPCFG1_TRIG_BIT)
#define SPCFG1_ZRO_VLD_BIT	(16)
#define SPCFG1_ZRO_VLD_MASK	(1 << SPCFG1_ZRO_VLD_BIT)
#define SPCFG1_INIT_LVL_BIT	(17)
#define SPCFG1_INIT_LVL_MASK	(1 << SPCFG1_INIT_LVL_BIT)

/* For SPCFG2 */
#define SPCFG2_CON_PRO_BIT	(0)
#define SPCFG2_CON_PRO_MASK	(1 << SPCFG2_CON_PRO_BIT)
#define SPCFG2_AUDIO_N_BIT	(1)
#define SPCFG2_AUDIO_N_MASK	(1 << SPCFG2_AUDIO_N_BIT)
#define SPCFG2_COPY_N_BIT	(2)
#define SPCFG2_COPY_N_MASK	(1 << SPCFG2_COPY_N_BIT)
#define SPCFG2_PRE_BIT		(3)
#define SPCFG2_PRE_MASK		(1 << SPCFG2_PRE_BIT)
#define SPCFG2_CH_MD_BIT	(6)
#define SPCFG2_CH_MD_MASK	(0x3 << SPCFG2_CH_MD_BIT)
#define SPCFG2_CAT_CODE_BIT	(8)
#define SPCFG2_CAT_CODE_MASK	(0xff << SPCFG2_CAT_CODE_BIT)
#define SPCFG2_CLK_ACU_BIT	(16)
#define SPCFG2_CLK_ACU_MASK	(0x3 << SPCFG2_CLK_ACU_BIT)
#define SPCFG2_MAX_WL_BIT	(18)
#define SPCFG2_MAX_WL_MASK	(1 << SPCFG2_MAX_WL_BIT)
#define SPCFG2_SAMPL_WL_BIT	(19)
#define SPCFG2_SAMPL_WL_MASK	(0x7 << SPCFG2_SAMPL_WL_BIT)
#define SPCFG2_SAMPL_WL_20BITM	(0x1 << SPCFG2_SAMPL_WL_BIT)
#define SPCFG2_SAMPL_WL_21BIT	(0x6 << SPCFG2_SAMPL_WL_BIT)
#define SPCFG2_SAMPL_WL_22BIT	(0x2 << SPCFG2_SAMPL_WL_BIT)
#define SPCFG2_SAMPL_WL_23BIT	(0x4 << SPCFG2_SAMPL_WL_BIT)
#define SPCFG2_SAMPL_WL_24BIT	(0x5 << SPCFG2_SAMPL_WL_BIT)
#define SPCFG2_SAMPL_WL_16BIT	(0x1 << SPCFG2_SAMPL_WL_BIT)
#define SPCFG2_SAMPL_WL_17BIT	(0x6 << SPCFG2_SAMPL_WL_BIT)
#define SPCFG2_SAMPL_WL_18BIT	(0x2 << SPCFG2_SAMPL_WL_BIT)
#define SPCFG2_SAMPL_WL_19BIT	(0x4 << SPCFG2_SAMPL_WL_BIT)
#define SPCFG2_SAMPL_WL_20BITL	(0x5 << SPCFG2_SAMPL_WL_BIT)
#define SPCFG2_ORG_FRQ_BIT	(22)
#define SPCFG2_ORG_FRQ_MASK	(0xf << SPCFG2_ORG_FRQ_BIT)
#define SPCFG2_FS_BIT		(26)
#define SPCFG2_FS_MASK		(0xf << SPCFG2_FS_BIT)

/* For SPFIFO */
#define SPFIFO_DATA_BIT		(0)
#define SPFIFO_DATA_MASK	(0xffffff << SPFIFO_DATA_BIT)


/* For SPDIF */
#define __spdif_test_underrun(parent)     \
	ingenic_aic_get_reg(parent, SPSTATE, SPSTATE_F_FFUR_MASK, SPSTATE_F_FFUR_BIT)
#define __spdif_clear_underrun(parent)     \
	ingenic_aic_set_reg(parent, SPSTATE, 0, SPSTATE_F_FFUR_MASK, SPSTATE_F_FFUR_BIT)
#define __spdif_is_enable_transmit_dma(parent)    \
	ingenic_aic_get_reg(parent, SPCTRL, SPCTRL_DMA_EN_MASK, SPCTRL_DMA_EN_BIT)
#define __spdif_enable_transmit_dma(parent)    \
	ingenic_aic_set_reg(parent, SPCTRL, 1, SPCTRL_DMA_EN_MASK, SPCTRL_DMA_EN_BIT)
#define __spdif_disable_transmit_dma(parent)    \
	ingenic_aic_set_reg(parent, SPCTRL, 0, SPCTRL_DMA_EN_MASK, SPCTRL_DMA_EN_BIT)
#define __spdif_reset(parent)					\
	ingenic_aic_set_reg(parent, SPCTRL, 1, SPCTRL_SFT_RST_MASK, SPCTRL_SFT_RST_BIT)
#define __spdif_get_reset(parent)					\
	ingenic_aic_get_reg(parent, SPCTRL,SPCTRL_SFT_RST_MASK, SPCTRL_SFT_RST_BIT)
#define __spdif_enable(parent)					\
	ingenic_aic_set_reg(parent, SPENA, 1, SPENA_SPEN_MASK, SPENA_SPEN_BIT)
#define __spdif_disable(parent)					\
	ingenic_aic_set_reg(parent, SPENA, 0, SPENA_SPEN_MASK, SPENA_SPEN_BIT)
#define __spdif_set_dtype(parent, n)			\
	ingenic_aic_set_reg(parent, SPCTRL, n, SPCTRL_D_TYPE_MASK, SPCTRL_D_TYPE_BIT)
#define __spdif_set_trigger(parent, n)			\
	ingenic_aic_set_reg(parent, SPCFG1, n, SPCFG1_TRIG_MASK, SPCFG1_TRIG_BIT)
#define __spdif_set_ch1num(parent, n)		\
	ingenic_aic_set_reg(parent, SPCFG1, n, SPCFG1_CH1_NUM_MASK, SPCFG1_CH1_NUM_BIT)
#define __spdif_set_ch2num(parent, n)		\
	ingenic_aic_set_reg(parent, SPCFG1, n, SPCFG1_CH2_NUM_MASK, SPCFG1_CH2_NUM_BIT)
#define __spdif_set_srcnum(parent, n)		\
	ingenic_aic_set_reg(parent, SPCFG1, n, SPCFG1_SRC_NUM_MASK, SPCFG1_SRC_NUM_BIT)
#define __interface_select_spdif(parent)      \
	ingenic_aic_set_reg(parent, SPCTRL, 1, SPCTRL_SPDIF_I2S_MASK, SPCTRL_SPDIF_I2S_BIT)
#define __spdif_play_lastsample(parent)        \
	ingenic_aic_set_reg(parent, SPCFG1, 1, SPCFG1_ZRO_VLD_MASK, SPCFG1_ZRO_VLD_BIT)
#define __spdif_init_set_low(parent)			\
	ingenic_aic_set_reg(parent, SPCFG1, 0, SPCFG1_INIT_LVL_MASK, SPCFG1_INIT_LVL_BIT)
#define __spdif_choose_consumer(parent)					\
	ingenic_aic_set_reg(parent, SPCFG2, 0, SPCFG2_CON_PRO_MASK, SPCFG2_CON_PRO_BIT)
#define __spdif_clear_audion(parent)				\
	ingenic_aic_set_reg(parent, SPCFG2, 0, SPCFG2_AUDIO_N_MASK, SPCFG2_AUDIO_N_BIT)
#define __spdif_set_copyn(parent)					\
	ingenic_aic_set_reg(parent, SPCFG2, 1, SPCFG2_COPY_N_MASK, SPCFG2_COPY_N_BIT)
#define __spdif_clear_pre(parent)					\
	ingenic_aic_set_reg(parent, SPCFG2, 0, SPCFG2_PRE_MASK, SPCFG2_PRE_BIT)
#define __spdif_choose_chmd(parent)				\
	ingenic_aic_set_reg(parent, SPCFG2, 0, SPCFG2_CH_MD_MASK, SPCFG2_CH_MD_BIT)
#define __spdif_set_category_code_normal(parent)	\
	ingenic_aic_set_reg(parent, SPCFG2, 0, SPCFG2_CAT_CODE_MASK, SPCFG2_CAT_CODE_BIT)
#define __spdif_set_clkacu(parent, n)				\
	ingenic_aic_set_reg(parent, SPCFG2, n, SPCFG2_CLK_ACU_MASK, SPCFG2_CLK_ACU_BIT)
#define __spdif_set_sample_size(parent, n)		\
	ingenic_aic_set_reg(parent, SPCFG2, n, SPCFG2_SAMPL_WL_MASK, SPCFG2_SAMPL_WL_BIT)
#define __spdif_set_max_wl(parent, n)                           \
	ingenic_aic_set_reg(parent, SPCFG2, n, SPCFG2_MAX_WL_MASK, SPCFG2_MAX_WL_BIT)
#define	__spdif_set_ori_sample_freq(parent, org_frq_tmp)	\
	ingenic_aic_set_reg(parent, SPCFG2, org_frq_tmp, SPCFG2_ORG_FRQ_MASK, SPCFG2_ORG_FRQ_BIT)
#define	__spdif_set_sample_freq(parent, fs_tmp)			\
	ingenic_aic_set_reg(parent, SPCFG2, fs_tmp, SPCFG2_FS_MASK, SPCFG2_FS_BIT)
#define __spdif_set_valid(parent)				\
	ingenic_aic_set_reg(parent, SPCTRL, 0, SPCTRL_INVALID_MASK, SPCTRL_INVALID_BIT)
#define __spdif_mask_trig(parent)				\
	ingenic_aic_set_reg(parent, SPCTRL, 1, SPCTRL_M_TRIG_MASK, SPCTRL_M_TRIG_BIT)
#define __spdif_disable_underrun_intr(parent)  \
	ingenic_aic_set_reg(parent, SPCTRL, 1, SPCTRL_M_FFUR_MASK, SPCTRL_M_FFUR_BIT)
#define __spdif_set_signn(parent)                             \
	ingenic_aic_set_reg(parent, SPCTRL, 1, SPCTRL_SIGN_N_MASK, SPCTRL_SIGN_N_BIT)
#define __spdif_clear_signn(parent)                   \
	ingenic_aic_set_reg(parent, SPCTRL, 0, SPCTRL_SIGN_N_MASK, SPCTRL_SIGN_N_BIT)






#endif  /*__ASOC_AIC_SPDIF_H__*/

