

#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/spinlock.h>
#include <linux/mutex.h>
#include <linux/miscdevice.h>
#include <linux/clk.h>
#include <bit_field.h>
#include <assert.h>
#include <linux/delay.h>
#include <linux/slab.h>
#include <linux/io.h>
#include <linux/dma-mapping.h>
#include <linux/dmaengine.h>
#include <linux/hrtimer.h>
#include <linux/err.h>

#include <sound/pcm.h>
#include <sound/pcm_params.h>
#include <sound/initval.h>
#include <sound/soc.h>
#include <sound/soc-dai.h>

#include "../audio_dma/audio_regs.h"
#include "../audio_dma/audio.h"

#include "aic.h"

#include "aic_regs.h"

#include "aic_gpio.c"

#define AIC_NUMS 5

static const char *ce_names[4] = {
    "ce_i2s0","ce_i2s1","ce_i2s2","ce_i2s3",
};

static const char *gate_names[AIC_NUMS] = {
    "gate_i2s0", "gate_i2s1", "gate_i2s2", "gate_i2s3", "gate_pcm"
};

static const char *rx_names[AIC_NUMS] = {
    "div_i2s0", "div_i2s0", "div_i2s2", NULL, "mux_pcm"
};

static const char *tx_names[AIC_NUMS] = {
    "div_i2s1", "div_i2s1", NULL, "div_i2s3", "mux_pcm"
};

static int split_clk[AIC_NUMS] = {
    1, 1, 1, 1, 0
};

static const unsigned long iobase[] = {
    KSEG1ADDR(AUDIO_AIC0_BASE),
    KSEG1ADDR(AUDIO_AIC1_BASE),
    KSEG1ADDR(AUDIO_AIC2_BASE),
    KSEG1ADDR(AUDIO_AIC3_BASE),
    KSEG1ADDR(AUDIO_AIC4_BASE),
};

#define AIC_ADDR(id, reg) ((volatile unsigned long *)((iobase[id]) + (reg)))

static inline void aic_write_reg(int id, unsigned int reg, unsigned int value)
{
    *AIC_ADDR(id, reg) = value;
}

static inline unsigned int aic_read_reg(int id, unsigned int reg)
{
    return *AIC_ADDR(id, reg);
}

static inline void aic_set_bit(int id, unsigned int reg, int start, int end, unsigned int val)
{
    set_bit_field(AIC_ADDR(id, reg), start, end, val);
}

static inline unsigned int aic_get_bit(int id, unsigned int reg, int start, int end)
{
    return get_bit_field(AIC_ADDR(id, reg), start, end);
}

struct aic_params {
    unsigned char is_split_clk;
    unsigned char is_capture;
    unsigned char is_master;
    unsigned char inv_sync;
    unsigned char inv_bclk;
    unsigned char mode;
    unsigned char data_bits;
    unsigned char channels;
    unsigned int sys_freq;
    unsigned int sample_rate;
};

struct aic_data {
    struct snd_soc_dai_driver dai_driver;

    spinlock_t spinlock;
    struct mutex lock;

    struct clk *gate_clk;
    struct clk *rx_clk;
    struct clk *tx_clk;

    struct aic_params playback;

    struct aic_params capture;
};

struct aic_data aic_datas[AIC_NUMS];

static inline int to_ss(int data_bits)
{
    if (data_bits == 8) return 0;
    if (data_bits == 12) return 1;
    if (data_bits == 13) return 2;
    if (data_bits == 16) return 3;
    if (data_bits == 18) return 4;
    if (data_bits == 20) return 5;
    if (data_bits == 24) return 6;
    if (data_bits == 32) return 7;
    return 3;
}

static inline void aic_dump_regs(int id)
{
    printk(KERN_EMERG "BAICRCFG: %x\n", aic_read_reg(id, BAICRCFG));
    printk(KERN_EMERG "BAICTCFG: %x\n", aic_read_reg(id, BAICTCFG));
    printk(KERN_EMERG "BAICRDIV: %x\n", aic_read_reg(id, BAICRDIV));
    printk(KERN_EMERG "BAICTDIV: %x\n", aic_read_reg(id, BAICTDIV));
    printk(KERN_EMERG "BAICCCR: %x\n", aic_read_reg(id, BAICCCR));
}

