#include <linux/init.h>
#include <linux/module.h>
#include <linux/gpio.h>
#include <linux/kernel.h>
#include <linux/miscdevice.h>
#include <linux/slab.h>
#include <linux/kthread.h>
#include <linux/delay.h>
#include <linux/mutex.h>
#include <linux/interrupt.h>
#include <linux/irq.h>
#include <linux/workqueue.h>
#include <linux/wait.h>
#include <assert.h>

#include <utils/gpio.h>
#include <utils/clock.h>

struct gpio_ps2_data {
    unsigned int id;
    unsigned int is_enable;
    unsigned int is_recv;
    unsigned int is_enable_irq;

    unsigned int clk_gpio;
    unsigned int data_gpio;

    char *dev_name;
    struct miscdevice ps2_mdev;

    spinlock_t spin_lock;
    struct mutex mutex_lock;
    wait_queue_head_t recv_wait;
};

static struct gpio_ps2_data ps2_dev[2] = {
    {
        .dev_name = "ps2_0",
    },
    {
        .dev_name = "ps2_1",
    }
};

module_param_named(ps2_0_is_enable, ps2_dev[0].is_enable, int, 0644);
module_param_gpio_named(ps2_0_clk_gpio, ps2_dev[0].clk_gpio, 0644);
module_param_gpio_named(ps2_0_data_gpio, ps2_dev[0].data_gpio, 0644);

module_param_named(ps2_1_is_enable, ps2_dev[1].is_enable, int, 0644);
module_param_gpio_named(ps2_1_clk_gpio, ps2_dev[1].clk_gpio, 0644);
module_param_gpio_named(ps2_1_data_gpio, ps2_dev[1].data_gpio, 0644);

#define MCU_MAGIC_NUMBER    'P'

#define CMD_read_raw_byte           _IOW(MCU_MAGIC_NUMBER, 110, void *)
#define CMD_read_str            _IOW(MCU_MAGIC_NUMBER, 111, void *)
#define CMD_write_raw_byte          _IOW(MCU_MAGIC_NUMBER, 112, void *)
#define CMD_write_str           _IOW(MCU_MAGIC_NUMBER, 113, void *)

enum ps2_key_code_type {
    KEY_MAKE_CODE = 1,
    KEY_BREAK_CODE,
};

struct ps2_scan_code {
    unsigned char m[KEY_MAKE_CODE];
    unsigned char b[KEY_BREAK_CODE];
};

#define SHIFT_KEY_CODE 16

static unsigned char shift_code_table[] = {
    ['!'] = '1',
    ['@'] = '2',
    ['#'] = '3',
    ['$'] = '4',
    ['%'] = '5',
    ['^'] = '6',
    ['&'] = '7',
    ['*'] = '8',
    ['('] = '9',
    [')'] = '0',
    ['_'] = '-',
    ['+'] = '=',
    ['{'] = '[',
    ['}'] = ']',
    ['|'] = '\\',
    ['~'] = '`',
    ['?'] = '/',
    ['<'] = ',',
    ['>'] = '.',
    ['A'] = 'a',
    ['B'] = 'b',
    ['C'] = 'c',
    ['D'] = 'd',
    ['E'] = 'e',
    ['F'] = 'f',
    ['G'] = 'g',
    ['H'] = 'h',
    ['I'] = 'i',
    ['J'] = 'j',
    ['K'] = 'k',
    ['L'] = 'l',
    ['M'] = 'm',
    ['N'] = 'n',
    ['O'] = 'o',
    ['P'] = 'p',
    ['Q'] = 'q',
    ['R'] = 'r',
    ['S'] = 's',
    ['T'] = 't',
    ['U'] = 'u',
    ['V'] = 'v',
    ['W'] = 'w',
    ['X'] = 'x',
    ['Y'] = 'y',
    ['Z'] = 'z',
};

