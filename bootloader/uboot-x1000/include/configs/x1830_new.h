
#ifndef __CONFIG_X1830_H__
#define __CONFIG_X1830_H__

#define CONFIG_ARG_EXTRA ""
#define CONFIG_SPL_OS_BOOT
#define CONFIG_SPL_OTA_NAME       "ota"

#ifdef CONFIG_SPL_JZMMC_SUPPORT
#define CONFIG_ROOTFS_SQUASHFS
#define CONFIG_ROOTFS_DEV "root=/dev/mmcblk0p3 rootwait"
#define CONFIG_ROOTFS_INITRC "init=/linuxrc"
#define CONFIG_ROOTFS2_SQUASHFS
#define CONFIG_ROOTFS2_DEV "root=/dev/mmcblk0p5 rootwait"
#define CONFIG_ROOTFS2_INITRC "init=/linuxrc"
#else
#define CONFIG_ROOTFS_SQUASHFS
#define CONFIG_ROOTFS_DEV "root=/dev/mtdblock_bbt_ro2"
#define CONFIG_ROOTFS_INITRC "init=/linuxrc"
#define CONFIG_ROOTFS2_SQUASHFS
#define CONFIG_ROOTFS2_DEV "root=/dev/mtdblock_bbt_ro4"
#define CONFIG_ROOTFS2_INITRC "init=/linuxrc"
#endif

#define CONFIG_SYS_VPLL_FREQ        1200000000    /*If VPLL not use mast be set 0*/
#define CONFIG_SYS_EPLL_MNOD        ((197 << 20) | (3 << 14) | (1 << 11) | (1<<5))
#define CONFIG_SYS_EPLL_FREQ        1188000000

#include "ingenic_config/config_x1830_def.h"
#include "ingenic_config/config_uart.h"
#include "ingenic_config/config_rootfs.h"
#include "ingenic_config/config_x1830.h"
#include "ingenic_config/config_boot_args.h"

/*#define CONFIG_DDR_TEST*/
/*#define CONFIG_DDR_TEST_CPU*/
/*#define CONFIG_DDR_AUTO_SELF_REFRESH*/
/*#define CONFIG_UPVOLTAGE            GPIO_PB(13)*/
/* #define CONFIG_DDR_AUTO_REFRESH_TEST */

#endif /*__CONFIG_X1830_H__*/
