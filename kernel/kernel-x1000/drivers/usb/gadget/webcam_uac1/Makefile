#
# USB peripheral controller drivers
#

usb_f_uvc-y			:= f_uvc.o uvc_queue.o uvc_v4l2.o uvc_video.o
obj-$(CONFIG_USB_WEBCAM_AUDIO) += usb_f_uvc.o

usb_f_uac1-y			:= f_uac1.o u_audio.o
obj-$(CONFIG_USB_WEBCAM_AUDIO) += usb_f_uac1.o

g_webcam_audio-y			:= webcam_uac1.o
obj-$(CONFIG_USB_WEBCAM_AUDIO) += g_webcam_audio.o


