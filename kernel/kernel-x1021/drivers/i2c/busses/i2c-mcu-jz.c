/*
 * I2C bus driver using the MCU
 *
 * Copyright (C) 2018 Ingenic Semiconductor Co., Ltd.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */
#include <linux/i2c.h>
#include <linux/i2c-algo-bit.h>
#include <linux/i2c-gpio.h>
#include <linux/init.h>
#include <linux/module.h>
#include <linux/slab.h>
#include <linux/platform_device.h>
#include <linux/of_gpio.h>
#include <linux/interrupt.h>
#include <linux/delay.h>
#include <mach/jzmcu.h>

extern int firmware_loaded;

struct i2c_mcu {
	void __iomem *iomem;
	int irq;
	struct i2c_adapter adap;
	struct completion complete;
	unsigned int sda_pin;
	unsigned int scl_pin;
	int udelay;
};

static irqreturn_t i2c_mcu_irq(int irqno, void *dev_id)
{
	struct i2c_mcu *i2c = dev_id;

	complete(&i2c->complete);

	return IRQ_HANDLED;
}

/* --- setting states on the bus with the right timing: ---------------	*/
static void setsda(struct i2c_mcu *i2c, int state)
{
	if (state)
		gpio_direction_input(i2c->sda_pin);
	else
		gpio_direction_output(i2c->sda_pin, 0);
}

static void setscl(struct i2c_mcu *i2c, int state)
{
	if (state)
		gpio_direction_input(i2c->scl_pin);
	else
		gpio_direction_output(i2c->scl_pin, 0);
}

static int getsda(struct i2c_mcu *i2c)
{
	return gpio_get_value(i2c->sda_pin);
}

static inline void sdalo(struct i2c_mcu *i2c)
{
	setsda(i2c, 0);
	udelay((i2c->udelay + 1) / 2);
}

static inline void sdahi(struct i2c_mcu *i2c)
{
	setsda(i2c, 1);
	udelay((i2c->udelay + 1) / 2);
}

static inline void scllo(struct i2c_mcu *i2c)
{
	setscl(i2c, 0);
	udelay(i2c->udelay / 2);
}

static int sclhi(struct i2c_mcu *i2c)
{
	setscl(i2c, 1);
	udelay(i2c->udelay);
	return 0;
}

static void i2c_start(struct i2c_mcu *i2c)
{
	setsda(i2c, 0);
	udelay(i2c->udelay);
	scllo(i2c);
}

static void i2c_repstart(struct i2c_mcu *i2c)
{
	sdahi(i2c);
	sclhi(i2c);
	setsda(i2c, 0);
	udelay(i2c->udelay);
	scllo(i2c);
}


static void i2c_stop(struct i2c_mcu *i2c)
{
	sdalo(i2c);
	sclhi(i2c);
	setsda(i2c, 1);
	udelay(i2c->udelay);
}

static int i2c_outb(struct i2c_adapter *adap, unsigned char c)
{
	int i;
	int sb;
	int ack;
	struct i2c_mcu *i2c = adap->algo_data;

	for (i = 7; i >= 0; i--) {
		sb = (c >> i) & 1;
		setsda(i2c, sb);
		udelay((i2c->udelay + 1) / 2);
		if (sclhi(i2c) < 0) {
			dev_dbg(&adap->dev, "i2c_outb: 0x%02x, "
				"timeout at bit #%d\n", (int)c, i);
			return -ETIMEDOUT;
		}
		scllo(i2c);
	}
	sdahi(i2c);
	if (sclhi(i2c) < 0) {
		dev_dbg(&adap->dev, "i2c_outb: 0x%02x, "
			"timeout at ack\n", (int)c);
		return -ETIMEDOUT;
	}

	ack = !getsda(i2c);
	dev_dbg(&adap->dev, "i2c_outb: 0x%02x %s\n", (int)c,
		ack ? "A" : "NA");

	scllo(i2c);
	return ack;
}

static int i2c_inb(struct i2c_adapter *adap)
{
	int i;
	unsigned char indata = 0;
	struct i2c_mcu *i2c = adap->algo_data;

	sdahi(i2c);
	for (i = 0; i < 8; i++) {
		if (sclhi(i2c) < 0) {
			dev_dbg(&adap->dev, "i2c_inb: timeout at bit "
				"#%d\n", 7 - i);
			return -ETIMEDOUT;
		}
		indata *= 2;
		if (getsda(i2c))
			indata |= 0x01;
		setscl(i2c, 0);
		udelay(i == 7 ? i2c->udelay / 2 : i2c->udelay);
	}
	return indata;
}

