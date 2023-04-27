OUTPUT_ARCH(mips)
ENTRY(_reset)
MEMORY{
	tcsm_bank0(rw)  : ORIGIN = 0xF4000000 , LENGTH = 0x1000
	tcsm_bank1(rw)  : ORIGIN = 0xF4001000 , LENGTH = 0x800
}

SECTIONS
{
	.tcsm_bank0 : {
		. = ALIGN(4);
                *(.text)
                *(.data)
                CONSTRUCTORS
                _gp = ALIGN(4);
                *(.sdata)
                *(.sdata.*)
                __bss_start = .;
                *(.sbss)
                *(.bss)
                __bss_end = .;
                *(.scommon)
	} > tcsm_bank0

	.tcsm_bank1 : {
		*(.rodata*)
	} > tcsm_bank1

	/DISCARD/ : {
		*(COMMON)
		*(.pdri)
		*(.comment)
		*(.gnu.attributes)
		*(.reginfo)
		*(.MIPS.abiflags)
	}
}
