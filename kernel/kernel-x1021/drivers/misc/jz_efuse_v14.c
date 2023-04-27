#include <linux/clk.h>
#include <linux/ctype.h>
#include <linux/delay.h>
#include <linux/fs.h>
#include <linux/gpio.h>
#include <linux/init.h>
#include <linux/jiffies.h>
#include <linux/kernel.h>
#include <linux/miscdevice.h>
#include <linux/module.h>
#include <linux/platform_device.h>
#include <linux/proc_fs.h>
#include <linux/sched.h>
#include <linux/seq_file.h>
#include <linux/slab.h>
#include <linux/timer.h>
#include <mach/jz_efuse.h>
#include <soc/base.h>
#include <jz_proc.h>

#define DRV_NAME "jz-efuse-v14"

#define EFUSE_CTRL 0x0
#define EFUSE_CFG 0x4
#define EFUSE_STATE 0x8
#define EFUSE_DATA 0xC

#define CHIP_ID_ADDR (0x00)
#define USER_ID_ADDR (0x0C)
#define SARADC_CAL (0x10)
#define TRIM_ADDR (0x12)
#define PROGRAM_PROTECT_ADDR (0x13)
#define CPU_ID_ADDR (0x14)
#define SPECIAL_ADDR (0x16)
#define CUSTOMER_RESV_ADDR (0x18)

unsigned int seg_addr[] = {
    CHIP_ID_ADDR,
    USER_ID_ADDR,
    SARADC_CAL,
    TRIM_ADDR,
    PROGRAM_PROTECT_ADDR,
    CPU_ID_ADDR,
    SPECIAL_ADDR,
    CUSTOMER_RESV_ADDR,
};

struct efuse_wr_info {
    unsigned int seg_id;
    unsigned int bytes;
    unsigned int offset;
    unsigned char* buf;
};

#define CMD_READ _IOWR('k', 51, struct efuse_wr_info*)
#define CMD_WRITE _IOWR('k', 52, struct efuse_wr_info*)

struct jz_efuse {
    struct jz_efuse_platform_data* pdata;
    struct device* dev;
    struct miscdevice mdev;
    unsigned int* id2addr;
    spinlock_t lock;
    void __iomem* iomem;
    struct timer_list vddq_protect_timer;
};

static struct jz_efuse* efuse;

static unsigned int efuse_readl(unsigned int reg_off)
{
    return readl(efuse->iomem + reg_off);
}

static void efuse_writel(unsigned int val, unsigned int reg_off)
{
    writel(val, efuse->iomem + reg_off);
}

static void efuse_vddq_set(unsigned long is_on)
{
    /* enable vdd time cannot exceed 1 second */
    if (is_on)
        mod_timer(&efuse->vddq_protect_timer, jiffies + HZ);
    else
        del_timer(&efuse->vddq_protect_timer);

    gpio_set_value(efuse->pdata->gpio_vddq_en_n, is_on ? efuse->pdata->gpio_en_level : !efuse->pdata->gpio_en_level);
}

static int efuse_open(struct inode* inode, struct file* filp)
{
    return 0;
}

static int efuse_release(struct inode* inode, struct file* filp)
{
    /*clear configer register*/
    /*efuse_writel(0, EFUSE_CFG);*/
    return 0;
}

static int jz_efuse_check_arg(unsigned int seg_id, unsigned int bit_num)
{
    if (seg_id == CHIP_ID) {
        if (bit_num > 96) {
            printk("efuse read segment %d data length %d > 96 bit", seg_id, bit_num);
            return -1;
        }
    } else if (seg_id == USER_ID) {
        if (bit_num > 32) {
            printk("efuse read segment %d data length %d > 32 bit", seg_id, bit_num);
            return -1;
        }
    } else if (seg_id == SARADC_CAL_DAT) {
        if (bit_num > 16) {
            printk("efuse read segment %d data length %d > 16 bit", seg_id, bit_num);
            return -1;
        }
    } else if (seg_id == TRIM_DATA) {
        if (bit_num > 8) {
            printk("efuse read segment %d data length %d > 16 bit", seg_id, bit_num);
            return -1;
        }
    } else if (seg_id == PROGRAM_PROTECT) {
        if (bit_num > 8) {
            printk("efuse read segment %d data length %d > 8 bit", seg_id, bit_num);
            return -1;
        }
    } else if (seg_id == CPU_ID) {
        if (bit_num > 16) {
            printk("efuse read segment %d data length %d > 16 bit", seg_id, bit_num);
            return -1;
        }
    } else if (seg_id == SPECIAL_USE) {
        if (bit_num > 16) {
            printk("efuse read segment %d data length %d > 16 bit", seg_id, bit_num);
            return -1;
        }
    } else if (seg_id == CUSTOMER_RESV) {
        if (bit_num > 832) {
            printk("efuse read segment %d data length %d > 832 bit", seg_id, bit_num);
            return -1;
        }
    } else {
        printk("efuse read segment num is error(0 ~ 7)");
        return -1;
    }

    return 0;
}

