#ifndef CTS_CONFIG_H
#define CTS_CONFIG_H

/** Driver version */
#define CFG_CTS_DRIVER_MAJOR_VERSION        1
#define CFG_CTS_DRIVER_MINOR_VERSION        1
#define CFG_CTS_DRIVER_PATCH_VERSION        0

#define CFG_CTS_DRIVER_VERSION              "v1.1.00"

#define CFG_CTS_MAX_I2C_XFER_SIZE			48u

#define CFG_CTS_MAX_TOUCH				TS_MAX_FINGER

#define CFG_CTS_GET_DATA_RETRY             3
#define CFG_CTS_GET_FLAG_RETRY             20
#define CFG_CTS_GET_FLAG_DELAY             3

#define CFG_CTS_FIRMWARE_IN_FS
#ifdef CFG_CTS_FIRMWARE_IN_FS
#define CFG_CTS_FIRMWARE_FILEPATH       "/etc/firmware/ICNT8918.bin"
#endif

//#define CONFIG_PROC_FS
#ifdef CONFIG_PROC_FS
    /* Proc FS for backward compatibility for APK tool com.ICN85xx */
    #define CONFIG_CTS_LEGACY_TOOL
#endif /* CONFIG_PROC_FS */
#ifdef CONFIG_SYSFS
    /* Sys FS for gesture report, debug feature etc. */
#define CONFIG_CTS_SYSFS
#endif				/* CONFIG_SYSFS */

#define CFG_CTS_MAX_TOUCH_NUM               (2)

/* Virtual key support */
/*#define CONFIG_CTS_VIRTUALKEY*/
#ifdef CONFIG_CTS_VIRTUALKEY
#define CFG_CTS_MAX_VKEY_NUM            (4)
#define CFG_CTS_NUM_VKEY                (3)
#define CFG_CTS_VKEY_KEYCODES           {KEY_BACK, KEY_HOME, KEY_MENU}
#endif				/* CONFIG_CTS_VIRTUALKEY */

/* Gesture wakeup */
//#define CFG_CTS_GESTURE
#ifdef CFG_CTS_GESTURE
#define GESTURE_D_TAP                       0x02
#define GESTURE_UP                          0x03
#define GESTURE_DOWN                        0x04



#endif				/* CFG_CTS_GESTURE */

#define CONFIG_CTS_SLOTPROTOCOL

#ifdef CONFIG_CTS_LEGACY_TOOL
    #define CFG_CTS_TOOL_PROC_FILENAME      "icn85xx_tool"
#endif /* CONFIG_CTS_LEGACY_TOOL */
#if 1 //def CONFIG_ARCH_MSM
#include "cts_plat_qcom_config.h"
#endif /* CONFIG_ARCH_MSM */

#endif /* CTS_CONFIG_H */

