/*
 * x1000 gpio open_short test
 *
 * Copyright (c) 2015 Ingenic Semiconductor Co.,Ltd
 * Author: wczhu <wczhu@ingenic.cn>
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation; either version 2 of
 * the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston,
 * MA 02111-1307 USA
 */

#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/init.h>
#include <linux/fs.h>
#include <linux/miscdevice.h>
#include <linux/types.h>
#include <asm/uaccess.h>
#include <asm/io.h>
#include <soc/gpio.h>
#include <soc/irq.h>
#include <linux/gpio.h>
#include <linux/errno.h>
#include <linux/delay.h>
#include <linux/interrupt.h>
#include <linux/irq.h>
#include <linux/clk.h>
#include <linux/platform_device.h>
/*
 * this is the normal test case
 * in normal don't change the Indicator light order
 */

#define myprintk(...) printk("\n[xtang printk]:---------------->File:%s, Line:%d, Function:%s,----------------------->\n" \
		                     __VA_ARGS__, __FILE__, __LINE__ ,__FUNCTION__);


#define OPEN_SHORT_TEST	_IOW('k', 20, int)
#define LED_BLUE		_IOW('k', 30, int)
#define LED_RED1		_IOW('k', 31, int)
#define LED_RED2		_IOW('k', 32, int)
#define LED_GREEN1		_IOW('k', 33, int)
#define LED_GREEN2		_IOW('k', 34, int)



/*
 * this is the supported test case
 * you can add the test case if you need
 */
#define SET_GPIO_HIGH	_IOW('k', 40, int)
#define SET_GPIO_LOW	_IOW('k', 41, int)
#define GET_GPIO_VALUE	_IOR('k', 42, int)

/*
 * this is the factory test Indicator light
 * blue led indicate under test
 * red1 and red2 led indicate failed test
 * green1 and green2 led indicate successed test
 * you can change the led gpio according the board class
 */
#define GPIO_BLUE		GPIO_PD(4)
#define GPIO_BLUE_IN	GPIO_PD(5)
#define GPIO_RED1		GPIO_PB(1)
#define GPIO_RED1_IN	GPIO_PB(2)
#define GPIO_RED2		GPIO_PB(5)
#define GPIO_RED2_IN	GPIO_PB(21)
#define GPIO_GREEN1		GPIO_PB(3)
#define GPIO_GREEN1_IN	GPIO_PB(4)
#define GPIO_GREEN2		GPIO_PB(0)
#define GPIO_GREEN2_IN	GPIO_PB(22)

#if 0
#define GPIO_IOBASE_D	(0xB0010300)

#define PXPIN	0x00	/* PIN Level Register */
#define PXINT	0x10	/* Port Interrupt Register */
#define PXMSK	0x20	/* Port Interrupt Mask Reg */
#define PXPAT1	0x30	/* Port Pattern 1 Set Reg. */
#define PXPAT0	0x40	/* Port Pattern 0 Register */
#define PXFLG	0x50	/* Port Flag Register */
#define PXPEN	0x70	/* Port Pull Disable Register */

#define read_reg(addr)	(*(volatile u32*)(GPIO_IOBASE_D + addr))

#define read_reg_a(addr)	(*(volatile u32*)(GPIO_IOBASE_A + addr))

static void print_portd_gpio_regs(void)
{
	printk("---- PXPIN = 0x%08x\n", read_reg(PXPIN));
	printk("---- PXINT = 0x%08x\n", read_reg(PXINT));
	printk("---- PXMSK = 0x%08x\n", read_reg(PXMSK));
	printk("---- PXPAT1 = 0x%08x\n", read_reg(PXPAT1));
	printk("---- PXPAT0 = 0x%08x\n", read_reg(PXPAT0));
	printk("---- PXFLG = 0x%08x\n", read_reg(PXFLG));
	printk("---- PXPEN = 0x%08x\n", read_reg(PXPEN));
	printk("******************\n");
}
#endif

struct gpio_pair {
	unsigned gpio1;
	unsigned gpio2;
};

struct gpio_pair_three {
	unsigned gpio1;
	unsigned gpio2;
	unsigned gpio3;
};

enum gpio_direction {
	IO = 0,
	INPUT,
	OUTPUT,
};

struct gpio_special {
	unsigned gpio;
	enum gpio_direction dir;
	void (*enable_gpio_input) (void);
	void (*enable_gpio_output) (void);
};

struct gpio_special_pair {
	struct gpio_special gpio1;
	struct gpio_special gpio2;
};

static struct gpio test_gpios[] = {
/*
 * gpio pairs
 */
	{GPIO_PB(22), GPIOF_IN, ""}, /*DMIC0_IN*/
	{GPIO_PB(24), GPIOF_IN, ""},/*SMB0_SDA*/
	{GPIO_PB(23), GPIOF_IN, ""},
	{GPIO_PC(27), GPIOF_IN, ""},
	{GPIO_PC(26), GPIOF_IN, ""},
	{GPIO_PC(25), GPIOF_IN, ""},

