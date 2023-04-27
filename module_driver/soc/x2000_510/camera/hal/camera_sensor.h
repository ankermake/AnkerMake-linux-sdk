/*
 * Copyright (C) 2020 Ingenic Semiconductor Co., Ltd.
 *
 * Camera driver for Sensor
 *
 */

#ifndef __X2000_CAMERA_SENSOR_H__
#define __X2000_CAMERA_SENSOR_H__

#include <linux/list.h>
#include <linux/delay.h>
#include <linux/kernel.h>
#include "camera.h"


enum sensor_reg_ops {
    SENSOR_REG_OP_DATA = 1,
    SENSOR_REG_OP_DELAY,
    SENSOR_REG_OP_END,
};

struct sensor_reg_op {
    unsigned int flag;
    unsigned int reg;
    unsigned int val;
};

typedef enum {
    SENSOR_DATA_BUS_MIPI = 0,
    SENSOR_DATA_BUS_DVP,
    SENSOR_DATA_BUS_BT601,      /* 暂不支持 */
    SENSOR_DATA_BUS_BT656,      /* 暂不支持 */
    SENSOR_DATA_BUS_BT1120,     /* 暂不支持 */
    SENSOR_DATA_BUS_BUTT,
} sensor_data_bus_type;

typedef enum {
    SENSOR_DATA_DMA_MODE_RAW     = 0,    /* RAW8/10/12 */
    SENSOR_DATA_DMA_MODE_YUV422  = 3,
    SENSOR_DATA_DMA_MODE_NV12    = 6,
    SENSOR_DATA_DMA_MODE_NV21    = 7,
    SENSOR_DATA_DMA_MODE_GREY    = 100, /* 自定义 */
} sensor_data_dma_mode;

typedef enum {
    DVP_RAW8            = 0,    /* 该顺序不可调整 */
    DVP_RAW10,
    DVP_RAW12,
    DVP_YUV422,
    DVP_RESERVED1,
    DVP_RESERVED2,
    DVP_YUV422_8BIT,
} dvp_data_fmt;

typedef enum {
    DVP_PA_LOW_10BIT,
    DVP_PA_HIGH_10BIT,
    DVP_PA_12BIT,
    DVP_PA_LOW_8BIT,
    DVP_PA_HIGH_8BIT,
} dvp_gpio_mode;

typedef enum {
    DVP_HREF_MODE,
    DVP_HSYNC_MODE,
    DVP_SONY_MODE,
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
    POLARITY_SAMPLE_RISING,
    POLARITY_SAMPLE_FALLING,
} dvp_sample_polarity;

typedef enum {
    DVP_IMG_SCAN_PROGRESS,
    DVP_IMG_SCAN_INTERLACE,
} dvp_img_scan_mode;


struct dvp_bus {
    dvp_data_fmt data_fmt;
    dvp_gpio_mode gpio_mode;
    dvp_timing_mode timing_mode;
    yuv_data_order yuv_data_order;
    dvp_sample_polarity pclk_polarity;
    dvp_sync_polarity hsync_polarity;
    dvp_sync_polarity vsync_polarity;
    dvp_img_scan_mode img_scan_mode;
};

/*
 * MIPI information
 */
typedef enum {
    MIPI_RAW8           = 0,    /* 该顺序不可调整 */
    MIPI_RAW10,
    MIPI_RAW12,
    MIPI_RESERVED1,
    MIPI_RESERVED2,
    MIPI_RESERVED3,
    MIPI_RESERVED4,
    MIPI_YUV422         = 7,
    MIPI_RESERVED5,
} mipi_data_fmt;


struct mipi_csi_bus {
    mipi_data_fmt data_fmt;
    int lanes;
    int clk;
    unsigned short clk_settle_time;  /* unit: ns, range: 95 ~ 300ns */
    unsigned short data_settle_time; /* unit: ns, range: 85 ~ 145ns + 10*UI */
};

struct sensor_info {
    void *private_init_setting;

    /* The following attributes are determined by private_init_setting */
    int width;
    int height;
    sensor_pixel_fmt fmt;

    unsigned int fps;     /* fps = Numerator / denominator;  [31:16]:Numerator [15:0]:denominator */
    unsigned short total_width;
    unsigned short total_height;

