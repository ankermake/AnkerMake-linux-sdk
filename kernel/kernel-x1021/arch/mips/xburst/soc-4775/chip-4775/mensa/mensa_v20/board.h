#ifndef __BOARD_H__
#define __BOARD_H__
#include <gpio.h>
#include <soc/gpio.h>

/* ****************************GPIO KEY START******************************** */
/* #define GPIO_HOME_KEY		GPIO_PD(18) */
/* #define ACTIVE_LOW_HOME		1 */

#define GPIO_MENU_KEY		GPIO_PB(31)
#define ACTIVE_LOW_MENU		0

#define GPIO_BACK_KEY		GPIO_PD(19)
#define ACTIVE_LOW_BACK		0

#define GPIO_VOLUMEUP_KEY         GPIO_PD(18)
#define ACTIVE_LOW_VOLUMEUP	1

#define GPIO_VOLUMEDOWN_KEY         GPIO_PD(17)
#define ACTIVE_LOW_VOLUMEDOWN	0

#define GPIO_WAKEUP_KEY        GPIO_PA(30)
#define ACTIVE_LOW_WAKEUP      0

/* ****************************GPIO KEY END********************************** */

/* ****************************GPIO WIFI START******************************** */
#ifdef CONFIG_BCM_43438_RFKILL
/**
 ** Bluetooth gpio
 **/
#define BLUETOOTH_UPORT_NAME    "ttyS0"
#define GPIO_BT_REG_ON          GPIO_PD(27)
#define GPIO_BT_WAKE            GPIO_PD(29)
#define GPIO_BT_INT             GPIO_PB(20) /* HOST_WAKE_BT */
#define GPIO_BT_UART_RTS        GPIO_PF(2)
#endif

#define WLAN_SDIO_INDEX			1
#define GPIO_WIFI_RST_N		GPIO_PD(28) /* WL_REG_EN */
#define GPIO_WIFI_WAKE 		GPIO_PD(26)

/* ****************************GPIO WIFI END********************************** */

/* ****************************GPIO CAMERA START******************************** */
#define FRONT_CAMERA_INDEX	0
#define BACK_CAMERA_INDEX	1

#define FRONT_CAMERA_I2C_1
#define BACK_CAMERA_I2C_1

#define FRONT_CAMERA_PW_EN		GPIO_PB(0)
#define FRONT_CAMERA_SENSOR_RESET	GPIO_PA(27)
#define FRONT_CAMERA_SENSOR_EN		GPIO_PB(1)

#define BACK_CAMERA_PW_EN		GPIO_PA(2)
#define BACK_CAMERA_SENSOR_RESET	GPIO_PA(3)
#define BACK_CAMERA_SENSOR_EN		GPIO_PA(1)
/* ****************************GPIO CAMERA END********************************** */

/* ****************************GPIO GMAC START******************************* */
#ifndef CONFIG_MDIO_GPIO
#ifdef CONFIG_JZGPIO_PHY_RESET
#define GMAC_PHY_PORT_GPIO GPIO_PF(15)
#define GMAC_PHY_ACTIVE_HIGH 1
#define GMAC_CRLT_PORT -1
#define GMAC_CRLT_PORT_PINS -1
#define GMAC_CRTL_PORT_INIT_FUNC GPIO_FUNC_1
#define GMAC_CRTL_PORT_SET_FUNC GPIO_OUTPUT0
#define GMAC_PHY_DELAYTIME 10
#endif
#else /* CONFIG_MDIO_GPIO */
#define MDIO_MDIO_MDC_GPIO GPIO_PF(13)
#define MDIO_MDIO_GPIO GPIO_PF(14)
#endif
/* ****************************GPIO GMAC END********************************* */

/* ****************************GPIO MMC START********************************** */
#define GPIO_SD0_VCC_EN_N	GPIO_PB(3)
#define GPIO_SD0_CD_N		GPIO_PB(2)
#define GPIO_SD0_RESET		-1//GPIO_PF(7)
/* ****************************GPIO MMC END********************************** */

/* ****************************GPIO TOUCHSCREEN START********************************** */
#define GPIO_TP_RESET		GPIO_PB(28)
#define GPIO_TP_INT		GPIO_PB(29)
/* ****************************GPIO TOUCHSCREEN END********************************** */

/* ****************************GPIO LCD START********************************** */
#define GPIO_LCD_PWM		GPIO_PE(1)
#define GPIO_LCD_DISP		GPIO_PB(30)
#define GPIO_LCD_DE			-1
#define GPIO_LCD_VSYNC		-1
#define GPIO_LCD_HSYNC		-1
#define GPIO_LCD_CS			GPIO_PC(0)
#define GPIO_LCD_CLK		GPIO_PC(1)
#define GPIO_LCD_SDO		GPIO_PC(10)
#define GPIO_LCD_SDI		GPIO_PC(11)
#define GPIO_LCD_BACK_SEL	GPIO_PC(20)
#define GPIO_LCD_PWM_EN		GPIO_PE(3)
/* ****************************GPIO LCD END********************************** */

/* ****************************GPIO USB START********************************** */
#define GPIO_USB_HOST_5V_EN             GPIO_PA(29)
#define GPIO_USB_DETE                   GPIO_PA(16)
#define GPIO_USB_ID                   GPIO_PB(5)
#define GPIO_USB_DRVVBUS		GPIO_PE(10)
#define GPIO_USB_DRVVBUS_LEVEL		1
/* ****************************GPIO USB END********************************** */

/* ****************************GPIO AUDIO START********************************** */
#define GPIO_HP_MUTE		-1	/*hp mute gpio*/
#define GPIO_HP_MUTE_LEVEL		-1		/*vaild level*/

#define GPIO_SPEAKER_EN			GPIO_PA(0)/*speaker enable gpio*/
#define GPIO_SPEAKER_EN_LEVEL	1

#define GPIO_HANDSET_EN		  -1		/*handset enable gpio*/
#define GPIO_HANDSET_EN_LEVEL -1

#define	GPIO_HP_DETECT		GPIO_PA(17)	/*hp detect gpio*/
#define GPIO_HP_INSERT_LEVEL    1
#define GPIO_MIC_SELECT		-1		/*mic select gpio*/
#define GPIO_BUILDIN_MIC_LEVEL	-1		/*builin mic select level*/
#define GPIO_MIC_DETECT		-1
#define GPIO_MIC_INSERT_LEVEL -1
#define GPIO_MIC_DETECT_EN		-1  /*mic detect enable gpio*/
#define GPIO_MIC_DETECT_EN_LEVEL	-1 /*mic detect enable gpio*/
/* ****************************GPIO AUDIO END********************************** */

/* ****************************GPIO EFUSE START********************************** */
#define GPIO_EFUSE_VDDQ GPIO_PE(2)
/* ****************************GPIO EFUSE END********************************** */

/**
 * pmem information
 **/
/* auto allocate pmem in arch_mem_init(), do not assigned base addr, just set 0 */
#define JZ_PMEM_ADSP_BASE   0x0         // 0x3e000000
#define JZ_PMEM_ADSP_SIZE   0x02000000
#endif /* __BOARD_H__ */
