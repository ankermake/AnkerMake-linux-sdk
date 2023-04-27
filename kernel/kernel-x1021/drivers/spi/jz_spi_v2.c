#include <linux/module.h>
#include <linux/init.h>
#include <linux/kernel.h>
#include <linux/moduleparam.h>
#include <linux/spinlock.h>
#include <linux/workqueue.h>
#include <linux/device.h>
#include <linux/platform_device.h>
#include <linux/clk.h>
#include <linux/gpio.h>
#include <linux/io.h>
#include <linux/irq.h>
#include <linux/interrupt.h>
#include <linux/wait.h>

#include <linux/spi/spi.h>
#include <linux/spi/spi_bitbang.h>

#include <mach/jzssi.h>

#include "jz_spi_regs.h"
#include <hal_x1021/jz_spi_hal.h>

#include <jz_spi_v2.h>

#define MAX_FIFO_LEN 128

struct jz_spi {
    //spi_bitbang 结构体必须放在 第一个!
    struct spi_bitbang  bitbang;

    int id;
    int irq;
    struct device *dev;
    struct spi_master *master;
    void __iomem    *iomem;
    struct resource *ioarea;

    struct jz_spi_info *pdata;  //平台设备私有数据

    struct jzspi_hal_config config; //SPI控制器配置结构体

    int tx_real_len;    //剩余需要发送数据的长度
    int rx_real_len;    //剩余需要接收数据的长度
    int tx_len;         //需要发送总长度
    int rx_len;         //需要接收总长度
    const void *tx_buff;    //发送buff
    void *rx_buff;          //接收buff

    struct clk *clk;        //SSI 控制器时钟
    struct clk *clk_cgu;    //cgu_spi 时钟
    struct clk *parent_clk;

    unsigned int working_flag;  //工作状态：    1：繁忙，0：空闲
    wait_queue_head_t idle_wait;
};

#ifdef TEST_JZ_SPI_INTERFACE
static struct jz_spi *test_spi_hw[2];
static struct jzspi_config_data *test_spi_config[2];
#endif

static inline void clear_rx_fifo(void *base_addr)
{
    unsigned int c;

    while (jzspi_hal_get_rx_fifo_num(base_addr))
        jzspi_hal_read_rx_fifo(base_addr, 32, &c, 1);
}

static inline void wait_tx_fifo_empty(void *base_addr)
{
    while (jzspi_hal_get_tx_fifo_num(base_addr));
}

static void jzspi_write_fifo(void *base_addr, int bits_per_word, int len, const void *buf, int tlen, int n)
{
    if(len < n) {
        n = len;
    }

    if (tlen > 0) {
        if (tlen > n) {
            jzspi_hal_write_tx_fifo(base_addr, bits_per_word, buf, n);
        } else {
            jzspi_hal_write_tx_fifo(base_addr, bits_per_word, buf, tlen);
            jzspi_hal_write_tx_fifo_only(base_addr, n - tlen, 0xF);
        }

    } else {
        jzspi_hal_write_tx_fifo_only(base_addr, n, 0xF);
    }
}

static void jzspi_read_fifo(void *base_addr, int bits_per_word, void *buf, int rlen, int n)
{
    if (rlen > 0) {
        if (rlen < n) {
            jzspi_hal_read_rx_fifo(base_addr, bits_per_word, buf, rlen);
            jzspi_hal_read_rx_fifo_only(base_addr, n - rlen);
        } else {
            jzspi_hal_read_rx_fifo(base_addr, bits_per_word, buf, n);
        }

    } else {
        jzspi_hal_read_rx_fifo_only(base_addr, n);
    }
}

static void spi_write_read_inr(struct jz_spi * hw,
    const void * tx_buf, unsigned int tlen, void * rx_buf, unsigned int rlen)
{
    int len;
    struct jzspi_hal_config *config = &hw->config;
    int bytes_per_word = to_bytes(config->bits_per_word);

    if (tx_buf == NULL)
        tlen = 0;

    if (rx_buf == NULL)
        rlen = 0;

    len = tlen > rlen ? tlen : rlen;

    if(len == 0)
        return;

    BUG_ON(hw->working_flag);

    hw->tx_buff = tx_buf;
    hw->rx_buff = rx_buf;
    hw->tx_real_len = tlen;
    hw->rx_real_len = rlen;
    hw->tx_len = len;
    hw->rx_len = len;
    hw->working_flag = 1;

    wait_tx_fifo_empty(hw->iomem);
    clear_rx_fifo(hw->iomem);

    if (len > MAX_FIFO_LEN) {
        jzspi_hal_set_rx_fifo_threshold(hw->iomem, MAX_FIFO_LEN * 3 / 4);
    } else {
        jzspi_hal_set_rx_fifo_threshold(hw->iomem, len);
    }

    //第一次 写FIFO 写满
    jzspi_write_fifo(hw->iomem, config->bits_per_word, len, hw->tx_buff, tlen, MAX_FIFO_LEN);
    hw->tx_real_len -= MAX_FIFO_LEN;
    hw->tx_buff += (MAX_FIFO_LEN * bytes_per_word);
    hw->tx_len -= MAX_FIFO_LEN;

    jzspi_set_bit(hw->iomem, SSISR, SSISR_TUNDR, 0);
    jzspi_set_bit(hw->iomem, SSICR0, SSICR0_RIE, 1);

}

