#include <linux/kernel.h>
#include <linux/platform_device.h>
#include <linux/gpio.h>
#include <linux/module.h>
#include <linux/slab.h>
#include <linux/interrupt.h>
#include <linux/usb.h>
#include <linux/workqueue.h>

#include <linux/usb/gadget.h>
#include <linux/usb/gpio_vbus.h>
#include <linux/usb/otg.h>

#include <linux/usb/phy-ingenic.h>

/*
 * Needs to be loaded before the UDC driver that will use it.
 */
struct usb_phy_data {
    int vbus;

    struct usb_phy phy;
    struct device *dev;
    struct delayed_work work;

    int gpio_wakeup;
    int gpio_wakeup_inverted;
    int wakeup_irq;
    int wakeup_flag;

    int vbus_irq;
    int gpio_vbus;
    int gpio_vbus_inverted;
    int gpio_vbus_wakeup_enable;

    int gpio_drvvbus;
    int gpio_drvvbus_inverted;

    int gpio_switch;
    int gpio_switch_inverted;
};


/*
 * This driver relies on "both edges" triggering.  VBUS has 100 msec to
 * stabilize, so the peripheral controller driver may need to cope with
 * some bouncing due to current surges (e.g. charging local capacitance)
 * and contact chatter.
 *
 * REVISIT in desperate straits, toggling between rising and falling
 * edges might be workable.
 */
#define VBUS_IRQ_FLAGS \
    (IRQF_SHARED | IRQF_TRIGGER_RISING | IRQF_TRIGGER_FALLING)

#define WAKEUP_IRQ_FLAGS \
	(IRQF_SHARED | IRQF_TRIGGER_RISING | IRQF_TRIGGER_FALLING)

static int is_vbus_powered(struct ingenic_phy_data *pdata)
{
    int vbus;

    vbus = gpio_get_value(pdata->gpio_vbus);
    if (pdata->gpio_vbus_inverted)
        vbus = !vbus;

    return vbus;
}


static void ingenic_phy_work(struct work_struct *work)
{
    struct usb_phy_data *phy_data =
        container_of(work, struct usb_phy_data, work.work);
    struct ingenic_phy_data *pdata = phy_data->dev->platform_data;
    int status, vbus;

    if (!phy_data->phy.otg->gadget)
        return;

    vbus = is_vbus_powered(pdata);
    if ((vbus ^ phy_data->vbus) == 0)
        return;
    phy_data->vbus = vbus;

    if (vbus) {
        status = USB_EVENT_VBUS;
        phy_data->phy.state = OTG_STATE_B_PERIPHERAL;
        phy_data->phy.last_event = status;
        usb_gadget_vbus_connect(phy_data->phy.otg->gadget);

        atomic_notifier_call_chain(&phy_data->phy.notifier,
                        status, phy_data->phy.otg->gadget);
    } else {
        usb_gadget_vbus_disconnect(phy_data->phy.otg->gadget);
        status = USB_EVENT_NONE;
        phy_data->phy.state = OTG_STATE_B_IDLE;
        phy_data->phy.last_event = status;

        atomic_notifier_call_chain(&phy_data->phy.notifier,
                        status, phy_data->phy.otg->gadget);
    }
}


/* VBUS change IRQ handler */
static irqreturn_t ingenic_phy_irq(int irq, void *data)
{
    struct platform_device *pdev = data;
    struct ingenic_phy_data *pdata = pdev->dev.platform_data;
    struct usb_phy_data *phy_data = platform_get_drvdata(pdev);
    struct usb_otg *otg = phy_data->phy.otg;

    dev_dbg(&pdev->dev, "VBUS %s (gadget: %s)\n",
        is_vbus_powered(pdata) ? "supplied" : "inactive",
        otg->gadget ? otg->gadget->name : "none");

    if (otg->gadget)
        schedule_delayed_work(&phy_data->work, msecs_to_jiffies(100));

    return IRQ_HANDLED;
}


/* OTG transceiver interface */

static int usb_phy_set_vbus(struct usb_phy *phy, int on)
{
    struct usb_phy_data *usb_phy = container_of(phy, struct usb_phy_data, phy);

    if (usb_phy->gpio_drvvbus)
        gpio_set_value(usb_phy->gpio_drvvbus, on);

    return 0;
}

/* bind/unbind the peripheral controller */
static int ingenic_phy_set_peripheral(struct usb_otg *otg,
                    struct usb_gadget *gadget)
{
    struct usb_phy_data *phy_data;
    struct ingenic_phy_data *pdata;
    struct platform_device *pdev;

    phy_data = container_of(otg->phy, struct usb_phy_data, phy);
    pdev = to_platform_device(phy_data->dev);
    pdata = phy_data->dev->platform_data;

    if (!gadget) {
        dev_dbg(&pdev->dev, "unregistering gadget '%s'\n",
            otg->gadget->name);

        usb_gadget_vbus_disconnect(otg->gadget);
        otg->phy->state = OTG_STATE_UNDEFINED;

        otg->gadget = NULL;
        return 0;
    }

