#include <linux/spinlock.h>
#include <linux/io.h>
#include <linux/clk.h>
#include <linux/delay.h>
#include <linux/err.h>
#include <soc/cache.h>
#include <soc/cpm.h>
#include <soc/base.h>
#include <soc/extal.h>
#include "clk.h"
static DEFINE_SPINLOCK(cpm_cgu_lock);
struct clk_selectors {
	unsigned int route[4];
};
enum {
	SELECTOR_A = 0,
	SELECTOR_H,
        SELECTOR_SPI
};
const struct clk_selectors selector[] = {
#define CLK(X)  CLK_ID_##X
/*
 *         bit31,bit30
 *          0   , 0       STOP
 *          0   , 1       SCLKA
 *          1   , 0       MPLL
 *          1   , 1       INVALID
 */
	[SELECTOR_A].route = {CLK(STOP),CLK(SCLKA),CLK(MPLL),CLK(INVALID)},
	[SELECTOR_H].route = {CLK(SCLKA),CLK(MPLL),CLK(VPLL),CLK(EPLL)},
	/*
	 *         bit29
	 *          0       EXT1
	 *          1       SFC/2
	 */
	[SELECTOR_SPI].route = {CLK(EXT1),CLK(CGU_SFC),CLK(INVALID),CLK(INVALID)},
#undef CLK
};

#define IS_CGU_CLK(x) (x&CLK_FLG_CGU)

struct cgu_clk {
	/*
	 * off: reg offset.
	 * ce_busy_stop: CE offset  + 1 is busy.
	 * coe : coe for div
	 * div: div bit width
	 * sels: {select}
	 */
	int off,ce_busy_stop,coe,div,sel,cache;
};
static struct cgu_clk cgu_clks[] = {
	[CGU_DDR] = 	{ CPM_DDRCDR, 27, 1, 4, SELECTOR_A},
	[CGU_VPU] = 	{ CPM_VPUCDR, 27, 1, 4, SELECTOR_H},    /*helix/radix*/
	[CGU_MACPHY] = 	{ CPM_MACCDR, 27, 1, 8, SELECTOR_H},
	[CGU_LPC] = 	{ CPM_LPCDR,  26, 1, 8, SELECTOR_H},
	[CGU_MSC_MUX]=	{ CPM_MSC0CDR, 27, 2, 0, SELECTOR_H},
	[CGU_MSC0] = 	{ CPM_MSC0CDR, 27, 2, 8, SELECTOR_H},
	[CGU_MSC1] = 	{ CPM_MSC1CDR, 27, 2, 8, SELECTOR_H},
	[CGU_I2S] = 	{ CPM_I2SCDR, 29, 0, 20, SELECTOR_H},
	[CGU_SFC] =     { CPM_SSICDR, 26, 1, 8, SELECTOR_H},
	[CGU_SSI] = 	{ CPM_SSICDR, 26, 1, 8, SELECTOR_SPI},
	[CGU_CIM] =     { CPM_CIMCDR, 27, 1, 8, SELECTOR_H},
	[CGU_ISP] = 	{ CPM_ISPCDR, 27, 1, 4, SELECTOR_H},
};

