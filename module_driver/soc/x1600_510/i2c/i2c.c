/*
 * Copyright (C) 2020 Ingenic Semiconductor Co., Ltd.
 *
 * I2C adapter driver for the Ingenic I2C controller
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */

#include <linux/module.h>
#include <linux/errno.h>
#include <linux/platform_device.h>
#include <linux/interrupt.h>
#include <linux/clk.h>
#include <linux/i2c.h>
#include <linux/delay.h>
#include <utils/gpio.h>
#include <bit_field.h>

#include "i2c_regs.h"

#define I2C0_IOBASE                     0x10050000
#define I2C1_IOBASE                     0x10051000

#define IRQ_I2C1                        (32 + 28)
#define IRQ_I2C0                        (32 + 29)

#define MAX_FIFO_LEN                    64
#define TX_TL                           32
#define RX_TL                           (32 - 1)

#define READ_CMD                        0x100
#define STOP_CMD                        0x200
#define RESTART_CMD                     0x400

#define ABRT_7B_ADDR_NOACK              1
#define ABRT_10B_ADDR_NOACK             2

#define I2C_START                       1
#define I2C_RE_START                    2
#define I2C_STOP                        4

#define MAX_SETUP_TIME                  255
#define MAX_HOLD_TIME                   65535

enum i2c_addr_type {
    I2C_ADDR_BIT_7,
    I2C_ADDR_BIT_10,
};

struct jz_func_alter {
    int pin;
    int function;
};

struct jz_i2c_pin {
    struct jz_func_alter scl;
    struct jz_func_alter sda;
};

struct jz_i2c_drv {
    int id;
    int is_enable;
    int is_finish;
    int rate;
    const char *scl_name;
    const char *sda_name;
    struct jz_func_alter scl;
    struct jz_func_alter sda;

    int alter_num;
    struct jz_i2c_pin *alter_pin;

    int irq;
    int status;

    int abtsrc;

    int len;
    int cmd;
    int rd_num;
    char *tx_buf;
    char *rx_buf;

    struct clk *clk;
    char *clk_name;
    struct completion complete;
    const char *irq_name;
    struct i2c_adapter adap;
};

/*
 * I2C Pin Alternate Information
 */
static struct jz_i2c_pin jz_i2c0_pin[2] = {
    {
        .sda        = {GPIO_PA(29),  GPIO_FUNC_2},
        .scl        = {GPIO_PA(28),  GPIO_FUNC_2},
    },
    {
        .sda        = {GPIO_PB(31),  GPIO_FUNC_0},
        .scl        = {GPIO_PB(30),  GPIO_FUNC_0},
    },
};

static struct jz_i2c_pin jz_i2c1_pin[2] = {
    {
        .sda        = {GPIO_PB(16),  GPIO_FUNC_2},
        .scl        = {GPIO_PB(15),  GPIO_FUNC_2},
    },
    {
        .sda        = {GPIO_PB(20),  GPIO_FUNC_0},
        .scl        = {GPIO_PB(19),  GPIO_FUNC_0},
    },
};


/*
 * I2C Controller information
 */
static struct jz_i2c_drv jz_i2c_dev[6] = {
    {
        .id         = 0,
        .irq        = (IRQ_INTC_BASE + IRQ_I2C0),
        .irq_name   = "I2C0",
        .clk_name   = "gate_i2c0",
        .scl_name   = "i2c0_scl",
        .sda_name   = "i2c0_sda",
        .alter_pin  = jz_i2c0_pin,
        .alter_num  = ARRAY_SIZE(jz_i2c0_pin),
    },
    {
        .id         = 1,
        .irq        = (IRQ_INTC_BASE + IRQ_I2C1),
        .irq_name   = "I2C1",
        .clk_name   = "gate_i2c1",
        .scl_name   = "i2c1_scl",
        .sda_name   = "i2c1_sda",
        .alter_pin  = jz_i2c1_pin,
        .alter_num  = ARRAY_SIZE(jz_i2c1_pin),
    },
};

/*
 * 模块参数信息
 */
