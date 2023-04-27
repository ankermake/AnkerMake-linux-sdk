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
#include <soc/irq.h>
#include <linux/gpio.h>
#include <utils/gpio.h>
#include <linux/spinlock.h>
#include <linux/mm.h>
#include <assert.h>
#include <linux/spi/spi.h>
#include <linux/dmaengine.h>
#include <linux/dma-mapping.h>
#include <linux/delay.h>
#include <bit_field.h>
#include "spi_regs.h"
#include "spi.h"

#include <mach/jzssi.h>
#include <mach/jzdma.h>

#define SSI0_IOBASE     0x10043000

static const unsigned long iobase[] = {
    SSI0_IOBASE,
};

#define SPI_ADDR(id, reg) ((volatile unsigned long *)(KSEG1ADDR(iobase[id]) + reg))

static inline void spi_write_reg(int id, unsigned int reg, unsigned int value)
{
    *SPI_ADDR(id, reg) = value;
}

static inline unsigned int spi_read_reg(int id, unsigned int reg)
{
    return *SPI_ADDR(id, reg);
}

static inline void spi_set_bits(int id, unsigned int reg, int start, int end, unsigned int val)
{
    set_bit_field(SPI_ADDR(id, reg), start, end, val);
}

static inline unsigned int spi_get_bits(int id, unsigned int reg, int start, int end)
{
    return get_bit_field(SPI_ADDR(id, reg), start, end);
}

/*获取DMA传输的目标地址*/
unsigned long spi_hal_get_dma_ssidr_addr(int id)
{
    return iobase[id] + SSIDR;
}

/////////////////////////////////////////////////////////////////////////////////////

#define DMA_REQ_TYPE_SSI0_TX 0x16
#define DMA_REQ_TYPE_SSI0_RX 0x17
#define DMA_REQ_TYPE_SSI1_TX 0x18
#define DMA_REQ_TYPE_SSI1_RX 0x19

static unsigned int spi_dma_type[] = {
    DMA_REQ_TYPE_SSI0_TX,
    /* DMA_REQ_TYPE_SSI0_RX, */
    DMA_REQ_TYPE_SSI1_TX,
    /* DMA_REQ_TYPE_SSI1_RX */
};

#define BUFFER_SIZE    PAGE_SIZE
#define MAX_FIFO_LEN 128

#define DMA_MAX_BURST   64

static DEFINE_MUTEX(spi_lock);

static int spi_is_enable_dma = 1;

struct jz_func_alter {
    int pin;
    int function;
    const char *name;
};

struct jz_spi_pin {
    struct jz_func_alter spi_pins[3];
};

struct jz_spi_drv {
    int id;
    int irq;
    int status;
    struct clk *clk;

    int tlen;
    int rlen;
    int total_tx_len;
    int total_rx_len;
    void *tbuff;
    void *rbuff;

    unsigned int cur_mode;
    unsigned int cur_bpw;
    unsigned int cur_speed;

    struct spi_master *master;
    struct spi_device *tgl_spi;
    struct spi_config_data config;
    unsigned int working_flag;  //工作状态：    1：繁忙，0：空闲
    wait_queue_head_t idle_wait;

    spinlock_t spin_lock;
    struct dma_chan *txchan;
    struct dma_chan *rxchan;
    struct scatterlist  sg_rx;  // I/O scatter list
    struct scatterlist  sg_tx;  // I/O scatter list
    struct completion done_tx_dma;
    struct completion done_rx_dma;
    struct device   *dev;
    enum jzdma_type dma_type;
    void *tx_buffer;// dma temp tbuff
    void *rx_buffer;// dma temp rbuff
};

struct jz_spi_param {
    int irq;
    int is_enable;
    int is_finish;

    int spi_miso;
    int spi_mosi;
    int spi_clk;
    const char *irq_name;

    int alter_num;
    struct jz_spi_pin *alter_pin;
};

struct jz_spi_drv *jzspi_handle[1];

static struct jz_spi_pin jz_spi0_pin[3] = {
    {
        {
            {GPIO_PA(24), GPIO_FUNC_2, "spi0_clk"},
            {GPIO_PA(23), GPIO_FUNC_2, "spi0_miso"},
            {GPIO_PA(22), GPIO_FUNC_2, "spi0_mosi"},
        },
    },
    {
        {
            {GPIO_PA(26), GPIO_FUNC_2, "spi0_clk"},
            {GPIO_PA(28), GPIO_FUNC_2, "spi0_miso"},
            {GPIO_PA(29), GPIO_FUNC_2, "spi0_mosi"},
        },
    },
    {
        {
            {GPIO_PD(0), GPIO_FUNC_0, "spi0_clk"},
            {GPIO_PD(3), GPIO_FUNC_0, "spi0_miso"},
            {GPIO_PD(2), GPIO_FUNC_0, "spi0_mosi"},
        },
    },
};

struct jz_spi_param jzspi_dev[] = {
    [0] = {
        .irq = IRQ_SSI0,
        .irq_name = "SSI0",
        .alter_pin = jz_spi0_pin,
        .alter_num = ARRAY_SIZE(jz_spi0_pin),
    },
};

module_param_named(spi0_is_enable, jzspi_dev[0].is_enable, int, 0644);
module_param_gpio_named(spi0_miso, jzspi_dev[0].spi_miso, 0644);
module_param_gpio_named(spi0_mosi, jzspi_dev[0].spi_mosi, 0644);
module_param_gpio_named(spi0_clk, jzspi_dev[0].spi_clk, 0644);