static void spi_start_transfer(struct jz_spi *hw)
{
    clk_enable(hw->clk);

    jzspi_hal_enable_transfer(hw->iomem, &hw->config);
}

static void spi_stop_transfer(struct jz_spi *hw)
{
    jzspi_hal_disable_transfer(hw->iomem);
    hw->working_flag = 0;
    clk_disable(hw->clk);
}

static irqreturn_t spi_intr_handler(int irq, void *dev)
{
    int len;
    struct jz_spi *hw = (struct jz_spi *)dev;
    struct jzspi_hal_config *config = &hw->config;
    int bytes_per_word = to_bytes(config->bits_per_word);

    int n = MAX_FIFO_LEN * 3 / 4;
    unsigned int ssisr  = jzspi_read_reg(hw->iomem, SSISR);
    unsigned int ssicr0 = jzspi_read_reg(hw->iomem, SSICR0);

    if (get_bit_field(&ssisr, SSISR_TUNDR)) {
        printk(KERN_DEBUG "UNDER\n");
        jzspi_set_bit(hw->iomem, SSISR, SSISR_TUNDR, 0);
    }

    if (get_bit_field(&ssisr,SSISR_ROVER)) {
        printk(KERN_DEBUG "ROVER come in\n");
        jzspi_set_bit(hw->iomem, SSISR, SSISR_ROVER, 0);
    }

    if (get_bit_field(&ssicr0, SSICR0_RIE) && get_bit_field(&ssisr,SSISR_RFHF)) {
        if (hw->tx_len > 0) {
            jzspi_set_bit(hw->iomem, SSISR, SSISR_TUNDR, 0);
            len = jzspi_get_bit(hw->iomem, SSISR, SSISR_RFIFO_NUM);
            jzspi_read_fifo(hw->iomem, config->bits_per_word, hw->rx_buff, hw->rx_real_len, len);
            hw->rx_real_len -= len;
            hw->rx_len -= len;
            hw->rx_buff += (len * bytes_per_word);

            jzspi_write_fifo(hw->iomem, config->bits_per_word, hw->tx_len, hw->tx_buff, hw->tx_real_len, len);
            hw->tx_real_len -= len;
            hw->tx_buff += (len * bytes_per_word);
            hw->tx_len -= len;

        } else {
            jzspi_set_bit(hw->iomem, SSISR, SSISR_TUNDR, 0);
            len = jzspi_get_bit(hw->iomem, SSISR, SSISR_RFIFO_NUM);
            jzspi_read_fifo(hw->iomem, config->bits_per_word, hw->rx_buff, hw->rx_real_len, len);
            hw->rx_real_len -= len;
            hw->rx_len -= len;
            hw->rx_buff += (len * bytes_per_word);

            //判断传输完成，设置状态，若传输完成则调用 回调函数
            if (hw->tx_len <= 0 && hw->rx_len <= 0) {
                jzspi_set_bit(hw->iomem, SSICR0, SSICR0_RIE, 0);

#ifdef TEST_JZ_SPI_INTERFACE
                if(test_spi_config[hw->id]) {
                    if(test_spi_config[hw->id]->finish_irq_callback)
                        test_spi_config[hw->id]->finish_irq_callback(test_spi_config[hw->id]);
                }
#endif
                hw->working_flag = 0;
                wake_up(&hw->idle_wait);
            }
        }

        jzspi_hal_clear_underrun(hw->iomem);

        //根据 total_len 大小设置 阈值
        if (hw->rx_len > MAX_FIFO_LEN) {
            jzspi_hal_set_rx_fifo_threshold(hw->iomem, n);
        } else {
            jzspi_hal_set_rx_fifo_threshold(hw->iomem, hw->rx_len);
        }
    }

    if (get_bit_field(&ssicr0, SSICR0_TIE) && get_bit_field(&ssisr,SSISR_TFHE)) {
        printk(KERN_DEBUG "THFH come in\n");
    }
    return IRQ_HANDLED;
}

