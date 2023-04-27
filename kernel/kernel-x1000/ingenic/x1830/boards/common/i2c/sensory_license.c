#include <linux/i2c.h>
#include <linux/init.h>
#include <board.h>

#define DEVICE_NAME         "sensory_license"

static struct i2c_board_info sensory_license_bd_info = {
        I2C_BOARD_INFO(DEVICE_NAME, SENSORY_LICENSE_I2C_ADDR),
};

static int __init sy6026l_bd_info_init(void)
{
    i2c_register_board_info(SENSORY_LICENSE_I2CBUS_NUM, &sensory_license_bd_info, 1);
}
arch_initcall(sy6026l_bd_info_init);
