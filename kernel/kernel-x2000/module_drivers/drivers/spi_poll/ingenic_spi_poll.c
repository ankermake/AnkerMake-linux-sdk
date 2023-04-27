#include <linux/module.h>
#include <linux/init.h>
#include <linux/kernel.h>
#include <linux/spinlock.h>
#include <linux/workqueue.h>
#include <linux/device.h>
#include <linux/platform_device.h>
#include <linux/clk.h>
#include <linux/irq.h>
#include <linux/interrupt.h>
#include <linux/wait.h>
#include <soc/gpio.h>
#include <linux/gpio.h>

#include <linux/spi/spi.h>
#include <linux/spi/spi_bitbang.h>
#include <linux/delay.h>
#include "ingenic_spi_hal.h"
#include "ingenic_spi_poll.h"
#include <linux/types.h>
#include <linux/jiffies.h>

// #define INGENIC_SPI_POLL_TEST

#define MAX_FIFO_LEN 128

static int spi_clk_rate = CONFIG_INGENIC_POLL_SPI_CLK_RATE;

static struct clk *spi_div_clk;
static struct clk *spi_mux_clk;
static struct clk *clk_cgu;

static int cgu_clk_rate;

struct jz_func_alter {
    int pin;
    int function;
    const char *name;
};

struct jz_spi_pin {
    struct jz_func_alter clk_pin;
    struct jz_func_alter miso_pin;
    struct jz_func_alter mosi_pin;
    struct jz_func_alter cs_pin;
};

struct jz_spi_drv {
    int id;
    int status;
    struct clk *clk;

    struct jz_spi_pin *pin;

    int tlen;
    int rlen;
    int len;
    int total_len;
    void *tbuff;
    void *rbuff;
    int fifo_per_rw_size;
    struct jzspi_config_data config;
    unsigned int working_flag;  //工作状态：    1：繁忙，0：空闲
};


static struct jz_spi_pin jz_spi0_pin = {

#ifdef CONFIG_INGENIC_POLL_SPI0_CLK_PA31
    .clk_pin = {GPIO_PA(31), GPIO_FUNC_0, "spi0_clk"},
#else
    .clk_pin = {GPIO_PB(12), GPIO_FUNC_1, "spi0_clk"},
#endif

#ifdef CONFIG_INGENIC_POLL_SPI0_MISO_PA29
    .miso_pin = {GPIO_PA(29), GPIO_FUNC_0, "spi0_miso"},
#elif CONFIG_INGENIC_POLL_SPI0_MISO_PB14
    .miso_pin = {GPIO_PB(14), GPIO_FUNC_1, "spi0_miso"},
#endif

#ifdef CONFIG_INGENIC_POLL_SPI0_MOSI_PA30
    .mosi_pin = {GPIO_PA(30), GPIO_FUNC_0, "spi0_mosi"},
#elif CONFIG_INGENIC_POLL_SPI0_MOSI_PB13
    .mosi_pin = {GPIO_PB(13), GPIO_FUNC_1, "spi0_mosi"},
#endif

#ifdef CONFIG_INGENIC_POLL_SPI0_CS_PA28
    .cs_pin = {GPIO_PA(28), GPIO_FUNC_0, "spi0_cs"},
#elif CONFIG_INGENIC_POLL_SPI0_CS_PB16
    .cs_pin = {GPIO_PB(16), GPIO_FUNC_1, "spi0_cs"},
#endif
};

struct jz_spi_drv jzspi_dev[1] = {
#ifdef CONFIG_INGENIC_POLL_SPI0
    {
        .id = 0,
        .pin = &jz_spi0_pin,
    },
#endif
};


static inline int to_bytes(int bits_per_word)
{
    if (bits_per_word <= 8)
        return 1;
    else if (bits_per_word <= 16)
        return 2;
    else
        return 4;
}

static int gpio_set_func(int gpio, enum gpio_function func)
{
    int port = gpio / 32;
	int pin = BIT(gpio % 32);

    return jzgpio_set_func(port, func, pin);
}

static inline int gpio_init(int gpio, enum gpio_function func, const char *name)
{
    int ret = 0;
    ret = gpio_request(gpio, name);
    if (ret < 0)
        return ret;

    gpio_set_func(gpio , func);

    return 0;
}

