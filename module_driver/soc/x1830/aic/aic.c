#include <linux/init.h>
#include <linux/module.h>
#include <linux/device.h>
#include <linux/delay.h>
#include <linux/clk.h>
#include <sound/core.h>
#include <sound/pcm.h>
#include <sound/pcm_params.h>
#include <sound/initval.h>
#include <sound/soc.h>
#include <sound/soc-dai.h>
#include <linux/slab.h>
#include <linux/delay.h>
#include <linux/interrupt.h>
#include <linux/irq.h>
#include <soc/irq.h>
#include <common.h>
#include <linux/cdev.h>
#include <linux/hrtimer.h>
#include <linux/io.h>
#include <linux/memory.h>
#include <linux/mm.h>
#include <linux/wait.h>
#include <linux/sched.h>
#include <linux/kthread.h>
#include <linux/workqueue.h>
#include <assert.h>
#include <utils/gpio.h>

#include <ingenic_asla_sound_card.h>

#include "aic_regs.h"

#define gpio_i2s_dac_lr_clk GPIO_PC(18)
#define gpio_i2s_dac_data   GPIO_PC(19)
#define gpio_i2s_adc_data   GPIO_PC(20)
#define gpio_i2s_adc_lr_clk GPIO_PC(21)
#define gpio_i2s_adc_bclk   GPIO_PC(22)
#define gpio_i2s_mclk       GPIO_PC(23)
#define gpio_i2s_dac_bclk   GPIO_PC(24)

struct dma_param {
    int dma_type;
    struct dma_chan *dma_chan;
    dma_addr_t dma_addr;

    char *buf;
    int buf_len;
    int period_size;
    int buswidth;
    int maxburst;
};

struct aic_data {
    unsigned int dma_pos;
    unsigned int rw_pos;
    unsigned int read_pos;
    unsigned int data_size;
    unsigned int period_time;
    unsigned int unit_size;
    unsigned int period_size;
    unsigned int frame_size;
    unsigned int channels;
    unsigned int format;

    void *dma_buffer;
    void *real_buffer;
    unsigned int dma_buffer_size;
    unsigned int buffer_size;

    int stereo_to_mono;

    unsigned int is_running;
    unsigned int expect_state;

    struct dma_param dma;

    struct hrtimer hrtimer;

    struct mutex mutex;
    struct task_struct *thread;
    unsigned int thread_stop;
    unsigned int thread_is_stop;

    struct work_struct work;
    struct work_struct start_work;
    struct work_struct stop_work;

    struct snd_pcm_substream *substream;
};

struct aic_dev {
    struct device *dev;
    struct platform_device *pdev;

    struct aic_data playback;
    struct aic_data capture;

    int as_master;
    int interface;

    int sample_rate;

    int clk_id;
    int clk_dir;
    int clk_div;
    unsigned int clk_freq;
    unsigned int set_clk_freq;

    unsigned int is_init;
    unsigned int is_enabled;

    struct clk *clk;
    struct clk *cgu_clk;
    struct mutex mutex;
    struct workqueue_struct *workqueue;
};

struct aic_dev aic_dev;

static int gpio_state = 0;

#define PCM_INTERFACE_I2S_MSB 1
#define PCM_INTERFACE_I2S 0

#define PCM_ON  1
#define PCM_OFF 0

#define MCLK_GPIO   BIT(0)
#define ADC_GPIO    BIT(1)
#define DAC_GPIO    BIT(2)

#define PLAYBACK_INIT   BIT(0)
#define CAPTURE_INIT    BIT(1)

#include "aic_dma.c"
#include "aic_hal.c"

#define BUFFER_ALIGN 32
#define PERIOD_BYTES_MIN 1024
#define PERIODS_MIN 1
#define PERIODS_MAX 128
#define MAX_DMA_BUFFERSIZE (128*1024)

#define ASOC_AIC_FORMATS (SNDRV_PCM_FMTBIT_S8 | SNDRV_PCM_FMTBIT_S16_LE | SNDRV_PCM_FMTBIT_S24_LE)
#define ASOC_AIC_RATE (SNDRV_PCM_RATE_8000 | SNDRV_PCM_RATE_16000 | SNDRV_PCM_RATE_44100 | \
                        SNDRV_PCM_RATE_48000 | SNDRV_PCM_RATE_96000)

static const struct snd_pcm_hardware aic_pcm_hardware = {
    .info = SNDRV_PCM_INFO_MMAP |
        SNDRV_PCM_INFO_PAUSE |
        SNDRV_PCM_INFO_RESUME |
        SNDRV_PCM_INFO_MMAP_VALID |
        SNDRV_PCM_INFO_INTERLEAVED |
        SNDRV_PCM_INFO_BLOCK_TRANSFER,
    .formats = ASOC_AIC_FORMATS,
    .rates = ASOC_AIC_RATE,
    .channels_min = 1,
    .channels_max = 2,
    .buffer_bytes_max = MAX_DMA_BUFFERSIZE,
    .period_bytes_min = PERIOD_BYTES_MIN,
    .period_bytes_max = MAX_DMA_BUFFERSIZE,
    .periods_min = PERIODS_MIN,
    .periods_max = PERIODS_MAX,
    .fifo_size = 0,
};

