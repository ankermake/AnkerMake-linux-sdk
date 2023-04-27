#ifndef _SOC_TCU_H_
#define _SOC_TCU_H_

#include "tcu_regs.h"

#define MODE_NAME_LEN 64
#define TCU_CHANNEL_NUM   7
#define TCU_FULL_NUM 0xffff
#define TCU_HALF_NUM 0x7000

enum tcu_prescale {
    TCU_PRESCALE_1,
    TCU_PRESCALE_4,
    TCU_PRESCALE_16,
    TCU_PRESCALE_64,
    TCU_PRESCALE_256,
    TCU_PRESCALE_1024
};

enum tcu_irq_type {
    NULL_IRQ_MODE,
    FULL_IRQ_MODE,
    HALF_IRQ_MODE,
    FULL_HALF_IRQ_MODE,
};

enum tcu_clksrc {
    TCU_CLKSRC_NULL,
    TCU_CLKSRC_EXT = ONE_BIT_OFFSET(2),
    TCU_CLKSRC_GPIO0 = ONE_BIT_OFFSET(6),
    TCU_CLKSRC_GPIO1 = ONE_BIT_OFFSET(7)
};

enum tcu_count_mode{
    COUNT_MODE_FCZ,
    COUNT_MODE_MCZ,
    COUNT_MODE_MH
};

enum tcu_gate_sel{
    GATE_SEL_HZ,
    GATE_SEL_CLK,
    GATE_SEL_GPIO0,
    GATE_SEL_GPIO1
};

enum tcu_gate_pola{
    GATE_POLA_LOW,
    GATE_POLA_HIGH
};

enum tcu_dir_sel{
    DIR_SEL_HH,
    DIR_SEL_CLK,
    DIR_SEL_GPIO0,
    DIR_SEL_GPIO1,
    DIR_SEL_GPIO_QUA
};

enum tcu_dir_pola{
    DIR_POLA_LOW,
    DIR_POLA_HIGH
};

enum tcu_mode_sel{
    GENERAL_MODE,
    GATE_MODE,
    DIRECTION_MODE,
    QUADRATURE_MODE,
    POS_MODE,
    CAPTURE_MODE,
    FILTER_MODE
};

enum tcu_pos_sel{
    GPIO0_POS_CLR = 1,
    GPIO1_POS_CLR,
    GPIO0_NEG_CLR
};

enum tcu_capture_sel{
    CAPTURE_CLK,
    CAPTURE_GPIO0,
    CAPTURE_GPIO1
};

enum clk_polarity{
    CLKSRC_INIT,
    CLKSRC_NEG_EN,
    CLKSRC_POS_EN,
    CLKSRC_POS_NEG_EN
};

struct ingenic_tcu {
    struct resource *res;
    struct clk *clk;
    int irq;
    int irq_base;
    spinlock_t lock;
};

struct tcu_chn {
    int index;			/* Channel number */
    int enable_flag;
    int config_flag;
    int gpio0, gpio1;
    int capture_num;
    int capture_flag;
    int shutdown_mode;
    unsigned int  reg_base;
    int half_num, full_num;
    int fil_a_num, fil_b_num;
    char mode_name[MODE_NAME_LEN];
    wait_queue_head_t capture_waiter;
    unsigned int clk_div;  /* 1/4/16/64/256/1024/mask  write to register for  0/1/2/3/4/5/*/

    struct ingenic_tcu *tcu;
    enum tcu_irq_type irq_type;
    enum tcu_clksrc clk_src, gpio0_en, gpio1_en;
    enum tcu_prescale prescale;
    enum tcu_count_mode count_mode;
    enum tcu_gate_sel gate_sel;
    enum tcu_gate_pola gate_pola;
    enum tcu_dir_sel dir_sel;
    enum tcu_dir_pola dir_pola;
    enum tcu_pos_sel pos_clr_sel;
    enum tcu_capture_sel capture_sel;
    enum tcu_mode_sel mode_sel;
    enum clk_polarity ext_polarity, gpio0_polarity, gpio1_polarity;
};

struct tcu_mode_info{
    int num;
    int id;
    int name_size;
    char *name;
};

struct tcu_channel_info {
    int id;
    long long count;
    struct tcu_mode_info mode_info;
};

void tcu_write_reg(unsigned int reg, int val)
{
    *TCU_ADDR(reg) = val;
}

unsigned int tcu_read_reg(unsigned int reg)
{
    return *TCU_ADDR(reg);
}

void tcu_write_chn_reg(unsigned int channel, unsigned int reg, int val)
{
    *TCU_ADDR(0x40 + channel * 0x10 + reg) = val;
}

unsigned int tcu_read_chn_reg(unsigned int channel, unsigned int reg)
{
    return *TCU_ADDR(0x40 + channel * 0x10 + reg);
}

static inline unsigned int tcu_get_bit(unsigned int reg, int start, int end)
{
    return get_bit_field(TCU_ADDR(reg), start, end);
}

static inline void tcu_enable_counter(struct tcu_chn *tcu_chn)
{
    tcu_write_reg(TCU_TESR, 1 << tcu_chn->index);
}

static inline int tcu_disable_counter(struct tcu_chn *tcu_chn)
{
    tcu_write_reg(TCU_TECR, 1 << tcu_chn->index);
    return 1;
}

static inline void tcu_start_counter(struct tcu_chn *tcu_chn)
{
    tcu_write_reg(TCU_TSCR, 1 << tcu_chn->index);
}

