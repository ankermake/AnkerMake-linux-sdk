
#ifndef __JZ_CAMERA_H__
#define __JZ_CAMERA_H__

#include <media/soc_camera.h>
#include <linux/clk.h>
#include <linux/proc_fs.h>

#define MAX_BUFFER_NUM		    (5)
#define MAX_VIDEO_MEM		    (16 * 1024 * 1024)

/* define the maximum number of camera sensor that attach to cim controller */
#define MAX_SOC_CAM_NUM         	1
#define VERSION_CODE		KERNEL_VERSION(0, 0, 1)
#define VIC_ONLY_NAME "tx-isp"
#define MIPI_ONLY_NAME "tx-mipi"

/*
 * Structures
 */
struct jz_camera_dma_desc {
	dma_addr_t next;
	unsigned int id;
	unsigned int buf;
} __attribute__ ((aligned (32)));

/* buffer for one video frame */
struct jz_buffer {
	/* common v4l buffer stuff -- must be first */
	struct vb2_buffer vb2;
	struct list_head list;
};

struct jz_camera_dev {
	struct soc_camera_host soc_host;
	struct soc_camera_device *icd[MAX_SOC_CAM_NUM];
	struct vic_sensor_config* cfg;
    unsigned int uv_data_offset;
	int	sequence;
	int	dma_stopped;
	int	start_streaming_called;
	unsigned int buf_cnt;

	struct vb2_alloc_ctx		*alloc_ctx;
	struct jz_buffer *active;
	struct list_head video_buffer_list;

	struct resource *vic_res;

	struct clk *mclk;
	struct clk *isp_clk;
	struct clk *cgu_isp_clk;
	struct clk *csi_clk;
    void __iomem *vic_base;
#ifdef CONFIG_MIPI_SENSOR
    void __iomem *mipi_base;
    struct resource *mipi_res;
#endif
        struct device *dev;


	unsigned int irq;
	unsigned long mclk_freq;
	spinlock_t lock;

	void *desc_vaddr;
	struct jz_camera_dma_desc *dma_desc;
	struct jz_camera_dma_desc *dma_desc_head;
	struct jz_camera_dma_desc *dma_desc_tail;

	/* for debug */
	long long debug_ms_start;

};

typedef enum {
    fmt_BAYER_RGGB_16BIT,
    fmt_BAYER_GRBG_16BIT,
    fmt_BAYER_GBRG_16BIT,
    fmt_BAYER_BGGR_16BIT,
    fmt_YUV422_YUYV,
    fmt_YUV422_UYVY,
    fmt_YUV422_YVYU,
    fmt_YUV422_VYUY,
    fmt_NV12,
    fmt_NV21,
    fmt_BAYER_RGGB_8BIT,
    fmt_BAYER_GRBG_8BIT,
    fmt_BAYER_GBRG_8BIT,
    fmt_BAYER_BGGR_8BIT,
    fmt_Y8,
} camera_data_fmt;

#define fmt_is(fmt, start, end) (start <= (fmt) && (fmt) <= end)

#define fmt_is_NV12(fmt) fmt_is(fmt, fmt_NV12, fmt_NV21)
#define fmt_is_YUV422(fmt) fmt_is(fmt, fmt_YUV422_YUYV, fmt_YUV422_VYUY)
#define fmt_is_BAYER_8BIT(fmt) fmt_is(fmt, fmt_BAYER_RGGB_8BIT, fmt_BAYER_BGGR_8BIT)
#define fmt_is_BAYER_16BIT(fmt) fmt_is(fmt, fmt_BAYER_RGGB_16BIT, fmt_BAYER_BGGR_16BIT)

struct camera_info {
    const char *name;
    unsigned int xres;
    unsigned int yres;
    unsigned int frame_period_us;
    camera_data_fmt data_fmt;

    /* 一帧数据经过对齐之后的大小 */
    unsigned int frame_size;

    /* 诸如 NV12 等 planner 格式下, UV 数据在一帧中的偏移 */
    unsigned int uv_data_offset;
};


typedef enum {
    VIC_bt656,
    VIC_bt601,
    VIC_mipi_csi,
    VIC_dvp,
    VIC_bt1120,
} vic_interface;

typedef enum {
    DVP_RAW8,
    DVP_RAW10,
    DVP_RAW12,
    DVP_YUV422,
    DVP_RGB565,
    DVP_RGB888,
    DVP_YUV422_8BIT,
    DVP_RGB16_8BIT,
} dvp_data_fmt;

typedef enum {
    DVP_PA_LOW_10BIT,
    DVP_PA_HIGH_10BIT,
    DVP_PA_12BIT,
    DVP_PA_LOW_8BIT,
    DVP_PA_HIGH_8BIT,
} dvp_gpio_mode;

typedef enum {
    DVP_href_mode,
    DVP_hsync_mode,
    DVP_sony_mode,
} dvp_timing_mode;

typedef enum {
    /* 这里的顺序是:          clk1,clk2,clk3,clk4
     *      YUV_UYVY_8bit     U    Y    V    Y
     */
    YUV_Y1V0Y0U0,
    YUV_Y1U0Y0V0,
    YUV_V0Y1U0Y0,
    YUV_U0Y1V0Y0,
} yuv_data_order;

typedef enum {
    POLARITY_HIGH_ACTIVE,
    POLARITY_LOW_ACTIVE,
} dvp_sync_polarity;

typedef enum {
    DVP_img_scan_progress,
    DVP_img_scan_interlace,
} dvp_img_scan_mode;

/*
 * MIPI information
 */
typedef enum {
    MIPI_RAW8,
    MIPI_RAW10,
    MIPI_RAW12,
    MIPI_RGB555,
    MIPI_RGB565,
    MIPI_RGB666,
    MIPI_RGB888,
    MIPI_YUV422,
    MIPI_YUV422_10BIT,
} mipi_data_fmt;

typedef enum {
    MIPI_WAITLP11_MODE,
    MIPI_NOWAITLP11_MODE,
} mipi_mode;

typedef enum {
    ISP_preset_mode_1,
    ISP_preset_mode_2,
    ISP_preset_mode_3,
} isp_port_mode;


struct dvp_bus_info {
    dvp_data_fmt dvp_data_fmt;
    dvp_gpio_mode dvp_gpio_mode;
    dvp_timing_mode dvp_timing_mode;
    yuv_data_order dvp_yuv_data_order;
    dvp_sync_polarity dvp_hsync_polarity;
    dvp_sync_polarity dvp_vsync_polarity;
    dvp_img_scan_mode dvp_img_scan_mode;
};


struct mipi_csi_bus_info {
    mipi_data_fmt data_fmt;
    mipi_mode mode;
    int lanes;
    int clk;
};

struct vic_sensor_config {
    struct camera_info info;
    vic_interface vic_interface;
    union {
        struct dvp_bus_info dvp_cfg_info;
        struct mipi_csi_bus_info mipi_cfg_info;
    };

    long isp_clk_rate;
    isp_port_mode isp_port_mode;
};

#endif
