/*
 * Copyright (C) 2020 Ingenic Semiconductor Co., Ltd.
 *
 * ISP Driver
 */

#ifndef __X2000_ISP_H__
#define __X2000_ISP_H__

#include <linux/videodev2.h>
#include "camera_sensor.h"
#include "dsys.h"

#include "isp-core/inc/tiziano_core.h"
#include "isp-core/inc/tiziano_isp.h"



typedef enum {
    STATE_UNKOWN,
    STATE_CLOSE,
    STATE_OPEN,
    STATE_RUN,
} isp_state_t;


#define NOTIFICATION_TYPE_CORE_OPS (0x1<<24)
#define NOTIFICATION_TYPE_TUN_OPS     (0x2<<24)
#define NOTIFICATION_TYPE_SENSOR_OPS (0x3<<24)
#define NOTIFICATION_TYPE_FS_OPS     (0x4<<24)
#define NOTIFICATION_TYPE_OPS(n)    ((n) & (0xff<<24))
enum tisp_notification {
    /* the events of subdev */
    TISP_EVENT_SYNC_SENSOR_ATTR = NOTIFICATION_TYPE_CORE_OPS,
    /* the tuning node of isp's core */
    TISP_EVENT_ACTIVATE_MODULE = NOTIFICATION_TYPE_TUN_OPS,
    TISP_EVENT_SLAVE_MODULE,
    TISP_EVENT_CORE_DAY_NIGHT,
    /* the events of sensor are defined as follows. */
    TISP_EVENT_SENSOR_S_REGISTER = NOTIFICATION_TYPE_SENSOR_OPS,
    TISP_EVENT_SENSOR_G_REGISTER,
    TISP_EVENT_SENSOR_INT_TIME,
    TISP_EVENT_SENSOR_AGAIN,
    TISP_EVENT_SENSOR_DGAIN,
    TISP_EVENT_SENSOR_FPS,
    TISP_EVENT_SENSOR_HFLIP,
    TISP_EVENT_SENSOR_VFLIP,
    TISP_EVENT_SENSOR_RESIZE,
};

enum tisp_i2c_index {
    TISP_I2C_SET_INTEGRATION,
    TISP_I2C_SET_AGAIN,
    TISP_I2C_SET_DGAIN,
    TISP_I2C_SET_BUTTON,
};
struct tisp_i2c_msg {
    unsigned int flag;
    unsigned int value;
};

struct jz_isp_data {
    int index;
    int is_finish;

    int irq;
    const char *irq_name;

    struct mutex lock;

    /* Camera Device */
    struct camera_device camera;

    /* isp firmware  */
    struct task_struct *process_thread;
    tisp_core_t core;
    struct isp_core_tuning_driver *tuning;

    /* isp tuning state */
    unsigned int dn_state; //0:day, 1: night
    unsigned int daynight_change;
    unsigned int hflip_state; //0:disable, 1: enable
    unsigned int hflip_change;
    unsigned int vflip_state; //0:disable, 1: enable
    unsigned int vflip_change;
    /* IRQ callbacks */
    int (*irq_func_cb[32])(void *);
    void *irq_func_data[32];

    /* i2c sync messages */
    struct tisp_i2c_msg i2c_msgs[TISP_I2C_SET_BUTTON];

    /* err cnt */
    int isp_err;
    int isp_err1;
    int isp_overflow;
    int isp_breakfrm;

#ifdef SOC_CAMERA_DEBUG
    struct kobject *dsysfs_parent_kobj;
    struct kobject dsysfs_kobj;
#endif
};

int isp_stream_on(int index, struct sensor_attr *attr);
int isp_stream_off(int index, struct sensor_attr *attr);
int isp_power_on(int index);
void isp_power_off(int index);

int isp_component_bind_sensor(int index, struct sensor_attr *sensor);
void isp_component_unbind_sensor(int index, struct sensor_attr *sensor);

int jz_isp_drv_init(int index);
void jz_isp_drv_deinit(int index);



#endif /* __X2000_ISP_H__ */
