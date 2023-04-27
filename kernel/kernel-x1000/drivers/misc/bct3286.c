/*
 *  BCT3286 LED controller driver
 *
 *  Copyright (C) 2017 Ingenic GaoWei <wei.gao@ingenic.com>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 */

#include <linux/init.h>
#include <linux/module.h>
#include <linux/ioctl.h>
#include <linux/fs.h>
#include <linux/device.h>
#include <linux/err.h>
#include <linux/list.h>
#include <linux/errno.h>
#include <linux/mutex.h>
#include <linux/slab.h>
#include <linux/compat.h>
#include <linux/delay.h>
#include <linux/gpio.h>
#include <soc/gpio.h>

#include <linux/miscdevice.h>
#include <linux/spi/spi.h>
#include <linux/spi/bct3286.h>

/* #define DEBUG */
#define DRV_NAME "led"
#define LIGHT_RGB_COUNT 12
#define LIGHT_COUNT (LIGHT_RGB_COUNT * 3)

static unsigned char normal_mode[]={0x14, 0x00};
static unsigned char rgb_mode[]={0x14, 0x0e};
static unsigned char rgb_brightness[]={0x16, 0x0d};

static unsigned char leds_reg[] = {
	0x00, 0,
	0x01, 0,
	0x02, 0,
	0x03, 0,
	0x04, 0,
	0x05, 0,
};

struct bct3286_data {
	struct miscdevice mdev;
	spinlock_t spi_lock;
	struct spi_device *spi;
	struct bct3286_board_info *info;
	char brightness;
};

static struct bct3286_data *bct3286 = NULL;

/*-------------------------------------------------------------------------*/

static void bct3286_complete(void *arg)
{
	complete(arg);
}

static ssize_t
bct3286_sync(struct bct3286_data *bct3286, struct spi_message *message)
{
	DECLARE_COMPLETION_ONSTACK(done);
	int status;

	message->complete = bct3286_complete;
	message->context = &done;

	if (bct3286->spi == NULL)
		status = -ESHUTDOWN;
	else
		status = spi_async(bct3286->spi, message);

	if (status == 0) {
		wait_for_completion(&done);
		status = message->status;
		if (status == 0)
			status = message->actual_length;
	}
	return status;
}

static inline ssize_t
bct3286_sync_write(struct bct3286_data *bct3286, unsigned char *buf, size_t len)
{
	struct spi_transfer t = {
			.tx_buf	= (void*)buf,
			.len	= (unsigned)len,
		};
	struct spi_message m;

#ifdef DEBUG
	printk("reg:%02x >> %02x\n", buf[0], buf[1]);
#endif

	spi_message_init(&m);
	spi_message_add_tail(&t, &m);
	return bct3286_sync(bct3286, &m);
}

static inline ssize_t
bct3286_sync_read(struct bct3286_data *bct3286, size_t len)
{
	return 0;
}

/*-------------------------------------------------------------------------*/

static ssize_t
bct3286_read(struct file *filp, char __user *buf, size_t count, loff_t *f_pos)
{
	return count;
}

static ssize_t bct3286_update(unsigned char *buffer, char brightness)
{
	ssize_t retval = 0, i, j;
	unsigned char led_status[LIGHT_COUNT] = {0};

	if (brightness > -1)
		bct3286->brightness = brightness;

	if (buffer) {
		for (i=0; i<LIGHT_COUNT; i++) {
			led_status[i] = buffer[bct3286->info->order[i]];
		}

		for (i=0,j=0; i<LIGHT_COUNT; i+=LIGHT_COUNT/3, j++) {
			leds_reg[j*4+1] = led_status[i+4]/255 << 0 |
				led_status[i+5]/255 << 1 |
				led_status[i+6]/255 << 2 |
				led_status[i+7]/255 << 3 |
				led_status[i+8]/255 << 4 |
				led_status[i+9]/255 << 5 |
				led_status[i+10]/255 << 6 |
				led_status[i+11]/255 << 7;
			leds_reg[j*4+3] = led_status[i+0]/255 << 4 |
				led_status[i+1]/255 << 5 |
				led_status[i+2]/255 << 6 |
				led_status[i+3]/255 << 7 |
				bct3286->brightness;
		}
	} else {
		for (j=0; j<3; j++)
			leds_reg[j*4+3] &= ~((unsigned char)0xf) | bct3286->brightness;
	}

	for (i=0; i<12; i+=2) {
		retval = bct3286_sync_write(bct3286, &leds_reg[i], 2);
	}

	return retval;
}

static ssize_t bct3286_update_rgb(unsigned char *buffer, char brightness)
{
	ssize_t retval = 0, i;
	unsigned char led_status[LIGHT_RGB_COUNT] = {0};

	if (buffer) {
		for (i=0; i<LIGHT_RGB_COUNT; i++) {
			led_status[i] = buffer[bct3286->info->rgb_order[i]];
		}

		for (i=0; i<LIGHT_RGB_COUNT / 4; i++) {
			leds_reg[i*4+1] = led_status[i*4+3] << 4 | led_status[i*4+2];
			leds_reg[i*4+3] = led_status[i*4+1] << 4 | led_status[i*4];
		}

		for (i=0; i<12; i+=2) {
			retval = bct3286_sync_write(bct3286, &leds_reg[i], 2);
		}
	}

	if (brightness > -1) {
		rgb_brightness[1] = 0x0c | brightness;
		retval = bct3286_sync_write(bct3286, rgb_brightness, 2);
	}

	return retval;
}