static int jzspi_get_cgv(int id, int clk_rate)
{
    int cgv;
    int real_set_rate;
    int set_rate = clk_rate;
    int prev_rate = clk_rate;
    int diff;

    if (cgu_clk_rate < clk_rate)
        printk(KERN_ERR "spi set speed invalid , try to set spi speed %d but max speed is %d \n" ,
        clk_rate, cgu_clk_rate / 2);

    cgv = cgu_clk_rate / 2 / clk_rate - 1;

    real_set_rate = cgu_clk_rate / 2 / (cgv + 1);
    if (real_set_rate != clk_rate)
        prev_rate = cgu_clk_rate / 2 / (cgv + 2);

    if ((clk_rate - prev_rate) < (real_set_rate - clk_rate))
        cgv = cgu_clk_rate / 2 / prev_rate - 1;

    if (cgv < 0)
        cgv = 0;
    if (cgv > 255)
        cgv = 255;

    real_set_rate = cgu_clk_rate / 2 / (cgv + 1);

    /* 实际输出频率与设置的频率超过10%的情况才会打印 */
    diff = real_set_rate > set_rate ? real_set_rate - set_rate : -1 * (real_set_rate - set_rate);
    if (diff * 100 / set_rate > 10) {
        printk(KERN_DEBUG "SPI%d :user set frequency: %d, the real frequency: %d\n",
                id, set_rate, real_set_rate);
    }

    return cgv;
}

/* 批量读取 */
static void jzspi_hal_read_rx_fifo(int id, int bits_per_word, void *buf, int len)
{
    int i = 0;
    unsigned char *p8 = buf;
    unsigned short *p16 = buf;
    unsigned int *p32 = buf;
    unsigned int bytes_per_word = to_bytes(bits_per_word);

    switch (bytes_per_word)
    {
        case 1:
            for (i = 0; i < len; i++) {
                p8[i] = spi_hal_read_fifo(id) & 0xff;
            }
            break;

        case 2:
            for (i = 0; i < len; i++)
                p16[i] = spi_hal_read_fifo(id) & 0xffff;
            break;

        case 4:
            for (i = 0; i < len; i++)
                p32[i] = spi_hal_read_fifo(id);
            break;

        default:
            break;
    }
}
/* 批量写入 */
static void jzspi_hal_write_tx_fifo(int id, int bits_per_word, const void *buf, int len)
{
    int i = 0;
    unsigned char *p8 = (unsigned char *)buf;
    unsigned short *p16 = (unsigned short *)buf;
    unsigned int *p32 = (unsigned int *)buf;
    unsigned int bytes_per_word = to_bytes(bits_per_word);

    switch (bytes_per_word)
    {
        case 1:
            for (i = 0; i < len; i++) {
                spi_hal_write_fifo(id, p8[i]);
            }
            break;

        case 2:
            for (i = 0; i < len; i++)
                spi_hal_write_fifo(id, p16[i]);
            break;

        case 4:
            for (i = 0; i < len; i++)
                spi_hal_write_fifo(id, p32[i]);
            break;

        default:
            break;
    }
}
/* 单写 */
static void jzspi_hal_write_tx_fifo_only(int id, unsigned int len, unsigned int val)
{
    int i = 0;

    for (i = 0; i < len; i++)
        spi_hal_write_fifo(id, val);
}

volatile unsigned int val_xxxx;
/* 单读 */
static void jzspi_hal_read_rx_fifo_only(int id, int len)
{
    int i = 0;

    for (i = 0; i < len; i++)
        val_xxxx = spi_hal_read_fifo(id);
}

static void jzspi_write_fifo(struct jz_spi_drv *hw, int bits_per_word, int len, const void *buf, int tlen, int n)
{
    int id = hw->config.id;
    if (len < n) {
        n = len;
    }

    if (tlen > 0) {
        if (tlen > n) {
            jzspi_hal_write_tx_fifo(id, bits_per_word, buf, n);
        } else {
            jzspi_hal_write_tx_fifo(id, bits_per_word, buf, tlen);
            jzspi_hal_write_tx_fifo_only(id, n - tlen, 0x0);/* 高位补0 */
        }

    } else {
        jzspi_hal_write_tx_fifo_only(id, n, 0x0);
    }
}

