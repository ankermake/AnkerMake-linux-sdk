/*
 * Copyright (C) 2021 Ingenic Semiconductor Co., Ltd.
 *
 * Camera driver for the Camera gpio
 *
 */

#include <utils/gpio.h>
#include <bit_field.h>
#include "camera_sensor.h"

#ifndef GPIO_PORT_A
#define GPIO_PORT_A                     0
#endif

#define CIM_D0                          8
#define CIM_D1                          9
#define CIM_D2                          10
#define CIM_D3                          11
#define CIM_D4                          12
#define CIM_D5                          13
#define CIM_D6                          14
#define CIM_D7                          15

#define CIM_HSYNC                       (1 << 22)
#define CIM_VSYNC                       (1 << 21)
#define CIM_PCLK                        (1 << 20)
#define CIM_EXPOSURE                    (1 << 19)


static const char * dvp_names[] = {
    [0] = "CIM_D0",
    [1] = "CIM_D1",
    [2] = "CIM_D2",
    [3] = "CIM_D3",
    [4] = "CIM_D4",
    [5] = "CIM_D5",
    [6] = "CIM_D6",
    [7] = "CIM_D7",
    [8] = "reserved0",
    [9] = "reserved1",
    [10] = "reserved2",
    [11] = "CIM_EXPOSURE",
    [12] = "CIM_PCLK",
    [13] = "CIM_VSYNC",
    [14] = "CIM_HSYNC",
};


struct cim_vic_mclk_data {
    int gpio;
    char *name;
    int function;
    int count;
};

static struct cim_vic_mclk_data camera_mclk[2] = {
    {
        .gpio           = GPIO_PA(24),  /* PA24 */
        .name           = "CIM_MCLK_PA24",
        .function       = GPIO_FUNC_2,
    },

    {
        .gpio           = GPIO_PC(25),  /* PC25:外部时钟24M旁路输出,输出频率24M不可调整 */
        .name           = "CIM_MCLK_PC25",
        .function       = GPIO_FUNC_0,
    },
};


static DEFINE_MUTEX(mclk_lock);

static inline void dvp_release_pins(unsigned int pins)
{
    int i;

    for (i = 0; i < 32; i++) {
        if (pins & (1 << i)) {
            gpio_free(GPIO_PA(i));
        }
    }
}

static inline int dvp_request_pins(unsigned int pins)
{
    int i;
    int tmp = 0;

    for (i = 0; i < 32; i++) {
        if (pins & (1 << i)) {
            int ret = gpio_request(GPIO_PA(i), dvp_names[i - CIM_D0]);
            if (ret) {
                printk(KERN_ERR "camera: failed to request GPIO_PA%d\n", i);
                dvp_release_pins(tmp);
                return ret;
            }
            tmp |= (1 << i);
        }
    }

    return 0;
}

/*
 * @brief 检查mclk管脚是否正确
 * @param gpio : mclk管脚
 * @return 成功返回index, 失败返回负数
 */
static int camera_mclk_gpio_parameter_check(int gpio)
{
    int i = 0;
    struct cim_vic_mclk_data *mclk_pin_alter = camera_mclk;
    int num = ARRAY_SIZE(camera_mclk);

    for (i = 0; i < num; i++) {
        if (gpio == mclk_pin_alter[i].gpio)
            break;
    }

    if (i >= num) {
        char mclk_string[10];
        gpio_to_str(gpio, mclk_string);
        printk(KERN_ERR "camera mclk(%s) is invaild\n", mclk_string);
        return -EINVAL;
    }

    return i;
}

static unsigned int save_pins;

static int camera_dvp_gpio_init(int data_pin_start, int data_pin_end)
{
    unsigned int pins = bit_field_mask(data_pin_start, data_pin_end) |
                        CIM_PCLK | CIM_HSYNC | CIM_VSYNC;

    int ret = dvp_request_pins(pins);
    if (ret)
        return ret;

    save_pins = pins;

    gpio_port_set_func(GPIO_PORT_A, pins, GPIO_FUNC_2);

    return 0;
}

int dvp_init_select_gpio(void)
{
    int ret = 0;

    ret = camera_dvp_gpio_init(CIM_D0, CIM_D7);

    if (ret)
        printk(KERN_ERR "dvp select func: failed to init dvp pins\n");

    return ret;
}


void dvp_deinit_gpio(void)
{
    unsigned int pins = save_pins;

    dvp_release_pins(pins);

    save_pins = 0;
}


int camera_mclk_gpio_init(int gpio)
{
    int index = -1;
    int ret = 0;

    index = camera_mclk_gpio_parameter_check(gpio);
    if (index < 0)
        return index;

    mutex_lock(&mclk_lock);

    if( camera_mclk[index].count > 0 ) {
        camera_mclk[index].count++;
        goto mclk_unlock;
    }

    int port = camera_mclk[index].gpio / 32;
    int offset = camera_mclk[index].gpio % 32;
    ret = gpio_request(camera_mclk[index].gpio, camera_mclk[index].name);
    if (ret) {
        printk(KERN_ERR "camera mclk: failed to request GPIO_P%c%d\n", 'A'+port, offset);
        goto mclk_unlock;
    }

    gpio_port_set_func(port, BIT(offset), camera_mclk[index].function);
    camera_mclk[index].count++;

mclk_unlock:
    mutex_unlock(&mclk_lock);

    return ret;
}


int camera_mclk_gpio_deinit(int gpio)
{
    int index = -1;

    index = camera_mclk_gpio_parameter_check(gpio);
    if (index < 0)
        return index;

    mutex_lock(&mclk_lock);

    if( camera_mclk[index].count > 1 ) {
        camera_mclk[index].count--;
        goto mclk_unlock;
    }

    gpio_free(camera_mclk[index].gpio);
    camera_mclk[index].count--;

mclk_unlock:
    mutex_unlock(&mclk_lock);

    return 0;
}

EXPORT_SYMBOL(dvp_init_select_gpio);
EXPORT_SYMBOL(dvp_deinit_gpio);
