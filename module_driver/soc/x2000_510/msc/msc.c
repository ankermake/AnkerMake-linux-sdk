#include <linux/version.h>
#include <linux/delay.h>
#include <linux/dma-mapping.h>
#include <linux/platform_device.h>
#include <linux/slab.h>
#include <linux/clk.h>
#include <linux/io.h>
#include <linux/module.h>
#include <linux/of.h>
#include <linux/of_gpio.h>
#include <linux/of_address.h>
#include <linux/pm.h>
#include <linux/pm_runtime.h>
#include <linux/mmc/host.h>
#include <soc/cpm.h>
#include <linux/mmc/core.h>
#include <linux/mmc/slot-gpio.h>
#include <soc/gpio.h>
#include <linux/gpio.h>
#include <utils/gpio.h>
#include <dt-bindings/interrupt-controller/x2000-irq.h>
#include "msc.h"
#include "sdhci.h"

#define CLK_CTRL
/* Software redefinition caps */
#define CAPABILITIES1_SW    0x276dc898
#define CAPABILITIES2_SW    0
#define MSC0_IOBASE 0x13450000 /* 64KB, MMC SD Controller0 */
#define MSC1_IOBASE 0x13460000 /* 64KB, MMC SD Controller1 */
#define MSC2_IOBASE 0x13490000 /* 64KB, MMC SD Controller2 */

static LIST_HEAD(manual_list);
#define CPM_MSC0_CLK_R        (0xB0000068)
#define CPM_MSC1_CLK_R        (0xB00000a4)
#define CPM_MSC2_CLK_R        (0xB00000a8)
#define CPM_MSC0_CLK_EXT_BIT  21

#ifdef CONFIG_FPGA_TEST
#define MSC_CLK_H_FREQ        (0x1 << 20)
static void sdhci_ingenic_fpga_clk(unsigned int clock)
{
#define CPM_MSC_CLK_R CPM_MSC0_CLK_R
//#define CPM_MSC_CLK_R CPM_MSC1_CLK_R
    unsigned int val;

    if(500000 <= clock){
        val = readl((const volatile void*)CPM_MSC_CLK_R);
        val |= MSC_CLK_H_FREQ;
        writel(val, (void*)CPM_MSC_CLK_R);
    } else {
        val = readl((const volatile void*)CPM_MSC_CLK_R);
        val &= ~MSC_CLK_H_FREQ;
        writel(val, (void*)CPM_MSC_CLK_R);
    }
    printk("\tclk=%d, CPM_MSC0_CLK_R: %08x\n\n", clock, readl((const volatile void*)CPM_MSC0_CLK_R));
}
#endif
struct jz_func_alter {
    int pin;
    int function;
    const char *name;
};

struct module_mmc_pin {
    int                 num;
    int                 enable_level;
};
#define module_string_length 24

struct jz_msc_drv {
    int is_enable;
    int id;
    int bus_width;
    int max_frequency;
    char cd_method[24];
    char speed[24];
    bool cap_power_off_card;
    bool cap_mmc_hw_reset;
    bool cap_sdio_irq;
    bool full_pwr_cycle;
    bool keep_power_in_suspend;
    bool enable_sdio_wakeup;
    int dsr;
    bool pio_mode;
    bool force_1_bit_only;
    bool enable_autocmd12;
    bool enable_cpm_rx_tuning;
    bool enable_cpm_tx_tuning;
    bool sdio_clk;
    struct module_mmc_pin rst;
    struct module_mmc_pin wp;
    struct module_mmc_pin pwr;
    struct module_mmc_pin cd;
    struct module_mmc_pin sdr;
    struct jz_func_alter *alter_pin;
};

static struct jz_func_alter jz_msc0_pin[10] = {
    {GPIO_PD(17), GPIO_FUNC_0, "msc0_clk"},
    {GPIO_PD(18), GPIO_FUNC_0, "msc0_cmd"},
    {GPIO_PD(19), GPIO_FUNC_0, "msc0_d0"},
    {GPIO_PD(20), GPIO_FUNC_0, "msc0_d1"},
    {GPIO_PD(21), GPIO_FUNC_0, "msc0_d2"},
    {GPIO_PD(22), GPIO_FUNC_0, "msc0_d3"},
    {GPIO_PD(23), GPIO_FUNC_0, "msc0_d4"},
    {GPIO_PD(24), GPIO_FUNC_0, "msc0_d5"},
    {GPIO_PD(25), GPIO_FUNC_0, "msc0_d6"},
    {GPIO_PD(26), GPIO_FUNC_0, "msc0_d7"},
};

static struct jz_func_alter jz_msc1_pin[6] = {
    {GPIO_PD(8), GPIO_FUNC_0, "msc1_clk"},
    {GPIO_PD(9), GPIO_FUNC_0, "msc1_cmd"},
    {GPIO_PD(10), GPIO_FUNC_0, "msc1_d0"},
    {GPIO_PD(11), GPIO_FUNC_0, "msc1_d1"},
    {GPIO_PD(12), GPIO_FUNC_0, "msc1_d2"},
    {GPIO_PD(13), GPIO_FUNC_0, "msc1_d3"},
};

static struct jz_func_alter jz_msc2_pin[6] = {
    {GPIO_PE(0), GPIO_FUNC_0, "msc2_clk"},
    {GPIO_PE(1), GPIO_FUNC_0, "msc2_cmd"},
    {GPIO_PE(2), GPIO_FUNC_0, "msc2_d0"},
    {GPIO_PE(3), GPIO_FUNC_0, "msc2_d1"},
    {GPIO_PE(4), GPIO_FUNC_0, "msc2_d2"},
    {GPIO_PE(5), GPIO_FUNC_0, "msc2_d3"},
};

struct jz_msc_drv jzmsc_gpio[3] = {
    {
        .id = 0,
        .alter_pin = jz_msc0_pin,
    },
    {
        .id = 1,
        .alter_pin = jz_msc1_pin,
    },
    {
        .id = 2,
        .alter_pin = jz_msc2_pin,
    },
};

static int wifi_power_on = -1;
static int wifi_reg_on = -1;
static int wifi_power_on_level = -1;
static int wifi_reg_on_level = -1;
module_param_gpio(wifi_power_on, 0644);
module_param(wifi_power_on_level, int, 0644);
module_param_gpio(wifi_reg_on, 0644);
module_param(wifi_reg_on_level, int, 0644);