	{GPIO_PC(24), GPIOF_IN, ""},
	{GPIO_PD(4), GPIOF_IN, ""},
	{GPIO_PD(5), GPIOF_IN, ""},
	{GPIO_PD(2), GPIOF_IN, ""},
	{GPIO_PD(3), GPIOF_IN, ""},
	{GPIO_PD(0), GPIOF_IN, ""},
	{GPIO_PD(1), GPIOF_IN, ""},
	{GPIO_PB(1), GPIOF_IN, ""},
	{GPIO_PB(0), GPIOF_IN, ""},
	{GPIO_PB(3), GPIOF_IN, ""},
	{GPIO_PB(2), GPIOF_IN, ""},
	{GPIO_PB(4), GPIOF_IN, ""},
	{GPIO_PB(5), GPIOF_IN, ""},/*DMIC1_IN*/
	{GPIO_PB(21), GPIOF_IN, ""},/*DMIC_CLK*/
	{GPIO_PB(25), GPIOF_IN, ""},
	{GPIO_PB(7), GPIOF_IN, ""},
	{GPIO_PB(6), GPIOF_IN, ""},
	{GPIO_PB(10), GPIOF_IN, ""},
	{GPIO_PB(13), GPIOF_IN, ""},
	{GPIO_PB(11), GPIOF_IN, ""},
	{GPIO_PB(12), GPIOF_IN, ""},
	{GPIO_PB(15), GPIOF_IN, ""},
	{GPIO_PB(14), GPIOF_IN, ""},
	{GPIO_PB(16), GPIOF_IN, ""},
	{GPIO_PA(1), GPIOF_IN, ""},
	{GPIO_PA(0), GPIOF_IN, ""},
	{GPIO_PB(18), GPIOF_IN, ""},
	{GPIO_PA(3), GPIOF_IN, ""},
	{GPIO_PA(2), GPIOF_IN, ""},
	{GPIO_PA(4), GPIOF_IN, ""},
	{GPIO_PA(6), GPIOF_IN, ""},
	{GPIO_PB(17), GPIOF_IN, ""},
	{GPIO_PB(20), GPIOF_IN, ""},
	{GPIO_PA(5), GPIOF_IN, ""},
	{GPIO_PA(7), GPIOF_IN, ""},
	{GPIO_PA(8), GPIOF_IN, ""},
	{GPIO_PA(9), GPIOF_IN, ""},
	{GPIO_PA(11), GPIOF_IN, ""},
	{GPIO_PB(19), GPIOF_IN, ""},
	{GPIO_PA(10), GPIOF_IN, ""},
	{GPIO_PA(20), GPIOF_IN, ""},
	{GPIO_PA(21), GPIOF_IN, ""},
	{GPIO_PA(22), GPIOF_IN, ""},
	{GPIO_PA(23), GPIOF_IN, ""},
	{GPIO_PA(24), GPIOF_IN, ""},
	{GPIO_PA(25), GPIOF_IN, ""},
	{GPIO_PA(12), GPIOF_IN, ""},
	{GPIO_PA(13), GPIOF_IN, ""},
	{GPIO_PA(15), GPIOF_IN, ""},
	{GPIO_PA(14), GPIOF_IN, ""},
	{GPIO_PA(16), GPIOF_IN, ""},
	{GPIO_PA(18), GPIOF_IN, ""},
	{GPIO_PA(19), GPIOF_IN, ""},
	{GPIO_PA(17), GPIOF_IN, ""},
/*
** gpio special pairs
**/

	{GPIO_PC(23), GPIOF_IN, ""},
	{GPIO_PB(31), GPIOF_IN, ""},
/*
 * gpio special pairs
 */
};

/*
 * these gpios is not to be open_short tested,
 * they are used to supported other test module
 * */
static struct gpio supported_set_gpios[] = {
};

static struct gpio_pair gpio_pairs[] = {
	{GPIO_PB(22), GPIO_PB(24)},
	{GPIO_PB(23), GPIO_PC(27)},
	{GPIO_PC(26), GPIO_PC(25)},

	{GPIO_PC(24), GPIO_PD(4)},
	{GPIO_PD(5), GPIO_PD(2)},
	{GPIO_PD(3), GPIO_PD(0)},
	{GPIO_PD(1), GPIO_PB(1)},
	{GPIO_PB(0), GPIO_PB(3)},
	{GPIO_PB(2), GPIO_PB(4)},
	{GPIO_PB(5), GPIO_PB(21)},
	{GPIO_PB(25), GPIO_PB(7)},
	{GPIO_PB(6), GPIO_PB(10)},
	{GPIO_PB(13), GPIO_PB(11)},
	{GPIO_PB(12), GPIO_PB(15)},
	{GPIO_PB(14), GPIO_PB(16)},
	{GPIO_PA(1), GPIO_PA(0)},
	{GPIO_PB(18), GPIO_PA(3)},
	{GPIO_PA(2), GPIO_PA(4)},
	{GPIO_PA(6), GPIO_PB(17)},
	{GPIO_PB(20), GPIO_PA(5)},
	{GPIO_PA(7), GPIO_PA(8)},
	{GPIO_PA(9), GPIO_PA(11)},
	{GPIO_PB(19), GPIO_PA(10)},
	{GPIO_PA(20), GPIO_PA(21)},
	{GPIO_PA(22), GPIO_PA(23)},
	{GPIO_PA(24), GPIO_PA(25)},
	{GPIO_PA(12), GPIO_PA(13)},
	{GPIO_PA(15), GPIO_PA(14)},
	{GPIO_PA(16), GPIO_PA(18)},
	{GPIO_PA(19), GPIO_PA(17)},

};

