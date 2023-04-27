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

#define BUFFER_ALIGN 32
#define PERIOD_BYTES_MIN 1024
#define PERIODS_MIN 3
#define PERIODS_MAX 128
#define MAX_DMA_BUFFERSIZE (PERIODS_MAX*PERIOD_BYTES_MIN)

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


static const struct snd_pcm_hardware spdif_pcm_hardware = {
    .info = SNDRV_PCM_INFO_MMAP |
        SNDRV_PCM_INFO_PAUSE |
        SNDRV_PCM_INFO_RESUME |
        SNDRV_PCM_INFO_MMAP_VALID |
        SNDRV_PCM_INFO_INTERLEAVED |
        SNDRV_PCM_INFO_BLOCK_TRANSFER,
    .formats = ASOC_SPDIF_FORMATS,
    .buffer_bytes_max = MAX_DMA_BUFFERSIZE,
    .period_bytes_min = PERIOD_BYTES_MIN,
    .period_bytes_max = PERIOD_BYTES_MIN * 3,
    .periods_min = PERIODS_MIN,
    .periods_max = PERIODS_MAX,
    .fifo_size = 0,
};

#define State_idle 0
#define State_running 1
#define State_request_stop 2

struct spdif_dma_data {
    enum audio_dev_id spdif_dev;
    enum audio_dev_id dma_dev;
    struct audio_dma_desc *dma_desc;
    struct snd_pcm_substream *substream;
    unsigned long dma_addr;
    unsigned int buf_size;
    int period_ns;
    unsigned char hrtimer_state;
    struct hrtimer hrtimer;
};

static inline void *m_dma_alloc_coherent(int size)
{
    dma_addr_t dma_handle;
    void *mem = dma_alloc_coherent(NULL, size, &dma_handle, GFP_KERNEL);
    assert(mem);

    return (void *)CKSEG0ADDR(mem);
}

static inline void m_dma_free_coherent(void *mem, int size)
{
    dma_addr_t dma_handle = virt_to_phys(mem);
    dma_free_coherent(NULL, size, (void *)CKSEG1ADDR(mem), dma_handle);
}

static enum hrtimer_restart spdif_hrtimer_callback(struct hrtimer *hrtimer)
{
    struct spdif_dma_data *spdif_dma = container_of(hrtimer, struct spdif_dma_data, hrtimer);

    if (spdif_dma->hrtimer_state == State_request_stop) {
        spdif_dma->hrtimer_state = State_idle;
        return HRTIMER_NORESTART;
    }

    hrtimer_forward(hrtimer,
        hrtimer_get_expires(hrtimer), ns_to_ktime(spdif_dma->period_ns));

    snd_pcm_period_elapsed(spdif_dma->substream);

    return HRTIMER_RESTART;
}

static int spdif_dma_pcm_open(struct snd_pcm_substream *substream)
{
    struct snd_pcm_runtime *runtime = substream->runtime;
    struct spdif_dma_data *spdif_dma;

    int ret = snd_soc_set_runtime_hwparams(substream, &spdif_pcm_hardware);
    if (ret) {
        printk(KERN_ERR "spdif_dma: snd_soc_set_runtime_hwparams failed: %d\n", ret);
        return ret;
    }

    ret = snd_pcm_hw_constraint_step(runtime, 0, SNDRV_PCM_HW_PARAM_BUFFER_BYTES, BUFFER_ALIGN);
    if (ret) {
        printk(KERN_ERR "spdif_dma: align hw_param buffer failed ret = %d\n", ret);
        return ret;
    }

    ret = snd_pcm_hw_constraint_step(runtime, 0, SNDRV_PCM_HW_PARAM_PERIOD_BYTES, BUFFER_ALIGN);
    if (ret) {
        printk(KERN_ERR "spdif_dma: align hw_param period failed ret = %d\n", ret);
        return ret;
    }

    ret = snd_pcm_hw_constraint_integer(runtime, SNDRV_PCM_HW_PARAM_PERIODS);
    if (ret < 0) {
        printk(KERN_ERR "spdif_dma: snd_pcm_hw_constraint_integer failed: %d\n", ret);
        return ret;
    }

    spdif_dma = kmalloc(sizeof(*spdif_dma), GFP_KERNEL);

    spdif_dma->dma_desc = m_dma_alloc_coherent(sizeof(*spdif_dma->dma_desc));

    hrtimer_init(&spdif_dma->hrtimer, CLOCK_MONOTONIC, HRTIMER_MODE_REL);
    spdif_dma->hrtimer.function = spdif_hrtimer_callback;
    spdif_dma->substream = substream;

    runtime->private_data = spdif_dma;

    return 0;
}

