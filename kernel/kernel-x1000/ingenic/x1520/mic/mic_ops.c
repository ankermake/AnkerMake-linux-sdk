#include "mic.h"

static int buffer_total_len = MIC_BUFFER_TOTAL_LEN;

void mic_set_buffer_time_ms(int buffer_time_ms)
{
    buffer_total_len = (buffer_time_ms * 16) * 20;
}

static int mic_alloc_sub_mic_buf(struct mic_dev *mic_dev, int mic_type) {
    struct mic *mic = (mic_type == DMIC) ? &mic_dev->dmic : &mic_dev->amic;

    if (mic_type == DMIC) {
        mic->frame_size = sizeof(short) * 4;        // 4channles * sizeof(short) * (16000 / 16000)
    }
    else {
        mic->frame_size = sizeof(int) * 3;          // 1channles * sizeof(int) * (48000 / 16000)
    }

    int total_size = buffer_total_len * mic->frame_size /
            (sizeof(short) * 4 + sizeof(int) * 3);
    int i;

    mic->buf_len = mic_dev->raws_pre_period * mic->frame_size;

    mic->buf_cnt = total_size / mic->buf_len;
    mic->buf_cnt = mic->buf_cnt < 2 ? 2 : mic->buf_cnt;

    if (mic->buf != NULL) {
        kfree(mic->buf[0]);
        kfree(mic->buf);
        mic->buf = NULL;
    }

    mic->buf = (char **)kmalloc(mic->buf_cnt * sizeof(char *), GFP_KERNEL);
    if (!mic->buf) {
        dev_err(mic_dev->dev, "%s: %d: failed to malloc buf!!\n",
                __func__, __LINE__);
        return -ENOMEM;
    }

    mic->buf[0] = kmalloc(mic->buf_len * mic->buf_cnt, GFP_KERNEL);
    if (!mic->buf[0]) {
        dev_err(mic_dev->dev, "%s: %d: failed to malloc buf!!\n",
                __func__, __LINE__);
        return -ENOMEM;
    }

    for (i = 1; i < mic->buf_cnt; i++) {
        mic->buf[i] = mic->buf[i-1] + mic->buf_len;
    }

    memset(mic->buf[0], 0, mic->buf_len * mic->buf_cnt);

    return 0;
}

static void dmic_hal_enable_record(struct mic_dev *mic_dev) {
    dmic_enable(mic_dev);
    mic_dma_submit_cyclic(mic_dev, &mic_dev->dmic);

    mic_hrtimer_start(mic_dev);
}

static void amic_submit_dma(void *data)
{
    struct mic_dev *mic_dev = data;
    mic_dma_submit_cyclic(mic_dev, &mic_dev->amic);
}

static void amic_hal_enable_record(struct mic_dev *mic_dev) {
    i2s_enable_capture_dma(amic_submit_dma, mic_dev);

    mic_hrtimer_start(mic_dev);
}

static void dmic_hal_disable_record(struct mic_dev *mic_dev) {
    mic_hrtimer_stop(mic_dev);

    mic_dma_terminate(&mic_dev->dmic);
    dmic_disable(mic_dev);
}

static void amic_hal_disable_record(struct mic_dev *mic_dev) {
    mic_hrtimer_stop(mic_dev);

    mic_dma_terminate(&mic_dev->amic);

    i2s_disable_capture_dma();
}

int mic_set_periods_ms_force(struct mic_dev *mic_dev, int periods_ms) {
    int ret;
    struct mic *amic = &mic_dev->amic;

    printk("entry: %s\n", __func__);

    if (periods_ms < 20) {
        periods_ms = 20;
    } else if (periods_ms > 1000) {
        periods_ms = 1000;
    }

    mutex_lock(&mic_dev->buf_lock);

    mic_dev->is_stoped = 1;

    if (amic->is_enabled)
        amic_hal_disable_record(mic_dev);

    mic_dev->periods_ms = periods_ms;
    mic_dev->raws_pre_period = 16000 * mic_dev->periods_ms / MSEC_PER_SEC;


    ret = mic_alloc_sub_mic_buf(mic_dev, AMIC);
    if (ret < 0) {
        goto err_alloc_amic;
    }

    mic_dev->is_stoped = 0;
    if (amic->is_enabled)
        amic_hal_enable_record(mic_dev);

    mutex_unlock(&mic_dev->buf_lock);

    return 0;

err_alloc_amic:


    return ret;
}

