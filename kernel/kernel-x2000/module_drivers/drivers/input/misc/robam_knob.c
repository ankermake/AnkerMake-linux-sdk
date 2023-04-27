/* drivers/input/misc/robam_knob.c
 *
 */

#include <linux/input.h>
#include <linux/delay.h>
#include <linux/slab.h>
#include <linux/interrupt.h>
#include <linux/platform_device.h>
#include <linux/of_gpio.h>
#include <linux/kernel.h>
#include <linux/semaphore.h>
#include <linux/module.h>
#include <linux/gpio.h>
#include <linux/syscalls.h>
#include <linux/unistd.h>
#include <linux/uaccess.h>
#include <linux/fs.h>
#include <linux/string.h>
#include <linux/kthread.h>
#include <linux/cdev.h>

#include "linux/input/robam_knob.h"

#define MAJOR_NUM 241
#define MINOR_NUM 0
static dev_t devnum = 0;
static struct cdev robam_knob_cdev;
static struct class *robam_knob_class;
static char *modname    = "robam_knob_mod";
static char *devicename = "robam_knob";
static char *classname  = "robam_knob_class";

static long a_pin     = -1;
static long b_pin     = -1;
static long led_pin   = -1;
static long touch_pin = -1;

struct knob_event {
	u8 knob_sta;
};

struct robam_knob_data {
	struct input_dev *input_dev;
	struct knob_event event;
};
static struct robam_knob_data *robam_knob;

//   Modiffied by Qi

#define HI_SPEED	(80000L)	/* 80ms  */
#define MID_SPEED	(210000L)	/* 210ms   */  
#define MAX_VALUE	(300000L)	/* 300ms   */  

//  以下为上报旋扭状态， 请修改为可识别事件状态
#define KEY_NOT_PRESSED				0		 //  无旋转
#define KEY_UP_PRESSED					1   //  正旋转 慢速
#define KEY_UP_PRESSED_1S			2  //  正旋转 中速度
#define KEY_UP_PRESSED_3S			3 //  正旋转  快速度
#define KEY_DOWN_PRESSED			1	//  反旋转 慢速
#define KEY_DOWN_PRESSED_1S			2	//  反旋转 中速度
#define KEY_DOWN_PRESSED_3S			3	 //  反旋转  快速度
// 以上为上报旋扭状态， 请修改为可识别事件状态

static unsigned char  encoder_direction = KEY_NOT_PRESSED;  // 当前旋扭方向
static unsigned char  encoder_lastdirection = KEY_NOT_PRESSED; // 上次旋扭方向
static s64 end_tm = 0,start_tm = 0;
static s64 diff_tm = 0;
struct timeval posi_tm;
static int  a_state=0;
static int  b_state=0;
//   above

#if 0
/*Read rabam　knob data.*/
static int robam_knob_read_knobdata(struct robam_knob_data *data)
{
	return 0;
}
/*
 *report the knob information
 */
static int robam_knob_report_value(struct robam_knob_data *data)
{
	struct knob_event *event = &data->event;
	input_sync(data->input_dev);
	return 0;

}

static void robam_knob_work_handler(struct robam_knob_data *robam_knob)
{
	int ret = 0;
	ret = robam_knob_read_knobdata(robam_knob);
	if (ret == 0)
		robam_knob_report_value(robam_knob);
}

static int robam_knob_thread_func(void *data)
{
	struct robam_knob_data *robam_knob = (struct robam_knob_data*)data;
	while (1) {
		msleep(30);
		robam_knob_work_handler(robam_knob);
	}

	return 0;
}

static int  robam_knob_create_thread (struct robam_knob_data* robam_knob) {
	struct task_struct *p =NULL;
	int rc = 0;

	p = kthread_run(robam_knob_thread_func, (void*)robam_knob,"kthread_run");
	if (IS_ERR(p))
	{
		rc = PTR_ERR(p);
		printk(" error %d create thread_name thread", rc);
	}

	return 0;
}
#endif


