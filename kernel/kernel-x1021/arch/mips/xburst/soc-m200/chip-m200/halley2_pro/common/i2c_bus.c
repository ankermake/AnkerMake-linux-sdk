#include <linux/platform_device.h>
#include <linux/i2c.h>
#include <linux/i2c-gpio.h>
#include <linux/interrupt.h>
#include "board_base.h"
#include <uapi/linux/input.h>

/* *****************************touchscreen******************************* */
#ifdef CONFIG_TOUCHSCREEN_GT9XX
#include <linux/input/gt9xx_ts.h>
static struct gt9xx_platform_data gt9xx_tsc_pdata = {
	.x_max          = 720,
	.y_max          = 1280,
	.va_x_max	= 720,
	.va_y_max	= 1280,
	/* .irqflags = IRQF_TRIGGER_FALLING|IRQF_DISABLED, */
	.irq = GPIO_TP_INT,
	.reset = GPIO_TP_RESET,
};
#endif

#ifdef CONFIG_TOUCHSCREEN_CYPRESS_CYTTSP5
#include <linux/cyttsp5_core.h>
#include <linux/cyttsp5_platform.h>

#define CYTTSP5_USE_I2C

#ifdef CYTTSP5_USE_I2C
#define CYTTSP5_I2C_TCH_ADR 0x24
#define CYTTSP5_LDR_TCH_ADR 0x24
#endif

#define CYTTSP5_HID_DESC_REGISTER 1

#define CY_VKEYS_X 720
#define CY_VKEYS_Y 1280
#define CY_MAXX 880
#define CY_MAXY 1280
#define CY_MINX 0
#define CY_MINY 0

#define CY_ABS_MIN_X CY_MINX
#define CY_ABS_MIN_Y CY_MINY
#define CY_ABS_MAX_X CY_MAXX
#define CY_ABS_MAX_Y CY_MAXY
#define CY_ABS_MIN_P 0
#define CY_ABS_MAX_P 255
#define CY_ABS_MIN_W 0
#define CY_ABS_MAX_W 255
#define CY_PROXIMITY_MIN_VAL	0
#define CY_PROXIMITY_MAX_VAL	1

#define CY_ABS_MIN_T 0

#define CY_ABS_MAX_T 15

/* Button to keycode conversion */
static u16 cyttsp5_btn_keys[] = {
	/* use this table to map buttons to keycodes (see input.h) */
	KEY_HOMEPAGE,		/* 172 */ /* Previously was KEY_HOME (102) */
				/* New Android versions use KEY_HOMEPAGE */
	KEY_MENU,		/* 139 */
	KEY_BACK,		/* 158 */
	KEY_SEARCH,		/* 217 */
	KEY_VOLUMEDOWN,		/* 114 */
	KEY_VOLUMEUP,		/* 115 */
	KEY_CAMERA,		/* 212 */
	KEY_POWER		/* 116 */
};

static struct touch_settings cyttsp5_sett_btn_keys = {
	.data = (uint8_t *)&cyttsp5_btn_keys[0],
	.size = ARRAY_SIZE(cyttsp5_btn_keys),
	.tag = 0,
};

static struct cyttsp5_core_platform_data _cyttsp5_core_platform_data = {
	.irq_gpio = CYTTSP5_I2C_IRQ_GPIO,
	.rst_gpio = CYTTSP5_I2C_RST_GPIO,
	.hid_desc_register = CYTTSP5_HID_DESC_REGISTER,
	.xres = cyttsp5_xres,
	.init = cyttsp5_init,
	.power = cyttsp5_power,
	.detect = cyttsp5_detect,
	.irq_stat = cyttsp5_irq_stat,
	.sett = {
		NULL,	/* Reserved */
		NULL,	/* Command Registers */
		NULL,	/* Touch Report */
		NULL,	/* Parade Data Record */
		NULL,	/* Test Record */
		NULL,	/* Panel Configuration Record */
		NULL,	/* &cyttsp5_sett_param_regs, */
		NULL,	/* &cyttsp5_sett_param_size, */
		NULL,	/* Reserved */
		NULL,	/* Reserved */
		NULL,	/* Operational Configuration Record */
		NULL, /* &cyttsp5_sett_ddata, *//* Design Data Record */
		NULL, /* &cyttsp5_sett_mdata, *//* Manufacturing Data Record */
		NULL,	/* Config and Test Registers */
		&cyttsp5_sett_btn_keys,	/* button-to-keycode table */
	},
	.flags = CY_CORE_FLAG_RESTORE_PARAMETERS,
	.easy_wakeup_gesture = CY_CORE_EWG_NONE,
};

