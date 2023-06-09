/*
 * A simple sysfs interface for the generic PWM framework
 *
 * Copyright (C) 2013 H Hartley Sweeten <hsweeten@visionengravers.com>
 *
 * Based on previous work by Lars Poeschel <poeschel@lemonage.de>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2, or (at your option)
 * any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 */

#include <linux/device.h>
#include <linux/mutex.h>
#include <linux/err.h>
#include <linux/slab.h>
#include <linux/kdev_t.h>
#include <linux/pwm.h>

/*
 *
 * pwm sysfs usage:
 *   First, set CONFIG_PWM_SYSFS, build linux kernel.
 *
 * After target boot up, check pwm sysfs interface
 *   # ls /sys/class/pwm/pwmchip0/
 *     device     export     npwm       power      subsystem  uevent     unexport
 *
 * Request pwm channel 1
 *   # echo 1 > /sys/class/pwm/pwmchip0/export
 *   # ls /sys/class/pwm/pwmchip0/pwm1/
 *     duty_ns  enable      period_ns      polarity    power       uevent
 *
 *   # echo 1000000 > /sys/class/pwm/pwmchip0/pwm1/period_ns
 *   # echo 500000 > /sys/class/pwm/pwmchip0/pwm1/duty_ns
 *   # echo 1 > /sys/class/pwm/pwmchip0/pwm1/enable
 *
 */




/***************** kernel-4.4_x2000/include/linux/sysfs.h *****************/
#include <linux/stat.h>

#define __ATTR_WO(_name) {						\
	.attr	= { .name = __stringify(_name), .mode = S_IWUSR },	\
	.store	= _name##_store,					\
}

#define DEVICE_ATTR_WO(_name) \
	struct device_attribute dev_attr_##_name = __ATTR_WO(_name)


#define __ATTRIBUTE_GROUPS(_name)				\
static const struct attribute_group *_name##_groups[] = {	\
	&_name##_group,						\
	NULL,							\
}

#define ATTRIBUTE_GROUPS(_name)					\
static const struct attribute_group _name##_group = {		\
	.attrs = _name##_attrs,					\
};								\
__ATTRIBUTE_GROUPS(_name)

/***************** kernel-4.4_x2000/include/linux/sysfs.h *****************/




struct pwm_export {
	struct device child;
	struct pwm_device *pwm;
};

static struct pwm_export *child_to_pwm_export(struct device *child)
{
	return container_of(child, struct pwm_export, child);
}

static struct pwm_device *child_to_pwm_device(struct device *child)
{
	struct pwm_export *export = child_to_pwm_export(child);

	return export->pwm;
}

static ssize_t period_ns_show(struct device *child,
			   struct device_attribute *attr,
			   char *buf)
{
	const struct pwm_device *pwm = child_to_pwm_device(child);

	return sprintf(buf, "%u\n", pwm_get_period(pwm));
}

static ssize_t period_ns_store(struct device *child,
			    struct device_attribute *attr,
			    const char *buf, size_t size)
{
	struct pwm_device *pwm = child_to_pwm_device(child);
	unsigned int val;
	int ret;

	ret = kstrtouint(buf, 0, &val);
	if (ret)
		return ret;

	ret = pwm_config(pwm, pwm_get_duty_cycle(pwm), val);

	return ret ? : size;
}

static ssize_t duty_ns_show(struct device *child,
			       struct device_attribute *attr,
			       char *buf)
{
	const struct pwm_device *pwm = child_to_pwm_device(child);

	return sprintf(buf, "%u\n", pwm_get_duty_cycle(pwm));
}

static ssize_t duty_ns_store(struct device *child,
				struct device_attribute *attr,
				const char *buf, size_t size)
{
	struct pwm_device *pwm = child_to_pwm_device(child);
	unsigned int val;
	int ret;

	ret = kstrtouint(buf, 0, &val);
	if (ret)
		return ret;

	ret = pwm_config(pwm, val, pwm_get_period(pwm));

	return ret ? : size;
}

