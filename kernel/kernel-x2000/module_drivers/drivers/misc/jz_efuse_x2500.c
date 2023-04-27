/*
 * linux/drivers/misc/jz_efuse_x2500.c - Ingenic efuse driver
 *
 * Copyright (C) 2012 Ingenic Semiconductor Co., Ltd.
 * Author: Mick <dongyue.ye@ingenic.com>.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */


#include <linux/module.h>
#include <linux/platform_device.h>
#include <linux/slab.h>
#include <linux/kernel.h>
#include <linux/gpio.h>
#include <linux/init.h>
#include <linux/miscdevice.h>
#include <linux/sched.h>
#include <linux/ctype.h>
#include <linux/fs.h>
#include <linux/timer.h>
#include <linux/jiffies.h>
#include <linux/clk.h>
#include <linux/proc_fs.h>
#include <linux/seq_file.h>
#include <linux/of_gpio.h>
#include <linux/of_platform.h>
#include <linux/delay.h>
#include <soc/base.h>
#include <soc/jz_efuse.h>
#include <ingenic_proc.h>

//#define DEBUG
#define DRV_NAME "jz-efuse"

#define EFUSE_CTRL		0x0
#define EFUSE_CFG		0x4
#define EFUSE_STATE		0x8
#define EFUSE_DATA		0xC
#define EFUSE_SPEEN     0x10
#define EFUSE_SPESEG    0x14

#define OFFSET_TO_ADDR(addr,i,read_bytes) (addr+(i*read_bytes))

#define CHIP_ID_ADDR (0x00)
#define USER_ID_ADDR (0x18)
#define SARADC_CAL (0x20)
#define TRIM_ADDR (0x24)
#define PROGRAM_PROTECT_ADDR (0x26)
#define CPU_ID_ADDR (0x28)
#define SPECIAL_ADDR (0x2c)
#define CUSTOMER_RESV_ADDR (0x30)

uint32_t seg_addr[] = {
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
	uint32_t seg_id;
	uint32_t bytes;
	uint32_t offset;
	uint32_t *buf;
};

#define CMD_READ			_IOWR('k', 51, struct efuse_wr_info *)
#define CMD_WRITE			_IOWR('k', 52, struct efuse_wr_info *)
#define CMD_SPEWR			_IOWR('k', 53, struct efuse_wr_info *)
#define CMD_SPERE			_IOWR('k', 54, struct efuse_wr_info *)
#define CMD_GPIO_REQUEST	_IOWR('k', 55, struct efuse_wr_info *)
#define CMD_GPIO_FREE		_IOWR('k', 56, struct efuse_wr_info *)

struct jz_efuse {
	struct jz_efuse_platform_data *pdata;
	struct device *dev;
	struct miscdevice mdev;
	uint32_t *id2addr;
	struct efuse_wr_info *wr_info;
	spinlock_t lock;
	void __iomem *iomem;
	int gpio_vddq_en_n;
	int gpio_vddq_en_n_loop;
	struct timer_list vddq_protect_timer;
};

static struct jz_efuse *efuse;

static uint32_t efuse_readl(uint32_t reg_off)
{
	return readl(efuse->iomem + reg_off);
}

static void efuse_writel(uint32_t val, uint32_t reg_off)
{
	writel(val, efuse->iomem + reg_off);
}

static void efuse_vddq_set(unsigned long is_on)
{
	if (is_on) {
		pr_warn("efuse vdd set -----> \n");
		dump_stack();
		mod_timer(&efuse->vddq_protect_timer, jiffies + HZ);
	}
	if(efuse->gpio_vddq_en_n != -ENODEV)
		gpio_set_value(efuse->gpio_vddq_en_n, !is_on);

	if(efuse->gpio_vddq_en_n_loop != -ENODEV){
		gpio_set_value(efuse->gpio_vddq_en_n_loop, !is_on);
	}
}

static int efuse_open(struct inode *inode, struct file *filp)
{
	return 0;
}

