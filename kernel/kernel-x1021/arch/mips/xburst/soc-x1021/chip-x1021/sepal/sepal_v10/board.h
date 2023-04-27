#ifndef __BOARD_H__
#define __BOARD_H__
#include <gpio.h>
#include <soc/gpio.h>
#include <linux/jz_dwc.h>

/* ****************************GPIO POWER START******************************** */
#define POWER_ON_GPIO			GPIO_PC(2)
#define POWER_ON_ACTIVE_LEVEL	1
/* ****************************GPIO POWER END******************************** */

/* ****************************GPIO I2C START******************************** */
#ifdef CONFIG_SOFT_I2C0_GPIO_V12_JZ
#define GPIO_I2C0_SDA GPIO_PA(12)
#define GPIO_I2C0_SCK GPIO_PA(13)
#endif

#ifdef CONFIG_SOFT_I2C1_GPIO_V12_JZ
#define GPIO_I2C1_SDA GPIO_PB(25)
#define GPIO_I2C1_SCK GPIO_PB(26)
#endif

#ifdef CONFIG_SOFT_I2C2_GPIO_V12_JZ
#define GPIO_I2C2_SDA GPIO_PC(27)
#define GPIO_I2C2_SCK GPIO_PC(28)
#endif
/* ****************************GPIO I2C END********************************** */

/* ****************************GPIO SPI START******************************** */

#if defined(CONFIG_JZ_SPI0) || defined(CONFIG_SPI_GPIO)
#define SPI0_CHIP_ENABLE GPIO_PC(14)
#endif

#ifdef CONFIG_JZ_SPI1
#define SPI1_CHIP_ENABLE GPIO_PB(11)
#endif

#ifdef CONFIG_SPI_GPIO
#define GPIO_SPI_SCK  GPIO_PC(13)
#define GPIO_SPI_MOSI GPIO_PC(12)
#define GPIO_SPI_MISO GPIO_PC(11)
#endif
/* ****************************GPIO SPI END********************************** */

/* ****************************GPIO KEY START******************************** */
#define GPIO_HOME			GPIO_PC(0)
#define ACTIVE_LOW_HOME			1
#define WAKEUP_HOME			1
#define CAN_DISABLE_MENU		1
/* ****************************GPIO KEY END********************************** */

/* ****************************GPIO MMC START******************************** */
#define GPIO_MMC_RST_N			-1
#define GPIO_MMC_RST_N_LEVEL	LOW_ENABLE
#define GPIO_MMC_CD_N			-1
#define GPIO_MMC_CD_N_LEVEL		LOW_ENABLE
#define GPIO_MMC_PWR			-1
#define GPIO_MMC_PWR_LEVEL		HIGH_ENABLE
#define GPIO_MMC_WP_N			-1
#define GPIO_MMC_WP_N_LEVEL		LOW_ENABLE
/* ****************************GPIO MMC END******************************** */

/* ****************************GPIO USB START******************************** */
#define GPIO_USB_ID			-1
#define GPIO_USB_ID_LEVEL		LOW_ENABLE
#define GPIO_USB_DETE			-1
#define GPIO_USB_DETE_LEVEL		LOW_ENABLE
#define GPIO_USB_DRVVBUS		-1
#define GPIO_USB_DRVVBUS_LEVEL		HIGH_ENABLE
/* ****************************GPIO USB END********************************** */

/* ****************************GPIO DVP CAMERA START***************************** */
#if defined(CONFIG_JZ_VIC_CORE)
#define VIC_SENSOR_NAME "sensor"
#ifdef CONFIG_CAMERA_GC0328_SUPPORT
#define PIN_VIC_I2C_SEL1 GPIO_PA(10)
#define PIN_VIC_I2C_SEL2 GPIO_PA(9)
#define PIN_VIC_PWR_ON GPIO_PA(11)
#define PIN_VIC_RST GPIO_PA(8)
#define SENSOR_DEV_ID   0X21
#elif defined CONFIG_CAMERA_GC0308_SUPPORT
#define PIN_VIC_I2C_SEL1 GPIO_PA(-10)
#define PIN_VIC_I2C_SEL2 GPIO_PA(-9)
#define PIN_VIC_PWR_ON GPIO_PA(18)
#define PIN_VIC_RST GPIO_PA(22)
#define SENSOR_DEV_ID   0X21
#elif defined CONFIG_CAMERA_SC031GS_SUPPORT
#define PIN_VIC_I2C_SEL1 GPIO_PA(-10)
#define PIN_VIC_I2C_SEL2 GPIO_PA(-9)
#define PIN_VIC_PWR_ON GPIO_PA(18)
#define PIN_VIC_RST GPIO_PA(22)
#define SENSOR_DEV_ID   0X30
#endif
#endif

