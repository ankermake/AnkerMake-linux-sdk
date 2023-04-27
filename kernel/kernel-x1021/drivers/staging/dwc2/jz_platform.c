/*
 * jz4780_platform.c - JZ4780 DWC2 controller platform driver
 *
 * Copyright (C) Matthijs Kooijman <matthijs@stdin.nl>
 * Copyright (C) 2014 Imagination Technologies Ltd.
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
#include <linux/gpio.h>
#include <linux/module.h>
#include <linux/slab.h>
#include <linux/device.h>
#include <linux/dma-mapping.h>
#include <linux/of_device.h>
#include <linux/platform_device.h>
#include <linux/clk.h>
#include <linux/regulator/consumer.h>
#include <linux/jz_dwc.h>

#include "core.h"
#include "hcd.h"
#include "debug.h"
#include "jz_platform.h"

static int __dwc2_lowlevel_hw_enable(struct dwc2_hsotg *hsotg);
static int __dwc2_lowlevel_hw_disable(struct dwc2_hsotg *hsotg);


#define USBRESET_DETECT_TIME	0x96
#define OTG_REG_GUSBCFG		0xb3500000

static const char dwc2_driver_name[] = "jz-dwc2";

static const struct dwc2_core_params params_jz4780 = {
	.otg_cap			= 2,
	.otg_ver			=  1,
	.dma_enable			=  1,	/* DMA Enabled */
#ifdef CONFIG_USB_DWC2_DMA_BUFFER_MODE
	.dma_desc_enable		=  0,
#endif
#ifdef CONFIG_USB_DWC2_DMA_DESCRIPTOR_MODE
	.dma_desc_enable		=  1,
#endif

	.speed				= -1,
	.enable_dynamic_fifo		= -1,
	.en_multiple_tx_fifo		= -1,
	.host_rx_fifo_size		=  1024, /* 1024 DWORDs */
	.host_nperio_tx_fifo_size	=  1024, /* 1024 DWORDs */
	.host_perio_tx_fifo_size	=  1024, /* 1024 DWORDs */
	.max_transfer_size		= -1,
	.max_packet_count		= -1,
	.host_channels			= -1,
	.phy_type			= -1,
	.phy_utmi_width			= 16,
	.phy_ulpi_ddr			= -1,
	.phy_ulpi_ext_vbus		= -1,
	.i2c_enable			= -1,
	.ulpi_fs_ls			= -1,
	.host_support_fs_ls_low_power	= -1,
	.host_ls_low_power_phy_clk	= -1,
	.ts_dline			= -1,
	.reload_ctl			= -1,
	.ahbcfg				= -1,
	.uframe_sched			= 0,
	.external_id_pin_ctl		= 1,
	.hibernation			=  0,
};

struct jzdwc_pin __attribute__((weak)) dwc2_drvvbus_pin = {
	.num = -1,
	.enable_level = -1,
};

struct jzdwc_pin __attribute__((weak)) dwc2_dete_pin = {
	.num = -1,
	.enable_level = -1,
};


struct jzdwc_pin __attribute__((weak)) dwc2_id_pin = {
	.num = -1,
	.enable_level = -1,
};


/**
 *  jz_set_vbus
 *  interface: control dwc2_drvvbus_pin level
 */
void jz_set_vbus(struct dwc2_hsotg *hsotg, int is_on)
{

	struct jz_otg_info *jz_info =  container_of(hsotg,struct jz_otg_info, hsotg);

	if(jz_info->drvvbus_pin->num < 0)
		return;

	if (is_on && !gpio_get_value(jz_info->drvvbus_pin->num)){
		gpio_direction_output(jz_info->drvvbus_pin->num,
				jz_info->drvvbus_pin->enable_level == HIGH_ENABLE);
	}else if (!is_on && gpio_get_value(jz_info->drvvbus_pin->num)){
		gpio_direction_output(jz_info->drvvbus_pin->num,
				jz_info->drvvbus_pin->enable_level == LOW_ENABLE);
	}
}


#ifndef CONFIG_BOARD_HAS_NO_DETE_FACILITY
/**
 *  jz_set_idpin_interrupt
 *  interface: control dwc2_id_pin  interrupt
 */
