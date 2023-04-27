#ifndef _SENSOR_BOARD_H
#define _SENSOR_BOARD_H

#include <linux/module.h>
#include <linux/string.h>



enum camera_reg_ops {
    CAMERA_REG_OP_DATA                  = 1,
    CAMERA_REG_OP_DELAY,
    CAMERA_REG_OP_END,
};

struct camera_reg_op {
    unsigned int flag;
    unsigned int reg;
    unsigned int val;
};


struct sensor_gpios {
    int gpio_power;
    int gpio_sensor_pwdn;
    int gpio_sensor_rst;
    int gpio_i2c_sel1;
    int gpio_i2c_sel2;
};

struct sensor_board_info {
    const char *name;

    struct sensor_gpios gpios;
    int dvp_gpio_func;
};


struct sensor_board_info* get_sensor_board_info(const char *name);

#endif
