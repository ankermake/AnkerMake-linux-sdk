#include <bit_field.h>
#include <common.h>
#include <linux/kernel.h>
#include <linux/io.h>
#include <linux/irq.h>
#include <dt-bindings/interrupt-controller/x2000-v12-irq.h>
#include <dt-bindings/dma/ingenic-pdma.h>

#include <soc/base.h>

#include <linux/interrupt.h>
#include <linux/clk.h>
#include <linux/gpio.h>
#include <linux/module.h>
#include <linux/delay.h>
#include <linux/kthread.h>
#include <linux/completion.h>
#include <linux/dmaengine.h>
#include <linux/dma-mapping.h>

enum uart_parity {
    UART_PARITY_NONE,
    UART_PARITY_ODD,
    UART_PARITY_EVEN,
};

enum uart_follow_contrl {
    UART_FC_NONE,
    UART_FC_CTS_RTS,
};

struct uart_config {
    unsigned char uart_id;
    unsigned char data_bits;
    unsigned char stop_bits;
    unsigned char loop_mode;
    unsigned char rx_poll_mode;
    unsigned char rx_dma_mode;
    unsigned char tx_poll_mode;
    enum uart_parity parity;
    enum uart_follow_contrl follow_contrl;
    unsigned int baud_rate;

    int rx_dma_bufsize;
};


#include "uart_baud_data.h"

#define URBR  0x000
#define UTHR  0x000
#define UDLLR 0x000
#define UDLHR 0x004
#define UIER  0x004
#define UIIR  0x008
#define UFCR  0x008
#define ULCR  0x00c
#define UMCR  0x010
#define ULSR  0x014
#define UMSR  0x018
#define USPR  0x01c
#define ISR   0x020
#define UMR   0x024
#define UACR  0x028
#define URCR  0x040
#define UTCR  0x044
#define URTDCR 0x050

#define UIER_RTOIE 4, 4
#define UIER_MSIE  3, 3
#define UIER_RLSIE 2, 2
#define UIER_TDRIE 1, 1
#define UIER_RDRIE 0, 0

#define UIIR_FFMSEL 6, 7
#define UIIR_INID   1, 3
#define UIIR_INPEND 0, 0

#define UFCR_RDTR 6, 7
#define UFCR_UME  4, 4
#define UFCR_DME  3, 3
#define UFCR_TFRT 2, 2
#define UFCR_RFRT 1, 1
#define UFCR_FME  0, 0

#define ULCR_DLAB 7, 7
#define ULCR_SBK  6, 6
#define ULCR_STPAR 5, 5
#define ULCR_PARM 4, 4
#define ULCR_PARE 3, 3
#define ULCR_SBLS 2, 2
#define ULCR_WLS 0, 1

#define UMCR_MDCE 7, 7
#define UMCR_FCM 6, 6
#define UMCR_LOOP 4, 4
#define UMCR_RTS 1, 1

#define ULSR_FIFOE 7, 7
#define ULSR_TEMP 6, 6
#define ULSR_TDRQ 5, 5
#define ULSR_BI 4, 4
#define ULSR_FMER 3, 3
#define ULSR_PARER 2, 2
#define ULSR_OVER 1, 1
#define ULSR_DRY 0, 0

#define UMSR_CTS 4, 4
#define UMSR_CCTS 0, 0

#define UART_FIFO_LEN 64
#define UART_NUMS 10

#define RTD_EN 7, 7
#define RTD_R 1, 1
#define RTD_T 0, 0

#ifndef UART6_IOBASE
#define UART6_IOBASE   0x10036000 /* 4KB, UART6 Controller */
#define UART7_IOBASE   0x10037000 /* 4KB, UART7 Controller */
#define UART8_IOBASE   0x10038000 /* 4KB, UART8 Controller */
#define UART9_IOBASE   0x10039000 /* 4KB, UART9 Controller */
#endif

static const unsigned long iobase[] = {
    KSEG1ADDR(UART0_IOBASE),
    KSEG1ADDR(UART1_IOBASE),
    KSEG1ADDR(UART2_IOBASE),
    KSEG1ADDR(UART3_IOBASE),
    KSEG1ADDR(UART4_IOBASE),
    KSEG1ADDR(UART5_IOBASE),
    KSEG1ADDR(UART6_IOBASE),
    KSEG1ADDR(UART7_IOBASE),
    KSEG1ADDR(UART8_IOBASE),
    KSEG1ADDR(UART9_IOBASE),
};