static struct gpio_pair_three gpio_pairs_three[] = {
};

 static void pb31_set_nopull(void)
 {
      gpio_direction_input(GPIO_PB(31));
 }

static struct gpio_special_pair gpio_special_pairs[] = {
	{
	        {GPIO_PB(31), INPUT, NULL, NULL},
	        {GPIO_PC(23), OUTPUT, NULL, NULL},
	},
//

};

static int test_gpio_pair(struct gpio_pair gpio_pair)
{
	gpio_direction_input(gpio_pair.gpio2);
	gpio_direction_output(gpio_pair.gpio1, 0);
	udelay(10);
	if (gpio_get_value(gpio_pair.gpio2) != 0){
		printk("==1=====gpio_pair.gpio2=%d\n",gpio_get_value(gpio_pair.gpio2));
		return -1;
	}

	gpio_direction_output(gpio_pair.gpio1, 1);
	udelay(10);
	if (gpio_get_value(gpio_pair.gpio2) != 1){
		printk("===2====gpio_pair.gpio2=%d\n",gpio_get_value(gpio_pair.gpio2));
		return -1;
	}
	gpio_direction_input(gpio_pair.gpio1);
	gpio_direction_output(gpio_pair.gpio2, 0);
	udelay(10);
	if (gpio_get_value(gpio_pair.gpio1) != 0){
		printk("====3===gpio_pair.gpio1=%d\n",gpio_get_value(gpio_pair.gpio1));
		return -1;
	}
	gpio_direction_output(gpio_pair.gpio2, 1);
	udelay(10);
	if (gpio_get_value(gpio_pair.gpio1) != 1){
		printk("=====4==gpio_pair.gpio1=%d\n",gpio_get_value(gpio_pair.gpio1));
		return -1;
	}
	return 0;
}

static int test_gpio_pair_three(struct gpio_pair_three gpio_pair_three)
{
	gpio_direction_input(gpio_pair_three.gpio2);
	gpio_direction_input(gpio_pair_three.gpio3);
	gpio_direction_output(gpio_pair_three.gpio1, 0);
	udelay(10);
	if (gpio_get_value(gpio_pair_three.gpio2) != 0)
		return -1;

	gpio_direction_output(gpio_pair_three.gpio1, 1);
	udelay(10);
	if (gpio_get_value(gpio_pair_three.gpio2) != 1)
		return -1;

	gpio_direction_input(gpio_pair_three.gpio1);
	gpio_direction_input(gpio_pair_three.gpio3);
	gpio_direction_output(gpio_pair_three.gpio2, 0);
	udelay(10);
	if (gpio_get_value(gpio_pair_three.gpio3) != 0)
		return -1;

	gpio_direction_output(gpio_pair_three.gpio2, 1);
	udelay(10);
	if (gpio_get_value(gpio_pair_three.gpio3) != 1)
		return -1;

	gpio_direction_input(gpio_pair_three.gpio1);
	gpio_direction_input(gpio_pair_three.gpio2);
	gpio_direction_output(gpio_pair_three.gpio3, 0);
	udelay(10);
	if (gpio_get_value(gpio_pair_three.gpio1) != 0)
		return -1;

	gpio_direction_output(gpio_pair_three.gpio3, 1);
	udelay(10);
	if (gpio_get_value(gpio_pair_three.gpio1) != 1)
		return -1;

	return 0;
}

static int test_gpio_special_pairs(struct gpio_special_pair gpio_pair)
{
	if (gpio_pair.gpio1.dir == INPUT && gpio_pair.gpio2.dir == INPUT)
		return -1;

	if (gpio_pair.gpio1.dir == OUTPUT && gpio_pair.gpio2.dir == OUTPUT)
		return -1;

	if (gpio_pair.gpio1.dir == INPUT || gpio_pair.gpio2.dir == OUTPUT
	    || (gpio_pair.gpio1.dir == IO && gpio_pair.gpio2.dir == IO)) {

		if (gpio_pair.gpio1.enable_gpio_input)
			gpio_pair.gpio1.enable_gpio_input();

		gpio_direction_input(gpio_pair.gpio1.gpio);

		if (gpio_pair.gpio2.enable_gpio_output)
			gpio_pair.gpio2.enable_gpio_output();

		gpio_direction_output(gpio_pair.gpio2.gpio, 0);
		udelay(10);
		if (gpio_get_value(gpio_pair.gpio1.gpio) != 0) {
			return -1;
		}

		gpio_direction_output(gpio_pair.gpio2.gpio, 1);
		udelay(10);
		if (gpio_get_value(gpio_pair.gpio1.gpio) != 1) {
			return -1;
		}
	}

	if (gpio_pair.gpio1.dir == OUTPUT || gpio_pair.gpio2.dir == INPUT
	    || (gpio_pair.gpio1.dir == IO && gpio_pair.gpio2.dir == IO)) {

		if (gpio_pair.gpio2.enable_gpio_input)
			gpio_pair.gpio2.enable_gpio_input();

		gpio_direction_input(gpio_pair.gpio2.gpio);

		if (gpio_pair.gpio1.enable_gpio_output)
			gpio_pair.gpio1.enable_gpio_output();

		gpio_direction_output(gpio_pair.gpio1.gpio, 0);
		udelay(10);
		if (gpio_get_value(gpio_pair.gpio2.gpio) != 0) {
			return -1;
		}
		gpio_direction_output(gpio_pair.gpio1.gpio, 1);
		udelay(10);
		if (gpio_get_value(gpio_pair.gpio2.gpio) != 1) {
			return -1;
		}
	}

	return 0;
}

