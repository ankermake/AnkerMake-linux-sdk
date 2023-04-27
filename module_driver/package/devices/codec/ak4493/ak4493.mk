#-------------------------------------------------------
package_name = codec_ak4493
package_module_src = devices/codec/ak4493
package_make_hook =
package_init_hook =
package_finalize_hook = codec_ak4493_finalize_hook
package_clean_hook =
#-------------------------------------------------------

codec_ak4493_init_file = output/codec_ak4493.sh

TARGET_INSTALL_HOOKS +=
TARGET_INSTALL_CLEAN_HOOKS +=

define codec_ak4493_finalize_hook
	$(Q)cp devices/codec/ak4493/codec_ak4493.ko output/
	$(Q)echo -n 'insmod codec_ak4493.ko' > $(codec_ak4493_init_file)
	$(Q)echo -n ' cd_sel_gpio=$(MD_AK4493_GPIO_CD_SEL)' >> $(codec_ak4493_init_file)
	$(Q)echo -n ' ubw_sel_gpio=$(MD_AK4493_GPIO_UBW_SEL)' >> $(codec_ak4493_init_file)
	$(Q)echo -n ' pdn_gpio=$(MD_AK4493_GPIO_PDN)' >> $(codec_ak4493_init_file)
	$(Q)echo -n ' mute_gpio=$(MD_AK4493_GPIO_MUTE)' >> $(codec_ak4493_init_file)
	$(Q)echo -n ' i2c_bus_num=$(MD_AK4493_I2C_BUSNUM)' >> $(codec_ak4493_init_file)
	$(Q)echo  >> $(codec_ak4493_init_file)
endef