module_param_named(i2c0_is_enable,  jz_i2c_dev[0].is_enable, int, 0644);
module_param_named(i2c0_rate,       jz_i2c_dev[0].rate, int, 0644);
module_param_gpio_named(i2c0_scl,   jz_i2c_dev[0].scl.pin, 0644);
module_param_gpio_named(i2c0_sda,   jz_i2c_dev[0].sda.pin, 0644);

module_param_named(i2c1_is_enable,  jz_i2c_dev[1].is_enable, int, 0644);
module_param_named(i2c1_rate,       jz_i2c_dev[1].rate, int, 0644);
module_param_gpio_named(i2c1_scl,   jz_i2c_dev[1].scl.pin, 0644);
module_param_gpio_named(i2c1_sda,   jz_i2c_dev[1].sda.pin, 0644);


static const unsigned long iobase[] = {
    KSEG1ADDR(I2C0_IOBASE),
    KSEG1ADDR(I2C1_IOBASE),
};

#define I2C_ADDR(id, reg)               ((volatile unsigned long *)((iobase[id]) + (reg)))

static inline void jz_i2c_write_reg(int id, unsigned int reg, unsigned int value)
{
    *I2C_ADDR(id, reg) = value;
}

static inline unsigned int jz_i2c_read_reg(int id, unsigned int reg)
{
    return *I2C_ADDR(id, reg);
}

static inline void jz_i2c_set_bit(int id, unsigned int reg, int start, int end, unsigned int val)
{
    set_bit_field(I2C_ADDR(id, reg), start, end, val);
}

static inline unsigned int jz_i2c_get_bit(int id, unsigned int reg, int start, int end)
{
    return get_bit_field(I2C_ADDR(id, reg), start, end);
}

static void jz_i2c_hal_disable_smb(int id)
{
    jz_i2c_set_bit(id, SMB_ENABLE, SMBENABLE_SMBENB, 0);
}

static void jz_i2c_hal_enable_smb(int id)
{
    jz_i2c_set_bit(id, SMB_ENABLE, SMBENABLE_SMBENB, 1);
}

static unsigned int jz_i2c_hal_wait_smb_enable_status(int id)
{
    return jz_i2c_get_bit(id, SMB_ENBST, SMBENBST_SMBEN);
}

static void jz_i2c_hal_set_speed_standard(int id)
{
    jz_i2c_set_bit(id, SMB_CON, SMBCON_SPEED, 1);
}

static void jz_i2c_hal_set_speed_fast(int id)
{
    jz_i2c_set_bit(id, SMB_CON, SMBCON_SPEED, 2);
}

static void jz_i2c_hal_set_smb_shcnt(int id, int val)
{
    if (val < 6) {
        val = 6;
    }

    jz_i2c_set_bit(id, SMB_SHCNT, SMBSHCNT, val);
}

static void jz_i2c_hal_set_smb_slcnt(int id, int val)
{
    if (val < 8) {
        val = 8;
    }

    jz_i2c_set_bit(id, SMB_SLCNT, SMBSLCNT, val);
}

static void jz_i2c_hal_set_smb_fhcnt(int id, int val)
{
    if (val < 6) {
        val = 6;
    }

    jz_i2c_set_bit(id, SMB_FHCNT, SMBFHCNT, val);
}

static void jz_i2c_hal_set_smb_flcnt(int id, int val)
{
    if (val < 8) {
        val = 8;
    }

    jz_i2c_set_bit(id, SMB_FLCNT, SMBFLCNT, val);
}

static void jz_i2c_hal_set_setup_time(int id, int val)
{
    jz_i2c_set_bit(id, SMB_SDASU, SMBSDASU, val);
}

static void jz_i2c_hal_set_hold_time(int id, int val)
{
    jz_i2c_set_bit(id, SMB_SDAHD, SMBSDAHD, val);
}

static void jz_i2c_hal_set_addressing(int id, enum i2c_addr_type addr_bit)
{
    if (addr_bit == I2C_ADDR_BIT_10)
        jz_i2c_set_bit(id, SMB_TAR, SMBTAR_MATP, 1);
    else
        jz_i2c_set_bit(id, SMB_TAR, SMBTAR_MATP, 0);
}

/*-----------------------------------------------------------------------------------------------*/

