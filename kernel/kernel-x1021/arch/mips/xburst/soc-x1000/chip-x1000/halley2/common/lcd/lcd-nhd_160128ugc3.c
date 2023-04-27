#include <linux/init.h>
#include <linux/platform_device.h>
#include <linux/mm.h>
#include <linux/console.h>
#include <linux/delay.h>
#include <linux/gpio.h>
#include <linux/pwm_backlight.h>
#include <linux/lcd.h>
#include <linux/regulator/consumer.h>
#include <mach/jzfb.h>
#include "../board_base.h"

#ifdef CONFIG_SLCD_NHD_18BIT
static int slcd_inited = 1;
#else
static int slcd_inited = 0;
#endif


struct nhd_160128ugc_power{
	struct regulator *vlcdio;
	struct regulator *vlcdvcc;
	int inited;
};

static struct nhd_160128ugc_power lcd_power = {
	NULL,
	NULL,
	0
};

int nhd_160128ugc_power_init(struct lcd_device *ld)
{
	int ret;
	printk("======nhd_160128ugc_power_init=============\n");
	if(GPIO_LCD_RST > 0){
		ret = gpio_request(GPIO_LCD_RST,"lcd rst");
		if (ret) {
			printk(KERN_ERR "can's request lcd rst\n");
			return ret;
		}
	}
	if(GPIO_LCD_CS > 0) {
		ret = gpio_request(GPIO_LCD_CS,"lcd cd");
		if (ret) {
			printk(KERN_ERR "can's request lcd cs\n");
			return ret;
		}
	}
	if(GPIO_LCD_RD > 0){
		ret = gpio_request(GPIO_LCD_RD,"lcd rd");
		if (ret) {
			printk(KERN_ERR "can's request lcd rd\n");
			return ret;
		}
	}
	printk("set lcd_power.inited ========1\n");
	lcd_power.inited = 1;
	return 0;
}

int nhd_160128ugc_power_reset(struct lcd_device *ld)
{
	if (!lcd_power.inited)
		return -EFAULT;
	gpio_direction_output(GPIO_LCD_RST,0);
	msleep(10);
	gpio_direction_output(GPIO_LCD_RST,1);
	msleep(10);
		return 0;
}

int nhd_160128ugc_power_on(struct lcd_device *ld,int enable)
{
	if (!lcd_power.inited && nhd_160128ugc_power_init(ld))
		return -EFAULT;
	if (enable) {
		gpio_direction_output(GPIO_LCD_CS,0);
		gpio_direction_output(GPIO_LCD_RD,1);
		nhd_160128ugc_power_reset(ld);
	} else {
		mdelay(5);
		gpio_direction_output(GPIO_LCD_RD,0);
		gpio_direction_output(GPIO_LCD_RST,0);
		gpio_direction_output(GPIO_LCD_CS,1);
		slcd_inited = 0;
	}
	return 0;
}

struct lcd_platform_data nhd_160128ugc_pdata = {
	.reset = nhd_160128ugc_power_reset,
	.power_on = nhd_160128ugc_power_on,
};

