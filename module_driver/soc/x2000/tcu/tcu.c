
#include <common.h>
#include <bit_field.h>
#include <linux/slab.h>
#include <linux/irq.h>
#include <linux/interrupt.h>
#include <linux/platform_device.h>
#include <linux/clk.h>
#include <linux/proc_fs.h>
#include <linux/seq_file.h>
#include <linux/delay.h>
#include <linux/wait.h>
#include <linux/miscdevice.h>
#include <linux/module.h>
#include <utils/clock.h>
#include "tcu_regs.h"
#include "tcu.h"

#define GPIO_COUNTER_GET_MODE_NUM                       _IOWR('t', 194, unsigned long *)
#define GPIO_COUNTER_GET_MODE_NAME                      _IOWR('t', 195, unsigned long *)
#define GPIO_COUNTER_CONFIG                             _IOWR('t', 196, unsigned long *)
#define GPIO_COUNTER_ENABLE                             _IOWR('t', 197, unsigned int)
#define GPIO_COUNTER_DISABLE                            _IOWR('t', 198, unsigned int)
#define GPIO_COUNTER_GET_COUNT                          _IOWR('t', 199, unsigned long *)
#define GPIO_COUNTER_GET_CAPTURE                        _IOWR('t', 200, unsigned long *)

static struct tcu_chn g_tcu_chn[NR_TCU_CHNS] = {{0}};
static struct ingenic_tcu *tcu;

static char *counter_mode_name[] = {
    "pos_gpio0_up_count",                               // gpio0递增计数
    "pos_gpio1_up_count",                               // gpio1递增计数
    "pos_gpio0_up_count_gpio1_rising_edge_clear",       // gpio0递增计数，gpio1上升沿时清除计数
    "pos_gpio1_up_count_gpio0_rising_edge_clear",       // gpio1递增计数，gpio0上升沿时清除计数
    "pos_gpio1_up_count_gpio0_falling_edge_clear",      // gpio1递增计数，gpio0下降沿时清除计数
    "capture_gpio0_gpio1_srcclk",                   // gpio1作为时钟源对gpio0进行捕获，获得周期和高电平时间
    "capture_gpio1_gpio0_srcclk",                   // gpio0作为时钟源对gpio1进行捕获，获得周期和高电平时间
    "capture_gpio0_ext_srcclk_clk/1",               // 外部时钟的1分频作为时钟源对gpio0进行捕获，获得周期和高电平时间
    "capture_gpio0_ext_srcclk_clk/4",
    "capture_gpio0_ext_srcclk_clk/16",
    "capture_gpio0_ext_srcclk_clk/64",
    "capture_gpio0_ext_srcclk_clk/265",
    "capture_gpio0_ext_srcclk_clk/1024",
    "capture_gpio1_ext_srcclk_clk/1",               // 外部时钟的1分频作为时钟源对gpio1进行捕获，获得周期和高电平时间
    "capture_gpio1_ext_srcclk_clk/4",
    "capture_gpio1_ext_srcclk_clk/16",
    "capture_gpio1_ext_srcclk_clk/64",
    "capture_gpio1_ext_srcclk_clk/265",
    "capture_gpio1_ext_srcclk_clk/1024",
    "quadrature_gpio_bothway_count",                // 正交模式，可进行双向计数，计数方向由gpio0和gpio1的正交结果而决定。
};

#ifdef TCU_DEBUG
void tcu_dump_reg(struct tcu_chn *tcu_chn)
{
    static int count = 0;
    count ++;
    printk(KERN_ERR "\n\n----------------------------------------count N0.%d----------------------------------------------------\n\n",count);
    printk(KERN_ERR "-stop-----addr-%08x-value-%08x-----------\n",(unsigned int)(TCU_TSR),tcu_read_reg(TCU_TSR));
    printk(KERN_ERR "-mask-----addr-%08x-value-%08x-----------\n",(unsigned int)(TCU_TMR),tcu_read_reg(TCU_TMR));
    printk(KERN_ERR "-enable---addr-%08x-value-%08x-----------\n",(unsigned int)(TCU_TER),tcu_read_reg(TCU_TER));
    printk(KERN_ERR "-flag-----addr-%08x-value-%08x-----------\n",(unsigned int)(TCU_TFR),tcu_read_reg(TCU_TFR));
    printk(KERN_ERR "-Control--addr-%08x-value-%08x-----------\n",(unsigned int)(TCU_ADDR(tcu_chn->reg_base + CHN_TCSR)),tcu_read_chn_reg(tcu_chn->index,CHN_TCSR));
    printk(KERN_ERR "-full-----addr-%08x-value-%08x-----------\n",(unsigned int)(TCU_ADDR(tcu_chn->reg_base + CHN_TDFR)),tcu_read_chn_reg(tcu_chn->index,CHN_TDFR));
    printk(KERN_ERR "-half-   -addr-%08x-value-%08x-----------\n",(unsigned int)(TCU_ADDR(tcu_chn->reg_base + CHN_TDHR)),tcu_read_chn_reg(tcu_chn->index,CHN_TDHR));
    printk(KERN_ERR "-TCNT-----addr-%08x-value-%08x-----------\n",(unsigned int)(TCU_ADDR(tcu_chn->reg_base + CHN_TCNT)),tcu_read_chn_reg(tcu_chn->index,CHN_TCNT));
    printk(KERN_ERR "-CAP reg_base-------value-%08x-----------\n",tcu_read_reg(CHN_CAP(tcu_chn->index)));
    printk(KERN_ERR "-CAP_VAL register---value-%08x-----------\n",tcu_read_reg(CHN_CAP_VAL(tcu_chn->index)));
}
#endif