static ssize_t
bct3286_write(struct file *filp, const char __user *buf,
		size_t count, loff_t *f_pos)
{
	unsigned long missing;
	unsigned char buffer[LIGHT_COUNT + 1] = {0};

	missing = copy_from_user(buffer, buf, count);
	if (missing == 0) {
		if (count == LIGHT_COUNT) {
			bct3286_update(buffer, -1);
			bct3286_sync_write(bct3286, normal_mode, 2);
		} else if (count == LIGHT_COUNT + 1) {
			bct3286_update(buffer, buffer[LIGHT_COUNT]);
			bct3286_sync_write(bct3286, normal_mode, 2);
		} else if (count == LIGHT_RGB_COUNT) {
			bct3286_update_rgb(buffer, -1);
			bct3286_sync_write(bct3286, rgb_mode, 2);
		} else if (count == LIGHT_RGB_COUNT + 1) {
			bct3286_update_rgb(buffer, buffer[LIGHT_RGB_COUNT]);
			bct3286_sync_write(bct3286, rgb_mode, 2);
		}
		else
			return -EMSGSIZE;
	}

	return count;
}

static long bct3286_ioctl(struct file *filp, unsigned int cmd, unsigned long arg)
{
	int ret = 0;

	/* switch (cmd) { */
	/* 	case cmd_get_chipid: */
	/* 	default: */
	/* } */

	return ret;
}
static int bct3286_open(struct inode *inode, struct file *filp)
{
	int retval = -ENXIO;
	struct spi_device *spi;

	/* nonseekable_open(inode, filp); */

	spi = spi_dev_get(bct3286->spi);
	if (spi == NULL)
		return -ESHUTDOWN;

	spi->bits_per_word = 8;
	spi->max_speed_hz = 100000;

	retval = spi_setup(spi);

	spi_dev_put(spi);

	// start up, normal mode.
	bct3286_sync_write(bct3286, rgb_brightness, 2);
	return retval;
}

static int bct3286_release(struct inode *inode, struct file *filp)
{
	return 0;
}

static const struct file_operations bct3286_fops = {
	.owner =	THIS_MODULE,
	.write =	bct3286_write,
	.read =		bct3286_read,
	.unlocked_ioctl = bct3286_ioctl,
	.open =		bct3286_open,
	.release =	bct3286_release,
};

static void bct3286_reset(struct bct3286_data *bct3286)
{
	gpio_direction_output(bct3286->info->rst, 1);
	msleep(1);
	gpio_direction_output(bct3286->info->rst, 0);
	msleep(1);
	gpio_direction_output(bct3286->info->rst, 1);
}

static int bct3286_probe(struct spi_device *spi)
{
	int ret = 0;

	/* Allocate driver data */
	bct3286 = kzalloc(sizeof(*bct3286), GFP_KERNEL);
	if (!bct3286)
		return -ENOMEM;

	/* Initialize the driver data */
	bct3286->spi = spi;
	bct3286->info = spi->dev.platform_data;
	spin_lock_init(&bct3286->spi_lock);

	bct3286->mdev.minor = MISC_DYNAMIC_MINOR;
	bct3286->mdev.name =  DRV_NAME;
	bct3286->mdev.fops = &bct3286_fops;

	ret = misc_register(&bct3286->mdev);
	if (ret < 0) {
		dev_err(&spi->dev, "misc_register failed\n");
		return ret;
	}

	spi_set_drvdata(spi, bct3286);

	gpio_request(bct3286->info->rst, "bct3286_rst");
	bct3286_reset(bct3286);
	bct3286->brightness = 0x7;

	return ret;
}

static int bct3286_remove(struct spi_device *spi)
{
	/* make sure ops on existing fds can abort cleanly */
	spin_lock_irq(&bct3286->spi_lock);
	bct3286->spi = NULL;
	spi_set_drvdata(spi, NULL);
	spin_unlock_irq(&bct3286->spi_lock);

	kfree(bct3286);

	return 0;
}

static struct spi_driver bct3286_spi_driver = {
	.driver = {
		.name =		"bct3286",
		.owner =	THIS_MODULE,
	},
	.probe =	bct3286_probe,
	.remove =	bct3286_remove,
};

/*-------------------------------------------------------------------------*/

static int __init bct3286_init(void)
{
	return spi_register_driver(&bct3286_spi_driver);
}

static void __exit bct3286_exit(void)
{
	spi_unregister_driver(&bct3286_spi_driver);
}

module_init(bct3286_init);
module_exit(bct3286_exit);

MODULE_DESCRIPTION("BCT3286 Driver");
MODULE_LICENSE("GPL");
MODULE_ALIAS("spi:bct3286");
