#include <linux/module.h>
#include <linux/miscdevice.h>
#include <linux/platform_device.h>
#include <linux/ioctl.h>
#include <bit_field.h>
#include <linux/interrupt.h>
#include <linux/clk.h>
#include <linux/dma-mapping.h>
#include "common.h"
#include <utils/clock.h>

#include "rotator_regs.h"
#include "rotator.h"

#define MIN_WIDTH   4
#define MIN_HEIGHT  4
#define MAX_WIDTH   2047
#define MAX_HEIGHT  2047

#define ROTATOR_TIMEOUT 400

#define CMD_rotator_complete_conversion                   _IOWR('R', 122, struct rotator_config_data *)

struct frame_descriptor
{
    unsigned long next_des_addr;
    unsigned long src_buffer_addr;
    unsigned long src_stride;
    unsigned long frame_stop;
    unsigned long dst_buffer_addr;
    unsigned long dst_stride;
    unsigned long interrupt_control;
}__attribute__ ((aligned(8)));

struct rotator_device
{
    struct clk *clk;
    struct mutex mutex;
    wait_queue_head_t wq;
    unsigned int is_finish;
    int irq_is_request;

    struct frame_descriptor *desc;
    struct miscdevice *mdev;
};

static struct rotator_device rotator_dev;
/***************************************/
#define IRQ_ROTATOR         IRQ_INTC_BASE + 29

#define ROTATOR_IOBASE      0x13070000
#define ROTATOR_REG_BASE    KSEG1ADDR(ROTATOR_IOBASE)
#define ROTATOR_ADDR(reg)   ((volatile unsigned long *)(ROTATOR_REG_BASE + reg))

static inline void rotator_write_reg(unsigned int reg, unsigned int value)
{
    *ROTATOR_ADDR(reg) = value;
}

static inline unsigned int rotator_read_reg(unsigned int reg)
{
    return *ROTATOR_ADDR(reg);
}

static inline void rotator_set_bits(unsigned int reg, int start, int end, unsigned int val)
{
    set_bit_field(ROTATOR_ADDR(reg), start, end, val);
}

static inline unsigned int rotator_get_bits(unsigned int reg, int start, int end)
{
    return get_bit_field(ROTATOR_ADDR(reg), start, end);
}

static inline void *rotator_m_dma_alloc_coherent(struct device *dev, int size)
{
    dma_addr_t dma_handle;
    void *mem = dma_alloc_coherent(dev, size, &dma_handle, GFP_KERNEL);
    assert(mem);

    return (void *)CKSEG0ADDR(mem);
}

static inline void rotator_m_dma_free_coherent(struct device *dev, void *mem, int size)
{
    dma_addr_t dma_handle = virt_to_phys(mem);
    dma_free_coherent(dev, size, (void *)CKSEG1ADDR(mem), dma_handle);
}

/***************************************/
int rotator_bytes_per_pixel(enum rotator_fmt fmt)
{
    if (fmt == ROTATOR_RGB888 || fmt == ROTATOR_ARGB8888)
        return 4;
    if (fmt == ROTATOR_Y8)
        return 1;
    return 2;
}

static void rotator_init_desc(struct frame_descriptor *desc, struct rotator_config_data *config)
{
    memset(desc, 0, sizeof(struct frame_descriptor));

    desc->frame_stop = 1;

    set_bit_field(&desc->interrupt_control, f_EOF_MASK, 1);
    set_bit_field(&desc->interrupt_control, f_SOF_MASK, 1);

    desc->next_des_addr = virt_to_phys(desc);

    desc->src_buffer_addr = (unsigned long)(config->src_buf);
    desc->src_stride = config->src_stride;

    desc->dst_buffer_addr = (unsigned long)(config->dst_buf);
    desc->dst_stride = config->dst_stride;

    dma_cache_wback((unsigned long)desc, sizeof(struct frame_descriptor));
}

static void rotator_enable(void)
{
    rotator_set_bits(ROTATOR_INT_MASK, ROTATORINTMASK_EOF_MASK, 1);
    rotator_set_bits(ROTATOR_CTRL, ROTATORCTRL_START, 1);
}

