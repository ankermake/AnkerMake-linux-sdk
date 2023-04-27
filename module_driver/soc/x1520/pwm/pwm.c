#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/miscdevice.h>
#include <linux/err.h>
#include <linux/slab.h>
#include <linux/gpio.h>
#include <linux/clk.h>
#include <soc/extal.h>
#include <soc/gpio.h>
#include <linux/fs.h>
#include <linux/sched.h>
#include <linux/mutex.h>

#include <bit_field.h>
#include <utils/gpio.h>
#include <utils/clock.h>

#include <pwm.h>

#define TCU_NUMS 4

#define TSTR    0xF0
#define TSTSTR   0xF4
#define TSTCR   0xF8
#define TSTCLR  0xF8
#define TSR     0x1C
#define TSSTR   0x2C
#define TSCLR   0x3C
#define TCER    0x10
#define TCSTR   0x14
#define TCCLR   0x18
#define TFR     0x20
#define TFSTR   0x24
#define TFCLR   0x28
#define TMR     0x30
#define TMSTR   0x34
#define TMCLR   0x38

#define TDFR(n)     (0x40 + (n)*0x10)
#define TDHR(n)     (0x44 + (n)*0x10)
#define TCNT(n)     (0x48 + (n)*0x10)
#define TCSR(n)     (0x4c + (n)*0x10)

#define TCU_IOBASE  0x10002000

#define TCU_REG_BASE  KSEG1ADDR(TCU_IOBASE)

#define TCU_ADDR(reg) ((volatile unsigned long *)(TCU_REG_BASE + (reg)))

#define TCSR_CLRZ   10, 10
#define TCSR_SD     9, 9
#define TCSR_INIT   8, 8
#define TCSR_PWM_EN 7, 7
#define TCSR_PRESCALE   3, 5
#define TCSR_CLK_EN 0, 2

#define PWM_DEFAULT_FULL_NUM (255)

#define PWM_DUTY_MAX_COUNT   (65536)

#define EXT_EN  4   /*timer input clock is EXT*/

#define PWM_MAGIC_NUMBER    'P'
#define PWM_REQUEST             _IOW(PWM_MAGIC_NUMBER, 11, char)
#define PWM_RELEASE             _IOW(PWM_MAGIC_NUMBER, 22, unsigned int)
#define PWM_CONFIG              _IOW(PWM_MAGIC_NUMBER, 33, struct pwm_config_data)
#define PWM_SET_LEVEL           _IOWR(PWM_MAGIC_NUMBER, 44, unsigned int)

struct jz_pwm_gpio {
    char name[5];
    int id;
    int gpio;
    int func;
};

struct jz_tcu {
    unsigned int is_request;
    unsigned int is_init;
    const char *name;
    unsigned long rate;
    unsigned long real_rate;
    unsigned long half_num;
    unsigned long full_num;

    int is_enable;
    struct jz_pwm_gpio *gpio_def;
    unsigned int idle_level;
    unsigned int sd_mode;
    unsigned int levels;

};

static DEFINE_MUTEX(pwm_mutex);

struct jz_tcu tcudata[TCU_NUMS];

static unsigned long clk_src_rate;

/* 支持TCU2模式的通道有，ch1、ch2 */
#define TCU2_LIST (BIT(1) | BIT(2))

static inline int is_tcu2_support_id(int id)
{
    return TCU2_LIST & BIT(id);
}

static inline void tcu_write_reg(unsigned int reg, unsigned int value)
{
    *TCU_ADDR(reg) = value;
}

static inline unsigned int tcu_read_reg(unsigned int reg)
{
    return *TCU_ADDR(reg);
}

static inline void tcu_set_bit(unsigned int reg, int start, int end, unsigned int value)
{
    set_bit_field(TCU_ADDR(reg), start, end, value);
}

static inline unsigned int tcu_get_bit(unsigned int reg, int start, int end)
{
    return get_bit_field(TCU_ADDR(reg), start, end);
}

