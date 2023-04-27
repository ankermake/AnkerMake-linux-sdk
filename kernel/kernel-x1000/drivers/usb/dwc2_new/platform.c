// SPDX-License-Identifier: (GPL-2.0+ OR BSD-3-Clause)
/*
 * platform.c - DesignWare HS OTG Controller platform driver
 *
 * Copyright (C) Matthijs Kooijman <matthijs@stdin.nl>
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions, and the following disclaimer,
 *    without modification.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. The names of the above-listed copyright holders may not be used
 *    to endorse or promote products derived from this software without
 *    specific prior written permission.
 *
 * ALTERNATIVELY, this software may be distributed under the terms of the
 * GNU General Public License ("GPL") as published by the Free Software
 * Foundation; either version 2 of the License, or (at your option) any
 * later version.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS
 * IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO,
 * THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR
 * CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
 * EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
 * PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
 * PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF
 * LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING
 * NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
 * SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/slab.h>
#include <linux/clk.h>
#include <linux/device.h>
#include <linux/dma-mapping.h>
#include <linux/of_device.h>
#include <linux/mutex.h>
#include <linux/platform_device.h>
#include <linux/reset.h>

#include "core.h"
#include "hcd.h"

#define DWC2_DRIVER_NAME	"dwc2"
#define DWC2_VBUS_REG_NAME	"vbus"

typedef enum otg_mode {
	DEVICE_ONLY = 1,
	HOST_ONLY = 2,
	DUAL_MODE = 3,
} otg_mode_t;

static otg_mode_t dwc2_usb_mode(void) {
	if (IS_ENABLED(CONFIG_USB_DWC2_NEW_DUAL_ROLE))
		return DUAL_MODE;
	else if (IS_ENABLED(CONFIG_USB_DWC2_NEW_HOST))
		return HOST_ONLY;
	else
		return DEVICE_ONLY;
}

extern void jz_otg_phy_init(otg_mode_t mode);
extern void jz_otg_phy_suspend(int suspend);
extern int  jz_otg_phy_is_suspend(void);
extern void jz_otg_phy_powerdown(void);
extern void jz_otg_ctr_reset(void);

#define OTG_CLK_NAME		"otg1"
#define CGU_USB_CLK_NAME	"cgu_usb"
#define USB_PWR_CLK_NAME	"pwc_usb"

/*
 * Check the dr_mode against the module configuration and hardware
 * capabilities.
 *
 * The hardware, module, and dr_mode, can each be set to host, device,
 * or otg. Check that all these values are compatible and adjust the
 * value of dr_mode if possible.
 *
 *                      actual
 *    HW  MOD dr_mode   dr_mode
 *  ------------------------------
 *   HST  HST  any    :  HST
 *   HST  DEV  any    :  ---
 *   HST  OTG  any    :  HST
 *
 *   DEV  HST  any    :  ---
 *   DEV  DEV  any    :  DEV
 *   DEV  OTG  any    :  DEV
 *
 *   OTG  HST  any    :  HST
 *   OTG  DEV  any    :  DEV
 *   OTG  OTG  any    :  dr_mode
 */
static int dwc2_get_dr_mode(struct dwc2_hsotg *hsotg)
{
	enum usb_dr_mode mode;

	if (IS_ENABLED(CONFIG_USB_DWC2_NEW_HOST))
		hsotg->dr_mode = USB_DR_MODE_HOST;
	else if (IS_ENABLED(CONFIG_USB_DWC2_NEW_PERIPHERAL))
		hsotg->dr_mode = USB_DR_MODE_PERIPHERAL;
	else
		hsotg->dr_mode = USB_DR_MODE_OTG;

	mode = hsotg->dr_mode;

	if (dwc2_hw_is_device(hsotg)) {
		if (IS_ENABLED(CONFIG_USB_DWC2_NEW_HOST)) {
			dev_err(hsotg->dev,
				"Controller does not support host mode.\n");
			return -EINVAL;
		}
		mode = USB_DR_MODE_PERIPHERAL;
	} else if (dwc2_hw_is_host(hsotg)) {
		if (IS_ENABLED(CONFIG_USB_DWC2_NEW_PERIPHERAL)) {
			dev_err(hsotg->dev,
				"Controller does not support device mode.\n");
			return -EINVAL;
		}
		mode = USB_DR_MODE_HOST;
	}

	if (mode != hsotg->dr_mode) {
		dev_err(hsotg->dev,
			 "Configuration mismatch. dr_mode forced to %s\n",
			mode == USB_DR_MODE_HOST ? "host" : "device");

		hsotg->dr_mode = mode;
	}

	return 0;
}


