#include <board.h>
#include <common.h>
#include <linux/gpio.h>
#include <linux/init.h>
#include <linux/interrupt.h>
#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/mutex.h>
#include <linux/pwm.h>

#include <linux/hrtimer.h>
#include <linux/jiffies.h>
#include <linux/miscdevice.h>
#include <linux/poll.h>
#include <linux/sched.h>
#include <linux/slab.h>
#include <linux/uaccess.h>
#include <linux/workqueue.h>

#include <jz_spi_v2.h>
#include <soc/gpio.h>

extern void test_jz_adc_enable(void);
extern void test_jz_adc_disable(void);
extern unsigned int test_jz_adc_channel_sample(unsigned int channel, unsigned int block);
extern unsigned int test_jz_adc_read_value(unsigned int channel, unsigned int block);

static void thermal_head_data_complete(struct jzspi_config_data *config);
static void thermal_head_dst_complete(struct jzspi_config_data *config);

struct paper_properties {
    unsigned int paper_printing_energy; //纸张的标准印刷能量
    unsigned int paper_temperature_coefficient; //纸张的温度系数
};

/* 打印机状态 用bit代表状态*/
#define PRINTER_WORKING 0x80 //打印机正在打印
#define PRINTER_PAPER_EMPTY 0x01 //打印机缺纸
#define PRINTER_THERMAL_HEAD_OVERHEAT 0x02 //打印机热敏头过热
#define PRINTER_MOTOR_OVERHEAT 0x04 //打印机马达过热

/* 错误掩码，用于判断write借口是否能正常使用 */
#define PRINTER_ERROR_MASK (PRINTER_PAPER_EMPTY | PRINTER_THERMAL_HEAD_OVERHEAT | PRINTER_MOTOR_OVERHEAT)

/* 获取打印机状态 */
#define GET_PRINTER_STATUS _IOR('p', 0x21, unsigned int*)
/* 设置纸张特性 */
#define SET_PAPER_PROPERTIES _IOWR('p', 0x22, struct paper_properties*)

#define THERMAL_HEAD_STATUS_HEAT    0
#define THERMAL_HEAD_STATUS_END     1

#define MOTOR_STATUS_WORK           0
#define MOTOR_STATUS_WAIT           1

/* 缺纸检测io */
#define PS_GPIO GPIO_PB(28)
/* 缺纸io时，ps的电平 */
#define PS_PAPER_EMPTY_LEVEL 0

/* 电机ic使能 */
#define MOTOR_ENABLE_GPIO GPIO_PB(14)
#define MOTOR_ENABLE_LEVEL 1

#define POWER_ENABLE_GPIO GPIO_PA(17)
#define POWER_ENABLE_LEVEL 1

/* 12v电源 pwm控制器id */
#define POWER_PWM_ID 1

/* 加热头数据通许spi id */
#define TH_DATA_SPI_ID 0

/* 加热头spi数据通许频率 单位 Hz */
#define TH_DATA_SPI_FREQ (6 * 1000 * 1000)

/* 加热头使能加热spi id */
#define TH_DST_SPI_ID 1

/* 加热头spi数据通许频率 单位 Hz */
#define TH_DST_SPI_FREQ (1 * 1000 * 1000)

/* 加热头热敏电阻检测adc id */
#define TH_ADC_ID 0

/* 加热头数据锁存io */
#define LAT_GPIO GPIO_PB(13)

/* 电机相位控制io */
#define MOTOR_A_GPIO GPIO_PB(6)
#define MOTOR_AA_GPIO GPIO_PB(7)
#define MOTOR_B_GPIO GPIO_PB(10)
#define MOTOR_BB_GPIO GPIO_PB(9)

/* 纸张的默认打印能量 */
#define PAPER_PRINTING_ENERGY 410500

/* 纸张的默认温度系数 */
#define PAPER_TEMPERATURE_COEFFICIENT 4200

/* 总热敏头点数，代码限制必须能被32整除*/
#define TOTAL_DOTS 384

/* 能同时激活的热敏头点数 */
#define ACTIVATED_DOTS 48

/* 数据缓冲大小 单位 行 */
#define BUFFER_LINE_COUNT 1024

/* 步进电机多少步为一个周期 */
#define MOTOR_STEP_COUNT 8

/* 电机步进允许误差 单位：us
    不允许超过步进电机的最小步进时间
*/
#define MOTOR_STEP_ERROR_ALLOWABLE_TIME 4

/* 一个像素点电机需要走几步 以1_2phase为单位 */
#define MOTOR_LINE_STEP 4

/* 电机从运行到停止最少要做几次减速 */
#define MOTOR_DECELERATION_COUNT 4

/* 电机启动加速，电机启动前几步不打印数据用于电机起始加速 */
#define MOTOR_ACCELERATE_COUNT 4

/* 12伏电源 pwm周期  单位 ns */
#define POWER_PWM_PERIOD 1000000

/* 热敏头出现过热后，间隔多长时间查询温度 单位：jiffies */
#define HEAD_OVERHEAT_MONITORING_TIME (HZ / 2)

/* 电机最长运行时间  单位：us*/
#define MOTOR_RUN_MAX_TIME (20 * 1000 * 1000)

/* 电机过热休息时间 单位：jiffies*/
#define MOTOR_OVERHEAT_SLEEP_TIME (5 * HZ)

/* 热敏头过热温度 */
#define THERMAL_HEAD_OVERHEAT_TEMPERATURE 70

/* 热敏头恢复正常温度 */
#define THERMAL_HEAD_RECOVERY_TEMPERATURE 60

struct printer_dev {
    struct mutex lock; //用于锁杂项设备接口资源

    unsigned int buf_write_flag; //环形缓冲区的写位置标记 单位一行数据
    unsigned int buf_read_flag; //环形缓冲区的读位置标记 单位一行数据
    unsigned char* printer_line_buf; //打印数据环形缓冲区

    wait_queue_head_t buf_wait; //等待缓冲区可写入数据
    wait_queue_head_t buf_flush_wait; //等待缓冲区数据全部打印完成

    struct miscdevice printer_mdev; //对接应用层杂项设备
    unsigned int printer_status; //打印机状态
    unsigned int printer_mdev_open; //杂项设备的打开标记