static const int16_t cyttsp5_abs[] = {
	ABS_MT_POSITION_X, CY_ABS_MIN_X, CY_ABS_MAX_X, 0, 0,
	ABS_MT_POSITION_Y, CY_ABS_MIN_Y, CY_ABS_MAX_Y, 0, 0,
	ABS_MT_PRESSURE, CY_ABS_MIN_P, CY_ABS_MAX_P, 0, 0,
	CY_IGNORE_VALUE, CY_ABS_MIN_W, CY_ABS_MAX_W, 0, 0,
	ABS_MT_TRACKING_ID, CY_ABS_MIN_T, CY_ABS_MAX_T, 0, 0,
	ABS_MT_TOUCH_MAJOR, 0, 255, 0, 0,
	ABS_MT_TOUCH_MINOR, 0, 255, 0, 0,
	ABS_MT_ORIENTATION, -127, 127, 0, 0,
	ABS_MT_TOOL_TYPE, 0, MT_TOOL_MAX, 0, 0,
	ABS_MT_DISTANCE, 0, 255, 0, 0,	/* Used with hover */
};

struct touch_framework cyttsp5_framework = {
	.abs = (uint16_t *)&cyttsp5_abs[0],
	.size = ARRAY_SIZE(cyttsp5_abs),
	.enable_vkeys = 0,
};

static struct cyttsp5_mt_platform_data _cyttsp5_mt_platform_data = {
	.frmwrk = &cyttsp5_framework,
    .flags = CY_MT_FLAG_FLIP | CY_MT_FLAG_INV_X,
	.inp_dev_name = CYTTSP5_MT_NAME,
	.vkeys_x = CY_VKEYS_X,
	.vkeys_y = CY_VKEYS_Y,
};

static struct cyttsp5_btn_platform_data _cyttsp5_btn_platform_data = {
	.inp_dev_name = CYTTSP5_BTN_NAME,
};

static const int16_t cyttsp5_prox_abs[] = {
	ABS_DISTANCE, CY_PROXIMITY_MIN_VAL, CY_PROXIMITY_MAX_VAL, 0, 0,
};

struct touch_framework cyttsp5_prox_framework = {
	.abs = (uint16_t *)&cyttsp5_prox_abs[0],
	.size = ARRAY_SIZE(cyttsp5_prox_abs),
};

static struct cyttsp5_proximity_platform_data
		_cyttsp5_proximity_platform_data = {
	.frmwrk = &cyttsp5_prox_framework,
	.inp_dev_name = CYTTSP5_PROXIMITY_NAME,
};

static struct cyttsp5_platform_data _cyttsp5_platform_data = {
	.core_pdata = &_cyttsp5_core_platform_data,
	.mt_pdata = &_cyttsp5_mt_platform_data,
	.loader_pdata = &_cyttsp5_loader_platform_data,
	.btn_pdata = &_cyttsp5_btn_platform_data,
	.prox_pdata = &_cyttsp5_proximity_platform_data,
};

#if 0
static ssize_t cyttsp5_virtualkeys_show(struct kobject *kobj,
		struct kobj_attribute *attr, char *buf)
{
	return sprintf(buf,
		__stringify(EV_KEY) ":"
		__stringify(KEY_BACK) ":1360:90:160:180:"
		__stringify(EV_KEY) ":"
		__stringify(KEY_MENU) ":1360:270:160:180:"
		__stringify(EV_KEY) ":"
		__stringify(KEY_HOMEPAGE) ":1360:450:160:180:"
		__stringify(EV_KEY) ":"
		__stringify(KEY_SEARCH) ":1360:630:160:180\n");
}

static struct kobj_attribute cyttsp5_virtualkeys_attr = {
	.attr = {
		.name = "virtualkeys.cyttsp5_mt",
		.mode = S_IRUGO,
	},
	.show = &cyttsp5_virtualkeys_show,
};

static struct attribute *cyttsp5_properties_attrs[] = {
	&cyttsp5_virtualkeys_attr.attr,
	NULL
};

static struct attribute_group cyttsp5_properties_attr_group = {
	.attrs = cyttsp5_properties_attrs,
};
#endif
#endif
/* *****************************touchscreen end*************************** */

