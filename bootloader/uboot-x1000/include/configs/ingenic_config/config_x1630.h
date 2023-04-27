#if defined(CONFIG_DDR3_600M)
#elif defined(CONFIG_DDR3_500M)
#elif defined(CONFIG_DDR3_400M)
#elif defined(CONFIG_DDR3_300M)
#else
#define CONFIG_DDR3_600M
#endif

/**
 * PLL
 **/
#if defined(CONFIG_CPU_1536M)
#define CONFIG_SYS_APLL_FREQ      1536000000
#define CONFIG_SYS_APLL_MNOD      ((95 << 20) | (2 << 14) | (0 << 11) | (1<<5))
#define ARG_LPJ         "lpj=7660000"
#elif defined(CONFIG_CPU_1200M)
#define CONFIG_SYS_APLL_FREQ      1200000000
#define CONFIG_SYS_APLL_MNOD      ((149 << 20) | (2 << 14) | (1 << 11) | (1<<5))
#define ARG_LPJ         "lpj=5980000"
#elif defined(CONFIG_CPU_900M)
#define CONFIG_SYS_APLL_FREQ      900000000
#define CONFIG_SYS_APLL_MNOD      ((74 << 20) | (1 << 14) | (1 << 11) | (2<<5))
#define ARG_LPJ         "lpj=4480000"
#else /* default 1200M */
#define CONFIG_SYS_APLL_FREQ      1200000000
#define CONFIG_SYS_APLL_MNOD      ((149 << 20) | (2 << 14) | (1 << 11) | (1<<5))
#define ARG_LPJ         "lpj=5980000"
#endif

#if defined(CONFIG_RMEM_18M)
#define ARG_MEM "mem=46M@0x0 rmem=18M@0x2e00000"
#else
#define ARG_MEM "mem=64M@0x0"
#endif

#if defined(CONFIG_DDR3_600M) || defined(CONFIG_DDR3_400M)
#define CONFIG_SYS_MPLL_FREQ        1200000000
#define CONFIG_SYS_MPLL_MNOD        ((149 << 20) | (2 << 14) | (1 << 11) | (1<<5))
#elif defined(CONFIG_DDR3_500M)
#define CONFIG_SYS_MPLL_FREQ        1000000000
#define CONFIG_SYS_MPLL_MNOD        ((124 << 20) | (2 << 14) | (1 << 11) | (1<<5))
#elif defined(CONFIG_DDR3_300M)
#define CONFIG_SYS_MPLL_FREQ        600000000
#define CONFIG_SYS_MPLL_MNOD        ((99 << 20) | (1 << 14) | (2 << 11) | (2<<5))
#endif
#define CONFIG_SYS_VPLL_FREQ        1200000000    /*If VPLL not use mast be set 0*/
#define CONFIG_SYS_EPLL_MNOD        ((197 << 20) | (3 << 14) | (1 << 11) | (1<<5))
#define CONFIG_SYS_EPLL_FREQ        1188000000

/**
 * CPU & DDR
 **/
#define CONFIG_SYS_CPU_FREQ        CONFIG_SYS_APLL_FREQ

#ifndef CONFIG_DDR3_400M
#define CONFIG_SYS_MEM_FREQ        ((CONFIG_SYS_MPLL_FREQ) >> 1)
#else
#define CONFIG_SYS_MEM_FREQ        ((CONFIG_SYS_MPLL_FREQ) / 3)
#endif
#ifndef CONFIG_DDR3_500M
#define CONFIG_SYS_CPCCR_SEL        ((2 << 30) | (1 << 28) | (2 << 26) | (2 << 24) \
                        | (11 << 16) | (5 << 12) | (5 << 8)    \
                        | (1 << 4) | (0 << 0))
#else
#define CONFIG_SYS_CPCCR_SEL        ((2 << 30) | (1 << 28) | (2 << 26) | (2 << 24) \
                        | (9 << 16) | (4 << 12) | (4 << 8)    \
                        | (1 << 4) | (0 << 0))
#endif

#define CONFIG_CPU_SEL_PLL        APLL
#define CONFIG_DDR_SEL_PLL        MPLL