static int try_address(struct i2c_adapter *adap,
		       unsigned char addr, int retries)
{
	struct i2c_mcu *i2c = adap->algo_data;
	int i, ret = 0;

	for (i = 0; i <= retries; i++) {
		ret = i2c_outb(adap, addr);
		if (ret == 1 || i == retries)
			break;
		dev_dbg(&adap->dev, "emitting stop condition\n");
		i2c_stop(i2c);
		udelay(i2c->udelay);
		dev_dbg(&adap->dev, "emitting start condition\n");
		i2c_start(i2c);
	}
	if (i && ret)
		dev_dbg(&adap->dev, "Used %d tries to %s client at "
			"0x%02x: %s\n", i + 1,
			addr & 1 ? "read from" : "write to", addr >> 1,
			ret == 1 ? "success" : "failed, timeout?");
	return ret;
}

static int sendbytes(struct i2c_adapter *adap, struct i2c_msg *msg)
{
	const unsigned char *temp = msg->buf;
	int count = msg->len;
	unsigned short nak_ok = msg->flags & I2C_M_IGNORE_NAK;
	int retval;
	int wrcount = 0;

	while (count > 0) {
		retval = i2c_outb(adap, *temp);

		if ((retval > 0) || (nak_ok && (retval == 0))) {
			count--;
			temp++;
			wrcount++;
		} else if (retval == 0) {
			dev_err(&adap->dev, "sendbytes: NAK bailout.\n");
			return -EIO;
		} else {
			dev_err(&adap->dev, "sendbytes: error %d\n",
					retval);
			return retval;
		}
	}
	return wrcount;
}

static int acknak(struct i2c_adapter *adap, int is_ack)
{
	struct i2c_mcu *i2c = adap->algo_data;

	if (is_ack)
		setsda(i2c, 0);
	udelay((i2c->udelay + 1) / 2);
	if (sclhi(i2c) < 0) {
		dev_err(&adap->dev, "readbytes: ack/nak timeout\n");
		return -ETIMEDOUT;
	}
	scllo(i2c);
	return 0;
}

static int readbytes(struct i2c_adapter *adap, struct i2c_msg *msg)
{
	int inval;
	int rdcount = 0;
	unsigned char *temp = msg->buf;
	int count = msg->len;
	const unsigned flags = msg->flags;

	while (count > 0) {
		inval = i2c_inb(adap);
		if (inval >= 0) {
			*temp = inval;
			rdcount++;
		} else {
			break;
		}

		temp++;
		count--;

		if (rdcount == 1 && (flags & I2C_M_RECV_LEN)) {
			if (inval <= 0 || inval > I2C_SMBUS_BLOCK_MAX) {
				if (!(flags & I2C_M_NO_RD_ACK))
					acknak(adap, 0);
				dev_err(&adap->dev, "readbytes: invalid "
					"block length (%d)\n", inval);
				return -EPROTO;
			}
			count += inval;
			msg->len += inval;
		}

		dev_dbg(&adap->dev, "readbytes: 0x%02x %s\n",
			inval,
			(flags & I2C_M_NO_RD_ACK)
				? "(no ack/nak)"
				: (count ? "A" : "NA"));

		if (!(flags & I2C_M_NO_RD_ACK)) {
			inval = acknak(adap, count);
			if (inval < 0)
				return inval;
		}
	}
	return rdcount;
}

