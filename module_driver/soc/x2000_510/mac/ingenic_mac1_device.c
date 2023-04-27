#include <linux/kernel.h>
#include <linux/slab.h>
#include <linux/platform_device.h>
#include <linux/phy.h>
#include <linux/mfd/syscon.h>
#include <linux/regmap.h>
#include <linux/clk.h>
#include <linux/of.h>
#include <linux/of_net.h>
#include <linux/irq.h>
#include <utils/gpio.h>

#include <soc/cpm.h>

#include "platfrom_stmmac.h"
#include "stmmac.h"

#define IRQ_GMAC1    (IRQ_INTC_BASE + 53)

#define JZ_MAC1_BASE_ADDR   0x134A0000
#define JZ_MAC1_END_ADDR    0x134AFFFF

#define MAC_INTERFACT_MASK      0x7
#define MAC_INTERFACT_RMII      0x4
#define MAC_INTERFACT_RGMII      0x1
#define CPM_MAC_PHY1            0xe8
#define MAC_CRLT_PORT               1
#define MAC_CRLT_FUNC               GPIO_FUNC_3
#define MAC_CRLT_GPIO(pin)          GPIO_PB(pin)

#define MAC_RGMII_TX_DELAY_MASK 0xff
#define MAC_RGMII_TX_DELAY      12
#define MAC_RGMII_RX_DELAY_MASK 0xff
#define MAC_RGMII_RX_DELAY      4
#define MAC_RGMII_DELAY_SELECT   0x1
#define MAC_RGMII_TX_SEL_DELAY  19
#define MAC_RGMII_RX_SEL_DELAY  11
#define MAC_SOFT_RESET_MAC           (1 << 3)


#define MAC_RMII_GPIO_PHY_CLK      0xde3300
#define MAC_RGMII_GPIO_PHY_CLK     0xfeff00

#define MAC_TX_CLK_GPIO            0x200000
#define MAC_PHY_CLK_GPIO           0x800000
#define MAC_MDIO_GPIO              0x180000

enum {
    RMII,
    RGMII,
};

static unsigned int mac_enable;
static unsigned int tx_clk_delay;
static unsigned int rx_clk_delay;
static unsigned int phy_interface;
static unsigned int phy_mask;
static unsigned int mac_crtl_gpio_mask;
static unsigned int tx_clk_rate;
static unsigned int tx_clk_enable_delay;
static unsigned int enable_tx_clk;
static unsigned int enable_crystal_clk;
static unsigned int enable_virtual_phy;

module_param_named(mac1_enable_flag, mac_enable, uint, 0444);
module_param_named(mac1_tx_clk_rate, tx_clk_rate, uint, 0444);
module_param_named(mac1_tx_clk_enable_delay, tx_clk_enable_delay, uint, 0444);
module_param_named(mac1_enable_tx_clk, enable_tx_clk, uint, 0444);
module_param_named(mac1_enable_crystal_clk, enable_crystal_clk, uint, 0444);
module_param_named(mac1_tx_clk_delay, tx_clk_delay, uint, 0444);
module_param_named(mac1_rx_clk_delay, rx_clk_delay, uint, 0444);
module_param_named(mac1_phy_interface, phy_interface, uint, 0444);
module_param_named(mac1_phy_probe_mask, phy_mask, uint, 0444);
module_param_named(mac1_enable_virtual_phy, enable_virtual_phy, uint, 0444);

static int mac1_phy_reset_gpio;
static unsigned int phy_reset_level;
static unsigned int phy_reset_time_us;

module_param_gpio(mac1_phy_reset_gpio, 0444);
module_param_named(mac1_phy_reset_level, phy_reset_level, uint, 0644);
module_param_named(mac1_phy_reset_time_us, phy_reset_time_us, uint, 0644);

static struct clk *phy_tx_clk;
static struct clk *mux_tx_clk;
static struct clk *mpll_clk;
static struct clk *epll_clk;

static void mac_free_gpio(unsigned long pins)
{
    int i;

    for (i = 0; i < 32; i++) {
        if (pins & (1 << i))
            gpio_free(MAC_CRLT_GPIO(i));
    }

}

