#include <linux/mmc/host.h>
#include <linux/regulator/consumer.h>
#include <linux/gpio.h>
#include <linux/wakelock.h>
#include <linux/err.h>
#include <linux/delay.h>

#include <mach/jzmmc.h>

#include "board_base.h"

static inline int __sdio_private_init(void)
{
    printk("dummy sdio_private_init\n");
    return 0;
}
int sdio_private_init(void) __attribute__ ((weak,alias("__sdio_private_init")));

#ifdef CONFIG_JZMMC_V12_MMC0
#ifdef CONFIG_RTL8822CS
/* common pdata for both tf_card and sdio wifi on audio board */
struct jzmmc_platform_data tf_pdata = {
    .removal             = MANUAL,
    .sdio_clk            = 1,
    .ocr_avail           = MMC_VDD_32_33 | MMC_VDD_33_34,
    .capacity            = MMC_CAP_4_BIT_DATA | MMC_CAP_NONREMOVABLE | MMC_CAP_SDIO_IRQ,
    .pm_flags            = MMC_PM_IGNORE_PM_NOTIFY,
    .recovery_info       = NULL,
    .gpio                = NULL,
    .max_freq            = CONFIG_MMC0_MAX_FREQ,
#ifdef CONFIG_MMC0_PIO_MODE
    .pio_mode            = 1,
#else
    .pio_mode            = 0,
#endif
    .private_init        = sdio_private_init,
};
#else
static struct card_gpio tf_gpio = {
    .wp     = {GPIO_MMC_WP_N,    GPIO_MMC_WP_N_LEVEL},
    .cd     = {GPIO_MMC_CD_N,    GPIO_MMC_CD_N_LEVEL},
    .pwr    = {GPIO_MMC_PWR,     GPIO_MMC_PWR_LEVEL},
    .rst    = {GPIO_MMC_RST_N,   GPIO_MMC_RST_N_LEVEL},
};

/* common pdata for both tf_card and sdio wifi on audio board */
struct jzmmc_platform_data tf_pdata = {
    .removal              = REMOVABLE,
    .sdio_clk             = 0,
    .ocr_avail            = MMC_VDD_32_33 | MMC_VDD_33_34,
    .capacity             = MMC_CAP_SD_HIGHSPEED | MMC_CAP_MMC_HIGHSPEED,
    .pm_flags             = 0,
    .recovery_info        = NULL,
    .gpio                 = &tf_gpio,
    .max_freq             = CONFIG_MMC0_MAX_FREQ,
#ifdef CONFIG_MMC0_PIO_MODE
    .pio_mode             = 1,
#else
    .pio_mode             = 0,
#endif
};
#endif
#endif    /*CONFIG_JZMMC_V12_MMC0*/

#ifdef CONFIG_JZMMC_V12_MMC1
struct jzmmc_platform_data sdio_pdata = {
    .removal            = MANUAL,
    .sdio_clk           = 1,
    .ocr_avail          = MMC_VDD_32_33 | MMC_VDD_33_34,

#ifdef CONFIG_ESP8089
    .capacity           = MMC_CAP_4_BIT_DATA | MMC_CAP_SDIO_IRQ,
#elif CONFIG_RTL8822CS
    .capacity           = MMC_CAP_4_BIT_DATA | MMC_CAP_NONREMOVABLE | MMC_CAP_SDIO_IRQ,
#else
    .capacity           = MMC_CAP_4_BIT_DATA | MMC_CAP_NONREMOVABLE | MMC_CAP_SD_HIGHSPEED | MMC_CAP_SDIO_IRQ,
#endif
    .pm_flags           = MMC_PM_IGNORE_PM_NOTIFY,
    .recovery_info      = NULL,
    .gpio               = NULL,
    .max_freq           = CONFIG_MMC1_MAX_FREQ,
#ifdef CONFIG_MMC1_PIO_MODE
    .pio_mode           = 1,
#else
    .pio_mode           = 0,
#endif
    .private_init       = sdio_private_init,
};
#endif    /*CONFIG_JZMMC_V12_MMC1*/