static int open_short_test(void)
{
	int ret = 0;
	int i;
	int size_pairs = ARRAY_SIZE(gpio_pairs);
	int size_pairs_three = ARRAY_SIZE(gpio_pairs_three);
	int size_special_pairs = ARRAY_SIZE(gpio_special_pairs);
	for (i = 0; i < size_pairs; i++) {
		ret = test_gpio_pair(gpio_pairs[i]);
		if (ret) {
			printk("open-short test error1\n");
			printk("-------->i = %d, gpio pairs: %d  %d communication failure\n", i,
			       gpio_pairs[i].gpio1, gpio_pairs[i].gpio2);
			return ret;
		}
			printk("i = %d, gpio pairs: %d  %d communication successful\n", i,
			       gpio_pairs[i].gpio1, gpio_pairs[i].gpio2);
	}
	for (i = 0; i < size_pairs_three; i++) {
		ret = test_gpio_pair_three(gpio_pairs_three[i]);
		if (ret) {
			printk("open-short test error2\n");
			printk("i = %d, gpio pairs: %d  %d  %d\n", i,
			       gpio_pairs_three[i].gpio1,
			       gpio_pairs_three[i].gpio2,
			       gpio_pairs_three[i].gpio3);
			return ret;
		}
	}

	for (i = 0; i < size_special_pairs; i++) {
		ret = test_gpio_special_pairs(gpio_special_pairs[i]);
		if (ret) {
			printk("open-short test error3\n");
			printk("i = %d, gpio special pairs: %d  %d communication failure\n", i,
					gpio_special_pairs[i].gpio1.gpio,
					gpio_special_pairs[i].gpio2.gpio);
			return ret;
		}
			printk("i = %d, gpio special pairs: %d  %d communication successful\n", i,
					gpio_special_pairs[i].gpio1.gpio,
					gpio_special_pairs[i].gpio2.gpio);
	}

	return ret;
}

static struct miscdevice factory_mdev;

static int factory_test_open(struct inode *inode, struct file *file)
{
	/*printk (KERN_INFO "Hey! factory_test device opened\n");*/
	return 0;
}

static int factory_test_release(struct inode *inode, struct file *file)
{
	/*printk (KERN_INFO "Hmmm! factory_test device closed\n");*/
	return 0;
}

static long factory_test_ioctl(struct file *filep, unsigned int cmd,
			   unsigned long arg)
{
	int i;
	long ret = 0;
	switch (cmd) {
	case OPEN_SHORT_TEST:
		ret = open_short_test();
		break;
		/*arg 0: led on;  arg 1: led off */
	case LED_BLUE:
		gpio_direction_input(GPIO_BLUE_IN);
//		gpio_set_pull(GPIO_BLUE_IN, 0);
		gpio_direction_output(GPIO_BLUE, arg);
		break;
	case LED_RED1:
	    gpio_direction_input(GPIO_RED1_IN);
//		gpio_set_pull(GPIO_RED1_IN, 0);
		gpio_direction_output(GPIO_RED1, arg);
		break;
	case LED_RED2:
		gpio_direction_input(GPIO_RED2_IN);
//		gpio_set_pull(GPIO_RED2_IN, 0);
		gpio_direction_output(GPIO_RED2, arg);
		break;
	case LED_GREEN1:
		gpio_direction_input(GPIO_GREEN1_IN);
//		gpio_set_pull(GPIO_GREEN1_IN, 0);
		gpio_direction_output(GPIO_GREEN1, arg);
		break;
	case LED_GREEN2:
		gpio_direction_input(GPIO_GREEN2_IN);
//		gpio_set_pull(GPIO_GREEN2_IN, 0);
		gpio_direction_output(GPIO_GREEN2, arg);
		break;
	case SET_GPIO_HIGH:
		for (i = 0; i < ARRAY_SIZE(supported_set_gpios); i++) {
			if (!strcmp(supported_set_gpios[i].label, (char*)arg))
				break;
		}
		if (i == ARRAY_SIZE(supported_set_gpios)) {
			printk("gpio is not supported\n");
			return -EINVAL;
		}
		gpio_direction_output(supported_set_gpios[i].gpio, 1);
		break;
	case SET_GPIO_LOW:
		for (i = 0; i < ARRAY_SIZE(supported_set_gpios); i++) {
			if (!strcmp(supported_set_gpios[i].label, (char*)arg))
				break;
		}
		if (i == ARRAY_SIZE(supported_set_gpios)) {
			printk("gpio is not supported\n");
			return -EINVAL;
		}
		gpio_direction_output(supported_set_gpios[i].gpio, 0);
		break;
	case GET_GPIO_VALUE:
		for (i = 0; i < ARRAY_SIZE(supported_set_gpios); i++) {
			if (!strcmp(supported_set_gpios[i].label, (char*)arg))
				break;
		}
		if (i == ARRAY_SIZE(supported_set_gpios)) {
			printk("gpio is not supported\n");
			return -EINVAL;
		}
		ret = gpio_get_value(supported_set_gpios[i].gpio);
		break;
	default:
		printk("cmd is not allowed\n");
		return -EINVAL;
	}
	return ret;
}

