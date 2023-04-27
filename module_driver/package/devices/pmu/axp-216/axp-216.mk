#-------------------------------------------------------
package_name = axp-216
package_depends = utils
package_module_src = devices/pmu/axp-216/
package_make_hook =
package_init_hook =
package_finalize_hook = axp_216_finalize_hook
package_clean_hook =
#-------------------------------------------------------

axp_216_init_file = output/axp-216.sh

define axp_216_finalize_hook
	$(Q)cp devices/pmu/axp-216/axp-216.ko output/
	$(Q)echo -n 'insmod axp-216.ko' > $(axp_216_init_file)
	$(Q)echo -n ' pmu_int_gpio=$(MD_X2000_AXP216_INT_GPIO)' >> $(axp_216_init_file)
	$(Q)echo -n ' i2c_bus_num=$(MD_X2000_AXP216_I2C_BUSNUM)' >> $(axp_216_init_file)
	$(Q)echo -n ' rtcldo=\"regulator_name=$(MD_X2000_AXP216_RTCLDO_NAME) boot_on=$(MD_X2000_AXP216_RTCLDO_BOOTON) work_voltage=$(MD_X2000_AXP216_RTCLDO_WORK_VOLTAGE) suspend_volage=$(MD_X2000_AXP216_RTCLDO_SUSPEND_VOLTAGE)\"'>> $(axp_216_init_file)
	$(Q)echo -n ' aldo1=\"regulator_name=$(MD_X2000_AXP216_ALDO1_NAME) boot_on=$(MD_X2000_AXP216_ALDO1_BOOTON) work_voltage=$(MD_X2000_AXP216_ALDO1_WORK_VOLTAGE) suspend_volage=$(MD_X2000_AXP216_ALDO1_SUSPEND_VOLTAGE)\"'>> $(axp_216_init_file)
	$(Q)echo -n ' aldo2=\"regulator_name=$(MD_X2000_AXP216_ALDO2_NAME) boot_on=$(MD_X2000_AXP216_ALDO2_BOOTON) work_voltage=$(MD_X2000_AXP216_ALDO2_WORK_VOLTAGE) suspend_volage=$(MD_X2000_AXP216_ALDO2_SUSPEND_VOLTAGE)\"'>> $(axp_216_init_file)
	$(Q)echo -n ' aldo3=\"regulator_name=$(MD_X2000_AXP216_ALDO3_NAME) boot_on=$(MD_X2000_AXP216_ALDO3_BOOTON) work_voltage=$(MD_X2000_AXP216_ALDO3_WORK_VOLTAGE) suspend_volage=$(MD_X2000_AXP216_ALDO3_SUSPEND_VOLTAGE)\"'>> $(axp_216_init_file)
	$(Q)echo -n ' eldo1=\"regulator_name=$(MD_X2000_AXP216_ELDO1_NAME) boot_on=$(MD_X2000_AXP216_ELDO1_BOOTON) work_voltage=$(MD_X2000_AXP216_ELDO1_WORK_VOLTAGE) suspend_volage=$(MD_X2000_AXP216_ELDO1_SUSPEND_VOLTAGE)\"'>> $(axp_216_init_file)
	$(Q)echo -n ' eldo2=\"regulator_name=$(MD_X2000_AXP216_ELDO2_NAME) boot_on=$(MD_X2000_AXP216_ELDO2_BOOTON) work_voltage=$(MD_X2000_AXP216_ELDO2_WORK_VOLTAGE) suspend_volage=$(MD_X2000_AXP216_ELDO2_SUSPEND_VOLTAGE)\"'>> $(axp_216_init_file)
	$(Q)echo -n ' dcdc1=\"regulator_name=$(MD_X2000_AXP216_DCDC1_NAME) boot_on=$(MD_X2000_AXP216_DCDC1_BOOTON) work_voltage=$(MD_X2000_AXP216_DCDC1_WORK_VOLTAGE) suspend_volage=$(MD_X2000_AXP216_DCDC1_SUSPEND_VOLTAGE)\"'>> $(axp_216_init_file)
	$(Q)echo -n ' dcdc2=\"regulator_name=$(MD_X2000_AXP216_DCDC2_NAME) boot_on=$(MD_X2000_AXP216_DCDC2_BOOTON) work_voltage=$(MD_X2000_AXP216_DCDC2_WORK_VOLTAGE) suspend_volage=$(MD_X2000_AXP216_DCDC2_SUSPEND_VOLTAGE)\"'>> $(axp_216_init_file)
	$(Q)echo -n ' dcdc3=\"regulator_name=$(MD_X2000_AXP216_DCDC3_NAME) boot_on=$(MD_X2000_AXP216_DCDC3_BOOTON) work_voltage=$(MD_X2000_AXP216_DCDC3_WORK_VOLTAGE) suspend_volage=$(MD_X2000_AXP216_DCDC3_SUSPEND_VOLTAGE)\"'>> $(axp_216_init_file)
	$(Q)echo -n ' dcdc4=\"regulator_name=$(MD_X2000_AXP216_DCDC4_NAME) boot_on=$(MD_X2000_AXP216_DCDC4_BOOTON) work_voltage=$(MD_X2000_AXP216_DCDC4_WORK_VOLTAGE) suspend_volage=$(MD_X2000_AXP216_DCDC4_SUSPEND_VOLTAGE)\"'>> $(axp_216_init_file)
	$(Q)echo -n ' dcdc5=\"regulator_name=$(MD_X2000_AXP216_DCDC5_NAME) boot_on=$(MD_X2000_AXP216_DCDC5_BOOTON) work_voltage=$(MD_X2000_AXP216_DCDC5_WORK_VOLTAGE) suspend_volage=$(MD_X2000_AXP216_DCDC5_SUSPEND_VOLTAGE)\"'>> $(axp_216_init_file)
	$(Q)echo -n ' gpioldo1=\"regulator_name=$(MD_X2000_AXP216_GPIOLDO1_NAME) boot_on=$(MD_X2000_AXP216_GPIOLDO1_BOOTON) work_voltage=$(MD_X2000_AXP216_GPIOLDO1_WORK_VOLTAGE) suspend_volage=$(MD_X2000_AXP216_GPIOLDO1_SUSPEND_VOLTAGE)\"'>> $(axp_216_init_file)
	$(Q)echo  >> $(axp_216_init_file)
endef