static const char *abrt_src[] = {
    "I2C_TXABRT_ABRT_7B_ADDR_NOACK",
    "I2C_TXABRT_ABRT_10ADDR1_NOACK",
    "I2C_TXABRT_ABRT_10ADDR2_NOACK",
    "I2C_TXABRT_ABRT_XDATA_NOACK",
    "I2C_TXABRT_ABRT_GCALL_NOACK",
    "I2C_TXABRT_ABRT_GCALL_READ",
    "I2C_TXABRT_ABRT_HS_ACKD",
    "I2C_TXABRT_SBYTE_ACKDET",
    "I2C_TXABRT_ABRT_HS_NORSTRT",
    "I2C_TXABRT_SBYTE_NORSTRT",
    "I2C_TXABRT_ABRT_10B_RD_NORSTRT",
    "I2C_TXABRT_ABRT_MASTER_DIS",
    "I2C_TXABRT_ARB_LOST",
    "I2C_TXABRT_SLVFLUSH_TXFIFO",
    "I2C_TXABRT_SLV_ARBLOST",
    "I2C_TXABRT_SLVRD_INTX",
};

static inline void jz_i2c_set_rxfifo_threshold(int id, int len)
{
    if (len > MAX_FIFO_LEN)
        jz_i2c_set_bit(id, SMB_RXTL, SMBRXTL_RXTL, RX_TL);
    else
        jz_i2c_set_bit(id, SMB_RXTL, SMBRXTL_RXTL, len - 1);
}

static inline void jz_i2c_set_txfifo_threshold(int id, int len)
{
    if (len > MAX_FIFO_LEN)
        jz_i2c_set_bit(id, SMB_TXTL, SMBTXTL_TXTL, TX_TL);
    else
        jz_i2c_set_bit(id, SMB_TXTL, SMBTXTL_TXTL, 0);
}

static void jz_i2c_read_rx_fifo(struct jz_i2c_drv *drv, int len)
{
    unsigned short tmp;
    int id = drv->id;
    drv->len -= len;

    while(len > 0) {
        tmp = jz_i2c_get_bit(id, SMB_DC, SMBDC_DAT);
        *drv->rx_buf++ = tmp;
        len--;
    }
}

/*
 * NOTE:发送读数据的个数是通过写RX FIFO 的次数来确定的
 */
static void jz_i2c_write_rx_fifo(struct jz_i2c_drv *drv, int len, int first)
{
    unsigned short tmp = 0;
    int id = drv->id;
    int cmd = drv->cmd;

    len = len > drv->rd_num ? drv->rd_num : len;/* 本次最多可以发送的读操作个数 */
    drv->rd_num -= len;

    while (len > 0) {
        tmp = READ_CMD;
        if (first && (cmd & I2C_RE_START)) {
            tmp |= RESTART_CMD;
            first = 0;
        }

        if (len == 1 && drv->rd_num == 0) {
            if (cmd & I2C_STOP)
                tmp |= STOP_CMD;
        }
        len--;
        jz_i2c_write_reg(id, SMB_DC, tmp);
    }
}

static void jz_i2c_write_tx_fifo(struct jz_i2c_drv* drv, int len, int first)
{
    unsigned short tmp = 0;
    int id = drv->id;
    int cmd = drv->cmd;

    len = len > drv->len ? drv->len : len;/* 本次最多可以发送的数据个数 */
    drv->len -= len;

    while (len > 0) {
        tmp = *(drv->tx_buf)++;
        tmp &= 0xff;
        if (first && (cmd & I2C_RE_START)) {
            tmp |= RESTART_CMD;
            first = 0;
        }

        if (len == 1 && drv->len == 0) {
            if (cmd & I2C_STOP)
                tmp |= STOP_CMD;
        }
        len--;
        jz_i2c_write_reg(id, SMB_DC, tmp);
    }
}

