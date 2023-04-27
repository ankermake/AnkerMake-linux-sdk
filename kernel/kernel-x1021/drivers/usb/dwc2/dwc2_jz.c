#define DEBUG
#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/slab.h>
#include <linux/platform_device.h>
#include <linux/regulator/consumer.h>
#include <linux/clk.h>
#include <linux/interrupt.h>
#include <linux/dma-mapping.h>
#include <linux/usb/otg.h>
#include <linux/gpio.h>
#include <linux/delay.h>
#include <linux/wakelock.h>
#include <linux/workqueue.h>

#include <linux/jz_dwc.h>
#include <soc/base.h>

#include <linux/usb/hcd.h>
#include "core.h"
#include "gadget.h"
#include "dwc2_jz.h"
#include "host.h"

#define OTG_CLK_NAME		"otg1"
#define VBUS_REG_NAME		"vbus"
#define CGU_USB_CLK_NAME	"cgu_usb"
#define USB_PWR_CLK_NAME	"pwc_usb"

/* DWC2_USB_DETE_TIMER_INTERVAL must be longer than DWC2_HOST_ID_TIMER_INTERVAL */
#define DWC2_HOST_ID_TIMER_INTERVAL (HZ / 4)
#define DWC2_USB_DETE_TIMER_INTERVAL (HZ / 2)
struct dwc2_jz {
	struct platform_device  dwc2;
	struct device		*dev;
	struct clk		*clk;
	struct clk		*cgu_clk;
	struct clk		*pwr_clk;
	struct mutex	irq_lock;		/* protect irq op */
	struct work_struct  turn_off_work;

	/*device*/
#if DWC2_DEVICE_MODE_ENABLE
#ifndef CONFIG_BOARD_HAS_NO_DETE_FACILITY
	struct jzdwc_pin 	*dete_pin;	/* Host mode may used this pin to judge extern vbus mode
						 * Device mode may used this pin judge B-device insert */
	int 		dete_irq;
	struct delayed_work	gpio_dete_work;
#endif
	int			pullup_on;
#endif
	/*host*/
#if DWC2_HOST_MODE_ENABLE
#if defined(CONFIG_USB_DWC2_SYS_OTG_ID)
	int id_level;
	struct delayed_work	gpio_otg_id_work;
	struct wake_lock	resume_wake_lock;
#elif defined(CONFIG_USB_DWC2_GPIO_OTG_ID)
	int			id_irq;
	struct jzdwc_pin 	*id_pin;
	struct delayed_work	gpio_otg_id_work;
	struct wake_lock	resume_wake_lock;
#endif
	struct jzdwc_pin 	*drvvbus_pin;		/*Use drvvbus pin or regulator to set vbus on*/
	struct regulator 	*vbus;
	atomic_t	vbus_on;
	struct work_struct	vbus_work;
	void *work_data;
#endif
};

#define to_dwc2_jz(dwc) container_of((dwc)->pdev, struct dwc2_jz, dwc2)

void dwc2_clk_enable(struct dwc2 *dwc)
{
	struct dwc2_jz *jz = to_dwc2_jz(dwc);
	if (jz->pwr_clk)
		clk_enable(jz->pwr_clk);
        if (jz->cgu_clk)
	        clk_enable(jz->cgu_clk);
	clk_enable(jz->clk);
}

void dwc2_clk_disable(struct dwc2 *dwc)
{
	struct dwc2_jz *jz = to_dwc2_jz(dwc);

	clk_disable(jz->clk);
        if (jz->cgu_clk)
	        clk_disable(jz->cgu_clk);
	if (jz->pwr_clk)
		clk_disable(jz->pwr_clk);
}
#if DWC2_DEVICE_MODE_ENABLE
#ifndef CONFIG_BOARD_HAS_NO_DETE_FACILITY
/*usb insert detect*/
struct jzdwc_pin __attribute__((weak)) dwc2_dete_pin = {
	.num = -1,
	.enable_level = -1,
};
#endif
#endif

#if DWC2_HOST_MODE_ENABLE
#ifdef CONFIG_USB_DWC2_GPIO_OTG_ID
/*usb host plug insert detect*/
struct jzdwc_pin __attribute__((weak)) dwc2_id_pin = {
	.num = -1,
	.enable_level = -1,
};
#endif
/*usb drvvbus pin*/
struct jzdwc_pin __attribute__((weak)) dwc2_drvvbus_pin = {
	.num = -1,
	.enable_level = -1,
};
#endif

