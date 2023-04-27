/*
 * Copyright (C) 2020 Ingenic Semiconductor Co., Ltd.
 *
 * Camera driver for the Ingenic VIC
 *
 */
#ifndef __X2000_VIC_H__
#define __X2000_VIC_H__

#include "vic_regs.h"
#include "camera_sensor.h"
#include "dsys.h"
#include <common.h>
#include <bit_field.h>

/*
 * VIC Operation
 */
static const unsigned long vic_iobase[] = {
        KSEG1ADDR(VIC0_IOBASE),
        KSEG1ADDR(VIC1_IOBASE),
};

#define VIC_ADDR(id, reg)               ((volatile unsigned long *)((vic_iobase[id]) + (reg)))

static inline void vic_write_reg(int index, unsigned int reg, unsigned int val)
{
    *VIC_ADDR(index, reg) = val;
}

static inline unsigned int vic_read_reg(int index, unsigned int reg)
{
    return *VIC_ADDR(index, reg);
}

static inline void vic_set_bit(int index, unsigned int reg, unsigned int start, unsigned int end, unsigned int val)
{
    set_bit_field(VIC_ADDR(index, reg), start, end, val);
}

static inline unsigned int vic_get_bit(int index, unsigned int reg, unsigned int start, unsigned int end)
{
    return get_bit_field(VIC_ADDR(index, reg), start, end);
}

unsigned int is_output_y8(int index, struct sensor_attr *attr);
unsigned int is_output_yuv422(int index, struct sensor_attr *attr);
void vic_reset(unsigned int index);
int vic_stream_on(int index, struct sensor_attr *attr);
void vic_stream_off(int index, struct sensor_attr *attr);
int vic_power_on(int index);
void vic_power_off(int index);
int vic_power_state(int index);
int vic_stream_state(int index);

unsigned int vic_enable_ts(int index, int ms_ch, unsigned int offset);

int jz_arch_vic_init(void);
void jz_arch_vic_exit(void);

#ifdef SOC_CAMERA_DEBUG
int dsysfs_vic_dump_reg(int index, char *buf);
int dsysfs_vic_show_sensor_info(int index, char *buf);
int dsysfs_vic_ctrl(int index, char *buf);
#endif

#ifdef SOC_CAMERA_DEBUG
struct kobject *dsysfs_get_root_dir(int index);
#endif

#endif /* __X2000_VIC_H__ */