static int mac_request_pins(unsigned long pins)
{
    int i, ret;
    unsigned long req_pins = 0;

    for (i = 0; i < 32; i++) {
        if (pins & (1 << i)) {
            ret = gpio_request(MAC_CRLT_GPIO(i), "mac1_gpio");
            if (ret) {
                mac_free_gpio(req_pins);
                return -1;
            }

            req_pins |= (1 << i);
        }
    }

    return 0;
}

static void mac_gpio_select(void)
{
    if (phy_interface == RGMII)
        mac_crtl_gpio_mask = MAC_RGMII_GPIO_PHY_CLK;
    else
        mac_crtl_gpio_mask = MAC_RMII_GPIO_PHY_CLK;

    if (enable_tx_clk)
        mac_crtl_gpio_mask |= MAC_TX_CLK_GPIO;

    if (enable_crystal_clk)
        mac_crtl_gpio_mask &= ~MAC_PHY_CLK_GPIO;

    if (enable_virtual_phy)
        mac_crtl_gpio_mask &= ~MAC_MDIO_GPIO;

}

static int mac_enable_tx_clk(void)
{
    mpll_clk = clk_get(NULL, "mpll");
    if (IS_ERR(phy_tx_clk)) {
        printk(KERN_ERR "%s: mpll get clk failure\n", __func__);
        return -1;
    }

    epll_clk = clk_get(NULL, "epll");
    if (IS_ERR(phy_tx_clk)) {
        clk_put(mpll_clk);
        printk(KERN_ERR "%s: epll get clk failure\n", __func__);
        return -1;
    }

    mux_tx_clk = clk_get(NULL, "mux_mactxphy1");
    if (IS_ERR(mux_tx_clk)) {
        clk_put(epll_clk);
        clk_put(mpll_clk);
        printk(KERN_ERR "%s: mux get clk failure\n", __func__);
        return -1;
    }

    phy_tx_clk = clk_get(NULL, "div_mactxphy1");
    if (IS_ERR(phy_tx_clk)) {
        clk_put(mux_tx_clk);
        clk_put(epll_clk);
        clk_put(mpll_clk);
        printk(KERN_ERR "%s: phy get clk failure\n", __func__);
        return -1;
    }

    if (phy_interface == RGMII) {
        clk_set_parent(mux_tx_clk, mpll_clk);
        clk_set_rate(phy_tx_clk, 125000000);
        clk_prepare_enable(phy_tx_clk);
    } else {
        /* 100M used*/
        clk_set_parent(mux_tx_clk, epll_clk);
        if (enable_tx_clk) {
            if (tx_clk_rate)
                clk_set_rate(phy_tx_clk, tx_clk_rate);
            else
                printk(KERN_WARNING "%s: phy tx clk rate is 0\n", __func__);

            clk_prepare_enable(phy_tx_clk);
        }
    }

    return 0;
}

static void mac_disable_tx_clk(void)
{
    if (phy_interface == RGMII || enable_tx_clk)
        clk_disable_unprepare(phy_tx_clk);

    clk_put(phy_tx_clk);
    clk_put(mux_tx_clk);
    clk_put(epll_clk);
    clk_put(mpll_clk);
}

