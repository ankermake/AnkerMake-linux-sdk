#include <linux/module.h>
#include <linux/string.h>
#include <linux/input.h>
#include <linux/gpio_keys.h>
#include <linux/slab.h>
#include <linux/gpio.h>
#include <linux/list.h>
#include <linux/kernel.h>
#include <linux/platform_device.h>

#include <utils/string.h>
#include <utils/gpio.h>
struct button_data {
    struct platform_device pdev;
    struct gpio_keys_platform_data pdata;
    struct gpio_keys_button buttons[0];
};

static int save_cnt = 0;
static int bnt_cnt = 0;
static int total_cnt = 0;
static struct button_data *data = NULL;

static int param_keyboard_set(const char *arg, const struct kernel_param *kp)
{
    int ret;
    int gpio = -1, active_low = -1, code = -1, is_register = 0, is_alloc = 0, wakeup = -1;
    const char *tag = NULL;

    int i, N;
    char **argv = str_to_words(arg, &N);

    for (i = 0; i < N; i++) {
        if (strstarts(argv[i], "alloc=")) {
            unsigned long num;
            ret = kstrtoul(argv[i] + strlen("alloc="), 0, &num);
            if (ret) {
                printk(KERN_ERR "%s is not a valid num\n", argv[i]);
                goto parse_err;
            }

            if (num == 0)
                return -EINVAL;

            if (total_cnt) {
                printk(KERN_ERR "new alloc %ld can not be done, old keys not register!\n", num);
                goto parse_err;
            }

            is_alloc = 1;
            total_cnt = num;
            continue;
        }

        if (!strcmp(argv[i], "register")) {
            is_register = 1;
            break;
        }

        if (strstarts(argv[i], "gpio=")) {
            ret = str_to_gpio(argv[i] + strlen("gpio="));
            if (ret < -1) {
                printk(KERN_ERR "%s is not a gpio\n", argv[i]);
                goto parse_err;
            }
            gpio = ret;
            continue;
        }

        if (strstarts(argv[i], "key_code=")) {
            unsigned long num;
            ret = kstrtoul(argv[i] + strlen("key_code="), 0, &num);
            if (ret) {
                printk(KERN_ERR "%s is not a valid num\n", argv[i]);
                goto parse_err;
            }
            code = num;
            continue;
        }

        if (strstarts(argv[i], "tag=")) {
            if (tag)
                kfree(tag);
            tag = kstrdup(argv[i] + strlen("tag="), GFP_KERNEL);
            continue;
        }

        if (strstarts(argv[i], "active_level=")) {
            char *str = argv[i] + strlen("active_level=");
            if (!strcmp(str, "0"))
                active_low = 1;
            else if (!strcmp(str, "1"))
                active_low = 0;
            else {
                printk(KERN_ERR "%s active_level only support '0' or '1'\n", argv[i]);
                goto parse_err;
            }
            continue;
        }

        if (strstarts(argv[i], "wakeup=")) {
            char *str = argv[i] + strlen("wakeup=");
            if (!strcmp(str, "y"))
                wakeup = 1;
            else
                wakeup = 0;
            continue;
        }

        printk(KERN_ERR "%s can not be parse!\n", argv[i]);
        goto parse_err;
    }

    str_free_words(argv);

    if (is_alloc) {
        data = kzalloc(sizeof(*data) + total_cnt * sizeof(struct gpio_keys_button), GFP_KERNEL);
        if (!data) {
            printk(KERN_ERR "failed to alloc %d keys\n", total_cnt);
            total_cnt = 0;
        }
        return 0;
    }

    if (is_register) {
        if (!bnt_cnt) {
            printk(KERN_ERR "can not register, no key is set!\n");
            return -ENODEV;
        }

        data->pdev.name = "md-gpio-keys";
        data->pdev.id = -1;
        data->pdev.dev.platform_data = &data->pdata;
        data->pdata.buttons = data->buttons;
        data->pdata.nbuttons = bnt_cnt;

        save_cnt = bnt_cnt;
        bnt_cnt = 0;
        total_cnt = 0;

        return platform_device_register(&data->pdev);
    }

    if (gpio == -1) {
        printk(KERN_ERR "gpio must define!\n");
        return -EINVAL;
    }

    if (active_low == -1) {
        printk(KERN_ERR "activel_level must define!\n");
        return -EINVAL;
    }

    if (code == -1) {
        printk(KERN_ERR "key_code must define!\n");
        return -EINVAL;
    }

    if (bnt_cnt >= total_cnt) {
        printk(KERN_ERR "too many gpio keys, total:%d!\n", total_cnt);
        return -ERANGE;
    }

    i = bnt_cnt++;

    data->buttons[i].gpio = gpio;
    data->buttons[i].code = code;
    data->buttons[i].active_low = active_low;
    data->buttons[i].desc = tag;
    data->buttons[i].debounce_interval = 20;
    data->buttons[i].wakeup = wakeup;

    return 0;
parse_err:
    str_free_words(argv);
    return -EINVAL;
}

static int param_keyboard_get(char *buffer, const struct kernel_param *kp)
{
    int i;
    char *buf = buffer;
    int N = bnt_cnt ? bnt_cnt : save_cnt;

    for (i = 0; i < N; i++) {
        char gpio[10];
        int code = data->pdata.buttons[i].code;
        char *wakeup = data->pdata.buttons[i].wakeup ? "true" : "false";
        char *active_level = data->pdata.buttons[i].active_low ? "low" : "high";
        const char *tag = data->pdata.buttons[i].desc;
        gpio_to_str(data->pdata.buttons[i].gpio, gpio);
        buf += sprintf(buf, "gpio=%s code=%d tag=%s active_level=%s wakeup=%s\n",
            gpio, code, tag, active_level, wakeup);
    }

    return buf - buffer;
}

static struct kernel_param_ops param_keyboard_ops = {
    .set = param_keyboard_set,
    .get = param_keyboard_get,
};

module_param_cb(keyboard, &param_keyboard_ops, NULL, 0644);
