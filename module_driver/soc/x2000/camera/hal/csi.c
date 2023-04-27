/*
 * Copyright (C) 2020 Ingenic Semiconductor Co., Ltd.
 *
 * Camera driver for the Ingenic MIPI-CSI controller
 *
 */

#include <linux/sched.h>
#include <bit_field.h>
#include <linux/delay.h>
#include <linux/clk.h>
#include <linux/clk-provider.h>
#include <common.h>

#include "csi.h"

#define MIPI_DPHY_CLKCFG_IN_MHZ         150     /* APB clk */

struct jz_mipi_csi_drv {
    int id;
    int is_enable;
    const char *name;
    const char *csi_clk_name;       /* CSI0/1 共用 */
    struct clk *csi_gate_clk;
};

/*
 * MIPI CSI information
 */
static struct jz_mipi_csi_drv mipi_csi_dev[2] = {
    {
        .id                     = 0,
        .is_enable              = 0,
        .name                   = "csi0",
        .csi_clk_name           = "gate_csi",
    },

    {
        .id                     = 1,
        .is_enable              = 0,
        .name                   = "csi1",
        .csi_clk_name           = "gate_csi",
    },

};

DEFINE_SPINLOCK(csi_reset_lock);

static const unsigned long mipi_csi_iobase[] = {
        KSEG1ADDR(MIPI_CSI0_IOBASE),
        KSEG1ADDR(MIPI_CSI1_IOBASE),
};

/*
 * MIPI CSI operation
 */
#define MIPI_CSI_ADDR(id, reg)          ((volatile unsigned long *)((mipi_csi_iobase[id]) + (reg)))

static inline unsigned int mipi_csi_read_reg(int index, unsigned int reg)
{
    return *MIPI_CSI_ADDR(index, reg);
}

static inline void mipi_csi_write_reg(int index, unsigned int reg, unsigned int val)
{
    *MIPI_CSI_ADDR(index, reg) = val;
}

static inline unsigned int mipi_csi_get_bit(int index, unsigned int reg, int start, int end)
{
    return get_bit_field(MIPI_CSI_ADDR(index, reg), start, end);
}

static inline void mipi_csi_set_bit(int index, unsigned int reg, int start, int end, unsigned int val)
{
    set_bit_field(MIPI_CSI_ADDR(index, reg), start, end, val);
}

static inline void mipi_csi_dphy_test_clear(int index, int value)
{
    mipi_csi_set_bit(index, MIPI_PHY_TST_CTRL0, MIPI_PHY_test_ctrl0_testclr, value);
}

static inline void mipi_csi_dphy_test_clock(int index, int value)
{
    mipi_csi_set_bit(index, MIPI_PHY_TST_CTRL0, MIPI_PHY_test_ctrl0_testclk, value);
}

static inline void mipi_csi_dphy_test_en(int index, unsigned char on_falling_edge)
{
    mipi_csi_set_bit(index, MIPI_PHY_TST_CTRL1, MIPI_PHY_test_ctrl1_testen, on_falling_edge);
}

static inline void mipi_csi_dphy_test_data_in(int index, unsigned char data)
{
    mipi_csi_write_reg(index, MIPI_PHY_TST_CTRL1, data);
}

static inline void mipi_csi_dphy_set_lanes(int index, int lanes)
{
    mipi_csi_set_bit(index, MIPI_N_LANES, MIPI_PHY_n_lanes, lanes - 1);
}

static inline void mipi_csi_dphy_phy_shutdown(int index, int enable)
{
    mipi_csi_set_bit(index, MIPI_PHY_SHUTDOWNZ, MIPI_PHY_phy_shutdown, enable);
}

static inline void mipi_csi_dphy_dphy_reset(int index, int state)
{
    mipi_csi_set_bit(index, MIPI_DPHY_RSTZ, MIPI_PHY_dphy_reset, state);
}

