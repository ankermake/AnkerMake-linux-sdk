#include <linux/kernel.h>
#include <linux/slab.h>
#include <linux/platform_device.h>
#include <linux/phy.h>
#include <linux/mfd/syscon.h>
#include <linux/regmap.h>
#include <linux/clk.h>
#include <linux/of.h>
#include <linux/of_net.h>
#include <linux/stmmac.h>
#include <linux/delay.h>

#include <soc/irq.h>
#include <soc/cpm.h>

#define JZ_GMAC_BASE_ADDR   0x134B0000
#define JZ_GMAC_END_ADDR    0x134BFFFF

#define MAC_PHY_INTERFACE_MII_MODE   (0)
#define MAC_PHY_INTERFACE_RMII_MODE  (1 << 2)
#define MAC_PHY_INTERFACE_MASK       (0x7 << 0)
#define MAC_SOFT_RESET_GMAC          (1 << 3)
#define MAC_PHY_CLK_MODE_SELECT      (1 << 21)
#define MAC_INNERAL_PHY_ENABLE       (1 << 22)
#define MAC_SELECT_INNERAL_PHY       (1 << 23)

#define GMAC_INTERFACE_MII_RATE     25000000
#define GMAC_INTERFACE_RMII_RATE    50000000

#define GMAC_INTERFACE_MODE         PHY_INTERFACE_MODE_MII

struct jz_dwmac {
    int interface;
    struct clk *clk;
};

static void *jz_gmac_setup(struct platform_device *pdev)
{
    struct jz_dwmac *dwmac;

    dwmac = devm_kzalloc(&pdev->dev, sizeof(*dwmac), GFP_KERNEL);
    if (!dwmac)
        return ERR_PTR(-ENOMEM);

    dwmac->interface = GMAC_INTERFACE_MODE;

    dwmac->clk = devm_clk_get(&pdev->dev, "cgu_macphy");
    if (IS_ERR(dwmac->clk))
        dwmac->clk = NULL;

    return dwmac;
}

static int jz_gmac_init(struct platform_device *pdev, void *priv)
{
    struct jz_dwmac *dwmac = priv;

    if (dwmac->interface == PHY_INTERFACE_MODE_MII) {
        clk_set_rate(dwmac->clk, GMAC_INTERFACE_MII_RATE);
    } else {
        clk_set_rate(dwmac->clk, GMAC_INTERFACE_RMII_RATE);
    }

    clk_prepare_enable(dwmac->clk);

    return 0;
}

static void jz_gmac_exit(struct platform_device *pdev, void *priv)
{
    struct jz_dwmac *dwmac = priv;

    clk_disable_unprepare(dwmac->clk);
}

static struct resource jz_mac_resources[] = {
    {
        .start    = JZ_GMAC_BASE_ADDR,
        .end    = JZ_GMAC_END_ADDR,
        .flags    = IORESOURCE_MEM,
    },
    {
        .start = IRQ_GMAC,
        .end   = IRQ_GMAC,
        .flags = IORESOURCE_IRQ,
    },
};

int gmac_phy_reset(void *priv)
{
    unsigned int mac_phy_reg;

    cpm_outl(0xc8007016, CPM_MACPHYP);

    mac_phy_reg = cpm_inl(CPM_MACPHYC);
    mac_phy_reg |= MAC_SELECT_INNERAL_PHY | MAC_INNERAL_PHY_ENABLE | MAC_PHY_CLK_MODE_SELECT;
    mac_phy_reg &= ~MAC_PHY_INTERFACE_MASK;
    mac_phy_reg |= MAC_PHY_INTERFACE_MII_MODE;
    cpm_outl(mac_phy_reg, CPM_MACPHYC);

    mac_phy_reg = cpm_inl(CPM_MACPHYC);
    mac_phy_reg |= MAC_SOFT_RESET_GMAC;
    cpm_outl(mac_phy_reg, CPM_MACPHYC);

    mdelay(1);

    mac_phy_reg = cpm_inl(CPM_MACPHYC);
    mac_phy_reg &= ~MAC_SOFT_RESET_GMAC;
    cpm_outl(mac_phy_reg, CPM_MACPHYC);

    mdelay(1);

    return 0;
}


struct stmmac_mdio_bus_data stmmac_mdio_bus_data = {
    .phy_reset = &gmac_phy_reset,
    .phy_mask = 0xfffffff0,
};

struct plat_stmmacenet_data jz_mii_bus_device = {
    .bus_id = 0,
    .phy_addr = 0,
    .interface = GMAC_INTERFACE_MODE,
    .mdio_bus_data = &stmmac_mdio_bus_data,
    .max_speed = 100,
    .maxmtu = 1500,
    .setup = jz_gmac_setup,
    .init = jz_gmac_init,
    .exit = jz_gmac_exit,
};

void jz_mac_device_release(struct device *dev){

}

struct platform_device jz_mac_device = {
    .name = "stmmaceth",
    .id        = 0,
    .dev = {
        .platform_data    = &jz_mii_bus_device,
        .release = jz_mac_device_release,
    },
    .num_resources    = ARRAY_SIZE(jz_mac_resources),
    .resource    = jz_mac_resources,
};

static int __init jz_mac_dev_init(void)
{
    int ret;

    ret = platform_device_register(&jz_mac_device);
    if (ret)
        printk(KERN_ERR "Unable to register mac device: %d\n", ret);

    return ret;
}

static void __exit jz_mac_dev_exit(void)
{
    platform_device_unregister(&jz_mac_device);
}

module_init(jz_mac_dev_init);
module_exit(jz_mac_dev_exit);
