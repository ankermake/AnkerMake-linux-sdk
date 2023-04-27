#include <linux/init.h>
#include <linux/module.h>
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

#include "spdif.h"

#include "spdif_regs.h"
#include "spdif_dma.h"

#include "spdif_gpio.c"

static const unsigned long iobase[] = {
    KSEG1ADDR(SPDIF_OUT_BASE),
    KSEG1ADDR(SPDIF_IN_BASE),
};

#define SPDIF_ADDR(id, reg) ((volatile unsigned long *)((iobase[id]) + (reg)))

static inline void spdif_write_reg(int id, unsigned int reg, unsigned int value)
{
    *SPDIF_ADDR(id, reg) = value;
}

static inline unsigned int spdif_read_reg(int id, unsigned int reg)
{
    return *SPDIF_ADDR(id, reg);
}

static inline void spdif_set_bit(int id, unsigned int reg, int start, int end, unsigned int val)
{
    set_bit_field(SPDIF_ADDR(id, reg), start, end, val);
}

static inline unsigned int spdif_get_bit(int id, unsigned int reg, int start, int end)
{
    return get_bit_field(SPDIF_ADDR(id, reg), start, end);
}

struct spdif_data {
    struct snd_soc_dai_driver dai_driver;
    spinlock_t spinlock;
    struct mutex lock;
    struct clk *parent_gate_clk;
    struct clk *parent_ce_clk;
    struct clk *parent_clk;
    struct clk *gate_clk;
    struct clk *clk;
    bool non_pcm;
    uint out_clk_div;
    uint in_clk_div;
    unsigned int sysclk;
};

struct spdif_data spdif_datas;

static inline void spdif_dump_regs(int id)
{
    printk(KERN_EMERG "SPENA: %x\n", spdif_read_reg(id, SPENA));
    printk(KERN_EMERG "SPCTRL: %x\n", spdif_read_reg(id, SPCTRL));
    printk(KERN_EMERG "SPCFG1: %x\n", spdif_read_reg(id, SPCFG1));
    printk(KERN_EMERG "SPCFG2: %x\n", spdif_read_reg(id, SPCFG2));
    printk(KERN_EMERG "SPDIV: %x\n", spdif_read_reg(id, SPDIV));
}

static void spdif_start_playback(int id)
{
    spdif_write_reg(id, SPENA, SPO_SPEN);
}

static int spdif_stop_playback(int id)
{
    uint val, timeout = 0xfff;

    spdif_write_reg(id, SPENA, 0);

    do {
       val = spdif_read_reg(id, SPENA);
    } while((val & SPO_SPEN) && --timeout);
    if (!timeout)
        return -EINVAL;

    return 0;
}

static void spdif_start_capture(int id)
{
    spdif_set_bit(id, SPIENA, SPI_SPIEN_MASK, SPI_SPIEN);
    ndelay(200);
    spdif_set_bit(id, SPIENA, SPI_RESET_MASK, SPI_RESET);
}

static void spdif_stop_capture(int id)
{
    spdif_set_bit(id, SPIENA, SPI_RESET_MASK, SPI_RESET);
    ndelay(200);
    spdif_set_bit(id, SPIENA, SPI_SPIEN_MASK, 0);
}

#define ASOC_SPDIF_FORMATS (SNDRV_PCM_FMTBIT_S16_LE | SNDRV_PCM_FMTBIT_S16_BE |\
	SNDRV_PCM_FMTBIT_U16_LE | SNDRV_PCM_FMTBIT_U16_BE |\
	SNDRV_PCM_FMTBIT_S24_LE | SNDRV_PCM_FMTBIT_S24_BE |\
	SNDRV_PCM_FMTBIT_U24_LE | SNDRV_PCM_FMTBIT_U24_BE |\
	SNDRV_PCM_FMTBIT_S24_3LE | SNDRV_PCM_FMTBIT_S24_3BE |\
	SNDRV_PCM_FMTBIT_U24_3LE | SNDRV_PCM_FMTBIT_U24_3BE |\
	SNDRV_PCM_FMTBIT_S20_3LE | SNDRV_PCM_FMTBIT_S20_3BE |\
	SNDRV_PCM_FMTBIT_U20_3LE | SNDRV_PCM_FMTBIT_U20_3BE |\
	SNDRV_PCM_FMTBIT_S18_3LE | SNDRV_PCM_FMTBIT_S18_3BE |\
	SNDRV_PCM_FMTBIT_U18_3LE | SNDRV_PCM_FMTBIT_U18_3BE)