static inline void mipi_csi_dphy_csi2_reset(int index, int state)
{
    mipi_csi_set_bit(index, MIPI_CSI2_RESETN, MIPI_PHY_csi2_reset, state);
}

static inline int mipi_csi_dphy_get_state(int index)
{
    return mipi_csi_read_reg(index ,MIPI_PHY_STATE);
}

/*
 * MIPI PHY (Only One PHY)
 */
#define MIPI_PHY_ADDR(reg)              ((volatile unsigned long *)CKSEG1ADDR(MIPI_PHY_IOBASE + (reg)))

static inline unsigned int mipi_phy_read_reg(unsigned int reg)
{
    return *MIPI_PHY_ADDR(reg);
}

static inline void mipi_phy_write_reg(unsigned int reg, unsigned int val)
{
    *MIPI_PHY_ADDR(reg) = val;
}

static inline void mipi_phy_set_bit(unsigned int reg, int start, int end, unsigned int val)
{
    set_bit_field(MIPI_PHY_ADDR(reg), start, end, val);
}

/*
 * mode: =0, 1 channel
 *       =1, 2 channel
 *       4lane模式下, 该值必须为0
 */
static inline void mipi_phy_set_csi_mode(int mode)
{
    mipi_phy_write_reg(MIPI_PHY_1C2C_MODE, mode);
}

/*
 * Tclk-settle = (4 + precounter_clk) * cfgclkin(100M)
 * precounter_clk = default = 0x08
 * Tclk-settle = 120ns
 */
static inline void mipi_phy_set_clk0_settle(int value)
{
    mipi_phy_set_bit(MIPI_PHY_PRECOUNTER_IN_CLK, MIPI_PHY_precounter_clk0, value);
}

static inline void mipi_phy_set_clk1_settle(int value)
{
    mipi_phy_set_bit(MIPI_PHY_PRECOUNTER_IN_CLK, MIPI_PHY_precounter_clk1, value);
}

/*
 * Ths-settle = (4 + precounter_data) * cfgclkin(100M)
 * precounter_data = default = 0x07
 * Ths-settle = 110ns
 */
static inline void mipi_phy_set_data0_settle(int value)
{
    mipi_phy_set_bit(MIPI_PHY_PRECOUNTER_IN_DATA, MIPI_PHY_precounter_data0, value);
}

static inline void mipi_phy_set_data1_settle(int value)
{
    mipi_phy_set_bit(MIPI_PHY_PRECOUNTER_IN_DATA, MIPI_PHY_precounter_data1, value);
}

static inline void mipi_phy_set_data2_settle(int value)
{
    mipi_phy_set_bit(MIPI_PHY_PRECOUNTER_IN_DATA, MIPI_PHY_precounter_data2, value);
}

static inline void mipi_phy_set_data3_settle(int value)
{
    mipi_phy_set_bit(MIPI_PHY_PRECOUNTER_IN_DATA, MIPI_PHY_precounter_data3, value);
}

