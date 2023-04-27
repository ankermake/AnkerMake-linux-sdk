#ifndef _ROTATOR_H_
#define _ROTATOR_H_

enum rotator_fmt {
    ROTATOR_RGB555,
    ROTATOR_ARGB1555,
    ROTATOR_RGB565,
    ROTATOR_RGB888,
    ROTATOR_ARGB8888,//default
    ROTATOR_Y8,
    ROTATOR_YUV422,
};

enum rotator_convert_order {
    ROTATOR_ORDER_RGB_TO_RGB,//default
    ROTATOR_ORDER_RBG_TO_RGB,
    ROTATOR_ORDER_GRB_TO_RGB,
    ROTATOR_ORDER_GBR_TO_RGB,
    ROTATOR_ORDER_BRG_TO_RGB,
    ROTATOR_ORDER_BGR_TO_RGB,
};

enum rotator_mirror {
    ROTATOR_NO_MIRROR,
    ROTATOR_MIRROR,
};

enum rotator_angle {
    ROTATOR_ANGLE_0,
    ROTATOR_ANGLE_90,
    ROTATOR_ANGLE_180,
    ROTATOR_ANGLE_270,
};

struct rotator_config_data {
    void *src_buf;
    void *dst_buf;

    unsigned int frame_height;//像素点
    unsigned int frame_width;//像素点

    unsigned int src_stride;//像素点偏移数
    unsigned int dst_stride;//像素点偏移数

    enum rotator_fmt src_fmt;
    enum rotator_convert_order convert_order;
    enum rotator_fmt dst_fmt;

    enum rotator_mirror horizontal_mirror;
    enum rotator_mirror vertical_mirror;
    enum rotator_angle rotate_angle;
};

int rotator_bytes_per_pixel(enum rotator_fmt fmt);

int rotator_complete_conversion(struct rotator_config_data *config);

#endif