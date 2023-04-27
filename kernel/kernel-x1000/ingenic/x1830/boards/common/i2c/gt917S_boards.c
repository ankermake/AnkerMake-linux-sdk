#include <gt917S_boards.h>
#include <board.h>

static struct gt917S_platform_data gt917S_data = {
    .gpio_shutdown  = GTP_RST_PORT,
    .gpio_int       = GTP_INT_PORT,
    .i2c_num        = GT917S_I2CBUS_NUM,
    .x              = GTP_MAX_WIDTH,
    .y              = GTP_MAX_HEIGHT,
};

static struct i2c_board_info gt917S_bd_info = {
    I2C_BOARD_INFO("Goodix-TS", 0x14),
    .platform_data = (void *)&gt917S_data,
};

static int __init gt917S_bd_info_init(void){
    i2c_register_board_info(GT917S_I2CBUS_NUM, &gt917S_bd_info, 1);
    return 0;
}
arch_initcall(gt917S_bd_info_init);
