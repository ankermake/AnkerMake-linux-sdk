#include "sslv_hal.h"
#include <bit_field.h>
#include <linux/slab.h>

#define CTRLR    0x0000
#define SSIENR   0x0008
#define MWCR     0x000C
#define TXFTLR   0x0018
#define RXFTLR   0x001C
#define TXFLR    0x0020
#define RXFLR    0x0024
#define SR       0x0028
#define IMR      0x002C
#define ISR      0x0030
#define RISR     0x0034
#define TXOICR   0x0038
#define RXOICR   0x003C
#define RXUICR   0x0040
#define ICR      0x0048
#define DMACR    0x004C
#define DMATDLR  0x0050
#define DMARDLR  0x0054
#define IDR      0x0058
#define DR       0x0060

#define CTRLR_Reserved  23, 31
#define CTRLR_SPI_FRF   21, 22
#define CTRLR_DFS_32    16, 20
#define CTRLR_CFS       12, 15
#define CTRLR_SRL       11, 11
#define CTRLR_SLV_OE    10, 10
#define CTRLR_TMOD      8, 9
#define CTRLR_SCPOL     7, 7
#define CTRLR_SCPH      6, 6
#define CTRLR_FRF       4, 5
#define CTRLR_Reserved1 0, 3

#define SSIENR_Reserved   1, 31
#define SSIENR_SSI_SLV_EN 0, 0

#define MWCR_Reserved 2, 31
#define MWCR_MDD      1, 1
#define MWCR_MWMOD    0, 0

#define TXFTLR_Reserved 6, 31
#define TXFTLR_TFT      0, 5

#define RXFTLR_Reserved 6, 31
#define RXFTLR_RFT      0, 5

#define TXFLR_Reserved  7, 31
#define TXFLR_TXTFL     0, 6

#define RXFLR_Reserved  7, 31
#define RXFLR_RXTFL     0, 6

#define SR_Reserved 6, 31
#define SR_TXE      5, 5
#define SR_RFF      4, 4
#define SR_RFNE     3, 3
#define SR_TFE      2, 2
#define SR_TFNF     1, 1
#define SR_BUSY     0, 0

#define IMR_Reserved 5, 31
#define IMR_RXFIM    4, 4
#define IMR_RXOIM    3, 3
#define IMR_RXUIM    2, 2
#define IMR_TXOIM    1, 1
#define IMR_TXEIM    0, 0

#define ISR_Reserved 5, 31
#define ISR_RXFIS    4, 4
#define ISR_RXOIS    3, 3
#define ISR_RXUIS    2, 2
#define ISR_TXOIS    1, 1
#define ISR_TXEIS    0, 0

#define RISR_Reserved 5, 31
#define RISR_RXFIR    4, 4
#define RISR_RXOIR    3, 3
#define RISR_RXUIR    2, 2
#define RISR_TXOIR    1, 1
#define RISR_TXEIR    0, 0

#define TXOICR_Reserved 1, 31
#define TXOICR_TXOICR   0, 0

#define RXOICR_Reserved 1, 31
#define RXOICR_RXOICR   0, 0

#define RXUICR_Reserved 1, 31
#define RXUICR_RXUICR   0, 0

#define ICR_Reserved 1, 31
#define ICR_ICR      0, 0

#define DMACR_Reserved 2, 31
#define DMACR_TDMAE    1, 1
#define DMACR_RDMAE    0, 0

#define DMATDLR_Reserved 6, 31
#define DMATDLR_DMATDL   0, 5

#define DMARDLR_Reserved 6, 31
#define DMARDLR_DMARDL   0, 5

#define SSI_SLV_IOBASE 0x10045000

static const unsigned long iobase[] = {
    SSI_SLV_IOBASE,
};

#define SSLV_ADDR(id, reg) ((volatile unsigned long *)(KSEG1ADDR(iobase[id]) + reg))

static inline void sslv_write_reg(int id, unsigned int reg, unsigned int value)
{
    *SSLV_ADDR(id, reg) = value;
}

static inline unsigned int sslv_read_reg(int id, unsigned int reg)
{
    return *SSLV_ADDR(id, reg);
}

static inline void sslv_set_bits(int id, unsigned int reg, int start, int end, unsigned int val)
{
    set_bit_field(SSLV_ADDR(id, reg), start, end, val);
}

static inline unsigned int sslv_get_bits(int id, unsigned int reg, int start, int end)
{
    return get_bit_field(SSLV_ADDR(id, reg), start, end);
}

