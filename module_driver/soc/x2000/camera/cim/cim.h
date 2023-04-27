/*
 * Copyright (C) 2020 Ingenic Semiconductor Co., Ltd.
 *
 * Camera driver for the Ingenic CIM Controller
 *
 */
#ifndef __X2000_CIM_H__
#define __X2000_CIM_H__

#include "camera_sensor.h"
#include "dsys.h"

int cim_register_sensor_route(int index, int mem_cnt, struct sensor_attr *sensor);
int cim_unregister_sensor_route(int index, struct sensor_attr *sensor);

int jz_cim_drv_init(void);
void jz_cim_drv_deinit(void);


#endif /* __X2000_CIM_H__ */
