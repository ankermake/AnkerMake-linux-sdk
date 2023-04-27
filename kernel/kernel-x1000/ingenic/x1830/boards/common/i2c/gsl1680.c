#include <linux/i2c.h>
#include <linux/init.h>
#include <board.h>
#include <gsl1680_pdata.h>

static struct gsl1680_platform_data gsl1680_platform_data = {
    .gpio_shutdown = GPIO_GSL1680_SHUTDOWN,
    .gpio_int = GPIO_GSL1680_INT,
    .i2c_num = GSL1680_I2CBUS_NUM,
};

static struct i2c_board_info gsl1680_bd_info = {
    I2C_BOARD_INFO("gslX680", 0x40),
    .platform_data = (void *)&gsl1680_platform_data,
};

static int __init gsl1680_bd_info_init(void)
{
    i2c_register_board_info(GSL1680_I2CBUS_NUM, &gsl1680_bd_info, 1);
    return 0;
}
arch_initcall(gsl1680_bd_info_init);
