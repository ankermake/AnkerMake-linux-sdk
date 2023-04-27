#include <linux/kernel.h>
#include <linux/init.h>


#if defined(CONFIG_JZ_VIC_CORE)
#include "ov9281_cfg.c"
#elif defined(CONFIG_SOC_CAMERA_OV9281_DVP)
#include "ov9281_dvp_cfg.c"
#elif defined(CONFIG_SOC_CAMERA_OV9281_MIPI)
#include "ov9281_mipi_cfg.c"
#endif

static struct vic_sensor_config* vic_sensor_pcfg = NULL;

struct vic_sensor_config* get_sensor_data(void) {
    return vic_sensor_pcfg;
}

static int registor_sensor_data(void) {
#ifdef CONFIG_JZ_VIC_CORE
    vic_sensor_pcfg = &ov9281_dvp_sensor_config;
#elif defined(CONFIG_SOC_CAMERA_OV9281_DVP)
    vic_sensor_pcfg = &ov9281_dvp_sensor_config;
#elif defined(CONFIG_SOC_CAMERA_OV9281_MIPI)
    vic_sensor_pcfg = &ov9281_mipi_sensor_config;
#endif
    return 0;
}

subsys_initcall(registor_sensor_data);