/*Timer control config signal pos and neg*/
static inline void tcu_config_clk_polarity(struct tcu_chn *tcu_chn, enum tcu_clksrc clksrc)
{
    switch(clksrc){
        case TCU_CLKSRC_EXT :
            tcu_clr_pos_neg(tcu_chn, CLK_POS);
            switch(tcu_chn->ext_polarity){
                case CLKSRC_NEG_EN :
                    tcu_set_neg(tcu_chn, CLK_NEG);
                    break;
                case CLKSRC_POS_EN :
                    tcu_set_pos(tcu_chn, CLK_POS);
                    break;
                case CLKSRC_POS_NEG_EN :
                    tcu_set_pos_neg(tcu_chn, CLK_POS, CLK_NEG);
                    break;
                default :
                    break;
            }
            break;
        case TCU_CLKSRC_GPIO0 :
            tcu_clr_pos_neg(tcu_chn, GPIO0_POS);
            switch(tcu_chn->gpio0_polarity){
                case CLKSRC_NEG_EN :
                    tcu_set_neg(tcu_chn, GPIO0_NEG);
                    break;
                case CLKSRC_POS_EN :
                    tcu_set_pos(tcu_chn, GPIO0_POS);
                    break;
                case CLKSRC_POS_NEG_EN :
                    tcu_set_pos_neg(tcu_chn, GPIO0_NEG, GPIO0_POS);
                    break;
                default :
                    break;
            }
            break;
        case TCU_CLKSRC_GPIO1 :
            tcu_clr_pos_neg(tcu_chn, GPIO1_POS);
            switch(tcu_chn->gpio1_polarity){
                case CLKSRC_NEG_EN :
                    tcu_set_neg(tcu_chn, GPIO1_NEG);
                    break;
                case CLKSRC_POS_EN :
                    tcu_set_pos(tcu_chn, GPIO1_POS);
                    break;
                case CLKSRC_POS_NEG_EN :
                    tcu_set_pos_neg(tcu_chn, GPIO1_POS, GPIO1_NEG);
                    break;
                default :
                    break;
            }
            break;
        default :
            break;
    }
}

static inline void tcu_set_shutdown(struct tcu_chn *tcu_chn)
{
    if(tcu_chn->shutdown_mode)
        tcu_write_chn_reg(tcu_chn->index, CHN_TCSR, tcu_read_chn_reg(tcu_chn->index, CHN_TCSR) | (TCU_CONTROL_BIT(15)));
    else
        tcu_write_chn_reg(tcu_chn->index, CHN_TCSR, tcu_read_chn_reg(tcu_chn->index, CHN_TCSR) & ~(TCU_CONTROL_BIT(15)));
}

static inline void tcu_irq_unmask(struct tcu_chn *tcu_chn)
{
    switch (tcu_chn->irq_type) {
        case NULL_IRQ_MODE :
            tcu_full_mask(tcu_chn);
            tcu_half_mask(tcu_chn);
            break;
        case FULL_IRQ_MODE :
            tcu_full_unmask(tcu_chn);
            tcu_half_mask(tcu_chn);
            break;
        case HALF_IRQ_MODE :
            tcu_full_mask(tcu_chn);
            tcu_half_unmask(tcu_chn);
            break;
        case FULL_HALF_IRQ_MODE :
            tcu_full_unmask(tcu_chn);
            tcu_half_unmask(tcu_chn);
            break;
        default:
            break;
    }
}

/*config gate work mode */
static inline void tcu_config_gate_mode(struct tcu_chn *tcu_chn)
{
    unsigned int tcsr;
    if (tcu_chn->gate_pola)
        tcu_write_chn_reg(tcu_chn->index, CHN_TCSR, tcu_read_chn_reg(tcu_chn->index, CHN_TCSR) | (TCU_CONTROL_BIT(14)));
    else
        tcu_write_chn_reg(tcu_chn->index, CHN_TCSR, tcu_read_chn_reg(tcu_chn->index, CHN_TCSR) & ~(TCU_CONTROL_BIT(14)));

    tcsr = tcu_read_chn_reg(tcu_chn->index, CHN_TCSR) & ~(CSR_GATE_MSK);
    tcu_write_chn_reg(tcu_chn->index, CHN_TCSR, tcsr | tcu_chn->gate_sel << 11);
}