/* ****************************GPIO CAMERA END******************************* */

/* ****************************GPIO AUDIO START****************************** */
#define GPIO_HP_MUTE		-1	/*hp mute gpio*/
#define GPIO_HP_MUTE_LEVEL	-1	/*vaild level*/

#define GPIO_SPEAKER_EN	   GPIO_PB(31)	      /*speaker enable gpio*/
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
/* ****************************GPIO AUDIO END******************************** */

/* ****************************GPIO GMAC START******************************* */
#ifdef CONFIG_JZ_MAC
#ifndef CONFIG_MDIO_GPIO
#ifdef CONFIG_JZGPIO_PHY_RESET
#define GMAC_PHY_PORT_GPIO GPIO_PE(11)
#define GMAC_PHY_PORT_START_FUNC GPIO_OUTPUT0
#define GMAC_PHY_PORT_END_FUNC GPIO_OUTPUT1
#define GMAC_PHY_DELAYTIME 100000
#endif
#else /* CONFIG_MDIO_GPIO */
#define MDIO_MDIO_MDC_GPIO GPIO_PF(13)
#define MDIO_MDIO_GPIO GPIO_PF(14)
#endif
#endif /* CONFIG_JZ4775_MAC */
/* ****************************GPIO GMAC END********************************* */

/* ****************************GPIO WIFI BT START******************************* */
#define WL_MMC_NUM  0 //sdio use MMC1

#define GPIO_BT_DIS_N           GPIO_PC(6)
#define GPIO_HOST_WAKE_BT       GPIO_PC(7)
#define GPIO_BT_WAKE_HOST       GPIO_PC(5)
#define GPIO_BT_UART_RTS        GPIO_PB(21)

#define GPIO_WL_DIS_N		    GPIO_PB(27)             /* AP_EN_WLREG  WL_DIS_N power on */
#define GPIO_WL_DEV_WAKE_HOST   GPIO_PB(31)             /* WL_WAKEUP_AP  interrupt */

/*
 * WiFi
 */
#ifdef CONFIG_ESP8089
#define ESP8089_CHIP_EN                 GPIO_PB(27)
#define ESP8089_IOPWR_EN                -1  // wifi_io  GPIO_PC(17)
#define ESP8089_IOPWR_EN_LEVEL          0
#define ESP8089_PWR_EN                  -1  //ADD3V3_WIFI GPIO_PC(19)
#define ESP8089_PWR_EN_LEVEL            1
#define ESP8089_WKUP_CPU                -1 //GPIO_PC(21)
#endif


/* ****************************GPIO WIFI BT END********************************* */

#define LCD_ENABLE_TE_PIN                   1
#define LCD_ENABLE_RDY_PIN                  0
#define LCD_REFRESH_MODE                    REFRESH_PAN_DISPLAY
#define LCD_INIT_LCD_WHEN_BOOTUP            1
#define LCD_REFRESH_LCD_WHEN_RESUME         1
#define LCD_WAIT_FRAME_END_WHEN_PAN_DISPLAY 0
#define GPIO_LCD_CS                         GPIO_PB(17)
#define GPIO_LCD_RD                         -1
#define GPIO_LCD_POWER_EN                   GPIO_PB(28)
#define GPIO_LCD_RST                        -1
#define GPIO_LCD_PWM                        GPIO_PC(17)

#endif /* __BOARD_H__ */
