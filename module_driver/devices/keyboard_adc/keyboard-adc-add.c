#include <linux/platform_device.h>
#include <linux/input.h>
#include <linux/module.h>
#include "adc_keyboard.h"

static struct adc_keys_button adc_key_info[] = {
    {
        .code           = -1,       //KEY_UP,
        .value          = 0,        //测量电压值(mv)
    },
    {
        .code           = -1,       //KEY_DOWN,
        .value          = 300,
    },
    {
        .code           = -1,       //KEY_LEFT,
        .value          = 600,
    },
    {
        .code           = -1,       //KEY_RIGHT,
        .value          = 900,
    },
    {
        .code           = -1,       //KEY_MENU,
        .value          = 1200,
    },
    {
        .code           = -1,       //KEY_HOME,
        .value          = 1500,
    },
    {
        .code           = -1,
        .value          = 1800,
    },
    {
        .code           = -1,
        .value          = 1800,
    },
    {
        .code           = -1,       //注意以-1结束本组有效键值
        .value          = 1800,
    },
};

static struct adc_button_data data = {
    .adc_init_value = 1800,     //不按按键时的电压值
    .adc_deviation = 50,        //adc的波动范围，单位:mv
    .adc_key_detectime = 100,   //按键检测间隔，单位:ms
    .buttons = adc_key_info,
};

static struct platform_device adc_keys_dev = {
    .name          = "adc_key",
    .id            = 0,
    .num_resources = 0,
    .dev           = {
        .platform_data = &data,
    }
};

module_param_named(key1_code, adc_key_info[0].code, int, 0644);
module_param_named(key1_value, adc_key_info[0].value, int, 0644);
module_param_named(key2_code, adc_key_info[1].code, int, 0644);
module_param_named(key2_value, adc_key_info[1].value, int, 0644);
module_param_named(key3_code, adc_key_info[2].code, int, 0644);
module_param_named(key3_value, adc_key_info[2].value, int, 0644);
module_param_named(key4_code, adc_key_info[3].code, int, 0644);
module_param_named(key4_value, adc_key_info[3].value, int, 0644);
module_param_named(key5_code, adc_key_info[4].code, int, 0644);
module_param_named(key5_value, adc_key_info[4].value, int, 0644);
module_param_named(key6_code, adc_key_info[5].code, int, 0644);
module_param_named(key6_value, adc_key_info[5].value, int, 0644);
module_param_named(key7_code, adc_key_info[6].code, int, 0644);
module_param_named(key7_value, adc_key_info[6].value, int, 0644);
module_param_named(key8_code, adc_key_info[7].code, int, 0644);
module_param_named(key8_value, adc_key_info[7].value, int, 0644);

module_param_named(adc_init_value, data.adc_init_value, int, 0644);
module_param_named(adc_channel, adc_keys_dev.id, int, 0644);
module_param_named(adc_deviation, data.adc_deviation, int, 0644);
module_param_named(adc_key_detectime, data.adc_key_detectime, int, 0644);

int adc_keyboard_dev_init(void)
{
    platform_device_register(&adc_keys_dev);

    return 0;
}

int adc_keyboard_dev_deinit(void)
{
    platform_device_unregister(&adc_keys_dev);

    return 0;
}