enum {
    DMA_RQ_UART5_RX = 0b001011, /* UART5 receive-fifo-full transfer request. (URBR --> external address) */
    DMA_RQ_UART4_RX = 0b001101, /* UART4 receive-fifo-full transfer request. (URBR --> external address) */
    DMA_RQ_UART3_RX = 0b001111, /* UART3 receive-fifo-full transfer request. (URBR --> external address) */
    DMA_RQ_UART2_RX = 0b010001, /* UART2 receive-fifo-full transfer request. (URBR --> external address) */
    DMA_RQ_UART1_RX = 0b010011, /* UART1 receive-fifo-full transfer request. (URBR --> external address) */
    DMA_RQ_UART0_RX = 0b010101, /* UART0 receive-fifo-full transfer request. (URBR --> external address) */
    DMA_RQ_UART6_RX = 0b110001, /* UART6 receive-fifo-full transfer request. (URBR --> external address) */
    DMA_RQ_UART7_RX = 0b110011, /* UART7 receive-fifo-full transfer request. (URBR --> external address) */
    DMA_RQ_UART8_RX = 0b110101, /* UART8 receive-fifo-full transfer request. (URBR --> external address) */
    DMA_RQ_UART9_RX = 0b110111, /* UART9 receive-fifo-full transfer request. (URBR --> external address) */
};

static const char rxdma_slaveid[] = {
    [5] = DMA_RQ_UART5_RX,
    [4] = DMA_RQ_UART4_RX,
    [3] = DMA_RQ_UART3_RX,
    [2] = DMA_RQ_UART2_RX,
    [1] = DMA_RQ_UART1_RX,
    [0] = DMA_RQ_UART0_RX,
    [6] = DMA_RQ_UART6_RX,
    [7] = DMA_RQ_UART7_RX,
    [8] = DMA_RQ_UART8_RX,
    [9] = DMA_RQ_UART9_RX,
};

#define UART_ADDR(id, reg)    ((volatile unsigned long *)(iobase[id] + reg))

static inline void uart_write_reg(int id, unsigned int reg, int val)
{
    *UART_ADDR(id, reg) = val;
}

static inline unsigned int uart_read_reg(int id, unsigned int reg)
{
    return *UART_ADDR(id, reg);
}

static inline void uart_set_bit(int id, unsigned int reg, int start, int end, unsigned int val)
{
    set_bit_field(UART_ADDR(id, reg), start, end, val);
}

static inline unsigned int uart_get_bit(int id, unsigned int reg, int start, int end)
{
    return get_bit_field(UART_ADDR(id, reg), start, end);
}

static inline unsigned int reg_get_bit(unsigned int reg, int start, int end)
{
    return (reg & bit_field_mask(start, end)) >> start;
}

static struct baudtoregs_t *uart_get_bauddata(u32 baud)
{
    int i;

    for (i = 0; i < ARRAY_SIZE(baudtoregs); i++) {
        if (baudtoregs[i].baud >= baud)
            return &baudtoregs[i];
    }

    return &baudtoregs[i-1];;
}

static void uart_init_config(struct uart_config *config)
{
    int id = config->uart_id;

    assert_range(config->stop_bits, 1, 2);
    assert_range(config->data_bits, 5, 8);
    assert_range(config->parity, UART_PARITY_NONE, UART_PARITY_EVEN);
    assert_range(config->follow_contrl, UART_FC_NONE, UART_FC_CTS_RTS);

    struct baudtoregs_t *bauddata = uart_get_bauddata(config->baud_rate);
    if (!bauddata)
        panic("failed to get bauddata for rate: %d\n", config->baud_rate);

    /* 关闭uart */
    uart_set_bit(id, UFCR, UFCR_UME, 0);

    /* 设置波特率 div */
    uart_set_bit(id, ULCR, ULCR_DLAB, 1);
    uart_write_reg(id, UDLHR, bauddata->div >> 8);
    uart_write_reg(id, UDLLR, bauddata->div & 0xff);
    uart_set_bit(id, ULCR, ULCR_DLAB, 0);

    /* 设置波特率 M, AC */
    uart_write_reg(id, UMR, bauddata->umr);
    uart_write_reg(id, UACR, bauddata->uacr);

    /* 设置 parity, data bits, stop bits */
    unsigned long ulcr = 0;
    set_bit_field(&ulcr, ULCR_PARE, config->parity != UART_PARITY_NONE);
    set_bit_field(&ulcr, ULCR_PARM, config->parity == UART_PARITY_EVEN);
    set_bit_field(&ulcr, ULCR_SBLS, config->stop_bits == 2);
    set_bit_field(&ulcr, ULCR_WLS, config->data_bits - 5);

    uart_write_reg(id, ULCR, ulcr);

    /* 设置 loop mode, Modem Control/follow control */
    unsigned long umcr = 0;
    set_bit_field(&umcr, UMCR_FCM,  config->follow_contrl == UART_FC_CTS_RTS);
    set_bit_field(&umcr, UMCR_MDCE, config->follow_contrl == UART_FC_CTS_RTS);
    set_bit_field(&umcr, UMCR_LOOP, config->loop_mode);

    uart_write_reg(id, UMCR, umcr);
}