static int __dwc2_lowlevel_hw_enable(struct dwc2_hsotg *hsotg)
{
	int ret = 0;

	if (hsotg->pwr_clk)
		clk_prepare_enable(hsotg->pwr_clk);

	if (hsotg->cgu_clk)
		clk_prepare_enable(hsotg->cgu_clk);

	clk_prepare_enable(hsotg->clk);

	/* usb_phy_init */
	jz_otg_phy_init(dwc2_usb_mode());

	if (hsotg->uphy)
		ret = usb_phy_init(hsotg->uphy);

	return ret;
}

/**
 * dwc2_lowlevel_hw_enable - enable platform lowlevel hw resources
 * @hsotg: The driver state
 *
 * A wrapper for platform code responsible for controlling
 * low-level USB platform resources (phy, clock, regulators)
 */
int dwc2_lowlevel_hw_enable(struct dwc2_hsotg *hsotg)
{
	int ret = __dwc2_lowlevel_hw_enable(hsotg);

	if (ret == 0)
		hsotg->ll_hw_enabled = true;
	return ret;
}

static int __dwc2_lowlevel_hw_disable(struct dwc2_hsotg *hsotg)
{
	if (hsotg->uphy)
		usb_phy_shutdown(hsotg->uphy);

	/* usb_phy_shutdown */
	jz_otg_phy_powerdown();

	clk_disable_unprepare(hsotg->clk);
	if (hsotg->cgu_clk)
		clk_disable_unprepare(hsotg->cgu_clk);
	if (hsotg->pwr_clk)
		clk_disable_unprepare(hsotg->pwr_clk);

	return 0;
}

/**
 * dwc2_lowlevel_hw_disable - disable platform lowlevel hw resources
 * @hsotg: The driver state
 *
 * A wrapper for platform code responsible for controlling
 * low-level USB platform resources (phy, clock, regulators)
 */
int dwc2_lowlevel_hw_disable(struct dwc2_hsotg *hsotg)
{
	int ret = __dwc2_lowlevel_hw_disable(hsotg);

	if (ret == 0)
		hsotg->ll_hw_enabled = false;
	return ret;
}

static int dwc2_lowlevel_hw_init(struct dwc2_hsotg *hsotg)
{
	int ret;

	hsotg->uphy = devm_usb_get_phy(hsotg->dev, USB_PHY_TYPE_USB2);
	if (IS_ERR(hsotg->uphy)) {
		ret = PTR_ERR(hsotg->uphy);
		switch (ret) {
		case -ENODEV:
		case -ENXIO:
			hsotg->uphy = NULL;
			break;
		case -EPROBE_DEFER:
			return ret;
		default:
			dev_err(hsotg->dev, "error getting usb phy %d\n", ret);
			return ret;
		}
	}

	/* Clock */
	hsotg->clk = devm_clk_get(hsotg->dev, OTG_CLK_NAME);
	if (IS_ERR(hsotg->clk)) {
		dev_err(hsotg->dev, "clk gate get error\n");
		return PTR_ERR(hsotg->clk);
	}

	hsotg->cgu_clk = devm_clk_get(hsotg->dev, CGU_USB_CLK_NAME);
	if (IS_ERR(hsotg->cgu_clk)) {
		hsotg->cgu_clk = NULL;
		dev_warn(hsotg->dev, "no cgu clk check it\n");
	} else {
		clk_set_rate(hsotg->cgu_clk, 24000000);
	}

	hsotg->pwr_clk = devm_clk_get(hsotg->dev, USB_PWR_CLK_NAME);
	if (IS_ERR(hsotg->pwr_clk))
		hsotg->pwr_clk = NULL;

	jz_otg_ctr_reset();

	return 0;
}

/**
 * dwc2_driver_remove() - Called when the DWC_otg core is unregistered with the
 * DWC_otg driver
 *
 * @dev: Platform device
 *
 * This routine is called, for example, when the rmmod command is executed. The
 * device may or may not be electrically present. If it is present, the driver
 * stops device processing. Any resources used on behalf of this device are
 * freed.
 */
