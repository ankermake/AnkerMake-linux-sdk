#ifndef __DDR2_M14D1G1664A_CONFIG_H
#define	__DDR2_M14D1G1664A_CONFIG_H

/*
 * CL:3,50M ~ 200M
 * CL:4,200M ~ 266M
 * CL:5,266M ~ 400M
 * CL:6,
 * CL:7,400M ~ 533M
 *
 * */
#ifndef CONFIG_DDR2_M14D1G1664A_MEM_FREQ
#define CONFIG_DDR2_M14D1G1664A_MEM_FREQ CONFIG_SYS_MEM_FREQ
#endif

#if (CONFIG_DDR_SEL_PLL == MPLL)
#define CONFIG_SYS_PLL_FREQ CONFIG_SYS_MPLL_FREQ
#else
#define CONFIG_SYS_PLL_FREQ CONFIG_SYS_APLL_FREQ
#endif

#define CONFIG_DDR_DATA_RATE (CONFIG_DDR2_M14D1G1664A_MEM_FREQ * 2)

#if((CONFIG_SYS_PLL_FREQ % CONFIG_DDR2_M14D1G1664A_MEM_FREQ) ||\
	(CONFIG_SYS_PLL_FREQ / CONFIG_DDR2_M14D1G1664A_MEM_FREQ < 0) ||\
	(CONFIG_SYS_PLL_FREQ / CONFIG_DDR2_M14D1G1664A_MEM_FREQ > 15))
#error DDR memoryclock division ratio should be an integer between 1 and 16, check CONFIG_SYS_MPLL_FREQ and CONFIG_DDR2_M14D1G1664A_MEM_FREQ
#endif

#if ((CONFIG_DDR_DATA_RATE > 100000000) &&\
		(CONFIG_DDR_DATA_RATE <= 400000000))
#define CONFIG_DDR_CL	3
#elif((CONFIG_DDR_DATA_RATE > 400000000) &&\
		(CONFIG_DDR_DATA_RATE <= 533000000))
#define CONFIG_DDR_CL	4
#elif((CONFIG_DDR_DATA_RATE > 533000000) &&\
		(CONFIG_DDR_DATA_RATE <= 800000000))
#define CONFIG_DDR_CL	5
#elif((CONFIG_DDR_DATA_RATE > 800000000) &&\
		(CONFIG_DDR_DATA_RATE <= 1066000000))
#define CONFIG_DDR_CL	7
#else
#define CONFIG_DDR_CL	-1
#endif

#define CONFIG_DDR_AL	0

#if(-1 == CONFIG_DDR_CL)
#error CONFIG_DDR2_M14D1G1664A_MEM_FREQ don't support, check data_rate range
#endif

static inline void DDR2_M14D1G1664A_init(void *data)
{
	struct ddr_chip_info *c = (struct ddr_chip_info *)data;
	unsigned int RL = CONFIG_DDR_CL + CONFIG_DDR_AL;

	c->DDR_ROW = 13;
	c->DDR_COL = 10;
	c->DDR_ROW1 = 13;
	c->DDR_COL1 = 10;

	c->DDR_BANK8 = 1;
	c->DDR_CL = CONFIG_DDR_CL;

	c->DDR_tRAS = DDR__ns(45);
	c->DDR_tRTP = DDR__ps(7500);
	c->DDR_tRP = DDR__ps(16000);
	c->DDR_tRCD = DDR__ps(16000);
	c->DDR_tRC = DDR__ps(56250);
	c->DDR_tRRD = DDR__ns(10);
	c->DDR_tWR = DDR__ns(15);
	c->DDR_tWTR = DDR__ps(7500);
	c->DDR_tRFC = DDR__ps(127500);
	c->DDR_tMINSR = DDR__ns(60);
	c->DDR_tXP = DDR__tck(3);
	c->DDR_tMRD = DDR__tck(2);

	c->DDR_BL =  8;
	c->DDR_RL = DDR__tck(RL);
	c->DDR_WL = DDR__tck(RL - 1);
	c->DDR_tCCD = DDR__tck(2);
	c->DDR_tRTW = (((c->DDR_BL > 4) ? 6 : 4) + 1);
	c->DDR_tFAW = DDR__ns(45);
	c->DDR_tCKE = DDR__tck(3);
	c->DDR_tCKESR = DDR__tck(3);
	c->DDR_tRDLAT = DDR__tck(7);
	c->DDR_tWDLAT = DDR__tck(3);

	c->DDR_tXSNR = (c->DDR_tRFC + DDR__ns(10));
	c->DDR_tXSRD = DDR__tck(200);
	c->DDR_tCKSRE = DDR__ns(10000);
	c->DDR_tREFI = DDR__ns(7800);

	c->DDR_CLK_DIV = 1;
}

#define DDR2_M14D1G1664A {					\
	.name 	= "M14D1G1664A",					\
	.id	= DDR_CHIP_ID(VENDOR_ESMT, TYPE_DDR2, MEM_128M),	\
	.type	= DDR2,						\
	.freq	= CONFIG_DDR2_M14D1G1664A_MEM_FREQ,			\
	.size	= 128,						\
	.init	= DDR2_M14D1G1664A_init,				\
}

#endif /* __DDR2_CONFIG_H */
