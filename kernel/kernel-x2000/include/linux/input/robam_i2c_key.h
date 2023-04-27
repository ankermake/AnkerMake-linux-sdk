#ifndef __LINUX_ROBAM_KEY_H__
#define __LINUX_ROBAM_KEY_H__

#define ROBAM_KEY_NAME	"robam_key_name"

#define ROBAM_LED   _IOW('R', 1, int)    //照明按键背景LED6

#define ROBAM_POWER_BIT  0x10
#define ROBAM_LIGHT_BIT  0x20

#define ROBAM_KEY1_BIT   0x40
#define ROBAM_KEY2_BIT   0x80
#define ROBAM_KEY3_BIT   0x100
#define ROBAM_KEY4_BIT   0x08
#define ROBAM_KEY5_BIT   0x10
#define ROBAM_KEY6_BIT   0x20

#ifdef CONFIG_KEYBOARD_ROBAM_TWO_KEY
#define ROBAM_KEY_DATA_LENGTH   3
#define ROBAM_KEY_SEND_DATA_LEN 3
#endif 

#ifdef CONFIG_KEYBOARD_ROBAM_SIX_KEY
#define ROBAM_KEY_DATA_LENGTH   4
#define ROBAM_KEY_SEND_DATA_LEN 6
#endif 

#endif