#if DWC2_DEVICE_MODE_ENABLE
#ifndef CONFIG_BOARD_HAS_NO_DETE_FACILITY
static int __dwc2_get_detect_pin_status(struct dwc2_jz *jz) {
	int insert = 0;
	if (gpio_is_valid(jz->dete_pin->num)) {
		insert = gpio_get_value(jz->dete_pin->num);
		if (jz->dete_pin->enable_level == LOW_ENABLE)
			return !insert;
	}
	return insert;
}
#endif
#endif

static int __dwc2_get_id_level(struct dwc2_jz* jz) {
	int id_level = 1;
#if defined(CONFIG_USB_DWC2_SYS_OTG_ID)
	id_level = jz->id_level;
#elif defined(CONFIG_USB_DWC2_GPIO_OTG_ID)
	if (gpio_is_valid(jz->id_pin->num)) {
		id_level = gpio_get_value(jz->id_pin->num);
		if (jz->id_pin->enable_level == HIGH_ENABLE)
			id_level = !id_level;
	}
#endif
	return id_level;
}

int dwc2_get_id_level(struct dwc2 *dwc)
{
	struct dwc2_jz *jz = to_dwc2_jz(dwc);
	return __dwc2_get_id_level(jz);
}
EXPORT_SYMBOL_GPL(dwc2_get_id_level);

void dwc2_gpio_irq_mutex_lock(struct dwc2 *dwc)
{
	struct dwc2_jz *jz = to_dwc2_jz(dwc);
	mutex_lock(&jz->irq_lock);
}
EXPORT_SYMBOL_GPL(dwc2_gpio_irq_mutex_lock);

void dwc2_gpio_irq_mutex_unlock(struct dwc2 *dwc)
{
	struct dwc2_jz *jz = to_dwc2_jz(dwc);
	mutex_unlock(&jz->irq_lock);
}
EXPORT_SYMBOL_GPL(dwc2_gpio_irq_mutex_unlock);

#if DWC2_DEVICE_MODE_ENABLE
extern void dwc2_gadget_plug_change(int plugin);

#ifndef CONFIG_BOARD_HAS_NO_DETE_FACILITY
static void gpio_usb_detect_work(struct work_struct *work)
{
	struct dwc2_jz* jz = container_of(to_delayed_work(work), struct dwc2_jz, gpio_dete_work);
	int insert = __dwc2_get_detect_pin_status(jz);
	struct dwc2 *dwc = platform_get_drvdata(&jz->dwc2);

	mutex_lock(&jz->irq_lock);

	if((dwc->lx_state == DWC_OTG_L3 && __dwc2_get_id_level(jz)) || \
			(dwc->lx_state != DWC_OTG_L3 && dwc2_is_device_mode(dwc))) {
		pr_info("DWC USB %s\n", insert ? "connect" : "disconnect");
		flush_work(&dwc->otg_id_work);
		dwc2_disable_global_interrupts(dwc);
		synchronize_irq(dwc->irq);
		dwc2_gadget_plug_change(insert);
		if (dwc->lx_state != DWC_OTG_L3)
			dwc2_enable_global_interrupts(dwc);
	}

	mutex_unlock(&jz->irq_lock);
	enable_irq(jz->dete_irq);
}

static irqreturn_t usb_detect_interrupt(int irq, void *dev_id)
{
	struct dwc2_jz	*jz = (struct dwc2_jz *)dev_id;

	disable_irq_nosync(jz->dete_irq);
	schedule_delayed_work(&jz->gpio_dete_work, DWC2_USB_DETE_TIMER_INTERVAL);
	return IRQ_HANDLED;
}
#endif
#endif /* !DWC2_DEVICE_MODE_ENABLE */

