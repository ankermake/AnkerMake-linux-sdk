/*
 * Ingenic CMOS camera sensor driver only for x1021
 *
 * Copyright (C) 2019, Ingenic Semiconductor Inc. xiaoyan.zhang@ingenic.com
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 *
 */

#include <linux/module.h>
#include <linux/errno.h>
#include <linux/major.h>
#include <linux/string.h>
#include <linux/fcntl.h>
#include <linux/mm.h>
#include <linux/slab.h>
#include <linux/init.h>
#include <linux/device.h>
#include <linux/i2c.h>
#include <linux/fs.h>
#include <linux/miscdevice.h>
#include <linux/gpio.h>
#include <mach/jz_sensor.h>
#include <linux/delay.h>
#include <soc/base.h>
#include <soc/cpm.h>

#define DRIVER_NAME     "sensor"
#define SENSORDRV_LIB_VERSION        "1.0.0 "__DATE__    /* DRV LIB VERSION */
/*
 *    ioctl commands
 */
#define IOCTL_READ_REG            0 /* read sensor registers */
#define IOCTL_WRITE_REG           1 /* write sensor registers */
#define IOCTL_READ_EEPROM         2 /* read  sensor eeprom */
#define IOCTL_WRITE_EEPROM        3 /* write sensor eeprom */
#define IOCTL_SET_ADDR            4 /* set i2c address */
#define IOCTL_SET_CLK             5 /* set i2c clock */

#define SIZE 12
struct IO_MSG {
    unsigned int write_size;
    unsigned int read_size;
    unsigned char reg_buf[SIZE];
};

struct eeprom_buf{
        u8 length;
        u8 offset;
        u8 rom[256];
};

struct jz_sensor_dev {
    int i2c_bus_id;
    const char *name;
    struct i2c_client *client;
    struct miscdevice misc_dev;
    struct IO_MSG reg_msg;
};

static int sensor_i2c_master_send(struct i2c_client *client, char *buf, int count)
{
    int ret;
    struct i2c_msg msg;

    msg.addr = client->addr;
    msg.flags = 0;
    msg.len = count;
    msg.buf = buf;

    ret = i2c_transfer(client->adapter, &msg, 1);
    if (ret != 1) {
        printk("ERROR: %s %d jz_sensor_write_reg failed.\n", __FUNCTION__, __LINE__);
        return -1;
    }

    return 0;
}
static int jz_sensor_read_reg(struct jz_sensor_dev *jz_sensor)
{
    struct i2c_client *client = jz_sensor->client;
    unsigned char* reg = jz_sensor->reg_msg.reg_buf;
    unsigned char value = 0;

    struct i2c_msg msg[2] = {
            [0] = {
            .addr    = client->addr,
            .flags    = 0,
            .len    = jz_sensor->reg_msg.write_size,
            .buf    = reg,
        },
        [1] = {
            .addr    = client->addr,
            .flags    = I2C_M_RD,
            .len    = jz_sensor->reg_msg.read_size,
            .buf    = &value,
        }
    };
    i2c_transfer(client->adapter, msg, 2);
    jz_sensor->reg_msg.reg_buf[2] = value;
    return 0;
}

static int jz_sensor_write_reg(struct jz_sensor_dev *jz_sensor)
{
    return sensor_i2c_master_send(jz_sensor->client,
        jz_sensor->reg_msg.reg_buf, jz_sensor->reg_msg.write_size);
}

static int jz_sensor_read_eeprom (struct jz_sensor_dev *jz_sensor, u8 *buf, u8 offset, u8 size)
{
    return i2c_smbus_read_i2c_block_data(jz_sensor->client, offset, size, buf);
}

static int jz_sensor_write_eeprom (struct jz_sensor_dev *jz_sensor, u8 *buf, u8 offset, u8 size)
{
    return i2c_smbus_write_i2c_block_data(jz_sensor->client, offset, size, buf);
}

static int sensor_open(struct inode *inode, struct file *filp)
{
    return 0;
}

static int sensor_release(struct inode *inode, struct file *filp)
{
    return 0;
}