static int dwc2_driver_remove(struct platform_device *dev)
{
	struct dwc2_hsotg *hsotg = platform_get_drvdata(dev);
	struct dwc2_gregs_backup *gr;
	int ret = 0;

	gr = &hsotg->gr_backup;

	/* Exit Hibernation when driver is removed. */
	if (hsotg->hibernated) {
		if (gr->gotgctl & GOTGCTL_CURMODE_HOST)
			ret = dwc2_exit_hibernation(hsotg, 0, 0, 1);
		else
			ret = dwc2_exit_hibernation(hsotg, 0, 0, 0);

		if (ret)
			dev_err(hsotg->dev,
				"exit hibernation failed.\n");
	}

	/* Exit Partial Power Down when driver is removed. */
	if (hsotg->in_ppd) {
		ret = dwc2_exit_partial_power_down(hsotg, 0, true);
		if (ret)
			dev_err(hsotg->dev,
				"exit partial_power_down failed\n");
	}

	/* Exit clock gating when driver is removed. */
	if (hsotg->params.power_down == DWC2_POWER_DOWN_PARAM_NONE &&
	    hsotg->bus_suspended) {
		if (dwc2_is_device_mode(hsotg))
			dwc2_gadget_exit_clock_gating(hsotg, 0);
		else
			dwc2_host_exit_clock_gating(hsotg, 0);
	}

	if (hsotg->hcd_enabled)
		dwc2_hcd_remove(hsotg);
	if (hsotg->gadget_enabled)
		dwc2_hsotg_remove(hsotg);

	dwc2_drd_exit(hsotg);

	if (hsotg->ll_hw_enabled)
		dwc2_lowlevel_hw_disable(hsotg);

	return ret;
}

/**
 * dwc2_driver_shutdown() - Called on device shutdown
 *
 * @dev: Platform device
 *
 * In specific conditions (involving usb hubs) dwc2 devices can create a
 * lot of interrupts, even to the point of overwhelming devices running
 * at low frequencies. Some devices need to do special clock handling
 * at shutdown-time which may bring the system clock below the threshold
 * of being able to handle the dwc2 interrupts. Disabling dwc2-irqs
 * prevents reboots/poweroffs from getting stuck in such cases.
 */
static void dwc2_driver_shutdown(struct platform_device *dev)
{
	struct dwc2_hsotg *hsotg = platform_get_drvdata(dev);

	dwc2_disable_global_interrupts(hsotg);
	synchronize_irq(hsotg->irq);
}

/**
 * dwc2_check_core_endianness() - Returns true if core and AHB have
 * opposite endianness.
 * @hsotg:	Programming view of the DWC_otg controller.
 */
static bool dwc2_check_core_endianness(struct dwc2_hsotg *hsotg)
{
	u32 snpsid;

	snpsid = ioread32(hsotg->regs + GSNPSID);
	if ((snpsid & GSNPSID_ID_MASK) == DWC2_OTG_ID ||
	    (snpsid & GSNPSID_ID_MASK) == DWC2_FS_IOT_ID ||
	    (snpsid & GSNPSID_ID_MASK) == DWC2_HS_IOT_ID)
		return false;
	return true;
}

/**
 * Check core version
 *
 * @hsotg: Programming view of the DWC_otg controller
 *
 */
int dwc2_check_core_version(struct dwc2_hsotg *hsotg)
{
	struct dwc2_hw_params *hw = &hsotg->hw_params;

	/*
	 * Attempt to ensure this device is really a DWC_otg Controller.
	 * Read and verify the GSNPSID register contents. The value should be
	 * 0x45f4xxxx, 0x5531xxxx or 0x5532xxxx
	 */

	hw->snpsid = dwc2_readl(hsotg, GSNPSID);
	if ((hw->snpsid & GSNPSID_ID_MASK) != DWC2_OTG_ID &&
	    (hw->snpsid & GSNPSID_ID_MASK) != DWC2_FS_IOT_ID &&
	    (hw->snpsid & GSNPSID_ID_MASK) != DWC2_HS_IOT_ID) {
		dev_err(hsotg->dev, "Bad value for GSNPSID: 0x%08x\n",
			hw->snpsid);
		return -ENODEV;
	}

	dev_dbg(hsotg->dev, "Core Release: %1x.%1x%1x%1x (snpsid=%x)\n",
		hw->snpsid >> 12 & 0xf, hw->snpsid >> 8 & 0xf,
		hw->snpsid >> 4 & 0xf, hw->snpsid & 0xf, hw->snpsid);
	return 0;
}

