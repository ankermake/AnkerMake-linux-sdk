hook_value := $($(hook_name))

ifneq ($(strip $(hook_value)),)
ifneq ($(filter $(hook_value), $(package_hooks)),)
$(error error: [$(package)] [$(hook_name)] may use a redefined hook: [$(hook_value)])
endif
package_hooks += $(hook_value)
endif
