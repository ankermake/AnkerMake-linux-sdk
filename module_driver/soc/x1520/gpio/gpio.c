#include <linux/gpio.h>
#include <linux/interrupt.h>
#include <linux/irq.h>
#include <linux/miscdevice.h>
#include <common.h>
#include <linux/list.h>
#include <linux/ioctl.h>
#include <linux/fs.h>
#include <linux/module.h>
#include <linux/mutex.h>
#include <linux/spinlock.h>
#include <linux/string.h>
#include <soc/base.h>
#include <bit_field.h>

#include <utils/gpio.h>
#include "gpio_regs.h"

static DEFINE_SPINLOCK(spinlock);
static DEFINE_MUTEX(lock);

static int debug;
module_param(debug, int, 0644);

static const unsigned long gpiobase[] = {
    [GPIO_PORT_A] = (unsigned long)CKSEG1ADDR(GPIO_IOBASE + 0 * GPIO_PORT_OFF),
    [GPIO_PORT_B] = (unsigned long)CKSEG1ADDR(GPIO_IOBASE + 1 * GPIO_PORT_OFF),
    [GPIO_PORT_C] = (unsigned long)CKSEG1ADDR(GPIO_IOBASE + 2 * GPIO_PORT_OFF),
    [SHADOW] = (unsigned long)CKSEG1ADDR(GPIO_IOBASE + GPIO_SHADOW_OFF),
};

#define GPIO_ADDR(port, reg) ((volatile unsigned long *)(gpiobase[port] + reg))

static inline void gpio_write(enum gpio_port port, unsigned int reg, int val)
{
    *GPIO_ADDR(port, reg) = val;
}

static inline unsigned int gpio_read(enum gpio_port port, unsigned int reg)
{
    return *GPIO_ADDR(port, reg);
}

static enum gpio_function gpio_get_func(int gpio)
{
    enum gpio_function function;

    enum gpio_port port = gpio / 32;
    unsigned int pin = gpio % 32;

    unsigned long flags;

    spin_lock_irqsave(&spinlock, flags);

    unsigned int intr = !!(gpio_read(port, PXINT) & (1 << pin));
    unsigned int mask = !!(gpio_read(port, PXMSK) & (1 << pin));
    unsigned int pat1 = !!(gpio_read(port, PXPAT1) & (1 << pin));
    unsigned int pat0 = !!(gpio_read(port, PXPAT0) & (1 << pin));

    unsigned int func = intr << 3 | mask << 2 | pat1 << 1 | pat0;

    unsigned int pull = !!(gpio_read(port, PXPEN) & (1 << pin));

    function = ((func | 0x10) & 0x1f)| \
               (((pull << 5) | 0x80) & 0xe0);

    spin_unlock_irqrestore(&spinlock, flags);

    return function;
}

static int m_gpio_get_value(int gpio)
{
    enum gpio_port port = gpio / 32;
    unsigned int pin = gpio % 32;

    return !!(gpio_read(port, PXPIN) & (1 << pin));
}

#define CMD_gpio_set_func                _IO('G', 120)
#define CMD_gpio_get_func                _IO('G', 121)
#define CMD_gpio_get_value               _IO('G', 122)
#define CMD_gpio_get_help                _IO('G', 123)
#define CMD_gpio_get_func2               _IO('G', 124)

struct func_name {
    int func;
    const char *name;
};

static struct func_name common_func_names[] = {
    {GPIO_FUNC_0, "func0"},
    {GPIO_FUNC_1, "func1"},
    {GPIO_FUNC_2, "func2"},
    {GPIO_FUNC_3, "func3"},
    {GPIO_OUTPUT0, "output0"},
    {GPIO_OUTPUT1, "output1"},
    {GPIO_INPUT, "input"},
    {GPIO_INT_LO, "int_lo"},
    {GPIO_INT_HI, "int_hi"},
    {GPIO_INT_FE, "int_fe"},
    {GPIO_INT_RE, "int_re"},
    {GPIO_INT_MASK_LO, "int_lo_m"},
    {GPIO_INT_MASK_HI, "int_hi_m"},
    {GPIO_INT_MASK_FE, "int_fe_m"},
    {GPIO_INT_MASK_RE, "int_re_m"},
    {0, NULL},
};