static void uart_set_rts(struct uart_config *config, int enable_rts)
{
    int id = config->uart_id;

    unsigned long umcr = 0;
    set_bit_field(&umcr, UMCR_MDCE, config->follow_contrl == UART_FC_CTS_RTS);
    set_bit_field(&umcr, UMCR_FCM,  config->follow_contrl == UART_FC_CTS_RTS);
    set_bit_field(&umcr, UMCR_LOOP, config->loop_mode);

    if (enable_rts && config->follow_contrl == UART_FC_CTS_RTS) {
        set_bit_field(&umcr, UMCR_FCM, 0);
        set_bit_field(&umcr, UMCR_RTS, 1);
    } else {
        set_bit_field(&umcr, UMCR_RTS, 0);
    }

    uart_write_reg(id, UMCR, umcr);
}

static int uart_get_tx_fifo_empty(struct uart_config *config)
{
    int id = config->uart_id;
    return uart_get_bit(id, ULSR, ULSR_TEMP);
}

static int uart_get_cts(struct uart_config *config)
{
    int id = config->uart_id;

    return uart_get_bit(id, UMSR, UMSR_CTS);
}

static void uart_disable(int id)
{
    /* 关闭uart */
    uart_set_bit(id, UFCR, UFCR_UME, 0);
}

static void uart_enable_fifo_mode(struct uart_config *config)
{
    int id = config->uart_id;

    /* 设置中断
     * 程序调用接收时动态开启 接收 timeout, rdy, 接收错误 中断
     */
    unsigned long uier = 0;
    set_bit_field(&uier, UIER_RTOIE, 0);
    set_bit_field(&uier, UIER_RDRIE, 0);
    set_bit_field(&uier, UIER_RLSIE, 0);

    /* 发送时动态开启 uart 发送中断 */
    set_bit_field(&uier, UIER_TDRIE, 0);

    uart_write_reg(id, UIER, uier);

    unsigned long urtdcr = 0;
    set_bit_field(&urtdcr, RTD_EN, config->rx_dma_mode);
    set_bit_field(&urtdcr, RTD_R, config->rx_dma_mode);
    set_bit_field(&urtdcr, RTD_T, 0);

    /* 使能uart, 使能fifo 模式, 清接收/发送fifo, 设置接收fifo 32字节触发 */
    unsigned long ufcr = 0;
    set_bit_field(&ufcr, UFCR_UME, 1);
    set_bit_field(&ufcr, UFCR_FME, 1);
    set_bit_field(&ufcr, UFCR_TFRT, 1);
    set_bit_field(&ufcr, UFCR_RFRT, 1);
    set_bit_field(&ufcr, UFCR_DME, config->rx_dma_mode);
    set_bit_field(&ufcr, UFCR_RDTR, 0);
    uart_write_reg(id, UFCR, ufcr);
}

static void uart_disable_rx_irq(int id)
{
    unsigned long uier = uart_read_reg(id, UIER);
    set_bit_field(&uier, UIER_RTOIE, 0);
    set_bit_field(&uier, UIER_RDRIE, 0);
    uart_write_reg(id, UIER, uier);
}

static void uart_enable_rx_irq(int id)
{
    unsigned long uier = uart_read_reg(id, UIER);
    set_bit_field(&uier, UIER_RTOIE, 1);
    set_bit_field(&uier, UIER_RDRIE, 1);
    uart_write_reg(id, UIER, uier);
}

static void uart_disable_tx_irq(int id)
{
    uart_set_bit(id, UIER, UIER_TDRIE, 0);
}

static void uart_enable_tx_irq(int id)
{
    uart_set_bit(id, UIER, UIER_TDRIE, 1);
}

static int uart_write_fifo(int id, char *buf, unsigned int len)
{
    int i;
    int n = UART_FIFO_LEN - uart_read_reg(id, UTCR);

    if (n < len)
        len = n;

    for (i = 0; i < len; i++)
        uart_write_reg(id, UTHR, (unsigned char)buf[i]);

    return len;
}

static int uart_read_fifo(int id, char *buf, unsigned int len)
{
    int i;
    int n = uart_read_reg(id, URCR);

    if (n < len)
        len = n;

    for (i = 0; i < len; i++)
        ((unsigned char *)buf)[i] = uart_read_reg(id, URBR);

    return len;
}

struct uart_data {
    u8 is_inited;
    u8 is_irq_inited;
    u8 is_gpio_inited;
    volatile u8 tx_busy;
    volatile u8 rx_busy;
    volatile u8 is_stop;
    char *rx_buf;
    unsigned int rx_len;
    char *tx_buf;
    unsigned int tx_len;
    struct clk *clk;
    struct completion tx_done;
    struct completion rx_done;
    struct dma_chan *dma_chan;
    struct dma_async_tx_descriptor *dma_desc;
    unsigned char *dma_addr;
    dma_addr_t dma_addr_phys;
    int dma_bufsize;
    int last_dma_off;
};

static struct uart_data uartdata[UART_NUMS];

static const char *uartstr[] = {
    "uart0",
    "uart1",
    "uart2",
    "uart3",
    "uart4",
    "uart5",
    "uart6",
    "uart7",
    "uart8",
    "uart9",
};

static const char *uartclkstr[] = {
    "gate_uart0",
    "gate_uart1",
    "gate_uart2",
    "gate_uart3",
    "gate_uart4",
    "gate_uart5",
    "gate_uart6",
    "gate_uart7",
    "gate_uart8",
    "gate_uart9",
};