/*config direction work mode*/
static inline void tcu_config_direction_mode(struct tcu_chn *tcu_chn)
{
    unsigned int tcsr;
    if(tcu_chn->dir_pola)
        tcu_write_chn_reg(tcu_chn->index, CHN_TCSR, tcu_read_chn_reg(tcu_chn->index, CHN_TCSR) | (TCU_CONTROL_BIT(13)));
    else
        tcu_write_chn_reg(tcu_chn->index, CHN_TCSR, tcu_read_chn_reg(tcu_chn->index, CHN_TCSR) & ~(TCU_CONTROL_BIT(13)));

    tcsr = tcu_read_chn_reg(tcu_chn->index, CHN_TCSR) & ~(CSR_DIR_MSK);
    tcu_write_chn_reg(tcu_chn->index, CHN_TCSR, tcsr | tcu_chn->dir_sel << 8);
}

/*config quadrature work mode*/
static inline void tcu_config_quadrature_mode(struct tcu_chn *tcu_chn)
{
    unsigned int tcsr;

    tcu_chn->gpio0_polarity = CLKSRC_POS_NEG_EN;
    tcu_chn->gpio1_polarity = CLKSRC_POS_NEG_EN;
    tcu_chn->gpio0_en = TCU_CLKSRC_GPIO0;
    tcu_chn->gpio1_en = TCU_CLKSRC_GPIO1;
    tcu_chn->dir_sel = DIR_SEL_GPIO_QUA;// use gpio0 and gpio1 quadrature result with direction signal
    tcu_chn->irq_type = FULL_IRQ_MODE;

    if(tcu_chn->dir_pola)
        tcu_write_chn_reg(tcu_chn->index, CHN_TCSR, tcu_read_chn_reg(tcu_chn->index, CHN_TCSR) | (TCU_CONTROL_BIT(13)));
    else
        tcu_write_chn_reg(tcu_chn->index, CHN_TCSR, tcu_read_chn_reg(tcu_chn->index, CHN_TCSR) & ~(TCU_CONTROL_BIT(13)));

    tcsr = tcu_read_chn_reg(tcu_chn->index, CHN_TCSR) & ~(CSR_DIR_MSK);
    tcu_write_chn_reg(tcu_chn->index, CHN_TCSR, tcsr | tcu_chn->dir_sel << 8);

}

/*config pos work mode*/
static inline void tcu_config_pos_mode(struct tcu_chn *tcu_chn)
{
    unsigned int tcsr;
    if (tcu_chn->pos_clr_sel > 0 && tcu_chn->pos_clr_sel < 4){
        tcsr = tcu_read_reg(CHN_CAP(tcu_chn->index)) & ~(CAP_SEL_MSK);
        tcu_write_reg(CHN_CAP(tcu_chn->index),tcsr | tcu_chn->pos_clr_sel << 16);
    }
}

/*config capture work mode*/
static inline void tcu_config_capture_mode(struct tcu_chn *tcu_chn)
{
    unsigned int tcsr;
    if (tcu_chn->capture_sel >= 0 && tcu_chn->capture_sel < 3){
    tcsr = tcu_read_reg(CHN_CAP(tcu_chn->index)) & ~(CAP_SEL_MSK | CAP_NUM_MSK);
    tcu_write_reg(CHN_CAP(tcu_chn->index),tcsr | tcu_chn->capture_sel << 16 | tcu_chn->capture_num );
    }
}

/*config filter work mode*/
static inline void tcu_config_filter_mode(struct tcu_chn *tcu_chn)
{
    unsigned int tcsr;
    tcsr = tcu_read_reg(CHN_FIL_VAL(tcu_chn->index)) & ~(FIL_VAL_GPIO1_MSK | FIL_VAL_GPIO0_MSK);
    tcu_write_reg(CHN_FIL_VAL(tcu_chn->index),tcsr | tcu_chn->fil_a_num | tcu_chn->fil_b_num << 16 );
}

/*Choose a working mode*/
static inline void tcu_sel_work_mode(struct tcu_chn *tcu_chn)
{
    switch(tcu_chn->mode_sel){
        case GENERAL_MODE :
            break;
        case GATE_MODE :
            tcu_config_gate_mode(tcu_chn);
            break;
        case DIRECTION_MODE :
            tcu_config_direction_mode(tcu_chn);
            break;
        case QUADRATURE_MODE :
            tcu_config_quadrature_mode(tcu_chn);
            break;
        case POS_MODE :
            tcu_config_pos_mode(tcu_chn);
            break;
        case CAPTURE_MODE :
            tcu_config_capture_mode(tcu_chn);
            break;
        case FILTER_MODE :
            tcu_config_filter_mode(tcu_chn);
            break;
        default :
            break;
    }

}

