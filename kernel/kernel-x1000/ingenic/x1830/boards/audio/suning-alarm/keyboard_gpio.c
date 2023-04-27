#include <linux/gpio_keys.h>
#include <board.h>
#include <uapi/linux/input.h>
#include <linux/kernel.h>

struct gpio_keys_button board_buttons[] = {
	{
		.gpio		= GPIO_VOLUMEDOWN,
		.code   	= KEY_VOLUMEDOWN,
		.desc		= "volume down",
		.active_low	= ACTIVE_LOW_VOLUMEDOWN,	/*ap configure &  mic mute*/
		.type		= EV_KEY,
	},
	{
		.gpio           = GPIO_VOLUMEUP,
		.code           = KEY_VOLUMEUP,
		.desc           = "volume up",
		.active_low     = ACTIVE_LOW_VOLUMEUP,	/*pause & ota update*/
		.type		= EV_KEY,
	},
	{
		.gpio           = GPIO_MICMUTE,
		.code           = KEY_MICMUTE,
		.desc           = "mic mute",
		.active_low     = ACTIVE_LOW_MICMUTE,
		.type		= EV_KEY,
	},
	{
		.gpio           = GPIO_PLAY,
		.code           = KEY_PLAY,
		.desc           = "play",
		.active_low     = ACTIVE_LOW_PLAY,
		.type		= EV_KEY,
	},
	{
		.gpio           = GPIO_WLAN,
		.code           = KEY_WLAN,
		.desc           = "wlan",
		.active_low     = ACTIVE_LOW_WLAN,
		.type		= EV_KEY,
	},
	{
		.gpio           = GPIO_BLUETOOTH,
		.code           = KEY_BLUETOOTH,
		.desc           = "bluetooth",
		.active_low     = ACTIVE_LOW_BLUETOOTH,
		.type		= EV_KEY,
	},
};


struct gpio_keys_platform_data board_button_data = {
	.buttons	= board_buttons,
	.nbuttons	= ARRAY_SIZE(board_buttons),
};
