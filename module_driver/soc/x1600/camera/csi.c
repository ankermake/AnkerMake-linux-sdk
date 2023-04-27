/*
 * Copyright (C) 2021 Ingenic Semiconductor Co., Ltd.
 *
 * Camera driver for the Ingenic MIPI-CSI controller
 *
 */

#include <common.h>
#include <bit_field.h>
#include <linux/delay.h>
#include <linux/clk.h>
#include <linux/clk-provider.h>
#include "csi.h"

struct jz_mipi_csi_drv {
    int id;
    int is_enable;
    const char *name;
    const char *csi_clk_name;
    struct clk *csi_gate_clk;
};

/*
 * MIPI CSI information
 */
static struct jz_mipi_csi_drv mipi_csi_dev = {
    .id                     = 0,
    .is_enable              = 0,
    .name                   = "mipi_csi",
    .csi_clk_name           = "gate_csi",
};

static DEFINE_SPINLOCK(csi_reset_lock);


/*
 * MIPI CSI operation
 */
#define MIPI_CSI_ADDR(reg)              ((volatile unsigned long *)CKSEG1ADDR(MIPI_CSI_IOBASE + reg))

static inline unsigned int mipi_csi_read_reg(unsigned int reg)
{
    return *MIPI_CSI_ADDR(reg);
}

static inline void mipi_csi_write_reg(unsigned int reg, unsigned int val)
{
    *MIPI_CSI_ADDR(reg) = val;
}

static inline unsigned int mipi_csi_get_bit(unsigned int reg, int start, int end)
{
    return get_bit_field(MIPI_CSI_ADDR(reg), start, end);
}

static inline void mipi_csi_set_bit(unsigned int reg, int start, int end, unsigned int val)
{
    set_bit_field(MIPI_CSI_ADDR(reg), start, end, val);
}

static inline void mipi_csi_dphy_test_clear(int value)
{
    mipi_csi_set_bit(MIPI_PHY_TST_CTRL0, MIPI_PHY_test_ctrl0_testclr, value);
}

static inline void mipi_csi_dphy_test_clock(int value)
{
    mipi_csi_set_bit(MIPI_PHY_TST_CTRL0, MIPI_PHY_test_ctrl0_testclk, value);
}

static inline void mipi_csi_dphy_test_en(unsigned char on_falling_edge)
{
    mipi_csi_set_bit(MIPI_PHY_TST_CTRL1, MIPI_PHY_test_ctrl1_testen, on_falling_edge);
}

static inline void mipi_csi_dphy_test_data_in(unsigned char data)
{
    mipi_csi_write_reg(MIPI_PHY_TST_CTRL1, data);
}

static inline void mipi_csi_dphy_set_lanes(int lanes)
{
    mipi_csi_set_bit(MIPI_N_LANES, MIPI_PHY_n_lanes, lanes - 1);
}

static inline void mipi_csi_dphy_phy_shutdown(int enable)
{
    mipi_csi_set_bit(MIPI_PHY_SHUTDOWNZ, MIPI_PHY_phy_shutdown, enable);
}

static inline void mipi_csi_dphy_dphy_reset(int state)
{
    mipi_csi_set_bit(MIPI_DPHY_RSTZ, MIPI_PHY_dphy_reset, state);
}

static inline void mipi_csi_dphy_csi2_reset(int state)
{
    mipi_csi_set_bit(MIPI_CSI2_RESETN, MIPI_PHY_csi2_reset, state);
}

static inline int mipi_csi_dphy_get_state(void)
{
    return mipi_csi_read_reg(MIPI_PHY_STATE);
}

/******************************************************************************
 *
 * MIPI D-PHY Receiver
 *
 *****************************************************************************/
#define MIPI_PHY_ADDR(reg)              ((volatile unsigned long *)CKSEG1ADDR(MIPI_PHY_IOBASE + reg))

static inline unsigned int mipi_phy_receiver_read_reg(unsigned int reg)
{
    return *MIPI_PHY_ADDR(reg);
}

