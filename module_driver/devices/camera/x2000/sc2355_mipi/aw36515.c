/*
 * Copyright (C) 2020 Ingenic Semiconductor Co., Ltd.
 *
 * aw36515
 *
 */


#include <linux/module.h>
#include <common.h>
#include <utils/i2c.h>
#include <bit_field.h>
#include <linux/regulator/consumer.h>
#include <camera/hal/camera_sensor.h>
#include "aw36515.h"


#define AW36413_DEVICE_NAME             "aw36515"
#define AW36413_DEVICE_I2C_ADDR         0X63
#define AW36413_CHIP_ID                 0x30


static int led_driver_i2c_bus_num = -1;
module_param(led_driver_i2c_bus_num, int, 0644);

static struct i2c_client *aw_i2c_dev;

enum aw_mode aw36515_mode = AW_MODE_IR;


/** aw36515 register */
enum aw_reg_index {
    AW_REG_CHIP_ID = 0x0,
    AW_REG_ENABLE,
    AW_REG_IVFM,
    AW_REG_LED1_FLASH_I,
    AW_REG_LED2_FLASH_I,
    AW_REG_LED1_TORCH_I,
    AW_REG_LED2_TORCH_I,
    AW_REG_TIMING_CONFIG,
    AW_REG_FLAGS1,
    AW_REG_FLAGS2,
    AW_REG_DEVICE_ID,
    AW_REG_LAST_FLASH,
    AW_REG_LIST_SIZE,
};

/* aw36515 mode current desc */
struct aw_mode_current_desc {
    int base_current;       //unit: ua
    int max_current;        //unit: ua
    unsigned char base_current_level;
    unsigned char max_current_level;
    int per_level_current;  //unit: ua

    unsigned char led1_current_reg;
    unsigned char led2_current_reg;
};
static struct aw_mode_current_desc aw36515_mode_current_desc[AW_MODE_SIZE] = {
    /* Flash mode and IR mode */
    {
        .base_current                   = 3910,
        .max_current                    = 2000000,
        .base_current_level             = 0,
        .max_current_level              = 0xFF,
        .per_level_current              = 7830,

        .led1_current_reg               = AW_REG_LED1_FLASH_I,
        .led2_current_reg               = AW_REG_LED2_FLASH_I,
    },
    /* Torch mode */
    {
        .base_current                   = 980,
        .max_current                    = 500000,
        .base_current_level             = 0,
        .max_current_level              = 0xFF,
        .per_level_current              = 1960,

        .led1_current_reg               = AW_REG_LED1_TORCH_I,
        .led2_current_reg               = AW_REG_LED2_TORCH_I,
    },
};



static int aw36515_write(struct i2c_client *i2c, unsigned char reg, unsigned char value)
{
    unsigned char buf[2] = {reg, value};
    struct i2c_msg msg = {
        .addr   = i2c->addr,
        .flags  = 0,
        .len    = 2,
        .buf    = buf,
    };

    int ret = i2c_transfer(i2c->adapter, &msg, 1);
    if (ret < 0)
        printk(KERN_ERR "aw36515: failed to write reg: %x\n", (int)reg);

    return ret;
}

static int aw36515_read(struct i2c_client *i2c, unsigned char reg, unsigned char *value)
{
    unsigned char buf[1] = {reg & 0xff};
    struct i2c_msg msg[2] = {
        [0] = {
            .addr   = i2c->addr,
            .flags  = 0,
            .len    = 1,
            .buf    = buf,
        },
        [1] = {
            .addr   = i2c->addr,
            .flags  = I2C_M_RD,
            .len    = 1,
            .buf    = value,
        }
    };

    int ret = i2c_transfer(i2c->adapter, msg, 2);
    if (ret < 0)
        printk(KERN_ERR "aw36515(%x): failed to read reg: %x\n", i2c->addr, (int)reg);

    return ret;
}

static inline void aw36515_reg_dump(void)
{
    int i, ret;
    unsigned char value;

    for (i = 0; i < AW_REG_LIST_SIZE; i++) {
        ret = aw36515_read(aw_i2c_dev, i, &value);
        if (ret < 0) {
            printk(KERN_ERR "%s Reg[0x%x] read fail: %d \n", __func__, i, ret);
        } else {
            printk(KERN_ERR " 0x%x, 0x%x \n", i, value);
        }
    }
}

static int aw36515_rate_2_current(enum aw_mode mode, unsigned char rate)
{
    struct aw_mode_current_desc *cd;

    assert_range(mode, 0, AW_MODE_SIZE);
    assert_range(rate, 0, 100); //rate = [0,100]
    cd = &aw36515_mode_current_desc[mode];
    return cd->max_current * rate / 100;
}

static unsigned char aw36515_current_2_level(enum aw_mode mode, int current_ua)
{
    unsigned char level;
    struct aw_mode_current_desc *cd;

    //assert_range(mode, 0, AW_MODE_SIZE);
    cd = &aw36515_mode_current_desc[mode];

    if (current_ua > cd->max_current) {
        level = cd->max_current_level;
    } else if (current_ua < cd->base_current) {
        level = cd->base_current_level;
    } else {
        level = (unsigned char)((current_ua - cd->base_current) / cd->per_level_current);
    }

    return level;
}

