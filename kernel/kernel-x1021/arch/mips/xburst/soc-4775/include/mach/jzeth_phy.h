/*
 * JZ Ethernet PHY, usually used in arch code.
 *
 * Copyright (C) 2015 Ingenic Semiconductor Co., Ltd.
 */
#ifndef __JZETH_PHY_H__
#define __JZETH_PHY_H__

#include <soc/gpio.h>

/**
 * struct jz_ethphy_feature -
 * @phy_hwreset:	PHY hardware GPIO reset
 * @mdc_mincycle:	MDC minimum cycle time (Unit: nanosecond)
 */
struct jz_ethphy_feature {
	struct jz_gpio_phy_reset	phy_hwreset;
	u32				mdc_mincycle;
};

#endif /* __JZETH_PHY_H__ */
