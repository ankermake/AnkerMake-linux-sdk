#include <linux/platform_device.h>
#include <linux/resource.h>
#include <linux/kernel.h>
#include <linux/gpio.h>
#include <soc/base.h>
#include <soc/irq.h>
#include <linux/device.h>
#include <linux/init.h>
#include <mach/jzdma.h>

static struct resource aic_resources[] = {
    [0] = {
        .start  = AIC0_IOBASE,
        .end    = AIC0_IOBASE + 0x1000 - 1,
        .flags  = IORESOURCE_MEM,
    },
    [1] = {
        .start  = IRQ_AIC0,
        .end    = IRQ_AIC0,
        .flags  = IORESOURCE_IRQ,
    },
    [2] = {
        .start  = JZDMA_REQ_I2S0,
        .end    = JZDMA_REQ_I2S0,
        .flags  = IORESOURCE_DMA,
    },
};

/* stop no dev release warning */
static void aic_device_release(struct device *dev){}

static struct platform_device aic_device = {
    .name           = "ingenic-aic",
    .id             = -1,
    .resource       = aic_resources,
    .num_resources  = ARRAY_SIZE(aic_resources),
    .dev            = {
        .release = aic_device_release,
    },
};

int aic_platform_device_init(void)
{
    int ret;

    ret = platform_device_register(&aic_device);
    if (ret) {
        printk(KERN_ERR "AIC: Failed to register aic dev: %d\n", ret);
        return ret;
    }

    return 0;
}

void aic_platform_device_exit(void)
{
    platform_device_unregister(&aic_device);
}