static int m_gpio_request(int gpio, const char *name, int func)
{
    char buf[10];

    int ret = gpio_request(gpio, name);
    if (ret) {
        printk(KERN_ERR "AIC: failed to request %s gpio: %s\n", name, gpio_to_str(gpio, buf));
        return -EINVAL;
    }

    gpio_set_func(gpio, func);

    return 0;
}

static int aic_ensure_dac_gpio_request(void)
{
    int ret = 0;

    if (!(gpio_state & DAC_GPIO)) {
        ret = m_gpio_request(gpio_i2s_dac_lr_clk, "dac-LR-clk", GPIO_FUNC_0);
        if (ret < 0)
            return -EINVAL;

        ret = m_gpio_request(gpio_i2s_dac_bclk, "dac-bclk", GPIO_FUNC_0);
        if (ret < 0)
            goto gpio_free_lr_clk;

        ret = m_gpio_request(gpio_i2s_dac_data, "dac-data", GPIO_FUNC_0);
        if (ret < 0)
            goto gpio_free_bclk;

        gpio_state |= DAC_GPIO;
    }

    return 0;

gpio_free_bclk:
    gpio_free(gpio_i2s_dac_bclk);
gpio_free_lr_clk:
    gpio_free(gpio_i2s_dac_lr_clk);

    return ret;
}

static int aic_ensure_adc_gpio_request(void)
{
    int ret = 0;

    if (!(gpio_state & ADC_GPIO)) {
        ret = m_gpio_request(gpio_i2s_adc_lr_clk, "adc-LR-clk", GPIO_FUNC_0);
        if (ret < 0)
            return -EINVAL;
        ret = m_gpio_request(gpio_i2s_adc_bclk, "adc-bclk", GPIO_FUNC_0);
        if (ret < 0)
            goto gpio_free_lr_clk;
        ret = m_gpio_request(gpio_i2s_adc_data, "adc-data", GPIO_FUNC_0);
        if (ret < 0)
            goto gpio_free_bclk;

        gpio_state |= ADC_GPIO;
    }

    return 0;

gpio_free_bclk:
    gpio_free(gpio_i2s_adc_bclk);
gpio_free_lr_clk:
    gpio_free(gpio_i2s_adc_lr_clk);

    return ret;
}

static int aic_ensure_mclk_gpio_requeset(void)
{
    int ret = 0;

    if (aic_dev.clk_dir == 0)
        return 0;

    if (!(gpio_state & MCLK_GPIO)) {
        ret = m_gpio_request(gpio_i2s_mclk, "i2s-mclk", GPIO_FUNC_0);
        if (ret < 0)
            return -EINVAL;
        gpio_state |= MCLK_GPIO;
    }

    return 0;
}

static void aic_gpio_release(void)
{
    if (gpio_state & MCLK_GPIO)
        gpio_free(gpio_i2s_mclk);

    if (gpio_state & ADC_GPIO) {
        gpio_free(gpio_i2s_adc_lr_clk);
        gpio_free(gpio_i2s_adc_bclk);
        gpio_free(gpio_i2s_adc_data);
    }

    if (gpio_state & DAC_GPIO) {
        gpio_free(gpio_i2s_dac_lr_clk);
        gpio_free(gpio_i2s_dac_bclk);
        gpio_free(gpio_i2s_dac_data);
    }
    gpio_state = 0;
}

static inline void *m_dma_alloc_coherent(int size)
{
    dma_addr_t dma_handle;
    void *mem = dma_alloc_coherent(NULL, size, &dma_handle, GFP_KERNEL);
    assert(mem);

    return (void *)CKSEG0ADDR(mem);
}

static inline void m_dma_free_coherent(int size)
{
    void *dma_buffer = aic_dev.capture.dma_buffer;
    if (dma_buffer) {
        dma_addr_t dma_handle = virt_to_phys(dma_buffer);
        dma_free_coherent(NULL, size, (void *)CKSEG1ADDR(dma_buffer), dma_handle);
        aic_dev.capture.dma_buffer = NULL;
    }
}

static unsigned int aic_get_writed_pos(void)
{
    int old_dma_pos = aic_dev.playback.dma_pos;
    struct dma_param *dma = &aic_dev.playback.dma;
    unsigned int buffer_size = aic_dev.playback.buffer_size;
    int buffer_addr = virt_to_phys(aic_dev.playback.dma_buffer);

    dma_addr_t cur_addr = get_dma_addr(dma->dma_chan, buffer_addr, buffer_size, DMA_MEM_TO_DEV);
    unsigned int pos = cur_addr - buffer_addr;

    if (pos == old_dma_pos)
        return 0;

    aic_dev.playback.dma_pos = pos;

    return pos;
}

