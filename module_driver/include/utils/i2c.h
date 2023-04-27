#ifndef _UTILS_I2C_H_
#define _UTILS_I2C_H_

#include <linux/i2c.h>

/**
 * 向指定的i2c总线注册i2c设备
 */
struct i2c_client *i2c_register_device(struct i2c_board_info *info, int i2c_bus_num);

#endif /* _UTILS_I2C_H_ */