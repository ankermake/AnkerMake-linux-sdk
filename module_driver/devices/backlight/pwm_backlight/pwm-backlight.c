#include <pwm.h>
#include <utils/gpio.h>
#include <linux/platform_device.h>
#include <linux/fb.h>
#include <linux/backlight.h>
#include <utils/gpio.h>
#include <linux/module.h>

struct pwm_backlight_data {
    int pwm_id;
    int pwm_gpio;
    int pwm_active_level;
    int pwm_enable_gpio;
    int pwm_enable_gpio_vaild_level;
    int pwm_set_value;
    char *pwm_name;

    struct backlight_device *bl;
    struct pwm_config_data config;
};

static inline int m_gpio_request(int gpio , const char *name)
{
    if (gpio < 0)
        return 0;

    int ret = gpio_request(gpio, name);
    if (ret) {
        char buf[20];
        printk(KERN_ERR "pwm_backlight: failed to request %s: %s\n", name, gpio_to_str(gpio, buf));
        return ret;
    }

    return 0;
}

static inline void m_gpio_free(int gpio)
{
    if (gpio >= 0)
        gpio_free(gpio);
}

static inline void m_gpio_direction_output(int gpio, int value)
{
    if (gpio >= 0)
        gpio_direction_output(gpio, value);
}

static struct pwm_backlight_data backlight_data = {
    .config     = {
        .shutdown_mode = PWM_abrupt_shutdown,
        .idle_level = PWM_idle_low,
        .accuracy_priority = PWM_accuracy_levels_first,
        .freq = 1000000,
        .levels = 300,
    }
};

static int pwm_backlight_update_status(struct backlight_device *bl)
{
    struct pwm_backlight_data *data = &backlight_data;
    int brightness = bl->props.brightness;

    if (bl->props.power != FB_BLANK_UNBLANK ||
        bl->props.fb_blank != FB_BLANK_UNBLANK ||
        bl->props.state & BL_CORE_FBBLANK)
        brightness = 0;

    if (brightness) {
        pwm2_set_level(data->pwm_id, brightness);
        m_gpio_direction_output(data->pwm_enable_gpio, data->pwm_enable_gpio_vaild_level);
    } else {
        pwm2_set_level(data->pwm_id, 0);
        m_gpio_direction_output(data->pwm_enable_gpio, !data->pwm_enable_gpio_vaild_level);
    }

    return 0;
}

static int pwm_backlight_get_brightness(struct backlight_device *bl)
{
    return bl->props.brightness;
}

static const struct backlight_ops pwm_backlight_backlight_ops = {
    .update_status = pwm_backlight_update_status,
    .get_brightness = pwm_backlight_get_brightness,
};

module_param_gpio_named(pwm_gpio, backlight_data.pwm_gpio, 0644);
module_param_named(default_brightness, backlight_data.pwm_set_value, int, 0644);
module_param_named(backlight_dev_name, backlight_data.pwm_name, charp, 0644);
module_param_gpio_named(power_gpio, backlight_data.pwm_enable_gpio, 0644);
module_param_named(power_gpio_vaild_level, backlight_data.pwm_enable_gpio_vaild_level, int, 0644);

module_param_named(pwm_active_level, backlight_data.pwm_active_level, int, 0644);
module_param_named(pwm_freq, backlight_data.config.freq, ulong, 0644);
module_param_named(max_brightness, backlight_data.config.levels, ulong, 0644);


int __init pwm_backlight_drv_init(void)
{
    struct pwm_backlight_data *data = &backlight_data;
    struct backlight_properties props;
    struct backlight_device *bl;
    int ret;

    ret = m_gpio_request(data->pwm_enable_gpio, data->pwm_name);
    if (ret)
        goto err_gpio_request;

    data->pwm_id = pwm2_request(data->pwm_gpio, data->pwm_name);
    data->config.idle_level = !data->pwm_active_level;
    pwm2_config(data->pwm_id, &data->config);

    memset(&props, 0, sizeof(struct backlight_properties));
    props.type = BACKLIGHT_RAW;
    props.max_brightness = data->config.levels;
    bl = backlight_device_register(data->pwm_name, NULL, NULL,
                       &pwm_backlight_backlight_ops, &props);
    if (IS_ERR(bl)) {
        printk(KERN_ERR "pwm_backlight %s failed to register backlight\n", data->pwm_name);
        ret = PTR_ERR(bl);
        goto err_alloc;
    }

    bl->props.brightness = data->pwm_set_value;
    data->bl = bl;
    backlight_update_status(bl);

    return 0;

err_alloc:
    m_gpio_free(data->pwm_enable_gpio);
err_gpio_request:
    return ret;
}

void __exit pwm_backlight_drv_exit(void)
{
    struct pwm_backlight_data *data = &backlight_data;

    backlight_device_unregister(data->bl);
    pwm2_set_level(data->pwm_id, 0);
    pwm2_release(data->pwm_id);
    m_gpio_free(data->pwm_enable_gpio);
}

module_init(pwm_backlight_drv_init);
module_exit(pwm_backlight_drv_exit);

MODULE_LICENSE("GPL");
MODULE_DESCRIPTION("pwm_backlight driver");