include $(DRIVERS_DIR)/tools/common_module.mk

ccflags-y += -I$(SOC_DIR)/spdif

MODULE_NAME := x2000_510_spdif_sound_card

module-y += x2000_510_spdif_snd_card.o

obj-m = $(MODULE_NAME).o
$(MODULE_NAME)-y = $(module-y)