static inline void mipi_phy_receiver_write_reg(unsigned int reg, unsigned int val)
{
    *MIPI_PHY_ADDR(reg) = val;
}

static inline void mipi_phy_receiver_set_bit(unsigned int reg, int start, int end, unsigned int val)
{
    set_bit_field(MIPI_PHY_ADDR(reg), start, end, val);
}

static inline void mipi_phy_receiver_set_lanes_enable(int lanes)
{
    mipi_phy_receiver_set_bit(MIPI_PHY_RXPHY_REG_0_00, MIPI_PHY_RXPHY_power_enable, 1);

    mipi_phy_receiver_set_bit(MIPI_PHY_RXPHY_REG_0_00, MIPI_PHY_RXPHY_clk_lane_enable, 1);

    mipi_phy_receiver_set_bit(MIPI_PHY_RXPHY_REG_0_00, MIPI_PHY_RXPHY_data_lane0_enable, 1);
    if (lanes >= 2)
        mipi_phy_receiver_set_bit(MIPI_PHY_RXPHY_REG_0_00, MIPI_PHY_RXPHY_data_lane1_enable, 1);
}


static inline void mipi_phy_receiver_set_lanes_disable(int lanes)
{
    mipi_phy_receiver_set_bit(MIPI_PHY_RXPHY_REG_0_00, MIPI_PHY_RXPHY_data_lane0_enable, 0);
    if (lanes >= 2)
        mipi_phy_receiver_set_bit(MIPI_PHY_RXPHY_REG_0_00, MIPI_PHY_RXPHY_data_lane1_enable, 0);

    mipi_phy_receiver_set_bit(MIPI_PHY_RXPHY_REG_0_00, MIPI_PHY_RXPHY_clk_lane_enable, 0);

    mipi_phy_receiver_set_bit(MIPI_PHY_RXPHY_REG_0_00, MIPI_PHY_RXPHY_power_enable, 2);
}

static inline void mipi_phy_receiver_clock_lane_countinuous_mode(int continue_mode)
{
    if (continue_mode)
        mipi_phy_receiver_set_bit(MIPI_PHY_RXPHY_REG_2_0A, MIPI_PHY_RXPHY_clk_continue_mode, 3);
    else
        mipi_phy_receiver_set_bit(MIPI_PHY_RXPHY_REG_2_0A, MIPI_PHY_RXPHY_clk_continue_mode, 0);
}

static inline void mipi_phy_set_clk_settle(int value)
{
    mipi_phy_receiver_set_bit(MIPI_PHY_RXPHY_REG_2_18, MIPI_PHY_RXPHY_clk_settle_time, value);
}

static inline void mipi_phy_set_data0_settle(int value)
{
    mipi_phy_receiver_set_bit(MIPI_PHY_RXPHY_REG_3_18, MIPI_PHY_RXPHY_data0_settle_time, value);
}

static inline void mipi_phy_set_data1_settle(int value)
{
    mipi_phy_receiver_set_bit(MIPI_PHY_RXPHY_REG_4_18, MIPI_PHY_RXPHY_data1_settle_time, value);
}


