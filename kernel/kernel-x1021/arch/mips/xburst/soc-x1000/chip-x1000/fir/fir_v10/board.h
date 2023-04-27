#ifndef __BOARD_H__
#define __BOARD_H__
#include <gpio.h>
#include <soc/gpio.h>


/* ****************************GPIO KEY START******************************** */
/* #define GPIO_HOME_KEY		GPIO_PD(18) */
/* #define ACTIVE_LOW_HOME		1 */

/* #define GPIO_VOLUMEDOWN_KEY         GPIO_PD(18) */
/* #define ACTIVE_LOW_VOLUMEDOWN	0 */

#define GPIO_ENDCALL_KEY            GPIO_PB(31)
#define ACTIVE_LOW_ENDCALL      0

/* ****************************GPIO KEY END********************************** */


#ifdef CONFIG_BCMDHD_1_141_66

#define GPIO_WIFI_MOD_KEY	    GPIO_PA(20)
#define GPIO_BT_MOD_KEY		    GPIO_PA(21)

#define ACTIVE_LOW_WIFI_MOD	    1
#define ACTIVE_LOW_BT_MOD	    1

/**
 *  ** Bluetooth gpio
 *   **/
#define BLUETOOTH_UPORT_NAME    "ttyS0"
#define GPIO_BT_REG_ON          GPIO_PC(18)
#define GPIO_BT_WAKE            GPIO_PC(20)
#define GPIO_BT_INT             GPIO_PC(19)
#define GPIO_BT_UART_RTS        GPIO_PC(13)

#define GPIO_WIFI_RST_N		GPIO_PC(17)
#define GPIO_WIFI_WAKE 		GPIO_PC(16)

#endif

/* MSC GPIO Definition */
#define GPIO_SD0_CD_N       -1

#define ZIGBEE_WKUP	GPIO_PD(5)
#define ZIGBEE_INT	GPIO_PD(4)
#define ZIGBEE_RST	GPIO_PB(22)
#define ZIGBEE_PWR_EN	GPIO_PB(1)

/* ****************************BT WIFI END********************************** */

#ifdef CONFIG_SPI_GPIO
#define GPIO_SPI_SCK  GPIO_PA(26)
#define GPIO_SPI_MOSI GPIO_PA(29)
#define GPIO_SPI_MISO GPIO_PA(28)
#endif

#if defined(CONFIG_JZ_SPI) || defined(CONFIG_JZ_SFC)
#define SPI_CHIP_ENABLE GPIO_PA(27)
#endif

/* ****************************GPIO USB START******************************** */
#define GPIO_USB_ID            	GPIO_PC(21)
#define GPIO_USB_ID_LEVEL       LOW_ENABLE
#ifdef CONFIG_BOARD_HAS_NO_DETE_FACILITY
#define GPIO_USB_DETE           -1 /*GPIO_PC(22)*/
#define GPIO_USB_DETE_LEVEL     LOW_ENABLE
#else
#define GPIO_USB_DETE           GPIO_PC(22)
#define GPIO_USB_DETE_LEVEL     LOW_ENABLE
#endif
#define GPIO_USB_DRVVBUS        GPIO_PB(25)
#define GPIO_USB_DRVVBUS_LEVEL      HIGH_ENABLE
/* ****************************GPIO USB END********************************** */

/* ****************************GPIO AUDIO START****************************** */
#define GPIO_HP_MUTE        -1  /*hp mute gpio*/
#define GPIO_HP_MUTE_LEVEL  -1  /*vaild level*/

#ifdef CONFIG_HALLEY2_MINI_CORE_V10
#define GPIO_SPEAKER_EN    GPIO_PC(23)         /*speaker enable gpio*/
#define GPIO_SPEAKER_EN_LEVEL   0
#else
#define GPIO_SPEAKER_EN    GPIO_PB(07)         /*speaker enable gpio*/
#define GPIO_SPEAKER_EN_LEVEL   1
#endif

