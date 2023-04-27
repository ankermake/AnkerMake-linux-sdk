#include <linux/module.h>
#include <linux/gpio.h>
#include <common.h>
#include <linux/slab.h>
#include <board.h>
#include <linux/platform_device.h>
#include <board.h>

#define DEVNAME "x01_led"

enum x01_led_type {
    MONOCHROME_LED,
    RGB_LED,
    BGR_LED,
    RBG_LED,
    BRG_LED,
    GRB_LED,
    GBR_LED,
    ERROR_TYPE
};

struct jz_pwm_led {
    const char *name;
    unsigned int pwm_id;
    bool active_low;
};

struct jz_pwm_led_platform_data {
    const char *dev_name;
    int led_type;
    int led_pins;
    struct jz_pwm_led *pwm_leds;
};

static struct jz_pwm_led pwm_leds[] = {
    {
        .name       = "red",
        .pwm_id     = RED_LED_PWM_ID,
        .active_low = RED_LED_ACTIVE_LOW,
    },
    {
        .name       = "green",
        .pwm_id     = GREEN_LED_PWM_ID,
        .active_low = GREEN_LED_ACTIVE_LOW,
    },
    {
        .name       = "blue",
        .pwm_id     = BLUE_LED_PWM_ID,
        .active_low = BLUE_LED_ACTIVE_LOW,
    },
};

static struct jz_pwm_led_platform_data pwm_led_platform_data = {
    .dev_name = DEVNAME,
    .led_type = RGB_LED,
    .led_pins = ARRAY_SIZE(pwm_leds),
    .pwm_leds = pwm_leds,
};

static struct platform_device pwm_led_dev = {
    .name = DEVNAME,
    .id = -1,
    .dev = {
        .platform_data = &pwm_led_platform_data,
    },
};

static int __init pwm_led_dev_init(void)
{
	int ret;

	ret = platform_device_register(&pwm_led_dev);
	if (ret < 0)
		pr_err("failed to register led_dev\n");

	return ret;
}

static void __exit pwm_led_dev_exit(void)
{
	platform_device_unregister(&pwm_led_dev);
}

module_init(pwm_led_dev_init);
module_exit(pwm_led_dev_exit);
MODULE_LICENSE("GPL");
