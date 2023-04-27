#include <mach/jz_camera.h>

static struct vic_sensor_config ov9281_mipi_sensor_config = {
    .info = {
        .name               = "ov9281 mipi",
        .xres               = 1280,
        .yres               = 720,
        .frame_period_us    = 33333, // 72 甯?
        .data_fmt           = fmt_YUV422_YUYV,
    },

    .mipi_cfg_info = {
        .data_fmt           = MIPI_RAW8,
        .lanes              = 2,
    },

    .isp_clk_rate           = 90 * 1000 * 1000,
    .vic_interface          = VIC_mipi_csi,
};