    struct hrtimer printer_timer; //打印任务定时器
    unsigned int motor_clock_us; //记录步进电机启动时间节点
    unsigned int motor_step; //电机的步进位置
    unsigned int* phase1_2_steps; //1_2的电机加速表
    unsigned int phase1_2_steps_count; //1_2的电机加速表的加速步数
    unsigned int* phase2_2_steps; //2_2的电机加速表
    unsigned int phase2_2_steps_count; //1_2的电机加速表的加速步数
    unsigned int phase_flag; //步进电机模式，1_2模式为0, 2_2模式为1
    unsigned int current_acceleration_step; //记录加速表的位置，优化查询加速表时间和强制执行加速步骤，不允许跳步加速，但允许跳步减速。

    unsigned int motor_start_clock; //电机上电时间点，用于计算电机过热

    unsigned char (*motor_step_level)[4]; /* 步进电机时序表,二维数组指针 */

    struct pwm_device* pwm; //打印机供电模块使能时钟
    struct jzspi_config_data data_spi_config; //热敏打印头数据通讯spi，不用指针，回调函数里面需要container_of
    struct jzspi_config_data dst_spi_config; //热敏打印头dst spi，不用指针，回调函数里面需要container_of
    unsigned char *dst_spi_data; //用于dst的spi数据， 数据内容没有实际意义
    unsigned int thermal_head_clock_us; //热敏头开始加热的时间点

    unsigned int motor_deceleration_count; //电机减速计数
    unsigned int motor_accelerate_count; //电机启动加速计数
    unsigned int heat_state; //加热状态 0：启动加热 1：加热中  2：加热完成
    unsigned int motor_line_state; //0：电机运行中 1：电机一行步进完成,等待加热完成

    struct delayed_work head_overheat_dwork; //热敏头过热温度检测服务
    struct delayed_work motor_overheat_dwork; //马达过热保护函数
    struct delayed_work paper_empty_dwork; //缺纸检测的消抖延迟工作

    struct paper_properties paper; //纸张特性，用于动态适应打印机

    unsigned int data_int_offset; //数据处理偏移
    unsigned int data_bit_offset; //数据处理偏移

    unsigned int current_heat_dot;  //加热点数

    unsigned int printing_energy;   //打印加热能量
    unsigned int thermal_head_pulse_cycle;  //打印加热周期

    unsigned int new_line_flag; //打印新的一行标记
};

/* 热敏打印机结构体 */
static struct printer_dev thermal_printer;

/* 步进电机时序表 */
static unsigned char motor_step_level[8][4] = {
    { 1, 0, 0, 0 }, //Step0
    { 1, 0, 0, 1 }, //Step1
    { 0, 0, 0, 1 }, //Step2
    { 0, 1, 0, 1 }, //Step3
    { 0, 1, 0, 0 }, //Step4
    { 0, 1, 1, 0 }, //Step5
    { 0, 0, 1, 0 }, //Step6
    { 1, 0, 1, 0 }, //Step7
};

/* 1-2电机加速步骤 单位（us）*/
static unsigned int phase1_2_steps[] = {
    2812, 1738, 1342, 1127
};

/* 2-2电机加速步骤 单位（us）*/
static unsigned int phase2_2_steps[] = {
    1877, 1570, 1376, 1239, 1135, 1054,
    987, 933, 886, 845, 810, 779, 751, 726, 703, 681,
    663, 645, 629, 614, 600, 587, 575, 563, 552, 542,
    532, 522, 514, 506, 498, 490, 483, 476, 469, 463,
    457, 451, 445, 440, 435, 429, 425, 419, 415, 411,
    407, 403, 399, 395, 391, 387, 383, 379
};

/* 加热头的spi数据接口 */
static struct jzspi_config_data thermal_head_data_spi_config = {
    .id = TH_DATA_SPI_ID,
    .cs = 0,
    .clk_rate = TH_DATA_SPI_FREQ,
    .cs_valid_level = Valid_low,
    .transfer_mode = Transfer_irq_callback_mode,
    .tx_endian = Endian_mode0,
    .rx_endian = Endian_mode0,
    .bits_per_word = 16,                // 32 or 16
    .auto_cs = 1,
    .spi_pha = 0,
    .spi_pol = 0,
    .loop_mode = 0,
    .finish_irq_callback = thermal_head_data_complete,
};

/* 加热头的dst spi */
static struct jzspi_config_data thermal_head_dst_spi_config = {
    .id = TH_DST_SPI_ID,
    .cs = 0,
    .clk_rate = TH_DST_SPI_FREQ,
    .cs_valid_level = Valid_high,
    .transfer_mode = Transfer_irq_callback_mode,
    .tx_endian = Endian_mode0,
    .rx_endian = Endian_mode0,
    .bits_per_word = 16,                // 32 or 16
    .auto_cs = 1,
    .spi_pha = 0,
    .spi_pol = 0,
    .loop_mode = 0,
    .finish_irq_callback = thermal_head_dst_complete,
};

/* 获取本地时间节点 */
static inline unsigned int local_clock_us(void)
{
    return local_clock() >> 10;
}

/* 电机相位输出 */
static inline void motor_step_drive(struct printer_dev* dev)
{
    gpio_set_value(MOTOR_A_GPIO, dev->motor_step_level[dev->motor_step][0]);
    gpio_set_value(MOTOR_AA_GPIO, dev->motor_step_level[dev->motor_step][1]);
    gpio_set_value(MOTOR_B_GPIO, dev->motor_step_level[dev->motor_step][2]);
    gpio_set_value(MOTOR_BB_GPIO, dev->motor_step_level[dev->motor_step][3]);
}

/* 计算步进电机当前step的时间 */
static unsigned int calculate_motor_step_clock(struct printer_dev* dev)
{
    int i;
    unsigned int clock;
    unsigned int* phase_steps;
    unsigned int phase_steps_count;

    /* 计算上一次操作电机相位到这次操作电机相位的时间差 */
    clock = local_clock_us() - dev->motor_clock_us;

    /* phase_flag 2_2相加速模式为1  1_2相加速模式为0
        1_2相加模式与2_2相加速模式的相互转换， 需要的更新电机相位之前*/
    if (dev->phase_flag) {
        /* 在2_2相加速模式下，如果同时支持1_2相加速模式，并且步进电机的步进时间超过2_2相加速表的最大值，则减速为1_2加速模式 */
        if ((dev->phase1_2_steps_count > 0) && (clock > dev->phase2_2_steps[0])) {
            dev->phase_flag = 0;
            dev->current_acceleration_step = dev->phase1_2_steps_count - 1;
            phase_steps = dev->phase1_2_steps;
            phase_steps_count = dev->phase1_2_steps_count;
        } else {
            phase_steps = dev->phase2_2_steps;
            phase_steps_count = dev->phase2_2_steps_count;
        }
    } else {
        /* 在1_2相加速模式下，如果同时支持2_2相加速模式，并且步进电机的步进时间小于1_2相加速表的最小值，则加速为2_2加速模式 */
        if ((dev->phase2_2_steps_count > 0) && (clock <= dev->phase1_2_steps[dev->phase1_2_steps_count - 1])) {
            dev->phase_flag = 1;
            dev->current_acceleration_step = 0;
            phase_steps = dev->phase2_2_steps;
            phase_steps_count = dev->phase2_2_steps_count;
        } else {
            phase_steps = dev->phase1_2_steps;
            phase_steps_count = dev->phase1_2_steps_count;
        }
    }

    /* 获取上一步的加速表位置, 计算当前相位的时间 */
    i = dev->current_acceleration_step;
    for (i = dev->current_acceleration_step; i >= 0; i--) {
        if (clock <= phase_steps[i])
            break;
    }

    i++;
    if (i >= phase_steps_count)
        i = phase_steps_count - 1;

    clock = phase_steps[i];
    dev->current_acceleration_step = i;

    return clock;
}

