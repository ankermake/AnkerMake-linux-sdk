#include <linux/gpio.h>
#include <linux/irq.h>
#include <linux/interrupt.h>
#include <linux/delay.h>
#include <gsl1680_pdata.h>

static struct gsl1680_platform_data *gsl1680_pdata = NULL;

static int ctp_i2c_test(struct i2c_client *client)
{
    return 1;
}

void ctp_wakeup(int wakeup, int what)
{
    if (wakeup) {
        gpio_direction_output(gsl1680_pdata->gpio_shutdown, 1);
        msleep(5);
    } else {
        gpio_direction_output(gsl1680_pdata->gpio_shutdown, 0);
        msleep(5);
    }
}

void sw_gpio_eint_set_enable(int irq, int enable)
{
    if (enable)
        enable_irq(irq);
    else
        disable_irq_nosync(irq);
}