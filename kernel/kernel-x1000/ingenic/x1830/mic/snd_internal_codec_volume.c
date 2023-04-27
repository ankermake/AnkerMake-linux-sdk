#include "audio_dev.h"
#include "i2s.h"
#include <sound/control.h>
#include <sound/core.h>
#include <sound/pcm.h>
#include <sound/pcm_params.h>
#include <sound/initval.h>
#include <sound/soc.h>
#include <sound/tlv.h>

enum {
    INTERNAL_CODEC_MASTER_VOLUME,
    INTERNAL_CODEC_NUM_REGS,
};

#define INTERNAL_CODEC_PLAY_MIN     0x00
#define INTERNAL_CODEC_PLAY_MAX     0x1f
#define INTERNAL_CODEC_PLAY_MIN_DB     (-39)
#define INTERNAL_CODEC_PLAY_MAX_DB     6

void jz_codec_get_volume_reg_range(int *min_vol, int *max_vol);

static int internal_codec_dai_info_volume(struct snd_kcontrol *kctrl,
                   struct snd_ctl_elem_info *uinfo)
{
    struct audio_dev *audio = snd_kcontrol_chip(kctrl);
    int min_vol, max_vol;

    jz_codec_get_volume_reg_range(&min_vol, &max_vol);

    uinfo->type = SNDRV_CTL_ELEM_TYPE_INTEGER;
    uinfo->count = 1;
    uinfo->value.integer.min = min_vol;
    uinfo->value.integer.max = max_vol;

    return 0;
}

static int internal_codec_dai_get_volume(struct snd_kcontrol *kctrl,
                  struct snd_ctl_elem_value *ucontrol)
{
    struct audio_dev *audio = snd_kcontrol_chip(kctrl);
    u32 vol;

    switch (kctrl->private_value) {
    case SNDRV_PCM_STREAM_PLAYBACK:
        ucontrol->value.integer.value[0] = i2s_get_playback_volume();
        break;
    case SNDRV_PCM_STREAM_CAPTURE:

        break;
    default:
        printk("%s() invalid private_value=%ld\n",
            __func__, kctrl->private_value);
        return -EINVAL;
    }

    return 0;
}

static int internal_codec_dai_put_volume(struct snd_kcontrol *kctrl,
                  struct snd_ctl_elem_value *ucontrol)
{
    struct audio_dev *audio = snd_kcontrol_chip(kctrl);
    u32 vol;

    if (ucontrol->value.integer.value[0] < INTERNAL_CODEC_PLAY_MIN ||
        ucontrol->value.integer.value[0] > INTERNAL_CODEC_PLAY_MAX)
        return -EINVAL;

    vol = ucontrol->value.integer.value[0];

    switch (kctrl->private_value) {
    case SNDRV_PCM_STREAM_PLAYBACK:
        i2s_set_playback_volume(vol);
        break;
    case SNDRV_PCM_STREAM_CAPTURE:

        break;
    default:
        printk("%s() invalid private_value=%ld\n",
            __func__, kctrl->private_value);
        return -EINVAL;
    }

    return 1;
}

static const DECLARE_TLV_DB_SCALE(db_scale,
        INTERNAL_CODEC_PLAY_MIN_DB, INTERNAL_CODEC_PLAY_MAX_DB, 0);

static struct snd_kcontrol_new playback_controls = {
    .iface      = SNDRV_CTL_ELEM_IFACE_MIXER,
    .access = SNDRV_CTL_ELEM_ACCESS_READWRITE
                | SNDRV_CTL_ELEM_ACCESS_TLV_READ,
    .name       = "Master Playback Volume",
    .index      = 0,
    .tlv = { .p = db_scale },
    .info       = internal_codec_dai_info_volume,
    .get        = internal_codec_dai_get_volume,
    .put        = internal_codec_dai_put_volume,
    .private_value  = SNDRV_PCM_STREAM_PLAYBACK,
};

int snd_internal_codec_add_volume_control(struct audio_dev *audio) {

    struct snd_kcontrol *kctrl = snd_ctl_new1(&playback_controls, audio);
    strcpy(audio->card->mixername, "Headset Mixer");
    int ret = snd_ctl_add(audio->card, kctrl);
    assert(ret == 0);

    return 0;
}
