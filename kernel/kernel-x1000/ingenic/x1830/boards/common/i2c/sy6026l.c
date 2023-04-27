#include <sound/sy6026l.h>
#include <linux/i2c.h>
#include <linux/init.h>
#include <board.h>

static struct sy6026l_platform_data sy6026l_platform_data = {
    .gpio_rst_b = GPIO_SY6026L_RST,
    .gpio_rst_b_level = GPIO_SY6026L_RST_LEVEL,
    .gpio_fault_b = CPIO_SY6026L_FAULT_EN,
    .gpio_fault_b_level = CPIO_SY6026L_FAULT_EN_LEVEL,
};

static struct i2c_board_info sy6026l_bd_info = {
    I2C_BOARD_INFO("sy6026l", SY6026L_I2C_ADDR),
    .platform_data = (void *)&sy6026l_platform_data,
};

static int __init sy6026l_bd_info_init(void)
{
	i2c_register_board_info(SY6026L_I2CBUS_NUM, &sy6026l_bd_info, 1);
}
arch_initcall(sy6026l_bd_info_init);
