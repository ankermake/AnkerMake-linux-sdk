#ifndef _SOC_MSCALER_1830_H__
#define _SOC_MSCALER_1830_H__

/*
 * 31    23    15    7    0
 *     A     R    G    B
 * MSCALER_FORMAT_BGRA_8888
 * 当输出格式为BRGA格式时，每四个字节组成一个像素点，每个像素点的组成
 * 关系为字节0：B，1：G，2：R，3：A
*/

enum mscaler_fmt{
    MSCALER_FORMAT_NV12         = 0,
    MSCALER_FORMAT_NV21         = 1,

    MSCALER_FORMAT_BGRA_8888    = (0<<2)+2,
    MSCALER_FORMAT_GBRA_8888    = (1<<2)+2,
    MSCALER_FORMAT_RBGA_8888    = (2<<2)+2,
    MSCALER_FORMAT_BRGA_8888    = (3<<2)+2,
    MSCALER_FORMAT_GRBA_8888    = (4<<2)+2,
    MSCALER_FORMAT_RGBA_8888    = (5<<2)+2,

    MSCALER_FORMAT_ABGR_8888    = (8<<2)+2,
    MSCALER_FORMAT_AGBR_8888    = (9<<2)+2,
    MSCALER_FORMAT_ARBG_8888    = (10<<2)+2,
    MSCALER_FORMAT_ABRG_8888    = (11<<2)+2,
    MSCALER_FORMAT_AGRB_8888    = (12<<2)+2,
    MSCALER_FORMAT_ARGB_8888    = (13<<2)+2,

    MSCALER_FORMAT_BGR_565      = (0<<2)+3,
    MSCALER_FORMAT_GBR_565      = (1<<2)+3,
    MSCALER_FORMAT_RBG_565      = (2<<2)+3,
    MSCALER_FORMAT_BRG_565      = (3<<2)+3,
    MSCALER_FORMAT_GRB_565      = (4<<2)+3,
    MSCALER_FORMAT_RGB_565      = (5<<2)+3,
};

struct mscaler_frame_part
{
    /* 用户空间虚拟地址 */
    void                       *mem;

    /* 分量大小 cache_line对齐 */
    unsigned int                mem_size;

    /* 实际物理地址,必须 cache_line 对齐       */
    unsigned long               phys_addr;

    /* 行字节对齐要求，必须同时满足MSCALER_ALIGN对齐 */
    unsigned int                stride;
};

struct mscaler_frame
{
    /* 输入图像格式只支持 nv12/nv21 输出图像格式全支持mscaler_fmt */
    enum mscaler_fmt            fmt;

    /* 图像宽 */
    unsigned int                xres;

    /* 图像高 */
    unsigned int                yres;

    struct mscaler_frame_part   y;

    struct mscaler_frame_part   uv;
};

struct mscaler_param
{
    struct mscaler_frame        *src;

    struct mscaler_frame        *dst;
};

struct mscaler_info
{
    void                        *phys_addr;

    unsigned long               alloc_size;

    unsigned long               alloc_alignsize;
};

#define JZMSCALER_IOC_MAGIC  'M'
#define IOCTL_MSCALER_CONVERT            _IOW(JZMSCALER_IOC_MAGIC, 100, struct mscaler_param)
#define IOCTL_MSCALER_ALIGN_SIZE         _IOR(JZMSCALER_IOC_MAGIC, 101, int)
#define IOCTL_L1CACHE_ALIGN_SIZE         _IOR(JZMSCALER_IOC_MAGIC, 102, int)
#define IOCTL_MSCALER_ALLOC_ALIGN_SIZE   _IOWR(JZMSCALER_IOC_MAGIC, 103, struct mscaler_info)
#define IOCTL_MSCALER_FREE_ALIGN_SIZE   _IO(JZMSCALER_IOC_MAGIC, 104)

#endif