static void jz_i2c_hal_set_speed(int id)
{
    int high_cnt = 0;
    int low_cnt = 0;
    int setup_time = 0;
    int hold_time = 0;
    int mode = 1;
    unsigned long rate = 0;
    unsigned long i2c_clk_rate = clk_get_rate(jz_i2c_dev[id].clk);

    /*
     * I2C通讯速度: 标准模式 100K 或 快速模式400K
     */
    jz_i2c_dev[id].rate = jz_i2c_dev[id].rate <= (100*1000) ? (100 * 1000) : (400 * 1000);
    rate = jz_i2c_dev[id].rate;
    mode = jz_i2c_dev[id].rate <= 100000 ? 1 : 0;

    /*
     *           high
     *          _____       _____
     *  clk  __|     |_____|     |____ ...
     *                 low
     *         |<-单个周期->|
     *
     *  在这里 high_cnt和low_cnt各占单个周期的二分之一.
     */
    high_cnt = i2c_clk_rate / (rate * 2);
    low_cnt = i2c_clk_rate / (rate * 2);

    /*
     *            high
     *          _____       _____      _____
     *  clk  __|  |  |_____|     |____|     |____ ...
     *            |  |  |
     *            |  |  |
     *            |__|__|     _____
     *  data _____/  |  |\___/      \____ ...
     *            |  |  |
     *     setup->|  |< |
     *             ->|  |<-hold
     *
     *  在这里 setup time和hold time各占单个周期的四分之一.
     */
    setup_time = i2c_clk_rate / (rate * 4);
    if (setup_time > 1)
        setup_time -= 1;

    hold_time = i2c_clk_rate / (rate * 4);

    if (setup_time > MAX_SETUP_TIME)
        setup_time = MAX_SETUP_TIME;

    if (setup_time < 1)
        setup_time = 1;

    if (hold_time > MAX_HOLD_TIME)
        hold_time = MAX_HOLD_TIME;

    /* 若没有配置，默认使用高速模式 */
    if (mode) {
        /* standard 标准模式 */
        jz_i2c_hal_set_speed_standard(id);
        jz_i2c_hal_set_smb_shcnt(id, high_cnt);
        jz_i2c_hal_set_smb_slcnt(id, low_cnt);
    } else {
        /* fast 高速模式 */
        jz_i2c_hal_set_speed_fast(id);
        jz_i2c_hal_set_smb_fhcnt(id, high_cnt);
        jz_i2c_hal_set_smb_flcnt(id, low_cnt);
    }

    jz_i2c_hal_set_setup_time(id, setup_time);
    jz_i2c_hal_set_hold_time(id, hold_time);
}


static void jz_i2c_init_setting(struct jz_i2c_drv *drv)
{
    int id = drv->id;
    unsigned int smb_flt_value = 0;

    /* 开时钟 */
    clk_enable(drv->clk);

    /* 关控制器,且等待至完全关闭状态 */
    jz_i2c_hal_disable_smb(id);
    jz_i2c_set_bit(id, SMB_CON, SMBCON_RESTART, 1);
    while (jz_i2c_hal_wait_smb_enable_status(id) != 0);

    /* 设置I2C的地址类型 */
    jz_i2c_hal_set_addressing(id, I2C_ADDR_BIT_7);

    /* 设置I2C的速度 */
    jz_i2c_hal_set_speed(id);

    /* 设置I2C 时序滤波寄存器 单位:APB clock cycles */
    struct clk *clk_apb = clk_get(NULL, "gate_apb0");

    /* 过滤小于时钟周期的1/6的波长 */
    smb_flt_value = clk_get_rate(clk_apb) / jz_i2c_dev[id].rate / 6;
    if (smb_flt_value > 0xff)
        smb_flt_value = 0xff;

    jz_i2c_write_reg(id, SMB_FSPKLEN, smb_flt_value);

    /* 关中断 */
    jz_i2c_write_reg(id, SMB_INTM, 0);

    /* 关时钟 */
    clk_disable(drv->clk);
}

static void jz_i2c_enable(struct jz_i2c_drv *drv)
{
    int id = drv->id;

    /* 开时钟 */
    clk_enable(jz_i2c_dev[id].clk);

    /* 关控制器,且等待至完全关闭状态 */
    jz_i2c_hal_disable_smb(id);
    while (jz_i2c_hal_wait_smb_enable_status(id) != 0);

    /* 设置I2C的速度 */
    jz_i2c_hal_set_speed(id);

    /* 关中断 */
    jz_i2c_write_reg(id, SMB_INTM, 0);

    /* 开控制器 */
    jz_i2c_hal_enable_smb(id);
}

