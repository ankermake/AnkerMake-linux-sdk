#ifndef __BOARD_H__
#define __BOARD_H__
#include <gpio.h>
#include <linux/jz_dwc.h>
#include <soc/gpio.h>

/* ****************************GPIO LCD START******************************** */
#if defined(CONFIG_LCD_STARTEK_KD050HDFIA019) || defined(CONFIG_LCD_STARTEK_KD050HDFIA020) || defined(CONFIG_LCD_VISIONOX_T078ZC04H01) || defined(CONFIG_LCD_XINLI_X078DTLN06)
#define GPIO_MIPI_RST_N GPIO_PA(14)
#define GPIO_MIPI_PWR GPIO_PA(15)
#endif

#ifdef CONFIG_BACKLIGHT_PWM
#define GPIO_LCD_PWM GPIO_PE(0)
#endif

/* #ifdef CONFIG_BACKLIGHT_DIGITAL_PULSE */
/* #define GPIO_GIGITAL_PULSE      GPIO_PE(0) */
/* #endif */
/* ****************************GPIO LCD END********************************** */

/* ****************************GPIO I2C START******************************** */
#ifdef CONFIG_SOFT_I2C0_GPIO_V12_JZ
#define GPIO_I2C0_SDA GPIO_PD(30)
#define GPIO_I2C0_SCK GPIO_PD(31)
#endif
#ifdef CONFIG_SOFT_I2C1_GPIO_V12_JZ
#define GPIO_I2C1_SDA GPIO_PE(30)
#define GPIO_I2C1_SCK GPIO_PE(31)
#endif
#ifdef CONFIG_SOFT_I2C2_GPIO_V12_JZ
#define GPIO_I2C2_SDA GPIO_PA(12)
#define GPIO_I2C2_SCK GPIO_PA(13)
//#define GPIO_I2C2_SDA GPIO_PE(00)
//#define GPIO_I2C2_SCK GPIO_PE(03)
#endif
#ifdef CONFIG_SOFT_I2C3_GPIO_V12_JZ
#define GPIO_I2C3_SDA GPIO_PB(7)
#define GPIO_I2C3_SCK GPIO_PB(8)
#endif
/* ****************************GPIO I2C END********************************** */

/* ****************************GPIO SPI START******************************** */
#ifndef CONFIG_SPI_GPIO
#define GPIO_SPI_SCK GPIO_PE(15)
#define GPIO_SPI_MOSI GPIO_PE(17)
#define GPIO_SPI_MISO GPIO_PE(14)
#endif
/* ****************************GPIO SPI END********************************** */

/* ****************************GPIO TOUCHSCREEN START************************ */
#ifdef CONFIG_TOUCHSCREEN_GT9XX
#define GPIO_TP_INT		GPIO_PE(3)
#define GPIO_TP_RESET		GPIO_PE(1)
#endif

#ifdef CONFIG_TOUCHSCREEN_CYPRESS_CYTTSP5
#define CYTTSP5_I2C_IRQ_GPIO GPIO_PE(3)
#define CYTTSP5_I2C_RST_GPIO GPIO_PE(1)
#endif

/* ****************************GPIO TOUCHSCREEN END************************** */

/* ****************************GPIO KEY START******************************** */
/* #define GPIO_HOME_KEY		GPIO_PD(18) */
/* #define ACTIVE_LOW_HOME		1 */

#define GPIO_VOLUMEDOWN_KEY GPIO_PD(18)
#define ACTIVE_LOW_VOLUMEDOWN 0

#define GPIO_ENDCALL_KEY GPIO_PA(30)
#define ACTIVE_LOW_ENDCALL 1

/* ****************************GPIO KEY END********************************** */

/* ****************************GPIO EFUSE START****************************** */
#define GPIO_EFUSE_VDDQ -1
#define GPIO_EFUSE_VDDQ_EN_LEVEL 0
/* ****************************GPIO EFUSE END******************************** */

/* ****************************GPIO MMC START******************************** */
#define GPIO_MMC_CD -1
#define GPIO_MMC_CD_LEVEL -1

