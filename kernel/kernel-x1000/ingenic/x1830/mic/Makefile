
obj-y += mic.o
obj-y += mic_ops.o
obj-y += mic_dma.o
obj-y += mic_hrtimer.o

obj-y += amic.o
obj-y += dmic_hal.o

obj-y += x1800_codec.o
obj-y += i2s.o
obj-y += dma_ops.o

obj-$(CONFIG_X1800_MIC_SND_INTERNAL_CODEC) += snd_internal_codec.o 
obj-$(CONFIG_X1800_MIC_SND_INTERNAL_CODEC) += snd_internal_codec_volume.o 
obj-$(CONFIG_X1800_MIC_SND_INTERNAL_CODEC) += snd_hrtimer.o

dir = ingenic/x1830/mic/
make_sure_x01_lib_is_here := $(shell cp $(dir)/mic_lib.txt $(dir)/mic_lib.o)
obj-y += mic_lib.o
