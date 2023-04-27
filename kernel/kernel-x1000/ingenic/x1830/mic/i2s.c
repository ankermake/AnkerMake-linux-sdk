/**
 * xb_snd_i2s.c
 *
 * jbbi <jbbi@ingenic.cn>
 *
 * 24 APR 2012
 *
 */

#include <linux/init.h>
#include <linux/delay.h>
#include <linux/workqueue.h>
#include <linux/sched.h>
#include <linux/interrupt.h>
#include <linux/platform_device.h>
#include <linux/proc_fs.h>
#include <linux/clk.h>
#include <linux/vmalloc.h>
#include <linux/string.h>
#include <linux/sound.h>
#include <linux/slab.h>
#include <sound/core.h>
#include <linux/switch.h>
#include <linux/dma-mapping.h>
#include <linux/soundcard.h>
#include <linux/wait.h>
#include <mach/jzdma.h>
#include <mach/jzsnd.h>
#include <soc/irq.h>
#include <soc/base.h>
#include "i2s.h"
#include "x1800_codec.h"
#include "mic.h"
#include <linux/mutex.h>

static struct i2s_device * g_i2s_dev;
static struct codec_info * g_codec_dev = NULL;

static DEFINE_MUTEX(m_lock);

/*##################################################################*\
 | dump
\*##################################################################*/

static void dump_i2s_reg(struct i2s_device *i2s_dev)
{
    int i;
    unsigned long reg_addr[] = {
        AICFR,AICCR,I2SCR,AICSR,I2SSR,I2SDIV
    };

    for (i = 0;i < 6; i++) {
        printk("##### aic reg0x%x,=0x%x.\n",
            (unsigned int)reg_addr[i],i2s_read_reg(i2s_dev, reg_addr[i]));
    }

	pr_info("AIC_FR\t\t%p : 0x%08x\n", (0xb0020000+AICFR),  i2s_read_reg(i2s_dev, AICFR));
	pr_info("AIC_CR\t\t%p : 0x%08x\n", (0xb0020000+AICCR),  i2s_read_reg(i2s_dev, AICCR));
	pr_info("AIC_I2SCR\t%p : 0x%08x\n",(0xb0020000+I2SCR),  i2s_read_reg(i2s_dev, I2SCR));
	pr_info("AIC_SR\t\t%p : 0x%08x\n", (0xb0020000+AICSR),  i2s_read_reg(i2s_dev, AICSR));
	pr_info("AIC_I2SSR\t%p : 0x%08x\n",(0xb0020000+I2SSR),  i2s_read_reg(i2s_dev, I2SSR));
	pr_info("AIC_I2SDIV\t%p : 0x%08x\n",(0xb0020000+I2SDIV),i2s_read_reg(i2s_dev, I2SDIV));
	pr_info("AIC_DR\t%p : 0x%08x\n",(0xb0020000+AICDR),     i2s_read_reg(i2s_dev, AICDR));
	pr_info("AIC_I2SCDR\t 0x%08x\n",*(volatile unsigned int*)0xb0000060);
}

void m_dump_i2s_reg(void)
{
    dump_i2s_reg(g_i2s_dev);
}

/*##################################################################*\
  |* codec control
\*##################################################################*/
/**
 * register the codec
 **/
static int codec_ctrl(struct codec_info *cur_codec, unsigned int cmd, unsigned long arg)
{
    if(cur_codec->codec_ctl_2) {
        return cur_codec->codec_ctl_2(cur_codec, cmd, arg);
    } else {
        return cur_codec->codec_ctl(cmd, arg);
    }
}

int i2s_register_codec(char *name, void *codec_ctl,unsigned long codec_clk,enum codec_mode mode)
{   /*to be modify*/
    struct codec_info *tmp = vmalloc(sizeof(struct codec_info));
    if ((name != NULL) && (codec_ctl != NULL)) {
        //tmp->name = name;
        memcpy(tmp->name, name, sizeof(tmp->name));
        tmp->codec_ctl = codec_ctl;
        tmp->codec_clk = codec_clk;
        tmp->codec_ctl_2 = NULL;
        tmp->codec_mode = mode;

        printk("i2s register ,codec is :%p\n", tmp);
        return 0;
    } else {
        return -1;
    }
}