/* 启动打印机 */
static void enable_printer(struct printer_dev* dev)
{
    if (!(dev->printer_status & PRINTER_WORKING)) {
        ktime_t kt;
        unsigned long flags;

        local_irq_save(flags);
        /* 设置打印机状态 注意这个并非原子操作*/
        dev->printer_status |= PRINTER_WORKING;
        local_irq_restore(flags);

        /* 使能adc控制器 */
        test_jz_adc_enable();
        test_jz_adc_channel_sample(TH_ADC_ID, 1);
        /* 使能加热头数据通讯spi控制器 */
        spi_start_config(&dev->data_spi_config);
        spi_start_config(&dev->dst_spi_config);
        /* 使能12v电源 */
        gpio_set_value(POWER_ENABLE_GPIO, POWER_ENABLE_LEVEL);
        /* 使能12v电源的pwm */
        pwm_config(dev->pwm, POWER_PWM_PERIOD / 2, POWER_PWM_PERIOD);
        pwm_enable(dev->pwm);
        /* 设置步进电机相位 */
        dev->motor_step = MOTOR_STEP_COUNT - 1;
        motor_step_drive(dev);
        /* 使能步进电机相位IC */
        gpio_set_value(MOTOR_ENABLE_GPIO, MOTOR_ENABLE_LEVEL);
        /* 记录步进电机相位修改时间 */
        dev->motor_clock_us = local_clock_us();
        dev->motor_start_clock = dev->motor_clock_us;

        /* 初始化加速和减速计数 */
        dev->motor_deceleration_count = 0;
        dev->motor_accelerate_count = 0;

        /* 初始化加热头状态 */
        dev->heat_state = THERMAL_HEAD_STATUS_END;
        /* 初始化电机状态 */
        dev->motor_line_state = MOTOR_STATUS_WORK;

        /* 如果有1_2的加速表， 优先使用
        定时时间为启动保持时间
        */
        if (dev->phase1_2_steps_count) {
            dev->phase_flag = 0;
            dev->current_acceleration_step = 0;
            kt = ktime_set(0, dev->phase1_2_steps[0] * 1000);
        } else {
            dev->phase_flag = 1;
            dev->current_acceleration_step = 0;
            kt = ktime_set(0, dev->phase2_2_steps[0] * 1000);
        }

        hrtimer_start(&dev->printer_timer, kt, HRTIMER_MODE_REL);
    }
}

/* 关闭打印机 */
static void disable_printer(struct printer_dev* dev)
{
    /* 关闭电机相位ic */
    gpio_set_value(MOTOR_ENABLE_GPIO, !MOTOR_ENABLE_LEVEL);
    gpio_set_value(MOTOR_A_GPIO, 0);
    gpio_set_value(MOTOR_AA_GPIO, 0);
    gpio_set_value(MOTOR_B_GPIO, 0);
    gpio_set_value(MOTOR_BB_GPIO, 0);
    /* 关闭12V电源 */
    gpio_set_value(POWER_ENABLE_GPIO, !POWER_ENABLE_LEVEL);
    pwm_disable(dev->pwm);
    /* 关闭加热头数据spi控制器 */
    spi_stop_config(&dev->data_spi_config);
    spi_stop_config(&dev->dst_spi_config);
    /* 关闭热敏头adc检测控制器 */
    test_jz_adc_disable();
    /* 更新电机状态 此函数在中断调用，不需要关中断设置状态*/
    dev->printer_status &= ~PRINTER_WORKING;
}

/* 加热头热敏电阻温度表
 -20 至 100 （°C）（欧姆）
 */
static unsigned int thermistor_temperature[] = {
    316971, 298074, 280440, 263974, 248592, 234215, 220771, 208194, 196421, 185397,
    175068, 165386, 156307, 147789, 139794, 132286, 125233, 118604, 112371, 106508,
    100990, 95796, 90903, 86293, 81948, 77851, 73985, 70337, 66893, 63640,
    60567, 57662, 54916, 52318, 49860, 47533, 45330, 43243, 41266, 39391,
    37614, 35928, 34329, 32810, 31369, 30000, 28699, 27462, 26287, 25169,
    24106, 23094, 22131, 21213, 20340, 19508, 18715, 17959, 17238, 16550,
    15894, 15268, 14670, 14099, 13554, 13033, 12536, 12060, 11605, 11170,
    10753, 10355, 9974, 9609, 9259, 8924, 8604, 8296, 8002, 7719,
    7448, 7188, 6939, 6700, 6470, 6249, 6038, 5834, 5639, 5451,
    5270, 5097, 4930, 4770, 4615, 4467, 4324, 4186, 4054, 3926,
    3803, 3685, 3571, 3461, 3355, 3253, 3154, 3059, 2968, 2879,
    2794, 2712, 2632, 2556, 2482, 2410, 2341, 2274, 2210, 2147,
    2087
};

