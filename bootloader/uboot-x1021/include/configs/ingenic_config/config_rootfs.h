
#if defined(CONFIG_ROOTFS_UBI)
#define ARG_ROOTFS_TYPE "rootfstype=ubifs ro"
#elif defined(CONFIG_ROOTFS_SQUASHFS)
#define ARG_ROOTFS_TYPE "rootfstype=squashfs ro"
#elif defined(CONFIG_ROOTFS_RAMDISK)
#define ARG_ROOTFS_TYPE "rw"
#else
#error "please add more define here"
#endif

#define ARG_ROOTFS CONFIG_ROOTFS_INITRC" "CONFIG_ROOTFS_DEV" "ARG_ROOTFS_TYPE

#ifdef CONFIG_ROOTFS2_DEV
#if defined(CONFIG_ROOTFS2_UBI)
#define ARG_ROOTFS2_TYPE "rootfstype=ubifs ro"
#elif defined(CONFIG_ROOTFS2_SQUASHFS)
#define ARG_ROOTFS2_TYPE "rootfstype=squashfs ro"
#elif defined(CONFIG_ROOTFS2_RAMDISK)
#define ARG_ROOTFS2_TYPE "rw"
#else
#error "please add more define here"
#endif

#define ARG_ROOTFS2 CONFIG_ROOTFS2_INITRC " " CONFIG_ROOTFS2_DEV " " ARG_ROOTFS2_TYPE

#endif /* CONFIG_ROOTFS_DEV2 */
