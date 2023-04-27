#ifndef __BOARD_BASE_H__
#define __BOARD_BASE_H__
#include <linux/i2c.h>
#include <mach/jz_efuse.h>
#include <board.h>
#if IS_ENABLED(CONFIG_JZ_EFUSE_V12)
extern struct jz_efuse_platform_data jz_efuse_pdata;
#endif
#ifdef CONFIG_KEYBOARD_GPIO
extern struct platform_device jz_button_device;
#endif
#ifdef CONFIG_VIDEO_OVISP
extern struct ovisp_camera_platform_data ovisp_camera_info;
#endif
#ifdef CONFIG_INV_MPU_IIO
extern struct mpu_platform_data mpu9250_platform_data;
#endif
#if (defined(CONFIG_I2C_GPIO) || defined(CONFIG_I2C0_V12_JZ))
extern struct i2c_board_info jz_i2c0_devs[];
extern int jz_i2c0_devs_size;
#endif
#if (defined(CONFIG_I2C_GPIO) || defined(CONFIG_I2C1_V12_JZ))
extern struct i2c_board_info jz_i2c1_devs[];
extern int jz_i2c1_devs_size;
#endif
#if (defined(CONFIG_I2C_GPIO) || defined(CONFIG_I2C2_V12_JZ))
extern struct i2c_board_info jz_i2c2_devs[];
extern int jz_i2c2_devs_size;
#endif
#ifdef CONFIG_I2C_GPIO
#ifndef CONFIG_I2C0_V12_JZ
extern struct platform_device i2c0_gpio_device;
#endif
#ifndef CONFIG_I2C1_V12_JZ
extern struct platform_device i2c1_gpio_device;
#endif
#ifndef CONFIG_I2C2_V12_JZ
extern struct platform_device i2c2_gpio_device;
#endif
#ifndef CONFIG_I2C3_V12_JZ
extern struct platform_device i2c3_gpio_device;
#endif
#endif	/* CONFIG_I2C_GPIO */

#ifdef CONFIG_SOUND_OSS_XBURST
extern struct snd_codec_data codec_data;
#endif
#ifdef CONFIG_BCM_PM_CORE
extern struct platform_device bcm_power_platform_device;
#endif
#ifdef CONFIG_BCM2079X_NFC
extern struct bcm2079x_platform_data bcm2079x_pdata;
#endif
#ifndef CONFIG_NAND
#ifdef CONFIG_JZMMC_V12_MMC0
extern struct jzmmc_platform_data inand_pdata;
#endif
#endif
#ifdef CONFIG_JZMMC_V12_MMC1
extern struct jzmmc_platform_data sdio_pdata;
#endif

/* Digital pulse backlight*/
#ifdef CONFIG_BACKLIGHT_DIGITAL_PULSE
extern struct platform_device digital_pulse_backlight_device;
extern struct platform_digital_pulse_backlight_data bl_data;
#endif
#ifdef CONFIG_BACKLIGHT_PWM
extern struct platform_device backlight_device;
#endif

#ifdef CONFIG_JZ_EPD_V12
extern struct platform_device jz_epd_device;
extern struct jz_epd_platform_data jz_epd_pdata;
#endif

/* lcd pdata and display panel */
#if (defined(CONFIG_FB_JZ_V12) || defined(CONFIG_DRM_JZDRM))
extern struct jzfb_platform_data jzfb_pdata;
#endif
#ifdef CONFIG_LCD_STARTEK_KD050HDFIA019
extern struct mipi_dsim_lcd_device	startek_kd050hdfia019_device;
#endif
#ifdef CONFIG_LCD_STARTEK_KD050HDFIA020
extern struct mipi_dsim_lcd_device	startek_kd050hdfia020_device;
#endif
#ifdef CONFIG_LCD_VISIONOX_T078ZC04H01
extern struct mipi_dsim_lcd_device	visionox_t078zc04h01_device;
#endif
#ifdef CONFIG_LCD_XINLI_X078DTLN06
extern struct mipi_dsim_lcd_device	xinli_x078dtln06_device;
#endif
#ifdef CONFIG_JZ_BATTERY
extern struct jz_adc_platform_data adc_platform_data;
#endif
#if defined(CONFIG_SND_ASOC_INGENIC)
extern struct platform_device snd_alsa_device;
#endif
#ifdef CONFIG_MTD_NAND_JZ
extern struct platform_device jz_mtd_nand_device;
#ifdef CONFIG_MTD_NAND_JZ_NORMAL
extern struct xburst_nand_chip_platform_data nand_chip_data;
#endif
#endif	/*CONFIG_MTD_NAND_JZ*/
#ifdef CONFIG_BCM_43438_RFKILL
extern struct platform_device bt_power_device;
#endif
#endif	/* __BOARD_BASE_H__ */