static void aic_init_setting(int id, struct aic_params *param)
{
    int cfg_reg = param->is_split_clk && param->is_capture ? BAICRCFG : BAICTCFG;
    int div_reg = param->is_split_clk && param->is_capture ? BAICRDIV : BAICTDIV;

    aic_write_reg(id, BAICTLCR, param->is_split_clk);

    unsigned long cfg_value = 0;
    set_bit_field(&cfg_value, R_SWLR, 0);
    set_bit_field(&cfg_value, R_CHANNEL, param->channels / 2);
    set_bit_field(&cfg_value, R_ISYNC, param->inv_sync);
    set_bit_field(&cfg_value, R_NEG, param->inv_bclk);
    set_bit_field(&cfg_value, R_ASVTSU, 0);
    set_bit_field(&cfg_value, ISS, to_ss(param->data_bits));
    set_bit_field(&cfg_value, R_MODE, param->mode); // i2s mode
    set_bit_field(&cfg_value, R_MASTER, param->is_master); // master mode
    aic_write_reg(id, cfg_reg, cfg_value);

    unsigned long div_value = 0;
    int bclk = param->sample_rate * 64;
    int bclk_div = ((param->sys_freq + bclk - 1) / bclk) & ~0x01ul;
    set_bit_field(&div_value, R_BCLKDIV, bclk_div);
    set_bit_field(&div_value, R_SYNC_DIV, 3);
    aic_write_reg(id, div_reg, div_value);
}

static void aic_start_playback(int id)
{
    aic_set_bit(id, BAICTCFG, T_MASTER, aic_datas[id].playback.is_master);
    if (aic_datas[id].playback.is_split_clk == 1 || aic_get_bit(id, BAICCCR, REN) == 0)
        aic_set_bit(id, BAICTCTL, T_RST, 1);

    aic_set_bit(id, BAICCCR, TEN, 1);
}

static void aic_stop_playback(int id)
{
    aic_set_bit(id, BAICCCR, TEN, 0);
    if (aic_datas[id].playback.is_split_clk == 1 || aic_get_bit(id, BAICCCR, REN) == 0)
        aic_set_bit(id, BAICTCFG, T_MASTER, 0);
}

static void aic_start_capture(int id)
{
    unsigned int cfgreg = BAICRCFG;
    if (aic_datas[id].capture.is_split_clk == 0) {
        cfgreg = BAICTCFG;
        if (aic_get_bit(id, BAICCCR, TEN) == 0)
            aic_set_bit(id, BAICTCTL, T_RST, 1);
    }

    aic_set_bit(id, cfgreg, T_MASTER, aic_datas[id].capture.is_master);
    aic_set_bit(id, BAICRCTL, R_RST, 1);
    aic_set_bit(id, BAICCCR, REN, 1);
}

static void aic_stop_capture(int id)
{
    aic_set_bit(id, BAICCCR, REN, 0);
    if (aic_datas[id].capture.is_split_clk == 0) {
        if (aic_get_bit(id, BAICCCR, TEN) == 0)
            aic_set_bit(id, BAICTCFG, T_MASTER, 0);
        else
            aic_set_bit(id, BAICRCFG, R_MASTER, 0);
    }
}

#define ASOC_AIC_FORMATS \
     (SNDRV_PCM_FMTBIT_S8 | SNDRV_PCM_FMTBIT_U8) \
    | (SNDRV_PCM_FMTBIT_S16_LE | SNDRV_PCM_FMTBIT_U16_LE) \
    | (SNDRV_PCM_FMTBIT_S24_LE | SNDRV_PCM_FMTBIT_U24_LE) \
    | (SNDRV_PCM_FMTBIT_S32_LE | SNDRV_PCM_FMTBIT_U32_LE) \
    | (SNDRV_PCM_FMTBIT_S24_3LE | SNDRV_PCM_FMTBIT_U24_3BE)\
    | (SNDRV_PCM_FMTBIT_U24_3LE | SNDRV_PCM_FMTBIT_U24_3BE)\
    | (SNDRV_PCM_FMTBIT_S20_3LE | SNDRV_PCM_FMTBIT_S20_3BE)\
    | (SNDRV_PCM_FMTBIT_U20_3LE | SNDRV_PCM_FMTBIT_U20_3BE)\
    | (SNDRV_PCM_FMTBIT_S18_3LE | SNDRV_PCM_FMTBIT_S18_3BE)\
    | (SNDRV_PCM_FMTBIT_U18_3LE | SNDRV_PCM_FMTBIT_U18_3BE)

