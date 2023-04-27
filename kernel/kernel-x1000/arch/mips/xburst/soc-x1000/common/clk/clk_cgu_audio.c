#include <linux/spinlock.h>
#include <linux/io.h>
#include <linux/clk.h>
#include <linux/delay.h>
#include <linux/err.h>
#include <soc/cache.h>
#include <soc/cpm.h>
#include <soc/base.h>
#include <soc/extal.h>
#include <soc/ddr.h>
#include "clk.h"

/* LRCLK=Sameple Rate, MCLK = 4*BCLK = 4*64*LRCLK */
static unsigned int audio_rate [] = {
	8000, 11025, 12000, 16000, 22050, 24000,
	32000, 44100, 48000, 88200, 96000,
};
#define AUDIO_RATE_NUM (sizeof(audio_rate))
#define AUDIO_DIV_SIZE (ARRAY_SIZE(audio_rate) * 2)

struct audio_div_values {
	unsigned int savem:8;
	unsigned int saven:24;
};
struct audio_div {
	unsigned int rate;
	unsigned int savem;
	unsigned int saven;
};
static struct audio_div audiodiv_apll[AUDIO_RATE_NUM];
static struct audio_div audiodiv_mpll[AUDIO_RATE_NUM];

static DEFINE_SPINLOCK(cpm_cgu_lock);
struct clk_selectors {
	unsigned int route[4];
};
enum {
	SELECTOR_AUDIO = 0,
};
const struct clk_selectors audio_selector[] = {
#define CLK(X)  CLK_ID_##X
/*
 *         bit31,bit30
 *          0   , 0       EXT1
 *          0   , 1       APLL
 *          1   , 0       EXT1
 *          1   , 1       MPLL
 */
	[SELECTOR_AUDIO].route = {CLK(EXT1),CLK(SCLKA),CLK(EXT1),CLK(MPLL)},
#undef CLK
};

struct cgu_clk {
	int off,en,maskm,bitm,maskn,bitn,maskd,bitd,sel,cache;
};
static struct cgu_clk cgu_clks[] = {
	[CGU_AUDIO_I2S] = {CPM_I2SCDR, 1<<29, 0x1f << 13, 13, 0x1fff, 0, SELECTOR_AUDIO},
	[CGU_AUDIO_I2S1] = {CPM_I2SCDR1, -1, -1, -1, -1, -1, -1},
	[CGU_AUDIO_PCM] = {CPM_PCMCDR, 1<<29, 0x1f << 13, 13, 0x1fff, 0, SELECTOR_AUDIO},
	[CGU_AUDIO_PCM1] = {CPM_PCMCDR1, -1, -1, -1, -1, -1, -1},
};

static unsigned long cgu_audio_get_rate(struct clk *clk)
{
	unsigned long m,n,d;
	unsigned long flags;
	int no = CLK_CGU_AUDIO_NO(clk->flags);

	if(clk->parent == get_clk_from_id(CLK_ID_EXT1))
		return clk->parent->rate;

	spin_lock_irqsave(&cpm_cgu_lock,flags);
	m = cpm_inl(cgu_clks[no].off);
	n = m & cgu_clks[no].maskn;
	m &= cgu_clks[no].maskm;
	if(no == CGU_AUDIO_I2S){
		d = readl((volatile unsigned int*)I2S_PRI_DIV);
		spin_unlock_irqrestore(&cpm_cgu_lock,flags);
		return (clk->parent->rate*m)/(n*((d&0x3f) + 1)*(64));
	}else if (no == CGU_AUDIO_PCM){
		d = inl(PCM_PRI_DIV);
		spin_unlock_irqrestore(&cpm_cgu_lock,flags);
		return (clk->parent->rate*m)/(n*(((d&0x1f<<6)>>6)+1)*8);
	}
	spin_unlock_irqrestore(&cpm_cgu_lock,flags);
	return -EINVAL;
}
static int cgu_audio_enable(struct clk *clk,int on)
{
	int no = CLK_CGU_AUDIO_NO(clk->flags);
	int reg_val;
	unsigned long flags;

	spin_lock_irqsave(&cpm_cgu_lock,flags);

	if(on){
		reg_val = cpm_inl(cgu_clks[no].off);
		if(reg_val & (cgu_clks[no].en))
			goto cgu_enable_finish;
		if(!cgu_clks[no].cache)
			printk("must set rate before enable\n");

		cpm_outl(cgu_clks[no].cache, cgu_clks[no].off);
		cpm_outl(cgu_clks[no].cache | cgu_clks[no].en, cgu_clks[no].off);
		if (1 || no == CGU_AUDIO_I2S) {
			/* write access I2SCDR1 to trigger M/N enable*/
			cpm_outl(cpm_inl(cgu_clks[no+1].off), cgu_clks[no+1].off);
		}
		cgu_clks[no].cache = 0;
	}else{
		reg_val = cpm_inl(cgu_clks[no].off);
		reg_val &= ~cgu_clks[no].en;
		cpm_outl(reg_val,cgu_clks[no].off);
	}
cgu_enable_finish:
	spin_unlock_irqrestore(&cpm_cgu_lock,flags);
	return 0;
}

