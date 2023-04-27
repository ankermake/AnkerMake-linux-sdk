
#-------------------------------------------------------
package_name = soc_camera
package_depends = utils rmem_manager
package_module_src = soc/x2000/camera
package_make_hook =
package_init_hook =
package_finalize_hook = soc_camera_finalize_hook
package_clean_hook =
#-------------------------------------------------------

soc_camera_init_file = output/soc_camera.sh

define soc_camera_finalize_hook
	$(Q)cp soc/x2000/camera/soc_camera.ko output/
	$(Q)echo "insmod soc_camera.ko \\" > $(soc_camera_init_file)

	$(Q)echo "    reset_user_frame_when_stream_on=0 \\" >> $(soc_camera_init_file)

	$(Q)echo -n "    vic0_is_enable=$(if $(MD_X2000_CAMERA_VIC0),1,0 ) " >> $(soc_camera_init_file)
	$(Q)echo -n "vic0_is_isp_enable=$(if $(MD_X2000_CAMERA_VIC0_ISP_ENABLE),1,0 ) " >> $(soc_camera_init_file)
	$(Q)echo -n "vic0_mclk_io=$(if $(MD_X2000_CAMERA_VIC0),$(MD_X2000_CAMERA_VIC0_MCLK),-1)  " >> $(soc_camera_init_file)
	$(Q)echo "vic0_frame_nums=$(if $(MD_X2000_CAMERA_VIC0_FRAME_NUMS),$(MD_X2000_CAMERA_VIC0_FRAME_NUMS),0 ) \\" >> $(soc_camera_init_file)

	$(Q)echo -n "    vic1_is_enable=$(if $(MD_X2000_CAMERA_VIC1),1,0 ) " >> $(soc_camera_init_file)
	$(Q)echo -n "vic1_is_isp_enable=$(if $(MD_X2000_CAMERA_VIC1_ISP_ENABLE),1,0 ) " >> $(soc_camera_init_file)
	$(Q)echo -n "vic1_mclk_io=$(if $(MD_X2000_CAMERA_VIC1),$(MD_X2000_CAMERA_VIC1_MCLK),-1)  " >> $(soc_camera_init_file)
	$(Q)echo "vic1_frame_nums=$(if $(MD_X2000_CAMERA_VIC1_FRAME_NUMS),$(MD_X2000_CAMERA_VIC1_FRAME_NUMS),0 ) \\" >> $(soc_camera_init_file)

	$(Q)echo -n "    cim_is_enable=$(if $(MD_X2000_CAMERA_CIM),1,0 ) " >> $(soc_camera_init_file)
	$(Q)echo -n "cim_mclk_io=$(if $(MD_X2000_CAMERA_CIM),$(MD_X2000_CAMERA_CIM_MCLK),-1)  " >> $(soc_camera_init_file)
	$(Q)echo -n "cim_frame_nums=$(if $(MD_X2000_CAMERA_CIM_FRAME_NUMS),$(MD_X2000_CAMERA_CIM_FRAME_NUMS),0 ) " >> $(soc_camera_init_file)

	$(Q)echo  >> $(soc_camera_init_file)

endef
