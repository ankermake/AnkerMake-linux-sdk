#include <linux/module.h>
#include <linux/init.h>
#include <linux/kernel.h>
#include <linux/workqueue.h>
#include <linux/device.h>
#include <linux/clk.h>
#include <linux/irq.h>
#include <linux/interrupt.h>
#include <soc/gpio.h>
#include <linux/gpio.h>
#include <utils/gpio.h>

#include <linux/sched.h>
#include <linux/wait.h>
#include "sslv_hal.h"
#include "sslv.h"
#include <assert.h>
#include <linux/miscdevice.h>
#include <common.h>

#define IRQ_SSI_SLV    (IRQ_INTC_BASE + 62)

#define MAX_FIFO_LEN 64

struct jz_func_alter {
    int pin;
    int function;
    const char *name;
};

struct jz_sslv_pin {
    struct jz_func_alter sslv_pins[4];
};

struct jz_sslv_drv {
    int id;
    int irq;
    int status;
    int is_enable;
    int is_finish;
    int sslv_dt;
    int sslv_dr;
    int sslv_clk;
    int sslv_cs;
    struct clk *clk;
    const char *irq_name;
    unsigned char *device_name;
    struct miscdevice sslv_mdev;

    int alter_num;
    struct jz_sslv_pin *alter_pin;

    int tlen;
    int rlen;
    void *tbuff;
    void *rbuff;

    wait_queue_head_t waiter;
    struct sslv_config_data *config;
    unsigned int working_flag;  //工作状态：1：繁忙，0：空闲
    struct sslv_device dev;
    void (*sslv_receive_callback)(struct sslv_device *dev);
};

static struct jz_sslv_pin jz_sslv_pin[2] = {
    {
        {
            {GPIO_PA(31), GPIO_FUNC_1, "sslv_clk"},
            {GPIO_PA(30), GPIO_FUNC_1, "sslv_dt"},
            {GPIO_PA(29), GPIO_FUNC_1, "sslv_dr"},
            {GPIO_PA(28), GPIO_FUNC_1, "sslv_cs"},
        },
    },

    {
        {
            {GPIO_PB(12), GPIO_FUNC_2, "sslv_clk"},
            {GPIO_PB(13), GPIO_FUNC_2, "sslv_dt"},
            {GPIO_PB(14), GPIO_FUNC_2, "sslv_dr"},
            {GPIO_PB(17), GPIO_FUNC_2, "sslv_cs"},
        },
    },
};

struct jz_sslv_drv jzsslv_dev[] = {
    {
        .id = 0,
        .irq = IRQ_SSI_SLV,
        .irq_name = "SSI_SLV",
        .device_name = "sslv0",
        .alter_pin = jz_sslv_pin,
        .alter_num = ARRAY_SIZE(jz_sslv_pin),
    }
};

module_param_named(sslv0_is_enable, jzsslv_dev[0].is_enable, int, 0644);
module_param_gpio_named(sslv0_dt, jzsslv_dev[0].sslv_dt, 0644);
module_param_gpio_named(sslv0_dr, jzsslv_dev[0].sslv_dr, 0644);
module_param_gpio_named(sslv0_clk, jzsslv_dev[0].sslv_clk, 0644);
module_param_gpio_named(sslv0_cs, jzsslv_dev[0].sslv_cs, 0644);

#define MCU_MAGIC_NUMBER    'S'

#define CMD_enable             _IOW(MCU_MAGIC_NUMBER, 110, void *)
#define CMD_disable            _IOW(MCU_MAGIC_NUMBER, 111, void *)
#define CMD_receive            _IOW(MCU_MAGIC_NUMBER, 112, void *)
#define CMD_send               _IOW(MCU_MAGIC_NUMBER, 113, void *)
#define CMD_get_info           _IOW(MCU_MAGIC_NUMBER, 114, void *)
#define CMD_set_mode           _IOW(MCU_MAGIC_NUMBER, 115, void *)
#define CMD_set_bits           _IOW(MCU_MAGIC_NUMBER, 116, void *)

