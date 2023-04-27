#include <linux/kernel.h>
#include <linux/slab.h>
#include <linux/sched.h>
#include <linux/module.h>
#include <linux/delay.h>
#include <linux/i2c.h>
#include <linux/fs.h>
#include <linux/mutex.h>
#include <linux/sched.h>
#include <linux/miscdevice.h>

#define DEVICE_NAME         "sensory_license"

#define READ_STATUS_ADDR    0x10
#define READ_ID_ADDR        0x22
#define WRITE_FEW_REG_ADDR  0x55
#define READ_FEW_REG_ADDR   0x66

#define CMD_SENSORY_LICENSE_STATUS      _IOR('S', 0x11, unsigned char *)
#define CMD_SENSORY_LICENSE_CHIPID      _IOR('S', 0x22, unsigned char *)
#define CMD_SENSORY_LICENSE_WRITE       _IOW('S', 0x33, unsigned char *)
#define CMD_SENSORY_LICENSE_READ        _IOR('S', 0x44, unsigned char *)
#define CMD_SENSORY_LICENSE_CHECK       _IOWR('S', 0x55, unsigned char *)

struct sensory_license_dev {
    struct miscdevice mdev;
    struct mutex mutex;
    struct i2c_client *client;
};

static int sensory_license_read(struct i2c_client *client, char *writebuf,
            int writelen, char *readbuf, int readlen)
{
    int ret;

    if (writelen > 0) {
        struct i2c_msg msgs[] = {
            {
             .addr = client->addr,
             .flags = 0,
             .len = writelen,
             .buf = writebuf,
             },
            {
             .addr = client->addr,
             .flags = I2C_M_RD,
             .len = readlen,
             .buf = readbuf,
             },
        };
        ret = i2c_transfer(client->adapter, msgs, 2);
        if (ret < 0)
            dev_err(&client->dev, "f%s: i2c read error.\n",
                __func__);
    } else {
        struct i2c_msg msgs[] = {
            {
             .addr = client->addr,
             .flags = I2C_M_RD,
             .len = readlen,
             .buf = readbuf,
             },
        };
        ret = i2c_transfer(client->adapter, msgs, 1);
        if (ret < 0)
            dev_err(&client->dev, "%s:i2c read error.\n", __func__);
    }
    return ret;
}

static int sensory_license_write(struct i2c_client *client, char *writebuf, int writelen)
{
    int ret;

    struct i2c_msg msg[] = {
        {
         .addr = client->addr,
         .flags = 0,
         .len = writelen,
         .buf = writebuf,
         },
    };

    ret = i2c_transfer(client->adapter, msg, 1);
    if (ret < 0)
        dev_err(&client->dev, "%s i2c write error.\n", __func__);

    return ret;
}

