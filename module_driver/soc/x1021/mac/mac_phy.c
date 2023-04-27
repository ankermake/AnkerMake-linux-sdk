#include <linux/kernel.h>
#include <linux/errno.h>
#include <linux/unistd.h>
#include <linux/interrupt.h>
#include <linux/init.h>
#include <linux/netdevice.h>
#include <linux/etherdevice.h>
#include <linux/module.h>
#include <linux/mii.h>
#include <linux/ethtool.h>
#include <linux/phy.h>
#include <linux/device.h>
#include <linux/types.h>
#include <linux/fs.h>

static int test_mode = 0;
module_param(test_mode, int, S_IRUGO | S_IWUSR);

static int ingenic_config_init(struct phy_device *phydev)
{
    int val;
    int timeout = 10;
    u32 features;

    val = phy_read(phydev, MII_BMCR);
    val |= BMCR_RESET;
    phy_write(phydev, MII_BMCR, val);

    do {
        if (--timeout == 0) {
            printk(KERN_ERR "mac phy reset timeout\n");
            return val;
        }

        msleep(1);
        val = phy_read(phydev, MII_BMCR);
    } while(val & BMCR_RESET);

    features = (SUPPORTED_TP | SUPPORTED_MII
            | SUPPORTED_AUI | SUPPORTED_FIBRE |
            SUPPORTED_BNC);

    val = phy_read(phydev, MII_BMSR);
    if (val < 0)
        return val;

    if (val & BMSR_ANEGCAPABLE)
        features |= SUPPORTED_Autoneg;

    if (val & BMSR_100FULL)
        features |= SUPPORTED_100baseT_Full;
    if (val & BMSR_100HALF)
        features |= SUPPORTED_100baseT_Half;
    if (val & BMSR_10FULL)
        features |= SUPPORTED_10baseT_Full;
    if (val & BMSR_10HALF)
        features |= SUPPORTED_10baseT_Half;

    phydev->supported = features;
    phydev->advertising = features;

    if (unlikely(test_mode)) {
        switch (test_mode) {
            case 1:
                phy_write(phydev, 0x00, 0x2100);
                phy_write(phydev, 0x11, 0x04);
                printk("mac phy enter 100M TX eye diagram test mode!!!\n");
                break;
            case 2:
                phy_write(phydev, 0x00, 0x2100);
                phy_write(phydev, 0x11, 0x44);
                printk("mac phy enter 100M RX eye diagram test mode!!!\n");
                break;
            default:
                printk("module_param\tvalue\tfunction\n");
                printk("test_mode\t0\tnormal mode\n");
                printk("test_mode\t1\tmac phy 100M TX eye diagram test mode\n");
                printk("test_mode\t2\tmac phy 100M RX eye diagram test mode\n");
                break;
        }
    }

    return 0;
}

static int inegneic_config_aneg(struct phy_device *phydev)
{

    if (unlikely(test_mode))
        return 0;

    return genphy_config_aneg(phydev);
}


static struct phy_driver ingenic_phy_driver = {
    .name           = "X1021_MAC_PHY",
    .phy_id         = 0x001cc816,
    .phy_id_mask    = 0xffffffff,
    .features       = 0,
    .config_init    = ingenic_config_init,
    .config_aneg    = inegneic_config_aneg,
    .read_status    = genphy_read_status,
    .suspend        = genphy_suspend,
    .resume         = genphy_resume,
    .driver         = { .owner = THIS_MODULE,},
};

static int __init ingenic_phy_init(void)
{
    return phy_driver_register(&ingenic_phy_driver);
}

static void __exit ingenic_phy_exit(void)
{
    phy_driver_unregister(&ingenic_phy_driver);
}

module_init(ingenic_phy_init);
module_exit(ingenic_phy_exit);

MODULE_DESCRIPTION("X1021 SoC mac phy driver");
MODULE_LICENSE("GPL");