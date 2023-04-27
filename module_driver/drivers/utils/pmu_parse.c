#include <linux/module.h>
#include <linux/string.h>
#include <linux/slab.h>
#include <utils/string.h>
#include <linux/kernel.h>
#include <utils/pmu.h>

#include <linux/regulator/machine.h>

static int param_pmu_set(const char *arg, const struct kernel_param *kp)
{
    struct pmu_constraints_data *pmu_data = (struct pmu_constraints_data *) kp->arg;
    int N, i, ret;
    char **parse_result = str_to_words(arg, &N);
    for (i = 0; i < N; i++) {
        /* parse name */
        if (strstarts(parse_result[i], "regulator_name="))
            pmu_data->name = parse_result[i] + strlen("regualtor_name=");

        /* parse boot status */
        if (strstarts(parse_result[i], "boot_on=")) {
            if (!strcmp(parse_result[i] + strlen("boot_on="), "enable"))
                pmu_data->boot_status = 1;

            if (!strcmp(parse_result[i] + strlen("boot_on="), "disable"))
                pmu_data->boot_status = 0;
        }

        /* parse work voltage */
        if (strstarts(parse_result[i], "work_voltage=")) {
            ret = kstrtouint(parse_result[i] + strlen("work_voltage="), 0, &pmu_data->work_voltage);
            if (ret) {
                printk(KERN_ERR "Try to set a invalid work_voltage\n");
                goto parse_err;
            }
        }

        /* parse suspend voltage */
        if (strstarts(parse_result[i], "suspend_volage=")) {
            ret = kstrtouint(parse_result[i] + strlen("suspend_volage="), 0, &pmu_data->suspend_voltage);
            if (ret) {
                printk(KERN_ERR "Try to set a invalid suspend_volage\n");
                goto parse_err;
            }
        }
    }

    return 0;

parse_err:
    str_free_words(parse_result);
    return -EINVAL;
}

static int param_pmu_get(char *buffer, const struct kernel_param *kp)
{
    struct pmu_constraints_data *pmu_data = (struct pmu_constraints_data *) kp->arg;
    if (!buffer)
        buffer = kmalloc(100, GFP_KERNEL);

    sprintf(buffer, "%s :regulator_name=%s, boot_on=%d, work_voltage:%d, suspend_voltage:%d\n",
        kp->name, pmu_data->name, pmu_data->boot_status,
            pmu_data->work_voltage, pmu_data->suspend_voltage);

    return strlen(buffer);
}

struct kernel_param_ops param_pmu_ops = {
    .set = param_pmu_set,
    .get = param_pmu_get,
};
EXPORT_SYMBOL(param_pmu_ops);