static unsigned int aic_get_readable_size(void)
{
    int ret = 0;
    struct dma_param *dma = &aic_dev.capture.dma;
    struct dma_chan *dma_chan = dma->dma_chan;
    unsigned int buffer_size = aic_dev.capture.dma_buffer_size;
    int buffer_addr = virt_to_phys(aic_dev.capture.dma_buffer);

    dma_addr_t cur_addr = get_dma_addr(dma_chan, buffer_addr, buffer_size, DMA_DEV_TO_MEM);
    unsigned int pos = cur_addr - buffer_addr;
    unsigned int size = sub_pos(buffer_size, pos, aic_dev.capture.dma_pos);
    unsigned int unit_size = aic_dev.capture.unit_size;

    aic_dev.capture.dma_pos = pos;
    aic_dev.capture.data_size += size;

    if (aic_dev.capture.data_size > (buffer_size - unit_size)) {
        unsigned int align_pos = ALIGN(pos, unit_size);
        aic_dev.capture.rw_pos = add_pos(buffer_size, align_pos, unit_size);
        aic_dev.capture.data_size = buffer_size - sub_pos(buffer_size, aic_dev.capture.rw_pos, pos);
    }

    ret = aic_dev.capture.data_size > unit_size ? aic_dev.capture.data_size - unit_size : 0;

    if (aic_dev.capture.stereo_to_mono)
        return ret/2;
    else
        return ret;
}

static void stereo_to_mono(void *dst_, void *src_, int bytes)
{
    int count;
    int fmt = aic_dev.capture.format;

    if (fmt == SNDRV_PCM_FORMAT_S8) {
        char *dst = dst_;
        char *src = src_;
        count = bytes / sizeof(*dst) / 2;
        while (count--) {
            *dst++ = *src++;
            src++;
        }
    }

    if (fmt == SNDRV_PCM_FORMAT_S16_LE) {
        short *dst = dst_;
        short *src = src_;
        count = bytes / sizeof(*dst) / 2;
        while (count--) {
            *dst++ = *src++;
            src++;
        }
    }

    if (fmt == SNDRV_PCM_FORMAT_S24_LE) {
        int *dst = dst_;
        int *src = src_;
        count = bytes / sizeof(*dst) / 2;
        while (count--) {
            *dst++ = *src++;
            src++;
        }
    }
}

static void do_memcpy(void *dst, void *src, int bytes)
{
    if (aic_dev.capture.stereo_to_mono) {
        dma_cache_sync(NULL, src, bytes, DMA_DEV_TO_MEM);
        stereo_to_mono(dst, src, bytes);
    } else {
        dma_cache_sync(NULL, src, bytes, DMA_DEV_TO_MEM);
        memcpy(dst, src, bytes);
    }
}

static int aic_do_read_buffer(void *mem, unsigned int bytes)
{
    unsigned int buffer_size = aic_dev.capture.dma_buffer_size;
    unsigned int pos = aic_dev.capture.rw_pos;
    void *src = aic_dev.capture.dma_buffer;
    int alsa_dma_bytes = bytes;

    if (aic_dev.capture.stereo_to_mono)
        bytes *= 2;

    if (pos + bytes <= buffer_size) {
        do_memcpy(mem, src + pos, bytes);
    } else {
        unsigned int size1 = buffer_size - pos;
        do_memcpy(mem, src + pos, size1);
        do_memcpy(mem + size1, src, bytes - size1);
    }

    dma_cache_sync(NULL, mem, alsa_dma_bytes, DMA_MEM_TO_DEV);

    aic_dev.capture.rw_pos = add_pos(buffer_size, pos, bytes);

    aic_dev.capture.data_size -= bytes;

    return 0;
}

static int aic_read_bytes(struct aic_data *data, void *mem, int bytes)
{
    unsigned int len = 0;

    while (bytes) {
        unsigned int n = aic_get_readable_size();
        if (!n)
            break;

        if (n > bytes)
            n = bytes;

        aic_do_read_buffer(mem, n);
        len += n;
        mem += n;
        bytes -= n;
    }

    return len;
}

static int capture_dma_copy_thread(void *data_)
{
    int n = 0;
    int len = 0;
    struct aic_data *data = data_;
    int buffer_size = data->buffer_size;
    unsigned int period_time_us = data->period_time / 1000;

    data->read_pos = 0;

    while (1) {
        if (data->thread_stop)
            break;

        mutex_lock(&data->mutex);

        len = buffer_size - data->read_pos;
        if (len > 512)
            len = 512;

        n = aic_read_bytes(data, data->real_buffer + data->read_pos, len);
        data->read_pos += n;
        if (data->read_pos >= buffer_size)
            data->read_pos = 0;

        if (n)
            snd_pcm_period_elapsed(data->substream);

        mutex_unlock(&data->mutex);

        if (n != len)
            usleep_range(period_time_us, period_time_us);
    }
    data->thread_is_stop = 1;

    return 0;
}

