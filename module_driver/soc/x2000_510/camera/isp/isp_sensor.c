/*
 * Copyright (C) 2020 Ingenic Semiconductor Co., Ltd.
 *
 * ISP Driver
 */

#include <linux/module.h>
#include <linux/list.h>
#include <linux/gpio.h>
#include <linux/irq.h>
#include <linux/slab.h>
#include <linux/delay.h>

#include <common.h>
#include <bit_field.h>

#include "isp-core/inc/tiziano_core.h"
#include "isp-core/inc/tiziano_isp.h"
#include "isp.h"



static void sensor_hw_reset_enable(sensor_control_t *ctrl)
{
}

static void sensor_hw_reset_disable(sensor_control_t *ctrl)
{
}

static void sensor_alloc_integration_time(sensor_control_t *ctrl, uint16_t *int_time, sensor_context_t *p_ctx)
{
    struct jz_isp_data *isp = (struct jz_isp_data *)ctrl->priv_data;
    struct sensor_attr *sensor = isp->camera.sensor;

    if(*int_time < sensor->sensor_info.min_integration_time)
        *int_time = sensor->sensor_info.min_integration_time;
    if(*int_time > sensor->sensor_info.max_integration_time)
        *int_time = sensor->sensor_info.max_integration_time;
}

static void sensor_set_integration_time(sensor_control_t *ctrl, uint16_t int_time, sensor_param_t* param)
{
    struct jz_isp_data *isp = (struct jz_isp_data *)ctrl->priv_data;
    struct sensor_attr *sensor = isp->camera.sensor;

    //printk("<<-------- %s: %d\n", __func__, int_time);
    if(int_time != sensor->sensor_info.integration_time){
        sensor->sensor_info.integration_time = int_time;
#if 1
        isp->i2c_msgs[TISP_I2C_SET_INTEGRATION].flag = 1;
        isp->i2c_msgs[TISP_I2C_SET_INTEGRATION].value = int_time;
#else
        sensor->ops.set_integration_time(int_time);
#endif
    }
}

static int32_t sensor_alloc_analog_gain(sensor_control_t *ctrl, int32_t gain, sensor_context_t *p_ctx)
{
    struct jz_isp_data *isp = (struct jz_isp_data *)ctrl->priv_data;
    struct sensor_attr *sensor = isp->camera.sensor;
    unsigned int again = 0;

    if (sensor->ops.alloc_again) {
        gain = sensor->ops.alloc_again(gain, LOG2_GAIN_SHIFT, &again);
        p_ctx->again = again;
        //printk("---->> %s: %d(0x%x)\n", __func__, gain, again);
    } else
        printk(KERN_ERR "sensor->ops.alloc_again is NULL!\n");

    return gain;
}

static void sensor_set_analog_gain(sensor_control_t *ctrl, uint32_t again_reg_val, sensor_context_t *p_ctx)
{
    struct jz_isp_data *isp = (struct jz_isp_data *)ctrl->priv_data;
    struct sensor_attr *sensor = isp->camera.sensor;

    //printk("<<-------- %s: 0x%x\n", __func__, again_reg_val);
    if(again_reg_val != sensor->sensor_info.again){
        sensor->sensor_info.again = again_reg_val;
#if 1
        isp->i2c_msgs[TISP_I2C_SET_AGAIN].flag = 1;
        isp->i2c_msgs[TISP_I2C_SET_AGAIN].value = again_reg_val;
#else
        sensor->ops.set_analog_gain(again_reg_val);
#endif
    }
}

static int32_t sensor_alloc_digital_gain(sensor_control_t *ctrl, int32_t gain, sensor_context_t *p_ctx)
{
    struct jz_isp_data *isp = (struct jz_isp_data *)ctrl->priv_data;
    struct sensor_attr *sensor = isp->camera.sensor;
    unsigned int dgain = 0;

    if (sensor->ops.alloc_dgain) {
        gain = sensor->ops.alloc_dgain(gain, LOG2_GAIN_SHIFT, &dgain);
        p_ctx->dgain = dgain;
        //printk("---->> %s: %d(0x%x)\n", __func__, gain, dgain);
    } else
        printk(KERN_ERR "sensor->ops.alloc_dgain is NULL!\n");

    return gain;
}

static void sensor_set_digital_gain(sensor_control_t *ctrl, uint32_t dgain_reg_val, sensor_context_t *p_ctx)
{
    struct jz_isp_data *isp = (struct jz_isp_data *)ctrl->priv_data;
    struct sensor_attr *sensor = isp->camera.sensor;

    //printk("<<-------- %s: 0x%x\n", __func__, dgain_reg_val);
    if(dgain_reg_val != sensor->sensor_info.dgain){
        sensor->sensor_info.dgain = dgain_reg_val;
#if 1
        isp->i2c_msgs[TISP_I2C_SET_DGAIN].flag = 1;
        isp->i2c_msgs[TISP_I2C_SET_DGAIN].value = dgain_reg_val;
#else
        sensor->ops.set_digital_gain(dgain_reg_val);
#endif
    }
}

