#include <linux/gpio.h>
#include <linux/module.h>
#include <linux/delay.h>
#include <linux/kthread.h>

#include <soc/gpio.h>
#include "board.h"

int is_factory_mode;
core_param(is_factory_mode, is_factory_mode, int, 0600);

static int m_threadfn(void *data)
{
    int gpio = GPIO_VOLUMEDOWN;
    int cnt = 0;
    int cnt_1_press = 0;

    while (cnt++ < (3000 / 10)) {
        if (gpio_get_value(gpio) == 0) {
            if (cnt_1_press++ == 30) {
                is_factory_mode = 1;
                printk("------------------------is pb07 factory\n");
                return 0;
            }
        } else {
            cnt_1_press = 0;
        }

        msleep(10);
    }

    return 0;
}

static int m_threadfn_2(void *data)
{
    int gpio_test = GPIO_VOLUMEUP;
    int cnt = 0;
    int cnt_2_press = 0;

    gpio_direction_input(gpio_test);

    /*jz_gpio_set_func(gpio_test, GPIO_INPUT_PULL);*/

    msleep(2000);

    while (cnt++ < (3000 / 10)) {
        if (gpio_get_value(gpio_test) == 0) {
            if (cnt_2_press++ == 200) {
                is_factory_mode = 1;
                printk("------------------------is pb06 factory\n");
                return 0;
            }
        } else {
            if (cnt_2_press)
                printk("---factory press cnt: %d\n", cnt_2_press);
            cnt_2_press = 0;
        }

        msleep(10);
    }

    return 0;
}

static int __init factory_test_init(void)
{
    kthread_run(m_threadfn, NULL, "factory-mode-detect");
    kthread_run(m_threadfn_2, NULL, "factory-mode-detect2");
    return 0;
}

late_initcall_sync(factory_test_init);