static struct ps2_scan_code code_table[] = {
    ['a'] = {.m = {0x1c},.b = {0xf0,0x1c,}},
    ['b'] = {.m = {0x32},.b = {0xf0,0x32,}},
    ['c'] = {.m = {0x21},.b = {0xf0,0x21,}},
    ['d'] = {.m = {0x23},.b = {0xf0,0x23,}},
    ['e'] = {.m = {0x24},.b = {0xf0,0x24,}},
    ['f'] = {.m = {0x2b},.b = {0xf0,0x2b,}},
    ['g'] = {.m = {0x34},.b = {0xf0,0x34,}},
    ['h'] = {.m = {0x33},.b = {0xf0,0x33,}},
    ['i'] = {.m = {0x43},.b = {0xf0,0x43,}},
    ['j'] = {.m = {0x3b},.b = {0xf0,0x3b,}},
    ['k'] = {.m = {0x42},.b = {0xf0,0x42,}},
    ['l'] = {.m = {0x4b},.b = {0xf0,0x4b,}},
    ['m'] = {.m = {0x3a},.b = {0xf0,0x3a,}},
    ['n'] = {.m = {0x31},.b = {0xf0,0x31,}},
    ['o'] = {.m = {0x44},.b = {0xf0,0x44,}},
    ['p'] = {.m = {0x4d},.b = {0xf0,0x4d,}},
    ['q'] = {.m = {0x15},.b = {0xf0,0x15,}},
    ['r'] = {.m = {0x2d},.b = {0xf0,0x2d,}},
    ['s'] = {.m = {0x1b},.b = {0xf0,0x1b,}},
    ['t'] = {.m = {0x2c},.b = {0xf0,0x2c,}},
    ['u'] = {.m = {0x3c},.b = {0xf0,0x3c,}},
    ['v'] = {.m = {0x2a},.b = {0xf0,0x2a,}},
    ['w'] = {.m = {0x1d},.b = {0xf0,0x1d,}},
    ['x'] = {.m = {0x22},.b = {0xf0,0x22,}},
    ['y'] = {.m = {0x35},.b = {0xf0,0x35,}},
    ['z'] = {.m = {0x1a},.b = {0xf0,0x1a,}},
    ['0'] = {.m = {0x45},.b = {0xf0,0x45,}},
    ['1'] = {.m = {0x16},.b = {0xf0,0x16,}},
    ['2'] = {.m = {0x1e},.b = {0xf0,0x1e,}},
    ['3'] = {.m = {0x26},.b = {0xf0,0x26,}},
    ['4'] = {.m = {0x25},.b = {0xf0,0x25,}},
    ['5'] = {.m = {0x2e},.b = {0xf0,0x2e,}},
    ['6'] = {.m = {0x36},.b = {0xf0,0x36,}},
    ['7'] = {.m = {0x3d},.b = {0xf0,0x3d,}},
    ['8'] = {.m = {0x3e},.b = {0xf0,0x3e,}},
    ['9'] = {.m = {0x46},.b = {0xf0,0x46,}},
    ['`'] = {.m = {0x0e},.b = {0xf0,0x0e,}},
    ['-'] = {.m = {0x4e},.b = {0xf0,0x4e,}},
    ['='] = {.m = {0x55},.b = {0xf0,0x55,}},
    ['\\'] = {.m = {0x5d},.b = {0xf0,0x5d,}},
    [']'] = {.m = {0x5b},.b = {0xf0,0x5b,}},
    [';'] = {.m = {0x4c},.b = {0xf0,0x4c,}},
    ['\''] = {.m = {0x52},.b = {0xf0,0x52,}},
    [','] = {.m = {0x41},.b = {0xf0,0x41,}},
    ['.'] = {.m = {0x49},.b = {0xf0,0x49,}},
    ['/'] = {.m = {0x4a},.b = {0xf0,0x4a,}},
    ['['] = {.m = {0x54},.b = {0xf0,0x54,}},
    ['\r'] = {.m = {0x5a},.b = {0xf0,0x5a,}},
    ['\n'] = {.m = {0x5a},.b = {0xf0,0x5a,}},
    [8] = {.m = {0x66},.b = {0xf0,0x66,}},    /* BKSP */
    [32] = {.m = {0x29},.b = {0xf0,0x29,}},   /* SPACE */
    [9] = {.m = {0x0d},.b = {0xf0,0x0d,}},    /* TAB */
    [20] = {.m = {0x58},.b = {0xf0,0x58,}},   /* CAPS */
    [SHIFT_KEY_CODE] = {.m = {0x12},.b = {0xf0,0x12,}},   /* SHIFT */
    [17] = {.m = {0x14},.b = {0xf0,0x14,}},   /* CTRL */
    [18] = {.m = {0x11},.b = {0xf0,0x11,}},   /* ALT */
};

