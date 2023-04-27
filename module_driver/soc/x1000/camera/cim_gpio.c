#include <utils/gpio.h>

#define CIM_PCLK 8
#define CIM_HSYNC 9
#define CIM_VSYNC 10
#define CIM_MCLK 11
#define CIM_D7 12
#define CIM_D6 13
#define CIM_D5 14
#define CIM_D4 15
#define CIM_D3 16
#define CIM_D2 17
#define CIM_D1 18
#define CIM_D0 19

static char dvp_names[][10] = {
    [0] = "CIM_PCLK",
    [1] = "CIM_HSYNC",
    [2] = "CIM_VSYNC",
    [3] = "CIM_MCLK",
    [4] = "CIM_D7",
    [5] = "CIM_D6",
    [6] = "CIM_D5",
    [7] = "CIM_D4",
    [8] = "CIM_D3",
    [9] = "CIM_D2",
    [10] = "CIM_D1",
    [11] = "CIM_D0",
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
            int ret = gpio_request(GPIO_PA(i), dvp_names[i - CIM_PCLK]);
            if (ret) {
                printk(KERN_ERR "failed to request GPIO_PA%d\n", i);
                dvp_release_pins(tmp);
                return ret;
            }
            tmp |= (1 << i);
        }
    }

    return 0;
}

static unsigned int save_pins;

static int cim_init_gpio(void)
{
    unsigned int pins = bit_field_mask(CIM_PCLK, CIM_D0);

    int ret = dvp_request_pins(pins);
    if (ret)
        return ret;

    save_pins = pins;

    gpio_port_set_func(GPIO_PORT_A, pins, GPIO_FUNC_2);

    return 0;
}

static inline void cim_deinit_gpio(void)
{
    unsigned int pins = save_pins;

    dvp_release_pins(pins);

    save_pins = 0;
}
