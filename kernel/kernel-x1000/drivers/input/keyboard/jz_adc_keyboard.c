#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/spinlock.h>
#include <linux/input.h>
#include <linux/delay.h>
#include <linux/slab.h>
#include <linux/gpio_keys.h>
#include <linux/platform_device.h>

#define CHANGE_RANGE           50      //adc的波动范围

struct jz_keyboard {
    int adc_ref;  //按键没按下时的AD值
    int adc_ch;   //adc通道
    int is_open;
    struct input_dev *jz_input_dev;
    struct workqueue_struct *aux_workqueue;
    struct work_struct aux_work;
    struct gpio_keys_button *key_map;
    void *pdata;
};

extern int get_adc_aux(int ch);

static unsigned int keys_map(struct jz_keyboard *keyboard, int adc_value)
{
    int max, min;
    int index = 0;

    do {
        max = keyboard->key_map[index].value + CHANGE_RANGE;
        min = keyboard->key_map[index].value - CHANGE_RANGE;
        //printk("max = %d\n", u16AinMax);
        //printk("min = %d\n", u16AinMin);
        if( (adc_value > min) && (adc_value < max) )
        {
            return keyboard->key_map[index].code;
        }
        index++;
    } while (keyboard->key_map[index].code != -1);

    return 0;
}


void report_key_value(struct input_dev* jz_input_dev, unsigned int code, int is_down)
{
    input_event(jz_input_dev, EV_KEY, code, is_down);  //key up
    input_sync(jz_input_dev);
}


void auxwork_handler(struct work_struct *p_work)
{
    unsigned int sadc_val = 0;
    static unsigned int old_map = 0;
    static unsigned int new_map = 0;
    struct jz_keyboard *keyboard;

    keyboard = container_of(p_work, struct jz_keyboard, aux_work);

    while (1) {
        sadc_val = get_adc_aux(keyboard->adc_ch);
        //printk("sadc_val = %d\n", sadc_val);
        if (false == keyboard->is_open) break;
        if (sadc_val < 0) {
            printk("jz_adc_aux read value error !!\n");
        } else {
            if (sadc_val < keyboard->adc_ref-CHANGE_RANGE || sadc_val > keyboard->adc_ref+CHANGE_RANGE) {
                new_map = keys_map(keyboard, sadc_val);
                if (new_map != 0) {
                    if (old_map != 0 && new_map != old_map) {
                        report_key_value(keyboard->jz_input_dev, old_map, 0);
                    }
                    report_key_value(keyboard->jz_input_dev, new_map, 1);
                }
                old_map = new_map;
            }
            else if (new_map != 0) {
                report_key_value(keyboard->jz_input_dev, new_map, 0);
            }
        }
        msleep(100);
    }
}

static int jz_keyboard_open(struct input_dev *dev)
{
    struct jz_keyboard *keyboard;

    keyboard = input_get_drvdata(dev);
    if (NULL == keyboard) {
        printk("Failed input_get_drvdata\n");
        return -1;
    }
    keyboard->is_open = true;
    keyboard->aux_workqueue = create_workqueue("aux_workqueue");
    if (!keyboard->aux_workqueue)
        printk("Failed to create test_workqueue\n");

    INIT_WORK(&keyboard->aux_work, auxwork_handler);
    queue_work(keyboard->aux_workqueue, &keyboard->aux_work);

    return 0;
}

static void jz_keyboard_close(struct input_dev *dev)
{
    struct jz_keyboard *keyboard;

    keyboard = input_get_drvdata(dev);
    keyboard->is_open = false;
    destroy_workqueue(keyboard->aux_workqueue);
}


