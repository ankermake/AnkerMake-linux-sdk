

#if defined(CONFIG_RMEM_16M)
#define ARG_MEM "mem=16M@0x0 rmem=16M@0x1000000"
#elif defined(CONFIG_RMEM_6M)
#define ARG_MEM "mem=26M@0x0  rmem=6M@0x1a00000"
#elif defined(CONFIG_RMEM_7M)
#define ARG_MEM "mem=25M@0x0  rmem=7M@0x1900000"
#else
#define ARG_MEM "mem=32M@0x0"
#endif

/*#define CONFIG_DDR_TEST*/
#define CONFIG_DDR_PARAMS_CREATOR
#define CONFIG_DDR_HOST_CC
#define CONFIG_DDR_TYPE_LPDDR
#define CONFIG_DDR_CS0			1	/* 1-connected, 0-disconnected */
#define CONFIG_DDR_CS1			0	/* 1-connected, 0-disconnected */
#define CONFIG_DDR_DW32			0	/* 1-32bit-width, 0-16bit-width */
#define CONFIG_DDR_tREFI	    DDR__ns(7800)
/*
   Output Drive Strength: Controls the output drive strength. Valid values are:
   000 = Full strength driver
   001 = Half strength driver
   110 = Quarter strength driver
   111 = Octant strength driver
   100 = Three-quarters strength driver
 */
#define CONFIG_DDR_DRIVER_STRENGTH             4

/*#define CONFIG_MDDR_JSD12164PAI_KGD*/	    /*DDR 64M param file*/

#define  CONFIG_DDR_64M     64
#define  CONFIG_DDR_32M     32
#define CONFIG_MDDR_EMD56164PC_50I

#if defined(CONFIG_SPL_SFC_NOR) || defined(CONFIG_SPL_SFC_NAND)
#define CONFIG_SPL_SFC_SUPPORT
#define CONFIG_SPL_VERSION	1
#endif

#define PARTITION_NUM 10

/**
 * Boot command definitions.
 */
#define CONFIG_BOOTDELAY 0

/* CLK CGU */
#define  CGU_CLK_SRC {				\
		{OTG, EXCLK},			\
		{LCD, MPLL},			\
		{MSC, MPLL},			\
		{SFC, MPLL},			\
		{CIM, MPLL},			\
		{PCM, MPLL},			\
		{I2S, EXCLK},			\
		{SRC_EOF,SRC_EOF}		\
	}

/* GPIO */
#define CONFIG_JZ_GPIO

/**
 * Command configuration.
 */
#define CONFIG_CMD_BOOTD	/* bootd			*/
#define CONFIG_CMD_CONSOLE	/* coninfo			*/
#define CONFIG_CMD_ECHO		/* echo arguments		*/
#define CONFIG_CMD_EXT4 	/* ext4 support			*/
#define CONFIG_CMD_FAT		/* FAT support			*/
#define CONFIG_CMD_MEMORY	/* md mm nm mw cp cmp crc base loop mtest */
#define CONFIG_CMD_MISC		/* Misc functions like sleep etc*/
#define CONFIG_CMD_RUN		/* run command in env variable	*/
#define CONFIG_CMD_SAVEENV	/* saveenv			*/
#define CONFIG_CMD_SETGETDCR	/* DCR support on 4xx		*/
#define CONFIG_CMD_SOURCE	/* "source" command support	*/
#define CONFIG_CMD_GETTIME
#define CONFIG_CMD_UNZIP        /* unzip from memory to memory  */
#define CONFIG_CMD_DHCP
#define CONFIG_CMD_NET     /* networking support*/
#define CONFIG_CMD_PING

#ifdef CONFIG_CMD_EFUSE
#define	CONFIG_X1000_EFUSE
#define	CONFIG_JZ_EFUSE
#define CONFIG_EFUSE_GPIO	GPIO_PB(27)
#define CONFIG_EFUSE_LEVEL	0
#endif

#ifndef CONFIG_SPL_BUILD
#define CONFIG_USE_ARCH_MEMSET
#define CONFIG_USE_ARCH_MEMCPY
#endif

/* MMC */
/* #define CONFIG_CMD_MMC */
/*#define CONFIG_MMC_TRACE*/

