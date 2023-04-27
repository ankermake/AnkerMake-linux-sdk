#
# 递归包含 package-y 中定义的 .mk 文件
#

# 保存当前的 package-y 变量
package-old := $(package-y)

# 包含单个 package
ifeq ($(no_including_info),)
$(info ..include $(package))
endif
include $(package)

# 检查是否定义了 package_name
# 如果定义了 package_name, 那么去解析其它的选项
ifneq ($(strip $(package_name)),)

ifneq ($(filter $(package_name), $(packages)),)
$(error error: [$(package)] [$($(package_name)_package_mk)] has same package_name: [$(package_name)])
endif

hook_name := package_make_hook
include tools/check_package_hooks.mk

hook_name := package_clean_hook
include tools/check_package_hooks.mk

hook_name := package_init_hook
include tools/check_package_hooks.mk

hook_name := package_finalize_hook
include tools/check_package_hooks.mk

packages += $(package_name)

$(package_name)_package_mk := $(package)
$(package_name)_package_name := $(package_name)
$(package_name)_package_module_src := $(package_module_src)
$(package_name)_package_CFLAGS := $(package_CFLAGS)
$(package_name)_package_clean_hook := $(package_clean_hook)
$(package_name)_package_make_hook := $(package_make_hook)
$(package_name)_package_init_hook := $(package_init_hook)
$(package_name)_package_finalize_hook := $(package_finalize_hook)
$(package_name)_package_depends := $(package_depends)

package_module_srcs += $(package_name)_package_module_src
package_module_src_dirs += $(package_module_src)
package_module_clean += $(package_name)_package_module_clean
package_init_hooks += $(package_name)_package_init_hook
package_make_hooks += $(package_name)_package_make_hook
package_finalize_hooks += $(package_name)_package_finalize_hook
package_clean_hooks += $(package_name)_package_clean_hook

# module_srcs
$(package_name)_package_module_src: $(patsubst %,%_package_module_src, $(package_depends))
ifneq ($(strip $(package_module_src)),)
	$(call MK_MODULE_O, $($@))
endif

# module_clean
$(package_name)_package_module_clean: $(patsubst %,%_package_module_clean, $(package_depends))
ifneq ($(strip $(package_module_src)),)
	$(call MK_MODULE_CLEAN, $($(patsubst %_clean,%_src,$@)))
endif

# init_hooks
$(package_name)_package_init_hook: $(patsubst %,%_package_init_hook, $(package_depends))
ifneq ($(strip $(package_init_hook)),)
	$($($@))
endif

# make_hooks
$(package_name)_package_make_hook: $(patsubst %,%_package_make_hook, $(package_depends))
ifneq ($(strip $(package_make_hook)),)
	$($($@))
endif

# finalize_hooks
$(package_name)_package_finalize_hook: $(patsubst %,%_package_finalize_hook, $(package_depends))
ifneq ($(strip $(package_finalize_hook)),)
	$($($@))
endif

# clean_hooks
$(package_name)_package_clean_hook: $(patsubst %,%_package_clean_hook, $(package_depends))
ifneq ($(strip $(package_clean_hook)),)
	$($($@))
endif

endif # end if package_name

package_name =
package_module_src =
package_clean_hook =
package_make_hook =
package_init_hook =
package_finalize_hook =
package_depends =

# 继续包含 package-y 变量增加的内容
$(foreach file,$(filter-out $(package-old),$(package-y)),$(eval package:=$(file))$(eval include tools/include_package.mk))
