#include <linux/module.h>
#include <utils/gpio.h>
#include <linux/slab.h>
#include <common.h>
#include <linux/regulator/driver.h>
#include <linux/regulator/fixed.h>
#include <linux/regulator/machine.h>
#include <linux/platform_device.h>

struct gpio_regulator_data {
    const char *reg_name;
    char *supply_name;
    char gpio_name[12];
    int gpio;
    int active_level;
    struct regulator_desc desc;
    struct regulator_dev *dev;
    struct regulator_init_data init_data;
    struct regulator_consumer_supply consumer_supply;
};

#define GPIO_REGULATOR_NUM       3

static struct gpio_regulator_data reg_data[GPIO_REGULATOR_NUM] = {
    { .reg_name   = "gpio_regulator0", },
    { .reg_name   = "gpio_regulator1", },
    { .reg_name   = "gpio_regulator2", },
};

module_param_gpio_named(regulator_gpio0, reg_data[0].gpio, 0644);
module_param_gpio_named(regulator_gpio1, reg_data[1].gpio, 0644);
module_param_gpio_named(regulator_gpio2, reg_data[2].gpio, 0644);
module_param_named(active_gpio0, reg_data[0].active_level, int, 0644);
module_param_named(active_gpio1, reg_data[1].active_level, int, 0644);
module_param_named(active_gpio2, reg_data[2].active_level, int, 0644);
module_param_named(regulator_name0, reg_data[0].supply_name, charp, 0644);
module_param_named(regulator_name1, reg_data[1].supply_name, charp, 0644);
module_param_named(regulator_name2, reg_data[2].supply_name, charp, 0644);

static int gpio_regulator_enable(struct regulator_dev *rdev)
{
    struct gpio_regulator_data *data = rdev_get_drvdata(rdev);

    gpio_direction_output(data->gpio, !!data->active_level);
    return 0;
}

static int gpio_regulator_disable(struct regulator_dev *rdev)
{
    struct gpio_regulator_data *data = rdev_get_drvdata(rdev);

    gpio_direction_output(data->gpio, !data->active_level);
    return 0;
}

static int gpio_regulator_is_enabled(struct regulator_dev *rdev)
{
    struct gpio_regulator_data *data = rdev_get_drvdata(rdev);
    int ret;

    ret = gpio_get_value(data->gpio);
    if (ret == !!data->active_level) {
        return 1;
    }
    return 0;
}

static struct regulator_ops gpio_regulator_voltage_ops = {
    .enable       = gpio_regulator_enable,
    .disable      = gpio_regulator_disable,
    .is_enabled   = gpio_regulator_is_enabled,
};

static int init_gpio_regulator_data(int index, struct gpio_regulator_data *drvdata)
{
    int ret;

    sprintf(drvdata->gpio_name, "reg_gpio%d", index);
    ret = gpio_request(drvdata->gpio, drvdata->gpio_name);
    if (ret) {
        printk(KERN_ERR "could not obtain regulator seting gpios: %d\n", ret);
        return -1;
    }

    drvdata->desc.owner = THIS_MODULE;
    drvdata->desc.id = index;
    drvdata->desc.type = REGULATOR_VOLTAGE;
    drvdata->desc.ops = &gpio_regulator_voltage_ops;
    drvdata->desc.name = drvdata->supply_name;

    drvdata->init_data.constraints.valid_ops_mask = REGULATOR_CHANGE_STATUS;
    drvdata->init_data.num_consumer_supplies = 1;
    drvdata->init_data.consumer_supplies = &drvdata->consumer_supply;
    drvdata->init_data.consumer_supplies->supply = drvdata->supply_name;
    return 0;
}

static int gpio_regulator_probe(struct platform_device *pdev)
{
    struct gpio_regulator_data *drvdata;
    struct regulator_config cfg = { };
    int i, ret;

    for (i = 0; i < GPIO_REGULATOR_NUM; i++) {
        if (reg_data[i].gpio == -1 || !strcmp(reg_data[i].supply_name, "-1")
            || !strlen(reg_data[i].supply_name))
            continue;

        ret = init_gpio_regulator_data(i, &reg_data[i]);
        if (ret)
            continue;

        drvdata = &reg_data[i];
        cfg.dev = &pdev->dev;
        cfg.init_data = &drvdata->init_data;
        cfg.driver_data = drvdata;
        drvdata->dev = regulator_register(&drvdata->desc, &cfg);
        if (IS_ERR(drvdata->dev)) {
            gpio_free(drvdata->gpio);
            printk(KERN_ERR "Failed to register regulator.\n");
            continue;
        }
    }
    return 0;
}

static void gpio_regulator_release(struct device *dev)
{

}

static struct platform_device gpio_reg_dev = {
    .name    = "gpio_regulator",
    .id      = -1,
    .dev = {
        .release       = gpio_regulator_release,
    },
};

static int gpio_regulator_remove(struct platform_device *pdev)
{
    int i;

    for (i = 0; i < GPIO_REGULATOR_NUM; i++) {
        if (reg_data[i].dev) {
            regulator_unregister(reg_data[i].dev);
            gpio_free(reg_data[i].gpio);
        }
    }
    return 0;
}

struct platform_driver gpio_reg_drv = {
    .probe     = gpio_regulator_probe,
    .remove    = gpio_regulator_remove,
    .driver    = {
        .name  = "gpio_regulator",
    }
};

static int __init intit_gpio_regulator(void)
{
    platform_device_register(&gpio_reg_dev);
    platform_driver_register(&gpio_reg_drv);
    return 0;
}

static void __exit exit_gpio_regulator(void)
{
    platform_driver_unregister(&gpio_reg_drv);
    platform_device_unregister(&gpio_reg_dev);
}

module_init(intit_gpio_regulator);
module_exit(exit_gpio_regulator);

MODULE_DESCRIPTION("X2000 gpio regulator driver");
MODULE_LICENSE("GPL");