struct file_operations factory_test_fops = {
	.open = factory_test_open,
	.release = factory_test_release,
	.unlocked_ioctl = factory_test_ioctl,
};


static int factory_probe(struct platform_device *pdev)
{
	int ret;

#if 1
	ret = gpio_request_array(test_gpios, ARRAY_SIZE(test_gpios));
	if (ret) {
		printk("open_short test1 request gpio failed\n");
		goto gpio_req_failed1;
	}

	ret = gpio_request_array(supported_set_gpios,ARRAY_SIZE(supported_set_gpios));
	if (ret) {
		printk("open_short test2 request gpio failed\n");
		goto gpio_req_failed2;
	}
#else
	if (gpio_request_one(GPIO_PB(23), GPIOF_IN,"")) {
		        pr_err("The GPIO %d is requested by other driver not available for this test\n", GPIO_PB(23));
	} else {
		printk("\n----line=%d-------->%d request ok!!\n",__LINE__,GPIO_PB(23));
	}
	if (gpio_request_one(GPIO_PC(27), GPIOF_IN,"")) {
		        pr_err("The GPIO %d is requested by other driver not available for this test\n", GPIO_PC(27));
	} else {
		printk("\n------line=%d------>%d request ok!!\n",__LINE__,GPIO_PC(27));
	}
	if (gpio_request_one(GPIO_PC(26), GPIOF_IN,"")) {
		        pr_err("The GPIO %d is requested by other driver not available for this test\n", GPIO_PC(26));
	} else {
		printk("\n------line=%d------>%d request ok!!\n",__LINE__,GPIO_PC(26));
	}
	if (gpio_request_one(GPIO_PC(25), GPIOF_IN,"")) {
		        pr_err("The GPIO %d is requested by other driver not available for this test\n", GPIO_PC(25));
	} else {
		printk("\n------line=%d------>%d request ok!!\n",__LINE__,GPIO_PC(25));
	}
	if (gpio_request_one(GPIO_PC(24), GPIOF_IN,"")) {
		        pr_err("The GPIO %d is requested by other driver not available for this test\n", GPIO_PC(24));
	} else {
		printk("\n------line=%d------>%d request ok!!\n",__LINE__,GPIO_PC(24));
	}
	if (gpio_request_one(GPIO_PD(4), GPIOF_IN,"")) {
		        pr_err("The GPIO %d is requested by other driver not available for this test\n", GPIO_PD(4));
	} else {
		printk("\n------line=%d------>%d request ok!!\n",__LINE__,GPIO_PD(4));
	}
	if (gpio_request_one(GPIO_PD(5), GPIOF_IN,"")) {
		        pr_err("The GPIO %d is requested by other driver not available for this test\n", GPIO_PD(5));
	} else {
		printk("\n------line=%d------>%d request ok!!\n",__LINE__,GPIO_PD(5));
	}
	if (gpio_request_one(GPIO_PD(2), GPIOF_IN,"")) {
		        pr_err("The GPIO %d is requested by other driver not available for this test\n", GPIO_PD(2));
	} else {
		printk("\n------line=%d------>%d request ok!!\n",__LINE__,GPIO_PD(2));
	}
	if (gpio_request_one(GPIO_PD(3), GPIOF_IN,"")) {
		        pr_err("The GPIO %d is requested by other driver not available for this test\n", GPIO_PD(3));
	} else {
		printk("\n------line=%d------>%d request ok!!\n",__LINE__,GPIO_PD(3));
	}
	if (gpio_request_one(GPIO_PD(0), GPIOF_IN,"")) {
		        pr_err("The GPIO %d is requested by other driver not available for this test\n", GPIO_PD(0));
	} else {
		printk("\n------line=%d------>%d request ok!!\n",__LINE__,GPIO_PD(0));
	}
	if (gpio_request_one(GPIO_PD(1), GPIOF_IN,"")) {
		        pr_err("The GPIO %d is requested by other driver not available for this test\n", GPIO_PD(1));
	} else {
		printk("\n------line=%d------>%d request ok!!\n",__LINE__,GPIO_PD(1));
	}
	if (gpio_request_one(GPIO_PB(1), GPIOF_IN,"")) {
		        pr_err("The GPIO %d is requested by other driver not available for this test\n", GPIO_PB(1));
	} else {
		printk("\n------line=%d------>%d request ok!!\n",__LINE__,GPIO_PB(1));
	}
	if (gpio_request_one(GPIO_PB(0), GPIOF_IN,"")) {
		        pr_err("The GPIO %d is requested by other driver not available for this test\n", GPIO_PB(0));
	} else {
		printk("\n------line=%d------>%d request ok!!\n",__LINE__,GPIO_PB(0));
	}
	if (gpio_request_one(GPIO_PB(3), GPIOF_IN,"")) {
		        pr_err("The GPIO %d is requested by other driver not available for this test\n", GPIO_PB(3));
	} else {
		printk("\n------line=%d------>%d request ok!!\n",__LINE__,GPIO_PB(3));
	}
	if (gpio_request_one(GPIO_PB(2), GPIOF_IN,"")) {
		        pr_err("The GPIO %d is requested by other driver not available for this test\n", GPIO_PB(2));
	} else {
		printk("\n------line=%d------>%d request ok!!\n",__LINE__,GPIO_PB(2));
	}
	if (gpio_request_one(GPIO_PB(4), GPIOF_IN,"")) {
		        pr_err("The GPIO %d is requested by other driver not available for this test\n", GPIO_PB(4));
	} else {
		printk("\n------line=%d------>%d request ok!!\n",__LINE__,GPIO_PB(4));
	}/*<----ok*/
	if (gpio_request_one(GPIO_PB(25), GPIOF_IN,"")) {
		        pr_err("The GPIO %d is requested by other driver not available for this test\n", GPIO_PB(25));
	} else {
		printk("\n------line=%d------>%d request ok!!\n",__LINE__,GPIO_PB(25));
	}

	if (gpio_request_one(GPIO_PB(7), GPIOF_IN,"")) {
		        pr_err("The GPIO %d is requested by other driver not available for this test\n", GPIO_PB(7));
	} else {
		printk("\n------line=%d------>%d request ok!!\n",__LINE__,GPIO_PB(7));
	}
	if (gpio_request_one(GPIO_PB(6), GPIOF_IN,"")) {
		        pr_err("The GPIO %d is requested by other driver not available for this test\n", GPIO_PB(6));
	} else {
		printk("\n------line=%d------>%d request ok!!\n",__LINE__,GPIO_PB(6));
	}

	if (gpio_request_one(GPIO_PB(10), GPIOF_IN,"")) {
		        pr_err("The GPIO %d is requested by other driver not available for this test\n", GPIO_PB(10));
	} else {
		printk("\n------line=%d------>%d request ok!!\n",__LINE__,GPIO_PB(10));
	}
	if (gpio_request_one(GPIO_PB(13), GPIOF_IN,"")) {
		        pr_err("The GPIO %d is requested by other driver not available for this test\n", GPIO_PB(13));
	} else {
		printk("\n------line=%d------>%d request ok!!\n",__LINE__,GPIO_PB(13));
	}
	if (gpio_request_one(GPIO_PB(11), GPIOF_IN,"")) {
		        pr_err("The GPIO %d is requested by other driver not available for this test\n", GPIO_PB(11));
	} else {
		printk("\n------line=%d------>%d request ok!!\n",__LINE__,GPIO_PB(11));
	}
	if (gpio_request_one(GPIO_PB(12), GPIOF_IN,"")) {
		        pr_err("The GPIO %d is requested by other driver not available for this test\n", GPIO_PB(12));
	} else {
		printk("\n------line=%d------>%d request ok!!\n",__LINE__,GPIO_PB(12));
	}
	if (gpio_request_one(GPIO_PB(15), GPIOF_IN,"")) {
		        pr_err("The GPIO %d is requested by other driver not available for this test\n", GPIO_PB(15));
	} else {
		printk("\n------line=%d------>%d request ok!!\n",__LINE__,GPIO_PB(15));
	}
	if (gpio_request_one(GPIO_PB(14), GPIOF_IN,"")) {
		        pr_err("The GPIO %d is requested by other driver not available for this test\n", GPIO_PB(14));
	} else {
		printk("\n------line=%d------>%d request ok!!\n",__LINE__,GPIO_PB(14));
	}
	if (gpio_request_one(GPIO_PB(16), GPIOF_IN,"")) {
		        pr_err("The GPIO %d is requested by other driver not available for this test\n", GPIO_PB(16));
	} else {
		printk("\n------line=%d------>%d request ok!!\n",__LINE__,GPIO_PB(16));
	}
	if (gpio_request_one(GPIO_PA(1), GPIOF_IN,"")) {
		        pr_err("The GPIO %d is requested by other driver not available for this test\n", GPIO_PA(1));
	} else {
		printk("\n------line=%d------>%d request ok!!\n",__LINE__,GPIO_PA(1));
	}
	if (gpio_request_one(GPIO_PA(0), GPIOF_IN,"")) {
		        pr_err("The GPIO %d is requested by other driver not available for this test\n", GPIO_PA(0));
	} else {
		printk("\n------line=%d------>%d request ok!!\n",__LINE__,GPIO_PA(0));
	}
	if (gpio_request_one(GPIO_PB(18), GPIOF_IN,"")) {
		        pr_err("The GPIO %d is requested by other driver not available for this test\n", GPIO_PB(18));
	} else {
		printk("\n------line=%d------>%d request ok!!\n",__LINE__,GPIO_PB(18));
	}
	if (gpio_request_one(GPIO_PA(3), GPIOF_IN,"")) {
		        pr_err("The GPIO %d is requested by other driver not available for this test\n", GPIO_PA(3));
	} else {
		printk("\n------line=%d------>%d request ok!!\n",__LINE__,GPIO_PA(3));
	}
	if (gpio_request_one(GPIO_PA(2), GPIOF_IN,"")) {
		        pr_err("The GPIO %d is requested by other driver not available for this test\n", GPIO_PA(2));
	} else {
		printk("\n------line=%d------>%d request ok!!\n",__LINE__,GPIO_PA(2));
	}
	if (gpio_request_one(GPIO_PA(4), GPIOF_IN,"")) {
		        pr_err("The GPIO %d is requested by other driver not available for this test\n", GPIO_PA(4));
	} else {
		printk("\n------line=%d------>%d request ok!!\n",__LINE__,GPIO_PA(4));
	}
	if (gpio_request_one(GPIO_PA(6), GPIOF_IN,"")) {
		        pr_err("The GPIO %d is requested by other driver not available for this test\n", GPIO_PA(6));
	} else {
		printk("\n------line=%d------>%d request ok!!\n",__LINE__,GPIO_PA(6));
	}
	if (gpio_request_one(GPIO_PB(17), GPIOF_IN,"")) {
		        pr_err("The GPIO %d is requested by other driver not available for this test\n", GPIO_PB(17));
	} else {
		printk("\n------line=%d------>%d request ok!!\n",__LINE__,GPIO_PB(17));
	}
	if (gpio_request_one(GPIO_PB(20), GPIOF_IN,"")) {
		        pr_err("The GPIO %d is requested by other driver not available for this test\n", GPIO_PB(20));
	} else {
		printk("\n------line=%d------>%d request ok!!\n",__LINE__,GPIO_PB(20));
	}
	if (gpio_request_one(GPIO_PA(5), GPIOF_IN,"")) {
		        pr_err("The GPIO %d is requested by other driver not available for this test\n", GPIO_PA(5));
	} else {
		printk("\n------line=%d------>%d request ok!!\n",__LINE__,GPIO_PA(5));
	}
	if (gpio_request_one(GPIO_PA(7), GPIOF_IN,"")) {
		        pr_err("The GPIO %d is requested by other driver not available for this test\n", GPIO_PA(7));
	} else {
		printk("\n------line=%d------>%d request ok!!\n",__LINE__,GPIO_PA(7));
	}
	if (gpio_request_one(GPIO_PA(8), GPIOF_IN,"")) {
		        pr_err("The GPIO %d is requested by other driver not available for this test\n", GPIO_PA(8));
	} else {
		printk("\n------line=%d------>%d request ok!!\n",__LINE__,GPIO_PA(8));
	}
	if (gpio_request_one(GPIO_PA(9), GPIOF_IN,"")) {
		        pr_err("The GPIO %d is requested by other driver not available for this test\n", GPIO_PA(9));
	} else {
		printk("\n------line=%d------>%d request ok!!\n",__LINE__,GPIO_PA(9));
	}
	if (gpio_request_one(GPIO_PB(19), GPIOF_IN,"")) {
		        pr_err("The GPIO %d is requested by other driver not available for this test\n", GPIO_PB(19));
	} else {
		printk("\n------line=%d------>%d request ok!!\n",__LINE__,GPIO_PB(19));
	}
	if (gpio_request_one(GPIO_PA(10), GPIOF_IN,"")) {
		        pr_err("The GPIO %d is requested by other driver not available for this test\n", GPIO_PA(10));
	} else {
		printk("\n------line=%d------>%d request ok!!\n",__LINE__,GPIO_PA(10));
	}
	if (gpio_request_one(GPIO_PA(20), GPIOF_IN,"")) {
		        pr_err("The GPIO %d is requested by other driver not available for this test\n", GPIO_PA(20));
	} else {
		printk("\n------line=%d------>%d request ok!!\n",__LINE__,GPIO_PA(20));
	}

	if (gpio_request_one(GPIO_PA(21), GPIOF_IN,"")) {
		        pr_err("The GPIO %d is requested by other driver not available for this test\n", GPIO_PA(21));
	} else {
		printk("\n------line=%d------>%d request ok!!\n",__LINE__,GPIO_PA(21));
	}
	if (gpio_request_one(GPIO_PA(22), GPIOF_IN,"")) {
		        pr_err("The GPIO %d is requested by other driver not available for this test\n", GPIO_PA(22));
	} else {
		printk("\n------line=%d------>%d request ok!!\n",__LINE__,GPIO_PA(22));
	}
	if (gpio_request_one(GPIO_PA(23), GPIOF_IN,"")) {
		        pr_err("The GPIO %d is requested by other driver not available for this test\n", GPIO_PA(23));
	} else {
		printk("\n------line=%d------>%d request ok!!\n",__LINE__,GPIO_PA(23));
	}
	if (gpio_request_one(GPIO_PA(24), GPIOF_IN,"")) {
		        pr_err("The GPIO %d is requested by other driver not available for this test\n", GPIO_PA(24));
	} else {
		printk("\n------line=%d------>%d request ok!!\n",__LINE__,GPIO_PA(24));
	}
	if (gpio_request_one(GPIO_PA(25), GPIOF_IN,"")) {
		        pr_err("The GPIO %d is requested by other driver not available for this test\n", GPIO_PA(25));
	} else {
		printk("\n------line=%d------>%d request ok!!\n",__LINE__,GPIO_PA(25));
	}
	if (gpio_request_one(GPIO_PA(12), GPIOF_IN,"")) {
		        pr_err("The GPIO %d is requested by other driver not available for this test\n", GPIO_PA(12));
	} else {
		printk("\n------line=%d------>%d request ok!!\n",__LINE__,GPIO_PA(12));
	}
	if (gpio_request_one(GPIO_PA(13), GPIOF_IN,"")) {
		        pr_err("The GPIO %d is requested by other driver not available for this test\n", GPIO_PA(13));
	} else {
		printk("\n------line=%d------>%d request ok!!\n",__LINE__,GPIO_PA(13));
	}
	if (gpio_request_one(GPIO_PA(15), GPIOF_IN,"")) {
		        pr_err("The GPIO %d is requested by other driver not available for this test\n", GPIO_PA(15));
	} else {
		printk("\n------line=%d------>%d request ok!!\n",__LINE__,GPIO_PA(15));
	}
	if (gpio_request_one(GPIO_PA(14), GPIOF_IN,"")) {
		        pr_err("The GPIO %d is requested by other driver not available for this test\n", GPIO_PA(14));
	} else {
		printk("\n------line=%d------>%d request ok!!\n",__LINE__,GPIO_PA(14));
	}
	if (gpio_request_one(GPIO_PA(16), GPIOF_IN,"")) {
		        pr_err("The GPIO %d is requested by other driver not available for this test\n", GPIO_PA(16));
	} else {
		printk("\n------line=%d------>%d request ok!!\n",__LINE__,GPIO_PA(16));
	}
	if (gpio_request_one(GPIO_PA(18), GPIOF_IN,"")) {
		        pr_err("The GPIO %d is requested by other driver not available for this test\n", GPIO_PA(18));
	} else {
		printk("\n------line=%d------>%d request ok!!\n",__LINE__,GPIO_PA(18));
	}
	if (gpio_request_one(GPIO_PA(19), GPIOF_IN,"")) {
		        pr_err("The GPIO %d is requested by other driver not available for this test\n", GPIO_PA(19));
	} else {
		printk("\n------line=%d------>%d request ok!!\n",__LINE__,GPIO_PA(19));
	}
	if (gpio_request_one(GPIO_PA(17), GPIOF_IN,"")) {
		        pr_err("The GPIO %d is requested by other driver not available for this test\n", GPIO_PA(17));
	} else {
		printk("\n------line=%d------>%d request ok!!\n",__LINE__,GPIO_PA(17));
	}
	if (gpio_request_one(GPIO_PC(23), GPIOF_IN,"")) {
		        pr_err("The GPIO %d is requested by other driver not available for this test\n", GPIO_PC(23));
	} else {
		printk("\n------line=%d------>%d request ok!!\n",__LINE__,GPIO_PC(23));
	}
	if (gpio_request_one(GPIO_PB(31), GPIOF_IN,"")) {
		        pr_err("The GPIO %d is requested by other driver not available for this test\n", GPIO_PB(31));
	} else {
		printk("\n------line=%d------>%d request ok!!\n",__LINE__,GPIO_PB(31));
	}

#endif
	factory_mdev.minor = MISC_DYNAMIC_MINOR;
	factory_mdev.name = "test_gpio";
	factory_mdev.fops = &factory_test_fops;
	ret = misc_register(&factory_mdev);
//	printk("\n\n----------file=%s,func=%s,line=%d------factory_mdev.name = %s \n\n",__FILE__,__func__,__LINE__,factory_mdev.name);
	if (ret) {
		printk("register test miscdevice failed!\n");
		goto misc_failed;
	}

	printk("factory_test_driver probe success\n ");
	return ret;

misc_failed:
	gpio_free_array(supported_set_gpios, ARRAY_SIZE(supported_set_gpios));
gpio_req_failed2:
	gpio_free_array(test_gpios, ARRAY_SIZE(test_gpios));
gpio_req_failed1:
	return ret;
error_put_platform_device:
		return ret;

}

static int factory_remove(struct platform_device *pdev)
{
	misc_deregister(&factory_mdev);
	gpio_free_array(supported_set_gpios, ARRAY_SIZE(supported_set_gpios));
	gpio_free_array(test_gpios, ARRAY_SIZE(test_gpios));
	return 0;
}

static struct platform_driver factory_test_driver = {
	.driver.name = "factory-test",
	.driver.owner = THIS_MODULE,
	.probe = factory_probe,
	.remove = factory_remove,
};

static int __init factory_test_init(void)
{
	return platform_driver_register(&factory_test_driver);
}

static void __exit factory_test_exit(void)
{
	platform_driver_unregister(&factory_test_driver);
}

module_init(factory_test_init);
module_exit(factory_test_exit);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("wanchaozhu");
MODULE_DESCRIPTION("Ingenic factory test driver");
