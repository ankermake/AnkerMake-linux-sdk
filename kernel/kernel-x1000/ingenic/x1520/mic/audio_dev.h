
#ifndef __AUDIO_DEV_H__
#define __AUDIO_DEV_H__
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
#include <sound/core.h>
#include <sound/pcm.h>
#include <sound/pcm_params.h>
#include <mach/jzdma.h>
#include <linux/workqueue.h>
#include <common.h>

#include "dma_ops.h"

struct audio_dev;

struct audio_rtd_params {
    struct audio_dev *audio; /* parent chip */
    /* Size of the ring buffer */
    size_t dma_bytes;
    dma_addr_t dma_area;
    int dma_id;

    struct snd_pcm_substream *ss;
    int stream;

    int offset;

    size_t period_size;
    int period_ms;
    int periods;

    unsigned max_psize;

    struct dma_param *dma;
    struct hrtimer hrtimer;
    int hrtimer_uesd_count;
    int hrtimer_is_running;

    spinlock_t lock;

    struct work_struct work;
};

struct audio_dev {
    struct platform_device pdev;
    struct platform_driver pdrv;

    struct audio_rtd_params p_prm;
    struct audio_rtd_params c_prm;

    struct snd_card *card;
    struct snd_pcm *pcm;

    /* timekeeping for the playback endpoint */
    unsigned int p_interval;
    unsigned int p_residue;

    /* pre-calculated values for playback iso completion */
    unsigned int p_pktsize;
    unsigned int p_pktsize_residue;
    unsigned int p_framesize;
};

void i2s_enable_playback_dma(int rate);
void i2s_disable_playback_dma(void);
void i2s_enable_capture_dma(void (*callback)(void *data), void *data);
void i2s_disable_capture_dma(void);


void snd_hrtimer_init(struct audio_rtd_params *prm);
void snd_hrtimer_start(struct audio_rtd_params *prm);
void snd_hrtimer_stop(struct audio_rtd_params *prm);

int snd_internal_codec_init(struct dma_param *playback_dma, struct dma_param *capture_dma);
int snd_internal_codec_add_volume_control(struct audio_dev *audio);

#endif