static void jz_i2c_disable(struct jz_i2c_drv *drv)
{
    int tmp;
    int id = drv->id;
    unsigned long timeout = 1000000;

    /* 判断I2C状态 */
    tmp = jz_i2c_get_bit(id, SMB_ST, SMBST_MSTACT);

    while (tmp && (--timeout > 0)) {
        /* 经测试，在传输数据量大的情况下，建议延时10us及以上 */
        udelay(10);
        tmp = jz_i2c_get_bit(id, SMB_ST, SMBST_MSTACT);
    }

    if (timeout <= 0) {
        printk(KERN_ERR "i2c%d disable timeout!\n", id);
    }

    /* 关中断 */
    jz_i2c_write_reg(id, SMB_INTM, 0);

    /* 关控制器,且等待至完全关闭状态 */
    jz_i2c_hal_disable_smb(id);
    while (jz_i2c_hal_wait_smb_enable_status(id) != 0);

    /* 关时钟 */
    clk_disable(jz_i2c_dev[id].clk);
}

static void jz_i2c_reset(struct jz_i2c_drv *drv)
{
    jz_i2c_disable(drv);
    udelay(10);
    jz_i2c_enable(drv);
}

static irqreturn_t i2c_intr_handler(int irq, void *dev)
{
    struct jz_i2c_drv *drv = (struct jz_i2c_drv *)dev;
    int rx_valid;
    int tx_valid;

    int id = drv->id;
    unsigned long smb_intst = jz_i2c_read_reg(id, SMB_INTST);

    /* TXABT 中断被触发 */
    if (get_bit_field(&smb_intst, SMBINTST_TXABT)) {
        drv->abtsrc = jz_i2c_read_reg(id, SMB_ABTSRC);

        //清除 TXABT 中断
        jz_i2c_get_bit(id, SMB_CTXABT, SMBCTXABT);
        complete(&drv->complete);

        return IRQ_HANDLED;
    }

    /* TX FIFO 空中断被触发 */
    if (get_bit_field(&smb_intst, SMBINTST_TXEMP)) {
        if (drv->len == 0) {
            /* 关 FIFO空 中断 */
            jz_i2c_set_bit(id, SMB_INTM, SMBINTM_MTXEMP, 0);

            /* 如果MSG没有STOP信号，需要在这里唤醒 */
            if (!(drv->cmd & I2C_STOP)) {
                jz_i2c_set_bit(id, SMB_INTM, SMBINTM_MISTP, 0);

                complete(&drv->complete);

                return IRQ_HANDLED;
            }
        } else {
            tx_valid = MAX_FIFO_LEN - jz_i2c_read_reg(id, SMB_TXFLR);

            jz_i2c_set_txfifo_threshold(id, drv->len);

            jz_i2c_write_tx_fifo(drv, tx_valid, 0);
        }
    }

    /* RX FIFO 满中断被触发 */
    if (get_bit_field(&smb_intst, SMBINTST_RXFL)) {

        rx_valid = jz_i2c_read_reg(id, SMB_RXFLR);
        jz_i2c_read_rx_fifo(drv, rx_valid);

        if (drv->len > 0) {
            jz_i2c_set_rxfifo_threshold(id, drv->len);
            jz_i2c_write_rx_fifo(drv, rx_valid, 0);
        } else {
            complete(&drv->complete);

            return IRQ_HANDLED;
        }
    }

    /* 停止信号中断被触发 */
    if (get_bit_field(&smb_intst, SMBINTST_ISTP)) {
        jz_i2c_get_bit(id, SMB_CSTP, SMBCSTP);

        complete(&drv->complete);
    }

    return IRQ_HANDLED;
}

static inline unsigned int timeout_us(int id, int len)
{
    unsigned long rate;

    if (!len) {
        return 0;
    }

    rate = jz_i2c_dev[id].rate / 1000;

    return (len + 1) * 1000 * 1000 * 9 * 2 / rate;
}

