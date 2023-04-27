/*
 *  Copyright (C) 2015 Wu Jiao <jiao.wu@ingenic.com wujiaososo@qq.com>
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
#include <linux/sysfs.h>
#include <linux/platform_device.h>
#include <linux/suspend.h>
#include <linux/ctype.h>
#include <linux/string.h>
#include <linux/gpio.h>
#include <soc/gpio.h>

static int stricmp(const char *s1, const char *s2)
{
    int c1, c2;

    do {
        c1 = tolower(*s1++);
        c2 = tolower(*s2++);
    } while (c1 == c2 && c1 != 0);
    return c1 - c2;
}

static char *endl_to_0(char * str, int len)
{
    char *ptr = NULL;
    if (!str)
        return NULL;
    ptr = str;
    while (--len && *str != '\n') {
        str++;
    }
    *str = '\0';
    return ptr;
}

static int str_to_gpio_num(const char *str)
{
    int port;
    int pin;

    if (tolower(str[0]) != 'p')
        return -1;

    if (tolower(str[1]) < 'a' || tolower(str[1]) > 'g')
        return -1;

    port = tolower(str[1]) - 'a';

    if (strlen(str) == 3) {
        if (str[2] < '0' || str[2] > '9')
            return -1;

        pin = str[2] - '0';
    } else if (strlen(str) == 4) {
        if (str[2] < '0' || str[2] > '9' ||
            str[3] < '0' || str[3] > '9')
            return -1;

        pin = (str[2] - '0') * 10 + (str[3] - '0');
    } else {
        return -1;
    }

    return port * 32 + pin;
}

static int str_to_gpio_func(const char *str)
{
    if (!stricmp(str, "low"))
        return GPIO_OUTPUT0;
    if (!stricmp(str, "high"))
        return GPIO_OUTPUT1;
    if (!stricmp(str, "in") || !stricmp(str, "input"))
        return GPIO_INPUT;
    if (!stricmp(str, "no_pull"))
        return GPIO_PULL_HIZ;
    if (!stricmp(str, "pull_high"))
        return GPIO_PULL_UP;
    if (!stricmp(str, "pull_low"))
        return GPIO_PULL_DOWN;
    if (!stricmp(str, "func0"))
        return GPIO_FUNC_0;
    if (!stricmp(str, "func1"))
        return GPIO_FUNC_1;
    if (!stricmp(str, "func2"))
        return GPIO_FUNC_2;
    if (!stricmp(str, "func3"))
        return GPIO_FUNC_3;

    return -1;
}

static int m_gpio = 0;

static ssize_t get_value_r(struct device *dev, struct device_attribute *attr, char *buf)
{
    return sprintf(buf, "%d", gpio_get_value(m_gpio));
}

static ssize_t get_value_w(struct device *dev, struct device_attribute *attr, const char *buf, size_t count)
{
    int gpio, func;
    char str[256], str0[256] = "", str1[256] = "";

    if (count > 256) {
        printk("%s: len is too long %d\n", __func__, count);
        return count;
    }

    memcpy(str, buf, count);
    endl_to_0(str, count);

    sscanf(str, "%s %s", str0, str1);

    gpio = str_to_gpio_num(str0);
    if (gpio < 0) {
        printk("%s: gpio is not valid: %s\n", __func__, str0);
        return count;
    }

    func = str_to_gpio_func(str1);
    if (func < 0) {
        printk("%s: func is not valid: %s\n", __func__, str1);
        return count;
    }

    printk("gpio: %d, func: %x\n", gpio, func);

    jz_gpio_set_func(gpio, func);

    m_gpio = gpio;

    return count;
}

static DEVICE_ATTR(get_value, S_IRUGO|S_IWUGO, get_value_r, get_value_w);

static ssize_t test_r(struct device *dev, struct device_attribute *attr, char *buf)
{
    return 0;
}

static ssize_t test_w(struct device *dev, struct device_attribute *attr, const char *buf, size_t count)
{
    int gpio, func;
    char str[256], str0[256] = "", str1[256] = "";

    if (count > 256) {
        printk("%s: len is too long %d\n", __func__, count);
        return count;
    }

    memcpy(str, buf, count);
    endl_to_0(str, count);

    sscanf(str, "%s %s", str0, str1);

    gpio = str_to_gpio_num(str0);
    if (gpio < 0) {
        printk("%s: gpio is not valid: %s\n", __func__, str0);
        return count;
    }

    func = str_to_gpio_func(str1);
    if (func < 0) {
        printk("%s: func is not valid: %s\n", __func__, str1);
        return count;
    }

    printk("gpio: %d, func: %x\n", gpio, func);

    jz_gpio_set_func(gpio, func);

    return count;
}

static DEVICE_ATTR(test, S_IRUGO|S_IWUGO, test_r, test_w);

static struct attribute *gpio_test_attrs[] = {
    &dev_attr_test.attr,
    &dev_attr_get_value.attr,
    NULL,
};

static struct attribute_group gpio_test_attr_group = {
    .name    = "debug",
    .attrs    = gpio_test_attrs,
};

static int gpio_test_probe(struct platform_device *pdev) {
    int ret;
    
    ret = sysfs_create_group(&pdev->dev.kobj, &gpio_test_attr_group);
    BUG_ON(ret != 0);
    
    return 0;
}

static int gpio_test_remove(struct platform_device *pdev) {
    return 0;
}

int gpio_test_suspend(struct platform_device *pdev, pm_message_t state) {
    return 0;
}

int gpio_test_resume(struct platform_device *pdev) {
    return 0;
}

void gpio_test_shutdown(struct platform_device *pdev) {
    
}

static struct platform_driver gpio_test_driver = {
    .probe = gpio_test_probe,
    .remove  = gpio_test_remove,
    .driver.name = "gpio_test",
    .suspend = gpio_test_suspend,
    .resume = gpio_test_resume,
    .shutdown = gpio_test_shutdown,
};

static struct platform_device gpio_test_device = {
    .id = -1,
    .name = "gpio_test",
};

int __init gpio_test_init(void) {
    int ret;

    ret = platform_driver_register(&gpio_test_driver);
    if (ret) {
        pr_err("gpio_test: gpio_test driver register failed\n");
        ret = -EINVAL;
        goto error_platform_driver_register_failed;
    }
    
    ret = platform_device_register(&gpio_test_device);
    if (ret) {
        pr_err("gpio_test: gpio_test device register failed\n");
        ret = -EINVAL;
        goto error_platform_device_register_failed;
    }
    
    return 0;
error_platform_driver_register_failed:
    ;/* ToDo : write your error deal code here */
error_platform_device_register_failed:
    platform_driver_unregister(&gpio_test_driver);
return ret;
}
module_init(gpio_test_init);

void __exit gpio_test_exit(void) {
    platform_device_unregister(&gpio_test_device);
    platform_driver_unregister(&gpio_test_driver);
}
module_exit(gpio_test_exit);



