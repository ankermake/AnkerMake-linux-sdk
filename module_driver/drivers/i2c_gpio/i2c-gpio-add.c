#include <linux/module.h>
#include <linux/string.h>
#include <linux/slab.h>
#include <linux/gpio.h>
#include <utils/string.h>
#include <utils/gpio.h>
#include <linux/list.h>
#include <linux/kernel.h>
#include <linux/platform_device.h>

#include "i2c-gpio.h"

#ifndef DRIVER_NAME
#define DRIVER_NAME                     "md_i2c_gpio"
#endif

static LIST_HEAD(gpio_i2c_list);

struct gpio_i2c_data {
    struct list_head link;
    struct platform_device pdev;
    struct i2c_gpio_pdata pdata;
};

static void i2c_gpio_add_device_release(struct device *_dev)
{
    /* TODO:Fix rmmod Warning */
}

static int param_i2c_bus_set(const char *arg, const struct kernel_param *kp)
{
    int ret, bus_num = -1;
    struct i2c_gpio_pdata pdata;

    memset(&pdata, 0, sizeof(pdata));
    pdata.scl_pin = -1;
    pdata.sda_pin = -1;

    int i, N;
    char **argv = str_to_words(arg, &N);
    for (i = 0; i < N; i++) {
        if (strstarts(argv[i], "scl=")) {
            ret = str_to_gpio(argv[i] + strlen("scl="));
            if (ret < -1) {
                printk(KERN_ERR "%s is not a gpio\n", argv[i]);
                goto parse_err;
            }
            pdata.scl_pin = ret;
            continue;
        }

        if (strstarts(argv[i], "sda=")) {
            ret = str_to_gpio(argv[i] + strlen("sda="));
            if (ret < -1) {
                printk(KERN_ERR "%s is not a gpio\n", argv[i]);
                goto parse_err;
            }
            pdata.sda_pin = ret;
            continue;
        }

        if (strstarts(argv[i], "rate=")) {
            unsigned long rate;
            ret = kstrtoul(argv[i] + strlen("rate="), 0, &rate);
            if (ret) {
                printk(KERN_ERR "%s is not a valid rate\n", argv[i]);
                goto parse_err;
            }
            pdata.udelay = ((rate == 0) ? 0 : 1000000UL / rate);
            continue;
        }

        if (strstarts(argv[i], "bus_num=")) {
            unsigned long num;
            ret = kstrtoul(argv[i] + strlen("bus_num="), 0, &num);
            if (ret) {
                printk(KERN_ERR "%s is not a valid num\n", argv[i]);
                goto parse_err;
            }
            bus_num = num;
            continue;
        }

        printk(KERN_ERR "%s can not be parse!\n", argv[i]);
        goto parse_err;
    }

    if (pdata.scl_pin == -1) {
        printk(KERN_ERR "scl must define!\n");
        goto parse_err;
    }

    if (bus_num == -1) {
        printk(KERN_ERR "bus_num must define!\n");
        goto parse_err;
    }

    str_free_words(argv);

    struct gpio_i2c_data *data = kzalloc(sizeof(*data), GFP_KERNEL);

    data->pdata         = pdata;
    data->pdev.name     = DRIVER_NAME;
    data->pdev.id       = bus_num;
    data->pdev.dev.platform_data = &data->pdata;
    data->pdev.dev.release       = i2c_gpio_add_device_release,

    ret = platform_device_register(&data->pdev);
    if (ret < 0) {
        printk(KERN_ERR "\ni2c_gpio: failed to register platform device\n");
        kfree(data);
        return ret;
    }

    list_add_tail(&data->link, &gpio_i2c_list);

    return 0;

parse_err:
    str_free_words(argv);
    return -EINVAL;
}

static int param_i2c_bus_get(char *buffer, const struct kernel_param *kp)
{
    struct list_head *pos;
    char *buf = buffer;

    list_for_each(pos, &gpio_i2c_list) {
        struct gpio_i2c_data *data = list_entry(pos, struct gpio_i2c_data, link);
        char scl[10];
        char sda[10];
        int bus_num = data->pdev.id;

        gpio_to_str(data->pdata.scl_pin, scl);
        gpio_to_str(data->pdata.sda_pin, sda);
        buf += sprintf(buf, "bus_num=%d udelay=%dus scl=%s sda=%s\n", bus_num, data->pdata.udelay, scl, sda);
    }

    return buf - buffer;
}

static struct kernel_param_ops param_i2c_bus_ops = {
    .set = param_i2c_bus_set,
    .get = param_i2c_bus_get,
};

module_param_cb(i2c_bus, &param_i2c_bus_ops, NULL, 0644);

int i2c_gpio_platform_device_remove(void)
{
    struct gpio_i2c_data *data, *p_next;

    list_for_each_entry_safe(data, p_next, &gpio_i2c_list, link) {
        list_del(&data->link);
        platform_device_unregister(&data->pdev);
        kfree(data);
    }

    return 0;
}

EXPORT_SYMBOL(i2c_gpio_platform_device_remove);