void jz_set_idpin_interrupt(struct dwc2_hsotg *hsotg, int is_on)
{

	struct jz_otg_info *jz_info =  container_of(hsotg,struct jz_otg_info, hsotg);

	if (IS_ERR_OR_NULL(jz_info->id_pin))
		return;

	if(jz_info->id_pin->num < 0)
		return;

	if (is_on && !jz_info->id_irq_enable){
		enable_irq(gpio_to_irq(jz_info->id_pin->num));
		jz_info->id_irq_enable = 1;
	}else if(!is_on && jz_info->id_irq_enable ){
		disable_irq_nosync(gpio_to_irq(jz_info->id_pin->num));
		jz_info->id_irq_enable = 0;
	}
}

/**
 *  jz_set_detepin_interrupt
 *  interface: control dwc2_dete_pin  interrupt
 */
void jz_set_detepin_interrupt(struct dwc2_hsotg *hsotg, int is_on)
{

	struct jz_otg_info *jz_info =  container_of(hsotg,struct jz_otg_info, hsotg);

	if (IS_ERR_OR_NULL(jz_info->dete_pin))
		return;

	if(jz_info->dete_pin->num < 0)
		return;

	if (is_on && !jz_info->dete_irq_enable){
		enable_irq(gpio_to_irq(jz_info->dete_pin->num));
		jz_info->dete_irq_enable = 1;
	}else if (!is_on && jz_info->dete_irq_enable){
		disable_irq_nosync(gpio_to_irq(jz_info->dete_pin->num));
		jz_info->dete_irq_enable = 0;
	}
}


#define USB_DETE_VBUS	0x1
#define USB_DETE_ID	0x2
static void usb_dete_id_work(struct work_struct *work)
{
	struct jz_otg_info *jz_info =
		container_of(work, struct jz_otg_info, work.work);
	int value = 0;
	struct dwc2_hsotg *hsotg = &jz_info->hsotg;
	int status = 0;
	unsigned long flags = 0;

	u32 gotgctl;

	if (!IS_ERR_OR_NULL(jz_info->dete_pin)){
		if(gpio_is_valid(jz_info->dete_pin->num)){
			value = gpio_get_value(jz_info->dete_pin->num);
			if(value == jz_info->dete_pin->enable_level)
				status &= USB_DETE_VBUS;       /* connect PC */
			else
				status |= USB_DETE_VBUS;       /* disconnect PC */
		}
	}


	if (!IS_ERR_OR_NULL(jz_info->id_pin)){
		if(gpio_is_valid(jz_info->id_pin->num))
			value = gpio_get_value(jz_info->id_pin->num);
			if(value == jz_info->id_pin->enable_level)
				status &= ~USB_DETE_ID;      /* host mode */
			else
				status |= USB_DETE_ID;       /* device mode*/
	}

	switch (status){
		case 0:
			/*JZ_CONNECT_PC and JZ_CONTROLLOR_HOST*/
			spin_lock_irqsave(&hsotg->lock, flags);
			if(!hsotg->ll_hw_enabled)
				dwc2_lowlevel_hw_enable(hsotg);
			spin_unlock_irqrestore(&hsotg->lock, flags);
			break;
		case 1:
			/*JZ_DISCONNECT_PC and JZ_CONTROLLOR_HOST*/
			spin_lock_irqsave(&hsotg->lock, flags);
			if(!hsotg->ll_hw_enabled)
				dwc2_lowlevel_hw_enable(hsotg);
			spin_unlock_irqrestore(&hsotg->lock, flags);
			break;
		case 2:
			/*JZ_CONNECT_PC and JZ_CONTROLLOR_DEVIVE*/
			spin_lock_irqsave(&hsotg->lock, flags);
			if(!hsotg->ll_hw_enabled)
				dwc2_lowlevel_hw_enable(hsotg);
			spin_unlock_irqrestore(&hsotg->lock, flags);
			break;
		case 3:
			/*JZ_CONNECT_DISCONNET_PC and JZ_CONTROLLOR_DEVIVE*/
			spin_lock(&hsotg->lock);
			dwc2_hsotg_disconnect(hsotg);
			spin_unlock(&hsotg->lock);

			/*clear for otg protocol*/
			if (hsotg->op_state == OTG_STATE_B_HOST) {
				hsotg->op_state = OTG_STATE_B_PERIPHERAL;
			/*
			* OTG_STATE_B_PERIPHERAL:  the On-The-Go B-device acts as the peripheral, and responds to requests from the A-device
			* OTG_STATE_B_HOST      :  the On-The-Go B-device acts as the peripheral, the B-device issues a bus reset, and starts generating SOF’s.
			**/
			} else {
				hsotg->lx_state = DWC2_L0;
			}
			gotgctl = dwc2_readl(hsotg->regs + GOTGCTL);
			gotgctl &= ~GOTGCTL_DEVHNPEN;			/* Device HNP Disabled (Host Negotiation Protocol) */
			dwc2_writel(gotgctl, hsotg->regs + GOTGCTL);

			spin_lock_irqsave(&hsotg->lock, flags);
			if(hsotg->ll_hw_enabled)
				dwc2_lowlevel_hw_disable(hsotg);
			spin_unlock_irqrestore(&hsotg->lock, flags);
			break;
		default:
			printk("%s---do not define id_pin or dete pin, --status =  0x%x\n", __func__, status);
			break;
	}



}

