#ifndef __BOARD_H__
#define __BOARD_H__
#include <gpio.h>
#include <soc/gpio.h>

#define GPIO_SPI0_MISO          GPIO_PC(11)
#define GPIO_SPI0_MOSI          GPIO_PC(12)
#define GPIO_SPI0_SCK           GPIO_PC(15)

#define IM501_RTC_CLK_PWM_ID    2
#define IM501_IRQ_GPIO          GPIO_PC(14)
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
#define GPIO_HOME               GPIO_PC(1)
#define ACTIVE_LOW_HOME         0
#define GPIO_LEFT               GPIO_PB(2)
#define ACTIVE_LOW_LEFT         1
#define GPIO_DOWN               GPIO_PB(3)
#define ACTIVE_LOW_DOWN         1
#define GPIO_RIGHT              GPIO_PB(4)
#define ACTIVE_LOW_RIGHT        1
#define GPIO_UP                 GPIO_PB(5)
#define ACTIVE_LOW_UP           1
/* ****************************GPIO KEY END********************************** */

/* ****************************GPIO SLCD START******************************** */
#define GPIO_LCD_POWER_EN       GPIO_PD(8)
#define GPIO_LCD_CS             GPIO_PD(18)
#define GPIO_LCD_RD             -1
#define GPIO_LCD_RST            GPIO_PD(14)
/* ****************************GPIO SLCD END********************************** */


/* ****************************GPIO USB START******************************** */
#define GPIO_USB_ID             -1
#define GPIO_USB_ID_LEVEL       -1
#define GPIO_USB_DETE           -1
#define GPIO_USB_DETE_LEVEL     -1
#define GPIO_USB_DRVVBUS        -1
#define GPIO_USB_DRVVBUS_LEVEL  -1
/* ****************************GPIO USB END********************************** */


/* ****************************GPIO AUDIO START****************************** */
#define GPIO_HP_MUTE		-1	/*hp mute gpio*/
#define GPIO_HP_MUTE_LEVEL	-1	/*vaild level*/

#define GPIO_SPEAKER_EN	   GPIO_PB(9)	      /*speaker enable gpio*/
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

/* ****************************GPIO WIFI START******************************* */
#define WL_MMC_NUM                1 /*sdio use MMC1*/
#define GPIO_BT_DIS_N           GPIO_PB(29)    /* AP_EN_BTREG */
#define GPIO_BT_HOST_WAKE_DEV   GPIO_PB(28)    /* AP_WAKEUP_BT */
#define GPIO_BT_DEV_WAKE_HOST   GPIO_PB(27)    /* BT_WAKEUP_AP */
#define GPIO_BT_UART_RTS        GPIO_PB(21)
#define GPIO_WL_DIS_N           GPIO_PB(30)    /* AP_EN_WLREG  WL_DIS_N power on */
#define GPIO_WL_DEV_WAKE_HOST   GPIO_PB(31)    /* WL_WAKEUP_AP  interrupt */
#define GPIO_EN_BT_WIFI_VDD     GPIO_PC(9)
/* ****************************GPIO WIFI END********************************* */

#define GPIO_I2C1_SDA       GPIO_PB(25)
#define GPIO_I2C1_SCK       GPIO_PB(26)

#define GPIO_GSL1680_SHUTDOWN   GPIO_PD(16)
#define GPIO_GSL1680_INT        GPIO_PD(26)
#define GSL1680_I2CBUS_NUM      1

#define GPIO_EFUSE_VDDQ         GPIO_PC(17)    /* EFUSE must be -ENODEV or a gpio */

/* ****************************CAMERA START***************************** */
#include <tx-isp/tx-isp-common.h>

#define CIM1_SENSOR_NAME        "gc0328"
#define CIM1_GPIO_POWER         GPIO_PA(19)
#define CIM1_GPIO_SENSOR_PWDN   -1
#define CIM1_GPIO_SENSOR_RST    GPIO_PA(20)
#define CIM1_GPIO_I2C_SEL1      GPIO_PA(8)
#define CIM1_GPIO_I2C_SEL2      -1
#define CIM1_DVP_GPIO_FUNC      DVP_PA_LOW_8BIT
/* ****************************CAMERA END******************************* */

#endif /* __BOARD_H__ */
