/*
 *  Copyright (C) 2017 xin shuan <shuan.xin@ingenic.com>
 *
 *  This program is free software; you can redistribute it and/or modify it
 *  under  the terms of the GNU General  Public License as published by the
 *  Free Software Foundation;  either version 2 of the License, or (at your
 *  option) any later version.
 *
 *  You should have received a copy of the GNU General Public License along
 *  with this program; if not, write to the Free Software Foundation, Inc.,
 *  675 Mass Ave, Cambridge, MA 02139, USA.
 */

#include <linux/module.h>
#include <linux/gpio.h>
#include <linux/string.h>
#include <linux/miscdevice.h>
#include <linux/fs.h>
#include <linux/delay.h>
#include <common.h>
#include <linux/slab.h>
#include <linux/mutex.h>
#include <linux/pwm.h>
#include <linux/platform_device.h>

#define DEVNAME "x01_led"

#define X01TYPE 'F'
#define X01_SET_GET_LED_TYPE	_IOW(X01TYPE, 0, int)
#define X01_SET_GET_LED_PINS	_IOW(X01TYPE, 1, int)
#define X01_SET_LED_NUM_BRIGHT	_IOW(X01TYPE, 2, int)
#define X01_SET_LED_PINS		_IOW(X01TYPE, 3, int)
#define X01_SET_LED_DRIVER_TYPE _IOW(X01TYPE, 4, int)

#define BRIGHT_MAX 255
#define PERIOD_NS 30000

enum x01_led_type {
    MONOCHROME_LED,
    RGB_LED,
    BGR_LED,
    RBG_LED,
    BRG_LED,
    GRB_LED,
    GBR_LED,
    ERROR_TYPE
};

enum led_driver_type {
    LINEAR,
    MATRIX
};

struct led_range {
    int start;
    int end;
    unsigned char *buf;
};

struct jz_pwm_led {
    const char *name;
    unsigned int pwm_id;
    bool active_low;
};

struct jz_pwm_led_platform_data {
    const char *dev_name;
    int led_type;
    int led_pins;
    struct jz_pwm_led *pwm_leds;
};

struct jz_pwm_led_dev {
    unsigned int open_cnt;
    struct mutex led_mutex;
    int led_type;
    int led_pins;
    struct jz_pwm_led *pwm_leds;
    struct pwm_device **pwm_dev;
    unsigned char *led_buf;

    struct miscdevice miscdev;
};


static int x01_set_led_range_bright(struct jz_pwm_led_dev *led_dev, struct led_range *led_data)
{
    int i, tmp, ret = 0;

    if(led_data->start > led_data->end) {
        pr_err("ERROR: %s:%d start %d > end %d\n", __func__, __LINE__, led_data->start, led_data->end);
        return -EINVAL;
    }

    if(led_data->end > led_dev->led_pins - 1) {
        pr_err("ERROR: %s:%d end %d > pins - 1 %d\n", __func__, __LINE__, led_data->end, led_dev->led_pins);
        return -EINVAL;
    }

    for(i = 0; i + led_data->start <= led_data->end; i++) {
        tmp = pwm_config(led_dev->pwm_dev[i + led_data->start], led_data->buf[i] * PERIOD_NS / BRIGHT_MAX, PERIOD_NS);
        if(tmp) {
            ret = tmp;
            pr_err("ERROR: %s:%d pwm_config %d led_data->buf[i] = %d\n", __func__, __LINE__, ret, led_data->buf[i]);
        }

        if(led_data->buf[i] == 0 || led_data->buf[i] == BRIGHT_MAX)
            pwm_disable(led_dev->pwm_dev[i + led_data->start]);
        else
            pwm_enable(led_dev->pwm_dev[i + led_data->start]);
    }

    return ret;
}

static int x01_led_open(struct inode *inodp, struct file *filp)
{
    struct miscdevice *mdev = filp->private_data;
    struct jz_pwm_led_dev *led_dev = container_of(mdev, struct jz_pwm_led_dev, miscdev);

    mutex_lock(&led_dev->led_mutex);
    led_dev->open_cnt++;
    mutex_unlock(&led_dev->led_mutex);
    return 0;
}

