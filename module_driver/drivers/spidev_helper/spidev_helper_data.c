#include <linux/init.h>
#include <linux/module.h>
#include <linux/kernel.h>

#include <linux/gpio.h>
#include <soc/gpio.h>
#include <linux/spi/spi.h>
#include <linux/delay.h>
#include <linux/io.h>
#include <linux/miscdevice.h>
#include <linux/device.h>
#include <linux/ctype.h>
#include <linux/fs.h>
#include <asm/device.h>

#include <linux/string.h>
#include <linux/seq_file.h>
#include <linux/sched.h>
#include <linux/slab.h>

#include <utils/gpio.h>
#include <utils/spi.h>

struct spidev_register_data {
    int busnum;
    char *cs_gpio;
    char spidev_path[20];
};

#define SPI_ADD_DEVICE                        _IOWR('s', 200, struct spidev_register_data *)
#define SPI_DEL_DEVICE                        _IOWR('s', 201, char *)


static int spidev_open(struct inode *inode, struct file *filp)
{
    return 0;
}

static int spidev_close(struct inode *inode, struct file *filp)
{
    return 0;
}

static int spidev_to_spi(char *name)
{
    int len;

    len = strlen("/dev/spidev");

    if (strncmp(name, "/dev/spidev", len))
        return -1;

    sprintf(name, "spi%s", &name[len]);

    return 0;
}

static int check_arg(char *arg)
{
    char check;
    return copy_from_user(&check, arg, 1);
}

static long spidev_helper_ioctl(struct file *filp, unsigned int cmd, unsigned long arg)
{
    int gpio;
    int ret = 0;
    char dev_name[20];
    struct device *dev;
    struct spi_device *spi;
    struct spidev_register_data *data;

    struct spi_board_info spidev = {
        .modalias = "spidev",
        .max_speed_hz = 1*1000*1000,
        .mode = SPI_MODE_0,
    };

    if (check_arg((char*)arg))
        return -1;

    switch (cmd) {
    case SPI_ADD_DEVICE:
        data = (struct spidev_register_data *)arg;
        if (check_arg(data->cs_gpio)) {
            ret = -1;
            break;
        }

        gpio = str_to_gpio(data->cs_gpio);
        if (gpio == -EINVAL) {
            printk(KERN_ERR "spidev_helper: gpio is invalid:%s !\n", data->cs_gpio);
            ret = -1;
            break;
        }

        spidev.controller_data = (void *)gpio;

        spi = spi_register_device(&spidev, data->busnum);
        if (spi == NULL) {
            ret = -1;
            break;
        }

        sprintf(data->spidev_path, "/dev/spidev%d.%d", data->busnum, spi->chip_select);

        break;
    case SPI_DEL_DEVICE:
        strcpy(dev_name, (char *)arg);
        ret = spidev_to_spi(dev_name);
        if (ret < 0)
            break;

        dev = bus_find_device_by_name(&spi_bus_type, NULL, dev_name);
        if (dev == NULL) {
            ret = -1;
            break;
        }

        spi = to_spi_device(dev);

        spi_unregister_device(spi);
        spi->dev.release(&spi->dev);

        break;
    default:
        printk(KERN_ERR "SPI:no support this cmd:%d\n", cmd);
        ret = -1;
        break;
    }

    return ret;
}

static struct file_operations spidev_misc_fops = {
    .open = spidev_open,
    .release = spidev_close,
    .unlocked_ioctl = spidev_helper_ioctl,
};

static struct miscdevice mdev = {
    .minor = MISC_DYNAMIC_MINOR,
    .name = "spidev_helper",
    .fops = &spidev_misc_fops,
};

static int spidev_helper_init(void)
{
    int ret;

    ret = misc_register(&mdev);
    BUG_ON(ret < 0);

    return 0;
}

static void spidev_helper_exit(void)
{
    misc_deregister(&mdev);
}

module_init(spidev_helper_init);
module_exit(spidev_helper_exit);

MODULE_DESCRIPTION("spidev helper module");
MODULE_LICENSE("GPL");