#if DWC2_HOST_MODE_ENABLE
#if defined(CONFIG_USB_DWC2_GPIO_OTG_ID) || defined(CONFIG_USB_DWC2_SYS_OTG_ID)
static void gpio_dwc2_id_status_change_work(struct work_struct *work)
{
	struct dwc2_jz* jz = container_of(to_delayed_work(work), struct dwc2_jz, gpio_otg_id_work);
	struct dwc2 *dwc = platform_get_drvdata(&jz->dwc2);
	uint32_t	 count = 0;
	unsigned long	 flags;

	mutex_lock(&jz->irq_lock);
	wake_lock(&jz->resume_wake_lock);
	dwc2_turn_on(dwc);

	flush_work(&dwc->otg_id_work);

	dwc2_disable_global_interrupts(dwc);
	synchronize_irq(dwc->irq);

	if (__dwc2_get_id_level(jz)) { /* B-Device connector (Device Mode) */
#if DWC2_DEVICE_MODE_ENABLE
		dctl_data_t	 dctl;
		dev_warn(dwc->dev, "init DWC core as B_PERIPHERAL\n");
#else
		dev_warn(dwc->dev, "DWC core A_HOST id disconnect\n");
#endif
		dwc->otg_mode = DEVICE_ONLY;
		jz_set_vbus(dwc, 0);
#if DWC2_DEVICE_MODE_ENABLE
		dwc2_spin_lock_irqsave(dwc, flags);
		dwc2_core_init(dwc);
		dwc2_spin_unlock_irqrestore(dwc, flags);

		/* Wait for switch to device mode. */
		while (!dwc2_is_device_mode(dwc)) {
			msleep(100);
			if (++count > 10000)
				break;
		}

		if (count > 10000)
			dev_warn(dwc->dev, "%s:%d Connection id status change timed out",
				__func__, __LINE__);


		dwc2_spin_lock_irqsave(dwc, flags);
		dwc->op_state = DWC2_B_PERIPHERAL;
		dwc2_device_mode_init(dwc);

		if (likely(!dwc->plugin)) {
			/* soft disconnect */
			dctl.d32 = dwc_readl(&dwc->dev_if.dev_global_regs->dctl);
			dctl.b.sftdiscon = 1;
			dwc_writel(dctl.d32, &dwc->dev_if.dev_global_regs->dctl);

			/* note that we do not close PHY, this is because after set sftdiscon=1,
			 * a session end detect interrupt will be generate
			 */
		} else {
			dctl.d32 = dwc_readl(&dwc->dev_if.dev_global_regs->dctl);
			dctl.b.sftdiscon = dwc->pullup_on ? 0 : 1;
			dwc_writel(dctl.d32, &dwc->dev_if.dev_global_regs->dctl);
		}

		dwc2_spin_unlock_irqrestore(dwc, flags);
#endif		 /* DWC2_DEVICE_MODE_ENABLE */
	} else {	      /* A-Device connector (Host Mode) */
		dwc->otg_mode = HOST_ONLY;
		dev_warn(dwc->dev, "init DWC as A_HOST\n");

		dwc2_spin_lock_irqsave(dwc, flags);
		dwc2_core_init(dwc);
		dwc2_spin_unlock_irqrestore(dwc, flags);

		while (!dwc2_is_host_mode(dwc)) {
			msleep(100);
			if (++count > 10000)
				break;
		}

		if (count > 10000)
			dev_warn(dwc->dev, "%s:%d Connection id status change timed out",
				__func__, __LINE__);

		dwc2_spin_lock_irqsave(dwc, flags);
#if !IS_ENABLED(CONFIG_USB_DWC2_HOST_ONLY)
		if (dwc->ep0state != EP0_DISCONNECTED) {
			dwc2_gadget_handle_session_end(dwc);
			dwc2_gadget_disconnect(dwc);
			dwc2_start_ep0state_watcher(dwc, 0);
		}
#endif

		dwc->op_state = DWC2_A_HOST;
		dwc->hcd->self.is_b_host = 0;
		dwc2_host_mode_init(dwc);
		dwc2_spin_unlock_irqrestore(dwc, flags);
		jz_set_vbus(dwc, 1);
		set_bit(HCD_FLAG_POLL_RH, &dwc->hcd->flags);
	}
	dwc2_enable_global_interrupts(dwc);

	mutex_unlock(&jz->irq_lock);
	wake_lock_timeout(&jz->resume_wake_lock, 3 * HZ);

#ifdef CONFIG_USB_DWC2_GPIO_OTG_ID
	enable_irq(jz->id_irq);
#endif
}

#endif

#ifdef CONFIG_USB_DWC2_GPIO_OTG_ID
static irqreturn_t usb_host_id_interrupt(int irq, void *dev_id) {
	struct dwc2_jz	*jz = (struct dwc2_jz *)dev_id;

	disable_irq_nosync(jz->id_irq);
	schedule_delayed_work(&jz->gpio_otg_id_work, DWC2_HOST_ID_TIMER_INTERVAL);

	return IRQ_HANDLED;
}
#endif

