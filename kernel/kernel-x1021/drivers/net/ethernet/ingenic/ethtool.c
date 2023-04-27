/*
 * JZ4775 On-Chip GMAC ethtool Driver
 *
 * Copyright (C) 2013 - 2014  Ingenic Semiconductor CO., LTD.
 *
 * Licensed under the GPL-2 or later.
 */

/* ethtool support for Jz4775 GMAC */
#include <linux/netdevice.h>
#include <linux/ethtool.h>
#include <linux/mii.h>

#include "jz_mac_v12/jz_mac_v12.h"

/**
 * jz_Gamc_get_settings - Exported for Ethtool to query PHY setting
 * @ndev:	pointer of net DEVICE structure
 * @cmd:
 */
static int jz_Gamc_get_settings(struct net_device *ndev, struct ethtool_cmd *cmd)
{
	struct jz_mac_local *jz_local = netdev_priv(ndev);
	return mii_ethtool_gset(&jz_local->mii, cmd);
}

/**
 * jz_Gamc_set_settings - Exported for Ethtool to set PHY setting
 * @ndev:	pointer of net DEVICE structure
 * @cmd:
 */
static int jz_Gamc_set_settings(struct net_device *ndev, struct ethtool_cmd *cmd)
{
        struct jz_mac_local *jz_local = netdev_priv(ndev);
	return mii_ethtool_sset(&jz_local->mii, cmd);
}

/**
 * jz_Gamc_get_drvinfo - Exported for Ethtool to query the driver version
 * @ndev:	pointer of net DEVICE structure
 * @info:
 */
static void jz_Gamc_get_drvinfo(struct net_device *ndev, struct ethtool_drvinfo *info)
{
	/* Inherit standard device info */
	strncpy (info->driver, "jz4775 Gmac", sizeof info->driver);
	strncpy (info->version, "v1.0.1", sizeof info->version);
}

/**
 * jz_Gamc_nway_reset - Exported for Ethtool to restart PHY autonegotiation
 * @ndev:	pointer of net DEVICE structure
 */
static int jz_Gamc_nway_reset(struct net_device *ndev)
{
        struct jz_mac_local *jz_local = netdev_priv(ndev);
        return mii_nway_restart(&jz_local->mii);
}


static struct ethtool_ops jz_Gamc_ethtool_ops = {
	.get_settings	= jz_Gamc_get_settings,
	.set_settings	= jz_Gamc_set_settings,
	.get_drvinfo	= jz_Gamc_get_drvinfo,
	.nway_reset	= jz_Gamc_nway_reset,
	.get_link	= ethtool_op_get_link,
};

void jzmac_set_ethtool_ops(struct net_device *netdev)
{
	SET_ETHTOOL_OPS(netdev, &jz_Gamc_ethtool_ops);
}
