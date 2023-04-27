#define BUFFER_ALIGN 32
#define PERIOD_BYTES_MIN 1024
#define PERIODS_MIN 3
#define PERIODS_MAX 128
#define MAX_DMA_BUFFERSIZE (PERIODS_MAX*PERIOD_BYTES_MIN)

#define State_idle 0
#define State_running 1
#define State_request_stop 2

struct dmic_dma_data {
    enum audio_dev_id dma_dev;
    struct audio_dma_desc *dma_desc;
    struct snd_pcm_substream *substream;
    unsigned long dma_addr;
    void *dma_buffer;
    void *dst_buffer;
    unsigned int buf_size;
    unsigned int dma_buf_size;
    int period_us;

    struct task_struct *thread;
    unsigned int thread_stop;
    unsigned int thread_is_stop;
    unsigned int start_capture;
    int read_pos;
    unsigned int dma_pos;
    unsigned int rw_pos;
    unsigned int data_size;

    struct work_struct start_work;
    struct work_struct stop_work;
    struct workqueue_struct *workqueue;
    wait_queue_head_t wq;
    wait_queue_head_t thread_stop_wq;

    unsigned int working_flag;
};

static const struct snd_pcm_hardware dmic_pcm_hardware = {
    .info   = SNDRV_PCM_INFO_INTERLEAVED |
            SNDRV_PCM_INFO_PAUSE |
            SNDRV_PCM_INFO_RESUME |
            SNDRV_PCM_INFO_MMAP |
            SNDRV_PCM_INFO_MMAP_VALID |
            SNDRV_PCM_INFO_BATCH,
    .formats = SNDRV_PCM_FMTBIT_S16_LE | SNDRV_PCM_FMTBIT_S24_LE,
    .buffer_bytes_max = MAX_DMA_BUFFERSIZE,
    .period_bytes_min   = PERIOD_BYTES_MIN,
    .period_bytes_max   = PERIOD_BYTES_MIN * 3,
    .periods_min        = PERIODS_MIN,
    .periods_max        = PERIODS_MAX,
    .fifo_size      = 0,
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

static void copy_to_user_channels_16(void *dst_, void *src_, int bytes)
{
    short *dst = dst_;
    short *src = src_;
    int i,j;
    int dma_channels = dmic_dev.dma_channels;
    int user_channels = dmic_dev.channels;
    int samples = bytes / dmic_dev.dma_frame_size;

    for (i = 0; i < user_channels; i++) {
        int data_offset = dmic_dev.data_offset[i];
        for (j = 0; j < samples; j++) {
            dst[j * user_channels + i] = src[data_offset + dma_channels * j];
        }
    }
}

static void copy_to_user_channels_24(void *dst_, void *src_, int bytes)
{
    int *dst = dst_;
    int *src = src_;
    int i,j;
    int dma_channels = dmic_dev.dma_channels;
    int user_channels = dmic_dev.channels;
    int samples = bytes / dmic_dev.dma_frame_size;

    for (i = 0; i < user_channels; i++) {
        int data_offset = dmic_dev.data_offset[i];
        for (j = 0; j < samples; j++) {
            dst[j * user_channels + i] = src[data_offset + dma_channels * j];
        }
    }
}

static void do_memcpy(void *dst, void *src, int bytes)
{
    dma_cache_sync(NULL, src, bytes, DMA_DEV_TO_MEM);
    if (dmic_dev.fmt_width == 16)
        copy_to_user_channels_16(dst, src, bytes);
    else
        copy_to_user_channels_24(dst, src, bytes);
}

static void dmic_dma_do_pcm_copy(struct dmic_dma_data *data, void *mem, int bytes)
{
    unsigned int buffer_size = data->dma_buf_size;
    unsigned int pos = data->rw_pos;
    void *src = data->dma_buffer;
    int dma_bytes;

    dma_bytes = bytes * dmic_dev.dma_frame_size / dmic_dev.frame_size;

    if (pos + dma_bytes <= buffer_size) {
        do_memcpy(mem, src+pos, dma_bytes);
    } else {
        unsigned int size1 = buffer_size - pos;
        unsigned int mem_pos = size1 * dmic_dev.frame_size / dmic_dev.dma_frame_size;
        do_memcpy(mem, src+pos, size1);
        do_memcpy(mem+mem_pos, src, dma_bytes-size1);
    }

    dma_cache_sync(NULL, mem, bytes, DMA_MEM_TO_DEV);

    data->rw_pos = add_pos(buffer_size, pos, dma_bytes);
    data->data_size -= dma_bytes;
}

static dma_addr_t get_dma_addr(unsigned long start, unsigned int len)
{
    dma_addr_t cur_addr;

    cur_addr = audio_read_reg(DBA(5));
    if (cur_addr >= start && cur_addr < start + len)
        return cur_addr;
    else
        return 0;
}

static unsigned int dmic_get_readable_size(struct dmic_dma_data *data)
{
    dma_addr_t cur_addr;
    unsigned int pos, size, unit_size;
    unsigned int buffer_size = data->dma_buf_size;
    int buffer_addr = virt_to_phys(data->dma_buffer);
    int ret;
    int samples;

    cur_addr = get_dma_addr(buffer_addr, buffer_size);
    if (!cur_addr)
        return 0;

    pos = cur_addr - buffer_addr;
    size = sub_pos(buffer_size, pos, data->dma_pos);
    unit_size = 32;

    data->dma_pos = pos;
    data->data_size += size;

    if (data->data_size > (buffer_size - unit_size)) {
        unsigned int align_pos = ALIGN(pos, unit_size);
        data->rw_pos = add_pos(buffer_size, align_pos, unit_size);
        data->data_size = buffer_size - sub_pos(buffer_size, data->rw_pos, pos);
    }

    ret = data->data_size > unit_size ? data->data_size - unit_size : 0;
    samples = ret / dmic_dev.dma_frame_size;

    ret = samples * dmic_dev.frame_size;

    return ret;
}

static int dmic_dma_read_bytes(struct dmic_dma_data *data, void *mem, int bytes)
{
    unsigned int len = 0;

    while (bytes) {
        unsigned int n = dmic_get_readable_size(data);
        if (!n)
            break;

        if (n > bytes)
            n = bytes;

        dmic_dma_do_pcm_copy(data, mem, n);
        len += n;
        mem += n;
        bytes -= n;
    }

    return len;
}

static int dmic_dma_pcm_copy_thread(void *data_)
{
    struct dmic_dma_data *data = (struct dmic_dma_data *)data_;
    int buffer_size = data->buf_size;
    unsigned int period_time_us = data->period_us;
    int len = 0;
    int n = 0;

    while (1) {
        wait_event(data->wq, !data->working_flag);
        data->working_flag = 1;

        if (data->thread_stop)
            break;

        audio_dma_start(Dev_tar_dma5, data->dma_desc);

        data->read_pos = 0;

        while (1) {
            if (!data->start_capture)
                break;

            len = buffer_size - data->read_pos;

            n = dmic_dma_read_bytes(data, data->dst_buffer + data->read_pos, len);
            if (data->read_pos + n >= buffer_size)
                data->read_pos = 0;
            else
                data->read_pos += n;

            if (n)
                snd_pcm_period_elapsed(data->substream);

            if (n != len)
                usleep_range(period_time_us, period_time_us);
        }

        audio_dma_stop(Dev_tar_dma5);
    }

    data->thread_is_stop = 1;
    wake_up_all(&data->thread_stop_wq);

    return 0;
}

static int dmic_dma_pcm_open(struct snd_pcm_substream *substream)
{
    struct snd_pcm_runtime *runtime = substream->runtime;
    struct dmic_dma_data *dmic_dma;

    int ret = snd_soc_set_runtime_hwparams(substream, &dmic_pcm_hardware);
    if (ret) {
        printk(KERN_ERR "DMIC_DMA: align hw_param buffer failed ret = %d\n", ret);
        return ret;
    }

    ret = snd_pcm_hw_constraint_step(runtime, 0, SNDRV_PCM_HW_PARAM_BUFFER_BYTES, BUFFER_ALIGN);
    if (ret) {
        printk(KERN_ERR "DMIC_DMA: align hw_param buffer failed ret = %d\n", ret);
        return ret;
    }

    ret = snd_pcm_hw_constraint_step(runtime, 0, SNDRV_PCM_HW_PARAM_PERIOD_BYTES, BUFFER_ALIGN);
    if (ret) {
        printk(KERN_ERR "DMIC_DMA: align hw_param period failed ret = %d\n", ret);
        return ret;
    }

    ret = snd_pcm_hw_constraint_integer(runtime, SNDRV_PCM_HW_PARAM_PERIODS);
    if (ret < 0) {
        printk(KERN_ERR "DMIC_DMA: snd_pcm_hw_constraint_integer failed: %d\n", ret);
        return ret;
    }

    dmic_dma = kmalloc(sizeof(*dmic_dma), GFP_KERNEL);

    dmic_dma->dma_desc = m_dma_alloc_coherent(sizeof(*dmic_dma->dma_desc));

    dmic_dma->dma_buffer = dmic_dev.dma_buffer;

    init_waitqueue_head(&dmic_dma->wq);
    init_waitqueue_head(&dmic_dma->thread_stop_wq);

    dmic_dma->substream = substream;

    runtime->private_data = dmic_dma;

    return 0;
}

static void dmic_dma_wake_up(struct dmic_dma_data *data)
{
    data->working_flag = 0;

    wake_up(&data->wq);
}

static int dmic_dma_pcm_close(struct snd_pcm_substream *substream)
{
    struct snd_pcm_runtime *runtime = substream->runtime;
    struct dmic_dma_data *dmic_dma = runtime->private_data;

    m_dma_free_coherent(dmic_dma->dma_desc, sizeof(*dmic_dma->dma_desc));

    kfree(dmic_dma);

    return 0;
}

static int dmic_dma_pcm_hw_params(struct snd_pcm_substream *substream,
                struct snd_pcm_hw_params *hw_params)
{
    struct snd_pcm_runtime *runtime = substream->runtime;
    struct dmic_dma_data *dmic_dma = runtime->private_data;
    int dma_channels, ret;

    int format = params_format(hw_params);
    int channels = params_channels(hw_params);
    int buf_size = params_buffer_bytes(hw_params);
    int period_size = params_period_bytes(hw_params);
    int sample_rate = params_rate(hw_params);
    int fmt_width = snd_pcm_format_width(format);
    int unit_size = 32;

    if (dmic_int0)
        dma_channels = 2;
    if (dmic_int1)
        dma_channels = 4;
    if (dmic_int2)
        dma_channels = 6;
    if (dmic_int3)
        dma_channels = 8;

    ret = dmic_gpio_request();
    if (ret < 0) {
        printk(KERN_ERR "DMIC: gpio_request failed. ret = %d\n", ret);
        return ret;
    }

    ret = snd_pcm_lib_malloc_pages(substream, buf_size);
    if (ret < 0) {
        printk(KERN_ERR "DMIC: snd_pcm_lib_malloc_pages error ret = %d\n", ret);
        return ret;
    }

    dmic_dma->dst_buffer = (unsigned long *)CKSEG0ADDR(substream->dma_buffer.area);

    audio_dma_desc_init(dmic_dma->dma_desc, dmic_dma->dma_buffer, MAX_DMA_BUFFERSIZE, unit_size, dmic_dma->dma_desc);

    int frame_size = params_physical_width(hw_params) * channels / 8;
    int dma_frame_size = params_physical_width(hw_params) * dma_channels / 8;
    int period_ms = 1000 *  period_size / sample_rate / frame_size;

    dmic_dma->dma_dev = Dev_tar_dma5;
    dmic_dma->dma_addr = virt_to_phys(dmic_dma->dma_buffer);
    dmic_dma->period_us = period_ms * 1000;
    dmic_dma->dma_buf_size = MAX_DMA_BUFFERSIZE;
    dmic_dma->buf_size = buf_size;
    dmic_dma->read_pos = 0;
    dmic_dma->dma_pos = 0;
    dmic_dma->rw_pos = 0;
    dmic_dma->data_size = 0;

    dmic_dma->working_flag = 1;
    dmic_dma->thread_stop = 0;
    dmic_dma->thread_is_stop = 0;
    dmic_dma->start_capture = 0;

    dmic_dev.channels = channels;
    dmic_dev.dma_channels = dma_channels;
    dmic_dev.sample_rate = sample_rate;
    dmic_dev.dma_frame_size = dma_frame_size;
    dmic_dev.frame_size = frame_size;
    dmic_dev.fmt_width = fmt_width;

    audio_connect_dev(Dev_src_dmic, Dev_tar_dma5);
    audio_dma_config(Dev_tar_dma5, dma_channels, fmt_width, unit_size, format);

    dmic_dma->thread = kthread_create(dmic_dma_pcm_copy_thread, dmic_dma, "dmic_dma_copy");
    wake_up_process(dmic_dma->thread);

    return 0;
}

static int dmic_dma_pcm_hw_free(struct snd_pcm_substream *substream)
{
    struct snd_pcm_runtime *runtime = substream->runtime;
    struct dmic_dma_data *dmic_dma = runtime->private_data;

    dmic_dma->thread_stop = 1;
    dmic_dma_wake_up(dmic_dma);

    wait_event_timeout(dmic_dma->thread_stop_wq, dmic_dma->thread_is_stop, HZ);

    audio_disconnect_dev(Dev_src_dmic, Dev_tar_dma5);

    return snd_pcm_lib_free_pages(substream);
}

static int dmic_dma_pcm_trigger(struct snd_pcm_substream *substream, int cmd)
{
    struct snd_pcm_runtime *runtime = substream->runtime;
    struct dmic_dma_data *dmic_dma = runtime->private_data;
    int ret = 0;

    switch (cmd) {
    case SNDRV_PCM_TRIGGER_START:
    case SNDRV_PCM_TRIGGER_RESUME:
    case SNDRV_PCM_TRIGGER_PAUSE_RELEASE:
        dmic_dma->start_capture = 1;
        dmic_dma_wake_up(dmic_dma);
        break;
    case SNDRV_PCM_TRIGGER_STOP:
    case SNDRV_PCM_TRIGGER_SUSPEND:
    case SNDRV_PCM_TRIGGER_PAUSE_PUSH:
        dmic_dma->start_capture = 0;
        dmic_dma_wake_up(dmic_dma);
        break;
    default:
        ret = -EINVAL;
        break;
    }

    return ret;
}

static snd_pcm_uframes_t dmic_dma_pcm_pointer(struct snd_pcm_substream *substream)
{
    struct snd_pcm_runtime *runtime = substream->runtime;
    struct dmic_dma_data *dmic_dma = runtime->private_data;

    return bytes_to_frames(substream->runtime, dmic_dma->read_pos);
}

static int dmic_dma_pcm_null(struct snd_pcm_substream *substream)
{
    return 0;
}

static struct snd_pcm_ops dmic_dma_pcm_ops = {
    .open = dmic_dma_pcm_open,
    .close = dmic_dma_pcm_close,
    .ioctl = snd_pcm_lib_ioctl,
    .hw_params = dmic_dma_pcm_hw_params,
    .hw_free = dmic_dma_pcm_hw_free,
    .trigger = dmic_dma_pcm_trigger,
    .pointer = dmic_dma_pcm_pointer,
    .prepare = dmic_dma_pcm_null,
};

static int dmic_dma_pcm_new(struct snd_soc_pcm_runtime *rtd)
{
    struct snd_pcm *pcm = rtd->pcm;

    int ret = snd_pcm_lib_preallocate_pages_for_all(
        pcm, SNDRV_DMA_TYPE_DEV, rtd->dev, MAX_DMA_BUFFERSIZE, MAX_DMA_BUFFERSIZE);
    if (ret)
        panic("DMIC_DMA: failed to pre allocate pages\n");

    return 0;
}

static void dmic_dma_pcm_free(struct snd_pcm *pcm)
{
    snd_pcm_lib_preallocate_free_for_all(pcm);
}

static struct snd_soc_platform_driver pcm_platform_driver = {
    .ops        = &dmic_dma_pcm_ops,
    .pcm_new    = dmic_dma_pcm_new,
    .pcm_free   = dmic_dma_pcm_free,
};

int dmic_dma_driver_init(struct platform_device *pdev)
{
    return snd_soc_register_platform(&pdev->dev, &pcm_platform_driver);
}

void dmic_dma_driver_exit(struct platform_device *pdev)
{
    snd_soc_unregister_platform(&pdev->dev);
}