static int code_table_size = ARRAY_SIZE(code_table);
static int shift_code_table_size = ARRAY_SIZE(shift_code_table);

static inline int get_clk(int id)
{
    return gpio_get_value(ps2_dev[id].clk_gpio);
}

static inline void reset_clk(int id)
{
    udelay(20);
    gpio_direction_output(ps2_dev[id].clk_gpio, 0);
    udelay(40);
    gpio_direction_input(ps2_dev[id].clk_gpio);
    udelay(20);
}

static inline void set_clk(int id, int level)
{
    if (level)
        gpio_direction_input(ps2_dev[id].clk_gpio);
    else
        gpio_direction_output(ps2_dev[id].clk_gpio, 0);
}

static inline int get_data(int id)
{
    return gpio_get_value(ps2_dev[id].data_gpio);
}

static inline void set_data(int id, int level)
{
    if (level)
        gpio_direction_input(ps2_dev[id].data_gpio);
    else
        gpio_direction_output(ps2_dev[id].data_gpio, 0);
}

static int do_recv_byte(int id, unsigned char *data)
{
    unsigned long long timeout = 20 * 1000;
    unsigned long long start;

    /* 如果数据线为高,放弃传输 */
    if (get_data(id))
        return -1;

    /* 等待时钟线为高 */
    start = local_clock_us();
    while (!get_clk(id)) {
        if (local_clock_us() - start > timeout)
            return -ETIMEDOUT;
    }

    /* 如果数据仍为高,表示传输出错 */
    if (get_data(id))
        return -1;

    int data_val = 0;
    int cnt = 0;

    /* 读8位数据 */
    int i;
    for (i = 0; i < 8; i++) {
        reset_clk(id);

        if (!get_clk(id))
            return -1;

        int bit = get_data(id);
        if (bit)
            cnt++;

        /* 每读一个位都要判断时钟线状态,如果被拉低表传输出错 */
        if (!get_clk(id)) {
            printk(KERN_ERR "do_recv_byte err\n");
            return -1;
        }

        data_val |= (bit << i);
    }

    /* 读奇偶校验位 */
    reset_clk(id);

    if (!get_clk(id))
        return -1;

    int parity = get_data(id);
    if (parity) {
        if (cnt % 2) {
            // printk(KERN_ERR "parity err, parity = %d, cnt = %d\n", parity, cnt);
            return -1;
        }
    } else {
        if (!(cnt % 2)) {
            // printk(KERN_ERR "parity err, parity = %d, cnt = %d\n", parity, cnt);
            return -1;
        }
    }

    /* 读停止位 */
    reset_clk(id);

    if (!get_clk(id))
        return -1;

    int stop = get_data(id);
    if (!stop)
        return -1;

    if (!get_clk(id))
        return -1;

    /* 发送应答位 */
    udelay(15);
    set_data(id, 0);
    udelay(5);
    set_clk(id, 0);
    udelay(40);
    set_clk(id, 1);
    udelay(5);
    set_data(id, 1);

    *data = data_val;

    /* 给主机时间抑制下次的传输 */
    udelay(45);

    /* 再次检查时钟、数据线有没有恢复初始状态 */
    start = local_clock_us();
    while (!get_clk(id)) {
        if (local_clock_us() - start > timeout)
            return -ETIMEDOUT;
    }

    start = local_clock_us();
    while (!get_data(id)) {
        if (local_clock_us() - start > timeout)
            return -ETIMEDOUT;
    }

    return 0;
}

