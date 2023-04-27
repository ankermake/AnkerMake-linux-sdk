#ifndef __CONFIG_X1021_H__
#define __CONFIG_X1021_H__

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

#define CONFIG_SYS_VPLL_FREQ        912000000 /*If VPLL not use mast be set 0*/
#define CONFIG_SYS_VPLL_MNOD        ((75 << 20) | (1 << 14) | (1 << 11) | (2<<5))

#include "ingenic_config/config_x1021_def.h"
#include "ingenic_config/config_uart.h"
#include "ingenic_config/config_rootfs.h"
#include "ingenic_config/config_x1021.h"
#include "ingenic_config/config_boot_args.h"

#endif /*__CONFIG_X1021_H__*/