#define CONFIG_SPL_DDR_SOFT_TRAINING
#define DDR2_CHIP_MR0_DLL_RST
#define CONFIG_DDR_TYPE_DDR2
#define CONFIG_DDR_PARAMS_CREATOR
#define CONFIG_DDR_HOST_CC
#define CONFIG_DDR_CS0            1    /* 1-connected, 0-disconnected */
#define CONFIG_DDR_CS1            0    /* 1-connected, 0-disconnected */
#define CONFIG_DDR_DW32            0    /* 1-32bit-width, 0-16bit-width */
#define CONFIG_DDRC_CTRL_PDT DDRC_CTRL_PDT_128
#define CONFIG_DDR2_M14D1G1664A
#define CONFIG_X1XXX_INNOPHY
#define DDR2_CHIP_DRIVER_OUT_STRENGTH 0
#define CONFIG_DDR_PHY_IMPEDANCE 40000
#define CONFIG_DDR_PHY_ODT_IMPEDANCE 50000



/**
 * Command configuration.
 */
#define CONFIG_CMD_BOOTD	/* bootd			*/
#define CONFIG_CMD_SAVEENV	/* saveenv			*/
#define CONFIG_CMD_CONSOLE	/* coninfo			*/
#define CONFIG_CMD_MEMORY	/* md mm nm mw cp cmp crc base loop mtest */
#define CONFIG_CMD_RUN		/* run command in env variable	*/


/* GPIO */
#define CONFIG_JZ_GPIO

/**
 * Miscellaneous configurable options
 */
#define CONFIG_LZO
#define CONFIG_RBTREE
#define CONFIG_SKIP_LOWLEVEL_INIT
#define CONFIG_BOARD_EARLY_INIT_F
#define CONFIG_SYS_NO_FLASH
#define CONFIG_SYS_FLASH_BASE	0 /* init flash_base as 0 */
#define CONFIG_ENV_OVERWRITE
#define CONFIG_MISC_INIT_R	1
#define CONFIG_BOOTP_MASK	(CONFIG_BOOTP_DEFAUL)
#define CONFIG_SYS_MAXARGS 	16
#define CONFIG_SYS_LONGHELP

#if defined(CONFIG_SPL_SFC_NOR) || defined(CONFIG_SPL_SFC_NAND)
  #define CONFIG_SPL_SFC_SUPPORT
#endif
#if defined(CONFIG_SPL_JZMMC_SUPPORT)
  #if defined(CONFIG_JZ_MMC_MSC0)
    #define CONFIG_SYS_PROMPT CONFIG_SYS_BOARD "-msc0# "
  #endif
#elif defined(CONFIG_SPL_NOR_SUPPORT)
  #define CONFIG_SYS_PROMPT CONFIG_SYS_BOARD "-nor# "
#elif defined(CONFIG_SPL_SFC_SUPPORT)
  #if defined(CONFIG_SPL_SFC_NOR)
    #define CONFIG_SYS_PROMPT CONFIG_SYS_BOARD "-sfcnor# "
  #else  /* CONFIG_SPL_SFC_NAND */
    #define CONFIG_SYS_PROMPT CONFIG_SYS_BOARD "-sfcnand# "
  #endif
#endif

/* MMC */
#if defined(CONFIG_JZ_MMC_MSC0)
#define CONFIG_CMD_MMC			/* MMC/SD support*/
#define CONFIG_GENERIC_MMC		1
#define CONFIG_MMC			1
#define CONFIG_JZ_MMC			1
#define CONFIG_JZ_MMC_SPLMSC 0
#define CONFIG_JZ_MMC_MSC0_PB 		1
#define CONFIG_MSC_DATA_WIDTH_4BIT
#endif  /* JZ_MMC_MSC0 */

#define CONFIG_SYS_CBSIZE 1024 /* Console I/O Buffer Size */
#define CONFIG_SYS_PBSIZE (CONFIG_SYS_CBSIZE + sizeof(CONFIG_SYS_PROMPT) + 16)

#define CONFIG_SYS_MONITOR_LEN		((256 * 1024) - CONFIG_SPL_PAD_TO)
#define CONFIG_SYS_MALLOC_LEN		(32 * 1024 * 1024)
#define CONFIG_SYS_BOOTPARAMS_LEN	(128 * 1024)

#define CONFIG_SYS_SDRAM_BASE		0x80000000 /* cached (KSEG0) address */
#define CONFIG_SYS_SDRAM_MAX_TOP	0x90000000 /* don't run into IO space */
#define CONFIG_SYS_INIT_SP_OFFSET	0x400000
#define CONFIG_SYS_LOAD_ADDR		0x88000000
#define CONFIG_SYS_MEMTEST_START	0x80000000
#define CONFIG_SYS_MEMTEST_END		0x84000000