static inline void mipi_csi_dphy_dump_reg(int index)
{
    printk("================ dump mipi csi(%d) reg ================\n", index);
    printk("VERSION             : 0x%08x\n", mipi_csi_read_reg(index, MIPI_VERSION));
    printk("N_LANES             : 0x%08x\n", mipi_csi_read_reg(index, MIPI_N_LANES));
    printk("PHY_SHUTDOWNZ       : 0x%08x\n", mipi_csi_read_reg(index, MIPI_PHY_SHUTDOWNZ));
    printk("DPHY_RSTZ           : 0x%08x\n", mipi_csi_read_reg(index, MIPI_DPHY_RSTZ));
    printk("CSI2_RESETN         : 0x%08x\n", mipi_csi_read_reg(index, MIPI_CSI2_RESETN));
    printk("PHY_STATE           : 0x%08x\n", mipi_csi_read_reg(index, MIPI_PHY_STATE));
    printk("DATA_IDS_1          : 0x%08x\n", mipi_csi_read_reg(index, MIPI_DATA_IDS_1));
    printk("DATA_IDS_2          : 0x%08x\n", mipi_csi_read_reg(index, MIPI_DATA_IDS_2));
    printk("ERR1                : 0x%08x\n", mipi_csi_read_reg(index, MIPI_ERR1));
    printk("ERR2                : 0x%08x\n", mipi_csi_read_reg(index, MIPI_ERR2));
    printk("MASK1               : 0x%08x\n", mipi_csi_read_reg(index, MIPI_MASK1));
    printk("MASK2               : 0x%08x\n", mipi_csi_read_reg(index, MIPI_MASK2));
    printk("PHY_TST_CTRL0       : 0x%08x\n", mipi_csi_read_reg(index, MIPI_PHY_TST_CTRL0));
    printk("PHY_TST_CTRL1       : 0x%08x\n", mipi_csi_read_reg(index, MIPI_PHY_TST_CTRL1));

    printk("VC0_FRAME_NUM       : 0x%08x\n", mipi_csi_read_reg(index, MIPI_VC0_FRAME_NUM));
    printk("VC1_FRAME_NUM       : 0x%08x\n", mipi_csi_read_reg(index, MIPI_VC1_FRAME_NUM));
    printk("VC2_FRAME_NUM       : 0x%08x\n", mipi_csi_read_reg(index, MIPI_VC2_FRAME_NUM));
    printk("VC3_FRAME_NUM       : 0x%08x\n", mipi_csi_read_reg(index, MIPI_VC3_FRAME_NUM));
    printk("VC0_LINE_NUM        : 0x%08x\n", mipi_csi_read_reg(index, MIPI_VC0_LINE_NUM));
    printk("VC1_LINE_NUM        : 0x%08x\n", mipi_csi_read_reg(index, MIPI_VC1_LINE_NUM));
    printk("VC2_LINE_NUM        : 0x%08x\n", mipi_csi_read_reg(index, MIPI_VC2_LINE_NUM));
    printk("VC3_LINE_NUM        : 0x%08x\n", mipi_csi_read_reg(index, MIPI_VC3_LINE_NUM));

    printk("MIPI_PHY_1C2C_MODE  : 0x%08x\n", mipi_phy_read_reg(MIPI_PHY_1C2C_MODE));
}