static void jzspi_read_fifo(struct jz_spi_drv *hw, int bits_per_word, void *buf, int rlen, int n)
{
    int id = hw->config.id;
    if (rlen > 0) {
        if (rlen < n) {
            jzspi_hal_read_rx_fifo(id, bits_per_word, buf, rlen);
            jzspi_hal_read_rx_fifo_only(id, n - rlen);
        } else {
            jzspi_hal_read_rx_fifo(id, bits_per_word, buf, n);
        }

    } else {
        jzspi_hal_read_rx_fifo_only(id, n);
    }
}

static void spi_start_write_read(struct jz_spi_drv *drv)
{
    int len;
    int l = 0;
    int id = drv->config.id;
    int bytes_per_word = to_bytes(drv->config.bits_per_word);
    int n = drv->fifo_per_rw_size;

    spi_hal_clear_transmit_fifo_underrrun_flag(id);
    spi_hal_clear_receive_fifo_overrun_flag(id);

    if (spi_hal_get_receive_fifo_more_threshold_flag(id)) {

        if (drv->len > 0) {
            len = spi_hal_get_receive_fifo_number(id);
            jzspi_read_fifo(drv, drv->config.bits_per_word, drv->rbuff, drv->rlen, len);
            drv->rlen -= len;
            drv->total_len -= len;
            drv->rbuff += (len * bytes_per_word);

            jzspi_write_fifo(drv, drv->config.bits_per_word, drv->len, drv->tbuff, drv->tlen, len);
            drv->tlen -= len;
            drv->tbuff += (len * bytes_per_word);
            drv->len -= len;

        } else {
            len = spi_hal_get_receive_fifo_number(id);
            jzspi_read_fifo(drv, drv->config.bits_per_word, drv->rbuff, drv->rlen, len);
            drv->rlen -= len;
            drv->total_len -= len;
            drv->rbuff += (len * bytes_per_word);

            /* 判断传输完成，设置状态，若传输完成则调用 回调函数 */
            if (drv->tlen <= 0 && drv->rlen <= 0) {
                drv->working_flag = 0;
                return;
            }
        }

        l = drv->len > drv->rlen ? drv->len : drv->rlen;

        /* 根据 len 大小设置 阈值 */
        if (l > MAX_FIFO_LEN) {
            spi_hal_set_receive_threshold(id, n);
        } else {
            spi_hal_set_receive_threshold(id, l);
        }
    }

    return;
}

static int spi_write_read_poll(struct jz_spi_drv * hw,
    void *tx_buf, int tlen, void * rx_buf, int rlen)
{
    uint64_t start;
    int len;
    struct jzspi_config_data *config = &hw->config;
    int id = config->id;
    int bytes_per_word = to_bytes(config->bits_per_word);
    int n = hw->fifo_per_rw_size;


    if (tx_buf == NULL)
        tlen = 0;
    if (rx_buf == NULL)
        rlen = 0;

    len = tlen > rlen ? tlen : rlen;
    if (!len)
        return 0;

    if (hw->working_flag) {
        printk(KERN_ERR "SPI%d busy.\n", id);
        return 0;
    }

    hw->tbuff  = tx_buf;
    hw->rbuff  = rx_buf;
    hw->tlen   = tlen;
    hw->rlen   = rlen;
    hw->len    = len;
    hw->total_len = len;
    hw->working_flag = 1;

    spi_hal_set_transmit_threshold(id, 0);
    if (len > MAX_FIFO_LEN) {
        spi_hal_set_receive_threshold(id, n);
    } else {
        spi_hal_set_receive_threshold(id, len);
    }

    /* 第一次 写FIFO 写满 */
    jzspi_write_fifo(hw, config->bits_per_word, len, hw->tbuff, hw->tlen, MAX_FIFO_LEN);
    hw->tlen -= MAX_FIFO_LEN;
    hw->tbuff += (MAX_FIFO_LEN * bytes_per_word);
    hw->len -= MAX_FIFO_LEN;

    start = get_jiffies_64();

    while (hw->working_flag) {
        spi_start_write_read(hw);
        if (get_jiffies_64() - start > HZ) {
            printk(KERN_ERR "ssi%d: write read timeout\n", id);
            return -1;
        }
    }

    return len * bytes_per_word;
}