static inline void show_txabrt(struct jz_i2c_drv *drv, int src)
{
    int i;

    for (i = 0; i < 16; i++) {
        if (src & (0x1 << i)) {
            printk(KERN_ERR "i2c%d TXABRT[%d]=%s\n", drv->id, i, abrt_src[i]);
        }
    }
}

static int jz_i2c_read(struct jz_i2c_drv *drv, int device_addr, char *buf, int len, int cmd)
{
    int ret = 0;
    int id = drv->id;
    int timeout = timeout_us(id, len);
    unsigned long smb_intm = jz_i2c_read_reg(id, SMB_INTM);

    if (len <= 0)
        return 0;

    jz_i2c_dev[id].rd_num = len;
    jz_i2c_dev[id].len = len;
    jz_i2c_dev[id].rx_buf = buf;
    jz_i2c_dev[id].cmd = cmd;

    reinit_completion(&drv->complete);

    /* 清除STOP TXABT中断 */
    jz_i2c_get_bit(id, SMB_CSTP, SMBCSTP);
    jz_i2c_get_bit(id, SMB_CTXABT, SMBCTXABT);

    /* 设置 RX FIFO 阈值 */
    jz_i2c_set_rxfifo_threshold(id, len);

    if (cmd & I2C_START) {
        jz_i2c_write_reg(id, SMB_TAR, device_addr);
    }

    jz_i2c_write_rx_fifo(&jz_i2c_dev[id], MAX_FIFO_LEN, 1);

    /* 开中断 */
    set_bit_field(&smb_intm, SMBINTM_MRXFL, 1);
    set_bit_field(&smb_intm, SMBINTM_MTXABT, 1);

    if (cmd & I2C_STOP) {
        set_bit_field(&smb_intm, SMBINTM_MISTP, 1);
    }

    jz_i2c_write_reg(id, SMB_INTM, smb_intm);

    /* 等待读取完成 */
    if ( wait_for_completion_timeout(&drv->complete, msecs_to_jiffies(timeout / 1000 + 1)) <= 0 ) {
        printk(KERN_ERR "i2c%d read timeout(%d ms)\n", id, timeout / 1000 + 1);

        jz_i2c_reset(drv);

        ret = -ETIMEDOUT;
    }

    if (jz_i2c_dev[id].abtsrc) {

        int src = jz_i2c_dev[id].abtsrc;

        show_txabrt(drv, src);

        printk(KERN_DEBUG "I2C DEVICE Read MAYBE NO SUPPORT RESTART CMD\n");
        ret = -EIO;

        jz_i2c_dev[id].abtsrc = 0;
    }

    return ret;
}


static int jz_i2c_write(struct jz_i2c_drv *drv, int device_addr, char *buf, int len, int cmd)
{
    int ret = 0;
    int id = drv->id;
    int timeout = timeout_us(id, len);
    unsigned long smb_intm = jz_i2c_read_reg(id, SMB_INTM);

    jz_i2c_dev[id].tx_buf   = buf;
    jz_i2c_dev[id].len      = len;
    jz_i2c_dev[id].cmd      = cmd;

    reinit_completion(&drv->complete);

    /* 清除STOP TXABT中断 */
    jz_i2c_get_bit(id, SMB_CSTP, SMBCSTP);
    jz_i2c_get_bit(id, SMB_CTXABT, SMBCTXABT);

    /* 设置TX触发中断的阈值 */
    jz_i2c_set_bit(id, SMB_TXTL, SMBTXTL_TXTL, 0);

    if (cmd & I2C_START) {
        jz_i2c_write_reg(id, SMB_TAR, device_addr);
    }

    /* 执行第一次写操作 */
    jz_i2c_write_tx_fifo(&jz_i2c_dev[id], MAX_FIFO_LEN, 1);

    /* 开中断 */
    set_bit_field(&smb_intm, SMBINTM_MTXEMP, 1);
    set_bit_field(&smb_intm, SMBINTM_MTXABT, 1);

    if (jz_i2c_dev[id].cmd & I2C_STOP) {
        set_bit_field(&smb_intm, SMBINTM_MISTP, 1);
    }

    jz_i2c_write_reg(id, SMB_INTM, smb_intm);

    /* 等待发送完成 */
    if ( wait_for_completion_timeout(&drv->complete, msecs_to_jiffies(timeout / 1000 + 1)) <= 0) {
        printk(KERN_ERR "i2c%d write timeout(%d ms)\n", id, timeout / 1000 + 1);

        jz_i2c_reset(drv);

        ret = -ETIMEDOUT;
    }

    if (jz_i2c_dev[id].abtsrc) {

        int src = jz_i2c_dev[id].abtsrc;

        show_txabrt(drv, src);

        printk(KERN_DEBUG "I2C DEVICE Write MAYBE NO SUPPORT RESTART CMD\n");
        ret = -EIO;

        jz_i2c_dev[id].abtsrc = 0;
    }

    return ret;
}