#ifdef CONFIG_CMD_MMC
#define CONFIG_GENERIC_MMC		1
#define CONFIG_MMC			1
#define CONFIG_JZ_MMC			1

#ifdef CONFIG_JZ_MMC_MSC0
#define CONFIG_JZ_MMC_SPLMSC 0
#define CONFIG_JZ_MMC_MSC0_PA_4BIT 1
/* #define CONFIG_JZ_MMC_MSC0_PA_8BIT 1 */
/* #define CONFIG_MSC_DATA_WIDTH_8BIT */
#define CONFIG_MSC_DATA_WIDTH_4BIT
/* #define CONFIG_MSC_DATA_WIDTH_1BIT */
#endif

#ifdef CONFIG_JZ_MMC_MSC1
/*#define CONFIG_JZ_MMC_SPLMSC 1*/
#define CONFIG_JZ_MMC_MSC1_PC 1
/* #define CONFIG_MSC_DATA_WIDTH_8BIT */
#define CONFIG_MSC_DATA_WIDTH_4BIT
/* #define CONFIG_MSC_DATA_WIDTH_1BIT */
#endif
#endif

/**
 * Serial download configuration
 */
#define CONFIG_LOADS_ECHO	1	/* echo on for serial download */

/**
 * Miscellaneous configurable options
 */
#define CONFIG_DOS_PARTITION

#define CONFIG_LZO
#define CONFIG_RBTREE

#define CONFIG_SKIP_LOWLEVEL_INIT
#define CONFIG_BOARD_EARLY_INIT_F
#define CONFIG_SYS_NO_FLASH
#define CONFIG_SYS_FLASH_BASE	0 /* init flash_base as 0 */
#define CONFIG_ENV_OVERWRITE
#define CONFIG_MISC_INIT_R 1

#define CONFIG_BOOTP_MASK	(CONFIG_BOOTP_DEFAUL)

#define CONFIG_SYS_MAXARGS 16
#define CONFIG_SYS_LONGHELP

#if defined(CONFIG_SPL_JZMMC_SUPPORT)
	#if	defined(CONFIG_JZ_MMC_MSC0)
	#define CONFIG_SYS_PROMPT CONFIG_SYS_BOARD "-msc0# "
	#else
	#define CONFIG_SYS_PROMPT CONFIG_SYS_BOARD "-msc1# "
	#endif
#elif defined(CONFIG_SPL_NOR_SUPPORT)
#define CONFIG_SYS_PROMPT CONFIG_SYS_BOARD "-nor# "
#elif defined(CONFIG_SPL_SFC_SUPPORT)
	#if defined(CONFIG_SPL_SFC_NOR)
		#define CONFIG_SYS_PROMPT CONFIG_SYS_BOARD "-sfcnor# "
	#else  /* CONFIG_SPL_SFC_NAND */
		#define CONFIG_SYS_PROMPT CONFIG_SYS_BOARD "-sfcnand# "
	#endif
#elif defined(CONFIG_SPL_SPI_NAND)
		#define CONFIG_SYS_PROMPT CONFIG_SYS_BOARD "-spinand# "
#endif

#define CONFIG_SYS_CBSIZE 1024 /* Console I/O Buffer Size */
#define CONFIG_SYS_PBSIZE (CONFIG_SYS_CBSIZE + sizeof(CONFIG_SYS_PROMPT) + 16)

#if defined(CONFIG_SUPPORT_EMMC_BOOT)
#define CONFIG_SYS_MONITOR_LEN		(384 * 1024)
#else
#define CONFIG_SYS_MONITOR_LEN		(512 << 10)
#endif

#define CONFIG_SYS_MALLOC_LEN		(8 * 1024 * 1024)
#define CONFIG_SYS_BOOTPARAMS_LEN	(128 * 1024)

#define CONFIG_SYS_SDRAM_BASE		0x80000000 /* cached (KSEG0) address */
#define CONFIG_SYS_SDRAM_MAX_TOP	0x90000000 /* don't run into IO space */
#define CONFIG_SYS_INIT_SP_OFFSET	0x400000
#define CONFIG_SYS_LOAD_ADDR		0x88000000
#define CONFIG_SYS_MEMTEST_START	0x80000000
#define CONFIG_SYS_MEMTEST_END		0x88000000
#define CONFIG_SYS_TEXT_BASE		0x80100000
#define CONFIG_SYS_MONITOR_BASE		CONFIG_SYS_TEXT_BASE

