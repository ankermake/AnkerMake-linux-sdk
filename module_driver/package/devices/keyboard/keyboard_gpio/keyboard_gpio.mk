
#-------------------------------------------------------
package_name = keyboard_gpio
package_depends = utils
package_module_src = devices/keyboard_gpio/
package_make_hook =
package_init_hook =
package_finalize_hook = keyboard_gpio_finalize_hook
package_clean_hook =
#-------------------------------------------------------

keyboard_gpio_init_file = output/keyboard_gpio_add.sh

define keyboard_gpio_cmd
	$(Q)echo -n 'echo ' >> $(keyboard_gpio_init_file)
	$(Q)echo -n 'gpio="$($(1)_GPIO)" ' >> $(keyboard_gpio_init_file)
	$(Q)echo -n 'key_code=$(shell tools/key_to_code.sh $($(1)_CODE)) ' >> $(keyboard_gpio_init_file)
	$(Q)echo -n 'tag="$($(1)_CODE)" ' >> $(keyboard_gpio_init_file)
	$(Q)echo -n 'active_level=$($(1)_ACTIVE_LEVEL) ' >> $(keyboard_gpio_init_file)
	$(Q)echo -n 'wakeup=$($(1)_WAKEUP) ' >> $(keyboard_gpio_init_file)
	$(Q)echo ' > keyboard' >> $(keyboard_gpio_init_file)
endef

do_keyboard_gpio_def = $(if $($(strip $1)), $(call keyboard_gpio_def,$(strip $1)))
do_keyboard_gpio_cmd = $(if $($(strip $1)), $(call keyboard_gpio_cmd,$(strip $1)))

define keyboard_gpio_finalize_hook
	$(Q)cp devices/keyboard_gpio/keyboard_gpio_add.ko output/
	$(Q)echo "insmod keyboard_gpio_add.ko" > $(keyboard_gpio_init_file)
	$(Q)echo 'cd /sys/module/keyboard_gpio_add/parameters/ ' >> $(keyboard_gpio_init_file)
	$(Q)echo 'echo alloc=16 > keyboard' >> $(keyboard_gpio_init_file)
	$(call do_keyboard_gpio_cmd, MD_KEYBOARD_GPIO0)
	$(call do_keyboard_gpio_cmd, MD_KEYBOARD_GPIO1)
	$(call do_keyboard_gpio_cmd, MD_KEYBOARD_GPIO2)
	$(call do_keyboard_gpio_cmd, MD_KEYBOARD_GPIO3)
	$(call do_keyboard_gpio_cmd, MD_KEYBOARD_GPIO4)
	$(call do_keyboard_gpio_cmd, MD_KEYBOARD_GPIO5)
	$(call do_keyboard_gpio_cmd, MD_KEYBOARD_GPIO6)
	$(call do_keyboard_gpio_cmd, MD_KEYBOARD_GPIO7)
	$(call do_keyboard_gpio_cmd, MD_KEYBOARD_GPIO8)
	$(call do_keyboard_gpio_cmd, MD_KEYBOARD_GPIO9)
	$(call do_keyboard_gpio_cmd, MD_KEYBOARD_GPIO10)
	$(call do_keyboard_gpio_cmd, MD_KEYBOARD_GPIO11)
	$(call do_keyboard_gpio_cmd, MD_KEYBOARD_GPIO12)
	$(call do_keyboard_gpio_cmd, MD_KEYBOARD_GPIO13)
	$(call do_keyboard_gpio_cmd, MD_KEYBOARD_GPIO14)
	$(call do_keyboard_gpio_cmd, MD_KEYBOARD_GPIO15)
	$(Q)echo 'echo register > keyboard' >> $(keyboard_gpio_init_file)
endef