/* *****************************light sensor****************************** */
#ifdef CONFIG_SENSORS_LTR_303ALS_01
#include <linux/platform_data/ltr_303.h>
struct ltr303_platform_data ltr_303als_01_pdata = {
	.pfd_gpio_int_no = GPIO_LIGHT_SENSOR_INT,
};
#endif
/* *****************************light sensor end************************** */

/* *****************************camera****************************** */

#if 0
	{
		.board_info = &xc6130_board_info,
		.flags = CAMERA_CLIENT_IF_MIPI,
		.mclk_rate = 26000000,
		.max_video_width = 1920,
		.max_video_height = 1080,
		.power = xc6130_power,
		.reset = xc6130_reset,
	},
#endif /* CONFIG_SOC_CAMERA_XC6130 */
/* *****************************camera end************************** */
#if (defined(CONFIG_SOFT_I2C0_GPIO_V12_JZ) || defined(CONFIG_I2C0_V12_JZ))
struct i2c_board_info jz_i2c0_devs[] __initdata = {
#ifdef CONFIG_TOUCHSCREEN_GT9XX
	{
		I2C_BOARD_INFO("Goodix-TS", 0x5d),
		.platform_data = &gt9xx_tsc_pdata,
	},
#endif

#ifdef CYTTSP5_USE_I2C
	{
		I2C_BOARD_INFO(CYTTSP5_I2C_NAME, CYTTSP5_I2C_TCH_ADR),
		.irq =  CYTTSP5_I2C_IRQ_GPIO,
		.platform_data = &_cyttsp5_platform_data,
	},
#endif

#if defined(CONFIG_BCM2079X_NFC)
	{
		I2C_BOARD_INFO("bcm2079x-i2c", 0x77),
		.platform_data = &bcm2079x_pdata,
	},
#endif /*CONFIG_BCM2079X_NFC*/
};

int jz_i2c0_devs_size = ARRAY_SIZE(jz_i2c0_devs);
#endif

#if (defined(CONFIG_SOFT_I2C1_GPIO_V12_JZ) || defined(CONFIG_I2C1_V12_JZ))
struct i2c_board_info jz_i2c1_devs[] __initdata = {
#ifdef CONFIG_SENSORS_LTR_303ALS_01
	{
		I2C_BOARD_INFO("ltr-303als-01", 0x29),
		.platform_data = &ltr_303als_01_pdata,
	},
#endif
};

int jz_i2c1_devs_size = ARRAY_SIZE(jz_i2c1_devs);
#endif

#if 0
#if (defined(CONFIG_SOFT_I2C3_GPIO_V12_JZ) || defined(CONFIG_I2C3_V12_JZ))
struct i2c_board_info jz_i2c3_devs[] __initdata = {
#ifdef CONFIG_SOC_CAMERA_XC6130
	{
		I2C_BOARD_INFO("xc6130", 0x36),
		.platform_data = &ltr_303als_01_pdata,
	},
#endif
};

int jz_i2c1_devs_size = ARRAY_SIZE(jz_i2c1_devs);
#endif
#endif

/*
 * define gpio i2c,if you use gpio i2c,
 * please enable gpio i2c and disable i2c controller
 */
#ifdef CONFIG_I2C_GPIO
#define DEF_GPIO_I2C(NO)						\
	static struct i2c_gpio_platform_data i2c##NO##_gpio_data = {	\
		.sda_pin	= GPIO_I2C##NO##_SDA,			\
		.scl_pin	= GPIO_I2C##NO##_SCK,			\
	};								\
	struct platform_device i2c##NO##_gpio_device = {		\
		.name	= "i2c-gpio",					\
		.id	= NO,						\
		.dev	= { .platform_data = &i2c##NO##_gpio_data,},	\
	};

#ifdef CONFIG_SOFT_I2C0_GPIO_V12_JZ
DEF_GPIO_I2C(0);
#endif
#ifdef CONFIG_SOFT_I2C1_GPIO_V12_JZ
DEF_GPIO_I2C(1);
#endif
#ifdef CONFIG_SOFT_I2C2_GPIO_V12_JZ
DEF_GPIO_I2C(2);
#endif
#ifdef CONFIG_SOFT_I2C3_GPIO_V12_JZ
DEF_GPIO_I2C(3);
#endif
#endif /*CONFIG_I2C_GPIO*/
