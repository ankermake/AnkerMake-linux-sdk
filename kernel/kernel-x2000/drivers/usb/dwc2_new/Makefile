# SPDX-License-Identifier: GPL-2.0
ccflags-$(CONFIG_USB_DWC2_NEW_DEBUG)	+= -DDEBUG
ccflags-$(CONFIG_USB_DWC2_NEW_VERBOSE)	+= -DVERBOSE_DEBUG

obj-$(CONFIG_USB_DWC2_NEW)			+= dwc2.o
dwc2-y					:= core.o core_intr.o platform.o drd.o
dwc2-y					+= params.o

ifneq ($(filter y,$(CONFIG_USB_DWC2_NEW_HOST) $(CONFIG_USB_DWC2_NEW_DUAL_ROLE)),)
	dwc2-y				+= hcd.o hcd_intr.o
	dwc2-y				+= hcd_queue.o hcd_ddma.o
endif

ifneq ($(filter y,$(CONFIG_USB_DWC2_NEW_PERIPHERAL) $(CONFIG_USB_DWC2_NEW_DUAL_ROLE)),)
	dwc2-y       			+= gadget.o
endif