module_param_named(msc0_is_enable, jzmsc_gpio[0].is_enable, int, 0644);
module_param_named(msc0_bus_width, jzmsc_gpio[0].bus_width, int, 0644);
module_param_string(msc0_cd_method, jzmsc_gpio[0].cd_method, module_string_length, 0644);
module_param_string(msc0_speed, jzmsc_gpio[0].speed, module_string_length, 0644);
module_param_named(msc0_max_frequency, jzmsc_gpio[0].max_frequency, int, 0644);
module_param_named(msc0_cap_power_off_card, jzmsc_gpio[0].cap_power_off_card, bool, 0644);
module_param_named(msc0_cap_mmc_hw_reset, jzmsc_gpio[0].cap_mmc_hw_reset, bool, 0644);
module_param_named(msc0_cap_sdio_irq, jzmsc_gpio[0].cap_sdio_irq, bool, 0644);
module_param_named(msc0_full_pwr_cycle, jzmsc_gpio[0].full_pwr_cycle, bool, 0644);
module_param_named(msc0_keep_power_in_suspend, jzmsc_gpio[0].keep_power_in_suspend, bool, 0644);
module_param_named(msc0_enable_sdio_wakeup, jzmsc_gpio[0].enable_sdio_wakeup, bool, 0644);
module_param_named(msc0_dsr, jzmsc_gpio[0].dsr, int, 0644);
module_param_named(msc0_pio_mode, jzmsc_gpio[0].pio_mode, bool, 0644);
module_param_named(msc0_enable_autocmd12, jzmsc_gpio[0].enable_autocmd12, bool, 0644);
module_param_named(msc0_enable_cpm_rx_tuning, jzmsc_gpio[0].enable_cpm_rx_tuning, bool, 0644);
module_param_named(msc0_enable_cpm_tx_tuning, jzmsc_gpio[0].enable_cpm_tx_tuning, bool, 0644);
module_param_named(msc0_sdio_clk, jzmsc_gpio[0].sdio_clk, bool, 0644);
module_param_gpio_named(msc0_rst, jzmsc_gpio[0].rst.num, 0644);
module_param_gpio_named(msc0_wp, jzmsc_gpio[0].wp.num, 0644);
module_param_gpio_named(msc0_pwr, jzmsc_gpio[0].pwr.num, 0644);
module_param_gpio_named(msc0_cd, jzmsc_gpio[0].cd.num, 0644);
module_param_gpio_named(msc0_sdr, jzmsc_gpio[0].sdr.num, 0644);
module_param_named(msc0_rst_enable_level, jzmsc_gpio[0].rst.enable_level, int, 0644);
module_param_named(msc0_wp_enable_level, jzmsc_gpio[0].wp.enable_level, int, 0644);
module_param_named(msc0_pwr_enable_level, jzmsc_gpio[0].pwr.enable_level, int, 0644);
module_param_named(msc0_cd_enable_level, jzmsc_gpio[0].cd.enable_level, int, 0644);
module_param_named(msc0_sdr_enable_level, jzmsc_gpio[0].sdr.enable_level, int, 0644);

module_param_named(msc1_is_enable, jzmsc_gpio[1].is_enable, int, 0644);
module_param_named(msc1_bus_width, jzmsc_gpio[1].bus_width, int, 0644);
module_param_string(msc1_cd_method, jzmsc_gpio[1].cd_method, module_string_length, 0644);
module_param_string(msc1_speed, jzmsc_gpio[1].speed, module_string_length, 0644);
module_param_named(msc1_max_frequency, jzmsc_gpio[1].max_frequency, int, 0644);
module_param_named(msc1_cap_power_off_card, jzmsc_gpio[1].cap_power_off_card, bool, 0644);
module_param_named(msc1_cap_mmc_hw_reset, jzmsc_gpio[1].cap_mmc_hw_reset, bool, 0644);
module_param_named(msc1_cap_sdio_irq, jzmsc_gpio[1].cap_sdio_irq, bool, 0644);
module_param_named(msc1_full_pwr_cycle, jzmsc_gpio[1].full_pwr_cycle, bool, 0644);
module_param_named(msc1_keep_power_in_suspend, jzmsc_gpio[1].keep_power_in_suspend, bool, 0644);
module_param_named(msc1_enable_sdio_wakeup, jzmsc_gpio[1].enable_sdio_wakeup, bool, 0644);
module_param_named(msc1_dsr, jzmsc_gpio[1].dsr, int, 0644);
module_param_named(msc1_pio_mode, jzmsc_gpio[1].pio_mode, bool, 0644);
module_param_named(msc1_enable_autocmd12, jzmsc_gpio[1].enable_autocmd12, bool, 0644);
module_param_named(msc1_enable_cpm_rx_tuning, jzmsc_gpio[1].enable_cpm_rx_tuning, bool, 0644);
module_param_named(msc1_enable_cpm_tx_tuning, jzmsc_gpio[1].enable_cpm_tx_tuning, bool, 0644);
module_param_named(msc1_sdio_clk, jzmsc_gpio[1].sdio_clk, bool, 0644);
module_param_gpio_named(msc1_rst, jzmsc_gpio[1].rst.num, 0644);
module_param_gpio_named(msc1_wp, jzmsc_gpio[1].wp.num, 0644);
module_param_gpio_named(msc1_pwr, jzmsc_gpio[1].pwr.num, 0644);
module_param_gpio_named(msc1_cd, jzmsc_gpio[1].cd.num, 0644);
module_param_gpio_named(msc1_sdr, jzmsc_gpio[1].sdr.num, 0644);
module_param_named(msc1_rst_enable_level, jzmsc_gpio[1].rst.enable_level, int, 0644);
module_param_named(msc1_wp_enable_level, jzmsc_gpio[1].wp.enable_level, int, 0644);
module_param_named(msc1_pwr_enable_level, jzmsc_gpio[1].pwr.enable_level, int, 0644);
module_param_named(msc1_cd_enable_level, jzmsc_gpio[1].cd.enable_level, int, 0644);
module_param_named(msc1_sdr_enable_level, jzmsc_gpio[1].sdr.enable_level, int, 0644);

