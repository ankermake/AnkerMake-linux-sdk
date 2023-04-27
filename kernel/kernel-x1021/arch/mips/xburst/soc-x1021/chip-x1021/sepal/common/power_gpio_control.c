#include <linux/init.h>
#include <linux/kernel.h>
#include <linux/gpio.h>
#include <linux/module.h>
#include <jz_notifier.h>
#include <ingenic_common.h>
#include <board.h>

static int power_notify(struct jz_notifier *notify, void *data)
{
    gpio_direction_output(POWER_ON_GPIO, !POWER_ON_ACTIVE_LEVEL);

    return NOTIFY_STOP;
}

static struct jz_notifier power_notifier = {
    .jz_notify = power_notify,
    .level = NOTEFY_PROI_HIGH,
    .msg = JZ_POST_HIBERNATION,
};

static int __init power_gpio_control_init(void)
{
    int ret;

    if(gpio_is_valid(POWER_ON_GPIO)) {
        ret = gpio_request(POWER_ON_GPIO, "power_on_gpio");
        assert(ret >= 0);

        ret = jz_notifier_register(&power_notifier, NOTEFY_PROI_HIGH);
        assert(ret >= 0);

        gpio_direction_output(POWER_ON_GPIO, POWER_ON_ACTIVE_LEVEL);
    }

    return 0;
}

subsys_initcall(power_gpio_control_init);
