#ifndef __MIC_H__
#define __MIC_H__
#include <linux/module.h>
#include <linux/moduleparam.h>
#include <linux/device.h>
#include <linux/mm.h>
#include <linux/dmaengine.h>
#include <linux/dma-mapping.h>
#include <linux/hrtimer.h>
#include <linux/dmaengine.h>
#include <linux/types.h>
#include <linux/fs.h>
#include <linux/circ_buf.h>
#include <linux/cdev.h>
#include <linux/platform_device.h>
#include <linux/kernel.h>
#include <linux/io.h>
#include <linux/clk.h>
#include <linux/memory.h>
#include <linux/mm.h>
#include <linux/init.h>
#include <linux/slab.h>
#include <linux/vmalloc.h>
#include <linux/ktime.h>
#include <linux/hrtimer.h>
#include <linux/spinlock.h>
#include <linux/wait.h>
#include <linux/sched.h>
#include <mach/jzdma.h>
#include <common.h>

#include "audio_dev.h"

enum mic_type {
    AMIC,
    DMIC,
};

struct mic_dev;

struct mic {
    int type;

    struct mic_dev *mic_dev;
    struct dma_param *dma;

    int offset;
    unsigned long long cnt;

    char **buf;
    int buf_len;
    int buf_cnt;
    int period_len;
    int frame_size;
    struct scatterlist sg[2];

    int gain;
    int is_enabled;
};

struct raw_data {
    short dmic_channel[4];
    short amic_channel;
    short amic[3];
};

struct gain_range {
    int min_dB;
    int max_dB;
};

typedef enum mic_open_type {
    NORMAL,
    ALGORITHM,
} mic_open_type_t;

struct mic_dev {
    struct device *dev;
    struct platform_device *pdev;

    int major;
    int minor;
    int nr_devs;
    struct class *class;
    struct cdev cdev;

    struct list_head filp_data_list;
    spinlock_t list_lock;
    int is_stoped;

    int periods_ms;
    int raws_pre_period;
    struct mutex buf_lock;
    wait_queue_head_t wait_queue;

    int cnt;
    int hrtimer_uesd_count;
    struct hrtimer hrtimer;

    struct mic dmic;
    struct mic amic;
    int dmic_exoff_cnt;

    char uuid[512];
    mic_open_type_t open_type;
};

struct mic_file_data {
    struct list_head entry;
    unsigned long long cnt;

    int dmic_offset;
    int amic_offset;
    int periods_ms;
    int dmic_enabled;
    int amic_enabled;
    int wait_force;
    mic_open_type_t open_type;
};

#define MIC_DEFAULT_PERIOD_MS       (250)       //250 ms
/**
 * 2s buffer, 12=sizeof(short) *4 + sizeof(int) * 3
 */
#define MIC_BUFFER_TOTAL_LEN        (2 * 16000 * 20)

#define MIC_SET_PERIODS_MS          0x100
#define MIC_GET_PERIODS_MS          0x101
#define DMIC_ENABLE_RECORD          0x102
#define DMIC_DISABLE_RECORD         0x103
#define AMIC_ENABLE_RECORD          0x104
#define AMIC_DISABLE_RECORD         0x105
#define MIC_SET_DMIC_GAIN           0x200
#define MIC_GET_DMIC_GAIN_RANGE     0x201
#define MIC_GET_DMIC_GAIN           0x202
#define MIC_SET_AMIC_GAIN           0x300
#define MIC_GET_AMIC_GAIN_RANGE     0x301
#define MIC_GET_AMIC_GAIN           0x302
#define MIC_CHECK_MIC_UUID          0x400
#define MIC_DMIC_SET_OFFSET         0x500
#define MIC_DMIC_SET_BUFFER_TIME_MS 0x501

void dmic_enable(struct mic_dev *mic_dev);
void dmic_disable(struct mic_dev *mic_dev);
void dmic_set_gain(struct mic_dev *mic_dev, int gain);
void dmic_get_gain_range(struct gain_range *range);
void amic_set_gain(struct mic_dev *mic_dev, int gain);
void amic_get_gain_range(struct gain_range *range);
void mic_set_buffer_time_ms(int buffer_time_ms);
void amic_dma_reset(void);
void amic_enable(void);
void amic_disable(void);

void mic_dma_terminate(struct mic *mic);
int mic_dma_submit(struct mic_dev *mic_dev, struct mic *mic);
int mic_dma_submit_cyclic(struct mic_dev *mic_dev, struct mic *mic);
int mic_prepare_dma(struct mic_dev *mic_dev, struct mic *mic);

void mic_hrtimer_start(struct mic_dev *mic_dev);
void mic_hrtimer_stop(struct mic_dev *mic_dev);
void mic_hrtimer_init(struct mic_dev *mic_dev);

int mic_sys_init(struct mic_dev *mic_dev);
int mic_set_periods_ms(struct mic_file_data *fdata, struct mic_dev *mic_dev, int periods_ms);
int dmic_enable_record(struct mic_file_data *fdata, struct mic_dev *mic_dev);
int dmic_disable_record(struct mic_file_data *fdata, struct mic_dev *mic_dev);
int amic_enable_record(struct mic_file_data *fdata, struct mic_dev *mic_dev);
int amic_disable_record(struct mic_file_data *fdata, struct mic_dev *mic_dev);

#define DMIC_AMIC_DIFF_CNT_MAX  48
#define DMIC_FIFO_REQUEST   32
#define AMIC_FIFO_REQUEST   16

#endif