static uint16_t sensor_get_normal_fps(sensor_control_t *ctrl, sensor_param_t* param)
{
    struct jz_isp_data *isp = (struct jz_isp_data *)ctrl->priv_data;
    struct sensor_attr *sensor = isp->camera.sensor;

    unsigned int fps = sensor->sensor_info.fps;
    return (((fps >> 16) / (fps & 0xffff)) << 8) + ((((fps >> 16) % (fps & 0xffff)) << 8) / (fps & 0xffff));
}

static uint16_t sensor_read_black_pedestal(sensor_control_t *ctrl, int i,uint32_t gain)
{
    unsigned int black = 0;
    return black;
}

static void sensor_set_mode(sensor_control_t *ctrl, uint8_t mode, sensor_param_t* param)
{
    struct jz_isp_data *isp = (struct jz_isp_data *)ctrl->priv_data;
    struct sensor_attr *sensor = isp->camera.sensor;

    //sensor->ops.set_mode(mode);
    param->active.width = sensor->sensor_info.width;
    param->active.height = sensor->sensor_info.height;
    param->total.width = sensor->sensor_info.total_width;
    param->total.height = sensor->sensor_info.total_height;
    param->integration_time_min = sensor->sensor_info.min_integration_time;
    param->integration_time_max = sensor->sensor_info.max_integration_time;
    param->mode = mode;
}

static void sensor_start_changes(sensor_control_t *ctrl, sensor_context_t *p_ctx)
{
}

static void sensor_end_changes(sensor_control_t *ctrl, sensor_context_t *p_ctx)
{
}

static uint16_t sensor_get_id(sensor_control_t *ctrl)
{
    //return sensor->chip_id;
    return 0;
}

static void sensor_set_wdr_mode(sensor_control_t *ctrl, uint8_t mode, sensor_param_t* param)
{
    //    ISP_INFO("^^^ %s ^^^\n",__func__);
    // This sensor does not support native WDR
}

static uint32_t sensor_fps_control(sensor_control_t *ctrl, uint8_t fps, sensor_param_t* param)
{
    struct jz_isp_data *isp = (struct jz_isp_data *)ctrl->priv_data;
    struct sensor_attr *sensor = isp->camera.sensor;

    param->total.width = sensor->sensor_info.total_width;
    param->total.height = sensor->sensor_info.total_height;
    param->integration_time_min = sensor->sensor_info.min_integration_time;
    param->integration_time_max = sensor->sensor_info.max_integration_time;

    return sensor->sensor_info.fps;
}

static void sensor_disable_isp(sensor_control_t *ctrl)
{
}

static uint32_t sensor_get_lines_per_second(sensor_control_t *ctrl, sensor_param_t* param)
{
    uint32_t lines_per_second=0;
    return lines_per_second;
}


void sensor_init(sensor_control_t *ctrl)
{
    struct jz_isp_data *isp = (struct jz_isp_data *)ctrl->priv_data;
    struct sensor_attr *sensor = isp->camera.sensor;
    sensor_info_t *sensor_info = &isp->core.sensor_info.sensor_info;

    sensor->sensor_info.integration_time = 0xffff;
    sensor->sensor_info.again = 0xffffffff;
    sensor->sensor_info.dgain = 0xffffffff;
    sensor_info->max_again = ctrl->param.again_log2_max = sensor->sensor_info.max_again;
    sensor_info->max_dgain = ctrl->param.dgain_log2_max = sensor->sensor_info.max_dgain;
    ctrl->param.integration_time_apply_delay = sensor->sensor_info.integration_time_apply_delay;
    ctrl->param.analog_gain_apply_delay = sensor->sensor_info.again_apply_delay;
    ctrl->param.digital_gain_apply_delay = sensor->sensor_info.dgain_apply_delay;
    sensor_info->min_integration_time = ctrl->param.integration_time_min = sensor->sensor_info.min_integration_time;
    sensor_info->max_integration_time =ctrl->param.integration_time_max = sensor->sensor_info.max_integration_time;
    sensor_info->fps = sensor->sensor_info.fps;
    sensor_info->total_width = sensor->sensor_info.total_width;
    sensor_info->total_height = sensor->sensor_info.total_height;
    ctrl->hw_reset_disable = sensor_hw_reset_disable;
    ctrl->hw_reset_enable = sensor_hw_reset_enable;
    ctrl->alloc_analog_gain = sensor_alloc_analog_gain;
    ctrl->alloc_digital_gain = sensor_alloc_digital_gain;
    ctrl->alloc_integration_time = sensor_alloc_integration_time;
    ctrl->set_integration_time = sensor_set_integration_time;
    ctrl->start_changes = sensor_start_changes;
    ctrl->end_changes = sensor_end_changes;
    ctrl->set_analog_gain = sensor_set_analog_gain;
    ctrl->set_digital_gain = sensor_set_digital_gain;
    ctrl->get_normal_fps = sensor_get_normal_fps;
    ctrl->read_black_pedestal = sensor_read_black_pedestal;
    ctrl->set_mode = sensor_set_mode;
    ctrl->set_wdr_mode = sensor_set_wdr_mode;
    ctrl->fps_control = sensor_fps_control;
    ctrl->get_id = sensor_get_id;
    ctrl->disable_isp = sensor_disable_isp;
    ctrl->get_lines_per_second = sensor_get_lines_per_second;
}

EXPORT_SYMBOL(sensor_init);