static long sensory_license_ioctl(struct file *filp, unsigned int cmd, unsigned long arg)
{
    int err = 0;
    char addr;
    char data[10];

    struct miscdevice *mdev = filp->private_data;
    struct sensory_license_dev *sensory_dev = container_of(mdev, struct sensory_license_dev, mdev);

    mutex_lock(&sensory_dev->mutex);
    switch (cmd) {
        case CMD_SENSORY_LICENSE_STATUS:
            addr = READ_STATUS_ADDR;
            err = sensory_license_read(sensory_dev->client, &addr, 1, data, 1);
            if (err < 0) {
                printk("%s read sensory license status failed.\n", __func__);
                goto sensory_license_ioctl_err;
            }

            copy_to_user((void __user *)arg, data, 1);

            break;
        case CMD_SENSORY_LICENSE_CHIPID:
            addr = READ_ID_ADDR;
            err = sensory_license_read(sensory_dev->client, &addr, 1, data, 4);
            if (err < 0) {
                printk("%s read sensory license id failed.\n", __func__);
                goto sensory_license_ioctl_err;
            }

            copy_to_user((void __user *)arg, data, 4);

            break;
        case CMD_SENSORY_LICENSE_WRITE:
            copy_from_user(&data[1], (void __user *)arg, 8);

            data[0] = WRITE_FEW_REG_ADDR;
            err = sensory_license_write(sensory_dev->client, data, 9);
            if (err < 0) {
                printk("%s write sensory license few failed.\n", __func__);
                goto sensory_license_ioctl_err;
            }
            usleep_range(5000,5000);

            break;
        case CMD_SENSORY_LICENSE_READ:
            addr = READ_FEW_REG_ADDR;
            err = sensory_license_read(sensory_dev->client, &addr, 1, data, 8);
            if (err < 0) {
                printk("%s read sensory license few failed.\n", __func__);
                goto sensory_license_ioctl_err;
            }

            copy_to_user((void __user *)arg, data, 8);

            break;
        case CMD_SENSORY_LICENSE_CHECK:
            copy_from_user(&data[1], (void __user *)arg, 8);

            data[0] = WRITE_FEW_REG_ADDR;
            err = sensory_license_write(sensory_dev->client, data, 9);
            if (err < 0) {
                printk("%s write sensory license few failed.\n", __func__);
                goto sensory_license_ioctl_err;
            }
            usleep_range(5000,5000);

            addr = READ_STATUS_ADDR;
            err = sensory_license_read(sensory_dev->client, &addr, 1, data, 1);
            if (err < 0) {
                printk("%s read sensory license status failed.\n", __func__);
                goto sensory_license_ioctl_err;
            }

            if(data[0] == 0x00 || data[0] == 0xFF) {
                printk("%s sensory license wrong state.\n", __func__);
                err = -EIO;
                goto sensory_license_ioctl_err;
            }

            addr = READ_FEW_REG_ADDR;
            err = sensory_license_read(sensory_dev->client, &addr, 1, data, 8);
            if (err < 0) {
                printk("%s read sensory license few failed.\n", __func__);
                goto sensory_license_ioctl_err;
            }
            copy_to_user((void __user *)arg, data, 8);

            break;
        default:
            break;
    }

sensory_license_ioctl_err:
    mutex_unlock(&sensory_dev->mutex);
    return 0;
}

static int sensory_license_open(struct inode *inode, struct file *filp)
{
    return 0;
}

static int sensory_license_release(struct inode *inode, struct file *filp)
{
    return 0;
}

static const struct file_operations sensory_license_fops = {
    .owner =	THIS_MODULE,
    .open       = sensory_license_open,
    .release    = sensory_license_release,
    .unlocked_ioctl = sensory_license_ioctl,
};


static int sensory_license_probe(struct i2c_client *client,
            const struct i2c_device_id *id)
{
    int err;
    struct sensory_license_dev *sensory_dev;

    sensory_dev = kzalloc(sizeof(struct sensory_license_dev), GFP_KERNEL);

    sensory_dev->client = client;
    sensory_dev->mdev.minor = MISC_DYNAMIC_MINOR;
    sensory_dev->mdev.name =  DEVICE_NAME;
    sensory_dev->mdev.fops = &sensory_license_fops;

    err = misc_register(&sensory_dev->mdev);
    if (err < 0) {
        printk("%s misc_register failed.\n", __func__);
        kfree(sensory_dev);
        return err;
    }

    mutex_init(&sensory_dev->mutex);
    i2c_set_clientdata(client, sensory_dev);
    return 0;
}

static int sensory_license_remove(struct i2c_client *client)
{
    struct sensory_license_dev *sensory_dev;
    sensory_dev = i2c_get_clientdata(client);

    misc_deregister(&sensory_dev->mdev);
    mutex_destroy(&sensory_dev->mutex);

    kfree(sensory_dev);
    i2c_set_clientdata(client, NULL);
    return 0;
}

static const struct i2c_device_id sensory_license_ids[] = {
    { DEVICE_NAME, 0 },
    {}
};

static struct i2c_driver sensory_license_driver = {
    .probe  = sensory_license_probe,
    .remove = sensory_license_remove,
    .id_table = sensory_license_ids,
    .driver = {
        .name   = DEVICE_NAME,
        .owner = THIS_MODULE,
    },
};

module_i2c_driver(sensory_license_driver);
MODULE_LICENSE("GPL");
