include $(DRIVERS_DIR)/tools/common_module.mk

MODULE_NAME := x1600_dummy_sound_card

module-y += x1600_dummy_snd_card.o

KBUILD_EXTRA_SYMBOLS := $(DRIVERS_DIR)/drivers/utils/Module.symvers

obj-m = $(MODULE_NAME).o
$(MODULE_NAME)-y = $(module-y)