#define ASOC_SPDIF_RATE        (SNDRV_PCM_RATE_8000 | SNDRV_PCM_RATE_11025 | \
                SNDRV_PCM_RATE_16000 | SNDRV_PCM_RATE_22050 |\
                SNDRV_PCM_RATE_32000 | SNDRV_PCM_RATE_44100 |\
                SNDRV_PCM_RATE_48000 | SNDRV_PCM_RATE_64000 | SNDRV_PCM_RATE_88200 |\
                SNDRV_PCM_RATE_96000 | SNDRV_PCM_RATE_176400 |\
                SNDRV_PCM_RATE_192000)

static int spdif_trigger(struct snd_pcm_substream *substream, int cmd,
        struct snd_soc_dai *dai)
{
    int is_capture = substream->stream == SNDRV_PCM_STREAM_CAPTURE;

    switch (cmd) {
    case SNDRV_PCM_TRIGGER_START:
    case SNDRV_PCM_TRIGGER_RESUME:
    case SNDRV_PCM_TRIGGER_PAUSE_RELEASE:
        if (is_capture)
            spdif_start_capture(is_capture);
        else
            spdif_start_playback(is_capture);
        break;
    case SNDRV_PCM_TRIGGER_STOP:
    case SNDRV_PCM_TRIGGER_SUSPEND:
    case SNDRV_PCM_TRIGGER_PAUSE_PUSH:
        if (is_capture)
            spdif_stop_capture(is_capture);
        else
            return spdif_stop_playback(is_capture);
        break;
    default:
        return -EINVAL;
    }
    return 0;
}

static int spdif_max_wl(uint *max_wl, int fmt_width)
{
    if (fmt_width <= 24 && fmt_width >= 20)
        *max_wl = 1;
    else if (fmt_width <= 20 && fmt_width >= 16)
        *max_wl = 0;
    else
        return -EINVAL;

    return 0;
}

struct sampl_freq {
    u32 freq;
    u32 val;
} sample[] = {
    /* References : IEC 60958-3 configure */
    {22050,  0x2<<4 | 0xb},
    {24000,  0x6<<4 | 0x9},
    {32000,  0xc<<4 | 0x3},
    {44100,  0x0<<4 | 0xf},
    {48000,  0x4<<4 | 0xd},
    {88200,  0x1<<4 | 0xe},
    {96000,  0xa<<4 | 0x5},
    {192000, 0x7<<4 | 0x8},
    {176400, 0x3<<4 | 0xc},
    {8000,   0x6},
    {11025,  0x5},
    {12000,  0x4},
    {16000,  0x1},
};

static int spdif_rate(unsigned long sample_rate)
{
    int i;

    for (i = 0; i < ARRAY_SIZE(sample); i++) {
        if (sample[i].freq == sample_rate) {
            return sample[i].val;
        }
    }
    panic("spdif: unsupport rate: %ld\n",  sample_rate);
}

static int spdif_sampl_wl(int fmt_width)
{
    if (fmt_width == 20 || fmt_width == 16)
        return 1;
    if (fmt_width == 21 || fmt_width == 17)
        return 6;
    if (fmt_width == 22 || fmt_width == 18)
        return 2;
    if (fmt_width == 23 || fmt_width == 19)
        return 4;
    if (fmt_width == 24 || fmt_width == 20)
        return 5;
    panic("spdif: unsupport fmt_width: %d\n",  fmt_width);
}

static int spdif_div(int sysclk, int stream, unsigned long rate, int channels)
{
    int clk_ratio = stream ? 1280 : 32 * 2 * channels;  //fixed 2channels
    int clk, div;

    clk = rate * clk_ratio;
    div = ((sysclk + clk - 1) / clk) & (~0x1UL);

    return div;
}

static int spdif_set_clkdiv(struct snd_soc_dai *dai, int div_id, int div)
{
    struct spdif_data *spdif = &spdif_datas;

    switch (div_id) {
    case SNDRV_PCM_STREAM_PLAYBACK:
        spdif->out_clk_div = div;
        break;
    case SNDRV_PCM_STREAM_CAPTURE:
        spdif->in_clk_div = div;
        break;
    default:
        pr_err("%s:%d\n", __func__, __LINE__);
        return -EINVAL;
    }

    return 0;
}

static int spdif_hw_params(struct snd_pcm_substream *substream,
        struct snd_pcm_hw_params *hw_params, struct snd_soc_dai *dai)
{
    struct spdif_data *spdif = &spdif_datas;
    int is_capture = substream->stream == SNDRV_PCM_STREAM_CAPTURE;
    int rate = params_rate(hw_params);
    int channels = params_channels(hw_params);
    int fmt_width = snd_pcm_format_width(params_format(hw_params));
    uint max_wl = 0, div = 0, sampl_wl = 0, rate_reg = 0;

