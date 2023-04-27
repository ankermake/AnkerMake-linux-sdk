#include <linux/gpio_keys.h>
#include <board.h>
#include <uapi/linux/input.h>
#include <linux/kernel.h>

struct gpio_keys_button board_buttons[] = {
     {
        .gpio           = GPIO_HOME,
        .code           = KEY_HOME,
        .desc           = "ok key",
        .active_low     = ACTIVE_LOW_HOME,/*ap configure &  mic mute*/
        .type           = EV_KEY,
    },
    {
        .gpio           = GPIO_UP,
        .code           = KEY_UP,
        .desc           = "up key",
        .active_low     = ACTIVE_LOW_UP,/*pause & ota update*/
        .type           = EV_KEY,
    },
    {
        .gpio           = GPIO_LEFT,
        .code           = KEY_LEFT,
        .desc           = "left key",
        .active_low     = ACTIVE_LOW_LEFT,
        .type           = EV_KEY,
    },
    {
        .gpio           = GPIO_RIGHT,
        .code           = KEY_RIGHT,
        .desc           = "rignt key",
        .active_low     = ACTIVE_LOW_RIGHT,
        .type           = EV_KEY,

    },
    {
        .gpio           = GPIO_MENU,
        .code           = KEY_MENU,
        .desc           = "menu key",
        .active_low     = ACTIVE_LOW_MENU,
        .type           = EV_KEY,
        .gpio_pullup    =  1,
    },
    {
        .gpio           = GPIO_DOWN,
        .code           = KEY_DOWN,
        .desc           = "down key",
        .active_low     = ACTIVE_LOW_DOWN,
        .type           = EV_KEY,
    },
    {
        .gpio           = GPIO_PIR,
        .code           = KEY_SYSRQ,
        .desc           = "pir key",
        .active_low     = ACTIVE_LOW_PIR,
        .type           = EV_KEY,
    },
};


struct gpio_keys_platform_data board_button_data = {
    .buttons		= board_buttons,
    .nbuttons		= ARRAY_SIZE(board_buttons),
};
