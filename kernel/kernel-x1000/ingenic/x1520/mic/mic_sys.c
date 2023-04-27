#include <linux/poll.h>
#include "mic.h"

struct mic_dev *m_mic_dev = NULL;
static char *mic_uuid = "0";
static const short mic_xor[256] = {0};

static inline void wait_for_dma_data(struct mic_dev *mic_dev, struct mic_file_data *fdata) {
    int mic_cnt = mic_dev->cnt;

    if (fdata->wait_force) {
        fdata->wait_force = 0;
        wait_event(mic_dev->wait_queue,
                (mic_cnt != mic_dev->cnt
                || mic_dev->is_stoped
                || !(fdata->amic_enabled)
                ));
    }
}

static int mic_read(struct file *filp, struct raw_data *raw_data, int count)
{
    struct mic_dev *mic_dev = m_mic_dev;
    struct mic_file_data *fdata = (struct mic_file_data *)filp->private_data;
    struct mic *dmic = &mic_dev->dmic;
    struct mic *amic = &mic_dev->amic;
    int i, j, is_lost = 0;

    mutex_lock(&mic_dev->buf_lock);
    int amic_offset = amic->offset / amic->frame_size * amic->frame_size;

    int amic_buf_total_len = amic->buf_len * amic->buf_cnt;

    int amic_off_cnt = (amic_offset + amic_buf_total_len
            - fdata->amic_offset) % amic_buf_total_len / amic->frame_size ;

    int dmic_copy_cnt = 0;
    int amic_copy_cnt = min(amic_off_cnt, count);

    int aoff = fdata->amic_offset;

    amic_copy_cnt = min(amic_copy_cnt, amic_off_cnt - (-mic_dev->dmic_exoff_cnt));
    dmic_copy_cnt = amic_copy_cnt;
    aoff += (-mic_dev->dmic_exoff_cnt) * amic->frame_size;
    aoff %= amic_buf_total_len;

    int *amic_buf = (int *)(amic->buf[0] + aoff);
    /**
     * copy amic
     */
    int nr = (amic_buf_total_len - aoff) / amic->frame_size;
    nr = min(amic_copy_cnt, nr);


    for (i = 0; i < nr; i++) {
        raw_data[i].amic_channel = amic_buf[i*3];
        raw_data[i].amic[0] = amic_buf[i*3 + 0];
        raw_data[i].amic[1] = amic_buf[i*3 + 1];
        raw_data[i].amic[2] = amic_buf[i*3 + 2];
    }

    if (nr < amic_copy_cnt) {
        amic_buf = (int *)amic->buf[0];
        nr = amic_copy_cnt - nr;

        for(j = 0; j < nr; j++, i++) {
            raw_data[i].amic_channel = amic_buf[j*3];
            raw_data[i].amic[0] = amic_buf[j*3 + 0];
            raw_data[i].amic[1] = amic_buf[j*3 + 1];
            raw_data[i].amic[2] = amic_buf[j*3 + 2];
        }
    }

    if (is_lost) {
        raw_data[i].amic_channel = raw_data[i-1].amic_channel;
        raw_data[i].amic[0] = raw_data[i-1].amic[0];
        raw_data[i].amic[1] = raw_data[i-1].amic[1];
        raw_data[i].amic[2] = raw_data[i-1].amic[2];
    }

    fdata->amic_offset = (fdata->amic_offset +
            amic_copy_cnt * amic->frame_size) % amic_buf_total_len;
    mutex_unlock(&mic_dev->buf_lock);
    return dmic_copy_cnt * sizeof(struct raw_data);
}

static int mic_sys_read(struct file *filp, char *buf, size_t size, loff_t *f_pos) {
    struct mic_dev *mic_dev = m_mic_dev;
    struct mic_file_data *fdata = (struct mic_file_data *)filp->private_data;
    int i, offset = 0;

    if (fdata->open_type != mic_dev->open_type
            || !(fdata->amic_enabled)) {

        printk("failed to read mic data\n");
        return -1;
    }

    while (size) {
        /**
         * wait for dma callback
         */
        wait_for_dma_data(mic_dev, fdata);

        if (!mic_dev->hrtimer_uesd_count
                || !(fdata->amic_enabled)) {
            dev_err(mic_dev->dev, "mic is no enable\n");
            return -EPERM;
        }

        int bytes = mic_read(filp, (struct raw_data *)(buf + offset),
                size / sizeof(struct raw_data));

        offset += bytes;
        size -= bytes;
    }

    if (fdata->open_type == ALGORITHM) {
        int cnt = offset / sizeof(struct raw_data);
        struct raw_data *data = (struct raw_data *)buf;

        for (i = 0; i < cnt; i++) {
            int xor_id = i % (sizeof(mic_xor) / sizeof(short));
            data[i].amic_channel ^= mic_xor[xor_id];
        }
    }

    return offset;
}

static int mic_sys_write(struct file *filp, const char *buf, size_t count, loff_t *f_pos)
{
    return count;
}


