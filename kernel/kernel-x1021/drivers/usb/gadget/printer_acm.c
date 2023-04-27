#include <linux/kernel.h>
#include <linux/module.h>

#include "u_printer.h"
#include "u_serial.h"


#define DRIVER_DESC		"Printer and ACM Gadget"
#define DRIVER_VERSION	"May 2019"
#define SERIALNUM		"1"
/*-------------------------------------------------------------------------*/

#define PRINTER_CDC_VENDOR_NUM		0x6666	/* NetChip */
#define PRINTER_CDC_PRODUCT_NUM		0x8888	/* Printer + ACM */

/*-------------------------------------------------------------------------*/
USB_GADGET_COMPOSITE_OPTIONS();

/*
 * Kbuild is not very cooperative with respect to linking separately
 * compiled library objects into one module.  So for now we won't use
 * separate compilation ... ensuring init/exit sections work to shrink
 * the runtime footprint, and giving us at least some parts of what
 * a "gcc --combine ... part1.c part2.c part3.c ... " build would.
 */
/*-------------------------------------------------------------------------*/

static struct usb_device_descriptor device_desc = {
	.bLength =		sizeof device_desc,
	.bDescriptorType =	USB_DT_DEVICE,
	.bcdUSB =		cpu_to_le16(0x0200),
	.bDeviceClass =		USB_CLASS_PER_INTERFACE,
	.bDeviceSubClass =	0,
	.bDeviceProtocol =	0,
	/* .bMaxPacketSize0 = f(hardware) */

	/* Vendor and product id can be overridden by module parameters.  */
	.idVendor =		cpu_to_le16(PRINTER_CDC_VENDOR_NUM),
	.idProduct =		cpu_to_le16(PRINTER_CDC_PRODUCT_NUM),
	/* .bcdDevice = f(hardware) */
	/* .iManufacturer = DYNAMIC */
	/* .iProduct = DYNAMIC */
	/* NO SERIAL NUMBER */
	.bNumConfigurations =	1,
};

static struct usb_otg_descriptor otg_descriptor = {
	.bLength =		sizeof otg_descriptor,
	.bDescriptorType =	USB_DT_OTG,

	/* REVISIT SRP-only hardware is possible, although
	 * it would not be called "OTG" ...
	 */
	.bmAttributes =		USB_OTG_SRP | USB_OTG_HNP,
};

static const struct usb_descriptor_header *otg_desc[] = {
	(struct usb_descriptor_header *) &otg_descriptor,
	NULL,
};

/* Some systems will want different product identifiers published in the
 * device descriptor, either numbers or strings or both.  These string
 * parameters are in UTF-8 (superset of ASCII's 7 bit characters).
 */

static char *iPNPstring;
module_param(iPNPstring, charp, S_IRUGO);
MODULE_PARM_DESC(iPNPstring, "MFG:linux;MDL:g_printer;CLS:PRINTER;SN:1;");

/* Number of requests to allocate per endpoint, not used for ep0. */
static unsigned qlen = 10;
module_param(qlen, uint, S_IRUGO|S_IWUSR);

#define QLEN	qlen

static char				*pnp_string =
	"MFG:linux;MDL:g_printer;CLS:PRINTER;SN:1;";

/* string IDs are assigned dynamically */
static struct usb_string strings_dev[] = {
	[USB_GADGET_MANUFACTURER_IDX].s = "",
	[USB_GADGET_PRODUCT_IDX].s = DRIVER_DESC,
	[USB_GADGET_SERIAL_IDX].s = SERIALNUM,
	{  } /* end of list */
};

static struct usb_gadget_strings stringtab_dev = {
	.language	= 0x0409,	/* en-us */
	.strings	= strings_dev,
};

static struct usb_gadget_strings *dev_strings[] = {
	&stringtab_dev,
	NULL,
};

/*-------------------------------------------------------------------------*/
static struct usb_function *f_printer;
static struct usb_function_instance *fi_printer;

static struct usb_function *f_acm;
static struct usb_function_instance *fi_serial;
/*
 * We _always_ have both Printer and ACM functions.
 */
