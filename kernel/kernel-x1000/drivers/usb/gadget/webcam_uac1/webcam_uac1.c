// SPDX-License-Identifier: GPL-2.0+
/*
 *	webcam.c -- USB webcam gadget driver
 *
 *	Copyright (C) 2009-2010
 *	    Laurent Pinchart (laurent.pinchart@ideasonboard.com)
 */

#include <linux/kernel.h>
#include <linux/device.h>
#include <linux/module.h>
#include <linux/usb/video.h>
#include <linux/usb/composite.h>

#include "u_uvc.h"
#include "u_uac1.h"

USB_GADGET_COMPOSITE_OPTIONS();

/*-------------------------------------------------------------------------*/

/* module parameters specific to the Video streaming endpoint */
static unsigned int streaming_interval = 1;
module_param(streaming_interval, uint, S_IRUGO|S_IWUSR);
MODULE_PARM_DESC(streaming_interval, "1 - 16");

static unsigned int streaming_maxpacket = 3072;
module_param(streaming_maxpacket, uint, S_IRUGO|S_IWUSR);
MODULE_PARM_DESC(streaming_maxpacket, "1 - 1023 (FS), 1 - 3072 (hs/ss)");

static unsigned int streaming_maxburst;
module_param(streaming_maxburst, uint, S_IRUGO|S_IWUSR);
MODULE_PARM_DESC(streaming_maxburst, "0 - 15 (ss only)");

/* Playback(USB-IN) Default Stereo - Fl/Fr */
static int p_chmask = UAC1_DEF_PCHMASK;
module_param(p_chmask, uint, S_IRUGO);
MODULE_PARM_DESC(p_chmask, "Playback Channel Mask");

/* Playback Default 48 KHz */
static int p_srate = UAC1_DEF_PSRATE;
module_param(p_srate, uint, S_IRUGO);
MODULE_PARM_DESC(p_srate, "Playback Sampling Rate");

/* Playback Default 16bits/sample */
static int p_ssize = UAC1_DEF_PSSIZE;
module_param(p_ssize, uint, S_IRUGO);
MODULE_PARM_DESC(p_ssize, "Playback Sample Size(bytes)");

/* Capture(USB-OUT) Default Stereo - Fl/Fr */
static int c_chmask = UAC1_DEF_CCHMASK;
module_param(c_chmask, uint, S_IRUGO);
MODULE_PARM_DESC(c_chmask, "Capture Channel Mask");

/* Capture Default 48 KHz */
static int c_srate = UAC1_DEF_CSRATE;
module_param(c_srate, uint, S_IRUGO);
MODULE_PARM_DESC(c_srate, "Capture Sampling Rate");

/* Capture Default 16bits/sample */
static int c_ssize = UAC1_DEF_CSSIZE;
module_param(c_ssize, uint, S_IRUGO);
MODULE_PARM_DESC(c_ssize, "Capture Sample Size(bytes)");

/* --------------------------------------------------------------------------
 * Device descriptor
 */

#define WEBCAM_VENDOR_ID		0x1d6b	/* Linux Foundation */
#define WEBCAM_PRODUCT_ID		0x0102	/* Webcam A/V gadget */

/* string IDs are assigned dynamically */
#define STRING_DESCRIPTION_IDX		USB_GADGET_FIRST_AVAIL_IDX

static struct usb_string strings_dev[] = {
	[USB_GADGET_MANUFACTURER_IDX].s = "INGENIC",
	[USB_GADGET_PRODUCT_IDX].s = "Webcam Audio gadget",
	[USB_GADGET_SERIAL_IDX].s = "1.0",
	[STRING_DESCRIPTION_IDX].s = "Video and Audio",
	{  }
};

static struct usb_gadget_strings stringtab_dev = {
	.language = 0x0409,	/* en-us */
	.strings = strings_dev,
};

static struct usb_gadget_strings *webcam_audio_strings[] = {
	&stringtab_dev,
	NULL,
};

static struct usb_function_instance *fi_uvc;
static struct usb_function *f_uvc;

static struct usb_function_instance *fi_uac1;
static struct usb_function *f_uac1;

static struct usb_device_descriptor device_desc = {
	.bLength		= USB_DT_DEVICE_SIZE,
	.bDescriptorType	= USB_DT_DEVICE,
	.bcdUSB			= cpu_to_le16(0x0200),
	.bDeviceClass		= USB_CLASS_MISC,
	.bDeviceSubClass	= 0x02,
	.bDeviceProtocol	= 0x01,
	.bMaxPacketSize0	= 0, /* dynamic */
	.idVendor		= cpu_to_le16(WEBCAM_VENDOR_ID),
	.idProduct		= cpu_to_le16(WEBCAM_PRODUCT_ID),
	.iManufacturer		= 0, /* dynamic */
	.iProduct		= 0, /* dynamic */
	.iSerialNumber		= 0, /* dynamic */
	.bNumConfigurations	= 0, /* dynamic */
};

