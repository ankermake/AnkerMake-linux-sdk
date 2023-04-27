#include <linux/platform_device.h>
#include <linux/gpio_keys.h>
#include <linux/input.h>
#include <linux/input/matrix_keypad.h>
#include "board_base.h"

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
