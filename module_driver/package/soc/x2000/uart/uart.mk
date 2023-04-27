
#-------------------------------------------------------
package_name = soc_uart
package_depends = utils
package_module_src = soc/x2000/uart/
package_make_hook =
package_init_hook =
package_finalize_hook = uart_finalize_hook
package_clean_hook =
#-------------------------------------------------------

uart_init_file = output/soc_uart.sh

ifeq ($(MD_X2000_UART0),y)
define soc_uart0_write_param
	$(Q)echo -n "uart0_enable=1 " >> $(uart_init_file)
	$(Q)echo -n "uart0_rx=$(MD_X2000_UART0_RX) " >> $(uart_init_file)
	$(Q)echo -n "uart0_tx=$(MD_X2000_UART0_TX) " >> $(uart_init_file)
	$(Q)echo -n "uart0_rts=$(MD_X2000_UART0_RTS) " >> $(uart_init_file)
	$(Q)echo    "uart0_cts=$(MD_X2000_UART0_CTS) \\" >> $(uart_init_file)
endef
endif

ifeq ($(MD_X2000_UART1),y)
define soc_uart1_write_param
	$(Q)echo -n "uart1_enable=1 " >> $(uart_init_file)
	$(Q)echo -n "uart1_rx=$(MD_X2000_UART1_RX) " >> $(uart_init_file)
	$(Q)echo -n "uart1_tx=$(MD_X2000_UART1_TX) " >> $(uart_init_file)
	$(Q)echo -n "uart1_rts=$(MD_X2000_UART1_RTS) " >> $(uart_init_file)
	$(Q)echo    "uart1_cts=$(MD_X2000_UART1_CTS) \\" >> $(uart_init_file)
endef
endif

ifeq ($(MD_X2000_UART2),y)
define soc_uart2_write_param
	$(Q)echo -n "uart2_enable=1 " >> $(uart_init_file)
	$(Q)echo -n "uart2_rx=$(MD_X2000_UART2_RX) " >> $(uart_init_file)
	$(Q)echo -n "uart2_tx=$(MD_X2000_UART2_TX) " >> $(uart_init_file)
	$(Q)echo -n "uart2_rts=$(MD_X2000_UART2_RTS) " >> $(uart_init_file)
	$(Q)echo    "uart2_cts=$(MD_X2000_UART2_CTS) \\" >> $(uart_init_file)
endef
endif

ifeq ($(MD_X2000_UART3),y)
define soc_uart3_write_param
	$(Q)echo -n "uart3_enable=1 " >> $(uart_init_file)
	$(Q)echo -n "uart3_rx=$(MD_X2000_UART3_RX) " >> $(uart_init_file)
	$(Q)echo -n "uart3_tx=$(MD_X2000_UART3_TX) " >> $(uart_init_file)
	$(Q)echo -n "uart3_rts=$(MD_X2000_UART3_RTS) " >> $(uart_init_file)
	$(Q)echo    "uart3_cts=$(MD_X2000_UART3_CTS) \\" >> $(uart_init_file)
endef
endif

ifeq ($(MD_X2000_UART4),y)
define soc_uart4_write_param
	$(Q)echo -n "uart4_enable=1 " >> $(uart_init_file)
	$(Q)echo -n "uart4_rx=$(MD_X2000_UART4_RX) " >> $(uart_init_file)
	$(Q)echo -n "uart4_tx=$(MD_X2000_UART4_TX) " >> $(uart_init_file)
	$(Q)echo -n "uart4_rts=$(MD_X2000_UART4_RTS) " >> $(uart_init_file)
	$(Q)echo    "uart4_cts=$(MD_X2000_UART4_CTS) \\" >> $(uart_init_file)
endef
endif

ifeq ($(MD_X2000_UART5),y)
define soc_uart5_write_param
	$(Q)echo -n "uart5_enable=1 " >> $(uart_init_file)
	$(Q)echo -n "uart5_rx=$(MD_X2000_UART5_RX) " >> $(uart_init_file)
	$(Q)echo -n "uart5_tx=$(MD_X2000_UART5_TX) " >> $(uart_init_file)
	$(Q)echo -n "uart5_rts=$(MD_X2000_UART5_RTS) " >> $(uart_init_file)
	$(Q)echo    "uart5_cts=$(MD_X2000_UART5_CTS) \\" >> $(uart_init_file)
endef
endif

ifeq ($(MD_X2000_UART6),y)
define soc_uart6_write_param
	$(Q)echo -n "uart6_enable=1 " >> $(uart_init_file)
	$(Q)echo -n "uart6_rx=$(MD_X2000_UART6_RX) " >> $(uart_init_file)
	$(Q)echo -n "uart6_tx=$(MD_X2000_UART6_TX) " >> $(uart_init_file)
	$(Q)echo -n "uart6_rts=$(MD_X2000_UART6_RTS) " >> $(uart_init_file)
	$(Q)echo    "uart6_cts=$(MD_X2000_UART6_CTS) \\" >> $(uart_init_file)
endef
endif

ifeq ($(MD_X2000_UART7),y)
define soc_uart7_write_param
	$(Q)echo -n "uart7_enable=1 " >> $(uart_init_file)
	$(Q)echo -n "uart7_rx=$(MD_X2000_UART7_RX) " >> $(uart_init_file)
	$(Q)echo -n "uart7_tx=$(MD_X2000_UART7_TX) " >> $(uart_init_file)
	$(Q)echo -n "uart7_rts=$(MD_X2000_UART7_RTS) " >> $(uart_init_file)
	$(Q)echo    "uart7_cts=$(MD_X2000_UART7_CTS) \\" >> $(uart_init_file)
endef
endif

ifeq ($(MD_X2000_UART8),y)
define soc_uart8_write_param
	$(Q)echo -n "uart8_enable=1 " >> $(uart_init_file)
	$(Q)echo -n "uart8_rx=$(MD_X2000_UART8_RX) " >> $(uart_init_file)
	$(Q)echo -n "uart8_tx=$(MD_X2000_UART8_TX) " >> $(uart_init_file)
	$(Q)echo -n "uart8_rts=$(MD_X2000_UART8_RTS) " >> $(uart_init_file)
	$(Q)echo    "uart8_cts=$(MD_X2000_UART8_CTS) \\" >> $(uart_init_file)
endef
endif

ifeq ($(MD_X2000_UART9),y)
define soc_uart9_write_param
	$(Q)echo -n "uart9_enable=1 " >> $(uart_init_file)
	$(Q)echo -n "uart9_rx=$(MD_X2000_UART9_RX) " >> $(uart_init_file)
	$(Q)echo -n "uart9_tx=$(MD_X2000_UART9_TX) " >> $(uart_init_file)
	$(Q)echo -n "uart9_rts=$(MD_X2000_UART9_RTS) " >> $(uart_init_file)
	$(Q)echo    "uart9_cts=$(MD_X2000_UART9_CTS) \\" >> $(uart_init_file)
endef
endif

define uart_finalize_hook
	$(Q)cp soc/x2000/uart/soc_uart.ko output/
	$(Q)echo  -n "insmod soc_uart.ko " > $(uart_init_file)
	$(soc_uart0_write_param)
	$(soc_uart1_write_param)
	$(soc_uart2_write_param)
	$(soc_uart3_write_param)
	$(soc_uart4_write_param)
	$(soc_uart5_write_param)
	$(soc_uart6_write_param)
	$(soc_uart7_write_param)
	$(soc_uart8_write_param)
	$(soc_uart9_write_param)
	$(Q)echo  >> $(uart_init_file)
endef