DECLARE_UVC_HEADER_DESCRIPTOR(1);

static const struct UVC_HEADER_DESCRIPTOR(1) uvc_control_header = {
	.bLength		= UVC_DT_HEADER_SIZE(1),
	.bDescriptorType	= USB_DT_CS_INTERFACE,
	.bDescriptorSubType	= UVC_VC_HEADER,
	.bcdUVC			= cpu_to_le16(0x0110),
	.wTotalLength		= 0, /* dynamic */
	.dwClockFrequency	= cpu_to_le32(48000000),
	.bInCollection		= 0, /* dynamic */
	.baInterfaceNr[0]	= 0, /* dynamic */
};

static const struct uvc_camera_terminal_descriptor uvc_camera_terminal = {
	.bLength		= UVC_DT_CAMERA_TERMINAL_SIZE(3),
	.bDescriptorType	= USB_DT_CS_INTERFACE,
	.bDescriptorSubType	= UVC_VC_INPUT_TERMINAL,
	.bTerminalID		= 1,
	.wTerminalType		= cpu_to_le16(0x0201),
	.bAssocTerminal		= 0,
	.iTerminal		= 0,
	.wObjectiveFocalLengthMin	= cpu_to_le16(0),
	.wObjectiveFocalLengthMax	= cpu_to_le16(0),
	.wOcularFocalLength		= cpu_to_le16(0),
	.bControlSize		= 3,
	.bmControls[0]		= 0,
	.bmControls[1]		= 0,
	.bmControls[2]		= 0,
};

static const struct uvc_new_processing_unit_descriptor uvc_processing = {
	.bLength		= UVC_DT_NEW_PROCESSING_UNIT_SIZE,
	.bDescriptorType	= USB_DT_CS_INTERFACE,
	.bDescriptorSubType	= UVC_VC_PROCESSING_UNIT,
	.bUnitID		= 2,
	.bSourceID		= 1,
	.wMaxMultiplier		= 0,
	.bControlSize		= 3,
	.bmControls[0]		= 0,
	.bmControls[1]		= 0,
	.bmControls[2]		= 0,
	.iProcessing		= 0,
	.bmVideoStandards	= 0,
};

static const struct uvc_output_terminal_descriptor uvc_output_terminal = {
	.bLength		= UVC_DT_OUTPUT_TERMINAL_SIZE,
	.bDescriptorType	= USB_DT_CS_INTERFACE,
	.bDescriptorSubType	= UVC_VC_OUTPUT_TERMINAL,
	.bTerminalID		= 3,
	.wTerminalType		= cpu_to_le16(0x0101),
	.bAssocTerminal		= 0,
	.bSourceID		= 2,
	.iTerminal		= 0,
};

DECLARE_UVC_INPUT_HEADER_DESCRIPTOR(1, 1);

static const struct UVC_INPUT_HEADER_DESCRIPTOR(1, 1) uvc_input_header = {
	.bLength		= UVC_DT_INPUT_HEADER_SIZE(1, 1),
	.bDescriptorType	= USB_DT_CS_INTERFACE,
	.bDescriptorSubType	= UVC_VS_INPUT_HEADER,
	.bNumFormats		= 1,
	.wTotalLength		= 0, /* dynamic */
	.bEndpointAddress	= 0, /* dynamic */
	.bmInfo			= 0,
	.bTerminalLink		= 3,
	.bStillCaptureMethod	= 0,
	.bTriggerSupport	= 0,
	.bTriggerUsage		= 0,
	.bControlSize		= 1,
	.bmaControls[0][0]	= 0,
};

static const struct uvc_format_uncompressed uvc_format_yuv = {
	.bLength		= UVC_DT_FORMAT_UNCOMPRESSED_SIZE,
	.bDescriptorType	= USB_DT_CS_INTERFACE,
	.bDescriptorSubType	= UVC_VS_FORMAT_UNCOMPRESSED,
	.bFormatIndex		= 1,
	.bNumFrameDescriptors	= 1,
	.guidFormat		=
		{ 'Y',  'U',  'Y',  '2', 0x00, 0x00, 0x10, 0x00,
		 0x80, 0x00, 0x00, 0xaa, 0x00, 0x38, 0x9b, 0x71},
	.bBitsPerPixel		= 16,
	.bDefaultFrameIndex	= 1,
	.bAspectRatioX		= 0,
	.bAspectRatioY		= 0,
	.bmInterfaceFlags	= 0,
	.bCopyProtect		= 0,
};