void mipi_csi_dphy_dump_reg(int index)
{
    printk(KERN_ERR "================ dump mipi csi reg ================\n");
    printk(KERN_ERR "VERSION             : 0x%08x\n", mipi_csi_read_reg(MIPI_VERSION));
    printk(KERN_ERR "N_LANES             : 0x%08x\n", mipi_csi_read_reg(MIPI_N_LANES));
    printk(KERN_ERR "PHY_SHUTDOWNZ       : 0x%08x\n", mipi_csi_read_reg(MIPI_PHY_SHUTDOWNZ));
    printk(KERN_ERR "DPHY_RSTZ           : 0x%08x\n", mipi_csi_read_reg(MIPI_DPHY_RSTZ));
    printk(KERN_ERR "CSI2_RESETN         : 0x%08x\n", mipi_csi_read_reg(MIPI_CSI2_RESETN));
    printk(KERN_ERR "PHY_STATE           : 0x%08x\n", mipi_csi_read_reg(MIPI_PHY_STATE));
    printk(KERN_ERR "DATA_IDS_1          : 0x%08x\n", mipi_csi_read_reg(MIPI_DATA_IDS_1));
    printk(KERN_ERR "DATA_IDS_2          : 0x%08x\n", mipi_csi_read_reg(MIPI_DATA_IDS_2));
    printk(KERN_ERR "ERR1                : 0x%08x\n", mipi_csi_read_reg(MIPI_ERR1));
    printk(KERN_ERR "ERR2                : 0x%08x\n", mipi_csi_read_reg(MIPI_ERR2));
    printk(KERN_ERR "MASK1               : 0x%08x\n", mipi_csi_read_reg(MIPI_MASK1));
    printk(KERN_ERR "MASK2               : 0x%08x\n", mipi_csi_read_reg(MIPI_MASK2));
    printk(KERN_ERR "PHY_TST_CTRL0       : 0x%08x\n", mipi_csi_read_reg(MIPI_PHY_TST_CTRL0));
    printk(KERN_ERR "PHY_TST_CTRL1       : 0x%08x\n", mipi_csi_read_reg(MIPI_PHY_TST_CTRL1));

    printk(KERN_ERR "================ dump mipi dphy receiver reg ================\n");
    printk(KERN_ERR "REG_0_00(DPHY_CFG)  : 0x%08x\n", mipi_phy_receiver_read_reg(MIPI_PHY_RXPHY_REG_0_00));
    printk(KERN_ERR "REG_2_0A(DPHY_MODE) : 0x%08x\n", mipi_phy_receiver_read_reg(MIPI_PHY_RXPHY_REG_2_0A));
    printk(KERN_ERR "REG_2_18(CLK_CFG)   : 0x%08x\n", mipi_phy_receiver_read_reg(MIPI_PHY_RXPHY_REG_2_18));
    printk(KERN_ERR "REG_3_18(DATA0_CFG) : 0x%08x\n", mipi_phy_receiver_read_reg(MIPI_PHY_RXPHY_REG_3_18));
    printk(KERN_ERR "REG_4_18(DATA1_CFG) : 0x%08x\n", mipi_phy_receiver_read_reg(MIPI_PHY_RXPHY_REG_4_18));
}


static unsigned char mipi_csi_event_disable(unsigned int  mask, unsigned char err_reg_no)
{
    switch (err_reg_no) {
    case CSI_ERR_MASK_REGISTER1:
        mipi_csi_write_reg(MIPI_MASK1, mask | mipi_csi_read_reg(MIPI_MASK1));
        break;
    case CSI_ERR_MASK_REGISTER2:
        mipi_csi_write_reg(MIPI_MASK2, mask | mipi_csi_read_reg(MIPI_MASK2));
        break;
    default:
        return -EINVAL;
    }

    return 0;
}