static int bit_doAddress(struct i2c_adapter *adap, struct i2c_msg *msg)
{
	unsigned short flags = msg->flags;
	unsigned short nak_ok = msg->flags & I2C_M_IGNORE_NAK;
	struct i2c_mcu *i2c = adap->algo_data;

	unsigned char addr;
	int ret, retries;

	retries = nak_ok ? 0 : adap->retries;

	if (flags & I2C_M_TEN) {
		addr = 0xf0 | ((msg->addr >> 7) & 0x06);
		dev_dbg(&adap->dev, "addr0: %d\n", addr);
		ret = try_address(adap, addr, retries);
		if ((ret != 1) && !nak_ok)  {
			dev_err(&adap->dev,
				"died at extended address code\n");
			return -ENXIO;
		}
		ret = i2c_outb(adap, msg->addr & 0xff);
		if ((ret != 1) && !nak_ok) {
			dev_err(&adap->dev, "died at 2nd address code\n");
			return -ENXIO;
		}
		if (flags & I2C_M_RD) {
			dev_dbg(&adap->dev, "emitting repeated "
				"start condition\n");
			i2c_repstart(i2c);
			addr |= 0x01;
			ret = try_address(adap, addr, retries);
			if ((ret != 1) && !nak_ok) {
				dev_err(&adap->dev,
					"died at repeated address code\n");
				return -EIO;
			}
		}
	} else {
		addr = msg->addr << 1;
		if (flags & I2C_M_RD)
			addr |= 1;
		if (flags & I2C_M_REV_DIR_ADDR)
			addr ^= 1;
		ret = try_address(adap, addr, retries);
		if ((ret != 1) && !nak_ok)
			return -ENXIO;
	}

	return 0;
}

static int i2c_mcu_xfer(struct i2c_adapter *adap,
		    struct i2c_msg *msg, int count)
{
	struct i2c_mcu *i2c = adap->algo_data;
	int i, ret;

	if(firmware_loaded) {
		volatile struct mailbox_pend_addr_s *mailbox_pend_addr =
			(volatile struct mailbox_pend_addr_s *)(i2c->iomem + PEND);

		writel(i2c->sda_pin, i2c->iomem + ARGS + 0x10);
		writel(i2c->scl_pin, i2c->iomem + ARGS + 0x14);
		writel(i2c->udelay, i2c->iomem + ARGS + 0x18);
		writel(i2c->adap.retries, i2c->iomem + ARGS + 0x1c);

		writel(count, i2c->iomem + ARGS + 0x20);
		for(i = 0; i < count; i++, msg++) {
			writew(msg->addr, i2c->iomem + ARGS + 0x24 + 12 * i);
			writew(msg->flags, i2c->iomem + ARGS + 0x26 + 12 * i);
			writew(msg->len, i2c->iomem + ARGS + 0x28 + 12 * i);
			writel(virt_to_phys(msg->buf), i2c->iomem + ARGS + 0x2c + 12 * i);
			dma_cache_wback_inv((unsigned long)(msg->buf), msg->len);
		}

		mailbox_pend_addr->irq_mask &=
			~(1 << (i2c->adap.nr + IRQ_PEND_I2C0 - IRQ_MCU_BASE));
		mailbox_pend_addr->irq_state |=
			(1 << (i2c->adap.nr + IRQ_PEND_I2C0 - IRQ_MCU_BASE));
		writel(0xFFFFFFFF, i2c->iomem + DMNMB);

		wait_for_completion_timeout(&i2c->complete, 3 * HZ);
		mdelay(1);

		ret = readl(i2c->iomem + ARGS + 0x20);
	} else {
		unsigned short nak_ok;

		dev_dbg(&adap->dev, "emitting start condition\n");
		i2c_start(i2c);
		for (i = 0; i < count; i++, msg++) {
			nak_ok = msg->flags & I2C_M_IGNORE_NAK;
			if (!(msg->flags & I2C_M_NOSTART)) {
				if (i) {
					dev_dbg(&adap->dev, "emitting "
						"repeated start condition\n");
					i2c_repstart(i2c);
				}
				ret = bit_doAddress(adap, msg);
				if ((ret != 0) && !nak_ok) {
					dev_dbg(&adap->dev, "NAK from "
						"device addr 0x%02x msg #%d\n",
						msg->addr, i);
					goto bailout;
				}
			}
			if (msg->flags & I2C_M_RD) {
				ret = readbytes(adap, msg);
				if (ret >= 1)
					dev_dbg(&adap->dev, "read %d byte%s\n",
						ret, ret == 1 ? "" : "s");
				if (ret < msg->len) {
					if (ret >= 0)
						ret = -EIO;
					goto bailout;
				}
			} else {
				ret = sendbytes(adap, msg);
				if (ret >= 1)
					dev_dbg(&adap->dev, "wrote %d byte%s\n",
						ret, ret == 1 ? "" : "s");
				if (ret < msg->len) {
					if (ret >= 0)
						ret = -EIO;
					goto bailout;
				}
			}
		}
		ret = i;

bailout:
		dev_dbg(&adap->dev, "emitting stop condition\n");
		i2c_stop(i2c);
	}

	return ret;
}

