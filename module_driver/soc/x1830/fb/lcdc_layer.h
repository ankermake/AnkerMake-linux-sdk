#ifndef _LCDC_LAYER_H_
#define _LCDC_LAYER_H_

#include "lcdc_data.h"

enum lcdc_layer_order {
    lcdc_layer_top,
    lcdc_layer_bottom,
};

struct lcdc_layer {
    enum fb_fmt fb_fmt;

    unsigned int xres;
    unsigned int yres;

    unsigned int xpos;
    unsigned int ypos;

    enum lcdc_layer_order layer_order;
    int layer_enable;

    struct {
        void *mem;
        unsigned int stride; // 单位： 字节
    } rgb;

    struct {
        void *mem;
        unsigned int stride; // 单位： 字节
    } y;

    struct {
        void *mem;
        unsigned int stride; // 单位： 字节
    } uv;

    struct {
        unsigned char enable;
        unsigned char value;
    } alpha;
};

/*
 * raw layer api
 */
void lcdc_config_layer(unsigned int frame_index,
     unsigned int layer_id, struct lcdc_layer *cfg);

void lcdc_set_layer_order(unsigned int frame_index,
     unsigned int layer_id, enum lcdc_layer_order layer_order);

void lcdc_enable_layer(unsigned int frame_index,
     unsigned int layer_id, int enable);

void lcdc_pan_display(unsigned int frame_index);

void lcdc_layer_server_update(
     unsigned int layer_id, struct lcdc_layer *cfg, int sync);

void lcdc_layer_server_disable(unsigned int layer_id, int sync);

void lcdc_layer_server_init(void);
void lcdc_layer_server_exit(void);

#endif /* _LCDC_LAYER_H_ */