static int __dwc2_get_drvvbus_level(struct dwc2_jz *jz)
{
	int drvvbus = 0;
	if (gpio_is_valid(jz->drvvbus_pin->num)) {
		drvvbus = gpio_get_value(jz->drvvbus_pin->num);
		if (jz->drvvbus_pin->enable_level == LOW_ENABLE)
			drvvbus = !drvvbus;
	}
	return drvvbus;
}

static void dwc2_vbus_work(struct work_struct *work)
{
	struct dwc2_jz* jz = container_of(work, struct dwc2_jz, vbus_work);
	struct dwc2 *dwc = (struct dwc2 *)jz->work_data;
	int old_is_on = atomic_read(&jz->vbus_on);
	int is_on = 0, ret = 0;

	if (NULL == jz->vbus && NULL == jz->drvvbus_pin)
		return;

	is_on = old_is_on ? dwc2_is_host_mode(dwc) : 0;

	dev_info(jz->dev, "set vbus %s(%s) for %s mode\n",
			is_on ? "on" : "off",
			old_is_on ? "on" : "off",
			dwc2_is_host_mode(dwc) ? "host" : "device");

	if (jz->vbus) {
		if (is_on && !regulator_is_enabled(jz->vbus))
			ret = regulator_enable(jz->vbus);
		else if (!is_on && regulator_is_enabled(jz->vbus))
			ret = regulator_disable(jz->vbus);
		WARN(ret != 0, "dwc2 usb host ,regulator can not be used\n");
	} else {
		if (is_on && !__dwc2_get_drvvbus_level(jz))
			gpio_direction_output(jz->drvvbus_pin->num,
					jz->drvvbus_pin->enable_level == HIGH_ENABLE);
		else if (!is_on && __dwc2_get_drvvbus_level(jz))
			gpio_direction_output(jz->drvvbus_pin->num,
					jz->drvvbus_pin->enable_level == LOW_ENABLE);
	}
	return;
}

void jz_set_vbus(struct dwc2 *dwc, int is_on)
{
	struct dwc2_jz* jz = (struct dwc2_jz *)to_dwc2_jz(dwc);

	/*CHECK it lost some vbus set is ok ??*/
	atomic_set(&jz->vbus_on, !!is_on);
	jz->work_data = (void *)dwc;
	if (!work_pending(&jz->vbus_work)) {
		schedule_work(&jz->vbus_work);
		if (!in_atomic())
			flush_work(&jz->vbus_work);
	}
}
EXPORT_SYMBOL_GPL(jz_set_vbus);

static ssize_t jz_vbus_show(struct device *dev,
		struct device_attribute *attr,
		char *buf) {
	struct dwc2_jz *jz = dev_get_drvdata(dev);
	struct dwc2 *dwc = platform_get_drvdata(&jz->dwc2);
	int vbus_is_on = 0;

	if (jz->vbus) {
		if (regulator_is_enabled(jz->vbus))
			vbus_is_on = 1;
		else
			vbus_is_on = 0;
	} else if (jz->drvvbus_pin) {
		vbus_is_on = __dwc2_get_drvvbus_level(jz);
	} else {
		vbus_is_on = dwc2_is_host_mode(dwc);
	}

	return sprintf(buf, "%s\n", vbus_is_on ? "on" : "off");
}

static ssize_t jz_vbus_set(struct device *dev,
		struct device_attribute *attr,
		const char *buf, size_t count)
{
	struct dwc2_jz *jz = dev_get_drvdata(dev);
	struct dwc2 *dwc = platform_get_drvdata(&jz->dwc2);
	int is_on = 0;

	if (strncmp(buf, "on", 2) == 0)
		is_on = 1;
	jz_set_vbus(dwc, is_on);

	return count;
}

static DEVICE_ATTR(vbus, S_IWUSR | S_IRUSR,
		jz_vbus_show, jz_vbus_set);

#if defined(CONFIG_USB_DWC2_SYS_OTG_ID)
static ssize_t jz_id_show(struct device *dev, struct device_attribute *attr,
		char *buf) {
	struct dwc2_jz *jz = dev_get_drvdata(dev);

	return sprintf(buf, "%s\n",  jz->id_level ? "1" : "0");
}

static ssize_t jz_id_store(struct device *dev, struct device_attribute *attr,
		const char *buf, size_t count)
{
	int id_level = 1;
	struct dwc2_jz *jz = dev_get_drvdata(dev);

	if (strncmp(buf, "1", 1) == 0)
		id_level = 1;
	else if (strncmp(buf, "0", 1) == 0)
		id_level = 0;

	if(id_level != jz->id_level) {
		jz->id_level = id_level;
		schedule_delayed_work(&jz->gpio_otg_id_work, 0);
	}

	return count;
}

