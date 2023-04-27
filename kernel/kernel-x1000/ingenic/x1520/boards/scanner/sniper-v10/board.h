#ifndef __BOARD_H__
#define __BOARD_H__

#include <gpio.h>
#include <soc/gpio.h>
#ifdef CONFIG_VIDEO_TX_ISP
#include <tx-isp/tx-isp-common.h>
#endif

/*
 * down voltage to reduce power waste
*/
//#define DOWNVOLTAGE                      GPIO_PC(15)

/*
 * Camera
 */
#ifdef CONFIG_SOC_CAMERA_OV9281_DVP
#define CAMERA_SENSOR_RESET 			GPIO_PB(25)
#define CAMERA_FRONT_SENSOR_PWDN 		GPIO_PB(26)
#define CAMERA_FRONT_SENSOR_PWDN_EN_LEVEL  1
#define CAMERA_FRONT_SENSOR_PWDN_DIS_LEVEL  0
#elif defined CONFIG_SOC_CAMERA_OV9281_MIPI
#define CAMERA_SENSOR_RESET 			GPIO_PA(10)
#define CAMERA_FRONT_SENSOR_PWDN 		GPIO_PA(5)
#define CAMERA_FRONT_SENSOR_PWDN_EN_LEVEL  0
#define CAMERA_FRONT_SENSOR_PWDN_DIS_LEVEL  1
#endif

#ifdef CONFIG_TX_ISP_CAMERA_OV9732
#define CIM1_SENSOR_NAME                "ov9732"
#define CIM1_GPIO_POWER                 (-1)
#define CIM1_GPIO_SENSOR_PWDN           (-1)
#define CIM1_GPIO_SENSOR_RST            GPIO_PA(10)
#define CIM1_GPIO_I2C_SEL1              (-1)
#define CIM1_GPIO_I2C_SEL2              (-1)
#define CIM1_DVP_GPIO_FUNC              DVP_PA_LOW_10BIT
#endif


#ifdef CONFIG_TX_ISP_CAMERA_OV9281_MIPI
#define CIM1_SENSOR_NAME                "ov9281"
#define CIM1_GPIO_POWER                 (-1)
#define CIM1_GPIO_SENSOR_PWDN           (-1)
#define CIM1_GPIO_SENSOR_RST            GPIO_PA(11)
#define CIM1_GPIO_I2C_SEL1              (-1)
#define CIM1_GPIO_I2C_SEL2              (-1)
#define CIM1_DVP_GPIO_FUNC              DVP_PA_LOW_10BIT
#endif

#ifdef CONFIG_TX_ISP_CAMERA_OV9281_DVP
#define CIM1_SENSOR_NAME                "ov9281"
#define CIM1_GPIO_POWER                 (-1)
#define CIM1_GPIO_SENSOR_PWDN           (-1)
#define CIM1_GPIO_SENSOR_RST            GPIO_PA(10)
#define CIM1_GPIO_I2C_SEL1              (-1)
#define CIM1_GPIO_I2C_SEL2              (-1)
#define CIM1_DVP_GPIO_FUNC              DVP_PA_LOW_10BIT
#endif

#ifdef CONFIG_TX_ISP_CAMERA_OV5693
#define CIM1_SENSOR_NAME                "ov5693"
#define CIM1_GPIO_POWER                 (-1)
#define CIM1_GPIO_SENSOR_PWDN           (-1)
#define CIM1_GPIO_SENSOR_RST            GPIO_PA(8)
#define CIM1_GPIO_I2C_SEL1              (-1)
#define CIM1_GPIO_I2C_SEL2              (-1)
#define CIM1_DVP_GPIO_FUNC              (-1)
#endif

#ifdef CONFIG_TX_ISP_CAMERA_SC031GS
#define CIM1_SENSOR_NAME                "sc031"
#define CIM1_GPIO_POWER                 (-1)
#define CIM1_GPIO_SENSOR_PWDN           (-1)
#define CIM1_GPIO_SENSOR_RST            GPIO_PA(10)
#define CIM1_GPIO_I2C_SEL1              (-1)
#define CIM1_GPIO_I2C_SEL2              (-1)
#define CIM1_DVP_GPIO_FUNC              DVP_PA_LOW_8BIT
#endif

#ifdef CONFIG_TX_ISP_CAMERA_TW9912
#define CIM1_SENSOR_NAME                "tw9912"
#define CIM1_GPIO_POWER                 (-1)
#define CIM1_GPIO_SENSOR_PWDN           (-1)
#define CIM1_GPIO_SENSOR_RST            (-1)
#define CIM1_GPIO_I2C_SEL1              (-1)
#define CIM1_GPIO_I2C_SEL2              (-1)
#define CIM1_DVP_GPIO_FUNC              DVP_PA_LOW_8BIT
#endif


/*
 * I2C(GPIO)
 */
#ifdef CONFIG_SOFT_I2C0_GPIO_V12_JZ
#define GPIO_I2C0_SDA                   GPIO_PA(12)
#define GPIO_I2C0_SCK                   GPIO_PA(13)
#endif

#ifdef CONFIG_SOFT_I2C1_GPIO_V12_JZ
#define GPIO_I2C1_SDA                   GPIO_PB(25)
#define GPIO_I2C1_SCK                   GPIO_PB(26)
#endif


/*
 * SPI(GPIO)
 */