static int jz_spi_setupxfer(struct spi_device *spi, struct spi_transfer *t)
{
    struct jz_spi *hw = spi_master_get_devdata(spi->master);

    unsigned char bpw;
    unsigned int hz;

    //更新传输信息
    bpw = spi->bits_per_word;
    hz  = spi->max_speed_hz;

    if (t) {
        if (t->bits_per_word)
            bpw = t->bits_per_word;
        if (t->speed_hz)
            hz = t->speed_hz;
    }

    BUG_ON(bpw < 2 || bpw > 32);

    hw->config.bits_per_word = bpw;

    hw->config.clk_rate = hz;

    if (spi->mode & SPI_LSB_FIRST) {
        hw->config.tx_endian = Endian_mode2;    //Least significant byte in a word first, least significant bit in a byte first.
        hw->config.rx_endian = Endian_mode2;
    } else {
        hw->config.tx_endian = Endian_mode0;    //Most significant byte in a word first, most significant bit in a byte first.
        hw->config.rx_endian = Endian_mode0;
    }


    hw->config.spi_pha = !!(spi->mode & SPI_CPHA);

    hw->config.spi_pol = !!(spi->mode & SPI_CPOL);

    hw->config.loop_mode = !!(spi->mode & SPI_LOOP);

    BUG_ON(spi->chip_select >= hw->pdata->num_chipselect);

    hw->config.cs = spi->chip_select;

    hw->config.cs_mode = HAND_PULL_CS;
    return 0;
}

static int jz_spi_setup(struct spi_device *spi)
{
    unsigned long flags;
    struct jz_spi *hw = spi_master_get_devdata(spi->master);

    clk_enable(hw->clk);

    if (spi->chip_select)
        jzspi_set_bit(hw->iomem, SSICR1, SSICR1_FRMHL1, spi->mode & SPI_CS_HIGH);
    else
        jzspi_set_bit(hw->iomem, SSICR1, SSICR1_FRMHL0, spi->mode & SPI_CS_HIGH);

    spin_lock_irqsave(&hw->bitbang.lock, flags);
    if (!hw->bitbang.busy) {
        hw->bitbang.chipselect(spi, BITBANG_CS_INACTIVE);
    }
    spin_unlock_irqrestore(&hw->bitbang.lock, flags);

    return 0;
}

static int jz_spi_txrx(struct spi_device * spi, struct spi_transfer *t)
{
    struct jz_spi * hw = spi_master_get_devdata(spi->master);

    spi_write_read_inr(hw, t->tx_buf, t->len, t->rx_buf, t->len);

    wait_event(hw->idle_wait, !hw->working_flag);

    return t->len;
}


static void jz_spi_chipsel(struct spi_device *spi, int value)
{
    struct jz_spi *hw = spi_master_get_devdata(spi->master);
#ifdef  CONFIG_JZ_SPI_PIO_CE
    unsigned int level = spi->mode & SPI_CS_HIGH ? 1 : 0;
#endif

    if (value) {
        spi_start_transfer(hw);
#ifdef  CONFIG_JZ_SPI_PIO_CE
        gpio_set_value(spi->chipselect[cs], level);
#endif

    } else {
        spi_stop_transfer(hw);
#ifdef  CONFIG_JZ_SPI_PIO_CE
        gpio_set_value(spi->chipselect[cs], !level);
#endif
    }
}

