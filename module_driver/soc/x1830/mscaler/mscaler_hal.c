#include "mscaler_regs.h"

static int mscaler_coefficient[512]={
    #include "coefficient_file.txt"
};

/* MCSA_CTRL*/
#define lfb_ncu_mode 0
#define dmain_reset_mode 1
#define axi_stop_mode 2

/* MCSA_SRC_IN FMT*/
#define IN_FMT_NV12 0
#define IN_FMT_NV21 1
#define IN_FMT_TILE 2

/* MCSA_SRC_IN SEL*/
#define FROM_DRAM 0
#define FROM_NCU 1
#define FROM_LFB 2

enum mscaler_channel{
    MSCALER_CHANNEL0         = 0,
    MSCALER_CHANNEL1         = 1,
    MSCALER_CHANNEL2         = 2,
};
#define MSCALER_CHANNEL         MSCALER_CHANNEL0

struct mscaler_reg_struct {
    char *name;
    unsigned int addr;
};

static struct mscaler_reg_struct mscaler_regs_name[] = {
    {"MSCA_CTRL", MSCA_CTRL},
    {"MSCA_CH_EN", MSCA_CH_EN},
    {"MSCA_CH_STAT", MSCA_CH_STAT},
    {"MSCA_IRQ_STAT", MSCA_IRQ_STAT},
    {"MSCA_IRQ_MASK", MSCA_IRQ_MASK},
    {"MSCA_CLR_IRQ", MSCA_CLR_IRQ},
    {"DMA_OUT_ARB", DMA_OUT_ARB},
    {"CLK_GATE_EN", CLK_GATE_EN},
    {"CLK_DIS", CLK_DIS},
    {"GLO_RSZ_COEF_WR", GLO_RSZ_COEF_WR},
    {"MSCA_SRC_IN", MSCA_SRC_IN},
    {"MSCA_SRC_SIZE", MSCA_SRC_SIZE},
    {"MSCA_SRC_Y_ADDR", MSCA_SRC_Y_ADDR},
    {"MSCA_SRC_Y_STRI", MSCA_SRC_Y_STRI},
    {"MSCA_SRC_UV_ADDR", MSCA_SRC_UV_ADDR},
    {"MSCA_SRC_UV_STRI", MSCA_SRC_UV_STRI},
    {"CHx_RSZ_OSIZE", CHx_RSZ_OSIZE(MSCALER_CHANNEL0)},
    {"CHx_RSZ_STEP", CHx_RSZ_STEP(MSCALER_CHANNEL0)},
    {"CHx_Y_HRSZ_COEF_WR", CHx_Y_HRSZ_COEF_WR(MSCALER_CHANNEL0)},
    {"CHx_Y_HRSZ_COEF_RD", CHx_Y_HRSZ_COEF_RD(MSCALER_CHANNEL0)},
    {"CHx_Y_VRSZ_COEF_WR", CHx_Y_VRSZ_COEF_WR(MSCALER_CHANNEL0)},
    {"CHx_Y_VRSZ_COEF_RD", CHx_Y_VRSZ_COEF_RD(MSCALER_CHANNEL0)},
    {"CHx_UV_HRSZ_COEF_WR", CHx_UV_HRSZ_COEF_WR(MSCALER_CHANNEL0)},
    {"CHx_UV_HRSZ_COEF_RD", CHx_UV_HRSZ_COEF_RD(MSCALER_CHANNEL0)},
    {"CHx_UV_VRSZ_COEF_WR", CHx_UV_VRSZ_COEF_WR(MSCALER_CHANNEL0)},
    {"CHx_UV_VRSZ_COEF_RD", CHx_UV_VRSZ_COEF_RD(MSCALER_CHANNEL0)},
    {"CHx_CROP_OPOS", CHx_CROP_OPOS(MSCALER_CHANNEL0)},
    {"CHx_CROP_OSIZE", CHx_CROP_OSIZE(MSCALER_CHANNEL0)},
    {"CHx_FRA_CTRL_LOOP", CHx_FRA_CTRL_LOOP(MSCALER_CHANNEL0)},
    {"CHx_FRA_CTRL_MASK", CHx_FRA_CTRL_MASK(MSCALER_CHANNEL0)},
    {"CHx_MS0_POS", CHx_MS0_POS(MSCALER_CHANNEL0)},
    {"CHx_MS0_SIZE", CHx_MS0_SIZE(MSCALER_CHANNEL0)},
    {"CHx_MS0_VALUE", CHx_MS0_VALUE(MSCALER_CHANNEL0)},
    {"CHx_MS1_POS", CHx_MS1_POS(MSCALER_CHANNEL0)},
    {"CHx_MS1_SIZE", CHx_MS1_SIZE(MSCALER_CHANNEL0)},
    {"CHx_MS1_VALUE", CHx_MS1_VALUE(MSCALER_CHANNEL0)},
    {"CHx_MS2_POS", CHx_MS2_POS(MSCALER_CHANNEL0)},
    {"CHx_MS2_SIZE", CHx_MS2_SIZE(MSCALER_CHANNEL0)},
    {"CHx_MS2_VALUE", CHx_MS2_VALUE(MSCALER_CHANNEL0)},
    {"CHx_MS3_POS", CHx_MS3_POS(MSCALER_CHANNEL0)},
    {"CHx_MS3_SIZE", CHx_MS3_SIZE(MSCALER_CHANNEL0)},
    {"CHx_MS3_VALUE", CHx_MS3_VALUE(MSCALER_CHANNEL0)},
    {"CHx_OUT_FMT", CHx_OUT_FMT(MSCALER_CHANNEL0)},
    {"CHx_DMAOUT_Y_ADDR", CHx_DMAOUT_Y_ADDR(MSCALER_CHANNEL0)},
    {"CHx_Y_ADDR_FIFO_STA", CHx_Y_ADDR_FIFO_STA(MSCALER_CHANNEL0)},
    {"CHx_DMAOUT_Y_LAST_ADDR", CHx_DMAOUT_Y_LAST_ADDR(MSCALER_CHANNEL0)},
    {"CHx_DMAOUT_Y_LAST_STATS_NUM", CHx_DMAOUT_Y_LAST_STATS_NUM(MSCALER_CHANNEL0)},
    {"CHx_Y_LAST_ADDR_FIFO_STA", CHx_Y_LAST_ADDR_FIFO_STA(MSCALER_CHANNEL0)},
    {"CHx_DMAOUT_Y_STRI", CHx_DMAOUT_Y_STRI(MSCALER_CHANNEL0)},
    {"CHx_DMAOUT_UV_ADDR", CHx_DMAOUT_UV_ADDR(MSCALER_CHANNEL0)},
    {"CHx_UV_ADDR_FIFO_STA", CHx_UV_ADDR_FIFO_STA(MSCALER_CHANNEL0)},
    {"CHx_DMAOUT_UV_LAST_ADDR", CHx_DMAOUT_UV_LAST_ADDR(MSCALER_CHANNEL0)},
    {"CHx_DMAOUT_UV_LAST_STATS_NUM", CHx_DMAOUT_UV_LAST_STATS_NUM(MSCALER_CHANNEL0)},
    {"CHx_UV_LAST_ADDR_FIFO_STA", CHx_UV_LAST_ADDR_FIFO_STA(MSCALER_CHANNEL0)},
    {"CHx_DMAOUT_UV_STRI", CHx_DMAOUT_UV_STRI(MSCALER_CHANNEL0)},
    {"CHx_DMAOUT_Y_ADDR_CLR", CHx_DMAOUT_Y_ADDR_CLR(MSCALER_CHANNEL0)},
    {"CHx_DMAOUT_UV_ADDR_CLR", CHx_DMAOUT_UV_ADDR_CLR(MSCALER_CHANNEL0)},
    {"CHx_DMAOUT_Y_LAST_ADDR_CLR", CHx_DMAOUT_Y_LAST_ADDR_CLR(MSCALER_CHANNEL0)},
    {"CHx_DMAOUT_UV_LAST_ADDR_CLR", CHx_DMAOUT_UV_LAST_ADDR_CLR(MSCALER_CHANNEL0)},
    {"CSC_C0_COEF", CSC_C0_COEF},
    {"CSC_C1_COEF", CSC_C1_COEF},
    {"CSC_C2_COEF", CSC_C2_COEF},
    {"CSC_C3_COEF", CSC_C3_COEF},
    {"CSC_C4_COEF", CSC_C4_COEF},
    {"CSC_OFFSET_PARA", CSC_OFFSET_PARA},
    {"CSC_GLO_ALPHA", CSC_GLO_ALPHA},
    {"SYS_PRO_CLK_EN", SYS_PRO_CLK_EN},
    {"CH0_CLK_NUM", CH0_CLK_NUM},
    {"CH1_CLK_NUM", CH1_CLK_NUM},
    {"CH2_CLK_NUM", CH2_CLK_NUM},
};

