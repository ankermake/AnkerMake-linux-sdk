#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/spinlock.h>
#include <linux/input.h>
#include <linux/delay.h>
#include <linux/slab.h>
#include <linux/platform_device.h>
#include "adc_keyboard.h"

struct adc_keyboard {
    int adc_channel;   //adc通道
    int is_open;
    struct input_dev *adc_input_dev;
    struct workqueue_struct *aux_workqueue;
    struct work_struct aux_work;
    struct adc_button_data *key_map;
    void *pdata;
};

static unsigned int debug = 0;
module_param(debug, int, 0644);

extern int adc_enable(void);
extern int adc_disable(void);
extern int adc_read_channel_voltage(unsigned int channel);

static unsigned int get_keyboard_code(struct adc_keyboard *keyboard, int adc_value)
{
    int max, min;
    int index = 0;
    struct adc_button_data *key_map;

    key_map = keyboard->key_map;
    do {
        max = key_map->buttons[index].value + key_map->adc_deviation;
        min = key_map->buttons[index].value - key_map->adc_deviation;
        //printk("max = %d\n", u16AinMax);
        //printk("min = %d\n", u16AinMin);
        if( (adc_value > min) && (adc_value < max) )
        {
            return key_map->buttons[index].code;
        }
        index++;
    } while (key_map->buttons[index].code != -1);

    return 0;
}


static void report_key_value(struct input_dev* adc_input_dev, unsigned int code, int is_down)
{
    input_event(adc_input_dev, EV_KEY, code, is_down);  //key up
    input_sync(adc_input_dev);
}

static void auxwork_handler(struct work_struct *p_work)
{
    unsigned int time = 0;
    unsigned int adc_val = 0;
    unsigned int old_key = 0;
    unsigned int key = 0;
    struct adc_keyboard *keyboard;
    struct adc_button_data *key_map;

    keyboard = container_of(p_work, struct adc_keyboard, aux_work);
    key_map = keyboard->key_map;
    time = key_map->adc_key_detectime;

    adc_enable();

    while (1) {
        if (!keyboard->is_open)
            break;

        adc_val = adc_read_channel_voltage(keyboard->adc_channel);
        if (debug)
            printk(KERN_EMERG "adc_val = %d\n", adc_val);
        if (adc_val < 0) {
            printk(KERN_ERR "jz_adc_aux read value error !!\n");
            goto to_sleep;
        }

        key = get_keyboard_code(keyboard, adc_val);
        if(key != old_key) {
            if (old_key)
                report_key_value(keyboard->adc_input_dev, old_key, 0);
            if (key)
                report_key_value(keyboard->adc_input_dev, key, 1);

            old_key = key;
        }

to_sleep:
        usleep_range(time*1000,time*1000);
    }
}

static int adc_keyboard_open(struct input_dev *dev)
{
    struct adc_keyboard *keyboard;

    keyboard = input_get_drvdata(dev);
    if (NULL == keyboard) {
        printk(KERN_ERR "Failed input_get_drvdata\n");
        return -1;
    }
    keyboard->is_open = true;
    keyboard->aux_workqueue = create_singlethread_workqueue("aux_workqueue");
    if (!keyboard->aux_workqueue) {
        printk(KERN_ERR "Failed to create test_workqueue\n");
        return -1;
    }

    INIT_WORK(&keyboard->aux_work, auxwork_handler);
    queue_work(keyboard->aux_workqueue, &keyboard->aux_work);

    return 0;
}

static void adc_keyboard_close(struct input_dev *dev)
{
    struct adc_keyboard *keyboard;

    keyboard = input_get_drvdata(dev);
    keyboard->is_open = false;
    destroy_workqueue(keyboard->aux_workqueue);

    adc_disable();
}


static int keyboard_probe(struct platform_device *platform_dev)
{
    int ret;
    int index = 0;
    struct adc_keyboard *keyboard_info;
    struct adc_button_data *key_map;
    struct input_dev *adc_input_dev;

    keyboard_info = kzalloc(sizeof(struct adc_keyboard), GFP_KERNEL);
    if (!keyboard_info) {
        printk(KERN_ERR "Failed to allocate driver structre\n");
        return -ENOMEM;
    }
    if (!(keyboard_info->adc_input_dev = input_allocate_device())) {
        printk(KERN_ERR "jz adc keyboard drvier allocate memory failed!\n");
        return ENOMEM;
    }

    adc_input_dev = keyboard_info->adc_input_dev;
    adc_input_dev->name = "jz adc keyboard";
    // adc_input_dev->phys = "input/event0";
    adc_input_dev->id.bustype = BUS_HOST;
    adc_input_dev->id.vendor  = 0x0005;
    adc_input_dev->id.product = 0x0001;
    adc_input_dev->id.version = 0x0100;

    adc_input_dev->open    = adc_keyboard_open;
    adc_input_dev->close   = adc_keyboard_close;
    adc_input_dev->evbit[0] = BIT(EV_KEY) | BIT(EV_SYN);

    keyboard_info->adc_channel = platform_dev->id;
    keyboard_info->key_map = (struct adc_button_data *)platform_dev->dev.platform_data;

    key_map = keyboard_info->key_map;
    do {
        set_bit(key_map->buttons[index].code, adc_input_dev->keybit);
        index++;
    } while (key_map->buttons[index].code != -1);

    input_set_drvdata(keyboard_info->adc_input_dev, keyboard_info);
    platform_set_drvdata(platform_dev, keyboard_info);

    ret = input_register_device(keyboard_info->adc_input_dev);
    if (ret) {
        input_free_device(keyboard_info->adc_input_dev);
        return ret;
    }


    printk("jz adc keyboard driver has been initialized successfully!\n");

    return 0;
}


static int keyboard_remove(struct platform_device *platform_dev)
{
    struct adc_keyboard *keyboard_info;

    keyboard_info = platform_get_drvdata(platform_dev);
    input_unregister_device(keyboard_info->adc_input_dev);
    kfree(keyboard_info);

    return 0;
}


struct platform_driver adc_keys_drv = {
    .probe   = keyboard_probe,
    .remove  = keyboard_remove,
    .driver  = {
          .owner = THIS_MODULE,
          .name = "adc_key",
        },
};

extern int adc_keyboard_dev_init(void);

int __init adc_keyboard_drv_init(void)
{
    adc_keyboard_dev_init();

    platform_driver_register(&adc_keys_drv);

    return 0;
}

extern int adc_keyboard_dev_deinit(void);

void __exit adc_keyboard_drv_exit(void)
{
    adc_keyboard_dev_deinit();

    platform_driver_unregister(&adc_keys_drv);
}

module_init(adc_keyboard_drv_init);
module_exit(adc_keyboard_drv_exit);

MODULE_LICENSE("GPL");
MODULE_DESCRIPTION("x2000 ADC_KEYBOARD driver");