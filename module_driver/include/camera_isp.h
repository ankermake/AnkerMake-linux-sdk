#ifndef _CAMERA_ISP_H_
#define _CAMERA_ISP_H_

#include <camera.h>

/* 裁剪 */
struct channel_crop {
    int enable;
    unsigned int top;   /* 起始坐标 */
    unsigned int left;
    unsigned int width;
    unsigned int height;
};

/* 缩放 */
struct channel_scaler {
    int enable;
    unsigned int width;
    unsigned int height;
};

struct frame_image_format {
    unsigned int width;             /* ISP MScaler 输出分宽 */
    unsigned int height;            /* ISP MScaler 输出分高 */
    unsigned int pixel_format;      /* ISP MScaler 输出格式 */
    unsigned int frame_size;        /* ISP MScaler 帧大小 */

    struct channel_scaler scaler;   /* ISP MScaler 缩放属性 */
    struct channel_crop crop;       /* ISP MScaler 裁剪属性 */

    int frame_nums;                 /* ISP MScaler 缓存个数 */
};


#endif /* _CAMERA_ISP_H_ */