#define MSCALER_REG_BASE    0xB30B0000

#define MSCALER_ADDR(reg) ((volatile unsigned long *)(MSCALER_REG_BASE + reg))

static inline void mscaler_write_reg(unsigned int reg, unsigned int value)
{
    *MSCALER_ADDR(reg) = value;
}

static inline unsigned int mscaler_read_reg(unsigned int reg)
{
    return *MSCALER_ADDR(reg);
}

static inline void mscaler_set_bits(unsigned int reg, int start, int end, int val)
{
    int mask = 0;
    unsigned int oldv = 0;
    unsigned int new = 0;
    mask = ((1ul << (end - start + 1)) - 1) << start;

    oldv = mscaler_read_reg(reg);
    new = oldv & (~mask);
    new |= val << start;
    mscaler_write_reg(reg, new);
}

static int mscaler_dump_regs(int channel)
{
    int i = 0;
    int num = 0;
    printk("----- dump regs -----\n");
    num = sizeof(mscaler_regs_name) / sizeof(struct mscaler_reg_struct);
    for (i = 0; i < num; i++) {
        printk("_mscaler_reg: %s: \t0x%08x\r\n", mscaler_regs_name[i].name, mscaler_read_reg(mscaler_regs_name[i].addr));
    }
    return 0;
}

