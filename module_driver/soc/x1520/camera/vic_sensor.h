#ifndef _VIC_SENSOR_H_
#define _VIC_SENSOR_H_

#include <linux/list.h>
#include "camera.h"

typedef enum {
    VIC_bt656,
    VIC_bt601,
    VIC_mipi_csi,
    VIC_dvp,
    VIC_bt1120,
} vic_interface;

typedef enum {
    SENSOR_DATA_DMA_MODE_RAW        = 0,   /* RAW8/10/12 */
    SENSOR_DATA_DMA_MODE_RGB565     = 1,
    SENSOR_DATA_DMA_MODE_RGB888     = 2,
    SENSOR_DATA_DMA_MODE_YUV422     = 3,   /* YUV422 packet */
    SENSOR_DATA_DMA_MODE_YUV422SP0  = 4,   /* YUV422 semi-plannar0 */
    SENSOR_DATA_DMA_MODE_YUV422SP1  = 5,   /* YUV422 semi-plannar1 */
    SENSOR_DATA_DMA_MODE_NV12       = 6,
    SENSOR_DATA_DMA_MODE_NV21       = 7,
    SENSOR_DATA_DMA_MODE_GREY       = 100, /* 自定义 */
} sensor_data_dma_mode;

typedef enum {
    DVP_RAW8,
    DVP_RAW10,
    DVP_RAW12,
    DVP_YUV422,
    DVP_RGB565,
} dvp_data_fmt;

typedef enum {
    DVP_PA_LOW_10BIT,
    DVP_PA_HIGH_10BIT,
    DVP_PA_12BIT,
    DVP_PA_LOW_8BIT,
    DVP_PA_HIGH_8BIT,
} dvp_gpio_mode;

typedef enum {
    DVP_href_mode,
    DVP_hsync_mode,
    DVP_sony_mode,
} dvp_timing_mode;

/*                         clk1,clk2,clk3,clk4
 * 初始yuv 4字节顺序_1_2_3_4   1    2    3    4
 * 可转变成以下顺序
 */
typedef enum {
    order_2_1_4_3,
    order_2_3_4_1,
    order_1_2_3_4,
    order_1_4_3_2,
} yuv_data_order;

typedef enum {
    POLARITY_HIGH_ACTIVE,
    POLARITY_LOW_ACTIVE,
} dvp_sync_polarity;

typedef enum {
    DVP_img_scan_progress,
    DVP_img_scan_interlace,
} dvp_img_scan_mode;

/*
 * MIPI information
 */
typedef enum {
    MIPI_RAW8,
    MIPI_RAW10,
    MIPI_RAW12,
    MIPI_RGB555,
    MIPI_RGB565,
    MIPI_RGB666,
    MIPI_RGB888,
    MIPI_YUV422,
    MIPI_YUV422_10BIT,
} mipi_data_fmt;

struct dvp_bus_info {
    dvp_data_fmt dvp_data_fmt;
    dvp_gpio_mode dvp_gpio_mode;
    dvp_timing_mode dvp_timing_mode;
    yuv_data_order dvp_yuv_data_order;
    dvp_sync_polarity dvp_hsync_polarity;
    dvp_sync_polarity dvp_vsync_polarity;
    dvp_img_scan_mode dvp_img_scan_mode;
};

struct mipi_csi_bus_info {
    mipi_data_fmt data_fmt;
    int lanes;
    int clk;
};

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

struct vic_sensor_config {
    char *device_name;
    unsigned int cbus_addr;
    struct camera_info info;
    sensor_data_dma_mode dma_mode;  /* 控制器DMA输出格式选择 */
    vic_interface vic_interface;
    union {
        struct dvp_bus_info dvp_cfg_info;
        struct mipi_csi_bus_info mipi_cfg_info;
    };

    long isp_clk_rate;

    struct sensor_info sensor_info;
    struct sensor_ctrl_ops ops;
};

int vic_register_sensor(struct vic_sensor_config *sensor);
void vic_unregister_sensor(struct vic_sensor_config *sensor);

void vic_enable_sensor_mclk(unsigned long clk_rate);
void vic_disable_sensor_mclk(void);

int dvp_init_low10bit_gpio(void);
void dvp_deinit_gpio(void);

#endif /* _VIC_SENSOR_H_ */
