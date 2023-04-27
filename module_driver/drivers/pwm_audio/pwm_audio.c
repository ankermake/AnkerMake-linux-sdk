#include <linux/init.h>
#include <linux/err.h>
#include <linux/platform_device.h>
#include <linux/module.h>
#include <linux/vmalloc.h>
#include <linux/string.h>
#include <linux/kernel.h>
#include <linux/slab.h>
#include <linux/kthread.h>
#include <sound/core.h>
#include <sound/control.h>
#include <sound/pcm.h>

#include <soc/gpio.h>
#include <linux/gpio.h>
#include <utils/gpio.h>

#include <pwm.h>
#include "pwm_audio.h"

#define PWM_DUTY_MAX_COUNT   (0xFFFF)

#define BUFF_SIZE_MAX    (PAGE_SIZE * 16)
#define PRD_SIZE_MAX    PAGE_SIZE
#define MIN_PERIODS    2

struct snd_pwm_audio_chip {
    struct snd_card *card;
    struct snd_pcm *pcm;

    struct snd_pcm_substream *substream;

    bool is_stream_running;
    unsigned int alsa_buf_pos;
    struct task_struct *playback_task;
    struct task_struct *data_processing_task;
    wait_queue_head_t playback_waitq;
    wait_queue_head_t data_waitq;
    wait_queue_head_t data_processing_waitq;

    bool amp_power_enable;
    struct work_struct amp_power_work;
    struct workqueue_struct *amp_power_workqueue;

    unsigned int write_pos;
    unsigned int read_pos;

    int pwm_id;
    struct pwm_audio_pdata *pdata;
    unsigned int buf_size;

    unsigned int pwm_full_num;
    struct pwm_data *pwm_data;
    struct pwm_data *pwm_mute_data;
};

static int playback_thread(void *data)
{
    struct snd_pwm_audio_chip *pwm_audio = data;
    struct pwm_dma_data pwm_dma_data;

    pwm_dma_data.id = pwm_audio->pwm_id;
    pwm_dma_data.data_count = pwm_audio->buf_size;
    pwm_dma_data.dma_loop = 0;

    while (!kthread_should_stop()) {
        wait_event_interruptible(
            pwm_audio->data_waitq,
            kthread_should_stop() ||
            pwm_audio->write_pos != pwm_audio->read_pos);

        if (pwm_audio->write_pos == pwm_audio->read_pos)
            break;

        pwm_dma_data.data = pwm_audio->pwm_data + (pwm_audio->buf_size * (pwm_audio->read_pos % pwm_audio->pdata->buf_count));
        pwm2_dma_update(pwm_dma_data.id, &pwm_dma_data);
        pwm_audio->read_pos++;
        wake_up_interruptible(&pwm_audio->data_processing_waitq);
    }

    return 0;
}

static int data_processing_thread(void *data)
{
    int i, j;
    short diff;
    const short *audio_data;
    unsigned int data_mul;
    struct pwm_data *pwm_data;
    struct snd_pcm_runtime *runtime;
    struct snd_pcm_substream *substream;
    struct snd_pwm_audio_chip *pwm_audio = data;

    while (!kthread_should_stop()) {
        wait_event_interruptible(
            pwm_audio->playback_waitq,
            kthread_should_stop() ||
            pwm_audio->is_stream_running);

        if (!pwm_audio->is_stream_running)
            break;

        wait_event_interruptible(
            pwm_audio->data_processing_waitq,
            kthread_should_stop() ||
            (pwm_audio->pdata->buf_count - (pwm_audio->write_pos - pwm_audio->read_pos)));

        if ((pwm_audio->pdata->buf_count - (pwm_audio->write_pos - pwm_audio->read_pos)) == 0)
            break;

        substream = pwm_audio->substream;

        snd_pcm_stream_lock(substream);

        runtime = substream->runtime;
        if (!runtime || !snd_pcm_running(substream)) {
            snd_pcm_stream_unlock(substream);
            continue;
        }

        data_mul = pwm_audio->pdata->base_freq / runtime->rate;
        pwm_data = pwm_audio->pwm_data + (pwm_audio->buf_size * (pwm_audio->write_pos % pwm_audio->pdata->buf_count));

        for (i = 0; i < pwm_audio->buf_size; i += data_mul) {
            audio_data = (const short *)(runtime->dma_area + pwm_audio->alsa_buf_pos);
            diff = audio_data[0] * pwm_audio->pwm_full_num / PWM_DUTY_MAX_COUNT;
            pwm_data[i].high = pwm_audio->pwm_full_num / 2 + diff;
            if (pwm_data[i].high >= pwm_audio->pwm_full_num)
                pwm_data[i].high = pwm_audio->pwm_full_num - 1;
            if (pwm_data[i].high == 0)
                pwm_data[i].high = 1;
            pwm_data[i].low = pwm_audio->pwm_full_num - pwm_data[i].high;

            for (j = 1; j < data_mul; j++) {
                pwm_data[i + j] = pwm_data[i];
            }

            pwm_audio->alsa_buf_pos += 2;
            if (pwm_audio->alsa_buf_pos >= runtime->dma_bytes) {
                pwm_audio->alsa_buf_pos = 0;
            }
        }

        snd_pcm_stream_unlock(substream);

        if ((pwm_audio->alsa_buf_pos % snd_pcm_lib_period_bytes(substream)) < (pwm_audio->buf_size / data_mul))
            snd_pcm_period_elapsed(substream);

        pwm_audio->write_pos++;
        wake_up_interruptible(&pwm_audio->data_waitq);
    }

    return 0;
}

