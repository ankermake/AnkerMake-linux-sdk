#include <linux/platform_device.h>
#include <linux/i2c.h>
#include <linux/i2c-gpio.h>
#include <linux/interrupt.h>
#include "board_base.h"
#include <linux/device.h>
#include <linux/module.h>
#include <linux/kernel.h>
#include <mach/jzsnd.h>
#include <soc/gpio.h>
#include <linux/jz_dwc.h>

#ifdef CONFIG_EEPROM_AT24
#include <asm-generic/sizes.h>
#include <linux/i2c/at24.h>
#include <mach/platform.h>
#endif

#define SZ_16K	0x00004000

//extern struct platform_device jz_i2c2_device;

#if defined(CONFIG_TOUCHSCREEN_FT6X06)
 #include <linux/input/ft6x06_ts.h>
static struct ft6x06_platform_data ft6x06_tsc_pdata = {
	.x_max          = 0,
	.y_max          = 0,
	.va_x_max	= 320,
	.va_y_max	= 480,
	.irqflags = IRQF_TRIGGER_FALLING|IRQF_DISABLED,
	.irq = GPIO_TP_INT,
	.reset = GPIO_TP_RESET,
};
#endif

#ifdef CONFIG_EEPROM_AT24
static struct at24_platform_data at24c16 = {
	.byte_len = SZ_16K / 8,
	.page_size = 16,

};
#endif
#ifdef CONFIG_WM8594_CODEC_V12
static struct snd_codec_data wm8594_codec_pdata = {
	.codec_sys_clk = 1200000,
};
#endif


#ifdef CONFIG_BATTERY_CW2015
#include <linux/power/cw2015_battery.h>

/* 1800mA */
/* static unsigned char config_info[SIZE_BATINFO] = { */
/* 	0x15, 0x6A, 0x68, 0x63, 0x5E, */
/* 	0x5C, 0x51, 0x4E, 0x4B, 0x49, */
/* 	0x47, 0x47, 0x42, 0x41, 0x3A, */
/* 	0x22, 0x10, 0x09, 0x08, 0x0C, */
/* 	0x15, 0x36, 0x55, 0x6F, 0x86, */
/* 	0x74, 0x0C, 0xCD, 0x21, 0x41, */
/* 	0x4E, 0x53, 0x56, 0x5D, 0x68, */
/* 	0x6C, 0x3C, 0x0E, 0x73, 0x07, */
/* 	0x00, 0x3D, 0x52, 0x87, 0x8F, */
/* 	0x91, 0x94, 0x52, 0x82, 0x8C, */
/* 	0x92, 0x96, 0x5D, 0x61, 0x87, */
/* 	0xCB, 0x2F, 0x7D, 0x72, 0xA5, */
/* 	0xB5, 0xC1, 0xAE, 0x11 */
/* }; */

/* 3000mA */
static unsigned char config_info[SIZE_BATINFO] = {
	0x15, 0x81, 0x67, 0x5C, 0x45, 0x48,
	0x32, 0x42, 0x52, 0x40, 0x4D, 0x4F,
	0x41, 0x35, 0x31, 0x30, 0x2F, 0x30,
	0x32, 0x39, 0x3C, 0x3D, 0x42, 0x47,
	0x1C, 0x81, 0x0B, 0x85, 0x24, 0x45,
	0x6C, 0x7E, 0x89, 0x84, 0x84, 0x83,
	0x44, 0x1C, 0x55, 0x2D, 0x25, 0x35,
	0x52, 0x87, 0x8F, 0x91, 0x94, 0x52,
	0x82, 0x8C, 0x92, 0x96, 0x85, 0xA2,
	0xB7, 0xCB, 0x2F, 0x7D, 0x72, 0xA5,
	0xB5, 0xC1, 0xAE, 0x19
};

static struct cw_bat_platform_data cw_bat_platdata = {
	.bat_low_pin = BATTERY_LOW_PIN,
	.bat_low_func = BATTERY_LOW_FUNC,
	.chg_ok_pin = BATTERY_CHARGE_STATUS,
	.chg_ok_func = BATTERY_CHARGE_FUNC,
	.chg_iset_pin = BATTERY_ISET_PIN,
	.chg_iset_func = BATTERY_ISET_FUNC, /* HIGN Maximum charge current is 500mA, LOW Maximum charge current 1.5A */
	.usb_dete_pin = GPIO_USB_DETE,
	.usb_dete_level = GPIO_USB_DETE_LEVEL,
	.is_usb_charge = 1,
	.is_dc_charge = 0,
	.cw_bat_config_info = config_info,
};
#endif

#if (defined(CONFIG_SOFT_I2C0_GPIO_V12_JZ) || defined(CONFIG_I2C0_V12_JZ))
struct i2c_board_info jz_i2c0_devs[] __initdata = {
#ifdef CONFIG_SENSORS_BMA2X2
	{
		I2C_BOARD_INFO("bma2x2", 0x18),
		.irq = GPIO_GSENSOR_INTR,
	},
#endif
#ifdef CONFIG_BATTERY_CW2015
	{
		I2C_BOARD_INFO("cw201x", 0x62),
		.platform_data = &cw_bat_platdata,
	},
#endif
};
int jz_i2c0_devs_size = ARRAY_SIZE(jz_i2c0_devs);