static irqreturn_t usb_dete_id_irq_handler(int irq, void *dev_id)
{
	struct jz_otg_info *jz_info = (struct jz_otg_info *)dev_id;

	schedule_delayed_work(&jz_info->work, msecs_to_jiffies(100));
	return IRQ_HANDLED;
}
#endif /* CONFIG_BOARD_HAS_NO_DETE_FACILITY */

static int __dwc2_lowlevel_hw_enable(struct dwc2_hsotg *hsotg)
{
	int ret = 0;
	int usbphy_modal = 0;
	struct jz_otg_info *jz_info =  container_of(hsotg,struct jz_otg_info, hsotg);

	#if defined(CONFIG_USB_DWC2_PERIPHERAL)
		usbphy_modal =  DEVICE_ONLY,
	#else
		usbphy_modal =  DUAL_MODE;
	#endif

	jz_otg_phy_init(usbphy_modal);
	jz_otg_phy_suspend(0);
	if (jz_info->otg_gate_clk) {
		ret = clk_prepare_enable(jz_info->otg_gate_clk);
		if (ret){
			dev_err(hsotg->dev, "Failed to enable otg1 clock:%d\n", ret);
			return ret;
		}
	}

	if (jz_info->otg_div_clk) {
		ret = clk_prepare_enable(jz_info->otg_div_clk);
		if (ret) {
			dev_err(hsotg->dev, "Failed to enable otgphy clk:%d\n", ret);
			return ret;
		}

		ret = clk_set_rate(jz_info->otg_div_clk, 24000000);
		if (ret) {
			dev_err(hsotg->dev, "Failed to set usb otg phy clk rate: %d\n",
				ret);
			return ret;
		}
	}

	return ret;
}

static int __dwc2_lowlevel_hw_disable(struct dwc2_hsotg *hsotg)
{
	int ret = 0;
	struct jz_otg_info *jz_info =  container_of(hsotg,struct jz_otg_info, hsotg);

	if (jz_info->otg_div_clk)
		clk_disable_unprepare(jz_info->otg_div_clk);

	if (jz_info->otg_gate_clk)
		clk_disable_unprepare(jz_info->otg_gate_clk);

	jz_otg_phy_suspend(1);
	jz_otg_phy_powerdown();
	return ret;
}



int dwc2_lowlevel_hw_enable(struct dwc2_hsotg *hsotg)
{
	int ret = __dwc2_lowlevel_hw_enable(hsotg);
	if (ret == 0)
		hsotg->ll_hw_enabled = true;
	return 0;
}

int dwc2_lowlevel_hw_disable(struct dwc2_hsotg *hsotg)
{
	int ret = __dwc2_lowlevel_hw_disable(hsotg);
	if (ret == 0)
		hsotg->ll_hw_enabled = false;
	return 0;
}

