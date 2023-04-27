#include <linux/gpio.h>
#include <linux/device.h>
#include <board.h>

#if defined(GPIO_SPEAKER_EN) && defined(GPIO_SPEAKER_EN_LEVEL)
static int gpio_speaker_en = -1;
#endif

static int __speaker_init(struct device *dev)
{
#if defined(GPIO_SPEAKER_EN) && defined(GPIO_SPEAKER_EN_LEVEL)
    int ret;
    if (dev) {
        ret = devm_gpio_request_one(dev, GPIO_SPEAKER_EN,
                GPIOF_DIR_OUT | (GPIO_SPEAKER_EN_LEVEL ? 0 : GPIOF_INIT_HIGH),
                "speaker_en");
    } else {
        ret = gpio_request_one(GPIO_SPEAKER_EN,
                GPIOF_DIR_OUT | (GPIO_SPEAKER_EN_LEVEL ? 0 : GPIOF_INIT_HIGH),
                "speaker_en");
    }
    if (!ret)
        gpio_speaker_en = GPIO_SPEAKER_EN;
    return ret;
#else
    return 0;
#endif
}
static void __speaker_deinit(struct device *dev)
{
#if defined(GPIO_SPEAKER_EN) && defined(GPIO_SPEAKER_EN_LEVEL)
    if (gpio_speaker_en != -1) {
        if (dev)
            devm_gpio_free(dev, GPIO_SPEAKER_EN);
        else
            gpio_free(GPIO_SPEAKER_EN);
    }
    gpio_speaker_en = -1;
#endif
}
static int __speaker_enable(bool enable)
{
#if defined(GPIO_SPEAKER_EN) && defined(GPIO_SPEAKER_EN_LEVEL)
    if (gpio_speaker_en == -1) {
        pr_err("no speaker_en gpio\n");
        return -1;
    }
    if (enable)
        return gpio_direction_output(GPIO_SPEAKER_EN, GPIO_SPEAKER_EN_LEVEL);
    else
        return gpio_direction_output(GPIO_SPEAKER_EN, (int)(!GPIO_SPEAKER_EN_LEVEL));
#else
    return 0;
#endif
}

int jz_speaker_init(struct device *dev) __attribute__ ((weak,alias("__speaker_init")));
EXPORT_SYMBOL_GPL(jz_speaker_init);
void jz_speaker_deinit(struct device *dev) __attribute__ ((weak,alias("__speaker_deinit")));
EXPORT_SYMBOL_GPL(jz_speaker_deinit);
int jz_speaker_enable(bool enable) __attribute__ ((weak,alias("__speaker_enable")));
EXPORT_SYMBOL_GPL(jz_speaker_enable);