static inline int gpio_init(int gpio, enum gpio_function func, const char *name)
{
    int ret = 0;
    ret = gpio_request(gpio, name);
    if (ret < 0)
        return ret;

    gpio_set_func(gpio, func);

    return 0;
}

static int m_gpio_request(int gpio, struct jz_sslv_drv *drv, int id)
{
    int i;
    int ret = 0;
    char buf[10];
    int num = drv->alter_num;
    struct jz_sslv_pin *sslv_pin_alter = drv->alter_pin;
    struct jz_func_alter *sslv_pin;

    if (gpio < 0)
        return 0;

    for (i = 0; i < num; i++) {
        sslv_pin = sslv_pin_alter[i].sslv_pins;
        if (gpio == sslv_pin[id].pin) {
            ret = gpio_init(gpio, sslv_pin[id].function, sslv_pin[id].name);
            if (ret < 0) {
                printk(KERN_ERR "[SSLV]err: failed to request %s: %s!\n", sslv_pin[id].name, gpio_to_str(gpio, buf));
                return -EINVAL;
            }

            return 0;
        }
    }

    printk(KERN_ERR "[SSLV]err: %s(%s) is invaild!\n", sslv_pin[id].name, gpio_to_str(gpio, buf));
    return -EINVAL;
}

static int sslv_gpio_request(struct jz_sslv_drv *drv)
{
    int ret = 0;
    int id = drv->id;

    if (drv->sslv_clk < 0) {
        printk(KERN_ERR "[SSLV]err: sslv%d sslv_clk must be set.\n", id);
        return -EINVAL;
    }

    if (drv->sslv_cs < 0) {
        printk(KERN_ERR "[SSLV]err: sslv%d sslv_cs must be set.\n", id);
        return -EINVAL;
    }

    ret = m_gpio_request(drv->sslv_clk, drv, 0);
    if (ret)
        return -EINVAL;

    ret = m_gpio_request(drv->sslv_dt, drv, 1);
    if (ret)
        goto err_dt;

    ret = m_gpio_request(drv->sslv_dr, drv, 2);
    if (ret)
        goto err_dr;

    ret = m_gpio_request(drv->sslv_cs, drv, 3);
    if (ret)
        goto err_cs;

    return 0;

err_cs:
    if (drv->sslv_dr >= 0 )
        gpio_free(drv->sslv_dr);
err_dr:
    if (drv->sslv_dt >= 0 )
        gpio_free(drv->sslv_dt);
err_dt:
    gpio_free(drv->sslv_clk);

    return -EINVAL;
}

static void sslv_gpio_release(int id)
{
    if (jzsslv_dev[id].sslv_cs >= 0)
        gpio_free(jzsslv_dev[id].sslv_cs);
    if (jzsslv_dev[id].sslv_dt >= 0 )
        gpio_free(jzsslv_dev[id].sslv_dt);
    if (jzsslv_dev[id].sslv_dr >= 0)
        gpio_free(jzsslv_dev[id].sslv_dr);
    if (jzsslv_dev[id].sslv_clk >= 0)
        gpio_free(jzsslv_dev[id].sslv_clk);
}

static void jz_sslv_init_setting(int id)
{
    sslv_hal_disable(id);

    sslv_hal_disable_all_interrupt(id);

    sslv_hal_set_transmit_threshold(id, 0);
    sslv_hal_set_receive_threshold(id, 0);

    /* standard sslv format */
    sslv_hal_set_standard_format(id);

    sslv_hal_enable(id);
}

static inline int to_bytes(int bits_per_word)
{
    if (bits_per_word <= 8)
        return 1;
    else if (bits_per_word <= 16)
        return 2;
    else
        return 4;
}

