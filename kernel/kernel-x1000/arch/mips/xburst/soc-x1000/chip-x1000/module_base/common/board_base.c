#include <linux/platform_device.h>
#include <linux/i2c.h>
#include <linux/i2c-gpio.h>
#include <linux/input.h>
#include <linux/spi/spi.h>
#include <linux/spi/spi_gpio.h>
#include <linux/interrupt.h>
#include <linux/jz_dwc.h>
#include <linux/delay.h>
#include <mach/jzsnd.h>
#include <mach/platform.h>
#include <mach/jzfb.h>
#include <mach/jzmmc.h>
#include <mach/jzssi.h>
#include <gpio.h>
#include <mach/jz_efuse.h>
#include "board_base.h"


struct jz_platform_device
{
	struct platform_device *pdevices;
	void *pdata;
	int size;
};

static struct jz_platform_device platform_devices_array[] __initdata = {
#define DEF_DEVICE(DEVICE, DATA, SIZE)  \
	{ .pdevices = DEVICE,   \
		.pdata = DATA, .size = SIZE,}

#ifdef CONFIG_SERIAL_JZ47XX_UART0
	DEF_DEVICE(&jz_uart0_device, 0, 0),
#endif
#ifdef CONFIG_SERIAL_JZ47XX_UART1
	DEF_DEVICE(&jz_uart1_device, 0, 0),
#endif
#ifdef CONFIG_SERIAL_JZ47XX_UART2
	DEF_DEVICE(&jz_uart2_device, 0, 0),
#endif
#ifdef CONFIG_JZMMC_V11_MMC1
	DEF_DEVICE(&jz_msc1_device,&tf_pdata,sizeof(struct jzmmc_platform_data)),
#endif
#ifdef CONFIG_JZMMC_V12_MMC0
	DEF_DEVICE(&jz_msc0_device,&tf_pdata,sizeof(struct jzmmc_platform_data)),
#endif
#ifdef CONFIG_JZMMC_V12_MMC1
	DEF_DEVICE(&jz_msc1_device,&sdio_pdata,sizeof(struct jzmmc_platform_data)),
#endif
#ifdef CONFIG_JZMMC_V11_MMC2
	DEF_DEVICE(&jz_msc2_device,&sdio_pdata,sizeof(struct jzmmc_platform_data)),
#endif

/* end of ALSA audio driver */
#if defined(CONFIG_USB_DWC2) || defined(CONFIG_USB_JZ_DWC2)
	DEF_DEVICE(&jz_dwc_otg_device,0,0),
#endif

#ifdef CONFIG_JZ_WDT
       DEF_DEVICE(&jz_wdt_device, 0, 0),
#endif

#ifdef CONFIG_XBURST_DMAC_V13
	DEF_DEVICE(&jz_pdma_device, 0, 0),
#endif
#ifdef CONFIG_XBURST_DMAC
	DEF_DEVICE(&jz_pdma_device, 0, 0),
#endif

#ifdef CONFIG_USB_OHCI_HCD
	DEF_DEVICE(&jz_ohci_device,0,0),
#endif

#ifdef CONFIG_RTC_DRV_JZ
	DEF_DEVICE(&jz_rtc_device, 0, 0),
#endif
#ifdef CONFIG_JZ_SECURITY
	DEF_DEVICE(&jz_security_device, 0, 0),
#endif
#ifdef	CONFIG_JZ_SFC
	DEF_DEVICE(&jz_sfc_device, &sfcflash_info, sizeof(struct jz_sfc_info)),
#endif
#if defined(CONFIG_JZ_VPU_V13) || defined(CONFIG_VIDEO_INGENIC_X1000_JPEG)
	DEF_DEVICE(&jz_vpu_device, 0, 0),
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

	return 0;
}


/*
 *  * Called by arch/mips/kernel/proc.c when 'cat /proc/cpuinfo'.
 *   * Android requires the 'Hardware:' field in cpuinfo to setup the init.%hardware%.rc.
 *    */
const char *get_board_type(void)
{
	return CONFIG_PRODUCT_NAME;
}


arch_initcall(board_base_init);
