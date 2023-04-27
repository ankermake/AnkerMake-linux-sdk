#include <utils/gpio.h>
#include <linux/platform_device.h>
#include <linux/fb.h>
#include <linux/backlight.h>
#include <utils/gpio.h>
#include <linux/module.h>

static inline int m_gpio_request(int gpio , const char *name)
{
    if (gpio < 0)
        return 0;

    int ret = gpio_request(gpio, name);
    if (ret) {
        char buf[20];
        printk(KERN_ERR "gpio_backlight: failed to request %s: %s\n", name, gpio_to_str(gpio, buf));
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


static struct backlight_device *backlight_dev;
static int backlight_gpio = -1;
static char *backlight_dev_name = NULL;
static int gpio_active_level = -1;
static int backlight_enabled = -1;


module_param_gpio_named(backlight_gpio, backlight_gpio, 0644);
module_param_named(backlight_dev_name, backlight_dev_name, charp, 0644);
module_param_named(backlight_enabled, backlight_enabled, int, 0644);
module_param_named(gpio_active_level, gpio_active_level, int, 0644);


static int gpio_backlight_update_status(struct backlight_device *bl)
{
    int brightness = bl->props.brightness;

    if (bl->props.power != FB_BLANK_UNBLANK ||
        bl->props.fb_blank != FB_BLANK_UNBLANK ||
        bl->props.state & BL_CORE_FBBLANK)
        brightness = 0;

    if (brightness)
        m_gpio_direction_output(backlight_gpio, gpio_active_level);
    else
        m_gpio_direction_output(backlight_gpio, !gpio_active_level);

    return 0;
}

static int gpio_backlight_get_brightness(struct backlight_device *bl)
{
    return bl->props.brightness;
}

static const struct backlight_ops gpio_backlight_backlight_ops = {
    .update_status = gpio_backlight_update_status,
    .get_brightness = gpio_backlight_get_brightness,
};


int __init gpio_backlight_drv_init(void)
{
    struct backlight_properties props;
    struct backlight_device *bl;
    int ret;

    ret = m_gpio_request(backlight_gpio, backlight_dev_name);
    if (ret)
        goto err_gpio_request;


    memset(&props, 0, sizeof(struct backlight_properties));
    props.type = BACKLIGHT_RAW;
    props.max_brightness = 1;
    bl = backlight_device_register(backlight_dev_name, NULL, NULL,
                       &gpio_backlight_backlight_ops, &props);
    if (IS_ERR(bl)) {
        printk(KERN_ERR "gpio backlight %s failed to register backlight\n", backlight_dev_name);
        ret = PTR_ERR(bl);
        goto err_alloc;
    }

    bl->props.brightness = backlight_enabled;
    backlight_dev = bl;
    backlight_update_status(bl);

    return 0;

err_alloc:
    m_gpio_free(backlight_gpio);
err_gpio_request:
    return ret;
}

void __exit gpio_backlight_drv_exit(void)
{
    backlight_device_unregister(backlight_dev);
    m_gpio_free(backlight_gpio);
}

module_init(gpio_backlight_drv_init);
module_exit(gpio_backlight_drv_exit);

MODULE_LICENSE("GPL");
MODULE_DESCRIPTION("gpio_backlight driver");