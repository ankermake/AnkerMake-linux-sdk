#include <linux/platform_device.h>
#include <linux/spi/spi.h>
#include <linux/spi/spi_gpio.h>
#include <linux/mtd/mtd.h>
#include <linux/mtd/partitions.h>
#include <mach/jzssi.h>
#include "board_base.h"


#if defined(CONFIG_JZ_SPI) || defined(CONFIG_SPI_GPIO)
struct spi_board_info jz_spi_board_info[] = {

#ifdef CONFIG_JZ_SPI0
	{
		.modalias = "spidev",		    //device name, match driver name
#ifdef CONFIG_SPI_GPIO
		.controller_data = (void*)SPI0_CHIP_ENABLE,
#endif
		.platform_data = NULL,	    //device private data
		.max_speed_hz = 10*1000*1000, 	    //set bus max rate
		.bus_num = 0,		    //bus num
		.chip_select = 0,	    //chip select
		.mode = 0,
	},
#endif

#ifdef CONFIG_JZ_SPI1
	{
		.modalias = "spidev",		    //device name, match driver name=
		.platform_data = NULL,	    //device private data
		.max_speed_hz = 10*1000*1000, 	    //set bus max rate
		.bus_num = 1,		    //bus num
		.chip_select = 0,	    //chip select
		.mode = 0,
	},
#endif
};
int jz_spi_devs_size = ARRAY_SIZE(jz_spi_board_info);
#endif

#ifdef CONFIG_JZ_SPI0
struct jz_spi_info spi0_info_cfg = {
	.chnl = 0,
	.bus_num = 0,
	.max_clk = 54000000,
	.num_chipselect = 1,
	.allow_cs_same  = 1,
	.chipselect     = {SPI0_CHIP_ENABLE},
};
#endif

#ifdef CONFIG_JZ_SPI1
struct jz_spi_info spi1_info_cfg = {
	.chnl = 1,
	.bus_num = 1,
	.max_clk = 54000000,
	.num_chipselect = 1,
	.allow_cs_same  = 1,
	.chipselect     = {SPI1_CHIP_ENABLE},
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
	.id     = 0,
	.dev    = {
		.platform_data = &jz_spi_gpio_data,
	},
};
#endif