static void rotator_config(struct frame_descriptor *desc, struct rotator_config_data *config)
{
    static const unsigned char src_fmt[] = {
        [ROTATOR_RGB555]     = 0,
        [ROTATOR_ARGB1555]   = 1,
        [ROTATOR_RGB565]     = 2,
        [ROTATOR_RGB888]     = 4,
        [ROTATOR_ARGB8888]   = 5,
        [ROTATOR_Y8]         = 6,
        [ROTATOR_YUV422]     = 10,
    };

    static const unsigned char dst_fmt[] = {
        [ROTATOR_ARGB8888]   = 0,
        [ROTATOR_RGB565]     = 1,
        [ROTATOR_RGB555]     = 2,
        [ROTATOR_YUV422]     = 3,
        [ROTATOR_Y8]         = 4,
    };

    rotator_set_bits(ROTATOR_QOS_CFG, ROTATORQOSCFG_STD_CLK, 13);
    rotator_set_bits(ROTATOR_QOS_CTRL, ROTATORQOSCTRL_FIX_EN, 1);
    rotator_set_bits(ROTATOR_GLB_CFG, ROTATORGLBCFG_WDMA_BURST_LEN, 3);
    rotator_set_bits(ROTATOR_GLB_CFG, ROTATORGLBCFG_RDMA_BURST_LEN, 3);

    rotator_write_reg(ROTATOR_FRM_CFG_ADDR, virt_to_phys(desc));

    rotator_set_bits(ROTATOR_FRM_SIZE, ROTATORFRMSIZE_FRAM_HEIGHT, config->frame_height);
    rotator_set_bits(ROTATOR_FRM_SIZE, ROTATORFRMSIZE_FRAM_WIDTH, config->frame_width);

    rotator_set_bits(ROTATOR_GLB_CFG, ROTATORGLBCFG_RDMA_FMT, src_fmt[config->src_fmt]);
    rotator_set_bits(ROTATOR_GLB_CFG, ROTATORGLBCFG_RDMA_ORDER, config->convert_order);
    rotator_set_bits(ROTATOR_GLB_CFG, ROTATORGLBCFG_WDMA_FMT, dst_fmt[config->dst_fmt]);

    rotator_set_bits(ROTATOR_GLB_CFG, ROTATORGLBCFG_ROT_ANGLE, config->rotate_angle);
    rotator_set_bits(ROTATOR_GLB_CFG, ROTATORGLBCFG_VERTICAL_MIRROR, config->vertical_mirror);
    rotator_set_bits(ROTATOR_GLB_CFG, ROTATORGLBCFG_HORIZONTAL_MIRROR, config->horizontal_mirror);
}

static int rotator_check_src_fmt(struct rotator_config_data *config)
{
    switch (config->src_fmt) {
        case ROTATOR_RGB555:
        case ROTATOR_ARGB1555:
        case ROTATOR_RGB565:
        case ROTATOR_RGB888:
        case ROTATOR_ARGB8888:
        case ROTATOR_Y8:
        case ROTATOR_YUV422:
            return 1;
        default:
            break;
    }

    return 0;
}

static int rotator_check_dst_fmt(struct rotator_config_data *config)
{
    switch (config->dst_fmt) {
        case ROTATOR_ARGB8888:
        case ROTATOR_RGB565:
        case ROTATOR_RGB555:
        case ROTATOR_YUV422:
        case ROTATOR_Y8:
            return 1;
        default:
            break;
    }

    return 0;
}

static int rotator_check_params(struct rotator_config_data *config)
{
    int ret;

    if (config->frame_height > MAX_HEIGHT || config->frame_height < MIN_HEIGHT) {
        printk(KERN_ERR "rotator frame_height %d not in range 4~2047\n", config->frame_height);
        return -EINVAL;
    }

    if (config->frame_width > MAX_WIDTH || config->frame_width < MIN_WIDTH) {
        printk(KERN_ERR "rotator frame_width %d not in range 4~2047\n", config->frame_width);
        return -EINVAL;
    }

    ret = rotator_check_src_fmt(config);
    if (!ret) {
        printk(KERN_ERR "rotator source fmt %d no support!\n", config->src_fmt);
        return -EINVAL;
    }

    ret = rotator_check_dst_fmt(config);
    if (!ret) {
        printk(KERN_ERR "rotator destin fmt %d no support!\n", config->dst_fmt);
        return -EINVAL;
    }

    return 0;
}

static irqreturn_t rotator_irq_handler(int irq, void *data);
int rotator_complete_conversion(struct rotator_config_data *config)
{
    int ret = 0;
    struct frame_descriptor *desc;

    ret = rotator_check_params(config);
    if (ret < 0) {
        printk(KERN_ERR "rotator params error!\n");
        return -EINVAL;
    }

    mutex_lock(&rotator_dev.mutex);

    desc = rotator_dev.desc;

    rotator_init_desc(desc, config);

    rotator_config(desc, config);

    if (!rotator_dev.irq_is_request) {
        ret = request_irq(IRQ_ROTATOR, rotator_irq_handler, IRQF_SHARED, "rotator", &rotator_dev);
        BUG_ON(ret);
        rotator_dev.irq_is_request = 1;
    }

    rotator_dev.is_finish = 0;
    rotator_enable();

    ret = wait_event_interruptible_timeout(rotator_dev.wq, rotator_dev.is_finish, msecs_to_jiffies(ROTATOR_TIMEOUT));
    if (ret <= 0)
        printk(KERN_ERR "rotator convert timeout!\n");

    mutex_unlock(&rotator_dev.mutex);

    return (ret > 0) ? 0 : -ETIMEDOUT;
}