static int jz_spi_probe(struct platform_device *pdev)
{
    struct jz_spi * hw;
    struct spi_master * master;
    struct resource *res;
    int err;
    char tmp_name[20];
#ifdef CONFIG_JZ_SPI_PIO_CE
    int i;
#endif

    master = spi_alloc_master(&pdev->dev, sizeof(struct jz_spi));
    BUG_ON(master == NULL);

    hw = spi_master_get_devdata(master);

    hw->master = spi_master_get(master);
    hw->dev = &pdev->dev;

    res = platform_get_resource(pdev, IORESOURCE_MEM, 0);
    BUG_ON(res == NULL);

    hw->ioarea = request_mem_region(res->start, resource_size(res), pdev->name);
    BUG_ON(hw->ioarea == NULL);

    hw->iomem = ioremap_nocache(res->start, resource_size(res));
    BUG_ON(hw->iomem == NULL);

    hw->irq = platform_get_irq(pdev, 0);
    BUG_ON(hw->irq < 0);

    hw->pdata  = pdev->dev.platform_data;
    BUG_ON(hw->pdata == NULL);

    platform_set_drvdata(pdev, hw);

    init_waitqueue_head(&hw->idle_wait);

    master->mode_bits = (SPI_CPOL | SPI_CPHA | SPI_CS_HIGH | SPI_LSB_FIRST | SPI_LOOP);
    master->bus_num = hw->pdata->bus_num;
    master->num_chipselect = hw->pdata->num_chipselect;

    hw->bitbang.master          = hw->master;
    hw->bitbang.setup_transfer  = jz_spi_setupxfer;
    hw->bitbang.chipselect      = jz_spi_chipsel;
    hw->bitbang.txrx_bufs       = jz_spi_txrx;
    hw->bitbang.master->setup   = jz_spi_setup;

#ifdef CONFIG_JZ_SPI_PIO_CE
    for (i = 0; i < hw->pdata->num_chipselect; i++) {
        sprintf(tmp_name, "JZ_SPI%d_CS", i);
        err = gpio_request_one(hw->pdata->chipselect[i], GPIOF_OUT_INIT_LOW, tmp_name);
        BUG_ON(err);
    }
#endif
    hw->id = pdev->id;
    sprintf(tmp_name, "ssi%d", pdev->id);
    hw->clk = clk_get(hw->dev, tmp_name);
    hw->clk_cgu = clk_get(hw->dev, "cgu_spi");

    BUG_ON(IS_ERR(hw->clk) || IS_ERR(hw->clk_cgu));

    hw->config.src_clk_rate = clk_get_rate(hw->clk_cgu);

#ifdef CONFIG_JZ_SPI_SELECT_EXT_CLK
    hw->parent_clk = clk_get(hw->dev, "ext1");
#else
    hw->parent_clk = clk_get(hw->dev, "cgu_sfc");
#endif
    BUG_ON(IS_ERR(hw->parent_clk));
    clk_set_parent(hw->clk_cgu, hw->parent_clk);
    clk_enable(hw->clk_cgu);
    clk_enable(hw->clk);

    err = request_irq(hw->irq, spi_intr_handler, 0 ,pdev->name , hw);
    BUG_ON(err);

    jzspi_hal_init_setting(hw->iomem);

    err = spi_bitbang_start(&hw->bitbang);
    BUG_ON(err);

    clk_disable(hw->clk);

#ifdef TEST_JZ_SPI_INTERFACE
    test_spi_hw[pdev->id] = hw;
#endif

    return 0;
}

static int jz_spi_remove(struct platform_device *pdev)
{
    struct resource *res;
    struct jz_spi *hw = platform_get_drvdata(pdev);

    spi_bitbang_stop(&hw->bitbang);
    free_irq(hw->irq, hw);
    clk_disable(hw->clk_cgu);
    clk_put(hw->clk);
    clk_put(hw->clk_cgu);
    clk_put(hw->parent_clk);

#ifdef CONFIG_JZ_SPI_PIO_CE
    int i;
    for (i = 0; i < hw->pdata->num_chipselect; i++)
        gpio_free(hw->pdata->chipselect[i]);
#endif
    platform_set_drvdata(pdev, NULL);

    iounmap(hw->iomem);
    res = platform_get_resource(pdev, IORESOURCE_MEM, 0);
    if (res)
        release_mem_region(res->start, resource_size(res));

    spi_master_put(hw->master);

    return 0;
}

#ifdef CONFIG_PM
static int jz_spi_suspend(struct platform_device *pdev, pm_message_t state)
{
    // struct jz_spi *hw = platform_get_drvdata(pdev);

    // spi_master_suspend(hw->master);
    // wait_event(hw->idle_wait, !hw->working_flag);
    // clk_disable(hw->clk_cgu);

    return 0;
}

static int jz_spi_resume(struct platform_device *pdev)
{
    // struct jz_spi *hw = platform_get_drvdata(pdev);

    // clk_enable(hw->clk_cgu);
    // spi_master_resume(hw->master);

    return 0;
}

#else
#define jz_spi_suspend NULL
#define jz_spi_resume  NULL
#endif

static struct platform_driver jz_spidrv = {
    .driver = {
        .name   =   "jz-ssi",
        .owner  =   THIS_MODULE,
    },
    .remove = jz_spi_remove,
    .suspend = jz_spi_suspend,
    .resume = jz_spi_resume,
};

