#include <linux/gpio.h>
#include <soc/gpio.h>
#include "x1021_fb_linux.h"

#define GPIO_SLCD_WR GPIO_PB(15)
#define GPIO_SLCD_TE GPIO_PB(16)
#define GPIO_SLCD_DC GPIO_PB(18)
#define GPIO_SCLD_RDY GPIO_PB(28)

#define GPIO_SLCD_D0 GPIO_PB(6)
#define GPIO_SLCD_D1 GPIO_PB(7)
#define GPIO_SLCD_D2 GPIO_PB(8)
#define GPIO_SLCD_D3 GPIO_PB(9)
#define GPIO_SLCD_D4 GPIO_PB(10)
#define GPIO_SLCD_D5 GPIO_PB(11)
#define GPIO_SLCD_D6 GPIO_PB(13)
#define GPIO_SLCD_D7 GPIO_PB(14)

void x1021_init_slcd_gpio_pb(struct jzfb *jzfb)
{
    jz_gpio_set_func(GPIO_SLCD_WR , GPIO_FUNC_3);
    jz_gpio_set_func(GPIO_SLCD_DC , GPIO_FUNC_3);

    if (jzfb->pdata->config.te_pin == TE_LCDC_TRIGGER)
        jz_gpio_set_func(GPIO_SLCD_TE , GPIO_FUNC_3);

    if (jzfb->pdata->config.enable_rdy_pin)
        jz_gpio_set_func(GPIO_SCLD_RDY, GPIO_FUNC_3);
 
    jz_gpio_set_func(GPIO_SLCD_D0 , GPIO_FUNC_3);
    jz_gpio_set_func(GPIO_SLCD_D1 , GPIO_FUNC_3);
    jz_gpio_set_func(GPIO_SLCD_D2 , GPIO_FUNC_3);
    jz_gpio_set_func(GPIO_SLCD_D3 , GPIO_FUNC_3);
    jz_gpio_set_func(GPIO_SLCD_D4 , GPIO_FUNC_3);
    jz_gpio_set_func(GPIO_SLCD_D5 , GPIO_FUNC_3);
    jz_gpio_set_func(GPIO_SLCD_D6 , GPIO_FUNC_3);
    jz_gpio_set_func(GPIO_SLCD_D7 , GPIO_FUNC_3);
}