    unsigned short integration_time;
    unsigned short min_integration_time;
    unsigned short max_integration_time;
    unsigned short integration_time_apply_delay;
    unsigned short one_line_expr_in_us;
    unsigned int again;
    unsigned int max_again;
    unsigned short again_apply_delay;
    unsigned int dgain;
    unsigned int max_dgain;
    unsigned short dgain_apply_delay;
};

struct sensor_dbg_register {
    unsigned long long reg;
    unsigned long long val;
    unsigned int size;      /* val size, unit:byte */
};

/**
 * camera功能开关
 */
typedef enum {
    CAMERA_OPS_MODE_DISABLE = 0,    /* 不使能该模块功能 */
    CAMERA_OPS_MODE_ENABLE,         /* 使能该模块功能 */
} camera_ops_mode;

struct sensor_ctrl_ops {
    /* base */
    int (*power_on)(void);
    void (*power_off)(void);
    int (*stream_on)(void);
    void (*stream_off)(void);

    /* debug */
    int (*get_register)(struct sensor_dbg_register *reg);
    int (*set_register)(struct sensor_dbg_register *reg);

    /* isp tuning */
    unsigned int (*alloc_integration_time)(unsigned int it, unsigned char shift, unsigned int *sensor_it);
    int (*set_integration_time)(int value);
    unsigned int (*alloc_again)(unsigned int isp_gain, unsigned char shift, unsigned int *sensor_again);
    int (*set_analog_gain)(int value);
    unsigned int (*alloc_dgain)(unsigned int isp_gain, unsigned char shift, unsigned int *sensor_dgain);
    int (*set_digital_gain)(int value);
#if 0
    unsigned int (*alloc_integration_time_short)(unsigned int it, unsigned char shift, unsigned int *sensor_it);
    int (*set_integration_time_short)(int value);
    unsigned int (*alloc_again_short)(unsigned int isp_gain, unsigned char shift, unsigned int *sensor_again);
    int (*set_analog_gain_short)(int value);
    unsigned int (*alloc_dgain_short)(unsigned int isp_gain, unsigned char shift, unsigned int *sensor_dgain);
    int (*set_digital_gain_short)(int value);

    int (*get_black_pedestal)(int value);
    int (*set_wdr)(int wdr_en);
#endif

    int (*set_fps)(int fps);
    /* for vic and cim only */
    int (*get_hflip)(camera_ops_mode *mode);
    int (*set_hflip)(camera_ops_mode mode);
    int (*get_vflip)(camera_ops_mode *mode);
    int (*set_vflip)(camera_ops_mode mode);

    int (*reset_fmt)(struct camera_info *info);

    /* !!note: run in interrupt context */
    int (*frame_start_callback)(void);
    int (*frame_done_callback)(void);
};

struct sensor_attr {
    char *device_name;
    unsigned int cbus_addr;
    struct camera_info info;

    sensor_data_dma_mode dma_mode;  /* 控制器DMA输出格式选择 */
    sensor_data_bus_type dbus_type;
    union {
        struct dvp_bus dvp;
        struct mipi_csi_bus mipi;
    };

    long isp_clk_rate;

    struct sensor_info sensor_info;
    struct sensor_ctrl_ops ops;

};

enum frame_data_status {
    frame_status_free,
    frame_status_trans,
    frame_status_usable,
    frame_status_user,
};

struct frame_data {
    struct list_head link;
    int status;
    void *addr;
    struct frame_info info;
};

struct camera_device {
    struct sensor_attr *sensor;
    unsigned int is_power_on;
    unsigned int is_stream_on;
};


static inline void m_msleep(int ms)
{
    usleep_range(ms*1000, ms*1000);
}

static inline unsigned long long get_time_us(void)
{
    struct timespec64 ts;
    unsigned long long timestamp;

    ktime_get_ts64(&ts);
    timestamp = (unsigned long long)ts.tv_sec * USEC_PER_SEC + ts.tv_nsec / NSEC_PER_USEC;

    return timestamp;
}


int camera_register_sensor(int index, struct sensor_attr *sensor);
void camera_unregister_sensor(int index, struct sensor_attr *sensor);

void camera_enable_sensor_mclk(int index, unsigned long clk_rate);
void camera_disable_sensor_mclk(int index);

int dvp_init_select_gpio(struct dvp_bus *dvp_bus,int dvp_gpio_func);
void dvp_deinit_gpio(void);

#endif /* __X2000_CAMERA_SENSOR_H__ */