int keyboard_init_probe(struct platform_device *platform_dev)
{
    int ret;
    int index = 0;
    struct jz_keyboard *keyboard_info;

    keyboard_info = kzalloc(sizeof(struct jz_keyboard), GFP_KERNEL);
    if (!keyboard_info) {
        printk("Failed to allocate driver structre\n");
        return -ENOMEM;
    }
    if (!(keyboard_info->jz_input_dev = input_allocate_device())) {
        printk("jz adc keyboard drvier allocate memory failed!\n");
        return ENOMEM;
    }

    keyboard_info->jz_input_dev->name = "jz adc keyboard";
    // jz_input_dev->phys = "input/event0";
    keyboard_info->jz_input_dev->id.bustype = BUS_HOST;
    keyboard_info->jz_input_dev->id.vendor  = 0x0005;
    keyboard_info->jz_input_dev->id.product = 0x0001;
    keyboard_info->jz_input_dev->id.version = 0x0100;

    keyboard_info->jz_input_dev->open    = jz_keyboard_open;
    keyboard_info->jz_input_dev->close   = jz_keyboard_close;
    keyboard_info->jz_input_dev->evbit[0] = BIT(EV_KEY) | BIT(EV_SYN);

    keyboard_info->adc_ch = platform_dev->id;
    keyboard_info->key_map = (struct gpio_keys_button *)platform_dev->dev.platform_data;

    do {
        set_bit(keyboard_info->key_map[index].code, keyboard_info->jz_input_dev->keybit);
        index++;
    } while (keyboard_info->key_map[index].code != -1);
    keyboard_info->adc_ref = keyboard_info->key_map[index].value;

    input_set_drvdata(keyboard_info->jz_input_dev, keyboard_info);
    platform_set_drvdata(platform_dev, keyboard_info);

    ret = input_register_device(keyboard_info->jz_input_dev);
    if (ret) {
        input_free_device(keyboard_info->jz_input_dev);
        return ret;
    }


    printk("jz adc keyboard driver has been initialized successfully!\n");

    return 0;
}


static int keyboard_remove(struct platform_device *platform_dev)
{
    struct jz_keyboard *keyboard_info;

    keyboard_info = platform_get_drvdata(platform_dev);
    input_unregister_device(keyboard_info->jz_input_dev);
    kfree(keyboard_info);

    return 0;
}

#ifdef CONFIG_KEYBOARD_JZ_ADC0
struct platform_driver adc0_keys_drv = {
    .probe   = keyboard_init_probe,
    .remove  = keyboard_remove,
    .driver  = {
          .owner = THIS_MODULE,
          .name = "adc0_key",
        },
};
#endif

#ifdef CONFIG_KEYBOARD_JZ_ADC1
struct platform_driver adc1_keys_drv = {
    .probe   = keyboard_init_probe,
    .remove  = keyboard_remove,
    .driver  = {
          .owner = THIS_MODULE,
          .name = "adc1_key",
        },
};
#endif

#ifdef CONFIG_KEYBOARD_JZ_ADC2
struct platform_driver adc2_keys_drv = {
    .probe   = keyboard_init_probe,
    .remove  = keyboard_remove,
    .driver  = {
          .owner = THIS_MODULE,
          .name = "adc2_key",
        },
};
#endif

#ifdef CONFIG_KEYBOARD_JZ_ADC3
struct platform_driver adc3_keys_drv = {
    .probe   = keyboard_init_probe,
    .remove  = keyboard_remove,
    .driver  = {
          .owner = THIS_MODULE,
          .name = "adc3_key",
        },
};
#endif

int __init adc_keyboard_drv_init(void)
{
#ifdef CONFIG_KEYBOARD_JZ_ADC0
    platform_driver_register(&adc0_keys_drv);
#endif

#ifdef CONFIG_KEYBOARD_JZ_ADC1
    platform_driver_register(&adc1_keys_drv);
#endif

#ifdef CONFIG_KEYBOARD_JZ_ADC2
    platform_driver_register(&adc2_keys_drv);
#endif

#ifdef CONFIG_KEYBOARD_JZ_ADC3
    platform_driver_register(&adc3_keys_drv);
#endif

    return 0;
}


void __exit adc_keyboard_drv_exit(void)
{
#ifdef CONFIG_KEYBOARD_JZ_ADC0
    platform_driver_unregister(&adc0_keys_drv);
#endif

#ifdef CONFIG_KEYBOARD_JZ_ADC1
    platform_driver_unregister(&adc1_keys_drv);
#endif

#ifdef CONFIG_KEYBOARD_JZ_ADC2
    platform_driver_unregister(&adc2_keys_drv);
#endif

#ifdef CONFIG_KEYBOARD_JZ_ADC3
    platform_driver_unregister(&adc3_keys_drv);
#endif
}

module_init(adc_keyboard_drv_init);
module_exit(adc_keyboard_drv_exit);