static struct func_name pull_func_names[] = {
    {GPIO_PULL_HIZ, "pull_hiz"},
    {GPIO_PULL_ENABLE, "pull"},
    {0, NULL},
};

static int to_gpio_func(const char *str, struct func_name *funcs)
{
    int i;

    for (i = 0; funcs[i].name != NULL; i++) {
        if (!strcmp(funcs[i].name, str))
            return funcs[i].func;
    }

    return -1;
}

static int str_to_gpio_func(const char *str)
{
    int func;

    func = to_gpio_func(str, common_func_names);
    if (func != -1)
        return func;

    func = to_gpio_func(str, pull_func_names);
    if (func != -1)
        return func;

    return -1;
}

static int dump_func(
 unsigned int func, char *buf, unsigned int size,
 const char *prefix, struct func_name *funcs)
{
    int i;

    for (i = 0; funcs[i].name != NULL; i++) {
        if (funcs[i].func == func) {
            int len = strlen(prefix) + strlen(funcs[i].name) + 1;
            if (len <= size)
                return sprintf(buf, "%s%s", prefix, funcs[i].name);
            return 0;
        }
    }

    return 0;
}

static void gpio_dump_func(int gpio, char *buf, unsigned int size)
{
    int len;
    unsigned int func = gpio_get_func(gpio);

    len = dump_func(func & 0x1f, buf, size, "", common_func_names);
    if (!len)
        return;

    size -= len; buf += len;
    len = dump_func(func & 0xe0, buf, size, " ", pull_func_names);
    if (!len)
        return;

    return;
}

static void gpio_dump_func2(int gpio, char *buf[], unsigned int count, unsigned int size)
{
    unsigned int func = gpio_get_func(gpio);

    if (count >= 1)
        dump_func(func & 0x1f, buf[0], size, "", common_func_names);

    if (count >= 2)
        dump_func(func & 0xe0, buf[1], size, "", pull_func_names);
}

static void gpio_dump_help(char *buf, unsigned int size)
{
    snprintf(buf, size, "%s",
        " common_func:\n"
        "    func0: device func0\n"
        "    func1: device func0\n"
        "    func2: device func0\n"
        "    func3: device func0\n"
        "    output0: output 0\n"
        "    output1: output 1\n"
        "    input: input mode\n"
        "    int_lo: interrupt by low level\n"
        "    int_hi: interrupt by high level\n"
        "    int_fe: interrupt by falling edge\n"
        "    int_re: interrupt by rising edge\n"
        "    int_lo_m: interrupt by low level, interrupt is masked\n"
        "    int_hi_m: interrupt by high level, interrupt is masked\n"
        "    int_fe_m: interrupt by falling edge, interrupt is masked\n"
        "    int_re_m: interrupt by rising edge, interrupt is masked\n"
        " pull_func:\n"
        "    pull_hiz: no pull, keep hiz\n"
        "    pull: only support one pull state，up or down see gpio pad_type\n"
        );
}

static inline int check_gpio(int gpio)
{
    return 0 <= gpio && gpio < 3*32;
}

static inline int check_func(int save_func, int func, int flag)
{
    return !((save_func & flag) && (func & flag));
}

