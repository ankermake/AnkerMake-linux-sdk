#include <linux/kernel.h>
#include <linux/sched.h>
#include <linux/vmalloc.h>
#include <linux/module.h>
#include <soc/gpio.h>

EXPORT_SYMBOL(init_mm);
EXPORT_SYMBOL(get_vm_area);

EXPORT_SYMBOL(jz_gpio_save_reset_func);
EXPORT_SYMBOL(jz_gpio_restore_func);