static int efuse_release(struct inode *inode, struct file *filp)
{
	/*clear configer register*/
	/*efuse_writel(0, EFUSE_CFG);*/
	return 0;
}


static int jz_efuse_check_arg(uint32_t seg_id, uint32_t bit_num)
{
	if(seg_id == CHIP_ID) {
		if(bit_num > 96) {
			dev_err(efuse->dev, "read segment %d data length %d > 96 bit", seg_id, bit_num);
			return -1;
		}
	} else if(seg_id == USER_ID) {
		if(bit_num > 32) {
			dev_err(efuse->dev, "read segment %d data length %d > 32 bit", seg_id, bit_num);
			return -1;
		}
	} else if(seg_id == SARADC_CAL_DAT) {
		if(bit_num > 16) {
			dev_err(efuse->dev, "read segment %d data length %d > 16 bit", seg_id, bit_num);
			return -1;
		}
	} else if(seg_id == TRIM_DATA) {
		if(bit_num > 8) {
			dev_err(efuse->dev, "read segment %d data length %d > 16 bit", seg_id, bit_num);
			return -1;
		}
	} else if(seg_id == PROGRAM_PROTECT) {
		if(bit_num > 8) {
			dev_err(efuse->dev, "read segment %d data length %d > 8 bit", seg_id, bit_num);
			return -1;
		}
	} else if(seg_id == CPU_ID) {
		if(bit_num > 16) {
			dev_err(efuse->dev, "read segment %d data length %d > 16 bit", seg_id, bit_num);
			return -1;
		}
	} else if(seg_id == SPECIAL_USE) {
		if(bit_num > 16) {
			dev_err(efuse->dev, "read segment %d data length %d > 16 bit", seg_id, bit_num);
			return -1;
		}
	} else if(seg_id == CUSTOMER_RESV) {
		if(bit_num > 320) {
			dev_err(efuse->dev, "read segment %d data length %d > 320 bit", seg_id, bit_num);
			return -1;
		}
	} else {
		dev_err(efuse->dev, "read segment num is error(0 ~ 7)");
		return -1;
	}

	return 0;

}

