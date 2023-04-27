
#-------------------------------------------------------
package_name = i2c_gpio
package_depends = utils
package_module_src = drivers/i2c_gpio/
package_make_hook =
package_init_hook =
package_finalize_hook = i2c_gpio_finalize_hook
package_clean_hook =
#-------------------------------------------------------

i2c_gpio_init_file = output/i2c_gpio_add.sh

define i2c_gpio_cmd
	$(Q)echo -n 'echo '                                >> $(i2c_gpio_init_file)
	$(Q)echo -n 'bus_num=$($(1)_BUS_NUM) '             >> $(i2c_gpio_init_file)
	$(Q)echo -n 'rate=$($(1)_RATE) '                   >> $(i2c_gpio_init_file)
	$(Q)echo -n 'scl=$($(1)_SCL) '                     >> $(i2c_gpio_init_file)
	$(Q)echo -n 'sda=$($(1)_SDA) '                     >> $(i2c_gpio_init_file)
	$(Q)echo ' > i2c_bus'                              >> $(i2c_gpio_init_file)
endef

do_i2c_gpio_cmd = $(if $($(strip $1)), $(call i2c_gpio_cmd,$(strip $1)))

define i2c_gpio_finalize_hook
	$(Q)cp drivers/i2c_gpio/i2c_gpio_add.ko output/
	$(Q)echo "insmod i2c_gpio_add.ko"                  > $(i2c_gpio_init_file)
	$(Q)echo "cd /sys/module/i2c_gpio_add/parameters/" >> $(i2c_gpio_init_file)
	$(call do_i2c_gpio_cmd, MD_I2C_GPIO0)
	$(call do_i2c_gpio_cmd, MD_I2C_GPIO1)
	$(call do_i2c_gpio_cmd, MD_I2C_GPIO2)
	$(call do_i2c_gpio_cmd, MD_I2C_GPIO3)
	$(call do_i2c_gpio_cmd, MD_I2C_GPIO4)
	$(call do_i2c_gpio_cmd, MD_I2C_GPIO5)
	$(call do_i2c_gpio_cmd, MD_I2C_GPIO6)
	$(call do_i2c_gpio_cmd, MD_I2C_GPIO7)
endef
