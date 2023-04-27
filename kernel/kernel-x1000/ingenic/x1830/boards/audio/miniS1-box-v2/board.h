#ifndef __BOARD_H__
#define __BOARD_H__
#include <gpio.h>
#include <soc/gpio.h>


/* ****************************PWM LED START******************************** */
#define RED_LED_PWM_ID          0
#define RED_LED_ACTIVE_LOW      false
#define GREEN_LED_PWM_ID        1
#define GREEN_LED_ACTIVE_LOW    false
#define BLUE_LED_PWM_ID         2
#define BLUE_LED_ACTIVE_LOW     false
/* ****************************PWM LED END********************************** */

/* ****************************GPIO I2C START******************************** */
#define GPIO_I2C2_SDA		GPIO_PC(27)
#define GPIO_I2C2_SCK		GPIO_PC(28)

#define SI1143_INT_GPIO     	GPIO_PB(13)
#define SI1143_I2C_BUS_NUM	2

#define AW9163_INT_GPIO     GPIO_PB(13)
#define AW9163_PDN_GPIO     -1
#define AW9163_I2C_BUS_NUM  2

#define GPIO_I2C0_SDA		GPIO_PA(12)
#define GPIO_I2C0_SCK		GPIO_PA(13)

#define SMA1301_ES7241_I2C_ADDR         0x1e
#define SMA1301_ES7241_I2CBUS_NUM       0
#define SMA1301_ES7241_POWER_GPIO       GPIO_PB(14)
#define SMA1301_ES7241_POWER_ON_LEVEL   1
/* ****************************GPIO I2C END********************************** */

/* ****************************GPIO KEY START******************************** */
#define GPIO_VOLUMEDOWN			GPIO_PB(7)
#define ACTIVE_LOW_VOLUMEDOWN	1
#define GPIO_VOLUMEUP			GPIO_PB(6)
#define ACTIVE_LOW_VOLUMEUP		1
#define GPIO_MICMUTE			GPIO_PB(9)
#define ACTIVE_LOW_MICMUTE		1
#define GPIO_PLAY				GPIO_PB(8)
#define ACTIVE_LOW_PLAY			1

#define GPIO_WLAN				GPIO_PC(0)
#define ACTIVE_LOW_WLAN			1
#define GPIO_BLUETOOTH			GPIO_PC(1)
#define ACTIVE_LOW_BLUETOOTH	0
/* ****************************GPIO KEY END********************************** */

/* ****************************GPIO SLCD START******************************** */
#define GPIO_LCD_POWER_EN		GPIO_PB(11)
#define GPIO_LCD_CS				GPIO_PD(18)
#define GPIO_LCD_RD				GPIO_PD(26)
#define GPIO_LCD_RST			GPIO_PD(8)
/* ****************************GPIO SLCD END********************************** */


/* ****************************GPIO USB START******************************** */
#define GPIO_USB_ID				-1
#define GPIO_USB_ID_LEVEL		-1
#define GPIO_USB_DETE			-1
#define GPIO_USB_DETE_LEVEL		-1
#define GPIO_USB_DRVVBUS		-1
#define GPIO_USB_DRVVBUS_LEVEL	-1
/* ****************************GPIO USB END********************************** */
#ifndef CONFIG_SND_ASOC_JZ_ICDC_D4
/* ****************************GPIO AUDIO START****************************** */
#define GPIO_HP_MUTE		-1	/*hp mute gpio*/
#define GPIO_HP_MUTE_LEVEL	-1	/*vaild level*/

#define GPIO_SPEAKER_EN	   GPIO_PB(15)	      /*speaker enable gpio*/
#define GPIO_SPEAKER_EN_LEVEL	1

#define GPIO_HANDSET_EN		-1	/*handset enable gpio*/
#define GPIO_HANDSET_EN_LEVEL   -1

#define	GPIO_HP_DETECT	-1		/*hp detect gpio*/
#define GPIO_HP_INSERT_LEVEL    1
#define GPIO_MIC_SELECT		-1	/*mic select gpio*/
#define GPIO_BUILDIN_MIC_LEVEL	-1	/*builin mic select level*/
#define GPIO_MIC_DETECT		-1
#define GPIO_MIC_INSERT_LEVEL   -1
#define GPIO_MIC_DETECT_EN	-1  /*mic detect enable gpio*/
#define GPIO_MIC_DETECT_EN_LEVEL -1 /*mic detect enable gpio*/

#define HP_SENSE_ACTIVE_LEVEL	1
#define HOOK_ACTIVE_LEVEL		-1

#define HP_MIN_VOLUME_REGVAL 0
#define HP_MAX_VOLUME_REGVAL 23
#define HP_DEFAULT_VOLUME_REGVAL 13
/* ****************************GPIO AUDIO END******************************** */
#else
/* ****************************GPIO AUDIO START****************************** */
#define GPIO_SPEAKER_EN				GPIO_PB(15)	/*speaker enable gpio*/
#define GPIO_SPEAKER_EN_LEVEL		1
#if defined(CONFIG_SND_ASOC_JZ_ICDC_D4)
#define MIC_DEF_VOL_REGVAL	(13)
#define HP_DEF_VOL_REGVAL	(24)
#define MIC_MAX_VOL_REGVAL	(31)
#define HP_MAX_VOL_REGVAL	(24)
#endif
/* ****************************GPIO AUDIO END******************************** */
#endif

/* ****************************GPIO WIFI START******************************* */
#define WL_MMC_NUM				1 /*sdio use MMC1*/
#define GPIO_BT_DIS_N           GPIO_PB(29)	/* AP_EN_BTREG */
#define GPIO_BT_HOST_WAKE_DEV   GPIO_PB(28)	/* AP_WAKEUP_BT */
#define GPIO_BT_DEV_WAKE_HOST   GPIO_PB(27)	/* BT_WAKEUP_AP */
#define GPIO_WL_DIS_N			GPIO_PB(30)	/* AP_EN_WLREG  WL_DIS_N power on */
#define GPIO_WL_DEV_WAKE_HOST   GPIO_PB(31)	/* WL_WAKEUP_AP  interrupt */
#define GPIO_EN_BT_WIFI_VDD		GPIO_PB(18)
/* ****************************GPIO WIFI END********************************* */


#define GPIO_EFUSE_VDDQ			GPIO_PB(17)	/* EFUSE must be -ENODEV or a gpio */
#endif /* __BOARD_H__ */