/* 热敏头过热检测函数 */
static void head_overheat_work_func_t(struct work_struct* work)
{
    int i;
    unsigned int adc_data;
    int temperature;
    unsigned int termistor_resistance;
    struct delayed_work* head_overheat_dwork = to_delayed_work(work);
    struct printer_dev* dev = container_of(head_overheat_dwork, struct printer_dev, head_overheat_dwork);

    /* 使能adc控制器 */
    test_jz_adc_enable();
    test_jz_adc_channel_sample(TH_ADC_ID, 1);
    msleep(10);
    adc_data = test_jz_adc_read_value(TH_ADC_ID, 1);

    /* 关闭热敏头adc检测控制器 */
    test_jz_adc_disable();

    /* 热敏电阻的adc数据计算温度
        上拉电阻是30000欧姆， adc满量程是1024
    */
    termistor_resistance = 30000 * adc_data / (1024 - adc_data);

    /* 查表计算温度值 */
    for (i = 0; i < ARRAY_SIZE(thermistor_temperature); i++) {
        if (termistor_resistance >= thermistor_temperature[i])
            break;
    }
    /* 需要放在for循环之外，因为有可能查表匹配失败 */
    temperature = i - 20;

    /* 检查温度是否回复正常 */
    if (temperature < THERMAL_HEAD_RECOVERY_TEMPERATURE) {
        unsigned long flags;

        local_irq_save(flags);
        /* 温度恢复正常，清除标志位 重启打印机 */
        dev->printer_status &= ~PRINTER_THERMAL_HEAD_OVERHEAT;
        local_irq_restore(flags);

        /* 上锁确保不会和write接口同时启动打印机 */
        mutex_lock(&dev->lock);

        enable_printer(dev);

        mutex_unlock(&dev->lock);
    } else {
        /* 温度异常，重启温度检测 */
        schedule_delayed_work(&dev->head_overheat_dwork, HEAD_OVERHEAT_MONITORING_TIME);
    }
}

/* 电机过热回复函数 */
static void motor_overheat_work_func_t(struct work_struct* work)
{
    unsigned long flags;
    struct delayed_work* motor_overheat_dwork = to_delayed_work(work);
    struct printer_dev* dev = container_of(motor_overheat_dwork, struct printer_dev, motor_overheat_dwork);

    local_irq_save(flags);
    /* 温度恢复正常，清除标志位 重启打印机 */
    dev->printer_status &= ~PRINTER_MOTOR_OVERHEAT;
    local_irq_restore(flags);

    /* 上锁确保不会和write接口同时启动打印机 */
    mutex_lock(&dev->lock);

    enable_printer(dev);

    mutex_unlock(&dev->lock);
}

static void thermal_head_end(struct printer_dev* dev)
{
    dev->heat_state = THERMAL_HEAD_STATUS_END;
    /* 如果步进电机处于等待状态，重新启动定时器 */
    if (dev->motor_line_state == MOTOR_STATUS_WAIT) {
        ktime_t kt;
        unsigned int clock;
        dev->motor_line_state = MOTOR_STATUS_WORK;
        clock = calculate_motor_step_clock(dev);
        kt = ktime_set(0, (clock - MOTOR_STEP_ERROR_ALLOWABLE_TIME) * 1000);
        hrtimer_start(&dev->printer_timer, kt, HRTIMER_MODE_REL);
    }
}

/* 加热数据处理函数 */
static void thermal_head_data_process(struct printer_dev* dev)
{
    int i;
    unsigned int* bufp;
    unsigned int heat_dot;
    unsigned int data[TOTAL_DOTS / 8 / 4];

    bufp = (unsigned int*)&dev->printer_line_buf[dev->buf_read_flag * (TOTAL_DOTS / 8)];

    heat_dot = 0;
    memset((void*)data, 0, sizeof(data));
    for (; dev->data_int_offset < (TOTAL_DOTS / 8 / 4); dev->data_int_offset++) {
        for (; dev->data_bit_offset < 32; dev->data_bit_offset++) {
            /* 加热头spi数据高位先出，所以先判断高位 */
            if (bufp[dev->data_int_offset] & (0x80000000 >> dev->data_bit_offset)) {
                heat_dot++;
                /* 超出最大加热点数，记录当前位置并启动加热 */
                if (heat_dot > ACTIVATED_DOTS) {
                    heat_dot = ACTIVATED_DOTS;
                    goto thermal_head_heat;
                }
                data[dev->data_int_offset] |= (0x80000000 >> dev->data_bit_offset);
            }
        }
        dev->data_bit_offset = 0;
    }

    dev->data_int_offset = 0;
    dev->data_bit_offset = 0;
    /* 一行数据处理完，设置标志位，并通知缓冲区有空间 */
    dev->buf_read_flag++;
    if (dev->buf_read_flag >= BUFFER_LINE_COUNT)
        dev->buf_read_flag = 0;
    wake_up_interruptible(&dev->buf_wait);
thermal_head_heat:
    if (heat_dot > 0) {
        dev->current_heat_dot = heat_dot;
        dev->heat_state = THERMAL_HEAD_STATUS_HEAT;

        /* 如果spi控制器配置的是16bit，做数据转换 */
        if(dev->data_spi_config.bits_per_word == 16) {
            for(i = 0; i < (TOTAL_DOTS / 8 / 4); i++)
                data[i] = (data[i] << 16) | (data[i] >> 16);
        }
        /* 发送打印数据 */
        spi_write_read(&dev->data_spi_config, data, TOTAL_DOTS / dev->data_spi_config.bits_per_word, NULL, 0);
    } else {
        /* 不需要加热，加热流程完成 */
        thermal_head_end(dev);
    }
}

