
#-------------------------------------------------------
package_name = keyboard_adc
package_depends = utils soc_adc
package_module_src = devices/keyboard_adc/
package_make_hook =
package_init_hook =
package_finalize_hook = keyboard_adc_finalize_hook
package_clean_hook =
#-------------------------------------------------------

keyboard_adc_init_file = output/keyboard_adc.sh

to_adc_keyboard_code = $(if $($(1)_CODE),$(shell tools/key_to_code.sh $($(1)_CODE)),-1)
to_adc_keyboard_value = $(if $($(1)_VALUE),$($(1)_VALUE),-1)

define keyboard_adc_finalize_hook
	$(Q)cp devices/keyboard_adc/keyboard_adc.ko output/
	$(Q)echo 'insmod keyboard_adc.ko \' > $(keyboard_adc_init_file)

	$(Q)echo    '	adc_init_value=$(MD_ADC_INIT_VALUE) \' >> $(keyboard_adc_init_file)
	$(Q)echo    '	adc_channel=$(MD_ADC_CHANNEL) \' >> $(keyboard_adc_init_file)
	$(Q)echo    '	adc_deviation=$(MD_ADC_DEVIATION) \' >> $(keyboard_adc_init_file)
	$(Q)echo    '	adc_key_detectime=$(MD_ADC_KEY_DETECTIME) \' >> $(keyboard_adc_init_file)
	$(Q)echo    '	debug=$(MD_ADC_KEY_DEBUG) \' >> $(keyboard_adc_init_file)

	$(Q)echo -n '	key1_code=$(call to_adc_keyboard_code,MD_ADC_KEY1) ' >> $(keyboard_adc_init_file)
	$(Q)echo 'key1_value=$(call to_adc_keyboard_value,MD_ADC_KEY1) \' >> $(keyboard_adc_init_file)

	$(Q)echo -n '	key2_code=$(call to_adc_keyboard_code,MD_ADC_KEY2) ' >> $(keyboard_adc_init_file)
	$(Q)echo 'key2_value=$(call to_adc_keyboard_value,MD_ADC_KEY2) \' >> $(keyboard_adc_init_file)

	$(Q)echo -n '	key3_code=$(call to_adc_keyboard_code,MD_ADC_KEY3) ' >> $(keyboard_adc_init_file)
	$(Q)echo 'key3_value=$(call to_adc_keyboard_value,MD_ADC_KEY3) \' >> $(keyboard_adc_init_file)

	$(Q)echo -n '	key4_code=$(call to_adc_keyboard_code,MD_ADC_KEY4) ' >> $(keyboard_adc_init_file)
	$(Q)echo 'key4_value=$(call to_adc_keyboard_value,MD_ADC_KEY4) \' >> $(keyboard_adc_init_file)

	$(Q)echo -n '	key5_code=$(call to_adc_keyboard_code,MD_ADC_KEY5) ' >> $(keyboard_adc_init_file)
	$(Q)echo 'key5_value=$(call to_adc_keyboard_value,MD_ADC_KEY5) \' >> $(keyboard_adc_init_file)

	$(Q)echo -n '	key6_code=$(call to_adc_keyboard_code,MD_ADC_KEY6) ' >> $(keyboard_adc_init_file)
	$(Q)echo 'key6_value=$(call to_adc_keyboard_value,MD_ADC_KEY6) \' >> $(keyboard_adc_init_file)

	$(Q)echo -n '	key7_code=$(call to_adc_keyboard_code,MD_ADC_KEY7) ' >> $(keyboard_adc_init_file)
	$(Q)echo 'key7_value=$(call to_adc_keyboard_value,MD_ADC_KEY7) \' >> $(keyboard_adc_init_file)

	$(Q)echo -n '	key8_code=$(call to_adc_keyboard_code,MD_ADC_KEY8) ' >> $(keyboard_adc_init_file)
	$(Q)echo 'key8_value=$(call to_adc_keyboard_value,MD_ADC_KEY8) ' >> $(keyboard_adc_init_file)
endef
