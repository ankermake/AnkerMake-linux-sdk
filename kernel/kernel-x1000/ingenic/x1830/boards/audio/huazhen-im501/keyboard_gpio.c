#include <linux/gpio_keys.h>
#include <board.h>
#include <uapi/linux/input.h>
#include <linux/kernel.h>

struct gpio_keys_button board_buttons[] = {
    {
        .gpio           = GPIO_HOME,
        .code           = KEY_HOME,
        .desc           = "home",
        .active_low     = ACTIVE_LOW_HOME,
        .type           = EV_KEY,
    },
    {
        .gpio           = GPIO_LEFT,
        .code           = KEY_LEFT,
        .desc           = "left",
        .active_low     = ACTIVE_LOW_LEFT,
        .type           = EV_KEY,
    },
    {
        .gpio           = GPIO_DOWN,
        .code           = KEY_DOWN,
        .desc           = "down",
        .active_low     = ACTIVE_LOW_DOWN,
        .type           = EV_KEY,
    },
    {
        .gpio           = GPIO_RIGHT,
        .code           = KEY_RIGHT,
        .desc           = "right",
        .active_low     = ACTIVE_LOW_RIGHT,
        .type           = EV_KEY,
    },
    {
        .gpio           = GPIO_UP,
        .code           = KEY_UP,
        .desc           = "up",
        .active_low     = ACTIVE_LOW_UP,
        .type           = EV_KEY,
    },
};


struct gpio_keys_platform_data board_button_data = {
    .buttons		= board_buttons,
    .nbuttons		= ARRAY_SIZE(board_buttons),
};
