
#ifdef CONFIG_ARG_QUIET
#define ARGS_QUIET "quiet"
#else
#define ARGS_QUIET ""
#endif

#define CONFIG_COMMON_ARGS ARGS_CONSOLE " " \
                           ARG_MEM " " \
                           ARG_LPJ " " \
                           ARGS_QUIET " " \
                           CONFIG_ARG_EXTRA

#define CONFIG_BOOTARGS           ARG_ROOTFS " " CONFIG_COMMON_ARGS

#define CONFIG_SPL_BOOTARGS       ARG_ROOTFS " " CONFIG_COMMON_ARGS
#define CONFIG_SPL_OS_NAME        "kernel"
#define CONFIG_SYS_SPL_ARGS_ADDR  CONFIG_SPL_BOOTARGS

#define CONFIG_SPL_BOOTARGS2      ARG_ROOTFS2 " " CONFIG_COMMON_ARGS
#define CONFIG_SPL_OS_NAME2       "kernel2"
#define CONFIG_SYS_SPL_ARGS_ADDR2 CONFIG_SPL_BOOTARGS2
