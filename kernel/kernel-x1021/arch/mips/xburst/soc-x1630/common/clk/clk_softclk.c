#include <linux/spinlock.h>
#include <linux/io.h>
#include <linux/clk.h>
#include <soc/base.h>
#include <linux/delay.h>


#include "clk.h"
#include "clk_softclk.h"




static DEFINE_SPINLOCK(softclk_lock);

static unsigned int soft_gate = 0;

int i2s_start(void)
{
	__i2s_enable_receive_dma();
	__i2s_enable_record();

	return 0;
}

int i2s_stop(void)
{
	__i2s_disable_record();

	return 0;
}

int dmic_start(void)
{
	__dmic_enable_rdms();
	__dmic_enable();
	return 0;
}

int dmic_stop(void)
{
	__dmic_disable_rdms();
	__dmic_disable();
	return 0;
}

static int softclk_enable(struct clk *clk,int on)
{
	unsigned long flags;

	int bit = CLK_SOFTCLK_BIT(clk->flags);

	if(on)
		soft_gate |= (1 << bit) ;
	else
		soft_gate &= ~(1 << bit);

	if(on){		//enable
		if(soft_gate == 7){ //aec mode
			pr_debug("aec amic dmic enable amic %s (%x:%x) dmic %s(%x:%x)\n",
					__amic_is_enable() ? "on" : "off",
					AIC0_IOBASE + AICCR,
					i2s_read_reg(AICCR),
					__dmic_is_enable() ? "on" : "off",
					DMIC_IOBASE + DMICCR0,
					dmic_read_reg(DMICCR0));
			pr_debug("aec aicsr %x dmic_fsr %x\n", i2s_read_reg(0x14), dmic_read_reg(0x38));
			spin_lock_irqsave(&softclk_lock,flags);
			dmic_start();	/*x1830 dmic start will be delay 4ms*/
			udelay(4200);
			i2s_start();
			spin_unlock_irqrestore(&softclk_lock,flags);
			pr_debug("aec amic dmic enable amic %s (%x:%x) dmic %s(%x:%x)\n",
					__amic_is_enable() ? "on" : "off",
					AIC0_IOBASE + AICCR,
					i2s_read_reg(AICCR),
					__dmic_is_enable() ? "on" : "off",
					DMIC_IOBASE + DMICCR0,
					dmic_read_reg(DMICCR0));
		}else if((bit ==1) && !(soft_gate & 1) ){	//normal amic enable
			pr_debug("normal amic enable\n");
			spin_lock_irqsave(&softclk_lock,flags);
			i2s_start();
			spin_unlock_irqrestore(&softclk_lock,flags);
		}else if((bit == 2) && !(soft_gate & 1)){	//normal dmic enable
			pr_debug("normal dmic enable\n");
			spin_lock_irqsave(&softclk_lock,flags);
			dmic_start();
			spin_unlock_irqrestore(&softclk_lock,flags);
		}

	}else{	//disable
		if(bit == 1){	//amic disable
			pr_debug("amic disable\n");
			spin_lock_irqsave(&softclk_lock,flags);
			i2s_stop();
			spin_unlock_irqrestore(&softclk_lock,flags);
		}
		if(bit == 2){	//dmic disable
			pr_debug("dmic disable\n");
			spin_lock_irqsave(&softclk_lock,flags);
			dmic_stop();
			spin_unlock_irqrestore(&softclk_lock,flags);
		}
	}

	return 0;
}



static struct clk_ops clk_softclk_ops = {
	.enable = softclk_enable,
};

void __init init_softclk_clk(struct clk *clk)
{
	int id = 0;

	if (clk->flags & CLK_FLG_PARENT) {
		id = CLK_PARENT(clk->flags);
		clk->parent = get_clk_from_id(id);
	}else
		clk->parent = get_clk_from_id(CLK_ID_EXT1);
	clk->rate = clk_get_rate(clk->parent);
	clk->ops = &clk_softclk_ops;
}
