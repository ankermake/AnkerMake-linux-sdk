#include <linux/platform_device.h>

#include <linux/gpio_keys.h>
#include <linux/input.h>
#include "board_base.h"

struct gpio_keys_button __attribute__((weak)) board_buttons[] = {
#ifdef GPIO_POWERDOWN
	{
		.gpio			= GPIO_POWERDOWN,
		.code   		= KEY_SLEEP,
		.desc			= "power down & wakeup",
		.type			= EV_KEY,
		.active_low		= ACTIVE_LOW_POWERDOWN,
		.debounce_interval	= 50,
		.wakeup			= 1,
	},
#endif
#ifdef GPIO_BOOT_SEL0
	{
		.gpio			= GPIO_BOOT_SEL0,
		.code   		= KEY_MODE,
		.desc			= "music Shortcut key 2",
		.type			= EV_KEY,
		.active_low		= ACTIVE_LOW_BOOT_SEL0,
		.debounce_interval	= 50,
	},
#endif
#ifdef GPIO_VOLUME_UP
	{
		.gpio			= GPIO_VOLUME_UP,
		.code   		= KEY_VOLUMEUP,
		.desc			= "music volume up",
		.type			= EV_KEY,
		.active_low             = ACTIVE_LOW_VOLUMEUP,
		.debounce_interval	= 50,
	},
#endif
#ifdef GPIO_VOLUME_DOWN
	{
		.gpio			= GPIO_VOLUME_DOWN,
		.code   		= KEY_VOLUMEDOWN,
		.desc			= "music volume down",
		.type			= EV_KEY,
		.active_low		= ACTIVE_LOW_VOLUMEDOWN,
		.debounce_interval	= 50,
	},
#endif
#ifdef GPIO_VOICE
	{
		.gpio			= GPIO_VOICE,
		.code   		= KEY_RECORD,
		.desc			= "stop recognition key",
		.type			= EV_KEY,
		.active_low		= ACTIVE_LOW_VOICE,
		.debounce_interval	= 50,
	},
#endif
#ifdef GPIO_PLAY_PAUSE
	{
		.gpio			= GPIO_PLAY_PAUSE,
		.code			= KEY_PLAYPAUSE,
		.desc			= "music play and pause",
		.type			= EV_KEY,
		.active_low		= ACTIVE_LOW_PLAYPAUSE,
		.debounce_interval	= 50,
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
