#ifdef CONFIG_SPL_RTOS_OTA_BOOT

#ifndef CONFIG_RTOS_OTA_TAG_OFFSET
#define CONFIG_RTOS_OTA_TAG_OFFSET (1024 * 1024)
#endif

#ifndef CONFIG_RTOS_OTA_OS0_OFFSET
#define CONFIG_RTOS_OTA_OS0_OFFSET (2048 * 1024)
#endif

#ifndef CONFIG_RTOS_OTA_OS1_OFFSET
#define CONFIG_RTOS_OTA_OS1_OFFSET (4096 * 1024)
#endif

#endif /* CONFIG_SPL_RTOS_OTA_BOOT */
