
#include <linux/interrupt.h>
#include "mic.h"

//#include "dmic_hal.h"
#include "i2s.h"

#include "audio_dev.h"

int mic_resource_init(
        struct platform_device *pdev,
        struct mic_dev *mic_dev, int type) {
    int ret;
    int res_no = (type == DMIC) ? 1 : 0;
    struct mic *mic = (type == DMIC) ? &mic_dev->dmic : &mic_dev->amic;

    mic->mic_dev = mic_dev;
    mic->type = type;
    mic->dma = dma_param_init(pdev, res_no);
    if (mic->dma < 0) {
        return -1;
    }
    struct dma_param *dma = mic->dma;

    if (type == AMIC) {
        dma->dma_addr = mic->dma->io_res->start + AICDR;
        dma->rx_buswidth = DMA_SLAVE_BUSWIDTH_2_BYTES;
        dma->rx_maxburst = 16;
    }

    return 0;
}

extern int amic_init(struct mic_dev *mic_dev);
static int mic_pdata_init(struct mic_dev *mic_dev, struct platform_device *pdev)
{
    int ret;

    ret = mic_resource_init(pdev, mic_dev, AMIC);
    if (ret < 0)
        return ret;


    ret = amic_init(mic_dev);
    if (ret < 0)
        return ret;


    return 0;
}

static int mic_probe(struct platform_device *pdev)
{
    int ret;
    struct clk *dmic_clk;
    struct mic_dev *mic_dev;

    mic_dev = kmalloc(sizeof(struct mic_dev), GFP_KERNEL);
    if(!mic_dev) {
        printk("voice dev alloc failed\n");
        goto __err_mic;
    }
    memset(mic_dev, 0, sizeof(struct mic_dev));

    mic_dev->dev = &pdev->dev;
    dev_set_drvdata(mic_dev->dev, mic_dev);
    mic_dev->pdev = pdev;

    mutex_init(&mic_dev->buf_lock);
    init_waitqueue_head(&mic_dev->wait_queue);

    dmic_clk = clk_get(mic_dev->dev,"dmic");
    if (IS_ERR(dmic_clk)) {
        dev_err(mic_dev->dev, "Failed to get clk dmic\n");
        goto __err_mic;
    }
    clk_enable(dmic_clk);

    ret = mic_pdata_init(mic_dev, pdev);
    if (ret < 0)
        return ret;

    mic_hrtimer_init(mic_dev);

    mic_sys_init(mic_dev);

    mic_set_periods_ms(NULL, mic_dev, MIC_DEFAULT_PERIOD_MS);

#ifdef CONFIG_X1520_MIC_SND_INTERNAL_CODEC
    snd_internal_codec_init(mic_dev->amic.dma, mic_dev->dmic.dma);
#endif

    return 0;

__err_mic:
    return -EFAULT;
}

static const  struct platform_device_id mic_id_table[] = {
        { .name = "mic", },
        {},
};

static struct platform_driver mic_platform_driver = {
        .probe = mic_probe,
        .driver = {
            .name = "mic",
            .owner = THIS_MODULE,
        },
        .id_table = mic_id_table,
};

static int __init mic_init(void) {
    return platform_driver_register(&mic_platform_driver);
}

static void __exit mic_exit(void)
{
}

module_init(mic_init);
module_exit(mic_exit);
