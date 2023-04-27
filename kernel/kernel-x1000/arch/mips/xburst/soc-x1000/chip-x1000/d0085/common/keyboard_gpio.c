#include <linux/platform_device.h>

#include <linux/gpio_keys.h>
#include <linux/input.h>
#include "board_base.h"

struct gpio_keys_button __attribute__((weak)) board_buttons[] = {
#ifdef GPIO_HOME_KEY
	{
		.gpio		= GPIO_HOME_KEY,
		.code   	= KEY_HOME,
		.desc		= "home key",
		.active_low	= ACTIVE_LOW_HOME,
	},
#endif
#ifdef GPIO_MENU_KEY
	{
		.gpio		= GPIO_MENU_KEY,
		.code   	= KEY_MENU,
		.desc		= "menu key",
		.active_low	= ACTIVE_LOW_MENU,
	},
#endif
#ifdef GPIO_BACK_KEY
	{
		.gpio		= GPIO_BACK_KEY,
		.code   	= KEY_BACK,
		.desc		= "back key",
		.active_low	= ACTIVE_LOW_BACK,
	},
#endif
#ifdef GPIO_VOLUMEDOWN_KEY
	{
		.gpio		= GPIO_VOLUMEDOWN_KEY,
		.code   	= KEY_VOLUMEDOWN,
		.desc		= "volum down key",
		.active_low	= ACTIVE_LOW_VOLUMEDOWN,
		.gpio_pullup 	= GPIO_PULLUP_VOLUMEDOWN,
	},
#endif
#ifdef GPIO_VOLUMEUP_KEY
	{
		.gpio		= GPIO_VOLUMEUP_KEY,
		.code   	= KEY_VOLUMEUP,
		.desc		= "volum up key",
		.active_low	= ACTIVE_LOW_VOLUMEUP,
		.gpio_pullup 	= GPIO_PULLUP_VOLUMEUP,
	},
#endif
#ifdef GPIO_MODE_KEY
	{
		.gpio		= GPIO_MODE_KEY,
		.code   	= KEY_MODE,
		.desc		= "mode key",
		.active_low	= ACTIVE_LOW_MODE,
		.gpio_pullup 	= GPIO_PULLUP_MODE,
	},
#endif
#ifdef GPIO_MUTE_KEY
	{
		.gpio		= GPIO_MUTE_KEY,
		.code   	= KEY_MUTE,
		.desc		= "mute key",
		.active_low	= ACTIVE_LOW_MUTE,
		.gpio_pullup 	= GPIO_PULLUP_MUTE,
	},
#endif
#ifdef GPIO_WAKEUP_KEY
	{
		.gpio           = GPIO_WAKEUP_KEY,
		.code           = KEY_POWER,
		.desc           = "end call key",
		.active_low     = ACTIVE_LOW_WAKEUP,
		.wakeup         = 1,
	},
#endif
};

static struct gpio_keys_platform_data board_button_data = {
	.buttons	= board_buttons,
	.nbuttons	= ARRAY_SIZE(board_buttons),
};

struct platform_device jz_button_device = {
	.name		= "gpio-keys",
	.id		= -1,
	.num_resources	= 0,
	.dev		= {
                .platform_data	= &board_button_data,
	}
};