static DEVICE_ATTR(id, S_IRUSR | S_IWUSR |  S_IRGRP | S_IROTH,
		jz_id_show, jz_id_store);

#elif defined(CONFIG_USB_DWC2_GPIO_OTG_ID)
static ssize_t jz_id_show(struct device *dev,
		struct device_attribute *attr,
		char *buf) {
	struct dwc2_jz *jz = dev_get_drvdata(dev);
	int id_level = __dwc2_get_id_level(jz);

	return sprintf(buf, "%s\n", id_level ? "1" : "0");
}

static DEVICE_ATTR(id, S_IRUSR |  S_IRGRP | S_IROTH,
		jz_id_show, NULL);
#endif

#endif	/* DWC2_HOST_MODE_ENABLE */

static void dwc2_power_off(struct dwc2 *dwc)
{
	if (dwc->lx_state != DWC_OTG_L3) {
		pr_info("dwc otg power off\n");
		dwc->phy_inited = 0;
		dwc2_disable_global_interrupts(dwc);
		jz_otg_phy_suspend(1);
		jz_otg_phy_powerdown();
		dwc2_clk_disable(dwc);
		dwc->lx_state = DWC_OTG_L3;
	}
	return;
}

static void dwc2_turn_off_work(struct work_struct *work)
{
	struct dwc2_jz *jz = container_of(work, struct dwc2_jz, turn_off_work);
	struct dwc2 *dwc = platform_get_drvdata(&jz->dwc2);
	unsigned long flags;
	unsigned int id;

	mutex_lock(&jz->irq_lock);
	id = dwc2_get_id_level(dwc);
	dwc2_spin_lock_irqsave(dwc, flags);
	if (!dwc->plugin && !dwc2_has_ep_enabled(dwc) && id)
		dwc2_power_off(dwc);
	dwc2_spin_unlock_irqrestore(dwc, flags);
	mutex_unlock(&jz->irq_lock);
}
/*call with dwc2 spin lock*/
int dwc2_turn_off(struct dwc2 *dwc, bool graceful)
{
	struct dwc2_jz *jz = to_dwc2_jz(dwc);

	if (graceful)
		schedule_work(&jz->turn_off_work);
	else
		dwc2_power_off(dwc);
	return 0;
}
EXPORT_SYMBOL_GPL(dwc2_turn_off);

static void dwc2_power_on(struct dwc2 *dwc)
{
	if (dwc->lx_state == DWC_OTG_L3) {
		pr_info("power on and reinit dwc2 otg\n");
		dwc2_clk_enable(dwc);
		jz_otg_ctr_reset();
		jz_otg_phy_init(dwc2_usb_mode());
		jz_otg_phy_suspend(0);
		dwc2_core_init(dwc);
#if !IS_ENABLED(CONFIG_USB_DWC2_HOST_ONLY)
		if (dwc2_is_device_mode(dwc))
			dwc2_device_mode_init(dwc);
#endif
		dwc->lx_state = DWC_OTG_L0;
		dwc2_enable_global_interrupts(dwc);
	}
}

/*call with dwc2 spin lock*/
int dwc2_turn_on(struct dwc2* dwc)
{
	dwc2_power_on(dwc);
	return 0;
}
EXPORT_SYMBOL_GPL(dwc2_turn_on);

static struct attribute *dwc2_jz_attributes[] = {
#if DWC2_HOST_MODE_ENABLE
	&dev_attr_vbus.attr,
#if defined(CONFIG_USB_DWC2_GPIO_OTG_ID) || defined(CONFIG_USB_DWC2_SYS_OTG_ID)
	&dev_attr_id.attr,
#endif
#endif	/*DWC2_HOST_MODE_ENABLE*/
	NULL
};

static const struct attribute_group dwc2_jz_attr_group = {
	.attrs = dwc2_jz_attributes,
};


static u64 dwc2_jz_dma_mask = DMA_BIT_MASK(32);

