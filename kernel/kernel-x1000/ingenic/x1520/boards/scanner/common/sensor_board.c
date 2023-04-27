#include <linux/sensor_board.h>
#include <board.h>
#include <linux/platform_device.h>
#include <linux/delay.h>
#include <media/soc_camera.h>
#include <mach/platform.h>
#include <linux/regulator/machine.h>
#include <gpio.h>
#include "board_base.h"


#ifdef CONFIG_VIDEO_TX_ISP
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
#endif

#ifdef CONFIG_SOC_CAMERA
static int flags = 1;
static int camera_sensor_reset(struct device *dev) {
	if (flags){
		gpio_request(CAMERA_SENSOR_RESET, "sensor_rst0");
		gpio_request(CAMERA_FRONT_SENSOR_PWDN, "sensor_en0");
		flags = 0;
	}
	gpio_direction_output(CAMERA_SENSOR_RESET, 0);
	mdelay(20);
	gpio_direction_output(CAMERA_SENSOR_RESET, 1);
	mdelay(20);
	return 0;
}
static int camera_sensor_power(struct device *dev, int on) {
	/* enable or disable the camera */
	gpio_direction_output(CAMERA_FRONT_SENSOR_PWDN, on ? CAMERA_FRONT_SENSOR_PWDN_EN_LEVEL : CAMERA_FRONT_SENSOR_PWDN_DIS_LEVEL);
	mdelay(20);

	return 0;
}

static struct soc_camera_link iclink_front = {
    .bus_id     = 0,        /* Must match with the camera ID */
    .board_info = &jz_v4l2_camera_devs,
    .i2c_adapter_id = 0,
    .power = camera_sensor_power,
    .reset = camera_sensor_reset,
};

struct platform_device camera_sensor = {
	.name	= "soc-camera-pdrv",
	.id	= -1,
	.dev	= {
		.platform_data = &iclink_front,
	},
};

static int __init board_cim_init(void )
{
	platform_device_register(&camera_sensor);
	return 0;
}
core_initcall(board_cim_init);
#endif
