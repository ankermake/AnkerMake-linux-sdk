


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
#include <linux/err.h>
#include <linux/kthread.h>

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

static const struct snd_pcm_hardware aic_pcm_hardware = {
    .info = SNDRV_PCM_INFO_MMAP |
        SNDRV_PCM_INFO_PAUSE |
        SNDRV_PCM_INFO_RESUME |
        SNDRV_PCM_INFO_MMAP_VALID |
        SNDRV_PCM_INFO_INTERLEAVED |
        SNDRV_PCM_INFO_BLOCK_TRANSFER,
    .formats = ASOC_AIC_FORMATS,
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

struct aic_dma_data {
    enum audio_dev_id aic_dev;
    enum audio_dev_id dma_dev;
    struct audio_dma_desc *dma_desc;
    struct snd_pcm_substream *substream;
    unsigned long dma_addr;
    void *dma_buffer;
    void *real_dma_buffer;
    unsigned int buf_size;
    int period_us;
    struct task_struct *thread;
    volatile unsigned int thread_stop;
    volatile unsigned int thread_is_stop;
    volatile unsigned int start_capture;
    wait_queue_head_t thread_stop_wq;
    int dma_pos;
    int is_mute;
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

static int aic_dma_pcm_notice_thread(void *data_)
{
    struct aic_dma_data *data = (struct aic_dma_data *)data_;
    unsigned int period_time_us = data->period_us;
    int dma_buffer_size = data->buf_size;
    void *dma_buf = data->real_dma_buffer;
    int is_capture = data->substream->stream == SNDRV_PCM_STREAM_CAPTURE;

    while (1) {
        if (data->thread_stop)
            break;

        if (!data->start_capture) {
            usleep_range(10*1000, 10*1000);
            continue;
        }

        int old_dma_pos = data->dma_pos == dma_buffer_size ? 0 : data->dma_pos;

        int pos;
        int start_addr;

        int cur_addr = audio_dma_get_current_addr(data->dma_dev);
        if (cur_addr >= virt_to_phys(data->dma_buffer) && cur_addr <= virt_to_phys(data->dma_buffer) + dma_buffer_size) {
            start_addr = virt_to_phys(data->dma_buffer);
        } else {
            start_addr = virt_to_phys(data->real_dma_buffer);
        }

        pos = cur_addr - start_addr;

        if (pos) {
            if (!is_capture && !data->is_mute) {
                if (pos > old_dma_pos) {
                    memset(dma_buf + old_dma_pos, 0, pos - old_dma_pos);
                    dma_cache_sync(NULL, dma_buf + old_dma_pos, pos - old_dma_pos, DMA_MEM_TO_DEV);
                } else {
                    int size = dma_buffer_size - old_dma_pos;
                    memset(dma_buf + old_dma_pos, 0, size);
                    dma_cache_sync(NULL, dma_buf + old_dma_pos, size, DMA_MEM_TO_DEV);
                    memset(dma_buf, 0, pos);
                    dma_cache_sync(NULL, dma_buf, pos, DMA_MEM_TO_DEV);
                }
            }

            data->dma_pos = pos;
            snd_pcm_period_elapsed(data->substream);
        }

        usleep_range(period_time_us, period_time_us);
    }

    data->dma_pos = 0;
    snd_pcm_period_elapsed(data->substream);

    data->thread_is_stop = 1;
    wake_up_all(&data->thread_stop_wq);

    return 0;
}

static int aic_dma_pcm_open(struct snd_pcm_substream *substream)
{
    struct snd_pcm_runtime *runtime = substream->runtime;
    struct aic_dma_data *aic_dma;

    int ret = snd_soc_set_runtime_hwparams(substream, &aic_pcm_hardware);
    if (ret) {
        printk(KERN_ERR "aic_dma: snd_soc_set_runtime_hwparams failed: %d\n", ret);
        return ret;
    }

    ret = snd_pcm_hw_constraint_step(runtime, 0, SNDRV_PCM_HW_PARAM_BUFFER_BYTES, BUFFER_ALIGN);
    if (ret) {
        printk(KERN_ERR "aic_dma: align hw_param buffer failed ret = %d\n", ret);
        return ret;
    }

    ret = snd_pcm_hw_constraint_step(runtime, 0, SNDRV_PCM_HW_PARAM_PERIOD_BYTES, BUFFER_ALIGN);
    if (ret) {
        printk(KERN_ERR "aic_dma: align hw_param period failed ret = %d\n", ret);
        return ret;
    }

    ret = snd_pcm_hw_constraint_integer(runtime, SNDRV_PCM_HW_PARAM_PERIODS);
    if (ret < 0) {
        printk(KERN_ERR "aic_dma: snd_pcm_hw_constraint_integer failed: %d\n", ret);
        return ret;
    }

    aic_dma = kmalloc(sizeof(*aic_dma), GFP_KERNEL);

    aic_dma->dma_desc = m_dma_alloc_coherent(sizeof(*aic_dma->dma_desc));

    init_waitqueue_head(&aic_dma->thread_stop_wq);

    aic_dma->substream = substream;

    runtime->private_data = aic_dma;

    return 0;
}

static int aic_dma_pcm_close(struct snd_pcm_substream *substream)
{
    struct snd_pcm_runtime *runtime = substream->runtime;
    struct aic_dma_data *aic_dma = runtime->private_data;

    m_dma_free_coherent(aic_dma->dma_desc, sizeof(*aic_dma->dma_desc));

    kfree(aic_dma);

    return 0;
}

static int aic_dma_pcm_prepare(struct snd_pcm_substream *substream)
{
    return 0;
}

static int aic_dma_mmap(struct snd_pcm_substream *substream, struct vm_area_struct *vma)
{
    return snd_pcm_lib_default_mmap(substream, vma);
}

#ifdef DEBUG
static void aic_playback_cb(void *data)
{
    struct aic_dma_data *aic_dma = data;
    int dma_id = dev_to_dma_id(aic_dma->dma_dev);

    unsigned int dsr = audio_read_reg(DSR(dma_id));

    audio_write_reg(DSR(dma_id), dsr);

    printk(KERN_EMERG "aic_playback_cb: %x\n", dsr);
}
#endif

static struct aic_dma_data *aic_icodec_dev = NULL;

/* 由于使用icodec的mute功能无法达到完全静音的效果，
 * 所以这里需要借助 aic_mute_icodec 来实现。
 * */
void aic_mute_icodec(int is_mute)
{
    void *buffer;

    if (aic_icodec_dev == NULL)
        return;

    if (is_mute) {
        buffer = aic_icodec_dev->dma_buffer;
        aic_icodec_dev->is_mute = 1;
    } else {
        buffer = aic_icodec_dev->real_dma_buffer;
        aic_icodec_dev->is_mute = 0;
    }

    aic_icodec_dev->dma_desc->dma_addr = virt_to_phys(buffer);
    aic_icodec_dev->dma_addr = virt_to_phys(buffer);
}
EXPORT_SYMBOL(aic_mute_icodec);

static int aic_dma_pcm_hw_params(struct snd_pcm_substream *substream,
        struct snd_pcm_hw_params *hw_params)
{
    struct snd_pcm_runtime *runtime = substream->runtime;
    struct aic_dma_data *aic_dma = runtime->private_data;
    struct snd_soc_pcm_runtime *rtd = snd_pcm_substream_chip(substream);
    struct snd_soc_dai *dai = rtd->cpu_dai;
    int id = dai->id;
    int is_capture = substream->stream == SNDRV_PCM_STREAM_CAPTURE;

    int format = params_format(hw_params);
    int channels = params_channels(hw_params);
    int buf_size = params_buffer_bytes(hw_params);
    int period_size = params_period_bytes(hw_params);
    int sample_rate = params_rate(hw_params);
    int fmt_width = snd_pcm_format_width(format);
    int unit_size = 32;

    if (is_capture) {
        aic_dma->aic_dev = Dev_src_baic0 + id;
        aic_dma->dma_dev = audio_requst_tar_dma_dev();
    } else {
        aic_dma->aic_dev = Dev_tar_baic0 + id;
        aic_dma->dma_dev = audio_requst_src_dma_dev();
    }

    if (aic_dma->dma_dev == 0) {
        printk(KERN_ERR "aic_dma: aic%d failed to request dma channel\n", id);
        return -EBUSY;
    }

    int ret = snd_pcm_lib_malloc_pages(substream, buf_size);
    if (ret < 0) {
        printk(KERN_ERR "aic_dma: aic%d failed to alloc pages： %d\n", id,   ret);
        audio_release_dma_dev(aic_dma->dma_dev);
        snd_pcm_lib_free_pages(substream);
        return ret;
    }

    int frame_size = params_physical_width(hw_params) * channels / 8;
    int period_ms = 1000 * period_size / sample_rate / frame_size;
    if (!period_ms)
        period_ms = 1;
    aic_dma->period_us = period_ms * 1000;
    aic_dma->thread_stop = 0;
    aic_dma->thread_is_stop = 0;
    aic_dma->start_capture = 0;

    void *dma_buffer = (void *)CKSEG0ADDR(substream->dma_buffer.area);

    audio_dma_desc_init(aic_dma->dma_desc, dma_buffer, buf_size, unit_size, aic_dma->dma_desc);

    /* id = 0 表示使用 icodec */
    if (id == 0) {
        if (!is_capture) {
            if (aic_icodec_dev)
                m_dma_free_coherent(aic_dma->dma_buffer, aic_dma->buf_size);
            aic_dma->dma_buffer = m_dma_alloc_coherent(buf_size);
            aic_icodec_dev = aic_dma;
        }
    }

    aic_dma->real_dma_buffer = dma_buffer;
    aic_dma->dma_addr = virt_to_phys(dma_buffer);
    aic_dma->buf_size = buf_size;
    aic_dma->dma_pos = 0;
    aic_dma->is_mute = 0;

    if (is_capture)
        audio_connect_dev(aic_dma->aic_dev, aic_dma->dma_dev);
    else
        audio_connect_dev(aic_dma->dma_dev, aic_dma->aic_dev);

#ifdef DEBUG
    audio_dma_set_callback(aic_dma->dma_dev, aic_playback_cb, aic_dma);
#else
    audio_dma_set_callback(aic_dma->dma_dev, NULL, NULL);
#endif

    audio_dma_config(aic_dma->dma_dev, channels, fmt_width, unit_size, format);

    aic_dma->thread = kthread_create(aic_dma_pcm_notice_thread, aic_dma, "aic_dma_notice");
    wake_up_process(aic_dma->thread);

    return 0;
}

static int aic_dma_pcm_hw_free(struct snd_pcm_substream *substream)
{
    struct snd_pcm_runtime *runtime = substream->runtime;
    struct aic_dma_data *aic_dma = runtime->private_data;
    struct snd_soc_pcm_runtime *rtd = snd_pcm_substream_chip(substream);
    struct snd_soc_dai *dai = rtd->cpu_dai;
    int id = dai->id;
    int is_capture = substream->stream == SNDRV_PCM_STREAM_CAPTURE;

    aic_dma->thread_stop = 1;

    wait_event_timeout(aic_dma->thread_stop_wq, aic_dma->thread_is_stop, HZ);

    if (is_capture)
        audio_disconnect_dev(aic_dma->aic_dev, aic_dma->dma_dev);
    else
        audio_disconnect_dev(aic_dma->dma_dev, aic_dma->aic_dev);

    audio_release_dma_dev(aic_dma->dma_dev);

    snd_pcm_lib_free_pages(substream);

    if (id == 0)
        if (!is_capture) {
            m_dma_free_coherent(aic_dma->dma_buffer, aic_dma->buf_size);
            aic_icodec_dev = NULL;
        }

    return 0;
}

static int aic_dma_trigger(struct snd_pcm_substream *substream, int cmd)
{
    struct snd_pcm_runtime *runtime = substream->runtime;
    struct aic_dma_data *aic_dma = runtime->private_data;
    int ret = 0;

    switch (cmd) {
    case SNDRV_PCM_TRIGGER_START:
    case SNDRV_PCM_TRIGGER_RESUME:
    case SNDRV_PCM_TRIGGER_PAUSE_RELEASE:
        audio_dma_start(aic_dma->dma_dev, aic_dma->dma_desc);
        aic_dma->start_capture = 1;
        break;
    case SNDRV_PCM_TRIGGER_STOP:
    case SNDRV_PCM_TRIGGER_SUSPEND:
    case SNDRV_PCM_TRIGGER_PAUSE_PUSH:
        audio_dma_stop(aic_dma->dma_dev);
        aic_dma->start_capture = 0;
        break;
    default:
        ret = -EINVAL;
        break;
    }

    return ret;
}

static int debug = 0;
module_param(debug, int, 0644);

static snd_pcm_uframes_t aic_dma_pcm_pointer(struct snd_pcm_substream *substream)
{
    int pos;
    struct snd_pcm_runtime *runtime = substream->runtime;
    struct aic_dma_data *aic_dma = runtime->private_data;

    pos = aic_dma->dma_pos;
    if (debug)
        printk(KERN_EMERG "pos: %x\n", pos);

    if (pos >= aic_dma->buf_size)
        pos = 0;

    return bytes_to_frames(substream->runtime, pos);
}

struct snd_pcm_ops aic_dma_pcm_ops = {
    .open       = aic_dma_pcm_open,
    .close      = aic_dma_pcm_close,
    .prepare    = aic_dma_pcm_prepare,
    .ioctl      = snd_pcm_lib_ioctl,
    .hw_params  = aic_dma_pcm_hw_params,
    .hw_free    = aic_dma_pcm_hw_free,
    .pointer    = aic_dma_pcm_pointer,
    .trigger    = aic_dma_trigger,
    .mmap       = aic_dma_mmap,
};

static int aic_dma_pcm_new(struct snd_soc_pcm_runtime *rtd)
{
    struct snd_pcm *pcm = rtd->pcm;

    int ret = snd_pcm_lib_preallocate_pages_for_all(
        pcm, SNDRV_DMA_TYPE_DEV, rtd->dev, MAX_DMA_BUFFERSIZE, MAX_DMA_BUFFERSIZE);
    if (ret)
        panic("aic_dma: failed to pre allocate pages\n");

    return 0;
}

static void aic_dma_pcm_free(struct snd_pcm *pcm)
{
    snd_pcm_lib_preallocate_free_for_all(pcm);
}

static struct snd_soc_platform_driver pcm_platform_driver = {
    .ops        = &aic_dma_pcm_ops,
    .pcm_new    = aic_dma_pcm_new,
    .pcm_free   = aic_dma_pcm_free,
};

int aic_dma_driver_init(struct platform_device *pdev)
{
    return snd_soc_register_platform(&pdev->dev, &pcm_platform_driver);
}

void aic_dma_driver_exit(struct platform_device *pdev)
{
    snd_soc_unregister_platform(&pdev->dev);
}
