#ifndef __INGENIC_CLK_PLLV2_H__
#define __INGENIC_CLK_PLLV2_H__
#include "clk-comm.h"


extern const struct clk_ops ingenic_pll_v2_ro_ops;

#define PLL_DESC(_regoff, _m, _ml, _n, _nl, _od1, _od1l, _od0, _od0l, _en, _on, _bs)	\
{	\
	.regoff = _regoff, \
	.m_sft = _m, \
	.m_width = _ml,	\
	.n_sft = _n, \
	.n_width = _nl,	\
	.od1_sft = _od1, \
	.od1_width = _od0l, \
	.od0_sft = _od0, \
	.od0_width = _od0l, \
	.en_bit = _en, \
	.on_bit = _on, \
	.bs_bit	= _bs,	\
}

struct ingenic_pll_hwdesc {
	u32 regoff;
	u8 m_sft;
	u8 m_width;
	u8 n_sft;
	u8 n_width;
	u8 od1_sft;
	u8 od1_width;
	u8 od0_sft;
	u8 od0_width;
	u8 on_bit;
	u8 en_bit;
	s8 bs_bit;
};

#endif /*__INGENIC_CLK_PLLV2_H__*/
