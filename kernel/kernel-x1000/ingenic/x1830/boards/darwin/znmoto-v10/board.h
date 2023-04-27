#ifndef __BOARD_H__
#define __BOARD_H__

#include <gpio.h>
#include <soc/gpio.h>
#include <tx-isp/tx-isp-common.h>

/*
 * Key GPIO
 */
#define GPIO_WKUP                       GPIO_PA(30)
#define ACTIVE_LOW_WKUP                 (1)
#define GPIO_BOOT                       GPIO_PC(1)
#define ACTIVE_LOW_BOOT                 (1)
//#define GPIO_PIR                        GPIO_PC(19)
#define ACTIVE_LOW_PIR                  (0)
//#define GPIO_KEY_UP					GPIO_PA(31)
//#define GPIO_KEY_DOWN				GPIO_PA(32)
//#define GPIO_KEY_LEFT				GPIO_PA(33)
//#define GPIO_KEY_RIGHT				GPIO_PA(34)
//#define GPIO_KEY_HOME				GPIO_PA(35)
//#define GPIO_KEY_BACK				GPIO_PA(36)



/*
 * SLCD(ili6122 rm68172 st7701 LCD controller leakage)
 */
#ifdef CONFIG_LCD_RM68172
#define GPIO_LCD_POWER_EN               GPIO_PB(2)
#define GPIO_LCD_POWER_ACTIVE_LEVEL     (0)
#define GPIO_LCD_BL_EN                  (-1)
#define GPIO_LCD_BL_ACTIVE_LEVEL        (0)
#define GPIO_LCD_RST                    GPIO_PB(3)
#define GPIO_LCD_RST_ACTIVE_LEVEL       (0)
#endif


#ifdef CONFIG_LCD_ST7701
#define GPIO_LCD_POWER_EN               GPIO_PB(2)
#define GPIO_LCD_POWER_ACTIVE_LEVEL     (0)
#define GPIO_LCD_BL_EN                  (-1)
#define GPIO_LCD_BL_ACTIVE_LEVEL        (0)
#define GPIO_LCD_RST                    GPIO_PB(3)
#define GPIO_LCD_RST_ACTIVE_LEVEL       (0)
#endif



#ifdef CONFIG_LCD_ST7789V
#define GPIO_LCD_POWER_EN       GPIO_PD(8)
#define GPIO_LCD_CS             GPIO_PD(18)
#define GPIO_LCD_RD             (-1)
#define GPIO_LCD_RST            GPIO_PB(6)
#endif

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

#define GPIO_SPEAKER_EN                 GPIO_PC(20)  /*speaker enable gpio*/
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
*
* sdcard
*/

#ifdef CONFIG_JZMMC_V12_MMC0

#define GPIO_MMC_WP_N          		    (-1)
#define GPIO_MMC_WP_N_LEVEL             (-1)
#define GPIO_MMC_CD_N                   GPIO_PB(13)
#define GPIO_MMC_CD_N_LEVEL             LOW_ENABLE
#define GPIO_MMC_PWR                    (-1)
#define GPIO_MMC_PWR_LEVEL              (-1)
#define GPIO_MMC_RST_N                  (-1)
#define GPIO_MMC_RST_N_LEVEL            (-1)

#endif


/*
 * WiFi
 */
#ifdef CONFIG_ESP8089
#define ESP8089_CHIP_EN                 GPIO_PC(19)
#define ESP8089_IOPWR_EN                -1
#define ESP8089_IOPWR_EN_LEVEL          0
#define ESP8089_PWR_EN                  -1
#define ESP8089_PWR_EN_LEVEL            0
#define ESP8089_WKUP_CPU                GPIO_PC(16)
#endif

#ifdef CONFIG_RTL8723DS
#define WL_MMC_NUM                      (1) /*sdio use MMC1*/
#define GPIO_BT_DIS_N                   GPIO_PB(25)    /* AP_EN_BTREG */
#define GPIO_BT_HOST_WAKE_DEV           GPIO_PB(26)    /* AP_WAKEUP_BT */
#define GPIO_BT_DEV_WAKE_HOST           GPIO_PB(01)    /* BT_WAKEUP_AP */
#define GPIO_BT_UART_RTS                GPIO_PB(21)
#define GPIO_WL_DIS_N                   GPIO_PB(04)    /* AP_EN_WLREG  WL_DIS_N power on */
#define GPIO_WL_DEV_WAKE_HOST           GPIO_PB(05)    /* WL_WAKEUP_AP  interrupt */
#define GPIO_EN_BT_WIFI_VDD             GPIO_PC(18)
#endif