static inline void tcu_stop_counter(struct tcu_chn *tcu_chn)
{
    tcu_write_reg(TCU_TSSR, 1 << tcu_chn->index);
}

static inline void tcu_set_count(struct tcu_chn *tcu_chn, u16 cnt)
{
    tcu_write_chn_reg(tcu_chn->index, CHN_TCNT, cnt);
}

static inline int tcu_get_cap_val(struct tcu_chn *tcu_chn)
{
    return tcu_read_reg(CHN_CAP_VAL(tcu_chn->index));
}

static inline void tcu_clear_capture_reg(struct tcu_chn *tcu_chn)
{
    tcu_write_reg(CHN_CAP(tcu_chn->index), 0);
}

/*Timer Mast Register set/clr operation and flag clr operation (full)*/
static inline void tcu_full_mask(struct tcu_chn *tcu_chn)
{
    tcu_write_reg(TCU_TMSR, 1 << tcu_chn->index);
}

static inline void tcu_full_unmask(struct tcu_chn *tcu_chn)
{
    tcu_write_reg(TCU_TMCR, 1 << tcu_chn->index);
}

static inline void tcu_clear_full_flag(struct tcu_chn *tcu_chn)
{
    tcu_write_reg(TCU_TFCR, 1 << tcu_chn->index);
}

static inline int tcu_get_full_flag(struct tcu_chn *tcu_chn)
{
    return tcu_get_bit(TCU_TFR, tcu_chn->index, tcu_chn->index);
}

/*Timer Mast Register set/clr operation and flag clr operation (half)*/
static inline void tcu_half_mask(struct tcu_chn *tcu_chn)
{
    tcu_write_reg(TCU_TMSR, 1 << (tcu_chn->index + 16));
}

static inline void tcu_half_unmask(struct tcu_chn *tcu_chn)
{
    tcu_write_reg(TCU_TMCR, 1 << (tcu_chn->index + 16));
}

static inline void tcu_clear_half_flag(struct tcu_chn *tcu_chn)
{
    tcu_write_reg(TCU_TFCR, 1 << (tcu_chn->index + 16));
}

/*Timer Control Register select the TCNT count clock frequency*/
static inline void tcu_set_prescale(struct tcu_chn *tcu_chn, enum tcu_prescale prescale)
{
    unsigned int tcsr = tcu_read_chn_reg(tcu_chn->index, CHN_TCSR) & ~CSR_DIV_MSK;
    tcu_write_chn_reg(tcu_chn->index, CHN_TCSR, tcsr | (prescale << 3));
}

/*Timer Counter clear to zero*/
static inline void tcu_clear_tcnt(struct tcu_chn *tcu_chn)
{
    tcu_write_chn_reg(tcu_chn->index, CHN_TCNT, 0);
}

/*Timer Counter clear to zero*/
static inline void tcu_clear_control(struct tcu_chn *tcu_chn)
{
    tcu_write_chn_reg(tcu_chn->index, CHN_TCSR, 0);
}

/*Timer Data FULL/HALF Register*/
static inline void tcu_set_chn_full(struct tcu_chn *tcu_chn, unsigned int value)
{
    tcu_write_chn_reg(tcu_chn->index, CHN_TDFR, value);
}

static inline void tcu_set_chn_half(struct tcu_chn *tcu_chn, unsigned int value)
{
    tcu_write_chn_reg(tcu_chn->index, CHN_TDHR, value);
}

/*Timer Control Register set count mode */
static inline void tcu_set_count_mode(struct tcu_chn *tcu_chn, enum tcu_count_mode count_mode)
{
    unsigned int tcsr = tcu_read_chn_reg(tcu_chn->index, CHN_TCSR) & ~(CSR_CM_MSK);
    tcu_write_chn_reg(tcu_chn->index, CHN_TCSR, tcsr | count_mode << 22);
}

/*Timer control set  pos and neg*/
static inline void tcu_set_pos(struct tcu_chn *tcu_chn,unsigned int offset)
{
    unsigned int tcsr;
    if (offset > 15 && offset < 22){
        tcsr = tcu_read_chn_reg(tcu_chn->index, CHN_TCSR) | TCU_CONTROL_BIT(offset);
        tcu_write_chn_reg(tcu_chn->index, CHN_TCSR, tcsr);
    }
}

static inline void tcu_set_neg(struct tcu_chn *tcu_chn,unsigned int offset)
{
    unsigned int tcsr;
    if (offset > 15 && offset < 22){
        tcsr = tcu_read_chn_reg(tcu_chn->index, CHN_TCSR) | TCU_CONTROL_BIT(offset);
        tcu_write_chn_reg(tcu_chn->index, CHN_TCSR, tcsr);
    }

}

static inline void tcu_set_pos_neg(struct tcu_chn *tcu_chn,unsigned int offset,unsigned int offset1)
{
    tcu_set_pos(tcu_chn,offset);
    tcu_set_neg(tcu_chn,offset1);
}

static inline void tcu_clr_pos_neg(struct tcu_chn *tcu_chn,unsigned int bit)
{
    unsigned int tcsr = tcu_read_chn_reg(tcu_chn->index, CHN_TCSR) & ~(TCU_CONTROL_BIT(bit) | TCU_CONTROL_BIT((bit + 1)));
    tcu_write_chn_reg(tcu_chn->index, CHN_TCSR, tcsr);
}

#endif /* _SOC_TCU_H_ */