static int x01_led_close(struct inode *inodp, struct file *filp)
{
    struct miscdevice *mdev = filp->private_data;
    struct jz_pwm_led_dev *led_dev = container_of(mdev, struct jz_pwm_led_dev, miscdev);

    mutex_lock(&led_dev->led_mutex);
    led_dev->open_cnt--;

    if (led_dev->open_cnt == 0) {
        int i;
        for(i = 0; i < led_dev->led_pins; i++) {
            pwm_config(led_dev->pwm_dev[i], 0, PERIOD_NS);
            pwm_disable(led_dev->pwm_dev[i]);
        }
    }
    mutex_unlock(&led_dev->led_mutex);
    return 0;
}

static long x01_led_ioctl(struct file *filp, unsigned int cmd, unsigned long args)
{
    int ret = 0;
    struct led_range buf;
    struct miscdevice *mdev = filp->private_data;
    struct jz_pwm_led_dev *led_dev = container_of(mdev, struct jz_pwm_led_dev, miscdev);

    mutex_lock(&led_dev->led_mutex);

    if (_IOC_TYPE(cmd) == X01TYPE) {
        switch (cmd) {
        case X01_SET_GET_LED_TYPE:
            ret = copy_to_user((void __user *)args, (void *)&led_dev->led_type, sizeof(led_dev->led_type));

            if(ret != 0) {
                ret = -EFAULT;
                pr_err("ERROR: %s:%d X01_SET_GET_LED_TYPE %d\n", __func__, __LINE__, ret);
            }
            break;

        case X01_SET_GET_LED_PINS:
            ret = copy_to_user((void __user *)args, (void *)&led_dev->led_pins, sizeof(led_dev->led_pins));
            if(ret != 0) {
                ret = -EFAULT;
                pr_err("ERROR: %s:%d X01_SET_GET_LED_PINS %d\n", __func__, __LINE__, ret);
            }
            break;

        case X01_SET_LED_NUM_BRIGHT:
            ret = copy_from_user((void *)&buf, (void __user *)args, sizeof(struct led_range));
            if(!ret) {
                ret = copy_from_user((void *)led_dev->led_buf, (void *)buf.buf, sizeof(unsigned char) * led_dev->led_pins);
                buf.buf = led_dev->led_buf;
            }
            if (ret) {
                ret = -EFAULT;
                pr_err("ERROR: %s:%d X01_SET_LED_NUM_BRIGHT %d\n", __func__, __LINE__, ret);
                break;
            }
            ret = x01_set_led_range_bright(led_dev, &buf);
            if (ret < 0)
                pr_err("ERROR: %s:%d write led data failed %d\n", __func__, __LINE__, ret);
            break;

        case X01_SET_LED_PINS:
            if(args > led_dev->led_pins) {
                ret = -EINVAL;
                pr_err("ERROR: %s:%d X01_SET_LED_PINS %d\n", __func__, __LINE__, ret);
                break;
            }

            break;

        case X01_SET_LED_DRIVER_TYPE:
            if(args != LINEAR) {
                ret = -ENOTSUPP;
                pr_err("ERROR: %s:%d X01_SET_LED_DRIVER_TYPE %d\n", __func__, __LINE__, ret);
            }
            break;
        default:
            ret =  -EINVAL;
        }
    }

    mutex_unlock(&led_dev->led_mutex);
    return ret;
}


static struct file_operations x01_file_ops = {
    .owner = THIS_MODULE,
    .open = x01_led_open,
    .unlocked_ioctl = x01_led_ioctl,
    .release = x01_led_close,
};