static int jz_mac_init(struct platform_device *pdev, void *priv)
{
    int ret;
    unsigned int cpm_mphyc;

    cpm_mphyc = 0;
    if (phy_interface == RGMII)
        cpm_mphyc |= MAC_INTERFACT_RGMII;
    else
        cpm_mphyc |= MAC_INTERFACT_RMII;

    if (rx_clk_delay) {
        cpm_mphyc |= (((rx_clk_delay - 1) & MAC_RGMII_RX_DELAY_MASK) << MAC_RGMII_RX_DELAY);
        cpm_mphyc |= (MAC_RGMII_DELAY_SELECT << MAC_RGMII_RX_SEL_DELAY);
    }

    if (tx_clk_delay){
        cpm_mphyc |= (((tx_clk_delay - 1) & MAC_RGMII_TX_DELAY_MASK) << MAC_RGMII_TX_DELAY);
        cpm_mphyc |= (MAC_RGMII_DELAY_SELECT << MAC_RGMII_TX_SEL_DELAY);
    }

    cpm_outl(cpm_mphyc, CPM_MAC_PHY1);

    cpm_mphyc = cpm_inl(CPM_MAC_PHY1);
    cpm_mphyc |= MAC_SOFT_RESET_MAC;
    cpm_outl(cpm_mphyc, CPM_MAC_PHY1);
    usleep_range(1000, 1000);
    cpm_mphyc = cpm_inl(CPM_MAC_PHY1);
    cpm_mphyc &= ~MAC_SOFT_RESET_MAC;
    cpm_outl(cpm_mphyc, CPM_MAC_PHY1);
    usleep_range(1000, 1000);

    mac_gpio_select();

    ret = mac_request_pins(mac_crtl_gpio_mask);
    if (ret) {
        printk(KERN_ERR "%s: init mac1 gpio failure\n", __func__);
        goto err_request_pins;
    }

    gpio_port_set_func(MAC_CRLT_PORT, mac_crtl_gpio_mask, MAC_CRLT_FUNC | GPIO_PULL_HIZ);

    if (gpio_is_valid(mac1_phy_reset_gpio)) {
        ret = gpio_request(mac1_phy_reset_gpio, "mac1_phy_reset");
        if (ret) {
            printk(KERN_ERR "%s: phy reset gpio request failure\n", __func__);
            goto err_phy_reset_gpio;
        }
        gpio_direction_output(mac1_phy_reset_gpio, !phy_reset_level);
    }

    ret = mac_enable_phy_clk();
    if (ret) {
        printk(KERN_ERR "%s: enable phy clk failure\n", __func__);
        goto err_enable_phy_clk;
    }

    ret = mac_enable_tx_clk();
    if (ret)
        goto err_enable_tx_clk;

    return 0;

err_enable_tx_clk:
    mac_disable_phy_clk();
err_enable_phy_clk:
    if (gpio_is_valid(mac1_phy_reset_gpio))
        gpio_free(mac1_phy_reset_gpio);
err_phy_reset_gpio:
    mac_free_gpio(mac_crtl_gpio_mask);
err_request_pins:

    return ret;
}

static void jz_mac_exit(struct platform_device *pdev, void *priv)
{
    mac_disable_tx_clk();
    mac_disable_phy_clk();

    if (gpio_is_valid(mac1_phy_reset_gpio))
        gpio_free(mac1_phy_reset_gpio);

    mac_free_gpio(mac_crtl_gpio_mask);
}

