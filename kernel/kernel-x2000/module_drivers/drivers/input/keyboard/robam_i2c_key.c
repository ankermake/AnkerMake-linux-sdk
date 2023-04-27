/* drivers/input/keyboard//robam_key.c
 *
 */

#include <linux/i2c.h>
#include <linux/input.h>
#include <linux/delay.h>
#include <linux/slab.h>
#include <linux/interrupt.h>
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
#include <linux/input/robam_i2c_key.h>

#define MAJOR_NUM 240
#define MINOR_NUM 0
static dev_t devnum = 0;
static struct cdev robam_key_cdev;
static struct class *robam_key_class;
static char *modname = "robam_key_mod";
static char *devicename = "robam_key";
static char *classname = "robam_key_class";

static struct i2c_client *robam_key_client;

struct key_event {
#ifdef CONFIG_KEYBOARD_ROBAM_TWO_KEY
    u8 key_sta;
#endif
#ifdef CONFIG_KEYBOARD_ROBAM_FOUR_KEY
    u8 key_sta;
#endif
#ifdef CONFIG_KEYBOARD_ROBAM_SIX_KEY
	uint16_t key_sta;
#endif
};

struct robam_key_data {
	struct input_dev *input_dev;
	struct i2c_client *client;
	struct key_event event;
};


#define POLYNOMIAL  0x131 // P(x) = x^8 + x^5 + x^4 + 1 = 100110001
/*=========================================================
 * * 函 数 名 : f_IIC_CalcCrc
 * * 功能描述: CRC校验
 * * 调用方法: 函数调用进入
 * * 输入数据: num校验数组
 * * 生成数据: f_IIC_CalcCrc的返回值（CRC 8-Bit校验值）
 * =========================================================*/
unsigned char f_IIC_CalcCrc(unsigned char *num, size_t len)
{
	unsigned char i;                // bit mask
	unsigned char crc = 0xFF;     // calculated checksum
	unsigned char byteCtr;        // byte counter

  // calculates 8-Bit checksum with given POLYNOMIAL
	for(byteCtr = 0; byteCtr < len - 1; byteCtr++)
	{
		crc ^= *num;
		num++;
		for(i = 8; i > 0; --i)
		{
			if(crc & 0x80)
				crc = (crc << 1) ^ POLYNOMIAL;
			else
				crc = (crc << 1);
		}
	}
	return crc;
}

/*read data from i2c*/
int robam_key_i2c_Read(struct i2c_client *client, unsigned char *writebuf,
		    int writelen, unsigned char *readbuf, int readlen)
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
		if (ret < 0){
			dev_err(&client->dev, "%s, ret=%d: -i2c read error.\n",
				__func__, ret);
		}
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
		if (ret < 0){
			dev_err(&client->dev, "%s:i2c read error.\n", __func__);
		}
	}
	return ret;
}

/*write data by i2c*/
int robam_key_i2c_Write(struct i2c_client *client, unsigned char *writebuf, int writelen)
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
	if (ret < 0){
		dev_err(&client->dev, "%s i2c write error.\n", __func__);
	}
	return ret;
}

/*Read rabam　key data.*/
static int robam_key_read_keydata(struct robam_key_data *data)
{
	struct key_event *event = &data->event;
	unsigned char buf[ROBAM_KEY_DATA_LENGTH] = { 0 };
	int ret = -1;
    unsigned char checksum;
        
	memset(event, 0, sizeof(struct key_event));
	ret = robam_key_i2c_Read(data->client, buf, 1, buf, ROBAM_KEY_DATA_LENGTH);
	if (ret < 0) {
		return ret;
	}
/*
    printk("cljiang-------------- %d\n", ret);
    int i;
    for(i = 0;i < ROBAM_KEY_DATA_LENGTH;i++){
        printk("buf[%d] = %x\n", i, buf[i]);
    }
*/

	//check
	checksum = f_IIC_CalcCrc(buf, ROBAM_KEY_DATA_LENGTH);
	if(checksum != buf[ROBAM_KEY_DATA_LENGTH-1]){
		printk("robam key data crc check error\n");
		return -1;
	}


#ifdef CONFIG_KEYBOARD_ROBAM_TWO_KEY
    event->key_sta = buf[0];
#endif
#ifdef CONFIG_KEYBOARD_ROBAM_FOUR_KEY
    event->key_sta = buf[0];
#endif
#ifdef CONFIG_KEYBOARD_ROBAM_SIX_KEY
	event->key_sta = buf[0] | (buf[1] << 8);
#endif
	return 0;
}
/*
 *report the key information
 */