int jz_efuse_read(uint32_t seg_id, uint32_t r_bytes, uint32_t offset, uint32_t *buf)
{
	unsigned long flags;
	unsigned int val, addr = 0, bit_num, remainder;
	unsigned int count = r_bytes;
	unsigned char *save_buf = (unsigned char *)buf;
	unsigned int data = 0;
	unsigned int i;


	/* check the bit_num  */

	bit_num = (r_bytes + offset) * 8;
	if (jz_efuse_check_arg(seg_id, bit_num) == -1)
	{
		dev_err(efuse->dev, "efuse arg check error \n");
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

	while(!(efuse_readl(EFUSE_STATE) & 1));
	data = efuse_readl(EFUSE_DATA);

	if ((count + remainder) <= 4) {
		data = data >> (8 * remainder);
		while(count){
			*(save_buf) = data & 0xff;
			data = data >> 8;
			count--;
			save_buf++;
		}
		goto end;
	}else {
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

		while(!(efuse_readl(EFUSE_STATE) & 1));
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

	while(!(efuse_readl(EFUSE_STATE) & 1));
	data = efuse_readl(EFUSE_DATA);

	while(count) {
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

void special_segment_efuse_read(unsigned int* value)
{
	unsigned int val = 0, addr = 0;

	efuse_writel(0, EFUSE_STATE);

	val = 0xa55aa55a;
	efuse_writel(val, EFUSE_SPEEN);

	/* x2500 address needs to be changed,aligned to word */
	addr = SPECIAL_ADDR / 4;
	val = addr << 21;
	efuse_writel(val, EFUSE_CTRL);

	val |= 1;
	efuse_writel(val, EFUSE_CTRL);

	while(!(efuse_readl(EFUSE_STATE) & 0x1));
	*value = efuse_readl(EFUSE_DATA);
}

void special_segment_efuse_write(unsigned int value)
{
	unsigned int val;
	unsigned long flags;
	unsigned int addr = 0;

	efuse_vddq_set(1);
	efuse_writel(0, EFUSE_STATE);

	val = 0xa55aa55a;
	efuse_writel(val, EFUSE_SPEEN);

	efuse_writel(0x20, EFUSE_CTRL);
	val = value;
	efuse_writel(val, EFUSE_DATA);

	/* x2500 address needs to be changed,aligned to word */
	addr = SPECIAL_ADDR / 4;
	val = addr << 21 | 1 << 15 | 1 << 5;
	efuse_writel(val, EFUSE_CTRL);

	spin_lock_irqsave(&efuse->lock, flags);
	//efuse_vddq_set(1);
	udelay(10);

	val |= 1 << 1;
	efuse_writel(val, EFUSE_CTRL);

	while(!(efuse_readl(EFUSE_STATE) & 0x2));

	//efuse_vddq_set(0);
	spin_unlock_irqrestore(&efuse->lock, flags);

	efuse_writel(0, EFUSE_CTRL);
	efuse_writel(0, EFUSE_STATE);
	efuse_writel(0, EFUSE_SPEEN);

	special_segment_efuse_read(&val);
	efuse_vddq_set(0);
}

int jz_efuse_write(uint32_t seg_id, uint32_t w_bytes, uint32_t offset, uint32_t *buf)
{
	unsigned long flags;
	unsigned int val, addr = 0, bit_num, remainder;
	unsigned int count = w_bytes;
	unsigned char *save_buf = (unsigned char *)buf;
	unsigned char data[4] = {0};
	unsigned int i;

	bit_num = (w_bytes + offset) * 8;
	if (jz_efuse_check_arg(seg_id, bit_num) == -1)
	{
		dev_err(efuse->dev, "efuse arg check error \n");
		return -1;
	}

	/* First word writing */
	addr = (efuse->id2addr[seg_id] + offset) / 4;
	remainder = (efuse->id2addr[seg_id] + offset) % 4;

	if ((count + remainder) <= 4) {
		for (i = 0; i < remainder; i++)
			data[i] = 0;
		while(count) {
			data[i] = *save_buf;
			save_buf++;
			i++;
			count--;
		}
		while(i < 4) {
			data[i] = 0;
			i++;
		}
		efuse_writel(0x20, EFUSE_CTRL);
		val = data[0] | (data[1] << 8) | (data[2] << 16) | (data[3] << 24);
		efuse_writel(val, EFUSE_DATA);
		val = addr << 21 | 1 << 15 | 1 << 5;
		efuse_writel(val, EFUSE_CTRL);

		spin_lock_irqsave(&efuse->lock, flags);

		//efuse_vddq_set(1);
		udelay(10);
		val |= 2;
		efuse_writel(val, EFUSE_CTRL);

		while(!(efuse_readl(EFUSE_STATE) & 2));
		//efuse_vddq_set(0);

		efuse_writel(0, EFUSE_CTRL);
		efuse_writel(0, EFUSE_STATE);

		spin_unlock_irqrestore(&efuse->lock, flags);

		goto end;
	}else {
		for (i = 0; i < remainder; i++)
			data[i] = 0;
		for (i = remainder; i < 4; i++) {
			data[i] = *save_buf;
			save_buf++;
			count--;
		}
		efuse_writel(0x20, EFUSE_CTRL);
		val = data[0] | (data[1] << 8) | (data[2] << 16) | (data[3] << 24);
		efuse_writel(val, EFUSE_DATA);
		val = addr << 21 | 1 << 15 | 1 << 5;
		efuse_writel(val, EFUSE_CTRL);

		spin_lock_irqsave(&efuse->lock, flags);

		//efuse_vddq_set(1);
		udelay(10);
		val |= 2;
		efuse_writel(val, EFUSE_CTRL);

		while(!(efuse_readl(EFUSE_STATE) & 2));
		//efuse_vddq_set(0);

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
		efuse_writel(0x20, EFUSE_CTRL);
		val = data[0] | (data[1] << 8) | (data[2] << 16) | (data[3] << 24);
		efuse_writel(val, EFUSE_DATA);
		val = addr << 21 | 1 << 15 | 1 << 5;
		efuse_writel(val, EFUSE_CTRL);

		spin_lock_irqsave(&efuse->lock, flags);

		//efuse_vddq_set(1);
		udelay(10);
		val |= 2;
		efuse_writel(val, EFUSE_CTRL);

		while(!(efuse_readl(EFUSE_STATE) & 2));
		//efuse_vddq_set(0);

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
		}else {
			data[i] = 0;
		}
	}
	efuse_writel(0x20, EFUSE_CTRL);
	val = data[0] | (data[1] << 8) | (data[2] << 16) | (data[3] << 24);
	efuse_writel(val, EFUSE_DATA);
	val = addr << 21 | 1 << 15 | 1 << 5;
	efuse_writel(val, EFUSE_CTRL);

	spin_lock_irqsave(&efuse->lock, flags);
	//efuse_vddq_set(1);
	udelay(10);
	val |= 2;
	efuse_writel(val, EFUSE_CTRL);

	while(!(efuse_readl(EFUSE_STATE) & 2));
	//efuse_vddq_set(0);

	spin_unlock_irqrestore(&efuse->lock, flags);

	efuse_writel(0, EFUSE_CTRL);
	efuse_writel(0, EFUSE_STATE);
end:
	save_buf = NULL;

	return 0;
}
EXPORT_SYMBOL_GPL(jz_efuse_write);

int jz_efuse_gpio_request(void)
{
	int ret = 0;

#ifndef CONFIG_OF_GPIO
	efuse->gpio_vddq_en_n = -ENODEV;
	efuse->gpio_vddq_en_n_loop = -ENODEV;
	efuse->gpio_vddq_en_n = efuse->pdata->gpio_vddq_en_n;
	efuse->gpio_vddq_en_n_loop = efuse->pdata->gpio_vddq_en_n_loop;
#endif

	if (efuse->gpio_vddq_en_n == -ENODEV && efuse->gpio_vddq_en_n_loop == -ENODEV) {
		dev_err(efuse->dev, "gpio error\n");
		ret = -ENODEV;
		return ret;
	}

	if(efuse->gpio_vddq_en_n != -ENODEV){
		ret = devm_gpio_request(efuse->dev, efuse->gpio_vddq_en_n, "gpio_vddq_en_n");
		if (ret) {
			dev_err(efuse->dev, "Failed to request gpio pin: %d\n", ret);
			return ret;
		}
	}
	if(efuse->gpio_vddq_en_n_loop != -ENODEV){
		ret = devm_gpio_request(efuse->dev, efuse->gpio_vddq_en_n_loop, "gpio_vddq_en_n_loop");
		if (ret) {
			dev_err(efuse->dev, "Failed to request gpio pin: %d\n", ret);
			return ret;
		}
	}

	if(efuse->gpio_vddq_en_n != -ENODEV){
		ret = gpio_direction_output(efuse->gpio_vddq_en_n, 1); /* power off by default */
		if (ret) {
			dev_err(efuse->dev, "Failed to set gpio as output: %d\n", ret);
			goto fail_free_gpio;
		}
	}
	if(efuse->gpio_vddq_en_n_loop != -ENODEV){
		ret = gpio_direction_output(efuse->gpio_vddq_en_n_loop, 1); /* power off by default */
		if (ret) {
			dev_err(efuse->dev, "Failed to set gpio as output: %d\n", ret);
			goto fail_free_gpio;
		}
	}
	efuse_vddq_set(1);
	return 0;

fail_free_gpio:
	if (efuse->gpio_vddq_en_n != -ENODEV) {
		gpio_free(efuse->gpio_vddq_en_n);
	}
	if (efuse->gpio_vddq_en_n_loop != -ENODEV) {
		gpio_free(efuse->gpio_vddq_en_n_loop);
	}
	return ret;
}

int jz_efuse_gpio_free(void)
{
	efuse_vddq_set(0);
	if (efuse->gpio_vddq_en_n != -ENODEV) {
		gpio_free(efuse->gpio_vddq_en_n);
	}
	if (efuse->gpio_vddq_en_n_loop != -ENODEV) {
		gpio_free(efuse->gpio_vddq_en_n_loop);
	}
	return 0;
}

static long efuse_ioctl(struct file *filp, unsigned int cmd, unsigned long arg)
{
	struct miscdevice *dev = filp->private_data;
	struct jz_efuse *efuse = container_of(dev, struct jz_efuse, mdev);
	unsigned int * arg_r;
	unsigned int arg_sw = 0;
	unsigned int * arg_sr = NULL;
	int ret = 0;

	arg_r = (unsigned int *)arg;
	arg_sw = (int)arg;
	arg_sr = (int*)arg;
	efuse->wr_info = (struct efuse_wr_info *)arg_r;
	printk("cmd = %x\n",arg);
	dump_stack();
	switch (cmd) {
	case CMD_READ:
		ret = jz_efuse_read(efuse->wr_info->seg_id, efuse->wr_info->bytes, efuse->wr_info->offset, efuse->wr_info->buf);
		break;
	case CMD_WRITE:
		ret = jz_efuse_write(efuse->wr_info->seg_id, efuse->wr_info->bytes, efuse->wr_info->offset, efuse->wr_info->buf);
		break;
	case CMD_SPEWR:
		special_segment_efuse_write(arg_sw);
		break;
	case CMD_SPERE:
		special_segment_efuse_read(arg_sr);
		break;
	case CMD_GPIO_REQUEST:
		jz_efuse_gpio_request();
		break;
	case CMD_GPIO_FREE:
		jz_efuse_gpio_free();
		break;
	default:
		ret = -1;
		printk("no support other cmd\n");
	}
	return ret;
}
static struct file_operations efuse_misc_fops = {
	.open		= efuse_open,
	.release	= efuse_release,
	.unlocked_ioctl	= efuse_ioctl,
};

static int efuse_read_chip_id_proc(struct seq_file *m, void *v)
{
	int len = 12;
	unsigned char buf[12];

	jz_efuse_read(0, len, 0,(uint32_t *)buf);
	seq_printf(m,"%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x\n",buf[0],buf[1],buf[2],buf[3],buf[4],buf[5],buf[6],buf[7],buf[8],buf[9],buf[10],buf[11]);

	return buf;
}

static void efuse_read_user_id_proc(struct seq_file *m, void *v)
{
	int len = 4;
	unsigned char buf[4];

	jz_efuse_read(1, len, 0,(uint32_t *)buf);
	seq_printf(m,"%02x%02x%02x%02x\n",buf[0],buf[1],buf[2],buf[3]);

	return buf;
}

static int efuse_read_chipID_proc_open(struct inode *inode, struct file *file)
{
	return single_open(file, efuse_read_chip_id_proc, PDE_DATA(inode));
}
static int efuse_read_userID_proc_open(struct inode *inode,struct file *file)
{
	return single_open(file, efuse_read_user_id_proc, PDE_DATA(inode));
}

static const struct file_operations efuse_proc_read_chipID_fops ={
	.read = seq_read,
	.open = efuse_read_chipID_proc_open,
	.write = NULL,
	.llseek = seq_lseek,
	.release = single_release,
};

static const struct file_operations efuse_proc_read_userID_fops ={
	.read = seq_read,
	.open = efuse_read_userID_proc_open,
	.write = NULL,
	.llseek = seq_lseek,
	.release = single_release,
};

static int jz_efuse_probe(struct platform_device *pdev)
{
	int ret = 0;
	struct clk *h2clk;
	struct clk *clk_gate;
	unsigned long rate;
	uint32_t val, ps;
	int i, rd_strobe, wr_strobe;
	uint32_t rd_adj, wr_adj;
	struct device_node *np = pdev->dev.of_node;
	enum of_gpio_flags flags;
	struct resource *resource = NULL;
	struct proc_dir_entry *res,*p;

	if (!pdev->dev.platform_data && !pdev->dev.of_node) {
		dev_err(efuse->dev, "no device data specified\n");
		return -ENOENT;
	}

	h2clk = clk_get(NULL, "div_ahb2");
	if (IS_ERR(h2clk)) {
		dev_err(efuse->dev, "get h2clk rate fail!\n");
		return -1;
	}
	rate = clk_get_rate(h2clk);
	ps = 1000000000 / (rate /1000);
	printk("rate = %lu, ps = %d\n", rate, ps);

	efuse= kzalloc(sizeof(struct jz_efuse), GFP_KERNEL);
	if (!efuse) {
		printk("efuse:malloc faile\n");
		return -ENOMEM;
	}

	efuse->gpio_vddq_en_n = -ENODEV;
	efuse->gpio_vddq_en_n_loop = -ENODEV;
#if 0
	if (pdev->dev.of_node) {
		efuse->gpio_vddq_en_n = of_get_named_gpio_flags(np, "ingenic,efuse-en-gpio", 0, &flags);
		if(!gpio_is_valid(efuse->gpio_vddq_en_n)) {
			ret = efuse->gpio_vddq_en_n;
			dev_err(efuse->dev, "gpio_vddq_en_n gpio invalid! %d\n", ret);
			return ret;
		}
#if 1
		efuse->gpio_vddq_en_n_loop = of_get_named_gpio_flags(np, "ingenic,efuse-en-gpio-loop", 0, &flags);
		if(!gpio_is_valid(efuse->gpio_vddq_en_n_loop)) {
			ret = efuse->gpio_vddq_en_n_loop;
			dev_err(efuse->dev, "gpio_vddq_en_n_loop gpio invalid! %d\n", ret);
			return ret;
		}
#endif
	} else {
		efuse->pdata = pdev->dev.platform_data;
		if(!efuse->pdata) {
			dev_err(&pdev->dev, "No platform data\n");
			ret = -1;
			goto fail_free_efuse;
		}
	}
#endif
	efuse->dev = &pdev->dev;
	clk_gate = devm_clk_get(efuse->dev, "gate_efuse");
	if (IS_ERR(clk_gate)) {
		dev_err(efuse->dev, "Cannot get efuse clock\n");
		return ERR_PTR(-ENOENT);
	}
	if(clk_prepare_enable(clk_gate)) {
		dev_err(efuse->dev, "cgu clk error\n");
		return ERR_PTR(-ENOENT);
	}

	resource = platform_get_resource(pdev, IORESOURCE_MEM, 0);
	if (IS_ERR_OR_NULL(resource)) {
		dev_err(&pdev->dev, "get efuse resource failed!\n");
		ret = -ENXIO;
		goto fail_free_efuse;
	}
	efuse->iomem = devm_ioremap(&pdev->dev, resource->start, resource_size(resource));
	if (!efuse->iomem) {
		dev_err(efuse->dev, "ioremap failed!\n");
		ret = -EBUSY;
		goto fail_free_efuse;
	}

	dev_info(efuse->dev, "setup vddq_protect_timer!\n");
	setup_timer(&efuse->vddq_protect_timer, efuse_vddq_set, 0);
	add_timer(&efuse->vddq_protect_timer);

	spin_lock_init(&efuse->lock);

	efuse->mdev.minor = MISC_DYNAMIC_MINOR;
	efuse->mdev.name =  DRV_NAME;
	efuse->mdev.fops = &efuse_misc_fops;

	spin_lock_init(&efuse->lock);

	ret = misc_register(&efuse->mdev);
	if (ret < 0) {
		dev_err(efuse->dev, "misc_register failed\n");
		goto fail_free_gpio;
	}
	platform_set_drvdata(pdev, efuse);


	efuse->id2addr = seg_addr;
	efuse->wr_info = NULL;


	for(i = 0; i < 0xf; i++)
		if((( i + 1) * ps) > 25000)
			break;
	if(i == 0xf) {
		dev_err(efuse->dev, "get efuse cfg rd_adj fail!\n");
		return -1;
	}
	rd_adj = i;

	for(i = 0; i < 0xf; i++)
		if(((i + 1) * ps) > 20000)
			break;
	if(i == 0xf) {
		dev_err(efuse->dev, "get efuse cfg wr_adj fail!\n");
		return -1;
	}
	wr_adj = i;

	for(i = 0; i < 0xf; i++)
		if(((rd_adj + i + 1) * ps) > 20000)
			break;
	if(i == 0xf) {
		dev_err(efuse->dev, "get efuse cfg rd_strobe fail!\n");
		return -1;
	}
	rd_strobe = i;

	for(i = 1; i < 0xfff; i++) {
		val = (wr_adj + i + 2000) * ps;
		if( val > (11000 * 1000) && val < (13000 * 1000))
		/*if(val > 10000)*/
			break;
	}
	if(i >= 0xfff) {
		dev_err(efuse->dev, "get efuse cfg wd_strobe fail!\n");
		return -1;
	}
	wr_strobe = i;

	dev_info(efuse->dev, "rd_adj = %d | rd_strobe = %d | "
		 "wr_adj = %d | wr_strobe = %d\n", rd_adj, rd_strobe,
		 wr_adj, wr_strobe);
	/*set configer register*/
	/*val = 1 << 31 | rd_adj << 20 | rd_strobe << 16 | wr_adj << 12 | wr_strobe;*/
	val = rd_adj << 23 | rd_strobe << 18 | wr_adj << 14 | wr_strobe;
	efuse_writel(val, EFUSE_CFG);

	clk_put(h2clk);

	p = jz_proc_mkdir("efuse");
	if (!p) {
		pr_warning("create_proc_entry for common efuse failed.\n");
		return -ENODEV;
	}

	res = proc_create("efuse_chip_id", 0444,p,&efuse_proc_read_chipID_fops);
	if(!res){
		pr_err("create proc of efuse_chip_id error!!!!\n");
	}

	res = proc_create("efuse_user_id",0444,p,&efuse_proc_read_userID_fops);
	if(!res){
		pr_err("create proc of efuse_user_id error!!!!\n");
	}

	dev_info(efuse->dev, "ingenic efuse interface module registered success.\n");
	return 0;

fail_free_efuse:
	kfree(efuse);
fail_free_gpio:
#if 0
	gpio_free(efuse->gpio_vddq_en_n);
	gpio_free(efuse->gpio_vddq_en_n_loop);
fail_free_io:
#endif
	iounmap(efuse->iomem);
	return ret;
}


static int jz_efuse_remove(struct platform_device *dev)
{
	struct jz_efuse *efuse = platform_get_drvdata(dev);


	misc_deregister(&efuse->mdev);
	if (efuse->gpio_vddq_en_n != -ENODEV) {
		gpio_free(efuse->gpio_vddq_en_n);
		dev_info(efuse->dev, "del vddq_protect_timer!\n");
		del_timer(&efuse->vddq_protect_timer);
	}
	if (efuse->gpio_vddq_en_n_loop != -ENODEV) {
		gpio_free(efuse->gpio_vddq_en_n_loop);
		dev_info(efuse->dev, "del vddq_protect_timer!\n");
		del_timer(&efuse->vddq_protect_timer);
	}
	iounmap(efuse->iomem);
	kfree(efuse);

	return 0;
}

static const struct of_device_id efuse_of_match[] = {
	{ .compatible = "ingenic,x2500-efuse"},
	{ .compatible = "ingenic,t40-efuse"},
	{},
};

static struct platform_driver jz_efuse_driver = {
	.driver = {
		.name = DRV_NAME,
		.owner = THIS_MODULE,
		.of_match_table = of_match_ptr(efuse_of_match),
	},
	.probe		= jz_efuse_probe,
	.remove		= jz_efuse_remove,
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

MODULE_DESCRIPTION("X2500 efuse driver");
MODULE_LICENSE("GPL v2");