#ifdef SOC_CAMERA_DEBUG
int dsysfs_mipi_dump_reg(int index, char *buf)
{
    char *p = buf;

    p += sprintf(p, "\t VERSION             : 0x%08x\n", mipi_csi_read_reg(index, MIPI_VERSION));
    p += sprintf(p, "\t N_LANES             : 0x%08x\n", mipi_csi_read_reg(index, MIPI_N_LANES));
    p += sprintf(p, "\t PHY_SHUTDOWNZ       : 0x%08x\n", mipi_csi_read_reg(index, MIPI_PHY_SHUTDOWNZ));
    p += sprintf(p, "\t DPHY_RSTZ           : 0x%08x\n", mipi_csi_read_reg(index, MIPI_DPHY_RSTZ));
    p += sprintf(p, "\t CSI2_RESETN         : 0x%08x\n", mipi_csi_read_reg(index, MIPI_CSI2_RESETN));
    p += sprintf(p, "\t PHY_STATE           : 0x%08x\n", mipi_csi_read_reg(index, MIPI_PHY_STATE));
    p += sprintf(p, "\t DATA_IDS_1          : 0x%08x\n", mipi_csi_read_reg(index, MIPI_DATA_IDS_1));
    p += sprintf(p, "\t DATA_IDS_2          : 0x%08x\n", mipi_csi_read_reg(index, MIPI_DATA_IDS_2));
    p += sprintf(p, "\t ERR1                : 0x%08x\n", mipi_csi_read_reg(index, MIPI_ERR1));
    p += sprintf(p, "\t ERR2                : 0x%08x\n", mipi_csi_read_reg(index, MIPI_ERR2));
    p += sprintf(p, "\t MASK1               : 0x%08x\n", mipi_csi_read_reg(index, MIPI_MASK1));
    p += sprintf(p, "\t MASK2               : 0x%08x\n", mipi_csi_read_reg(index, MIPI_MASK2));
    p += sprintf(p, "\t PHY_TST_CTRL0       : 0x%08x\n", mipi_csi_read_reg(index, MIPI_PHY_TST_CTRL0));
    p += sprintf(p, "\t PHY_TST_CTRL1       : 0x%08x\n", mipi_csi_read_reg(index, MIPI_PHY_TST_CTRL1));

    p += sprintf(p, "\t VC0_FRAME_NUM       : 0x%08x\n", mipi_csi_read_reg(index, MIPI_VC0_FRAME_NUM));
    p += sprintf(p, "\t VC1_FRAME_NUM       : 0x%08x\n", mipi_csi_read_reg(index, MIPI_VC1_FRAME_NUM));
    p += sprintf(p, "\t VC2_FRAME_NUM       : 0x%08x\n", mipi_csi_read_reg(index, MIPI_VC2_FRAME_NUM));
    p += sprintf(p, "\t VC3_FRAME_NUM       : 0x%08x\n", mipi_csi_read_reg(index, MIPI_VC3_FRAME_NUM));
    p += sprintf(p, "\t VC0_LINE_NUM        : 0x%08x\n", mipi_csi_read_reg(index, MIPI_VC0_LINE_NUM));
    p += sprintf(p, "\t VC1_LINE_NUM        : 0x%08x\n", mipi_csi_read_reg(index, MIPI_VC1_LINE_NUM));
    p += sprintf(p, "\t VC2_LINE_NUM        : 0x%08x\n", mipi_csi_read_reg(index, MIPI_VC2_LINE_NUM));
    p += sprintf(p, "\t VC3_LINE_NUM        : 0x%08x\n", mipi_csi_read_reg(index, MIPI_VC3_LINE_NUM));

    p += sprintf(p, "\t MIPI_PHY_1C2C_MODE  : 0x%08x\n", mipi_phy_read_reg(MIPI_PHY_1C2C_MODE));

    return p - buf;
}
#endif

static unsigned char mipi_csi_event_disable(int index, unsigned int  mask, unsigned char err_reg_no)
{
    switch (err_reg_no) {
    case CSI_ERR_MASK_REGISTER1:
        mipi_csi_write_reg(index, MIPI_MASK1, mask | mipi_csi_read_reg(index, MIPI_MASK1));
        break;
    case CSI_ERR_MASK_REGISTER2:
        mipi_csi_write_reg(index, MIPI_MASK2, mask | mipi_csi_read_reg(index, MIPI_MASK2));
        break;
    default:
        return -EINVAL;
    }

    return 0;
}

/*
 * APB = clkcfg_in
 * clk_settle_time(ns) = (clk_settle_count + 4) * cfgclk_in
 *
 * Example:
 * clk_settle_count = clk_settle_time * 150 / 1000 - 4;
 * data_settle_count = data_settle_time * 150 / 1000 - 4;
 */
static void mipi_dphy_phy_settle_time(int index, int lanes, int clk_time_ns, int data_time_ns)
{
    if (clk_time_ns < 95)
        clk_time_ns = 95;

    if (data_time_ns < 85)
        data_time_ns = 85;

    switch (index) {
    case 0:
        mipi_phy_set_clk0_settle(clk_time_ns   * MIPI_DPHY_CLKCFG_IN_MHZ / 1000 - 4);
        mipi_phy_set_data0_settle(data_time_ns * MIPI_DPHY_CLKCFG_IN_MHZ / 1000 - 4);
        mipi_phy_set_data1_settle(data_time_ns * MIPI_DPHY_CLKCFG_IN_MHZ / 1000 - 4);
        if (lanes > 2) {
            mipi_phy_set_data2_settle(data_time_ns * MIPI_DPHY_CLKCFG_IN_MHZ / 1000 - 4);
            mipi_phy_set_data3_settle(data_time_ns * MIPI_DPHY_CLKCFG_IN_MHZ / 1000 - 4);
        }
        break;

    case 1:
        mipi_phy_set_clk1_settle(clk_time_ns   * MIPI_DPHY_CLKCFG_IN_MHZ / 1000 - 4);
        mipi_phy_set_data2_settle(data_time_ns * MIPI_DPHY_CLKCFG_IN_MHZ / 1000 - 4);
        mipi_phy_set_data3_settle(data_time_ns * MIPI_DPHY_CLKCFG_IN_MHZ / 1000 - 4);
        break;
    }
}

