#ifndef __BCM_PM_CORE__
#define __BCM_PM_CORE__

struct bcm_power_platform_data {
	int wlan_pwr_en;
	int wlan_pwr_en_level;
	void (*clk_enable)(void);
	void (*clk_disable)(void);
};

#endif /*__BCM_PM_CORE__*/