module_param_named(msc2_is_enable, jzmsc_gpio[2].is_enable, int, 0644);
module_param_named(msc2_bus_width, jzmsc_gpio[2].bus_width, int, 0644);
module_param_string(msc2_cd_method, jzmsc_gpio[2].cd_method, module_string_length, 0644);
module_param_string(msc2_speed, jzmsc_gpio[2].speed, module_string_length, 0644);
module_param_named(msc2_max_frequency, jzmsc_gpio[2].max_frequency, int, 0644);
module_param_named(msc2_cap_power_off_card, jzmsc_gpio[2].cap_power_off_card, bool, 0644);
module_param_named(msc2_cap_mmc_hw_reset, jzmsc_gpio[2].cap_mmc_hw_reset, bool, 0644);
module_param_named(msc2_cap_sdio_irq, jzmsc_gpio[2].cap_sdio_irq, bool, 0644);
module_param_named(msc2_full_pwr_cycle, jzmsc_gpio[2].full_pwr_cycle, bool, 0644);
module_param_named(msc2_keep_power_in_suspend, jzmsc_gpio[2].keep_power_in_suspend, bool, 0644);
module_param_named(msc2_enable_sdio_wakeup, jzmsc_gpio[2].enable_sdio_wakeup, bool, 0644);
module_param_named(msc2_dsr, jzmsc_gpio[2].dsr, int, 0644);
module_param_named(msc2_pio_mode, jzmsc_gpio[2].pio_mode, bool, 0644);
module_param_named(msc2_enable_autocmd12, jzmsc_gpio[2].enable_autocmd12, bool, 0644);
module_param_named(msc2_enable_cpm_rx_tuning, jzmsc_gpio[2].enable_cpm_rx_tuning, bool, 0644);
module_param_named(msc2_enable_cpm_tx_tuning, jzmsc_gpio[2].enable_cpm_tx_tuning, bool, 0644);
module_param_named(msc2_sdio_clk, jzmsc_gpio[2].sdio_clk, bool, 0644);
module_param_gpio_named(msc2_rst, jzmsc_gpio[2].rst.num, 0644);
module_param_gpio_named(msc2_wp, jzmsc_gpio[2].wp.num, 0644);
module_param_gpio_named(msc2_pwr, jzmsc_gpio[2].pwr.num, 0644);
module_param_gpio_named(msc2_cd, jzmsc_gpio[2].cd.num, 0644);
module_param_gpio_named(msc2_sdr, jzmsc_gpio[2].sdr.num, 0644);
module_param_named(msc2_rst_enable_level, jzmsc_gpio[2].rst.enable_level, int, 0644);
module_param_named(msc2_wp_enable_level, jzmsc_gpio[2].wp.enable_level, int, 0644);
module_param_named(msc2_pwr_enable_level, jzmsc_gpio[2].pwr.enable_level, int, 0644);
module_param_named(msc2_cd_enable_level, jzmsc_gpio[2].cd.enable_level, int, 0644);
module_param_named(msc2_sdr_enable_level, jzmsc_gpio[2].sdr.enable_level, int, 0644);

static unsigned int sdhci_ingenic_get_cpm_msc(struct sdhci_host *host)
{
    char msc_ioaddr[16];
    unsigned int cpm_msc;
    sprintf(msc_ioaddr, "0x%x", (unsigned int)host->ioaddr);

    if (!strcmp(msc_ioaddr ,"0xb3450000"))
        cpm_msc = CPM_MSC0_CLK_R;
    if (!strcmp(msc_ioaddr ,"0xb3460000"))
        cpm_msc = CPM_MSC1_CLK_R;
    if (!strcmp(msc_ioaddr ,"0xb3490000"))
        cpm_msc = CPM_MSC2_CLK_R;
    return cpm_msc;
}

/**
 * sdhci_ingenic_msc_tuning  Enable msc controller tuning
 *
 * Tuning rx phase
 * */
static void sdhci_ingenic_en_msc_tuning(struct sdhci_host *host, unsigned int cpm_msc)
{
    if (host->flags & SDHCI_SDR50_NEEDS_TUNING ||
        host->flags & SDHCI_HS400_TUNING) {
        *(volatile unsigned int*)cpm_msc &= ~(0x1 << 20);
    }
}

static void sdhci_ingenic_sel_rx_phase(unsigned int cpm_msc)
{
    *(volatile unsigned int*)cpm_msc |= (0x1 << 20); // default

    *(volatile unsigned int*)cpm_msc &= ~(0x7 << 17);
    *(volatile unsigned int*)cpm_msc |= (0x7 << 17); // OK  RX 90 TX 270
}

static void sdhci_ingenic_sel_tx_phase(unsigned int cpm_msc)
{
    *(volatile unsigned int*)cpm_msc &= ~(0x3 << 15);
/*    *(volatile unsigned int*)cpm_msc |= (0x2 << 15); // 180  100M OK*/
    *(volatile unsigned int*)cpm_msc |= (0x3 << 15);
}

/**
 * sdhci_ingenic_set_clock - callback on clock change
 * @host: The SDHCI host being changed
 * @clock: The clock rate being requested.
 *
 * When the card's clock is going to be changed, look at the new frequency
 * and find the best clock source to go with it.
*/
static void sdhci_ingenic_set_clock(struct sdhci_host *host, unsigned int clock)
{
#ifndef CONFIG_FPGA_TEST
    struct sdhci_ingenic *sdhci_ing = sdhci_priv(host);
    unsigned int cpm_msc = sdhci_ingenic_get_cpm_msc(host);

    if (clock == 0)
        return;

    sdhci_set_clock(host, clock);

    if (clock > 400000) {
        clk_set_parent(sdhci_ing->clk_mux, sdhci_ing->clk_mpll);
    } else {
        clk_set_parent(sdhci_ing->clk_mux, sdhci_ing->clk_ext);
        cpm_set_bit(CPM_MSC0_CLK_EXT_BIT, CPM_MSC0CDR);
    }

    clk_set_rate(sdhci_ing->clk_cgu, clock);

    if (host->mmc->ios.timing == MMC_TIMING_MMC_HS200 ||
        host->mmc->ios.timing == MMC_TIMING_UHS_SDR104) {

        /* RX phase selecte */
        if (sdhci_ing->pdata->enable_cpm_rx_tuning == 1)
            sdhci_ingenic_sel_rx_phase(cpm_msc);
        else
            sdhci_ingenic_en_msc_tuning(host, cpm_msc);
        /* TX phase selecte */
        if (sdhci_ing->pdata->enable_cpm_tx_tuning == 1)
            sdhci_ingenic_sel_tx_phase(cpm_msc);
    }
#else //CONFIG_FPGA_TEST
    sdhci_ingenic_fpga_clk(clock);
#endif
}

/* I/O Driver Strength Types */
#define INGENIC_TYPE_0  0x0        //30
#define INGENIC_TYPE_1  0x1        //50
#define INGENIC_TYPE_2  0x2        //66
#define INGENIC_TYPE_3  0x3        //100

