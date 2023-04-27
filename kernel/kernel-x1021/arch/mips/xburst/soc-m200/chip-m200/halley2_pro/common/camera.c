#include <mach/camera.h>
#include <linux/i2c.h>
#include <linux/gpio.h>
#include <linux/delay.h>
#include "board_base.h"

int temp = 1;


#if defined(CONFIG_SOC_CAMERA_OV2710)
static int ov2710_power(int onoff)
{
	if(temp) {
		gpio_request(CAMERA_PWDN_N, "CAMERA_PWDN_N");
		gpio_request(CAMERA_PW_EN, "CAMERA_PW_EN");
		temp = 0;
	}
	if (onoff) { /* conflict with USB_ID pin */
		gpio_direction_output(CAMERA_PW_EN, 1);
		mdelay(10);
		gpio_direction_output(CAMERA_PWDN_N, 1);
		mdelay(10); /* this is necesary */
		gpio_direction_output(CAMERA_PWDN_N, 0);
		;
	} else {
		gpio_direction_output(CAMERA_PWDN_N, 0);
		gpio_direction_output(CAMERA_PWDN_N, 1);
		gpio_direction_output(CAMERA_PW_EN, 0);
		mdelay(10);
	}

	return 0;
}

static int ov2710_reset(void)
{
	/*reset*/
	return 0;
}

static struct i2c_board_info ov2710_board_info = {
	.type = "ov2710",
	.addr = 0x36,
};
#endif /* CONFIG_SOC_CAMERA_OV2710 */


#if defined(CONFIG_SOC_CAMERA_XC6130)
static int xc6130_power(int onoff)
{
	printk("\033[31m(l:%d, f:%s, F:%s) %d %s\033[0m\n", __LINE__, __func__, __FILE__, 0, "");
	if(temp) {
		gpio_request(CAMERA_RST, "CAMERA_RST");
		/* gpio_request(CAMERA_PWDN_N, "CAMERA_PWDN_N"); */
		gpio_request(CAMERA_PW_EN, "CAMERA_PW_EN");
		temp = 0;
	}
	if (onoff) {
		gpio_direction_output(CAMERA_RST, 0);
		gpio_direction_output(CAMERA_PW_EN, 1);
		mdelay(1000);
		gpio_direction_output(CAMERA_RST, 1);
		/* mdelay(100); */
		/* gpio_direction_output(CAMERA_RST, 1); */
		/* mdelay(100); */
	} else {
		/* gpio_direction_output(CAMERA_RST, 0); */
		/* mdelay(10); */
		/* gpio_direction_output(CAMERA_PW_EN, 0); */
	}

	return 0;
}

static int xc6130_reset(void)
{
	printk("\033[31m(l:%d, f:%s, F:%s) %d %s\033[0m\n", __LINE__, __func__, __FILE__, 0, "");
	return 0;
}

static struct i2c_board_info xc6130_board_info = {
	.type = "xc6130",
	.addr = 0x1b,
};
#endif /* CONFIG_SOC_CAMERA_XC6130 */

static struct ovisp_camera_client ovisp_camera_clients[] = {

#ifdef CONFIG_SOC_CAMERA_OV2710
	{
		.board_info = &ov2710_board_info,
		.flags = CAMERA_CLIENT_IF_MIPI,
		.mclk_rate = 26000000,
		.max_video_width = 1280,
		.max_video_height = 720,
		.power = ov2710_power,
		.reset = ov2710_reset,
	},
#endif /* CONFIG_SOC_CAMERA_OV2710 */
#ifdef CONFIG_SOC_CAMERA_XC6130
	{
		.board_info = &xc6130_board_info,
		.flags = CAMERA_CLIENT_IF_MIPI | CAMERA_CLIENT_ISP_BYPASS | CAMERA_CLIENT_CLK_EXT,
		.mclk_name = "cgu_cim",
		.mclk_rate = 24000000,
		.max_video_width = 1920,
		.max_video_height = 1080,
		.power = xc6130_power,
		.reset = xc6130_reset,
	},
#endif /* CONFIG_SOC_CAMERA_XC6130 */
};

struct ovisp_camera_platform_data ovisp_camera_info = {
#ifdef CONFIG_OVISP_I2C
	.i2c_adapter_id = 4, /* larger than host i2c nums */
	.flags = CAMERA_USE_ISP_I2C | CAMERA_USE_HIGH_BYTE
	| CAMERA_I2C_PIO_MODE | CAMERA_I2C_STANDARD_SPEED,
#else
	.i2c_adapter_id = 3, /* use cpu's i2c adapter */
	.flags = CAMERA_USE_HIGH_BYTE
	| CAMERA_I2C_PIO_MODE | CAMERA_I2C_STANDARD_SPEED,
#endif
	.client = ovisp_camera_clients,
	.client_num = ARRAY_SIZE(ovisp_camera_clients),
};
