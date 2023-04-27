#ifndef __VFB_H
#define __VFB_H


#include <linux/fb.h>
//#define CONFIG_THREE_FRAME_BUFFERS
#define CONFIG_TWO_FRAME_BUFFERS

#define VFB_XRES CONFIG_VFB_XRES
#define VFB_YRES CONFIG_VFB_YRES
#define VFB_BPP	 CONFIG_VFB_BPP
#ifdef CONFIG_TWO_FRAME_BUFFERS
#define NUM_FRAME_BUFFERS 2
#endif
#ifdef CONFIG_THREE_FRAME_BUFFERS
#define NUM_FRAME_BUFFERS 3
#endif

#define PIXEL_ALIGN 4

/* Reserved for future extend */
#define JZFB_SET_VSYNCINT	_IOW('F', 0x210, int)

#define JZFB_GET_LCDTYPE        _IOR('F', 0x122, int)

//extern int ingenicvfb_register_panel(struct jzfb_platform_data *panel);
int ingenic_oled_update_video_buffer(struct sfc_flash * flash, char *video_buf1, int buf_size1);

enum jzfb_format_order {
	FORMAT_X8R8G8B8 = 1,
	FORMAT_X8B8G8R8,
};

struct jzfb {
	struct fb_info *fb;
	struct device *dev;
	struct jzfb_platform_data *pdata;

	size_t vidmem_size;
	void *vidmem;
	dma_addr_t vidmem_phys;

	int current_buffer;

	enum jzfb_format_order fmt_order;	/* frame buffer pixel format order */
};

struct jzfb_platform_data {
	size_t num_modes;
	struct fb_videomode *modes;
	unsigned int bpp;
	unsigned int width;
	unsigned int height;
};

struct lcd_panel_ops {
	void (*init)(void *panel);
	void (*enable)(void *panel);
	void (*disable)(void *panel);
};
enum tft_lcd_color_even {
	TFT_LCD_COLOR_EVEN_RGB,
	TFT_LCD_COLOR_EVEN_RBG,
	TFT_LCD_COLOR_EVEN_BGR,
	TFT_LCD_COLOR_EVEN_BRG,
	TFT_LCD_COLOR_EVEN_GBR,
	TFT_LCD_COLOR_EVEN_GRB,
};

enum tft_lcd_color_odd {
	TFT_LCD_COLOR_ODD_RGB,
	TFT_LCD_COLOR_ODD_RBG,
	TFT_LCD_COLOR_ODD_BGR,
	TFT_LCD_COLOR_ODD_BRG,
	TFT_LCD_COLOR_ODD_GBR,
	TFT_LCD_COLOR_ODD_GRB,
};

enum tft_lcd_mode {
	TFT_LCD_MODE_PARALLEL_888,
	TFT_LCD_MODE_PARALLEL_666,
	TFT_LCD_MODE_PARALLEL_565,
	TFT_LCD_MODE_SERIAL_RGB,
	TFT_LCD_MODE_SERIAL_RGBD,
};

struct tft_config {
	unsigned int pix_clk_inv:1;
	unsigned int de_dl:1;
	unsigned int sync_dl:1;
	enum tft_lcd_color_even color_even;
	enum tft_lcd_color_odd color_odd;
	enum tft_lcd_mode mode;
};

#endif /* __VFB_H */
