#include <linux/platform_device.h>
#include "board_base.h"

#if IS_ENABLED(CONFIG_JZ_EFUSE_V12)
#include <mach/jz_efuse.h>
struct jz_efuse_platform_data jz_efuse_pdata = {
	/* supply 2.5V to VDDQ */
	.gpio_vddq_en = GPIO_EFUSE_VDDQ,
	.gpio_vddq_en_level = GPIO_EFUSE_VDDQ_EN_LEVEL,
};
#endif

#if defined(CONFIG_USB_JZ_DWC2)
#ifndef CONFIG_BOARD_HAS_NO_DETE_FACILITY
#if defined(GPIO_USB_ID) && defined(GPIO_USB_ID_LEVEL)
struct jzdwc_pin dwc2_id_pin = {
	.num = GPIO_USB_ID,
	.enable_level = GPIO_USB_ID_LEVEL,
};
#endif

#if defined(GPIO_USB_DETE) && defined(GPIO_USB_DETE_LEVEL)
struct jzdwc_pin dwc2_dete_pin = {
	.num = GPIO_USB_DETE,
	.enable_level = GPIO_USB_DETE_LEVEL,
};
#endif
#endif /*!CONFIG_BOARD_HAS_NO_DETE_FACILITY*/

#if defined(GPIO_USB_DRVVBUS) && defined(GPIO_USB_DRVVBUS_LEVEL) && !defined(USB_DWC2_DRVVBUS_FUNCTION_PIN)
struct jzdwc_pin dwc2_drvvbus_pin = {
	.num = GPIO_USB_DRVVBUS,
	.enable_level = GPIO_USB_DRVVBUS_LEVEL,
};
#endif
#endif /*CONFIG_USB_DWC2*/

#if defined(CONFIG_SND_ASOC_INGENIC)
struct platform_device snd_alsa_device = {
	.name = "ingenic-halley2pro",
};
#endif
