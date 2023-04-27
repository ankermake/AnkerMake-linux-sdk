/*
 * =====================================================================================
 *
 *       Filename:  DDR3_NK5CC128M8HKX.h
 *
 *    Description:
 *
 *        Version:  1.0
 *        Created:  2020年09月21日 17时55分13秒
 *       Revision:  none
 *       Compiler:  gcc
 *
 *         Author:  YOUR NAME (),
 *   Organization:
 *
 * =====================================================================================
 */

#ifndef __DDR3_NK5CC128M8HKX_H__
#define __DDR3_NK5CC128M8HKX_H__



/*
 * CL:6, CWL:5	303M ~ 400M
 *
 * CL:7, CWL:6	400M ~ 533M
 * CL:8, CWL:6	400M ~ 533M
 *
 * CL:9, CWL:7  533M ~ 666M
 * CL:10, CWL:7 533M ~ 666M
 *
 * CL:11, CWL:8 666M ~ 800M
 *
 * CL:13, CWL:9 800M ~ 933M
 *
 * */

#ifndef CONFIG_DDR3_NK5CC128M8HKX_MEM_FREQ
#define CONFIG_DDR3_NK5CC128M8HKX_MEM_FREQ CONFIG_SYS_MEM_FREQ
#endif

#if (CONFIG_DDR_SEL_PLL == MPLL)
#define CONFIG_SYS_PLL_FREQ CONFIG_SYS_MPLL_FREQ
#else
#define CONFIG_SYS_PLL_FREQ CONFIG_SYS_APLL_FREQ
#endif

#define CONFIG_DDR_DATA_RATE (CONFIG_DDR3_NK5CC128M8HKX_MEM_FREQ * 2)

#if((CONFIG_SYS_PLL_FREQ % CONFIG_DDR3_NK5CC128M8HKX_MEM_FREQ) ||\
	(CONFIG_SYS_PLL_FREQ / CONFIG_DDR3_NK5CC128M8HKX_MEM_FREQ < 0) ||\
	(CONFIG_SYS_PLL_FREQ / CONFIG_DDR3_NK5CC128M8HKX_MEM_FREQ > 15))
#error DDR memoryclock division ratio should be an integer between 1 and 16, check CONFIG_SYS_MPLL_FREQ and CONFIG_DDR3_NK5CC128M8HKX_MEM_FREQ
#endif

#if ((CONFIG_DDR_DATA_RATE > 660000000) && (CONFIG_DDR_DATA_RATE <= 800000000))
#define CONFIG_DDR_CL	6
#define CONFIG_DDR_CWL	5
#elif ((CONFIG_DDR_DATA_RATE > 800000000) && (CONFIG_DDR_DATA_RATE <= 1066000000))
#define CONFIG_DDR_CL	8
#define CONFIG_DDR_CWL	6
#elif ((CONFIG_DDR_DATA_RATE > 1066000000) && (CONFIG_DDR_DATA_RATE < 1332000000))
#define CONFIG_DDR_CL	10
#define CONFIG_DDR_CWL	7
#elif ((CONFIG_DDR_DATA_RATE >= 1332000000) && (CONFIG_DDR_DATA_RATE < 1600000000))
#define CONFIG_DDR_CL	10	//10 is ok
#define CONFIG_DDR_CWL	8
#elif ((CONFIG_DDR_DATA_RATE >= 1600000000) && (CONFIG_DDR_DATA_RATE < 1866000000))
#define CONFIG_DDR_CL	13
#define CONFIG_DDR_CWL	9
#else
#define CONFIG_DDR_CL	-1
#define CONFIG_DDR_CWL	-1
#endif

#if(-1 == CONFIG_DDR_CL)
#error CONFIG_DDR3_NK5CC128M8HKX_MEM_FREQ don't support, check data_rate range
#endif

static inline void DDR3_NK5CC128M8HKX_init(void *data)
{
	struct ddr_chip_info *c = (struct ddr_chip_info *)data;


	c->DDR_ROW  		= 13,
	c->DDR_ROW1 		= 13,
	c->DDR_COL  		= 10,
	c->DDR_COL1 		= 10,
	c->DDR_BANK8 		= 1,
	c->DDR_BL	   	= 8,
	c->DDR_CL	   	= CONFIG_DDR_CL,
	c->DDR_CWL	   	= CONFIG_DDR_CWL,

	c->DDR_RL	   	= DDR__tck(c->DDR_CL),
	c->DDR_WL	   	= DDR__tck(c->DDR_CWL),

	c->DDR_tRAS  		= DDR__ns(34);
	c->DDR_tRTP  		= DDR_SELECT_MAX__tCK_ps(4, 7500);
	c->DDR_tRP   		= DDR__ns(14);
	c->DDR_tRCD  		= DDR__ns(14);
	c->DDR_tRC   		= c->DDR_tRAS + c->DDR_tRP;
	c->DDR_tRRD  		= DDR_SELECT_MAX__tCK_ps(4, 6000);
	c->DDR_tWR   		= DDR__ns(15);
	c->DDR_tWTR  		= DDR_SELECT_MAX__tCK_ps(4, 7500);
	c->DDR_tCCD  		= DDR__tck(4);
	c->DDR_tFAW  		= DDR__ns(30);	//1333

	c->DDR_tRFC  		= DDR__ns(110);
	c->DDR_tREFI 		= DDR__ns(3900);

	c->DDR_tCKE  		= DDR_SELECT_MAX__tCK_ps(3, 5000);
	c->DDR_tCKESR 		= c->DDR_tCKE + DDR__tck(1);
	c->DDR_tXP  		= DDR_SELECT_MAX__tCK_ps(3, 6000);
}

#define DDR3_NK5CC128M8HKX {					\
	.name 	= "NK5CC128M8HKX",					\
	.id	= DDR_CHIP_ID(VENDOR_NANYA, TYPE_DDR3, MEM_256M),	\
	.type	= DDR3,						\
	.freq	= CONFIG_DDR3_NK5CC128M8HKX_MEM_FREQ,			\
	.size	= 256,						\
	.init	= DDR3_NK5CC128M8HKX_init,				\
}


#endif
