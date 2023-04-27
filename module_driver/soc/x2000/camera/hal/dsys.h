/*
 * Copyright (C) 2020 Ingenic Semiconductor Co., Ltd.
 *
 * debug sys
 *
 */
#ifndef __DSYS_H__
#define __DSYS_H__

#include <linux/kobject.h>
#include <linux/sysfs.h>
#include <linux/device.h>



#define SOC_CAMERA_DEBUG



#define DSYSFS_DEV_ATTR(_name, _mode, _show, _store) \
    struct device_attribute dsysfs_dev_attr_##_name = __ATTR(_name, _mode, _show, _store)
#define DSYSFS_BIN_ATTR(_name, _mode, _read, _write, _size) \
    struct bin_attribute dsysfs_bin_attr_##_name = __BIN_ATTR(_name, _mode, _read, _write, _size)



int dsysfs_create_group(struct kobject *kobj, struct kobject *parent, const char *dname, const struct attribute_group *grp);
void dsysfs_remove_group(struct kobject *kobj, const struct attribute_group *grp);
int dsysfs_create_dir(struct kobject *kobj, struct kobject *parent, const char *dname);
void dsysfs_remove_dir(struct kobject *kobj);



#endif /* __DSYS_H__ */