int jz_efuse_read(unsigned int seg_id, unsigned int r_bytes, unsigned int offset, unsigned char* buf)
{
    unsigned long flags;
    unsigned int val, addr, bit_num, remainder;
    unsigned int count = r_bytes;
    unsigned char* save_buf = buf;
    unsigned int data = 0;
    unsigned int i;

    /* check the bit_num  */
    bit_num = (r_bytes + offset) * 8;
    if (jz_efuse_check_arg(seg_id, bit_num) == -1) {
        printk("efuse arg check error \n");
        return -1;
    }
    spin_lock_irqsave(&efuse->lock, flags);

    /* First word reading */
    addr = (efuse->id2addr[seg_id] + offset) / 4;
    remainder = (efuse->id2addr[seg_id] + offset) % 4;

    efuse_writel(0, EFUSE_STATE);
    val = addr << 21;
    efuse_writel(val, EFUSE_CTRL);
    val |= 1;
    efuse_writel(val, EFUSE_CTRL);
    while (!(efuse_readl(EFUSE_STATE) & 1));

    data = efuse_readl(EFUSE_DATA);

    if ((count + remainder) <= 4) {
        data = data >> (8 * remainder);
        while (count) {
            *(save_buf) = data & 0xff;
            data = data >> 8;
            count--;
            save_buf++;
        }
        goto end;
    } else {
        data = data >> (8 * remainder);
        for (i = 0; i < (4 - remainder); i++) {
            *(save_buf) = data & 0xff;
            data = data >> 8;
            count--;
            save_buf++;
        }
    }

    /* Middle word reading */
again:
    if (count > 4) {
        addr++;
        efuse_writel(0, EFUSE_STATE);

        val = addr << 21;
        efuse_writel(val, EFUSE_CTRL);
        val |= 1;
        efuse_writel(val, EFUSE_CTRL);
        while (!(efuse_readl(EFUSE_STATE) & 1));

        data = efuse_readl(EFUSE_DATA);

        for (i = 0; i < 4; i++) {
            *(save_buf) = data & 0xff;
            data = data >> 8;
            count--;
            save_buf++;
        }

        goto again;
    }

    /* Final word reading */
    addr++;
    efuse_writel(0, EFUSE_STATE);

    val = addr << 21;
    efuse_writel(val, EFUSE_CTRL);
    val |= 1;
    efuse_writel(val, EFUSE_CTRL);
    while (!(efuse_readl(EFUSE_STATE) & 1));

    data = efuse_readl(EFUSE_DATA);

    while (count) {
        *(save_buf) = data & 0xff;
        data = data >> 8;
        count--;
        save_buf++;
    }

    efuse_writel(0, EFUSE_STATE);
end:
    spin_unlock_irqrestore(&efuse->lock, flags);

    return 0;
}
EXPORT_SYMBOL_GPL(jz_efuse_read);

