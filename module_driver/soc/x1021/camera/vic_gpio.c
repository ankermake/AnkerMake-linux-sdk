#include <utils/gpio.h>
#include <bit_field.h>
#include "vic_sensor.h"

#define DVP_D0 0
#define DVP_D1 1
#define DVP_D2 2
#define DVP_D3 3
#define DVP_D4 4
#define DVP_D5 5
#define DVP_D6 6
#define DVP_D7 7
#define DVP_D8 8
#define DVP_D9 9
#define DVP_D10 10
#define DVP_D11 11

#define DVP_PCLK (1 << 14)
#define DVP_MCLK (1 << 15)
#define DVP_HSYNC (1 << 16)
#define DVP_VSYNC (1 << 17)

static char dvp_names[][10] = {
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
    [14] = "DVP_PCLK",
    [15] = "DVP_MCLK",
    [16] = "DVP_HSYNC",
    [17] = "DVP_VSYNC",
};

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

static unsigned int save_pins;

static int dvp_init_gpio(int data_pin_start, int data_pin_end)
{
    unsigned int pins = bit_field_mask(data_pin_start, data_pin_end) |
                        DVP_PCLK | DVP_HSYNC | DVP_VSYNC;

    int ret = dvp_request_pins(pins);
    if (ret)
        return ret;

    save_pins = pins;

    gpio_port_set_func(GPIO_PORT_A, pins, GPIO_FUNC_1);

    return 0;
}

int dvp_init_low10bit_gpio(void)
{
    return dvp_init_gpio(0, 9);
}

int dvp_init_low8bit_gpio(void)
{
    return dvp_init_gpio(0, 7);
}

void dvp_deinit_gpio(void)
{
    unsigned int pins = save_pins;

    dvp_release_pins(pins);

    save_pins = 0;
}

int dvp_init_mclk_gpio(void)
{
    unsigned int pins = DVP_MCLK;

    int ret = dvp_request_pins(pins);
    if (ret)
        return ret;

    gpio_port_set_func(GPIO_PORT_A, pins, GPIO_FUNC_1);

    return 0;
}

void dvp_deinit_mclk_gpio(void)
{
    dvp_release_pins(DVP_MCLK);
}