void tcu_clear_irq_flag(struct tcu_chn *tcu_chn)
{
    tcu_clear_full_flag(tcu_chn);
    tcu_clear_half_flag(tcu_chn);
}

static void tcu_set_clk_polarity(struct tcu_chn *tcu_chn)
{
    /* 设置时钟极性 */
    tcu_config_clk_polarity(tcu_chn, tcu_chn->clk_src);
    tcu_config_clk_polarity(tcu_chn, tcu_chn->gpio0_en);
    tcu_config_clk_polarity(tcu_chn, tcu_chn->gpio1_en);
}

static void tcu_enable_clk_function(struct tcu_chn *tcu_chn)
{
    /* 使能时钟ext/gpio0/gpio1 */
    unsigned int tcsr;

    tcsr = tcu_read_chn_reg(tcu_chn->index, CHN_TCSR) & ~(ONE_BIT_OFFSET(2) | ONE_BIT_OFFSET(6) |ONE_BIT_OFFSET(7));
    tcu_write_chn_reg(tcu_chn->index, CHN_TCSR, tcsr | tcu_chn->clk_src | tcu_chn->gpio0_en | tcu_chn->gpio1_en);
}

static inline void tcu_get_prescale(struct tcu_chn *tcu_chn)
{
    switch(tcu_chn->clk_div) {
        case 1: tcu_chn->prescale = TCU_PRESCALE_1;
        break;
        case 4: tcu_chn->prescale = TCU_PRESCALE_4;
        break;
        case 16:tcu_chn->prescale = TCU_PRESCALE_16;
        break;
        case 64:tcu_chn->prescale = TCU_PRESCALE_64;
        break;
        case 256:tcu_chn->prescale = TCU_PRESCALE_256;
        break;
        case 1024:tcu_chn->prescale = TCU_PRESCALE_1024;
        break;
        default:
        break;
    }
}

void tcu_config_chn(struct tcu_chn *tcu_chn)
{
    spin_lock(&tcu_chn->tcu->lock);
    /* Clear IRQ flag */
    tcu_clear_irq_flag(tcu_chn);

    /* clear capture register */
    tcu_clear_capture_reg(tcu_chn);

    /*select work mode*/
    tcu_sel_work_mode(tcu_chn);

    /*full num and half num*/
    tcu_set_chn_full(tcu_chn, tcu_chn->full_num);
    tcu_set_chn_half(tcu_chn, tcu_chn->half_num);

    /*TCNT clear to 0*/
    // tcu_clear_tcnt(tcu_chn);

    /* shutdown mode */
    tcu_set_shutdown(tcu_chn);

    /* prescale */
    tcu_get_prescale(tcu_chn);
    if(!(tcu_read_reg(TCU_TER) & (1 << tcu_chn->index)))
        tcu_set_prescale(tcu_chn, tcu_chn->prescale);

    /*select counter mode*/
    tcu_set_count_mode(tcu_chn, tcu_chn->count_mode);

    /* set clk  */
    tcu_set_clk_polarity(tcu_chn);
    tcu_enable_clk_function(tcu_chn);

    spin_unlock(&tcu_chn->tcu->lock);
}

static void tcu_irq_ack(void)
{
    unsigned long flags;
    int id ;
    for(id = 0; id < NR_TCU_CHNS; id++){
        if(tcu_get_full_flag(&g_tcu_chn[id]) == 0)
            continue;
        spin_lock_irqsave(&g_tcu_chn[id].tcu->lock, flags);
        switch (g_tcu_chn[id].irq_type) {
            case FULL_IRQ_MODE :
                tcu_clear_full_flag(&g_tcu_chn[id]);
                break;
            case HALF_IRQ_MODE :
                tcu_clear_half_flag(&g_tcu_chn[id]);
                break;
            case FULL_HALF_IRQ_MODE :
                tcu_clear_full_flag(&g_tcu_chn[id]);
                tcu_clear_half_flag(&g_tcu_chn[id]);
                break;
            default:
                break;
        }
        spin_unlock_irqrestore(&g_tcu_chn[id].tcu->lock, flags);

        if (g_tcu_chn[id].mode_sel == CAPTURE_MODE) {
            g_tcu_chn[id].capture_flag = 1;
            wake_up_all(&g_tcu_chn[id].capture_waiter);
        }
    }
}

