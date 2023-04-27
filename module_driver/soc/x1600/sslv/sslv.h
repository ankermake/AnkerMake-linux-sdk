#ifndef __MD_SSLV_H__
#define __MD_SSLV_H__

#include <linux/mutex.h>

enum sslv_status_type {
    SSLV_IDLE,
    SSLV_BUSY,
};

struct sslv_config_data {
    unsigned int id;        /* SSLV 控制器 ID */
    char *name;             /* name 仅作为一个标识 */

    /* bits_per_word 代表数据的位宽.
        例如:bits_per_word = 32 时,SSLV传输过程中的最小数据单位为32bit. */
    unsigned int bits_per_word;

    /**
     * 极性 sslv_pol :
     * 当sslv_pol=0，在时钟空闲即无数据传输时,clk电平为低电平
     * 当sslv_pol=1，在时钟空闲即无数据传输时,clk电平为高电平
     * 相位 sslv_pha :
     * 当sslv_pha=0，表示在第一个跳变沿开始传输数据，下一个跳变沿完成传输
     * 当sslv_pha=1，表示在第二个跳变沿开始传输数据，下一个跳变沿完成传输
     */
    unsigned int sslv_pol;
    unsigned int sslv_pha;

    unsigned int loop_mode; /* 循环模式 */
};

struct sslv_device {
    struct sslv_config_data *config;
    int sslv_id;
    int status;
    int rx_flags;
    int cb_flags;
    struct mutex lock;
};

/**
 * @brief SSLV 设备使能: 将用户设置的Pol/Pha/Bits_per_word等参数写入到寄存器内
 * @param config config 结构体
 * @return 成功返回sslv设备结构体, 失败返回NULL
 */
struct sslv_device *sslv_enable(struct sslv_config_data *config);

/**
 * @brief SSLV 设备失能: 关时钟, 失能SSLV传输等
 * @param dev 注册时生成的sslv设备结构体
 * @return 成功返回0, 失败返回-1
 */
int sslv_disable(struct sslv_device *dev);

/**
 * @brief 获取 SSLV 接收FIFO个数: 读取接收FIFO个数寄存器并将结果返回
 * @param dev 注册时生成的sslv设备结构体
 * @return 返回未读的接收fifo个数
 */
unsigned int sslv_get_rx_fifo_num(struct sslv_device *dev);

/**
 * @brief SSLV 写FIFO数据: 将DATA写入数据寄存器
 * @param dev 注册时生成的sslv设备结构体
 * @param data 需要发送出去的数据
 * @return 无返回值
 */
void sslv_write_tx_fifo(struct sslv_device *dev, unsigned int data);

/**
 * @brief SSLV 读FIFO数据: 读取数据寄存器内接收到的一个数据
 * @param dev 注册时生成的sslv设备结构体
 * @return 返回读取到的一个数据值(读不到数据不退出)
 */
unsigned int sslv_read_rx_fifo(struct sslv_device *dev);

/**
 * @brief SSLV 发送: 将tx_buf内的tx_len个数据依次写入到数据寄存器然后通过FIFO发送出去,
 *          FIFO写满时等待直至数据全部写入
 * @param dev 注册时生成的sslv设备结构体
 * @param tx_buf 发送数据缓冲区
 * @param tx_len 发送数据个数(单位是bits_per_word)
 * @return 成功返回发送的数据字节数, 失败返回-1
 */
int sslv_transmit(struct sslv_device *dev, unsigned char *tx_buf, int tx_len);

/**
 * @brief SSLV 接收: 读取数据寄存器内rx_len个数据并写入到rx_buf内,
 *          FIFO内数据不够时等待直至数据接收完全
 * @param dev 注册时生成的sslv设备结构体
 * @param rx_buf 接收数据缓冲区
 * @param rx_len 接收数据个数(单位是bits_per_word)
 * @return 成功返回接收的数据字节数, 失败返回-1
 */
int sslv_receive(struct sslv_device *dev, unsigned char *rx_buf, int rx_len);

/**
 * @brief SSLV 回调接收: 在开启中断前设置接收FIFO阈值为rx_threshold,
 *      若接收FIFO个数大于rx_threshold就进入中断, 在中断内跳转至cb回调函数处理数据,
 *      直至应用调用sslv_stop_cb_receive关闭回调传输后停止接收数据
 * @param dev 注册时生成的sslv设备结构体
 * @param rx_threshold 设置接收FIFO阈值(一般设置为0, 最大为64)
 * @param cb 回调函数
 * @return 成功返回0, 失败返回-1
 */
int sslv_start_cb_receive(struct sslv_device *dev, unsigned char rx_threshold,
                    void (*cb)(struct sslv_device *dev));

/**
 * @brief SSLV 停止回调接收: 调用后关闭接收中断从而停止回调接收数据
 * @param dev 注册时生成的sslv设备结构体
 * @return 成功返回0, 失败返回-1
 */
int sslv_stop_cb_receive(struct sslv_device *dev);

#endif