#include <utils/gpio.h>
#include <bit_field.h>

#define SLCD_d0 0
#define SLCD_d1 1
#define SLCD_d2 2
#define SLCD_d3 3
#define SLCD_d4 4
#define SLCD_d5 5
#define SLCD_d6 6
#define SLCD_d7 7
#define SLCD_d8 8
#define SLCD_d9 9
#define SLCD_d10 10
#define SLCD_d11 11
#define SLCD_d12 12
#define SLCD_d13 13
#define SLCD_d14 14
#define SLCD_d15 15

#define SLCD_wr 17
#define SLCD_ce 18
#define SLCD_te 19
#define SLCD_dc 20

static char slcd_names[][9] = {
    [SLCD_d0] = "SLCD_d0",
    [SLCD_d1] = "SLCD_d1",
    [SLCD_d2] = "SLCD_d2",
    [SLCD_d3] = "SLCD_d3",
    [SLCD_d4] = "SLCD_d4",
    [SLCD_d5] = "SLCD_d5",
    [SLCD_d6] = "SLCD_d6",
    [SLCD_d7] = "SLCD_d7",
    [SLCD_d8] = "SLCD_d8",
    [SLCD_d9] = "SLCD_d9",
    [SLCD_d10] = "SLCD_d10",
    [SLCD_d11] = "SLCD_d11",
    [SLCD_d12] = "SLCD_d12",
    [SLCD_d13] = "SLCD_d13",
    [SLCD_d14] = "SLCD_d14",
    [SLCD_d15] = "SLCD_d15",
    [SLCD_wr] = "SLCD_wr",
    [SLCD_ce] = "SLCD_ce",
    [SLCD_te] = "SLCD_te",
    [SLCD_dc] = "SLCD_dc",
};

void slcd_free_gpio(void);

static unsigned long req_pins = 0;
static unsigned long req_ctl_pins = 0;

static int slcd_request_pins(unsigned long pins, unsigned long ctl_pins)
{
    int i;

    for (i = 0; i < 32; i++) {
        if (pins & (1 << i)) {
            int ret = gpio_request(GPIO_PA(i), slcd_names[i]);
            if (ret) {
                slcd_free_gpio();
                printk(KERN_ERR "can's request lcd %s gpio\n", slcd_names[i]);
                return -1;
            }
            req_pins |= (1 << i);
        }

        if (ctl_pins & (1 << i)) {
            int ret = gpio_request(GPIO_PB(i), slcd_names[i]);
            if (ret) {
                slcd_free_gpio();
                printk(KERN_ERR "can's request lcd %s gpio\n", slcd_names[i]);
                return -1;
            }
            req_ctl_pins |= (1 << i);
        }
    }

    return 0;
}

static int slcd_init_gpio(unsigned long pins, int use_te, int use_ce)
{
    int ret;
    unsigned long ctl_pins = 0;

    if (use_te)
        ctl_pins |= BIT(SLCD_te);

    if(use_ce)
        ctl_pins |= BIT(SLCD_ce);

    ctl_pins |= BIT(SLCD_wr);
    ctl_pins |= BIT(SLCD_dc);


    ret = slcd_request_pins(pins, ctl_pins);
    if(ret < 0)
        return ret;

    gpio_port_set_func(GPIO_PORT_A, pins, GPIO_FUNC_1);
    gpio_port_set_func(GPIO_PORT_B, ctl_pins, GPIO_FUNC_1);

    return 0;

}

int slcd_init_gpio_data8(int use_te, int use_ce)
{
    int ret;
    unsigned long pins = 0;

    pins = bit_field_mask(SLCD_d0, SLCD_d7);

    ret = slcd_init_gpio(pins, use_te, use_ce);

    return ret;
}

int slcd_init_gpio_data9(int use_te, int use_ce)
{
    int ret;
    unsigned long pins = 0;

    pins = bit_field_mask(SLCD_d0, SLCD_d8);

    ret = slcd_init_gpio(pins, use_te, use_ce);

    return ret;
}

int slcd_init_gpio_data16(int use_te, int use_ce)
{
    int ret;
    unsigned long pins = 0;

    pins = bit_field_mask(SLCD_d0, SLCD_d15);

    ret = slcd_init_gpio(pins, use_te, use_ce);

    return ret;
}

void slcd_free_gpio(void)
{
    int i;

    for (i = 0; i < 32; i++) {
        if (req_pins & (1 << i))
            gpio_free(GPIO_PA(i));

        if (req_ctl_pins & (1 << i))
            gpio_free(GPIO_PB(i));
    }

    req_pins = 0;
    req_ctl_pins = 0;
}