static int dwc2_lowlevel_hw_init(struct dwc2_hsotg *hsotg)
{
	int retval = 0;
	struct jz_otg_info *jz_info =  container_of(hsotg,struct jz_otg_info, hsotg);

	hsotg->phyif = GUSBCFG_PHYIF16;

	jz_info->otg_div_clk = devm_clk_get(hsotg->dev, "cgu_usb");
	if (IS_ERR(jz_info->otg_div_clk)) {
		jz_info->otg_div_clk = NULL;
	}

	jz_info->otg_gate_clk = devm_clk_get(hsotg->dev, "otg1");
	if (IS_ERR(jz_info->otg_gate_clk)) {
		retval = PTR_ERR(jz_info->otg_gate_clk);
		dev_err(hsotg->dev, "Failed to get clock:%d\n", retval);
		return retval;
	}

	jz_info->drvvbus_pin = &dwc2_drvvbus_pin;
	if(gpio_is_valid(jz_info->drvvbus_pin->num)) {
		retval = devm_gpio_request_one(hsotg->dev, jz_info->drvvbus_pin->num,
			GPIOF_DIR_OUT, "drvvbus_pin");
		if(retval < 0){
			dev_err(hsotg->dev, "Failed to register drvvbus: %d\n", retval);
			return retval;
		}
	}

#ifndef CONFIG_BOARD_HAS_NO_DETE_FACILITY
	INIT_DELAYED_WORK(&jz_info->work, usb_dete_id_work);
	schedule_delayed_work(&jz_info->work, msecs_to_jiffies(5000));   /* for inital status dete*/
	jz_info->dete_pin = &dwc2_dete_pin;
	if(gpio_is_valid(jz_info->dete_pin->num)) {
		retval = devm_gpio_request_one(hsotg->dev, jz_info->dete_pin->num,
				GPIOF_DIR_IN, "detect_pin");
		if(!retval){
			retval = devm_request_irq(hsotg->dev,
				gpio_to_irq(jz_info->dete_pin->num),
				usb_dete_id_irq_handler,
				IRQF_TRIGGER_RISING|IRQF_TRIGGER_FALLING|IRQF_ONESHOT,
				"usb-detect",
				(void *)jz_info);



			if(retval < 0){
				dev_err(hsotg->dev, "Failed to requeset dete irq: %d\n", retval);
				return retval;
			}
			jz_info->dete_irq_enable = 1;


		}else{
			dev_err(hsotg->dev, "Failed to register detepin: %d\n", retval);
			return retval;
		}

	}

	jz_info->id_pin = &dwc2_id_pin;
	if(gpio_is_valid(jz_info->id_pin->num)) {
		retval = devm_gpio_request_one(hsotg->dev, jz_info->id_pin->num,
				GPIOF_DIR_IN, "id_pin");
		if(!retval){
			retval = devm_request_irq(hsotg->dev,
				gpio_to_irq(jz_info->id_pin->num),
				usb_dete_id_irq_handler,
				IRQF_TRIGGER_RISING|IRQF_TRIGGER_FALLING|IRQF_ONESHOT,
				"usb-id",
				(void *)jz_info);


			if(retval < 0){
				dev_err(hsotg->dev, "Failed to requeset id irq: %d\n", retval);
				return retval;
			}
			jz_info->id_irq_enable = 1;


		}else{
			dev_err(hsotg->dev, "Failed to register idpin: %d\n", retval);
			return retval;
		}

	}
#endif /* CONFIG_BOARD_HAS_NO_DETE_FACILITY */

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
	struct jz_otg_info *jz_otg = platform_get_drvdata(dev);
	struct dwc2_hsotg *hsotg = &jz_otg->hsotg;
	unsigned long flags = 0;

	spin_lock_irqsave(&hsotg->lock, flags);
	if (hsotg->ll_hw_enabled)
		dwc2_lowlevel_hw_disable(hsotg);
	spin_unlock_irqrestore(&hsotg->lock, flags);

	dwc2_hcd_remove(&jz_otg->hsotg);

	return 0;
}