static int __init jz_spi_init(void)
{
    return platform_driver_probe(&jz_spidrv, jz_spi_probe);
}

static void __exit jz_spi_exit(void)
{
    platform_driver_unregister(&jz_spidrv);
}

subsys_initcall(jz_spi_init);
module_exit(jz_spi_exit);


#ifdef TEST_JZ_SPI_INTERFACE
static inline void wait_rx_fifo_full(void *base_addr, int n)
{
    jzspi_set_bit(base_addr, SSISR, SSISR_TUNDR, 0);
    while (jzspi_get_bit(base_addr, SSISR, SSISR_RFIFO_NUM) < n);
}

static void spi_write_read_poll(struct jzspi_config_data *config,
    void * tx_buf, int tlen, void * rx_buf, int rlen)
{
    int id = config->id;
    int bytes_per_word = to_bytes(config->bits_per_word);
    int n = MAX_FIFO_LEN * 3 / 4 ;
    int len = tlen > rlen ? tlen : rlen;

    BUG_ON((test_spi_hw[id] == NULL) || (id > 1));

    if((!len) || ((tx_buf == NULL) && (rx_buf == NULL)))
        return;

    wait_tx_fifo_empty(test_spi_hw[id]->iomem);
    clear_rx_fifo(test_spi_hw[id]->iomem);

    //第一次 写FIFO 写满
    jzspi_write_fifo(test_spi_hw[id]->iomem, config->bits_per_word, len, tx_buf, tlen, MAX_FIFO_LEN);
    tx_buf += MAX_FIFO_LEN * bytes_per_word;
    tlen -= MAX_FIFO_LEN;
    len -= MAX_FIFO_LEN;

    //循环读写FIFO 每次读写 FIFO 的 3 / 4
    while (len > 0) {

        wait_rx_fifo_full(test_spi_hw[id]->iomem, n);
        jzspi_read_fifo(test_spi_hw[id]->iomem, config->bits_per_word, rx_buf, rlen, n);
        rx_buf += n  * bytes_per_word;
        rlen -= n;

        jzspi_write_fifo(test_spi_hw[id]->iomem, config->bits_per_word, len, tx_buf, tlen, n);
        tx_buf += n * bytes_per_word;
        tlen -= n;
        len -= n;

    }

    //最后一次 把FIFO读空
    wait_rx_fifo_full(test_spi_hw[id]->iomem, rlen);
    jzspi_hal_read_rx_fifo(test_spi_hw[id]->iomem, config->bits_per_word, rx_buf, rlen);

}

void spi_start_config(struct jzspi_config_data *config)
{
    struct jzspi_hal_config *hal_config;
    int id = config->id;

    BUG_ON((test_spi_hw[id] == NULL) || (id > 1));

    test_spi_config[id] = config;
    hal_config = &test_spi_hw[id]->config;

    clk_enable(test_spi_hw[id]->clk);

    hal_config->cs = config->cs;
    hal_config->clk_rate = config->clk_rate;

    if(config->auto_cs)
        hal_config->cs_mode = AUTO_PULL_CS;
    else
        hal_config->cs_mode = HAND_PULL_CS;

    hal_config->tx_endian = config->tx_endian;
    hal_config->rx_endian = config->rx_endian;
    hal_config->bits_per_word = config->bits_per_word;
    hal_config->spi_pha = !!config->spi_pha;
    hal_config->spi_pol = !!config->spi_pol;
    hal_config->loop_mode = !!config->loop_mode;

    if (config->cs)
        jzspi_set_bit(test_spi_hw[id]->iomem, SSICR1, SSICR1_FRMHL1, config->cs_valid_level);
    else
        jzspi_set_bit(test_spi_hw[id]->iomem, SSICR1, SSICR1_FRMHL0, config->cs_valid_level);

    jzspi_hal_enable_transfer(test_spi_hw[id]->iomem, hal_config);
}

void spi_write_read(struct jzspi_config_data *config, void *tx_buf, int tx_len, void *rx_buf, int rx_len)
{
    int id = config->id;
    BUG_ON((test_spi_hw[id] == NULL) || (id > 1));

    if (config->transfer_mode == Transfer_irq_callback_mode)
        spi_write_read_inr(test_spi_hw[id], tx_buf, tx_len, rx_buf, rx_len);
    else if (config->transfer_mode == Transfer_poll_mode)
        spi_write_read_poll(config, tx_buf, tx_len, rx_buf, rx_len);
}

