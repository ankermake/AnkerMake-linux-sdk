#include <linux/i2c.h>
#include <linux/init.h>
#include <linux/gpio.h>
#include <board.h>

struct sma1301_platform_data {
    unsigned int init_vol;
    bool stereo_select;
    bool sck_pll_enable;
    const uint32_t *reg_array;
    uint32_t reg_array_len;
};

static struct sma1301_platform_data sma1301_platform_data = {
    .init_vol = 0x30,
    .stereo_select = false,
    .sck_pll_enable = false,
};

static struct i2c_board_info sma1301_bd_info = {
    I2C_BOARD_INFO("sma1301", SMA1301_ES7241_I2C_ADDR),
        .platform_data = (void *)&sma1301_platform_data,
};

static int __init sma1301_bd_info_init(void)
{
	printk("sma1301_bd_info_init\n");

	int ret = 0;
    if (gpio_is_valid(SMA1301_ES7241_POWER_GPIO)) {
        ret = gpio_request_one(SMA1301_ES7241_POWER_GPIO,
                SMA1301_ES7241_POWER_ON_LEVEL ? GPIOF_OUT_INIT_HIGH : GPIOF_OUT_INIT_LOW,
                "sma1301_es7241_power");
        if (ret < 0) {
            printk("request sma1301_es7241_power gpio (err %d)\n", ret);
            return ret;
        }
    }

    i2c_register_board_info(SMA1301_ES7241_I2CBUS_NUM, &sma1301_bd_info, 1);

    return 0;
}
arch_initcall(sma1301_bd_info_init);