static int jz_i2c_transfer(struct i2c_adapter *adap, struct i2c_msg *msg, int count)
{
    int i;
    int cmd;
    int ret = 0;
    struct jz_i2c_drv *drv = adap->algo_data;

    jz_i2c_enable(drv);

    /*
     * 当count大于1,即一个msg有多次传输时，开始传输时产生一个开始信号，传输结束时产生一个停止信号.
     * 传输过程只考虑需不需要restart信号.
    */
    for (i = 0; i < count; i++) {

        struct i2c_msg *m = msg + i;

        int addr_bit = m->flags & I2C_M_TEN ? I2C_ADDR_BIT_10 : I2C_ADDR_BIT_7;
        cmd = 0;

        if (i == 0) {
            cmd = I2C_START;/* 第一个msg产生开始信号 I2C_START */
        } else {
            /* 没有I2C_M_NOSTART代表传输过程需要产生restart信号 */
            if (!(m->flags & I2C_M_NOSTART))
                cmd = I2C_RE_START;
        }

        if (i == (count - 1)) {
            cmd |= I2C_STOP;/* 最后一个msg传输结束产生停止信号 */
        }

        jz_i2c_hal_set_addressing(drv->id, addr_bit);

        if (m->flags & I2C_M_RD) {
            ret = jz_i2c_read(drv, m->addr, m->buf, m->len, cmd);
        } else {
            ret = jz_i2c_write(drv, m->addr, m->buf, m->len, cmd);
        }

        if (ret != 0)
            break;
    } /* end of for(...) */

    jz_i2c_disable(drv);

    return ret ? : i;
}

static int jz_i2c_modules_parameter_check(struct jz_i2c_drv *drv)
{
    struct jz_i2c_pin *i2c_pin_alter = drv->alter_pin;
    int num = drv->alter_num;
    int i = 0;

    /* scl */
    for (i = 0; i < num; i++) {
        if (drv->scl.pin == i2c_pin_alter[i].scl.pin) {
            drv->scl.function = i2c_pin_alter[i].scl.function;
            break;
        }
    }
    if (i >= num) {
        char scl_string[10];
        gpio_to_str(drv->scl.pin, scl_string);
        printk(KERN_ERR "I2C%d scl(%s) is invaild\n", drv->id, scl_string);
        return -EINVAL;
    }

    /* sda */
    for (i = 0; i < num; i++) {
        if (drv->sda.pin == i2c_pin_alter[i].sda.pin) {
            drv->sda.function = i2c_pin_alter[i].sda.function;
            break;
        }
    }
    if (i >= num) {
        char sda_string[10];
        gpio_to_str(drv->sda.pin, sda_string);
        printk(KERN_ERR "I2C%d sda(%s) is invaild\n", drv->id, sda_string);
        return -EINVAL;
    }

    return 0;
}

static inline int gpio_init(int gpio, enum gpio_function func, const char *name)
{
    int ret = 0;

    ret = gpio_request(gpio, name);
    if (ret < 0)
        return ret;

    gpio_set_func(gpio , func);

    return 0;
}

static int jz_i2c_gpio_request( struct jz_i2c_drv *drv)
{
    int ret = 0;

    ret = gpio_init(drv->scl.pin , drv->scl.function, drv->scl_name);
    if (ret < 0)
        return ret;

    ret = gpio_init(drv->sda.pin , drv->sda.function, drv->sda_name);
    if (ret < 0) {
        gpio_free(drv->scl.pin);
        return ret;
    }

    return 0;
}