    otg->gadget = gadget;
    dev_dbg(&pdev->dev, "registered gadget '%s'\n", gadget->name);

    /* initialize connection state */
    phy_data->vbus = 0; /* start with disconnected */
    ingenic_phy_irq(phy_data->vbus_irq, pdev);
    return 0;
}


static void usb_phy_gpio_free(struct usb_phy_data *phy_data)
{
    if (gpio_is_valid(phy_data->gpio_vbus))
        gpio_free(phy_data->gpio_vbus);

    if (gpio_is_valid(phy_data->gpio_drvvbus))
        gpio_free(phy_data->gpio_drvvbus);

    if (gpio_is_valid(phy_data->gpio_switch))
        gpio_free(phy_data->gpio_switch);

    if (gpio_is_valid(phy_data->gpio_wakeup))
        gpio_free(phy_data->gpio_wakeup);
}

static int usb_phy_gpio_init(struct usb_phy_data *phy_data)
{
    int ret;
    struct platform_device *pdev = to_platform_device(phy_data->dev);

    if (gpio_is_valid(phy_data->gpio_vbus)) {
        ret = gpio_request(phy_data->gpio_vbus, "vbus_detect");
        if (ret) {
            dev_err(&pdev->dev, "can't request vbus gpio %d, err: %d\n", phy_data->gpio_vbus, ret);
            return ret;
        }
        gpio_direction_input(phy_data->gpio_vbus);
    }

    if (gpio_is_valid(phy_data->gpio_drvvbus)) {
        ret = gpio_request(phy_data->gpio_drvvbus, "gpio_drvvbus");
        if (ret) {
            dev_err(&pdev->dev, "can't request drvvbus gpio %d, err: %d\n", phy_data->gpio_drvvbus, ret);
            goto vbus_err;
        }
        gpio_direction_output(phy_data->gpio_drvvbus, phy_data->gpio_drvvbus_inverted);
    }

    if (gpio_is_valid(phy_data->gpio_switch)) {
        ret = gpio_request(phy_data->gpio_switch, "gpio_switch");
        if (ret) {
            dev_err(&pdev->dev, "can't request switch gpio %d, err: %d\n", phy_data->gpio_switch, ret);
            goto drvvbus_err;
        }

        gpio_direction_output(phy_data->gpio_switch, phy_data->gpio_switch_inverted);
    }

    if (gpio_is_valid(phy_data->gpio_wakeup)) {
        ret = gpio_request(phy_data->gpio_wakeup, "gpio_wakeup");
        if (ret) {
            dev_err(&pdev->dev, "can't request wakeup gpio %d, err: %d\n", phy_data->gpio_wakeup, ret);
            goto switch_err;
        }
        gpio_direction_input(phy_data->gpio_wakeup);
    }

    return 0;

switch_err:
    if (gpio_is_valid(phy_data->gpio_switch))
        gpio_free(phy_data->gpio_switch);
drvvbus_err:
    if (gpio_is_valid(phy_data->gpio_drvvbus))
        gpio_free(phy_data->gpio_drvvbus);
vbus_err:
    if (gpio_is_valid(phy_data->gpio_vbus))
        gpio_free(phy_data->gpio_vbus);

    return ret;
}

static irqreturn_t usb_wakeup_irq_handler(int irq, void *data)
{
	return IRQ_HANDLED;
}


