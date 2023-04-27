#include <linux/platform_device.h>
#include <linux/resource.h>
#include <linux/kernel.h>
#include <linux/gpio.h>
#include <soc/base.h>
#include <soc/irq.h>
#include <linux/device.h>
#include <linux/init.h>
#include <mach/jzdma.h>

static struct resource dmic_resources[] = {
    [0] = {
        .start  = DMIC_IOBASE,
        .end    = DMIC_IOBASE + 0x70 -1,
        .flags  = IORESOURCE_MEM,
    },
    [1] = {
        .start  = IRQ_DMIC,
        .end    = IRQ_DMIC,
        .flags  = IORESOURCE_IRQ,
    },
    [2] = {
        .start  = JZDMA_REQ_DMIC,
        .end    = JZDMA_REQ_DMIC,
        .flags  = IORESOURCE_DMA,
    },
};

/* stop no dev release warning */
static void dmic_device_release(struct device *dev){}

struct platform_device mic_device = {
    .name           = "dmic",
    .id             = -1,
    .resource       = dmic_resources,
    .num_resources  = ARRAY_SIZE(dmic_resources),
    .dev            = {
        .release = dmic_device_release,
    },
};

int dmic_platform_device_init(void)
{
    int ret;

    ret = platform_device_register(&mic_device);
    if (ret) {
        printk(KERN_ERR "DMIC: Failed to register mic dev: %d\n", ret);
        return ret;
    }

    return 0;
}

void dmic_platform_device_exit(void)
{
    platform_device_unregister(&mic_device);
}