static const int uartirq[] = {
    IRQ_INTC_BASE + IRQ_UART0,
    IRQ_INTC_BASE + IRQ_UART1,
    IRQ_INTC_BASE + IRQ_UART2,
    IRQ_INTC_BASE + IRQ_UART3,
    IRQ_INTC_BASE + IRQ_UART4,
    IRQ_INTC_BASE + IRQ_UART5,
    IRQ_INTC_BASE + IRQ_UART6,
    IRQ_INTC_BASE + IRQ_UART7,
    IRQ_INTC_BASE + IRQ_UART8,
    IRQ_INTC_BASE + IRQ_UART9,
};

enum uart_int_type {
    INT_moden_status,
    INT_tx_request,
    INT_rx_ready,
    INT_rx_line_status,
    INT_reserve_0,
    INT_reserve_1,
    INT_rx_timeout,
    INT_reserve_2,
};

enum gpio_type {
    type_tx,
    type_rx,
    type_cts,
    type_rts,
};

struct gpio_func {
    short id;
    short type;
    short gpio;
    short func;
};

struct uart_gpio {
    int is_enable;
    int rx;
    int tx;
    int cts;
    int rts;
    const char rx_name[10];
    const char tx_name[10];
    const char cts_name[10];
    const char rts_name[10];
};

static struct gpio_func gpio_funcs[] = {
    {0, type_rx, GPIO_PD(23), GPIO_FUNC_2},
    {0, type_tx, GPIO_PD(24), GPIO_FUNC_2},
    {0, type_cts, GPIO_PD(25), GPIO_FUNC_2},
    {0, type_rts, GPIO_PD(26), GPIO_FUNC_2},

    {1, type_rx, GPIO_PC(23), GPIO_FUNC_1},
    {1, type_tx, GPIO_PC(24), GPIO_FUNC_1},
    {1, type_cts, GPIO_PC(21), GPIO_FUNC_1},
    {1, type_rts, GPIO_PC(22), GPIO_FUNC_1},

    {2, type_rx, GPIO_PD(30), GPIO_FUNC_0},
    {2, type_tx, GPIO_PD(31), GPIO_FUNC_0},

    {3, type_rx, GPIO_PC(25), GPIO_FUNC_0},
    {3, type_tx, GPIO_PC(26), GPIO_FUNC_0},
    {3, type_cts, GPIO_PC(27), GPIO_FUNC_0},
    {3, type_rts, GPIO_PC(28), GPIO_FUNC_0},
    {3, type_rx, GPIO_PD(02), GPIO_FUNC_1},
    {3, type_tx, GPIO_PD(03), GPIO_FUNC_1},
    {3, type_cts, GPIO_PD(00), GPIO_FUNC_1},
    {3, type_rts, GPIO_PD(01), GPIO_FUNC_1},

    {4, type_rx, GPIO_PA(2), GPIO_FUNC_1},
    {4, type_tx, GPIO_PA(3), GPIO_FUNC_1},
    {4, type_cts, GPIO_PA(0), GPIO_FUNC_1},
    {4, type_rts, GPIO_PA(1), GPIO_FUNC_1},
    {4, type_rx, GPIO_PC(11), GPIO_FUNC_3},
    {4, type_tx, GPIO_PC(12), GPIO_FUNC_3},
    {4, type_cts, GPIO_PC(9), GPIO_FUNC_3},
    {4, type_rts, GPIO_PC(10), GPIO_FUNC_3},


    {5, type_rx, GPIO_PA(4), GPIO_FUNC_1},
    {5, type_tx, GPIO_PA(5), GPIO_FUNC_1},
    {5, type_rx, GPIO_PC(5), GPIO_FUNC_3},
    {5, type_tx, GPIO_PC(6), GPIO_FUNC_3},

    {6, type_rx, GPIO_PA(6), GPIO_FUNC_1},
    {6, type_tx, GPIO_PA(7), GPIO_FUNC_1},
    {6, type_rx, GPIO_PC(7), GPIO_FUNC_3},
    {6, type_tx, GPIO_PC(8), GPIO_FUNC_3},

    {7, type_rx, GPIO_PA(8), GPIO_FUNC_1},
    {7, type_tx, GPIO_PA(9), GPIO_FUNC_1},
    {7, type_rx, GPIO_PC(1), GPIO_FUNC_3},
    {7, type_tx, GPIO_PC(2), GPIO_FUNC_3},

    {8, type_rx, GPIO_PB(28), GPIO_FUNC_3},
    {8, type_tx, GPIO_PB(29), GPIO_FUNC_3},

    {9, type_rx, GPIO_PB(30), GPIO_FUNC_3},
    {9, type_tx, GPIO_PB(31), GPIO_FUNC_3},
};

#define UART_GPIO_DEF(id) \
    [id] = { \
        .rx_name = "uart"#id"_rx", \
        .tx_name = "uart"#id"_tx", \
        .cts_name = "uart"#id"_cts", \
        .rts_name = "uart"#id"_rts", \
    }