void sdhci_ingenic_voltage_switch(struct sdhci_host *host, int voltage)
{
    struct sdhci_ingenic *sdhci_ing = sdhci_priv(host);
    struct sdhci_ingenic_pdata *pdata = sdhci_ing->pdata;
    unsigned int val;

    if (pdata->sdr_v18 < 0)
        return;

    switch (voltage){
        case MMC_SIGNAL_VOLTAGE_330:
            val = cpm_inl(CPM_EXCLK_DS) & ~(1 << 31);
            cpm_outl(val, CPM_EXCLK_DS);
            /*Set up hardware circuit 3V*/
            gpio_direction_output(sdhci_ing->pdata->sdr_v18, 0);
            break;
        case MMC_SIGNAL_VOLTAGE_180:
            /*controlled SD voltage to 1.8V*/
            val = cpm_inl(CPM_EXCLK_DS) | (1 << 31);
            cpm_outl(val, CPM_EXCLK_DS);
            /*Set up hardware circuit 1.8V*/
            gpio_direction_output(pdata->sdr_v18, 1);
            break;
        default:
            return ;
    }
}

void sdhci_ingenic_power_set(struct sdhci_host *host,int onoff)
{
    struct sdhci_ingenic *sdhci_ing = sdhci_priv(host);
    struct sdhci_ingenic_pdata *pdata = sdhci_ing->pdata;

    if (pdata->gpio->pwr.num >= 0){
        if (onoff)
            gpio_direction_output(pdata->gpio->pwr.num,pdata->gpio->pwr.enable_level);
        else
            gpio_direction_output(pdata->gpio->pwr.num,!pdata->gpio->pwr.enable_level);
    }
}

void sdhci_ingenic_hwreset(struct sdhci_host *host)
{
    struct sdhci_ingenic *sdhci_ing = sdhci_priv(host);
    struct sdhci_ingenic_pdata *pdata = sdhci_ing->pdata;

    if (pdata->gpio->rst.num >= 0){
        gpio_direction_output(pdata->gpio->rst.num,pdata->gpio->rst.enable_level);
        udelay(10);
        gpio_direction_output(pdata->gpio->rst.num,!pdata->gpio->rst.enable_level);
        usleep_range(300,500);
    }
}

static struct sdhci_ops sdhci_ingenic_ops = {
    .set_clock                              = sdhci_ingenic_set_clock,
    .set_bus_width                          = sdhci_set_bus_width,
    .reset                                  = sdhci_reset,
    .set_uhs_signaling                      = sdhci_set_uhs_signaling,
    .voltage_switch                         = sdhci_ingenic_voltage_switch,
    .power_set                              = sdhci_ingenic_power_set,
    .hw_reset                               = sdhci_ingenic_hwreset,
};

#ifdef CONFIG_OF
static inline void ingenic_mmc_get_gpio(struct device_node *np,struct ingenic_mmc_pin *pin, char *gpioname)
{
       int gpio;
       enum of_gpio_flags flags;

       pin->num = -EBUSY;
       gpio = of_get_named_gpio_flags(np, gpioname, 0, &flags);
       if(gpio_is_valid(gpio)) {
              pin->num = gpio;
              pin->enable_level = (flags == OF_GPIO_ACTIVE_LOW ? LOW_ENABLE : HIGH_ENABLE);
       }
       printk("mmc gpio %s num:%d en-level: %d\n",gpioname, pin->num, pin->enable_level);
}

static inline void ingenic_mmc_clk_onoff(struct sdhci_ingenic *ingenic_ing, unsigned int on)
{
    if (on) {
        clk_prepare_enable(ingenic_ing->clk_cgu);
        clk_prepare_enable(ingenic_ing->clk_gate);
    } else {
        clk_disable_unprepare(ingenic_ing->clk_cgu);
        clk_disable_unprepare(ingenic_ing->clk_gate);
    }
}

/**
 *    jzmmc_manual_detect - insert or remove card manually
 *    @index: host->index, namely the index of the controller.
 *    @on: 1 means insert card, 0 means remove card.
 *
 *    This functions will be called by manually card-detect driver such as
 *    wifi. To enable this mode you can set value pdata.removal = MANUAL.
 */
int jzmmc_manual_detect(int index, int on)
{
    struct sdhci_ingenic *sdhci_ing;
    struct sdhci_host *host;
    struct list_head *pos;

    list_for_each(pos, &manual_list) {
        sdhci_ing = list_entry(pos, struct sdhci_ingenic, list);
        if (sdhci_ing->pdev->id == index) {
            break;
        } else
            sdhci_ing = NULL;
    }

    if (!sdhci_ing) {
        printk("no manual card detect\n");
        return -1;
    }

    host = sdhci_ing->host;

    if (on) {
        dev_err(&sdhci_ing->pdev->dev, "card insert manually\n");
        set_bit(INGENIC_MMC_CARD_PRESENT, &sdhci_ing->flags);
#ifdef CLK_CTRL
        ingenic_mmc_clk_onoff(sdhci_ing, 1);
#endif
        host->flags &= ~SDHCI_DEVICE_DEAD;
        host->quirks |= SDHCI_QUIRK_BROKEN_CARD_DETECTION;
        mmc_detect_change(sdhci_ing->host->mmc, 0);
    } else {
        dev_err(&sdhci_ing->pdev->dev, "card remove manually\n");
        clear_bit(INGENIC_MMC_CARD_PRESENT, &sdhci_ing->flags);

        host->flags |= SDHCI_DEVICE_DEAD;
        host->quirks &= ~SDHCI_QUIRK_BROKEN_CARD_DETECTION;
        mmc_detect_change(sdhci_ing->host->mmc, 0);
#ifdef CLK_CTRL
        ingenic_mmc_clk_onoff(sdhci_ing, 0);
#endif
    }

#if (LINUX_VERSION_CODE < KERNEL_VERSION(5, 1, 0))
    tasklet_schedule(&host->finish_tasklet);
#endif

    return 0;
}
EXPORT_SYMBOL(jzmmc_manual_detect);

/**
 *    jzmmc_clk_ctrl - enable or disable msc clock gate
 *    @index: host->index, namely the index of the controller.
 *    @on: 1-enable msc clock gate, 0-disable msc clock gate.
 */
int jzmmc_clk_ctrl(int index, int on)
{
    struct sdhci_ingenic *sdhci_ing;
    struct list_head *pos;

#ifdef CLK_CTRL
    list_for_each(pos, &manual_list) {
        sdhci_ing = list_entry(pos, struct sdhci_ingenic, list);
        if (sdhci_ing->pdev->id == index)
            break;
        else
            sdhci_ing = NULL;
    }

    if (!sdhci_ing) {
        printk("no manual card detect\n");
        return -1;
    }
    ingenic_mmc_clk_onoff(sdhci_ing, on);
#endif
    return 0;
}
EXPORT_SYMBOL(jzmmc_clk_ctrl);

