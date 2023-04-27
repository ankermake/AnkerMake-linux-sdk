include $(DRIVERS_DIR)/tools/common_module.mk

MODULE_NAME := soc_dmic

module-y += dmic_snd.o
module-y += dmic_card.o

KBUILD_EXTRA_SYMBOLS := $(DRIVERS_DIR)/drivers/utils/Module.symvers \
				 $(SOC_DIR)/audio_dma/Module.symvers

obj-m = $(MODULE_NAME).o
$(MODULE_NAME)-y = $(module-y)
