#include <linux/gpio_keys.h>
#include <board.h>
#include <uapi/linux/input.h>
#include <linux/kernel.h>



struct gpio_keys_button board_buttons[] = {
#ifdef GPIO_PIR
    {
        .gpio           = GPIO_PIR,
        .code           = KEY_SYSRQ,
        .desc           = "pir key",
        .active_low     = ACTIVE_LOW_PIR,
        .type           = EV_KEY,
    },
#endif
#ifdef GPIO_WKUP
    {
        .gpio           = GPIO_WKUP,
        .code           = KEY_WAKEUP,
        .desc           = "key wakeup",
        .active_low     = ACTIVE_LOW_WKUP,
        .type           = EV_KEY,
        .wakeup         = 1,
    },
#endif
#ifdef GPIO_KEY_UP
    {
        .gpio           = GPIO_KEY_UP,
        .code           = KEY_UP,
        .desc           = "key up",
        .active_low     = ACTIVE_LOW_PIR,
        .type           = EV_KEY,
    },
#endif
#ifdef GPIO_KEY_DOWN
    {
        .gpio           = GPIO_KEY_DOWN,
        .code           = KEY_DOWN,
        .desc           = "key down",
        .active_low     = ACTIVE_LOW_PIR,
        .type           = EV_KEY,
    },
#endif
#ifdef GPIO_KEY_LEFT
    {
        .gpio           = GPIO_KEY_LEFT,
        .code           = KEY_LEFT,
        .desc           = "key left",
        .active_low     = ACTIVE_LOW_PIR,
        .type           = EV_KEY,
    },
#endif
#ifdef GPIO_KEY_RIGHT
    {
        .gpio           = GPIO_KEY_RIGHT,
        .code           = KEY_RIGHT,
        .desc           = "key right",
        .active_low     = ACTIVE_LOW_PIR,
        .type           = EV_KEY,
    },
#endif
#ifdef GPIO_KEY_HOME
    {
        .gpio           = GPIO_KEY_HOME,
        .code           = KEY_HOME,
        .desc           = "key home",
        .active_low     = ACTIVE_LOW_PIR,
        .type           = EV_KEY,
    },
#endif
#ifdef GPIO_KEY_BACK
    {
        .gpio           = GPIO_KEY_BACK,
        .code           = KEY_BACK,
        .desc           = "key back",
        .active_low     = ACTIVE_LOW_PIR,
        .type           = EV_KEY,
    },
#endif
};


struct gpio_keys_platform_data board_button_data = {
    .buttons		= board_buttons,
    .nbuttons		= ARRAY_SIZE(board_buttons),
};



#ifdef CONFIG_KEYBOARD_JZ_ADC0
struct gpio_keys_button adc0_key_info[] = {
    {
        .code           = KEY_UP,
        .value          = 0,       //测量电压值(mv)
    },
    {
        .code           = KEY_DOWN,
        .value          = 300,
    },
    {
        .code           = KEY_LEFT,
        .value          = 600,
    },
    {
        .code           = KEY_RIGHT,
        .value          = 900,
    },
    {
        .code           = KEY_MENU,
        .value          = 1200,
    },
    {
        .code           = KEY_HOME,
        .value          = 1500,
    },
    {
        .code           = -1,   //注意以-1结束本组有效键值
        .value          = 1800, //不按按键时的adc值
    },
};
#endif

#ifdef CONFIG_KEYBOARD_JZ_ADC1
struct gpio_keys_button adc1_key_info[] = {
    {
        .code           = -1,   //注意以-1结束本组有效键值
        .value          = 1800, //不按按键时的adc值
    },
};
#endif

#ifdef CONFIG_KEYBOARD_JZ_ADC2
struct gpio_keys_button adc2_key_info[] = {
    {
        .code           = -1,   //注意以-1结束本组有效键值
        .value          = 1800, //不按按键时的adc值
    },
};
#endif

#ifdef CONFIG_KEYBOARD_JZ_ADC3
struct gpio_keys_button adc3_key_info[] = {
    {
        .code           = -1,   //注意以-1结束本组有效键值
        .value          = 1800, //不按按键时的adc值
    },
};
#endif

