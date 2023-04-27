#ifndef CTS_SYSFS_H
#define CTS_SYSFS_H

#include "cts_config.h"

struct device;

#ifdef CONFIG_CTS_SYSFS
int cts_sysfs_add_device(struct device *dev);
void cts_sysfs_remove_device(struct device *dev);
#else /* CONFIG_CTS_SYSFS */
static inline int cts_sysfs_add_device(struct device *dev) {return -ENOTSUPP; }
static inline void cts_sysfs_remove_device(struct device *dev) {}
#endif /* CONFIG_CTS_SYSFS */

#endif /* CTS_SYSFS_H */

