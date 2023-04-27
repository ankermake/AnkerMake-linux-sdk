#-------------------------------------------------------
package_name = spi_fb
package_depends =
package_module_src = drivers/spi_fb
package_make_hook =
package_init_hook =
package_finalize_hook = spi_fb_finalize_hook
package_clean_hook =
#-------------------------------------------------------

spi_fb_init_file = output/spi_fb.sh

define spi_fb_finalize_hook
    $(Q)cp drivers/spi_fb/spi_fb.ko output/
    $(Q)echo -n 'insmod spi_fb.ko' > $(spi_fb_init_file)
    $(Q)echo -n ' frame_num=$(MD_X2000_FB0_FREMES)' >> $(spi_fb_init_file)
    $(Q)echo  >> $(spi_fb_init_file)
endef