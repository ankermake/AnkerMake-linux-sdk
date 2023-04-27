#include <linux/platform_device.h>
#include <linux/spi/spi.h>
#include <linux/spi/spi_gpio.h>
#include <linux/mtd/mtd.h>
#include <linux/mtd/partitions.h>
#include <mach/jzssi.h>
#include "board_base.h"
#include <mach/jzssi.h>
#include <microarray.h>
#include <ax88796_spi.h>
#ifdef CONFIG_LCD_RM68172
extern struct lcd_platform_data rm68172_pdata;
#endif
#ifdef CONFIG_LCD_ST7701
extern struct lcd_platform_data st7701s_pdata;
#endif

#if defined(CONFIG_JZ_SPI0) || defined(CONFIG_SPI_GPIO)

struct spi_board_info jz_spi0_board_info[] = {
#ifdef CONFIG_LCD_RM68172
       [0] = {
        .modalias        = "rm68172_tft",
        .platform_data   = &rm68172_pdata,
#ifdef CONFIG_SPI_GPIO
        .controller_data = (void *)SPI_CHIP_ENABLE, /* cs for spi gpio */
#else
        .controller_data = NULL, /* cs for spi gpio */
#endif
        .max_speed_hz    = 12000000,
        .bus_num         = 2,
        .chip_select     = 0,
    },
#endif
#ifdef CONFIG_LCD_ST7701
     [0] = {
        .modalias        = "st7701s_tft",
        .platform_data   = &st7701s_pdata,
#ifdef CONFIG_SPI_GPIO
        .controller_data = (void *)SPI_CHIP_ENABLE, /* cs for spi gpio */
#else
        .controller_data = NULL, /* cs for spi gpio */
#endif
        .max_speed_hz    = 12000000,
        .bus_num         = 2,
        .chip_select     = 0,
    },
#endif
};
int jz_spi0_devs_size = ARRAY_SIZE(jz_spi0_board_info);
#endif

#ifdef CONFIG_SPI_GPIO

static struct spi_gpio_platform_data jz_spi_gpio_data = {

    .sck	= GPIO_SPI0_SCK,
    .mosi	= GPIO_SPI0_MOSI,
    .miso	= GPIO_SPI0_MISO,
    .num_chipselect = 1,
};
#ifndef GPIO_SPI0_BUS_NUM
#define GPIO_SPI0_BUS_NUM 0
#endif

struct platform_device jz_spi_gpio_device = {
    .name   = "spi_gpio",
    .id     = GPIO_SPI0_BUS_NUM,
    .dev    = {
        .platform_data = &jz_spi_gpio_data,
    },
};


#endif
#ifdef CONFIG_JZ_SPI0
#if defined(CONFIG_FINGERPRINT_MICROARRAY)
static struct microarray_platform_data spi_microarray_pdata = {
    .power_2v8         = FINGERPRINT_POWER_2V8,
    .power_1v8         = FINGERPRINT_POWER_1V8,
    .power_en          = FINGERPRINT_POWER_EN,
    .gpio_int          = FINGERPRINT_INT,
    .reset             = FINGERPRINT_RESET,
};
#endif
#if defined(CONFIG_AX88796C_SPI)
static struct ax88796c_spi_pdata ax88796c_spi_pdata = {
    .gpio_irq       = AX88796C_SPI_IRQ_PIN,
    .gpio_reset     = AX88796C_SPI_RESET_PIN,
};
#endif

struct spi_board_info spi0_devs_info[] = {
    {
        .modalias        = "spidev",
        .mode            = SPI_MODE_0,
        .max_speed_hz    = 10000000,
        .bus_num         = 0,
        .chip_select     = 0,
        .controller_data = NULL, /* cs for spi gpio */
        .platform_data   = NULL,
    },
     {
        .modalias        = "spidev",
        .mode            = SPI_MODE_3,
        .max_speed_hz    = 10000000,
        .bus_num         = 0,
        .chip_select     = 1,
        .controller_data = NULL, /* cs for spi gpio */
        .platform_data   = NULL,
    },
#if defined(CONFIG_FINGERPRINT_MICROARRAY)
    {
        .modalias        = MICROARRAY_DRV_NAME,
        .mode            = SPI_MODE_0,
        .max_speed_hz    = 6000000,
        .bus_num         = 0,
        .chip_select     = 2,
        .controller_data = NULL, /* cs for spi gpio */
        .platform_data   = &ax88796c_spi_pdata,
    },
#endif
#if defined(CONFIG_AX88796C_SPI)
    {
        .modalias        = "ax88796c_spi",
        .mode            = SPI_MODE_3,
        .max_speed_hz    = 50000000,
        .bus_num         = 0,
        .chip_select     = 2,
        .controller_data = NULL, /* cs for spi gpio */
        .platform_data   = &ax88796c_spi_pdata,
    },
#endif
};
struct spi_cs_ctrl chipselect_ctrl[] = {
    {
        .cs_pin  = SPI0_CHIP_SELECT0,
        .hold_cs = true,
    },
    {
        .cs_pin  = SPI0_CHIP_SELECT1,
        .hold_cs = true,
    },
#if defined(CONFIG_FINGERPRINT_MICROARRAY)
    {
        .cs_pin  = SPI0_CHIP_SELECT1,
        .hold_cs = true,
    },
#endif
#if defined(CONFIG_AX88796C_SPI)
    {
        .cs_pin  = AX88796C_SPI_CS_PIN,
        .hold_cs = true,
    },
#endif
};

struct jz_spi_info spi0_info_cfg = {
    .chnl                = 0,
    .bus_num             = 0,
#ifdef CONFIG_JZ_SPI_CLK_SOURCE_EXTERNAL
    .max_clk             = 24000000,
#else
    .max_clk             = 50000000,
#endif
    .num_chipselect      = 4,
    .allow_cs_same       = 1,
    .chipselect          = chipselect_ctrl,
#ifdef CONFIG_JZ_SPI_BOARD_INFO_REGISTER
    .board_info          = spi0_devs_info,
    .board_size          = ARRAY_SIZE(spi0_devs_info),
#endif
};


#endif /* CONFIG_JZ_SPI0 */


