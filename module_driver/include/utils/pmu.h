#ifndef _UTILS_PMU_H_
#define _UTILS_PMU_H_

#include <linux/module.h>

struct pmu_constraints_data {
    const char *name;
    unsigned int boot_status;
    unsigned int work_voltage;
    unsigned int suspend_voltage;
};

/**
 * @brief module_param_pmu_named - module param for pmu
 * @param name: a valid C identifier which is the parameter name.
 * @param value: the actual lvalue to alter.
 * @param perm: visibility in sysfs.
 *
 */

#define module_param_pmu_named(name, value, perm)     \
    module_param_cb(name, &param_pmu_ops, &(value), perm);          \

/**
 * @brief module_param_pmu - module param for pmu
 * @param value: the variable to alter, and exposed parameter name.
 * @param perm: visibility in sysfs.
 *
 */
#define module_param_pmu(value, perm)           \
    module_param_pmu_named(value, value, perm)

/*
 * used by module_param_pmu_named, module_param_pmu
 */
extern struct kernel_param_ops param_pmu_ops;

#endif /* _UTILS_PMU_H_ */