/*A pin 中断处理函数*/
static irqreturn_t gpio_a_interrupt(int irq, void *dev_id)
{

	int value_a;
	int value_b;
	int value = KEY_NOT_PRESSED;

    udelay(4000);
    value_a = gpio_get_value(a_pin);
    value_b = gpio_get_value(b_pin);
//	gettimeofday(&posi_tm, NULL);
//	end_tm = (int64_t)posi_tm.tv_sec*1000 + posi_tm.tv_usec/1000;		/*   单位ms */
    end_tm = ktime_to_us(ktime_get());
	if (value_a >0 )  //  A_pin H level
	{
			if(b_state > 0)	//  B_pin H  level
			{
                a_state = 0;
                b_state = 0;
                encoder_direction = KEY_UP_PRESSED;
			}else{	// B_pin L  level
			
				a_state = 1;
			}		
			
	}else{ //  A_pin L level
	
		 	if (value_b <= 0)
			 {
				a_state = 0;
				b_state = 0;
			 }
	}
	if (encoder_direction > KEY_NOT_PRESSED)
	{
		encoder_direction = KEY_NOT_PRESSED;
		if (encoder_lastdirection != encoder_direction)
			{
					start_tm = end_tm;
					encoder_lastdirection= encoder_direction;
			}		
			diff_tm = end_tm - start_tm;
			start_tm = end_tm;
			if (diff_tm <= 0)
				diff_tm = MAX_VALUE + MID_SPEED;
			if (diff_tm <= HI_SPEED )
				value = KEY_UP_PRESSED_3S;
			else if (diff_tm <= MID_SPEED) 
				value = KEY_UP_PRESSED_1S;
			else
				value = KEY_UP_PRESSED;
	}
	else
	{
		diff_tm = end_tm - start_tm;
		if (diff_tm > MAX_VALUE)
			encoder_lastdirection = KEY_NOT_PRESSED;
	}
	
 //   above
 //   printk("%s: value = %d\n", __func__, value);
    input_report_rel(robam_knob->input_dev, REL_Y, value);  
	input_sync(robam_knob->input_dev);
	return IRQ_HANDLED;
}

/*B pin 中断处理函数*/	 
static irqreturn_t gpio_b_interrupt(int irq, void *dev_id)
{

	int value_a;
	int value_b;
	int value = KEY_NOT_PRESSED;

    udelay(4000);
    value_a = gpio_get_value(a_pin);
    value_b = gpio_get_value(b_pin);

//	gettimeofday(&posi_tm, NULL);
//	end_tm = (int64_t)posi_tm.tv_sec*1000 + posi_tm.tv_usec/1000;		/*   单位ms */
    end_tm = ktime_to_us(ktime_get()) ;
	if (value_b > 0 )  //  B_pin H level
	{	
		if(a_state > 0)	//  A_pin H  level
		{
					a_state = 0;
					b_state = 0;
					encoder_direction = KEY_DOWN_PRESSED;
		}
		else		//  A_pin L  level
		{
			b_state = 1;
		}		
	}
	else    //  B_pin L level
	{
			if (value_a  <= 0)	//  A_pin L level
			 {
				a_state = 0;
				b_state = 0;
			 }
	}
	if (encoder_direction > KEY_NOT_PRESSED)
	{
		encoder_direction = KEY_NOT_PRESSED;
		if (encoder_lastdirection != encoder_direction)
			{
					start_tm = end_tm;
					encoder_lastdirection= encoder_direction;
			}		
			diff_tm = end_tm - start_tm;
			start_tm = end_tm;
			if (diff_tm <= 0)
				diff_tm = MAX_VALUE + MID_SPEED;
			if (diff_tm <= HI_SPEED )
				value = KEY_DOWN_PRESSED_3S;
			else if (diff_tm <= MID_SPEED) 
				value = KEY_DOWN_PRESSED_1S;
			else
				value = KEY_DOWN_PRESSED;
	}
	else 
	{
		diff_tm = end_tm - start_tm;
		if (diff_tm > MAX_VALUE)
			encoder_lastdirection = KEY_NOT_PRESSED;
	}
 //   above
 //   printk("%s: value = %d\n", __func__, value);
    input_report_rel(robam_knob->input_dev, REL_X, value);  
	input_sync(robam_knob->input_dev);
	return IRQ_HANDLED;
}

