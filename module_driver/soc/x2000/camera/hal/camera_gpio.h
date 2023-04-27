/*
 * Copyright (C) 2020 Ingenic Semiconductor Co., Ltd.
 *
 * Camera driver for the Camera gpio
 *
 */

#ifndef __X2000_CAMERA_GPIO_H__
#define __X2000_CAMERA_GPIO_H__

int camera_mclk_gpio_init(int voltage);
int camera_mclk_gpio_deinit(int voltage);

#endif /* __X2000_CAMERA_GPIO_H__ */