DECLARE_UVC_FRAME_UNCOMPRESSED(1);

static const struct UVC_FRAME_UNCOMPRESSED(1) uvc_frame_yuv_720p = {
	.bLength		= UVC_DT_FRAME_UNCOMPRESSED_SIZE(1),
	.bDescriptorType	= USB_DT_CS_INTERFACE,
	.bDescriptorSubType	= UVC_VS_FRAME_UNCOMPRESSED,
	.bFrameIndex		= 1,
	.bmCapabilities		= 0,
	.wWidth			= cpu_to_le16(1280),
	.wHeight		= cpu_to_le16(720),
	.dwMinBitRate		= cpu_to_le32(18432000),
	.dwMaxBitRate		= cpu_to_le32(18432000),
	.dwMaxVideoFrameBufferSize	= cpu_to_le32(1843200),
	.dwDefaultFrameInterval	= cpu_to_le32(1000000),
	.bFrameIntervalType	= 1,
	.dwFrameInterval[0]	= cpu_to_le32(1000000),
};

static const struct uvc_color_matching_descriptor uvc_color_matching = {
	.bLength		= UVC_DT_COLOR_MATCHING_SIZE,
	.bDescriptorType	= USB_DT_CS_INTERFACE,
	.bDescriptorSubType	= UVC_VS_COLORFORMAT,
	.bColorPrimaries	= 1,
	.bTransferCharacteristics	= 1,
	.bMatrixCoefficients	= 4,
};

static const struct uvc_descriptor_header * const uvc_fs_control_cls[] = {
	(const struct uvc_descriptor_header *) &uvc_control_header,
	(const struct uvc_descriptor_header *) &uvc_camera_terminal,
	(const struct uvc_descriptor_header *) &uvc_processing,
	(const struct uvc_descriptor_header *) &uvc_output_terminal,
	NULL,
};

static const struct uvc_descriptor_header * const uvc_ss_control_cls[] = {
	(const struct uvc_descriptor_header *) &uvc_control_header,
	(const struct uvc_descriptor_header *) &uvc_camera_terminal,
	(const struct uvc_descriptor_header *) &uvc_processing,
	(const struct uvc_descriptor_header *) &uvc_output_terminal,
	NULL,
};

static const struct uvc_descriptor_header * const uvc_fs_streaming_cls[] = {
	(const struct uvc_descriptor_header *) &uvc_input_header,
	(const struct uvc_descriptor_header *) &uvc_format_yuv,
	(const struct uvc_descriptor_header *) &uvc_frame_yuv_720p,
	(const struct uvc_descriptor_header *) &uvc_color_matching,
	NULL,
};

static const struct uvc_descriptor_header * const uvc_hs_streaming_cls[] = {
	(const struct uvc_descriptor_header *) &uvc_input_header,
	(const struct uvc_descriptor_header *) &uvc_format_yuv,
	(const struct uvc_descriptor_header *) &uvc_frame_yuv_720p,
	(const struct uvc_descriptor_header *) &uvc_color_matching,
	NULL,
};

static const struct uvc_descriptor_header * const uvc_ss_streaming_cls[] = {
	(const struct uvc_descriptor_header *) &uvc_input_header,
	(const struct uvc_descriptor_header *) &uvc_format_yuv,
	(const struct uvc_descriptor_header *) &uvc_frame_yuv_720p,
	(const struct uvc_descriptor_header *) &uvc_color_matching,
	NULL,
};

/* --------------------------------------------------------------------------
 * USB configuration
 */

static int
webcam_audio_config_bind(struct usb_configuration *c)
{
	int status = 0;

	f_uvc = usb_get_function(fi_uvc);
	if (IS_ERR(f_uvc))
		return PTR_ERR(f_uvc);

	status = usb_add_function(c, f_uvc);
	if (status < 0) {
		usb_put_function(f_uvc);
		return status;
	}

	f_uac1 = usb_get_function(fi_uac1);
	if (IS_ERR(f_uac1)) {
		status = PTR_ERR(f_uac1);
		usb_remove_function(c, f_uvc);
		usb_put_function(f_uvc);
		return status;
	}

	status = usb_add_function(c, f_uac1);
	if (status < 0) {
		usb_put_function(f_uac1);
		usb_remove_function(c, f_uvc);
		usb_put_function(f_uvc);
		return status;
	}

	return status;
}