static void jz_spi_init_setting(int id)
{
    spi_hal_disable(id);
    spi_hal_set_auto_clear_transmit_fifo_empty_flag(id);
    spi_hal_clear_transmit_fifo(id);
    spi_hal_clear_receive_fifo(id);

    spi_hal_disable_transmit_interrupt(id);

    spi_hal_disable_receive_finish_control(id);
    spi_hal_set_receive_continue(id);
    spi_hal_receive_counter_unvalid(id);
    spi_hal_set_transmit_fifo_empty_mode(id, 0);

    /* 设置 TTRG RTRG */
    spi_hal_set_transmit_threshold(id, 0);
    spi_hal_set_receive_threshold(id, 0);

    spi_hal_set_start_delay_clk(id, 0);
    spi_hal_set_stop_delay_clk(id, 0);

    /* standard spi format */
    spi_hal_set_standard_transfer_format(id);

    /* disable interval mode */
    spi_hal_set_interval_time(id, 0);

}

static int m_gpio_request(struct jz_func_alter *pin)
{
    int ret;
    if (pin->pin <= 0)
        return 0;

    ret = gpio_init(pin->pin, pin->function, pin->name);
    if (ret < 0)
        printk(KERN_ERR "ssi: failed to requeset gpio: %d, %s\n", pin->pin, pin->name);

    return ret;
}

static int spi_gpio_requeset(struct jz_spi_drv *drv)
{
    int ret;
    ret = m_gpio_request(&drv->pin->clk_pin);
    if (ret < 0)
        return ret;

    ret = m_gpio_request(&drv->pin->mosi_pin);
    if (ret < 0)
        goto mosi_err;

    ret = m_gpio_request(&drv->pin->miso_pin);
    if (ret < 0)
        goto miso_err;

    return 0;

miso_err:
    if(drv->pin->mosi_pin.pin > 0)
        gpio_free(drv->pin->mosi_pin.pin);
mosi_err:
    if(drv->pin->clk_pin.pin > 0)
        gpio_free(drv->pin->clk_pin.pin);

    return ret;
}

static void spi_gpio_release(struct jz_spi_drv *drv)
{
    struct jz_spi_pin *pin = drv->pin;

    if (pin->miso_pin.pin > 0)
        gpio_free(pin->miso_pin.pin);

    if (pin->mosi_pin.pin > 0)
        gpio_free(pin->mosi_pin.pin);

    if (pin->clk_pin.pin > 0)
        gpio_free(pin->clk_pin.pin);

}


static void spi_init(int id)
{
    int ret;

    struct jz_spi_drv *drv = &jzspi_dev[id];
    char tmp_name[10];

    ret = spi_gpio_requeset(drv);
    if (ret < 0) {
        printk(KERN_ERR "spi%d gpio requeset failed\n", id);
        return;
    }

    sprintf(tmp_name, "gate_ssi%d", id);
    drv->clk = clk_get(NULL, tmp_name);
    BUG_ON(IS_ERR(drv->clk));

    clk_prepare_enable(drv->clk);

    jz_spi_init_setting(id);
}


static void spi_exit(int id)
{
    struct jz_spi_drv *drv = &jzspi_dev[id];

    clk_disable_unprepare(drv->clk);
    clk_put(drv->clk);
    spi_gpio_release(drv);
}