#define ASOC_AIC_RATE SNDRV_PCM_RATE_8000_768000

static int aic_trigger(struct snd_pcm_substream *substream, int cmd,
        struct snd_soc_dai *dai)
{
    int id = dai->id;
    int is_capture = substream->stream == SNDRV_PCM_STREAM_CAPTURE;

    switch (cmd) {
    case SNDRV_PCM_TRIGGER_START:
    case SNDRV_PCM_TRIGGER_RESUME:
    case SNDRV_PCM_TRIGGER_PAUSE_RELEASE:
        if (is_capture)
            aic_start_capture(id);
        else
            aic_start_playback(id);

        break;
    case SNDRV_PCM_TRIGGER_STOP:
    case SNDRV_PCM_TRIGGER_SUSPEND:
    case SNDRV_PCM_TRIGGER_PAUSE_PUSH:
        if (is_capture)
            aic_stop_capture(id);
        else
            aic_stop_playback(id);

        break;
    default:
        return -EINVAL;
    }

    return 0;
}

static int check_channels(int id, int channels)
{
    if (channels == 1 || channels == 2)
        return 0;

    if (channels > 8)
        return -EINVAL;

    if ((id != 2 && id != 3))
        return -EINVAL;

    return 0;
}

static int aic_hw_params(struct snd_pcm_substream *substream,
        struct snd_pcm_hw_params *hw_params, struct snd_soc_dai *dai)
{
    int id = dai->id;
    struct aic_data *aic = &aic_datas[id];
    int is_capture = substream->stream == SNDRV_PCM_STREAM_CAPTURE;
    struct aic_params *param = is_capture ? &aic->capture : &aic->playback;
    int sample_rate = params_rate(hw_params);
    int format = params_format(hw_params);
    int channels = params_channels(hw_params);

    if (check_channels(id, channels)) {
        printk(KERN_ERR "aic: aic%d channels not valid: %d\n", id, channels);
        return -EINVAL;
    }

    param->is_capture = is_capture;
    param->is_split_clk = split_clk[id];
    param->sample_rate = sample_rate;
    param->channels = channels;
    param->data_bits = snd_pcm_format_width(format);

    aic_init_setting(id, param);

    if (is_capture)
        return aic_rx_gpio_request(id, channels);
    else
        return aic_tx_gpio_request(id, channels);
}

static int aic_hw_free(struct snd_pcm_substream *substream, struct snd_soc_dai *dai)
{
    return 0;
}

static int aic_startup(struct snd_pcm_substream *substream, struct snd_soc_dai *dai)
{
    int id = dai->id;
    struct aic_data *aic = &aic_datas[id];
    int is_capture = substream->stream == SNDRV_PCM_STREAM_CAPTURE;

    if (is_capture) {
        if (!aic->rx_clk) {
            printk(KERN_ERR "aic: aic%d no capture\n", id);
            return -ENODEV;
        }

        if (!aic->capture.sys_freq) {
            aic->capture.sys_freq = 24000000;
            clk_set_rate(aic->rx_clk, 24000000);
        }

        clk_prepare_enable(aic->rx_clk);
    } else {
        if (!aic->tx_clk) {
            printk(KERN_ERR "aic: aic%d no playback\n", id);
            return -ENODEV;
        }

        if (aic->playback.sys_freq) {
            aic->playback.sys_freq = 24000000;
            clk_set_rate(aic->tx_clk, 24000000);
        }

        clk_prepare_enable(aic->tx_clk);
    }

    clk_prepare_enable(aic->gate_clk);

    return 0;
}

static void aic_shutdown(struct snd_pcm_substream *substream, struct snd_soc_dai *dai)
{
    int id = dai->id;
    struct aic_data *aic = &aic_datas[id];
    int is_capture = substream->stream == SNDRV_PCM_STREAM_CAPTURE;

    if (is_capture)
        clk_disable_unprepare(aic->rx_clk);
    else
        clk_disable_unprepare(aic->tx_clk);
}

