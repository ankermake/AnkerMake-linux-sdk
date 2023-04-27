/*
 * Copyright (C) 2020 Ingenic Semiconductor Co., Ltd.
 *
 * Camera driver for the Ingenic VIC
 *
 */
#ifndef __X2000_VIC_CHANNEL_MEM_H__
#define __X2000_VIC_CHANNEL_MEM_H__

#include "camera_sensor.h"
#include "dsys.h"

int vic_register_sensor_route_mem(int index, int mem_cnt, struct sensor_attr *sensor);
int vic_unregister_sensor_route_mem(int index, struct sensor_attr *sensor);

int jz_vic_mem_drv_init(int index);
void jz_vic_mem_drv_deinit(int index);


#endif /* __X2000_VIC_CHANNEL_MEM_H__ */