static long gpio_ioctl(struct file *filp, unsigned int cmd, unsigned long arg)
{
    switch (cmd) {
    case CMD_gpio_set_func: {
        unsigned long *data = (void *)arg;
        if (IS_ERR(data))
            return -EINVAL;

        char *gpio_str = (void *)data[0];
        if (IS_ERR(gpio_str))
            return -EINVAL;

        int gpio = str_to_gpio(gpio_str);
        if (!check_gpio(gpio)) {
            printk(KERN_ERR "gpio: %s is not valid\n", gpio_str);
            return -EINVAL;
        }

        int count = data[1];
        int save_func = 0;

        int i;
        for (i = 0; i < count; i++) {
            const char *str = (void *)data[2 + i];
            if (IS_ERR(str))
                return -EINVAL;

            if (strcmp(str, "pull_up") == 0 || strcmp(str, "pull_down") == 0) {
                printk(KERN_ERR "gpio: pelese change to set pull ! X1000 only support one pull state，up or down see gpio pad_type!\n");
                return -EINVAL;
            }

            int func = str_to_gpio_func(str);
            if (func == -1) {
                printk(KERN_ERR "gpio: error unknown func: %s\n", str);
                return -EINVAL;
            }

            if (GPIO_INT_LO <= func && func <= GPIO_INT_MASK_RE) {
                printk(KERN_ERR "gpio: can't set as irq func: %s\n", str);
                return -EINVAL;
            }

            if (!check_func(save_func, func, GPIO_FUNC_0)) {
                printk(KERN_ERR "gpio: %s func double set\n", "common");
                return -EINVAL;
            }

            if (!check_func(save_func, func, GPIO_PULL_FUNC)) {
                printk(KERN_ERR "gpio: %s func double set\n", "pull");
                return -EINVAL;
            }

            save_func |= func;
        }

        if (debug)
            printk(KERN_ERR "gpio: set func %d %x\n", gpio, save_func);

        gpio_set_func(gpio, save_func);

        return 0;
    }

    case CMD_gpio_get_func: {
        unsigned long *data = (void *)arg;
        if (IS_ERR(data))
            return -EINVAL;

        char *gpio_str = (void *)data[0];
        if (IS_ERR(gpio_str))
            return -EINVAL;

        char *buf = (void *)data[1];
        unsigned int size = data[2];
        if (IS_ERR(buf))
            return -EINVAL;

        int gpio = str_to_gpio(gpio_str);
        if (!check_gpio(gpio)) {
            printk(KERN_ERR "gpio: %s is not valid\n", gpio_str);
            return -EINVAL;
        }

        if (size < 2)
            return -EINVAL;

        gpio_dump_func(gpio, buf, size);

        return 0;
    }

    case CMD_gpio_get_func2: {
        unsigned long *data = (void *)arg;
        if (IS_ERR(data))
            return -EINVAL;

        char *gpio_str = (void *)data[0];
        if (IS_ERR(gpio_str))
            return -EINVAL;

        char **buf = (void *)data[1];
        unsigned int count = data[2];
        unsigned int size = data[3];
        if (IS_ERR(buf))
            return -EINVAL;

        int gpio = str_to_gpio(gpio_str);
        if (!check_gpio(gpio)) {
            printk(KERN_ERR "gpio: %s is not valid\n", gpio_str);
            return -EINVAL;
        }

        if (size < 2)
            return -EINVAL;

        if (count < 1)
            return -EINVAL;

        int i;
        for (i = 0; i < count; i++) {
            if (IS_ERR(buf[i]))
                return -EINVAL;
        }

        gpio_dump_func2(gpio, buf, count, size);

        return count < 2 ? count : 2;
    }

    case CMD_gpio_get_value: {
        char *gpio_str = (void *)arg;
        if (IS_ERR(gpio_str))
            return -EINVAL;

        int gpio = str_to_gpio(gpio_str);
        if (!check_gpio(gpio)) {
            printk(KERN_ERR "gpio: %s is not valid\n", gpio_str);
            return -EINVAL;
        }

        return m_gpio_get_value(gpio);
    }

    case CMD_gpio_get_help: {
        unsigned long *data = (void *)arg;
        if (IS_ERR(data))
            return -EINVAL;

        char *buf = (void *)data[0];
        unsigned int size = data[1];
        if (IS_ERR(buf))
            return -EINVAL;

        if (size < 2)
            return -EINVAL;

        gpio_dump_help(buf, size);

        return 0;
    }

    default:
        printk(KERN_ERR "gpio: do not support this cmd: %x\n", cmd);
        break;
    }

    return -ENODEV;
}

static int gpio_open(struct inode *inode, struct file *filp)
{
    return 0;
}

static int gpio_release(struct inode *inode, struct file *filp)
{
    return 0;
}

static struct file_operations gpio_misc_fops = {
    .open = gpio_open,
    .release = gpio_release,
    .unlocked_ioctl = gpio_ioctl,
};

static struct miscdevice gpio_mdev = {
    .minor = MISC_DYNAMIC_MINOR,
    .name = "gpio",
    .fops = &gpio_misc_fops,
};

static int gpio_init(void)
{
    int ret;

    ret = misc_register(&gpio_mdev);
    assert(!ret);

    return 0;
}

static void gpio_exit(void)
{
    int ret;

    ret = misc_deregister(&gpio_mdev);
    assert(!ret);
}

module_init(gpio_init);

module_exit(gpio_exit);

MODULE_DESCRIPTION("JZ x1520 gpio driver");
MODULE_LICENSE("GPL");