static int aic_set_dai_fmt(struct snd_soc_dai *dai, unsigned int fmt)
{
    int id = dai->id;
    struct aic_data *aic = &aic_datas[id];
    int inv_sync = 0, inv_bclk = 0;
    int mode;
    int is_master = 0;

    switch (fmt & SND_SOC_DAIFMT_INV_MASK) {
    case SND_SOC_DAIFMT_NB_IF:
        inv_sync = 1;
        break;
    case SND_SOC_DAIFMT_IB_NF:
        inv_bclk = 1;
        break;
    case SND_SOC_DAIFMT_IB_IF:
        inv_sync = 1;
        inv_bclk = 1;
        break;
    case SND_SOC_DAIFMT_NB_NF:
    default:
        break;
    }

    switch (fmt & SND_SOC_DAIFMT_FORMAT_MASK) {
    case SND_SOC_DAIFMT_I2S:
        mode = AIC_I2S;
        break;
    case SND_SOC_DAIFMT_LEFT_J:
        mode = AIC_LEFT_J;
        break;
    case SND_SOC_DAIFMT_RIGHT_J:
        mode = AIC_RIGHT_J;
        break;
    default:
        printk(KERN_ERR "aic: not support this format: %d\n",
                         fmt & SND_SOC_DAIFMT_FORMAT_MASK);
        return -EINVAL;
    }

    switch (fmt & SND_SOC_DAIFMT_MASTER_MASK) {
    case SND_SOC_DAIFMT_CBM_CFM:
        break;
    case SND_SOC_DAIFMT_CBS_CFS:
        is_master = 1;
        break;
    default:
        printk(KERN_ERR "aic: not support this master mpde: %d\n",
                             fmt & SND_SOC_DAIFMT_MASTER_MASK);
        return -EINVAL;
    }

    if (!(fmt & (AIC_FMT_RX | AIC_FMT_TX)))
        fmt |= AIC_FMT_RX | AIC_FMT_TX;

    if (fmt & AIC_FMT_RX) {
        aic->capture.inv_sync = inv_sync;
        aic->capture.inv_bclk = inv_bclk;
        aic->capture.mode = mode;
        aic->capture.is_master = is_master;
    }

    if (fmt & AIC_FMT_TX) {
        aic->playback.inv_sync = inv_sync;
        aic->playback.inv_bclk = inv_bclk;
        aic->playback.mode = mode;
        aic->playback.is_master = is_master;
    }

    return 0;
}

static int aic_set_sysclk(struct snd_soc_dai *dai, int clk_id,
        unsigned int freq, int dir)
{
    int id = dai->id;
    struct aic_data *aic = &aic_datas[id];

    if (clk_id == AIC_RX_MCLK) {
        if (!aic->rx_clk) {
            printk(KERN_ERR "aic: aic%d no rx clk\n", id);
            return -ENODEV;
        }

        clk_set_rate(aic->rx_clk, freq);
        aic->capture.sys_freq = freq;

        if (dir == AIC_CLK_NOT_OUTPUT)
            return 0;

        return aic_rx_mclk_gpio_request(id);
    }

    if (clk_id == AIC_TX_MCLK) {
        if (!aic->tx_clk) {
            printk(KERN_ERR "aic: aic%d no tx clk\n", id);
            return -ENODEV;
        }

        clk_set_rate(aic->tx_clk, freq);
        aic->playback.sys_freq = freq;

        if (dir == AIC_CLK_NOT_OUTPUT)
            return 0;

        return aic_tx_mclk_gpio_request(id);
    }

    return -EINVAL;
}

static struct snd_soc_dai_ops aic_dai_ops = {
    .startup    = aic_startup,
    .shutdown   = aic_shutdown,
    .trigger    = aic_trigger,
    .hw_params  = aic_hw_params,
    .hw_free    = aic_hw_free,
    .set_fmt    = aic_set_dai_fmt,
    .set_sysclk = aic_set_sysclk,
};

static int aic_dai_probe(struct snd_soc_dai *dai)
{
    return 0;
}

static int aic_dai_remove(struct snd_soc_dai *dai)
{
    return 0;
}

static struct snd_soc_dai_driver dai_drivers[AIC_NUMS];