static enum hrtimer_restart aic_hrtimer_callback(struct hrtimer *hrtimer)
{
    hrtimer_forward(hrtimer, hrtimer_get_expires(hrtimer), ns_to_ktime(aic_dev.playback.period_time));

    if (aic_get_writed_pos())
        snd_pcm_period_elapsed(aic_dev.playback.substream);

    return HRTIMER_RESTART;
}

static void aic_hrtimer_start(struct aic_data *data)
{
    hrtimer_forward_now(&data->hrtimer,
            ns_to_ktime(data->period_time));
    hrtimer_start_expires(&data->hrtimer, HRTIMER_MODE_ABS);
}

static void aic_hrtimer_stop(struct aic_data *data)
{
    hrtimer_cancel(&data->hrtimer);
}

static void aic_hrtimer_init(struct aic_data *data)
{
    hrtimer_init(&data->hrtimer, CLOCK_MONOTONIC, HRTIMER_MODE_REL);
    data->hrtimer.function = aic_hrtimer_callback;
}

static void aic_start_playback(void)
{
    mutex_lock(&aic_dev.mutex);
    aic_hal_start_playback();
    mutex_unlock(&aic_dev.mutex);
}

static void aic_start_capture(void)
{
    mutex_lock(&aic_dev.mutex);
    aic_hal_start_capture();
    mutex_unlock(&aic_dev.mutex);
}

static void aic_reset_dma_pos(void)
{
    aic_dev.playback.dma_pos = 0;
    snd_pcm_period_elapsed(aic_dev.playback.substream);
}

static void aic_stop_playback(void)
{
    mutex_lock(&aic_dev.mutex);
    aic_hal_stop_playback();
    aic_reset_dma_pos();
    mutex_unlock(&aic_dev.mutex);
}

static void aic_stop_capture(void)
{
    mutex_lock(&aic_dev.mutex);
    aic_hal_stop_capture();
    mutex_unlock(&aic_dev.mutex);
}

static void aic_init_common_setting(struct aic_data *data)
{
    if (aic_dev.is_init == 0)
        aic_hal_init_common_setting(data);

    if (data->substream->stream == SNDRV_PCM_STREAM_PLAYBACK)
        aic_dev.is_init |= PLAYBACK_INIT;
    else
        aic_dev.is_init |= CAPTURE_INIT;
}

static int aic_set_clk_rate(struct aic_data *data)
{
    int div = 0;
    unsigned int freq;
    int sample_rate = aic_dev.sample_rate;
    struct clk *cgu_clk = aic_dev.cgu_clk;
    int set_clk_freq = aic_dev.set_clk_freq;

    if (aic_dev.clk_id == SELECT_INNER_CODEC) {
        if (sample_rate == 96000)
            div = 128;
        else
            div = 256;
        freq = sample_rate * div;
        if (set_clk_freq != 0 && freq != set_clk_freq) {
            printk(KERN_ERR "AIC: no support clk freq %d\n", set_clk_freq);
            return -EINVAL;
        }
    } else {
        if (set_clk_freq == 0) {
            if (sample_rate <= 16000)
                div = 768;
            else if (sample_rate <= 24000)
                div = 512;
            else if (sample_rate <= 32000)
                div = 384;
            else if (sample_rate <= 48000)
                div = 256;
            else
                div = 256;
            freq = sample_rate * div;
        } else {
            freq = set_clk_freq;
            div = freq / sample_rate;
        }
    }

    aic_dev.clk_div = div;
    aic_dev.clk_freq = freq;

    clk_disable(cgu_clk);
    clk_set_rate(cgu_clk, freq);
    clk_enable(cgu_clk);

    return 0;
}

static void start_work(struct work_struct *work)
{
    struct aic_data *data = container_of(work, struct aic_data, start_work);

    if (data->expect_state == PCM_OFF)
        return;

    data->is_running = 1;

    if (data->substream->stream == SNDRV_PCM_STREAM_PLAYBACK) {
        mutex_lock(&data->mutex);
        aic_start_playback();
        aic_dma_submit_cyclic(&aic_dev.playback, DMA_MEM_TO_DEV);
        aic_hrtimer_start(&aic_dev.playback);
        mutex_unlock(&data->mutex);
    } else {
        mutex_lock(&data->mutex);
        aic_start_capture();
        aic_dma_submit_cyclic(&aic_dev.capture, DMA_DEV_TO_MEM);
        aic_dev.capture.thread_is_stop = 0;
        aic_dev.capture.thread = kthread_create(capture_dma_copy_thread, &aic_dev.capture, "capture_dma_copy");
        wake_up_process(aic_dev.capture.thread);
        mutex_unlock(&data->mutex);
    }
}

