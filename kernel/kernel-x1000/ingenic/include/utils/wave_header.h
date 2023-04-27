#ifndef _WAVE_HEADER_H_
#define _WAVE_HEADER_H_

#include <linux/kernel.h>

struct wave_riff_header {
    char     riff_tag[4];
    uint32_t file_len;
    char     wave_tag[4];
    char     fmt_tag[4];
    uint32_t wave_fmt_header_size;
};

struct wave_fmt_header {
    uint16_t fmt;
    uint16_t channel_num;
    uint32_t sample_frq_hz;
    uint32_t bytes_per_sec;
    uint16_t block_align;
    uint16_t bits_per_sample;
};

struct wave_data_header {
    char     data_tag[4];
    uint32_t data_len;
};

enum wave_fmt_t {
    WAVE_FMT_PCM = 1,
};


struct wave_header {
    struct wave_riff_header riff;
    struct wave_fmt_header fmt;
    struct wave_data_header data;
    uint8_t *extra_fmt_data;
};

int wave_header_size(struct wave_header *wave);

struct wave_header *create_wave_header(const char *buffer, int buffer_size);

#endif /* _WAVE_HEADER_H_ */
