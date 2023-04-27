#include <linux/kernel.h>
#include <linux/gpio.h>
#include <linux/serial_core.h>
#include <linux/console.h>
#include <linux/serial.h>
#include <linux/tty.h>
#include <linux/tty_flip.h>
#include <linux/tty_driver.h>
#include <soc/gpio.h>
#include <linux/device.h>
#include <linux/string.h>
#include <linux/platform_device.h>
#include <linux/delay.h>
#include <linux/kthread.h>
#include <linux/gfp.h>
#include <linux/module.h>

#define SPI_TO_UART_NAME    "jz_spi_to_uart"
#define SPI_TO_UART_MAX_DEVS    8

#define UART_BAUD_RATE  (115200)

#define SPI_GPIO_CS CONFIG_SERIAL_GPIO_SPI_TO_UART_CS_PIN
#define SPI_GPIO_DT CONFIG_SERIAL_GPIO_SPI_TO_UART_DT_PIN
#define SPI_GPIO_DR CONFIG_SERIAL_GPIO_SPI_TO_UART_DR_PIN
#define SPI_GPIO_CLK CONFIG_SERIAL_GPIO_SPI_TO_UART_CLK_PIN

static int spi_cs;
static int spi_dt;
static int spi_dr;
static int spi_clk;

static volatile is_online;

enum {
    CMD_WRITE_FIFO,
    CMD_READ_FIFO_CNT,
    CMD_READ_FIFO,
    CMD_WRITE_FIFO_CNT,
    CMD_DETECT,
};

static int str_to_gpio(const char *str)
{
    int gpio_n;
    int ret = -1;
    int gpio_num;
    unsigned int str_len;

    str_len = strlen(str);
    if(str[0] != 'p' && str[0] != 'P' && str_len != 3 && str_len != 4)
        return ret;

    if(str[1] >= 'A' && str[1] <= 'G')
        gpio_num = str[1] - 'A';
    else if(str[1] >= 'a' && str[1] <= 'g')
        gpio_num = str[1] - 'a';
    else
        return ret;

    ret = sscanf(str + 2, "%d", &gpio_n);
    if(ret < 0 || gpio_n <0 || gpio_n > 31)
        return ret;

    ret = gpio_num * 32 + gpio_n;
    return ret;
}

static void send_byte(unsigned int v)
{
    int i;

    for (i = 0; i < 8; i++) {
        int value = !!(v & (1 << (7 - i)));
        gpio_set_value(spi_dt, value);
        udelay(1);
        gpio_set_value(spi_clk, 1);
        udelay(1);
        gpio_set_value(spi_clk, 0);
    }
    udelay(1);
}

static int receive_byte(unsigned char *b)
{
    int i;
    int v = 0;

    for (i = 0; i < 8; i++) {
        gpio_set_value(spi_clk, 1);
        udelay(1);
        gpio_set_value(spi_clk, 0);
        udelay(1);
        if (gpio_get_value(spi_dr))
            v |= 1 << (7 - i);
    }

    *b = v;

    return 0;
}

static void set_cs_low(void)
{
    gpio_set_value(spi_cs, 0);
    udelay(1);
}

static void set_cs_high(void)
{
    udelay(1);
    gpio_set_value(spi_cs, 1);
    udelay(1);
}

static void send_char(unsigned char c)
{
    send_byte(CMD_WRITE_FIFO);
    send_byte(c);
}

static void receive_char(unsigned char cmd,  unsigned char *c)
{
    send_byte(cmd);
    receive_byte(c);
}

static int get_writeable_count(void)
{
    unsigned char writable_cnt;

    set_cs_low();
    receive_char(CMD_WRITE_FIFO_CNT, &writable_cnt);
    set_cs_high();

    return writable_cnt;
}

static void spi_uart_transfer(unsigned char c)
{
    while (!get_writeable_count() && is_online);
    set_cs_low();
    send_char(c);
    set_cs_high();
}

struct spi_to_uart_port {
    struct uart_port port;
};

static struct spi_to_uart_port my_uart_port;

static struct uart_driver spi_to_uart_drv;

static void serial_ingenic_console_putchar(struct uart_port *port, int ch)
{
    spi_uart_transfer(ch);
}

static void serial_ingenic_console_write(struct console *co, const char *s, unsigned int count)
{
    struct uart_port *up = &(my_uart_port.port);

    uart_console_write(up, s, count, serial_ingenic_console_putchar);
}