/* 加热头数据发送完成回调 */
static void thermal_head_data_complete(struct jzspi_config_data *config)
{
    unsigned int udelay_time;
    unsigned int printing_energy;
    unsigned int adjusted_resistance;
    struct printer_dev* dev = container_of(config, struct printer_dev, data_spi_config);
    /* 锁存数据 */
    gpio_set_value(LAT_GPIO, 0);
    ndelay(100);
    gpio_set_value(LAT_GPIO, 1);

    /* 同一行加热里面只计算一次的数据 */
    if (dev->new_line_flag) {
        int i;
        int temperature;
        unsigned int adc_data;
        unsigned int clock_interval;
        unsigned int termistor_resistance;
        dev->new_line_flag = 0;

        /* 获取热敏电阻adc数据，一行只读取一次温度，避开dst引起的电压波动问题 */
        adc_data = test_jz_adc_read_value(TH_ADC_ID, 0);

        /* 计算热敏电阻阻值
        上拉电阻是30000欧姆， adc满量程是1024
        */
        termistor_resistance = 30000 * adc_data / (1024 - adc_data);

        /* 根据热敏电阻阻值，查表计算温度值 */
        for (i = 0; i < ARRAY_SIZE(thermistor_temperature); i++) {
            if (termistor_resistance >= thermistor_temperature[i])
                break;
        }
        /* 需要放在for循环之外，因为有可能查表匹配失败 */
        temperature = i - 20;

        /* 温度异常处理 */
        if (temperature > THERMAL_HEAD_OVERHEAT_TEMPERATURE) {
            dev->printer_status |= PRINTER_THERMAL_HEAD_OVERHEAT;
            /* 温度异常，加热流程结束 */
            thermal_head_end(dev);
            return;
        }

        /* 印刷能量 单位（mJ * 1000000）
            E = (E25 - Tc * (Tx - 25)) * 1000000

            E25 ： 标准印刷能源,根据纸张差异变化 （0.4105 mJ）
            Tc ： 纸张的温度系数，根据纸张差异变化 （0.004200）
            Tx ： 热敏电阻检测到温度

            提示： 纸张的能量参数可以由应用层传参，以适用不同的纸张
        */
        dev->printing_energy = dev->paper.paper_printing_energy - dev->paper.paper_temperature_coefficient * (temperature - 25);

        /* 通过热头激活脉冲周期进行调整, 放大1000 浮点转整形
            在一行内用同一个C值
            C = 1000 - （1241000 / （W + 1516））

            W ： 从最后一个热头激活开始到当前热头激活开始（μs）之间的时间。 最大43690μs
        */
        clock_interval = local_clock_us() - dev->thermal_head_clock_us;
        if (clock_interval > 43690)
            clock_interval = 43690;
        dev->thermal_head_pulse_cycle = 1000 - (1241000 / (clock_interval + 1516));

        dev->thermal_head_clock_us = local_clock_us();
    }

    /* 热敏头电阻，单位（欧姆 * 1000）
        R = （(Rh + Riv + Ric + (Rc + rc) * Nsa)^2 / Rh ） * 1000
            = （219000 + 160 * Nsa） / 203

        Rh ： 热元件电阻 (203欧姆)
        Riv ： 个别接线电阻 （7欧姆）
        Ric ： IC中的导通电阻 （9欧姆）
        Rc ： 热敏头中的公共端子接线电阻 （0.160欧姆）
        rc ： Vp和GND之间的接线电阻 （0欧姆）
        Nsa ： 同时激活的点数
    */
    adjusted_resistance = (256 * dev->current_heat_dot * dev->current_heat_dot + 350400 * dev->current_heat_dot + 479610000) / 2030;

    /* 热敏头驱动电压 单位（V * 1000）
        For 5.5 V ≤ Vp ≤ 9.5 V
            V = 1.295 × Vp − 2.726
        For 10.8 V ≤ Vp ≤ 12.6 V
            V = Vp

        加热时间 t = （E * R * C ）/ V / V
        E = printing_energy
        R = adjusted_resistance
        V = 12000
        C = pulse_cycle_coefficient

        注意运算数据不能溢出
    */

    /* 把计算公式拆开，避免数据溢出 */
    printing_energy = dev->printing_energy / 100;
    adjusted_resistance = adjusted_resistance / 10;
    udelay_time = printing_energy * adjusted_resistance / 1200;
    udelay_time = udelay_time * dev->thermal_head_pulse_cycle / 1200 / 100;

    /* 计算spi的数据量决定dst电平
     * 128是spi控制器fifo模式单次最大发送量
     */
    udelay_time = udelay_time / dev->dst_spi_config.bits_per_word;
    if(udelay_time > 128)
        udelay_time = 128;
    spi_write_read(&dev->dst_spi_config, dev->dst_spi_data, udelay_time, NULL, 0);
}

/* 热敏头加热完成回调 */
static void thermal_head_dst_complete(struct jzspi_config_data *config)
{
    struct printer_dev* dev = container_of(config, struct printer_dev, dst_spi_config);

    if (!(dev->data_int_offset | dev->data_int_offset)) {
        /* 数据处理完成， 加热流程结束 */
        thermal_head_end(dev);
    } else {
        /* 一行数据还没处理完成，继续处理数据 */
        thermal_head_data_process(dev);
    }
}

/* 步进电机定时器 */
static enum hrtimer_restart printer_hrtimer_handler(struct hrtimer* timer)
{
    ktime_t kt;
    unsigned int clock;
    struct printer_dev* dev = container_of(timer, struct printer_dev, printer_timer);

    if (dev->printer_status & PRINTER_THERMAL_HEAD_OVERHEAT) {
        /* 关闭打印机电源 */
        disable_printer(dev);
        /* 打印机过热 */
        printk("printer thermal head overheat!!!\n");
        /* 通知写数据接口打印机过热 */
        wake_up_interruptible(&dev->buf_wait);
        /* 启动温度检测 */
        schedule_delayed_work(&dev->head_overheat_dwork, HEAD_OVERHEAT_MONITORING_TIME);
        return HRTIMER_NORESTART;
    }

    /* 计算电机步进时间 */
    clock = calculate_motor_step_clock(dev);

    /* 跟新电机相位，并设置相位记录时间点
        step先加后或，确保2_2相加速模式相位对齐
    */
    dev->motor_step++;

    /* 判断是否新的一行开始,可以处理新数据 */
    if (!(dev->motor_step % MOTOR_LINE_STEP))
        dev->new_line_flag = 1;

    if (dev->phase_flag)
        dev->motor_step |= 1;

    if (dev->motor_step >= MOTOR_STEP_COUNT)
        dev->motor_step -= MOTOR_STEP_COUNT;

    /* 设置步进电机，更新电机反转时间 */
    motor_step_drive(dev);
    dev->motor_clock_us = local_clock_us();

    /* 如果电机运行到一行的最后一步，而加热头还没加热完成，退出定时器等待加热服务启动定时器 */
    if ((!((dev->motor_step + 1) % MOTOR_LINE_STEP)) && (dev->heat_state != THERMAL_HEAD_STATUS_END))
    {
        dev->motor_line_state = MOTOR_STATUS_WAIT;
        return HRTIMER_NORESTART;
    }