static void aic_data_init(struct aic_data *aic, int id, struct snd_soc_dai_driver *dai_driver)
{
    spin_lock_init(&aic->spinlock);
    mutex_init(&aic->lock);

    aic->gate_clk = clk_get(NULL, gate_names[id]);
    if (IS_ERR(aic->gate_clk))
        panic("aic: failed to get clk: %s\n", gate_names[id]);

    if (rx_names[id]) {
        aic->rx_clk = clk_get(NULL, rx_names[id]);
        if (IS_ERR(aic->rx_clk))
            panic("aic: failed to get clk: %s\n", rx_names[id]);
    }

    if (tx_names[id]) {
        aic->tx_clk = clk_get(NULL, tx_names[id]);
        if (IS_ERR(aic->tx_clk))
            panic("aic: failed to get clk: %s\n", tx_names[id]);
    }

    static char *dai_name[] = {
        "ingenic-aic.0", "ingenic-aic.1", "ingenic-aic.2",
        "ingenic-aic.3", "ingenic-aic.4",
    };

    static char *playback_name[] = {
        "aic0 playback", "aic1 playback", "aic2 playback", "aic3 playback", "aic4 playback",
    };

    static char *capture_name[] = {
        "aic0 capture", "aic1 capture", "aic2 capture", "aic3 capture", "aic4 capture",
    };

    dai_driver->id = id;
    dai_driver->probe = aic_dai_probe;
    dai_driver->remove = aic_dai_remove;
    dai_driver->name = dai_name[id];
    dai_driver->ops = &aic_dai_ops;
    dai_driver->symmetric_rates = !split_clk[id];
    dai_driver->symmetric_channels = !split_clk[id];
    dai_driver->symmetric_samplebits = !split_clk[id];

    if (aic->tx_clk) {
        dai_driver->playback.stream_name = playback_name[id];
        dai_driver->playback.channels_min = 1;
        dai_driver->playback.channels_max = 2;
        dai_driver->playback.rates = ASOC_AIC_RATE;
        dai_driver->playback.formats = ASOC_AIC_FORMATS;
    }

    if (aic->rx_clk) {
        dai_driver->capture.stream_name = capture_name[id];
        dai_driver->capture.channels_min = 1;
        dai_driver->capture.channels_max = 2;
        dai_driver->capture.rates = ASOC_AIC_RATE;
        dai_driver->capture.formats = ASOC_AIC_FORMATS;
    }
}

static void aic_data_deinit(struct aic_data *aic)
{
    clk_put(aic->gate_clk);
    if (aic->rx_clk)
        clk_put(aic->rx_clk);
    if (aic->tx_clk)
        clk_put(aic->tx_clk);
}

static struct snd_soc_component_driver aic_component = {
    .name = "ingenic-aic-component",
};

int aic_dma_driver_init(struct platform_device *pdev);

void aic_dma_driver_exit(struct platform_device *pdev);

static int aic_probe(struct platform_device *pdev)
{
    int ret;
    int i;

    for (i = 0; i < AIC_NUMS; i++)
        aic_data_init(&aic_datas[i], i, &dai_drivers[i]);

    ret = snd_soc_register_component(&pdev->dev, &aic_component, dai_drivers, AIC_NUMS);
    if (ret)
        panic("aic: failed to register snd component: %d\n", ret);

    ret = aic_dma_driver_init(pdev);
    if (ret)
        panic("aic: failed to register snd platform: %d\n", ret);

    return 0;
}

static int aic_remove(struct platform_device *pdev)
{
    int i;

    aic_dma_driver_exit(pdev);

    snd_soc_unregister_component(&pdev->dev);

    for (i = 0; i < AIC_NUMS; i++)
        aic_data_deinit(&aic_datas[i]);

    aic_gpio_free();

    return 0;
}

static struct platform_driver aic_driver = {
    .probe = aic_probe,
    .remove = aic_remove,
    .driver = {
        .name = "ingenic-aic",
        .owner = THIS_MODULE,
    },
};

static void aic_device_release(struct device *dev) {}

static struct platform_device aic_pdev = {
    .id = -1,
    .name = "ingenic-aic",
    .dev = {
        .release = aic_device_release,
    },
};

static void init_ce_clks(void)
{
    int i;

    /* 所有div clk 使用 M/N 分频器
     */
    for (i = 0; i < 4; i++) {
        struct clk *clk = clk_get(NULL, ce_names[i]);
        if (IS_ERR(clk))
            panic("aic: failed to get clk: %s\n", ce_names[i]);

        clk_prepare_enable(clk);

        clk_put(clk);
    }
}

static int aic_module_init(void)
{
    int ret;

    init_ce_clks();

    ret = platform_driver_register(&aic_driver);
    assert(!ret);

    ret = platform_device_register(&aic_pdev);
    assert(!ret);

    return 0;
}
module_init(aic_module_init);

static void aic_module_exit(void)
{
    platform_device_unregister(&aic_pdev);

    platform_driver_unregister(&aic_driver);
}
module_exit(aic_module_exit);

MODULE_LICENSE("GPL");