static int spdif_dma_pcm_close(struct snd_pcm_substream *substream)
{
    struct snd_pcm_runtime *runtime = substream->runtime;
    struct spdif_dma_data *spdif_dma = runtime->private_data;

    m_dma_free_coherent(spdif_dma->dma_desc, sizeof(*spdif_dma->dma_desc));

    kfree(spdif_dma);

    return 0;
}

static int spdif_dma_pcm_prepare(struct snd_pcm_substream *substream)
{
    return 0;
}

static int spdif_dma_mmap(struct snd_pcm_substream *substream, struct vm_area_struct *vma)
{
    return snd_pcm_lib_default_mmap(substream, vma);
}

#ifdef DEBUG
static void spdif_playback_cb(void *data)
{
    struct spdif_dma_data *spdif_dma = data;
    int dma_id = dev_to_dma_id(spdif_dma->dma_dev);

    unsigned int dsr = audio_read_reg(DSR(dma_id));

    audio_write_reg(DSR(dma_id), dsr);

    printk(KERN_EMERG "spdif_playback_cb: %x\n", dsr);
}
#endif

static int spdif_dma_pcm_hw_params(struct snd_pcm_substream *substream,
        struct snd_pcm_hw_params *hw_params)
{
    struct snd_pcm_runtime *runtime = substream->runtime;
    struct spdif_dma_data *spdif_dma = runtime->private_data;
    int is_capture = substream->stream == SNDRV_PCM_STREAM_CAPTURE;

    int format = params_format(hw_params);
    int channels = params_channels(hw_params);
    int buf_size = params_buffer_bytes(hw_params);
    int period_size = params_period_bytes(hw_params);
    int sample_rate = params_rate(hw_params);
    int fmt_width = snd_pcm_format_width(format);
    int unit_size = 32;

    if (is_capture) {
        spdif_dma->spdif_dev = Dev_src_spdif_in;
        spdif_dma->dma_dev = audio_requst_tar_dma_dev();
    } else {
        spdif_dma->spdif_dev = Dev_tar_spdif_out;
        spdif_dma->dma_dev = audio_requst_src_dma_dev();
    }

    if (spdif_dma->dma_dev == 0) {
        printk(KERN_ERR "spdif_dma: spdif failed to request dma channel\n");
        return -EBUSY;
    }

    int ret = snd_pcm_lib_malloc_pages(substream, buf_size);
    if (ret < 0) {
        printk(KERN_ERR "spdif_dma: spdif failed to alloc pages： %d\n", ret);
        audio_release_dma_dev(spdif_dma->dma_dev);
        return ret;
    }

    int frame_size = params_physical_width(hw_params) * channels / 8;
    int period_ms = 1000 * period_size / sample_rate / frame_size;
    if (!period_ms)
        period_ms = 1;
    spdif_dma->period_ns = period_ms * 1000 * 1000;
    spdif_dma->hrtimer_state = State_idle;

    void *dma_buffer = (void *)CKSEG0ADDR(substream->dma_buffer.area);
    audio_dma_desc_init(spdif_dma->dma_desc, dma_buffer, buf_size, unit_size, spdif_dma->dma_desc);

    spdif_dma->dma_addr = virt_to_phys(dma_buffer);
    spdif_dma->buf_size = buf_size;

    if (is_capture)
        audio_connect_dev(spdif_dma->spdif_dev, spdif_dma->dma_dev);
    else
        audio_connect_dev(spdif_dma->dma_dev, spdif_dma->spdif_dev);

#ifdef DEBUG
    audio_dma_set_callback(spdif_dma->dma_dev, spdif_playback_cb, spdif_dma);
#else
    audio_dma_set_callback(spdif_dma->dma_dev, NULL, NULL);
#endif

    /* spdif只能支持双声道，但寄存器参数还是填1 */
    audio_dma_config(spdif_dma->dma_dev, 1, fmt_width, unit_size, format);

    return 0;
}

