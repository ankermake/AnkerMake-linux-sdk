#ifndef _X1021_FB_LINUX_H_
#define _X1021_FB_LINUX_H_

#include <linux/sched.h>
#include <linux/fb.h>
#include <jz_fb_x1021/jzfb_pdata.h>

#define JZFB_MAX_OPEN_COUNT 10

struct jzfb {
    struct jzfb_lcd_pdata *pdata;
    struct clk *clk_lcd;
    struct clk *clk_lpc;
    struct clk *clk_ahb1;
    struct jzfb_frame_desc *frame_desc;
    struct jzfb_frame_desc *frame_end_desc;
    unsigned int frame_desc_phys;

    void *fb_mem;
    void *fb_mem_mapped;
    unsigned int line_stride;
    unsigned int bytes_per_pixel;
    unsigned int frame_size;
    unsigned int frame_nums;

    void *user_fb_mem;
    unsigned long user_fb_mem_phys;
    unsigned int user_line_stride;
    unsigned int user_bytes_per_pixel;
    unsigned int user_frame_size;
    unsigned int user_frame_nums;
    unsigned int user_xres;
    unsigned int user_yres;
    unsigned int user_width;
    unsigned int user_height;

    struct fb_videomode videomode;
    struct fb_info *fb;

    int irq;
    int te_irq;
    int te_gpio;
    struct device *dev;
    spinlock_t spinlock;
    struct mutex mutex;
    wait_queue_head_t wait_queue;

    int frame_count;
    int enable_debug;

    int gpio_en:1;
    int lcd_en:1;
    int lcdc_en:1;
    int clk_en:1;
    int te_en:1;
    int te_frame_is_sending:1;
    int te_level:1;
    int jzfb_en:1;
    int is_suspend:1;
    int pan_display_en:1;
    int stop_en:1;
    int frame_sending:1;
    int wait_frame_end_when_pan_display:1;

    enum jzfb_copy_type fb_copy_type;

    struct jzfb_user_data {
        pid_t tgid;
        void *map_addr;
        int count;
    } user_data[JZFB_MAX_OPEN_COUNT];
};

#endif /* _X1021_FB_LINUX_H_ */