#define CONFIG_SYS_TEXT_BASE		0x80100000
#define CONFIG_SYS_MONITOR_BASE		CONFIG_SYS_TEXT_BASE

/**
 * Environment
 */
#if defined(CONFIG_ENV_IS_IN_MMC)
#define CONFIG_CMD_SAVEENV
#define CONFIG_SYS_MMC_ENV_DEV		0
#define CONFIG_ENV_SIZE			(16 * 1024)
#undef CONFIG_SYS_MONITOR_LEN
#define CONFIG_SYS_MONITOR_LEN          ((512 * 1024) - (CONFIG_SYS_MMCSD_RAW_MODE_U_BOOT_SECTOR * 512) - CONFIG_ENV_SIZE)
#define CONFIG_ENV_OFFSET		(CONFIG_SYS_MONITOR_LEN + CONFIG_SYS_MMCSD_RAW_MODE_U_BOOT_SECTOR * 512)
#elif defined(CONFIG_ENV_IS_IN_SFC_NAND)
#define CONFIG_CMD_SAVEENV
#define CONFIG_SYS_REDUNDAND_ENVIRONMENT
#define CONFIG_ENV_SECT_SIZE 		(128 * 1024) /* the  nand block size is 128K */
#define CONFIG_ENV_SIZE         	CONFIG_ENV_SECT_SIZE
#undef CONFIG_SYS_MONITOR_LEN
#define CONFIG_SYS_MONITOR_LEN          ((512 * 1024) - CONFIG_UBOOT_OFFSET)
#define CONFIG_ENV_OFFSET_REDUND 	(CONFIG_SYS_MONITOR_LEN + CONFIG_UBOOT_OFFSET)
#define CONFIG_ENV_OFFSET       	(CONFIG_ENV_OFFSET_REDUND + CONFIG_ENV_SECT_SIZE)
#elif defined(CONFIG_ENV_IS_IN_SFC_NOR)
#define CONFIG_CMD_SAVEENV
#define CONFIG_ENV_SIZE			(16 * 1024)
#undef CONFIG_SYS_MONITOR_LEN
#define CONFIG_SYS_MONITOR_LEN		((256 * 1024) - CONFIG_UBOOT_OFFSET - CONFIG_ENV_SIZE)
#define CONFIG_ENV_OFFSET		(CONFIG_SYS_MONITOR_LEN + CONFIG_UBOOT_OFFSET)
#else
#define CONFIG_ENV_IS_NOWHERE
#define CONFIG_ENV_SIZE			(512)
#endif

/**
 * SPL configuration
 */
#define CONFIG_SPL
#define CONFIG_SPL_FRAMEWORK
#define CONFIG_SPL_NO_CPU_SUPPORT_CODE
#define CONFIG_SPL_START_S_PATH		"$(CPUDIR)/$(SOC)"
#ifdef CONFIG_SPL_NOR_SUPPORT
  #define CONFIG_SPL_LDSCRIPT           "$(CPUDIR)/$(SOC)/u-boot-nor-spl.lds"
#else
  #define CONFIG_SPL_LDSCRIPT		"$(CPUDIR)/$(SOC)/u-boot-spl.lds"
#endif

#define CONFIG_SPL_GPIO_SUPPORT
#define CONFIG_SPL_BOARD_INIT
#define CONFIG_SPL_LIBGENERIC_SUPPORT
#ifdef CONFIG_SPL_NOR_SUPPORT
  #define CONFIG_SPL_TEXT_BASE		0xba000000
  #define CONFIG_SYS_OS_BASE		0
  #define CONFIG_SYS_SPL_ARGS_ADDR	0
  #define CONFIG_SYS_FDT_BASE		0
  #define CONFIG_SPL_PAD_TO		32768
  #define CONFIG_SPL_MAX_SIZE		CONFIG_SPL_PAD_TO
  #define CONFIG_SYS_UBOOT_BASE		(CONFIG_SPL_TEXT_BASE + CONFIG_SPL_PAD_TO - 0x40)  /* 0x40 = sizeof (image_header) */
#elif defined(CONFIG_SPL_JZMMC_SUPPORT)
  #define CONFIG_SPL_TEXT_BASE		0x80001000
  #define CONFIG_SPL_MAX_SIZE		26624
  #define CONFIG_SPL_PAD_TO	        CONFIG_SPL_MAX_SIZE
  #define CONFIG_SYS_MMCSD_RAW_MODE_U_BOOT_SECTOR	 ((CONFIG_SPL_PAD_TO + (17 * 1024)) >> 9) /* 28KB + 17K offset */