struct platform_device nhd_160128ugc_device = {
	.name = "nhd_160128ugc_slcd",
	.dev = {
		.platform_data = &nhd_160128ugc_pdata,
	},
};
#if 0
static struct smart_lcd_data_table nhd_160128ugc_data_table1[] = {
	//{SMART_CONFIG_CMD,0x0001},{SMART_CONFIG_DATA,0x0001},{SMART_CONFIG_UDELAY,10000},
	{SMART_CONFIG_CMD,0x04},{SMART_CONFIG_DATA,0x03},{SMART_CONFIG_UDELAY,2000},
	{SMART_CONFIG_CMD,0x04},{SMART_CONFIG_DATA,0x00},{SMART_CONFIG_UDELAY,2000},
	{SMART_CONFIG_CMD,0x3b},{SMART_CONFIG_DATA,0x00},
	{SMART_CONFIG_CMD,0x02},{SMART_CONFIG_DATA,0x01},
	{SMART_CONFIG_CMD,0x03},{SMART_CONFIG_DATA,0x90},
	{SMART_CONFIG_CMD,0x80},{SMART_CONFIG_DATA,0x01},
	{SMART_CONFIG_CMD,0x08},{SMART_CONFIG_DATA,0x04},
	{SMART_CONFIG_CMD,0x0a},{SMART_CONFIG_DATA,0x05},
	{SMART_CONFIG_CMD,0x0b},{SMART_CONFIG_DATA,0x9d},
	{SMART_CONFIG_CMD,0x0c},{SMART_CONFIG_DATA,0x8c},
	{SMART_CONFIG_CMD,0x0d},{SMART_CONFIG_DATA,0x57},
	{SMART_CONFIG_CMD,0x10},{SMART_CONFIG_DATA,0x56},
	{SMART_CONFIG_CMD,0x11},{SMART_CONFIG_DATA,0x4d},
	{SMART_CONFIG_CMD,0x12},{SMART_CONFIG_DATA,0x46},
	{SMART_CONFIG_CMD,0x13},{SMART_CONFIG_DATA,0xa0},
	{SMART_CONFIG_CMD,0x14},{SMART_CONFIG_DATA,0x01},
	{SMART_CONFIG_CMD,0x16},{SMART_CONFIG_DATA,0x76},
	{SMART_CONFIG_CMD,0x20},{SMART_CONFIG_DATA,0x00},
	{SMART_CONFIG_CMD,0x21},{SMART_CONFIG_DATA,0x00},
	{SMART_CONFIG_CMD,0x28},{SMART_CONFIG_DATA,0x7f},
	{SMART_CONFIG_CMD,0x29},{SMART_CONFIG_DATA,0x00},
	{SMART_CONFIG_CMD,0x06},{SMART_CONFIG_DATA,0x01},
	{SMART_CONFIG_CMD,0x05},{SMART_CONFIG_DATA,0x00},
	{SMART_CONFIG_CMD,0x15},{SMART_CONFIG_DATA,0x00},
};
#endif