void __iomem * devm_platform_get_and_ioremap_resource(struct platform_device *pdev,
				unsigned int index, struct resource **res)
{
	struct resource *r;

	r = platform_get_resource(pdev, IORESOURCE_MEM, index);
	if (res)
		*res = r;
	return devm_ioremap_resource(&pdev->dev, r);
}

/**
 * dwc2_driver_probe() - Called when the DWC_otg core is bound to the DWC_otg
 * driver
 *
 * @dev: Platform device
 *
 * This routine creates the driver components required to control the device
 * (core, HCD, and PCD) and initializes the device. The driver components are
 * stored in a dwc2_hsotg structure. A reference to the dwc2_hsotg is saved
 * in the device private data. This allows the driver to access the dwc2_hsotg
 * structure on subsequent calls to driver methods for this device.
 */
static int dwc2_driver_probe(struct platform_device *dev)
{
	struct dwc2_hsotg *hsotg;
	struct resource *res;
	int retval;

	hsotg = devm_kzalloc(&dev->dev, sizeof(*hsotg), GFP_KERNEL);
	if (!hsotg)
		return -ENOMEM;

	hsotg->dev = &dev->dev;

	/*
	 * Use reasonable defaults so platforms don't have to provide these.
	 */
	if (!dev->dev.dma_mask)
		dev->dev.dma_mask = &dev->dev.coherent_dma_mask;
	retval = dma_set_coherent_mask(&dev->dev, DMA_BIT_MASK(32));
	if (retval) {
		dev_err(&dev->dev, "can't set coherent DMA mask: %d\n", retval);
		return retval;
	}

	res = platform_get_resource(dev, IORESOURCE_MEM, 0);
	if (!res) {
		dev_err(&dev->dev, "missing memory resource\n");
		return -ENODEV;
	}

	hsotg->regs = devm_ioremap_resource(&dev->dev, res);
	if (IS_ERR(hsotg->regs)) {
		dev_err(&dev->dev, "fail ioremap memory resource\n");
		return PTR_ERR(hsotg->regs);
	}

	dev_dbg(&dev->dev, "mapped PA %08lx to VA %p\n",
		(unsigned long)res->start, hsotg->regs);

	retval = dwc2_lowlevel_hw_init(hsotg);
	if (retval)
		return retval;

	spin_lock_init(&hsotg->lock);

	hsotg->irq = platform_get_irq(dev, 0);
	if (hsotg->irq < 0)
		return hsotg->irq;

	dev_dbg(hsotg->dev, "registering common handler for irq%d\n",
		hsotg->irq);
	retval = devm_request_irq(hsotg->dev, hsotg->irq,
				  dwc2_handle_common_intr, IRQF_SHARED,
				  dev_name(hsotg->dev), hsotg);
	if (retval)
		return retval;

	hsotg->vbus_supply = devm_regulator_get(hsotg->dev, DWC2_VBUS_REG_NAME);
	if (IS_ERR(hsotg->vbus_supply))
		hsotg->vbus_supply = NULL;

	retval = dwc2_lowlevel_hw_enable(hsotg);
	if (retval)
		return retval;

	hsotg->needs_byte_swap = dwc2_check_core_endianness(hsotg);

	retval = dwc2_get_dr_mode(hsotg);
	if (retval)
		goto error;

	hsotg->need_phy_for_wake = false;

	/*
	 * Before performing any core related operations
	 * check core version.
	 */
	retval = dwc2_check_core_version(hsotg);
	if (retval)
		goto error;

	/*
	 * Reset before dwc2_get_hwparams() then it could get power-on real
	 * reset value form registers.
	 */
	retval = dwc2_core_reset(hsotg, false);
	if (retval)
		goto error;

	/* Detect config values from hardware */
	retval = dwc2_get_hwparams(hsotg);
	if (retval)
		goto error;

	/*
	 * For OTG cores, set the force mode bits to reflect the value
	 * of dr_mode. Force mode bits should not be touched at any
	 * other time after this.
	 */
	dwc2_force_dr_mode(hsotg);

	retval = dwc2_init_params(hsotg);
	if (retval)
		goto error;

	retval = dwc2_drd_init(hsotg);
	if (retval) {
		if (retval)
			dev_err(hsotg->dev, "failed to initialize dual-role\n");
		goto error;
	}

	if (hsotg->dr_mode != USB_DR_MODE_HOST) {
		retval = dwc2_gadget_init(hsotg);
		if (retval)
			goto error_drd;
		hsotg->gadget_enabled = 1;
	}

	/*
	 * If we need PHY for wakeup we must be wakeup capable.
	 * When we have a device that can wake without the PHY we
	 * can adjust this condition.
	 */
	if (hsotg->need_phy_for_wake)
		device_set_wakeup_capable(&dev->dev, true);

	if (hsotg->dr_mode != USB_DR_MODE_PERIPHERAL) {
		retval = dwc2_hcd_init(hsotg);
		if (retval) {
			if (hsotg->gadget_enabled)
				dwc2_hsotg_remove(hsotg);
			goto error_drd;
		}
		hsotg->hcd_enabled = 1;
	}

	platform_set_drvdata(dev, hsotg);
	hsotg->hibernated = 0;

	/* Gadget code manages lowlevel hw on its own */
	if (hsotg->dr_mode == USB_DR_MODE_PERIPHERAL)
		dwc2_lowlevel_hw_disable(hsotg);

#if IS_ENABLED(CONFIG_USB_DWC2_NEW_PERIPHERAL) || \
	IS_ENABLED(CONFIG_USB_DWC2_NEW_DUAL_ROLE)
	/* Postponed adding a new gadget to the udc class driver list */
	if (hsotg->gadget_enabled) {
		retval = usb_add_gadget_udc(hsotg->dev, &hsotg->gadget);
		if (retval) {
			dwc2_hsotg_remove(hsotg);
			goto error_hcd;
		}
	}
#endif /* CONFIG_USB_DWC2_NEW_PERIPHERAL || CONFIG_USB_DWC2_NEW_DUAL_ROLE */

	return 0;

#if IS_ENABLED(CONFIG_USB_DWC2_NEW_PERIPHERAL) || \
	IS_ENABLED(CONFIG_USB_DWC2_NEW_DUAL_ROLE)
error_hcd:
	if (hsotg->hcd_enabled)
		dwc2_hcd_remove(hsotg);
#endif

error_drd:
	dwc2_drd_exit(hsotg);

error:
	if (hsotg->dr_mode != USB_DR_MODE_PERIPHERAL)
		dwc2_lowlevel_hw_disable(hsotg);
	return retval;
}