#ifdef CONFIG_RTL8822CS
#define WL_MMC_NUM                      (0) /*sdio use MMC0*/
#define GPIO_BT_DIS_N                   GPIO_PC(5)    /* AP_EN_BTREG */
#define GPIO_BT_HOST_WAKE_DEV           (-1)    /* AP_WAKEUP_BT */
#define GPIO_BT_DEV_WAKE_HOST           (-1)    /* BT_WAKEUP_AP */
#define GPIO_BT_UART_RTS                (-1)
#define GPIO_WL_DIS_N                   GPIO_PC(8)    /* AP_EN_WLREG  WL_DIS_N power on */
#define GPIO_WL_DEV_WAKE_HOST           (-1)    /* WL_WAKEUP_AP  interrupt */
#define GPIO_EN_BT_WIFI_VDD             GPIO_PC(2)
#endif


/*
 * EFUSE
 */
#define GPIO_EFUSE_VDDQ                 GPIO_PA(22)    /* EFUSE must be -ENODEV or a gpio */


/*
 * Camera
 */
#ifdef CONFIG_TX_ISP_CAMERA_GC1034
#define CIM1_SENSOR_NAME                "gc1034"
#define CIM1_GPIO_POWER                 GPIO_PA(11)
#define CIM1_GPIO_SENSOR_PWDN           GPIO_PA(10)
#define CIM1_GPIO_SENSOR_RST            GPIO_PA(20)
#define CIM1_GPIO_I2C_SEL1              (-1)
#define CIM1_GPIO_I2C_SEL2              (-1)
#define CIM1_DVP_GPIO_FUNC              DVP_PA_LOW_10BIT
#endif

#ifdef CONFIG_TX_ISP_CAMERA_GC2375A
#define CIM1_SENSOR_NAME                "gc2375a"
#define CIM1_GPIO_POWER                 GPIO_PB(30)
#define CIM1_GPIO_SENSOR_PWDN           (-1)
#define CIM1_GPIO_SENSOR_RST            GPIO_PA(19)
#define CIM1_GPIO_I2C_SEL1              GPIO_PA(18)
#define CIM1_GPIO_I2C_SEL2              (-1)
#define CIM1_DVP_GPIO_FUNC              (-1)
#endif

#ifdef CONFIG_TX_ISP_CAMERA_GC2375A_DOUBLE
#define CIM1_SENSOR_NAME                "gc2375a"
#define CIM1_GPIO_POWER                 GPIO_PB(30)
#define CIM1_GPIO_SENSOR_PWDN           (-1)
#define CIM1_GPIO_SENSOR_RST            GPIO_PA(19)
#define CIM1_GPIO_I2C_SEL1              GPIO_PA(18)
#define CIM1_GPIO_I2C_SEL2              GPIO_PA(20)
#define CIM1_DVP_GPIO_FUNC              (-1)
#endif

#ifdef CONFIG_TX_ISP_CAMERA_OV2735
#define CIM1_SENSOR_NAME                "ov2735"
#define CIM1_GPIO_POWER                 GPIO_PB(30)
#define CIM1_GPIO_SENSOR_PWDN           (-1)
#define CIM1_GPIO_SENSOR_RST            GPIO_PA(19)
#define CIM1_GPIO_I2C_SEL1              GPIO_PA(20)
#define CIM1_GPIO_I2C_SEL2              (-1)
#define CIM1_DVP_GPIO_FUNC              DVP_PA_LOW_10BIT
#endif

#ifdef CONFIG_TX_ISP_CAMERA_OV9732
#define CIM1_SENSOR_NAME                "ov9732"
#define CIM1_GPIO_POWER                 (-1)
#define CIM1_GPIO_SENSOR_PWDN           (-1)
#define CIM1_GPIO_SENSOR_RST            GPIO_PA(18)
#define CIM1_GPIO_I2C_SEL1              (-1)
#define CIM1_GPIO_I2C_SEL2              (-1)
#define CIM1_DVP_GPIO_FUNC              DVP_PA_HIGH_10BIT
#endif

#ifdef CONFIG_TX_ISP_CAMERA_SC2235
#define CIM1_SENSOR_NAME                "sc2235"
#define CIM1_GPIO_POWER                 GPIO_PB(30)
#define CIM1_GPIO_SENSOR_PWDN           (-1)
#define CIM1_GPIO_SENSOR_RST            GPIO_PA(19)
#define CIM1_GPIO_I2C_SEL1              GPIO_PA(18)
#define CIM1_GPIO_I2C_SEL2              (-1)
#define CIM1_DVP_GPIO_FUNC              DVP_PA_LOW_8BIT
#endif