static irqreturn_t tcu_interrupt(int irq, void *dev_id)
{
    if(tcu_read_reg(TCU_TFR) & TCU_FLAG_RD){

    }else{	// TCU半/满阈值中断
        tcu_irq_ack();
    }
    return IRQ_HANDLED;
}

static int tcu_get_counter_mode_num(void)
{
    return ARRAY_SIZE(counter_mode_name);
}

static void tcu_capture_mode_analy(struct tcu_chn *tcu_chn, char *name)
{
    char *str = NULL;

    tcu_chn->capture_num = 0xa0;
    if (strstr(name, "gpio1_srcclk")) {
        /* 将gpio1作为输入时钟源 */
        tcu_chn->clk_src = TCU_CLKSRC_GPIO1;
        tcu_chn->gpio1_polarity = CLKSRC_POS_EN;
    }
    if (strstr(name, "gpio0_srcclk")) {
        tcu_chn->clk_src = TCU_CLKSRC_GPIO0;
        tcu_chn->gpio0_polarity = CLKSRC_POS_EN;
    }
    if (strstr(name, "ext_srcclk")) {
        tcu_chn->clk_src = TCU_CLKSRC_EXT;
        tcu_chn->ext_polarity = CLKSRC_POS_EN;

        /* 得到ext分频值*/
        str = strstr(name, "_clk/");
        str += strlen("_clk/");
        kstrtoint(str, 10, &tcu_chn->clk_div);
    }

    if (strstr(name, "capture_gpio0")) {
        /* 对gpio0进行捕获,获得周期以及高电平时间 */
        tcu_chn->capture_sel = CAPTURE_GPIO0;
        tcu_chn->gpio0_en = TCU_CLKSRC_GPIO0;
    }
    if (strstr(name, "capture_gpio1")) {
        tcu_chn->capture_sel = CAPTURE_GPIO1;
        tcu_chn->gpio1_en = TCU_CLKSRC_GPIO1;
    }
}

static void tcu_pos_mode_analy(struct tcu_chn *tcu_chn, char *name)
{
    if (strstr(name, "gpio0_up_count")) {
        /* 将gpio0作为输入时钟源,在上升沿的时候计数器计数 */
        tcu_chn->clk_src = TCU_CLKSRC_GPIO0;
        tcu_chn->gpio0_polarity = CLKSRC_POS_EN;
    }
    if (strstr(name, "gpio1_up_count")) {
        tcu_chn->clk_src = TCU_CLKSRC_GPIO1;
        tcu_chn->gpio1_polarity = CLKSRC_POS_EN;
    }
    if (strstr(name, "gpio0_rising_edge_clear")) {
        /* 将GPIO0设为pos模式的清零信号，在上升沿的时候清零计数值 */
        tcu_chn->pos_clr_sel = GPIO0_POS_CLR;
        tcu_chn->gpio0_en = TCU_CLKSRC_GPIO0;
    }
    if (strstr(name, "gpio1_rising_edge_clear")) {
        tcu_chn->pos_clr_sel = GPIO1_POS_CLR;
        tcu_chn->gpio1_en = TCU_CLKSRC_GPIO1;
    }
    if (strstr(name, "gpio0_falling_edge_clear")) {
        tcu_chn->pos_clr_sel = GPIO0_NEG_CLR;
        tcu_chn->gpio0_en = TCU_CLKSRC_GPIO0;
    }
}

static int get_counter_mode(struct tcu_chn *tcu_chn, char *name)
{
    int i;
    int num = tcu_get_counter_mode_num();

    for (i = 0; i < num; i++) {
        if (strcmp(counter_mode_name[i], name) == 0)
            break;
    }
    if (i == num)
        return -1;

    if (strstr(name, "capture")) {
        tcu_chn->mode_sel = CAPTURE_MODE;
        return 0;
    }
    if (strstr(name, "pos")) {
        tcu_chn->mode_sel = POS_MODE;
        return 0;
    }
    if (strstr(name, "quadrature")) {
        /* 正交模式 */
        tcu_chn->mode_sel = QUADRATURE_MODE;
        return 0;
    }

    return -1;
}

