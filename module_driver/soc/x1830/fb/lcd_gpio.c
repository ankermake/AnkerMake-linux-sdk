#include <common.h>
#include <bit_field.h>
#include <linux/gpio.h>
#include <soc/x1830/fb/lcdc_data.h>

#define TFT_d2 2
#define TFT_d3 3
#define TFT_d4 4
#define TFT_d5 5
#define TFT_d6 6
#define TFT_d7 7
#define TFT_pclk 8
#define TFT_de 9

#define TFT_d10 12
#define TFT_d11 13
#define TFT_d12 14
#define TFT_d13 15
#define TFT_d14 16
#define TFT_d15 17
#define TFT_vsync 18
#define TFT_hsync 19

#define TFT_d18 22
#define TFT_d19 23
#define TFT_d20 24
#define TFT_d21 25
#define TFT_d22 26
#define TFT_d23 27

static unsigned int m_pins;

static void jzfb_release_pins(void)
{
    int i;

    for (i = 0; i < 32; i++) {
        if (m_pins & (1 << i))
            gpio_free(GPIO_PD(i));
    }

    m_pins = 0;
}

static char tft_names[][10] = {
    [TFT_d2] = "TFT_d2",
    [TFT_d3] = "TFT_d3",
    [TFT_d4] = "TFT_d4",
    [TFT_d5] = "TFT_d5",
    [TFT_d6] = "TFT_d6",
    [TFT_d7] = "TFT_d7",
    [TFT_pclk] = "TFT_pclk",
    [TFT_de] = "TFT_de",
    [TFT_d10] = "TFT_d10",
    [TFT_d11] = "TFT_d11",
    [TFT_d12] = "TFT_d12",
    [TFT_d13] = "TFT_d13",
    [TFT_d14] = "TFT_d14",
    [TFT_d15] = "TFT_d15",
    [TFT_vsync] = "TFT_vsync",
    [TFT_hsync] = "TFT_hsync",
    [TFT_d18] = "TFT_d18",
    [TFT_d19] = "TFT_d19",
    [TFT_d20] = "TFT_d20",
    [TFT_d21] = "TFT_d21",
    [TFT_d22] = "TFT_d22",
    [TFT_d23] = "TFT_d23",
};

static int tft_request_pins(unsigned int pins)
{
    int i;

    for (i = 0; i < 32; i++) {
        if (pins & (1 << i)) {
            int ret = gpio_request(GPIO_PD(i), tft_names[i]);
            if (ret) {
                printk(KERN_ERR "jzfb: failed to request GPIO_PA%d\n", i);
                jzfb_release_pins();
                return ret;
            }
            m_pins |= (1 << i);
        }
    }

    return 0;
}

static int tft_init_gpio(int r, int g, int b)
{
    unsigned int pins =
        bit_field_mask(TFT_d7 - b + 1, TFT_d7) |
        bit_field_mask(TFT_d15 - g + 1, TFT_d15) |
        bit_field_mask(TFT_d23 - r + 1, TFT_d23) |
        BIT(TFT_pclk) | BIT(TFT_de) | BIT(TFT_vsync) | BIT(TFT_hsync);

    int ret = tft_request_pins(pins);
    if (ret)
        return ret;

    gpio_port_set_func(GPIO_PORT_D, pins, GPIO_FUNC_0);

    return 0;
}

static int tft_serial_init_gpio(void)
{
    unsigned int pins =
        bit_field_mask(TFT_d2, TFT_d7) |
        bit_field_mask(TFT_d10, TFT_d11) |
        BIT(TFT_pclk) | BIT(TFT_de) | BIT(TFT_vsync) | BIT(TFT_hsync);

    int ret = tft_request_pins(pins);
    if (ret)
        return ret;

    gpio_port_set_func(GPIO_PORT_D, pins, GPIO_FUNC_0);

    return 0;
}

#define SLCD_d0 2
#define SLCD_d1 3
#define SLCD_d2 4
#define SLCD_d3 5
#define SLCD_d4 6
#define SLCD_d5 7
#define SLCD_WR 9

#define SLCD_d6 12
#define SLCD_d7 13
#define SLCD_d8 14
#define SLCD_d9 15
#define SLCD_d10 16
#define SLCD_d11 17
#define SLCD_cs 18
#define SLCD_te 19

#define SLCD_d12 22
#define SLCD_d13 23
#define SLCD_d14 24
#define SLCD_d15 25
#define SLCD_rdy 26
#define SLCD_dc  27

static char slcd_names[][9] = {
    [SLCD_d0] = "SLCD_d0",
    [SLCD_d1] = "SLCD_d1",
    [SLCD_d2] = "SLCD_d2",
    [SLCD_d3] = "SLCD_d3",
    [SLCD_d4] = "SLCD_d4",
    [SLCD_d5] = "SLCD_d5",
    [SLCD_WR] = "SLCD_WR",
    [SLCD_d6] = "SLCD_d6",
    [SLCD_d7] = "SLCD_d7",
    [SLCD_d8] = "SLCD_d8",
    [SLCD_d9] = "SLCD_d9",
    [SLCD_d10] = "SLCD_d10",
    [SLCD_d11] = "SLCD_d11",
    [SLCD_cs] = "SLCD_cs",
    [SLCD_te] = "SLCD_te",
    [SLCD_d12] = "SLCD_d12",
    [SLCD_d13] = "SLCD_d13",
    [SLCD_d14] = "SLCD_d14",
    [SLCD_d15] = "SLCD_d15",
    [SLCD_rdy] = "SLCD_rdy",
    [SLCD_dc] = "SLCD_dc",
};

static int slcd_request_pins(unsigned int pins)
{
    int i;

    for (i = 0; i < 32; i++) {
        if (pins & (1 << i)) {
            int ret = gpio_request(GPIO_PD(i), slcd_names[i]);
            if (ret) {
                printk(KERN_ERR "jzfb: failed to request GPIO_PA%d\n", i);
                jzfb_release_pins();
                return ret;
            }
            m_pins |= (1 << i);
        }
    }

    return 0;
}

static int slcd_init_gpio(int pins, int use_rdy, int use_te, int use_cs)
{
    if (use_rdy)
        pins |= BIT(SLCD_rdy);

    if (use_te)
        pins |= BIT(SLCD_te);

    if (use_cs)
        pins |= BIT(SLCD_cs);

    pins |= BIT(SLCD_WR);
    pins |= BIT(SLCD_dc);

    int ret = slcd_request_pins(pins);
    if (ret)
        return ret;

    gpio_port_set_func(GPIO_PORT_D, pins, GPIO_FUNC_1);

    return 0;
}

static int slcd_init_gpio_data8(int use_rdy, int use_te, int use_cs)
{
    unsigned int pins = bit_field_mask(SLCD_d0, SLCD_d5) |
                        bit_field_mask(SLCD_d6, SLCD_d7);

    return slcd_init_gpio(pins, use_rdy, use_te, use_cs);
}

static int slcd_init_gpio_data9(int use_rdy, int use_te, int use_cs)
{
    unsigned int pins = bit_field_mask(SLCD_d0, SLCD_d5) |
                        bit_field_mask(SLCD_d6, SLCD_d8);

    return slcd_init_gpio(pins, use_rdy, use_te, use_cs);
}

static int slcd_init_gpio_data16(int use_rdy, int use_te, int use_cs)
{
    unsigned int pins = bit_field_mask(SLCD_d0, SLCD_d5) |
                        bit_field_mask(SLCD_d6, SLCD_d11) |
                        bit_field_mask(SLCD_d12, SLCD_d15);

    return slcd_init_gpio(pins, use_rdy, use_te, use_cs);
}