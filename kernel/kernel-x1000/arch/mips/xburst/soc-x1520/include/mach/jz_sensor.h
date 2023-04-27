#ifndef __JZ_SENSOR_H__
#define __JZ_SENSOR_H__

struct cim_sensor_plat_data {
    const char *name;
    unsigned int pin_pwron;
    unsigned int pin_rst;
};
#endif