static struct clk* cgu_audio_get_parent(struct clk *clk)
{
	unsigned int no,cgu,idx,pidx;
	unsigned long flags;
	struct clk* pclk;

	spin_lock_irqsave(&cpm_cgu_lock,flags);
	no = CLK_CGU_AUDIO_NO(clk->flags);
	cgu = cpm_inl(cgu_clks[no].off);
	idx = cgu >> 30;
	pidx = audio_selector[cgu_clks[no].sel].route[idx];
	if (pidx == CLK_ID_STOP || pidx == CLK_ID_INVALID){
		spin_unlock_irqrestore(&cpm_cgu_lock,flags);
		return NULL;
	}
	pclk = get_clk_from_id(pidx);
	spin_unlock_irqrestore(&cpm_cgu_lock,flags);

	return pclk;
}

static int cgu_audio_set_parent(struct clk *clk, struct clk *parent)
{
	int tmp_val,i;
	int no = CLK_CGU_AUDIO_NO(clk->flags);
	unsigned long flags;
	for(i = 0;i < 4;i++) {
		if(audio_selector[cgu_clks[no].sel].route[i] == get_clk_id(parent)){
			break;
		}
	}

	if(i >= 4)
		return -EINVAL;
	spin_lock_irqsave(&cpm_cgu_lock,flags);

	tmp_val = cpm_inl(cgu_clks[no].off);
	if (tmp_val&cgu_clks[no].en) {
		if(get_clk_id(parent) != CLK_ID_EXT1){
			tmp_val = cpm_inl(cgu_clks[no].off)&(~(3<<30));
			tmp_val |= i<<30;
			cpm_outl(tmp_val,cgu_clks[no].off);
		}else{
			tmp_val = cpm_inl(cgu_clks[no].off)&(~(3<<30|0x3fffff));
			tmp_val |= i<<30|1<<13|1;
			cpm_outl(tmp_val,cgu_clks[no].off);
		}

	} else {
		cgu_clks[no].cache &= ~(3 << 30);

		if(get_clk_id(parent) != CLK_ID_EXT1)
			cgu_clks[no].cache |= i<<30;
		else
			cgu_clks[no].cache |= i<<30|1<<13|1;
	}

	spin_unlock_irqrestore(&cpm_cgu_lock,flags);

	clk->parent = parent;

	return 0;
}

static int cgu_audio_set_rate(struct clk *clk, unsigned long rate)
{
	unsigned int reg_val;
	struct clk *parent = clk->parent;
	unsigned long long m, n;
	unsigned long long m_mul = 0;
	unsigned long long max_m;
	int no = CLK_CGU_AUDIO_NO(clk->flags);

	max_m = 0x1fffull * rate;
	do_div(max_m, parent->rate);

	if (max_m > 0x1ff)
		max_m = 0x1ff;

	if (max_m == 0)
		return -EINVAL;

	for (m = 1; m <= max_m; m++) {
		m_mul = m * parent->rate;
		if (!do_div(m_mul, rate))
			break;
	}

	n = m_mul;

	if (m > max_m)
		m = max_m;

	clk->rate = rate;

	reg_val = cgu_clks[no].cache & 0xf0000000;
	reg_val |= (m << 13);
	reg_val |= (n << 0);

	cgu_clks[no].cache |= reg_val;

	/* write access I2SCDR1 to trigger M/N enable */
	if (no == CGU_AUDIO_I2S) {
		reg_val = cpm_inl(CPM_I2SCDR1);
		cpm_outl(reg_val, CPM_I2SCDR1);
	}

	return 0;
}