static inline unsigned int mscaler_hal_get_irqstate(void)
{
    return mscaler_read_reg(MSCA_IRQ_STAT);
}

static inline unsigned int mscaler_hal_get_irqmask(void)
{
    return mscaler_read_reg(MSCA_IRQ_MASK);
}

static inline void mscaler_hal_clr_irq(unsigned int state)
{
    mscaler_write_reg(MSCA_CLR_IRQ, state);
}

static inline void mscaler_hal_disable_all_clkgate(void)
{
    mscaler_write_reg(CLK_GATE_EN, 0x0);
}

static inline void mscaler_hal_clr_allirq(void)
{
    mscaler_write_reg(MSCA_CLR_IRQ, 0x1ff);
}

static inline void mscaler_hal_clr_allmask(void)
{
    mscaler_write_reg(MSCA_IRQ_MASK, 0x0);
}

static inline void mscaler_hal_dma_done_mode(int depend_axi)
{
    mscaler_set_bits(DMA_OUT_ARB, 7, 7, depend_axi);
}

static inline void mscaler_hal_enable_channel(int channel)
{
    mscaler_set_bits(MSCA_CH_EN, channel, channel, 1);
}

static inline void mscaler_hal_disable_channel(int channel)
{
    mscaler_set_bits(MSCA_CH_EN, channel, channel, 0);
}

static inline void mscaler_hal_stop(void)
{
    mscaler_write_reg(MSCA_CTRL, axi_stop_mode);
}

static inline void mscaler_hal_softreset(void)
{
    mscaler_write_reg(MSCA_CTRL, dmain_reset_mode);
}

static inline void mscaler_hal_src_in(int src_in_sel, int src_in_fmt)
{
    mscaler_write_reg(MSCA_SRC_IN, (src_in_sel << 2) | src_in_fmt);
}

static inline void mscaler_hal_src_size(int srcw, int srch)
{
    mscaler_write_reg(MSCA_SRC_SIZE, (srcw << 16) | srch);
}

static inline void mscaler_hal_src_y_addr(int src_y_addr)
{
    mscaler_write_reg(MSCA_SRC_Y_ADDR,src_y_addr);
}

static inline void mscaler_hal_src_uv_addr(int src_uv_addr)
{
    mscaler_write_reg(MSCA_SRC_UV_ADDR, src_uv_addr);
}

static inline void mscaler_hal_src_y_stride(int src_ystride)
{
    mscaler_write_reg(MSCA_SRC_Y_STRI, src_ystride);
}

static inline void mscaler_hal_src_uv_stride(int src_uvstride)
{
    mscaler_write_reg(MSCA_SRC_UV_STRI, (src_uvstride << 16) | src_uvstride);
}