/* 批量读取 */
static void sslv_hal_read_rx_fifo(int id, int bits_per_word, void *buf, int len)
{
    int i = 0;
    unsigned char *p8 = buf;
    unsigned short *p16 = buf;
    unsigned int *p32 = buf;
    unsigned int bytes_per_word = to_bytes(bits_per_word);

    switch (bytes_per_word)
    {
        case 1:
            for (i = 0; i < len; i++)
                p8[i] = sslv_hal_read_fifo(id) & 0xff;
            break;

        case 2:
            for (i = 0; i < len; i++)
                p16[i] = sslv_hal_read_fifo(id) & 0xffff;
            break;

        case 4:
            for (i = 0; i < len; i++)
                p32[i] = sslv_hal_read_fifo(id);
            break;

        default:
            break;
    }
}

/* 批量写入 */
static void sslv_hal_write_tx_fifo(int id, int bits_per_word, const void *buf, int len)
{
    int i = 0;
    unsigned char *p8 = (unsigned char *)buf;
    unsigned short *p16 = (unsigned short *)buf;
    unsigned int *p32 = (unsigned int *)buf;
    unsigned int bytes_per_word = to_bytes(bits_per_word);

    switch (bytes_per_word)
    {
        case 1:
            for (i = 0; i < len; i++)
                sslv_hal_write_fifo(id, p8[i]);
            break;

        case 2:
            for (i = 0; i < len; i++)
                sslv_hal_write_fifo(id, p16[i]);
            break;

        case 4:
            for (i = 0; i < len; i++)
                sslv_hal_write_fifo(id, p32[i]);
            break;

        default:
            break;
    }
}

/* 使能传输 */
static void sslv_hal_enable_transfer(struct sslv_config_data *config)
{
    int id = config->id;

    BUG_ON(config->sslv_pha != 1);
    BUG_ON(config->sslv_pol != 1);
    int bits_per_word = config->bits_per_word;
    BUG_ON(bits_per_word < 4 || bits_per_word > 32);

    sslv_hal_disable(id);
    sslv_hal_disable_all_interrupt(id);

    sslv_hal_set_loop_mode(id, 0);

    sslv_hal_set_clk_scph(id, config->sslv_pha);
    sslv_hal_set_clk_scpol(id, config->sslv_pol);
    sslv_hal_set_transfer_mode(id, 0);
    sslv_hal_set_data_frame_size(id, bits_per_word);
    sslv_hal_set_loop_mode(id, config->loop_mode);

    sslv_hal_enable(id);
}

/* 默认接收中断处理子程 */
void sslv_rx_intr_default(struct sslv_device *dev)
{
    int id = dev->sslv_id;
    struct jz_sslv_drv *drv = &jzsslv_dev[id];
    int bytes_per_word = to_bytes(drv->config->bits_per_word);

    int len = sslv_hal_get_receive_fifo_number(id);
    int n = drv->rlen > len ? len : drv->rlen;
    sslv_hal_read_rx_fifo(id, drv->config->bits_per_word, drv->rbuff, n);
    drv->rlen -= n;
    drv->rbuff += (n * bytes_per_word);
    if (drv->rlen > 0) {
        n = drv->rlen > MAX_FIFO_LEN ? (MAX_FIFO_LEN * 3 / 4) : drv->rlen;
        sslv_hal_set_receive_threshold(id, n);
    } else {
        sslv_hal_disable_receive_threshold_interrupt(id);
        sslv_hal_disable_receive_overflow_interrupt(id);
        drv->working_flag = 0;
        wake_up_all(&drv->waiter);
    }
}