struct uart_gpio uartgpio[UART_NUMS] = {
    UART_GPIO_DEF(0),
    UART_GPIO_DEF(1),
    UART_GPIO_DEF(2),
    UART_GPIO_DEF(3),
    UART_GPIO_DEF(4),
    UART_GPIO_DEF(5),
    UART_GPIO_DEF(6),
    UART_GPIO_DEF(7),
    UART_GPIO_DEF(8),
    UART_GPIO_DEF(9),
};

module_param_named(uart0_enable, uartgpio[0].is_enable, int, 0644);
module_param_gpio_named(uart0_rx, uartgpio[0].rx, 0644);
module_param_gpio_named(uart0_tx, uartgpio[0].tx, 0644);
module_param_gpio_named(uart0_cts, uartgpio[0].cts, 0644);
module_param_gpio_named(uart0_rts, uartgpio[0].rts, 0644);

module_param_named(uart1_enable, uartgpio[1].is_enable, int, 0644);
module_param_gpio_named(uart1_rx, uartgpio[1].rx, 0644);
module_param_gpio_named(uart1_tx, uartgpio[1].tx, 0644);
module_param_gpio_named(uart1_cts, uartgpio[1].cts, 0644);
module_param_gpio_named(uart1_rts, uartgpio[1].rts, 0644);

module_param_named(uart2_enable, uartgpio[2].is_enable, int, 0644);
module_param_gpio_named(uart2_rx, uartgpio[2].rx, 0644);
module_param_gpio_named(uart2_tx, uartgpio[2].tx, 0644);
module_param_gpio_named(uart2_cts, uartgpio[2].cts, 0644);
module_param_gpio_named(uart2_rts, uartgpio[2].rts, 0644);

module_param_named(uart3_enable, uartgpio[3].is_enable, int, 0644);
module_param_gpio_named(uart3_rx, uartgpio[3].rx, 0644);
module_param_gpio_named(uart3_tx, uartgpio[3].tx, 0644);
module_param_gpio_named(uart3_cts, uartgpio[3].cts, 0644);
module_param_gpio_named(uart3_rts, uartgpio[3].rts, 0644);

module_param_named(uart4_enable, uartgpio[4].is_enable, int, 0644);
module_param_gpio_named(uart4_rx, uartgpio[4].rx, 0644);
module_param_gpio_named(uart4_tx, uartgpio[4].tx, 0644);
module_param_gpio_named(uart4_cts, uartgpio[4].cts, 0644);
module_param_gpio_named(uart4_rts, uartgpio[4].rts, 0644);

module_param_named(uart5_enable, uartgpio[5].is_enable, int, 0644);
module_param_gpio_named(uart5_rx, uartgpio[5].rx, 0644);
module_param_gpio_named(uart5_tx, uartgpio[5].tx, 0644);
module_param_gpio_named(uart5_cts, uartgpio[5].cts, 0644);
module_param_gpio_named(uart5_rts, uartgpio[5].rts, 0644);

module_param_named(uart6_enable, uartgpio[6].is_enable, int, 0644);
module_param_gpio_named(uart6_rx, uartgpio[6].rx, 0644);
module_param_gpio_named(uart6_tx, uartgpio[6].tx, 0644);
module_param_gpio_named(uart6_cts, uartgpio[6].cts, 0644);
module_param_gpio_named(uart6_rts, uartgpio[6].rts, 0644);

module_param_named(uart7_enable, uartgpio[7].is_enable, int, 0644);
module_param_gpio_named(uart7_rx, uartgpio[7].rx, 0644);
module_param_gpio_named(uart7_tx, uartgpio[7].tx, 0644);
module_param_gpio_named(uart7_cts, uartgpio[7].cts, 0644);
module_param_gpio_named(uart7_rts, uartgpio[7].rts, 0644);

module_param_named(uart8_enable, uartgpio[8].is_enable, int, 0644);
module_param_gpio_named(uart8_rx, uartgpio[8].rx, 0644);
module_param_gpio_named(uart8_tx, uartgpio[8].tx, 0644);
module_param_gpio_named(uart8_cts, uartgpio[8].cts, 0644);
module_param_gpio_named(uart8_rts, uartgpio[8].rts, 0644);

module_param_named(uart9_enable, uartgpio[9].is_enable, int, 0644);
module_param_gpio_named(uart9_rx, uartgpio[9].rx, 0644);
module_param_gpio_named(uart9_tx, uartgpio[9].tx, 0644);
module_param_gpio_named(uart9_cts, uartgpio[9].cts, 0644);
module_param_gpio_named(uart9_rts, uartgpio[9].rts, 0644);