static ssize_t sensor_read(struct file *filp, char *buf, size_t size, loff_t *l)
{
    printk("sensor: read is not implemented\n");
    return -1;
}

static ssize_t sensor_write(struct file *filp, const char *buf, size_t size, loff_t *l)
{
    printk("sensor: write is not implemented\n");
    return -1;
}

static long sensor_ioctl(struct file *filp, unsigned int cmd, unsigned long arg)
{
    int ret = 0;
    unsigned int i2c_addr;
    struct eeprom_buf rombuf;
    struct miscdevice *dev = filp->private_data;
    struct jz_sensor_dev *jz_sensor = container_of(dev, struct jz_sensor_dev, misc_dev);

    switch (cmd) {
    case IOCTL_READ_REG:
    {
        if (copy_from_user(&jz_sensor->reg_msg, (void *)arg, sizeof(struct IO_MSG)))
            return -EFAULT;
        ret = jz_sensor_read_reg(jz_sensor);
        if (ret)
            return -EFAULT;
        if (copy_to_user((void *)arg, &jz_sensor->reg_msg, sizeof(struct IO_MSG)))
            return -EFAULT;
        break;
    }

    case IOCTL_WRITE_REG:
    {
        if (copy_from_user(&jz_sensor->reg_msg, (void *)arg, sizeof(struct IO_MSG)))
            return -EFAULT;

        ret = jz_sensor_write_reg(jz_sensor);
        if (ret)
            return -EFAULT;
        break;
    }

    case IOCTL_READ_EEPROM:
    {
        if (copy_from_user(&rombuf, (void *)arg, sizeof(rombuf)))
            return -EFAULT;

        ret = jz_sensor_read_eeprom (jz_sensor, &rombuf.rom[0], rombuf.offset, 16);
        if (ret != 16) {
            printk("ERROR: %s %d jz_sensor_read_eeprom failed.\n", __FUNCTION__, __LINE__);
            return -EFAULT;
        }
        rombuf.offset += 16;
        ret = jz_sensor_read_eeprom (jz_sensor, &rombuf.rom[16], rombuf.offset, 16);
        if (ret != 16) {
            printk("ERROR: %s %d jz_sensor_read_eeprom failed.\n", __FUNCTION__, __LINE__);
            return -EFAULT;
        }
        rombuf.offset += 16;
        ret = jz_sensor_read_eeprom (jz_sensor, &rombuf.rom[32], rombuf.offset, 2);
        if (ret != 2) {
            printk("ERROR: %s %d jz_sensor_read_eeprom failed.\n", __FUNCTION__, __LINE__);
            return -EFAULT;
        }
        rombuf.length = 34 ;
        rombuf.offset = 0;
        if(copy_to_user((void *)arg, &rombuf, sizeof(rombuf)))
            return -EFAULT;

        break;
    }

    case IOCTL_WRITE_EEPROM:
    {
        if (copy_from_user(&rombuf, (void *)arg, sizeof(rombuf)))
            return -EFAULT;

        ret = jz_sensor_write_eeprom(jz_sensor, rombuf.rom, rombuf.offset, rombuf.length);
        if (ret != rombuf.length) {
            printk("ERROR: %s %d jz_sensor_read_eeprom jz_sensor_write_eeprom.\n", __FUNCTION__, __LINE__);
            return -EFAULT;
        }
        break;
    }

    case IOCTL_SET_ADDR:
        if (copy_from_user(&i2c_addr, (void *)arg, 4))
            return -EFAULT;
        jz_sensor->client->addr = i2c_addr >> 1;
        break;

    case IOCTL_SET_CLK:
        break;

    default:
        printk("Not supported command: 0x%x %s %d\n", cmd, __FUNCTION__, __LINE__);
        return -EINVAL;
    }
    return 0;
}

static const struct file_operations jz_sensor_fops = {
    .owner        = THIS_MODULE,
    .open        = sensor_open,
    .release    = sensor_release,
    .read        = sensor_read,
    .write        = sensor_write,
    .unlocked_ioctl    = sensor_ioctl,
};