    /* 判断是否有数据 */
    if ((dev->buf_write_flag == dev->buf_read_flag)) {
        /* 没有打印数据并且步进相位在停止相位， 关闭打印机 */
        if ((dev->motor_step == MOTOR_STEP_COUNT - 1) && (dev->motor_deceleration_count >= MOTOR_DECELERATION_COUNT)) {
            /* 关闭打印机电源 */
            disable_printer(dev);
            /* 通知fsync数据已经打印完成 */
            wake_up_interruptible(&dev->buf_flush_wait);
            return HRTIMER_NORESTART;
        }

        /* 步进电机减速逻辑，需要减速多少步之后才能关闭电机 */
        dev->motor_deceleration_count++;
        dev->current_acceleration_step = dev->current_acceleration_step - (dev->current_acceleration_step / MOTOR_DECELERATION_COUNT);
        if (dev->phase_flag) {
            clock = dev->phase2_2_steps[dev->current_acceleration_step];
        } else {
            clock = dev->phase1_2_steps[dev->current_acceleration_step];
        }

    } else {
        /* 有数据正常打印， 停止打印机的减速计数清零 */
        dev->motor_deceleration_count = 0;

        /* 电机启动加速，超过多少步之后才开始处理数据 */
        if (dev->motor_accelerate_count < MOTOR_ACCELERATE_COUNT) {
            dev->motor_accelerate_count++;
        } else {
            /* 有新行标记才能处理数据，没有标记说明这一行已经完整的加热过数据 */
            if (dev->new_line_flag) {
                /* 电机启动时间过长 */
                if ((local_clock_us() - dev->motor_start_clock) > MOTOR_RUN_MAX_TIME) {
                    /* 关闭打印机电源 */
                    disable_printer(dev);
                    printk("printer motor overheat!!!\n");
                    dev->printer_status |= PRINTER_MOTOR_OVERHEAT;
                    /* 通知写数据接口打印机马达过热 */
                    wake_up_interruptible(&dev->buf_wait);
                    /* 延迟冷却电机 */
                    schedule_delayed_work(&dev->motor_overheat_dwork, MOTOR_OVERHEAT_SLEEP_TIME);
                    return HRTIMER_NORESTART;
                }

                /* 打印机缺纸停止打印 */
                if (dev->printer_status & PRINTER_PAPER_EMPTY) {
                    /* 关闭打印机电源 */
                    disable_printer(dev);
                    /* 通知写数据接口打印机缺纸 */
                    wake_up_interruptible(&dev->buf_wait);
                    return HRTIMER_NORESTART;
                }

                /* 数据偏移复位 */
                dev->data_int_offset = 0;
                dev->data_bit_offset = 0;
                test_jz_adc_channel_sample(TH_ADC_ID, 0);
                thermal_head_data_process(dev);
            }
        }
    }

    kt = ktime_set(0, (clock - MOTOR_STEP_ERROR_ALLOWABLE_TIME) * 1000);
    hrtimer_forward_now(timer, kt);

    return HRTIMER_RESTART;
}

/* 缺纸服务不主动恢复打印 */
static void paper_empty_work_func_t(struct work_struct* work)
{
    unsigned long flags;
    struct delayed_work* paper_empty_dwork = to_delayed_work(work);
    struct printer_dev* dev = container_of(paper_empty_dwork, struct printer_dev, paper_empty_dwork);

    local_irq_save(flags);
    if (gpio_get_value(PS_GPIO) == PS_PAPER_EMPTY_LEVEL) {
        dev->printer_status |= PRINTER_PAPER_EMPTY;
        printk("printer paper empty!!!\n");
    } else {
        dev->printer_status &= ~PRINTER_PAPER_EMPTY;
    }
    local_irq_restore(flags);
}

/* 缺纸中断 */
irqreturn_t ps_irq_handler(int irq, void* data)
{
    struct printer_dev* dev = (struct printer_dev*)data;

    /* 使用延迟工作队列消抖 10ms*/
    schedule_delayed_work(&dev->paper_empty_dwork, HZ / 100);

    return IRQ_HANDLED;
}

static int thermal_printer_open(struct inode* inode, struct file* filp)
{
    int ret = -EBUSY;
    struct miscdevice* mdev = filp->private_data;
    struct printer_dev* dev = container_of(mdev, struct printer_dev, printer_mdev);

    /* 上锁同一时刻只能有一个线程操作printer_mdev_open */
    mutex_lock(&dev->lock);

    if (!dev->printer_mdev_open) {
        dev->printer_mdev_open = 1;
        ret = 0;
    }

    mutex_unlock(&dev->lock);

    return ret;
}

static int thermal_printer_close(struct inode* inode, struct file* filp)
{
    struct miscdevice* mdev = filp->private_data;
    struct printer_dev* dev = container_of(mdev, struct printer_dev, printer_mdev);

    /* 上锁同一时刻只能有一个线程操作printer_mdev_open */
    mutex_lock(&dev->lock);
    dev->printer_mdev_open = 0;
    mutex_unlock(&dev->lock);

    return 0;
}

static long thermal_printer_ioctl(struct file* filp, unsigned int cmd, unsigned long args)
{
    int ret = 0;
    struct paper_properties *paper;
    struct miscdevice* mdev = filp->private_data;
    struct printer_dev* dev = container_of(mdev, struct printer_dev, printer_mdev);

    /* handle ioctls */
    switch (cmd) {
    case GET_PRINTER_STATUS:
        ret = copy_to_user((void*)args, &dev->printer_status, sizeof(dev->printer_status));
        if (ret) {
            ret = -EINVAL;
            printk("ERROR: line %d, function %s, err %d.\n", __LINE__, __FUNCTION__, ret);
        }
        break;
    case SET_PAPER_PROPERTIES:
        paper = (struct paper_properties *)args;
        /* user can check paper parameter
         * paper->paper_printing_energy > MAX
         * paper->paper_temperature_coefficient > MAX
        */
        ret = copy_from_user(&dev->paper, paper, sizeof(struct paper_properties));
        if (ret) {
            ret = -EINVAL;
            printk("ERROR: line %d, function %s, err %d.\n", __LINE__, __FUNCTION__, ret);
        }
        break;
    default:
        /* could not handle ioctl */
        ret = -ENOTTY;
        printk("ERROR: line %d, function %s, err %d.\n", __LINE__, __FUNCTION__, ret);
    }

    return ret;
}

