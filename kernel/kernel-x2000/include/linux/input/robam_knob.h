#ifndef __LINUX_ROBAM_KEY_H__
#define __LINUX_ROBAM_KEY_H__

#define ROBAM_KNOB_NAME	"robam_knob"

#define ROBAM_LED   _IOW('R', 1, int)    //照明按键背景LED6


#define ROBAM_LIGHT_BIT   0x10    //照明按键位
#define ROBAM_POWER_BIT   0x20    //电源按键位

#define ROBAM_KEY_DATA_LENGTH   3

#endif