static inline int gpio_init(int gpio, enum gpio_function func, const char *name)
{
    int ret = 0;

    ret = gpio_request(gpio, name);
    if (ret < 0)
        return ret;

    gpio_set_func(gpio , func);

    return 0;
}

static int msc_gpio_request(struct jz_msc_drv *msc_gpio)
{
    int ret = 0;
    int i = 0;
    char buf[10];
    for (i = 0; i < msc_gpio->bus_width + 2; i++) {
        ret = gpio_init(msc_gpio->alter_pin->pin, msc_gpio->alter_pin->function, msc_gpio->alter_pin->name);
        if (ret < 0) {
            printk(KERN_ERR "MSC failed to request %s %s!\n", gpio_to_str(msc_gpio->alter_pin->pin, buf), msc_gpio->alter_pin->name);
            return -EINVAL;
        }
        msc_gpio->alter_pin++;
    }

    return 0;
}

static inline void msc_gpio_function_init(int id)
{
    int ret;
    struct jz_msc_drv *msc_gpio = &jzmsc_gpio[id];
    ret = msc_gpio_request(msc_gpio);
    if (ret < 0) {
        printk("spi%d gpio requeset failed\n", id);
        return;
    }

}
static void ingenic_mmc_init_gpio(struct ingenic_mmc_pin *pin, char *gpioname, int dir)
{
    if (gpio_is_valid(pin->num)) {
        if (gpio_request_one(pin->num, dir, gpioname)) {
            pr_info("%s no detect pin available\n", gpioname);
            pin->num = -EBUSY;
        }
    }
}

static int sdhci_ingenic_parse_dt(struct device *dev,
                                  struct sdhci_host *host,
                                  struct sdhci_ingenic_pdata *pdata,
                                  int index)
{
    struct card_gpio *card_gpio;
    char string[20];
    int dir = -1;
    card_gpio = devm_kzalloc(dev, sizeof(struct card_gpio), GFP_KERNEL);
    if(!card_gpio)
        return 0;

    msc_gpio_function_init(index);
    card_gpio->rst.num = jzmsc_gpio[index].rst.num;
    card_gpio->rst.enable_level = jzmsc_gpio[index].rst.enable_level;

    card_gpio->wp.num = jzmsc_gpio[index].wp.num;
    card_gpio->wp.enable_level = jzmsc_gpio[index].wp.enable_level;

    card_gpio->pwr.num = jzmsc_gpio[index].pwr.num;
    card_gpio->pwr.enable_level = jzmsc_gpio[index].pwr.enable_level;

    card_gpio->cd.num = jzmsc_gpio[index].cd.num;
    card_gpio->cd.enable_level = jzmsc_gpio[index].cd.enable_level;

    if (card_gpio->wp.num >= 0) {
        memset(string, 0, 20);
        sprintf(string, "msc%d_wp", index);
        ingenic_mmc_init_gpio(&card_gpio->wp, string, GPIOF_DIR_IN);
    }

    if (card_gpio->rst.num >= 0) {
        memset(string, 0, 20);
        sprintf(string, "msc%d_rst", index);
        ingenic_mmc_init_gpio(&card_gpio->rst, string, GPIOF_DIR_OUT);
    }

    if (card_gpio->pwr.num >= 0) {
        memset(string, 0, 20);
        sprintf(string, "msc%d_pwr", index);
        dir = card_gpio->pwr.enable_level ? GPIOF_OUT_INIT_HIGH : GPIOF_OUT_INIT_LOW;
        ingenic_mmc_init_gpio(&card_gpio->pwr, string, dir);
    }
    pdata->gpio = card_gpio;

    pdata->sdr_v18 = jzmsc_gpio[index].sdr.num;

    /* assuming internal card detect that will be configured by pinctrl */
    pdata->cd_type = SDHCI_INGENIC_CD_INTERNAL;

    if(jzmsc_gpio[index].pio_mode) {
        pdata->pio_mode = 1;
    }

    if (jzmsc_gpio[index].bus_width == 1) {
        pdata->force_1_bit_only = 1;
    }

    if(jzmsc_gpio[index].enable_autocmd12) {
        pdata->enable_autocmd12 = 1;
    }
    if(jzmsc_gpio[index].enable_cpm_rx_tuning) {
        pdata->enable_cpm_rx_tuning = 1;
    }
    if(jzmsc_gpio[index].enable_cpm_tx_tuning) {
        pdata->enable_cpm_tx_tuning = 1;
    }

    /* get the card detection method */
    if (strcmp(jzmsc_gpio[index].cd_method, "broken-cd") == 0) {
        pdata->cd_type = SDHCI_INGENIC_CD_NONE;
    }

    if (strcmp(jzmsc_gpio[index].cd_method, "non-removable") == 0) {
        pdata->cd_type = SDHCI_INGENIC_CD_PERMANENT;
    }
    if (strcmp(jzmsc_gpio[index].cd_method, "cd-inverted") == 0) {
        pdata->cd_type = SDHCI_INGENIC_CD_GPIO;
    }

        pdata->sdio_clk = jzmsc_gpio[index].sdio_clk;

    /* if(of_property_read_bool(np, "ingenic,removal-dontcare")) { */
    /*     pdata->removal = DONTCARE; */
    /* } else if(of_property_read_bool(np, "ingenic,removal-nonremovable")) { */
    /*     pdata->removal = NONREMOVABLE; */
    /* } else if(of_property_read_bool(np, "ingenic,removal-removable")) { */
    /*     pdata->removal = REMOVABLE; */
    /* } else if(of_property_read_bool(np, "ingenic,removal-manual")) { */
    /*     pdata->removal = MANUAL; */
    /* }; */

    /* mmc_of_parse_voltage(np, &pdata->ocr_avail); */

    return 0;
}
#else
static int sdhci_ingenic_parse_dt(struct device *dev,
                                  struct sdhci_host *host,
                                  struct sdhci_ingenic_pdata *pdata,
                                  int index)
{
    return -EINVAL;
}
#endif

