#include <gpio.h>
__initdata int gpio_ss_table[][2] = {
	{GSS_TABLET_END,GSS_TABLET_END},
	{GPIO_PB(17), GSS_IGNORE},
	{GPIO_PB(30), GSS_IGNORE},
	{GPIO_PB(31), GSS_IGNORE},
};