static unsigned long cgu_get_rate(struct clk *clk)
{
	unsigned long x;
	unsigned long flags;
	int no = CLK_CGU_NO(clk->flags);

	if (no == CGU_I2S) {
		unsigned int reg_val = 0;
		int m = 0, n = 0;
		reg_val = cpm_inl(cgu_clks[no].off) & 0xf0000000;
		n = reg_val & 0xfffff;
		m = (reg_val >> 20) & 0x1ff;
		printk(KERN_DEBUG"%s, parent = %ld, rate = %ld, m = %d, n = %d, reg val = 0x%08x\n",
				__func__, clk->parent->rate, clk->rate, m, n, cpm_inl(cgu_clks[no].off));
		return (clk->parent->rate * m) / n;
	}

	if(no == CGU_SSI) {
		x = cpm_inl(cgu_clks[no].off);
		if((x >> 29) & 0x1)
			return clk->parent->rate / 2;
		else
			return clk->parent->rate;
	}
	if(cgu_clks[no].div == 0)
		return clk_get_rate(clk->parent);
	spin_lock_irqsave(&cpm_cgu_lock,flags);
	x = cpm_inl(cgu_clks[no].off);
	x &= (1 << cgu_clks[no].div) - 1;
	x = (x + 1) * cgu_clks[no].coe;

	spin_unlock_irqrestore(&cpm_cgu_lock,flags);
	return clk->parent->rate / x;
}
static int cgu_enable(struct clk *clk,int on)
{
	int no = CLK_CGU_NO(clk->flags);
	int reg_val;
	int ce,stop,busy;
	int prev_on;
	unsigned int mask;
	unsigned long flags;
	if(no == CGU_MSC_MUX)
		return 0;
	if(no == CGU_SSI)
		return 0;

	spin_lock_irqsave(&cpm_cgu_lock,flags);

	if (no == CGU_I2S) {
		reg_val = cpm_inl(cgu_clks[no].off);
		if (on)
			reg_val |= (1 << 29);
		else
			reg_val &= ~(1 << 29);
		cpm_outl(reg_val,cgu_clks[no].off);
		printk(KERN_DEBUG"%s,%s reg val = 0x%08x\n",
				__func__, clk->name, cpm_inl(cgu_clks[no].off));
		goto cgu_enable_finish;
	}

	reg_val = cpm_inl(cgu_clks[no].off);
	stop = cgu_clks[no].ce_busy_stop;
	busy = stop + 1;
	ce = stop + 2;
	prev_on = !(reg_val & (1 << stop));
	mask = (1 << cgu_clks[no].div) - 1;
	if(prev_on && on)
		goto cgu_enable_finish;
	if((!prev_on) && (!on))
		goto cgu_enable_finish;
	if(on){
		if(cgu_clks[no].cache && ((cgu_clks[no].cache & mask) != (reg_val & mask))) {
			unsigned int x = cgu_clks[no].cache;
			x = (x & ~(0x1 << stop)) | (0x1 << ce);
			cpm_outl(x,cgu_clks[no].off);
			while(cpm_test_bit(busy,cgu_clks[no].off)) {
				printk("wait stable.[%d][%s]\n",__LINE__,clk->name);
			}
			cpm_clear_bit(ce, cgu_clks[no].off);
			x &= (1 << cgu_clks[no].div) - 1;
			x = (x + 1) * cgu_clks[no].coe;
			clk->rate = clk->parent->rate / x;
			cgu_clks[no].cache = 0;
		} else {
			reg_val |= (1 << ce);
			reg_val &= ~(1 << stop);
			cpm_outl(reg_val,cgu_clks[no].off);
			cpm_clear_bit(ce,cgu_clks[no].off);
		}
	} else {
		reg_val |= (1 << ce);
		reg_val |= ( 1<< stop);
		cpm_outl(reg_val,cgu_clks[no].off);
		cpm_clear_bit(ce,cgu_clks[no].off);
	}

cgu_enable_finish:
	spin_unlock_irqrestore(&cpm_cgu_lock,flags);
	return 0;
}

static int cgu_set_rate(struct clk *clk, unsigned long rate)
{
	unsigned long x,tmp;
	int i,no = CLK_CGU_NO(clk->flags);
	int ce,stop,busy;

	unsigned int reg_val, mask;
	unsigned long flags;
	unsigned long long m, n;
	unsigned long long m_mul;
	unsigned long long tmp_value;
	unsigned long long tmp_rate = (unsigned long long)rate;
	unsigned char sig = 0;

	if (no == CGU_MSC_MUX)
		return -1;

	if (no == CGU_SSI) /*FIXME*/
		return 0;

	if (no == CGU_I2S) {
		for(m=1;m<=0x1ff;m++)
		{
			m_mul = clk->parent->rate * m;
			for(n=1;n<=0xfffff;n++)
			{
				tmp_value = (unsigned long long)(n*tmp_rate);
				if( m_mul == tmp_value )
				{
					sig = 1;
					break;
				}
			}
			if( sig )
			{
				clk->rate = rate;
				break;
			}
		}
		reg_val = cpm_inl(cgu_clks[no].off) & 0xf0000000;
		reg_val |= (m << 20) | (n << 0) | (1 << 29);
		cpm_outl(reg_val,cgu_clks[no].off);
                printk(KERN_DEBUG"%s, parent = %ld, rate = %ld, n = %lld, reg val = 0x%08x\n",
                                __func__, clk->parent->rate, clk->rate, n, cpm_inl(cgu_clks[no].off));
                return 0;
        }
	spin_lock_irqsave(&cpm_cgu_lock,flags);
	mask = (1 << cgu_clks[no].div) - 1;
	tmp = clk->parent->rate / cgu_clks[no].coe;
	for (i = 1; i <= mask+1; i++) {
		if ((tmp / i) <= rate)
			break;
	}
	i--;
	if(i > mask)
		i = mask;
	reg_val = cpm_inl(cgu_clks[no].off);
	x = reg_val & ~mask;
	x |= i;
	stop = cgu_clks[no].ce_busy_stop;
	busy = stop + 1;
	ce = stop + 2;
	if(x & (1 << stop)) {
		cgu_clks[no].cache = x;
		clk->rate = tmp  / (i + 1);
	}
	else if((mask & reg_val) != i){

		x = (x & ~(0x1 << stop)) | (0x1 << ce);
		cpm_outl(x, cgu_clks[no].off);
		while(cpm_test_bit(busy,cgu_clks[no].off))
			printk("wait stable.[%d][%s]\n",__LINE__,clk->name);
		x &= ~(1 << ce);
		cpm_outl(x, cgu_clks[no].off);
		cgu_clks[no].cache = 0;
		clk->rate = tmp  / (i + 1);
	}
	spin_unlock_irqrestore(&cpm_cgu_lock,flags);
	return 0;
}

