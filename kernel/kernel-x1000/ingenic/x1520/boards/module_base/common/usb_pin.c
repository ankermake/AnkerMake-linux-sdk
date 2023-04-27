#include <linux/platform_device.h>
#include "board_base.h"
#include <linux/jz_dwc.h>

#if defined(GPIO_USB_ID) && defined(GPIO_USB_ID_LEVEL)
struct jzdwc_pin dwc2_id_pin = {
    .num                = GPIO_USB_ID,
    .enable_level       = GPIO_USB_ID_LEVEL,
};
#endif

#if defined(GPIO_USB_DETE) && defined(GPIO_USB_DETE_LEVEL)
struct jzdwc_pin dwc2_dete_pin = {
    .num                = GPIO_USB_DETE,
    .enable_level       = GPIO_USB_DETE_LEVEL,
};
#endif

#if defined(GPIO_USB_DRVVBUS) && defined(GPIO_USB_DRVVBUS_LEVEL) && !defined(USB_DWC2_DRVVBUS_FUNCTION_PIN)
struct jzdwc_pin dwc2_drvvbus_pin = {
    .num                = GPIO_USB_DRVVBUS,
    .enable_level       = GPIO_USB_DRVVBUS_LEVEL,
};
#endif