static struct clk *clk_cgu, *clk_sfc;

static void m_release(struct device *dev)
{

}

struct platform_device jz_spi_device[] = {
    {
        .name = "jz-spi0",
        .dev = {
            .release = m_release,
        },
    },
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

static inline int gpio_init(int gpio, enum gpio_function func, const char *name)
{
    int ret = 0;

    ret = gpio_request(gpio, name);
    if (ret < 0)
        return ret;

    gpio_set_func(gpio , func);

    return 0;
}

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
            for (i = 0; i < len; i++)
                p8[i] = spi_read_reg(id, SSIDR) & 0xff;/* read_fifo */
            break;

        case 2:
            for (i = 0; i < len; i++)
                p16[i] = spi_read_reg(id, SSIDR) & 0xffff;
            break;

        case 4:
            for (i = 0; i < len; i++)
                p32[i] = spi_read_reg(id, SSIDR);
            break;

        default:
            break;
    }
}

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
            for (i = 0; i < len; i++)
                spi_write_reg(id, SSIDR, p8[i]);/* write_fifo */
            break;

        case 2:
            for (i = 0; i < len; i++)
                spi_write_reg(id, SSIDR, p16[i]);
            break;

        case 4:
            for (i = 0; i < len; i++)
                spi_write_reg(id, SSIDR, p32[i]);
            break;

        default:
            break;
    }
}

static void jzspi_hal_write_tx_fifo_only(int id, unsigned int len, unsigned int val)
{
    int i = 0;

    for (i = 0; i < len; i++)
        spi_write_reg(id, SSIDR, val);/* write_fifo */
}

volatile unsigned int val_xxxx;

static void jzspi_hal_read_rx_fifo_only(int id, int len)
{
    int i = 0;

    for (i = 0; i < len; i++)
        val_xxxx = spi_read_reg(id, SSIDR);/* read_fifo */
}

static int jzspi_get_cgv(int id, int clk_rate)
{
    int cgv;
    int real_set_rate;
    int prev_rate = clk_rate;
    int cgu_clk_rate = clk_get_rate(clk_cgu);

    if (cgu_clk_rate < clk_rate)
        printk(KERN_ERR "spi set speed invalid, set=%d,max=%d\n", clk_rate, cgu_clk_rate / 2);

    cgv = cgu_clk_rate / 2 / clk_rate - 1;

    if (cgv < 0)
        cgv = 0;
    if (cgv > 255)
        cgv = 255;

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
    int diff = real_set_rate > clk_rate ? real_set_rate - clk_rate : -1 * (real_set_rate - clk_rate);
    if (diff * 100 / clk_rate > 10)
        printk(KERN_DEBUG "SPI%d freq: set=%d,real=%d\n", id, clk_rate, real_set_rate);

    return cgv;
}

static void jzspi_hal_config(int id, struct spi_config_data *config)
{
    int cgv;
    unsigned long ssicr0, ssicr1;

    if (!config->clk_rate)
        config->clk_rate = 1 * 1000 * 1000;

    cgv = jzspi_get_cgv(id, config->clk_rate);
    spi_set_bits(id, SSIGR, SSIGR_CGV, cgv);/* set_clock_dividing_number */

    ssicr1 = spi_read_reg(id, SSICR1);
    set_bit_field(&ssicr1, SSICR1_FRMHL0, 0);/* set_cs_level */
    set_bit_field(&ssicr1, SSICR1_PHA, config->spi_pha);/* set_clk_pha */
    set_bit_field(&ssicr1, SSICR1_POL, config->spi_pol);/* set_clk_pol */
    set_bit_field(&ssicr1, SSICR1_UNFIN, 0);/* set_transmit_fifo_empty_transfer_finish */
    set_bit_field(&ssicr1, SSICR1_FLEN, config->bits_per_word - 2);/* set_data_unit_bits_len */
    spi_write_reg(id, SSICR1, ssicr1);/* write_ssicr1 */

    ssicr0 = spi_read_reg(id, SSICR0);
    set_bit_field(&ssicr0, SSICR0_TIE, 0);/* disable_transmit_interrupt */
    set_bit_field(&ssicr0, SSICR0_TEIE, 0);/* disable_transmit_error_interrupt */
    set_bit_field(&ssicr0, SSICR0_RIE, 0);/* disable_receive_interrupt */
    set_bit_field(&ssicr0, SSICR0_REIE, 0);/* disable_receive_error_interrupt */
    /* set_transfer_endian */
    set_bit_field(&ssicr0, SSICR0_TENDIAN, config->tx_endian);
    set_bit_field(&ssicr0, SSICR0_RENDIAN, config->rx_endian);
    /* set_loop_mode */
    set_bit_field(&ssicr0, SSICR0_LOOP, config->loop_mode);
    set_bit_field(&ssicr0, SSICR0_FSEL, 0);/* select_cs */
    set_bit_field(&ssicr0, SSICR0_EACLRUN, 1);/* set_auto_clear_transmit_fifo_empty_flag */
    set_bit_field(&ssicr0, SSICR0_TFLUSH, 1);/* clear_transmit_fifo */
    set_bit_field(&ssicr0, SSICR0_RFLUSH, 1);/* clear_receive_fifo */
    spi_write_reg(id, SSICR0, ssicr0);/* write_ssicr0 */
}