static irqreturn_t gpio_touch_interrupt(int irq, void *dev_id)
{
	int value = gpio_get_value(touch_pin);

    input_report_key(robam_knob->input_dev, KEY_TOUCHPAD_ON, value);
	input_sync(robam_knob->input_dev);

	return IRQ_HANDLED;
}

static int of_panel_parse(struct device *dev)
{
//	struct rtc_device *rtc;
	struct device_node *np = dev->of_node;
	enum of_gpio_flags flags;
	int ret = -ENOMEM;
	int err, irq;


	a_pin = of_get_named_gpio_flags(np, "gpio-A", 0, &flags);
	if(!gpio_is_valid(a_pin)) {
		a_pin = -1;
	}else{
		ret = gpio_request(a_pin, "a-pin");
		if(unlikely(ret)){
			a_pin = -1;
		}
	}

	err = gpio_direction_input(a_pin);
	if (err < 0)
		goto out_free;

	irq = gpio_to_irq(a_pin);
	if (irq < 0)
		goto out_free;

	err = request_irq(irq, gpio_a_interrupt, IRQF_TRIGGER_RISING |
					  IRQF_TRIGGER_FALLING, "gpio-A", NULL);
	if (err)
		goto out_free;


	b_pin = of_get_named_gpio_flags(np, "gpio-B", 0, &flags);
	if(!gpio_is_valid(b_pin)) {
		b_pin = -1;
	}else{
		ret = gpio_request(b_pin, "b-pin");
		if(unlikely(ret)){
			b_pin = -1;
		}
	}

	err = gpio_direction_input(b_pin);
	if (err < 0)
		goto out_free;

	irq = gpio_to_irq(b_pin);
	if (irq < 0)
		goto out_free;

	err = request_irq(irq, gpio_b_interrupt, IRQF_TRIGGER_RISING |
					  IRQF_TRIGGER_FALLING, "gpio-B", NULL);
	if (err)
		goto out_free;

	led_pin = of_get_named_gpio_flags(np, "gpio-led", 0, &flags);
	if(!gpio_is_valid(led_pin)) {
		led_pin = -1;
	}else{
		ret = gpio_request(led_pin, "led-pin");
		if(unlikely(ret)){
			led_pin = -1;
		}
	}

	touch_pin = of_get_named_gpio_flags(np, "gpio-touch", 0, &flags);
	if(!gpio_is_valid(touch_pin)) {
		touch_pin = -1;
	}else{
		ret = gpio_request(touch_pin, "touch-pin");
		if(unlikely(ret)){
			touch_pin = -1;
		}
	}
	err = gpio_direction_input(touch_pin);
	if (err < 0)
		goto out_free;

	irq = gpio_to_irq(touch_pin);
	if (irq < 0)
		goto out_free;

	err = request_irq(irq, gpio_touch_interrupt, IRQF_TRIGGER_RISING | IRQF_TRIGGER_FALLING 
					  , "gpio-touch-rising", NULL);
	if (err)
		goto out_free;



out_free:
    return 0;

}