static int spdif_dma_pcm_hw_free(struct snd_pcm_substream *substream)
{
    struct snd_pcm_runtime *runtime = substream->runtime;
    struct spdif_dma_data *spdif_dma = runtime->private_data;
    int is_capture = substream->stream == SNDRV_PCM_STREAM_CAPTURE;

    if (spdif_dma->hrtimer_state != State_idle) {
        hrtimer_cancel(&spdif_dma->hrtimer);
        spdif_dma->hrtimer_state = State_idle;
    }

    if (is_capture)
        audio_disconnect_dev(spdif_dma->spdif_dev, spdif_dma->dma_dev);
    else
        audio_disconnect_dev(spdif_dma->dma_dev, spdif_dma->spdif_dev);

    audio_release_dma_dev(spdif_dma->dma_dev);

    return 0;
}

static int spdif_dma_trigger(struct snd_pcm_substream *substream, int cmd)
{
    struct snd_pcm_runtime *runtime = substream->runtime;
    struct spdif_dma_data *spdif_dma = runtime->private_data;
    int ret = 0;

    switch (cmd) {
    case SNDRV_PCM_TRIGGER_START:
    case SNDRV_PCM_TRIGGER_RESUME:
    case SNDRV_PCM_TRIGGER_PAUSE_RELEASE:
        audio_dma_start(spdif_dma->dma_dev, spdif_dma->dma_desc);
        if (spdif_dma->hrtimer_state == State_idle) {
            hrtimer_forward_now(&spdif_dma->hrtimer,
                        ns_to_ktime(spdif_dma->period_ns));
            hrtimer_start_expires(&spdif_dma->hrtimer, HRTIMER_MODE_ABS);
        }

        spdif_dma->hrtimer_state = State_running;
        break;
    case SNDRV_PCM_TRIGGER_STOP:
    case SNDRV_PCM_TRIGGER_SUSPEND:
    case SNDRV_PCM_TRIGGER_PAUSE_PUSH:
        audio_dma_stop(spdif_dma->dma_dev);
        spdif_dma->hrtimer_state = State_request_stop;
        break;
    default:
        ret = -EINVAL;
        break;
    }

    return ret;
}

static int debug = 0;
module_param(debug, int, 0644);

static snd_pcm_uframes_t spdif_dma_pcm_pointer(struct snd_pcm_substream *substream)
{
    int pos;
    struct snd_pcm_runtime *runtime = substream->runtime;
    struct spdif_dma_data *spdif_dma = runtime->private_data;

    pos = audio_dma_get_current_addr(spdif_dma->dma_dev);

    if (debug)
        printk(KERN_EMERG "pos: %x", pos);

    if (spdif_dma->dma_addr <= pos && pos < spdif_dma->dma_addr + spdif_dma->buf_size)
        pos = pos - spdif_dma->dma_addr;
    else
        pos = 0;

    return bytes_to_frames(substream->runtime, pos);
}

struct snd_pcm_ops spdif_dma_pcm_ops = {
    .open       = spdif_dma_pcm_open,
    .close      = spdif_dma_pcm_close,
    .prepare    = spdif_dma_pcm_prepare,
    .ioctl      = snd_pcm_lib_ioctl,
    .hw_params  = spdif_dma_pcm_hw_params,
    .hw_free    = spdif_dma_pcm_hw_free,
    .pointer    = spdif_dma_pcm_pointer,
    .trigger    = spdif_dma_trigger,
    .mmap       = spdif_dma_mmap,
};

static int spdif_dma_pcm_new(struct snd_soc_pcm_runtime *rtd)
{
    struct snd_pcm *pcm = rtd->pcm;

    int ret = snd_pcm_lib_preallocate_pages_for_all(
        pcm, SNDRV_DMA_TYPE_DEV, rtd->dev, MAX_DMA_BUFFERSIZE, MAX_DMA_BUFFERSIZE);
    if (ret)
        panic("spdif_dma: failed to pre allocate pages\n");

    return 0;
}

static void spdif_dma_pcm_free(struct snd_pcm *pcm)
{
    snd_pcm_lib_preallocate_free_for_all(pcm);
}

static struct snd_soc_platform_driver pcm_platform_driver = {
    .ops        = &spdif_dma_pcm_ops,
    .pcm_new    = spdif_dma_pcm_new,
    .pcm_free   = spdif_dma_pcm_free,
};

int spdif_dma_driver_init(struct platform_device *pdev)
{
    return snd_soc_register_platform(&pdev->dev, &pcm_platform_driver);
}

void spdif_dma_driver_exit(struct platform_device *pdev)
{
    snd_soc_unregister_platform(&pdev->dev);
}