static void jzspi_write_fifo(struct jz_spi_drv *hw, int bits_per_word, int total_tlen, const void *buf, int tlen, int n)
{
    int id = hw->id;

    if (total_tlen < n)
        n = total_tlen;

    if (tlen > 0) {
        if (tlen > n) {
            jzspi_hal_write_tx_fifo(id, bits_per_word, buf, n);
        } else {
            jzspi_hal_write_tx_fifo(id, bits_per_word, buf, tlen);
            jzspi_hal_write_tx_fifo_only(id, n - tlen, 0x0);
        }
    } else {
        jzspi_hal_write_tx_fifo_only(id, n, 0x0);
    }
}

static void jzspi_read_fifo(struct jz_spi_drv *hw, int bits_per_word, void *buf, int rlen, int n)
{
    int id = hw->id;

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

static irqreturn_t spi_intr_handler(int irq, void *dev)
{
    int len, n;
    struct jz_spi_drv *hw = (struct jz_spi_drv *)dev;
    int id = hw->id;
    int bytes_per_word = to_bytes(hw->config.bits_per_word);

    unsigned long ssisr = spi_read_reg(id, SSISR);/* read_ssisr */
    /* get_transmit_fifo_underrun_flag */
    if (get_bit_field(&ssisr, SSISR_TUNDR)) {
        set_bit_field(&ssisr, SSISR_TUNDR, 0);/* clear_transmit_fifo_underrrun_flag */
        spi_write_reg(id, SSISR, ssisr);/* write_ssisr */
    }

    /* get_receive_fifo_overrun_flag */
    if (get_bit_field(&ssisr, SSISR_ROVER)) {
        set_bit_field(&ssisr, SSISR_ROVER, 0);/* clear_receive_fifo_overrun_flag */
        spi_write_reg(id, SSISR, ssisr);/* write_ssisr */
    }

    /* get_receive_fifo_more_threshold_flag && get_receive_interrupt_control_value */
    if (spi_get_bits(id, SSISR, SSISR_RFHF) && spi_get_bits(id, SSICR0, SSICR0_RIE)) {
        if (hw->total_tx_len > 0) {
            len = spi_get_bits(id, SSISR, SSISR_RFIFO_NUM);/* get_receive_fifo_number */
            jzspi_read_fifo(hw, hw->config.bits_per_word, hw->rbuff, hw->rlen, len);
            hw->rlen -= len;
            hw->total_rx_len -= len;
            hw->rbuff += (len * bytes_per_word);

            jzspi_write_fifo(hw, hw->config.bits_per_word, hw->total_tx_len, hw->tbuff, hw->tlen, len);
            hw->tlen -= len;
            hw->total_tx_len -= len;
            hw->tbuff += (len * bytes_per_word);
        } else {
            len = spi_get_bits(id, SSISR, SSISR_RFIFO_NUM);/* get_receive_fifo_number */
            jzspi_read_fifo(hw, hw->config.bits_per_word, hw->rbuff, hw->rlen, len);
            hw->rlen -= len;
            hw->total_rx_len -= len;
            hw->rbuff += (len * bytes_per_word);

            /* 判断传输完成，设置状态，若传输完成则调用 回调函数 */
            if (hw->total_rx_len <= 0) {
                spi_set_bits(id, SSICR0, SSICR0_RIE, 0);/* disable_receive_interrupt */
                hw->working_flag = 0;
                wake_up(&hw->idle_wait);
                return IRQ_HANDLED;
            }
        }

        if (hw->total_rx_len < MAX_FIFO_LEN)
            n = hw->total_rx_len;
        else
            n = MAX_FIFO_LEN * 3 / 4;

        /* 根据 len 大小设置 阈值 */
        spi_set_bits(id, SSICR1, SSICR1_RTRG, n / 8);/* set_receive_threshold */
    }

    return IRQ_HANDLED;
}

static int spi_write_read_inr(struct jz_spi_drv * hw,
    void * tx_buf, int tlen, void * rx_buf, int rlen)
{
    int total_len, n;
    int timeout;
    struct spi_config_data *config = &hw->config;
    int id = hw->id;
    int bytes_per_word = to_bytes(config->bits_per_word);

    if (tx_buf == NULL)
        tlen = 0;
    if (rx_buf == NULL)
        rlen = 0;

    total_len = tlen > rlen ? tlen : rlen;
    if (!total_len)
        return -EINVAL;

    if (hw->working_flag) {
        printk(KERN_ERR "SPI%d busy.\n", id);
        return -EBUSY;
    }

    hw->tbuff  = tx_buf ? tx_buf : hw->tx_buffer;
    hw->rbuff  = rx_buf ? rx_buf : hw->rx_buffer;
    hw->tlen   = tlen;
    hw->rlen   = rlen;
    hw->total_tx_len = total_len;
    hw->total_rx_len = total_len;
    hw->working_flag = 1;

    if (total_len < MAX_FIFO_LEN)
        n = total_len;
    else
        n = MAX_FIFO_LEN * 3 / 4;

    /* set_transmit_threshold & set_receive_threshold */
    unsigned long ssicr1 = spi_read_reg(id, SSICR1);/* read_ssicr1 */
    set_bit_field(&ssicr1, SSICR1_TTRG, 0);
    set_bit_field(&ssicr1, SSICR1_RTRG, n / 8);
    spi_write_reg(id, SSICR1, ssicr1);/* write_ssicr1 */

    /* 第一次 写FIFO 写满 */
    jzspi_write_fifo(hw, config->bits_per_word, total_len, hw->tbuff, hw->tlen, MAX_FIFO_LEN);
    hw->tlen -= MAX_FIFO_LEN;
    hw->tbuff += (MAX_FIFO_LEN * bytes_per_word);
    hw->total_tx_len -= MAX_FIFO_LEN;

    spi_set_bits(id, SSICR0, SSICR0_RIE, 1);/* enable_receive_interrupt */

    timeout = wait_event_timeout(hw->idle_wait, !hw->working_flag, HZ);
    if (timeout == 0) {
        printk(KERN_ERR "SPI: spi transfer timeout\n");
        return -ETIMEDOUT;
    }

    return 0;
}

static void dma_tx_callback(void *data)
{
    struct jz_spi_drv *hw = data;
    complete(&hw->done_tx_dma);
}

static void dma_rx_callback(void *data)
{
    struct jz_spi_drv *hw = data;
    complete(&hw->done_rx_dma);
}

static int do_spi_write_read_dma(struct jz_spi_drv * hw,
    void * tx_buf, void * rx_buf, int tlen, int rlen)
{
    struct dma_async_tx_descriptor *rxdesc;
    struct dma_async_tx_descriptor *txdesc;
    struct dma_chan *rxchan = hw->rxchan;
    struct dma_chan *txchan = hw->txchan;
    int id = hw->id;
    int ret;

    if (tlen > 0) {
        INIT_COMPLETION(hw->done_tx_dma);
        sg_init_one(&hw->sg_tx, tx_buf, tlen);
        if (dma_map_sg(hw->txchan->device->dev,
                &hw->sg_tx, 1, DMA_TO_DEVICE) != 1) {
            printk(KERN_ERR "SPI%d dma_map_sg tx error.\n", id);
            BUG();
        }

        txdesc = txchan->device->device_prep_slave_sg(txchan,
                &hw->sg_tx, 1,
                DMA_TO_DEVICE, DMA_PREP_INTERRUPT | DMA_CTRL_ACK,NULL);
        if (!txdesc) {
            printk(KERN_ERR "SPI%d device_prep_slave_sg tx error.\n", id);
            BUG();
        }

        txdesc->callback = dma_tx_callback;
        txdesc->callback_param = hw;

        dmaengine_submit(txdesc);
        dma_async_issue_pending(txchan);
    }

    if (rlen > 0) {
        INIT_COMPLETION(hw->done_rx_dma);
        sg_init_one(&hw->sg_rx, rx_buf, rlen);
        if (dma_map_sg(hw->rxchan->device->dev,
                &hw->sg_rx, 1, DMA_FROM_DEVICE) != 1) {
            printk(KERN_ERR "SPI%d dma_map_sg rx error.\n", id);
            BUG();
        }

        rxdesc = rxchan->device->device_prep_slave_sg(rxchan,
                &hw->sg_rx, 1,
                DMA_FROM_DEVICE,
                DMA_PREP_INTERRUPT | DMA_CTRL_ACK,NULL);
        if (!rxdesc) {
            printk(KERN_ERR "SPI%d device_prep_slave_sg rx error.\n", id);
            BUG();
        }

        rxdesc->callback = dma_rx_callback;
        rxdesc->callback_param = hw;

        dmaengine_submit(rxdesc);
        dma_async_issue_pending(rxchan);

        ret = wait_for_completion_interruptible_timeout(&hw->done_rx_dma, 10 * HZ);
        if (ret <= 0) {
            printk(KERN_ERR "SPI: spi rx transfer timeout\n");
            goto dma_rx_err;
        }
        dma_unmap_sg(hw->rxchan->device->dev, &hw->sg_rx, 1, DMA_FROM_DEVICE);
    }

    if (tlen > 0) {
        ret = wait_for_completion_interruptible_timeout(&hw->done_tx_dma, 10 * HZ);
        if (ret <= 0) {
            printk(KERN_ERR "SPI: spi tx transfer timeout\n");
            goto dma_tx_err;
        }
        dma_unmap_sg(hw->txchan->device->dev, &hw->sg_tx, 1, DMA_TO_DEVICE);
    }

    return 0;

dma_rx_err:
    if (rlen > 0)
        dma_unmap_sg(rxchan->device->dev, &hw->sg_rx, 1, DMA_FROM_DEVICE);
dma_tx_err:
    if (tlen > 0)
        dma_unmap_sg(txchan->device->dev, &hw->sg_tx, 1, DMA_TO_DEVICE);

    return -ETIMEDOUT;
}

static int spi_write_read_dma(struct jz_spi_drv * hw,
    void * tx_buf, int tlen, void * rx_buf, int rlen)
{
    struct dma_slave_config rx_config, tx_config;
    struct spi_config_data *config = &hw->config;
    unsigned int bytes_per_word = to_bytes(config->bits_per_word);

    int id = hw->id;
    int ret = 0;
    dma_addr_t ssidr_addr = spi_hal_get_dma_ssidr_addr(id);

    if (tx_buf == NULL)
        tlen = 0;

    if (rx_buf == NULL)
        rlen = 0;

    int total_len = tlen > rlen ? tlen : rlen;
    if (!total_len)
        return -EINVAL;

    int total_size = total_len * bytes_per_word;
    int maxburst;/* unit is bytes */
    if (total_size % 64 == 0) maxburst = 64;
    else if (total_size % 32 == 0) maxburst = 32;
    else if (total_size % 16 == 0) maxburst = 16;
    else                maxburst = bytes_per_word;

    if (bytes_per_word == 1) {
        tx_config.dst_addr_width = DMA_SLAVE_BUSWIDTH_1_BYTE;
        tx_config.src_addr_width = DMA_SLAVE_BUSWIDTH_1_BYTE;
        rx_config.src_addr_width = DMA_SLAVE_BUSWIDTH_1_BYTE;
        rx_config.dst_addr_width = DMA_SLAVE_BUSWIDTH_1_BYTE;
    } else if (bytes_per_word == 2) {
        tx_config.dst_addr_width = DMA_SLAVE_BUSWIDTH_2_BYTES;
        tx_config.src_addr_width = DMA_SLAVE_BUSWIDTH_2_BYTES;
        rx_config.src_addr_width = DMA_SLAVE_BUSWIDTH_2_BYTES;
        rx_config.dst_addr_width = DMA_SLAVE_BUSWIDTH_2_BYTES;
    } else {
        tx_config.dst_addr_width = DMA_SLAVE_BUSWIDTH_4_BYTES;
        tx_config.src_addr_width = DMA_SLAVE_BUSWIDTH_4_BYTES;
        rx_config.src_addr_width = DMA_SLAVE_BUSWIDTH_4_BYTES;
        rx_config.dst_addr_width = DMA_SLAVE_BUSWIDTH_4_BYTES;
    }

    tx_config.dst_maxburst = maxburst;
    tx_config.src_maxburst = maxburst;
    rx_config.src_maxburst = maxburst;
    rx_config.dst_maxburst = maxburst;

    tx_config.dst_addr = ssidr_addr;
    rx_config.src_addr = ssidr_addr;

    tx_config.direction = DMA_MEM_TO_DEV;
    rx_config.direction = DMA_DEV_TO_MEM;

    dmaengine_slave_config(hw->txchan, &tx_config);
    dmaengine_slave_config(hw->rxchan, &rx_config);

    int n = MAX_FIFO_LEN - (maxburst / bytes_per_word);
    /* set_receive_threshold && set_transmit_threshold */
    unsigned long ssicr1 = spi_read_reg(id, SSICR1);/* read_ssicr1 */
    set_bit_field(&ssicr1, SSICR1_TTRG, n / 8);
    set_bit_field(&ssicr1, SSICR1_RTRG, (maxburst / bytes_per_word) / 8);
    spi_write_reg(id, SSICR1, ssicr1);/* write_ssicr1 */

    if (tlen <= MAX_FIFO_LEN)
        n = tlen;
    else
        n = DMA_MAX_BURST;

    int txdma_size = total_size;
    int rxdma_size = total_size;

    /* 发送速率小于10Mhz或发送长度少于MAX_FIFO_LEN时提前写发送数据 以节省时间 */
    if (config->clk_rate <= 10000000 || tlen <= MAX_FIFO_LEN) {
        jzspi_write_fifo(hw, config->bits_per_word, total_len, tx_buf, tlen, n);
        tlen = n > tlen ? 0 : tlen - n;
        txdma_size -= n * bytes_per_word;
        tx_buf += n * bytes_per_word;
    }

    /* 由于目前代码实现是运行时发送与接收同步, 且传输由发送触发故只读只写时需要配套运行发送接收 */
    /* eg:  bytes_per_word = 1, total_len = 1024, n = 64
     *  1. tlen = 1024, rlen = 1024  ==>  txdma_size = 1024, rxdma_size = 1024
            提前写入64个tx_fifo后txdma_size为960, 需要发送960个tx_fifo及接收1024个rx_fifo

     *  2. tlen = 0, rlen = 1024  ==>  txdma_size = 1024, rxdma_size = 1024, tlen < 0 ==> n = 0
            txdma_size为1024, 需要发送1024个无效数据0配合数据接收

     *  3. tlen = 1024, rlen = 0  ==>  txdma_size = 1024, rxdma_size = 1024
            提前写入64个tx_fifo后txdma_size为960, 需要发送960个tx_fifo及读取1024个rx_fifo
     */
    int tx, rx;
    int tsize = tlen * bytes_per_word;
    int rsize = rlen * bytes_per_word;
    while (txdma_size > 0 || rxdma_size > 0) {
        if (txdma_size > BUFFER_SIZE)
            tx = BUFFER_SIZE;
        else
            tx = txdma_size;

        if (rxdma_size > BUFFER_SIZE)
            rx = BUFFER_SIZE;
        else
            rx = rxdma_size;

        if (tsize >= tx)
            memcpy(hw->tx_buffer, tx_buf, tx);
        else {
            memcpy(hw->tx_buffer, tx_buf, tsize);
            memset(hw->tx_buffer + tsize, 0, tx - tsize);
        }

        ret = do_spi_write_read_dma(hw, hw->tx_buffer, hw->rx_buffer, tx, rx);
        if (ret)
            return ret;
        txdma_size -= tx;
        rxdma_size -= rx;

        if (rsize >= rx)
            memcpy(rx_buf, hw->rx_buffer, rx);
        else
            memcpy(rx_buf, hw->rx_buffer, rsize);

        tx_buf = tx_buf + tx;
        rx_buf = rx_buf + rx;
        tsize = tx > tsize ? 0 : tsize - tx;
        rsize = rx > rsize ? 0 : rsize - rx;
    }

    return 0;
}

static int jz_spi_config(struct jz_spi_drv *hw, struct spi_device *spi)
{
    BUG_ON(hw->cur_bpw < 2 || hw->cur_bpw > 32);

    hw->config.clk_rate = hw->cur_speed;
    hw->config.bits_per_word = hw->cur_bpw;

    if (spi->mode & SPI_LSB_FIRST) {
        hw->config.tx_endian = Spi_endian_lsb_first;
        hw->config.rx_endian = Spi_endian_lsb_first;
    } else {
        hw->config.tx_endian = Spi_endian_msb_first;
        hw->config.rx_endian = Spi_endian_msb_first;
    }

    hw->config.spi_pha = !!(spi->mode & SPI_CPHA);

    hw->config.spi_pol = !!(spi->mode & SPI_CPOL);

    hw->config.loop_mode = !!(spi->mode & SPI_LOOP);

    jzspi_hal_config(hw->id, &hw->config);

    return 0;
}

static int jz_spi_setup(struct spi_device *spi)
{
    int ret = 0;
    int cs_pin = (int)spi->controller_data;
    unsigned int idle_level = spi->mode & SPI_CS_HIGH ? 0 : 1;

    if (spi->controller_state == NULL) {
        if (cs_pin >= 0) {
            ret = gpio_request(cs_pin, "SPI_CS");
            if (ret) {
                printk(KERN_ERR "SPI: spi request cs_pin failed.\n");
                return ret;
            }
            gpio_direction_output(cs_pin, idle_level);
            spi->controller_state = spi->controller_data;
        }
    }

    return 0;
}

static void jz_spi_cleanup(struct spi_device *spi)
{
    int cs_pin = (int)spi->controller_data;

    if (cs_pin >= 0)
        gpio_free(cs_pin);

    spi->controller_state = NULL;
}

static int check_transfer_len(struct spi_transfer *t)
{
    int bytes_per_word = to_bytes(t->bits_per_word);

    if (t->len % bytes_per_word) {
        printk(KERN_ERR "len=%d must ALIGN %d\n", t->len, bytes_per_word);
        return -1;
    }

    return t->len / bytes_per_word;
}

static int jz_spi_transfer(struct spi_device * spi, struct spi_transfer *t)
{
    int len;
    int ret = 0;

    struct jz_spi_drv * hw = spi_master_get_devdata(spi->master);

    len = check_transfer_len(t);
    if(len < 0)
        return -1;

    if (spi_is_enable_dma)
        ret = spi_write_read_dma(hw, (void *)t->tx_buf, len, t->rx_buf, len);
    else
        ret = spi_write_read_inr(hw, (void *)t->tx_buf, len, t->rx_buf, len);

    return ret;
}

static void jz_spi_init_setting(int id)
{
    unsigned long ssicr0 = spi_read_reg(id, SSICR0);/* read_ssicr0 */
    set_bit_field(&ssicr0, SSICR0_SSIE, 0);/* disable_spi */
    set_bit_field(&ssicr0, SSICR0_EACLRUN, 1);/* set_auto_clear_transmit_fifo_empty_flag */
    set_bit_field(&ssicr0, SSICR0_TFLUSH, 1);/* clear_transmit_fifo */
    set_bit_field(&ssicr0, SSICR0_RFLUSH, 1);/* clear_receive_fifo */
    set_bit_field(&ssicr0, SSICR0_TIE, 0);/* disable_transmit_interrupt */
    set_bit_field(&ssicr0, SSICR0_RFINE, 0);/* disable_receive_finish_control */
    set_bit_field(&ssicr0, SSICR0_RFINC, 0);/* set_receive_continue */
    set_bit_field(&ssicr0, SSICR0_VRCNT, 0);/* receive_counter_unvalid */
    set_bit_field(&ssicr0, SSICR0_TFMODE, 0);/* set_transmit_fifo_empty_mode */
    spi_write_reg(id, SSICR0, ssicr0);/* write_ssicr0 */

    /* 设置 TTRG RTRG */
    unsigned long ssicr1 = spi_read_reg(id, SSICR1);/* read_ssicr1 */
    set_bit_field(&ssicr1, SSICR1_TTRG, 0);/* set_transmit_threshold */
    set_bit_field(&ssicr1, SSICR1_RTRG, 0);/* set_receive_threshold */
    set_bit_field(&ssicr1, SSICR1_TFVCK, 0);/* set_start_delay_clk */
    set_bit_field(&ssicr1, SSICR1_TCKFI, 0);/* set_stop_delay_clk */
    /* standard spi format */
    set_bit_field(&ssicr1, SSICR1_FMAT, 0);/* set_standard_transfer_format */
    spi_write_reg(0, SSICR1, ssicr1);/* write_ssicr1 */

    /* disable interval mode */
    spi_set_bits(id, SSIITR, SSIITR_IVLTM, 0);/* set_interval_time */
}

static int m_gpio_request(int gpio, struct jz_spi_param *drv, int id)
{
    int i;
    int ret = 0;
    char buf[10];
    int num = drv->alter_num;
    struct jz_spi_pin *spi_pin_alter = drv->alter_pin;
    struct jz_func_alter *spi_pin;

    if (gpio < 0)
        return 0;

    for (i = 0; i < num; i++) {
        spi_pin = spi_pin_alter[i].spi_pins;
        if (gpio == spi_pin[id].pin) {
            ret = gpio_init(gpio, spi_pin[id].function, spi_pin[id].name);
            if (ret < 0) {
                printk(KERN_ERR "SPI failed to request %s: %s!\n", spi_pin[id].name, gpio_to_str(gpio, buf));
                return -EINVAL;
            }

            return 0;
        }
    }

    printk(KERN_ERR "SPI %s(%s) is invaild!\n", spi_pin[id].name, gpio_to_str(gpio, buf));
    return -EINVAL;
}

static int spi_gpio_request(int id, struct jz_spi_param *drv)
{
    int ret = 0;

    if (drv->spi_clk < 0) {
        printk(KERN_ERR "SPI%d spi_clk no be set.\n", id);
        return -EINVAL;
    }

    ret = m_gpio_request(drv->spi_clk, drv, 0);
    if (ret)
        return -EINVAL;

    ret = m_gpio_request(drv->spi_miso, drv, 1);
    if (ret)
        goto err_miso;

    ret = m_gpio_request(drv->spi_mosi, drv, 2);
    if (ret)
        goto err_mosi;


    return 0;

err_mosi:
    if (drv->spi_miso >= 0 )
        gpio_free(drv->spi_miso);
err_miso:
    gpio_free(drv->spi_clk);

    return -EINVAL;
}

static void spi_gpio_release(int id)
{
    if (jzspi_dev[id].spi_miso >= 0 )
        gpio_free(jzspi_dev[id].spi_miso);
    if (jzspi_dev[id].spi_mosi >= 0)
        gpio_free(jzspi_dev[id].spi_mosi);
    if (jzspi_dev[id].spi_clk >= 0)
        gpio_free(jzspi_dev[id].spi_clk);
}

static bool spi_dma_chan_filter(struct dma_chan *chan, void *param)
{
    struct jz_spi_drv *hw = param;

    return hw->dma_type == (int)chan->private;
}

static void spi_dma_init(struct jz_spi_drv *hw)
{
    int id = hw->id;

    init_completion(&hw->done_tx_dma);
    init_completion(&hw->done_rx_dma);

    hw->tx_buffer = (void*)__get_free_pages(GFP_KERNEL | GFP_DMA, get_order(BUFFER_SIZE));
    if (!hw->tx_buffer) {
        printk(KERN_ERR "SPI%d Failed to alloc dma temp tx_buffer.\n", id);
        BUG();
    }
    assert(((unsigned long)(hw->tx_buffer) % DMA_MAX_BURST) == 0);

    hw->rx_buffer = (void*)__get_free_pages(GFP_KERNEL | GFP_DMA, get_order(BUFFER_SIZE));
    if (!hw->rx_buffer) {
        printk(KERN_ERR "SPI%d Failed to alloc dma temp rx_buffer.\n", id);
        BUG();
    }
    assert(((unsigned long)(hw->rx_buffer) % DMA_MAX_BURST) == 0);

    memset(hw->tx_buffer, 0, BUFFER_SIZE);
    memset(hw->rx_buffer, 0, BUFFER_SIZE);

    dma_cap_mask_t mask;
    dma_cap_zero(mask);
    dma_cap_set(DMA_SLAVE, mask);

    hw->dma_type = spi_dma_type[id];

    hw->txchan = dma_request_channel(mask, spi_dma_chan_filter, hw);
    if(!hw->txchan) {
        printk(KERN_ERR "SPI%d request dma tx channel failed.\n", id);
        BUG();
    }

    hw->rxchan = dma_request_channel(mask, spi_dma_chan_filter, hw);
    if(!hw->rxchan) {
        printk(KERN_ERR "SPI%d request dma rx channel failed.\n", id);
        BUG();
    }
}

static void spi_dma_deinit(struct jz_spi_drv *hw)
{
    if (hw->rxchan)
        dma_release_channel(hw->rxchan);
    if (hw->txchan)
        dma_release_channel(hw->txchan);

    __free_page(hw->tx_buffer);
    __free_page(hw->rx_buffer);
}

static inline void enable_cs(struct jz_spi_drv *hw, struct spi_device *spi)
{
    int cs_pin = (int)spi->controller_data;
    if (cs_pin >= 0)
        gpio_set_value(cs_pin, spi->mode & SPI_CS_HIGH ? 1 : 0);
}

static inline void disable_cs(struct jz_spi_drv *hw, struct spi_device *spi)
{
    int cs_pin = (int)spi->controller_data;
    if (cs_pin >= 0)
        gpio_set_value(cs_pin, spi->mode & SPI_CS_HIGH ? 0 : 1);
}

static int jz_spi_transfer_one_message(struct spi_master *master, struct spi_message *msg)
{
    struct jz_spi_drv *hw = spi_master_get_devdata(master);
    struct spi_device *spi = msg->spi;
    struct spi_transfer *xfer;
    int cs_change = 0;
    int status = 0;
    u32 speed;
    u8 bpw;

    if (hw->tgl_spi != NULL) { /* If last device toggled after mssg */
        if (hw->tgl_spi != spi) { /* if last mssg on diff device */
            disable_cs(hw, hw->tgl_spi);
            cs_change = 1;
        }
        hw->tgl_spi = NULL;
    } else {
        cs_change = 1;
    }

    /* If Master's(controller) state differs from that needed by Slave */
    if (hw->cur_speed != spi->max_speed_hz
            || hw->cur_mode != spi->mode
            || hw->cur_bpw != spi->bits_per_word) {
        hw->cur_bpw = spi->bits_per_word;
        hw->cur_speed = spi->max_speed_hz;
        hw->cur_mode = spi->mode;
        spi_set_bits(hw->id, SSICR0, SSICR0_SSIE, 0);/* disable_spi */
        jz_spi_config(hw, spi);
    }

    spi_set_bits(hw->id, SSICR0, SSICR0_SSIE, 1);/* enable_spi */

    list_for_each_entry(xfer, &msg->transfers, transfer_list) {
        /* Only BPW and Speed may change across transfers */
        bpw = xfer->bits_per_word;
        speed = xfer->speed_hz ? : spi->max_speed_hz;

        if (xfer->len % (bpw / 8)) {
            printk(KERN_ERR "Xfer length(%u) not a multiple of word size(%u)\n", xfer->len, bpw / 8);
            status = -EIO;
            goto out;
        }

        if (bpw != hw->cur_bpw || speed != hw->cur_speed) {
            hw->cur_bpw = bpw;
            hw->cur_speed = speed;
            spi_set_bits(hw->id, SSICR0, SSICR0_SSIE, 0);/* disable_spi */
            jz_spi_config(hw, spi);
            spi_set_bits(hw->id, SSICR0, SSICR0_SSIE, 1);/* enable_spi */
        }

        if (cs_change)
            enable_cs(hw, spi);

        cs_change = xfer->cs_change;
        if (!xfer->tx_buf && !xfer->rx_buf && xfer->len) {
            status = -EINVAL;
            break;
        }

        if (xfer->len) {
            status = jz_spi_transfer(spi, xfer);
            if (status != 0)
                break;
        }

        if (xfer->delay_usecs)
            udelay(xfer->delay_usecs);

        if (cs_change && !list_is_last(&xfer->transfer_list, &msg->transfers))
            disable_cs(hw, spi);

        msg->actual_length += xfer->len;
    }
out:
    if (!(status == 0 && cs_change)) {
        disable_cs(hw, spi);
        spi_set_bits(hw->id, SSICR0, SSICR0_SSIE, 0);/* disable_spi */
    } else
        hw->tgl_spi = spi;

    msg->status = status;

    spi_finalize_current_message(master);

    return 0;
}

static void spi_init(int id)
{
    int ret;
    struct jz_spi_drv *hw;
    struct jz_spi_param *drv = &jzspi_dev[id];
    struct spi_master * master;
    char tmp_name[128];

    ret = spi_gpio_request(id, drv);
    if (ret < 0) {
        printk(KERN_ERR "spi%d gpio requeset failed\n", id);
        return;
    }

    ret = platform_device_register(&jz_spi_device[id]);
    BUG_ON(ret);

    master = spi_alloc_master(&jz_spi_device[id].dev, sizeof(struct jz_spi_drv));
    BUG_ON(master == NULL);

    hw = spi_master_get_devdata(master);
    hw->master = spi_master_get(master);

    init_waitqueue_head(&hw->idle_wait);

    sprintf(tmp_name, "ssi%d", id);
    hw->clk = clk_get(NULL, tmp_name);
    BUG_ON(IS_ERR(hw->clk));

    clk_enable(hw->clk);

    jz_spi_init_setting(id);

    master->mode_bits = (SPI_CPOL | SPI_CPHA | SPI_CS_HIGH | SPI_LSB_FIRST | SPI_LOOP);
    master->bus_num = id;
    master->num_chipselect = 8;

    master->setup = jz_spi_setup;
    master->cleanup = jz_spi_cleanup;
    master->transfer_one_message = jz_spi_transfer_one_message;

    hw->id = id;
    hw->irq = jzspi_dev[id].irq;

    ret = spi_register_master(master);
    BUG_ON(ret);

    hw->dev = &jz_spi_device[id].dev;

    if (spi_is_enable_dma) {
        spi_dma_init(hw);
    } else {
        ret = request_irq(jzspi_dev[id].irq, spi_intr_handler, 0, jzspi_dev[id].irq_name , hw);
        BUG_ON(ret);
    }

    jzspi_handle[id] = hw;

    drv->is_finish = 1;
}

static void spi_exit(int id)
{
    struct jz_spi_drv *hw = jzspi_handle[id];

    if (hw) {
        spi_unregister_master(hw->master);

        spi_set_bits(id, SSICR0, SSICR0_SSIE, 0);/* disable_spi */
        spi_dma_deinit(hw);

        free_irq(hw->irq, hw);
        clk_disable(hw->clk);
        clk_put(hw->clk);
        spi_gpio_release(id);
        platform_device_unregister(&jz_spi_device[id]);
        jzspi_handle[id] = NULL;
    }
}

static int __init jz_spi_init(void)
{
    clk_cgu = clk_get(NULL, "cgu_ssi");
    BUG_ON(IS_ERR(clk_cgu));

    clk_sfc = clk_get(NULL, "cgu_sfc");
    BUG_ON(IS_ERR(clk_sfc));

    clk_set_parent(clk_cgu, clk_sfc);

    if (jzspi_dev[0].is_enable)
        spi_init(0);

    return 0;
}

static void __exit jz_spi_exit(void)
{
    if (jzspi_dev[0].is_finish)
        spi_exit(0);

    clk_put(clk_cgu);
    clk_put(clk_sfc);
}

module_init(jz_spi_init);
module_exit(jz_spi_exit);

MODULE_DESCRIPTION("JZ X1000 SPI Controller Driver");
MODULE_LICENSE("GPL");