#define GPIO_MMC_RST_N -1
#define GPIO_MMC_RST_N_LEVEL LOW_ENABLE
/* ****************************GPIO MMC END******************************** */

/* ****************************GPIO USB START******************************** */
/* #define GPIO_USB_ID            GPIO_PA(2) */
/* #define GPIO_USB_ID_LEVEL      LOW_ENABLE */
#define GPIO_USB_DETE GPIO_PE(10)
#define GPIO_USB_DETE_LEVEL HIGH_ENABLE
/* #define GPIO_USB_DRVVBUS       GPIO_PE(10) */
/* #define GPIO_USB_DRVVBUS_LEVEL HIGH_ENABLE */
/* ****************************GPIO USB END********************************** */

/* ****************************GPIO CAMERA START***************************** */
#if defined(CONFIG_SOC_CAMERA_XC6130)
#define CAMERA_RST GPIO_PF(2)
#endif
#define CAMERA_PWDN_N GPIO_PA(0) /* pin conflict with USB_ID */
#define CAMERA_PW_EN GPIO_PA(1)
#define CAMERA_MCLK GPIO_PE(2)   /* no use */
/* ****************************GPIO CAMERA END******************************* */

/* ****************************GPIO AUDIO START****************************** */
/* #define GPIO_HP_MUTE                     -1 /1* hp mute gpio *1/ */
/* #define GPIO_HP_MUTE_LEVEL                1 /1* vaild level *1/ */

/* #define GPIO_SPEAKER_EN                  -1 /1* speaker enable gpio *1/ */
/* #define GPIO_SPEAKER_EN_LEVEL             1 */

/* #define GPIO_HANDSET_EN                  -1 /1* handset enable gpio *1/ */
/* #define GPIO_HANDSET_EN_LEVEL            -1 */

/* #define GPIO_HP_DETECT           GPIO_PA(1) /1* hp detect gpio *1/ */
/* #define GPIO_HP_INSERT_LEVEL              0 */
/* #define GPIO_MIC_SELECT                  -1 /1* mic select gpio *1/ */
/* #define GPIO_BUILDIN_MIC_LEVEL           -1 /1* builin mic select level *1/
 */
/* #define GPIO_MIC_DETECT                  -1 */
/* #define GPIO_MIC_INSERT_LEVEL            -1 */
/* #define GPIO_MIC_DETECT_EN               -1 /1* mic detect enable gpio *1/ */
/* #define GPIO_MIC_DETECT_EN_LEVEL         -1 /1* mic detect enable gpio *1/ */

/* #define HP_SENSE_ACTIVE_LEVEL             1 */
/* #define HOOK_ACTIVE_LEVEL                -1 */
/* ****************************GPIO AUDIO END******************************** */

/* ****************************GPIO WIFI START******************************* */
#define GPIO_WIFI_WAKE  GPIO_PA(9)
#define GPIO_WIFI_RST_N GPIO_PA(8)
#define WLAN_SDIO_INDEX 1
/* ****************************GPIO WIFI END********************************* */

/* ****************************GPIO BLUETOOTH START************************** */
/*
#define HOST_WAKE_BT	GPIO_PA(12)
#define BT_WAKE_HOST	GPIO_PA(10)
#define BT_REG_EN	GPIO_PA(11)
#define BT_UART_RTS	GPIO_PC(2)
#define BLUETOOTH_UPORT_NAME  "ttyS3"
*/
/* BT gpio */
#define BLUETOOTH_UPORT_NAME "ttyS3"
#define GPIO_BT_REG_ON   GPIO_PA(11)
#define GPIO_BT_WAKE     GPIO_PA(10)
#define GPIO_BT_INT      GPIO_PA(12)
#define GPIO_BT_UART_RTS        -1
/* ****************************GPIO BLUETOOTH END**************************** */

/* ****************************MISC START**************************** */
#define GPIO_PIR_INT GPIO_PA(2)
#define GPIO_LIGHT_SENSOR_INT GPIO_PA(3)
/* ****************************MISC END**************************** */
#endif /* __BOARD_H__ */
