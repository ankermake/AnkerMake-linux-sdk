#-------------------------------------------------------
package_name = gpio_regulator
package_depends = utils
package_module_src = devices/regulator/gpio_regulator/
package_make_hook =
package_init_hook =
package_finalize_hook = gpio_regulator_finalize_hook
package_clean_hook =
#-------------------------------------------------------

gpio_regulator_init_file = output/gpio_regulator.sh

define gpio_regulator_finalize_hook
	$(Q)cp devices/regulator/gpio_regulator/gpio_regulator.ko output/
	$(Q)echo -n 'insmod gpio_regulator.ko' > $(gpio_regulator_init_file)
	$(Q)echo -n ' regulator_gpio0=$(if $(MD_X2000_GPIO_REGULATOR0_PIN),$(MD_X2000_GPIO_REGULATOR0_PIN),-1)' >> $(gpio_regulator_init_file)
	$(Q)echo -n ' regulator_gpio1=$(if $(MD_X2000_GPIO_REGULATOR1_PIN),$(MD_X2000_GPIO_REGULATOR1_PIN),-1)' >> $(gpio_regulator_init_file)
	$(Q)echo -n ' regulator_gpio2=$(if $(MD_X2000_GPIO_REGULATOR2_PIN),$(MD_X2000_GPIO_REGULATOR2_PIN),-1)' >> $(gpio_regulator_init_file)
	$(Q)echo -n ' active_gpio0=$(if $(MD_X2000_GPIO0_ACTIVET_LEVEL),$(MD_X2000_GPIO0_ACTIVET_LEVEL),-1)' >> $(gpio_regulator_init_file)
	$(Q)echo -n ' active_gpio1=$(if $(MD_X2000_GPIO1_ACTIVET_LEVEL),$(MD_X2000_GPIO1_ACTIVET_LEVEL),-1)' >> $(gpio_regulator_init_file)
	$(Q)echo -n ' active_gpio2=$(if $(MD_X2000_GPIO2_ACTIVET_LEVEL),$(MD_X2000_GPIO2_ACTIVET_LEVEL),-1)' >> $(gpio_regulator_init_file)
	$(Q)echo -n ' regulator_name0=$(if $(MD_X2000_GPIO_REGULATOR0_NAME),$(MD_X2000_GPIO_REGULATOR0_NAME),-1)' >> $(gpio_regulator_init_file)
	$(Q)echo -n ' regulator_name1=$(if $(MD_X2000_GPIO_REGULATOR1_NAME),$(MD_X2000_GPIO_REGULATOR1_NAME),-1)' >> $(gpio_regulator_init_file)
	$(Q)echo -n ' regulator_name2=$(if $(MD_X2000_GPIO_REGULATOR2_NAME),$(MD_X2000_GPIO_REGULATOR2_NAME),-1)' >> $(gpio_regulator_init_file)
	$(Q)echo  >> $(gpio_regulator_init_file)
endef