static void tcu_reset_half_full_num(int id)
{
    /* 设置 HALF、 FULL */
    unsigned int full_num = tcudata[id].full_num;
    unsigned int half_num = tcudata[id].half_num;

    full_num = max(full_num, 1U);
    half_num = max(half_num, 1U);

    tcu_write_reg(TDFR(id), full_num - 1);
    tcu_write_reg(TDHR(id), half_num - 1);
}

static void tcu_reset_clk(int id)
{
    unsigned long r;
    unsigned long div;

    unsigned long rate = tcudata[id].rate;

    r = clk_src_rate;

    if (rate)
        r = r/rate;
    else
        r = 1024;

    switch (r) {
        case 0:
        case 1:div = 0;break;
        case 4:div = 1;break;
        case 16:div = 2;break;
        case 64:div = 3;break;
        case 256:div = 4;break;
        case 1024:div = 5;break;
        default:div = 5;break;
    }

    tcu_set_bit(TCSR(id), TCSR_PRESCALE, div);

    /* 设置时钟源 */
    tcu_set_bit(TCSR(id), TCSR_CLK_EN, EXT_EN);
}

static void jz_tcu_reset(int id)
{
    tcu_set_bit(TCSR(id), TCSR_SD, 0);

    /* 设置 TCSR.PCK_EN 为 1 ，设置时钟源 */
    tcu_set_bit(TCSR(id), TCSR_CLK_EN, 1);

    /* 设置 TCNT 值 */
    tcu_write_reg(TCNT(id), 0);

    /* 设置 TCSR.PCK_EN 为 0， 关闭 input clock */
    tcu_set_bit(TCSR(id), TCSR_CLK_EN, 0);

}

static void jz_tcu_init_state(int id)
{
    /* 设置 TCSR.PRESCALE=0, TCSR.PWM_EN=0， TCENR=0 */
    tcu_write_reg(TCCLR, 1 << id);

    tcu_write_reg(TSCLR, 1 << id);

    tcu_write_reg(TCSR(id), 0);

    /* 复位 TCU */
    jz_tcu_reset(id);

}

static void jz_tcu_stop_clock(int id)
{
    tcu_set_bit(TSSTR, id, id, 1);
}

static void jz_tcu_clear_tcnt(int id)
{
    if (is_tcu2_support_id(id)) {
        tcu_set_bit(TCSR(id), TCSR_CLRZ, 1);

        while (tcu_read_reg(TCNT(id)) != 0);

        tcu_set_bit(TCSR(id), TCSR_CLRZ, 0);
    } else {
        tcu_write_reg(TCNT(id), 0);
    }

}

static void jz_tcu_close_state(int id)
{
    tcu_set_bit(TCSR(id), TCSR_SD, 0);

    tcu_write_reg(TFCLR, (1 << (id + 16) | (1 << id)));

    /* 清0 TCNT 值 */
    jz_tcu_clear_tcnt(id);

    /* 设置 Timer STOP 位 */
    jz_tcu_stop_clock(id);
}

static void jz_tcu_set_sd_mode(int id)
{
    tcu_set_bit(TCSR(id), TCSR_SD, 0);
}

struct jz_pwm_gpio pwm_gpio_array[] = {
    { .name = "pwm0", .id = 0, .func = GPIO_FUNC_0, .gpio = GPIO_PB(17), },
    { .name = "pwm1", .id = 1, .func = GPIO_FUNC_0, .gpio = GPIO_PB(18), },
    { .name = "pwm2", .id = 2, .func = GPIO_FUNC_0, .gpio = GPIO_PC(8), },
    { .name = "pwm3", .id = 3, .func = GPIO_FUNC_0, .gpio = GPIO_PC(9), },
};

int pwm_gpio_array_size = ARRAY_SIZE(pwm_gpio_array);

static struct jz_pwm_gpio *tcu_get_pwm_gpio_func_def(int gpio)
{
    int i;
    struct jz_pwm_gpio *def;

    for (i = 0; i < pwm_gpio_array_size; i++) {
        def = &pwm_gpio_array[i];
        if (def->gpio == gpio)
            return def;
    }

