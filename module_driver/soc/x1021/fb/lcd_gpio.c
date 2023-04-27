#include <common.h>
#include <utils/gpio.h>
#include <bit_field.h>
#include <linux/gpio.h>
#include <soc/x1021/fb/lcdc_data.h>

static void slcd_free_gpio(void);

static int use_pc_pins = 0;

module_param(use_pc_pins, int, 0644);

#define SLCD_pb_d0 6
#define SLCD_pb_d1 7
#define SLCD_pb_d2 8
#define SLCD_pb_d3 9
#define SLCD_pb_d4 10
#define SLCD_pb_d5 11

#define SLCD_pb_d6 13
#define SLCD_pb_d7 14
#define SLCD_pb_wr 15
#define SLCD_pb_te 16
#define SLCD_pb_cs 17
#define SLCD_pb_dc 18

#define SLCD_pc_d0 2
#define SLCD_pc_d1 3
#define SLCD_pc_d2 4
#define SLCD_pc_d3 5
#define SLCD_pc_d4 6
#define SLCD_pc_d5 7
#define SLCD_pc_dc 8
#define SLCD_pc_cs 9

#define SLCD_pc_wr 15
#define SLCD_pc_te 16
#define SLCD_pc_d6 17
#define SLCD_pc_d7 18

static char slcd_pb_names[][9] = {
    [SLCD_pb_d0] = "SLCD_d0",
    [SLCD_pb_d1] = "SLCD_d1",
    [SLCD_pb_d2] = "SLCD_d2",
    [SLCD_pb_d3] = "SLCD_d3",
    [SLCD_pb_d4] = "SLCD_d4",
    [SLCD_pb_d5] = "SLCD_d5",
    [SLCD_pb_d6] = "SLCD_d6",
    [SLCD_pb_d7] = "SLCD_d7",
    [SLCD_pb_wr] = "SLCD_WR",
    [SLCD_pb_te] = "SLCD_te",
    [SLCD_pb_cs] = "SLCD_cs",
    [SLCD_pb_dc] = "SLCD_dc",
};

static char slcd_pc_names[][9] = {
    [SLCD_pc_d0] = "SLCD_d0",
    [SLCD_pc_d1] = "SLCD_d1",
    [SLCD_pc_d2] = "SLCD_d2",
    [SLCD_pc_d3] = "SLCD_d3",
    [SLCD_pc_d4] = "SLCD_d4",
    [SLCD_pc_d5] = "SLCD_d5",
    [SLCD_pc_d6] = "SLCD_d6",
    [SLCD_pc_d7] = "SLCD_d7",
    [SLCD_pc_wr] = "SLCD_WR",
    [SLCD_pc_te] = "SLCD_te",
    [SLCD_pc_cs] = "SLCD_cs",
    [SLCD_pc_dc] = "SLCD_dc",
};

static int rdy_enable;
static int req_pins = 0;

static int slcd_request_pins(unsigned int pins)
{
    int i;
    int ret;

    if (rdy_enable) {
        ret = gpio_request(GPIO_PB(28), "SLCD_rdy");
        if (ret) {
            printk(KERN_ERR "can's request lcd slcd_rdy gpio\n");
            return -1;
        }
    }

    if (use_pc_pins) {
        for (i = 0; i < 32; i++) {
            if (pins & (1 << i)) {
                ret = gpio_request(GPIO_PC(i), slcd_pc_names[i]);
                if (ret) {
                    printk(KERN_ERR "can's request lcd %s gpio\n", slcd_pc_names[i]);
                    slcd_free_gpio();
                    return -1;
                }

                req_pins |= (1 << i);
            }
        }
    } else {
        for (i = 0; i < 32; i++) {
            if (pins & (1 << i)) {
                ret = gpio_request(GPIO_PB(i), slcd_pb_names[i]);
                if (ret) {
                    printk(KERN_ERR "can's request lcd %s gpio\n", slcd_pb_names[i]);
                    slcd_free_gpio();
                    return -1;
                }

                req_pins |= (1 << i);
            }
        }
    }



    return 0;
}

static int slcd_init_gpio(unsigned int pins, int use_rdy, int use_te, int use_cs)
{
    int ret;

    if (use_rdy)
        rdy_enable = 1;

    if (use_pc_pins) {
        if (use_te)
            pins |= BIT(SLCD_pc_te);
        if (use_cs)
            pins |= BIT(SLCD_pc_cs);

        pins |= BIT(SLCD_pc_wr);
        pins |= BIT(SLCD_pc_dc);
    } else {
        if (use_te)
            pins |= BIT(SLCD_pb_te);
        if (use_cs)
            pins |= BIT(SLCD_pb_cs);

        pins |= BIT(SLCD_pb_wr);
        pins |= BIT(SLCD_pb_dc);
    }

    ret = slcd_request_pins(pins);
    if (ret < 0)
        return ret;

    if (rdy_enable)
        gpio_set_func(GPIO_PB(28), GPIO_FUNC_3);

    if (use_pc_pins)
        gpio_port_set_func(GPIO_PORT_C, pins, GPIO_FUNC_3);
    else
        gpio_port_set_func(GPIO_PORT_B, pins, GPIO_FUNC_3);

    return 0;
}

static int slcd_init_gpio_data8(int use_rdy, int use_te, int use_cs)
{
    unsigned int pins;
    int ret;

    if (use_pc_pins)
        pins = bit_field_mask(SLCD_pc_d0, SLCD_pc_d5) | bit_field_mask(SLCD_pc_d6, SLCD_pc_d7);
    else
        pins = bit_field_mask(SLCD_pb_d0, SLCD_pb_d5) | bit_field_mask(SLCD_pb_d6, SLCD_pb_d7);


    ret = slcd_init_gpio(pins, use_rdy, use_te, use_cs);
    if (ret < 0)
        return ret;

    return 0;
}

static void slcd_free_gpio(void)
{
    int i;

    if (rdy_enable)
        gpio_free(GPIO_PB(28));

    if (use_pc_pins) {
        for (i = 0; i < 32; i++) {
            if (req_pins & (1 << i))
                gpio_free(GPIO_PC(i));
        }
    } else {
        for (i = 0; i < 32; i++) {
            if (req_pins & (1 << i))
                gpio_free(GPIO_PB(i));
        }
    }

    rdy_enable = 0;
    req_pins = 0;
}