static int __init serial_ingenic_console_setup(struct console *co, char *options)
{
    struct uart_port *up = &(my_uart_port.port);
    int baud = UART_BAUD_RATE;
    int bits = 8;
    int parity = 'n';
    int flow = 'n';

    if (co->index == -1 || co->index >= SPI_TO_UART_MAX_DEVS)
        co->index = 0;

    if (options)
        uart_parse_options(options, &baud, &parity, &bits, &flow);

    return uart_set_options(up, co, baud, parity, bits, flow);
}

static struct console serial_ingenic_console = {
    .name       = "ttySC",
    .write      = serial_ingenic_console_write,
    .device     = uart_console_device,
    .setup      = serial_ingenic_console_setup,
    .flags      = CON_PRINTBUFFER,
    .index      = -1,
    .data       = &spi_to_uart_drv,
};

#define INGENIC_CONSOLE	&serial_ingenic_console

static struct uart_driver spi_to_uart_drv = {
    .owner		= THIS_MODULE,
    .dev_name	= "ttySC",
    .nr		= SPI_TO_UART_MAX_DEVS,
    .cons	= INGENIC_CONSOLE,
};


static void spi_uart_tx(struct uart_port *port)
{
    struct circ_buf *xmit = &port->state->xmit;
    unsigned int to_send;

    if (uart_circ_empty(xmit) || uart_tx_stopped(port))
        return;

    to_send = uart_circ_chars_pending(xmit);

    while (!uart_circ_empty(xmit)) {
        int i;
        for (i = 0; i < to_send; ++i) {
            spi_uart_transfer(xmit->buf[xmit->tail]);
            xmit->tail = (xmit->tail + 1) & (UART_XMIT_SIZE - 1);
        }
    }

    if (uart_circ_chars_pending(xmit) < WAKEUP_CHARS)
        uart_write_wakeup(port);
}

static int spi_uart_rx(void *data)
{
    struct uart_port *port = (struct uart_port *)data;
    unsigned long flags;

    while (1) {
        unsigned char cnt;
        unsigned char detect = 0;

        while (1) {
            spin_lock_irqsave(&port->lock, flags);
            set_cs_low();
            receive_char(CMD_READ_FIFO_CNT, &cnt);
            set_cs_high();
            spin_unlock_irqrestore(&port->lock, flags);

            spin_lock_irqsave(&port->lock, flags);
            set_cs_low();
            receive_char(CMD_DETECT, &detect);
            set_cs_high();
            spin_unlock_irqrestore(&port->lock, flags);

            if (detect == 0xa5) {
                is_online = 1;
                if (cnt)
                    break;
            } else {
                is_online = 0;
            }

            usleep_range(10 * 1000, 10 * 1000);
        }

        while (cnt) {
            unsigned char c;

            spin_lock_irqsave(&port->lock, flags);
            set_cs_low();
            receive_char(CMD_READ_FIFO, &c);
            set_cs_high();

            uart_insert_char(port, 0, (1 << 1), c,
                        TTY_NORMAL);

            spin_unlock_irqrestore(&port->lock, flags);

            tty_flip_buffer_push(&port->state->port);

            cnt--;
        }

        // printk(KERN_ERR "%c", c);
    }

    return 0;
}

static unsigned int spi_to_uart_return(struct uart_port *port)
{
    return 0;
}

static void spi_to_uart_start_tx(struct uart_port *port)
{
    spi_uart_tx(port);
}

static int spi_to_uart_startup(struct uart_port *port)
{
    kthread_run(spi_uart_rx, port, "spi-uart_read_task");

    return 0;
}

static void spi_to_uart_shutdown(struct uart_port *port)
{

}

static void spi_to_uart_set_termios(struct uart_port *port,
                    struct ktermios *termios,
                    struct ktermios *old)
{
    int baud = UART_BAUD_RATE;

    uart_update_timeout(port, termios->c_cflag, baud);
}

static const char *spi_to_uart_type(struct uart_port *port)
{
    return SPI_TO_UART_NAME;
}

static void spi_to_uart_config_port(struct uart_port *port, int flags)
{
    if (flags & UART_CONFIG_TYPE)
        port->type = PORT_8250;
}

static void spi_to_uart_null_void(struct uart_port *port)
{
    /* Do nothing */
}

static const struct uart_ops spi_to_uart_ops = {
    .tx_empty   = spi_to_uart_return,
    .set_mctrl  = spi_to_uart_null_void,
    .get_mctrl  = spi_to_uart_return,
    .stop_tx    = spi_to_uart_null_void,
    .start_tx   = spi_to_uart_start_tx,
    .stop_rx    = spi_to_uart_null_void,
    .break_ctl  = spi_to_uart_null_void,
    .startup    = spi_to_uart_startup,
    .shutdown   = spi_to_uart_shutdown,
    .set_termios    = spi_to_uart_set_termios,
    .type       = spi_to_uart_type,
    .config_port    = spi_to_uart_config_port,
    .release_port   = spi_to_uart_null_void,
    .request_port   = spi_to_uart_return,
};

