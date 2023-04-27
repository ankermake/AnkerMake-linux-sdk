
obj-$(CONFIG_JZ_MICCHAR_X1830_CODEC) += x1830_codec.o
obj-$(CONFIG_JZ_MICHAR_I2S)	+= i2s.o
obj-y += amic_hal.o dmic_hal.o mic_hrtimer.o mic_ops.o dma.o mic.o
obj-$(CONFIG_JZ_MICCHAR_WITH_SND_I2S) += snd.o snd_hrtimer.o

dir = ingenic/drivers/mic
make_sure_x01_lib_is_here := $(shell cp $(dir)/mic_lib.txt $(dir)/mic_lib.o)
obj-y += mic_lib.o
