/**
 * Basic configuration(SOC, Cache, UART, DDR).
 */
#define CONFIG_MIPS32		/* MIPS32 CPU core */
#define CONFIG_CPU_XBURST
#define CONFIG_SYS_LITTLE_ENDIAN
#define CONFIG_X1520		/* X1520 SoC */
#define CONFIG_DDR_AUTO_SELF_REFRESH
#define CONFIG_SPL_DDR_SOFT_TRAINING
/*#define CONFIG_UPVOLTAGE            GPIO_PC(15)*/
#define HIGH                        1
#define LOW                         0


#define CONFIG_SYS_EXTAL		24000000	/* EXTAL freq: 24 MHz */
#define CONFIG_SYS_HZ			1000		/* incrementer freq */

#define CONFIG_SYS_DCACHE_SIZE		32768
#define CONFIG_SYS_ICACHE_SIZE		32768
#define CONFIG_SYS_CACHELINE_SIZE	32

#ifndef CONFIG_SYS_UART_INDE
#define CONFIG_SYS_UART_INDEX		1
#endif

#ifndef CONFIG_BAUDRATE
#define CONFIG_BAUDRATE			3000000
#endif