int jz_efuse_write(unsigned int seg_id, unsigned int w_bytes, unsigned int offset, unsigned char* buf)
{
    unsigned long flags;
    unsigned int val, addr, bit_num, remainder;
    unsigned int count = w_bytes;
    unsigned char* save_buf = (unsigned char*)buf;
    unsigned char data[4] = { 0 };
    unsigned int i;

    if (gpio_is_valid(efuse->pdata->gpio_vddq_en_n)) {
        printk("efuse: dont have vdd en gpio error\n");
        return -1;
    }

    bit_num = (w_bytes + offset) * 8;
    if (jz_efuse_check_arg(seg_id, bit_num) == -1) {
        printk("efuse arg check error \n");
        return -1;
    }

    /* First word writing */
    addr = (efuse->id2addr[seg_id] + offset) / 4;
    remainder = (efuse->id2addr[seg_id] + offset) % 4;

    if ((count + remainder) <= 4) {
        for (i = 0; i < remainder; i++)
            data[i] = 0;
        while (count) {
            data[i] = *save_buf;
            save_buf++;
            i++;
            count--;
        }
        while (i < 4) {
            data[i] = 0;
            i++;
        }
        val = data[0] | (data[1] << 8) | (data[2] << 16) | (data[3] << 24);
        efuse_writel(val, EFUSE_DATA);
        val = addr << 21 | 1 << 15;
        efuse_writel(val, EFUSE_CTRL);

        spin_lock_irqsave(&efuse->lock, flags);

        efuse_vddq_set(1);

        udelay(10);
        val |= 2;
        efuse_writel(val, EFUSE_CTRL);
        while (!(efuse_readl(EFUSE_STATE) & 2));

        efuse_vddq_set(0);

        efuse_writel(0, EFUSE_CTRL);
        efuse_writel(0, EFUSE_STATE);

        spin_unlock_irqrestore(&efuse->lock, flags);

        goto end;
    } else {
        for (i = 0; i < remainder; i++)
            data[i] = 0;
        for (i = remainder; i < 4; i++) {
            data[i] = *save_buf;
            save_buf++;
            count--;
        }
        val = data[0] | (data[1] << 8) | (data[2] << 16) | (data[3] << 24);
        efuse_writel(val, EFUSE_DATA);
        val = addr << 21 | 1 << 15;
        efuse_writel(val, EFUSE_CTRL);

        spin_lock_irqsave(&efuse->lock, flags);

        efuse_vddq_set(1);

        udelay(10);
        val |= 2;
        efuse_writel(val, EFUSE_CTRL);
        while (!(efuse_readl(EFUSE_STATE) & 2));

        efuse_vddq_set(0);

        efuse_writel(0, EFUSE_CTRL);
        efuse_writel(0, EFUSE_STATE);

        spin_unlock_irqrestore(&efuse->lock, flags);
    }
    /* Middle word writing */
again:
    if (count > 4) {
        addr++;
        for (i = 0; i < 4; i++) {
            data[i] = *save_buf;
            save_buf++;
            count--;
        }
        val = data[0] | (data[1] << 8) | (data[2] << 16) | (data[3] << 24);
        efuse_writel(val, EFUSE_DATA);
        val = addr << 21 | 1 << 15;
        efuse_writel(val, EFUSE_CTRL);

        spin_lock_irqsave(&efuse->lock, flags);

        efuse_vddq_set(1);

        udelay(10);
        val |= 2;
        efuse_writel(val, EFUSE_CTRL);
        while (!(efuse_readl(EFUSE_STATE) & 2));

        efuse_vddq_set(0);

        efuse_writel(0, EFUSE_CTRL);
        efuse_writel(0, EFUSE_STATE);

        spin_unlock_irqrestore(&efuse->lock, flags);

        goto again;
    }

    /* Final word writing */
    addr++;
    for (i = 0; i < 4; i++) {
        if (count) {
            data[i] = *save_buf;
            save_buf++;
            count--;
        } else {
            data[i] = 0;
        }
    }
    val = data[0] | (data[1] << 8) | (data[2] << 16) | (data[3] << 24);
    efuse_writel(val, EFUSE_DATA);
    val = addr << 21 | 1 << 15;
    efuse_writel(val, EFUSE_CTRL);

    spin_lock_irqsave(&efuse->lock, flags);
    efuse_vddq_set(1);

    udelay(10);
    val |= 2;
    efuse_writel(val, EFUSE_CTRL);
    while (!(efuse_readl(EFUSE_STATE) & 2));

    efuse_vddq_set(0);
    spin_unlock_irqrestore(&efuse->lock, flags);

    efuse_writel(0, EFUSE_CTRL);
    efuse_writel(0, EFUSE_STATE);
end:
    save_buf = NULL;

    return 0;
}
EXPORT_SYMBOL_GPL(jz_efuse_write);

static long efuse_ioctl(struct file* filp, unsigned int cmd, unsigned long arg)
{
    struct efuse_wr_info wr_info;
    int ret = 0;

    switch (cmd) {
    case CMD_READ:
        if(copy_from_user(&wr_info, (void __user *)arg, sizeof(wr_info))){
            ret = -EFAULT;
            printk("CMD_READ: copy_from_user error\n");
            break;
        }
        wr_info.buf = kmalloc(wr_info.bytes, GFP_KERNEL);
        if (!wr_info.buf) {
            printk("CMD_READ: kmalloc error\n");
            return -EFAULT;
        }
        memset(wr_info.buf, 0, wr_info.bytes);
        ret = jz_efuse_read(wr_info.seg_id, wr_info.bytes, wr_info.offset, wr_info.buf);
        if (ret != 0) {
            printk("jz_efuse_read clibration table error\n");
            break;
        }
        if(copy_to_user((void __user *)(((struct efuse_wr_info*)arg)->buf), wr_info.buf, wr_info.bytes)){
            ret = -EFAULT;
            printk("CMD_READ: copy_to_user error\n");
            break;
        }
        kfree(wr_info.buf);
        break;
    case CMD_WRITE:
        if(copy_from_user(&wr_info, (void __user *)arg, sizeof(wr_info))){
            ret = -EFAULT;
            printk("CMD_WRITE: copy_from_user error\n");
            break;
        }
        wr_info.buf = kmalloc(wr_info.bytes, GFP_KERNEL);
        if (!wr_info.buf) {
            printk("CMD_WRITE: kmalloc error\n");
            return -EFAULT;
        }
        memset(wr_info.buf, 0, wr_info.bytes);
        ret = jz_efuse_write(wr_info.seg_id, wr_info.bytes, wr_info.offset, wr_info.buf);
        if (ret != 0) {
            printk("jz_efuse_write clibration table error\n");
            break;
        }
        kfree(wr_info.buf);
        break;
    default:
        ret = -1;
        printk("efuse: no support other cmd\n");
    }
    return ret;
}
static struct file_operations efuse_misc_fops = {
    .open = efuse_open,
    .release = efuse_release,
    .unlocked_ioctl = efuse_ioctl,
};