static int tcu_config_mode(struct tcu_chn *tcu_chn, char *mode_name)
{
    int ret;

    ret = get_counter_mode(tcu_chn, mode_name);
    if (ret < 0) {
        printk(KERN_ERR "TCU: config channel:%d failre\n", tcu_chn->index);
        return ret;
    }
    strncpy(tcu_chn->mode_name, mode_name, MODE_NAME_LEN);

    tcu_chn->irq_type = FULL_IRQ_MODE;
    tcu_chn->full_num = TCU_FULL_NUM;
    tcu_chn->half_num = TCU_HALF_NUM;
    tcu_chn->shutdown_mode = 0;

    /*In order to facilitate testing,
     * it has no practical significance.*/
    switch(tcu_chn->mode_sel){
        case GENERAL_MODE:
            /*Enable external clock to use rising edge counting , result TCNT != 0*/
            tcu_chn->clk_src = TCU_CLKSRC_EXT;
            tcu_chn->ext_polarity = CLKSRC_POS_EN;
            break;
        case GATE_MODE:
            /*gate signal hold on 0,counter start when control signal is 1,result TCNT == 0*/
            tcu_chn->gate_sel = GATE_SEL_HZ;
            tcu_chn->gate_pola = GATE_POLA_HIGH;
            tcu_chn->clk_src = TCU_CLKSRC_EXT;
            tcu_chn->ext_polarity = CLKSRC_POS_EN;
            break;
        case DIRECTION_MODE:
            /*use gpio0 with direction signa. counter sub when control signal is 1.result TCNT add and sub */
            tcu_chn->gpio0_en = TCU_CLKSRC_GPIO0;
            tcu_chn->dir_sel = DIR_SEL_GPIO0;
            tcu_chn->dir_pola = DIR_POLA_HIGH;
            tcu_chn->clk_src = TCU_CLKSRC_EXT;
            tcu_chn->ext_polarity = CLKSRC_POS_EN;
            break;
        case QUADRATURE_MODE:
            break;
        case POS_MODE:
            tcu_pos_mode_analy(tcu_chn, tcu_chn->mode_name);
            break;
        case CAPTURE_MODE:
            tcu_capture_mode_analy(tcu_chn, tcu_chn->mode_name);
            break;
        case FILTER_MODE:
            /*for easy test set max*/
            tcu_chn->gpio0_en = TCU_CLKSRC_GPIO0;
            tcu_chn->gpio0_polarity = CLKSRC_POS_EN;
            tcu_chn->fil_a_num = 0x3ff;
            tcu_chn->fil_b_num = 0x3ff;
            break;
        default:
             break;
    }

    tcu_config_chn(tcu_chn);

#ifdef TCU_DEBUG
        tcu_dump_reg(tcu_chn);
#endif
    return 0;
}

/*Timer Counter Enable/Disable Register*/
static inline void tcu_set_enable(struct tcu_chn *tcu_chn)
{
    spin_lock(&tcu_chn->tcu->lock);
    tcu_write_reg(TCU_TESR, 1 << tcu_chn->index);
    spin_unlock(&tcu_chn->tcu->lock);
}

static inline void tcu_set_disable(struct tcu_chn *tcu_chn)
{
    spin_lock(&tcu_chn->tcu->lock);
    tcu_write_reg(TCU_TECR, 1 << tcu_chn->index);
    spin_unlock(&tcu_chn->tcu->lock);
}

static int tcu_gpio_requset(int gpio, char *name)
{
    int ret = gpio_request(gpio, name);
    gpio_set_func(gpio, GPIO_FUNC_0);
    return ret;
}

static int tcu_gpio_init(struct tcu_chn *tcu_chn)
{
    int ret = 0;

    if (tcu_chn->gpio0_en == TCU_CLKSRC_GPIO0 || tcu_chn->clk_src == TCU_CLKSRC_GPIO0)
        ret |= tcu_gpio_requset(tcu_chn->gpio0, "tcu_clksrc_gpio0");
    if (tcu_chn->gpio1_en == TCU_CLKSRC_GPIO1 || tcu_chn->clk_src == TCU_CLKSRC_GPIO1)
	    ret |= tcu_gpio_requset(tcu_chn->gpio1, "tcu_clksrc_gpio1");

    return ret;
}

static int tcu_gpio_deinit(struct tcu_chn *tcu_chn)
{
    int ret = 0;

    if (tcu_chn->gpio0_en == TCU_CLKSRC_GPIO0 || tcu_chn->clk_src == TCU_CLKSRC_GPIO0)
        gpio_free(tcu_chn->gpio0);
    if (tcu_chn->gpio1_en == TCU_CLKSRC_GPIO1 || tcu_chn->clk_src == TCU_CLKSRC_GPIO1)
        gpio_free(tcu_chn->gpio1);

    return ret;
}

