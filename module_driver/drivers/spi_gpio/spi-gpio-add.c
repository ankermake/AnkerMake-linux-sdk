#include <linux/module.h>
#include <linux/string.h>
#include <linux/slab.h>
#include <linux/gpio.h>
#include <utils/string.h>
#include <utils/gpio.h>
#include <linux/list.h>
#include <linux/kernel.h>
#include <linux/platform_device.h>

#include "spi_gpio.h"

static LIST_HEAD(gpio_spi_list);

struct gpio_spi_data {
    struct list_head link;
    struct platform_device pdev;
    struct spi_gpio_pdata pdata;
};

static int param_spi_bus_set(const char *arg, const struct kernel_param *kp)
{
    int ret, bus_num = -1;
    struct spi_gpio_pdata pdata;

    memset(&pdata, 0, sizeof(pdata));
    pdata.sck = -1;
    pdata.mosi = -1;
    pdata.miso = -1;
    pdata.num_chipselect = 8;

    int i, N;
    char **argv = str_to_words(arg, &N);
    for (i = 0; i < N; i++) {
        if (strstarts(argv[i], "sck=")) {
            ret = str_to_gpio(argv[i] + strlen("sck="));
            if (ret < -1) {
                printk(KERN_ERR "%s is not a gpio\n", argv[i]);
                goto parse_err;
            }
            pdata.sck = ret;
            continue;
        }

        if (strstarts(argv[i], "mosi=")) {
            ret = str_to_gpio(argv[i] + strlen("mosi="));
            if (ret < -1) {
                printk(KERN_ERR "%s is not a gpio\n", argv[i]);
                goto parse_err;
            }
            pdata.mosi = ret;
            continue;
        }

        if (strstarts(argv[i], "miso=")) {
            ret = str_to_gpio(argv[i] + strlen("miso="));
            if (ret < -1) {
                printk(KERN_ERR "%s is not a gpio\n", argv[i]);
                goto parse_err;
            }
            pdata.miso = ret;
            continue;
        }

        if (strstarts(argv[i], "num_chipselect=")) {
            unsigned long n;
            ret = kstrtoul(argv[i] + strlen("num_chipselect="), 0, &n);
            if (ret) {
                printk(KERN_ERR "%s is not a valid num\n", argv[i]);
                goto parse_err;
            }
            pdata.num_chipselect = n;
            continue;
        }

        if (strstarts(argv[i], "bus_num=")) {
            unsigned long num;
            ret = kstrtoul(argv[i] + strlen("bus_num="), 0, &num);
            if (ret) {
                printk(KERN_ERR "%s is not a valid num\n", argv[i]);
                goto parse_err;
            }
            bus_num = num;
            continue;
        }

        printk(KERN_ERR "%s can not be parse!\n", argv[i]);
        goto parse_err;
    }

    if (pdata.sck == -1) {
        printk(KERN_ERR "sck must define!\n");
        goto parse_err;
    }

    if (bus_num == -1) {
        printk(KERN_ERR "bus_num must define!\n");
        goto parse_err;
    }

    str_free_words(argv);

    struct gpio_spi_data *data = kzalloc(sizeof(*data), GFP_KERNEL);

    data->pdata = pdata;
    data->pdev.name = "md_spi_gpio";
    data->pdev.id = bus_num;
    data->pdev.dev.platform_data = &data->pdata;

    ret = platform_device_register(&data->pdev);
    if (ret < 0) {
        printk(KERN_ERR "\nspi_gpio: failed to register platform device\n");
        kfree(data);
        return ret;
    }

    list_add_tail(&data->link, &gpio_spi_list);

    return 0;
parse_err:
    str_free_words(argv);
    return -EINVAL;
}

static int param_spi_bus_get(char *buffer, const struct kernel_param *kp)
{
    struct list_head *pos;
    char *buf = buffer;

    list_for_each(pos, &gpio_spi_list) {
        struct gpio_spi_data *data = list_entry(pos, struct gpio_spi_data, link);
        char sck[10];
        char mosi[10];
        char miso[10];
        int bus_num = data->pdev.id;
        int num_chipselect = data->pdata.num_chipselect;
        gpio_to_str(data->pdata.sck, sck);
        gpio_to_str(data->pdata.mosi, mosi);
        gpio_to_str(data->pdata.miso, miso);
        buf += sprintf(buf, "bus_num=%d sck=%s mosi=%s miso=%s num_chipselect=%d\n",
            bus_num, sck, mosi, miso, num_chipselect);
    }

    return buf - buffer;
}

static struct kernel_param_ops param_spi_bus_ops = {
    .set = param_spi_bus_set,
    .get = param_spi_bus_get,
};

module_param_cb(spi_bus, &param_spi_bus_ops, NULL, 0644);
