#ifndef __BOARD_H__
#define __BOARD_H__
#include <gpio.h>
#include <soc/gpio.h>

/* ****************************GPIO KEY START******************************** */
#define GPIO_POWERDOWN 		GPIO_PB(31)
#define GPIO_BOOT_SEL0		GPIO_PB(28)
#define GPIO_BOOT_SEL1		GPIO_PB(29)
#define ACTIVE_LOW_POWERDOWN 	1
#define ACTIVE_LOW_F4 		1
#define ACTIVE_LOW_F5 		1
/* ****************************GPIO KEY END******************************** */

/* ****************************MATRIX KEY START******************************** */
#define KEY_R2 GPIO_PB(1)
#define KEY_R3 GPIO_PB(0)
#define KEY_C2 GPIO_PB(4)

#define KEY_R0 GPIO_PB(3)
#define KEY_R1 GPIO_PB(2)
/* ****************************MATRIX KEY END******************************** */

#define BLUETOOTH_UPORT_NAME    "ttyS0"
#define GPIO_BT_REG_ON          GPIO_PC(18)
#define GPIO_BT_WAKE            GPIO_PC(19)
#define GPIO_BT_INT             GPIO_PC(20)
#define GPIO_BT_UART_RTS        GPIO_PC(13)

/* MSC GPIO Definition */
#define GPIO_SD0_CD_N       GPIO_PC(21)

/*wifi*/
#define GPIO_WLAN_PW_EN     -1//GPIO_PD(24)
#define WL_REG_EN       GPIO_PG(8)
#define GPIO_WIFI_WAKE GPIO_PC(16)
#define GPIO_WIFI_RST_N			GPIO_PC(17)
#define WLAN_SDIO_INDEX			1

#if defined(CONFIG_JZ_SPI) || defined(CONFIG_JZ_SFC)
#define SPI_CHIP_ENABLE GPIO_PA(27)
#endif

#ifdef CONFIG_SPI_GPIO
#define GPIO_SPI_SCK  GPIO_PA(26)
#define GPIO_SPI_MOSI GPIO_PA(29)
#define GPIO_SPI_MISO GPIO_PA(28)
#endif


/* ****************************GPIO USB START******************************** */
#define GPIO_USB_ID             GPIO_PD(2)/*GPIO_PD(2)*/
#define GPIO_USB_ID_LEVEL       LOW_ENABLE
#define GPIO_USB_DETE           GPIO_PB(8) /*GPIO_PB(8)*/
#define GPIO_USB_DETE_LEVEL     LOW_ENABLE
#define GPIO_USB_DRVVBUS        GPIO_PB(25)
#define GPIO_USB_DRVVBUS_LEVEL      HIGH_ENABLE
/* ****************************GPIO USB END********************************** */

/* ****************************GPIO AUDIO START****************************** */
#define GPIO_HP_MUTE        -1  /*hp mute gpio*/
#define GPIO_HP_MUTE_LEVEL  -1  /*vaild level*/

#define GPIO_SPEAKER_EN    	GPIO_PC(25)         /*speaker enable gpio*/
#define GPIO_SPEAKER_EN_LEVEL   1
#define GPIO_AMP_POWER_EN	-1	/* amp power enable pin */
#define GPIO_AMP_POWER_EN_LEVEL	-1

#define GPIO_HANDSET_EN     	-1  /*handset enable gpio*/
#define GPIO_HANDSET_EN_LEVEL   -1

#define GPIO_HP_DETECT  	-1      /*hp detect gpio*/
#define GPIO_HP_INSERT_LEVEL    -1
#define GPIO_LINEIN_DETECT          GPIO_PD(3)      /*linein detect gpio*/
#define GPIO_LINEIN_INSERT_LEVEL    0
#define GPIO_MIC_SELECT     	-1  /*mic select gpio*/
#define GPIO_BUILDIN_MIC_LEVEL  -1  /*builin mic select level*/
#define GPIO_MIC_DETECT     	-1
#define GPIO_MIC_INSERT_LEVEL   -1
#define GPIO_MIC_DETECT_EN  	-1  /*mic detect enable gpio*/
#define GPIO_MIC_DETECT_EN_LEVEL -1 /*mic detect enable gpio*/

#define HOOK_ACTIVE_LEVEL       -1
/* ****************************GPIO AUDIO END******************************** */

/* ****************************GPIO LCD START****************************** */
#ifdef  CONFIG_LCD_XRM2002903
#define GPIO_LCD_RD     GPIO_PB(16)
#define GPIO_LCD_CS     GPIO_PB(18)
#define GPIO_LCD_RST    GPIO_PD(5)
//#define GPIO_BL_PWR_EN  GPIO_PC(24)
#define GPIO_LCD_PWM    GPIO_PC(24)
#endif
/* ****************************GPIO LCD END******************************** */

#define GPIO_EFUSE_VDDQ			GPIO_PC(27)	/* EFUSE must be -ENODEV or a gpio */

#ifdef CONFIG_I2C_GPIO

#ifdef CONFIG_SOFT_I2C0_GPIO_V12_JZ
#define GPIO_I2C0_SDA		GPIO_PB(24)
#define GPIO_I2C0_SCK		GPIO_PB(23)
#endif

#ifdef CONFIG_SOFT_I2C1_GPIO_V12_JZ
#define GPIO_I2C1_SDA		-1
#define GPIO_I2C1_SCK		-1
#endif

#endif /* CONFIG_I2C_GPIO */


#define KEY_R2 GPIO_PB(1)
#define KEY_R3 GPIO_PB(0)
#define KEY_C2 GPIO_PB(4)

#define KEY_R0 GPIO_PB(3)
#define KEY_R1 GPIO_PB(2)

static const unsigned int matrix_keypad_rows[] = {KEY_R0, KEY_R1};
static const unsigned int matrix_keypad_cols[] = {KEY_R2, KEY_R3, KEY_C2};


#endif /* __BOARD_H__ */
