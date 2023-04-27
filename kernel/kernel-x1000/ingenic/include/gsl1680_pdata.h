#ifndef _GSL1680_PDATA_H_
#define _GSL1680_PDATA_H_

struct gsl1680_platform_data {
    int gpio_shutdown;
    int gpio_int;
    int i2c_num;
};

#endif /* _GSL1680_PDATA_H_ */