static void stop_work(struct work_struct *work)
{
    struct aic_data *data = container_of(work, struct aic_data, stop_work);

    if (data->expect_state == PCM_ON)
        return;

    if (data->substream->stream == SNDRV_PCM_STREAM_PLAYBACK) {
        mutex_lock(&data->mutex);
        aic_hrtimer_stop(&aic_dev.playback);
        aic_dma_terminate(&aic_dev.playback, DMA_MEM_TO_DEV);
        aic_stop_playback();
        mutex_unlock(&data->mutex);
    } else {
        data->thread_stop = 1;
        while (!aic_dev.capture.thread_is_stop)
            usleep_range(300, 300);
        aic_dev.capture.thread = NULL;

        mutex_lock(&data->mutex);
        aic_dma_terminate(&aic_dev.capture, DMA_DEV_TO_MEM);
        aic_stop_capture();
        mutex_unlock(&data->mutex);
    }
    data->is_running = 0;
}

static void aic_pcm_start(struct aic_data *data)
{
    data->expect_state = PCM_ON;
    queue_work(aic_dev.workqueue, &data->start_work);
}

static void aic_pcm_stop(struct aic_data *data)
{
    data->expect_state = PCM_OFF;
    queue_work(aic_dev.workqueue, &data->stop_work);
}

static int aic_trigger(struct snd_pcm_substream *substream, int cmd,
        struct snd_soc_dai *dai)
{
    switch (cmd) {
    case SNDRV_PCM_TRIGGER_START:
    case SNDRV_PCM_TRIGGER_RESUME:
    case SNDRV_PCM_TRIGGER_PAUSE_RELEASE:
        if (substream->stream == SNDRV_PCM_STREAM_PLAYBACK)
            aic_pcm_start(&aic_dev.playback);
        else
            aic_pcm_start(&aic_dev.capture);
        break;
    case SNDRV_PCM_TRIGGER_STOP:
    case SNDRV_PCM_TRIGGER_SUSPEND:
    case SNDRV_PCM_TRIGGER_PAUSE_PUSH:
        if (substream->stream == SNDRV_PCM_STREAM_PLAYBACK)
            aic_pcm_stop(&aic_dev.playback);
        else
            aic_pcm_stop(&aic_dev.capture);
        break;
    default:
        return -EINVAL;
    }
    return 0;
}

static int aic_hw_params(struct snd_pcm_substream *substream,
        struct snd_pcm_hw_params *hw_params, struct snd_soc_dai *dai)
{
    struct aic_data *data;
    int sample_rate;
    int ret = 0;

    mutex_lock(&aic_dev.mutex);

    if (substream->stream == SNDRV_PCM_STREAM_PLAYBACK) {
        if (aic_dev.is_init & PLAYBACK_INIT) {
            printk(KERN_ERR "AIC: playback is already initialized.\n");
            ret = -EBUSY;
            goto unlock_mutex;
        }
        data = &aic_dev.playback;
    } else {
        if (aic_dev.is_init & CAPTURE_INIT) {
            printk(KERN_ERR "AIC: caputer is already initialized.\n");
            ret = -EBUSY;
            goto unlock_mutex;
        }
        data = &aic_dev.capture;
    }

    sample_rate = params_rate(hw_params);
    if (aic_dev.is_init) {
        if (sample_rate != aic_dev.sample_rate) {
            printk(KERN_ERR "AIC: capture and playback sample rate %d are different!\n", aic_dev.sample_rate);
            ret = -EINVAL;
            goto unlock_mutex;
        }
    }

    data->channels = params_channels(hw_params);
    data->format = params_format(hw_params);
    data->substream = substream;
    aic_dev.sample_rate = sample_rate;

    ret = aic_set_clk_rate(data);
    if (ret < 0)
        goto unlock_mutex;

    ret = aic_ensure_mclk_gpio_requeset();
    if (ret < 0)
        goto unlock_mutex;

    aic_init_common_setting(data);

    if (substream->stream == SNDRV_PCM_STREAM_PLAYBACK) {
        /* 外部codec才需要request gpio */
        if (aic_dev.clk_id == SELECT_EXT_CODEC) {
            ret = aic_ensure_dac_gpio_request();
            if (ret < 0)
                goto unlock_mutex;
        }
        aic_init_playback_setting(data);
    } else {
        if (aic_dev.clk_id == SELECT_EXT_CODEC) {
            ret = aic_ensure_adc_gpio_request();
            if (ret < 0)
                goto unlock_mutex;
        }
        aic_init_capture_setting(data);
    }

unlock_mutex:
    mutex_unlock(&aic_dev.mutex);

    return ret;
}

static int aic_free(struct snd_pcm_substream *substream,
            struct snd_soc_dai *dai)
{
    mutex_lock(&aic_dev.mutex);

    if (substream->stream == SNDRV_PCM_STREAM_PLAYBACK)
        aic_dev.is_init &= ~PLAYBACK_INIT;
    else
        aic_dev.is_init &= ~CAPTURE_INIT;

