#ifndef __DRIVERS_USB_DWC2_JZ4780_H
#define __DRIVERS_USB_DWC2_JZ4780_H

int dwc2_turn_off(struct dwc2 *dwc, bool graceful);
int dwc2_turn_on(struct dwc2* dwc);
void dwc2_gpio_irq_mutex_lock(struct dwc2 *dwc);
void dwc2_gpio_irq_mutex_unlock(struct dwc2 *dwc);

int dwc2_get_id_level(struct dwc2 *dwc);

#if IS_ENABLED(CONFIG_USB_DWC2_DEVICE_ONLY)
static inline void jz_set_vbus(struct dwc2 *dwc, int is_on) {};
#else
void jz_set_vbus(struct dwc2 *dwc, int is_on);
#endif

#endif	/* __DRIVERS_USB_DWC2_JZ4780_H */
