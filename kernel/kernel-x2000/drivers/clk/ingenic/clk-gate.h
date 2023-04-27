#ifndef __INGENIC_CLK_GATE_H__
#define __INGENIC_CLK_GATE_H__

#include "clk-comm.h"

extern const struct clk_ops ingenic_gate_ops;

#define GATE_INIT_DATA(_id, _name, _parent,  _flags, _plock, _phwdesc)	\
	CLK_INIT_DATA(_id, _name, _name, NULL, NULL, NULL, &ingenic_gate_ops, _parents,	\
			1, _flags, _plock, _phwdesc)

#define GATE_DESC(_regoff, _bit)	\
{	\
	.regoff = _regoff, \
	.bit = _bit,	\
	.invert = 0,	\
}

#define GATE_DESC_INVERT(_regoff, _bit)	\
{	\
	.regoff = _regoff, \
	.bit = _bit,	\
	.invert = 1,	\
}


struct ingenic_gate_hwdesc {
	u32 regoff;	 /*Clock Control Register*/
	u8 bit:6;	 /*Clock Source select shift in register*/
	u8 invert:1;	 /*Normal 1 is gate, 0 is ungate, invert 1 is ungate, 0 is gate*/
};
#endif /*__INGENIC_CLK_GATE_H__*/