static int mipi_csi_gate_clock_enable(int index)
{
    struct jz_mipi_csi_drv *drv = &mipi_csi_dev[index];

    if (!drv->csi_gate_clk) {
        drv->csi_gate_clk = clk_get(NULL, drv->csi_clk_name);
        assert(!IS_ERR(drv->csi_gate_clk));
        assert(!clk_prepare(drv->csi_gate_clk));
    }

    clk_enable(drv->csi_gate_clk);

    return 0;
}

static int mipi_csi_gate_clock_disable(int index)
{
    struct jz_mipi_csi_drv *drv = &mipi_csi_dev[index];

    assert(!IS_ERR(drv->csi_gate_clk));

    clk_disable(drv->csi_gate_clk);

    return 0;
}

static int mipi_csi_phy_ready(int index, int lanes)
{
    int ready;
    int ret = 0;

    ready = mipi_csi_dphy_get_state(index);
    ret = ready & (1 << MIPI_PHY_STATE_CLK_LANE_STOP );

    switch (lanes) {
    case 4:
        ret |= ret && (ready & (1 << MIPI_PHY_STATE_DATA_LANE3));
        ret |= ret && (ready & (1 << MIPI_PHY_STATE_DATA_LANE2));
    case 2:
        ret |= ret && (ready & (1 << MIPI_PHY_STATE_DATA_LANE1));
    case 1:
        ret |= ret && (ready & (1 << MIPI_PHY_STATE_DATA_LANE0));
        break;
    default:
        printk(KERN_ERR "Do not support lane num %d!\n", lanes);
        break;
    }

    return !!ret;
}

static int mipi_csi_phy_configure(int index, struct mipi_csi_bus *mipi_info)
{
    int ret = 0;
    unsigned long flags;
    spin_lock_irqsave(&csi_reset_lock, flags);

    int lanes = mipi_info->lanes;

    /* 参数检查 */
    if (mipi_csi_dev[index].is_enable) {
        printk(KERN_ERR "csi%d is already used by other controller, please check config paramer!\n", index);
        ret = -EINVAL;
        goto mipi_csi_spin_unlock;
    }

    if (index == 1) {
        if (lanes > 2) {
            printk(KERN_ERR "csi%d lane num %d must less the 2!\n", index, lanes);
            ret = -EINVAL;
            goto mipi_csi_spin_unlock;
        }
    }

    if (mipi_csi_dev[0].is_enable || mipi_csi_dev[1].is_enable) {
        goto mipi_csi_init;
    }

    /* 初始化公用控制器 */
    int index0 = 0;
    int index1 = 1;

    mipi_csi_gate_clock_enable(index0);
    mipi_csi_gate_clock_enable(index1);

    if (lanes > 2)
        mipi_phy_set_csi_mode(0);   /* 1 csi host mode */
    else
        mipi_phy_set_csi_mode(1);   /* 2 csi host mode */

    /*
     * Reset PHY CSI
     * CSI0 CSI1 DPHY must be reset at the same time
     */
    mipi_csi_dphy_phy_shutdown(index0,  0);
    mipi_csi_dphy_dphy_reset(index0,    0);
    mipi_csi_dphy_csi2_reset(index0,    0);

    mipi_csi_dphy_phy_shutdown(index1,  0);
    mipi_csi_dphy_dphy_reset(index1,    0);
    mipi_csi_dphy_csi2_reset(index1,    0);

    udelay(100);

    mipi_csi_dphy_phy_shutdown(index0,  1);
    mipi_csi_dphy_dphy_reset(index0,    1);
    mipi_csi_dphy_csi2_reset(index0,    1);

    mipi_csi_dphy_phy_shutdown(index1,  1);
    mipi_csi_dphy_dphy_reset(index1,    1);
    mipi_csi_dphy_csi2_reset(index1,    1);

mipi_csi_init:
    mipi_dphy_phy_settle_time(index, lanes, mipi_info->clk_settle_time, mipi_info->data_settle_time);

    mipi_csi_dphy_test_clear(index, 1);
    udelay(100);

    mipi_csi_dphy_set_lanes(index,  lanes);

    /* MASK all interrupts */
    mipi_csi_event_disable(index, 0xffffffff, CSI_ERR_MASK_REGISTER1);
    mipi_csi_event_disable(index, 0xffffffff, CSI_ERR_MASK_REGISTER2);

    mipi_csi_dev[index].is_enable = 1;

mipi_csi_spin_unlock:
    spin_unlock_irqrestore(&csi_reset_lock, flags);
    return ret;
}

