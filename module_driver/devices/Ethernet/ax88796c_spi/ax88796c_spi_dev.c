#include <linux/platform_device.h>
#include <linux/moduleparam.h>
#include <linux/spi/spi.h>
#include <linux/string.h>
#include <utils/spi.h>
#include <utils/gpio.h>
#include "ax88796_spi.h"

static struct ax88796c_spi_pdata pdata;

static struct spi_board_info ax88796c_spi_dev;

static int gpio_irq = -1;
static int gpio_cs = -1;
static int gpio_reset = -1;
static int bus_num = -1;

module_param_gpio(gpio_cs, 0644);
module_param_gpio(gpio_irq, 0644);
module_param_gpio(gpio_reset, 0644);
module_param_named(spi_bus_num, bus_num, int, 0644);

int ax88796c_platform_device_register(void)
{
    if (gpio_cs < 0) {
        printk(KERN_ERR "ax88796c_spi: gpio_cs must define\n");
        return -EINVAL;
    }

    if (gpio_irq < 0) {
        printk(KERN_ERR "ax88796c_spi: gpio_irq must define\n");
        return -EINVAL;
    }

    if (bus_num < 0) {
        printk(KERN_ERR "ax88796c_spi: spi_bus_num must define\n");
        return -EINVAL;
    }

    if (gpio_reset < 0) {
        printk(KERN_WARNING "ax88796c_spi: gpio_reset no define!\n");
    }

    pdata.gpio_irq = gpio_irq;
    pdata.gpio_reset = gpio_reset;
    strcpy(ax88796c_spi_dev.modalias, "ax88796c_spi");
    ax88796c_spi_dev.controller_data = (void *)gpio_cs;
    ax88796c_spi_dev.platform_data = &pdata;
    ax88796c_spi_dev.max_speed_hz = 50*1000*1000;
    ax88796c_spi_dev.mode = SPI_MODE_3;

    struct spi_device *dev = spi_register_device(&ax88796c_spi_dev, bus_num);
    if (!dev) {
        printk(KERN_ERR "ax88796c_spi: failed to register spi dev\n");
        return -EINVAL;
    }

    return 0;
}

