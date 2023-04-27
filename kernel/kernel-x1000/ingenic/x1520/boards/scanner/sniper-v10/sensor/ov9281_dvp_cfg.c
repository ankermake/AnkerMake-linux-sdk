#include <mach/jz_camera.h>

static struct vic_sensor_config ov9281_dvp_sensor_config = {
    .info = {
        .name               = "ov9281",
        .xres               = 640,
        .yres               = 480,
        .frame_period_us    = 13888, // 72 甯?
        .data_fmt           = fmt_NV12,
    },

    .dvp_cfg_info = {
        .dvp_data_fmt       = DVP_YUV422,
        .dvp_gpio_mode      = DVP_PA_LOW_8BIT,
        .dvp_timing_mode    = DVP_href_mode,
        .dvp_yuv_data_order = YUV_V0Y1U0Y0,
        .dvp_hsync_polarity = POLARITY_HIGH_ACTIVE,
        .dvp_vsync_polarity = POLARITY_HIGH_ACTIVE,
        .dvp_img_scan_mode  = DVP_img_scan_progress,
    },
    .isp_clk_rate           = 125 * 1000 * 1000,
    .vic_interface          = VIC_dvp,
    .isp_port_mode          = ISP_preset_mode_1,
};


