#ifndef __BOARD_BASE_H__
#define __BOARD_BASE_H__
#include <linux/i2c.h>
#include <linux/pwm.h>
#include <mach/sfc_flash.h>
#include <mach/jzfb.h>
#include <mach/jzsnd.h>
#include <linux/platform_data/asoc-ingenic.h>
#include <board.h>
#include <linux/lcd.h>

#ifdef CONFIG_KEYBOARD_GPIO
extern struct platform_device jz_button_device;
#endif

#ifdef CONFIG_KEYBOARD_JZ_ADC0
extern struct platform_device adc0_keys_dev;
#endif
#ifdef CONFIG_KEYBOARD_JZ_ADC1
extern struct platform_device adc1_keys_dev;
#endif
#ifdef CONFIG_KEYBOARD_JZ_ADC2
extern struct platform_device adc2_keys_dev;
#endif
#ifdef CONFIG_KEYBOARD_JZ_ADC3
extern struct platform_device adc3_keys_dev;
#endif

#ifdef CONFIG_JZMMC_V12_MMC0
extern struct jzmmc_platform_data tf_pdata;
#endif

#ifdef CONFIG_JZMMC_V12_MMC1
extern struct jzmmc_platform_data sdio_pdata;
#endif

#ifdef CONFIG_JZ_SFC
extern struct platform_device jz_sfc_device;
extern struct jz_sfc_info sfcflash_info;
#endif

#ifdef CONFIG_JZ_WDT
extern struct platform_device jz_wdt_device;
#endif

#ifdef CONFIG_SERIAL_JZ47XX_UART0
extern struct jz_uart_platform_data jz_uart0_platform_data;
#endif

#ifdef CONFIG_SERIAL_JZ47XX_UART1
extern struct jz_uart_platform_data jz_uart1_platform_data;
#endif

#ifdef CONFIG_JZ_EFUSE
extern struct jz_efuse_platform_data jz_efuse_pdata;
#endif

#ifdef CONFIG_FB_JZ_V14
extern struct jzfb_platform_data jzfb_pdata;
extern struct platform_device jz_fb_device;
#endif

#ifdef CONFIG_JZ_MAC
extern struct platform_device jz_mii_bus;
extern struct platform_device jz_mac_device;
#endif
/*-----------------------------------------*/
#ifdef CONFIG_JZ_AES
extern struct platform_device jz_aes_device;
#endif

#ifdef CONFIG_JZ_DES
extern struct platform_device jz_des_device;
#endif

#ifdef CONFIG_JZ_PWM
extern struct platform_device jz_pwm_device;
#endif

#ifdef CONFIG_JZ_PWM_GENERIC
extern struct platform_device jz_pwm_devs;
#endif

#ifdef CONFIG_SPI_GPIO
extern struct platform_device jz_spi_gpio_device;
#endif

#ifdef CONFIG_JZ_SPI0
extern struct jz_spi_info spi0_info_cfg;
#endif
#if defined(CONFIG_JZ_SPI0) || defined(CONFIG_SPI_GPIO)
extern struct spi_board_info jz_spi0_board_info[];
extern int jz_spi0_devs_size;
#endif

#ifdef CONFIG_SND_ASOC_JZ_ICDC_D4
extern struct snd_codec_pdata snd_codec_pdata;
#endif

#ifdef CONFIG_BCM_43438_RFKILL
extern struct platform_device   bt_power_device;
#endif
#if defined(CONFIG_SPI_GPIO)
extern struct spi_board_info jz_spi_board_info[];
extern int jz_spi_devs_size;
#endif

extern struct snd_codec_data codec_data;

#if defined(CONFIG_USB_ANDROID_HID)
extern struct platform_device jz_hidg;
#endif

#endif	/* __BOARD_BASE_H__ */
