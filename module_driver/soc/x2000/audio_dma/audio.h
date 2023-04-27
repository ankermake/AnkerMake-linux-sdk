#ifndef _AUDIO_H_
#define _AUDIO_H_

enum audio_dev_id {
    Dev_src_dmic,
    Dev_src_spdif_in,
    Dev_src_baic0,
    Dev_src_baic1,
    Dev_src_baic2,
    Dev_src_reserved0,
    Dev_src_baic4,
    Dev_src_mixer_out,
    Dev_src_dma0,
    Dev_src_dma1,
    Dev_src_dma2,
    Dev_src_dma3,
    Dev_src_dma4,
    Dev_src_reserved1,
    Dev_src_reserved2,

    Dev_tar_start,
    Dev_tar_baic0 = Dev_tar_start,
    Dev_tar_baic1,
    Dev_tar_spdif_out,
    Dev_tar_baic3,
    Dev_tar_baic4,
    Dev_tar_dma5,
    Dev_tar_dma6,
    Dev_tar_dma7,
    Dev_tar_dma8,
    Dev_tar_dma9,
    Dev_tar_mixer_in0,
    Dev_tar_mixer_in1,
};

static inline int dev_to_slot_id(enum audio_dev_id dev_id)
{
    return dev_id + 1;
}

static inline int dev_to_dma_id(enum audio_dev_id dev_id)
{
    if (dev_id < Dev_tar_start)
        return dev_id - Dev_src_dma0;
    else
        return dev_id - Dev_tar_dma5 + 5;
}

static inline int dev_to_baic_id(enum audio_dev_id dev_id)
{
    if (dev_id < Dev_tar_start)
        return dev_id - Dev_src_baic0;
    else
        return dev_id - Dev_tar_baic1 + 1;
}

struct audio_dma_desc {
    unsigned long cmd;
    unsigned long dma_addr;
    unsigned long next_desc;
    unsigned long trans_count;
};

enum audio_dev_id audio_requst_src_dma_dev(void);

enum audio_dev_id audio_requst_tar_dma_dev(void);

void audio_release_dma_dev(enum audio_dev_id dev_id);

void audio_connect_dev(enum audio_dev_id src_id, enum audio_dev_id tar_id);

void audio_disconnect_dev(enum audio_dev_id src_id, enum audio_dev_id tar_id);

void audio_dma_desc_init(
    struct audio_dma_desc *desc,
    void *buf, int buf_size, int unit_size,
    struct audio_dma_desc *next);

void audio_dma_config(enum audio_dev_id dev_id, int channels, int data_bits, int unit_size, snd_pcm_format_t format);

void audio_dma_set_callback(enum audio_dev_id dev_id,
     void (*dma_callback)(void *data), void *data);

void audio_dma_start(enum audio_dev_id dev_id, struct audio_dma_desc *desc);

void audio_dma_stop(enum audio_dev_id dev_id);

void audio_dma_pause_release(enum audio_dev_id dev_id, struct audio_dma_desc *desc);

void audio_dma_pause_push(enum audio_dev_id dev_id);

unsigned int audio_dma_get_current_addr(enum audio_dev_id dev_id);

#endif /* _AUDIO_H_ */
