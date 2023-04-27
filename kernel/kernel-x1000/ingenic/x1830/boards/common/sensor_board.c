#include <linux/sensor_board.h>
#include <board.h>



struct sensor_board_info sensor_board_list[] = {
#ifdef CIM1_SENSOR_NAME
    {
        .name = CIM1_SENSOR_NAME,
        .gpios = {
            .gpio_power       = CIM1_GPIO_POWER,
            .gpio_sensor_pwdn = CIM1_GPIO_SENSOR_PWDN,
            .gpio_sensor_rst  = CIM1_GPIO_SENSOR_RST,
            .gpio_i2c_sel1    = CIM1_GPIO_I2C_SEL1,
            .gpio_i2c_sel2    = CIM1_GPIO_I2C_SEL2,
        },
        .dvp_gpio_func = CIM1_DVP_GPIO_FUNC,
    },
#endif

#ifdef CIM2_SENSOR_NAME
    {
        .name = CIM2_SENSOR_NAME,
        .gpios = {
            .gpio_power       = CIM2_GPIO_POWER,
            .gpio_sensor_pwdn = CIM2_GPIO_SENSOR_PWDN,
            .gpio_sensor_rst  = CIM2_GPIO_SENSOR_RST,
            .gpio_i2c_sel1    = CIM2_GPIO_I2C_SEL1,
            .gpio_i2c_sel2    = CIM2_GPIO_I2C_SEL2,
        },
        .dvp_gpio_func = CIM2_DVP_GPIO_FUNC,
    },
#endif
};


struct sensor_board_info* get_sensor_board_info(const char *name)
{
    int i = 0;
    int sensor_num = sizeof(sensor_board_list)/sizeof(struct sensor_board_info);

    for (; i<sensor_num; i++) {
        if (!strcmp(sensor_board_list[i].name, name))
            return &sensor_board_list[i];
    }

    return NULL;
}
EXPORT_SYMBOL(get_sensor_board_info);