static int robam_knob_probe(struct platform_device *pdev)
{
	struct input_dev *input_dev;
	int err = 0;
	int ret = 0;

	ret = of_panel_parse(&pdev->dev);
	if(ret < 0) {
		return ret;
	}

	robam_knob = kzalloc(sizeof(struct robam_knob_data), GFP_KERNEL);

	if (!robam_knob) {
		err = -ENOMEM;
		goto exit_alloc_data_failed;
	}

	input_dev = input_allocate_device();
	if (!input_dev) {
		err = -ENOMEM;
		dev_err(&pdev->dev, "failed to allocate input device\n");
		goto exit_input_dev_alloc_failed;
	}
    set_bit(KEY_TOUCHPAD_ON, input_dev->keybit);
    set_bit(REL_X, input_dev->relbit);
    set_bit(REL_Y, input_dev->relbit);

	set_bit(EV_KEY, input_dev->evbit);
	set_bit(EV_REL, input_dev->evbit);
	input_dev->name = ROBAM_KNOB_NAME;
	err = input_register_device(input_dev);
	if (err) {
		dev_err(&pdev->dev,
			"robam_knob_probe: failed to register input device: %s\n",
			dev_name(&pdev->dev));
		goto exit_input_register_device_failed;
	}
	robam_knob->input_dev = input_dev;

	//robam_knob_create_thread(robam_knob);
	return 0;

exit_input_register_device_failed:
	input_free_device(input_dev);

exit_alloc_data_failed:
exit_input_dev_alloc_failed:
	return err;
}


static long robam_knob_ioctl(struct file *fd, unsigned int cmd, unsigned long arg)
{
    int value;
	if(cmd != ROBAM_LED){
		return -1;
	}
//    printk("%s----------cmd = %x, arg = %ld\n",__func__, cmd, arg);
    value = (int)arg;
    if(value > 0){
        gpio_direction_output(led_pin, 1);
    }else{
        gpio_direction_output(led_pin, 0);
    }
	return 0;
}

static int robam_knob_open(struct inode *inode, struct file *fd)
{
	return 0;
}

static int robam_knob_release(struct inode *inode, struct file *fd)
{
	fd->private_data = NULL;
	return 0;
}

static const struct of_device_id robam_knob_id[] = {
	{.compatible = ROBAM_KNOB_NAME,},
	{}
};


static struct platform_driver robam_knob_driver = {
	.probe = robam_knob_probe,
	.driver = {
		   .name = ROBAM_KNOB_NAME,
		   .owner = THIS_MODULE,
		   .of_match_table = of_match_ptr(robam_knob_id),
		   },
};

module_platform_driver(robam_knob_driver);
struct file_operations robam_knob_fops =
{
	.owner = THIS_MODULE,
	.unlocked_ioctl = robam_knob_ioctl,
	.open   = robam_knob_open,
	.release = robam_knob_release,
};

static int __init robam_knob_init(void)
{
	int result;
    int ret;
    printk("========================%s, %d\n", __func__, __LINE__);
	devnum = MKDEV(MAJOR_NUM, MINOR_NUM);
	result = register_chrdev_region(devnum, 1, modname);
	if(result < 0)
	{
		printk("robam_knob_mod : can't get major number!/n");
		return result;
	}

	cdev_init(&robam_knob_cdev, &robam_knob_fops);
	robam_knob_cdev.owner = THIS_MODULE;
	robam_knob_cdev.ops = &robam_knob_fops;
	result = cdev_add(&robam_knob_cdev, devnum, 1);
	if(result){
		printk("Failed at cdev_add()\n");
	}

	robam_knob_class = class_create(THIS_MODULE, classname);

	if(IS_ERR(robam_knob_class))	{
		printk("Failed at class_create().Please exec [mknod] before operate the device/n");
	}else{
		device_create(robam_knob_class, NULL, devnum,NULL, devicename);
	}

	return ret;
}

static void __exit robam_knob_exit(void)
{
	cdev_del(&robam_knob_cdev);
	device_destroy(robam_knob_class, devnum);
	class_destroy(robam_knob_class);
	unregister_chrdev_region(devnum, 1);
}
late_initcall(robam_knob_init);
//module_init(robam_knob_init);
module_exit(robam_knob_exit);

MODULE_AUTHOR("wei.yan<wyan@ingenic.com>");
MODULE_DESCRIPTION("robam knob driver");
MODULE_LICENSE("GPL");