static int pwm_audio_pcm_open(struct snd_pcm_substream *substream)
{
    struct snd_pwm_audio_chip *pwm_audio = substream->private_data;
    struct snd_pcm_runtime *runtime = substream->runtime;

    runtime->hw.info = SNDRV_PCM_INFO_INTERLEAVED | SNDRV_PCM_INFO_BLOCK_TRANSFER
         | SNDRV_PCM_INFO_MMAP | SNDRV_PCM_INFO_MMAP_VALID;
    runtime->hw.formats = SNDRV_PCM_FMTBIT_S16_LE;

    runtime->hw.rates = SNDRV_PCM_RATE_8000 | SNDRV_PCM_RATE_16000;
    runtime->hw.rate_min = 8000;

    if (pwm_audio->pdata->base_freq == 32000) {
        runtime->hw.rates |= SNDRV_PCM_RATE_32000;
        runtime->hw.rate_max = 32000;
    } else if (pwm_audio->pdata->base_freq == 48000) {
        runtime->hw.rates |= SNDRV_PCM_RATE_48000;
        runtime->hw.rate_max = 48000;
    } else if (pwm_audio->pdata->base_freq == 96000) {
        runtime->hw.rates |= SNDRV_PCM_RATE_48000 | SNDRV_PCM_RATE_96000;
        runtime->hw.rate_max = 96000;
    } else {
        return -EINVAL;
    }

    runtime->hw.channels_min = 1;
    runtime->hw.channels_max = 1;
    runtime->hw.buffer_bytes_max = BUFF_SIZE_MAX,
    runtime->hw.period_bytes_min = 192;
    runtime->hw.period_bytes_max = PRD_SIZE_MAX,
    runtime->hw.periods_min = MIN_PERIODS,
    runtime->hw.periods_max = BUFF_SIZE_MAX / PRD_SIZE_MAX,
    runtime->hw.fifo_size = 0;

    snd_pcm_hw_constraint_integer(runtime, SNDRV_PCM_HW_PARAM_PERIODS);

    pwm_audio->substream = substream;
    pwm_audio->data_processing_task = kthread_run(data_processing_thread, pwm_audio, "pwm_audio_data");
    if (IS_ERR(pwm_audio->data_processing_task)) {
        printk(KERN_ERR "%s: Couldn't start thread\n", __func__);
        return PTR_ERR(pwm_audio->data_processing_task);
    }

    pwm_audio->playback_task = kthread_run(playback_thread, pwm_audio, "pwm_audio_data");
    if (IS_ERR(pwm_audio->playback_task)) {
        printk(KERN_ERR "%s: Couldn't start thread\n", __func__);
        kthread_stop(pwm_audio->data_processing_task);
        return PTR_ERR(pwm_audio->playback_task);
    }
    return 0;
}

int pwm_audio_pcm_close(struct snd_pcm_substream *substream)
{
    struct snd_pwm_audio_chip *pwm_audio = substream->private_data;
    pwm_audio->is_stream_running = false;
    kthread_stop(pwm_audio->data_processing_task);
    kthread_stop(pwm_audio->playback_task);
    wake_up_interruptible(&pwm_audio->playback_waitq);

    return 0;
}

