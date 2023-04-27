#include <linux/gpio_keys.h>
#include <board.h>
#include <uapi/linux/input.h>
#include <linux/kernel.h>

struct gpio_keys_button board_buttons[] = {
	{
		.gpio		= GPIO_HOME,
		.code   	= KEY_HOME,
		.desc		= "home key",
		.active_low	= ACTIVE_LOW_HOME,	/*ap configure &  mic mute*/
		.type		= EV_KEY,
	},
	{
		.gpio           = GPIO_UP,
		.code           = KEY_UP,
		.desc           = "up key",
		.active_low     = ACTIVE_LOW_UP,	/*pause & ota update*/
		.type		= EV_KEY,
	},
	{
		.gpio           = GPIO_LEFT,
		.code           = KEY_LEFT,
		.desc           = "volume down",
		.active_low     = ACTIVE_LOW_LEFT,
		.type		= EV_KEY,
	},
	{
		.gpio           = GPIO_RIGHT,
		.code           = KEY_RIGHT,
		.desc           = "volume up",
		.active_low     = ACTIVE_LOW_RIGHT,
		.type		= EV_KEY,
	},
};


struct gpio_keys_platform_data board_button_data = {
	.buttons	= board_buttons,
	.nbuttons	= ARRAY_SIZE(board_buttons),
};
