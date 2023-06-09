#define CONFIG_MIPS32R2		/* MIPS32 CPU core */
#define CONFIG_CPU_XBURST
#define CONFIG_SYS_LITTLE_ENDIAN
#define CONFIG_X1000
#define CONFIG_CHECK_SOCID
#define CONFIG_SYS_EXTAL		24000000	/* EXTAL freq: 24 MHz */
#define CONFIG_SYS_HZ			1000 /* incrementer freq */

#define CONFIG_SMALL_BAUDRATE_TABLE

#define CONFIG_SYS_DCACHE_SIZE		16384
#define CONFIG_SYS_ICACHE_SIZE		16384
#define CONFIG_SYS_CACHELINE_SIZE	32

#ifdef CONFIG_GPIO_SPI_TO_UART
#define CONFIG_SYS_UART_INDEX		-1
#endif

#ifdef CONFIG_GPIO_SPI_TO_UART2
#define CONFIG_SYS_UART_INDEX		-1
#endif

#ifndef CONFIG_SYS_UART_INDEX
#define CONFIG_SYS_UART_INDEX		2
#endif

#ifndef CONFIG_BAUDRATE
#define CONFIG_BAUDRATE			3000000
#endif