static void amp_power_work_handler(struct work_struct *p_work)
{
    int i;
    struct pwm_dma_data pwm_dma_data;
    struct snd_pwm_audio_chip *pwm_audio = container_of(p_work, struct snd_pwm_audio_chip, amp_power_work);

    pwm_dma_data.id = pwm_audio->pwm_id;
    pwm_dma_data.data = pwm_audio->pwm_mute_data;
    pwm_dma_data.data_count = pwm_audio->buf_size;
    pwm_dma_data.dma_loop = 0;

    if (pwm_audio->amp_power_enable) {
        for (i = 0; i < pwm_audio->pdata->amp_mute_up; i++)
            pwm2_dma_update(pwm_dma_data.id, &pwm_dma_data);
        gpio_set_value(pwm_audio->pdata->amp_power_gpio, 1);

        pwm_audio->is_stream_running = true;
        wake_up_interruptible(&pwm_audio->playback_waitq);
    } else {
        gpio_set_value(pwm_audio->pdata->amp_power_gpio, 0);
        for (i = 0; i < pwm_audio->pdata->amp_mute_down; i++)
            pwm2_dma_update(pwm_dma_data.id, &pwm_dma_data);
    }
}

static int pwm_audio_pcm_trigger(struct snd_pcm_substream *substream, int cmd)
{
    struct snd_pwm_audio_chip *pwm_audio = substream->private_data;
    int err = 0;

    switch (cmd) {
    case SNDRV_PCM_TRIGGER_START:
        pwm_audio->amp_power_enable = true;
        if (gpio_is_valid(pwm_audio->pdata->amp_power_gpio)) {
            queue_work(pwm_audio->amp_power_workqueue, &pwm_audio->amp_power_work);
        } else {
            pwm_audio->is_stream_running = true;
            wake_up_interruptible(&pwm_audio->playback_waitq);
        }
        break;
    case SNDRV_PCM_TRIGGER_STOP:
        pwm_audio->amp_power_enable = false;
        pwm_audio->is_stream_running = false;
        if (gpio_is_valid(pwm_audio->pdata->amp_power_gpio))
            queue_work(pwm_audio->amp_power_workqueue, &pwm_audio->amp_power_work);
        break;
    default:
        err = -EINVAL;
    }

    return err;
}

static snd_pcm_uframes_t pwm_audio_pcm_pointer(struct snd_pcm_substream *substream)
{
    struct snd_pwm_audio_chip *pwm_audio = substream->private_data;

    return bytes_to_frames(substream->runtime, pwm_audio->alsa_buf_pos);
}

static int pwm_audio_pcm_null(struct snd_pcm_substream *substream)
{
    return 0;
}

static int pwm_audio_pcm_hw_params(struct snd_pcm_substream *substream,
                struct snd_pcm_hw_params *hw_params)
{
    int ret = snd_pcm_lib_malloc_pages(substream, params_buffer_bytes(hw_params));
    if (ret < 0) {
        printk(KERN_EMERG "failed to allocate pages: %d\n", ret);
        return ret;
    }

    return 0;
}

static int pwm_audio_pcm_hw_free(struct snd_pcm_substream *substream)
{
    return snd_pcm_lib_free_pages(substream);
}

static const struct snd_pcm_ops pwm_audio_pcm_ops = {
    .open = pwm_audio_pcm_open,
    .close = pwm_audio_pcm_close,
    .trigger = pwm_audio_pcm_trigger,
    .pointer = pwm_audio_pcm_pointer,
    .prepare = pwm_audio_pcm_null,
    .ioctl = snd_pcm_lib_ioctl,
    .hw_params = pwm_audio_pcm_hw_params,
    .hw_free = pwm_audio_pcm_hw_free,
};

