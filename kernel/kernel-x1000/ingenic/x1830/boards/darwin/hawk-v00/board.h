#ifndef __BOARD_H__
#define __BOARD_H__

#include <gpio.h>
#include <soc/gpio.h>
#include <tx-isp/tx-isp-common.h>

/*
 * Key GPIO
 */
#define GPIO_HOME                       GPIO_PC(1)
#define ACTIVE_LOW_HOME                 (0)
#define GPIO_LEFT                       GPIO_PB(2)
#define ACTIVE_LOW_LEFT                 (1)
#define GPIO_DOWN                       GPIO_PB(3)
#define ACTIVE_LOW_DOWN                 (1)
#define GPIO_RIGHT                      GPIO_PB(4)
#define ACTIVE_LOW_RIGHT                (1)
#define GPIO_UP                         GPIO_PB(5)
#define ACTIVE_LOW_UP                   (1)


/*
 * SLCD(ili6122 LCD controller leakage)
 */
#define GPIO_LCD_POWER_EN               GPIO_PB(2)
#define GPIO_LCD_POWER_ACTIVE_LEVEL     (0)
#define GPIO_LCD_BL_EN                  (-1)
#define GPIO_LCD_BL_ACTIVE_LEVEL        (1)
#define GPIO_LCD_RST                    (-1)
#define GPIO_LCD_RST_ACTIVE_LEVEL       (0)


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
#define GPIO_HP_MUTE                    (-1) /*hp mute gpio*/
#define GPIO_HP_MUTE_LEVEL              (-1) /*vaild level*/

#define GPIO_SPEAKER_EN                 GPIO_PB(9)  /*speaker enable gpio*/
#define GPIO_SPEAKER_EN_LEVEL           (1)

#define GPIO_HANDSET_EN                 (-1) /*handset enable gpio*/
#define GPIO_HANDSET_EN_LEVEL           (-1)

#define	GPIO_HP_DETECT                  (-1) /*hp detect gpio*/
#define GPIO_HP_INSERT_LEVEL            (1)
#define GPIO_MIC_SELECT                 (-1) /*mic select gpio*/
#define GPIO_BUILDIN_MIC_LEVEL          (-1) /*builin mic select level*/
#define GPIO_MIC_DETECT                 (-1)
#define GPIO_MIC_INSERT_LEVEL           (-1)
#define GPIO_MIC_DETECT_EN              (-1)  /*mic detect enable gpio*/
#define GPIO_MIC_DETECT_EN_LEVEL        (-1) /*mic detect enable gpio*/

#define HP_SENSE_ACTIVE_LEVEL           (1)
#define HOOK_ACTIVE_LEVEL               (-1)

#define HP_MIN_VOLUME_REGVAL            (0)
#define HP_MAX_VOLUME_REGVAL            (23)
#define HP_DEFAULT_VOLUME_REGVAL        (13)


/*
 * WiFi
 */
#define WL_MMC_NUM                      (1) /*sdio use MMC1*/
#define GPIO_BT_DIS_N                   GPIO_PB(29)    /* AP_EN_BTREG */
#define GPIO_BT_HOST_WAKE_DEV           GPIO_PB(28)    /* AP_WAKEUP_BT */
#define GPIO_BT_DEV_WAKE_HOST           GPIO_PB(27)    /* BT_WAKEUP_AP */
#define GPIO_BT_UART_RTS                GPIO_PB(21)
#define GPIO_WL_DIS_N                   GPIO_PB(30)    /* AP_EN_WLREG  WL_DIS_N power on */
#define GPIO_WL_DEV_WAKE_HOST           GPIO_PB(31)    /* WL_WAKEUP_AP  interrupt */
#define GPIO_EN_BT_WIFI_VDD             GPIO_PC(9)


/*
 * EFUSE
 */
#define GPIO_EFUSE_VDDQ                 GPIO_PC(17)    /* EFUSE must be -ENODEV or a gpio */


/*
 * Camera
 */
#ifdef CONFIG_TX_ISP_CAMERA_GC2375A
#define CIM1_SENSOR_NAME                "gc2375a"
#define CIM1_GPIO_POWER                 GPIO_PB(30)
#define CIM1_GPIO_SENSOR_PWDN           (-1)
#define CIM1_GPIO_SENSOR_RST            GPIO_PA(18)
#define CIM1_GPIO_I2C_SEL1              (-1)
#define CIM1_GPIO_I2C_SEL2              (-1)
#define CIM1_DVP_GPIO_FUNC              (-1)

#define CIM2_SENSOR_NAME                "gc2375a"
#define CIM2_GPIO_POWER                 GPIO_PB(30)
#define CIM2_GPIO_SENSOR_PWDN           (-1)
#define CIM2_GPIO_SENSOR_RST            GPIO_PA(18)
#define CIM2_GPIO_I2C_SEL1              (-1)
#define CIM2_GPIO_I2C_SEL2              (-1)
#define CIM2_DVP_GPIO_FUNC              (-1)
#endif

#ifdef CONFIG_SENSOR_DVP_OV9732
#define CIM1_SENSOR_NAME                "ov9732"
#define CIM1_GPIO_POWER                 GPIO_PB(30)
#define CIM1_GPIO_SENSOR_PWDN           (-1)
#define CIM1_GPIO_SENSOR_RST            GPIO_PA(19)
#define CIM1_GPIO_I2C_SEL1              (-1)
#define CIM1_GPIO_I2C_SEL2              (-1)
#define CIM1_DVP_GPIO_FUNC              DVP_PA_LOW_10BIT

#define CIM2_SENSOR_NAME                "ov9732"
#define CIM2_GPIO_POWER                 GPIO_PB(30)
#define CIM2_GPIO_SENSOR_PWDN           (-1)
#define CIM2_GPIO_SENSOR_RST            GPIO_PA(18)
#define CIM2_GPIO_I2C_SEL1              (-1)
#define CIM2_GPIO_I2C_SEL2              (-1)
#define CIM2_DVP_GPIO_FUNC              DVP_PA_LOW_10BIT
#endif

/*
 * GMAC
 */

#ifdef CONFIG_JZ_MAC

    #ifndef CONFIG_MDIO_GPIO

    #ifdef CONFIG_JZGPIO_PHY_RESET
    #define GMAC_PHY_PORT_GPIO          GPIO_PC(22)
    #define GMAC_PHY_PORT_START_FUNC    GPIO_OUTPUT0
    #define GMAC_PHY_PORT_END_FUNC      GPIO_OUTPUT1
    #define GMAC_PHY_DELAYTIME          100000
    #endif

    #else /* CONFIG_MDIO_GPIO */

    #define MDIO_MDIO_MDC_GPIO          GPIO_PB(10)
    #define MDIO_MDIO_GPIO              GPIO_PB(11)

    #endif
#endif /* CONFIG_JZ_MAC */

/*
 * I2C1
 */
#define GPIO_I2C1_SDA                   GPIO_PB(25)
#define GPIO_I2C1_SCK                   GPIO_PB(26)

/*
 * SPI
 */
#define GPIO_SPI0_MISO                  GPIO_PC(11)
#define GPIO_SPI0_MOSI                  GPIO_PC(12)
#define GPIO_SPI0_SCK                   GPIO_PC(15)
#ifdef CONFIG_JZ_SPI0
#define SPI0_CHIP_SELECT1               GPIO_PC(16)
#define SPI0_CHIP_SELECT0               GPIO_PB(29)
#define SPI0_CHIP_SELECT2               GPIO_PC(26)
#endif

#endif /* __BOARD_H__ */
