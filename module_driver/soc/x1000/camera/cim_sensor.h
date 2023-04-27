#ifndef _CIM_SENSOR_H_
#define _CIM_SENSOR_H_

#include <camera.h>

typedef enum {
    CIM_ITU656_Progressive_mode,
    CIM_ITU656_Interlace_mode,
    CIM_sync_mode,
} cim_interface;

typedef enum {
    CIM_DVP_BYTE0,
    CIM_DVP_BYTE1,
    CIM_DVP_BYTE2,
    CIM_DVP_BYTE3,
} cim_data_index;

typedef enum {
    POLARITY_HIGH_ACTIVE,
    POLARITY_LOW_ACTIVE,
} dvp_sync_polarity;

typedef enum {
    DVP_RISING_EDGE,
    DVP_FALLING_EDGE,
} dvp_pclk_sample_edge;

struct sensor_ctrl_ops {
    /* base */
    int (*power_on)(void);
    void (*power_off)(void);
    int (*stream_on)(void);
    void (*stream_off)(void);
};

struct sensor_info {
    int width;
    int height;
    sensor_pixel_fmt fmt;
    unsigned int fps;   /* fps = Numerator / denominator;  [31:16]:Numerator [15:0]:denominator */
    void *private_init_settings; /* sensor init setting list */
};

struct cim_sensor_config {
    char *device_name;
    unsigned int cbus_addr;
    struct camera_info info;

    cim_interface cim_interface;
    cim_data_index index_byte0;
    cim_data_index index_byte1;
    cim_data_index index_byte2;
    cim_data_index index_byte3;
    dvp_sync_polarity hsync_polarity;
    dvp_sync_polarity vsync_polarity;
    dvp_pclk_sample_edge data_sample_edge;

    struct sensor_info sensor_info;
    struct sensor_ctrl_ops ops;
};

void cim_enable_sensor_mclk(unsigned long clk_rate);
void cim_disable_sensor_mclk(void);

int cim_register_sensor(struct cim_sensor_config *sensor);
void cim_unregister_sensor(struct cim_sensor_config *sensor);

#endif /* _CIM_SENSOR_H_ */