static irqreturn_t rotator_irq_handler(int irq, void *data)
{
    if (rotator_get_bits(ROTATOR_STATUS, ROTATORSTATUS_EOF)) {
        rotator_set_bits(ROTATOR_CLR_STATUS, ROTATORCLRSTATUS_CLR_EOF, 1);
        rotator_dev.is_finish = 1;
        wake_up_interruptible(&rotator_dev.wq);
    }

    if (rotator_get_bits(ROTATOR_STATUS, ROTATORSTATUS_GEN_STOP_ACK)) {
        rotator_set_bits(ROTATOR_CLR_STATUS, ROTATORCLRSTATUS_CLR_GEN_STP_ACK, 1);
    }

    return IRQ_HANDLED;
}

static long rotator_ioctl(struct file *filp, unsigned int cmd, unsigned long arg)
{
    int ret = 0;
    struct rotator_config_data *data;
    data = (struct rotator_config_data *)arg;

    switch (cmd) {
    case CMD_rotator_complete_conversion:
        ret = rotator_complete_conversion(data);
        break;

    default:
        printk(KERN_ERR "ROTATOR: not support this cmd:%x\n", cmd);
        ret = -1;
        break;
    }

    return ret;
}

static struct file_operations rotator_fops = {
    .owner = THIS_MODULE,
    .unlocked_ioctl = rotator_ioctl,
};

static struct miscdevice rotator_mdev = {
    .minor = MISC_DYNAMIC_MINOR,
    .name  = "jz_rotator",
    .fops  = &rotator_fops,
};

static int ingenic_rotator_probe(struct platform_device *pdev)
{
    int ret;

    mutex_init(&rotator_dev.mutex);
    init_waitqueue_head(&rotator_dev.wq);

    rotator_dev.clk = clk_get(NULL, "gate_rot");
    if (IS_ERR(rotator_dev.clk)) {
        printk(KERN_ERR "get rotator clk fail!\n");
        return -1;
    }

    clk_prepare_enable(rotator_dev.clk);

    rotator_dev.mdev = &rotator_mdev;
    ret = misc_register(&rotator_mdev);
    BUG_ON(ret < 0);

    rotator_dev.irq_is_request = 0;
    rotator_dev.desc = rotator_m_dma_alloc_coherent(&pdev->dev, sizeof(struct frame_descriptor));

    return 0;
}

static int ingenic_rotator_remove(struct platform_device *pdev)
{
    rotator_m_dma_free_coherent(&pdev->dev, rotator_dev.desc, sizeof(struct frame_descriptor));
    misc_deregister(&rotator_mdev);
    if (rotator_dev.irq_is_request == 1) {
        free_irq(IRQ_ROTATOR, &rotator_dev);
    }
    clk_disable_unprepare(rotator_dev.clk);
    clk_put(rotator_dev.clk);

    return 0;
}

static struct platform_driver ingenic_rotator_driver = {
    .probe = ingenic_rotator_probe,
    .remove = ingenic_rotator_remove,
    .driver = {
        .owner = THIS_MODULE,
        .name = "ingenic-rotator",
    },
};

/* stop no dev release warning */
static void jz_rotator_dev_release(struct device *dev){}

struct platform_device ingenic_rotator_device = {
    .name = "ingenic-rotator",
    .dev  = {
        .release = jz_rotator_dev_release,
    },
};

static int __init jz_rotator_init(void)
{
    int ret = platform_device_register(&ingenic_rotator_device);
    if (ret)
        return ret;

    return platform_driver_register(&ingenic_rotator_driver);
}
module_init(jz_rotator_init);

static void __exit jz_rotator_exit(void)
{
    platform_device_unregister(&ingenic_rotator_device);

    platform_driver_unregister(&ingenic_rotator_driver);
}
module_exit(jz_rotator_exit);

EXPORT_SYMBOL(rotator_complete_conversion);
EXPORT_SYMBOL(rotator_bytes_per_pixel);

MODULE_DESCRIPTION("X2000 SoC ROTATOR driver");
MODULE_LICENSE("GPL");