    if (spdif_max_wl(&max_wl, fmt_width)) {
        printk(KERN_ERR "spdif: spdif fmt_width not valid: %d\n", fmt_width);
        return -EINVAL;
    }

    if (is_capture) {
        div = spdif_div(spdif->sysclk, substream->stream, rate, channels);
        if (spdif->in_clk_div)
            div = spdif->in_clk_div;
        spdif_write_reg(is_capture, SPIDIV, SPI_SET_DV(div));
    } else {
        if (!spdif->non_pcm) {
            sampl_wl = spdif_sampl_wl(fmt_width);
            rate_reg = spdif_rate(rate);
            spdif_set_bit(is_capture, SPCFG2, SPO_CFG2_FRQ, rate_reg);
            spdif_set_bit(is_capture, SPCFG2, SPO_CFG2_SAMPL_WL, sampl_wl);
            spdif_set_bit(is_capture, SPCFG2, SPO_CFG2_MAX_WL, max_wl);
        }
        spdif_set_bit(is_capture, SPCTRL, SPO_SIGN_N, snd_pcm_format_signed(params_format(hw_params)) ? 0 : 1);
        div = spdif_div(spdif->sysclk, substream->stream, rate, channels);
        if (spdif->out_clk_div)
            div = spdif->out_clk_div;
        spdif_write_reg(is_capture, SPDIV, SPO_SET_DV(div));
    }

    return 0;
}

static int spdif_hw_free(struct snd_pcm_substream *substream, struct snd_soc_dai *dai)
{
    return 0;
}

static int spdif_startup(struct snd_pcm_substream *substream, struct snd_soc_dai *dai)
{
    struct spdif_data *spdif = &spdif_datas;
    int is_capture = substream->stream == SNDRV_PCM_STREAM_CAPTURE;

    if (spdif->clk)
        clk_prepare_enable(spdif->clk);

    if (spdif->gate_clk)
        clk_prepare_enable(spdif->gate_clk);

    if (!is_capture) {
        spdif_out_gpio_request();
        spdif_set_bit(is_capture, SPCFG2, SPO_PCM_MODE, spdif->non_pcm ? SPO_AUDIO_N : 0);

        if (!spdif->non_pcm) {
            uint ch_cfg = SPO_SET_SCR_NUM(1) | SPO_SET_CH1_NUM(1) | SPO_SET_CH2_NUM(1);

            spdif_set_bit(is_capture, SPCFG1, SPO_CFG1, ch_cfg);
            spdif_set_bit(is_capture, SPCFG2, SPO_CFG2_CATCODE, 0);
            spdif_set_bit(is_capture, SPCFG2, SPO_CFG2_CH_MD, 0);
        }

        spdif_set_bit(is_capture, SPCTRL, SPO_INVALID, 0);
    } else
        spdif_in_gpio_request();

    return 0;
}

static void spdif_shutdown(struct snd_pcm_substream *substream, struct snd_soc_dai *dai)
{
    struct spdif_data *spdif = &spdif_datas;
    if (spdif->clk)
        clk_disable_unprepare(spdif->clk);

    if (spdif->gate_clk)
        clk_disable_unprepare(spdif->gate_clk);
}

static int spdif_set_sysclk(struct snd_soc_dai *dai, int clk_id, unsigned int freq, int dir)
{
    struct spdif_data *spdif = &spdif_datas;
    clk_set_rate(spdif->clk, freq);
    spdif->sysclk = freq;
    return 0;
}

static struct snd_soc_dai_ops spdif_dai_ops = {
    .startup    = spdif_startup,
    .shutdown   = spdif_shutdown,
    .trigger    = spdif_trigger,
    .hw_params  = spdif_hw_params,
    .hw_free    = spdif_hw_free,
    .set_sysclk = spdif_set_sysclk,
    .set_clkdiv = spdif_set_clkdiv,
};

static int spdif_dai_probe(struct snd_soc_dai *dai)
{
    return 0;
}

static int spdif_dai_remove(struct snd_soc_dai *dai)
{
    return 0;
}

static struct snd_soc_dai_driver dai_drivers;

