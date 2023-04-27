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