int i2s_register_codec_2(struct codec_info * codec_dev)
{
    g_codec_dev = codec_dev;

    return 0;
}

int i2s_release_codec_2(struct codec_info * codec_dev)
{
    if(!codec_dev) {
        printk("codec_dev register failed\n");
        return -EINVAL;
    }
    list_del(&codec_dev->list);
    return 0;
}


/*##################################################################*\
|* dev_ioctl
\*##################################################################*/
static int i2s_set_fmt(struct i2s_device *i2s_dev, unsigned long *format)
{
    int ret = 0;
    int data_width = 0;
    struct codec_info * cur_codec = i2s_dev->cur_codec;
    /*
     * The value of format reference to soundcard.
     * AFMT_MU_LAW      0x00000001
     * AFMT_A_LAW       0x00000002
     * AFMT_IMA_ADPCM   0x00000004
     * AFMT_U8          0x00000008
     * AFMT_S16_LE      0x00000010
     * AFMT_S16_BE      0x00000020
     * AFMT_S8          0x00000040
     */
    printk("format = %d",*format);

    __i2s_set_iss_sample_size(i2s_dev, 1);
    __i2s_disable_signadj(i2s_dev);

//    ret = codec_ctrl(cur_codec, CODEC_SET_RECORD_DATA_WIDTH, *format);
//    if(ret != 0) {
//        printk("JZ I2S: CODEC ioctl error, command: CODEC_SET_RECORD_FORMAT");
//        return ret;
//    }
//
//    if (cur_codec->record_format != *format) {
//        cur_codec->record_format = *format;
//        ret |= NEED_RECONF_TRIGGER | NEED_RECONF_FILTER;
//        ret |= NEED_RECONF_DMA;
//    }

    return ret;
}

static int i2s_set_channel(struct i2s_device * i2s_dev, int* channel)
{
    int ret = 0;

    struct codec_info * cur_codec = i2s_dev->cur_codec;
    if (!cur_codec)
        return -ENODEV;
    printk("channel = %d",*channel);


    ret = codec_ctrl(cur_codec, CODEC_SET_RECORD_CHANNEL,(unsigned long)channel);
    if (ret < 0)
        return ret;
    ret = 0;

    if (cur_codec->record_codec_channel != *channel) {
        cur_codec->record_codec_channel = *channel;
        ret |= NEED_RECONF_FILTER;
    }

    return ret;
}

static int i2s_set_rate(struct i2s_device * i2s_dev, unsigned long *rate)
{
    int ret = 0;
    unsigned long cgu_aic_clk = 0;
    struct codec_info *cur_codec = i2s_dev->cur_codec;
    static unsigned long rate_save = 0;

    if (rate_save == *rate)
        return 0;

    rate_save = *rate;

    if (!cur_codec)
        return -ENODEV;

    printk("rate = %ld\n",*rate);
    clk_set_rate(i2s_dev->i2s_clk, (*rate) * 256);
//    printk(KERN_DEBUG"%s, i2s clk rate is %ld\n", __func__, clk_get_rate(i2s_dev->i2s_clk));
//
//    ret = codec_ctrl(cur_codec, CODEC_SET_RECORD_RATE,(unsigned long)rate);
//    cur_codec->record_rate = *rate;

    return ret;
}
#define I2S_TX_FIFO_DEPTH       64
#define I2S_RX_FIFO_DEPTH       32

void i2s_replay_zero_for_flush_codec(struct i2s_device *i2s_dev)
{
    mutex_lock(&m_lock);
    __i2s_write_tfifo(i2s_dev, 0);  //avoid pop
    __i2s_write_tfifo(i2s_dev, 0);
    __i2s_write_tfifo(i2s_dev, 0);
    __i2s_write_tfifo(i2s_dev, 0);
    __i2s_enable_replay(i2s_dev);
    msleep(2);
    __i2s_disable_replay(i2s_dev);
    mutex_unlock(&m_lock);
}


