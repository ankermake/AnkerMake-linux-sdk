#ifndef _ADC_KEYBOARD_H_
#define _ADC_KEYBOARD_H_

struct adc_keys_button {
    /* Configuration parameters */
    unsigned int code;      //按键值
    int value;              //测量电压值(mv)
};

struct adc_button_data {
    unsigned int adc_init_value;        //不按按键时的电压值
    unsigned int adc_deviation;         //adc的波动范围，单位:mv
    unsigned int adc_key_detectime;     //按键检测间隔，单位:ms
    struct adc_keys_button *buttons;
};

#endif