static int jzmmc_of_parse(struct mmc_host *host, int index)
{
    u32 bus_width;
    bool cd_cap_invert, cd_gpio_invert = false;
    bool ro_cap_invert, ro_gpio_invert = false;

    if (!host->parent)
        return 0;

    switch (jzmsc_gpio[index].bus_width) {
    case 8:
        host->caps |= MMC_CAP_8_BIT_DATA;
        /* Hosts capable of 8-bit transfers can also do 4 bits */
    case 4:
        host->caps |= MMC_CAP_4_BIT_DATA;
        break;
    case 1:
        break;
    default:
        dev_err(host->parent,
            "Invalid \"bus-width\" value %u!\n", bus_width);
        return -EINVAL;
    }

    /* f_max is obtained from the optional "max-frequency" property */
    host->f_max = jzmsc_gpio[index].max_frequency;

    /*
     * Configure CD and WP pins. They are both by default active low to
     * match the SDHCI spec. If GPIOs are provided for CD and / or WP, the
     * mmc-gpio helpers are used to attach, configure and use them. If
     * polarity inversion is specified in DT, one of MMC_CAP2_CD_ACTIVE_HIGH
     * and MMC_CAP2_RO_ACTIVE_HIGH capability-2 flags is set. If the
     * "broken-cd" property is provided, the MMC_CAP_NEEDS_POLL capability
     * is set. If the "non-removable" property is found, the
     * MMC_CAP_NONREMOVABLE capability is set and no card-detection
     * configuration is performed.
     */

    /* Parse Card Detection */
    if (strcmp(jzmsc_gpio[index].cd_method, "non-removable") == 0) {
        host->caps |= MMC_CAP_NONREMOVABLE;
    } else {
        if (strcmp(jzmsc_gpio[index].cd_method, "cd-inverted") == 0)
            cd_cap_invert = true;

        if (strcmp(jzmsc_gpio[index].cd_method, "broken-cd") == 0)
            host->caps |= MMC_CAP_NEEDS_POLL;

        if (jzmsc_gpio[index].cd.num >= 0) {
            dev_info(host->parent, "Got CD GPIO\n");
            cd_gpio_invert = true;
        }

        /*
         * There are two ways to flag that the CD line is inverted:
         * through the cd-inverted flag and by the GPIO line itself
         * being inverted from the GPIO subsystem. This is a leftover
         * from the times when the GPIO subsystem did not make it
         * possible to flag a line as inverted.
         *
         * If the capability on the host AND the GPIO line are
         * both inverted, the end result is that the CD line is
         * not inverted.
         */
        if (cd_cap_invert ^ cd_gpio_invert)
            host->caps2 |= MMC_CAP2_CD_ACTIVE_HIGH;
    }

    /* Parse Write Protection */
    ro_cap_invert = jzmsc_gpio[index].wp.enable_level;

    if (jzmsc_gpio[index].wp.num >= 0) {
        dev_info(host->parent, "Got WP GPIO\n");
        ro_gpio_invert = true;
    }

    if (jzmsc_gpio[index].wp.num != -1)
        host->caps2 |= MMC_CAP2_NO_WRITE_PROTECT;

    /* See the comment on CD inversion above */
    if (ro_cap_invert ^ ro_gpio_invert)
        host->caps2 |= MMC_CAP2_RO_ACTIVE_HIGH;

    if (strcmp(jzmsc_gpio[index].speed, "sd_card") == 0)
        host->caps |= MMC_CAP_SD_HIGHSPEED;

    if (strcmp(jzmsc_gpio[index].speed, "sdio") == 0 || jzmsc_gpio[index].sdr.num >= 0)
        host->caps |= MMC_CAP_UHS_SDR12 | MMC_CAP_UHS_SDR25 | MMC_CAP_UHS_SDR50 | MMC_CAP_UHS_SDR104 | MMC_CAP_UHS_DDR50;

    if (strcmp(jzmsc_gpio[index].speed, "emmc") == 0) {
        host->caps |= MMC_CAP_MMC_HIGHSPEED | MMC_CAP_1_8V_DDR;
        host->caps2 |= MMC_CAP2_HS200_1_8V_SDR;
    }

    if (jzmsc_gpio[index].cap_power_off_card)
        host->caps |= MMC_CAP_POWER_OFF_CARD;

    if (jzmsc_gpio[index].cap_mmc_hw_reset)
        host->caps |= MMC_CAP_HW_RESET;

    if (jzmsc_gpio[index].cap_sdio_irq)
        host->caps |= MMC_CAP_SDIO_IRQ;

    if (jzmsc_gpio[index].full_pwr_cycle)
        host->caps2 |= MMC_CAP2_FULL_PWR_CYCLE;

    if (jzmsc_gpio[index].keep_power_in_suspend)
        host->pm_caps |= MMC_PM_KEEP_POWER;

    if (jzmsc_gpio[index].enable_sdio_wakeup)
        host->pm_caps |= MMC_PM_WAKE_SDIO_IRQ;

    host->dsr = jzmsc_gpio[index].dsr;

    if (host->dsr)
        host->dsr_req = 1;
    else
        host->dsr_req = 0;

    if (host->dsr_req && (host->dsr & ~0xffff)) {
        dev_err(host->parent,
            "device tree specified broken value for DSR: 0x%x, ignoring\n",
            host->dsr);
        host->dsr_req = 0;
    }

    return 0;
}