static void mipi_dphy_phy_settle_time(int lanes, unsigned long freq)
{
    int settle_value = CSI_LANES_FREQ_700_800MHz;

    if (freq >= 80 * 1000 * 1000UL && freq < 110 * 1000 * 1000UL)
        settle_value = CSI_LANES_FREQ_80_110MHz;
    else if (freq >= 110 * 1000 * 1000UL && freq < 150 * 1000 * 1000UL)
        settle_value = CSI_LANES_FREQ_110_150MHz;
    else if (freq >= 150 * 1000 * 1000UL && freq < 300 * 1000 * 1000UL)
        settle_value = CSI_LANES_FREQ_150_300MHz;
    else if (freq >= 300 * 1000 * 1000UL && freq < 400 * 1000 * 1000UL)
        settle_value = CSI_LANES_FREQ_300_400MHz;
    else if (freq >= 400 * 1000 * 1000UL && freq < 500 * 1000 * 1000UL)
        settle_value = CSI_LANES_FREQ_400_500MHz;
    else if (freq >= 500 * 1000 * 1000UL && freq < 600 * 1000 * 1000UL)
        settle_value = CSI_LANES_FREQ_500_600MHz;
    else if (freq >= 600 * 1000 * 1000UL && freq < 700 * 1000 * 1000UL)
        settle_value = CSI_LANES_FREQ_600_700MHz;
    else if (freq >= 700 * 1000 * 1000UL && freq < 800 * 1000 * 1000UL)
        settle_value = CSI_LANES_FREQ_700_800MHz;
    else if (freq >= 800 * 1000 * 1000UL && freq < 1000 * 1000 * 1000UL)
        settle_value = CSI_LANES_FREQ_800_1000MHz;
    else if (freq >= 1000 * 1000 * 1000UL && freq < 1200 * 1000 * 1000UL)
        settle_value = CSI_LANES_FREQ_1000_1200MHz;
    else if (freq >= 1200 * 1000 * 1000UL && freq < 1400 * 1000 * 1000UL)
        settle_value = CSI_LANES_FREQ_1200_1400MHz;
    else if (freq >= 1400 * 1000 * 1000UL && freq < 1600 * 1000 * 1000UL)
        settle_value = CSI_LANES_FREQ_1400_1600MHz;
    else if (freq >= 1600 * 1000 * 1000UL && freq < 1800 * 1000 * 1000UL)
        settle_value = CSI_LANES_FREQ_1600_1800MHz;
    else if (freq >= 1800 * 1000 * 1000UL && freq < 2000 * 1000 * 1000UL)
        settle_value = CSI_LANES_FREQ_1800_2000MHz;
    else if (freq >= 2000 * 1000 * 1000UL && freq < 2200 * 1000 * 1000UL)
        settle_value = CSI_LANES_FREQ_2000_2200MHz;
    else if (freq >= 2200 * 1000 * 1000UL && freq < 2400 * 1000 * 1000UL)
        settle_value = CSI_LANES_FREQ_2200_2400MHz;
    else if (freq >= 2400 * 1000 * 1000UL && freq < 2500 * 1000 * 1000UL)
        settle_value = CSI_LANES_FREQ_2000_2200MHz;
    else {
        printk(KERN_ERR "freq not in range [80MHz ~ 2500MHz], clk/data settle time use defualt\n");
        return ;
    }

    mipi_phy_set_clk_settle(settle_value);
    mipi_phy_set_data0_settle(settle_value);
    mipi_phy_set_data1_settle(settle_value);
}

static int mipi_csi_gate_clock_enable(void)
{
    struct jz_mipi_csi_drv *drv = &mipi_csi_dev;

    drv->csi_gate_clk = clk_get(NULL, drv->csi_clk_name);
    assert(drv->csi_gate_clk);
    assert(!clk_prepare(drv->csi_gate_clk));

    clk_enable(drv->csi_gate_clk);

    return 0;
}

static int mipi_csi_gate_clock_disable(void)
{
    struct jz_mipi_csi_drv *drv = &mipi_csi_dev;

    assert(drv->csi_gate_clk);

    clk_disable(drv->csi_gate_clk);

    return 0;
}

static int mipi_csi_phy_ready(int lanes)
{
    int ready;
    int ret = 0;

    ready = mipi_csi_dphy_get_state();
    ret = ready & (1 << MIPI_PHY_STATE_CLK_LANE_STOP );

    switch (lanes) {
    case 2:
        ret |= ret && (ready & (1 << MIPI_PHY_STATE_DATA_LANE1));
    case 1:
        ret |= ret && (ready & (1 << MIPI_PHY_STATE_DATA_LANE0));
        break;
    default:
        printk(KERN_ERR "Do not support lane num %d!\n", lanes);
        ret = -EINVAL;
        break;
    }

    return !!ret;
}