/////////////////////////////////////////////////////////////////////////////////////

unsigned long sslv_hal_get_dma_slvdr_addr(int id)
{
    return iobase[id] + DR;
}

/* CTRLR : 设置前需要保证sslv disable(失能)(SSIENR_SSI_SLV_EN == 0) */
/* 设置帧格式- Motorola SPI */
inline void sslv_hal_set_standard_transfer_format(int id)
{
    sslv_set_bits(id, CTRLR, CTRLR_FRF, 0);
}

/* 设置帧格式- Texas Instruments SSP */
inline void sslv_hal_set_ssp_transfer_format(int id)
{
    sslv_set_bits(id, CTRLR, CTRLR_FRF, 1);
}

/* 设置帧格式- National Semiconductors Microwire */
inline void sslv_hal_set_national_transfer_format(int id)
{
    sslv_set_bits(id, CTRLR, CTRLR_FRF, 2);
}

/* 设置时钟相位 */
inline void sslv_hal_set_clk_scph(int id, unsigned int scph)
{
    sslv_set_bits(id, CTRLR, CTRLR_SCPH, scph);
}

/* 设置时钟极性 */
inline void sslv_hal_set_clk_scpol(int id, unsigned int scpol)
{
    sslv_set_bits(id, CTRLR, CTRLR_SCPOL, scpol);
}

/* 设置传输模式 transfer_mode(mode0 收发同时) */
inline void sslv_hal_set_transfer_mode(int id, unsigned int mode)
{
    sslv_hal_set_slv_transmit_enable(id, mode);

    sslv_set_bits(id, CTRLR, CTRLR_TMOD, mode);
}

/* 设置从模式输出使能 */
inline void sslv_hal_set_slv_transmit_enable(int id, unsigned int mode)
{
    if (mode == 0x2) // 2:receive_only
        sslv_set_bits(id, CTRLR, CTRLR_SLV_OE, 1);
    else
        sslv_set_bits(id, CTRLR, CTRLR_SLV_OE, 0);
}

/* 设置循环测试模式 */
inline void sslv_hal_set_loop_mode(int id, unsigned int mode)
{
    sslv_set_bits(id, CTRLR, CTRLR_SRL, !!mode);
}

/* microwire frame format设置控制字长度
 * size: 1 - 16 */
inline void sslv_hal_set_control_frame_size(int id, unsigned int size)
{
    sslv_set_bits(id, CTRLR, CTRLR_CFS, (size - 1) & 0xF);
}

/* 32-bit mode设置数据长度
 * size: 4 - 32 */
inline void sslv_hal_set_data_frame_size(int id, unsigned int size)
{
    sslv_set_bits(id, CTRLR, CTRLR_DFS_32, (size - 1) & 0x1F);
}

/* 设置格式需要4-5 21-22两个都设置 */
/* 设置Motorola SPI Frame Format */
inline void sslv_hal_set_standard_format(int id)
{
    sslv_hal_set_standard_transfer_format(id);

    sslv_set_bits(id, CTRLR, CTRLR_SPI_FRF, 0);
}


/* SSIENR */
/* 使能sslv */
inline void sslv_hal_enable(int id)
{
    sslv_set_bits(id, SSIENR, SSIENR_SSI_SLV_EN, 1);
}

/* 失能sslv */
inline void sslv_hal_disable(int id)
{
    sslv_set_bits(id, SSIENR, SSIENR_SSI_SLV_EN, 0);
}
inline unsigned int sslv_hal_get_isenable(int id)
{
    return sslv_get_bits(id, SSIENR, SSIENR_SSI_SLV_EN);
}


/* MWCR : 设置前需要确保sslv处于失能状态(SSIENR_SSI_SLV_EN == 0)
 *        the half-duplex Microwire serial protocol
 *        指National Semiconductors Microwire */
/* 设置sequential_transfer */
inline void sslv_hal_enable_microwire_sequential_transfer(int id)
{
    sslv_set_bits(id, MWCR, MWCR_MWMOD, 1);
}

/* 设置non_sequential_transfer */
inline void sslv_hal_disable_microwire_sequential_transfer(int id)
{
    sslv_set_bits(id, MWCR, MWCR_MWMOD, 0);
}

/* 设置串行receive */
inline void sslv_hal_set_microwire_receive(int id)
{
    sslv_set_bits(id, MWCR, MWCR_MDD, 0);
}

/* 设置串行transfer */
inline void sslv_hal_set_microwire_transfer(int id)
{
    sslv_set_bits(id, MWCR, MWCR_MDD, 1);
}


