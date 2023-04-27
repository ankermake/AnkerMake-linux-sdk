
#-------------------------------------------------------
package_name = soc_camera
package_depends = utils
package_module_src = soc/x1830/camera
package_make_hook =
package_init_hook =
package_finalize_hook = soc_camera_finalize_hook
package_clean_hook =
#-------------------------------------------------------

soc_camera_init_file = output/soc_camera.sh

define soc_camera_finalize_hook
	$(Q)cp soc/x1830/camera/soc_camera.ko output/
	$(Q)echo -n 'insmod soc_camera.ko' > $(soc_camera_init_file)
	$(Q)echo -n ' frame_nums=$(MD_X1830_CAMERA_FRAME_NUMS)' >> $(soc_camera_init_file)
	$(Q)echo  >> $(soc_camera_init_file)

endef
