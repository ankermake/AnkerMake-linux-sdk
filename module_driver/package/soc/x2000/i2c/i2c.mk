
#-------------------------------------------------------
package_name = soc_i2c
package_depends = utils
package_module_src = soc/x2000/i2c/
package_make_hook =
package_init_hook =
package_finalize_hook = soc_i2c_finalize_hook
package_clean_hook =
#-------------------------------------------------------

soc_i2c_init_file = output/soc_i2c.sh

define soc_i2c_finalize_hook
	$(Q)cp soc/x2000/i2c/soc_i2c.ko output/
	$(Q)echo "insmod soc_i2c.ko \\" > $(soc_i2c_init_file)

	$(Q)echo -n "	i2c0_is_enable=$(if $(MD_X2000_I2C0_BUS),1,0) " >> $(soc_i2c_init_file)
	$(Q)echo -n "i2c0_rate=$(if $(MD_X2000_I2C0_RATE),$(MD_X2000_I2C0_RATE),0) " >> $(soc_i2c_init_file)
	$(Q)echo -n "i2c0_scl=PC13 " >> $(soc_i2c_init_file)
	$(Q)echo "i2c0_sda=PC14 \\" >> $(soc_i2c_init_file)

	$(Q)echo -n "	i2c1_is_enable=$(if $(MD_X2000_I2C1_BUS),1,0) " >> $(soc_i2c_init_file)
	$(Q)echo -n "i2c1_rate=$(if $(MD_X2000_I2C1_RATE),$(MD_X2000_I2C1_RATE),0) " >> $(soc_i2c_init_file)
	$(Q)echo -n "i2c1_scl=$(if $(MD_X2000_I2C1_PC),PC23,PD11) " >> $(soc_i2c_init_file)
	$(Q)echo "i2c1_sda=$(if $(MD_X2000_I2C1_PC),PC24,PD12) \\" >> $(soc_i2c_init_file)

	$(Q)echo -n "	i2c2_is_enable=$(if $(MD_X2000_I2C2_BUS),1,0) " >> $(soc_i2c_init_file)
	$(Q)echo -n "i2c2_rate=$(if $(MD_X2000_I2C2_RATE),$(MD_X2000_I2C2_RATE),0) " >> $(soc_i2c_init_file)
	$(Q)echo -n "i2c2_scl=$(if $(MD_X2000_I2C2_PB),PB22,$(if $(MD_X2000_I2C2_PD),PD20,PE19)) " >> $(soc_i2c_init_file)
	$(Q)echo "i2c2_sda=$(if $(MD_X2000_I2C2_PB),PB23,$(if $(MD_X2000_I2C2_PD),PD21,PE20)) \\" >> $(soc_i2c_init_file)

	$(Q)echo -n "	i2c3_is_enable=$(if $(MD_X2000_I2C3_BUS),1,0) " >> $(soc_i2c_init_file)
	$(Q)echo -n "i2c3_rate=$(if $(MD_X2000_I2C3_RATE),$(MD_X2000_I2C3_RATE),0) " >> $(soc_i2c_init_file)
	$(Q)echo -n "i2c3_scl=$(if $(MD_X2000_I2C3_PA),PA16,PD30) " >> $(soc_i2c_init_file)
	$(Q)echo "i2c3_sda=$(if $(MD_X2000_I2C3_PA),PA17,PD31) \\" >> $(soc_i2c_init_file)

	$(Q)echo -n "	i2c4_is_enable=$(if $(MD_X2000_I2C4_BUS),1,0) " >> $(soc_i2c_init_file)
	$(Q)echo -n "i2c4_rate=$(if $(MD_X2000_I2C4_RATE),$(MD_X2000_I2C4_RATE),0) " >> $(soc_i2c_init_file)
	$(Q)echo -n "i2c4_scl=$(if $(MD_X2000_I2C4_PC),PC25,PD00) " >> $(soc_i2c_init_file)
	$(Q)echo "i2c4_sda=$(if $(MD_X2000_I2C4_PC),PC26,PD01) \\" >> $(soc_i2c_init_file)

	$(Q)echo -n "	i2c5_is_enable=$(if $(MD_X2000_I2C5_BUS),1,0) " >> $(soc_i2c_init_file)
	$(Q)echo -n "i2c5_rate=$(if $(MD_X2000_I2C5_RATE),$(MD_X2000_I2C5_RATE),0) " >> $(soc_i2c_init_file)
	$(Q)echo -n "i2c5_scl=$(if $(MD_X2000_I2C5_PC),PC27,PD04) " >> $(soc_i2c_init_file)
	$(Q)echo "i2c5_sda=$(if $(MD_X2000_I2C5_PC),PC28,PD05) " >> $(soc_i2c_init_file)
endef