    return NULL;
}

static void pwm_enable(int id)
{
    if (tcudata[id].is_enable)
        return;

    tcu_set_bit(TCSR(id), TCSR_INIT, !tcudata[id].idle_level);

    gpio_set_func(tcudata[id].gpio_def->gpio, tcudata[id].gpio_def->func);

    tcu_set_bit(TCSR(id), TCSR_PWM_EN, 1);

    tcu_write_reg(TCSTR, 1 << id);

    tcudata[id].is_enable = 1;

}

static void pwm_disable(int id)
{
    int cnt = 0;
    unsigned long timeout = 0;
    unsigned long long now = 0;
    int init_level = 0;

    if (!tcudata[id].is_enable)
        return;

    init_level = !tcudata[id].idle_level;

    jz_tcu_close_state(id);

    /* 停止计数 */
    tcu_write_reg(TCCLR, (1 << id));

    /* 停止输出 PWM */
    tcu_set_bit(TCSR(id), TCSR_PWM_EN, 0);

    if (tcudata[id].sd_mode == PWM_abrupt_shutdown || is_tcu2_support_id(id))
        gpio_direction_output(tcudata[id].gpio_def->gpio, !init_level);

    if (tcudata[id].sd_mode == PWM_graceful_shutdown) {
        /* 超时时间为2个周期 */
        timeout = 2 * 1000 *1000 / tcudata[id].real_rate;
        if(timeout == 0)
            timeout = 1;

        now = local_clock_us();

        while (tcu_read_reg(TCNT(id)) != cnt) {
            cnt = tcu_read_reg(TCNT(id));
            if ((local_clock_us() - now) > timeout) {
                if (tcu_read_reg(TCNT(id)) != cnt) {
                    printk(KERN_ERR "PWM: pwm ch%d  disable timeout\n", id);
                    break;
                }
            }
        }

    }

    tcudata[id].is_enable = 0;
}

static unsigned long get_pwm_source_rate_freq_first(int id, unsigned long freq)
{
    unsigned long rate;

    rate = clk_src_rate;

    if (freq > rate) {
        printk(KERN_ERR "PWM: pwm only support frequency low than %lu\n", rate);
        return 0;
    }

    while (rate / freq > PWM_DUTY_MAX_COUNT)
        rate = rate / 4;

    return rate;
}

static unsigned long get_pwm_source_rate_levels_first(int id, unsigned long freq)
{
    unsigned long rate;
    unsigned long tmp;

    rate = clk_src_rate;

    if (freq > rate) {
        printk(KERN_ERR "TCU: %s only support frequency low than %lu\n", tcudata[id].name, rate);
        return 0;
    }

    tmp = rate / freq;

    if (tmp < 4)
        rate = rate / 1;    /* 分频数 1 */
    else if (tmp < 16)
        rate = rate / 4;    /* 分频数 4 */
    else if (tmp < 64)
        rate = rate / 16;   /* 分频数 16 */
    else if (tmp < 256)
        rate = rate / 64;   /* 分频数 64 */
    else if (tmp < 1024)
        rate = rate / 256;  /* 分频数 256 */
    else
        rate = rate / 1024; /* 分频数 1024 */

    return rate;
}

int pwm2_set_level(int id, unsigned long level)
{
    if (id < 0 || id >= TCU_NUMS) {
        printk(KERN_ERR "PWM: %s, No support pwm ch %d !\n", __func__, id);
        return -1;
    }

    mutex_lock(&pwm_mutex);

    if (level != 0) {
        level = level * tcudata[id].full_num / tcudata[id].levels;
        if (level == 0)
            level = 1;
    }

    if (tcudata[id].is_init == 0) {
        printk(KERN_ERR "PWM: pwm ch%d is not config!\n", id);
        id = -1;
        goto unlock;
    }

    if (level > tcudata[id].full_num) {
        printk(KERN_ERR "PWM: pwm ch%d level %ld need less than %d\n", id, tcudata[id].half_num, tcudata[id].levels);
        id = -1;
        goto unlock;
    }

    tcudata[id].half_num = level;

    if (!tcudata[id].is_enable) {
        jz_tcu_init_state(id);
        /* 设置 TCNT count 时钟频率,并使能输入时钟源 */
        tcu_reset_clk(id);
    }

    /* 设置 TDHR 和 TDFR */
    tcu_reset_half_full_num(id);

    if (level > 0)
        pwm_enable(id);
    else
        pwm_disable(id);

unlock:
    mutex_unlock(&pwm_mutex);

    return id;
}
EXPORT_SYMBOL_GPL(pwm2_set_level);

