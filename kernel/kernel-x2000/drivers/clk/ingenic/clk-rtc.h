#ifndef __CLK_RTC_H__
#define __CLK_RTC_H__

#include "clk-comm.h"

extern const struct clk_ops ingenic_rtc_ops;

struct ingenic_rtc_hwdesc {
	u32 regoff;
	u8 bit_cs;
	u32 div;
};

#endif /*__CLK_RTC_H__*/