static inline void m_gpio_request(int id, int type, int gpio, const char *name)
{
    if (gpio == -1)
        return;

    if (gpio == 0)
        panic("%s gpio is not defined!\n", name);

    int i;
    for (i = 0; i < ARRAY_SIZE(gpio_funcs); i++) {
        struct gpio_func *f = &gpio_funcs[i];
        if (f->id == id && f->type == type && f->gpio == gpio) {
            int ret = gpio_request(gpio, name);
            if (ret) {
                char buf[12];
                printk(KERN_ERR "uart: failed to request %s for %s\n", 
                    gpio_to_str(gpio, buf), name);
            }

            gpio_set_func(gpio, f->func);
        }
    }

}

static void uart_gpio_request(int id)
{
    struct uart_gpio *uio = &uartgpio[id];

    if (!uio->is_enable)
        panic("uart%d is not enabled!\n", id);

    m_gpio_request(id, type_rx, uio->rx, uio->rx_name);
    m_gpio_request(id, type_tx, uio->tx, uio->tx_name);
    m_gpio_request(id, type_cts, uio->cts, uio->cts_name);
    m_gpio_request(id, type_rts, uio->rts, uio->rts_name);
}

volatile unsigned char ch_;

static irqreturn_t uart_irq_handler(int irq, void *data)
{
    struct uart_data *uart = data;
    int id = uart - uartdata;
    int int_type;
    int ret;

    unsigned int uiir = uart_read_reg(id, UIIR);

    //没有待处理中断
    if (reg_get_bit(uiir, UIIR_INPEND))
        return IRQ_HANDLED;

    int_type = reg_get_bit(uiir, UIIR_INID);

    switch (int_type)
    {
    case INT_tx_request:
        if (!uart->tx_busy)
            panic("why tx not busy: %d\n", id);

        if (uart->tx_len == 0) {
            uart_disable_tx_irq(id);
            complete(&uart->tx_done);
            printk(KERN_ERR "!\n");
        } else {
            ret = uart_write_fifo(id, uart->tx_buf, uart->tx_len);
            uart->tx_buf += ret;
            uart->tx_len -= ret;
        }

        break;

    case INT_rx_line_status:
        // printf("ulsr error: %x\n", uart_read_reg(id, ULSR));

        while (uart_get_bit(id, ULSR, ULSR_FIFOE)) {
            // 读接收 fifo，直到没有 fifo error
            ch_ = uart_read_reg(id, URBR);
        }

        while (uart_get_bit(id, ULSR, ULSR_DRY)) {
            // 这种状态是 fifo 出错了，不能体现fifo len，
            // 但 fifo 里面还有数据
            // 读接收 fifo，直到读空
            ch_ = uart_read_reg(id, URBR);
        }

        break;

    case INT_rx_ready:
    case INT_rx_timeout:
        if (!uart->rx_busy)
            panic("why rx not busy: %d\n", id);

        if (uart->rx_len == 0)
            panic("why rx len is 0: %d\n", id);

        ret = uart_read_fifo(id, uart->rx_buf, uart->rx_len);
        uart->rx_buf += ret;
        uart->rx_len -= ret;
        if (uart->rx_len == 0) {
            uart_disable_rx_irq(id);
			complete(&uart->rx_done);
            printk(KERN_ERR "@\n");
        }

        break;

    case INT_moden_status:
        panic("why moden staus: %d\n", id);

    default:
        break;
    }

    return IRQ_HANDLED;
}

static inline void check_irq_requested(int id)
{
    struct uart_data *uart = &uartdata[id];

    if (!uart->is_irq_inited) {
        int ret = request_irq(uartirq[id], uart_irq_handler, IRQF_NO_THREAD, 
                    uartstr[id], &uartdata[id]);
        if (ret)
            panic("uart: failed to request irq: %d %d\n", id, uartirq[id]);
        uart->is_irq_inited = 1;
    }
}

static void uart_check_line_status(int id)
{
    int int_type = uart_get_bit(id, UIIR, UIIR_INID);
    if (int_type == INT_rx_line_status) {
        while (uart_get_bit(id, ULSR, ULSR_FIFOE)) {
            // 读接收 fifo，直到没有 fifo error
            ch_ = uart_read_reg(id, URBR);
        }

        while (uart_get_bit(id, ULSR, ULSR_DRY)) {
            // 这种状态是 fifo 出错了，不能体现fifo len，
            // 但 fifo 里面还有数据
            // 读接收 fifo，直到读空
            ch_ = uart_read_reg(id, URBR);
        }
    }
}

void uart_send_poll_nowait(struct uart_config *config, char *buf, unsigned int len)
{
    int ret;
    int id = config->uart_id;
    struct uart_data *uart = &uartdata[id];

    assert(!uart->tx_busy);
    uart->tx_busy = 1;

    while (len && !uart->is_stop) {
        // uart_check_line_status(id);
        ret = uart_write_fifo(id, buf, len);
        buf += ret;
        len -= ret;
    }

    uart->tx_busy = 0;
}

static void uart_send_poll(struct uart_config *config, char *buf, unsigned int len)
{
    int ret;
    int id = config->uart_id;
    struct uart_data *uart = &uartdata[id];

    assert(!uart->tx_busy);
    uart->tx_busy = 1;

    while (len && !uart->is_stop) {
        // uart_check_line_status(id);
        ret = uart_write_fifo(id, buf, len);
        buf += ret;
        len -= ret;
    }

    while (!uart_get_bit(id, ULSR, ULSR_TEMP));
    uart->tx_busy = 0;
}

