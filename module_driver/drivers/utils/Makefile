include $(DRIVERS_DIR)/tools/common_module.mk

MODULE_NAME := utils
module-y += assert.o
module-y += utils_license.o
module-y += pmu_parse.o
module-$(MD_UTILS_LOG) += utils_log.o
module-$(MD_UTILS_GPIO) += gpio.o
module-$(MD_UTILS_I2C) += i2c.o
module-$(MD_UTILS_SPI) += spi.o
module-$(MD_UTILS_CLOCK) += clock.o
module-$(MD_UTILS_STRING) += string.o
module-$(MD_UTILS_STRING) += str_to_words.o
module-$(MD_UTILS_STRING) += str_to_long.o
module-$(MD_UTILS_RING_MEM) += ring_mem.o

obj-m = $(MODULE_NAME).o
$(MODULE_NAME)-y = $(module-y)
