
#-------------------------------------------------------
package_name = soc_camera
package_depends = utils
package_module_src = soc/x1600_510/camera
package_make_hook =
package_init_hook =
package_finalize_hook = soc_camera_finalize_hook
package_clean_hook =
#-------------------------------------------------------

soc_camera_init_file = output/soc_camera.sh

define soc_camera_finalize_hook
	$(Q)cp soc/x1600_510/camera/soc_camera.ko output/
	$(Q)echo "insmod soc_camera.ko \\" > $(soc_camera_init_file)

	$(Q)echo -n "mclk_io=$(if $(MD_X1600_510_CAMERA),$(MD_X1600_510_CAMERA_MCLK),-1)  " >> $(soc_camera_init_file)
	$(Q)echo -n "frame_nums=$(if $(MD_X1600_510_CAMERA_CIM_FRAME_NUMS),$(MD_X1600_510_CAMERA_CIM_FRAME_NUMS),0 ) " >> $(soc_camera_init_file)

	$(Q)echo  >> $(soc_camera_init_file)

endef
