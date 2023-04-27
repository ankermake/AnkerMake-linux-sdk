#include <utils/wave_header.h>
#include <linux/printk.h>
#include <linux/slab.h>

int wave_header_size(struct wave_header *wave)
{
    return sizeof(wave->riff) +
        wave->riff.wave_fmt_header_size +
        sizeof(wave->data);
}

struct wave_header *create_wave_header(const char *buffer, int buffer_size)
{
    struct wave_header *wave;
    struct wave_riff_header riff;
    struct wave_fmt_header fmt;
    struct wave_data_header data;
    const char *buf;
    int extra_size;

    if (buffer_size < sizeof(riff)) {
        printk("%s buffer_size is too small 1\n", __func__);
        return NULL;
    }

    buf = buffer;
    memcpy(&riff, buf, sizeof(riff));
    buf += sizeof(riff);

    if (riff.wave_fmt_header_size < sizeof(fmt)) {
        printk("invlaid wave_fmt_header_size: %d\n", riff.wave_fmt_header_size);
        return NULL;
    }

    if (buffer_size < (sizeof(riff) + riff.wave_fmt_header_size + sizeof(data))) {
        printk("%s buffer_size is too small 2\n", __func__);
        return NULL;
    }

    if (strncmp(riff.riff_tag, "RIFF", 4)) {
        printk("invalid riff tag : %c%c%c%c\n",
            riff.riff_tag[0],riff.riff_tag[1],riff.riff_tag[2],riff.riff_tag[3]);
        return NULL;
    }

    if (strncmp(riff.wave_tag, "WAVE", 4)) {
        printk("invalid wave tag : %c%c%c%c\n",
            riff.wave_tag[0],riff.wave_tag[1],riff.wave_tag[2],riff.wave_tag[3]);
        return NULL;
    }

    if (strncmp(riff.fmt_tag, "fmt ", 4)) {
        printk("invalid fmt tag : %c%c%c%c\n",
               riff.fmt_tag[0],riff.fmt_tag[1],riff.fmt_tag[2],riff.fmt_tag[3]);
        return NULL;
    }

    memcpy(&fmt, buf, sizeof(fmt));
    buf += sizeof(fmt);

    if (fmt.fmt != WAVE_FMT_PCM) {
        printk("only support pcm wave fmt: %d\n", fmt.fmt);
        return NULL;
    }

    if (!(fmt.channel_num == 1 ||
          fmt.channel_num == 2)) {
        printk("only support 1 or 2 channel: %d\n", fmt.channel_num);
        return NULL;
    }

    switch (fmt.bits_per_sample) {
    case 16:
    case 24:
    case 32:
        break;

    default:
        printk("can not supprot this sample bits: %d\n", fmt.bits_per_sample);
        return NULL;
    }

    buf = buffer + sizeof(riff) + riff.wave_fmt_header_size;
    memcpy(&data, buf, sizeof(data));

    if (strncmp(data.data_tag, "data", 4)) {
        printk("invalid data tag: %c%c%c%c\n",
               data.data_tag[0],data.data_tag[1],data.data_tag[2],data.data_tag[3]);
        return NULL;
    }

    extra_size = riff.wave_fmt_header_size - sizeof(fmt);

    wave = kmalloc(sizeof(wave) + extra_size, GFP_KERNEL);
    wave->riff = riff;
    wave->fmt = fmt;
    wave->data = data;

    if (extra_size) {
        wave->extra_fmt_data = (void *)&wave[1];
        memcpy(wave->extra_fmt_data, buffer + sizeof(riff) + sizeof(fmt), extra_size);
    } else {
        wave->extra_fmt_data = NULL;
    }

    return wave;
}
