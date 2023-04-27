#include <linux/platform_device.h>
#include <linux/gpio_keys.h>
#include <linux/input.h>
#include "board_base.h"
#include <linux/input/matrix_keypad.h>

static uint32_t board_keymap[] =
{
#ifdef CONFIG_BOARD_X1000_CANNA_V10
	KEY(0,0, KEY_MODE),		/* AP/STA */
	KEY(0,1, KEY_F1),		/* AIRKISS */
	KEY(0,2, KEY_WAKEUP),
	KEY(1,0, KEY_VOLUMEDOWN),
	KEY(1,1, KEY_VOLUMEUP),
	KEY(1,2, KEY_PLAYPAUSE),
	KEY(2,0, KEY_PREVIOUSSONG),
	KEY(2,1, KEY_NEXTSONG),
	KEY(2,2, KEY_MENU),
	KEY(3,0, KEY_F3),		/* music shortcut key 1 */
	KEY(3,1, KEY_BLUETOOTH),
	KEY(3,2, KEY_RECORD),
#endif

#ifdef CONFIG_BOARD_X1000_CANNA_V23
	KEY(0,0, KEY_MODE),
	KEY(0,1, KEY_MENU),
	KEY(0,2, KEY_RECORD),
	KEY(1,0, KEY_VOLUMEDOWN),
	KEY(1,1, KEY_VOLUMEUP),
	KEY(1,2, KEY_PLAYPAUSE),
#endif

};

static struct matrix_keymap_data board_keymap_data =
{
	.keymap = board_keymap,
	.keymap_size =ARRAY_SIZE(board_keymap),
};

static struct matrix_keypad_platform_data board_keypad_data =
{
	.keymap_data = &board_keymap_data,
	.col_gpios = matrix_keypad_cols,
	.row_gpios = matrix_keypad_rows,
	.num_col_gpios = ARRAY_SIZE(matrix_keypad_cols),
	.num_row_gpios = ARRAY_SIZE(matrix_keypad_rows),
	.col_scan_delay_us = 10,
	.debounce_ms =100,
	.wakeup  = 1,
	.active_low  = 1,
	.no_autorepeat = 1,
};

struct platform_device jz_matrix_keypad_device = {
	.name		= "matrix-keypad",
	.id		= -1,
	.num_resources	= 0,
	.dev		= {
		.platform_data	= &board_keypad_data,
	}
};
