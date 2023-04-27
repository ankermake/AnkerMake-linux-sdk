#include <linux/platform_device.h>
#include <linux/spi/spi.h>
#include <im501.h>
#include <board.h>

static struct im501_pdata pdata = {
    .gpio_irq       = IM501_IRQ_GPIO,
};

static struct spi_board_info im501_spi_dev = {
	.modalias = "im501_spi",
#ifdef IM501_CS_GPIO
	.controller_data = (void*)IM501_CS_GPIO,
#endif
	.platform_data = &pdata,
    .max_speed_hz = 20*1000*1000,
	.bus_num = IM501_SPI_BUS_NUM,
	.chip_select = IM501_SPI_CS,
	.mode = SPI_MODE_0,
};

static struct platform_device im501_codec = {
	.name = "im501-codec",
};


static int __init im501_platform_device_register(void) {
	platform_device_register(&im501_codec);
	spi_register_board_info(&im501_spi_dev, 1);
	return 0;
}


arch_initcall(im501_platform_device_register);
