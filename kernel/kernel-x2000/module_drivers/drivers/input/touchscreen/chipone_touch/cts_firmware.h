#ifndef CTS_FIRMWARE_H
#define CTS_FIRMWARE_H

#include "cts_config.h"
#include <linux/firmware.h>
#include "cts_efctrl.h"


struct cts_firmware {
	const char *name;	/* MUST set to non-NULL if driver builtin firmware */
	u16 hwid;
	u16 fwid;
	u8 *data;
	size_t size;
    u16 ver_offset;
    bool is_fw_in_fs;
	const struct firmware *fw;
};

#define FIRMWARE_VERSION_OFFSET     0x114
#define FIRMWARE_VERSION(firmware)  \
	get_unaligned_le16((firmware)->data + FIRMWARE_VERSION_OFFSET)

enum e_upgrade_err_type {
	R_OK = 0,
	R_FILE_ERR,
	R_STATE_ERR,
	R_ERASE_ERR,
	R_PROGRAM_ERR,
	R_VERIFY_ERR,
};

struct cts_device;

extern ssize_t cts_file_write(struct file *file, const char *buf, size_t count,
                             loff_t pos);
extern int cts_file_read(struct file *file, loff_t offset,
                 char *addr, unsigned long count);


extern const struct cts_firmware *cts_request_firmware(const struct cts_device
						       *cts_dev, u32 hwid,
						       u16 fwid,
						       u16 device_fw_ver);
extern void cts_release_firmware(const struct cts_firmware *firmware);

extern int cts_update_firmware(struct cts_device *cts_dev,
			       const struct cts_firmware *firmware,
			       bool to_flash);
extern bool cts_is_firmware_updating(const struct cts_device *cts_dev);







extern bool cts_is_firmware_updating(const struct cts_device *cts_dev);

extern int cts_update_firmware(struct cts_device *cts_dev,
        const struct cts_firmware *firmware, bool to_flash);

#ifdef CFG_CTS_FIRMWARE_IN_FS
extern const struct cts_firmware *cts_request_firmware_from_fs(
					const struct cts_device *cts_dev,
					const char *filepath);
extern int cts_update_firmware_from_file(struct cts_device *cts_dev,
					const char *filepath, bool to_flash);
extern const struct cts_firmware *cts_request_newer_firmware_from_fs(
					const struct cts_device *cts_dev,
					const char *filepath,
					u16 curr_version);

#else				/* CFG_CTS_FIRMWARE_IN_FS */
static inline const struct cts_firmware *cts_request_firmware_from_fs(
		const struct cts_device *cts_dev, const char *filepath)
{
	return NULL;
}

static inline int cts_update_firmware_from_file(struct cts_device *cts_dev,
					const char *filepath, bool to_flash)
{
	return -ENOTSUPP;
}
#endif				/* CFG_CTS_FIRMWARE_IN_FS */



extern int icnt8918_fw_update(struct cts_device *cts_dev,
        const struct cts_firmware *firmware,bool to_flash);

#endif /* CTS_FIRMWARE_H */