static void uart_receive_poll(struct uart_config *config, char *buf, unsigned int len)
{
    int ret;
    int id = config->uart_id;
    struct uart_data *uart = &uartdata[id];

    assert(!uart->rx_busy);
    uart->rx_busy = 1;

    while (len && !uart->is_stop) {
        uart_check_line_status(id);
        ret = uart_read_fifo(id, buf, len);
        buf += ret;
        len -= ret;
    }
    uart->rx_busy = 0;
}

static void uart_send_irq(struct uart_config *config, char *buf, unsigned int len)
{
    int ret;
    int id = config->uart_id;
    struct uart_data *uart = &uartdata[id];

    check_irq_requested(id);

    assert(uart->is_inited);

    uart->tx_busy = 1;

    /* 写入一部分数据到fifo
     */
    ret = uart_write_fifo(id, buf, len);
    uart->tx_buf = buf + ret;
    uart->tx_len = len - ret;

    /* 开启tx fifo 为空的中断
     */
    uart_enable_tx_irq(id);

    ret = wait_for_completion_timeout(&uart->tx_done, msecs_to_jiffies(10*1000));
    if (ret <= 0)
        printk(KERN_ERR "uart: %d send timeout\n", id);

    uart->tx_busy = 0;
}

static void uart_receive_irq(struct uart_config *config, char *buf, unsigned int len)
{
    int ret;
    int id = config->uart_id;
    struct uart_data *uart = &uartdata[id];

    check_irq_requested(id);

    assert(uart->is_inited);

    uart->rx_busy = 1;

    ret = uart_read_fifo(id, buf, len);
    uart->rx_buf = buf + ret;
    uart->rx_len = len - ret;

    if (uart->rx_len) {
        /* 开启 rx rdy/timeout 的中断
         */
        uart_enable_rx_irq(id);
        ret = wait_for_completion_interruptible(&uart->rx_done);
        if (ret < 0)
            printk(KERN_ERR "uart: %d receive interrupted\n", id);
    }

    uart->rx_busy = 0;
}

void soc_uart_send(struct uart_config *config, const char *buf, unsigned int len)
{
    if (config->tx_poll_mode)
        uart_send_poll(config, (char *)buf, len);
    else
        uart_send_irq(config, (char *)buf, len);
}

static dma_addr_t get_dma_addr(struct uart_data *uart)
{
    dma_addr_t addr;
    int count = 1;

    do {
        addr = uart->dma_chan->device->get_current_trans_addr(
            uart->dma_chan, NULL, NULL, DMA_DEV_TO_MEM);
        if ( addr >= uart->dma_addr_phys && addr < uart->dma_addr_phys + uart->dma_bufsize)
            return addr;
    } while (count--);


    panic("uart: failed to get dma addr %d %x\n", uart-uartdata, addr);
}

int soc_uart_read_fifosize(struct uart_config *config, int *total_size)
{
    int id = config->uart_id;
    int readable_size;

    if (config->rx_dma_mode) {
        if (total_size != NULL)
            *total_size = config->rx_dma_bufsize;

        struct uart_data *uart = &uartdata[id];
        int old = uart->last_dma_off;
        unsigned long start = uart->dma_addr_phys;
        int buf_size = uart->dma_bufsize;

        int off = get_dma_addr(uart) - start;
        readable_size = (off + buf_size - old) % buf_size;
    } else {
        if (total_size != NULL)
            *total_size = 64;

        readable_size = uart_read_reg(id, URCR);
    }

    return readable_size;
}

static void uart_receive_dma(struct uart_config *config, char *buf, unsigned int len)
{
    int id = config->uart_id;
    struct uart_data *uart = &uartdata[id];
    int old = uart->last_dma_off;
    unsigned long start = uart->dma_addr_phys;
    int buf_size = uart->dma_bufsize;

    while (len && !uart->is_stop) {
        int off;
        while (!uart->is_stop) {
            off = get_dma_addr(uart) - start;
            if (off != old)
                break;
            usleep_range(1000, 1000);
        }

        int n = (off + buf_size - old) % buf_size;
        if (n > len)
            n = len;

        if (off > old) {
            memcpy(buf, uart->dma_addr + old, n);
        } else {
            int n1 = buf_size - old;
            if (n < n1)
                n1 = n;
            memcpy(buf, uart->dma_addr + old, n1);
            memcpy(buf+n1, uart->dma_addr, n - n1);
        }

        len -= n;
        buf += n;
        uart->last_dma_off = (old + n) % buf_size;
    }
}

void soc_uart_receive(struct uart_config *config, char *buf, unsigned int len)
{
    if (config->rx_dma_mode)
        return uart_receive_dma(config, buf, len);
    if (config->rx_poll_mode)
        uart_receive_poll(config, buf, len);
    else
        uart_receive_irq(config, buf, len);
}