int pwm2_config(int id, struct pwm_config_data *config)
{
    unsigned long full_num;

    unsigned long freq = config->freq;
    unsigned int idle_level = config->idle_level;
    unsigned long levels = config->levels;

    if (id < 0 || id >= TCU_NUMS) {
        printk(KERN_ERR "PWM: %s, No support pwm ch %d !\n", __func__, id);
        return -1;
    }

    if (freq == 0) {
        printk(KERN_ERR "PWM: pwm ch %d only support frequency high than 0 Hz\n", id);
        return -1;
    }

    if (levels >= PWM_DUTY_MAX_COUNT) {
        printk(KERN_ERR "PWM: pwm ch %d levels need less than %d\n", id, PWM_DUTY_MAX_COUNT);
        return -1;
    }

    mutex_lock(&pwm_mutex);

    if (!tcudata[id].is_request) {
        printk(KERN_ERR "PWM: pwm ch %d is not request!\n", id);
        id = -1;
        goto unlock;
    }

    if (tcudata[id].is_enable) {
        printk(KERN_ERR "PWM: pwm ch %d Cannot configure at working\n", id);
        id = -1;
        goto unlock;
    }

    tcudata[id].sd_mode = config->shutdown_mode;
    tcudata[id].idle_level = idle_level;
    tcudata[id].levels = levels;

    if (config->accuracy_priority == PWM_accuracy_freq_first) {
        tcudata[id].rate = get_pwm_source_rate_freq_first(id, freq);

        full_num = tcudata[id].rate / freq;

        tcudata[id].real_rate = tcudata[id].rate / full_num;

    } else {
        tcudata[id].rate = get_pwm_source_rate_levels_first(id, freq);

        tcudata[id].real_rate = tcudata[id].rate / levels;

        full_num = levels;
    }

    tcudata[id].full_num = full_num;
    tcudata[id].half_num = 0;

    if (tcudata[id].is_init == 0) {
        if (is_tcu2_support_id(id)) {
            /* 初始TCU状态 */
            jz_tcu_init_state(id);
        } else {
            /* 设置 TCSR.PWM_EN=0 */
            tcu_write_reg(TCSR(id), 0);

            /* 设置 shutdown 模式 */
            jz_tcu_set_sd_mode(id);
        }

        tcudata[id].is_init = 1;
    }

    /* 设置 TCNT count 时钟频率,并使能输入时钟源 */
    tcu_reset_clk(id);

    /* 设置自动重装载值 */
    tcu_reset_half_full_num(id);

unlock:
    mutex_unlock(&pwm_mutex);

    return id;
}
EXPORT_SYMBOL_GPL(pwm2_config);

int pwm2_request(int gpio, const char *name)
{
    int id;
    char buf[8];
    struct jz_pwm_gpio *gpio_def;

    mutex_lock(&pwm_mutex);

    gpio_def = tcu_get_pwm_gpio_func_def(gpio);
    if (gpio_def == NULL) {
        printk(KERN_ERR "PWM: %s not support as pwm.\n", gpio_to_str(gpio, buf));
        id = -1;
        goto unlock;
    }

    id = gpio_def->id;

    if (tcudata[id].is_request)
        goto unlock;

    if (gpio_request(gpio, name)) {
        printk(KERN_ERR "PWM: pwm ch %d, request IO %s error !\n", id, gpio_to_str(gpio, buf));
        id = -1;
        goto unlock;
    }

    tcudata[id].is_request = 1;
    tcudata[id].gpio_def = gpio_def;
    tcudata[id].name = name;
    tcudata[id].rate  = clk_src_rate;

unlock:
    mutex_unlock(&pwm_mutex);

    return id;
}
EXPORT_SYMBOL_GPL(pwm2_request);

