#include <linux/gpio.h>
#include <utils/gpio.h>
#include <linux/kernel.h>
#include <linux/bitops.h>

#define SPDIF_FUNC GPIO_FUNC_2
#define SPDIF_IN   GPIO_PC(13)
#define SPDIF_OUT  GPIO_PC(14)

static int m_request_gpio(unsigned gpio, int func, const char *name)
{
    int ret = gpio_request(gpio, name);
    if (ret) {
        char str[10];
        printk(KERN_ERR "spdif: failed to request gpio: %s as %s\n",
            gpio_to_str(gpio, str), name);
        return ret;
    }

    gpio_set_func(gpio, func);

    return 0;
}


static unsigned int spdif_in_flag = 0;
static unsigned int spdif_out_flag = 0;

static int spdif_in_gpio_request(void)
{
    if (spdif_in_flag == 0) {
        spdif_in_flag = 1;
        return m_request_gpio(SPDIF_IN, SPDIF_FUNC, "spdif-in");
    }
    return 0;
}

static int spdif_out_gpio_request(void)
{
     if (spdif_out_flag == 0) {
        spdif_out_flag = 1;
        return m_request_gpio(SPDIF_OUT, SPDIF_FUNC, "spdif-out");
    }
    return 0;
}

static void spdif_gpio_free(void)
{
    if (spdif_in_flag) {
        gpio_free(SPDIF_IN);
        spdif_in_flag = 0;
    }

     if (spdif_out_flag) {
        gpio_free(SPDIF_OUT);
        spdif_out_flag = 0;
    }
}
