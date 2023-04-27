
#-------------------------------------------------------
package_name = soc_camera
package_depends = utils
package_module_src = soc/x1000/camera
package_make_hook =
package_init_hook =
package_finalize_hook = soc_camera_finalize_hook
package_clean_hook =
#-------------------------------------------------------

soc_camera_init_file = output/soc_camera.sh

define soc_camera_finalize_hook
	$(Q)cp soc/x1000/camera/soc_camera.ko output/
	$(Q)cat package/soc/x1520/camera/get_rmem.sh > $(soc_camera_init_file)
	$(Q)echo -n 'insmod soc_camera.ko' >> $(soc_camera_init_file)
	$(Q)echo -n ' frame_nums=$(MD_X1000_CAMERA_FRAME_NUMS)' >> $(soc_camera_init_file)
	$(Q)echo -n ' rmem_start=$$rmem_start' >> $(soc_camera_init_file)
	$(Q)echo -n ' rmem_size=$$rmem_size' >> $(soc_camera_init_file)
	$(Q)echo -n ' enable_debug=0' >> $(soc_camera_init_file)
	$(Q)echo  >> $(soc_camera_init_file)
endef