static void spdif_data_init(struct spdif_data *spdif, struct snd_soc_dai_driver *dai_driver)
{
    spin_lock_init(&spdif->spinlock);
    mutex_init(&spdif->lock);

    spdif->parent_ce_clk = clk_get(NULL, "ce_i2s2");
    if (IS_ERR(spdif->parent_ce_clk))
        panic("spdif: failed to get parent_ce_clk: ce_i2s2\n");
    clk_prepare_enable(spdif->parent_ce_clk);

    spdif->parent_gate_clk = clk_get(NULL, "gate_i2s2");
    if (IS_ERR(spdif->parent_gate_clk))
        panic("spdif: failed to get parent_gate_clk: gate_i2s2\n");

    spdif->gate_clk = clk_get(NULL, "gate_spdif");
    if (IS_ERR(spdif->gate_clk))
        panic("spdif: failed to get gate_clk: gate_spdif\n");

    spdif->clk = clk_get(NULL, "mux_spdif");
    if (IS_ERR(spdif->clk))
        panic("spdif: failed to get clk: mux_spdif\n");

    spdif->parent_clk = clk_get(NULL, "div_i2s2");
    if (IS_ERR(spdif->parent_clk))
        panic("spdif: failed to get parent_clk: div_i2s2\n");
    clk_set_parent(spdif->clk, spdif->parent_clk);

    dai_driver->id = 0;
    dai_driver->probe = spdif_dai_probe;
    dai_driver->remove = spdif_dai_remove;
    dai_driver->name = "ingenic-spdif";
    dai_driver->ops = &spdif_dai_ops;

    dai_driver->playback.stream_name = "spdif playback";
    dai_driver->playback.channels_min = 2;
    dai_driver->playback.channels_max = 2;
    dai_driver->playback.rates = ASOC_SPDIF_RATE;
    dai_driver->playback.formats = ASOC_SPDIF_FORMATS;

    dai_driver->capture.stream_name = "spdif capture";
    dai_driver->capture.channels_min = 2;
    dai_driver->capture.channels_max = 2;
    dai_driver->capture.rates = ASOC_SPDIF_RATE;
    dai_driver->capture.formats = ASOC_SPDIF_FORMATS;
}

static void spdif_data_deinit(struct spdif_data *spdif)
{
    if (spdif->parent_ce_clk)
        clk_put(spdif->parent_ce_clk);

    if (spdif->parent_clk)
        clk_put(spdif->parent_clk);

    if (spdif->parent_gate_clk)
        clk_put(spdif->parent_gate_clk);

    if (spdif->gate_clk)
        clk_put(spdif->gate_clk);

    if (spdif->clk)
        clk_put(spdif->clk);
}

static struct snd_soc_component_driver spdif_component = {
    .name = "ingenic-spdif-component",

    .open       = spdif_dma_pcm_open,
    .close      = spdif_dma_pcm_close,
    .prepare    = spdif_dma_pcm_prepare,
    .hw_params  = spdif_dma_pcm_hw_params,
    .hw_free    = spdif_dma_pcm_hw_free,
    .pointer    = spdif_dma_pcm_pointer,
    .trigger    = spdif_dma_trigger,
    .mmap       = spdif_dma_mmap,

    .pcm_construct    = spdif_dma_pcm_new,
    .pcm_destruct   = spdif_dma_pcm_free,
};

static int spdif_probe(struct platform_device *pdev)
{
    int ret;
    spdif_data_init(&spdif_datas, &dai_drivers);

    ret = snd_soc_register_component(&pdev->dev, &spdif_component, &dai_drivers, 1);
    if (ret)
        panic("spdif: failed to register snd component: %d\n", ret);

    return 0;
}

static int spdif_remove(struct platform_device *pdev)
{
    snd_soc_unregister_component(&pdev->dev);

    spdif_data_deinit(&spdif_datas);

    spdif_gpio_free();

    return 0;
}

static void spdif_device_release(struct device *dev) {}

static struct platform_driver spdif_driver = {
    .probe = spdif_probe,
    .remove = spdif_remove,
    .driver = {
        .name = "ingenic-spdif",
        .owner = THIS_MODULE,
    },
};

static struct platform_device spdif_pdev = {
    .id = -1,
    .name = "ingenic-spdif",
    .dev = {
        .release = spdif_device_release,
    },
};

static int spdif_module_init(void)
{
    int ret;

    ret = platform_driver_register(&spdif_driver);
    assert(!ret);

    ret = platform_device_register(&spdif_pdev);
    assert(!ret);

    return 0;
}
module_init(spdif_module_init);

static void spdif_module_exit(void)
{
    platform_device_unregister(&spdif_pdev);

    platform_driver_unregister(&spdif_driver);
}
module_exit(spdif_module_exit);

MODULE_LICENSE("GPL");