static int sdhci_ingenic_probe(struct platform_device *pdev)
{
    struct sdhci_ingenic_pdata *pdata;
    struct device *dev = &pdev->dev;
    struct sdhci_host *host;
    struct sdhci_ingenic *sdhci_ing;
    struct resource *regs;
    char clkname[16];
    int ret, irq;

    regs = platform_get_resource(pdev, IORESOURCE_MEM, 0);
    if (!regs) {
        dev_err(&pdev->dev, "No iomem resource\n");
        return -ENXIO;
    }

    irq = platform_get_irq(pdev, 0);
    if (irq < 0) {
        dev_err(dev, "no irq specified\n");
        return irq;
    }

    host = sdhci_alloc_host(dev, sizeof(struct sdhci_ingenic));
    if (IS_ERR(host)) {
        dev_err(dev, "sdhci_alloc_host() failed\n");
        return PTR_ERR(host);
    }
    sdhci_ing = sdhci_priv(host);

    pdata = devm_kzalloc(&pdev->dev, sizeof(*pdata), GFP_KERNEL);
    if (!pdata) {
        ret = -ENOMEM;
        return ret;
    }

    ret = sdhci_ingenic_parse_dt(&pdev->dev, host, pdata, pdev->id);
    if (ret)
        return ret;

    sprintf(clkname, "div_msc%d", pdev->id);
    sdhci_ing->clk_cgu = devm_clk_get(&pdev->dev, clkname);
    if(!sdhci_ing->clk_cgu) {
        dev_err(&pdev->dev, "Failed to Get MSC clk!\n");
        return PTR_ERR(sdhci_ing->clk_cgu);
    }

    sprintf(clkname, "gate_msc%d", pdev->id);
    sdhci_ing->clk_gate = devm_clk_get(&pdev->dev, clkname);
    if(!sdhci_ing->clk_gate) {
        dev_err(&pdev->dev, "Failed to Get PWC MSC clk!\n");
        return PTR_ERR(sdhci_ing->clk_gate);
    }

    if (!sdhci_ing->clk_ext)
        sdhci_ing->clk_ext = clk_get(NULL, "ext");

    if (!sdhci_ing->clk_mpll)
        sdhci_ing->clk_mpll = clk_get(NULL, "mpll");

    sprintf(clkname, "mux_msc%d", pdev->id);
    sdhci_ing->clk_mux = clk_get(NULL, clkname);
    if(!sdhci_ing->clk_mux) {
        dev_err(&pdev->dev, "Failed to Get PWC MSC clk!\n");
        return PTR_ERR(sdhci_ing->clk_mux);
    }

    ingenic_mmc_clk_onoff(sdhci_ing, 1);


    sdhci_ing->host = host;
    sdhci_ing->dev  = &pdev->dev;
    sdhci_ing->pdev = pdev;
    sdhci_ing->pdata = pdata;

    host->ioaddr = ioremap(regs->start, resource_size(regs));
    if (IS_ERR(host->ioaddr)) {
        return PTR_ERR(host->ioaddr);
    }

    platform_set_drvdata(pdev, host);

    /* sdio for WIFI init*/
    if (pdata->sdio_clk)
        list_add(&(sdhci_ing->list), &manual_list);
    if (pdata->sdr_v18 > 0 && \
            devm_gpio_request(dev, pdata->sdr_v18, "sdr-v18")) {
        printk("ERROR: no sdr-v18 pin available !!\n");
    }

    host->hw_name = "ingenic-sdhci";
    host->ops = &sdhci_ingenic_ops;
    host->quirks = 0;
    host->irq = irq;

    /* Software redefinition caps */
    host->quirks |= SDHCI_QUIRK_MISSING_CAPS;
    host->caps  = CAPABILITIES1_SW;
    host->caps1 = CAPABILITIES2_SW;

    /* not check wp */
    host->quirks |= SDHCI_QUIRK_INVERTED_WRITE_PROTECT;

    /* Setup quirks for the controller */
    host->quirks |= SDHCI_QUIRK_NO_ENDATTR_IN_NOPDESC;
    host->quirks |= SDHCI_QUIRK_NO_HISPD_BIT;

    /* Data Timeout Counter Value */
    //host->quirks |= SDHCI_QUIRK_BROKEN_TIMEOUT_VAL;
    host->timeout_clk = 24000; //TMCLK = 24MHz

    /* support force:1-bit-only */
    if (pdata->force_1_bit_only)
        host->quirks |= SDHCI_QUIRK_FORCE_1_BIT_DATA;

    /* This host supports the Auto CMD12 */
    if(pdata->enable_autocmd12)
        host->quirks |= SDHCI_QUIRK_MULTIBLOCK_READ_ACMD12;

    /* PIO transfer mode */
    if(pdata->pio_mode){
        host->quirks |= SDHCI_QUIRK_BROKEN_DMA;
        host->quirks |= SDHCI_QUIRK_BROKEN_ADMA;
    }
    /* TODO:SoCs need BROKEN_ADMA_ZEROLEN_DESC */
/*    host->quirks |= SDHCI_QUIRK_BROKEN_ADMA_ZEROLEN_DESC;*/

    if (pdata->cd_type == SDHCI_INGENIC_CD_NONE ||
        pdata->cd_type == SDHCI_INGENIC_CD_PERMANENT)
        host->quirks |= SDHCI_QUIRK_BROKEN_CARD_DETECTION;

    if (pdata->cd_type == SDHCI_INGENIC_CD_PERMANENT)
        host->mmc->caps = MMC_CAP_NONREMOVABLE;

    if (pdata->pm_caps)
        host->mmc->pm_caps |= pdata->pm_caps;

    host->quirks |= (SDHCI_QUIRK_32BIT_DMA_ADDR |
                     SDHCI_QUIRK_32BIT_DMA_SIZE);

    /* It supports additional host capabilities if needed */
    if (pdata->host_caps)
        host->mmc->caps |= pdata->host_caps;

    if (pdata->host_caps2)
        host->mmc->caps2 |= pdata->host_caps2;

#ifdef CONFIG_PM_RUNTIME
    pm_runtime_enable(&pdev->dev);
    pm_runtime_set_autosuspend_delay(&pdev->dev, 50);
    pm_runtime_use_autosuspend(&pdev->dev);
    pm_suspend_ignore_children(&pdev->dev, 1);
#endif

    ret = jzmmc_of_parse(host->mmc, pdev->id);
    if (ret) {
        dev_err(dev, "mmc_of_parse() failed\n");
        pm_runtime_forbid(&pdev->dev);
        pm_runtime_get_noresume(&pdev->dev);
        return ret;
    }

    ret = sdhci_add_host(host);
    if (ret) {
        dev_err(dev, "sdhci_add_host() failed\n");
        pm_runtime_forbid(&pdev->dev);
        pm_runtime_get_noresume(&pdev->dev);
        return ret;
    }

    /*   enable card  inserted and unplug wake-up system */
    if(host->mmc->slot.cd_irq > 0)
        enable_irq_wake(host->mmc->slot.cd_irq);

#ifdef CONFIG_PM_RUNTIME
    if (pdata->cd_type != SDHCI_INGENIC_CD_INTERNAL) {
        clk_disable_unprepare(sdhci_ing->clk_cgu);
        /* clk_disable_unprepare(sdhci_ing->clk_gate); */
    }
#endif

    return 0;
}

static void ingenic_mmc_gpio_deinit(struct card_gpio *card_gpio)
{
    if(gpio_is_valid(card_gpio->cd.num)) {
        gpio_free(card_gpio->cd.num);
    }
    if(gpio_is_valid(card_gpio->wp.num)) {
        gpio_free(card_gpio->wp.num);
    }
    if(gpio_is_valid(card_gpio->pwr.num)) {
        gpio_free(card_gpio->pwr.num);
    }
    if(gpio_is_valid(card_gpio->rst.num)) {
        gpio_free(card_gpio->rst.num);
    }
}