static int i2s_disable_channel(struct i2s_device *i2s_dev)
{
    struct codec_info * cur_codec = i2s_dev->cur_codec;
    if (cur_codec) {
            codec_ctrl(cur_codec, CODEC_TURN_OFF,CODEC_RMODE);
    }

    __i2s_disable_record(i2s_dev);

    return 0;
}

void amic_dma_reset(void) {
    int val;
    struct i2s_device * i2s_dev = g_i2s_dev;
    struct codec_info * cur_codec = i2s_dev->cur_codec;
    mutex_lock(&m_lock);

    __i2s_disable_record(i2s_dev);
    __i2s_disable_receive_dma(i2s_dev);

    __i2s_flush_rfifo(i2s_dev);

    /* read the first sample and ignore it */
    val = __i2s_read_rfifo(i2s_dev);
    __i2s_enable_receive_dma(i2s_dev);
    __i2s_enable_record(i2s_dev);
    mutex_unlock(&m_lock);
}

enum {
    I2S_PLAYBACK_CLK    =    (1 << 0),
    I2S_CAPTURE_CLK     =    (1 << 1),
    I2S_SET_CLK         =    (1 << 2),
};

static void i2s_clk_set_enable(int enable, int flag) {
    static int enable_flag = 0;
    struct i2s_device * i2s_dev = g_i2s_dev;

    if (enable) {
        if (!enable_flag) {
            clk_enable(i2s_dev->i2s_clk);
            clk_enable(i2s_dev->aic_clk);
            msleep(1);
            __i2s_enable(i2s_dev);
            printk("i2s enable\n");
        }
        enable_flag |= flag;
    } else {
        if (enable_flag == flag){
            __i2s_disable(i2s_dev);
            clk_disable(i2s_dev->aic_clk);
            clk_disable(i2s_dev->i2s_clk);
            printk("i2s disable\n");
        }
        enable_flag &= (~flag);
    }
}

void i2s_set_playback_volume(int vol) {
    mutex_lock(&m_lock);
    i2s_clk_set_enable(1, I2S_SET_CLK);
    codec_set_play_volume(&vol);
    i2s_clk_set_enable(0, I2S_SET_CLK);
    mutex_unlock(&m_lock);
}

int i2s_get_playback_volume(void) {
    mutex_lock(&m_lock);
    i2s_clk_set_enable(1, I2S_SET_CLK);
    int vol = codec_get_play_volume();
    i2s_clk_set_enable(0, I2S_SET_CLK);
    mutex_unlock(&m_lock);

    return vol;
}

void i2s_enable_playback_dma(int rate) {
    /* int i; */
    int timeout = 150000;
    struct i2s_device * i2s_dev = g_i2s_dev;

    mutex_lock(&m_lock);

    i2s_set_rate(i2s_dev, &rate);
    i2s_clk_set_enable(1, I2S_PLAYBACK_CLK);

    /* for (i= 0; i < 16 ; i++) { */
    /*     __i2s_write_tfifo(i2s_dev, 0x0); */
    /*     __i2s_write_tfifo(i2s_dev, 0x0); */
    /* } */
    __i2s_clear_tur(i2s_dev);
    __i2s_enable_replay(i2s_dev);
    /* while((!__i2s_test_tur(i2s_dev)) && (timeout--)){ */
    /*     if(timeout == 0){ */
    /*         printk("wait tansmit fifo under run error\n"); */
    /*         return; */
    /*     } */
    /* } */

    __i2s_enable_transmit_dma(i2s_dev);
    mutex_unlock(&m_lock);
}

void i2s_disable_playback_dma(void) {
    struct i2s_device * i2s_dev = g_i2s_dev;
    mutex_lock(&m_lock);

    __i2s_disable_transmit_dma(i2s_dev);
    i2s_clk_set_enable(0, I2S_PLAYBACK_CLK);

    mutex_unlock(&m_lock);
}