void spi_stop_config(struct jzspi_config_data *config)
{
    int id = config->id;
    BUG_ON((test_spi_hw[id] == NULL) || (id > 1));

    jzspi_hal_enable_fifo_empty_finish_transfer(test_spi_hw[id]->iomem);
    while (!jzspi_hal_get_end(test_spi_hw[id]->iomem));
    jzspi_hal_disable_transfer(test_spi_hw[id]->iomem);
    clk_disable(test_spi_hw[id]->clk);
    test_spi_hw[id]->working_flag = 0;
    test_spi_config[id] = NULL;
}
#endif


MODULE_AUTHOR("guangyue.luo <guangyue.luo@ingenic.com>");
MODULE_DESCRIPTION("JZ X1021 SPI Controller Driver");
MODULE_LICENSE("GPL");

#ifdef SPI_TEST
#include <linux/module.h>
#include <linux/init.h>
#include <linux/kernel.h>
#include <linux/spi/spi.h>
#include <linux/spi/spi_bitbang.h>
#include <linux/delay.h>
#include <linux/spinlock.h>
#include <jz_spi_v2.h>

void getStatus(struct jzspi_config_data *config)
{
    printk("callback OK!\n");
}

struct jzspi_config_data config = {
    .id = 0,
    .cs = 0,
    .auto_cs = 1,
    .clk_rate = 12 * 1000 * 1000,
    .cs_valid_level = Valid_low,
    .tx_endian = Endian_mode0,  //Most significant 「byte」 in a word first, most significant 「bit」 in a byte first.
    .rx_endian = Endian_mode0,
    .bits_per_word = 32,
    .spi_pha = 0,
    .spi_pol = 0,
    .loop_mode = 0,
    .transfer_mode = Transfer_irq_callback_mode,//Transfer_poll_mode,Transfer_irq_callback_mode
    .finish_irq_callback = getStatus,
};

u32 tx_buff[] = {0xAE0000 ,0x002533FF, 0x2533FF0D ,0X12347778};
u32 rx_buf[1024];
int test_probe(struct spi_device * spi)
{
    int ret;

    int len = sizeof(tx_buff)/sizeof(tx_buff[0]);
    struct spi_message  m;
    int i = 1000;
    while (i--) {
        struct spi_transfer	t = {
            .tx_buf = tx_buff,
            .rx_buf = rx_buf,
            .len    = len,
            .bits_per_word  = 32,
            .speed_hz   = 12 * 1000 * 1000,
            .cs_change = 1,
        };

        struct spi_transfer	t2 = {
            .tx_buf = tx_buff,
            .rx_buf = rx_buf,
            .len    = len,
            .bits_per_word  = 32,
            .speed_hz   = 6 * 1000 * 1000,
            .cs_change = 1,
        };

        struct spi_transfer	t3 = {
            .tx_buf = tx_buff,
            .rx_buf = rx_buf,
            .len    = len,
            .bits_per_word  = 32,
            .speed_hz   = 1 * 1000 * 1000,
            .cs_change = 0,
        };

        spi->mode = SPI_MODE_0 ;

        spi_message_init(&m);
        spi_message_add_tail(&t, &m);

        spi_message_add_tail(&t2, &m);

        spi_message_add_tail(&t3, &m);
        ret = spi_sync(spi, &m);
        mdelay(100);
        printk("1rx_buf=%08x\n\n",rx_buf[len-1]);
    //}
        // len = sizeof(tx_buff)/sizeof(tx_buff[0]);
        // spi_start_config(&config);
        // spi_write_read(&config, tx_buff, len, rx_buf, len);
        // spi_stop_config(&config);
        //     printk("2rx_buf=%08x\n",rx_buf[len-1]);
    }

    len = sizeof(tx_buff)/sizeof(tx_buff[0]);
    spi_start_config(&config);
    spi_write_read(&config, tx_buff, len, rx_buf, len);
    spi_stop_config(&config);
    printk("1rx_buf=%08x\n",rx_buf[len-1]);

    return 0;
}

static struct spi_driver test_driver = {
    .driver = {
        .name   = "spi_test",
        .owner  = THIS_MODULE,
    },
    .probe  = test_probe,
    .remove = NULL,
};
static int __init test_spi_init(void)
{
    return spi_register_driver(&test_driver);
}
static void __exit test_spi_exit(void)
{
    spi_unregister_driver(&test_driver);
}
late_initcall(test_spi_init);
module_exit(test_spi_exit);
#endif