#ifndef __CONFIG_X1520_H__
#define __CONFIG_X1520_H__


#define CONFIG_ARG_EXTRA ""
#define CONFIG_SPL_OS_BOOT
#define CONFIG_SPL_OTA_NAME       "ota"
#define CONFIG_ROOTFS_SQUASHFS
#define CONFIG_ROOTFS_DEV "root=/dev/mtdblock_bbt_ro2"
#define CONFIG_ROOTFS_INITRC "init=/linuxrc"
#define CONFIG_ROOTFS2_SQUASHFS
#define CONFIG_ROOTFS2_DEV "root=/dev/mtdblock_bbt_ro4"
#define CONFIG_ROOTFS2_INITRC "init=/linuxrc"

#include "ingenic_config/config_x1520_def.h"
#include "ingenic_config/config_uart.h"
#include "ingenic_config/config_rootfs.h"
#include "ingenic_config/config_x1520.h"
#include "ingenic_config/config_boot_args.h"

/*#define CONFIG_DDR_TEST_CPU
#define CONFIG_DDR_TEST*/


#endif /*__CONFIG_ISVP_H__*/