static int efuse_read_chip_id_proc(struct seq_file* m, void* v)
{
    int len = 0;
    unsigned char buf[12];

    jz_efuse_read(CHIP_ID, 12, 0, (uint8_t*)buf);
    len = seq_printf(m, "--------> chip id: %02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x\n", buf[0], buf[1], buf[2], buf[3], buf[4], buf[5], buf[6], buf[7], buf[8], buf[9], buf[10], buf[11]);

    return len;
}

static int efuse_read_user_id_proc(struct seq_file* m, void* v)
{
    int len = 0;
    unsigned char buf[4];

    jz_efuse_read(USER_ID, 4, 0, (uint8_t*)buf);
    len = seq_printf(m, "--------> user id: %02x%02x%02x%02x\n", buf[0], buf[1], buf[2], buf[3]);

    return len;
}

static int efuse_read_chipID_proc_open(struct inode* inode, struct file* file)
{
    return single_open(file, efuse_read_chip_id_proc, PDE_DATA(inode));
}
static int efuse_read_userID_proc_open(struct inode* inode, struct file* file)
{
    return single_open(file, efuse_read_user_id_proc, PDE_DATA(inode));
}

static const struct file_operations efuse_proc_read_chipID_fops = {
    .read = seq_read,
    .open = efuse_read_chipID_proc_open,
    .write = NULL,
    .llseek = seq_lseek,
    .release = single_release,
};

static const struct file_operations efuse_proc_read_userID_fops = {
    .read = seq_read,
    .open = efuse_read_userID_proc_open,
    .write = NULL,
    .llseek = seq_lseek,
    .release = single_release,
};