/* 中断处理函数 */
static irqreturn_t sslv_intr_handler(int irq, void *dev)
{
    unsigned int len;
    int id = (int)dev;
    struct jz_sslv_drv *drv = &jzsslv_dev[id];

    int bytes_per_word = to_bytes(drv->config->bits_per_word);
    int n;

    unsigned char tx_status = sslv_hal_get_transmit_threshold_intr_status(id);
    unsigned char rx_status = sslv_hal_get_receive_threshold_intr_status(id);
    sslv_hal_clear_all_interrupt(id);

    if (tx_status) {
        len = MAX_FIFO_LEN - sslv_hal_get_transmit_fifo_number(id);
        n = drv->tlen > len ? len : drv->tlen;
        sslv_hal_write_tx_fifo(id, drv->config->bits_per_word, drv->tbuff, len);
        drv->tlen -= n;
        drv->tbuff += (n * bytes_per_word);
        if (drv->tlen > 0) {
            n = drv->tlen > MAX_FIFO_LEN ? (MAX_FIFO_LEN / 4) : (MAX_FIFO_LEN - drv->tlen);
            sslv_hal_set_transmit_threshold(id, n);
        } else {
            sslv_hal_disable_transmit_threshold_interrupt(id);
            sslv_hal_disable_transmit_overflow_interrupt(id);
            drv->working_flag = 0;
            wake_up_all(&drv->waiter);
        }
    }

    if (rx_status) {
        if (drv->sslv_receive_callback != NULL)
            drv->sslv_receive_callback(&drv->dev);
        else
            sslv_rx_intr_default(&drv->dev);
    }

    return IRQ_HANDLED;
}

/* 底层发送 */
static int do_sslv_transmit(struct sslv_device *sslv, unsigned char *tx_buf, int tx_len)
{
    struct sslv_config_data *config = sslv->config;
    int id = config->id;
    struct jz_sslv_drv *drv = &jzsslv_dev[id];
    int bytes_per_word = to_bytes(config->bits_per_word);

    if (drv->working_flag) {
        printk(KERN_ERR "[SSLV]err: sslv%d busy.\n", id);
        return -1;
    }

    drv->tbuff = tx_buf;
    drv->tlen  = tx_len;

    int len = MAX_FIFO_LEN - sslv_hal_get_transmit_fifo_number(id); //可写部分
    int n = (tx_len > len ? len : tx_len);

    sslv_hal_write_tx_fifo(id, config->bits_per_word, tx_buf, n);
    drv->tlen -= n;
    drv->tbuff += (n * bytes_per_word);

    if (drv->tlen > 0) {
        drv->working_flag = 1;
        n = drv->tlen > MAX_FIFO_LEN ? (MAX_FIFO_LEN / 4) : (MAX_FIFO_LEN - drv->tlen);
        sslv_hal_set_transmit_threshold(id, n);

        sslv_hal_clear_all_interrupt(id);

        sslv_hal_enable_transmit_threshold_interrupt(id);
        sslv_hal_enable_transmit_overflow_interrupt(id);

        int ret = wait_event_interruptible(drv->waiter, !drv->working_flag);
        if (ret < 0) {
            sslv_hal_disable_transmit_threshold_interrupt(id);
            sslv_hal_disable_transmit_overflow_interrupt(id);
            drv->working_flag = 0;
            return -1;
        }
    }

    return tx_len * bytes_per_word;//返回发送成功的字节数
}

/* 底层接收 */
static int do_sslv_receive(struct sslv_device *sslv, unsigned char *rx_buf, int rx_len)
{
    struct sslv_config_data *config = sslv->config;
    int id = config->id;
    struct jz_sslv_drv *drv = &jzsslv_dev[id];
    int bytes_per_word = to_bytes(config->bits_per_word);

    if (drv->working_flag) {
        printk(KERN_ERR "[SSLV]err: sslv%d busy.\n", id);
        return -1;
    }

    assert(drv->sslv_receive_callback == NULL);
    drv->rbuff = rx_buf;
    drv->rlen  = rx_len;

    int len = sslv_hal_get_receive_fifo_number(id); //可读部分
    int n = rx_len > len ? len : rx_len;

    sslv_hal_read_rx_fifo(id, config->bits_per_word, rx_buf, n);
    drv->rlen -= n;
    drv->rbuff += (n * bytes_per_word);

    if (drv->rlen > 0) {
        drv->working_flag = 1;
        int n = drv->rlen > MAX_FIFO_LEN ? (MAX_FIFO_LEN * 3 / 4) : drv->rlen;
        sslv_hal_set_receive_threshold(id, n);

        sslv_hal_clear_all_interrupt(id);

        sslv_hal_enable_receive_overflow_interrupt(id);
        sslv_hal_enable_receive_threshold_interrupt(id);

        int ret = wait_event_interruptible(drv->waiter, !drv->working_flag);
        if (ret < 0) {
            sslv_hal_disable_receive_threshold_interrupt(id);
            sslv_hal_disable_receive_overflow_interrupt(id);
            drv->working_flag = 0;
            return -1;
        }
    }

    return rx_len * bytes_per_word;//返回接收成功的字节数
}