/*
 *
 * 1000M full demo
 * static u16 virtual_phy_reg[MII_NCONFIG + 1] = {
 *     [MII_BMCR] = BMCR_SPEED1000 | BMCR_FULLDPLX | BMCR_ANENABLE,
 *     [MII_BMSR] = BMSR_LSTATUS | BMSR_ANEGCAPABLE | BMSR_ANEGCOMPLETE | BMSR_ESTATEN,
 *     [MII_PHYSID1] = 0xAAAA,
 *     [MII_PHYSID2] = 0xBBBB,
 *     [MII_CTRL1000] = ADVERTISE_1000FULL,
 *     [MII_STAT1000] = LPA_1000FULL,
 *     [MII_ESTATUS] = ESTATUS_1000_TFULL,
 * };
 *
 * 1000M half demo
 * static u16 virtual_phy_reg[MII_NCONFIG + 1] = {
 *     [MII_BMCR] = BMCR_SPEED1000 | BMCR_ANENABLE,
 *     [MII_BMSR] = BMSR_LSTATUS | BMSR_ANEGCAPABLE | BMSR_ANEGCOMPLETE | BMSR_ESTATEN,
 *     [MII_PHYSID1] = 0xAAAA,
 *     [MII_PHYSID2] = 0xBBBB,
 *     [MII_CTRL1000] = ADVERTISE_1000HALF,
 *     [MII_STAT1000] = LPA_1000HALF,
 *     [MII_ESTATUS] = ESTATUS_1000_THALF,
 * };
 *
 * 100M full demo
 * static u16 virtual_phy_reg[MII_NCONFIG + 1] = {
 *     [MII_BMCR] = BMCR_SPEED100 | BMCR_FULLDPLX | BMCR_ANENABLE,
 *     [MII_BMSR] = BMSR_LSTATUS | BMSR_ANEGCAPABLE | BMSR_ANEGCOMPLETE | BMSR_100FULL,
 *     [MII_PHYSID1] = 0xAAAA,
 *     [MII_PHYSID2] = 0xBBBB,
 *     [MII_ADVERTISE] = ADVERTISE_100FULL | ADVERTISE_CSMA,
 *     [MII_LPA] = LPA_100FULL,
 * };
 *
 * 100M half demo
 * static u16 virtual_phy_reg[MII_NCONFIG + 1] = {
 *     [MII_BMCR] = BMCR_SPEED100 | BMCR_ANENABLE,
 *     [MII_BMSR] = BMSR_LSTATUS | BMSR_ANEGCAPABLE | BMSR_ANEGCOMPLETE | BMSR_100HALF,
 *     [MII_PHYSID1] = 0xAAAA,
 *     [MII_PHYSID2] = 0xBBBB,
 *     [MII_ADVERTISE] = ADVERTISE_100HALF | ADVERTISE_CSMA,
 *     [MII_LPA] = LPA_100HALF,
 * };
 *
 * 10M full demo
 * static u16 virtual_phy_reg[MII_NCONFIG + 1] = {
 *     [MII_BMCR] = BMCR_FULLDPLX | BMCR_ANENABLE,
 *     [MII_BMSR] = BMSR_LSTATUS | BMSR_ANEGCAPABLE | BMSR_ANEGCOMPLETE | BMSR_10FULL,
 *     [MII_PHYSID1] = 0xAAAA,
 *     [MII_PHYSID2] = 0xBBBB,
 *     [MII_ADVERTISE] = ADVERTISE_10FULL | ADVERTISE_CSMA,
 *     [MII_LPA] = LPA_10FULL,
 * };
 *
 * 10M half demo
 * static u16 virtual_phy_reg[MII_NCONFIG + 1] = {
 *     [MII_BMCR] = BMCR_ANENABLE,
 *     [MII_BMSR] = BMSR_LSTATUS | BMSR_ANEGCAPABLE | BMSR_ANEGCOMPLETE | BMSR_10HALF,
 *     [MII_PHYSID1] = 0xAAAA,
 *     [MII_PHYSID2] = 0xBBBB,
 *     [MII_ADVERTISE] = ADVERTISE_10HALF | ADVERTISE_CSMA,
 *     [MII_LPA] = LPA_10HALF,
 * };
 * 
 */

#define VIRTUAL_PHY_ID 1

static u16 virtual_rgmii_phy_reg[MII_NCONFIG + 1] = {
    [MII_BMCR] = BMCR_SPEED1000 | BMCR_FULLDPLX | BMCR_ANENABLE,
    [MII_BMSR] = BMSR_LSTATUS | BMSR_ANEGCAPABLE | BMSR_ANEGCOMPLETE | BMSR_ESTATEN,
    [MII_PHYSID1] = 0xAAAA,
    [MII_PHYSID2] = 0xBBBB,
    [MII_CTRL1000] = ADVERTISE_1000FULL,
    [MII_STAT1000] = LPA_1000FULL,
    [MII_ESTATUS] = ESTATUS_1000_TFULL,
};

static u16 virtual_rmii_phy_reg[MII_NCONFIG + 1] = {
    [MII_BMCR] = BMCR_SPEED100 | BMCR_FULLDPLX | BMCR_ANENABLE,
    [MII_BMSR] = BMSR_LSTATUS | BMSR_ANEGCAPABLE | BMSR_ANEGCOMPLETE | BMSR_100FULL,
    [MII_PHYSID1] = 0xAAAA,
    [MII_PHYSID2] = 0xBBBB,
    [MII_ADVERTISE] = ADVERTISE_100FULL | ADVERTISE_CSMA,
    [MII_LPA] = LPA_100FULL,
};

static int stmmac_virtual_phy_read(void *priv, int phy_id, int regnum)
{
    if (phy_id == VIRTUAL_PHY_ID) {
        if (phy_interface == RGMII)
            return virtual_rgmii_phy_reg[regnum];
        else if (phy_interface == RMII)
            return virtual_rmii_phy_reg[regnum];
    }

    return 0xFFFF;
}

