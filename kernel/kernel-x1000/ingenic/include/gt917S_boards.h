#ifndef _GT917S_PDATA_H_
#define _GT917S_PDATA_H_

#include  <linux/gpio.h>
#include <linux/i2c.h>
#include <linux/init.h>

struct gt917S_platform_data {
    int gpio_shutdown;
    int gpio_int;
    int i2c_num;
    int x;
    int y;
};

#endif /* _GT917S_PDATA_H_ */
