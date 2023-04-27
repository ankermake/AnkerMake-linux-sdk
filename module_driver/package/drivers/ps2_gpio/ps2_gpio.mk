#-------------------------------------------------------
package_name = ps2_gpio
package_depends = utils
package_module_src = drivers/ps2_gpio
package_make_hook =
package_init_hook =
package_finalize_hook = ps2_gpio_finalize_hook
package_clean_hook =
#-------------------------------------------------------

ps2_gpio_init_file = output/ps2_gpio.sh

define ps2_gpio_finalize_hook
	$(Q)cp drivers/ps2_gpio/ps2-gpio.ko output/
	$(Q)echo -n 'insmod ps2-gpio.ko ' >> $(ps2_gpio_init_file)

	$(Q)echo -n "	ps2_0_is_enable=$(if $(MD_GPIO_PS2_0),1,0) " >> $(ps2_gpio_init_file)
	$(Q)echo -n "	ps2_0_clk_gpio=$(if $(MD_GPIO_PS2_0),$(MD_GPIO_PS2_0_CLK),-1) " >> $(ps2_gpio_init_file)
	$(Q)echo "	ps2_0_data_gpio=$(if $(MD_GPIO_PS2_0),$(MD_GPIO_PS2_0_DATA),-1) \\" >> $(ps2_gpio_init_file)

	$(Q)echo -n "	ps2_1_is_enable=$(if $(MD_GPIO_PS2_1),1,0) " >> $(ps2_gpio_init_file)
	$(Q)echo -n "	ps2_1_clk_gpio=$(if $(MD_GPIO_PS2_1),$(MD_GPIO_PS2_1_CLK),-1) " >> $(ps2_gpio_init_file)
	$(Q)echo -n "	ps2_1_data_gpio=$(if $(MD_GPIO_PS2_1),$(MD_GPIO_PS2_1_DATA),-1)" >> $(ps2_gpio_init_file)

	$(Q)echo >> $(ps2_gpio_init_file)
endef
