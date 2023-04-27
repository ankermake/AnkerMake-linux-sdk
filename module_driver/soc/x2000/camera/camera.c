/*
 * Copyright (C) 2020 Ingenic Semiconductor Co., Ltd.
 *
 * Camera Driver for the Ingenic controller
 *
 */
#include <linux/module.h>
#include <linux/delay.h>

#include <utils/clock.h>

#include "vic.h"


static int jz_arch_camera_init(void)
{
    jz_arch_vic_init();

    return 0;
}


static void jz_arch_camera_exit(void)
{
    jz_arch_vic_exit();
}

module_init(jz_arch_camera_init);
module_exit(jz_arch_camera_exit);


MODULE_DESCRIPTION("Ingenic X2000 Camera Driver");
MODULE_LICENSE("GPL");