static int do_send_byte(int id, unsigned char data)
{
    unsigned long long timeout = 20*1000;
    unsigned long long start;

    /* 如果数据线为低,表示主机将发送数据到从机,进入接收程序 */
    if (!get_data(id))
        goto do_recv;

    udelay(40);

    /* 发送起始位0 */
    set_data(id, 0);

    /* 每写一个位都要判断时钟线状态,如果被拉低表传输出错,进入接收程序 */
    if (!get_clk(id))
        goto do_recv;

    reset_clk(id);

    int i;
    int cnt = 0;
    /* 写8位数据 */
    for (i = 0; i < 8; i++) {
        int bit = (data >> i) & 1;
        if (bit)
            cnt++;

        set_data(id, bit);

        if (!get_clk(id))
            goto do_recv;

        reset_clk(id);
    }

    /* 写奇偶校验位 */
    int parity = (cnt % 2) ? 0 : 1;
    set_data(id, parity);

    if (!get_clk(id))
        goto do_recv;

    reset_clk(id);

    /* 写停止位1 */
    set_data(id, 1);

    udelay(30);

    set_clk(id, 0);

    /* 最后一个时钟的脉冲所需要的时间更长 */
    udelay(50);

    set_clk(id, 1);

    udelay(50);

    /* 再次检查时钟、数据线有没有恢复初始状态 */
    start = local_clock_us();
    while (!get_clk(id)) {
        if (local_clock_us() - start > timeout)
            return -ETIMEDOUT;
    }

    start = local_clock_us();
    while (!get_data(id)) {
        if (local_clock_us() - start > timeout)
            return -ETIMEDOUT;
    }

    return 0;

do_recv:
    set_data(id, 1);
    ps2_dev[id].is_recv = 1;
    wake_up(&ps2_dev[id].recv_wait);
    return -1;
}

static irqreturn_t ps2_recv_handler(int irq, void *data)
{
    int id = (int)data;
    ps2_dev[id].is_recv = 1;
    wake_up(&ps2_dev[id].recv_wait);

    return IRQ_HANDLED;
}

static void disable_recv_irq(int id)
{
    if (!ps2_dev[id].is_enable_irq)
        return;

    free_irq(gpio_to_irq(ps2_dev[id].clk_gpio), NULL);

    ps2_dev[id].is_enable_irq = 0;
}

static void enable_recv_irq(int id)
{
    if (ps2_dev[id].is_enable_irq)
        return;

    request_irq(gpio_to_irq(ps2_dev[id].clk_gpio), ps2_recv_handler, IRQF_TRIGGER_RISING, "gpio_ps2_clk", (void *)id);

    ps2_dev[id].is_enable_irq = 1;
}

static int recv_byte(int id, unsigned char *data, int timeout_ms)
{
    unsigned long flags;
    long long start;
    unsigned char ch;

    int ret = 0;
    int timeout = timeout_ms ? 1 : 0;
    if (timeout)
        timeout_ms = timeout_ms < 10 ? 10 : timeout_ms;

    long long timeout_us = timeout_ms * 1000;

    while (1) {
        if (timeout) {
            start = local_clock_us();
            wait_event_timeout(ps2_dev[id].recv_wait, ps2_dev[id].is_recv, msecs_to_jiffies(10));
        }

        spin_lock_irqsave(&ps2_dev[id].spin_lock, flags);

        disable_recv_irq(id);

        ps2_dev[id].is_recv = 0;
        ret = do_recv_byte(id, &ch);
        if (!ret)
            *data = ch;

        enable_recv_irq(id);

        spin_unlock_irqrestore(&ps2_dev[id].spin_lock, flags);

        if (!timeout)
            break;

        timeout_us = timeout_us - (local_clock_us() - start);
        if (timeout_us <= 0) {
            ret = -ETIMEDOUT;
            break;
        }
    }

    return ret;
}