static int __init printer_acm_do_config(struct usb_configuration *c)
{
	int	status;
	struct f_printer_opts *opts;
	struct usb_gadget	*gadget = c->cdev->gadget;

	usb_ep_autoconfig_reset(gadget);

	usb_gadget_set_selfpowered(gadget);

	if (gadget_is_otg(gadget)) {
		c->descriptors = otg_desc;
		c->bmAttributes |= USB_CONFIG_ATT_WAKEUP;
	}

	/**********************ACM*************/
	fi_serial=usb_get_function_instance("acm");
	if (IS_ERR(fi_serial))
		return PTR_ERR(fi_serial);

	f_acm = usb_get_function(fi_serial);
	if (IS_ERR(f_acm)) {
		status = PTR_ERR(f_acm);
		goto err_acm_func;
	}

	status = usb_add_function(c, f_acm);
	if (status)
		goto err_acm_conf;

	fi_printer = usb_get_function_instance("printer");
	if (IS_ERR(fi_printer)) {
		status = PTR_ERR(fi_printer);;
		goto err_acm_remove;
	}

	opts = container_of(fi_printer, struct f_printer_opts, func_inst);
	opts->minor = 0;
	opts->q_len = QLEN;
	if (iPNPstring) {
		opts->pnp_string = kstrdup(iPNPstring, GFP_KERNEL);
		if (!opts->pnp_string) {
			status = -ENOMEM;
			goto err_printer_func;
		}
		opts->pnp_string_allocated = true;
		/*
		 * we don't free this memory in case of error
		 * as printer cleanup func will do this for us
		 */
	} else {
		opts->pnp_string = pnp_string;
	}

	f_printer = usb_get_function(fi_printer);
	if (IS_ERR(f_printer)) {
		status = PTR_ERR(f_printer);
		goto err_printer_func;
	}

	status = usb_add_function(c, f_printer);
	if (status)
		goto err_printer_conf;

	return 0;

err_printer_conf:
	usb_put_function(f_printer);
err_printer_func:
	usb_put_function_instance(fi_printer);
err_acm_remove:
	usb_remove_function(c, f_acm);
err_acm_conf:
	usb_put_function(f_acm);
err_acm_func:
	usb_put_function_instance(fi_serial);
	return status;
}

static struct usb_configuration printer_cdc_config_driver = {
	.label			= "Printer and ACM",
	.bConfigurationValue	= 1,
	/* .iConfiguration = DYNAMIC */
	.bmAttributes		= USB_CONFIG_ATT_ONE | USB_CONFIG_ATT_SELFPOWER,
};

/*-------------------------------------------------------------------------*/

static int __init printer_acm_bind(struct usb_composite_dev *cdev)
{
	int			status;
	struct usb_gadget	*gadget = cdev->gadget;

	/* Allocate string descriptor numbers ... note that string
	 * contents can be overridden by the composite_dev glue.
	 */

	status = usb_string_ids_tab(cdev, strings_dev);
	if (status < 0)
		goto fail1;
	device_desc.iManufacturer = strings_dev[USB_GADGET_MANUFACTURER_IDX].id;
	device_desc.iProduct = strings_dev[USB_GADGET_PRODUCT_IDX].id;

	/* register our configuration */
	status = usb_add_config(cdev, &printer_cdc_config_driver, printer_acm_do_config);
	if (status < 0)
		goto fail1;

	usb_composite_overwrite_options(cdev, &coverwrite);
	dev_info(&gadget->dev, "%s, version: " DRIVER_VERSION "\n",
			DRIVER_DESC);

	return status;

fail1:
	return status;
}

static int __exit printer_acm_unbind(struct usb_composite_dev *cdev)
{
	usb_put_function(f_printer);
	usb_put_function_instance(fi_printer);

	usb_put_function(f_acm);
	usb_put_function_instance(fi_serial);

	return 0;
}

static __refdata struct usb_composite_driver printer_acm_driver = {
	.name		= "g_printer_acm",
	.dev		= &device_desc,
	.strings	= dev_strings,
	.max_speed	= USB_SPEED_HIGH,
	.bind		= printer_acm_bind,
	.unbind		= __exit_p(printer_acm_unbind),
};

static int __init printer_acm_init(void)
{
	return usb_composite_probe(&printer_acm_driver);
}
late_initcall(printer_acm_init);

static void __exit printer_acm_cleanup(void)
{
	usb_composite_unregister(&printer_acm_driver);
}
module_exit(printer_acm_cleanup);

MODULE_DESCRIPTION(DRIVER_DESC);
MODULE_AUTHOR("xinshuan");
MODULE_LICENSE("GPL");
