/*
 * Copyright (C) 2020 Ingenic Semiconductor Co., Ltd.
 *
 * Camera driver for the Ingenic VIC
 *
 */
#ifndef __X2000_VIC_CHANNEL_TIZIANO_H__
#define __X2000_VIC_CHANNEL_TIZIANO_H__

#include "camera_sensor.h"
#include "dsys.h"


int vic_tiziano_stream_on(int index, struct sensor_attr *attr);
void vic_tiziano_stream_off(int index, struct sensor_attr *attr);
int vic_tiziano_power_on(int index);
void vic_tiziano_power_off(int index);

int vic_register_sensor_route_tiziano(int index, struct sensor_attr *sensor);
void vic_unregister_sensor_route_tiziano(int index, struct sensor_attr *sensor);

int jz_vic_tiziano_drv_init(int index);
void jz_vic_tiziano_drv_deinit(int index);

#ifdef SOC_CAMERA_DEBUG
struct kobject *dsysfs_get_root_dir(int index);
#endif

#endif /* __X2000_VIC_CHANNEL_TIZIANO_H__ */
