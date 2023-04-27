#include <linux/platform_device.h>
#include <linux/spi/spi.h>
#include <linux/spi/spi_gpio.h>
#include <linux/mtd/mtd.h>
#include <linux/mtd/partitions.h>
#include <mach/jzssi.h>
#include "board_base.h"

#ifdef CONFIG_BCT3286_LED
#include <linux/spi/bct3286.h>
#endif
#ifdef CONFIG_BCT3286_LED
static unsigned char bct3286_led_order[12 * 3] = {
	7  , 8  , 6  ,
	34 , 35 , 33 ,
	25 , 26 , 24 ,
	16 , 17 , 15 ,
	10 , 11 , 9  ,
	1  , 2  , 0  ,
	28 , 29 , 27 ,
	19 , 20 , 18 ,
	13 , 14 , 12 ,
	4  , 5  , 3  ,
	31 , 32 , 30 ,
	22 , 23 , 21 ,
};

static unsigned char bct3286_led_rgb_order[12] = {
	2, 11, 8, 5, 3, 9, 9, 6, 4, 1, 10, 7,
};

static struct bct3286_board_info bct3286_led_data = {
	.rst = GPIO_BCT3286_RST,
	.order = bct3286_led_order,
	.rgb_order = bct3286_led_rgb_order,
};

struct spi_board_info jz_spi_bus0_board_info[]  = {
	[0] = {
		.modalias	= "bct3286",
		.platform_data	= &bct3286_led_data,
		.max_speed_hz	= 12000000,
		.bus_num	= 0,
		.chip_select	= 0,
	},
};
#endif

#ifdef CONFIG_JZ_SPI0
struct jz_spi_info spi0_info_cfg = {
	.chnl = 0,
	.bus_num = 0,
	.max_clk = 54000000,
	.num_chipselect = 1,
	.allow_cs_same  = 1,
	.chipselect     = {SPI_CHIP_ENABLE,SPI_CHIP_ENABLE},
#ifdef CONFIG_BCT3286_LED
	.board_info 	= jz_spi_bus0_board_info,
	.board_size 	= 1,
#endif
};
#endif

#ifdef CONFIG_SPI_GPIO
static struct spi_gpio_platform_data jz_spi_gpio_data = {

	.sck	= GPIO_SPI_SCK,
	.mosi	= GPIO_SPI_MOSI,
	.miso	= GPIO_SPI_MISO,
	.num_chipselect = 1,
};

struct platform_device jz_spi_gpio_device = {
	.name   = "spi_gpio",
	.dev    = {
		.platform_data = &jz_spi_gpio_data,
	},
};
#endif
