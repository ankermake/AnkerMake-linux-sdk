#ifndef __BOARD_H__
#define __BOARD_H__
#include <gpio.h>
#include <soc/gpio.h>


#define GPIO_SPI0_MISO           GPIO_PC(11)
#define GPIO_SPI0_MOSI           GPIO_PC(12)
#define GPIO_SPI0_SCK            GPIO_PC(15)

#define IM501_IRQ_GPIO          GPIO_PC(9)
#define IM501_CS_GPIO           GPIO_PC(16)
#define IM501_SPI_BUS_NUM       0
#define IM501_SPI_CS            0

/* ****************************PWM LED START******************************** */
#define RED_LED_PWM_ID          4
#define RED_LED_ACTIVE_LOW      true
#define GREEN_LED_PWM_ID        5
#define GREEN_LED_ACTIVE_LOW    true
#define BLUE_LED_PWM_ID         1
#define BLUE_LED_ACTIVE_LOW     true
/* ****************************PWM LED END********************************** */

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

/* ****************************GPIO AUDIO START****************************** */
#define GPIO_SPEAKER_EN				GPIO_PB(15)	/*speaker enable gpio*/
#define GPIO_SPEAKER_EN_LEVEL		1
#if defined(CONFIG_SND_ASOC_JZ_ICDC_D4)
#define MIC_DEF_VOL_REGVAL	(13)
#define HP_DEF_VOL_REGVAL	(24)
#define MIC_MAX_VOL_REGVAL	(31)
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

#define GPIO_LCD_POWER_EN		GPIO_PB(8)
#define GPIO_LCD_CS				GPIO_PD(18)
#define GPIO_LCD_RD				GPIO_PD(26)
#define GPIO_LCD_RST			GPIO_PD(14)

#define GPIO_EFUSE_VDDQ			GPIO_PB(17)	/* EFUSE must be -ENODEV or a gpio */
#endif /* __BOARD_H__ */
