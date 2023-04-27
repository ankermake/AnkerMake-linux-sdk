
#ifndef __INGENIC_MEDIA_BUS_FORMAT_H
#define __INGENIC_MEDIA_BUS_FORMAT_H

/* RGB -  is  0x1118-0x111b */
#define INGENIC_MEDIA_BUS_FMT_RBG565_2X8_LE		0x1118
#define INGENIC_MEDIA_BUS_FMT_BRG565_2X8_LE		0x1119
#define INGENIC_MEDIA_BUS_FMT_GRB565_2X8_LE		0x111a
#define INGENIC_MEDIA_BUS_FMT_GBR565_2X8_LE		0x111b


#include <uapi/linux/videodev2.h>

#define INGENIC_V4L2_PIX_FMT_RBG565  v4l2_fourcc('R', 'B', 'G', 'P') /* 16  RBG-5-6-5     */
#define INGENIC_V4L2_PIX_FMT_BGR565  v4l2_fourcc('B', 'G', 'R', 'P') /* 16  BGR-5-6-5     */
#define INGENIC_V4L2_PIX_FMT_BRG565  v4l2_fourcc('B', 'R', 'G', 'P') /* 16  BRG-5-6-5     */
#define INGENIC_V4L2_PIX_FMT_GRB565  v4l2_fourcc('G', 'R', 'B', 'P') /* 16  GRB-5-6-5     */
#define INGENIC_V4L2_PIX_FMT_GBR565  v4l2_fourcc('G', 'B', 'R', 'P') /* 16  GBR-5-6-5     */

#endif
