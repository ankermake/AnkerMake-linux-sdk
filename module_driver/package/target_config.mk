export DRIVERS_DIR:=$(shell pwd)

# 编译 module
define MK_MODULE_O
	@echo ..building $1
	$(Q)+make -j9 $(SLIENT_ARG) -C $1
	@echo
endef

# 清除 module
define MK_MODULE_CLEAN
	@echo ..clean $1
	$(Q)+make $(SLIENT_ARG) -C $1 clean
endef