static int dwc2_jz_probe(struct platform_device *pdev) {
	struct platform_device		*dwc2;
	struct dwc2_jz		*jz;
	struct dwc2_platform_data	*dwc2_plat_data;
	int	ret = -ENOMEM;
	bool init_power_off = false;

	printk(KERN_DEBUG"dwc2 otg probe start\n");
	jz = devm_kzalloc(&pdev->dev, sizeof(*jz), GFP_KERNEL);
	if (!jz) {
		dev_err(&pdev->dev, "not enough memory\n");
		return -ENOMEM;
	}

	/*
	 * Right now device-tree probed devices don't get dma_mask set.
	 * Since shared usb code relies on it, set it here for now.
	 * Once we move to full device tree support this will vanish off.
	 */
	if (!pdev->dev.dma_mask)
		pdev->dev.dma_mask = &dwc2_jz_dma_mask;

	platform_set_drvdata(pdev, jz);
	dwc2_plat_data = devm_kzalloc(&pdev->dev,
			sizeof(struct dwc2_platform_data), GFP_KERNEL);
	if (!dwc2_plat_data)
		return -ENOMEM;
	dwc2 = &jz->dwc2;
	dwc2->name = "dwc2";
	dwc2->id = -1;
	device_initialize(&dwc2->dev);
	dma_set_coherent_mask(&dwc2->dev, pdev->dev.coherent_dma_mask);
	dwc2->dev.parent = &pdev->dev;
	dwc2->dev.dma_mask = pdev->dev.dma_mask;
	dwc2->dev.dma_parms = pdev->dev.dma_parms;
	dwc2->dev.platform_data = dwc2_plat_data;

	jz->dev	= &pdev->dev;
	mutex_init(&jz->irq_lock);
	INIT_WORK(&jz->turn_off_work, dwc2_turn_off_work);

	jz->clk = devm_clk_get(&pdev->dev, OTG_CLK_NAME);
	if (IS_ERR(jz->clk)) {
		dev_err(&pdev->dev, "clk gate get error\n");
		return PTR_ERR(jz->clk);
        }
        jz->cgu_clk = devm_clk_get(&pdev->dev, CGU_USB_CLK_NAME);
        if (IS_ERR(jz->cgu_clk)) {
                jz->cgu_clk = NULL;
                dev_warn(&pdev->dev, "no cgu clk check it\n");
        } else {
                clk_set_rate(jz->cgu_clk, 24000000);
	        clk_enable(jz->cgu_clk);
        }
	jz->pwr_clk = devm_clk_get(&pdev->dev, USB_PWR_CLK_NAME);
	if (IS_ERR(jz->pwr_clk))
		jz->pwr_clk = NULL;

	clk_enable(jz->clk);
	if (jz->pwr_clk)
		clk_enable(jz->pwr_clk);


#if !IS_ENABLED(CONFIG_USB_DWC2_HOST_ONLY)	/*DEVICE*/
#ifndef CONFIG_BOARD_HAS_NO_DETE_FACILITY
	jz->dete_irq = -1;
	jz->dete_pin = &dwc2_dete_pin;
	if (gpio_is_valid(jz->dete_pin->num)) {
		INIT_DELAYED_WORK(&jz->gpio_dete_work, gpio_usb_detect_work);
		ret = devm_gpio_request_one(&pdev->dev, jz->dete_pin->num,
				GPIOF_DIR_IN, "usb-insert-detect");
		if (!ret) {
			jz->dete_irq = gpio_to_irq(jz->dete_pin->num);
		}
	}
	if (jz->dete_irq < 0)
		dwc2_plat_data->keep_phy_on = 1;
#else
	dwc2_plat_data->keep_phy_on = 1;
#endif
#endif	/*DEVICE*/

#if !IS_ENABLED(CONFIG_USB_DWC2_DEVICE_ONLY)	/*HOST*/
	jz->drvvbus_pin = &dwc2_drvvbus_pin;
	jz->vbus = regulator_get(NULL, VBUS_REG_NAME);
	if (IS_ERR_OR_NULL(jz->vbus)) {
		if (gpio_is_valid(jz->drvvbus_pin->num)) {
			ret = devm_gpio_request_one(&pdev->dev, jz->drvvbus_pin->num,
					GPIOF_DIR_OUT, "drvvbus_pin");
			if (ret < 0) jz->drvvbus_pin = NULL;
		} else {
			jz->drvvbus_pin = NULL;
		}
		if (!jz->drvvbus_pin)
			dev_warn(&pdev->dev, "Not Found %s regulator \n", VBUS_REG_NAME);
		jz->vbus = NULL;
	}
	INIT_WORK(&jz->vbus_work, dwc2_vbus_work);
	atomic_set(&jz->vbus_on, 0);
#if defined(CONFIG_USB_DWC2_SYS_OTG_ID)
	jz->id_level = 1;
	wake_lock_init(&jz->resume_wake_lock, WAKE_LOCK_SUSPEND, "usb insert wake lock");
	INIT_DELAYED_WORK(&jz->gpio_otg_id_work, gpio_dwc2_id_status_change_work);
#elif defined(CONFIG_USB_DWC2_GPIO_OTG_ID)
	wake_lock_init(&jz->resume_wake_lock, WAKE_LOCK_SUSPEND, "usb insert wake lock");
	jz->id_pin = &dwc2_id_pin;
	jz->id_irq = -1;
	if(gpio_is_valid(jz->id_pin->num)) {
		INIT_DELAYED_WORK(&jz->gpio_otg_id_work, gpio_dwc2_id_status_change_work);
		ret = devm_gpio_request_one(&pdev->dev, jz->id_pin->num,
				GPIOF_DIR_IN, "otg-id-detect");
		if (!ret) {
			jz->id_irq = gpio_to_irq(jz->id_pin->num);
		}
	}

	if (jz->id_irq < 0)
		dwc2_plat_data->keep_phy_on = 1;
#else
	dwc2_plat_data->keep_phy_on = 1;
#endif
#endif	/*HOST*/

	jz_otg_ctr_reset();
	jz_otg_phy_init(dwc2_usb_mode());

	ret = platform_device_add_resources(dwc2, pdev->resource,
					pdev->num_resources);
	if (ret) {
		dev_err(&pdev->dev, "couldn't add resources to dwc2 device\n");
		goto fail_register_dwc2_dev;
	}

	ret = platform_device_add(dwc2);
	if (ret) {
		dev_err(&pdev->dev, "failed to register dwc2 device\n");
		goto fail_register_dwc2_dev;
	}

#if !IS_ENABLED(CONFIG_USB_DWC2_HOST_ONLY)
	if (dwc2_plat_data->keep_phy_on) {
		dwc2_gadget_plug_change(1);
	}
#ifndef CONFIG_BOARD_HAS_NO_DETE_FACILITY
	else {
		ret = devm_request_irq(&pdev->dev,
				jz->dete_irq, usb_detect_interrupt,
				IRQF_TRIGGER_RISING | IRQF_TRIGGER_FALLING,
				"usb-detect", jz);
		if (ret) {
			dev_err(&pdev->dev, "request usb-detect fail\n");
			goto fail_reuqest_irq;
		}
		if (__dwc2_get_detect_pin_status(jz)) {
			mutex_lock(&jz->irq_lock);
			dwc2_gadget_plug_change(1);
			mutex_unlock(&jz->irq_lock);
		} else {
			init_power_off = true;
		}
	}
#endif
#endif

#if !IS_ENABLED(CONFIG_USB_DWC2_DEVICE_ONLY)
#if defined(CONFIG_USB_DWC2_SYS_OTG_ID)
		if(!__dwc2_get_id_level(jz)) {
			init_power_off = false;
			schedule_delayed_work(&jz->gpio_otg_id_work, HZ / 2);
		}
#elif defined(CONFIG_USB_DWC2_GPIO_OTG_ID)
	if (jz->id_irq >= 0) {
		ret = devm_request_irq(&pdev->dev, jz->id_irq, usb_host_id_interrupt,
				IRQF_TRIGGER_RISING | IRQF_TRIGGER_FALLING, "usb-host-id", jz);
		if (ret) {
			dev_err(&pdev->dev, "request host id interrupt fail!\n");
			goto fail_reuqest_irq;
		}

		if(!__dwc2_get_id_level(jz)) {
			init_power_off = false;
			disable_irq(jz->id_irq);
			schedule_delayed_work(&jz->gpio_otg_id_work, HZ / 2);
		}
	}
#endif
#endif

	if (init_power_off) {
		struct dwc2 *dwc = platform_get_drvdata(dwc2);
		dwc2_turn_off(dwc, true);
	}

	ret = sysfs_create_group(&pdev->dev.kobj, &dwc2_jz_attr_group);
	if (ret < 0) {
		dev_err(&pdev->dev, "failed to create sysfs group\n");
	}

	printk(KERN_DEBUG "dwc2 otg probe success\n");
	return 0;

#if !defined(CONFIG_BOARD_HAS_NO_DETE_FACILITY) || defined(CONFIG_USB_DWC2_GPIO_OTG_ID)
fail_reuqest_irq:
	platform_device_unregister(&jz->dwc2);
#endif
fail_register_dwc2_dev:
	platform_set_drvdata(pdev, NULL);
#if DWC2_HOST_MODE_ENABLE
#if defined(CONFIG_USB_DWC2_GPIO_OTG_ID) || defined(CONFIG_USB_DWC2_SYS_OTG_ID)
	wake_lock_destroy(&jz->resume_wake_lock);
#endif
	if (!IS_ERR_OR_NULL(jz->vbus))
		regulator_put(jz->vbus);
#endif
	clk_disable(jz->clk);
        if (jz->cgu_clk)
	        clk_disable(jz->cgu_clk);
	if (jz->pwr_clk)
		clk_disable(jz->pwr_clk);
	dev_err(&pdev->dev, "device probe failed\n");
	return ret;
}