#elif defined(CONFIG_SPL_SFC_SUPPORT)
  #define CONFIG_SPI_SPL_CHECK
  #define CONFIG_SPL_TEXT_BASE		0x80001000
  #define CONFIG_SPL_PAD_TO		27648
  #define CONFIG_SPL_MAX_SIZE		(26 * 1024)
  #define CONFIG_UBOOT_OFFSET           CONFIG_SPL_PAD_TO
  #define CONFIG_SPL_VERSION            1
#endif	/*CONFIG_SPL_NOR_SUPPORT*/


#ifdef CONFIG_MTD_SFCNOR

#ifndef CONFIG_SFC_NOR_RATE
#define CONFIG_SFC_NOR_RATE                150000000
#endif

#ifndef CONFIG_SFC_NOT_QUAD
#define CONFIG_SFC_QUAD
#endif

#define CONFIG_JZ_SFC
#define CONFIG_JZ_SFC_NOR
#define CONFIG_CMD_SFC_NOR
#define CONFIG_SPIFLASH_PART_OFFSET         (CONFIG_SPL_MAX_SIZE)
#define CONFIG_SPI_NORFLASH_PART_OFFSET     (CONFIG_SPIFLASH_PART_OFFSET + ((size_t)&((struct burner_params*)0)->norflash_partitions))
#define CONFIG_NOR_MAJOR_VERSION_NUMBER     1
#define CONFIG_NOR_MINOR_VERSION_NUMBER     0
#define CONFIG_NOR_REVERSION_NUMBER         0
#define CONFIG_NOR_VERSION     (CONFIG_NOR_MAJOR_VERSION_NUMBER | (CONFIG_NOR_MINOR_VERSION_NUMBER << 8) | (CONFIG_NOR_REVERSION_NUMBER <<16))
#ifdef CONFIG_ENV_IS_IN_SFC
    #define CONFIG_ENV_IS_IN_SFC_NOR
#endif

#define CONFIG_SYS_PROMPT_EX "-sfcnor# "

#endif /* CONFIG_MTD_SFCNOR */


#ifdef CONFIG_MTD_SFCNAND

#ifndef CONFIG_SFC_NAND_RATE
#define CONFIG_SFC_NAND_RATE        100000000
#endif

#define CONFIG_SPI_NAND_BPP        (2048 +64)    /*Bytes Per Page*/

#define CONFIG_SPI_NAND_PPB        (64)        /*Page Per Block*/

#define CONFIG_JZ_SFC
#define CONFIG_SYS_NAND_SELF_INIT
#define CONFIG_SPIFLASH_PART_OFFSET     (CONFIG_SPL_MAX_SIZE)

#define CONFIG_CMD_SFCNAND
#define CONFIG_CMD_NAND
#define CONFIG_SYS_MAX_NAND_DEVICE    1
#define CONFIG_SYS_NAND_BASE        0
#define CONFIG_MTD_DEVICE
#define CONFIG_CMD_UBI
#define CONFIG_CMD_UBIFS
#define CONFIG_CMD_MTDPARTS
#define CONFIG_MTD_PARTITIONS
#define MTDIDS_DEFAULT                  "nand0=nand"
#define MTDPARTS_DEFAULT                "mtdparts=nand:1M(boot),8M(kernel),64M(rootfs),-(data)"
#ifdef CONFIG_ENV_IS_IN_SFC
    #define CONFIG_ENV_IS_IN_SFC_NAND
#endif

#define CONFIG_SYS_PROMPT_EX "-sfcnand# "

#endif /* CONFIG_MTD_SFCNAND */

/**
 * MBR & GPT configuration
 */
#ifdef CONFIG_MBR_CREATOR
#define CONFIG_MBR_P0_OFF	64mb
#define CONFIG_MBR_P0_END	556mb
#define CONFIG_MBR_P0_TYPE 	linux

#define CONFIG_MBR_P1_OFF	580mb
#define CONFIG_MBR_P1_END 	1604mb
#define CONFIG_MBR_P1_TYPE 	linux

#define CONFIG_MBR_P2_OFF	28mb
#define CONFIG_MBR_P2_END	58mb
#define CONFIG_MBR_P2_TYPE 	linux

#define CONFIG_MBR_P3_OFF	1609mb
#define CONFIG_MBR_P3_END	7800mb
#define CONFIG_MBR_P3_TYPE 	fat
#else
#define CONFIG_GPT_TABLE_PATH	"$(TOPDIR)/board/$(BOARDDIR)"
#endif