static int sdhci_ingenic_remove(struct platform_device *pdev)
{
    struct sdhci_host *host =  platform_get_drvdata(pdev);
    struct sdhci_ingenic *sdhci_ing = sdhci_priv(host);

    if (sdhci_ing->clk_ext)
        clk_put(sdhci_ing->clk_ext);

    if (sdhci_ing->clk_mpll)
        clk_put(sdhci_ing->clk_mpll);

    if (sdhci_ing->clk_mux)
        clk_put(sdhci_ing->clk_mux);

    if (sdhci_ing->ext_cd_irq)
        free_irq(sdhci_ing->ext_cd_irq, sdhci_ing);

#ifdef CONFIG_PM_RUNTIME
    if (pdata->cd_type != SDHCI_INGENIC_CD_INTERNAL){
        /* clk_prepare_enable(sdhci_ing->clk_gate); */
        clk_prepare_enable(sdhci_ing->clk_cgu);
    }
#endif
    sdhci_remove_host(host, 1);

    pm_runtime_dont_use_autosuspend(&pdev->dev);
    pm_runtime_disable(&pdev->dev);

    clk_disable_unprepare(sdhci_ing->clk_cgu);
    /* clk_disable_unprepare(sdhci_ing->clk_gate); */

    sdhci_free_host(host);
    platform_set_drvdata(pdev, NULL);
    ingenic_mmc_gpio_deinit(sdhci_ing->pdata->gpio);

    return 0;
}

#ifdef CONFIG_PM_SLEEP

static int sdhci_ingenic_suspend(struct device *dev)
{
    struct sdhci_host *host = dev_get_drvdata(dev);
    struct sdhci_ingenic *sdhci_ing = sdhci_priv(host) ;
    clk_disable_unprepare(sdhci_ing->clk_gate);
    return sdhci_suspend_host(host);
}

static int sdhci_ingenic_resume(struct device *dev)
{
    struct sdhci_host *host = dev_get_drvdata(dev);
    struct sdhci_ingenic *sdhci_ing = sdhci_priv(host) ;
    clk_prepare_enable(sdhci_ing->clk_gate);
    return sdhci_resume_host(host);
}
#endif

#ifdef CONFIG_PM
static int sdhci_ingenic_runtime_suspend(struct device *dev)
{
    return 0;
}


static int sdhci_ingenic_runtime_resume(struct device *dev)
{
    return 0;
}
#endif

#ifdef CONFIG_PM
static const struct dev_pm_ops sdhci_ingenic_pmops = {
    SET_SYSTEM_SLEEP_PM_OPS(sdhci_ingenic_suspend, sdhci_ingenic_resume)
    SET_RUNTIME_PM_OPS(sdhci_ingenic_runtime_suspend, sdhci_ingenic_runtime_resume,
               NULL)
};

#define SDHCI_INGENIC_PMOPS (&sdhci_ingenic_pmops)

#else
#define SDHCI_INGENIC_PMOPS NULL
#endif


static struct platform_device_id sdhci_ingenic_driver_ids[] = {
    {
        .name        = "md_ingenic,sdhci",
        .driver_data    = (kernel_ulong_t)NULL,
    },
    { }
};
MODULE_DEVICE_TABLE(platform, sdhci_ingenic_driver_ids);

#ifdef CONFIG_OF
static const struct of_device_id sdhci_ingenic_dt_match[] = {
    {.compatible = "md_ingenic,sdhci",},
    {},
};
MODULE_DEVICE_TABLE(of, sdhci_ingenic_dt_match);
#endif

static struct platform_driver msc_driver = {
    .probe        = sdhci_ingenic_probe,
    .remove        = sdhci_ingenic_remove,
    .id_table    = sdhci_ingenic_driver_ids,
    .driver        = {
        .owner    = THIS_MODULE,
        .name    = "md_ingenic,sdhci",
        .pm    = SDHCI_INGENIC_PMOPS,
        .of_match_table = of_match_ptr(sdhci_ingenic_dt_match),
    },
};
static void msc_dev_release(struct device *dev){}

static u64 msc_dmamask = ~(u32) 0;
#define DEF_MSC(NO)                            \
    static struct resource msc##NO##_resources[] = {        \
        {                            \
            .start          = MSC##NO##_IOBASE,        \
            .end            = MSC##NO##_IOBASE + 0x10000 - 1, \
            .flags          = IORESOURCE_MEM,        \
        },                            \
        {                            \
            .start          = IRQ_INTC_BASE+IRQ_MSC##NO,            \
            .end            = IRQ_INTC_BASE+IRQ_MSC##NO,            \
            .flags          = IORESOURCE_IRQ,        \
        },                            \
    };                                \
struct platform_device msc##NO##_device = {                  \
    .name = "md_ingenic,sdhci",                    \
    .id = NO,                        \
    .dev = {                        \
        .dma_mask               = &msc_dmamask,    \
        .coherent_dma_mask      = 0xffffffff,        \
        .release                = msc_dev_release,    \
    },                            \
    .resource       = msc##NO##_resources,               \
    .num_resources  = ARRAY_SIZE(msc##NO##_resources),    \
};
DEF_MSC(0);
DEF_MSC(1);
DEF_MSC(2);

static int msc_init(void)
{
    int ret;

    if (wifi_power_on != -1) {
        if (wifi_power_on_level)
            gpio_set_func(wifi_power_on, GPIO_OUTPUT1);
        else
            gpio_set_func(wifi_power_on, GPIO_OUTPUT0);
    }
    if (wifi_reg_on != -1) {
        if (wifi_reg_on_level)
            gpio_set_func(wifi_reg_on, GPIO_OUTPUT1);
        else
            gpio_set_func(wifi_reg_on, GPIO_OUTPUT0);
    }

    if (jzmsc_gpio[0].is_enable) {
        ret = platform_device_register(&msc0_device);
        if (ret) {
            printk(KERN_ERR "msc0: Failed to register msc dev: %d\n", ret);
            return ret;
        }
    }

    if (jzmsc_gpio[1].is_enable) {
        ret = platform_device_register(&msc1_device);
        if (ret) {
            printk(KERN_ERR "msc1: Failed to register msc dev: %d\n", ret);
            return ret;
        }
    }

    if (jzmsc_gpio[2].is_enable) {
        ret = platform_device_register(&msc2_device);
        if (ret) {
            printk(KERN_ERR "msc2: Failed to register msc dev: %d\n", ret);
            return ret;
        }
    }

    platform_driver_register(&msc_driver);
    return 0;
}

static void msc_exit(void)
{
    if (jzmsc_gpio[0].is_enable)
        platform_device_unregister(&msc0_device);

    if (jzmsc_gpio[1].is_enable)
        platform_device_unregister(&msc1_device);

    if (jzmsc_gpio[2].is_enable)
        platform_device_unregister(&msc2_device);

    platform_driver_unregister(&msc_driver);
    return ;
}

module_init(msc_init);

module_exit(msc_exit);

MODULE_DESCRIPTION("Ingenic SDHCI (MSC) driver");
MODULE_LICENSE("GPL v2");