static int x01_led_probe(struct platform_device *pdev)
{
    int i;
    int ret = 0;
    struct jz_pwm_led_dev *led_dev;
    struct jz_pwm_led_platform_data *pdata = (struct jz_pwm_led_platform_data *)pdev->dev.platform_data;

    if(pdata == NULL){
        pr_err("ERROR: %s:%d pdata == NULL\n", __func__, __LINE__);
        return -1;
    }

    led_dev = kzalloc(sizeof(struct jz_pwm_led_dev), GFP_KERNEL);
    if(led_dev == NULL) {
        pr_err("ERROR: %s:%d kzalloc led_dev\n", __func__, __LINE__);
        return -1;
    }

    led_dev->pwm_dev = kzalloc(sizeof(struct pwm_device *) * pdata->led_pins, GFP_KERNEL);
    if(led_dev->pwm_dev == NULL) {
        kfree(led_dev);
        pr_err("ERROR: %s:%d kzalloc pwm_dev\n", __func__, __LINE__);
        return -1;
    }

    led_dev->led_buf = kzalloc(sizeof(unsigned char) * pdata->led_pins, GFP_KERNEL);
    if(led_dev->led_buf == NULL) {
        kfree(led_dev->pwm_dev);
        kfree(led_dev);
        pr_err("ERROR: %s:%d kzalloc led_buf\n", __func__, __LINE__);
        return -1;
    }

    led_dev->led_type = pdata->led_type;
    led_dev->led_pins = pdata->led_pins;
    led_dev->pwm_leds = pdata->pwm_leds;

    for(i = 0; i < led_dev->led_pins; i++) {
        led_dev->pwm_dev[i] = pwm_request(led_dev->pwm_leds[i].pwm_id, led_dev->pwm_leds[i].name);
        if(IS_ERR(led_dev->pwm_dev[i])) {
            ret = PTR_ERR(led_dev->pwm_dev[i]);

            while(--i >= 0) {
                pwm_free(led_dev->pwm_dev[i]);
            }

            kfree(led_dev->led_buf);
            kfree(led_dev->pwm_dev);
            kfree(led_dev);
            pr_err("ERROR: %s:%d unable to request PWM for %s\n", __func__, __LINE__, led_dev->pwm_leds[i].name);
            return ret;
        }

        led_dev->pwm_dev[i]->active_level = !led_dev->pwm_leds[i].active_low;
        pwm_config(led_dev->pwm_dev[i], 0, PERIOD_NS);
        pwm_disable(led_dev->pwm_dev[i]);
    }

    led_dev->miscdev.minor = MISC_DYNAMIC_MINOR;
    led_dev->miscdev.name = pdata->dev_name;
    led_dev->miscdev.fops = &x01_file_ops;

    ret = misc_register(&led_dev->miscdev);
    if(ret) {
        for(i = 0; i < led_dev->led_pins; i++)
            pwm_free(led_dev->pwm_dev[i]);

        kfree(led_dev->led_buf);
        kfree(led_dev->pwm_dev);
        kfree(led_dev);
        pr_err("ERROR: %s:%d misc_register failed %d\n", __func__, __LINE__, ret);
        return ret;
    }

    mutex_init(&led_dev->led_mutex);
    platform_set_drvdata(pdev, led_dev);
    return 0;
}

static int x01_led_remove(struct platform_device *pdev)
{
    int i;
    struct jz_pwm_led_dev *led_dev = platform_get_drvdata(pdev);

    misc_deregister(&led_dev->miscdev);

    for(i = 0; i < led_dev->led_pins; i++) {
        pwm_config(led_dev->pwm_dev[i], 0, PERIOD_NS);
        pwm_disable(led_dev->pwm_dev[i]);
        pwm_free(led_dev->pwm_dev[i]);
    }

    kfree(led_dev->led_buf);
    kfree(led_dev->pwm_dev);
    kfree(led_dev);

    return 0;
}

static struct platform_driver pwm_led_drv = {
    .probe = x01_led_probe,
    .remove = x01_led_remove,
    .driver = {
        .name = DEVNAME,
    }
};

module_platform_driver(pwm_led_drv);
MODULE_LICENSE("GPL");