static int aw36515_set_current(int which_led, int current_ua)
{
    struct aw_mode_current_desc *cd;
    unsigned char level;
    int ret = 0;

    assert_range(which_led, 1, 3);
    cd = &aw36515_mode_current_desc[aw36515_mode];

    level = aw36515_current_2_level(aw36515_mode, current_ua);
    if (bit_field_mask(0, 0) & which_led)
        ret += aw36515_write(aw_i2c_dev, cd->led1_current_reg, level);
    if (bit_field_mask(1, 1) & which_led)
        ret += aw36515_write(aw_i2c_dev, cd->led2_current_reg, level);
    if (ret < 0) {
        printk(KERN_ERR "%s fail\n", __func__);
        return ret;
    }

    return 0;
}

int aw36515_set_current_rate(int which_led, unsigned char rate)
{
    return aw36515_set_current(which_led, aw36515_rate_2_current(aw36515_mode, rate));
}

int aw36515_set_mode(enum aw_mode mode)
{
    unsigned char reg_enable;
    int ret;

    assert_range(mode, 0, AW_MODE_SIZE);

    ret = aw36515_read(aw_i2c_dev, AW_REG_ENABLE, &reg_enable);
    if (ret < 0) {
        printk(KERN_ERR "%s fail: read enable reg err\n", __func__);
        return ret;
    }

    reg_enable &= ~0x3c;
    if (AW_MODE_IR == mode) {
        reg_enable |= 0x24;
    } else if (AW_MODE_TORCH == mode) {
        reg_enable |= 0x08;
    }

    ret = aw36515_write(aw_i2c_dev, AW_REG_ENABLE, reg_enable);
    if (ret < 0) {
        printk(KERN_ERR "%s fail: write enable reg err\n", __func__);
        return ret;
    }

    aw36515_mode = mode;

    return 0;
}

int aw36515_switch_led(int which_led, unsigned char rate)
{
    unsigned char reg_enable;
    int ret;

    assert_range(which_led, 1, 3);

    ret = aw36515_read(aw_i2c_dev, AW_REG_ENABLE, &reg_enable);
    if (ret < 0) {
        printk(KERN_ERR "%s fail: read enable reg err\n", __func__);
        return ret;
    }

    if (rate > 0) {
        reg_enable |= which_led;
    } else {
        reg_enable &= ~which_led;
    }

    ret = aw36515_write(aw_i2c_dev, AW_REG_ENABLE, reg_enable);
    if (ret < 0) {
        printk(KERN_ERR "%s fail: write enable reg err\n", __func__);
        return ret;
    }

    return 0;
}

int aw36515_led_ctrl(enum aw_mode mode, int which_led, unsigned char rate)
{
    int ret;

    ret = aw36515_set_mode(mode);
    ret += aw36515_set_current_rate(which_led, rate);
    ret += aw36515_switch_led(which_led, rate);
    if (ret) {
        printk(KERN_ERR "%s fail\n", __func__);
        return ret;
    }

    return 0;
}

static int aw36515_detect(struct i2c_client *i2c)
{
    int ret;
    unsigned char value = 0;

    ret = aw36515_read(aw_i2c_dev, 0x00, &value);
    if (ret < 0)
        return ret;
    if (value != AW36413_CHIP_ID) {
        printk(KERN_ERR "aw36515 read chip id :0x%x\n", value);
        return ret;
    }
    printk(KERN_INFO "aw36515 read chip id 0x%x\n", value);

    return 0;
}

int aw36515_power_on(enum aw_mode mode, int which_led, unsigned char rate)
{
    int ret;

    ret = aw36515_detect(aw_i2c_dev);
    if (ret) {
        printk(KERN_ERR "aw36515: failed to detect\n");
        return ret;
    }

    aw36515_write(aw_i2c_dev, AW_REG_ENABLE, 0x00); //tx pin disable
    ret = aw36515_led_ctrl(mode, which_led, rate);
    if (ret) {
        printk(KERN_ERR "aw36515: failed to ctrl led\n");
        return ret;
    }
    //aw36515_reg_dump();

    return 0;
}


static int aw36515_probe(struct i2c_client *client,
        const struct i2c_device_id *id)
{
    return 0;
}

static int aw36515_remove(struct i2c_client *client)
{
    return 0;
}

static const struct i2c_device_id aw36515_id[] = {
    {AW36413_DEVICE_NAME, 0},
    {},
};
MODULE_DEVICE_TABLE(i2c, aw36515_id);

static struct i2c_driver aw36515_driver = {
    .driver = {
        .owner          = THIS_MODULE,
        .name           = AW36413_DEVICE_NAME,
    },
    .probe              = aw36515_probe,
    .remove             = aw36515_remove,
    .id_table           = aw36515_id,
};

static struct i2c_board_info aw36515_info = {
    .type               = AW36413_DEVICE_NAME,
    .addr               = AW36413_DEVICE_I2C_ADDR,
};

int init_aw36515(void)
{
    if (led_driver_i2c_bus_num < 0) {
        printk(KERN_ERR "aw36515: led_driver_i2c_bus_num must be set\n");
        return -EINVAL;
    }

    aw_i2c_dev = i2c_register_device(&aw36515_info, led_driver_i2c_bus_num);
    if (aw_i2c_dev == NULL) {
        printk(KERN_ERR "aw36515: failed to register i2c device\n");
        i2c_del_driver(&aw36515_driver);
        return -EINVAL;
    }

    int ret = i2c_add_driver(&aw36515_driver);
    if (ret) {
        printk(KERN_ERR "aw36515: failed to register i2c driver\n");
        return ret;
    }

    return 0;
}

void exit_aw36515(void)
{
    i2c_unregister_device(aw_i2c_dev);
    i2c_del_driver(&aw36515_driver);
}
