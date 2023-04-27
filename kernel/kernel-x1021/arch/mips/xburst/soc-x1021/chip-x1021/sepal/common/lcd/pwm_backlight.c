#include <linux/pwm_backlight.h>
#include <linux/platform_device.h>
#include "board.h"

static struct platform_pwm_backlight_data backlight_data = {
    .pwm_id        = 0,
    .max_brightness    = 255,
    .dft_brightness    = 120,
    .pwm_period_ns    = 30000,
    .init        = NULL,
    .exit        = NULL,
};

struct platform_device backlight_device = {
    .name        = "pwm-backlight",
    .dev        = {
        .platform_data    = &backlight_data,
    },
};