/**
 * Environment
 */
#if defined(CONFIG_ENV_IS_IN_MMC)
#define CONFIG_SYS_MMC_ENV_DEV		0
#define CONFIG_ENV_SIZE			(32 << 10)
#define CONFIG_ENV_OFFSET		(CONFIG_SYS_MONITOR_LEN + CONFIG_SYS_MMCSD_RAW_MODE_U_BOOT_SECTOR * 512)
#elif defined(CONFIG_ENV_IS_IN_SFC)
#define CONFIG_ENV_SIZE			(4 << 10)
#define CONFIG_ENV_OFFSET		0x3f000 /*write nor flash 252k address*/
#define CONFIG_CMD_SAVEENV
#endif

/**
 * SPL configuration
 */
#define CONFIG_SPL
#define CONFIG_SPL_FRAMEWORK
#define CONFIG_SPL_NO_CPU_SUPPORT_CODE
#define CONFIG_SPL_START_S_PATH		"$(CPUDIR)/$(SOC)"
#ifdef CONFIG_SPL_NOR_SUPPORT
 #define CONFIG_SPL_LDSCRIPT             "$(CPUDIR)/$(SOC)/u-boot-nor-spl.lds"
#else
 #define CONFIG_SPL_LDSCRIPT		"$(CPUDIR)/$(SOC)/u-boot-spl.lds"
#endif
#define CONFIG_SYS_MMCSD_RAW_MODE_U_BOOT_SECTOR	0x3A /* 12KB+17K offset */
#define CONFIG_SYS_U_BOOT_MAX_SIZE_SECTORS	0x200 /* 256 KB */
#define CONFIG_SPL_GPIO_SUPPORT
#define CONFIG_SPL_BOARD_INIT
#define CONFIG_SPL_LIBGENERIC_SUPPORT
#if defined(CONFIG_SPL_NOR_SUPPORT)
 #define CONFIG_SPL_TEXT_BASE		0xba000000
 #define CONFIG_SYS_UBOOT_BASE		(CONFIG_SPL_TEXT_BASE + CONFIG_SPL_PAD_TO - 0x40)
					/* 0x40 = sizeof (image_header)*/
 #define CONFIG_SYS_OS_BASE		0
 #define CONFIG_SYS_SPL_ARGS_ADDR	0
 #define CONFIG_SYS_FDT_BASE		0
 #define CONFIG_SPL_PAD_TO		32768
 #define CONFIG_SPL_MAX_SIZE		(32 * 1024)
#elif defined(CONFIG_SPL_JZMMC_SUPPORT)
 #define CONFIG_SPL_PAD_TO		12288  /* spl size */
 #define CONFIG_SPL_TEXT_BASE		0xf4001000
 #define CONFIG_SPL_MAX_SIZE		(12 * 1024)
#elif defined(CONFIG_SPL_SFC_SUPPORT)
  #define CONFIG_UBOOT_OFFSET             (4<<12)
  #define CONFIG_JZ_SFC_PA_6BIT
 #ifdef	CONFIG_SPL_SFC_NAND
  #define CONFIG_SFC_NAND_RATE    100000000
  #define CONFIG_SPIFLASH_PART_OFFSET     0x3c00
  #define CONFIG_SPI_NAND_BPP			(2048 +64)		/*Bytes Per Page*/
  #define CONFIG_SPI_NAND_PPB			(64)		/*Page Per Block*/
  #define CONFIG_SPL_TEXT_BASE		0xf4001000
  #define CONFIG_SPL_MAX_SIZE		(12 * 1024)
  #define CONFIG_SPL_PAD_TO		16384
  #define CONFIG_JZ_SFC
  #define CONFIG_CMD_SFCNAND
  #define CONFIG_CMD_NAND
  #define CONFIG_SPI_SPL_CHECK
  #define CONFIG_SYS_MAX_NAND_DEVICE	1
  #define CONFIG_SYS_NAND_BASE    0xb3441000

  #define CONFIG_MTD_DEVICE
  #define CONFIG_CMD_UBI
  #define CONFIG_CMD_UBIFS
  #define CONFIG_CMD_MTDPARTS
  #define CONFIG_MTD_PARTITIONS
  #define MTDIDS_DEFAULT                  "nand0=nand"
  #define MTDPARTS_DEFAULT                "mtdparts=nand:1M(boot),8M(kernel),40M(rootfs),-(data)"


