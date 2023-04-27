/*
 * =====================================================================================
 *
 *       Filename:  DDR3L_W634GU6QB_11.h
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

#ifndef __DDR3L_W634GU6QB_11_H__
#define __DDR3L_W634GU6QB_11_H__



/*
 * CL:5, CWL:5  300M ~ 330M
 * CL:6, CWL:5	330M ~ 400M
 * CL:7, CWL:6
 * CL:8, CWL:6	400M ~ 533M
 * CL:9, CWL:7
 * CL:10, CWL:7 533M ~ 666M
 * CL:11, CWL:8
 * CL:13, CWL:9 800M ~ 933M
 * CL:14, CWL:10
 *
 * */

#ifndef CONFIG_DDR3L_W634GU6QB_11_MEM_FREQ
#define CONFIG_DDR3L_W634GU6QB_11_MEM_FREQ CONFIG_SYS_MEM_FREQ
#endif

#if (CONFIG_DDR_SEL_PLL == MPLL)
#define CONFIG_SYS_PLL_FREQ CONFIG_SYS_MPLL_FREQ
#else
#define CONFIG_SYS_PLL_FREQ CONFIG_SYS_APLL_FREQ
#endif

#define CONFIG_DDR_DATA_RATE (CONFIG_DDR3L_W634GU6QB_11_MEM_FREQ * 2)

#if((CONFIG_SYS_PLL_FREQ % CONFIG_DDR3L_W634GU6QB_11_MEM_FREQ) ||\
	(CONFIG_SYS_PLL_FREQ / CONFIG_DDR3L_W634GU6QB_11_MEM_FREQ < 0) ||\
	(CONFIG_SYS_PLL_FREQ / CONFIG_DDR3L_W634GU6QB_11_MEM_FREQ > 15))
#error DDR memoryclock division ratio should be an integer between 1 and 16, check CONFIG_SYS_MPLL_FREQ and CONFIG_DDR3L_W634GU6QB_11_MEM_FREQ
#endif

#if ((CONFIG_DDR_DATA_RATE > 600000000) && (CONFIG_DDR_DATA_RATE <= 660000000))
#define CONFIG_DDR_CL	5
#define CONFIG_DDR_CWL	5
#elif((CONFIG_DDR_DATA_RATE > 660000000) && (CONFIG_DDR_DATA_RATE <= 800000000))
#define CONFIG_DDR_CL	6
#define CONFIG_DDR_CWL	5
#elif((CONFIG_DDR_DATA_RATE > 800000000) && (CONFIG_DDR_DATA_RATE <= 1066000000))
#define CONFIG_DDR_CL	7
#define CONFIG_DDR_CWL	6
#elif((CONFIG_DDR_DATA_RATE > 1066000000) && (CONFIG_DDR_DATA_RATE <= 1333000000))
#define CONFIG_DDR_CL	9
#define CONFIG_DDR_CWL	7
#elif((CONFIG_DDR_DATA_RATE > 1333000000) && (CONFIG_DDR_DATA_RATE <= 1600000000))
#define CONFIG_DDR_CL	10
#define CONFIG_DDR_CWL	8
#elif((CONFIG_DDR_DATA_RATE > 1600000000) && (CONFIG_DDR_DATA_RATE <= 1868000000))
#define CONFIG_DDR_CL	13
#define CONFIG_DDR_CWL	9
#else
#define CONFIG_DDR_CL	-1
#define CONFIG_DDR_CWL	-1
#endif

#if(-1 == CONFIG_DDR_CL)
#error CONFIG_DDR3L_W634GU6QB_11_MEM_FREQ don't support, check data_rate range
#endif

static inline void DDR3L_W634GU6QB_11_init(void *data)
{
	struct ddr_chip_info *c = (struct ddr_chip_info *)data;


	c->DDR_ROW  		= 15,
	c->DDR_ROW1 		= 15,
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
//	c->DDR_tRC   		= c->DDR_tRAS + c->DDR_tRP;
	c->DDR_tRC   		= DDR__ns(48);
	c->DDR_tRRD  		= DDR_SELECT_MAX__tCK_ps(4, 6000);
	c->DDR_tWR   		= DDR__ns(15);
	c->DDR_tWTR  		= DDR_SELECT_MAX__tCK_ps(4, 7500);
	c->DDR_tCCD  		= DDR__tck(4);
	c->DDR_tFAW  		= DDR__ns(35);

	c->DDR_tRFC  		= DDR__ns(260);
	c->DDR_tREFI 		= DDR__ns(7800);

	c->DDR_tCKE  		= DDR_SELECT_MAX__tCK_ps(3, 5000);
	c->DDR_tCKESR 		= c->DDR_tCKE + DDR__tck(1);
	c->DDR_tXP  		= DDR_SELECT_MAX__tCK_ps(3, 6000);
}

#define DDR3L_W634GU6QB_11 {					\
	.name 	= "W634GU6QB-11",					\
	.id	= DDR_CHIP_ID(VENDOR_WINBOND, TYPE_DDR3, MEM_1G),	\
	.type	= DDR3,						\
	.freq	= CONFIG_DDR3L_W634GU6QB_11_MEM_FREQ,			\
	.size	= 1024,						\
	.init	= DDR3L_W634GU6QB_11_init,				\
}


#endif