static void tcu_set_default_config(unsigned int id)
{
    g_tcu_chn[id].index = id;
    g_tcu_chn[id].capture_num = 0;
    g_tcu_chn[id].fil_a_num	 = 0;
    g_tcu_chn[id].fil_b_num	 = 0;
    g_tcu_chn[id].clk_div = 0;
    g_tcu_chn[id].pos_clr_sel = 0;
    g_tcu_chn[id].gpio0	 = GPIO_PC(id*2);
    g_tcu_chn[id].gpio1	 = GPIO_PC(id*2 + 1);
    g_tcu_chn[id].reg_base	 = TCU_FULL0 + id * TCU_CHN_OFFSET;
    g_tcu_chn[id].irq_type	 = NULL_IRQ_MODE;
    g_tcu_chn[id].clk_src	 = TCU_CLKSRC_NULL;
    g_tcu_chn[id].gpio0_en	 = TCU_CLKSRC_NULL;
    g_tcu_chn[id].gpio1_en	 = TCU_CLKSRC_NULL;
    g_tcu_chn[id].prescale	 = TCU_PRESCALE_1;
    g_tcu_chn[id].shutdown_mode = 0;
    g_tcu_chn[id].count_mode	 = COUNT_MODE_FCZ;
    g_tcu_chn[id].mode_sel	 = GENERAL_MODE;
    g_tcu_chn[id].gate_pola	 = GATE_POLA_LOW;
    g_tcu_chn[id].gate_sel	 = GATE_SEL_HZ;
    g_tcu_chn[id].dir_sel	 = DIR_SEL_HH;
    g_tcu_chn[id].dir_pola	 = DIR_POLA_LOW;
    g_tcu_chn[id].enable_flag	 = 0;
    g_tcu_chn[id].tcu	 	 = tcu;
}

static int tcu_config(unsigned int channel_id, char *mode_name)
{
    int ret = 0;
    if(channel_id < 0 || channel_id > TCU_CHANNEL_NUM) {
        printk(KERN_ERR "TCU: Please select the correct channel 0 ~ 7 range.\n");
        return -1;
    }

    tcu_set_default_config(channel_id); // 恢复默认配置.
    tcu_gpio_deinit(&g_tcu_chn[channel_id]);// 释放gpio

    ret = tcu_config_mode(&g_tcu_chn[channel_id], mode_name);
    tcu_gpio_init(&g_tcu_chn[channel_id]);
    g_tcu_chn[channel_id].config_flag = 1;

    return ret;
}

static int tcu_enable(unsigned int channel_id)
{
    if(channel_id >= 0 && channel_id < 8){
        if(g_tcu_chn[channel_id].config_flag == 0){
            printk(KERN_ERR "TCU:channel %d not config\n",channel_id);
            return -1;
        }

        if(g_tcu_chn[channel_id].enable_flag){
            printk(KERN_ERR "TCU:channel %d already enable \n",channel_id);
            return -1;
        }

        tcu_irq_unmask(&g_tcu_chn[channel_id]);
        g_tcu_chn[channel_id].enable_flag = 1;
        if (g_tcu_chn[channel_id].mode_sel != CAPTURE_MODE) {
            tcu_set_enable(&g_tcu_chn[channel_id]);
            tcu_start_counter(&g_tcu_chn[channel_id]);
        }

#ifdef TCU_DEBUG
        tcu_dump_reg(&g_tcu_chn[channel_id]);
#endif
    } else {
        printk(KERN_ERR "TCU:Please select the correct channel 0 ~ 7 range.\n");
    }

    return 0;
}

static int tcu_disable(unsigned int channel_id)
{
    if(channel_id >= 0 && channel_id < 8){
        if(!g_tcu_chn[channel_id].config_flag){
            printk(KERN_ERR "TCU:channel %d not config\n",channel_id);
            return -1;
        }

        if(!g_tcu_chn[channel_id].enable_flag){
            printk(KERN_ERR "TCU:channel %d not enable \n",channel_id);
            return -1;
        }

        if (g_tcu_chn[channel_id].mode_sel != CAPTURE_MODE) {
            tcu_set_disable(&g_tcu_chn[channel_id]);
            tcu_clear_tcnt(&g_tcu_chn[channel_id]); //先清零后停止,不然先停止再清零就清不了
            tcu_stop_counter(&g_tcu_chn[channel_id]);
        }

        tcu_full_mask(&g_tcu_chn[channel_id]);
        tcu_half_mask(&g_tcu_chn[channel_id]);
        g_tcu_chn[channel_id].enable_flag = 0;
#ifdef TCU_DEBUG
        tcu_dump_reg(&g_tcu_chn[channel_id]);
#endif
    }else {
        printk(KERN_ERR "TCU:Please select the correct channel 0 ~ 7 range.\n");
    }

    return 0;
}

static unsigned int tcu_get_period_high_level(unsigned int channel_id, int *high_level_time, int *period_time)
{
    *high_level_time = tcu_get_bit(CHN_CAP_VAL(channel_id), 0, 15);
    *period_time = tcu_get_bit(CHN_CAP_VAL(channel_id), 16, 31);
    return 0;
}

static unsigned int tcu_wait_capture(unsigned int channel_id, int *high_level_time, int *period_time)
{
    struct tcu_chn* tcu_chn = &g_tcu_chn[channel_id];
    if (!tcu_chn->enable_flag) {
        printk(KERN_ERR "TCU:Please enable channel before getting a count\n");
        return -1;
    }

    if (!tcu_chn->capture_flag)
        wait_event_interruptible_timeout(tcu_chn->capture_waiter,
                                tcu_chn->capture_flag, msecs_to_jiffies(3000));

    tcu_get_period_high_level(channel_id, high_level_time, period_time);
    tcu_chn->capture_flag = 0;

    return 0;
}

