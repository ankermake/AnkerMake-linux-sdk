#include <linux/platform_device.h>
#include <linux/gpio.h>
#include <mach/jzmmc.h>
#include <linux/bcm_pm_core.h>
#include <linux/delay.h>

#include "board_base.h"

/*For WiFi*/

#define RESET                           0
#define NORMAL                          1

extern int jzmmc_manual_detect(int index, int on);
extern int jzmmc_clk_ctrl(int index, int on);
extern int bcm_power_on(void);
extern int bcm_power_down(void);

struct wifi_data {
    struct wake_lock wifi_wake_lock;
    int wifi_reset;
};


static struct wifi_data bcm_data;

struct resource wlan_resources[] = {
    [0] = {
        .start          = WL_WAKE_HOST,
        .end            = WL_WAKE_HOST,
        .name           = "bcmdhd_wlan_irq",
        .flags          = IORESOURCE_IRQ | IORESOURCE_IRQ_HIGHLEVEL | IORESOURCE_IRQ_SHAREABLE,
    },
};

struct platform_device wlan_device = {
    .name               = "bcmdhd_wlan",
    .id                 = 1,
    .dev = {
        .platform_data  = NULL,
    },
    .resource           = wlan_resources,
    .num_resources      = ARRAY_SIZE(wlan_resources),
};


static void wifi_le_set_io(void)
{
    /*when wifi is down, set WL_MSC1_D0 , WL_MSC1_D1, WL_MSC1_D2, WL_MSC1_D3,
      WL_MSC1_CLK, WL_MSC1_CMD pins to INPUT_NOPULL status*/
    jzgpio_set_func(GPIO_PORT_C, GPIO_INPUT, 0x1 << 2);
    jzgpio_set_func(GPIO_PORT_C, GPIO_INPUT, 0x1 << 3);
    jzgpio_set_func(GPIO_PORT_C, GPIO_INPUT, 0x1 << 4);
    jzgpio_set_func(GPIO_PORT_C, GPIO_INPUT, 0x1 << 5);
    jzgpio_set_func(GPIO_PORT_C, GPIO_INPUT, 0x1 << 6);
    jzgpio_set_func(GPIO_PORT_C, GPIO_INPUT, 0x1 << 7);
}

static void wifi_le_restore_io(void)
{
    /*when wifi is up ,set WL_MSC1_D0 , WL_MSC1_D1, WL_MSC1_D2, WL_MSC1_D3,
         WL_MSC1_CLK, WL_MSC1_CMD pins to GPIO_FUNC_0*/
    jzgpio_set_func(GPIO_PORT_C, GPIO_FUNC_0, 0x1 << 2);
    jzgpio_set_func(GPIO_PORT_C, GPIO_FUNC_0, 0x1 << 3);
    jzgpio_set_func(GPIO_PORT_C, GPIO_FUNC_0, 0x1 << 4);
    jzgpio_set_func(GPIO_PORT_C, GPIO_FUNC_0, 0x1 << 5);
    jzgpio_set_func(GPIO_PORT_C, GPIO_FUNC_0, 0x1 << 6);
    jzgpio_set_func(GPIO_PORT_C, GPIO_FUNC_0, 0x1 << 7);
}

int bcm_wlan_init(void)
{
    int reset;
    int ret = 0;
    static int wl_reg_flag = 0;
    wifi_le_set_io();

#ifdef WL_REG_EN
    if (wl_reg_flag != 0) {
        return 0;
    }
    ret = gpio_request(WL_REG_EN, "wl_reg_on-123");
    if (ret <0 ) {
        pr_err("wifi_reg on pin request failed.\n");
        return -EINVAL;
    } else {
        gpio_direction_output(WL_REG_EN, 1);
    }
    wl_reg_flag = 1;
#endif

#if defined(HOST_WIFI_RST)
    reset = HOST_WIFI_RST;
    if (gpio_request(HOST_WIFI_RST, "wifi_reset")) {
        pr_err("no wifi_reset pin available\n");

        return -EINVAL;
    } else {
        gpio_direction_output(reset, 1);
    }
#else
    reset = -1;
#endif
    bcm_data.wifi_reset = reset;

	return 0;
}
EXPORT_SYMBOL(bcm_wlan_init);

