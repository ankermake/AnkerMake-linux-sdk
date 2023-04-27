#ifndef __BOARD_H__
#define __BOARD_H__
#include <gpio.h>
#include <soc/gpio.h>


/* ****************************PWM LED START******************************** */
#define RED_LED_PWM_ID          4
#define RED_LED_ACTIVE_LOW      true
#define GREEN_LED_PWM_ID        5
#define GREEN_LED_ACTIVE_LOW    true
#define BLUE_LED_PWM_ID         1
#define BLUE_LED_ACTIVE_LOW     true
/* ****************************PWM LED END********************************** */

/* ****************************GPIO I2C START******************************** */
#define GPIO_I2C1_SDA		GPIO_PB(25)
#define GPIO_I2C1_SCK		GPIO_PB(26)
/* ****************************GPIO I2C END********************************** */

/* ****************************GPIO KEY START******************************** */
#define GPIO_HOME		GPIO_PC(1)	/*sw3*/
#define ACTIVE_LOW_HOME		0
#define GPIO_UP			GPIO_PC(28)	/*sw4*/
#define ACTIVE_LOW_UP           1
#define GPIO_LEFT		GPIO_PC(0)	/*sw2*/
#define ACTIVE_LOW_LEFT		1
#define GPIO_RIGHT		GPIO_PC(27)	/*sw1*/
#define ACTIVE_LOW_RIGHT        1
/* ****************************GPIO KEY END********************************** */


/* ****************************GPIO MMC START******************************** */
#define GPIO_MMC_RST_N			-1
#define GPIO_MMC_RST_N_LEVEL	        -1
//#define GPIO_MMC_CD_N			GPIO_PC(16)
#define GPIO_MMC_CD_N			GPIO_PB(1)
#define GPIO_MMC_CD_N_LEVEL		LOW_ENABLE
#define GPIO_MMC_PWR			-1
#define GPIO_MMC_PWR_LEVEL		-1
#define GPIO_MMC_WP_N			-1
#define GPIO_MMC_WP_N_LEVEL		-1
/* ****************************GPIO MMC END******************************** */

/* ****************************GPIO USB START******************************** */
#define GPIO_USB_ID				-1
#define GPIO_USB_ID_LEVEL		-1
#define GPIO_USB_DETE			-1
#define GPIO_USB_DETE_LEVEL		-1
#define GPIO_USB_DRVVBUS		-1
#define GPIO_USB_DRVVBUS_LEVEL	-1
/* ****************************GPIO USB END********************************** */

/* ****************************GPIO AUDIO START****************************** */
#define GPIO_SPEAKER_EN				GPIO_PB(3)	/*speaker enable gpio*/
#define GPIO_SPEAKER_EN_LEVEL		1
#if defined(CONFIG_SND_SOC_SY6026L)
#define GPIO_SY6026L_RST			GPIO_PB(5)
#define GPIO_SY6026L_RST_LEVEL		0
#define CPIO_SY6026L_FAULT_EN		GPIO_PC(23)
#define CPIO_SY6026L_FAULT_EN_LEVEL	0
#define SY6026L_I2C_ADDR			0x2a
#define SY6026L_I2CBUS_NUM			1
#endif
#if defined(CONFIG_SND_ASOC_JZ_ICDC_D4)
#define MIC_DEF_VOL_REGVAL	(16)
#define HP_DEF_VOL_REGVAL	(24)
#define MIC_MAX_VOL_REGVAL	(17)
#define HP_MAX_VOL_REGVAL	(26)
#endif
/* ****************************GPIO AUDIO END******************************** */

/* ****************************GPIO WIFI START******************************* */
#define WL_MMC_NUM				1 /*sdio use MMC1*/
#define GPIO_BT_DIS_N           GPIO_PB(29)	/* AP_EN_BTREG */
#define GPIO_BT_HOST_WAKE_DEV   GPIO_PB(28)	/* AP_WAKEUP_BT */
#define GPIO_BT_DEV_WAKE_HOST   GPIO_PB(27)	/* BT_WAKEUP_AP */
#define GPIO_WL_DIS_N			GPIO_PB(30)	/* AP_EN_WLREG  WL_DIS_N power on */
#define GPIO_WL_DEV_WAKE_HOST   GPIO_PB(31)	/* WL_WAKEUP_AP  interrupt */
#define GPIO_EN_BT_WIFI_VDD		GPIO_PB(18)
/* ****************************GPIO WIFI END********************************* */


/* ****************************GPIO LED START******************************* */
#define GPIO_LED_R	GPIO_PC(15)
#define GPIO_LED_G	GPIO_PC(16)
#define GPIO_LED_B	GPIO_PC(12)
/* ****************************GPIO LED END********************************* */

#define GPIO_EFUSE_VDDQ			GPIO_PB(17)	/* EFUSE must be -ENODEV or a gpio */
#endif /* __BOARD_H__ */