static int dwc2_jz_remove(struct platform_device *pdev)
{
	struct dwc2_jz	*jz = platform_get_drvdata(pdev);
	platform_set_drvdata(pdev, NULL);
#if DWC2_HOST_MODE_ENABLE
#if defined(CONFIG_USB_DWC2_GPIO_OTG_ID) || defined(CONFIG_USB_DWC2_SYS_OTG_ID)
	wake_lock_destroy(&jz->resume_wake_lock);
#endif
	if (!IS_ERR_OR_NULL(jz->vbus))
		regulator_put(jz->vbus);
#endif
	clk_disable(jz->clk);
        if (jz->cgu_clk)
	        clk_disable(jz->cgu_clk);
	if (jz->pwr_clk)
		clk_disable(jz->pwr_clk);
	platform_device_unregister(&jz->dwc2);

	return 0;
}

static int dwc2_jz_suspend(struct platform_device *pdev, pm_message_t state)
{
#if !defined(CONFIG_BOARD_HAS_NO_DETE_FACILITY) || defined(CONFIG_USB_DWC2_GPIO_OTG_ID)
	struct dwc2_jz	*jz = platform_get_drvdata(pdev);
#endif

#if DWC2_DEVICE_MODE_ENABLE
#ifndef  CONFIG_BOARD_HAS_NO_DETE_FACILITY
	if (jz->dete_irq >= 0)
		enable_irq_wake(jz->dete_irq);
#endif /*CONFIG_BOARD_HAS_NO_DETE_FACILITY*/
#endif


#if DWC2_HOST_MODE_ENABLE
#ifdef CONFIG_USB_DWC2_GPIO_OTG_ID
	if (jz->id_irq >= 0)
		enable_irq_wake(jz->id_irq);
#endif
#endif
	return 0;
}