#if 1
static struct smart_lcd_data_table nhd_160128ugc_data_table[] = {
	//{SMART_CONFIG_CMD,0x0001},{SMART_CONFIG_DATA,0x0001},{SMART_CONFIG_UDELAY,10000},
	{SMART_CONFIG_CMD,0x02},{SMART_CONFIG_DATA,0x01},
	{SMART_CONFIG_CMD,0x04},{SMART_CONFIG_DATA,0x01},
	{SMART_CONFIG_CMD,0x04},{SMART_CONFIG_DATA,0x00}, {SMART_CONFIG_UDELAY,10000},
	{SMART_CONFIG_CMD,0x03},{SMART_CONFIG_DATA,0x30},
	{SMART_CONFIG_CMD,0x80},{SMART_CONFIG_DATA,0x00},
	{SMART_CONFIG_CMD,0x06},{SMART_CONFIG_DATA,0x00},
	{SMART_CONFIG_CMD,0x08},{SMART_CONFIG_DATA,0x01},
	{SMART_CONFIG_CMD,0x09},{SMART_CONFIG_DATA,0x01},
	{SMART_CONFIG_CMD,0x0a},{SMART_CONFIG_DATA,0x01},
	{SMART_CONFIG_CMD,0x0b},{SMART_CONFIG_DATA,0x0a},
	{SMART_CONFIG_CMD,0x0c},{SMART_CONFIG_DATA,0x0a},
	{SMART_CONFIG_CMD,0x0d},{SMART_CONFIG_DATA,0x0a},
	{SMART_CONFIG_CMD,0x10},{SMART_CONFIG_DATA,0x52},
	{SMART_CONFIG_CMD,0x11},{SMART_CONFIG_DATA,0x38},
	{SMART_CONFIG_CMD,0x12},{SMART_CONFIG_DATA,0x3a},
	{SMART_CONFIG_CMD,0x13},{SMART_CONFIG_DATA,0x00},
	{SMART_CONFIG_CMD,0x14},{SMART_CONFIG_DATA,0x01},
	/* {SMART_CONFIG_CMD,0x16},{SMART_CONFIG_DATA,0x56},   //for 9bit */
	{SMART_CONFIG_CMD,0x16},{SMART_CONFIG_DATA,0x66},   //for 8bit
	{SMART_CONFIG_CMD,0x17},{SMART_CONFIG_DATA,0x00},
	{SMART_CONFIG_CMD,0x18},{SMART_CONFIG_DATA,0x9f},
	{SMART_CONFIG_CMD,0x19},{SMART_CONFIG_DATA,0x00},
	{SMART_CONFIG_CMD,0x1a},{SMART_CONFIG_DATA,0x7f},
	{SMART_CONFIG_CMD,0x20},{SMART_CONFIG_DATA,0x00},
	{SMART_CONFIG_CMD,0x21},{SMART_CONFIG_DATA,0x00},
	{SMART_CONFIG_CMD,0x28},{SMART_CONFIG_DATA,0x7f},
	{SMART_CONFIG_CMD,0x29},{SMART_CONFIG_DATA,0x00},
	{SMART_CONFIG_CMD,0x2e},{SMART_CONFIG_DATA,0x00},
	{SMART_CONFIG_CMD,0x2f},{SMART_CONFIG_DATA,0x00},
	{SMART_CONFIG_CMD,0x33},{SMART_CONFIG_DATA,0x00},
	{SMART_CONFIG_CMD,0x34},{SMART_CONFIG_DATA,0x9f},
	{SMART_CONFIG_CMD,0x35},{SMART_CONFIG_DATA,0x00},
	{SMART_CONFIG_CMD,0x36},{SMART_CONFIG_DATA,0x7f},
	{SMART_CONFIG_CMD,0x06},{SMART_CONFIG_DATA,0x01},
};
#endif
unsigned long nhd_cmd_buf[]= {
	/* 0x00440044,		// 0x22 << 1, for 9bit. */
	0x22222222,		// for 8bit.
};

struct fb_videomode jzfb0_videomode = {
	.name = "160x128",
	.refresh = 60,
	.xres = 160,
	.yres = 128,
	.pixclock = KHZ2PICOS(30000),
	.left_margin = 0,
	.right_margin = 0,
	.upper_margin = 0,
	.lower_margin = 0,
	.hsync_len = 0,
	.vsync_len = 0,
	.sync = FB_SYNC_HOR_HIGH_ACT | FB_SYNC_VERT_HIGH_ACT,
	.vmode = FB_VMODE_NONINTERLACED,
	.flag = 0,
};
struct jzfb_platform_data jzfb_pdata = {
	.num_modes = 1,
	.modes = &jzfb0_videomode,
	.lcd_type = LCD_TYPE_SLCD,
	.bpp    = 16,
	.width = 31,
	.height = 31,
	.pinmd  = 0,

	.smart_config.rsply_cmd_high       = 0,
	.smart_config.csply_active_high    = 0,
	.smart_config.newcfg_fmt_conv =  1,
	/* .smart_config.newcfg_cmd_9bit =  1, */
	/* write graphic ram command, in word, for example 8-bit bus, write_gram_cmd=C3C2C1C0. */
	.smart_config.write_gram_cmd = nhd_cmd_buf,
	.smart_config.length_cmd = ARRAY_SIZE(nhd_cmd_buf),
	.smart_config.bus_width = 8,
	.smart_config.length_data_table =  ARRAY_SIZE(nhd_160128ugc_data_table),
	.smart_config.data_table = nhd_160128ugc_data_table,
	.dither_enable = 0,
};