static int ingenic_usb_phy_probe(struct platform_device *pdev)
{
    int err;
    unsigned long irqflags;
    struct usb_phy_data *phy_data;
    struct ingenic_phy_data *pdata = pdev->dev.platform_data;

    if (pdata == NULL)
        return -EINVAL;

    phy_data = kzalloc(sizeof(struct usb_phy_data), GFP_KERNEL);
    if (!phy_data)
        return -ENOMEM;

    phy_data->phy.otg = kzalloc(sizeof(struct usb_otg), GFP_KERNEL);
    if (!phy_data->phy.otg) {
        kfree(phy_data);
        return -ENOMEM;
    }

    platform_set_drvdata(pdev, phy_data);
    phy_data->dev = &pdev->dev;
    phy_data->phy.label = "ingenic_usb_phy";
    phy_data->phy.dev = phy_data->dev;
    phy_data->phy.set_vbus = usb_phy_set_vbus;

    phy_data->phy.state = OTG_STATE_UNDEFINED;

    phy_data->phy.otg->phy = &phy_data->phy;
    phy_data->phy.otg->set_peripheral = ingenic_phy_set_peripheral;

    phy_data->gpio_vbus = pdata->gpio_vbus;
    phy_data->gpio_vbus_inverted = pdata->gpio_vbus_inverted;
    phy_data->gpio_vbus_wakeup_enable = pdata->gpio_vbus_wakeup_enable;

    phy_data->gpio_switch = pdata->gpio_switch;
    phy_data->gpio_switch_inverted = pdata->gpio_switch_inverted;

    phy_data->gpio_drvvbus = pdata->gpio_drvvbus;
    phy_data->gpio_drvvbus_inverted = pdata->gpio_drvvbus_inverted;

    phy_data->gpio_wakeup = pdata->gpio_wakeup;
    phy_data->gpio_wakeup_inverted = pdata->gpio_wakeup_inverted;

    err = usb_phy_gpio_init(phy_data);
    if (err)
        goto err_gpio;

    if (gpio_is_valid(phy_data->gpio_vbus)) {
        phy_data->vbus_irq = gpio_to_irq(phy_data->gpio_vbus);
        irqflags = VBUS_IRQ_FLAGS;
        err = request_irq(phy_data->vbus_irq, ingenic_phy_irq, irqflags, "vbus_detect", pdev);
        if (err) {
            dev_err(&pdev->dev, "can't request irq %i, err: %d\n", phy_data->vbus_irq, err);
            goto err_vbus_irq;
        }
    }

    if (gpio_is_valid(phy_data->gpio_wakeup)) {
        phy_data->wakeup_irq = gpio_to_irq(phy_data->gpio_wakeup);
        irqflags = WAKEUP_IRQ_FLAGS;
        err = request_irq(phy_data->wakeup_irq, usb_wakeup_irq_handler, irqflags, "usb_wakeup", phy_data);
        if (err) {
            dev_err(&pdev->dev, "can't request irq %i, err: %d\n", phy_data->wakeup_irq, err);
            goto err_wakeup_irq;
        }
    }

    device_init_wakeup(&pdev->dev, 1);

    ATOMIC_INIT_NOTIFIER_HEAD(&phy_data->phy.notifier);

    INIT_DELAYED_WORK(&phy_data->work, ingenic_phy_work);

    /* only active when a gadget is registered */
    err = usb_add_phy(&phy_data->phy, USB_PHY_TYPE_USB2);
    if (err) {
        dev_err(&pdev->dev, "can't register transceiver, err: %d\n", err);
        goto err_otg;
    }

    return 0;

err_otg:
    if (gpio_is_valid(phy_data->gpio_wakeup))
        free_irq(phy_data->wakeup_irq, pdev);
err_wakeup_irq:
    if (gpio_is_valid(phy_data->gpio_vbus))
        free_irq(phy_data->vbus_irq, pdev);
err_vbus_irq:
    usb_phy_gpio_free(phy_data);
err_gpio:
    kfree(phy_data->phy.otg);
    kfree(phy_data);
    return err;
}

static int __exit ingenic_usb_phy_remove(struct platform_device *pdev)
{
    struct usb_phy_data *phy_data = platform_get_drvdata(pdev);

    device_init_wakeup(&pdev->dev, 0);
    cancel_delayed_work_sync(&phy_data->work);

    usb_remove_phy(&phy_data->phy);

    free_irq(phy_data->vbus_irq, pdev);
    free_irq(phy_data->wakeup_irq, pdev);

    usb_phy_gpio_free(phy_data);
    kfree(phy_data->phy.otg);
    kfree(phy_data);

    return 0;
}


#ifdef CONFIG_PM

static int ingenic_usb_phy_pm_suspend(struct device *dev)
{
    struct usb_phy_data *phy_data = dev_get_drvdata(dev);

    if (phy_data->gpio_switch)
        gpio_set_value(phy_data->gpio_switch, 1);

    if (phy_data->gpio_wakeup)
        enable_irq_wake(phy_data->wakeup_irq);

    if (gpio_is_valid(phy_data->gpio_vbus) && phy_data->gpio_vbus_wakeup_enable)
        enable_irq_wake(phy_data->vbus_irq);

    return 0;
}

static int ingenic_usb_phy_pm_resume(struct device *dev)
{
    struct usb_phy_data *phy_data = dev_get_drvdata(dev);

    if (gpio_is_valid(phy_data->gpio_vbus) && phy_data->gpio_vbus_wakeup_enable)
        disable_irq_wake(phy_data->vbus_irq);

    if (phy_data->gpio_switch)
        gpio_set_value(phy_data->gpio_switch, 0);

    if (phy_data->gpio_wakeup)
        disable_irq_wake(phy_data->wakeup_irq);

    return 0;
}

static const struct dev_pm_ops ingenic_usb_phy_pm_ops = {
    .suspend    = ingenic_usb_phy_pm_suspend,
    .resume     = ingenic_usb_phy_pm_resume,
};

#endif

MODULE_ALIAS("platform:ingenic_usb_phy");

static struct platform_driver ingenic_usb_phy_driver = {
    .driver = {
        .name  = "ingenic_usb_phy",
        .owner = THIS_MODULE,
#ifdef CONFIG_PM
        .pm = &ingenic_usb_phy_pm_ops,
#endif
    },
    .remove  = __exit_p(ingenic_usb_phy_remove),
};


module_platform_driver_probe(ingenic_usb_phy_driver, ingenic_usb_phy_probe);