static int dwc2_jz_resume(struct platform_device *pdev)
{
#if !defined(CONFIG_BOARD_HAS_NO_DETE_FACILITY) || defined(CONFIG_USB_DWC2_GPIO_OTG_ID)
	struct dwc2_jz	*jz = platform_get_drvdata(pdev);
#endif

#if DWC2_DEVICE_MODE_ENABLE
#ifndef  CONFIG_BOARD_HAS_NO_DETE_FACILITY
	if (jz->dete_irq >= 0)
		disable_irq_wake(jz->dete_irq);
#endif /*CONFIG_BOARD_HAS_NO_DETE_FACILITY*/
#endif

#if DWC2_HOST_MODE_ENABLE
#ifdef CONFIG_USB_DWC2_GPIO_OTG_ID
	if (jz->id_irq >= 0)
		disable_irq_wake(jz->id_irq);
#endif
#endif
	return 0;
}

static struct platform_driver dwc2_jz_driver = {
	.probe		= dwc2_jz_probe,
	.remove		= dwc2_jz_remove,
	.suspend	= dwc2_jz_suspend,
	.resume		= dwc2_jz_resume,
	.driver		= {
		.name	= "jz-dwc2",
		.owner =  THIS_MODULE,
	},
};


static int __init dwc2_jz_init(void)
{
	return platform_driver_register(&dwc2_jz_driver);
}

static void __exit dwc2_jz_exit(void)
{
	platform_driver_unregister(&dwc2_jz_driver);
}

/* make us init after usbcore and i2c (transceivers, regulators, etc)
 * and before usb gadget and host-side drivers start to register
 */
fs_initcall(dwc2_jz_init);
module_exit(dwc2_jz_exit);
MODULE_ALIAS("platform:jz-dwc2");
MODULE_AUTHOR("Lutts Cao <slcao@ingenic.cn>");
MODULE_LICENSE("GPL");
MODULE_DESCRIPTION("DesignWare USB2.0 JZ Glue Layer");
