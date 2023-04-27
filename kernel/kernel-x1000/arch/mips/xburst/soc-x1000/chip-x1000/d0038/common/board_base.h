#ifndef __BOARD_BASE_H__
#define __BOARD_BASE_H__

#include <linux/spi/spi.h>
#include <linux/i2c.h>

#include "board.h"
#include "akm4951_arch.h"

#if defined(CONFIG_SND) && defined(CONFIG_SND_ASOC_INGENIC)
#if defined(CONFIG_SND_ASOC_JZ_AIC_V12)
extern struct snd_dma_data aic_dma_data;
#endif
#if defined(CONFIG_SND_ASOC_JZ_PCM_V13)
extern struct snd_dma_data pcm_dma_data;
#endif
#if defined(CONFIG_SND_ASOC_JZ_DMIC_V13)
extern struct snd_dma_data dmic_dma_data;
#endif
#endif

#ifdef CONFIG_AKM4951_EXTERNAL_CODEC
extern struct akm4951_arch_data akm4951_priv;
extern struct snd_codec_data akm4951_codec_data;
extern struct platform_device akm4951_codec_device;
#endif

#ifdef CONFIG_KEYBOARD_GPIO
extern struct platform_device jz_button_device;
#endif
#ifdef CONFIG_KEYBOARD_MATRIX
extern struct platform_device jz_matrix_keypad_device;
#endif
#ifdef CONFIG_LEDS_GPIO
extern struct platform_device jz_leds_gpio;
#endif

#ifdef CONFIG_JZ_INTERNAL_CODEC_V12
extern struct snd_codec_data codec_data;
#endif

#ifdef CONFIG_SND_ASOC_INGENIC
extern struct platform_device snd_alsa_device;
#endif

#ifdef CONFIG_JZMMC_V12_MMC0
extern struct jzmmc_platform_data tf_pdata;
#endif
#ifdef CONFIG_JZMMC_V12_MMC1
extern struct jzmmc_platform_data sdio_pdata;
#endif

#ifdef CONFIG_JZMMC_V11_MMC2
extern struct jzmmc_platform_data sdio_pdata;
#endif

#ifdef CONFIG_SOFT_I2C0_GPIO_V12_JZ
extern struct platform_device i2c0_gpio_device;
#endif

#ifdef CONFIG_SOFT_I2C1_GPIO_V12_JZ
extern struct platform_device i2c1_gpio_device;
#endif

#if (defined(CONFIG_SOFT_I2C0_GPIO_V12_JZ) || defined(CONFIG_I2C0_V12_JZ))
extern struct platform_device jz_i2c0_device;
extern struct i2c_board_info jz_i2c0_devs[];
extern int jz_i2c0_devs_size;
#endif
#if (defined(CONFIG_SOFT_I2C1_GPIO_V12_JZ) || defined(CONFIG_I2C1_V12_JZ))
extern struct platform_device jz_i2c1_device;
extern struct i2c_board_info jz_i2c1_devs[];
extern int jz_i2c1_devs_size;
#endif
#if (defined(CONFIG_SOFT_I2C2_GPIO_V12_JZ) || defined(CONFIG_I2C2_V12_JZ))
extern struct platform_device jz_i2c2_device;
extern struct i2c_board_info jz_i2c2_devs[];
extern int jz_i2c2_devs_size;
#endif

#ifdef CONFIG_BROADCOM_RFKILL
extern struct platform_device	bt_power_device;
extern struct platform_device	bluesleep_device;
#endif

#ifdef CONFIG_BCM_AP6212_RFKILL
extern struct platform_device   bt_power_device;
#endif

#ifdef CONFIG_BCM_AP6255_RFKILL
extern struct platform_device   bt_power_device;
#endif

#ifdef CONFIG_USB_DWC2
extern struct platform_device   jz_dwc_otg_device;
#endif

#ifdef CONFIG_JZ_SPI0
extern struct jz_spi_info spi0_info_cfg;
#endif
#ifdef CONFIG_JZ_SFC
extern struct jz_sfc_info sfc_info_cfg;
#endif

#ifdef CONFIG_SPI_GPIO
extern struct platform_device jz_spi_gpio_device;
#endif

#ifdef CONFIG_JZ_EFUSE_V13
extern struct jz_efuse_platform_data jz_efuse_pdata;
#endif

#ifdef CONFIG_LEDS_PWM
extern struct platform_device jz_leds_pwm;
#endif


/* JZ LCD driver */
#ifdef CONFIG_BACKLIGHT_PWM
extern struct platform_device backlight_device;
#endif
#ifdef CONFIG_FB_JZ_V13
extern struct jzfb_platform_data jzfb_pdata;
#endif
#ifdef CONFIG_LCD_FRD20024N
extern struct platform_device frd20024n_device;
#endif
/* end of LCD driver */

#endif