/*SFCNAND env*/
/* spi nand environment */
  #define CONFIG_SYS_REDUNDAND_ENVIRONMENT
  #define CONFIG_ENV_SECT_SIZE 0x20000 /* 128K*/
  #define SPI_NAND_BLK            0x20000 /*the spi nand block size */
  #define CONFIG_ENV_SIZE         SPI_NAND_BLK /* uboot is 1M but the last block size is the env*/
  #define CONFIG_ENV_OFFSET       0xc0000 /* offset is 768k */
  #define CONFIG_ENV_OFFSET_REDUND (CONFIG_ENV_OFFSET + CONFIG_ENV_SIZE)
  #define CONFIG_ENV_IS_IN_SFC_NAND


 #else
  #define CONFIG_SPI_SPL_CHECK
  #define CONFIG_SPL_TEXT_BASE		0xf4001000
  #define CONFIG_SPL_MAX_SIZE		(12 * 1024)
  #define CONFIG_SPL_PAD_TO		16384
  #define CONFIG_CMD_SFC_NOR
 #endif
#endif

#ifdef CONFIG_SPL_SPI_NAND
#define CONFIG_SPIFLASH_PART_OFFSET     0x3c00
#define CONFIG_UBOOT_OFFSET             (4<<12)
#define CONFIG_SPL_TEXT_BASE		0xf4001000
#define CONFIG_SPL_MAX_SIZE		(12 * 1024)
#define CONFIG_SPL_PAD_TO		16384
#define CONFIG_SPI_NAND_BPP			(2048 +128)		/*Bytes Per Page*/
#define CONFIG_SPI_NAND_PPB			(64)		/*Page Per Block*/
#define CONFIG_SPI_SPL_CHECK
#define CONFIG_JZ_SPI
#define CONFIG_JZ_SSI0_PA
#define CONFIG_MTD_SPINAND
#define CONFIG_CMD_SPINAND
#define CONFIG_SPI_FLASH
#define CONFIG_MTD_PARTITIONS
#define CONFIG_CMD_MTDPARTS
#define CONFIG_ENV_IS_IN_SPI_NAND

/* spi nand environment */
#define CONFIG_SYS_REDUNDAND_ENVIRONMENT
#define CONFIG_ENV_SECT_SIZE 0x20000 /* 128K*/
#define SPI_NAND_BLK		0x20000 /*the spi nand block size */
#define CONFIG_ENV_SIZE		SPI_NAND_BLK /* uboot is 1M but the last block size is the env*/
#define CONFIG_ENV_OFFSET	0xc0000 /* offset is 768k */
#define CONFIG_ENV_OFFSET_REDUND (CONFIG_ENV_OFFSET + CONFIG_ENV_SIZE)


#define CONFIG_CMD_NAND /*use the mtd and the function do_nand() */
#define CONFIG_SYS_MAX_NAND_DEVICE  1

#endif /*CONFIG_SPL_SPI_NAND*/

#ifdef CONFIG_CMD_SFC_NOR
#define CONFIG_JZ_SFC
#define CONFIG_JZ_SFC_NOR
#define CONFIG_SFC_QUAD
#define CONFIG_SFC_NOR_RATE    150000000
#endif

#ifdef CONFIG_JZ_SFC_NOR
#define CONFIG_SPIFLASH_PART_OFFSET     0x3c00
#define CONFIG_SPI_NORFLASH_PART_OFFSET     0x3c74
#define CONFIG_NOR_MAJOR_VERSION_NUMBER     1
#define CONFIG_NOR_MINOR_VERSION_NUMBER     0
#define CONFIG_NOR_REVERSION_NUMBER     0
#define CONFIG_NOR_VERSION     (CONFIG_NOR_MAJOR_VERSION_NUMBER | (CONFIG_NOR_MINOR_VERSION_NUMBER << 8) | (CONFIG_NOR_REVERSION_NUMBER <<16))
#endif
/**
 * MBR configuration
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

/*
* MTD support
*/
#define CONFIG_SYS_NAND_SELF_INIT