static int jz_efuse_probe(struct platform_device* pdev)
{
    int ret = 0;
    struct clk* h2clk;
    struct clk* devclk;
    unsigned long rate;
    unsigned int val, ns;
    int i, rd_strobe, wr_strobe;
    unsigned int rd_adj, wr_adj;
    struct proc_dir_entry *res, *p;

    h2clk = clk_get(NULL, "h2clk");
    if (IS_ERR(h2clk)) {
        printk("efuse: get h2clk fail!\n");
        return -1;
    }

    devclk = clk_get(NULL, "efuse");
    if (IS_ERR(devclk)) {
        printk("efuse: get efuse clk fail!\n");
        return -1;
    }
    clk_enable(devclk);

    rate = clk_get_rate(h2clk);
    ns = 1000000000 / rate;

    efuse = kzalloc(sizeof(struct jz_efuse), GFP_KERNEL);
    if (!efuse) {
        printk("efuse: malloc faile\n");
        return -ENOMEM;
    }

    efuse->pdata = pdev->dev.platform_data;
    if (!efuse->pdata) {
        printk("efuse: no platform data\n");
        ret = -1;
        goto fail_free_efuse;
    }

    efuse->dev = &pdev->dev;

    efuse->iomem = ioremap(EFUSE_IOBASE, 0xfff);
    if (!efuse->iomem) {
        printk("efuse: ioremap failed!\n");
        ret = -EBUSY;
        goto fail_free_efuse;
    }

    if (gpio_is_valid(efuse->pdata->gpio_vddq_en_n)) {
        ret = gpio_request(efuse->pdata->gpio_vddq_en_n, dev_name(efuse->dev));
        if (ret) {
            printk("efuse: failed to request gpio pin: %d\n", ret);
            goto fail_free_io;
        }
        ret = gpio_direction_output(efuse->pdata->gpio_vddq_en_n, !efuse->pdata->gpio_en_level); /* power off by default */
        if (ret) {
            printk("efuse: failed to set gpio as output: %d\n", ret);
            goto fail_free_gpio;
        }

        setup_timer(&efuse->vddq_protect_timer, efuse_vddq_set, 0);
    }

    for (i = 0; i < 0xf; i++)
        if (((i + 1) * ns) > 25)
            break;
    if (i == 0xf) {
        printk("get efuse cfg rd_adj fail!\n");
        ret = -1;
        goto fail_free_gpio;
    }
    rd_adj = i;

    for (i = 0; i < 0xf; i++)
        if (((i + 1) * ns) > 20)
            break;
    if (i == 0xf) {
        printk("get efuse cfg wr_adj fail!\n");
        ret = -1;
        goto fail_free_gpio;
    }
    wr_adj = i;

    for (i = 0; i < 0xf; i++)
        if (((rd_adj + i + 1) * ns) > 20)
            break;
    if (i == 0xf) {
        printk("get efuse cfg rd_strobe fail!\n");
        ret = -1;
        goto fail_free_gpio;
    }
    rd_strobe = i;

    for (i = 1; i < 0xfff; i++) {
        val = (wr_adj + i + 2000) * ns;
        /*if( val > 9800 && val < 10200)*/
        if (val >= 10000)
            break;
    }
    if (i >= 0xfff) {
        printk("get efuse cfg wd_strobe fail!\n");
        ret = -1;
        goto fail_free_gpio;
    }
    wr_strobe = i;

    printk("efuse: rd_adj = %d | rd_strobe = %d | "
                         "wr_adj = %d | wr_strobe = %d\n",
        rd_adj, rd_strobe,
        wr_adj, wr_strobe);
    /*set configer register*/
    /*val = 1 << 31 | rd_adj << 20 | rd_strobe << 16 | wr_adj << 12 | wr_strobe;*/
    val = rd_adj << 20 | rd_strobe << 16 | wr_adj << 12 | wr_strobe;
    efuse_writel(val, EFUSE_CFG);

    clk_put(h2clk);

    spin_lock_init(&efuse->lock);

    efuse->mdev.minor = MISC_DYNAMIC_MINOR;
    efuse->mdev.name = DRV_NAME;
    efuse->mdev.fops = &efuse_misc_fops;

    ret = misc_register(&efuse->mdev);
    if (ret < 0) {
        printk("efuse misc_register failed\n");
        goto fail_free_gpio;
    }
    platform_set_drvdata(pdev, efuse);

    efuse->id2addr = seg_addr;

    p = jz_proc_mkdir("efuse");
    if (!p) {
        printk("create_proc_entry for common efuse failed.\n");
    } else {
        res = proc_create("efuse_chip_id", 0444, p, &efuse_proc_read_chipID_fops);
        if (!res)
            printk("create proc of efuse_chip_id error!!!!\n");

        res = proc_create("efuse_user_id", 0444, p, &efuse_proc_read_userID_fops);
        if (!res)
            printk("create proc of efuse_user_id error!!!!\n");
    }

    printk("ingenic efuse interface module registered success.\n");
    return 0;

fail_free_gpio:
    if (gpio_is_valid(efuse->pdata->gpio_vddq_en_n))
        gpio_free(efuse->pdata->gpio_vddq_en_n);
fail_free_io:
    iounmap(efuse->iomem);
fail_free_efuse:
    kfree(efuse);
    efuse = NULL;
    return ret;
}

static int jz_efuse_remove(struct platform_device* dev)
{
    struct jz_efuse* efuse = platform_get_drvdata(dev);

    misc_deregister(&efuse->mdev);
    if (gpio_is_valid(efuse->pdata->gpio_vddq_en_n)) {
        gpio_free(efuse->pdata->gpio_vddq_en_n);
        del_timer(&efuse->vddq_protect_timer);
    }
    iounmap(efuse->iomem);
    kfree(efuse);
    efuse = NULL;

    return 0;
}

static struct platform_driver jz_efuse_driver = {
    .driver = {
        .name = DRV_NAME,
        .owner = THIS_MODULE,
    },
    .probe = jz_efuse_probe,
    .remove = jz_efuse_remove,
};

static int __init jz_efuse_init(void)
{
    return platform_driver_register(&jz_efuse_driver);
}

static void __exit jz_efuse_exit(void)
{
    platform_driver_unregister(&jz_efuse_driver);
}

module_init(jz_efuse_init);
module_exit(jz_efuse_exit);

MODULE_DESCRIPTION("X1021 efuse driver");
MODULE_LICENSE("GPL v2");