    mutex_unlock(&aic_dev.mutex);

    return 0;
}

static int aic_set_dai_fmt(struct snd_soc_dai *dai, unsigned int fmt)
{
    unsigned int interface = fmt & SND_SOC_DAIFMT_FORMAT_MASK;
    unsigned int clk_dir = fmt & SND_SOC_DAIFMT_MASTER_MASK;

    switch (interface) {
    case SND_SOC_DAIFMT_I2S:
        aic_dev.interface = PCM_INTERFACE_I2S;
        break;
    case SND_SOC_DAIFMT_MSB:
        aic_dev.interface = PCM_INTERFACE_I2S_MSB;
        break;
    default:
        printk(KERN_ERR "AIC: fmt error: %x", interface);
        return -EINVAL;
    }

    switch (clk_dir) {
    case SND_SOC_DAIFMT_CBM_CFM:
        aic_dev.as_master = 0; /* codec as master */
        break;
    case SND_SOC_DAIFMT_CBS_CFS:
        aic_dev.as_master = 1; /* codec as slave */
        break;
    default:
        printk(KERN_ERR "AIC: clk dir error: %x", clk_dir);
        return -EINVAL;
    }

    return 0;
}

static int aic_set_sysclk(struct snd_soc_dai *dai, int clk_id,
        unsigned int freq, int dir)
{
    aic_dev.clk_id = clk_id;

    aic_dev.set_clk_freq = freq;

    aic_dev.clk_dir = dir;

    return 0;
}

static struct snd_soc_dai_ops aic_dai_ops = {
    .trigger    = aic_trigger,
    .hw_params  = aic_hw_params,
    .hw_free    = aic_free,
    .set_fmt    = aic_set_dai_fmt,
    .set_sysclk = aic_set_sysclk,
};

static int aic_dai_probe(struct snd_soc_dai *dai)
{
    return 0;
}

static struct snd_soc_dai_driver aic_dai = {
    .probe = aic_dai_probe,
    .suspend = NULL,
    .resume = NULL,
    .playback = {
        .channels_min = 1,
        .channels_max = 2,
        .rates = ASOC_AIC_RATE,
        .formats = ASOC_AIC_FORMATS,
    },
    .capture = {
        .channels_min = 1,
        .channels_max = 2,
        .rates = ASOC_AIC_RATE,
        .formats = ASOC_AIC_FORMATS,
    },
    .ops = &aic_dai_ops,
};

static int aic_dma_pcm_open(struct snd_pcm_substream *substream)
{
    struct snd_pcm_runtime *runtime = substream->runtime;

    int ret = snd_soc_set_runtime_hwparams(substream, &aic_pcm_hardware);
    if (ret) {
        printk(KERN_ERR "AIC: snd_soc_set_runtime_hwparams failed ret = %d\n", ret);
        return ret;
    }

    ret = snd_pcm_hw_constraint_step(runtime, 0, SNDRV_PCM_HW_PARAM_BUFFER_BYTES, BUFFER_ALIGN);
    if (ret) {
        printk(KERN_ERR "AIC: align hw_param buffer failed ret = %d\n", ret);
        return ret;
    }

    ret = snd_pcm_hw_constraint_step(runtime, 0, SNDRV_PCM_HW_PARAM_PERIOD_BYTES, BUFFER_ALIGN);
    if (ret) {
        printk(KERN_ERR "AIC: align hw_param period failed ret = %d\n", ret);
        return ret;
    }

    ret = snd_pcm_hw_constraint_integer(runtime, SNDRV_PCM_HW_PARAM_PERIODS);
    if (ret < 0) {
        printk(KERN_ERR "AIC: snd_pcm_hw_constraint_integer failed ret = %d\n", ret);
        return ret;
    }

    return 0;
}

static int aic_dma_pcm_close(struct snd_pcm_substream *substream)
{
    return 0;
}

static int aic_dma_pcm_prepare(struct snd_pcm_substream *substream)
{
    return 0;
}

static int aic_dma_playback_mmap(struct snd_pcm_substream *substream, struct vm_area_struct *vma)
{
    unsigned long start;
    unsigned long off;
    void *dma_buffer;

    off = vma->vm_pgoff << PAGE_SHIFT;

    dma_buffer = aic_dev.playback.dma_buffer;

    start = virt_to_phys(dma_buffer);
    start &= PAGE_MASK;
    off += start;

    vma->vm_pgoff = off >> PAGE_SHIFT;
    vma->vm_flags |= VM_IO;

    /* 0: cachable,write through (cache + cacheline对齐写穿)
    * 1: uncachable,write Acceleration (uncache + 硬件写加速)
    * 2: uncachable
    * 3: cachable
    */
    pgprot_val(vma->vm_page_prot) &= ~_CACHE_MASK;
    pgprot_val(vma->vm_page_prot) |= 0;

    if (io_remap_pfn_range(vma, vma->vm_start, off >> PAGE_SHIFT,
                        vma->vm_end - vma->vm_start, vma->vm_page_prot))
    {
        return -EAGAIN;
    }

    return 0;
}

