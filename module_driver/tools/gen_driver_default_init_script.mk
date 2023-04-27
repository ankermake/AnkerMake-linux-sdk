include tools/config.mk
include $(init_order_file)

init_file = output/driver_default_init_script.sh

# 列出现有的初始化脚本
files := $(shell ls output/*.sh)
files := $(filter-out $(init_file), $(files))
files := $(patsubst output/%,%, $(files))

drivers := $(filter $(files), $(driver_init_order))
drivers += $(filter-out $(driver_init_order), $(files))

define write_to_file
	$(Q)echo "sh $1" >> $(init_file)
endef

all:
	$(Q)echo "#!/bin/sh" > $(init_file)
	$(Q)echo "" >> $(init_file)
	$(Q)chmod +x $(init_file)
	$(foreach init,$(drivers),$(call write_to_file,$(init))$(CMD_SEPARATOR))
