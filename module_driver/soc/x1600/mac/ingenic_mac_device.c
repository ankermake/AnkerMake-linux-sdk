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
#include <soc/base.h>
#include <soc/cpm.h>

#include <utils/gpio.h>
#include <bit_field.h>

#include "platfrom_stmmac.h"
#include "stmmac.h"

#define JZ_MAC_BASE_ADDR            0x134B0000
#define JZ_MAC_END_ADDR             0x134BFFFF

#define MAC_INTERFACE_RMII_RATE     50000000

#define MAC_CRLT_PORT               GPIO_PORT_B
#define MAC_CRLT_FUNC               GPIO_FUNC_1
#define MAC_CRLT_GPIO(pin)          GPIO_PB(pin)
#define MAC_NO_MDIO_GPIO_MASK       0x07F80000
#define MAC_CRLT_GPIO_MASK          0x1FF80000

#define MAC_PHY_INTERFACE_MODE_MASK     0x7
#define MAC_PHY_INTERFACE_RMII_MODE  (4 << 0)
#define MAC_SOFT_RESET_MAC           (1 << 3)

#define CPM_MPHYC                   0xe4
#define IRQ_GMAC0                   (IRQ_INTC_BASE + 32 + 23)

static struct clk *phy_clk;
static int phy_reset_gpio;
static int phy_power_gpio;
static unsigned int phy_power_level;
static unsigned int phy_reset_level;
static unsigned int phy_reset_time_us;
static unsigned int phy_probe_mask;
static unsigned int phy_clk_delay_us;
static unsigned int enable_virtual_phy;
static unsigned int mac_crtl_gpio_mask;

module_param_gpio(phy_reset_gpio, 0444);
module_param_gpio(phy_power_gpio, 0444);
module_param_named(phy_reset_level, phy_reset_level, uint, 0644);
module_param_named(phy_power_level, phy_power_level, uint, 0644);
module_param_named(phy_reset_time_us, phy_reset_time_us, uint, 0644);
module_param_named(phy_probe_mask, phy_probe_mask, uint, 0444);
module_param_named(phy_clk_delay_us, phy_clk_delay_us, uint, 0444);
module_param_named(enable_virtual_phy, enable_virtual_phy, uint, 0444);

static void mac_gpio_select(void)
{
    if (enable_virtual_phy)
        mac_crtl_gpio_mask = MAC_NO_MDIO_GPIO_MASK;
    else
        mac_crtl_gpio_mask = MAC_CRLT_GPIO_MASK;
}

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
            ret = gpio_request(MAC_CRLT_GPIO(i), "mac_gpio");
            if (ret) {
                mac_free_gpio(req_pins);
                printk(KERN_ERR "%s: request %d gpio failure\n", __func__, MAC_CRLT_GPIO(i));
                return -1;
            }

            req_pins |= (1 << i);
        }
    }

    return 0;
}

