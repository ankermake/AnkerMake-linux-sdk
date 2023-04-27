/*DEMO CODE NOT CHECK*/
#include <linux/platform_device.h>
#include <linux/spi/spi_gpio.h>
#include "board_base.h"
#if 0
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

static int __init spi_gpio_device_init(void)
{
	return platform_device_register(&jz_spi_gpio_device);
}
arch_initcall(spi_gpio_device_init);
#endif