static int send_byte(int id, unsigned char data)
{
    unsigned long flags;

    unsigned long long timeout_us = 20 * 1000;

    unsigned long long start = local_clock_us();

    int ret = 0;

    while (1) {
        if (get_clk(id))
            break;

        unsigned long long time = local_clock_us() - start;
        if (time > timeout_us) {
            return -ETIMEDOUT;
        }

        udelay(10);
    }

    spin_lock_irqsave(&ps2_dev[id].spin_lock, flags);

    disable_recv_irq(id);

    ret = do_send_byte(id, data);

    enable_recv_irq(id);

    spin_unlock_irqrestore(&ps2_dev[id].spin_lock, flags);

    return ret;
}

static int send_code(int id, void *code_, enum ps2_key_code_type type)
{
    int i;
    int ret = 0;

    unsigned char *code = (unsigned char *)code_;

    for (i = 0; i < type; i++) {
        if (!code[i])
            break;

        ret = send_byte(id, code[i]);
        if (ret < 0)
            break;
    }

    return ret;
}

static int send_data(int id, unsigned char data)
{
    struct ps2_scan_code *code;
    struct ps2_scan_code *shift_key = &code_table[SHIFT_KEY_CODE];

    int ret = 0;
    int is_need_shift_key = 0;

    if (data > code_table_size)
        return -1;

    if (data < shift_code_table_size && shift_code_table[data]) {
        data = shift_code_table[data];
        is_need_shift_key = 1;
    }

    if (is_need_shift_key)
        ret = send_code(id, shift_key->m, KEY_MAKE_CODE);

    code = &code_table[data];

    ret = ret ? ret : send_code(id, code->m, KEY_MAKE_CODE);
    ret = ret ? ret : send_code(id, code->b, KEY_BREAK_CODE);

    if (is_need_shift_key)
        ret = ret ? ret : send_code(id, shift_key->b, KEY_BREAK_CODE);

    return ret;
}

static int ps2_send_data(int id, unsigned char *buf, int count)
{
    int ret = 0;

    int i;
    for (i = 0; i < count; i++) {
        ret = send_data(id, *buf++);
        if (ret < 0) {
            printk(KERN_ERR "GPIO-PS2: send_data err, ret = %d\n", ret);
            break;
        }
    }

    return i;
}

struct m_private_data {
    unsigned int id;
};

static int ps2_open(struct inode *inode, struct file *filp)
{
    struct m_private_data *data = kmalloc(sizeof(*data), GFP_KERNEL);
    struct gpio_ps2_data *drv = container_of(filp->private_data,
            struct gpio_ps2_data, ps2_mdev);

    data->id = drv->id;
    filp->private_data = data;

    mutex_lock(&drv->mutex_lock);

    enable_recv_irq(drv->id);

    mutex_unlock(&drv->mutex_lock);

    return 0;
}

static int ps2_close(struct inode *inode, struct file *filp)
{
    struct m_private_data *data = filp->private_data;
    struct gpio_ps2_data *drv = &ps2_dev[data->id];

    mutex_lock(&drv->mutex_lock);

    disable_recv_irq(data->id);

    mutex_unlock(&drv->mutex_lock);

    kfree(data);

    return 0;
}

static int ps2_write(struct file *filp, const char __user *buffer, size_t count, loff_t *off)
{
    int ret = 0;
    struct m_private_data *data = filp->private_data;
    struct gpio_ps2_data *drv = &ps2_dev[data->id];

    unsigned char *buf = (unsigned char *)buffer;

    mutex_lock(&drv->mutex_lock);

    ret = ps2_send_data(data->id, buf, count);
    if (ret != count)
        printk(KERN_ERR "GPIO-PS2: write str err:%d.\n", ret);

    mutex_unlock(&drv->mutex_lock);

    return ret;
}

