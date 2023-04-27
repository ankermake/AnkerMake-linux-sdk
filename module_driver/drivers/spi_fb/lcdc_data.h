#ifndef _SPI_FB_H_
#define _SPI_FB_H_

enum fb_fmt {
    fb_fmt_RGB555,
    fb_fmt_RGB565,
    fb_fmt_RGB888,
    fb_fmt_ARGB8888,
};

struct lcdc;

struct lcdc_data {
    const char *name;
    unsigned int refresh;
    unsigned int xres;
    unsigned int yres;
    unsigned int pixclock;
    unsigned int left_margin;
    unsigned int right_margin;
    unsigned int upper_margin;
    unsigned int lower_margin;
    unsigned int hsync_len;
    unsigned int vsync_len;

    enum fb_fmt fb_format;

    int height; // height of picture in mm
    int width;  // width of picture in mm

    unsigned int cmd_of_start_frame;
    int (*power_on)(struct lcdc *lcdc);
    int (*power_off)(struct lcdc *lcdc);
    void (*write_fb_data)(unsigned char *addr);
};

int spi_fb_register_lcd(struct lcdc_data *data);

void spi_fb_unregister_lcd(struct lcdc_data *data);

#endif