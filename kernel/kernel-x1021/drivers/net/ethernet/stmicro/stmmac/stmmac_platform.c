/*******************************************************************************
  This contains the functions to handle the platform driver.

  Copyright (C) 2007-2011  STMicroelectronics Ltd

  This program is free software; you can redistribute it and/or modify it
  under the terms and conditions of the GNU General Public License,
  version 2, as published by the Free Software Foundation.

  This program is distributed in the hope it will be useful, but WITHOUT
  ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
  FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License for
  more details.

  You should have received a copy of the GNU General Public License along with
  this program; if not, write to the Free Software Foundation, Inc.,
  51 Franklin St - Fifth Floor, Boston, MA 02110-1301 USA.

  The full GNU General Public License is included in this distribution in
  the file called "COPYING".

  Author: Giuseppe Cavallaro <peppe.cavallaro@st.com>
*******************************************************************************/

#include <linux/platform_device.h>
#include <linux/io.h>
#include <linux/of.h>
#include <linux/of_net.h>
#include <linux/of_device.h>

#include "stmmac.h"

/**
 * stmmac_pltfr_probe
 * @pdev: platform device pointer
 * Description: platform_device probe function. It allocates
 * the necessary resources and invokes the main to init
 * the net device, register the mdio bus etc.
 */
static int stmmac_pltfr_probe(struct platform_device *pdev)
{
	int ret = 0;
	struct resource *mem, *irq;
	struct device *dev = &pdev->dev;
	void __iomem *addr = NULL;
	struct stmmac_priv *priv = NULL;
	struct plat_stmmacenet_data *plat_dat = NULL;
	const char *mac = NULL;

	mem = platform_get_resource(pdev, IORESOURCE_MEM, 0);
	irq = platform_get_resource(pdev, IORESOURCE_IRQ, 0);
	if (!mem || !irq) {
		dev_err(&pdev->dev, "no memory/irq resource for engine\n");
		return -ENXIO;
	}

	addr = devm_ioremap_resource(dev, mem);
	if (IS_ERR(addr))
		return PTR_ERR(addr);

	plat_dat = dev_get_platdata(&pdev->dev);

	/* Custom setup (if needed) */
	if (plat_dat->setup) {
		plat_dat->bsp_priv = plat_dat->setup(pdev);
		if (IS_ERR(plat_dat->bsp_priv))
			return PTR_ERR(plat_dat->bsp_priv);
	}

	/* Custom initialisation (if needed)*/
	if (plat_dat->init) {
		ret = plat_dat->init(pdev, plat_dat->bsp_priv);
		if (unlikely(ret))
			return ret;
	}

	priv = stmmac_dvr_probe(&(pdev->dev), plat_dat, addr);
	if (IS_ERR(priv)) {
		pr_err("%s: main driver probe failed", __func__);
		return PTR_ERR(priv);
	}

	/* Get MAC address if available (DT) */
	if (mac)
		memcpy(priv->dev->dev_addr, mac, ETH_ALEN);


	/* Get the MAC information */
	priv->dev->irq = irq->start;

	/*
	 * On some platforms e.g. SPEAr the wake up irq differs from the mac irq
	 * The external wake up irq can be passed through the platform code
	 * named as "eth_wake_irq"
	 *
	 * In case the wake up interrupt is not passed from the platform
	 * so the driver will continue to use the mac irq (ndev->irq)
	 */
	priv->wol_irq = priv->dev->irq;
	priv->lpi_irq = 0;

	platform_set_drvdata(pdev, priv->dev);


	pr_debug("STMMAC platform driver registration completed");

	return 0;
}

/**
 * stmmac_pltfr_remove
 * @pdev: platform device pointer
 * Description: this function calls the main to free the net resources
 * and calls the platforms hook and release the resources (e.g. mem).
 */
static int stmmac_pltfr_remove(struct platform_device *pdev)
{
	struct net_device *ndev = platform_get_drvdata(pdev);
	struct stmmac_priv *priv = netdev_priv(ndev);
	int ret = stmmac_dvr_remove(ndev);

	if (priv->plat->exit)
		priv->plat->exit(pdev, priv->plat->bsp_priv);

	if (priv->plat->free)
		priv->plat->free(pdev, priv->plat->bsp_priv);

	return ret;
}

#ifdef CONFIG_PM
static int stmmac_pltfr_suspend(struct device *dev)
{
	int ret;
	struct net_device *ndev = dev_get_drvdata(dev);
	struct stmmac_priv *priv = netdev_priv(ndev);
	struct platform_device *pdev = to_platform_device(dev);

	ret = stmmac_suspend(ndev);
	if (priv->plat->exit)
		priv->plat->exit(pdev, priv->plat->bsp_priv);

	return ret;
}

static int stmmac_pltfr_resume(struct device *dev)
{
	struct net_device *ndev = dev_get_drvdata(dev);
	struct stmmac_priv *priv = netdev_priv(ndev);
	struct platform_device *pdev = to_platform_device(dev);

	if (priv->plat->init)
		priv->plat->init(pdev, priv->plat->bsp_priv);

	return stmmac_resume(ndev);
}

#endif /* CONFIG_PM */

static SIMPLE_DEV_PM_OPS(stmmac_pltfr_pm_ops,
			stmmac_pltfr_suspend, stmmac_pltfr_resume);

struct platform_driver stmmac_pltfr_driver = {
	.probe = stmmac_pltfr_probe,
	.remove = stmmac_pltfr_remove,
	.driver = {
		   .name = STMMAC_RESOURCE_NAME,
		   .owner = THIS_MODULE,
		   .pm = &stmmac_pltfr_pm_ops,
		   },
};

MODULE_DESCRIPTION("STMMAC 10/100/1000 Ethernet PLATFORM driver");
MODULE_AUTHOR("Giuseppe Cavallaro <peppe.cavallaro@st.com>");
MODULE_LICENSE("GPL");
