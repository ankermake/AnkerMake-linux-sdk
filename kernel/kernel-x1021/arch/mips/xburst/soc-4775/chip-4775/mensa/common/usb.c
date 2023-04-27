#include <linux/err.h>
#include "board.h"

static int usb_host_5v_en(void) {
#ifdef CONFIG_BOARD_MANSA_V20
	if(gpio_request(GPIO_USB_HOST_5V_EN,"usb_en")){
		pr_err("USB Host OHCI Request Failed!\n");
		return -EINVAL;
	}else
	gpio_direction_output(GPIO_USB_HOST_5V_EN,1);
#endif
	return 0;
}

static int __init mensa_board_usb_init(void){
	usb_host_5v_en();
	return 0;
}
module_init(mensa_board_usb_init);