void i2s_enable_capture_dma(void (*callback)(void *data), void *data) {
    struct i2s_device * i2s_dev = g_i2s_dev;
    struct codec_info * cur_codec = i2s_dev->cur_codec;

    mutex_lock(&m_lock);
    i2s_clk_set_enable(1, I2S_CAPTURE_CLK);

    __i2s_flush_rfifo(i2s_dev);
    mdelay(1);

    local_irq_disable();
    __i2s_enable_receive_dma(i2s_dev);
    __i2s_enable_record(i2s_dev);
    if (callback)
        callback(data);
    local_irq_enable();
    mutex_unlock(&m_lock);
}

void i2s_disable_capture_dma(void) {
    struct i2s_device * i2s_dev = g_i2s_dev;
    struct codec_info * cur_codec = i2s_dev->cur_codec;
    mutex_lock(&m_lock);

    __i2s_disable_record(i2s_dev);
    __i2s_disable_receive_dma(i2s_dev);

    i2s_clk_set_enable(0, I2S_CAPTURE_CLK);
    mutex_unlock(&m_lock);
}

static void i2s_playback_init(struct i2s_device * i2s_dev, struct dma_param *dma) {
    int fmt_width = 16;
    int channels = 1;
    int buswidth;
    struct codec_info * cur_codec = i2s_dev->cur_codec;

    switch (fmt_width) {
    case 8:
        buswidth = DMA_SLAVE_BUSWIDTH_1_BYTE;
        __i2s_set_oss_sample_size(i2s_dev, 0);
        break;
    case 16:
        buswidth = DMA_SLAVE_BUSWIDTH_2_BYTES;
        __i2s_set_oss_sample_size(i2s_dev, 1);
        break;
    case 24:
        buswidth = DMA_SLAVE_BUSWIDTH_4_BYTES;
        __i2s_set_oss_sample_size(i2s_dev, 4);
        break;
    }

    dma->tx_buswidth = buswidth;
    dma->tx_maxburst = (I2S_TFIFO_DEPTH * buswidth)/2;

    int trigger = I2S_TFIFO_DEPTH - (dma->tx_maxburst/(int)buswidth);

    __i2s_set_transmit_trigger(i2s_dev, trigger / 2);
    __i2s_out_channel_select(i2s_dev, channels - 1);
    __i2s_disable_mono2stereo(i2s_dev);
}

static void i2s_record_init(struct i2s_device * i2s_dev, struct dma_param *dma) {
    unsigned long record_rate = DEFAULT_RECORD_SAMPLERATE;
    int record_channel = DEFAULT_RECORD_CHANNEL;
    int fmt_width = 16;
    int buswidth;

    if (fmt_width == 8)
        buswidth = DMA_SLAVE_BUSWIDTH_1_BYTE;
    else if (fmt_width == 16)
        buswidth = DMA_SLAVE_BUSWIDTH_2_BYTES;
    else
        buswidth = DMA_SLAVE_BUSWIDTH_4_BYTES;

    dma->rx_buswidth = buswidth;
    dma->rx_maxburst = (I2S_RFIFO_DEPTH * buswidth)/2;

    int trigger = dma->tx_maxburst/(int)buswidth;

    __i2s_set_receive_trigger(i2s_dev, 7);

    i2s_set_fmt(i2s_dev, &fmt_width);
    i2s_set_channel(i2s_dev, &record_channel);
    i2s_set_rate(i2s_dev, &record_rate);
}

