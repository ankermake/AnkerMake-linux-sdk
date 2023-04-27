#include <linux/module.h>
#include <linux/init.h>
#include <linux/of.h>
#include <linux/of_device.h>
#include <linux/of_gpio.h>
#include <linux/of_irq.h>
#include <linux/i2c.h>
#include <linux/slab.h>
#include <linux/delay.h>

#define SY7734_DRIVER_NAME                     "sy7734"

struct sy7734_pdata{
	unsigned int gpio_pwen;
	unsigned int gpio_pwen_flags;
};

static const struct i2c_device_id sy7734_id[] = {
	{SY7734_DRIVER_NAME, 0},
	{},
};
static const struct of_device_id sy7734_dt_match[] = {
	{.compatible = "sy7734", },
	{},
};
MODULE_DEVICE_TABLE(of, sy7734_dt_match);

static int sy7734_set_reg(struct i2c_client *client)
{
	int ret = 0;

#if 1
	//PWM模式
	ret = i2c_smbus_write_byte_data(client, 0x00, 0x90);
	ret = i2c_smbus_write_byte_data(client, 0x01, 0x54);
	ret = i2c_smbus_write_byte_data(client, 0x02, 0x63);//0x63=70mA, 0x21=23mA, 0x2a=30mA
#endif

#if 0
	//DC模式下不调节亮度可正常点亮LED
//	msleep(2*1000);
	ret = i2c_smbus_write_byte_data(client, 0x00, 0x95);
	printk("[%s][%s()][+%d]:\t1\n", __FILE__, __func__, __LINE__);
//	msleep(2*1000);
	ret = i2c_smbus_read_byte_data(client, 0x00);
	printk("[%s][%s()][+%d]:\treg_0x00 = 0x%x\n", __FILE__, __func__, __LINE__, ret);


	ret = i2c_smbus_write_byte_data(client, 0x01, 0x54);
	printk("[%s][%s()][+%d]:\t2\n", __FILE__, __func__, __LINE__);
	msleep(2*1000);
	ret = i2c_smbus_read_byte_data(client, 0x01);
	printk("[%s][%s()][+%d]:\treg_0x01 = 0x%x\n", __FILE__, __func__, __LINE__, ret);


	ret = i2c_smbus_write_byte_data(client, 0x02, 0x2a);//0x63=70mA, 0x21=23mA, 0x2a=30mA
	printk("[%s][%s()][+%d]:\t3\n", __FILE__, __func__, __LINE__);
	msleep(2*1000);
	ret = i2c_smbus_read_byte_data(client, 0x02);
	printk("[%s][%s()][+%d]:\treg_0x02 = 0x%x\n", __FILE__, __func__, __LINE__, ret);


	msleep(2*1000);
	ret = i2c_smbus_write_byte_data(client, 0x04, 0x30);
	printk("[%s][%s()][+%d]:\t4\n", __FILE__, __func__, __LINE__);
	msleep(2*1000);
	printk("[%s][%s()][+%d]:\treg_0x04 = 0x%x\n", __FILE__, __func__, __LINE__, ret);


	ret = i2c_smbus_read_byte_data(client, 0x03);
	printk("[%s][%s()][+%d]:\treg_0x03 = 0x%x\n", __FILE__, __func__, __LINE__, ret);
	ret = i2c_smbus_read_byte_data(client, 0x06);
	printk("[%s][%s()][+%d]:\treg_0x06 = 0x%x\n", __FILE__, __func__, __LINE__, ret);
#endif

#if 0
	//DC模式下不调节亮度可正常点亮LED
	msleep(2*1000);
	ret = i2c_smbus_write_byte_data(client, 0x00, 0x95);
	printk("[%s][%s()][+%d]:\t1\n", __FILE__, __func__, __LINE__);
	msleep(2*1000);
	ret = i2c_smbus_write_byte_data(client, 0x01, 0x54);
	printk("[%s][%s()][+%d]:\t2\n", __FILE__, __func__, __LINE__);
	msleep(2*1000);
	ret = i2c_smbus_write_byte_data(client, 0x02, 0x63);//0x63=70mA, 0x21=23mA, 0x2a=30mA
	ret = i2c_smbus_read_byte_data(client, 0x06);
	printk("[%s][%s()][+%d]:\treg_0x06 = 0x%x\n", __FILE__, __func__, __LINE__, ret);
	printk("[%s][%s()][+%d]:\t3\n", __FILE__, __func__, __LINE__);
	msleep(2*1000);
	ret = i2c_smbus_write_byte_data(client, 0x04, 0x80);
	printk("[%s][%s()][+%d]:\t4\n", __FILE__, __func__, __LINE__);
	msleep(2*1000);
	ret = i2c_smbus_read_byte_data(client, 0x04);
	printk("[%s][%s()][+%d]:\treg_0x04 = 0x%x\n", __FILE__, __func__, __LINE__, ret);
#endif
	printk("[%s][%s()][+%d]:\t\n", __FILE__, __func__, __LINE__);
//	return ret;
	return 0;
}