/* TXFTLR */
/* 设置发送fifo阈值
 * len: 0 - 63 */
inline void sslv_hal_set_transmit_threshold(int id, int len)
{
    if (len < 1)
        len = 1;

    sslv_set_bits(id , TXFTLR, TXFTLR_TFT, len & 0x3F);
}
inline unsigned char sslv_hal_get_transmit_threshold(int id)
{
    return sslv_read_reg(id, TXFTLR);
}


/* RXFTLR */
/* 设置接收fifo阈值
 * len: 1 - 64 */
inline void sslv_hal_set_receive_threshold(int id, int len)
{
    if (len < 1)
        len = 1;

    sslv_set_bits(id, RXFTLR, RXFTLR_RFT, (len - 1) & 0x3F);
}
inline unsigned char sslv_hal_get_receive_threshold(int id)
{
    return sslv_read_reg(id, RXFTLR);
}


/* TXFLR 只读 */
/* 获取发送fifo有效数据数量 */
inline unsigned int sslv_hal_get_transmit_fifo_number(int id)
{
    return sslv_get_bits(id, TXFLR, TXFLR_TXTFL);
}


/* RXFLR 只读 */
/* 获取接收fifo有效数据数量 */
inline unsigned int sslv_hal_get_receive_fifo_number(int id)
{
    return sslv_get_bits(id, RXFLR, RXFLR_RXTFL);
}


/* SR 只读 */
/* 获取sslv工作(busy)标志
 * return: 0 idle  1 busy */
inline unsigned int sslv_hal_get_busy_flag(int id)
{
    return sslv_get_bits(id, SR, SR_BUSY);
}

/* 获取发送fifo未满标志
 * return: 0 full  1 not */
inline unsigned int sslv_hal_get_transmit_fifo_full_flag(int id)
{
    return sslv_get_bits(id, SR, SR_TFNF);
}

/* 获取发送fifo空标志
 * return: 0 not  1 empty */
inline unsigned int sslv_hal_get_transmit_fifo_empty_flag(int id)
{
    return sslv_get_bits(id, SR, SR_TFE);
}

/* 获取接收fifo非空标志
 * return: 0 empty  1 not */
inline unsigned int sslv_hal_get_receive_fifo_empty_flag(int id)
{
    return sslv_get_bits(id, SR, SR_RFNE);
}

/* 获取接收fifo满标志
 * return: 0 not  1 full */
inline unsigned int sslv_hal_get_receive_fifo_full_flag(int id)
{
    return sslv_get_bits(id, SR, SR_RFF);
}

/* 获取发送错误标志
 * return: 0 no error  1 transmit error */
inline unsigned int sslv_hal_get_transmit_error_flag(int id)
{
    return sslv_get_bits(id, SR, SR_TXE);
}


/* IMR */
/* 使能发送fifo阈值中断 */
inline void sslv_hal_enable_transmit_threshold_interrupt(int id)
{
    sslv_set_bits(id, IMR, IMR_TXEIM, 1);
}

/* 失能发送fifo阈值中断 */
inline void sslv_hal_disable_transmit_threshold_interrupt(int id)
{
    sslv_set_bits(id, IMR, IMR_TXEIM, 0);
}

/* 是否使能发送fifo阈值中断 */
inline unsigned int sslv_hal_transmit_threshold_interrupt_is_enabled(int id)
{
    return sslv_get_bits(id, IMR, IMR_TXEIM);
}

/* 使能发送overflow溢出中断 */
inline void sslv_hal_enable_transmit_overflow_interrupt(int id)
{
    sslv_set_bits(id, IMR, IMR_TXOIM, 1);
}

/* 失能发送overflow溢出中断 */
inline void sslv_hal_disable_transmit_overflow_interrupt(int id)
{
    sslv_set_bits(id, IMR, IMR_TXOIM, 0);
}

/* 是否使能发送overflow溢出中断 */
inline unsigned int sslv_hal_transmit_overflow_interrupt_is_enabled(int id)
{
    return sslv_get_bits(id, IMR, IMR_TXOIM);
}

/* 使能接收underflow溢出中断 */
inline void sslv_hal_enable_receive_underflow_interrupt(int id)
{
    sslv_set_bits(id, IMR, IMR_RXUIM, 1);
}

/* 失能接收underflow溢出中断 */
inline void sslv_hal_disable_receive_underflow_interrupt(int id)
{
    sslv_set_bits(id, IMR, IMR_RXUIM, 0);
}