static ssize_t enable_show(struct device *child,
			   struct device_attribute *attr,
			   char *buf)
{
	const struct pwm_device *pwm = child_to_pwm_device(child);

	return sprintf(buf, "%d\n", pwm_is_enabled(pwm));
}

static ssize_t enable_store(struct device *child,
			    struct device_attribute *attr,
			    const char *buf, size_t size)
{
	struct pwm_device *pwm = child_to_pwm_device(child);
	int val, ret;

	ret = kstrtoint(buf, 0, &val);
	if (ret)
		return ret;

	switch (val) {
	case 0:
		pwm_disable(pwm);
		break;
	case 1:
		ret = pwm_enable(pwm);
		break;
	default:
		ret = -EINVAL;
		break;
	}

	return ret ? : size;
}

static ssize_t polarity_show(struct device *child,
			     struct device_attribute *attr,
			     char *buf)
{
	const struct pwm_device *pwm = child_to_pwm_device(child);
	const char *polarity = "unknown";

	switch (pwm_get_polarity(pwm)) {
	case PWM_POLARITY_NORMAL:
		polarity = "normal";
		break;

	case PWM_POLARITY_INVERSED:
		polarity = "inversed";
		break;
	}

	return sprintf(buf, "%s\n", polarity);
}

static ssize_t polarity_store(struct device *child,
			      struct device_attribute *attr,
			      const char *buf, size_t size)
{
	struct pwm_device *pwm = child_to_pwm_device(child);
	enum pwm_polarity polarity;
	int ret;

	if (sysfs_streq(buf, "normal"))
		polarity = PWM_POLARITY_NORMAL;
	else if (sysfs_streq(buf, "inversed"))
		polarity = PWM_POLARITY_INVERSED;
	else
		return -EINVAL;

	ret = pwm_set_polarity(pwm, polarity);

	return ret ? : size;
}

static ssize_t active_level_show(struct device *child,
			   struct device_attribute *attr,
			   char *buf)
{
	struct pwm_device *pwm = child_to_pwm_device(child);

	return sprintf(buf, "%u\n", pwm_get_active_level(pwm));
}

static ssize_t active_level_store(struct device *child,
			      struct device_attribute *attr,
			      const char *buf, size_t size)
{
	struct pwm_device *pwm = child_to_pwm_device(child);
	unsigned int val;
	int ret;

	ret = kstrtoint(buf, 0, &val);
	if (ret)
		return ret;

	if (val > 1)
		ret = -EINVAL;
	else
		pwm->active_level = val;

	return ret ? : size;
}

static DEVICE_ATTR_RW(period_ns);
static DEVICE_ATTR_RW(duty_ns);
static DEVICE_ATTR_RW(enable);
static DEVICE_ATTR_RW(polarity);
static DEVICE_ATTR_RW(active_level);

static struct attribute *pwm_attrs[] = {
	&dev_attr_period_ns.attr,
	&dev_attr_duty_ns.attr,
	&dev_attr_enable.attr,
	&dev_attr_polarity.attr,
	&dev_attr_active_level.attr,
	NULL
};
ATTRIBUTE_GROUPS(pwm);

static void pwm_export_release(struct device *child)
{
	struct pwm_export *export = child_to_pwm_export(child);

	kfree(export);
}

static int pwm_export_child(struct device *parent, struct pwm_device *pwm)
{
	struct pwm_export *export;
	int ret;

	if (test_and_set_bit(PWMF_EXPORTED, &pwm->flags))
		return -EBUSY;

	export = kzalloc(sizeof(*export), GFP_KERNEL);
	if (!export) {
		clear_bit(PWMF_EXPORTED, &pwm->flags);
		return -ENOMEM;
	}

	export->pwm = pwm;

	export->child.release = pwm_export_release;
	export->child.parent = parent;
	export->child.devt = MKDEV(0, 0);
	export->child.groups = pwm_groups;
	dev_set_name(&export->child, "pwm%u", pwm->hwpwm);

	ret = device_register(&export->child);
	if (ret) {
		clear_bit(PWMF_EXPORTED, &pwm->flags);
		kfree(export);
		return ret;
	}

	return 0;
}