static ssize_t thermal_printer_write(struct file* filp, const char __user* data, size_t len, loff_t* offset)
{
    int i, ret;
    unsigned int tmp;
    unsigned int count;
    struct miscdevice* mdev = filp->private_data;
    struct printer_dev* dev = container_of(mdev, struct printer_dev, printer_mdev);

    /* 如果写长度为零，直接返回 */
    if (len == 0) {
        return 0;
    }

    /* 数据大小不是行对齐，返回错误 */
    if (len % (TOTAL_DOTS / 8)) {
        ret = -EINVAL;
        printk("line %d, function %s, err %d.\n", __LINE__, __FUNCTION__, ret);
        return ret;
    }

    /* 打印机状态异常 */
    if (dev->printer_status & PRINTER_ERROR_MASK) {
        printk("Error printer status %x\n", dev->printer_status);
        return -EIO;
    }

    /* 上锁同一时刻只能有一个线程操作buf_write_flag */
    mutex_lock(&dev->lock);

    tmp = dev->buf_write_flag + 1;
    if (tmp >= BUFFER_LINE_COUNT)
        tmp = 0;

    /* 如果write_flag + 1 等于 read_flag 那么说明缓冲buf数据已满
        由于lock保证了tmp = dev->buf_write_flag + 1 一直成立
    */
    if (dev->buf_read_flag == tmp) {
        /* 如果设备以为非阻塞方式打开，直接返回错误代码 */
        if (filp->f_flags & (O_NONBLOCK | O_NDELAY)) {
            mutex_unlock(&dev->lock);
            return -EAGAIN;
        }
        /* 等待数据缓冲区可以装填数据 */
        wait_event_interruptible(dev->buf_wait, (dev->buf_read_flag != tmp) || (dev->printer_status & PRINTER_ERROR_MASK));

        /* 打印机状态异常 */
        if (dev->printer_status & PRINTER_ERROR_MASK) {
            mutex_unlock(&dev->lock);
            printk("Error printer status %x\n", dev->printer_status);
            return -EIO;
        }
    }

    /* 计算缓冲区剩余大小 单位：一行数据
    注意： 计算count时不能为负数， 类型定义是无符号类型*/
    count = dev->buf_read_flag + (BUFFER_LINE_COUNT - 1) - dev->buf_write_flag;
    if (count >= BUFFER_LINE_COUNT)
        count -= BUFFER_LINE_COUNT;

    /* 计算写入缓冲区数据大小 单位：一行数据 */
    count = min(count, len / (TOTAL_DOTS / 8));

    for(i = 0; i < count; i++) {
        /* 必须一次拷贝一行，确保printer_line_buf不会溢出 注意printer_line_buf的指针是char* */
        ret = copy_from_user(dev->printer_line_buf + dev->buf_write_flag * (TOTAL_DOTS / 8), data + i * (TOTAL_DOTS / 8), (TOTAL_DOTS / 8));
        if(ret) {
            mutex_unlock(&dev->lock);
            ret = -ENOMEM;
            printk("line %d, function %s, err %d.\n", __LINE__, __FUNCTION__, ret);
            return i * (TOTAL_DOTS / 8);
        }
        /* write_flag不能大于BUFFER_LINE_COUNT， 需要先判断再加
            如果大于BUFFER_LINE_COUNT，刚好产生中断会内存溢出 */
        if(dev->buf_write_flag + 1 >= BUFFER_LINE_COUNT)
            dev->buf_write_flag = 0;
        else
            dev->buf_write_flag++;
    }

    enable_printer(dev);

    mutex_unlock(&dev->lock);
    return count * (TOTAL_DOTS / 8);
}

/* poll 等待缓冲区可以写入 */
static unsigned int thermal_printer_poll(struct file* filp, struct poll_table_struct* wait)
{
    int status = 0;
    unsigned long tmp;
    struct miscdevice* mdev = filp->private_data;
    struct printer_dev* dev = container_of(mdev, struct printer_dev, printer_mdev);

    poll_wait(filp, &dev->buf_wait, wait);

    /* 上锁同一时刻只能有一个线程操作buf_write_flag */
    mutex_lock(&dev->lock);

    /* 如果write_flag + 1 等于 read_flag 那么说明缓冲buf数据已满
        由于lock保证了tmp = dev->buf_write_flag + 1 一直成立
     */
    tmp = dev->buf_write_flag + 1;
    if (tmp >= BUFFER_LINE_COUNT)
        tmp = 0;

    if (dev->buf_read_flag != tmp)
        status |= POLLOUT | POLLWRNORM;

    /* 打印机状态异常 */
    if (dev->printer_status & PRINTER_ERROR_MASK)
        status |= POLLERR;

    mutex_unlock(&dev->lock);

    return status;
}

/* fsync 等待缓冲区所有数据都打印完成 */
static int thermal_printer_fsync(struct file* filp, loff_t start, loff_t end, int datasync)
{
    struct miscdevice* mdev = filp->private_data;
    struct printer_dev* dev = container_of(mdev, struct printer_dev, printer_mdev);

    /* write_flag等于read_flag， 说明数据缓冲区为空 */
    if ((dev->printer_status & PRINTER_WORKING) || (dev->buf_write_flag != dev->buf_read_flag)) {
        wait_event_interruptible(dev->buf_flush_wait, ((!(dev->printer_status & PRINTER_WORKING)) && (dev->buf_write_flag == dev->buf_read_flag)));
    }

    return 0;
}

static struct file_operations thermal_printer_fops = {
    .owner = THIS_MODULE,
    .open = thermal_printer_open,
    .write = thermal_printer_write,
    .fsync = thermal_printer_fsync,
    .poll = thermal_printer_poll,
    .unlocked_ioctl = thermal_printer_ioctl,
    .release = thermal_printer_close,
    .llseek = noop_llseek,
};

