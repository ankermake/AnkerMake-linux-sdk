
ifeq ($(DRIVERS_DIR),)
$(error you must set DRIVERS_DIR !)
endif

include $(DRIVERS_DIR)/.config.in

SOC_DIR := $(DRIVERS_DIR)/$(MD_SOC_DIR)

all: modules

modules:
	@+$(MAKE) -C $(KERNEL_DIR) M=$(shell pwd) modules

clean:
	@find . -name '*.o' -o -name '.depend' -o -name '.*.cmd' \
	-o -name '*.mod.c' -o -name '.tmp_versions' -o -name '*.ko' \
	-o -name '*.symvers' -o -name 'modules.order' | xargs rm -rf

.PHONY: all modules clean

ccflags-y += -I$(DRIVERS_DIR) -I$(DRIVERS_DIR)/include/
ccflags-y += -I$(SOC_DIR)/include/
ccflags-y += -include $(DRIVERS_DIR)/config.h

ccflags-y += -Wno-declaration-after-statement -Werror