static unsigned int tcu_get_count(unsigned int channel_id, int *count)
{
    if (!g_tcu_chn[channel_id].enable_flag) {
        printk(KERN_ERR "TCU:Please enable channel before getting a count\n");
        return -1;
    }

    *count = tcu_read_chn_reg(g_tcu_chn[channel_id].index, CHN_TCNT);
    return 0;
}

static int tcu_get_counter_mode_name(char *buf)
{
    int id;
    int length = MODE_NAME_LEN;
    int num = tcu_get_counter_mode_num();

    for(id = 0; id < num; id++)
        strncpy(buf + id*length, counter_mode_name[id], length);

    return 0;
}

static long tcu_ioctl(struct file *filp, unsigned int cmd, unsigned long arg)
{
    int ret = 0;
    int *count;
    int channel_id;
    int *num, *size;
    int *high_level_time;
    int *period_time;

    unsigned long *argv = (void *)arg;

    switch (cmd) {
    case GPIO_COUNTER_CONFIG:
        channel_id = argv[0];
        char *mode_name = (char *)argv[1];
        ret = tcu_config(channel_id, mode_name);
        break;
    case GPIO_COUNTER_ENABLE:
        channel_id = arg;
        ret = tcu_enable(channel_id);
        break;
    case GPIO_COUNTER_DISABLE:
        channel_id = arg;
        ret = tcu_disable(channel_id);
        break;
    case GPIO_COUNTER_GET_CAPTURE:
        channel_id = argv[0];
        high_level_time = (int *)argv[1];
        period_time = (int *)argv[2];
        ret = tcu_wait_capture(channel_id, high_level_time, period_time);
        break;
    case GPIO_COUNTER_GET_COUNT:
        channel_id = argv[0];
        count = (int *)argv[1];
        ret = tcu_get_count(channel_id, count);
        break;
    case GPIO_COUNTER_GET_MODE_NUM:
        num = (int *)argv[0];
        size = (int *)argv[1];
        *num = tcu_get_counter_mode_num();
        *size = MODE_NAME_LEN;
        return ret;
    case GPIO_COUNTER_GET_MODE_NAME:
        ret = tcu_get_counter_mode_name((char *)arg);
        break;
    default:
        ret = -1;
        printk(KERN_ERR "TCU:no support your cmd:%x\n", cmd);
    }
    return ret;
}

static int tcu_open(struct inode *inode, struct file *filp)
{
    return 0;
}

static int tcu_release(struct inode *inode, struct file *filp)
{
    return 0;
}

static struct file_operations tcu_misc_fops = {
    .open = tcu_open,
    .release = tcu_release,
    .unlocked_ioctl = tcu_ioctl,
};

struct miscdevice tcu_mdevice = {
    .minor  = MISC_DYNAMIC_MINOR,
    .name   = "jz_tcu",
    .fops   = &tcu_misc_fops,
};

static int jz_tcu_init(void)
{
    int  i, ret = 0;

    tcu = kmalloc(sizeof(struct ingenic_tcu), GFP_KERNEL);
    if (!tcu) {
        printk(KERN_ERR "TCU:Failed to allocate driver struct\n");
        return -ENOMEM;
    }

    tcu->irq = IRQ_TCU0;
    ret = request_irq(tcu->irq, tcu_interrupt,
            IRQF_SHARED | IRQF_TRIGGER_LOW,"tcu-interrupt",tcu);
    if (ret) {
        printk(KERN_ERR "TCU:request_irq failed !! %d-\n",tcu->irq);
        return -ENOMEM;
    }

    spin_lock_init(&tcu->lock);
    tcu->clk = clk_get(NULL, "gate_tcu");
    clk_prepare_enable(tcu->clk);

    for (i = 0; i < NR_TCU_CHNS; i++) {
        tcu_set_default_config(i);
        init_waitqueue_head(&g_tcu_chn[i].capture_waiter);
    }

    ret = misc_register(&tcu_mdevice);
    if (ret < 0)
        panic(KERN_ERR "TCU: %s, tcu register misc dev error !\n", __func__);

    return 0;
}
module_init(jz_tcu_init);

static void jz_tcu_exit(void)
{
    misc_deregister(&tcu_mdevice);
    clk_disable_unprepare(tcu->clk);
    free_irq(tcu->irq,tcu);
}
module_exit(jz_tcu_exit);

MODULE_DESCRIPTION("Ingenic Soc TCU driver");
MODULE_LICENSE("GPL");