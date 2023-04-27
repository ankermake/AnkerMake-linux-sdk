/*
 * Copyright (C) 2020 Ingenic Semiconductor Co., Ltd.
 *
 * Camera Driver for the Ingenic controller
 *
 */
#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/delay.h>
#include <linux/platform_device.h>

#include <utils/clock.h>

#include "vic.h"


static int jz_arch_camera_probe(struct platform_device *pdev)
{
    jz_arch_vic_init(&pdev->dev);

    return 0;
}

static int jz_arch_camera_remove(struct platform_device *pdev)
{
    jz_arch_vic_exit();

    return 0;
}

static struct platform_driver jz_arch_camera_driver = {
    .probe = jz_arch_camera_probe,
    .remove = jz_arch_camera_remove,
    .driver = {
        .owner = THIS_MODULE,
        .name = "jz-arch-camera",
    },
};

/* stop no dev release warning */
static void jz_arch_camera_dev_release(struct device *dev){}

struct platform_device jz_arch_camera_device = {
    .name = "jz-arch-camera",
    .dev  = {
        .release = jz_arch_camera_dev_release,
    },
};

static int __init jz_arch_camera_init(void)
{
    int ret = platform_device_register(&jz_arch_camera_device);
    if (ret)
        return ret;

    return platform_driver_register(&jz_arch_camera_driver);
}
module_init(jz_arch_camera_init);

static void __exit jz_arch_camera_exit(void)
{
    platform_device_unregister(&jz_arch_camera_device);

    platform_driver_unregister(&jz_arch_camera_driver);
}
module_exit(jz_arch_camera_exit);

MODULE_DESCRIPTION("Ingenic X2000 Camera Driver");
MODULE_LICENSE("GPL");