static int jz_mac_init(struct platform_device *pdev, void *priv)
{
    int ret;
    unsigned int mac_phy_reg;

    mac_phy_reg = cpm_inl(CPM_MPHYC);
    mac_phy_reg &= ~MAC_PHY_INTERFACE_MODE_MASK;
    mac_phy_reg |= MAC_PHY_INTERFACE_RMII_MODE;
    cpm_outl(mac_phy_reg, CPM_MPHYC);

    mac_phy_reg = cpm_inl(CPM_MPHYC);
    mac_phy_reg |= MAC_SOFT_RESET_MAC;
    cpm_outl(mac_phy_reg, CPM_MPHYC);
    usleep_range(10000, 10000);
    mac_phy_reg = cpm_inl(CPM_MPHYC);
    mac_phy_reg &= ~MAC_SOFT_RESET_MAC;
    cpm_outl(mac_phy_reg, CPM_MPHYC);
    usleep_range(10000, 10000);

    phy_clk = clk_get(NULL, "div_macphy");
    if (IS_ERR(phy_clk)) {
        printk(KERN_ERR "%s: phy get clk failure\n", __func__);
        return -1;
    }

    clk_set_rate(phy_clk, MAC_INTERFACE_RMII_RATE);
    clk_prepare_enable(phy_clk);

    mac_gpio_select();

    ret = mac_request_pins(mac_crtl_gpio_mask);
    if (ret) {
        clk_disable_unprepare(phy_clk);
        clk_put(phy_clk);
        printk(KERN_ERR "%s: init mac gpio failure\n", __func__);
        return ret;
    }

    gpio_port_set_func(MAC_CRLT_PORT, mac_crtl_gpio_mask, MAC_CRLT_FUNC | GPIO_PULL_HIZ);


    if (gpio_is_valid(phy_power_gpio)) {
        ret = gpio_request(phy_power_gpio, "mac_power_reset");
        if (ret) {
            mac_free_gpio(mac_crtl_gpio_mask);
            clk_disable_unprepare(phy_clk);
            clk_put(phy_clk);
            printk(KERN_ERR "%s: phy reset gpio request failure\n", __func__);
            return ret;
        }

        gpio_direction_output(phy_power_gpio, phy_power_level);
    }

    if (gpio_is_valid(phy_reset_gpio)) {
        ret = gpio_request(phy_reset_gpio, "mac_phy_reset");
        if (ret) {
            mac_free_gpio(mac_crtl_gpio_mask);
            clk_disable_unprepare(phy_clk);
            clk_put(phy_clk);
            if (gpio_is_valid(phy_power_gpio))
                gpio_free(phy_power_gpio);
            printk(KERN_ERR "%s: phy reset gpio request failure\n", __func__);
            return ret;
        }

        gpio_direction_output(phy_reset_gpio, !phy_reset_level);
    }

    return 0;
}

static void jz_mac_exit(struct platform_device *pdev, void *priv)
{
    clk_disable_unprepare(phy_clk);
    clk_put(phy_clk);

    if (gpio_is_valid(phy_reset_gpio))
        gpio_free(phy_reset_gpio);

    if (gpio_is_valid(phy_power_gpio))
        gpio_free(phy_power_gpio);

    mac_free_gpio(mac_crtl_gpio_mask);
}

static int mac_phy_reset(void *priv)
{
    if (gpio_is_valid(phy_reset_gpio)) {
        /* reset phy */
        gpio_direction_output(phy_reset_gpio, phy_reset_level);
        usleep_range(phy_reset_time_us, phy_reset_time_us);

        gpio_direction_output(phy_reset_gpio, !phy_reset_level);
        usleep_range(phy_reset_time_us, phy_reset_time_us);
    }

    return 0;
}

/*
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

static struct stmmac_mdio_bus_data stmmac_mdio_bus_data = {
    .phy_reset = &mac_phy_reset,
    .phy_mask = 0,
};

static struct plat_stmmacenet_data jz_mii_bus_device = {
    .bus_id = 0,
    .phy_addr = -1,
    .interface = PHY_INTERFACE_MODE_RMII,
    .mdio_bus_data = &stmmac_mdio_bus_data,
    .max_speed = 100,
    .maxmtu = 1500,
    .clk_delay_us = 0,
    .init = jz_mac_init,
    .exit = jz_mac_exit,
};

static struct resource jz_mac_resources[] = {
    {
        .start  = JZ_MAC_BASE_ADDR,
        .end    = JZ_MAC_END_ADDR,
        .flags  = IORESOURCE_MEM,
    },
    {
        .start  = IRQ_GMAC0,
        .end    = IRQ_GMAC0,
        .flags  = IORESOURCE_IRQ,
    },
};

static void jz_mac_device_release(struct device *dev)
{

}

static struct platform_device jz_mac_device = {
    .name     = STMMAC_RESOURCE_NAME,
    .id       = 0,
    .dev = {
        .platform_data = &jz_mii_bus_device,
        .release       = jz_mac_device_release,
    },
    .num_resources     = ARRAY_SIZE(jz_mac_resources),
    .resource          = jz_mac_resources,
};

int platform_device_register_mac(void)
{
    stmmac_mdio_bus_data.phy_mask = phy_probe_mask;

    if (enable_virtual_phy)
        stmmac_mdio_bus_data.virtual_phy = &stmmac_virtual_phy;

    jz_mii_bus_device.clk_delay_us = phy_clk_delay_us;
    return platform_device_register(&jz_mac_device);
}

void platform_device_unregister_mac(void)
{
    platform_device_unregister(&jz_mac_device);
}
