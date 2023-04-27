target_default:module_srcs
	@echo fix warning > /dev/null

target_final:finalize_hooks
	$(Q)+make $(SLIENT_ARG) -f tools/gen_driver_default_init_script.mk init_order_file=$(MD_INIT_ORDER_FILE)

# 清除所有的 bultin.o .d .d
target_clean:clean_hooks
	@rm -rf output/*

# 生成 configs/目录下 *_defconfig 的规则
###########################################################
defconfigs := $(shell ls configs/ 2> /dev/null )

defconfigs := $(filter %_defconfig, $(defconfigs))

ifneq ($strip($(defconfigs)),)

$(defconfigs):
	$(Q)echo "..writing .config.in"
	$(Q)tools/configparser --input Config.in configs/$(@) --defconfig .config.in --header include/config.h > /dev/null
	$(Q)echo "..writing include/config.h"

.PONHY: $(defconfigs)

endif
###########################################################

# 确保output 目录存在
make_sure_output_exist:=$(shell mkdir -p output/)