static int pwm_unexport_match(struct device *child, void *data)
{
	return child_to_pwm_device(child) == data;
}

static int pwm_unexport_child(struct device *parent, struct pwm_device *pwm)
{
	struct device *child;

	if (!test_and_clear_bit(PWMF_EXPORTED, &pwm->flags))
		return -ENODEV;

	child = device_find_child(parent, pwm, pwm_unexport_match);
	if (!child)
		return -ENODEV;

	/* for device_find_child() */
	put_device(child);
	device_unregister(child);
	pwm_put(pwm);

	return 0;
}

static ssize_t export_store(struct device *parent,
			    struct device_attribute *attr,
			    const char *buf, size_t len)
{
	struct pwm_chip *chip = dev_get_drvdata(parent);
	struct pwm_device *pwm;
	unsigned int hwpwm;
	int ret;

	ret = kstrtouint(buf, 0, &hwpwm);
	if (ret < 0)
		return ret;

	if (hwpwm >= chip->npwm)
		return -ENODEV;

	pwm = pwm_request_from_chip(chip, hwpwm, "sysfs");
	if (IS_ERR(pwm))
		return PTR_ERR(pwm);

	ret = pwm_export_child(parent, pwm);
	if (ret < 0)
		pwm_put(pwm);

	return ret ? : len;
}
//static DEVICE_ATTR_WO(export);

static ssize_t unexport_store(struct device *parent,
			      struct device_attribute *attr,
			      const char *buf, size_t len)
{
	struct pwm_chip *chip = dev_get_drvdata(parent);
	unsigned int hwpwm;
	int ret;

	ret = kstrtouint(buf, 0, &hwpwm);
	if (ret < 0)
		return ret;

	if (hwpwm >= chip->npwm)
		return -ENODEV;

	ret = pwm_unexport_child(parent, &chip->pwms[hwpwm]);

	return ret ? : len;
}
//static DEVICE_ATTR_WO(unexport);

static ssize_t npwm_show(struct device *parent, struct device_attribute *attr,
			 char *buf)
{
	const struct pwm_chip *chip = dev_get_drvdata(parent);

	return sprintf(buf, "%u\n", chip->npwm);
}
//static DEVICE_ATTR_RO(npwm);

#if 0
static struct attribute *pwm_chip_attrs[] = {
	&dev_attr_export.attr,
	&dev_attr_unexport.attr,
	&dev_attr_npwm.attr,
	NULL,
};
ATTRIBUTE_GROUPS(pwm_chip);
#else
static struct device_attribute pwm_chip_attrs[] = {
	__ATTR_WO(export),
	__ATTR_WO(unexport),
	__ATTR_RO(npwm),
	__ATTR_NULL,
};
#endif


static struct class pwm_class = {
	.name = "pwm",
	.owner = THIS_MODULE,
	/* .dev_groups = pwm_chip_groups, */
	.dev_attrs = pwm_chip_attrs,
};

static int pwmchip_sysfs_match(struct device *parent, const void *data)
{
	return dev_get_drvdata(parent) == data;
}

void pwmchip_sysfs_export(struct pwm_chip *chip)
{
	struct device *parent;

	/*
	 * If device_create() fails the pwm_chip is still usable by
	 * the kernel its just not exported.
	 */
	parent = device_create(&pwm_class, chip->dev, MKDEV(0, 0), chip,
			       "pwmchip%d", chip->base);
	if (IS_ERR(parent)) {
		dev_warn(chip->dev,
			 "device_create failed for pwm_chip sysfs export\n");
	}
}

void pwmchip_sysfs_unexport(struct pwm_chip *chip)
{
	struct device *parent;

	parent = class_find_device(&pwm_class, NULL, chip,
				   pwmchip_sysfs_match);
	if (parent) {
		/* for class_find_device() */
		put_device(parent);
		device_unregister(parent);
	}
}

static int __init pwm_sysfs_init(void)
{
	return class_register(&pwm_class);
}
subsys_initcall(pwm_sysfs_init);
