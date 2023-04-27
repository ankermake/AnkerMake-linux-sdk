#ifndef __INGENIC_CLK_CGU_H__
#define __INGENIC_CLK_CGU_H__
#include "clk-comm.h"

extern const struct clk_ops ingenic_cgu_ops;

/*reg offset, sel, selmsk, ce, busy ,stop, cdr, cdrmsk, step, fixclk_array*/
#define CGU_DESC_BASE(_regoff, _cs, _csm, _ce, _busy, _stop, _cdr, _cdrm, _step, _fixclk, __fixclk_sz)	\
{	\
	.regoff = _regoff, \
	.cs_off = _cs,	\
	.cs_msk = _csm,	\
	.bit_ce = _ce,	\
	.bit_stop = _stop,	\
	.bit_busy = _busy,	\
	.cdr_off = _cdr,	\
	.cdr_msk = _cdrm,	\
	.div_step = _step,	\
	.fixclk = _fixclk,	\
	.fixclk_sz = __fixclk_sz, \
}

#define CGU_DESC_WITH_FIXCLK(_regoff, _cs, _csm, _ce, _busy, _stop, _cdr, _cdrm, _step, _fixclk)	\
	CGU_DESC_BASE(_regoff, _cs, _csm, _ce, _busy, _stop, _cdr, _cdrm, _step, _fixclk, ARRAY_SIZE(_fixclk))

#define CGU_DESC(_regoff, _cs, _csm, _ce, _busy, _stop, _cdr, _cdrm, _step) \
	CGU_DESC_BASE(_regoff, _cs, _csm, _ce, _busy, _stop, _cdr, _cdrm, _step, NULL, 0)

struct ingenic_cgu_hwdesc {
	u32 regoff;
	s8 cs_off;
	u8 cs_msk;
	u8 bit_ce;
	u8 bit_stop;
	u8 bit_busy;
	u16 cdr_msk;
	u16 cdr_off;
	int div_step;
	const u8 *fixclk;
	u8 fixclk_sz;
};

#define HAVE_FIXCLK(hwdesc)	\
		(hwdesc->fixclk == NULL)
#define FIXCLK_CS(hwdesc)	\
		(hwdesc->fixclk[0])

static bool inline is_fixclk(const struct ingenic_cgu_hwdesc *hwdesc, int cs)
{
	int i;
	if (!hwdesc || !hwdesc->fixclk)
		return false;

	for (i = 0; i < hwdesc->fixclk_sz; i++) {
		if (cs == hwdesc->fixclk[i])
			return true;
	}
	return false;
}

int ingenic_cgu_set_parent(struct clk_hw *hw, u8 index);
u8 ingenic_cgu_get_parent(struct clk_hw *hw);
int ingenic_cgu_determine_rate(struct clk_hw *hw, struct clk_rate_request *req);
unsigned long ingenic_cgu_recalc_rate(struct clk_hw *hw, unsigned long parent_rate);
int ingenic_cgu_set_rate(struct clk_hw *hw, unsigned long rate, unsigned long parent_rate);
int ingenic_cgu_enable(struct clk_hw *hw);
void ingenic_cgu_disable(struct clk_hw *hw);
int ingenic_cgu_is_enabled(struct clk_hw *hw);
#ifdef CONFIG_INGENIC_CLK_DEBUG_FS
int ingenic_cgu_debug_init(struct clk_hw *hw, struct dentry *dentry);
#endif
#endif /*__INGENIC_CLK_CGU_H__*/
