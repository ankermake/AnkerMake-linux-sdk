

#define CONFIG_SYS_APLL_FREQ		1008000000	/*If APLL not use mast be set 0*/
#define ARG_LPJ         "lpj=5000000"

#define CONFIG_SYS_MPLL_FREQ		600000000	/*If MPLL not use mast be set 0*/
#define CONFIG_CPU_SEL_PLL		APLL
#define CONFIG_DDR_SEL_PLL		MPLL
#define CONFIG_SYS_CPU_FREQ		1008000000
#define CONFIG_SYS_MEM_FREQ		200000000

#if defined(CONFIG_SYS_UART2_PA)
#elif defined(CONFIG_SYS_UART2_PD)
#else
#define CONFIG_SYS_UART2_PC
#endif


#define CONFIG_ARG_EXTRA ""
#define CONFIG_SPL_OS_BOOT
#define CONFIG_SPL_OTA_NAME       "ota"
#define CONFIG_ROOTFS_SQUASHFS
#define CONFIG_ROOTFS_DEV "root=/dev/mtdblock_bbt_ro2"
#define CONFIG_ROOTFS_INITRC "init=/linuxrc"
#define CONFIG_ROOTFS2_SQUASHFS
#define CONFIG_ROOTFS2_DEV "root=/dev/mtdblock_bbt_ro4"
#define CONFIG_ROOTFS2_INITRC "init=/linuxrc"

#include "ingenic_config/config_x1000_def.h"
#include "ingenic_config/config_uart.h"
#include "ingenic_config/config_rootfs.h"
#include "ingenic_config/config_x1000.h"
#include "ingenic_config/config_boot_args.h"