static int key_sta_old = 0;
static int robam_key_report_value(struct robam_key_data *data)
{
	struct key_event *event = &data->event;
    if(event->key_sta != key_sta_old){
        key_sta_old = event->key_sta;
    }else{
        return 0;
    }
#ifdef CONFIG_KEYBOARD_ROBAM_TWO_KEY
	input_report_key(data->input_dev, KEY_1, (event->key_sta) & ROBAM_POWER_BIT);
	input_report_key(data->input_dev, KEY_2, (event->key_sta) & ROBAM_LIGHT_BIT);
#endif
#ifdef CONFIG_KEYBOARD_ROBAM_FOUR_KEY
	input_report_key(data->input_dev, KEY_1, (event->key_sta) & ROBAM_POWER_BIT);
	input_report_key(data->input_dev, KEY_2, (event->key_sta) & ROBAM_LIGHT_BIT);
	input_report_key(data->input_dev, KEY_3, (event->key_sta) & ROBAM_KEY3_BIT);
	input_report_key(data->input_dev, KEY_4, (event->key_sta) & ROBAM_KEY4_BIT);
#endif
#ifdef CONFIG_KEYBOARD_ROBAM_SIX_KEY
	input_report_key(data->input_dev, KEY_1, (event->key_sta) & ROBAM_KEY1_BIT);
	input_report_key(data->input_dev, KEY_2, (event->key_sta) & ROBAM_KEY2_BIT);
	input_report_key(data->input_dev, KEY_3, (event->key_sta) & ROBAM_KEY3_BIT);
	input_report_key(data->input_dev, KEY_4, (event->key_sta) & ROBAM_KEY4_BIT);
	input_report_key(data->input_dev, KEY_5, (event->key_sta) & ROBAM_KEY5_BIT);
	input_report_key(data->input_dev, KEY_6, (event->key_sta) & ROBAM_KEY6_BIT);
#endif

	input_sync(data->input_dev);
	return 0;

}
static void robam_key_work_handler(struct robam_key_data *robam_key)
{
	int ret = 0;
	ret = robam_key_read_keydata(robam_key);
	if (ret == 0)
		robam_key_report_value(robam_key);
}

static int robam_key_thread_func(void *data)
{
	struct robam_key_data *robam_key = (struct robam_key_data*)data;
	while (1) {
		msleep(100);
		robam_key_work_handler(robam_key);
	}

	return 0;
}

static int  robam_key_create_thread (struct robam_key_data* robam_key) {
	struct task_struct *p =NULL;
	int rc = 0;

	p = kthread_run(robam_key_thread_func, (void*)robam_key,"kthread_run");
	if (IS_ERR(p))
	{
		rc = PTR_ERR(p);
		printk(" error %d create thread_name thread", rc);
	}

	return 0;
}

static int robam_key_probe(struct i2c_client *client,
			   const struct i2c_device_id *id)
{
	struct robam_key_data *robam_key;
	struct input_dev *input_dev;
	int err = 0;

	if (!i2c_check_functionality(client->adapter, I2C_FUNC_I2C)) {
		err = -ENODEV;
		goto exit_check_functionality_failed;
	}

	robam_key = kzalloc(sizeof(struct robam_key_data), GFP_KERNEL);

	if (!robam_key) {
		err = -ENOMEM;
		goto exit_alloc_data_failed;
	}

	i2c_set_clientdata(client, robam_key);
	robam_key->client = client;
	robam_key_client  = client;

	input_dev = input_allocate_device();
	if (!input_dev) {
		err = -ENOMEM;
		dev_err(&client->dev, "failed to allocate input device\n");
		goto exit_input_dev_alloc_failed;
	}
	robam_key->input_dev = input_dev;
#ifdef CONFIG_KEYBOARD_ROBAM_TWO_KEY
	set_bit(KEY_1, input_dev->keybit);
	set_bit(KEY_2, input_dev->keybit);
#endif
#ifdef CONFIG_KEYBOARD_ROBAM_FOUR_KEY
	set_bit(KEY_1, input_dev->keybit);
	set_bit(KEY_2, input_dev->keybit);
	set_bit(KEY_3, input_dev->keybit);
	set_bit(KEY_4, input_dev->keybit);
#endif

#ifdef CONFIG_KEYBOARD_ROBAM_SIX_KEY
	set_bit(KEY_1, input_dev->keybit);
	set_bit(KEY_2, input_dev->keybit);
	set_bit(KEY_3, input_dev->keybit);
	set_bit(KEY_4, input_dev->keybit);
	set_bit(KEY_5, input_dev->keybit);
	set_bit(KEY_6, input_dev->keybit);
#endif
	set_bit(EV_KEY, input_dev->evbit);
	input_dev->name = ROBAM_KEY_NAME;
	err = input_register_device(input_dev);
	if (err) {
		dev_err(&client->dev,
			"robam_key_probe: failed to register input device: %s\n",
			dev_name(&client->dev));
		goto exit_input_register_device_failed;
	}

	robam_key_create_thread(robam_key);
	return 0;

exit_input_register_device_failed:
	input_free_device(input_dev);

exit_alloc_data_failed:
exit_check_functionality_failed:
exit_input_dev_alloc_failed:
	return err;
}