static const struct of_device_id dwc2_of_match_table[] = {
	{ .compatible = "ingenic,jz4780-otg", .data = &params_jz4780 },
	{},
};
MODULE_DEVICE_TABLE(of, dwc2_of_match_table);

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
	const struct of_device_id *match;
	struct dwc2_core_params *params;
	struct dwc2_core_params defparams;
	struct jz_otg_info *jz_otg;
	struct dwc2_hsotg *hsotg;
	struct resource *res;
	int retval;
	int irq;
	u32 reg;
	unsigned long flags = 0;

	if (usb_disabled())
		return -ENODEV;

	match = of_match_device(dwc2_of_match_table, &dev->dev);
	if (match && match->data) {
		params = (struct dwc2_core_params *)match->data;
	} else {
		/* Default all params to autodetect */
		dwc2_set_all_params(&defparams, -1);
		defparams = params_jz4780;
		params = &defparams;
	}

	jz_otg = devm_kzalloc(&dev->dev, sizeof(*jz_otg), GFP_KERNEL);
	if (!jz_otg)
		return -ENOMEM;

	jz_otg->id_irq_enable = 0;
	jz_otg->dete_irq_enable = 0;

	hsotg = &jz_otg->hsotg;
	hsotg->dev = &dev->dev;
	spin_lock_init(&hsotg->lock);

	/*
	 * Use reasonable defaults so platforms don't have to provide these.
	 */
	if (!dev->dev.dma_mask)
		dev->dev.dma_mask = &dev->dev.coherent_dma_mask;
	retval = dma_set_coherent_mask(&dev->dev, DMA_BIT_MASK(32));
	if (retval)
		return retval;

	irq = platform_get_irq(dev, 0);
	if (irq < 0) {
		dev_err(&dev->dev, "missing IRQ resource\n");
		return irq;
	}

	dev_dbg(hsotg->dev, "registering common handler for irq%d\n",
		irq);
	retval = devm_request_irq(hsotg->dev, irq,
				  dwc2_handle_common_intr, IRQF_SHARED,
				  dev_name(hsotg->dev), hsotg);
	if (retval)
		return retval;

	res = platform_get_resource(dev, IORESOURCE_MEM, 0);
	hsotg->regs = devm_ioremap_resource(&dev->dev, res);
	if (IS_ERR(hsotg->regs))
		return PTR_ERR(hsotg->regs);

	dev_dbg(&dev->dev, "mapped PA %08lx to VA %p\n",
		(unsigned long)res->start, hsotg->regs);


	spin_lock_irqsave(&hsotg->lock, flags);
	retval = dwc2_lowlevel_hw_init(hsotg);
	if (retval){
		spin_unlock_irqrestore(&hsotg->lock, flags);
		return retval;
	}

	jz_otg_ctr_reset();
	retval = dwc2_lowlevel_hw_enable(hsotg);
	if (retval){
		spin_unlock_irqrestore(&hsotg->lock, flags);
		return retval;
	}
	spin_unlock_irqrestore(&hsotg->lock, flags);


	reg = dwc2_readl((unsigned int __iomem *)OTG_REG_GUSBCFG);
	dwc2_writel(reg | 0xc, (unsigned int __iomem *)OTG_REG_GUSBCFG);

#if defined(CONFIG_USB_DWC2_HOST)
	hsotg->dr_mode = USB_DR_MODE_HOST;
#elif defined(CONFIG_USB_DWC2_PERIPHERAL)
	hsotg->dr_mode = USB_DR_MODE_PERIPHERAL;
#else
	hsotg->dr_mode = USB_DR_MODE_OTG;
#endif

	mutex_init(&hsotg->init_mutex);

	/* Detect config values from hardware */
	retval = dwc2_get_hwparams(hsotg);
	if (retval)
		return retval;

	hsotg->core_params = devm_kzalloc(&dev->dev,
				sizeof(*hsotg->core_params), GFP_KERNEL);
	if (!hsotg->core_params)
		return -ENOMEM;

	dwc2_set_all_params(hsotg->core_params, -1);

	/* Validate parameter values */
	dwc2_set_parameters(hsotg, params);

	if (hsotg->dr_mode != USB_DR_MODE_HOST) {
		retval = dwc2_gadget_init(hsotg, irq);
		if (retval)
			return retval;
		hsotg->gadget_enabled = 1;
	}

	if (hsotg->dr_mode != USB_DR_MODE_PERIPHERAL) {
		retval = dwc2_hcd_init(hsotg, irq);
		if (retval) {
			if (hsotg->gadget_enabled)
				dwc2_hsotg_remove(hsotg);
			return retval;
		}
		hsotg->hcd_enabled = 1;
	}

	platform_set_drvdata(dev, jz_otg);

	dwc2_debugfs_init(hsotg);

	return retval;
}