/* 底层开始callback接收 */
static void do_sslv_start_cb_receive(struct sslv_device *sslv, unsigned char rx_threshold, void (*cb)(struct sslv_device *dev))
{
    struct sslv_config_data *config = sslv->config;
    int id = config->id;
    jzsslv_dev[id].config = config;

    jzsslv_dev[id].sslv_receive_callback = cb;
    assert(jzsslv_dev[id].sslv_receive_callback != NULL);

    sslv_hal_set_receive_threshold(id, rx_threshold);
    sslv_hal_clear_all_interrupt(id);
    sslv_hal_enable_receive_overflow_interrupt(id);
    sslv_hal_enable_receive_threshold_interrupt(id);
}

/* 底层停止callback接收 */
static void do_sslv_stop_cb_receive(struct sslv_device *sslv)
{
    struct sslv_config_data *config = sslv->config;

    int id = config->id;
    jzsslv_dev[id].config = config;
    jzsslv_dev[id].sslv_receive_callback = NULL;

    sslv_hal_set_receive_threshold(id, 0);
    sslv_hal_clear_all_interrupt(id);
    sslv_hal_disable_receive_overflow_interrupt(id);
    sslv_hal_disable_receive_threshold_interrupt(id);
}

#include <linux/spinlock.h>
#include <linux/wakelock.h>

static struct spinlock spinlock;

unsigned int sslv_get_rx_fifo_num(struct sslv_device *dev)
{
    struct sslv_config_data *config = dev->config;
    int id = config->id;

    return sslv_hal_get_receive_fifo_number(id);
}

void sslv_write_tx_fifo(struct sslv_device *dev, unsigned int data)
{
    struct sslv_config_data *config = dev->config;
    int id = config->id;

    sslv_hal_write_fifo(id, data);
}

unsigned int sslv_read_rx_fifo(struct sslv_device *dev)
{
    struct sslv_config_data *config = dev->config;
    int id = config->id;

    while (!sslv_hal_get_receive_fifo_number(id));

    return sslv_hal_read_fifo(id);
}

int sslv_transmit(struct sslv_device *dev, unsigned char *tx_buf, int tx_len)
{
    if (dev->status == SSLV_IDLE) {
        printk(KERN_ERR "[SSLV]err: IDLE, please enable first\n");
        return -1;
    }

    mutex_lock(&dev->lock);

    int ret = do_sslv_transmit(dev, tx_buf, tx_len);

    mutex_unlock(&dev->lock);

    return ret;
}

int sslv_receive(struct sslv_device *dev, unsigned char *rx_buf, int rx_len)
{
    unsigned long flags;
    if (dev->status == SSLV_IDLE) {
        printk(KERN_ERR "[SSLV]err: IDLE, please enable first\n");
        return -1;
    }

    mutex_lock(&dev->lock);

    spin_lock_irqsave(&spinlock, flags);
    assert(!dev->cb_flags);
    dev->rx_flags = 1;
    spin_unlock_irqrestore(&spinlock, flags);

    int ret = do_sslv_receive(dev, rx_buf, rx_len);

    spin_lock_irqsave(&spinlock, flags);
    dev->rx_flags = 0;
    spin_unlock_irqrestore(&spinlock, flags);

    mutex_unlock(&dev->lock);

    return ret;
}

int sslv_start_cb_receive(struct sslv_device *dev, unsigned char rx_threshold, void (*cb)(struct sslv_device *dev))
{
    unsigned long flags;

    if (dev->status == SSLV_IDLE) {
        printk(KERN_ERR "[SSLV]err: IDLE, please enable first\n");
        return -1;
    }

    spin_lock_irqsave(&spinlock, flags);

    assert(!dev->rx_flags && !dev->cb_flags);

    dev->cb_flags = 1;
    do_sslv_start_cb_receive(dev, rx_threshold, cb);

    spin_unlock_irqrestore(&spinlock, flags);

    return 0;
}