static u32 i2c_mcu_functionality(struct i2c_adapter *adap)
{
	unsigned int ret;

	ret = I2C_FUNC_I2C | I2C_FUNC_SMBUS_EMUL | I2C_FUNC_10BIT_ADDR;

	return ret;
}

static const struct i2c_algorithm i2c_mcu_algorithm = {
	.master_xfer = i2c_mcu_xfer,
	.functionality = i2c_mcu_functionality,
};

static int i2c_mcu_probe(struct platform_device *pdev)
{
	int ret = 0;
	struct i2c_mcu *i2c;
	struct resource *res;
	struct i2c_gpio_platform_data *pdata;

	if (!pdev->dev.platform_data)
		return -ENXIO;

	i2c = kzalloc(sizeof(struct i2c_mcu), GFP_KERNEL);
	if (!i2c) {
		dev_err(&i2c->adap.dev, "Error: Now we can not malloc memory for I2C!\n");
		ret = -ENOMEM;
		goto ERR0;
	}

	i2c->adap.owner = THIS_MODULE;
	i2c->adap.algo = &i2c_mcu_algorithm;
	i2c->adap.retries = 5;
	i2c->adap.timeout = 5;
	i2c->adap.algo_data = i2c;
	i2c->adap.dev.parent = &pdev->dev;
	i2c->adap.nr = pdev->id;
	sprintf(i2c->adap.name, "i2c%u", pdev->id);

	pdata = pdev->dev.platform_data;
	i2c->sda_pin = pdata->sda_pin;
	i2c->scl_pin = pdata->scl_pin;
	i2c->udelay = 5; // 100kHz

	ret = gpio_request(i2c->sda_pin, "sda");
	if (ret) {
		if (ret == -EINVAL)
			ret = -EPROBE_DEFER;	/* Try again later */
		goto err_request_sda;
	}
	ret = gpio_request(i2c->scl_pin, "scl");
	if (ret) {
		if (ret == -EINVAL)
			ret = -EPROBE_DEFER;	/* Try again later */
		goto err_request_scl;
	}

	res = platform_get_resource(pdev, IORESOURCE_MEM, 0);
	i2c->iomem = ioremap(res->start, resource_size(res));
	if (!i2c->iomem) {
		dev_err(&i2c->adap.dev, "Error: Now we can remap IO for I2C%d!\n", pdev->id);
		ret = -ENOMEM;
		goto io_failed;
	}

	i2c->irq = platform_get_irq(pdev, 0);
	ret = request_irq(i2c->irq, i2c_mcu_irq, IRQF_DISABLED, i2c->adap.name, i2c);
	if (ret) {
		dev_err(&i2c->adap.dev, "Error: Now we can request irq for I2C%d!\n", pdev->id);
		ret = -ENODEV;
		goto irq_failed;
	}

	init_completion(&i2c->complete);

	ret = i2c_add_numbered_adapter(&i2c->adap);
	if (ret < 0) {
		dev_err(&(i2c->adap.dev), KERN_INFO "I2C: Failed to add bus\n");
		goto adapt_failed;
	}

	platform_set_drvdata(pdev, i2c);
	dev_info(&i2c->adap.dev, "mcu i2c%d initialized\n", pdev->id);

	return 0;

adapt_failed:
	free_irq(i2c->irq, i2c);
irq_failed:
	iounmap(i2c->iomem);
io_failed:
	gpio_free(i2c->scl_pin);
err_request_scl:
	gpio_free(i2c->sda_pin);
err_request_sda:
	kfree(i2c);
ERR0:
	return ret;
}

static int i2c_mcu_remove(struct platform_device *pdev)
{
	struct i2c_mcu *i2c = platform_get_drvdata(pdev);

	free_irq(i2c->irq, i2c);
	iounmap(i2c->iomem);
	i2c_del_adapter(&i2c->adap);
	kfree(i2c);

	return 0;
}

static struct platform_driver i2c_mcu_driver = {
	.driver		= {
		.name	= "i2c-mcu",
		.owner	= THIS_MODULE,
	},
	.probe		= i2c_mcu_probe,
	.remove		= i2c_mcu_remove,
};

static int __init i2c_mcu_init(void)
{
	return platform_driver_register(&i2c_mcu_driver);
}
rootfs_initcall(i2c_mcu_init);

static void __exit i2c_mcu_exit(void)
{
	platform_driver_unregister(&i2c_mcu_driver);
}
module_exit(i2c_mcu_exit);