static struct usb_configuration webcam_audio_config_driver = {
	.label			= "Video and Audio",
	.bConfigurationValue	= 1,
	.iConfiguration		= 0, /* dynamic */
	.bmAttributes		= USB_CONFIG_ATT_SELFPOWER,
	.MaxPower		= CONFIG_USB_GADGET_VBUS_DRAW,
};

static int
webcam_audio_unbind(struct usb_composite_dev *cdev)
{
	if (!IS_ERR_OR_NULL(f_uac1))
		usb_put_function(f_uac1);
	if (!IS_ERR_OR_NULL(fi_uac1))
		usb_put_function_instance(fi_uac1);

	if (!IS_ERR_OR_NULL(f_uvc))
		usb_put_function(f_uvc);
	if (!IS_ERR_OR_NULL(fi_uvc))
		usb_put_function_instance(fi_uvc);

	return 0;
}

static int
webcam_audio_bind(struct usb_composite_dev *cdev)
{
	struct f_uac1_opts	*uac1_opts;
	struct f_uvc_opts *uvc_opts;
	int ret;

	fi_uvc = usb_get_function_instance("uvc");
	if (IS_ERR(fi_uvc))
		return PTR_ERR(fi_uvc);

	uvc_opts = container_of(fi_uvc, struct f_uvc_opts, func_inst);

	uvc_opts->streaming_interval = streaming_interval;
	uvc_opts->streaming_maxpacket = streaming_maxpacket;
	uvc_opts->streaming_maxburst = streaming_maxburst;

	uvc_opts->fs_control = uvc_fs_control_cls;
	uvc_opts->ss_control = uvc_ss_control_cls;
	uvc_opts->fs_streaming = uvc_fs_streaming_cls;
	uvc_opts->hs_streaming = uvc_hs_streaming_cls;
	uvc_opts->ss_streaming = uvc_ss_streaming_cls;

	fi_uac1 = usb_get_function_instance("uac1");
	if (IS_ERR(fi_uac1)) {
		ret = PTR_ERR(fi_uac1);
		goto fail_fi_uvc;
	}

	uac1_opts = container_of(fi_uac1, struct f_uac1_opts, func_inst);
	uac1_opts->p_chmask = p_chmask;
	uac1_opts->p_srate = p_srate;
	uac1_opts->p_ssize = p_ssize;
	uac1_opts->c_chmask = c_chmask;
	uac1_opts->c_srate = c_srate;
	uac1_opts->c_ssize = c_ssize;
	uac1_opts->req_number = UAC1_DEF_REQ_NUM;

	/* Allocate string descriptor numbers ... note that string contents
	 * can be overridden by the composite_dev glue.
	 */
	ret = usb_string_ids_tab(cdev, strings_dev);
	if (ret < 0)
		goto fail_fi_uac1;

	device_desc.iManufacturer =
		strings_dev[USB_GADGET_MANUFACTURER_IDX].id;
	device_desc.iProduct =
		strings_dev[USB_GADGET_PRODUCT_IDX].id;
	device_desc.iSerialNumber =
		strings_dev[USB_GADGET_SERIAL_IDX].id;
	webcam_audio_config_driver.iConfiguration =
		strings_dev[STRING_DESCRIPTION_IDX].id;

	/* Register our configuration. */
	if ((ret = usb_add_config(cdev, &webcam_audio_config_driver,
					webcam_audio_config_bind)) < 0)
		goto fail_fi_uac1;

	usb_composite_overwrite_options(cdev, &coverwrite);
	INFO(cdev, "Webcam Video Gadget\n");
	return 0;

fail_fi_uac1:
	usb_put_function_instance(fi_uac1);
fail_fi_uvc:
	usb_put_function_instance(fi_uvc);
	return ret;
}

/* --------------------------------------------------------------------------
 * Driver
 */

static struct usb_composite_driver webcam_audio_driver = {
	.name		= "g_webcam_audio",
	.dev		= &device_desc,
	.strings	= webcam_audio_strings,
	.max_speed	= USB_SPEED_HIGH,
	.bind		= webcam_audio_bind,
	.unbind		= webcam_audio_unbind,
};

static int __init webcam_audio_init(void)
{
	return usb_composite_probe(&webcam_audio_driver);
}

static void __exit webcam_audio_cleanup(void)
{
	usb_composite_unregister(&webcam_audio_driver);
}

module_init(webcam_audio_init);
module_exit(webcam_audio_cleanup);

MODULE_AUTHOR("INGENIC");
MODULE_DESCRIPTION("Webcam Video And Audio Gadget");
MODULE_LICENSE("GPL");