struct i2c_board_info jz_v4l2_camera_devs[] __initdata = {
#ifdef  CONFIG_SOC_CAMERA_OV7725
	[FRONT_CAMERA_INDEX] = {
		I2C_BOARD_INFO("ov772x_fornt", 0x21),
	},
#endif
#ifdef CONFIG_SOC_CAMERA_OV5640
	[FRONT_CAMERA_INDEX] = {
		I2C_BOARD_INFO("ov5640-front", 0x3c),
	},
#endif
#ifdef CONFIG_SOC_CAMERA_GC0308
	[FRONT_CAMERA_INDEX] = {
		I2C_BOARD_INFO("gc0308", 0x21),
	},
#endif
#ifdef CONFIG_SOC_CAMERA_GC2155
	[FRONT_CAMERA_INDEX] = {
		I2C_BOARD_INFO("gc2155", 0x3c),
	},
#endif
};
int jz_v4l2_devs_size = ARRAY_SIZE(jz_v4l2_camera_devs);
#endif

#if (defined(CONFIG_SOFT_I2C2_GPIO_V12_JZ) || defined(CONFIG_I2C2_V12_JZ))
struct i2c_board_info jz_i2c2_devs[] __initdata = {
#ifdef CONFIG_EEPROM_AT24
	{
		I2C_BOARD_INFO("at24",0x57),
		.platform_data  = &at24c16,
	},
#endif
#ifdef CONFIG_WM8594_CODEC_V12
	{
		I2C_BOARD_INFO("wm8594", 0x1a),
		.platform_data  = &wm8594_codec_pdata,
	},
#endif
#if defined(CONFIG_TOUCHSCREEN_FT6X06)
	{
		I2C_BOARD_INFO(FT6X06_NAME, 0x38),
		.platform_data = &ft6x06_tsc_pdata,
	},
#endif
};
#endif

#if (defined(CONFIG_SOFT_I2C1_GPIO_V12_JZ) || defined(CONFIG_I2C1_V12_JZ))
struct i2c_board_info jz_i2c1_devs[] __initdata = {
#ifdef CONFIG_EEPROM_AT24
	{
		I2C_BOARD_INFO("at24",0x57),
		.platform_data  = &at24c16,
	},
#endif
#ifdef CONFIG_WM8594_CODEC_V12
	{
		I2C_BOARD_INFO("wm8594", 0x1a),
		.platform_data  = &wm8594_codec_pdata,
	},
#endif
};
#endif


#if     defined(CONFIG_SOFT_I2C2_GPIO_V12_JZ) || defined(CONFIG_I2C2_V12_JZ)
int jz_i2c2_devs_size = ARRAY_SIZE(jz_i2c2_devs);
#endif

#if     defined(CONFIG_SOFT_I2C1_GPIO_V12_JZ) || defined(CONFIG_I2C1_V12_JZ)
int jz_i2c1_devs_size = ARRAY_SIZE(jz_i2c1_devs);
#endif


#ifdef CONFIG_EEPROM_AT24

struct i2c_client *at24_client;

static int  at24_dev_init(void)
{
	struct i2c_adapter *i2c_adap;


#if     defined(CONFIG_SOFT_I2C2_GPIO_V12_JZ) || defined(CONFIG_I2C2_V12_JZ)
	i2c_adap = i2c_get_adapter(2);
	at24_client = i2c_new_device(i2c_adap, jz_i2c2_devs);
#endif
#if     defined(CONFIG_SOFT_I2C1_GPIO_V12_JZ) || defined(CONFIG_I2C1_V12_JZ)
	i2c_adap = i2c_get_adapter(1);
	at24_client = i2c_new_device(i2c_adap, jz_i2c1_devs);
#endif
	i2c_put_adapter(i2c_adap);

	return 0;
}


static void  at24_dev_exit(void)
{
	 i2c_unregister_device(at24_client);
}



module_init(at24_dev_init);

module_exit(at24_dev_exit);


MODULE_LICENSE("GPL");
#endif
#ifdef CONFIG_I2C_GPIO
#define DEF_GPIO_I2C(NO)                        \
    static struct i2c_gpio_platform_data i2c##NO##_gpio_data = {    \
        .sda_pin    = GPIO_I2C##NO##_SDA,           \
        .scl_pin    = GPIO_I2C##NO##_SCK,           \
     };                              \
    struct platform_device i2c##NO##_gpio_device = {        \
        .name   = "i2c-gpio",                   \
        .id = NO,                       \
        .dev    = { .platform_data = &i2c##NO##_gpio_data,},    \
    };
#ifdef CONFIG_SOFT_I2C1_GPIO_V12_JZ
DEF_GPIO_I2C(1);
#endif
#ifdef CONFIG_SOFT_I2C0_GPIO_V12_JZ
DEF_GPIO_I2C(0);
#endif
#endif /*CONFIG_I2C_GPIO*/