static int __maybe_unused dwc2_suspend(struct device *dev)
{
	struct dwc2_hsotg *hsotg = dev_get_drvdata(dev);
	int ret = 0;
	struct jz_otg_info *jz_info =  container_of(hsotg,struct jz_otg_info, hsotg);
	unsigned long flags = 0;

	if (dwc2_is_device_mode(hsotg)){
#if IS_ENABLED(CONFIG_USB_DWC2_PERIPHERAL) || IS_ENABLED(CONFIG_USB_DWC2_DUAL)
#ifndef CONFIG_BOARD_HAS_NO_DETE_FACILITY
		jz_set_detepin_interrupt(hsotg, 0);
		jz_set_idpin_interrupt(hsotg, 0);
#endif
		/* git rid of judge vbus work */
		cancel_delayed_work_sync(&jz_info->work);

		ret = dwc2_hsotg_suspend(hsotg);
		if(ret)
			return ret;
#endif /* IS_ENABLED(CONFIG_USB_DWC2_PERIPHERAL) || IS_ENABLED(CONFIG_USB_DWC2_DUAL) */
	}else{
#if IS_ENABLED(CONFIG_USB_DWC2_HOST) || IS_ENABLED(CONFIG_USB_DWC2_DUAL)
		int count = 100;
		jz_set_vbus(hsotg, 0);   /*wait for dwc2_handle_disconnet_intr**/
		do{
			udelay(1);
			count--;
			if(count <= 0){
				printk("don't wait dwc2_handle_disconnet_init");
				break;
			}
		}while(hsotg->flags.b.port_connect_status);
#endif /* IS_ENABLED(CONFIG_USB_DWC2_HOST) || IS_ENABLED(CONFIG_USB_DWC2_DUAL) */
	}

	spin_lock_irqsave(&hsotg->lock, flags);
	if (hsotg->ll_hw_enabled)
		ret = dwc2_lowlevel_hw_disable(hsotg);
	spin_unlock_irqrestore(&hsotg->lock, flags);
	return ret;
}

static int __maybe_unused dwc2_resume(struct device *dev)
{
	struct dwc2_hsotg *hsotg = dev_get_drvdata(dev);
	struct jz_otg_info *jz_info =  container_of(hsotg,struct jz_otg_info, hsotg);
	int ret = 0;
	unsigned long flags = 0;

	spin_lock_irqsave(&hsotg->lock, flags);
	if (!hsotg->ll_hw_enabled) {
		ret = dwc2_lowlevel_hw_enable(hsotg);
		if (ret){
			spin_unlock_irqrestore(&hsotg->lock, flags);
			return ret;
		}
	}
	spin_unlock_irqrestore(&hsotg->lock, flags);


	if (dwc2_is_device_mode(hsotg)){
#if IS_ENABLED(CONFIG_USB_DWC2_PERIPHERAL) || IS_ENABLED(CONFIG_USB_DWC2_DUAL)
		ret = dwc2_hsotg_resume(hsotg);
		if (ret)
			return ret;
#ifndef CONFIG_BOARD_HAS_NO_DETE_FACILITY
		jz_set_idpin_interrupt(hsotg, 1);
		jz_set_detepin_interrupt(hsotg, 1);
		/* need to judge vbus */
		schedule_delayed_work(&jz_info->work, msecs_to_jiffies(1000));
#endif

#endif /* IS_ENABLED(CONFIG_USB_DWC2_PERIPHERAL) || IS_ENABLED(CONFIG_USB_DWC2_DUAL) */
	}else{
#if IS_ENABLED(CONFIG_USB_DWC2_HOST) || IS_ENABLED(CONFIG_USB_DWC2_DUAL)
		jz_set_vbus(hsotg, 1);
		dwc2_disable_global_interrupts(hsotg);
		dwc2_core_init(hsotg, true, -1, true);
		dwc2_enable_global_interrupts(hsotg);
		dwc2_host_start(hsotg);

#endif /* IS_ENABLED(CONFIG_USB_DWC2_HOST) || IS_ENABLED(CONFIG_USB_DWC2_DUAL) */
	}
	return ret;
}

static const struct dev_pm_ops dwc2_dev_pm_ops = {
	SET_SYSTEM_SLEEP_PM_OPS(dwc2_suspend, dwc2_resume)
};

static struct platform_driver dwc2_platform_driver = {
	.driver = {
		.name = dwc2_driver_name,
		.of_match_table = dwc2_of_match_table,
		.pm = &dwc2_dev_pm_ops,
	},
	.probe = dwc2_driver_probe,
	.remove = dwc2_driver_remove,
};

module_platform_driver(dwc2_platform_driver);

MODULE_DESCRIPTION("JZ4780 DWC2 controller platform driver");
MODULE_AUTHOR("Zubair Lutfullah Kakakhel <Zubair.Kakakhel@imgtec.com>");
MODULE_LICENSE("Dual BSD/GPL");