static void init_rx_dma(struct uart_config *config)
{
    int id = config->uart_id;
    struct uart_data *uart = &uartdata[id];

    dma_cap_mask_t mask;

    dma_cap_zero(mask);
    dma_cap_set(DMA_SLAVE, mask);
    dma_cap_set(DMA_CYCLIC, mask);

    uart->dma_chan = dma_request_channel(mask, NULL, NULL);
    if (!uart->dma_chan)
        panic("uart: failed to request dma chan %d\n", id);

    struct dma_slave_config slave = {0};
    slave.direction = DMA_DEV_TO_MEM;
    slave.src_addr = iobase[id] - KSEG1ADDR(0);
    slave.src_addr_width = 1;
    slave.dst_addr_width = 1;
    slave.src_maxburst = 1;
    slave.dst_maxburst = 1;
    slave.slave_id = rxdma_slaveid[id];
    int ret = dmaengine_slave_config(uart->dma_chan, &slave);
    if (ret)
        panic(KERN_ERR "uart: failed to config dma chan %d\n", id);

    uart->dma_bufsize = config->rx_dma_bufsize;
    uart->dma_addr = dma_alloc_coherent(
        NULL, uart->dma_bufsize, &uart->dma_addr_phys, GFP_KERNEL);
    if (!uart->dma_addr)
        panic(KERN_ERR "uart: failed to malloc dma buf %d %d\n", id, uart->dma_bufsize);
    uart->last_dma_off = 0;

    uart->dma_desc = dmaengine_prep_dma_cyclic(
        uart->dma_chan, uart->dma_addr_phys, uart->dma_bufsize,
            uart->dma_bufsize, DMA_DEV_TO_MEM, DMA_CTRL_ACK);
    if (!uart->dma_desc)
        panic(KERN_ERR "uart: failed to prep dma cyclic %d %d\n", id, uart->dma_bufsize);

    uart->dma_desc->callback = NULL;
    uart->dma_desc->callback_param = NULL;

    dmaengine_submit(uart->dma_desc);
    dma_async_issue_pending(uart->dma_chan);
}

void soc_uart_start(struct uart_config *config)
{
    int id = config->uart_id;
    struct uart_data *uart = &uartdata[id];

    assert(id < UART_NUMS);
    assert(!uart->is_inited);

    uart->clk = clk_get(NULL, uartclkstr[id]);
    assert(!IS_ERR(uart->clk));

    clk_prepare_enable(uart->clk);

    init_completion(&uart->tx_done);
    init_completion(&uart->rx_done);

    uart_init_config(config);

    uart_enable_fifo_mode(config);

    if (config->rx_dma_mode)
        init_rx_dma(config);

    if (!uart->is_gpio_inited)
        uart_gpio_request(id);

    uart->is_gpio_inited = 1;
    uart->is_inited = 1;
    uart->is_stop = 0;
}

void soc_uart_stop(struct uart_config *config)
{
    int id = config->uart_id;
    struct uart_data *uart = &uartdata[id];

    assert(id < UART_NUMS);
    assert(uart->is_inited);

    uart->is_stop = 1;
    if (uart->rx_busy)
        complete(&uart->rx_done);
    if (uart->tx_busy)
        complete(&uart->tx_done);

    while (uart->rx_busy || uart->tx_busy)
        usleep_range(500, 500);

    if (uart->dma_chan) {
        dmaengine_terminate_all(uart->dma_chan);
        dma_release_channel(uart->dma_chan);
        dma_free_coherent(NULL, uart->dma_bufsize, uart->dma_addr, uart->dma_addr_phys);
        uart->dma_chan = NULL;
    }

    if (uart->is_irq_inited) {
        disable_irq(uartirq[id]);
        free_irq(uartirq[id], &uartdata[id]);
        uart->is_irq_inited = 0;
    }

    uart_disable(id);

    clk_disable_unprepare(uart->clk);

    clk_put(uart->clk);

    uart->is_inited = 0;
}

// int uart_init(void)
// {
//     struct uart_config config = {
//         .uart_id = 3,
//         .data_bits = 8,
//         .stop_bits = 1,
//         .loop_mode = 0,
//         .tx_poll_mode = 0,
//         .rx_poll_mode = 0,
//         .rx_dma_mode = 1,
//         .rx_dma_bufsize = 1024,
//         .parity = UART_PARITY_NONE,
//         .follow_contrl = UART_FC_NONE,
//         .baud_rate = 115200,
//     };

//     soc_uart_start(&config);

//     const char *str = "hello world\r\n";
//     soc_uart_send(&config, str, strlen(str));

//     while (1) {
//         unsigned char c;
//         soc_uart_receive(&config, &c, 1);

//         soc_uart_send(&config, &c, 1);
//         if (c == '\r')
//             soc_uart_send(&config, "\n", 1);
//         if (c == 'q')
//             break;
//     }

//     return 0;
// }

// module_init(uart_init);
// MODULE_LICENSE("GPL");