/*
 * i2c_driver functions
 */
static int jz_sensor_probe(struct i2c_client *client,
            const struct i2c_device_id *did)
{
    int ret = -1;
    struct jz_sensor_dev *jz_sensor;
    struct cim_sensor_plat_data *pdata = client->dev.platform_data;
    printk(" SENSOR VERSION =  %s  \n",SENSORDRV_LIB_VERSION);
    if (!pdata) {
        dev_err(&client->dev, "Failed to get platform_data %s %d.\n", __FUNCTION__, __LINE__);
        return -ENXIO;
    }

    jz_sensor = kzalloc(sizeof(struct jz_sensor_dev), GFP_KERNEL);
    if(!jz_sensor) {
        ret = -ENOMEM;
        goto exit;
    }

    if(gpio_is_valid(pdata->pin_i2c_sel2)) {
        ret = gpio_request(pdata->pin_i2c_sel2, "select sensor pin2");
        if(ret < 0)
            goto exit_kfree;
       gpio_direction_output(pdata->pin_i2c_sel2, 1);
    }

    if(gpio_is_valid(pdata->pin_rst)) {
        ret = gpio_request(pdata->pin_rst, "sensor reset pin");
        if(ret < 0)
            goto exit_rst_pin;
        gpio_direction_output(pdata->pin_rst, 1);
        mdelay(10);
        gpio_direction_output(pdata->pin_rst, 0);
        mdelay(10);
        gpio_direction_output(pdata->pin_rst, 1);
        mdelay(10);

    }
    if(gpio_is_valid(pdata->pin_pwron)) {
        ret = gpio_request(pdata->pin_pwron, "sensor pwron ");
        if(ret < 0)
            goto exit_pwr_pin;
        gpio_direction_output(pdata->pin_pwron, 1);
    }

    jz_sensor->i2c_bus_id = client->adapter->nr;
    jz_sensor->name = pdata->name;
    jz_sensor->misc_dev.minor = MISC_DYNAMIC_MINOR;
    jz_sensor->misc_dev.name = jz_sensor->name;
    jz_sensor->misc_dev.fops = &jz_sensor_fops;
    ret = misc_register(&jz_sensor->misc_dev);
    if (ret < 0)
        goto exit_misc;

    i2c_set_clientdata(client, jz_sensor);
    jz_sensor->client = client;

    printk("###########%s driver registered, use i2c-%d.\n", jz_sensor->name, jz_sensor->i2c_bus_id);
    return 0;

exit_misc:
    gpio_free(pdata->pin_pwron);
exit_pwr_pin:
    gpio_free(pdata->pin_rst);
exit_rst_pin:
    gpio_free(pdata->pin_i2c_sel2);
exit_kfree:
    kfree(jz_sensor);
exit:
    return ret;
}

static int jz_sensor_remove(struct i2c_client *client)
{
    struct jz_sensor_dev *jz_sensor = i2c_get_clientdata(client);
    misc_deregister(&jz_sensor->misc_dev);
    kfree(jz_sensor);
    return 0;
}

static const struct i2c_device_id jz_sensor_id[] = {
    { "sensor",  0 },
    { }
};
MODULE_DEVICE_TABLE(i2c, jz_sensor_id);

static struct i2c_driver jz_sensor_i2c_driver = {
    .driver = {
        .name = DRIVER_NAME,
    },
    .probe    = jz_sensor_probe,
    .remove   = jz_sensor_remove,
    .id_table = jz_sensor_id,
};

/*
 * Module functions
 */
static int __init jz_sensor_init(void)
{
    return i2c_add_driver(&jz_sensor_i2c_driver);
}

static void __exit jz_sensor_exit(void)
{
    i2c_del_driver(&jz_sensor_i2c_driver);
}

module_init(jz_sensor_init);
module_exit(jz_sensor_exit);

MODULE_DESCRIPTION(" camera sensor driver");
MODULE_LICENSE("GPL");
MODULE_AUTHOR("Zhang Xiaoyan <xiaoyan.zhang@ingenic.com>");

