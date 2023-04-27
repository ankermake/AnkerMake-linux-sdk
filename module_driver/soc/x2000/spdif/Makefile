include $(DRIVERS_DIR)/tools/common_module.mk

MODULE_NAME := soc_spdif

module-y += spdif.o
module-y += spdif_dma.o

KBUILD_EXTRA_SYMBOLS := $(DRIVERS_DIR)/drivers/utils/Module.symvers \
				 $(SOC_DIR)/audio_dma/Module.symvers

obj-m = $(MODULE_NAME).o
$(MODULE_NAME)-y = $(module-y)