#define GPIO_HANDSET_EN     -1  /*handset enable gpio*/
#define GPIO_HANDSET_EN_LEVEL   -1

#define GPIO_HP_DETECT		-1/*hp detect gpio*/
#define GPIO_HP_INSERT_LEVEL    1
#define GPIO_MIC_SELECT     -1  /*mic select gpio*/
#define GPIO_BUILDIN_MIC_LEVEL  -1  /*builin mic select level*/
#define GPIO_MIC_DETECT     -1
#define GPIO_MIC_INSERT_LEVEL   -1
#define GPIO_MIC_DETECT_EN  -1  /*mic detect enable gpio*/
#define GPIO_MIC_DETECT_EN_LEVEL -1 /*mic detect enable gpio*/

#define HP_SENSE_ACTIVE_LEVEL   1
#define HOOK_ACTIVE_LEVEL       -1

#define GPIO_VOLUMEDOWN_KEY GPIO_PA(22)
#define GPIO_VOLUMEUP_KEY   GPIO_PB(28)

#define ACTIVE_LOW_VOLUMEDOWN 1
#define ACTIVE_LOW_VOLUMEUP   1

/* ****************************GPIO AUDIO END******************************** */

/* ****************************GPIO I2C START******************************** */
#ifndef CONFIG_I2C0_V12_JZ
#define GPIO_I2C0_SDA GPIO_PB(24)
#define GPIO_I2C0_SCK GPIO_PB(23)
#endif
#ifndef CONFIG_I2C1_V12_JZ
#define GPIO_I2C1_SDA GPIO_PC(27)
#define GPIO_I2C1_SCK GPIO_PC(26)
#endif
#ifndef CONFIG_I2C2_V12_JZ
#define GPIO_I2C2_SDA GPIO_PD(1)
#define GPIO_I2C2_SCK GPIO_PD(0)
#endif
/* ****************************GPIO I2C END********************************** */

/* ****************************CIM START******************************** */
#ifdef CONFIG_SENSORS_BMA2X2
#define GPIO_GSENSOR_INTR	GPIO_PB(2)
#endif

#ifdef CONFIG_VIDEO_JZ_CIM_HOST_V13
#define FRONT_CAMERA_INDEX  0
#define BACK_CAMERA_INDEX   1

#define CAMERA_SENSOR_RESET GPIO_PA(24)
#define CAMERA_FRONT_SENSOR_PWDN  GPIO_PA(25)
#define CAMERA_VDD_EN  GPIO_PA(23)
#endif

/* ****************************CIM END******************************** */

/* ****************************BATTERY START******************************** */

#define BATTERY_LOW_PIN		GPIO_PB(13)
#define BATTERY_LOW_FUNC	GPIO_INPUT
#define BATTERY_CHARGE_STATUS	GPIO_PB(10)
#define BATTERY_CHARGE_FUNC	GPIO_INPUT
#define BATTERY_ISET_PIN	GPIO_PB(8)
#define BATTERY_ISET_FUNC	GPIO_OUTPUT0

/* ****************************BATTERY END******************************** */

/* ****************************LCD START******************************** */

#ifdef  CONFIG_LCD_KD035HVFMD057
#define GPIO_LCD_CS     GPIO_PB(18)
#define GPIO_LCD_RD     GPIO_PB(16)
#define GPIO_LCD_RST    GPIO_PB(14)
#define GPIO_BL_PWR_EN  GPIO_PC(25)
#define GPIO_LCD_PWM    GPIO_PB(9)
#define GPIO_LCD_VDD_EN GPIO_PB(19)
#endif


/* TOUCHSCREEN */
#define GPIO_TP_INT     GPIO_PB(11)
#define GPIO_TP_RESET     GPIO_PB(12)
#define GPIO_TP_EN     GPIO_PB(15)

#endif /* __BOARD_H__ */
