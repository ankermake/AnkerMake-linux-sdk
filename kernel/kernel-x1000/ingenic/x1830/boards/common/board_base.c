#include <linux/platform_device.h>
#include <linux/i2c.h>
#include <linux/i2c-gpio.h>
#include <linux/input.h>
#include <linux/spi/spi.h>
#include <linux/spi/spi_gpio.h>
#include <linux/interrupt.h>
#include <linux/jz_dwc.h>
#include <mach/platform.h>
#include <mach/jz_efuse.h>
#include <mach/jzmmc.h>
#include <gpio.h>
#include <linux/pwm.h>
#include "board_base.h"
#include <mach/jzssi.h>
#include <mach/platform.h>
#include <linux/platform_data/asoc-ingenic.h>

struct jz_platform_device
{
	struct platform_device *pdevices;
	void *pdata;
	int size;
};

#ifdef CONFIG_SND_ASOC_INGENIC
static struct snd_dma_data snd_dma_data = {
    .dma_write_oncache = 1,
};
#endif

static struct jz_platform_device platform_devices_array[] __initdata = {
#define DEF_DEVICE(DEVICE, DATA, SIZE)	\
	{ .pdevices = DEVICE,	\
	  .pdata = DATA, .size = SIZE,}

#ifdef CONFIG_KEYBOARD_GPIO
	DEF_DEVICE(&jz_button_device, 0, 0),
#endif

#ifdef CONFIG_KEYBOARD_JZ_ADC0
	DEF_DEVICE(&adc0_keys_dev, 0, 0),
#endif
#ifdef CONFIG_KEYBOARD_JZ_ADC1
	DEF_DEVICE(&adc1_keys_dev, 0, 0),
#endif
#ifdef CONFIG_KEYBOARD_JZ_ADC2
	DEF_DEVICE(&adc2_keys_dev, 0, 0),
#endif
#ifdef CONFIG_KEYBOARD_JZ_ADC3
	DEF_DEVICE(&adc3_keys_dev, 0, 0),
#endif

#ifdef CONFIG_I2C0_V12_JZ
	DEF_DEVICE(&jz_i2c0_device, 0, 0),
#endif

#ifdef CONFIG_I2C1_V12_JZ
	DEF_DEVICE(&jz_i2c1_device, 0, 0),
#endif

#ifdef CONFIG_I2C2_V12_JZ
	DEF_DEVICE(&jz_i2c2_device, 0, 0),
#endif

#ifdef CONFIG_JZMMC_V12_MMC0
	DEF_DEVICE(&jz_msc0_device, &tf_pdata, sizeof(struct jzmmc_platform_data)),
#endif

#ifdef CONFIG_JZMMC_V12_MMC1
	DEF_DEVICE(&jz_msc1_device, &sdio_pdata, sizeof(struct jzmmc_platform_data)),
#endif

#ifdef CONFIG_MFD_JZ_SADC_V13
	DEF_DEVICE(&jz_adc_device, 0, 0),
#endif

#ifdef CONFIG_XBURST_DMAC
	DEF_DEVICE(&jz_pdma_device, 0, 0),
#endif

#ifdef CONFIG_SERIAL_JZ47XX_UART0
	DEF_DEVICE(&jz_uart0_device, 0, 0),
#endif

#ifdef CONFIG_SERIAL_JZ47XX_UART1
	DEF_DEVICE(&jz_uart1_device, 0, 0),
#endif

#if defined(CONFIG_USB_JZ_DWC2) || defined(CONFIG_USB_DWC2)
	DEF_DEVICE(&jz_dwc_otg_device, 0, 0),
#endif

#ifdef CONFIG_SND_ASOC_INGENIC
	DEF_DEVICE(&jz_aic_dma_device, &snd_dma_data, sizeof(snd_dma_data)),
	DEF_DEVICE(&jz_aic_device, NULL, 0),
#ifdef CONFIG_SND_ASOC_JZ_ICDC_D4
	DEF_DEVICE(&jz_icdc_device, &snd_codec_pdata, sizeof(struct snd_codec_pdata)),
#endif
#ifdef CONFIG_SND_ASOC_JZ_DMIC_V13
	DEF_DEVICE(&jz_dmic_device, NULL, 0),
	DEF_DEVICE(&jz_dmic_dma_device, NULL, 0),
	DEF_DEVICE(&jz_dmic_dump_cdc_device,0,0),
#endif
	DEF_DEVICE(&snd_alsa_device, NULL, 0),
#endif

#ifdef CONFIG_x1800_MIC
	DEF_DEVICE(&jz_codec_device, &codec_data, sizeof(struct snd_codec_data)),
	DEF_DEVICE(&mic_device,0,0),
#endif

#ifdef CONFIG_JZ_SFC
	DEF_DEVICE(&jz_sfc_device, &sfcflash_info, sizeof(struct jz_sfc_info)),
#endif

#ifdef CONFIG_JZ_WDT
	DEF_DEVICE(&jz_wdt_device, 0, 0),
#endif

#ifdef CONFIG_JZ_EFUSE
	DEF_DEVICE(&jz_efuse_device, &jz_efuse_pdata, sizeof(struct jz_efuse_platform_data)),
#endif

#ifdef CONFIG_FB_JZ_V14
	DEF_DEVICE(&jz_fb_device, &jzfb_pdata, sizeof(struct jzfb_platform_data)),
#endif

#ifdef CONFIG_JZ_MICCHAR
	DEF_DEVICE(&mic_device,0,0),
#endif

#ifdef CONFIG_JZ_PWM
	DEF_DEVICE(&jz_pwm_device, 0, 0),
#endif

#ifdef CONFIG_JZ_PWM_GENERIC
    DEF_DEVICE(&jz_pwm_devs, 0, 0),
#endif


#ifdef CONFIG_RTC_DRV_JZ
	DEF_DEVICE(&jz_rtc_device, 0, 0),
#endif

#ifdef CONFIG_SPI_GPIO
	DEF_DEVICE(&jz_spi_gpio_device, 0, 0),
#endif

#ifdef CONFIG_JZ_SPI0
	DEF_DEVICE(&jz_ssi0_device, &spi0_info_cfg, sizeof(struct jz_spi_info)),
#endif

#ifdef CONFIG_JZ_MAC
    DEF_DEVICE(&jz_mii_bus, 0, 0),
    DEF_DEVICE(&jz_mac_device, 0, 0),
#endif


#ifdef CONFIG_JZ_VPU_IRQ_TEST
	DEF_DEVICE(&jz_vpu_irq_device, 0, 0),
#elif defined(CONFIG_SOC_VPU)
    #ifdef CONFIG_VPU_HELIX
        #if (CONFIG_VPU_HELIX_NUM >= 1)
	        DEF_DEVICE(&jz_vpu_helix0_device, 0, 0),
        #endif
    #endif
    #ifdef CONFIG_VPU_RADIX
        #if (CONFIG_VPU_RADIX_NUM >= 1)
	        DEF_DEVICE(&jz_vpu_radix0_device, 0, 0),
        #endif
    #endif
#endif

#ifdef CONFIG_JZ_IPU_V12
	DEF_DEVICE(&jz_ipu_device, 0, 0),
#endif
#ifdef CONFIG_JZ_IPU_V13
	DEF_DEVICE(&jz_ipu_device, 0, 0),
#endif

/*-------------------------------------------*/
#ifdef CONFIG_JZ_AES
	DEF_DEVICE(&jz_aes_device, 0, 0),
#endif

#ifdef CONFIG_BCM_43438_RFKILL
	DEF_DEVICE(&bt_power_device,0,0),
#endif

#if defined(CONFIG_USB_ANDROID_HID)
       DEF_DEVICE(&jz_hidg, NULL, 0),
#endif
};

static int __init board_base_init(void)
{
	int pdevices_array_size, i;

	pdevices_array_size = ARRAY_SIZE(platform_devices_array);
	for(i = 0; i < pdevices_array_size; i++) {
		if(platform_devices_array[i].size)
			platform_device_add_data(platform_devices_array[i].pdevices,
						 platform_devices_array[i].pdata, platform_devices_array[i].size);
		platform_device_register(platform_devices_array[i].pdevices);
	}
#if defined(CONFIG_JZ_SPI0) || defined(CONFIG_SPI_GPIO)
        spi_register_board_info(jz_spi0_board_info, jz_spi0_devs_size);
#endif

	return 0;
}

/*
 * Called by arch/mips/kernel/proc.c when 'cat /proc/cpuinfo'.
 * Android requires the 'Hardware:' field in cpuinfo to setup the init.%hardware%.rc.
 */
const char *get_board_type(void)
{
	return CONFIG_PRODUCT_NAME;
}
arch_initcall(board_base_init);
