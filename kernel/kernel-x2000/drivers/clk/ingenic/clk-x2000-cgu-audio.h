#ifndef __INGENIC_CLK_CGU_AUDIO_H__
#define __INGENIC_CLK_CGU_AUDIO_H__
#include "clk-comm.h"

extern const struct clk_ops ingenic_cgu_audio_ops;
extern const struct clk_ops ingenic_cgu_i2s_ops;

#define CGU_AUDIO_DESC(_regoff, _cs, _csm, _ce, _m, _mmsk, _n, _nmsk)	\
{	\
	.regoff = _regoff, \
	.cs_off = _cs,	\
	.cs_msk = _csm,	\
	.bit_ce = _ce,	\
	.m_off = _m,	\
	.m_msk = _mmsk,	\
	.n_off = _n,	\
	.n_msk = _nmsk,	\
}

struct ingenic_cgu_audio_hwdesc {
	u32 regoff;
	s8 cs_off;
	u8 cs_msk;
	u8 bit_ce;
	u16 m_off;
	u16 m_msk;
	u32 n_off;
	u32 n_msk;
};

#endif /*__INGENIC_CLK_CGU_AUDIO_H__*/