void spi_start_config(struct jzspi_config_data *config)
{
    int cgv;
    int ret;

    int bits_per_word = config->bits_per_word;
    int id = config->id;
    struct jz_spi_drv *drv = &jzspi_dev[id];
    BUG_ON((drv == NULL) || (id > 0));

    if (config->cs > 0) {
        ret = m_gpio_request(&drv->pin->cs_pin);
        if(ret < 0)
            return;
    }

    spi_hal_disable(id);
    spi_hal_disable_transmit_interrupt(id);
    spi_hal_disable_transmit_error_interrupt(id);
    spi_hal_disable_receive_interrupt(id);
    spi_hal_disable_receive_error_interrupt(id);

    spi_hal_set_transfer_endian(id, config->tx_endian, config->rx_endian);
    spi_hal_set_loop_mode(id, 0);

    spi_hal_select_frame_valid_level(id, config->cs, config->cs_valid_level);

    spi_hal_set_clk_pha(id, config->spi_pha);
    spi_hal_set_clk_pol(id, config->spi_pol);
    spi_hal_set_loop_mode(id, config->loop_mode);

    spi_hal_clear_transmit_fifo(id);
    spi_hal_clear_receive_fifo(id);

    spi_hal_set_transmit_fifo_empty_transfer_finish(id);

    spi_hal_set_data_unit_bits_len(id, bits_per_word);

    if (!config->clk_rate)
        config->clk_rate = 1 * 1000 * 1000;

    cgv = jzspi_get_cgv(id, config->clk_rate);

    spi_hal_set_clock_dividing_number(id, cgv);

    spi_hal_enable(id);

    if (config->clk_rate > 10*1000*1000)
        drv->fifo_per_rw_size = MAX_FIFO_LEN * 3 / 4;
    else
        drv->fifo_per_rw_size = MAX_FIFO_LEN / 2;

}EXPORT_SYMBOL(spi_start_config);


int spi_write_read(struct jzspi_config_data *config, void *tx_buf, int tx_len, void *rx_buf, int rx_len)
{
    int id = config->id;
    int ret;
    struct jz_spi_drv *drv = &jzspi_dev[id];
    BUG_ON((drv == NULL) || (id > 0));

    ret = spi_write_read_poll(drv, tx_buf, tx_len, rx_buf, rx_len);

    return ret;

}EXPORT_SYMBOL(spi_write_read);


void spi_stop_config(struct jzspi_config_data *config)
{
    int id = config->id;
    struct jz_spi_drv *drv = &jzspi_dev[id];
    BUG_ON((drv == NULL) || (id > 0));

    spi_hal_disable(id);

    if (config->cs > 0)
        gpio_free(drv->pin->cs_pin.pin);

}EXPORT_SYMBOL(spi_stop_config);

#ifdef INGENIC_SPI_POLL_TEST

struct jzspi_config_data test_config = {
    .id = 0,
    .clk_rate = 12*1000*1000,
    .cs = 1,
    .cs_valid_level = Valid_low,
    .bits_per_word = 8,
    .spi_pha = 0,
    .spi_pol = 0,
    .tx_endian = Endian_msb_first,
    .rx_endian = Endian_msb_first,
};
unsigned int tx_buff[] = {0xAE0000 ,0x002533FF, 0x2533FF0D ,0X12347778};
unsigned int rx_buff[1024];

#endif

int jz_spi_init(void)
{
    struct clk *sclka_clk;

    spi_mux_clk = clk_get(NULL, "mux_ssi");
    BUG_ON(IS_ERR(spi_mux_clk));

    sclka_clk = clk_get(NULL, "sclka");
    BUG_ON(IS_ERR(sclka_clk));

    clk_set_parent(spi_mux_clk, sclka_clk);
    clk_prepare_enable(spi_mux_clk);
    clk_put(sclka_clk);

    spi_div_clk = clk_get(NULL, "div_ssi");
    BUG_ON(IS_ERR(spi_div_clk));

    clk_set_rate(spi_div_clk, spi_clk_rate);
    cgu_clk_rate = clk_get_rate(spi_div_clk);

    clk_prepare_enable(clk_cgu);

    spi_init(0);

#ifdef INGENIC_SPI_POLL_TEST
    int len;
    int cnt = 10;
    len = sizeof(tx_buff);

    spi_start_config(&test_config);

    while(cnt--) {

        tx_buff[0] += cnt;
        spi_write_read(&test_config, tx_buff, len, rx_buff, len);

        printk(KERN_ERR "rx_buff = %08x, %08x, %08x, %08x\n", rx_buff[0], rx_buff[1], rx_buff[2], rx_buff[3]);
    }

    spi_stop_config(&test_config);
#endif


    return 0;
}

static void jz_spi_exit(void)
{
    spi_exit(0);

    clk_disable_unprepare(clk_cgu);
    clk_put(clk_cgu);
}

module_init(jz_spi_init);
module_exit(jz_spi_exit);

MODULE_DESCRIPTION("JZ x1600 SPI Controller Driver");
MODULE_LICENSE("GPL");
