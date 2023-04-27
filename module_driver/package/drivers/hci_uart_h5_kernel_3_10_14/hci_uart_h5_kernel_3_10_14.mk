#-------------------------------------------------------
package_name = hci_uart_h5_kernel_3_10_14
package_depends = utils
package_module_src = drivers/hci_uart_h5_kernel_3_10_14/
package_make_hook =
package_init_hook =
package_finalize_hook = hci_uart_h5_kernel_3_10_14_finalize_hook
package_clean_hook =
#-------------------------------------------------------

hci_uart_h5_kernel_3_10_14_init_file = output/hci_uart_h5_kernel_3_10_14.sh

define hci_uart_h5_kernel_3_10_14_finalize_hook
	$(Q)cp drivers/hci_uart_h5_kernel_3_10_14/hci_uart_h5_kernel_3_10_14.ko output/
	$(Q)echo 'insmod hci_uart_h5_kernel_3_10_14.ko ' > $(hci_uart_h5_kernel_3_10_14_init_file)
endef