static int stmmac_virtual_phy_write(void *priv, int phy_id, int regnum, u16 val)
{
    return 0;
}

static struct stmmac_virtual_phy stmmac_virtual_phy = {
    .read = stmmac_virtual_phy_read,
    .write = stmmac_virtual_phy_write,
};

static int mac_phy_reset(void *priv)
{
    if (gpio_is_valid(mac1_phy_reset_gpio)) {
        /* reset phy */
        gpio_direction_output(mac1_phy_reset_gpio, phy_reset_level);
        usleep_range(phy_reset_time_us, phy_reset_time_us);

        gpio_direction_output(mac1_phy_reset_gpio, !phy_reset_level);
        usleep_range(phy_reset_time_us, phy_reset_time_us);
    }

    return 0;
}

static struct stmmac_mdio_bus_data stmmac_mdio_bus_data = {
    .phy_reset = &mac_phy_reset,
    .phy_mask = 0,
};

static void jz_fix_mac_speed(void *priv, unsigned int speed)
{
    if (phy_interface == RGMII) {
        unsigned long rate = 125000000;
        unsigned long tmp;

        switch (speed) {
        case SPEED_1000:
            rate = 125000000;
            clk_set_parent(mux_tx_clk, mpll_clk);
            break;
        case SPEED_100:
            rate = 25000000;
            clk_set_parent(mux_tx_clk, epll_clk);
            break;
        case SPEED_10:
            rate = 2500000;
            clk_set_parent(mux_tx_clk, epll_clk);
            break;
        default:
            printk(KERN_ERR "%s: invalid speed %u\n", __func__, speed);
            break;
        }

        clk_set_rate(phy_tx_clk, rate);
        tmp = clk_get_rate(phy_tx_clk);
        if (tmp != rate)
            printk(KERN_ERR "%s: clk_set_rate error, need rate %ld, real rate %ld\n", __func__, rate, tmp);
    }

}

static struct plat_stmmacenet_data jz_mii_bus_device = {
    .bus_id = 1,
    .phy_addr = -1,
    .mdio_bus_data = &stmmac_mdio_bus_data,
    .maxmtu = 1500,
    .clk_delay_us = 0,
    .init = jz_mac_init,
    .exit = jz_mac_exit,
    .fix_mac_speed  = jz_fix_mac_speed,
};

static void jz_mac_device_release(struct device *dev)
{

}

static struct resource jz_mac_resources[] = {
    {
        .start  = JZ_MAC1_BASE_ADDR,
        .end    = JZ_MAC1_END_ADDR,
        .flags  = IORESOURCE_MEM,
    },
    {
        .start = IRQ_GMAC1,
        .end   = IRQ_GMAC1,
        .flags = IORESOURCE_IRQ,
    },
};

static struct platform_device jz_mac1_device = {
    .name     = STMMAC_RESOURCE_NAME,
    .id       = 1,
    .dev = {
        .platform_data = &jz_mii_bus_device,
        .release       = jz_mac_device_release,
    },
    .num_resources     = ARRAY_SIZE(jz_mac_resources),
    .resource          = jz_mac_resources,
};

int platform_device_register_mac1(void)
{
    if (mac_enable) {
        stmmac_mdio_bus_data.phy_mask = phy_mask;
        if (phy_interface == RGMII) {
            jz_mii_bus_device.interface = PHY_INTERFACE_MODE_RGMII;
            jz_mii_bus_device.max_speed = 1000;
        } else if (phy_interface == RMII) {
            jz_mii_bus_device.interface = PHY_INTERFACE_MODE_RMII;
            jz_mii_bus_device.max_speed = 100;
            jz_mii_bus_device.clk_delay_us = tx_clk_enable_delay;
        } else {
            return -1;
        }

        if (enable_virtual_phy) {
            stmmac_mdio_bus_data.virtual_phy = &stmmac_virtual_phy;
        }

        return platform_device_register(&jz_mac1_device);
    }
        return 0;
}

void platform_device_unregister_mac1(void)
{
    if (mac_enable)
        platform_device_unregister(&jz_mac1_device);
}