static int sy7734_pwen(struct i2c_client *client, struct sy7734_pdata *sp)
{
	int ret = 0;

	struct sy7734_pdata sp_debug;

	struct device_node* dn = client->dev.of_node;
	sp->gpio_pwen = of_get_named_gpio_flags(dn, "sy7734,pwen", 0, &sp->gpio_pwen_flags);
	if(sp->gpio_pwen < 0){
		printk("Unable to get sy7734 pwen!\n");
		return -1;
	}
	sp->gpio_pwen_flags = sp->gpio_pwen_flags ? 0 : 1;

	if (gpio_is_valid(sp->gpio_pwen)){
		ret = gpio_request(sp->gpio_pwen, "sy7734_pwen");
		if(ret){
			printk("sy7734 pwen gpio request failed!\n");
			return -1;
		}
		printk("[%s][%s()][+%d]:\t\n", __FILE__, __func__, __LINE__);
		ret = gpio_direction_output(sp->gpio_pwen, sp->gpio_pwen_flags);
		if(ret){
			printk("sy7734 pwen gpio output failed!\n");
			return -1;
		}
	}

#if 0
	//将GPC00 PWM PIN申请并拉高
	sp_debug.gpio_pwen = of_get_named_gpio_flags(dn, "sy7734,pwm", 0, &sp_debug.gpio_pwen_flags);
	if(sp_debug.gpio_pwen < 0){
		printk("Unable to get sy7734 pwm!\n");
		return -1;
	}
	sp_debug.gpio_pwen_flags = sp_debug.gpio_pwen_flags ? 0 : 1;

	if (gpio_is_valid(sp_debug.gpio_pwen)){
		ret = gpio_request(sp_debug.gpio_pwen, "sy7734_pwm");
		if(ret){
			printk("sy7734 pwm gpio request failed!\n");
			return -1;
		}
		ret = gpio_direction_output(sp_debug.gpio_pwen, sp_debug.gpio_pwen_flags);
		if(ret){
			printk("sy7734 pwm gpio output failed!\n");
			return -1;
		}
	}
#endif

	printk("[%s][%s()][+%d]:\t\n", __FILE__, __func__, __LINE__);
	return ret;
}

static int sy7734_probe(struct i2c_client *client, const struct i2c_device_id *id)
{
	int ret = 0;
	struct sy7734_pdata *sp = NULL;
	printk("[%s][%s()][+%d]:\t\n", __FILE__, __func__, __LINE__);

	sp = (struct sy7734_pdata *)kzalloc(sizeof(*sp), GFP_KERNEL);
	i2c_set_clientdata(client, sp);
	printk("[%s][%s()][+%d]:\t\n", __FILE__, __func__, __LINE__);
	sy7734_pwen(client, sp);
	printk("[%s][%s()][+%d]:\t\n", __FILE__, __func__, __LINE__);
	ret = sy7734_set_reg(client);
	printk("[%s][%s()][+%d]:\t\n", __FILE__, __func__, __LINE__);
	return ret;
}

static int sy7734_remove(struct i2c_client *client)
{
	kfree(i2c_get_clientdata(client));
	printk("[%s][%s()][+%d]:\t\n", __FILE__, __func__, __LINE__);
}

static struct i2c_driver sy7734_driver = {
	.probe = sy7734_probe,
	.remove = sy7734_remove,
	.driver = {
		.name = SY7734_DRIVER_NAME,
		.owner = THIS_MODULE,
		.of_match_table = of_match_ptr(sy7734_dt_match),
	},
	.id_table = sy7734_id,
};

static int __init sy7734_init(void)
{
	int ret = 0;

	printk("[%s][%s()][+%d]:\t\n", __FILE__, __func__, __LINE__);
	ret = i2c_add_driver(&sy7734_driver);
	if ( ret != 0 ) {
		printk("sy7734 driver init failed!");
	}
	return ret;
}

static void __exit sy7734_exit(void)
{
	i2c_del_driver(&sy7734_driver);
}
module_init(sy7734_init);
module_exit(sy7734_exit);
