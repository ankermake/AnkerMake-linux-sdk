#include <linux/mmc/host.h>
#include <linux/gpio.h>
#include <linux/err.h>
#include <linux/delay.h>

#include <mach/jzmmc.h>

#include "board.h"
#define RESET               0
#define NORMAL              1

#ifdef CONFIG_JZMMC_V11_MMC0
#define KBYTE				(1024LL)
#define MBYTE				((KBYTE)*(KBYTE))
struct mmc_partition_info inand_partition_info[] = {
	[0] = {"mbr",           0,       512,   0}, 	//0 - 512KB
	[1] = {"xboot",		0,     3*MBYTE, 0}, 	//0 - 2MB
	[2] = {"boot",      3*MBYTE,   8*MBYTE, 0}, 	//3MB - 8MB
	[3] = {"recovery", 11*MBYTE,   8*MBYTE, 0}, 	//12MB - 8MB
	[4] = {"misc",     19*MBYTE,   4*MBYTE, 0}, 	//21MB - 4MB
	[5] = {"battery",  23*MBYTE,   1*MBYTE, 0}, 	//26MB - 1MB
	[6] = {"cache",    24*MBYTE,  30*MBYTE, 1}, 	//28MB - 30MB
	[7] = {"device_id",54*MBYTE,   2*MBYTE, 0},	//59MB - 2MB
	[8] = {"system",   56*MBYTE, 512*MBYTE, 1}, 	//64MB - 512MB
	[9] = {"data",    568*MBYTE, 1024*MBYTE,1}, 	//580MB - 1024MB
};

static struct mmc_recovery_info inand_recovery_info = {
	.partition_info			= inand_partition_info,
	.partition_num			= ARRAY_SIZE(inand_partition_info),
	.permission			= MMC_BOOT_AREA_PROTECTED,
	.protect_boundary		= 19*MBYTE,
};

struct jzmmc_platform_data inand_pdata = {
	.removal  			= DONTCARE,
	.sdio_clk			= 0,
	.ocr_avail			= MMC_VDD_32_33 | MMC_VDD_33_34,
	.capacity  			= MMC_CAP_SD_HIGHSPEED | MMC_CAP_MMC_HIGHSPEED | MMC_CAP_4_BIT_DATA | MMC_CAP_NONREMOVABLE,
	.pm_flags			= 0,
	.max_freq			= CONFIG_MMC0_MAX_FREQ,
	.recovery_info			= &inand_recovery_info,
	.gpio				= NULL,
#ifdef CONFIG_MMC0_PIO_MODE
	.pio_mode			= 1,
#else
	.pio_mode			= 0,
#endif
	.private_init			= NULL,
};
#endif

#ifdef CONFIG_JZMMC_V11_MMC2
static struct card_gpio tf_gpio = {
	.cd				= {GPIO_SD0_CD_N,	LOW_ENABLE},
	.wp				= {-1,			-1},
	.pwr			= {GPIO_SD0_VCC_EN_N, LOW_ENABLE},
	.rst			= {GPIO_SD0_RESET, LOW_ENABLE},
};

struct jzmmc_platform_data tf_pdata = {
	.removal  			= REMOVABLE,
	.sdio_clk			= 0,
	.ocr_avail			= MMC_VDD_32_33 | MMC_VDD_33_34,
	.capacity  			= MMC_CAP_SD_HIGHSPEED | MMC_CAP_MMC_HIGHSPEED | MMC_CAP_4_BIT_DATA,
	.pm_flags			= 0,
	.type               = REMOVABLE,
	.recovery_info			= NULL,
	.gpio				= &tf_gpio,
	.max_freq                       = CONFIG_MMC2_MAX_FREQ,
#ifdef CONFIG_MMC2_PIO_MODE
	.pio_mode                       = 1,
#else
	.pio_mode                       = 0,
#endif
};
#endif

#ifdef CONFIG_JZMMC_V11_MMC1
#ifdef CONFIG_BCMDHD_1_141_66
int bcm_ap6212_wlan_init(void);
#endif
struct jzmmc_platform_data sdio_pdata = {
	.removal  			= MANUAL,
	.sdio_clk			= 1,
	.ocr_avail			= MMC_VDD_32_33 | MMC_VDD_33_34,
	.capacity  			= MMC_CAP_SD_HIGHSPEED | MMC_CAP_MMC_HIGHSPEED | MMC_CAP_4_BIT_DATA,
	.pm_flags			= MMC_PM_IGNORE_PM_NOTIFY,
	.type               = MANUAL,
	.max_freq                       = CONFIG_MMC1_MAX_FREQ,
	.recovery_info			= NULL,
	.gpio				= NULL,
#ifdef CONFIG_MMC1_PIO_MODE
	.pio_mode			= 1,
#else
	.pio_mode			= 0,
#endif
#ifdef CONFIG_BCMDHD_1_141_66
	.private_init			= bcm_ap6212_wlan_init,
#endif
};
#endif
