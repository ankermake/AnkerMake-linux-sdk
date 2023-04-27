#-------------------------------------------------------
package_name = hci_uart_h5_kernel_4_4_94
package_depends = utils
package_module_src = drivers/hci_uart_h5_kernel_4_4_94/
package_make_hook =
package_init_hook =
package_finalize_hook = hci_uart_h5_kernel_4_4_94_finalize_hook
package_clean_hook =
#-------------------------------------------------------

hci_uart_h5_kernel_4_4_94_init_file = output/hci_uart_h5_kernel_4_4_94.sh

define hci_uart_h5_kernel_4_4_94_finalize_hook
	$(Q)cp drivers/hci_uart_h5_kernel_4_4_94/hci_uart_h5_kernel_4_4_94.ko output/
	$(Q)echo 'insmod hci_uart_h5_kernel_4_4_94.ko ' > $(hci_uart_h5_kernel_4_4_94_init_file)
endef