int mic_set_periods_ms(struct mic_file_data *fdata, struct mic_dev *mic_dev, int periods_ms) {
    int min_period = periods_ms;
    struct mic_file_data *tmp;

    printk("entry: %s\n", __func__);

    if (fdata) {
        fdata->periods_ms = periods_ms;
        if (!(fdata->dmic_enabled
                || fdata->amic_enabled)) {
            return 0;
        }
    }

    spin_lock(&mic_dev->list_lock);

    list_for_each_entry(tmp, &mic_dev->filp_data_list, entry) {
        if (tmp->dmic_enabled || tmp->amic_enabled)
            min_period = min(min_period, tmp->periods_ms);
    }


    spin_unlock(&mic_dev->list_lock);

    if (mic_dev->periods_ms == min_period || min_period == INT_MAX)
        return 0;

    return mic_set_periods_ms_force(mic_dev, min_period);
}

static void mic_check_open_type(struct mic_dev *mic_dev) {
    struct mic_file_data *tmp;

    spin_lock(&mic_dev->list_lock);

    mic_dev->open_type = NORMAL;

    list_for_each_entry(tmp, &mic_dev->filp_data_list, entry) {
        if (tmp->open_type == ALGORITHM
                && (tmp->dmic_enabled || tmp->amic_enabled)) {
            mic_dev->open_type = ALGORITHM;
            break;
        }
    }

    spin_unlock(&mic_dev->list_lock);
}

int dmic_enable_record(struct mic_file_data *fdata, struct mic_dev *mic_dev) {
#if 0
    struct mic *dmic = &mic_dev->dmic;
    int is_enable = dmic->is_enabled;

    printk("entry: %s\n", __func__);

    if (!is_enable) {
        dmic->cnt = 0;
        dmic->offset = 0;
    }

    fdata->cnt = dmic->cnt;
    fdata->dmic_enabled = 1;
    fdata->dmic_offset = dmic->offset;

    mic_set_periods_ms(NULL, mic_dev, INT_MAX);

    if (!is_enable)
        dmic_hal_enable_record(mic_dev);

    dmic_set_gain(mic_dev, dmic->gain);
    dmic->is_enabled = 1;

    mic_check_open_type(mic_dev);
#endif
    return 0;
}

int amic_enable_record(struct mic_file_data *fdata, struct mic_dev *mic_dev) {
    struct mic *amic = &mic_dev->amic;
    int is_enable = amic->is_enabled;

    printk("entry: %s\n", __func__);

    if (!is_enable) {
        amic->cnt = 0;
        amic->offset = 0;
    }

    fdata->cnt = amic->cnt;
    fdata->amic_enabled = 1;
    fdata->amic_offset = amic->offset;

    mic_set_periods_ms(NULL, mic_dev, INT_MAX);

    if (!is_enable) {
        amic_hal_enable_record(mic_dev);
    }

    amic_set_gain(mic_dev, amic->gain);

    mic_check_open_type(mic_dev);
    amic->is_enabled = 1;

    return 0;
}

int dmic_disable_record(struct mic_file_data *fdata, struct mic_dev *mic_dev) {
#if 0
    struct mic *dmic = &mic_dev->dmic;
    struct mic_file_data *tmp;
    int need_enabled = 0;
    printk("entry: %s\n", __func__);

    spin_lock(&mic_dev->list_lock);

    fdata->dmic_enabled = 0;
    wake_up_all(&mic_dev->wait_queue);

    mic_dev->open_type = NORMAL;

    list_for_each_entry(tmp, &mic_dev->filp_data_list, entry) {
        if (tmp->dmic_enabled) {
            need_enabled = 1;
            break;
        }
    }

    spin_unlock(&mic_dev->list_lock);

    mic_set_periods_ms(NULL, mic_dev, INT_MAX);

    if (dmic->is_enabled && !need_enabled) {
        dmic->is_enabled = 0;
        dmic_hal_disable_record(mic_dev);
        memset(dmic->buf[0], 0, dmic->buf_cnt * dmic->buf_len);
    }

    mic_check_open_type(mic_dev);
#endif
    return 0;
}

int amic_disable_record(struct mic_file_data *fdata, struct mic_dev *mic_dev) {
    struct mic *amic = &mic_dev->amic;
    struct mic_file_data *tmp;
    int need_enabled = 0;
    printk("entry: %s\n", __func__);

    spin_lock(&mic_dev->list_lock);

    fdata->amic_enabled = 0;
    wake_up_all(&mic_dev->wait_queue);

    list_for_each_entry(tmp, &mic_dev->filp_data_list, entry) {
        if (tmp->amic_enabled) {
            need_enabled = 1;
            break;
        }
    }


    spin_unlock(&mic_dev->list_lock);

    mic_set_periods_ms(NULL, mic_dev, INT_MAX);


    if (amic->is_enabled && !need_enabled) {
        amic->is_enabled = 0;
        amic_hal_disable_record(mic_dev);
        memset(amic->buf[0], 0, amic->buf_cnt * amic->buf_len);
    }

    mic_check_open_type(mic_dev);

    return 0;
}
