# 确保.config.in 产生
make_sure_update_config_in := $(shell tools/check_config.sh $(config_in) 2> /dev/null)

 # 导入当前的配置
sinclude .config.in

export packages
export package_hooks
export package_module_srcs
export package_module_clean
export package_init_hooks
export package_make_hooks
export package_finalize_hooks
export package_clean_hooks
export package_module_src_dirs
export TARGET_FINALIZE_HOOKS
export TARGET_INSTALL_HOOKS
export TARGET_INSTALL_CLEAN_HOOKS
export Q

# 文件目录
export TOPDIR := $(shell pwd)/

# 一些有用的自定义函数
include tools/config.mk

# make V=1 可以看见Makefile运转的细节
ifneq ($(V),1)
Q = @
SLIENT_ARG = --no-print-directory
else
Q =
SLIENT_ARG = V=1
endif

# 如果当前的目标是 xxx_defconfig 那么不打印inlcude 信息

no_including_info_ += $(filter %_defconfig, $(MAKECMDGOALS))
no_including_info_ += $(filter install%, $(MAKECMDGOALS))
no_including_info_ += $(filter clean%, $(MAKECMDGOALS))
ifneq ($(strip $(no_including_info_)),)
no_including_info = y
endif

# 包含此软件包定义的一些变量,函数
include package/target_config.mk

# 默认目标
default:all

# 包含所有 package
package := package/package.mk
include tools/include_package.mk

all: module_srcs make_hooks finalize_hooks target_default target_final

clean: clean_hooks target_clean

clean_hooks: $(package_module_clean) $(package_clean_hooks)

$(package_init_hooks):
init_hooks: $(package_init_hooks)

$(package_module_srcs): init_hooks
module_srcs: $(package_module_srcs)

$(package_make_hooks): module_srcs
make_hooks: $(package_make_hooks)

$(package_finalize_hooks): make_hooks target_default
finalize_hooks: $(package_finalize_hooks)
	$(foreach hook,$(TARGET_FINALIZE_HOOKS),$($(hook))$(CMD_SEPARATOR))

install:
	$(if $(FS_TARGET_DIR),,$(error must set FS_TARGET_DIR))
	$(Q)mkdir -p $(FS_TARGET_DIR)/module_driver/
	$(Q)cp -rf output/* $(FS_TARGET_DIR)/module_driver/
	$(Q)rm -f $(FS_TARGET_DIR)/etc/init.d/S*module_driver*
	$(Q)cp $(MD_INIT_START_FILE) $(FS_TARGET_DIR)/etc/init.d/
	$(foreach hook,$(TARGET_INSTALL_HOOKS),$($(hook))$(CMD_SEPARATOR))

clean_install:
	$(if $(FS_TARGET_DIR),,$(error must set FS_TARGET_DIR))
	$(Q)rm -rf $(FS_TARGET_DIR)/module_driver/
	$(Q)rm -f $(FS_TARGET_DIR)/etc/init.d/S*module_driver*
	$(foreach hook,$(TARGET_INSTALL_CLEAN_HOOKS),$($(hook))$(CMD_SEPARATOR))

# 包含此软件包定义的一些规则
include package/target_rule.mk

.PHONY: default all clean clean_hooks init_hooks module_srcs make_hooks finalize_hooks install clean_install

