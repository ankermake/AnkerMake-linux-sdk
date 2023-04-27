/*
 * Copyright (C) 2020 Ingenic Semiconductor Co., Ltd.
 *
 * debug sys
 *
 */
#include <linux/slab.h>
#include "dsys.h"


static void dynamic_kobj_release(struct kobject *kobj)
{
	pr_debug("kobject: (%p): %s\n", kobj, __func__);
	kfree(kobj);
}

static struct kobj_type dynamic_kobj_ktype = {
	.release	= dynamic_kobj_release,
	.sysfs_ops	= &kobj_sysfs_ops,
};


int dsysfs_create_group(struct kobject *kobj, struct kobject *parent, const char *dname, const struct attribute_group *grp)
{
    int ret;

    ret = kobject_init_and_add(kobj, &dynamic_kobj_ktype, parent, dname);
    if (ret) {
        printk("kobject init and add failed: %d\n", ret);
        return ret;
    }

    ret = sysfs_create_group(kobj, grp);
    if (ret) {
        printk("sysfs create group failed: %d\n", ret);
        kobject_del(kobj);
        return ret;
    }

    return 0;
}

void dsysfs_remove_group(struct kobject *kobj, const struct attribute_group *grp)
{
    sysfs_remove_group(kobj, grp);
    kobject_del(kobj);
}

int dsysfs_create_dir(struct kobject *kobj, struct kobject *parent, const char *dname)
{
    int ret;

    ret = kobject_init_and_add(kobj, &dynamic_kobj_ktype, parent, dname);
    if (ret) {
        printk("kobject init and add failed: %d\n", ret);
        return ret;
    }

    return 0;
}

void dsysfs_remove_dir(struct kobject *kobj)
{
    kobject_del(kobj);
}