static long ps2_ioctl(struct file *filp, unsigned int cmd, unsigned long arg)
{
    int ret = 0;

    struct m_private_data *data = filp->private_data;
    int id = data->id;
    struct gpio_ps2_data *drv = &ps2_dev[id];

    mutex_lock(&drv->mutex_lock);

    switch (cmd) {
        case CMD_write_raw_byte: {
            unsigned char *ch = (void *)arg;
            ret = send_byte(id, *ch);
            break;
        }

        case CMD_write_str:{
            unsigned long *array = (void *)arg;
            void *src = (void *)array[0];
            int len = array[1];
            ret = ps2_send_data(id, src, len);
            break;
        }

        case CMD_read_raw_byte: {
            unsigned long *array = (void *) arg;
            unsigned char *byte = (unsigned char *)array[0];
            int timeout_ms = array[1];
            unsigned char ch;
            ret = recv_byte(id, &ch, timeout_ms);
            if (!ret)
                *byte = ch;

            break;
        }
        case CMD_read_str:
        default:ret = -1;break;

    }

    mutex_unlock(&drv->mutex_lock);

    return ret;
}

static struct file_operations ps2_fops = {
    .owner   = THIS_MODULE,
    .open    = ps2_open,
    .release = ps2_close,
    .write   = ps2_write,
    .unlocked_ioctl = ps2_ioctl,
};

static void ps2_res_init(int id)
{
    int ret;
    struct gpio_ps2_data *drv = &ps2_dev[id];
    char gpio_str[10];

    assert(drv->clk_gpio >= 0);
    assert(drv->data_gpio >= 0);

    ret = gpio_request(drv->clk_gpio,"ps2_gpio_clk");
    if (ret) {
        printk(KERN_ERR "GPIO-PS2: ps2_%d, failed to request clk pin: %s\n", id, gpio_to_str(drv->clk_gpio, gpio_str));
        return;
    }

    ret = gpio_request(drv->data_gpio,"ps2_data_gpio");
    if (ret) {
        printk(KERN_ERR "GPIO-PS2: ps2_%d, failed to request data pin: %s\n", id, gpio_to_str(drv->data_gpio, gpio_str));
        gpio_free(drv->clk_gpio);
        return;
    }

    gpio_direction_input(drv->clk_gpio);
    gpio_direction_input(drv->data_gpio);

    mutex_init(&drv->mutex_lock);
    spin_lock_init(&drv->spin_lock);
    init_waitqueue_head(&drv->recv_wait);

    drv->ps2_mdev.minor = MISC_DYNAMIC_MINOR;
    drv->ps2_mdev.name  = drv->dev_name;
    drv->ps2_mdev.fops  = &ps2_fops;

    ret = misc_register(&drv->ps2_mdev);
    if (ret < 0)
        panic("GPIO-PS2: %s register err!\n", drv->dev_name);
}

static void ps2_res_exit(int id)
{
    struct gpio_ps2_data *drv = &ps2_dev[id];

    misc_deregister(&drv->ps2_mdev);

    gpio_free(drv->clk_gpio);
    gpio_free(drv->data_gpio);
}

static int __init ps2_init(void)
{

    if (ps2_dev[0].is_enable)
        ps2_res_init(0);

    if (ps2_dev[1].is_enable)
        ps2_res_init(1);

    return 0;
}

static void __exit ps2_exit(void)
{
    if (ps2_dev[0].is_enable)
        ps2_res_exit(0);

    if (ps2_dev[1].is_enable)
        ps2_res_exit(1);
}

module_init(ps2_init);
module_exit(ps2_exit);
MODULE_LICENSE("GPL");