static struct clk * cgu_get_parent(struct clk *clk)
{
	unsigned int no,cgu,idx,pidx;

	no = CLK_CGU_NO(clk->flags);
	cgu = cpm_inl(cgu_clks[no].off);
	if (no == CGU_SSI)
		idx = (cgu >> 29) & 0x1;
	else
		idx = cgu >> 30;

	pidx = selector[cgu_clks[no].sel].route[idx];
	if (pidx == CLK_ID_STOP || pidx == CLK_ID_INVALID)
		return NULL;

	return get_clk_from_id(pidx);
}

static int cgu_set_parent(struct clk *clk, struct clk *parent)
{
	int i,tmp;
	int no = CLK_CGU_NO(clk->flags);
	int parent_id;
	unsigned int reg_val,cgu,mask;
	int ce,stop,busy;
	unsigned long flags;
	stop = cgu_clks[no].ce_busy_stop;
	busy = stop + 1;
	ce = stop + 2;
	mask = (1 << cgu_clks[no].div) - 1;

	parent_id = get_clk_id(parent);
	for(i = 0;i < 4;i++) {
		if(selector[cgu_clks[no].sel].route[i] == parent_id){
			break;
		}
	}
	if(i >= 4)
		return -EINVAL;

	if(cgu_clks[no].sel == SELECTOR_SPI) {
		if (i >= 2)
			return -EINVAL;

		spin_lock_irqsave(&cpm_cgu_lock,flags);
		cgu = cpm_inl(cgu_clks[no].off);
		if(i == 0)
			cgu &= ~(1 << 29);
		else
			cgu |= (1 << 29);
		cpm_outl(cgu, cgu_clks[no].off);
		clk->rate = clk_get_rate(parent) / (1 << i);
		spin_unlock_irqrestore(&cpm_cgu_lock,flags);
		return 0;
	}
	spin_lock_irqsave(&cpm_cgu_lock,flags);
	cgu = cpm_inl(cgu_clks[no].off);
	reg_val = cgu;
	cgu &= ~(3 << 30);
	cgu |= (i << 30);
	tmp = parent->rate / cgu_clks[no].coe;
	for (i = 1; i <= mask+1; i++) {
		if ((tmp / i) <= clk->rate)
			break;
	}
	i--;
	mask = (1 << cgu_clks[no].div) - 1;
	cgu = (cgu & ~(0x1 << stop)) | (0x1 << ce);
	cgu = cgu & ~mask;
	cgu |= i;

	if(reg_val & (1 << stop))
		cgu_clks[no].cache = cgu;
	else if(((mask & reg_val) != i) || (no == CGU_SFC)){
		cpm_outl(cgu, cgu_clks[no].off);
		while(cpm_test_bit(busy,cgu_clks[no].off))
			printk("wait stable.[%d][%s]\n",__LINE__,clk->name);
		cgu &= ~(1 << ce);
		cpm_outl(cgu, cgu_clks[no].off);
		cgu_clks[no].cache = 0;
	}
	clk->rate = tmp / (i + 1);
	clk->parent = parent;

	spin_unlock_irqrestore(&cpm_cgu_lock,flags);

	return 0;
}

static int cgu_is_enabled(struct clk *clk) {
	int no = CLK_CGU_NO(clk->flags);
	int stop = cgu_clks[no].ce_busy_stop;

	if (no != CGU_I2S)
		return !(cpm_inl(cgu_clks[no].off) & (1 << stop));
	else
		return !!(cpm_inl(cgu_clks[no].off) & (1 << stop));
}

static struct clk_ops clk_cgu_ops = {
	.enable	= cgu_enable,
	.get_rate = cgu_get_rate,
	.set_rate = cgu_set_rate,
	.get_parent = cgu_get_parent,
	.set_parent = cgu_set_parent,
};

static struct clk_ops clk_ddr_ops = {
	.get_rate = cgu_get_rate,
	.get_parent = cgu_get_parent,
};

void __init init_cgu_clk(struct clk *clk)
{
	int no;
	int id;

	if (clk->flags & CLK_FLG_PARENT) {
		id = CLK_PARENT(clk->flags);
		clk->parent = get_clk_from_id(id);
	} else {
		clk->parent = cgu_get_parent(clk);
	}
	no = CLK_CGU_NO(clk->flags);
	cgu_clks[no].cache = 0;
	clk->rate = cgu_get_rate(clk);
	if(cgu_is_enabled(clk)) {
		clk->flags |= CLK_FLG_ENABLE;
	}

	if (no == CGU_MSC_MUX)
		clk->ops = NULL;
	else if (no == CGU_DDR)
		clk->ops = &clk_ddr_ops;
    else if (no == CGU_SFC) {
#ifdef CONFIG_JZ_SPI_SELECT_EXT_CLK
        cpm_clear_bit(29, CPM_SSICDR);
#else
        cpm_set_bit(29, CPM_SSICDR);
#endif
		clk->ops = &clk_cgu_ops;
    }
	else
		clk->ops = &clk_cgu_ops;

}