static int spi_gpio_init(void)
{
    int ret;

    spi_cs = str_to_gpio(SPI_GPIO_CS);
    if (spi_cs < 0) {
        printk(KERN_ERR "spi-uart: invalid gpio spi_cs: %s\n", SPI_GPIO_CS);
        return spi_cs;
    }

    spi_dt = str_to_gpio(SPI_GPIO_DT);
    if (spi_dt < 0) {
        printk(KERN_ERR "spi-uart: invalid gpio spi_dt: %s\n", SPI_GPIO_DT);
        return spi_dt;
    }

    spi_dr = str_to_gpio(SPI_GPIO_DR);
    if (spi_dr < 0) {
        printk(KERN_ERR "spi-uart: invalid gpio spi_dr: %s\n", SPI_GPIO_DR);
        return spi_dr;
    }

    spi_clk = str_to_gpio(SPI_GPIO_CLK);
    if (spi_clk < 0) {
        printk(KERN_ERR "spi-uart: invalid gpio spi_clk: %s\n", SPI_GPIO_CLK);
        return spi_clk;
    }

    ret = gpio_request(spi_cs, "spi-uart:spi_cs");
    if (ret) {
        printk(KERN_ERR "spi-uart: failed to request spi_cs: %d\n", spi_cs);
        return ret;
    }

    ret = gpio_request(spi_dt, "spi-uart:spi_dt");
    if (ret) {
        printk(KERN_ERR "spi-uart: failed to request spi_dt: %d\n", spi_dt);
        return ret;
    }

    ret = gpio_request(spi_dr, "spi-uart:spi_dr");
    if (ret) {
        printk(KERN_ERR "spi-uart: failed to request spi_dr: %d\n", spi_dr);
        return ret;
    }

    ret = gpio_request(spi_clk, "spi-uart:spi_clk");
    if (ret) {
        printk(KERN_ERR "spi-uart: failed to request spi_clk: %d\n", spi_clk);
        return ret;
    }

    gpio_direction_output(spi_cs, 1);
    gpio_direction_output(spi_dt, 0);
    gpio_direction_input(spi_dr);
    gpio_direction_output(spi_clk, 0);

    return 0;
}

static int spi_to_uart_probe(struct platform_device *pdev)
{
    struct spi_to_uart_port *p = &my_uart_port;

    int ret = spi_gpio_init();
    if (ret) {
        printk(KERN_ERR "spi_gpio_init err\n");
        return ret;
    }

    p->port.dev = &pdev->dev;
    p->port.type = PORT_8250;
    p->port.ops = &spi_to_uart_ops;
    p->port.line = 0;
    p->port.fifosize = 64;
    p->port.membase = 1;
    spin_lock_init(&p->port.lock);

    platform_set_drvdata(pdev, &p->port);

    return uart_add_one_port(&spi_to_uart_drv, &p->port);
}

static int spi_to_uart_remove(struct platform_device *pdev)
{
    printk(KERN_ERR "spi_to_uart_remove\n");
    return 0;
}

static void m_release(struct device *dev) {}

struct platform_device spi_to_uart_platform_dev = {
    .name = "ingenic_spi_to_uart",
    .id = 1,
    .dev = {
        .release = m_release,
    },
};

static struct platform_driver spi_to_uart_platform_drv = {
    .driver = {
        .name = "ingenic_spi_to_uart",
    },
    .probe	= spi_to_uart_probe,
    .remove	= spi_to_uart_remove,
};

static int __init gpio_spi_to_uart_init(void)
{
    int ret;

    ret = uart_register_driver(&spi_to_uart_drv);
    if (ret) {
        pr_err("Registering UART driver failed\n");
        return ret;
    }

    ret = platform_device_register(&spi_to_uart_platform_dev);
    if (ret) {
        pr_err("Registering SPI to UART DEV failed\n");
        return ret;
    }

    ret = platform_driver_register(&spi_to_uart_platform_drv);
    if (ret) {
        pr_err("Registering SPI to UART DRV failed\n");
        return ret;
    }

    return ret;
}
module_init(gpio_spi_to_uart_init);

static void __exit gpio_spi_to_uart_exit(void)
{
    uart_unregister_driver(&spi_to_uart_drv);
}
module_exit(gpio_spi_to_uart_exit);

MODULE_LICENSE("GPL");
MODULE_DESCRIPTION("INGENIC SPI TO UART serial driver");