static int __maybe_unused dwc2_suspend(struct device *dev)
{
	struct dwc2_hsotg *dwc2 = dev_get_drvdata(dev);
	bool is_device_mode = dwc2_is_device_mode(dwc2);
	int ret = 0;

	if (is_device_mode)
		dwc2_hsotg_suspend(dwc2);

	if (dwc2->ll_hw_enabled &&
	    (is_device_mode || dwc2_host_can_poweroff_phy(dwc2))) {
		ret = __dwc2_lowlevel_hw_disable(dwc2);
		dwc2->phy_off_for_suspend = true;
	}

	return ret;
}

static int __maybe_unused dwc2_resume(struct device *dev)
{
	struct dwc2_hsotg *dwc2 = dev_get_drvdata(dev);
	int ret = 0;

	if (dwc2->phy_off_for_suspend && dwc2->ll_hw_enabled) {
		ret = __dwc2_lowlevel_hw_enable(dwc2);
		if (ret)
			return ret;
	}
	dwc2->phy_off_for_suspend = false;

	if (dwc2_is_device_mode(dwc2))
		ret = dwc2_hsotg_resume(dwc2);

	return ret;
}

static const struct dev_pm_ops dwc2_dev_pm_ops = {
	SET_SYSTEM_SLEEP_PM_OPS(dwc2_suspend, dwc2_resume)
};

static struct platform_driver dwc2_platform_driver = {
	.driver = {
		.name = DWC2_DRIVER_NAME,
		.pm = &dwc2_dev_pm_ops,
	},
	.probe = dwc2_driver_probe,
	.remove = dwc2_driver_remove,
	.shutdown = dwc2_driver_shutdown,
};

module_platform_driver(dwc2_platform_driver);