static int aic_dma_pcm_mmap(struct snd_pcm_substream *substream, struct vm_area_struct *vma)
{
    if (substream->stream == SNDRV_PCM_STREAM_PLAYBACK)
        return aic_dma_playback_mmap(substream, vma);
    else
        return snd_pcm_lib_default_mmap(substream, vma);
}

static int aic_dma_pcm_hw_params(struct snd_pcm_substream *substream,
        struct snd_pcm_hw_params *hw_params)
{
    struct aic_data *data;
    int ret, channels, period_ms, buf_size, unit_size, format;

    if (substream->stream == SNDRV_PCM_STREAM_PLAYBACK) {
        data = &aic_dev.playback;
    } else {
        data = &aic_dev.capture;
        if (data->channels == 1) {
            data->stereo_to_mono = 1;
            data->channels = 2;
        } else {
            data->stereo_to_mono = 0;
        }
    }

    buf_size = params_buffer_bytes(hw_params);
    data->period_size = params_period_size(hw_params);
    channels = data->channels;

    format = data->format;
    if (format == SNDRV_PCM_FORMAT_S8) {
        data->dma.buswidth = DMA_SLAVE_BUSWIDTH_1_BYTE;
        data->dma.maxburst = 8;
    } else if (format == SNDRV_PCM_FORMAT_S24_LE) {
        data->dma.buswidth = DMA_SLAVE_BUSWIDTH_4_BYTES;
        data->dma.maxburst = 32;
    } else if (format == SNDRV_PCM_FORMAT_S16_LE) {
        if (substream->stream == SNDRV_PCM_STREAM_PLAYBACK) {
            if (channels == 1) {
                data->dma.buswidth = DMA_SLAVE_BUSWIDTH_2_BYTES;
                data->dma.maxburst = 16;
            } else {
                data->dma.buswidth = DMA_SLAVE_BUSWIDTH_4_BYTES;
                data->dma.maxburst = 32;
            }
        } else {
            data->dma.buswidth = DMA_SLAVE_BUSWIDTH_2_BYTES;
            data->dma.maxburst = 16;
        }
    }

    unit_size = data->dma.maxburst;
    if (buf_size % unit_size) {
        printk(KERN_ERR "AIC: ERROR buf_size UNALIGN %d.\n", buf_size);
        return -EINVAL;
    }

    data->unit_size = unit_size;

    substream->dma_buffer.dev.type = SNDRV_DMA_TYPE_DEV;
    ret = snd_pcm_lib_malloc_pages(substream, buf_size);
    if (ret < 0)
        return ret;

    data->frame_size = snd_pcm_format_physical_width(params_format(hw_params)) * channels / 8;/* 帧＝采样位数*通道 */

    if (substream->stream == SNDRV_PCM_STREAM_PLAYBACK) {
        data->dma_buffer = (unsigned long *)CKSEG0ADDR(substream->dma_buffer.area);
        data->dma_buffer_size = buf_size;
    } else {
        data->real_buffer = (unsigned long *)CKSEG0ADDR(substream->dma_buffer.area);

        if (data->stereo_to_mono)
            data->dma_buffer_size = buf_size * 2;
        else
            data->dma_buffer_size = buf_size;

        data->dma_buffer = m_dma_alloc_coherent(data->dma_buffer_size);
    }
    data->buffer_size = buf_size;

    period_ms = 1000 * data->period_size / aic_dev.sample_rate / data->frame_size;
    data->period_time = period_ms * 1000 * 1000;

    data->dma_pos = 0;
    data->rw_pos = 0;
    data->data_size = 0;
    data->thread_stop = 0;

    data->substream = substream;

    return 0;
}

static snd_pcm_uframes_t aic_dma_pcm_pointer(struct snd_pcm_substream *substream)
{
    int pos;

    if (substream->stream == SNDRV_PCM_STREAM_PLAYBACK)
        pos = aic_dev.playback.dma_pos;
    else
        pos = aic_dev.capture.read_pos;

    return bytes_to_frames(substream->runtime, pos);
}

static int aic_dma_pcm_hw_free(struct snd_pcm_substream *substream)
{
    if (substream->stream == SNDRV_PCM_STREAM_PLAYBACK) {
        while (aic_dev.playback.is_running)
            usleep_range(600, 600);
    } else {
        while (aic_dev.capture.is_running)
            usleep_range(600, 600);
        m_dma_free_coherent(aic_dev.capture.dma_buffer_size);
    }

    return snd_pcm_lib_free_pages(substream);
}

struct snd_pcm_ops aic_dma_pcm_ops = {
    .open       = aic_dma_pcm_open,
    .close      = aic_dma_pcm_close,
    .prepare    = aic_dma_pcm_prepare,
    .ioctl      = snd_pcm_lib_ioctl,
    .hw_params  = aic_dma_pcm_hw_params,
    .hw_free    = aic_dma_pcm_hw_free,
    .pointer    = aic_dma_pcm_pointer,
    .mmap       = aic_dma_pcm_mmap,
};

