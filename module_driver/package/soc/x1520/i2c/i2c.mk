
#-------------------------------------------------------
package_name = soc_i2c
package_depends =
package_module_src = soc/x1520/i2c/
package_make_hook =
package_init_hook =
package_finalize_hook = soc_i2c_finalize_hook
package_clean_hook =
#-------------------------------------------------------

soc_i2c_init_file = output/soc_i2c.sh

define soc_i2c_finalize_hook
	$(Q)cp soc/x1520/i2c/soc_i2c.ko output/
	$(Q)echo "insmod soc_i2c.ko \\" > $(soc_i2c_init_file)

	$(Q)echo -n "	i2c0_is_enable=$(if $(MD_X1520_I2C0_BUS),1,0) " >> $(soc_i2c_init_file)
	$(Q)echo -n "i2c0_rate=$(if $(MD_X1520_I2C0_RATE),$(MD_X1520_I2C0_RATE),0) " >> $(soc_i2c_init_file)
	$(Q)echo -n "i2c0_scl=PA13 " >> $(soc_i2c_init_file)
	$(Q)echo "i2c0_sda=PA12 \\" >> $(soc_i2c_init_file)

	$(Q)echo -n "	i2c1_is_enable=$(if $(MD_X1520_I2C1_BUS),1,0) " >> $(soc_i2c_init_file)
	$(Q)echo -n "i2c1_rate=$(if $(MD_X1520_I2C1_RATE),$(MD_X1520_I2C1_RATE),0) " >> $(soc_i2c_init_file)
	$(Q)echo -n "i2c1_scl=PB26 " >> $(soc_i2c_init_file)
	$(Q)echo "i2c1_sda=PB25 " >> $(soc_i2c_init_file)


endef