#ifdef CONFIG_SPI_GPIO
#define GPIO_SPI_SCK                    GPIO_PC(15)
#define GPIO_SPI_MOSI                   GPIO_PC(11)
#define GPIO_SPI_MISO                   GPIO_PC(12)
#endif


/*
 * Key
 */
#define GPIO_HOME                       GPIO_PC(0)
#define ACTIVE_LOW_HOME                 1

//#define WAKEUP_HOME                     1
//#define CAN_DISABLE_MENU                1


/*
 * MMC
 */
#define GPIO_MMC_RST_N                  -1
#define GPIO_MMC_RST_N_LEVEL            LOW_ENABLE

#define GPIO_MMC_CD_N                   GPIO_PB(27)
#define GPIO_MMC_CD_N_LEVEL             LOW_ENABLE

#define GPIO_MMC_PWR                    -1
#define GPIO_MMC_PWR_LEVEL              HIGH_ENABLE

#define GPIO_MMC_WP_N                   -1
#define GPIO_MMC_WP_N_LEVEL             LOW_ENABLE

/*
 * USB
 */
#define GPIO_USB_ID                     (-1)
#define GPIO_USB_ID_LEVEL               (-1)

#define GPIO_USB_DETE                   (-1)
#define GPIO_USB_DETE_LEVEL             (-1)

#define GPIO_USB_DRVVBUS                (-1)
#define GPIO_USB_DRVVBUS_LEVEL          (-1)


/*
 * Audio
 */
#define GPIO_HP_MUTE                    (-1)        /*hp mute gpio*/
#define GPIO_HP_MUTE_LEVEL              (-1)        /*vaild level*/

#define GPIO_SPEAKER_EN                 (-1) /*speaker enable gpio*/
#define GPIO_SPEAKER_EN_LEVEL           (1)

#define GPIO_HANDSET_EN                 (-1)        /*handset enable gpio*/
#define GPIO_HANDSET_EN_LEVEL           (-1)

#define	GPIO_HP_DETECT                  (-1)        /*hp detect gpio*/
#define GPIO_HP_INSERT_LEVEL            (1)
#define GPIO_MIC_SELECT                 (-1)        /*mic select gpio*/
#define GPIO_BUILDIN_MIC_LEVEL          (-1)        /*builin mic select level*/
#define GPIO_MIC_DETECT                 (-1)
#define GPIO_MIC_INSERT_LEVEL           (-1)
#define GPIO_MIC_DETECT_EN              (-1)        /*mic detect enable gpio*/
#define GPIO_MIC_DETECT_EN_LEVEL        (-1)        /*mic detect enable gpio*/

#define HP_SENSE_ACTIVE_LEVEL           (1)
#define HOOK_ACTIVE_LEVEL               (-1)

#define HP_MIN_VOLUME_REGVAL            (0)
#define HP_MAX_VOLUME_REGVAL            (23)
#define HP_DEFAULT_VOLUME_REGVAL        (13)

/*
 * GMAC
 */
#ifdef CONFIG_JZ_MAC

#ifndef CONFIG_MDIO_GPIO
    #ifdef CONFIG_JZGPIO_PHY_RESET
    #define GMAC_PHY_PORT_GPIO          GPIO_PE(11)
    #define GMAC_PHY_PORT_START_FUNC    GPIO_OUTPUT0
    #define GMAC_PHY_PORT_END_FUNC      GPIO_OUTPUT1
    #define GMAC_PHY_DELAYTIME          100000
    #endif
#else /* CONFIG_MDIO_GPIO */
    #define MDIO_MDIO_MDC_GPIO          GPIO_PF(13)
    #define MDIO_MDIO_GPIO              GPIO_PF(14)
#endif

#endif /* CONFIG_JZ_MAC */

/*
 * WiFi
 */
#define WL_WAKE_HOST                    GPIO_PC(11)
#define WL_REG_EN                       GPIO_PB(25)
#define WL_MMC_NUM                      (1)

#define WLAN_PWR_EN                     (-1)
#define BCM_PWR_EN                      GPIO_PC(15)
#define BCM_PWR_EN_LEVEL                (0)

/*
 * Bluetooth
 */
#ifdef  CONFIG_BCM_43438_RFKILL
#define BLUETOOTH_UART_GPIO_PORT        GPIO_PORT_B
#define BLUETOOTH_UART_GPIO_FUNC        GPIO_FUNC_0
#define BLUETOOTH_UART_FUNC_SHIFT       (1<<21)
#define HOST_WAKE_BT                    GPIO_PC(16)
#define BT_WAKE_HOST                    GPIO_PC(12)
#define BT_REG_EN                       GPIO_PB(26)
#define BT_UART_RTS                     GPIO_PB(21)
#define BT_PWR_EN                       (-1)
#define HOST_BT_RST                     (-1)
#define BLUETOOTH_UPORT_NAME            "ttyS0"
#endif

/*
 * SPI
 */
#ifdef CONFIG_JZ_SPI0
#define SPI0_CHIP_SELECT0               -1
#define SPI0_CHIP_SELECT1               GPIO_PC(14)
#endif

/*
 * Ethernet
 */
#ifdef CONFIG_AX88796C_SPI
#define AX88796C_SPI_CS_PIN             GPIO_PC(16)
#define AX88796C_SPI_IRQ_PIN            GPIO_PA(12)
#define AX88796C_SPI_RESET_PIN            -1
#endif

#endif /* __BOARD_H__ */