static int  robam_key_remove(struct i2c_client *client)
{
	struct robam_key_data *robam_key;
	robam_key = i2c_get_clientdata(client);
	input_unregister_device(robam_key->input_dev);
	i2c_set_clientdata(client, NULL);
	kfree(robam_key);
	return 0;
}

static long robam_key_ioctl(struct file *fd, unsigned int cmd, unsigned long arg)
{
	struct i2c_client *client = (struct i2c_client*) fd->private_data;
	unsigned char write_buf[ROBAM_KEY_SEND_DATA_LEN]= {0};
	int ret = -1;
    unsigned char checksum;
    memset(write_buf, 0, ROBAM_KEY_SEND_DATA_LEN);
	if(cmd != ROBAM_LED){
		return -1;
	}
    
#ifdef CONFIG_KEYBOARD_ROBAM_SIX_KEY
	write_buf[0] = arg & 0xff;
	write_buf[1] = (arg >> 8) & 0xff;
	write_buf[2] = (arg >> 16) & 0xff;
#endif
#ifdef CONFIG_KEYBOARD_ROBAM_TWO_KEY
	write_buf[0] = arg & 0xff;
	write_buf[1] = (arg >> 8) & 0xff;
#endif
#ifdef CONFIG_KEYBOARD_ROBAM_FOUR_KEY
	write_buf[0] = arg & 0xff;
	write_buf[1] = (arg >> 8) & 0xff;
#endif
	checksum = f_IIC_CalcCrc(write_buf, ROBAM_KEY_SEND_DATA_LEN);
	write_buf[ROBAM_KEY_SEND_DATA_LEN-1] = checksum;
	ret = robam_key_i2c_Write(client, write_buf, ROBAM_KEY_SEND_DATA_LEN);
	return ret;
}

static int robam_key_open(struct inode *inode, struct file *fd)
{
	fd->private_data =(void*)robam_key_client;
	return 0;
}

static int robam_key_release(struct inode *inode, struct file *fd)
{
	fd->private_data = NULL;
	return 0;
}

static const struct i2c_device_id robam_key_id[] = {
	{ROBAM_KEY_NAME, 0},
	{}
};

MODULE_DEVICE_TABLE(i2c, robam_key_id);

static struct i2c_driver robam_key_driver = {
	.probe = robam_key_probe,
	.remove = robam_key_remove,
	.id_table = robam_key_id,
	.driver = {
		   .name = ROBAM_KEY_NAME,
		   .owner = THIS_MODULE,
		   },
};

struct file_operations robam_key_fops =
{
	.owner = THIS_MODULE,
	.unlocked_ioctl = robam_key_ioctl,
	.open   = robam_key_open,
	.release = robam_key_release,
};

static int __init robam_key_init(void)
{
	int ret;
    int result;
	ret = i2c_add_driver(&robam_key_driver);
	if (ret) {
		printk(KERN_WARNING "Adding robam key driver failed "
		       "(errno = %d)\n", ret);
	} else {
		pr_info("Successfully added driver %s\n",
			robam_key_driver.driver.name);
	}

	
	devnum = MKDEV(MAJOR_NUM, MINOR_NUM);
	result = register_chrdev_region(devnum, 1, modname);
	if(result < 0)
	{
		printk("robam_key_mod : can't get major number!/n");
		return result;
	}

	cdev_init(&robam_key_cdev, &robam_key_fops);
	robam_key_cdev.owner = THIS_MODULE;
	robam_key_cdev.ops = &robam_key_fops;
	result = cdev_add(&robam_key_cdev, devnum, 1);
	if(result){
		printk("Failed at cdev_add()\n");
	}

	robam_key_class = class_create(THIS_MODULE, classname);

	if(IS_ERR(robam_key_class))	{
		printk("Failed at class_create().Please exec [mknod] before operate the device/n");
	}else{
		device_create(robam_key_class, NULL, devnum,NULL, devicename);
	}

	return ret;
}

static void __exit robam_key_exit(void)
{
	i2c_del_driver(&robam_key_driver);

	cdev_del(&robam_key_cdev);
	device_destroy(robam_key_class, devnum);
	class_destroy(robam_key_class);
	unregister_chrdev_region(devnum, 1);
}
late_initcall(robam_key_init);
//module_init(robam_key_init);
module_exit(robam_key_exit);

MODULE_AUTHOR("wei.yan<wyan@ingenic.com>");
MODULE_DESCRIPTION("robam key driver");
MODULE_LICENSE("GPL");