/* 问题：没做出错资源释放逻辑 */
static int __init thermal_printer_init(void)
{
    int ret;
    struct pwm_device* pwm;
    /* 抽象出结构体方便后面封装平台驱动 */
    struct printer_dev* dev = &thermal_printer;

    ret = gpio_request_one(PS_GPIO, GPIOF_DIR_IN, "printer_ps");
    if (ret < 0) {
        printk("line %d, function %s, err %d.\n", __LINE__, __FUNCTION__, ret);
        return ret;
    }

    /* 使能斯密特触发器功能 */
    jz_gpio_set_func(PS_GPIO, GPIO_SMT_ENABLE);
    /* 设置硬件消抖 */
    jz_gpio_set_glitch_filter(PS_GPIO, 0x0f);

    /* 12V电源使能 */
    ret = gpio_request_one(POWER_ENABLE_GPIO, POWER_ENABLE_LEVEL ? GPIOF_OUT_INIT_LOW : GPIOF_OUT_INIT_HIGH, "printer_power_enable");
    if (ret < 0) {
        printk("line %d, function %s, err %d.\n", __LINE__, __FUNCTION__, ret);
        return ret;
    }

    /* 步进电机相位ic使能io */
    ret = gpio_request_one(MOTOR_ENABLE_GPIO, MOTOR_ENABLE_LEVEL ? GPIOF_OUT_INIT_LOW : GPIOF_OUT_INIT_HIGH, "printer_motor_enable");
    if (ret < 0) {
        printk("line %d, function %s, err %d.\n", __LINE__, __FUNCTION__, ret);
        return ret;
    }

    /* 注意：热敏头数据锁存io，低电平使能 */
    ret = gpio_request_one(LAT_GPIO, GPIOF_OUT_INIT_HIGH, "printer_lat");
    if (ret < 0) {
        printk("line %d, function %s, err %d.\n", __LINE__, __FUNCTION__, ret);
        return ret;
    }

    ret = gpio_request_one(MOTOR_A_GPIO, GPIOF_OUT_INIT_LOW, "printer_motor_a");
    if (ret < 0) {
        printk("line %d, function %s, err %d.\n", __LINE__, __FUNCTION__, ret);
        return ret;
    }

    ret = gpio_request_one(MOTOR_AA_GPIO, GPIOF_OUT_INIT_LOW, "printer_motor_aa");
    if (ret < 0) {
        printk("line %d, function %s, err %d.\n", __LINE__, __FUNCTION__, ret);
        return ret;
    }

    ret = gpio_request_one(MOTOR_B_GPIO, GPIOF_OUT_INIT_LOW, "printer_motor_b");
    if (ret < 0) {
        printk("line %d, function %s, err %d.\n", __LINE__, __FUNCTION__, ret);
        return ret;
    }

    ret = gpio_request_one(MOTOR_BB_GPIO, GPIOF_OUT_INIT_LOW, "printer_motor_bb");
    if (ret < 0) {
        printk("line %d, function %s, err %d.\n", __LINE__, __FUNCTION__, ret);
        return ret;
    }

    /* 12v电源 pwm控制器 */
    dev->pwm = pwm_request(POWER_PWM_ID, "printer_pwm");
    if (IS_ERR(pwm)) {
        ret = PTR_ERR(pwm);
        printk("line %d, function %s, err %d.\n", __LINE__, __FUNCTION__, ret);
        return ret;
    }

    /* 加热头的spi数据接口 */
    memcpy(&dev->data_spi_config, &thermal_head_data_spi_config, sizeof(thermal_head_data_spi_config));

    /* 加热头的dst spi */
    memcpy(&dev->dst_spi_config, &thermal_head_dst_spi_config, sizeof(thermal_head_dst_spi_config));

    /* 步进电机相位时序表 */
    dev->motor_step_level = motor_step_level;

    /* 初始化热敏头开始加热的时间点 */
    dev->thermal_head_clock_us = local_clock_us();

    /* 设置纸张的默认参数 */
    dev->paper.paper_printing_energy = PAPER_PRINTING_ENERGY;
    dev->paper.paper_temperature_coefficient = PAPER_TEMPERATURE_COEFFICIENT;

    /* 打印机主逻辑定时器 */
    hrtimer_init(&dev->printer_timer, CLOCK_MONOTONIC, HRTIMER_MODE_REL);
    dev->printer_timer.function = printer_hrtimer_handler;

    /* 初始化过热检测服务 */
    INIT_DELAYED_WORK(&dev->head_overheat_dwork, head_overheat_work_func_t);
    INIT_DELAYED_WORK(&dev->motor_overheat_dwork, motor_overheat_work_func_t);

    /* 初始化缺纸检测消抖延迟工作队列 */
    INIT_DELAYED_WORK(&dev->paper_empty_dwork, paper_empty_work_func_t);

    /* 步进电机加速时间表 支持单独1_2模式， 单独2_2模式，1_2转2_2模式
        如果不支持对应模式， steps_count为0。
    */
    dev->phase1_2_steps = phase1_2_steps;
    dev->phase1_2_steps_count = ARRAY_SIZE(phase1_2_steps);
    dev->phase2_2_steps = phase2_2_steps;
    dev->phase2_2_steps_count = ARRAY_SIZE(phase2_2_steps);

    /* 加速表异常 */
    if (dev->phase1_2_steps_count == 0 && dev->phase2_2_steps_count == 0) {
        ret = -EINVAL;
        printk("ERROR: line %d, function %s, err %d.\n", __LINE__, __FUNCTION__, ret);
        return ret;
    }

    /* 应用层操作锁 */
    mutex_init(&dev->lock);

    /* 缓存数据填充等待队列 */
    init_waitqueue_head(&dev->buf_wait);
    init_waitqueue_head(&dev->buf_flush_wait);

    /* 检测纸张状态 */
    if (gpio_get_value(PS_GPIO) == PS_PAPER_EMPTY_LEVEL) {
        dev->printer_status |= PRINTER_PAPER_EMPTY;
        printk("printer paper empty!!!\n");
    }

    /* 注册缺纸检测中断 */
    ret = request_irq(gpio_to_irq(PS_GPIO), ps_irq_handler, IRQF_TRIGGER_RISING | IRQF_TRIGGER_FALLING, "printer_ps", dev);
    if (ret < 0) {
        printk("line %d, function %s, err %d.\n", __LINE__, __FUNCTION__, ret);
        return ret;
    }

    /* 申请缓冲区内存 指针是char* */
    dev->printer_line_buf = kzalloc(TOTAL_DOTS / 8 * BUFFER_LINE_COUNT, GFP_KERNEL);
    if (dev->printer_line_buf == NULL) {
        ret = -ENOMEM;
        printk("ERROR: line %d, function %s, err %d.\n", __LINE__, __FUNCTION__, ret);
        return ret;
    }

    dev->dst_spi_data = kzalloc(dev->dst_spi_config.bits_per_word / 8 * 128, GFP_KERNEL);
    if (dev->dst_spi_data == NULL) {
        ret = -ENOMEM;
        printk("ERROR: line %d, function %s, err %d.\n", __LINE__, __FUNCTION__, ret);
        return ret;
    }

    /* 注册应用层操作接口杂项设备 */
    dev->printer_mdev.minor = MISC_DYNAMIC_MINOR;
    dev->printer_mdev.name = "thermal_printer";
    dev->printer_mdev.fops = &thermal_printer_fops;

    ret = misc_register(&dev->printer_mdev);
    if (ret) {
        printk("ERROR: line %d, function %s, err %d.\n", __LINE__, __FUNCTION__, ret);
        return ret;
    }

    return 0;
}

static void __exit thermal_printer_exit(void)
{
    struct printer_dev* dev = &thermal_printer;

    misc_deregister(&dev->printer_mdev);
    kfree(dev->dst_spi_data);
    dev->dst_spi_data = NULL;
    kfree(dev->printer_line_buf);
    dev->printer_line_buf = NULL;
    free_irq(gpio_to_irq(PS_GPIO), dev);
    pwm_free(dev->pwm);

    gpio_free(MOTOR_BB_GPIO);
    gpio_free(MOTOR_B_GPIO);
    gpio_free(MOTOR_AA_GPIO);
    gpio_free(MOTOR_A_GPIO);
    gpio_free(LAT_GPIO);
    gpio_free(MOTOR_ENABLE_GPIO);
    gpio_free(POWER_ENABLE_GPIO);
    gpio_free(PS_GPIO);
}

late_initcall(thermal_printer_init);
module_exit(thermal_printer_exit);
