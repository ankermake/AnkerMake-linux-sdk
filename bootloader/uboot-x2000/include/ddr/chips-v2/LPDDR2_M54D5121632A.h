/*
 * =====================================================================================
 *
 *       Filename:  LPDDR2_M54D5121632A.h
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

#ifndef __LPDDR2_M54D5121632A_H__
#define __LPDDR2_M54D5121632A_H__

/*
 * RL:3,WL:1,25M ~ 166M
 * RL:3,WL:1,166M ~ 200M
 * RL:4,WL:2,200M ~ 266M
 * RL:5,WL:2,266M ~ 333M
 * RL:6,WL:3,333M ~ 400M
 * RL:7,WL:4,400M ~ 466M
 * RL:8,WL:4,466M ~ 533M
 *
 * */
#ifndef CONFIG_LPDDR2_M54D5121632A_MEM_FREQ
#define CONFIG_LPDDR2_M54D5121632A_MEM_FREQ CONFIG_SYS_MEM_FREQ
#endif

#if (CONFIG_DDR_SEL_PLL == MPLL)
#define CONFIG_SYS_PLL_FREQ CONFIG_SYS_MPLL_FREQ
#else
#define CONFIG_SYS_PLL_FREQ CONFIG_SYS_APLL_FREQ
#endif

#define CONFIG_DDR_DATA_RATE (CONFIG_LPDDR2_M54D5121632A_MEM_FREQ * 2)

#if((CONFIG_SYS_PLL_FREQ % CONFIG_LPDDR2_M54D5121632A_MEM_FREQ) ||\
	(CONFIG_SYS_PLL_FREQ / CONFIG_LPDDR2_M54D5121632A_MEM_FREQ < 0) ||\
	(CONFIG_SYS_PLL_FREQ / CONFIG_LPDDR2_M54D5121632A_MEM_FREQ > 15))
#error DDR memoryclock division ratio should be an integer between 1 and 16, check CONFIG_SYS_MPLL_FREQ and CONFIG_LPDDR2_M54D5121632A_MEM_FREQ
#endif

#if	((CONFIG_DDR_DATA_RATE > 50000000) &&\
		(CONFIG_DDR_DATA_RATE <= 333000000))
#define CONFIG_DDR_RL	3
#define CONFIG_DDR_WL	1
#elif((CONFIG_DDR_DATA_RATE > 333000000) &&\
		(CONFIG_DDR_DATA_RATE <= 400000000))
#define CONFIG_DDR_RL	3
#define CONFIG_DDR_WL	1
#elif((CONFIG_DDR_DATA_RATE > 400000000) &&\
		(CONFIG_DDR_DATA_RATE <= 533000000))
#define CONFIG_DDR_RL	4
#define CONFIG_DDR_WL	2
#elif((CONFIG_DDR_DATA_RATE > 533000000) &&\
		(CONFIG_DDR_DATA_RATE <= 667000000))
#define CONFIG_DDR_RL	5
#define CONFIG_DDR_WL	2
#elif((CONFIG_DDR_DATA_RATE > 667000000) &&\
		(CONFIG_DDR_DATA_RATE <= 800000000))
#define CONFIG_DDR_RL	6
#define CONFIG_DDR_WL	3
#elif((CONFIG_DDR_DATA_RATE > 800000000) &&\
		(CONFIG_DDR_DATA_RATE <= 933000000))
#define CONFIG_DDR_RL	7
#define CONFIG_DDR_WL	4
#elif((CONFIG_DDR_DATA_RATE > 933000000) &&\
		(CONFIG_DDR_DATA_RATE <= 1066000000))
#define CONFIG_DDR_RL	8
#define CONFIG_DDR_WL	4
#else
#define CONFIG_DDR_RL	-1
#define CONFIG_DDR_WL	-1
#endif

#if(-1 == CONFIG_DDR_RL)
#error CONFIG_LPDDR2_M54D5121632A_MEM_FREQ don't support, check data_rate range
#endif

static inline void LPDDR2_M54D5121632A_init(void *data)
{
	struct ddr_chip_info *c = (struct ddr_chip_info *)data;


	c->DDR_ROW  		= 13,
	c->DDR_ROW1 		= 13,
	c->DDR_COL  		= 10,
	c->DDR_COL1 		= 10,
	c->DDR_BANK8 		= 0,
	c->DDR_BL	   	= 8,
	c->DDR_RL	   	= CONFIG_DDR_RL,
	c->DDR_WL	   	= CONFIG_DDR_WL,

	c->DDR_tMRW  		= DDR__tck(5);
	c->DDR_tDQSCK 		= DDR__ps(2000);
	c->DDR_tDQSCKMAX 	= DDR__ps(10000);
	c->DDR_tRAS  		= DDR_SELECT_MAX__tCK_ps(3, 42 * 1000);
	c->DDR_tRTP  		= DDR_SELECT_MAX__tCK_ps(2, 7500);
	c->DDR_tRP   		= DDR_SELECT_MAX__tCK_ps(3, 21 * 1000);
	c->DDR_tRCD  		= DDR_SELECT_MAX__tCK_ps(3, 18 * 1000);
	c->DDR_tRC   		= c->DDR_tRAS + c->DDR_tRP;
	c->DDR_tRRD  		= DDR_SELECT_MAX__tCK_ps(2, 10 * 1000);
	c->DDR_tWR   		= DDR_SELECT_MAX__tCK_ps(3, 15 * 1000);
	c->DDR_tWTR  		= DDR_SELECT_MAX__tCK_ps(2, 7500);
	c->DDR_tCCD  		= DDR__tck(2);
	c->DDR_tFAW  		= DDR_SELECT_MAX__tCK_ps(8, 50 * 1000);

	c->DDR_tRFC  		= DDR__ns(90);
	c->DDR_tREFI 		= DDR__ns(7800);

	c->DDR_tCKE  		= DDR__tck(3);
	c->DDR_tCKESR 		= DDR_SELECT_MAX__tCK_ps(3, DDR__ns(15));
	c->DDR_tXSR  		= DDR_SELECT_MAX__tCK_ps(2, c->DDR_tRFC + DDR__ns(10));
	c->DDR_tXP  		= DDR_SELECT_MAX__tCK_ps(2, 7500);
}

#define LPDDR2_M54D5121632A {					\
	.name 	= "M54D5121632A",					\
	.id	= DDR_CHIP_ID(VENDOR_ESMT, TYPE_LPDDR2, MEM_64M),	\
	.type	= LPDDR2,						\
	.freq	= CONFIG_LPDDR2_M54D5121632A_MEM_FREQ,			\
	.size	= 64,						\
	.init	= LPDDR2_M54D5121632A_init,				\
}


#endif