static int i2s_global_init(struct platform_device *pdev, struct mic *mic)
{
    int ret = 0;
    struct i2s_device * i2s_dev;

    i2s_dev = (struct i2s_device *)kzalloc(sizeof(struct i2s_device), GFP_KERNEL);
    if(!i2s_dev) {
        dev_err(&pdev->dev, "failed to alloc i2s dev\n");
        return -ENOMEM;
    }
    memset(i2s_dev, 0, sizeof(*i2s_dev));

    i2s_dev->res = mic->dma->io_res;
    i2s_dev->i2s_iomem = mic->dma->iomem;

    i2s_dev->i2s_clk = clk_get(&pdev->dev, "cgu_i2s");
    if(IS_ERR(i2s_dev->i2s_clk)) {
        dev_err(&pdev->dev, "cgu i2s clk get failed!\n");
        goto err_get_i2s_clk;
    }
    i2s_dev->aic_clk = clk_get(&pdev->dev, "aic");
    if(IS_ERR(i2s_dev->aic_clk)) {
        dev_err(&pdev->dev, "aic clk get failed!\n");
        goto err_get_aic_clk;
    }

    i2s_dev->cur_codec = g_codec_dev;
    g_i2s_dev = i2s_dev;

    clk_enable(i2s_dev->aic_clk);

    i2s_set_reg(i2s_dev, AICFR,1,(1 << 31),31);
    msleep(1);
    i2s_set_reg(i2s_dev, AICFR,0,(1 << 31),31);

    __i2s_reset(i2s_dev);

    __i2s_disable(i2s_dev);
    schedule_timeout(5);
    __i2s_disable(i2s_dev);
    __i2s_stop_bitclk(i2s_dev);
    __i2s_stop_ibitclk(i2s_dev);
    /*select i2s trans*/
    __aic_select_i2s(i2s_dev);
    __i2s_select_i2s(i2s_dev);

    __i2s_internal_codec_master(i2s_dev);

    __i2s_slave_clkset(i2s_dev);
    /*sysclk output*/
    __i2s_enable_sysclk_output(i2s_dev);

    clk_set_rate(i2s_dev->i2s_clk, 8000*256);
    clk_enable(i2s_dev->i2s_clk);

    __i2s_start_bitclk(i2s_dev);
    __i2s_start_ibitclk(i2s_dev);

    __i2s_disable_receive_dma(i2s_dev);
    __i2s_disable_transmit_dma(i2s_dev);
    __i2s_disable_record(i2s_dev);
    __i2s_disable_replay(i2s_dev);
    __i2s_disable_loopback(i2s_dev);
    __i2s_clear_ror(i2s_dev);
    __i2s_clear_tur(i2s_dev);

    __i2s_disable_overrun_intr(i2s_dev);
    __i2s_disable_underrun_intr(i2s_dev);
    __i2s_disable_transmit_intr(i2s_dev);
    __i2s_disable_receive_intr(i2s_dev);
    __i2s_send_rfirst(i2s_dev);
    __i2s_disable_pack16(i2s_dev);

    codec_ctrl(i2s_dev->cur_codec, CODEC_INIT,0);

    i2s_playback_init(i2s_dev, mic->dma);
    i2s_record_init(i2s_dev, mic->dma);

    /* play zero or last sample when underflow */
    __i2s_play_lastsample(i2s_dev);
    __i2s_enable(i2s_dev);

    i2s_disable_playback_dma();
    i2s_disable_capture_dma();

//    dump_i2s_reg(i2s_dev);
//    codec_ctrl(i2s_dev->cur_codec, CODEC_DUMP_REG,0);

    return 0;

err_get_aic_clk:
    clk_put(i2s_dev->i2s_clk);
err_get_i2s_clk:
    iounmap(i2s_dev->i2s_iomem);
    kfree(i2s_dev);
    return ret;
}

static int i2s_global_exit(struct platform_device *pdev)
{
    struct i2s_device * i2s_dev = g_i2s_dev;

    if(!i2s_dev)
        return 0;

    __i2s_disable(i2s_dev);

    clk_disable(i2s_dev->i2s_clk);

    clk_disable(i2s_dev->aic_clk);

    free_irq(i2s_dev->i2s_irq, i2s_dev);

    clk_put(i2s_dev->aic_clk);
    clk_put(i2s_dev->i2s_clk);

    iounmap(i2s_dev->i2s_iomem);
    release_mem_region(i2s_dev->res->start, resource_size(i2s_dev->res));

    kfree(i2s_dev->cur_codec);
    kfree(i2s_dev);
    g_i2s_dev = NULL;

    return 0;
}

int i2s_init(struct platform_device *pdev, struct mic *mic)
{
    int ret;

    jz_codec_init();

    ret = i2s_global_init(pdev, mic);

    return ret;
}

static int i2s_exit(struct platform_device *pdev)
{
    int ret = 0;
    struct snd_dev_data *tmp;

    i2s_global_exit(pdev);
    jz_codec_exit();

    return ret;
}