static inline void mscaler_hal_csc_normal_configure(void)
{
    mscaler_write_reg(CSC_C0_COEF, 0x4ad);
    mscaler_write_reg(CSC_C1_COEF, 0x669);
    mscaler_write_reg(CSC_C2_COEF, 0x193);
    mscaler_write_reg(CSC_C3_COEF, 0x344);
    mscaler_write_reg(CSC_C4_COEF, 0x81a);
    mscaler_write_reg(CSC_OFFSET_PARA, (0x80 << 16) | 0x10);
    mscaler_write_reg(CSC_GLO_ALPHA, 0x80);
}

static void mscaler_hal_csc_offset_parameter(void)
{
    int i = 0;
    for (i = 0; i < 512; i++)
        mscaler_write_reg(GLO_RSZ_COEF_WR, mscaler_coefficient[i]);
}

static inline void mscaler_hal_resize_step(int channel, int srcw, int srch, int dstw, int dsth)
{
    unsigned int step_w, step_h;
    step_w = srcw * 512 / dstw;
    step_h = srch * 512 / dsth;
    if(step_w > 255)
       step_w--;
    if(step_h > 255)
       step_h--;
    mscaler_write_reg(CHx_RSZ_STEP(channel), (step_w << 16) | step_h);
}

static inline void mscaler_hal_resize_osize(int channel,int dstw, int dsth)
{
    mscaler_write_reg(CHx_RSZ_OSIZE(channel), (dstw << 16) | dsth);
}

static inline void mscaler_hal_dst_y_stride(int channel,int dst_ystride)
{
    mscaler_write_reg(CHx_DMAOUT_Y_STRI(channel), dst_ystride);
}

static inline void mscaler_hal_dst_uv_stride(int channel,int dst_uvstride)
{
    mscaler_write_reg(CHx_DMAOUT_UV_STRI(channel), dst_uvstride);
}

static inline unsigned int mscaler_hal_get_chx_y_addr_fifo_status(int channel)
{
     return mscaler_read_reg(CHx_Y_ADDR_FIFO_STA(channel));
}

static inline unsigned int mscaler_hal_judge_chx_y_addr_fifo_full(int channel)
{
    return mscaler_hal_get_chx_y_addr_fifo_status(channel) & CH_ADDR_FIFO_FULL;
}

static inline void mscaler_hal_dst_y_addr(int channel, int dst_y_addr)
{
    mscaler_write_reg(CHx_DMAOUT_Y_ADDR(channel), dst_y_addr);
}

static inline void mscaler_hal_dst_uv_addr(int channel, int dst_uv_addr)
{
    mscaler_write_reg(CHx_DMAOUT_UV_ADDR(channel), dst_uv_addr);
}
static inline void mscaler_hal_chx_framerate_ctrlloop(int channel, int ctrlloop)
{
    mscaler_write_reg( CHx_FRA_CTRL_LOOP(channel), ctrlloop);
}

static inline void mscaler_hal_chx_framerate_ctrlmask(int channel, int ctrlmask)
{
    mscaler_write_reg( CHx_FRA_CTRL_LOOP(channel), ctrlmask);
}

static inline void mscaler_hal_chx_framerate_ctrl(int channel, int ctrl)
{
    mscaler_hal_chx_framerate_ctrlloop(channel, ctrl);//loop num 0-31
    mscaler_hal_chx_framerate_ctrlmask(channel, (1<<ctrl));//loop mask bit  0-31
}

static inline void mscaler_hal_dst_fmt(int channel, int dst_fmt)
{
    mscaler_write_reg(CHx_OUT_FMT(channel), dst_fmt);
}

static inline void mscaler_hal_crop_opos(int channel)
{
    mscaler_write_reg(CHx_CROP_OPOS(channel), 0);
}

static inline void mscaler_hal_crop_osize(int channel,int dstw, int dsth)
{
    mscaler_write_reg(CHx_CROP_OSIZE(channel), (dstw << 16) | dsth);
}

static inline void mscaler_hal_dst_y_addr_clr(int channel)
{
    mscaler_write_reg(CHx_DMAOUT_Y_ADDR_CLR(channel), 1);
}

static inline void mscaler_hal_dst_uv_addr_clr(int channel)
{
    mscaler_write_reg(CHx_DMAOUT_UV_ADDR_CLR(channel), 1);
}

