#include <linux/platform_device.h>

#include <linux/gpio_keys.h>
#include <linux/input.h>
#include "board_base.h"

extern struct gpio_keys_platform_data board_button_data;
struct gpio_keys_platform_data __attribute__((weak)) board_button_data = {
	.buttons    = NULL,
	.nbuttons    = 0,
};

struct platform_device jz_button_device = {
	.name        = "gpio-keys",
	.id        = -1,
	.num_resources    = 0,
	.dev        = {
		.platform_data = &board_button_data,
	}
};


#ifdef CONFIG_KEYBOARD_JZ_ADC0
extern struct gpio_keys_button adc0_key_info;
struct platform_device adc0_keys_dev = {
	.name          = "adc0_key",
	.id            = 0,
	.num_resources = 0,
	.dev           = {
		.platform_data = &adc0_key_info,
	}
};
#endif

#ifdef CONFIG_KEYBOARD_JZ_ADC1
extern struct gpio_keys_button adc1_key_info;
struct platform_device adc1_keys_dev = {
	.name          = "adc1_key",
	.id            = 1,
	.num_resources = 0,
	.dev           = {
		.platform_data = &adc1_key_info,
	}
};
#endif

#ifdef CONFIG_KEYBOARD_JZ_ADC2
extern struct gpio_keys_button adc2_key_info;
struct platform_device adc2_keys_dev = {
	.name          = "adc2_key",
	.id            = 2,
	.num_resources = 0,
	.dev           = {
		.platform_data = &adc2_key_info,
	}
};
#endif

#ifdef CONFIG_KEYBOARD_JZ_ADC3
extern struct gpio_keys_button adc3_key_info;
struct platform_device adc3_keys_dev = {
	.name          = "adc3_key",
	.id            = 3,
	.num_resources = 0,
	.dev           = {
		.platform_data = &adc3_key_info,
	}
};
#endif

