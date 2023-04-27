#ifndef _MD_PWM_H_
#define _MD_PWM_H_

enum pwm_shutdown_mode {
    PWM_graceful_shutdown,  /* pwm停止输出时,尽量保证pwm的信号结尾是一个完整的周期 */
    PWM_abrupt_shutdown,    /* pwm停止输出时,立刻将pwm设置成空闲时电平 */
};

enum pwm_idle_level {
    PWM_idle_low,   /* pwm 空闲时电平为低 */
    PWM_idle_high,  /* pwm 空闲时电平为高 */
};

enum pwm_accuracy_priority {
    PWM_accuracy_freq_first,    /* 优先满足pwm的目标频率的精度,级数可能不准确 */
    PWM_accuracy_levels_first,  /* 优先满足pwm的级数设置,pwm频率可能不准确 */
};

struct pwm_config_data {
    enum pwm_shutdown_mode shutdown_mode;
    enum pwm_idle_level idle_level;
    enum pwm_accuracy_priority accuracy_priority;
    unsigned long freq;
    unsigned long levels;
    int id;
};

int pwm2_set_level(int id, unsigned long level);

int pwm2_config(int id, struct pwm_config_data *config);

int pwm2_request(int gpio, const char *name);

int pwm2_release(int id);


/* ---- dma mode ---- */

enum pwm_dma_start_level {
    PWM_start_low,   /* pwm dma模式的起始电平为低 */
    PWM_start_high,  /* pwm dma模式的起始电平为高 */
};

/*
    注意: high和low不能为零
        时间单位由pwm2_dma_init返回
 */
struct pwm_data {
    /* 低电平个数 */
    unsigned low:16;
    /* 高电平个数 */
    unsigned high:16;
};

struct pwm_dma_config {
    int id;
    enum pwm_idle_level idle_level;
    enum pwm_dma_start_level start_level;
};

struct pwm_dma_data {
    struct pwm_data *data;
    unsigned int data_count;
    unsigned int dma_loop;
    int id;
};

/* 初始化pwm的dma模式
    返回值： 失败返回-1, 成功返回dma模式频率*/
int pwm2_dma_init(int id, struct pwm_dma_config *dma_config);

/*
    使用dma模式连续更新pwm的频率
    普通dma模式: 函数会阻塞到dma数据全部转换成对应pwm输出
    循环dma模式：函数不会阻塞，需要调用pwm2_dma_disable_loop停止dma
 */
int pwm2_dma_update(int id, struct pwm_dma_data *dma_data);

/* 停止dma的循环模式 */
int pwm2_dma_disable_loop(int id);

#endif /* _MD_PWM_H_ */