static int pwm_audio_probe(struct platform_device *pdev)
{
    int i;
    int err;
    int rate;
    struct snd_pcm *pcm;
    struct snd_card *card;
    struct snd_pwm_audio_chip *pwm_audio;
    struct pwm_dma_config dma_config;
    struct pwm_audio_pdata *pdata = dev_get_platdata(&pdev->dev);

    if (!pdata) {
        printk(KERN_ERR "%s: pwm_audio_pdata is NULL\n", __func__);
        return -EINVAL;
    }

    if (pdata->base_freq != 32000 && pdata->base_freq != 48000 && pdata->base_freq != 96000) {
        printk(KERN_ERR "%s: base_freq only support 32000 or 48000 or 96000, but current base_freq is %d\n",
                __func__, pdata->base_freq);
        return -EINVAL;
    }

    err = snd_card_new(&pdev->dev, -1, NULL, THIS_MODULE,
               sizeof(struct snd_pwm_audio_chip), &card);
    if (err < 0) {
        printk(KERN_ERR "%s: snd_card_new fail\n", __func__);
        return err;
    }

    pwm_audio = card->private_data;
    pwm_audio->card = card;
    pwm_audio->pdata = pdata;

    err = snd_pcm_new(card, "pwm_audio_pcm", 0, 1, 0, &pcm);
    if (err < 0) {
        printk(KERN_ERR "%s: snd_pcm_new fail\n", __func__);
        goto err_card_free;
    }

    pwm_audio->pcm = pcm;
    pcm->private_data = pwm_audio;

    strcpy(pcm->name, "pwm_audio_pcm");
    snd_pcm_set_ops(pcm, SNDRV_PCM_STREAM_PLAYBACK, &pwm_audio_pcm_ops);

    snd_pcm_lib_preallocate_pages_for_all(pcm, SNDRV_DMA_TYPE_DEV,
                       NULL, BUFF_SIZE_MAX, BUFF_SIZE_MAX);

    if (gpio_is_valid(pdata->amp_power_gpio)) {
        err = gpio_request_one(pdata->amp_power_gpio, GPIOF_DIR_OUT|GPIOF_INIT_LOW, "pwm_audio_amp_power");
        if (err < 0) {
            printk(KERN_ERR "%s: amp_power_gpio gpio_request fail\n", __func__);
            err = -EBUSY;
            goto err_card_free;
        }

        INIT_WORK(&pwm_audio->amp_power_work, amp_power_work_handler);
        pwm_audio->amp_power_workqueue = create_singlethread_workqueue("amp_power_work");
    }

    pwm_audio->buf_size = pdata->base_freq * pdata->unit_time / 1000;
    pwm_audio->pwm_id = pwm2_request(pdata->pwm_gpio, "pwm_audio");
    if (pwm_audio->pwm_id < 0) {
        printk(KERN_ERR "%s: pwm2_request fail\n", __func__);
        err = -EBUSY;
        goto err_destroy_workqueue;
    }

    dma_config.id = pwm_audio->pwm_id;
    dma_config.idle_level = PWM_idle_low;
    dma_config.start_level = PWM_start_high;
    rate = pwm2_dma_init(pwm_audio->pwm_id, &dma_config);
    if (rate < 0) {
        printk(KERN_ERR "%s: pwm%d pwm2_dma_init fail\n", __func__, pwm_audio->pwm_id);
        err = -EBUSY;
        goto err_pwm_release;
    }

    if (rate % pdata->base_freq) {
        printk(KERN_ERR "%s: pwm%d not support base freq %d, rate %d\n", __func__, pwm_audio->pwm_id, pdata->base_freq, rate);
        err = -EINVAL;
        goto err_pwm_release;
    }

    pwm_audio->pwm_full_num = rate / pdata->base_freq;
    pwm_audio->pwm_data = kmalloc(sizeof(struct pwm_data) * pwm_audio->buf_size * 2, GFP_KERNEL);
    if (!pwm_audio->pwm_data) {
        printk(KERN_ERR "%s: pwm%d malloc pwm buf fail\n", __func__, pwm_audio->pwm_id);
        err = -ENOMEM;
        goto err_pwm_release;
    }

    pwm_audio->pwm_mute_data = kmalloc(sizeof(struct pwm_data) * pwm_audio->buf_size, GFP_KERNEL);
    if (!pwm_audio->pwm_mute_data) {
        printk(KERN_ERR "%s: pwm%d malloc pwm mute buf fail\n", __func__, pwm_audio->pwm_id);
        err = -ENOMEM;
        goto err_free_pwm_data;
    }

    /* init mute data */
    pwm_audio->pwm_mute_data[0].high = pwm_audio->pwm_full_num / 2;
    pwm_audio->pwm_mute_data[0].low = pwm_audio->pwm_full_num - pwm_audio->pwm_mute_data[0].high;
    for (i = 1; i < pwm_audio->buf_size; i++) {
        pwm_audio->pwm_mute_data[i] = pwm_audio->pwm_mute_data[0];
    }

    init_waitqueue_head(&pwm_audio->playback_waitq);
    init_waitqueue_head(&pwm_audio->data_waitq);
    init_waitqueue_head(&pwm_audio->data_processing_waitq);

    strcpy(card->driver, "pwm_audio");
    strcpy(card->shortname, "pwm_audio");
    strcpy(card->longname, "pwm_audio");
    err = snd_card_register(card);
    if (err < 0) {
        printk(KERN_ERR "%s: snd_card_register fail\n", __func__);
        goto err_free_pwm_mute_data;
    }

    platform_set_drvdata(pdev, card);
    return 0;

err_free_pwm_mute_data:
    kfree(pwm_audio->pwm_mute_data);
err_free_pwm_data:
    kfree(pwm_audio->pwm_data);
err_pwm_release:
    pwm2_release(pwm_audio->pwm_id);
err_destroy_workqueue:
    if (gpio_is_valid(pdata->amp_power_gpio)) {
        destroy_workqueue(pwm_audio->amp_power_workqueue);
        gpio_free(pdata->amp_power_gpio);
    }
err_card_free:
    snd_card_free(card);
    return err;
}

