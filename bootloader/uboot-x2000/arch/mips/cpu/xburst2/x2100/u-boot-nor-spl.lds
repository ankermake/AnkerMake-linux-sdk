MEMORY { .sram : ORIGIN = CONFIG_SPL_TEXT_BASE,\
		LENGTH = CONFIG_SPL_MAX_SIZE }

OUTPUT_ARCH(mips)
ENTRY(_start)
SECTIONS
{
	.text      :
	{
		__start = .;
		*(.start_section*)
		*(.text*)
		. = ALIGN(4);
	} >.sram
	.MIPS.abiflags :
	{
		. = ALIGN(4);
	} > .sram
	.rodata : {
		*(SORT_BY_ALIGNMENT(.rodata*))
		. = ALIGN(4);
	} >.sram

	__data_start = .;
	__data_end = . + SIZEOF(.data);
	__image_copy_end = __data_end;

	.data 0x80000000 : AT(ADDR(.rodata) + SIZEOF(.rodata))
	{
		*(SORT_BY_ALIGNMENT(.data*))
		. = ALIGN(4);
	}

	.bss 0x80000000 + SIZEOF(.data) : AT(ADDR(.rodata) + SIZEOF(.rodata) + SIZEOF(.data))
	{
		. = ALIGN(4);
		__bss_start = .;
		*(.sbss.*)
		*(.bss.*)
		*(COMMON)
		. = ALIGN(4);
		__bss_end = .;
	}

	/DISCARD/ : {
		*(.dynbss)
		*(.dynstr)
		*(.dynamic)
		*(.interp)
		*(.hash)
		*(.gnu.*)
		*(.plt)
		*(.got.plt)
		*(.rel.plt)
		*(.rel.dyn)
	}
}