int sslv_stop_cb_receive(struct sslv_device *dev)
{
    unsigned long flags;

    if (dev->status == SSLV_IDLE) {
        printk(KERN_ERR "[SSLV]err: IDLE, please enable first\n");
        return -1;
    }

    spin_lock_irqsave(&spinlock, flags);

    assert(!dev->rx_flags && dev->cb_flags);

    do_sslv_stop_cb_receive(dev);
    dev->cb_flags = 0;

    spin_unlock_irqrestore(&spinlock, flags);

    return 0;
}

struct sslv_device *sslv_enable(struct sslv_config_data *config)
{
    int id = config->id;
    assert(jzsslv_dev[id].is_enable && id == 0);
    struct sslv_device *dev = &jzsslv_dev[id].dev;

    sslv_hal_enable_transfer(config);
    sslv_hal_set_transmit_threshold(id, 0);
    sslv_hal_set_receive_threshold(id, 0);

    if (dev->status == SSLV_BUSY) {
        printk(KERN_ERR "[SSLV]err: BUSY, please disable first\n");
        return NULL;
    }

    jzsslv_dev[id].config = config;
    dev->sslv_id = config->id;
    dev->config = config;
    dev->status = SSLV_BUSY;
    mutex_init(&dev->lock);
    spin_lock_init(&spinlock);

    return dev;
}

int sslv_disable(struct sslv_device *dev)
{
    assert(dev);
    if (dev->status == SSLV_IDLE) {
        printk(KERN_ERR "[SSLV]err: IDLE, please enable first\n");
        return -1;
    }

    struct sslv_config_data *config = dev->config;
    int id = config->id;
    assert(id == 0);

    sslv_hal_disable(id);
    dev->status = SSLV_IDLE;

    return 0;
}

EXPORT_SYMBOL(sslv_get_rx_fifo_num);
EXPORT_SYMBOL(sslv_write_tx_fifo);
EXPORT_SYMBOL(sslv_read_rx_fifo);
EXPORT_SYMBOL(sslv_transmit);
EXPORT_SYMBOL(sslv_receive);
EXPORT_SYMBOL(sslv_start_cb_receive);
EXPORT_SYMBOL(sslv_stop_cb_receive);
EXPORT_SYMBOL(sslv_enable);
EXPORT_SYMBOL(sslv_disable);

#include <linux/delay.h>

struct sslv_config_data sslv_config = {
    .id = 0,
    .bits_per_word = 8,
    .sslv_pha = 1,
    .sslv_pol = 1,
    .loop_mode = 0,
};

static int sslv_open(struct inode *inode, struct file *filp)
{
    struct jz_sslv_drv *drv = container_of(filp->private_data,
            struct jz_sslv_drv, sslv_mdev);

    filp->private_data = &drv->dev;

    return 0;
}

static int sslv_release(struct inode *inode, struct file *filp)
{
    return 0;
}

