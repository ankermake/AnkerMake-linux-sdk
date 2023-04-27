#ifndef _JZFB_PDATA_H_
#define _JZFB_PDATA_H_

#include <hal_x1021/x1021_slcd_hal.h>
#include <linux/gpio.h>

struct jzfb;

enum smart_config_type {
	SMART_CONFIG_DATA,
	SMART_CONFIG_PRM,
	SMART_CONFIG_CMD,
	SMART_CONFIG_UDELAY,
};

struct smart_lcd_data_table {
	enum smart_config_type type;
	unsigned int value;
};

/* 旋转的方向是顺时针 */
enum jzfb_copy_type {
    FB_COPY_TYPE_NONE,
    FB_COPY_TYPE_ROTATE_0,
    FB_COPY_TYPE_ROTATE_180,
    FB_COPY_TYPE_ROTATE_90,
    FB_COPY_TYPE_ROTATE_270,
    FB_COPY_TYPE_ERR,
};

struct jzfb_lcd_pdata {
    struct jzfb_config_data config;
    int height; // height of picture in mm
    int width;  // width of picture in mm
    int override_te_gpio;
    enum jzfb_copy_type fb_copy_type;
    int init_lcd_when_start:1;
    int refresh_lcd_when_resume:1;
    int wait_frame_end_when_pan_display:1;
    unsigned int cmd_of_start_frame;
    struct smart_lcd_data_table *slcd_data_table;
    unsigned int slcd_data_table_length;
	void (*init_slcd_gpio)(struct jzfb *jzfb);
    int (*power_on)(struct jzfb *jzfb);
    int (*power_off)(struct jzfb *jzfb);
};

#endif /* _JZFB_PDATA_H_ */