static int cgu_audio_is_enabled(struct clk *clk) {
	int no,state;
	unsigned long flags;
	spin_lock_irqsave(&cpm_cgu_lock,flags);
	no = CLK_CGU_AUDIO_NO(clk->flags);
	state = (cpm_inl(cgu_clks[no].off) & cgu_clks[no].en);
	spin_unlock_irqrestore(&cpm_cgu_lock,flags);
	return state;
}

static struct clk_ops clk_cgu_audio_ops = {
	.enable	= cgu_audio_enable,
	.get_rate = cgu_audio_get_rate,
	.set_rate = cgu_audio_set_rate,
	.get_parent = cgu_audio_get_parent,
	.set_parent = cgu_audio_set_parent,
};
static  void get_audio_div(void)
{
	int i, a,m;
	struct audio_div_values divvalues[AUDIO_DIV_SIZE];

	memcpy((void *)divvalues,(void*)(0xf4000000),sizeof(divvalues));
	a = m = 0;
	for(i = 0; i < ARRAY_SIZE(divvalues); i ++) {
		if(a < ARRAY_SIZE(audio_rate)) {
			audiodiv_apll[a].rate = audio_rate[a];
			audiodiv_apll[a].savem = divvalues[i].savem;
			audiodiv_apll[a].saven = divvalues[i].saven;
			/* printk("audiodiv_apll[a].rate = %d, audiodiv_apll[a].savem =  %d, audiodiv_apll[a].saven = %d\n", */
			/* 	audiodiv_apll[a].rate, audiodiv_apll[a].savem, audiodiv_apll[a].saven); */
			a++;
		} else {
			audiodiv_mpll[m].rate = audio_rate[m];
			audiodiv_mpll[m].savem = divvalues[i].savem;
			audiodiv_mpll[m].saven = divvalues[i].saven;
			/* printk("audiodiv_mpll[m].rate = %d, audiodiv_mpll[m].savem =  %d, audiodiv_mpll[m].saven = %d\n", */
			/* 	audiodiv_mpll[m].rate, audiodiv_mpll[m].savem, audiodiv_mpll[m].saven); */
			m ++;
		}
	}

}
void __init init_cgu_audio_clk(struct clk *clk)
{
	int no,id,tmp_val;
	unsigned long flags;
	if(audiodiv_apll[0].rate == 0 && audiodiv_mpll[0].rate == 0)
		get_audio_div();
	if (clk->flags & CLK_FLG_PARENT) {
		id = CLK_PARENT(clk->flags);
		clk->parent = get_clk_from_id(id);
	} else {
		clk->parent = cgu_audio_get_parent(clk);
	}
	no = CLK_CGU_AUDIO_NO(clk->flags);
	cgu_clks[no].cache = 0;
	if(cgu_audio_is_enabled(clk)) {
		clk->flags |= CLK_FLG_ENABLE;
	}
	clk->rate = cgu_audio_get_rate(clk);
	spin_lock_irqsave(&cpm_cgu_lock,flags);
	tmp_val = cpm_inl(cgu_clks[no].off);
	tmp_val &= ~0x3fffff;
	tmp_val |= 1<<13|1;
	if((tmp_val&cgu_clks[no].en)&&(clk->rate == 24000000))
		cpm_outl(tmp_val,cgu_clks[no].off);
	else
		cgu_clks[no].cache = tmp_val;
	spin_unlock_irqrestore(&cpm_cgu_lock,flags);
	clk->ops = &clk_cgu_audio_ops;
}