#ifdef CONFIG_TX_ISP_CAMERA_SC2235_DOUBLE
#define CIM1_SENSOR_NAME                "sc2235"
#define CIM1_GPIO_POWER                 GPIO_PB(30)
#define CIM1_GPIO_SENSOR_PWDN           (-1)
#define CIM1_GPIO_SENSOR_RST            GPIO_PA(19)
#define CIM1_GPIO_I2C_SEL1              GPIO_PA(18)
#define CIM1_GPIO_I2C_SEL2              GPIO_PA(20)
#define CIM1_DVP_GPIO_FUNC              DVP_PA_LOW_8BIT
#endif

#ifdef CONFIG_TX_ISP_CAMERA_AR0230
#define CIM1_SENSOR_NAME                "ar0230"
#define CIM1_GPIO_POWER                 GPIO_PB(30)
#define CIM1_GPIO_SENSOR_PWDN           (-1)
#define CIM1_GPIO_SENSOR_RST            GPIO_PA(19)
#define CIM1_GPIO_I2C_SEL1              GPIO_PA(18)
#define CIM1_GPIO_I2C_SEL2              (-1)
#define CIM1_DVP_GPIO_FUNC              DVP_PA_LOW_10BIT
#endif

#ifdef CONFIG_TX_ISP_CAMERA_IMX307
#define CIM1_SENSOR_NAME                "imx307"
#define CIM1_GPIO_POWER                 GPIO_PB(30)
#define CIM1_GPIO_SENSOR_PWDN           (-1)
#define CIM1_GPIO_SENSOR_RST            GPIO_PA(19)
#define CIM1_GPIO_I2C_SEL1              GPIO_PA(20)
#define CIM1_GPIO_I2C_SEL2              (-1)
#define CIM1_DVP_GPIO_FUNC              (-1)
#endif

#ifdef CONFIG_TX_ISP_CAMERA_JXF37
#define CIM1_SENSOR_NAME                "jxf37"
#define CIM1_GPIO_POWER                 (-1)
#define CIM1_GPIO_SENSOR_PWDN           GPIO_PA(19)
#define CIM1_GPIO_SENSOR_RST            GPIO_PA(18)
#define CIM1_GPIO_I2C_SEL1              (-1)
#define CIM1_GPIO_I2C_SEL2              (-1)
#define CIM1_DVP_GPIO_FUNC              DVP_PA_HIGH_10BIT
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

/*
 *Finger Print(Microarray)
 */
#ifdef CONFIG_FINGERPRINT_MICROARRAY
#define FINGERPRINT_POWER_EN                (-1)
#define FINGERPRINT_POWER_2V8               (-1)
#define FINGERPRINT_POWER_1V8               (-1)
#define FINGERPRINT_INT                     GPIO_PC(23)
#define FINGERPRINT_RESET                   (-1)//GPIO_PC(13)
#endif

/*
 * TouchScreen(Goodix)
 */
#ifdef CONFIG_TOUCHSCREEN_GT917S
#define GT917S_I2CBUS_NUM                   1
#define GTP_MAX_WIDTH                       720
#define GTP_MAX_HEIGHT                      1280
#define GTP_RST_PORT                        GPIO_PB(20)
#define GTP_INT_PORT                        GPIO_PB(1)
#endif

/*
 * audio op(aw87519)
 */
#ifdef CONFIG_AW87519
#define GPIO_AW87519_RESET                  GPIO_PB(15)
#endif


/*
 * SPI
 */
#ifdef CONFIG_SPI_GPIO
#define SPI_CHIP_ENABLE                 GPIO_PB(0)
#define GPIO_SPI0_MISO                   (-1)
#define GPIO_SPI0_MOSI                   GPIO_PC(28)
#define GPIO_SPI0_SCK                    GPIO_PC(27)
#define GPIO_SPI0_BUS_NUM                2
#endif

#ifdef CONFIG_JZ_SPI0
#define SPI0_CHIP_SELECT0               GPIO_PC(16)
#define SPI0_CHIP_SELECT1               GPIO_PB(29)
#define SPI0_CHIP_SELECT2               GPIO_PC(26)
#endif
#endif /* __BOARD_H__ */
