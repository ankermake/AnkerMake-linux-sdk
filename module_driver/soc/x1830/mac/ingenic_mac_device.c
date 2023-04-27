#include <linux/kernel.h>
#include <linux/slab.h>
#include <linux/platform_device.h>
#include <linux/phy.h>
#include <linux/mfd/syscon.h>
#include <linux/regmap.h>
#include <linux/clk.h>
#include <linux/of.h>
#include <linux/of_net.h>

#include <soc/irq.h>
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
#define MAC_CRLT_GPIO_MASK               0x1efc0

#define MAC_PHY_INTERFACE_RMII_MODE  (4 << 0)
#define MAC_SOFT_RESET_MAC           (1 << 3)

static struct clk *phy_clk;

static int phy_reset_gpio;
static unsigned int phy_reset_level;
static unsigned int phy_reset_time_us;
static unsigned int phy_probe_mask;
static unsigned int phy_clk_delay_us;

module_param_gpio(phy_reset_gpio, 0444);
module_param_named(phy_reset_level, phy_reset_level, uint, 0644);
module_param_named(phy_reset_time_us, phy_reset_time_us, uint, 0644);
module_param_named(phy_probe_mask, phy_probe_mask, uint, 0444);
module_param_named(phy_clk_delay_us, phy_clk_delay_us, uint, 0444);

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

    mac_phy_reg = 0;
    mac_phy_reg |= MAC_PHY_INTERFACE_RMII_MODE;
    cpm_outl(mac_phy_reg, CPM_MPHYC);

    mac_phy_reg = cpm_inl(CPM_MPHYC);
    mac_phy_reg |= MAC_SOFT_RESET_MAC;
    cpm_outl(mac_phy_reg, CPM_MPHYC);
    usleep_range(1000, 1000);
    mac_phy_reg = cpm_inl(CPM_MPHYC);
    mac_phy_reg &= ~MAC_SOFT_RESET_MAC;
    cpm_outl(mac_phy_reg, CPM_MPHYC);
    usleep_range(1000, 1000);

    phy_clk = clk_get(NULL, "cgu_macphy");
    if (IS_ERR(phy_clk)) {
        printk(KERN_ERR "%s: phy get clk failure\n", __func__);
        return -1;
    }

    clk_set_rate(phy_clk, MAC_INTERFACE_RMII_RATE);
    clk_enable(phy_clk);

    ret = mac_request_pins(MAC_CRLT_GPIO_MASK);
    if (ret) {
        clk_disable(phy_clk);
        clk_put(phy_clk);
        printk(KERN_ERR "%s: init mac gpio failure\n", __func__);
        return ret;
    }

    gpio_port_set_func(MAC_CRLT_PORT, MAC_CRLT_GPIO_MASK, GPIO_FUNC_0 | GPIO_PULL_HIZ);

    if (gpio_is_valid(phy_reset_gpio)) {
        ret = gpio_request(phy_reset_gpio, "mac_phy_reset");
        if (ret) {
            mac_free_gpio(MAC_CRLT_GPIO_MASK);
            clk_disable(phy_clk);
            clk_put(phy_clk);
            printk(KERN_ERR "%s: phy reset gpio request failure\n", __func__);
            return ret;
        }

        gpio_direction_output(phy_reset_gpio, !phy_reset_level);
    }

    return 0;
}

static void jz_mac_exit(struct platform_device *pdev, void *priv)
{
    if (gpio_is_valid(phy_reset_gpio))
        gpio_free(phy_reset_gpio);

    mac_free_gpio(MAC_CRLT_GPIO_MASK);

    clk_disable(phy_clk);
    clk_put(phy_clk);
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
        .start  = IRQ_GMAC,
        .end    = IRQ_GMAC,
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
    jz_mii_bus_device.clk_delay_us = phy_clk_delay_us;
    return platform_device_register(&jz_mac_device);
}

void platform_device_unregister_mac(void)
{
    platform_device_unregister(&jz_mac_device);
}