int pwm2_release(int id)
{
    if (id < 0 || id >= TCU_NUMS) {
        printk(KERN_ERR "PWM: %s, No support pwm ch %d !\n", __func__, id);
        return -1;
    }

    mutex_lock(&pwm_mutex);

    if (tcudata[id].is_init == 1) {
        pwm_disable(id);

        tcudata[id].is_init = 0;
    }

    if (tcudata[id].is_request) {
        gpio_free(tcudata[id].gpio_def->gpio);

        tcudata[id].is_request = 0;
    }

    mutex_unlock(&pwm_mutex);

    return id;
}
EXPORT_SYMBOL_GPL(pwm2_release);

static int pwm_open(struct inode *inode, struct file *filp)
{
    return 0;
}

static int pwm_close(struct inode *inode, struct file *filp)
{
    return 0;
}

static long pwm_ioctl(struct file *filp, unsigned int cmd, unsigned long arg)
{
    int ret = 0;

    switch (cmd) {
    case PWM_REQUEST: {
        char gpio_name[12] = {0};
        if (copy_from_user(gpio_name, (char *)arg, 12)) {
            printk(KERN_ERR "PWM: copy_from_user err!\n");
            return -1;
        }

        int gpio = str_to_gpio(gpio_name);
        if (gpio < 0) {
            printk(KERN_ERR "PWM: gpio is invalid:%s !\n", gpio_name);
            return -1;
        }

        struct jz_pwm_gpio *gpio_def = tcu_get_pwm_gpio_func_def(gpio);
        if (!gpio_def) {
            printk(KERN_ERR "PWM: gpio is not pwm:%s !\n", gpio_name);
            return -1;
        }

        ret = pwm2_request(gpio_def->gpio, gpio_def->name);

        return ret;
    }
    case PWM_RELEASE: {
        ret = pwm2_release(arg);
        return ret;
    }
    case PWM_CONFIG: {
        struct pwm_config_data config;
        if (copy_from_user(&config, (void *)arg, sizeof(config))) {
            printk(KERN_ERR "PWM: copy_from_user err!\n");
            return -1;
        }

        ret = pwm2_config(config.id, &config);
        return ret;
    }
    case PWM_SET_LEVEL: {
        unsigned int tmp[2];
        if (copy_from_user(tmp, (void *)arg, sizeof(tmp))) {
            printk(KERN_ERR "PWM: copy_from_user err!\n");
            return -1;
        }

        ret = pwm2_set_level(tmp[0], tmp[1]);
        return ret;
    }
    default:
        return -1;
    }

    return ret;
}

static struct file_operations pwm_fops = {
    .owner= THIS_MODULE,
    .open= pwm_open,
    .release = pwm_close,
    .unlocked_ioctl = pwm_ioctl,
};

struct miscdevice pwm_mdevice = {
    .minor = MISC_DYNAMIC_MINOR,
    .name = "jz_pwm",
    .fops = &pwm_fops,
};

static int jz_pwm_init(void)
{
    int ret;
    struct clk *clk = clk_get(NULL, "ext1");
    BUG_ON(IS_ERR(clk));
    clk_src_rate = clk_get_rate(clk);

    ret = misc_register(&pwm_mdevice);
    if (ret < 0)
        panic("PWM: %s, pwm register misc dev error !\n", __func__);

    return 0;
}
module_init(jz_pwm_init);

static void jz_pwm_exit(void)
{
    int ret = misc_deregister(&pwm_mdevice);
    if (ret < 0)
        panic("PWM: %s, pwm deregister misc dev error !\n", __func__);
}
module_exit(jz_pwm_exit);

MODULE_DESCRIPTION("Ingenic SoC PWM driver");
MODULE_LICENSE("GPL");