int mipi_csi_phy_stop(int index)
{
    int ret = 0;
    unsigned long flags;

    spin_lock_irqsave(&csi_reset_lock, flags);

    if (!mipi_csi_dev[index].is_enable) {
        printk(KERN_ERR "csi%d is not enabled, please enable first!\n", index);
        ret = -EINVAL;
        goto mipi_csi_spin_unlock;
    }

    int index0 = 0;
    int index1 = 1;

    mipi_csi_dev[index].is_enable = 0;

    if ( (mipi_csi_dev[0].is_enable == 0) && (mipi_csi_dev[1].is_enable == 0)) {
        mipi_csi_dphy_csi2_reset(index0,    0);
        mipi_csi_dphy_csi2_reset(index1,    0);

        mipi_csi_dphy_dphy_reset(index0,    0);
        mipi_csi_dphy_dphy_reset(index1,    0);

        mipi_csi_dphy_phy_shutdown(index0,  0);
        mipi_csi_dphy_phy_shutdown(index1,  0);

        mipi_csi_gate_clock_disable(index0);
        mipi_csi_gate_clock_disable(index1);
    }

mipi_csi_spin_unlock:
    spin_unlock_irqrestore(&csi_reset_lock, flags);

    return ret;
}

int mipi_csi_phy_initialization(int index, struct mipi_csi_bus *mipi_info)
{
    int ret = 0;
    int csi_lanes = mipi_info->lanes;

    ret = mipi_csi_phy_configure(index, mipi_info);
    if  (ret < 0) {
        printk(KERN_ERR "mipi csi%d configure failed\n", index);
        return -EINVAL;
    }

    /*
     * 检查MIPI CLK DATA的状态是否为停止状态
     */
    int i;
    int retries = 30;
    for (i = 0; i < retries; i++) {
        if (mipi_csi_phy_ready(index, csi_lanes))
            break;

        udelay(2);
    }

    if (i >= retries) {
        /* 非错误,待确定是否一定需进入stop状态(LP11), 现以警告的形式给予提示 */
        printk("warning: mipi csi%d clk/data lanes NOT in stop state\n", index);
        printk("VERSION             : 0x%08x\n", mipi_csi_read_reg(index, MIPI_VERSION));
        printk("N_LANES             : 0x%08x\n", mipi_csi_read_reg(index, MIPI_N_LANES));
        printk("PHY_STATE           : 0x%08x\n", mipi_csi_read_reg(index, MIPI_PHY_STATE));
        ret = EIO;
    }

    return ret;
}
