/* SPDX-License-Identifier: GPL-2.0 */
#ifndef _PHY_INGENIC_H_
#define _PHY_INGENIC_H_

#include <asm/io.h>
#include <linux/err.h>
#include <linux/workqueue.h>
#include <linux/usb.h>
#include <linux/usb/gadget.h>
#include <linux/usb/otg.h>


struct ingenic_phy_data {
	int gpio_vbus;
	int gpio_vbus_inverted;

	int gpio_drvvbus;
	int gpio_drvvbus_inverted;

	int gpio_switch;
	int gpio_switch_inverted;

	int gpio_wakeup;
	int gpio_wakeup_inverted;

	int gpio_vbus_wakeup_enable;
};


#endif /* _PHY_INGENIC_H_ */