int bcm_customer_wlan_get_oob_irq(void)
{
    int host_oob_irq = 0;
    //printk("GPIO(WL_HOST_WAKE) = EXYNOS4_GPX0(7) = %d\n", GPIO_PA(9));
    gpio_request(WL_WAKE_HOST, "oob irq");
    host_oob_irq = gpio_to_irq(WL_WAKE_HOST);
    // gpio_direction_input(GPIO_PA(9));
    printk("host_oob_irq: %d \r\n", host_oob_irq);

    return host_oob_irq;
}
EXPORT_SYMBOL(bcm_customer_wlan_get_oob_irq);

int bcm_manual_detect(int on)
{
    jzmmc_manual_detect(WL_MMC_NUM, on);

    return 0;
}
EXPORT_SYMBOL(bcm_manual_detect);

int bcm_wlan_power_on(int flag)
{
    static struct wake_lock	*wifi_wake_lock = &bcm_data.wifi_wake_lock;
#ifdef WL_REG_EN
    int wl_reg_on	= WL_REG_EN;
#endif

#ifdef WL_RST_EN
    int reset = bcm_data.wifi_reset;
#endif

    if (wifi_wake_lock == NULL)
        pr_warn("%s: invalid wifi_wake_lock\n", __func__);
#ifdef WL_RST_EN
    else if (!gpio_is_valid(reset))
        pr_warn("%s: invalid reset\n", __func__);
#endif
    else
        goto start;

    return -ENODEV;
start:
    printk("wlan power on:%d\n", flag);
    wifi_le_restore_io();
    bcm_power_on();

    msleep(200);

    switch(flag) {
        case RESET:
#ifdef WL_REG_EN
            gpio_direction_output(wl_reg_on,1);
            msleep(200);
#endif
            msleep(200);
#ifdef WL_RST_EN
            gpio_direction_output(reset, 0);
            msleep(200);
            gpio_direction_output(reset, 1);
            msleep(200);
#endif
            break;

        case NORMAL:
            msleep(200);
#ifdef WL_REG_EN
            gpio_direction_output(wl_reg_on, 1);
            msleep(200);
#endif
#ifdef WL_RST_EN
            gpio_direction_output(reset, 0);
            msleep(200);
            gpio_direction_output(reset, 1);
            msleep(200);
#endif
            bcm_manual_detect(1);
            break;
    }

    return 0;
}
EXPORT_SYMBOL(bcm_wlan_power_on);

int bcm_wlan_power_off(int flag)
{
    static struct wake_lock	*wifi_wake_lock = &bcm_data.wifi_wake_lock;
#ifdef WL_REG_EN
    int wl_reg_on = WL_REG_EN;
#endif
#ifdef WL_RST_EN
    int reset = bcm_data.wifi_reset;
#endif

    if (wifi_wake_lock == NULL)
        pr_warn("%s: invalid wifi_wake_lock\n", __func__);
#ifdef WL_RST_EN
    else if (!gpio_is_valid(reset))
        pr_warn("%s: invalid reset\n", __func__);
#endif
    else
        goto start;

    return -ENODEV;
start:
    printk("wlan power off:%d\n", flag);
    switch(flag) {
        case RESET:
#ifdef WL_REG_EN
            gpio_direction_output(wl_reg_on,0);
#endif
#ifdef WL_RST_EN
            gpio_direction_output(reset, 0);
#endif
            msleep(200);
            break;

        case NORMAL:
#ifdef WL_RST_EN
            gpio_direction_output(reset, 0);
#endif
            udelay(65);

            /*
             *  control wlan reg on pin
             */
#ifdef WL_REG_EN
            gpio_direction_output(wl_reg_on,0);
#endif
            msleep(200);
            break;
    }

    //	wake_unlock(wifi_wake_lock);

    bcm_power_down();
    wifi_le_set_io();

    return 0;
}
EXPORT_SYMBOL(bcm_wlan_power_off);



void wlan_pw_en_enable(void)
{
    bcm_power_on();
}

void wlan_pw_en_disable(void)
{
    bcm_power_down();
}

