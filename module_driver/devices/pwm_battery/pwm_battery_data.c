#include <pwm.h>
#include <linux/delay.h>
#include <linux/gpio.h>
#include <linux/miscdevice.h>
#include <linux/module.h>
#include <utils/gpio.h>
#include <linux/fs.h>
#include <common.h>
#include <linux/list.h>
#include <linux/ioctl.h>
#include <linux/sched.h>
#include <linux/workqueue.h>
#include <linux/kthread.h>
#include <linux/mutex.h>

#include <linux/hrtimer.h>

static int pwm_gpio = -1;
static int status_gpio = -1;
static int power_gpio = -1;
static int status_active = 0;
static int power_active = 0;
static int pwm_max_level = 0;
static int res1_value = 0;
static int res2_value = 0;
static int deviation = 0;
static int max_pwm_voltage = 0;

module_param_gpio(pwm_gpio, 0644);
module_param_gpio(status_gpio, 0644);
module_param_gpio(power_gpio, 0644);
module_param(status_active, int, 0644);
module_param(power_active, int, 0644);
module_param(pwm_max_level, int, 0644);
module_param(res1_value, int, 0644);
module_param(res2_value, int, 0644);
module_param(deviation, int, 0644);
module_param(max_pwm_voltage, int, 0644);


#define PWM_SAMPLE_COUNT        5
#define PWM_STABLE_TIME_US      200
#define PWM_FREQ                140000

static struct pwm_battery_data {
    struct pwm_config_data config;
    int id;
    struct mutex mutex;
}pwm_battery;


static inline int m_gpio_request(int gpio , const char *name)
{
    if (gpio < 0)
        return 0;

    int ret = gpio_request(gpio, name);
    if (ret) {
        char buf[20];
        printk(KERN_ERR "pwm_battery: failed to request %s: %s\n", name, gpio_to_str(gpio, buf));
        return ret;
    }

    return 0;
}

static inline void m_gpio_free(int gpio)
{
    if (gpio >= 0)
        gpio_free(gpio);
}

static inline void m_gpio_direction_output(int gpio, int value)
{
    if (gpio >= 0)
        gpio_direction_output(gpio, value);
}

static int pwm_battery_request(void)
{
    int ret;

    if (status_gpio < 0) {
        printk(KERN_ERR "status_gpio must config\n");
        return -1;
    }

    if (pwm_gpio < 0) {
        printk(KERN_ERR "pwm_gpio must config\n");
        return -1;
    }

    ret = m_gpio_request(status_gpio, "battery status");
    if (ret < 0)
        return ret;

    ret = m_gpio_request(power_gpio, "battery power");
    if (ret < 0)
        goto power_err;

    ret = pwm2_request(pwm_gpio, "pwm_battery");
    if (ret < 0)
        goto pwm_err;

    pwm_battery.id = ret;

    return 0;

pwm_err:
    m_gpio_free(power_gpio);
power_err:
    m_gpio_free(status_gpio);

    return ret;
}

static int pwm_battery_enable(void)
{
    int ret;
    pwm_battery.config.accuracy_priority = PWM_accuracy_freq_first;
    pwm_battery.config.shutdown_mode = PWM_graceful_shutdown;
    pwm_battery.config.idle_level = PWM_idle_low;
    pwm_battery.config.freq = PWM_FREQ;
    pwm_battery.config.levels = pwm_max_level;

    ret = pwm2_config(pwm_battery.id, &pwm_battery.config);
    if (ret < 0)
        return ret;

    gpio_direction_input(status_gpio);

    m_gpio_direction_output(power_gpio, power_active);

    return 0;
}
static void pwm_battery_disable(void)
{
    m_gpio_direction_output(power_gpio, !power_active);
}

static int get_battery_voltage(void)
{
    int i;
    unsigned int voltage[PWM_SAMPLE_COUNT];
    unsigned int level = 1;
    unsigned int voltage_max = 0;
    unsigned int voltage_min = 0xffffffff;
    unsigned int voltage_total = 0;
    unsigned int voltage_av = -1;

    mutex_lock(&pwm_battery.mutex);

    for (i = 0; i < PWM_SAMPLE_COUNT; i++) {
        while(1) {
            if (level > pwm_battery.config.levels) {
                printk(KERN_ERR "pwm_battery:level more than max level\n");
                goto unlock;
            }

            pwm2_set_level(pwm_battery.id, level);

            usleep_range(PWM_STABLE_TIME_US, PWM_STABLE_TIME_US);

            if (gpio_get_value(status_gpio) == status_active)
                break;

            level++;
        }

        voltage[i] = (max_pwm_voltage * (level - 1) / pwm_max_level - deviation) * (res2_value  + res1_value) / res2_value;

        voltage_total += voltage[i];

        if (voltage[i] > voltage_max)
            voltage_max = voltage[i];
        if (voltage[i] < voltage_min)
            voltage_min = voltage[i];

        while (gpio_get_value(status_gpio) == status_active) {
            level--;
            if (level < 0) {
                printk(KERN_ERR "pwm_battery:level error\n");
                goto unlock;
            }

            pwm2_set_level(pwm_battery.id, level);
            usleep_range(PWM_STABLE_TIME_US, PWM_STABLE_TIME_US);
        }

        level -= 5;
        if (level < 0)
            level = 0;
    }

    pwm2_set_level(pwm_battery.id, 0);

    if (PWM_SAMPLE_COUNT > 2)
        voltage_av = (voltage_total - voltage_max - voltage_min) / (PWM_SAMPLE_COUNT - 2);
    else
        voltage_av = voltage_total / PWM_SAMPLE_COUNT;

unlock:
    mutex_unlock(&pwm_battery.mutex);
    return voltage_av;
}

static int pwm_battery_open(struct inode *inode, struct file *filp)
{
    return pwm_battery_enable();
}

static int pwm_battery_close(struct inode *inode, struct file *filp)
{
    pwm_battery_disable();
    return 0;
}

static long pwm_battery_ioctl(struct file *filp, unsigned int cmd, unsigned long arg)
{
    return 0;
}

static ssize_t
pwm_battery_read(struct file *file, char __user *ubuf,
                       size_t count, loff_t *ppos)
{
    int voltage;

    voltage = get_battery_voltage();
    if(voltage < 0)
        return -1;

    copy_to_user(ubuf, &voltage, sizeof(int));
    return sizeof(int);
}

static struct file_operations pwm_battery_misc_fops = {
    .open = pwm_battery_open,
    .read = pwm_battery_read,
    .release = pwm_battery_close,
    .unlocked_ioctl = pwm_battery_ioctl,
};

static struct miscdevice mdev = {
    .minor = MISC_DYNAMIC_MINOR,
    .name = "pwm_battery",
    .fops = &pwm_battery_misc_fops,
};

static int pwm_battery_init(void)
{
    int ret;

    ret = pwm_battery_request();
    if (ret < 0)
        return -1;

    mutex_init(&pwm_battery.mutex);

    ret = misc_register(&mdev);
    BUG_ON(ret < 0);

    return 0;
}

static void pwm_battery_exit(void)
{

    m_gpio_free(power_gpio);
    m_gpio_free(status_gpio);
    pwm2_release(pwm_battery.id);
    misc_deregister(&mdev);
}

module_init(pwm_battery_init);
module_exit(pwm_battery_exit);

MODULE_DESCRIPTION("pwm battery module");
MODULE_LICENSE("GPL");