/*
 * Copyright (C) 2020 Ingenic Semiconductor Co., Ltd.
 *
 * Camera driver for the Camera gpio
 *
 */

#include <utils/gpio.h>
#include <bit_field.h>
#include "camera_gpio.h"
#include "camera_cpm.h"
#include "camera_sensor.h"
#include "dvp_gpio_func.h"

#ifndef GPIO_PORT_A
#define GPIO_PORT_A                     0
#endif

#define DVP_D0                          0
#define DVP_D1                          1
#define DVP_D2                          2
#define DVP_D3                          3
#define DVP_D4                          4
#define DVP_D5                          5
#define DVP_D6                          6
#define DVP_D7                          7
#define DVP_D8                          8
#define DVP_D9                          9
#define DVP_D10                         10
#define DVP_D11                         11

#define DVP_HSYNC                       (1 << 12)
#define DVP_VSYNC                       (1 << 13)
#define DVP_PCLK                        (1 << 14)

static const char * dvp_names[] = {
    [0] = "DVP_D0",
    [1] = "DVP_D1",
    [2] = "DVP_D2",
    [3] = "DVP_D3",
    [4] = "DVP_D4",
    [5] = "DVP_D5",
    [6] = "DVP_D6",
    [7] = "DVP_D7",
    [8] = "DVP_D8",
    [9] = "DVP_D9",
    [10] = "DVP_D10",
    [11] = "DVP_D11",
    [12] = "DVP_HSYNC",
    [13] = "DVP_VSYNC",
    [14] = "DVP_PCLK"
};


struct cim_vic_mclk_data {
    int gpio;
    char *name;
    int function;
    int count;
    int cpm_mclk_voltage;
};

static struct cim_vic_mclk_data camera_mclk[2] = {
    {
        .gpio           = GPIO_PE(24),  /* PE24:1V8 */
        .name           = "CIM_VIC_MCLK_PE24",
        .function       = GPIO_FUNC_1,
        .cpm_mclk_voltage = 1,
    },

    {
        .gpio           = GPIO_PC(15),  /* PC15:3V3 */
        .name           = "CIM_VIC_MCLK_PC15",
        .function       = GPIO_FUNC_2,
        .cpm_mclk_voltage = 0,
    },
};

static int dvp_voltage_sel = 0; /* 门限电压选择
                                 * =1: 1.8V
                                 * =0: 3.3V
                                 */

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
            int ret = gpio_request(GPIO_PA(i), dvp_names[i]);
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

/*
 * @brief DVP接口门限电压选择(需根据实际电路设计选择对应模式)
 * @param mode : =1, DVP接口电源VDDIO33_CIM实际供电为1.8V
 *               =0， DVP接口电源VDDIO33_CIM实际供电为3.3V
 */
static void camera_dvp_gpio_voltage(int mode)
{
    cpm_set_bit(CPM_EXCLK_DS, CPM_EXCLK_MCLK_VOLTAGE, !!mode);
}

static unsigned int save_pins;

static int camera_dvp_gpio_init(int data_pin_start, int data_pin_end)
{
    unsigned int pins = bit_field_mask(data_pin_start, data_pin_end) |
                        DVP_PCLK | DVP_HSYNC | DVP_VSYNC;

    int ret = dvp_request_pins(pins);
    if (ret)
        return ret;

    save_pins = pins;

    gpio_port_set_func(GPIO_PORT_A, pins, GPIO_FUNC_0);

    return 0;
}

int dvp_init_select_gpio(struct dvp_bus *dvp_bus,int dvp_gpio_func)
{
    int ret = 0;

    dvp_bus->gpio_mode = (dvp_gpio_mode)dvp_gpio_func;

    camera_dvp_gpio_voltage(dvp_voltage_sel);

    switch(dvp_bus->gpio_mode) {
    case DVP_PA_LOW_10BIT:
        ret = camera_dvp_gpio_init(0, 9);
        break;

    case DVP_PA_HIGH_10BIT:
        ret = camera_dvp_gpio_init(2, 11);
        break;

    case DVP_PA_12BIT:
        ret = camera_dvp_gpio_init(0, 11);
        break;

    case DVP_PA_LOW_8BIT:
        ret = camera_dvp_gpio_init(0, 7);
        break;

    case DVP_PA_HIGH_8BIT:
        ret = camera_dvp_gpio_init(4, 11);
        break;

    default:
        printk(KERN_ERR "dvp select func: Unsupported this format.\n");
        ret = -EINVAL;
        break;
    }

    if (ret)
        printk(KERN_ERR "dvp select func: failed to init dvp pins\n");
    else
        printk(KERN_ERR "dvp select func :dvp_gpio_func --> (%s) OK.\n", dvp_gpio_func_array[dvp_bus->gpio_mode]);

    return ret;
}


void dvp_deinit_gpio(void)
{
    unsigned int pins = save_pins;

    dvp_release_pins(pins);

    save_pins = 0;

    dvp_voltage_sel = 0;
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

    /*
     * DVP门限电压的选择. 当DVP接口时有效,放在此处判断简化配置
     * 有一个配置为1.8V(=1)，则在初始化DVP时将其门限调整为1.8V
     */
    dvp_voltage_sel |= camera_mclk[index].cpm_mclk_voltage;

    int port = camera_mclk[index].gpio / 32;
    int offset = camera_mclk[index].gpio % 32;
    ret = gpio_request(camera_mclk[index].gpio, camera_mclk[index].name);
    if (ret) {
        printk(KERN_ERR "camera vic/cim mclk: failed to request GPIO_P%c%d\n", 'A'+port, offset);
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