static int pwm_audio_remove(struct platform_device *pdev)
{
    struct snd_card *card = platform_get_drvdata(pdev);
    struct snd_pwm_audio_chip *pwm_audio = card->private_data;
    int amp_power_gpio = pwm_audio->pdata->amp_power_gpio;
    int pwm_id = pwm_audio->pwm_id;
    void *pwm_data = pwm_audio->pwm_data;
    void *pwm_mute_data = pwm_audio->pwm_mute_data;
    struct workqueue_struct *amp_power_workqueue = pwm_audio->amp_power_workqueue;

    snd_card_free(card);
    pwm2_release(pwm_id);
    if (gpio_is_valid(amp_power_gpio)) {
        destroy_workqueue(amp_power_workqueue);
        gpio_free(amp_power_gpio);
    }
    kfree(pwm_data);
    kfree(pwm_mute_data);
    return 0;
}

static struct platform_driver pwm_audio_driver = {
    .probe = pwm_audio_probe,
    .remove = pwm_audio_remove,
    .driver = {
        .owner = THIS_MODULE,
        .name = "pwm-audio",
    },
};

/* stop no dev release warning */
static void pwm_audio_dev_release(struct device *dev){}

struct pwm_audio_pdata pwm_audio_pdata_1 = {
    .pwm_gpio = -1,
    .amp_power_gpio = -1,
    .amp_mute_up = 1,
    .amp_mute_down = 10,
    .base_freq = 48000,
    .unit_time = 50,
    .buf_count = 2,
};

struct platform_device pwm_audio_device_1 = {
    .name = "pwm-audio",
    .dev  = {
        .release = pwm_audio_dev_release,
        .platform_data = &pwm_audio_pdata_1,
    },
};

struct pwm_audio_pdata pwm_audio_pdata_2 = {
    .pwm_gpio = -1,
    .amp_power_gpio = -1,
    .amp_mute_up = 1,
    .amp_mute_down = 10,
    .base_freq = 48000,
    .unit_time = 50,
    .buf_count = 2,
};

struct platform_device pwm_audio_device_2 = {
    .name = "pwm-audio",
    .dev  = {
        .release = pwm_audio_dev_release,
        .platform_data = &pwm_audio_pdata_2,
    },
};

module_param_gpio_named(pwm_gpio_1, pwm_audio_pdata_1.pwm_gpio, 0644);
module_param_gpio_named(amp_power_gpio_1, pwm_audio_pdata_1.amp_power_gpio, 0644);

module_param_gpio_named(pwm_gpio_2, pwm_audio_pdata_2.pwm_gpio, 0644);
module_param_gpio_named(amp_power_gpio_2, pwm_audio_pdata_2.amp_power_gpio, 0644);

static int __init pwm_audio_init(void)
{
    if (gpio_is_valid(pwm_audio_pdata_1.pwm_gpio))
        platform_device_register(&pwm_audio_device_1);

    if (gpio_is_valid(pwm_audio_pdata_2.pwm_gpio))
        platform_device_register(&pwm_audio_device_2);

    return platform_driver_register(&pwm_audio_driver);
}
module_init(pwm_audio_init);

static void __exit pwm_audio_exit(void)
{
    if (gpio_is_valid(pwm_audio_pdata_1.pwm_gpio))
        platform_device_unregister(&pwm_audio_device_1);

    if (gpio_is_valid(pwm_audio_pdata_2.pwm_gpio))
        platform_device_unregister(&pwm_audio_device_2);

    platform_driver_unregister(&pwm_audio_driver);
}
module_exit(pwm_audio_exit);

MODULE_DESCRIPTION("pwm audio driver");
MODULE_LICENSE("GPL");