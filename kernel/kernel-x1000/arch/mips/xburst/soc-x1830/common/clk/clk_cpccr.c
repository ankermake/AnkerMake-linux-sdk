#include <linux/spinlock.h>
#include <linux/io.h>
#include <linux/clk.h>
#include <linux/delay.h>
#include <linux/err.h>
#include <linux/cpufreq.h>
#include <linux/bsearch.h>

#include <asm/cacheops.h>
#include <soc/cpm.h>
#include <soc/cache.h>
#include <soc/base.h>
#include <soc/extal.h>
#include "clk.h"

struct cpccr_clk {
	short off,sel,ce;
};
static struct cpccr_clk cpccr_clks[] = {
#define CPCCR_CLK(N,O,D,E)			\
	[N] = { .off = O, .sel = D, .ce = E}
	CPCCR_CLK(CDIV, 0, 28,22),
	CPCCR_CLK(L2CDIV, 4, 28,22),
	CPCCR_CLK(H0DIV, 8, 26,21),
	CPCCR_CLK(H2DIV, 12, 24,20),
	CPCCR_CLK(PDIV, 16, 24,20),
	CPCCR_CLK(SCLKA, -1,30,23),
#undef CPCCR_CLK
};
static unsigned int cpccr_selector[4] = {0,CLK_ID_SCLKA,CLK_ID_MPLL,0};
static unsigned int sclka_selector[4] = {0, CLK_ID_EXT1, CLK_ID_APLL, 0};

static unsigned long cpccr_get_rate(struct clk *clk)
{
	unsigned long cpccr = cpm_inl(CPM_CPCCR);
	unsigned int rate  = 0;
	int v;
	if (clk->parent)
		rate = clk_get_rate(clk->parent);
	if (CLK_CPCCR_NO(clk->flags) == SCLKA)
		v = 1;
	else
		v = ((cpccr >> cpccr_clks[CLK_CPCCR_NO(clk->flags)].off) & 0xf) + 1;
	return (rate/v);
}

void __init init_cpccr_clk(struct clk *clk)
{
	int sel, gate;
	unsigned long cpccr = cpm_inl(CPM_CPCCR);
	if(CLK_CPCCR_NO(clk->flags) != SCLKA) {
		sel = (cpccr >> cpccr_clks[CLK_CPCCR_NO(clk->flags)].sel) & 0x3;
		if(cpccr_selector[sel] != 0) {
			clk->parent = get_clk_from_id(cpccr_selector[sel]);
			clk->flags |= CLK_FLG_ENABLE;
		}else {
			clk->parent = NULL;
			clk->flags &= ~CLK_FLG_ENABLE;
		}
	} else {
		sel = (cpccr >> cpccr_clks[SCLKA].sel) & 0x3;
		gate = (cpccr >> cpccr_clks[SCLKA].ce) & 0x3;
		if (sclka_selector[sel] != 0) {
			clk->parent = get_clk_from_id(sclka_selector[sel]);
			if (gate)
				clk->flags &= ~CLK_FLG_ENABLE;
			else
				clk->flags |= CLK_FLG_ENABLE;
		} else {
			clk->parent = NULL;
			clk->flags &= ~CLK_FLG_ENABLE;
		}
	}
	clk->rate = cpccr_get_rate(clk);
}
