#ifndef __BOARD_H__
#define __BOARD_H__
#include <gpio.h>
#include <soc/gpio.h>

/* MSC GPIO Definition */
#define GPIO_SD0_CD_N       GPIO_PC(24)

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
#define GPIO_USB_DETE_LEVEL     HIGH_ENABLE
#endif
#define GPIO_USB_DRVVBUS        GPIO_PB(25)
#define GPIO_USB_DRVVBUS_LEVEL      HIGH_ENABLE
/* ****************************GPIO USB END********************************** */

#endif /* __BOARD_H__ */