static long sslv_ioctl(struct file *filp, unsigned int cmd, unsigned long arg)
{
    int ret = 0;
    struct sslv_device *dev = (struct sslv_device *)filp->private_data;

    switch (cmd) {
        case CMD_enable: {
            struct sslv_device *sslv_dev = sslv_enable(&sslv_config);
            if (sslv_dev == NULL) {
                printk(KERN_ERR "[SSLV]err: failed to enable sslv!\n");
                ret = -1;
            }
            break;
        }
        case CMD_disable: {
            ret = sslv_disable(dev);
            break;
        }
        case CMD_receive: {
            unsigned long *array = (void *)arg;
            void *rx_buf = (void *)array[0];
            int len = array[1];
            if (len % to_bytes(dev->config->bits_per_word)) {
                printk(KERN_ERR "[SSLV]err: the len of data should be multiples of bits_per_word!\n");
                return -1;
            }
            ret = sslv_receive(dev, rx_buf, len / to_bytes(dev->config->bits_per_word));
            break;
        }
        case CMD_send: {
            unsigned long *array = (void *)arg;
            void *tx_buf = (void *)array[0];
            int len = array[1];
            int add_zero = array[2];
            if (len % to_bytes(dev->config->bits_per_word)) {
                printk(KERN_ERR "[SSLV]err: the len of data should be multiples of bits_per_word!\n");
                return -1;
            }
            ret = sslv_transmit(dev, tx_buf, len / to_bytes(dev->config->bits_per_word));
            if (add_zero == 0 && ret >= 0) sslv_transmit(dev, (unsigned char*)&add_zero, 1);
            break;
        }
        case CMD_get_info: {
            memmove((void*)arg, &sslv_config, sizeof(struct sslv_config_data));
            break;
        }
        case CMD_set_mode: {
            unsigned char *mode = (void*)arg;
            if (dev->status == SSLV_BUSY) {
                printk(KERN_ERR "[SSLV]err: BUSY, please set mode before enable!\n");
                return -1;
            }
            if (*mode != 0x3) {
                printk(KERN_ERR "[SSLV]err: mode must be set as 0x3 at x1600_boards!\n");
                return -1;
            }

            sslv_config.sslv_pol = !!(*mode & 0x2);
            sslv_config.sslv_pha = !!(*mode & 0x1);
            break;
        }
        case CMD_set_bits: {
            unsigned char *bits = (void*)arg;
            if (dev->status == SSLV_BUSY) {
                printk(KERN_ERR "[SSLV]err: BUSY, please set bits before enable!\n");
                return -1;
            }

            sslv_config.bits_per_word = *bits & 0x3f;
            break;
        }
        default:
            break;
    }

    return ret;
}

static struct file_operations sslv_misc_fops = {
    .owner          = THIS_MODULE,
    .open           = sslv_open,
    .release        = sslv_release,
    .unlocked_ioctl = sslv_ioctl,
};

static void sslv_init(int id)
{
    int ret;
    struct jz_sslv_drv *drv = &jzsslv_dev[id];

    init_waitqueue_head(&drv->waiter);

    drv->clk = clk_get(NULL, "gate_ssislv");
    BUG_ON(IS_ERR(drv->clk));

    clk_prepare_enable(drv->clk);

    ret = sslv_gpio_request(drv);
    if (ret < 0) {
        printk(KERN_ERR "[SSLV]err: sslv%d gpio requeset failed\n", id);
        return;
    }

    memset(&drv->sslv_mdev, 0, sizeof(struct miscdevice));
    drv->sslv_mdev.minor = MISC_DYNAMIC_MINOR;
    drv->sslv_mdev.name = drv->device_name;
    drv->sslv_mdev.fops = &sslv_misc_fops;

    ret = misc_register(&drv->sslv_mdev);
    if (ret < 0) {
        printk(KERN_ERR "[SSLV]err: %s register failed\n", drv->device_name);
        return;
    }

    jz_sslv_init_setting(id);

    ret = request_irq(drv->irq, sslv_intr_handler, 0, drv->irq_name, (void *)id);
    BUG_ON(ret);

    drv->is_finish = 1;
}

static void sslv_exit(int id)
{
    struct jz_sslv_drv *drv = &jzsslv_dev[id];

    free_irq(drv->irq, drv);
    clk_disable_unprepare(drv->clk);
    clk_put(drv->clk);
    sslv_gpio_release(id);
}

static int __init jz_sslv_init(void)
{
    if (jzsslv_dev[0].is_enable)
        sslv_init(0);

    return 0;
}

static void __exit jz_sslv_exit(void)
{
    if (jzsslv_dev[0].is_finish)
        sslv_exit(0);
}

module_init(jz_sslv_init);
module_exit(jz_sslv_exit);

MODULE_DESCRIPTION("JZ x1600 SSLV Controller Driver");
MODULE_LICENSE("GPL");