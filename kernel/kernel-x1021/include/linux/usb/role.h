// SPDX-License-Identifier: GPL-2.0

#ifndef __LINUX_USB_ROLE_H
#define __LINUX_USB_ROLE_H

#include <linux/device.h>

struct usb_role_switch;

enum usb_role {
	USB_ROLE_NONE,
	USB_ROLE_HOST,
	USB_ROLE_DEVICE,
};

typedef int (*usb_role_switch_set_t)(struct usb_role_switch *sw,
				     enum usb_role role);
typedef enum usb_role (*usb_role_switch_get_t)(struct usb_role_switch *sw);

/**
 * struct usb_role_switch_desc - USB Role Switch Descriptor
 * @set: Callback for setting the role
 * @get: Callback for getting the role (optional)
 * @allow_userspace_control: If true userspace may change the role through sysfs
 * @driver_data: Private data pointer
 * @name: Name for the switch (optional)
 */
struct usb_role_switch_desc {
	usb_role_switch_set_t set;
	usb_role_switch_get_t get;
	bool allow_userspace_control;
	void *driver_data;
	const char *name;
};


#if IS_ENABLED(CONFIG_USB_ROLE_SWITCH)
int usb_role_switch_set_role(struct usb_role_switch *sw, enum usb_role role);
enum usb_role usb_role_switch_get_role(struct usb_role_switch *sw);

struct usb_role_switch *
usb_role_switch_register(struct device *parent,
			 const struct usb_role_switch_desc *desc);
void usb_role_switch_unregister(struct usb_role_switch *sw);

void usb_role_switch_set_drvdata(struct usb_role_switch *sw, void *data);
void *usb_role_switch_get_drvdata(struct usb_role_switch *sw);
const char *usb_role_string(enum usb_role role);
#else
static inline int usb_role_switch_set_role(struct usb_role_switch *sw,
		enum usb_role role)
{
	return 0;
}

static inline enum usb_role usb_role_switch_get_role(struct usb_role_switch *sw)
{
	return USB_ROLE_NONE;
}
static inline struct usb_role_switch *
usb_role_switch_register(struct device *parent,
			 const struct usb_role_switch_desc *desc)
{
	return ERR_PTR(-ENODEV);
}

static inline void usb_role_switch_unregister(struct usb_role_switch *sw) { }

static inline void
usb_role_switch_set_drvdata(struct usb_role_switch *sw, void *data)
{
}

static inline void *usb_role_switch_get_drvdata(struct usb_role_switch *sw)
{
	return NULL;
}

static inline const char *usb_role_string(enum usb_role role)
{
	return "unknown";
}

#endif

#endif /* __LINUX_USB_ROLE_H */