static uint32_t jz_i2c_functionality(struct i2c_adapter *adap)
{
    uint32_t ret;

    ret = I2C_FUNC_I2C | I2C_FUNC_SMBUS_EMUL | I2C_FUNC_10BIT_ADDR | I2C_FUNC_SMBUS_READ_BLOCK_DATA;

    return ret;
}
/*---------------------------------------------------------------------------*/

static const struct i2c_algorithm soc_i2c_algorithm = {
    .master_xfer            = jz_i2c_transfer,
    .functionality          = jz_i2c_functionality,
};


static void jz_i2c_deinit(int id)
{
    struct jz_i2c_drv *drv = &jz_i2c_dev[id];

    i2c_del_adapter(&drv->adap);

    free_irq(drv->irq, drv);

    gpio_free(drv->scl.pin);
    gpio_free(drv->sda.pin);

    clk_disable_unprepare(drv->clk);
    clk_put(drv->clk);
}

static int jz_i2c_init(int id)
{
    int ret = -1;
    struct jz_i2c_drv *drv = &jz_i2c_dev[id];

    ret = jz_i2c_modules_parameter_check(drv);
    if (ret < 0) {
        printk(KERN_ERR "i2c%d modules parameter is invaild\n", id);
        goto err_param_check;
    }

    drv->adap.owner         = THIS_MODULE;
    drv->adap.algo          = &soc_i2c_algorithm;
    drv->adap.retries       = 5;
    drv->adap.timeout       = 5;
    drv->adap.algo_data     = drv;
    drv->adap.nr            = drv->id;
    sprintf(drv->adap.name, "i2c%u", drv->id);

    drv->clk = clk_get(NULL, drv->clk_name);
    if (IS_ERR(drv->clk)) {
        printk(KERN_ERR "i2c%d get clock(%s) failed\n", id, drv->clk_name);
        goto err_get_clock;
    }

    if(clk_prepare(drv->clk) < 0) {
        printk(KERN_ERR "i2c%d  prepare clock(%s) failed\n", id, drv->clk_name);
        goto err_prepare_clock;
    }

    ret = jz_i2c_gpio_request(drv);
    if (ret < 0) {
        printk(KERN_ERR "i2c%d gpio requeset failed\n", id);
        goto err_reuqest_gpio;
    }

    jz_i2c_init_setting(drv);

    init_completion(&drv->complete);

    ret = request_irq(drv->irq, i2c_intr_handler, IRQF_TRIGGER_NONE, drv->irq_name , drv);
    if (ret < 0) {
        printk(KERN_ERR "i2c%d Failed to request irq\n", id);
        goto err_request_irq;
    }

    ret = i2c_add_numbered_adapter(&drv->adap);
    if (ret < 0) {
        printk(KERN_ERR "i2c%d Failed to add bus\n", id);
        goto err_add_adapter;
    }

    drv->is_finish = 1;
    //printk(KERN_DEBUG "i2c%d  register successfull\n", id);

    return 0;

err_add_adapter:
    free_irq(drv->irq, drv);
err_request_irq:
    gpio_free(drv->scl.pin);
    gpio_free(drv->sda.pin);
err_reuqest_gpio:
    clk_unprepare(drv->clk);
err_prepare_clock:
    clk_put(drv->clk);
err_get_clock:
err_param_check:
    drv->is_finish = 0;
    return ret;
}

static int __init jz_arch_i2c_init(void)
{
    if (jz_i2c_dev[0].is_enable)
        jz_i2c_init(0);

    if (jz_i2c_dev[1].is_enable)
        jz_i2c_init(1);

    return 0;
}

static void __exit jz_arch_i2c_exit(void)
{
    if (jz_i2c_dev[0].is_finish)
        jz_i2c_deinit(0);

    if (jz_i2c_dev[1].is_finish)
        jz_i2c_deinit(1);
}

module_init(jz_arch_i2c_init);
module_exit(jz_arch_i2c_exit);

MODULE_DESCRIPTION("X1600 SoC I2C driver");
MODULE_LICENSE("GPL");