static long mic_ioctl(struct file *filp, unsigned int cmd, unsigned long args)
{
    struct mic_dev *mic_dev = m_mic_dev;
    struct mic_file_data *fdata =
            (struct mic_file_data *)filp->private_data;

    switch (cmd) {
    case MIC_SET_PERIODS_MS:
        return mic_set_periods_ms(fdata, mic_dev, args);

    case MIC_GET_PERIODS_MS:
        copy_to_user((int *)args, &mic_dev->periods_ms, sizeof(int));
        return 0;

    case DMIC_ENABLE_RECORD:
        return dmic_enable_record(fdata, mic_dev);

    case DMIC_DISABLE_RECORD:
        dmic_disable_record(fdata, mic_dev);
        return 0;

    case AMIC_ENABLE_RECORD:
        return amic_enable_record(fdata, mic_dev);

    case AMIC_DISABLE_RECORD:
        amic_disable_record(fdata, mic_dev);
        return 0;

    case MIC_SET_DMIC_GAIN:
        return 0;

    case MIC_GET_DMIC_GAIN:
        copy_to_user((int *)args, &mic_dev->dmic.gain, sizeof(int));
        return 0;

    case MIC_GET_DMIC_GAIN_RANGE:
        return 0;

    case MIC_SET_AMIC_GAIN:
        amic_set_gain(mic_dev, args);
        return 0;

    case MIC_GET_AMIC_GAIN:
        copy_to_user((int *)args, &mic_dev->amic.gain, sizeof(int));
        return 0;

    case MIC_GET_AMIC_GAIN_RANGE:
        amic_get_gain_range((struct gain_range *)args);
        return 0;

    case MIC_CHECK_MIC_UUID:
        return 0;

    case MIC_DMIC_SET_OFFSET:
        mic_dev->dmic_exoff_cnt = (int)args;
        return 0;

    case MIC_DMIC_SET_BUFFER_TIME_MS:
        mic_set_buffer_time_ms((int)args);
        return 0;

    default:
        panic("error mic ioctl\n");
        break;
    }

    return 0;
}

unsigned int mic_poll(struct file *filp, struct poll_table_struct * table) {
    struct mic_dev *mic_dev = m_mic_dev;
    struct mic_file_data *fdata = (struct mic_file_data *)filp->private_data;
    unsigned int mask = 0;

    wait_for_dma_data(mic_dev, fdata);

    mask |= POLLIN | POLLRDNORM;

    return mask;
}

static int mic_open(struct inode *inode, struct file *filp)
{
    struct mic_dev *mic_dev = m_mic_dev;
    struct mic_file_data *fdata = (struct mic_file_data *)
            kmalloc(sizeof(struct mic_file_data), GFP_KERNEL);

    filp->private_data = fdata;
    memset(fdata, 0, sizeof(struct mic_file_data));

    fdata->dmic_enabled = 0;
    fdata->amic_enabled = 0;
    fdata->cnt = 0;
    fdata->dmic_offset = 0;
    fdata->amic_offset = 0;
    fdata->periods_ms = INT_MAX;
    fdata->open_type = NORMAL;
    INIT_LIST_HEAD(&fdata->entry);

    spin_lock(&mic_dev->list_lock);
    list_add_tail(&fdata->entry, &mic_dev->filp_data_list);
    spin_unlock(&mic_dev->list_lock);

    return 0;
}

static int mic_close(struct inode *inode, struct file *filp)
{
    struct mic_dev *mic_dev = m_mic_dev;
    struct mic_file_data *fdata =
            (struct mic_file_data *)filp->private_data;

    //dmic_disable_record(fdata, mic_dev);
    amic_disable_record(fdata, mic_dev);

    spin_lock(&mic_dev->list_lock);
    list_del(&fdata->entry);
    spin_unlock(&mic_dev->list_lock);

    kfree(fdata);

    return 0;
}

static struct file_operations mic_ops = {
    .owner = THIS_MODULE,
    .write = mic_sys_write,
    .read = mic_sys_read,
    .open = mic_open,
    .poll = mic_poll,
    .release = mic_close,
    .unlocked_ioctl = mic_ioctl,
};

int mic_sys_init(struct mic_dev *mic_dev) {
    dev_t dev = 0;
    int ret, dev_no;

    m_mic_dev = mic_dev;
    mic_dev->class = class_create(THIS_MODULE, "mic");
    mic_dev->minor = 0;
    mic_dev->nr_devs = 1;
    ret = alloc_chrdev_region(&dev, mic_dev->minor, mic_dev->nr_devs, "mic");
    if(ret) {
        printk("alloc chrdev failed\n");
        return -1;
    }
    mic_dev->major = MAJOR(dev);

    spin_lock_init(&mic_dev->list_lock);
    INIT_LIST_HEAD(&mic_dev->filp_data_list);

    dev_no = MKDEV(mic_dev->major, mic_dev->minor);
    cdev_init(&mic_dev->cdev, &mic_ops);
    mic_dev->cdev.owner = THIS_MODULE;
    cdev_add(&mic_dev->cdev, dev_no, 1);

    mic_dev->hrtimer_uesd_count = 0;
    mic_dev->is_stoped = 0;
    mic_dev->open_type = NORMAL;
    device_create(mic_dev->class, NULL, dev_no, NULL, "mic");
    return 0;
}