/* 是否使能接收underflow溢出中断 */
inline unsigned int sslv_hal_receive_underflow_interrupt_is_enabled(int id)
{
    return sslv_get_bits(id, IMR, IMR_RXUIM);
}

/* 使能接收overflow溢出中断 */
inline void sslv_hal_enable_receive_overflow_interrupt(int id)
{
    sslv_set_bits(id, IMR, IMR_RXOIM, 1);
}

/* 失能接收overflow溢出中断 */
inline void sslv_hal_disable_receive_overflow_interrupt(int id)
{
    sslv_set_bits(id, IMR, IMR_RXOIM, 0);
}

/* 是否使能接收overflow溢出中断 */
inline unsigned int sslv_hal_receive_overflow_interrupt_is_enabled(int id)
{
    return sslv_get_bits(id, IMR, IMR_RXOIM);
}

/* 使能接收fifo阈值中断 */
inline void sslv_hal_enable_receive_threshold_interrupt(int id)
{
    sslv_set_bits(id, IMR, IMR_RXFIM, 1);
}

/* 失能接收fifo阈值中断 */
inline void sslv_hal_disable_receive_threshold_interrupt(int id)
{
    sslv_set_bits(id, IMR, IMR_RXFIM, 0);
}

/* 是否使能接收fifo阈值中断 */
inline unsigned int sslv_hal_receive_threshold_interrupt_is_enabled(int id)
{
    return sslv_get_bits(id, IMR, IMR_RXFIM);
}

/* 使能全部中断 */
inline void sslv_hal_enable_all_interrupt(int id)
{
    sslv_write_reg(id, IMR, 0x1F);
}

/* 失能全部中断 */
inline void sslv_hal_disable_all_interrupt(int id)
{
    sslv_write_reg(id, IMR, 0);
}

/* ISR 只读 */
/* 获取发送fifo阈值中断状态 */
inline unsigned int sslv_hal_get_transmit_threshold_interrupt_status(int id)
{
    return sslv_get_bits(id, ISR, ISR_TXEIS);
}

/* 获取发送overflow溢出中断状态 */
inline unsigned int sslv_hal_get_transmit_overflow_interrupt_status(int id)
{
    return sslv_get_bits(id, ISR, ISR_TXOIS);
}

/* 获取接收underflow溢出中断状态 */
inline unsigned int sslv_hal_get_receive_underflow_interrupt_status(int id)
{
    return sslv_get_bits(id, ISR, ISR_RXUIS);
}

/* 获取接收overflow溢出中断状态 */
inline unsigned int sslv_hal_get_receive_overflow_interrupt_status(int id)
{
    return sslv_get_bits(id, ISR, ISR_RXOIS);
}

/* 获取接收fifo阈值中断状态 */
inline unsigned int sslv_hal_get_receive_threshold_interrupt_status(int id)
{
    return sslv_get_bits(id, ISR, ISR_RXFIS);
}


/* RISR 只读 中断屏蔽前 */
/* 获取发送fifo阈值raw中断状态 */
inline unsigned int sslv_hal_get_transmit_threshold_raw_interrupt_statues(int id)
{
    return sslv_get_bits(id, RISR, RISR_TXEIR);
}

/* 获取发送overflow溢出raw中断状态 */
inline unsigned int sslv_hal_get_transmit_overflow_raw_interrupt_statues(int id)
{
    return sslv_get_bits(id, RISR, RISR_TXOIR);
}

/* 获取接收underflow溢出raw中断状态 */
inline unsigned int sslv_hal_get_receive_underflow_raw_interrupt_statues(int id)
{
    return sslv_get_bits(id, RISR, RISR_RXUIR);
}

/* 获取接收overflow溢出raw中断状态 */
inline unsigned int sslv_hal_get_receive_overflow_raw_interrupt_statues(int id)
{
    return sslv_get_bits(id, RISR, RISR_RXOIR);
}

/* 获取接收fifo阈值raw中断状态 */
inline unsigned int sslv_hal_get_receive_threshold_raw_interrupt_statues(int id)
{
    return sslv_get_bits(id, RISR, RISR_RXFIR);
}


/*  */
/* 读取发送fifo阈值中断状态(中断使能且中断置位return1) */
inline unsigned int sslv_hal_get_transmit_threshold_intr_status(int id)
{
    if (sslv_hal_transmit_threshold_interrupt_is_enabled(id))
        return sslv_hal_get_transmit_threshold_raw_interrupt_statues(id);
    else
        return 0;
}

