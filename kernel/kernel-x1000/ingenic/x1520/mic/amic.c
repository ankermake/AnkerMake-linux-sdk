
#include "mic.h"
#include "i2s.h"

extern int i2s_init(struct platform_device *pdev, struct mic *mic);
extern void codec_set_gain(int gain);

void amic_set_gain(struct mic_dev *mic_dev, int gain) {
    struct mic *amic = &mic_dev->amic;
    printk("entry: %s, set: %d\n", __func__, gain);

    if (gain > 29)
        gain = 29;
    if (gain < -18)
        gain = -18;

    amic->gain = gain;

    gain -= -18;
    gain = gain * 2 / 3;


    codec_set_gain(gain);
}

void amic_get_gain_range(struct gain_range *range) {
    range->min_dB = -18;
    range->max_dB = 29;
}

int amic_init(struct mic_dev *mic_dev) {

    i2s_init(mic_dev->pdev, &mic_dev->amic);
    return 0;
}
