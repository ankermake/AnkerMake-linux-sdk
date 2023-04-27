#include <linux/kernel.h>
#include <linux/slab.h>
#include <linux/platform_device.h>
#include <linux/phy.h>
#include <linux/mfd/syscon.h>
#include <linux/regmap.h>
#include <linux/clk.h>
#include <linux/of.h>
#include <linux/of_net.h>

#include <bit_field.h>

#include <soc/irq.h>
#include <soc/cpm.h>
#include <utils/gpio.h>

#include "platfrom_stmmac.h"
#include "stmmac.h"

#define JZ_MAC_BASE_ADDR   0x134B0000
#define JZ_MAC_END_ADDR    0x134BFFFF

#define MAC_INTERFACE_MII_RATE      25000000

#define MAC_PHY_INTERFACE_MII_MODE   (0 << 0)
#define MAC_SOFT_RESET_MAC           (1 << 3)
#define MAC_PHY_CLK_MODE_SELECT      (1 << 21)
#define MAC_INNERAL_PHY_ENABLE       (1 << 22)
#define MAC_SELECT_INNERAL_PHY       (1 << 23)

#define MAC_PHY_PARAM_INFO          0xc8007016

#define MAC_LED_PORT               GPIO_PORT_B
#define MAC_LED_FUNC               GPIO_FUNC_2
#define MAC_LED_GPIO(pin)          GPIO_PB(pin)

#define MAC_TX_LED          7, 7
#define MAC_RX_LED          16,16
#define MAC_LINK_LED        15,15
#define MAC_DUPLEX_LED      14, 14
#define MAC_10M_SPEED_LED   6, 6
#define MAC_100M_SPEED_LED  13, 13


static int mac_tx_led;
static int mac_rx_led;
static int mac_link_led;
static int mac_duplex_led;
static int mac_10m_speed_led;
static int mac_100m_speed_led;
module_param_named(mac_tx_led, mac_tx_led, uint, 0644);
module_param_named(mac_rx_led, mac_rx_led, uint, 0644);
module_param_named(mac_link_led, mac_link_led, uint, 0644);
module_param_named(mac_duplex_led, mac_duplex_led, uint, 0644);
module_param_named(mac_10m_speed_led, mac_10m_speed_led, uint, 0644);
module_param_named(mac_100m_speed_led, mac_100m_speed_led, uint, 0644);

static struct clk *phy_clk;
static unsigned long mac_led_gpio_mask = 0;

static void mac_free_gpio(unsigned long pins)
{
    int i;

    for (i = 0; i < 32; i++) {
        if (pins & (1 << i))
            gpio_free(MAC_LED_GPIO(i));
    }
}

static int mac_request_pins(unsigned long pins)
{
    int i, ret;
    unsigned long req_pins = 0;

    for (i = 0; i < 32; i++) {
        if (mac_led_gpio_mask & (1 << i)) {
            ret = gpio_request(MAC_LED_GPIO(i), "mac_led_gpio");
            if (ret) {
                mac_free_gpio(req_pins);
                printk(KERN_ERR "request %d gpio fail\n", MAC_LED_GPIO(i));
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

    phy_clk = clk_get(NULL, "cgu_macphy");
    if (IS_ERR(phy_clk)) {
        printk(KERN_ERR "%s: phy get clk failure\n", __func__);
        return -1;
    }

    clk_set_rate(phy_clk, MAC_INTERFACE_MII_RATE);
    clk_enable(phy_clk);

    cpm_outl(MAC_PHY_PARAM_INFO, CPM_MACPHYP);

    mac_phy_reg = 0;
    mac_phy_reg |= MAC_SELECT_INNERAL_PHY;
    mac_phy_reg |= MAC_PHY_CLK_MODE_SELECT;
    mac_phy_reg |= MAC_PHY_INTERFACE_MII_MODE;
    cpm_outl(mac_phy_reg, CPM_MACPHYC);

    mac_phy_reg = cpm_inl(CPM_MACPHYC);
    mac_phy_reg |= MAC_SOFT_RESET_MAC;
    cpm_outl(mac_phy_reg, CPM_MACPHYC);

    usleep_range(1000, 1000);

    mac_phy_reg = cpm_inl(CPM_MACPHYC);
    mac_phy_reg &= ~MAC_SOFT_RESET_MAC;
    mac_phy_reg |= MAC_INNERAL_PHY_ENABLE;
    cpm_outl(mac_phy_reg, CPM_MACPHYC);

    usleep_range(1000, 1000);

    set_bit_field(&mac_led_gpio_mask, MAC_TX_LED, mac_tx_led);
    set_bit_field(&mac_led_gpio_mask, MAC_RX_LED, mac_rx_led);
    set_bit_field(&mac_led_gpio_mask, MAC_LINK_LED, mac_link_led);
    set_bit_field(&mac_led_gpio_mask, MAC_DUPLEX_LED, mac_duplex_led);
    set_bit_field(&mac_led_gpio_mask, MAC_10M_SPEED_LED, mac_10m_speed_led);
    set_bit_field(&mac_led_gpio_mask, MAC_100M_SPEED_LED, mac_100m_speed_led);

    ret = mac_request_pins(mac_led_gpio_mask);
    if (ret) {
        clk_disable(phy_clk);
        clk_put(phy_clk);
        printk(KERN_ERR "%s: init mac gpio failure\n", __func__);
        return ret;
    }

    gpio_port_set_func(MAC_LED_PORT, mac_led_gpio_mask, MAC_LED_FUNC);


    return 0;
}

static void jz_mac_exit(struct platform_device *pdev, void *priv)
{
    mac_free_gpio(mac_led_gpio_mask);
    clk_disable(phy_clk);
    clk_put(phy_clk);
}

static struct stmmac_mdio_bus_data stmmac_mdio_bus_data = {
    .phy_mask = 0xfffffffe,
};

static struct plat_stmmacenet_data jz_mii_bus_device = {
    .bus_id = 0,
    .phy_addr = -1,
    .interface = PHY_INTERFACE_MODE_MII,
    .mdio_bus_data = &stmmac_mdio_bus_data,
    .max_speed = 100,
    .maxmtu = 1500,
    .init = jz_mac_init,
    .exit = jz_mac_exit,
};

static struct resource jz_mac_resources[] = {
    {
        .start    = JZ_MAC_BASE_ADDR,
        .end    = JZ_MAC_END_ADDR,
        .flags    = IORESOURCE_MEM,
    },
    {
        .start = IRQ_GMAC,
        .end   = IRQ_GMAC,
        .flags = IORESOURCE_IRQ,
    },
};

void jz_mac_device_release(struct device *dev)
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
    return platform_device_register(&jz_mac_device);
}

void platform_device_unregister_mac(void)
{
    platform_device_unregister(&jz_mac_device);
}