/* 读取发送overflow溢出中断状态(中断使能且中断置位return1) */
inline unsigned int sslv_hal_get_transmit_overflow_intr_status(int id)
{
    if (sslv_hal_transmit_overflow_interrupt_is_enabled(id))
        return sslv_hal_get_transmit_overflow_raw_interrupt_statues(id);
    else
        return 0;
}

/* 读取接收underflow溢出中断状态(中断使能且中断置位return1) */
inline unsigned int sslv_hal_get_receive_underflow_intr_status(int id)
{
    if (sslv_hal_receive_underflow_interrupt_is_enabled(id))
        return sslv_hal_get_receive_underflow_raw_interrupt_statues(id);
    else
        return 0;
}

/* 读取接收overflow溢出中断状态(中断使能且中断置位return1) */
inline unsigned int sslv_hal_get_receive_overflow_intr_status(int id)
{
    if (sslv_hal_receive_overflow_interrupt_is_enabled(id))
        return sslv_hal_get_receive_overflow_raw_interrupt_statues(id);
    else
        return 0;
}

/* 读取接收fifo阈值中断状态(中断使能且中断置位return1) */
inline unsigned int sslv_hal_get_receive_threshold_intr_status(int id)
{
    if (sslv_hal_receive_threshold_interrupt_is_enabled(id))
        return sslv_hal_get_receive_threshold_raw_interrupt_statues(id);
    else
        return 0;
}


/* TXOICR */
/* 清除发送overflow溢出中断 */
inline void sslv_hal_clear_transmit_overflow_interrupt(int id)
{
    sslv_get_bits(id, TXOICR, TXOICR_TXOICR);
}


/* RXOICR */
/* 清除接收overflow溢出中断 */
inline void sslv_hal_clear_receive_overflow_interrupt(int id)
{
    sslv_get_bits(id, RXOICR, RXOICR_RXOICR);
}


/* RXUICR */
/* 清除接收underflow溢出中断 */
inline void sslv_hal_clear_receive_underflow_interrupt(int id)
{
    sslv_get_bits(id, RXUICR, RXUICR_RXUICR);
}


/* ICR 只读 */
/* 清除txo,rxu,rxo中断 */
inline unsigned int sslv_hal_clear_all_interrupt(int id)
{
    return sslv_get_bits(id, ICR, ICR_ICR);
}


/* DMACR */
/* 使能接收DMA */
inline void sslv_hal_enable_receive_dma(int id)
{
    sslv_set_bits(id, DMACR, DMACR_RDMAE, 1);
}

/* 失能接收DMA */
inline void sslv_hal_disable_receive_dma(int id)
{
    sslv_set_bits(id, DMACR, DMACR_RDMAE, 0);
}

/* 使能发送DMA */
inline void sslv_hal_enable_transmit_dma(int id)
{
    sslv_set_bits(id, DMACR, DMACR_TDMAE, 1);
}

/* 失能发送DMA */
inline void sslv_hal_disable_transmit_dma(int id)
{
    sslv_set_bits(id, DMACR, DMACR_TDMAE, 0);
}


/* DMATDLR */
/* 设置dma发送阈值
 * len: 0 - 63 */
inline void sslv_hal_set_transmit_dma_threshold(int id, int len)
{
    if (len < 1)
        len = 1;

    sslv_set_bits(id, DMATDLR, DMATDLR_DMATDL, len & 0x3f);
}
inline unsigned char sslv_hal_get_transmit_dma_threshold(int id)
{
    return sslv_read_reg(id, DMATDLR);
}


/* DMARDLR */
/* 设置dma接收阈值
 * len: 1 - 64 */
inline void sslv_hal_set_receive_dma_threshold(int id, int len)
{
    if (len < 1)
        len = 1;

    sslv_set_bits(id, DMARDLR, DMARDLR_DMARDL, (len - 1) & 0x3F);
}
inline unsigned char sslv_hal_get_receive_dma_threshold(int id)
{
    return sslv_read_reg(id, DMARDLR);
}


/* IDR 只读 */
/* 读取ID */
inline unsigned int sslv_hal_get_id_code(int id)
{
    return sslv_read_reg(id, IDR);
}


/* DR */
/* 读寄存器 */
inline unsigned int sslv_hal_read_fifo(int id)
{
    return sslv_read_reg(id, DR);
}

/* 写寄存器 */
inline void sslv_hal_write_fifo(int id, unsigned int data)
{
    sslv_write_reg(id, DR, data);
}