static void aic_dma_pcm_free(struct snd_pcm *pcm)
{
    dma_release_channel(aic_dev.playback.dma.dma_chan);
    dma_release_channel(aic_dev.capture.dma.dma_chan);
    snd_pcm_lib_preallocate_free_for_all(pcm);

    return;
}

static int aic_dma_pcm_new(struct snd_soc_pcm_runtime *rtd)
{
    int ret;
    struct snd_pcm *pcm = rtd->pcm;
    dma_cap_mask_t mask;
    struct dma_param *tx_dma = &aic_dev.playback.dma;
    struct dma_param *rx_dma = &aic_dev.capture.dma;

    dma_cap_zero(mask);
    dma_cap_set(DMA_SLAVE, mask);
    dma_cap_set(DMA_CYCLIC, mask);

    tx_dma->dma_chan = dma_request_channel(mask, dma_filter, tx_dma);
    if (!tx_dma->dma_chan)
        panic("AIC: aic dma tx_chan requested failed.\n");

    rx_dma->dma_chan = dma_request_channel(mask, dma_filter, rx_dma);
    if (!rx_dma->dma_chan)
        panic("AIC: aic dma rx_chan requested failed.\n");

    ret = snd_pcm_lib_preallocate_pages_for_all(pcm,
                    SNDRV_DMA_TYPE_DEV,
                    &aic_dev.pdev->dev,
                    MAX_DMA_BUFFERSIZE,
                    MAX_DMA_BUFFERSIZE);
    if (ret)
        panic("AIC: aic preallocate mem failed ret = %d\n", ret);

    return 0;
}

static struct snd_soc_platform_driver pcm_platform_driver = {
    .ops        = &aic_dma_pcm_ops,
    .pcm_new    = aic_dma_pcm_new,
    .pcm_free   = aic_dma_pcm_free,
};

static struct snd_soc_component_driver aic_component = {
    .name = "ingenic-aic-component",
};

static int aic_probe(struct platform_device *pdev)
{
    int ret;

    aic_dev.clk = clk_get(NULL, "aic");
    BUG_ON(IS_ERR(aic_dev.clk));

    aic_dev.cgu_clk = clk_get(NULL, "cgu_i2s");
    BUG_ON(IS_ERR(aic_dev.cgu_clk));

    clk_enable(aic_dev.clk);
    clk_set_rate(aic_dev.cgu_clk, 12288000);
    clk_enable(aic_dev.cgu_clk);

    aic_dev.dev = &pdev->dev;
    dev_set_drvdata(aic_dev.dev, NULL);
    aic_dev.pdev = pdev;

    dma_param_init(pdev, &aic_dev.playback.dma);
    dma_param_init(pdev, &aic_dev.capture.dma);

    aic_hrtimer_init(&aic_dev.playback);
    aic_hrtimer_init(&aic_dev.capture);

    INIT_WORK(&aic_dev.playback.start_work, start_work);
    INIT_WORK(&aic_dev.capture.start_work, start_work);
    INIT_WORK(&aic_dev.playback.stop_work, stop_work);
    INIT_WORK(&aic_dev.capture.stop_work, stop_work);

    aic_dev.workqueue = create_singlethread_workqueue("aic_work");

    mutex_init(&aic_dev.mutex);
    mutex_init(&aic_dev.capture.mutex);
    mutex_init(&aic_dev.playback.mutex);

    ret = snd_soc_register_component(&pdev->dev, &aic_component,
                &aic_dai, 1);
    if (ret)
        panic("AIC: aic snd_soc_register_component failed ret = %d!\n", ret);

    ret = snd_soc_register_platform(&pdev->dev, &pcm_platform_driver);
    if (ret)
        panic("AIC: aic snd_soc_register_platform failed ret = %d!\n", ret);

    return 0;
}

static int aic_remove(struct platform_device *pdev)
{
    snd_soc_unregister_platform(&pdev->dev);

    snd_soc_unregister_component(&pdev->dev);

    destroy_workqueue(aic_dev.workqueue);

    aic_gpio_release();

    clk_disable(aic_dev.cgu_clk);
    clk_disable(aic_dev.clk);
    clk_put(aic_dev.cgu_clk);
    clk_put(aic_dev.clk);

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

int aic_platform_device_init(void);
void aic_platform_device_exit(void);

static int aic_init(void)
{
    int ret = aic_platform_device_init();
    if (ret)
        return ret;

    return platform_driver_register(&aic_driver);
}
module_init(aic_init);

static void aic_exit(void)
{
    aic_platform_device_exit();

    platform_driver_unregister(&aic_driver);
}
module_exit(aic_exit);
MODULE_LICENSE("GPL");
