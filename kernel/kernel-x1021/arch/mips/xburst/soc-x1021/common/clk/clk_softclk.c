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
			spin_lock_irqsave(&softclk_lock,flags);
			udelay(4200);
			i2s_start();
			spin_unlock_irqrestore(&softclk_lock,flags);
		}else if((bit ==1) && !(soft_gate & 1) ){	//normal amic enable
			pr_debug("normal amic enable\n");
			spin_lock_irqsave(&softclk_lock,flags);
			i2s_start();
			spin_unlock_irqrestore(&softclk_lock,flags);
		}

	}else{	//disable
		if(bit == 1){	//amic disable
			pr_debug("amic disable\n");
			spin_lock_irqsave(&softclk_lock,flags);
			i2s_stop();
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