static int mipi_csi_phy_configure(struct mipi_csi_bus *mipi_info)
{
    int ret = 0;
    unsigned long flags;
    spin_lock_irqsave(&csi_reset_lock, flags);

    int lanes = mipi_info->lanes;

    /* 参数检查 */
    if (mipi_csi_dev.is_enable) {
        printk(KERN_ERR "mipi csi is already used by other controller, please check config paramer!\n");
        ret = -EINVAL;
        goto mipi_csi_spin_unlock;
    }

    mipi_csi_gate_clock_enable();

    /*
     * Reset PHY CSI
     */
    mipi_csi_dphy_phy_shutdown(0);
    mipi_csi_dphy_dphy_reset(0);
    mipi_csi_dphy_csi2_reset(0);

    udelay(100);

    mipi_csi_dphy_phy_shutdown(1);
    mipi_csi_dphy_dphy_reset(1);
    mipi_csi_dphy_csi2_reset(1);

    mipi_dphy_phy_settle_time(lanes, mipi_info->clk);

    mipi_csi_dphy_test_clear(1);
    udelay(100);

    mipi_csi_dphy_set_lanes(lanes);
    mipi_phy_receiver_set_lanes_enable(lanes);
    mipi_phy_receiver_clock_lane_countinuous_mode(1);

    /* MASK all interrupts */
    mipi_csi_event_disable(0xffffffff, CSI_ERR_MASK_REGISTER1);
    mipi_csi_event_disable(0xffffffff, CSI_ERR_MASK_REGISTER2);

    mipi_csi_dev.is_enable = 1;

mipi_csi_spin_unlock:
    spin_unlock_irqrestore(&csi_reset_lock, flags);
    return ret;
}

int mipi_csi_phy_stop(struct mipi_csi_bus *mipi_info)
{
    int ret = 0;
    unsigned long flags;

    spin_lock_irqsave(&csi_reset_lock, flags);

    int lanes = mipi_info->lanes;

    if (!mipi_csi_dev.is_enable) {
        printk(KERN_ERR "csi is not enabled, please enable first!\n");
        ret = -EINVAL;
        goto mipi_csi_spin_unlock;
    }

    mipi_csi_dev.is_enable = 0;

    mipi_phy_receiver_set_lanes_disable(lanes);

    mipi_csi_dphy_csi2_reset(0);

    mipi_csi_dphy_dphy_reset(0);

    mipi_csi_dphy_phy_shutdown(0);

    mipi_csi_gate_clock_disable();

mipi_csi_spin_unlock:
    spin_unlock_irqrestore(&csi_reset_lock, flags);

    return ret;
}

int mipi_csi_phy_initialization(struct mipi_csi_bus *mipi_info)
{
    int ret = 0;
    int csi_lanes = mipi_info->lanes;

    ret = mipi_csi_phy_configure(mipi_info);
    if  (ret < 0) {
        printk(KERN_ERR "mipi csi configure failed\n");
        return -EINVAL;
    }

    /*
     * 检查MIPI CLK DATA的状态是否为停止状态
     */
    int i;
    int retries = 30;
    for (i = 0; i < retries; i++) {
        if (mipi_csi_phy_ready(csi_lanes))
            break;

        udelay(2);
    }

    if (i >= retries) {
        /*
         * 此为警告而非错误，部分sensor在发送初始化命令后没有进入到STOP状态,即LP-11状态,
         * 但在sensor发送StreamON命令后必须有LP-11到HS的时序跳变，否则可能出现采图失败/出错等状况
         */
        printk(KERN_DEBUG "warning: mipi csi clk/data lanes NOT in stop state\n");
        printk(KERN_DEBUG "VERSION             : 0x%08x\n", mipi_csi_read_reg(MIPI_VERSION));
        printk(KERN_DEBUG "N_LANES             : 0x%08x\n", mipi_csi_read_reg(MIPI_N_LANES));
        printk(KERN_DEBUG "PHY_STATE           : 0x%08x\n", mipi_csi_read_reg(MIPI_PHY_STATE));
        ret = EIO;
    }

    return ret;
}
