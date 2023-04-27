#ifndef __FB_LAYER_MIXER_H_
#define __FB_LAYER_MIXER_H_

#include "soc/x2000/fb/lcdc_data.h"
struct fb_layer_mixer_dev;

struct fb_layer_mixer_output_cfg {
    int xres;
    int yres;
    enum fb_fmt format;
    void *dst_mem;
    void *dst_mem_virtual;
};

struct fb_layer_mixer_dev *fb_layer_mixer_create(struct device *dev);
void fb_layer_mixer_set_output_frame(struct fb_layer_mixer_dev *mixer, struct fb_layer_mixer_output_cfg *mixer_cfg);
void fb_layer_mixer_set_input_layer(struct fb_layer_mixer_dev *mixer, int layer_id, struct lcdc_layer *cfg);
void fb_layer_mixer_work_out_one_frame(struct fb_layer_mixer_dev *mixer);
void fb_layer_mixer_delete(struct fb_layer_mixer_dev *mixer);

#endif