
#include <osl.h>
#include <dhd_linux.h>

#ifdef CUSTOMER_HW_PLATFORM
#include <mach/gpio.h>
#include <mach/regs-gpio.h>
#include <plat/gpio-cfg.h>
#include <plat/sdhci.h>
#include <plat/devs.h>
#define	sdmmc_channel	s3c_device_hsmmc0
#endif

#if defined(CUSTOMER_HW_INTEL) && defined(BCMSDIO)
#include <linux/mmc/core.h>
#include <linux/mmc/card.h>
#include <linux/mmc/host.h>
#include <linux/mmc/sdio_func.h>
#include <linux/gpio.h>
#endif

#ifdef CUSTOMER_HW_INGENIC
// extern int ingenic_sdio_wlan_power_onoff(int onoff, int flag);
extern  void bcm_wlan_power_off(int);
extern  void bcm_wlan_power_on(int);
extern int ingenic_sdio_wlan_get_irq(void);
#define RESET  0
#define NORMAL 1
static int poweroff_num = 0;
static int poweron_num = 0;
#endif

#define WL_REG_ON	 -1
#define WL_HOST_WAKE -1 // WL_HOST_WAKE is output pin of WLAN module

struct wifi_platform_data dhd_wlan_control = {0};

#ifdef CUSTOMER_OOB
uint bcm_wlan_get_oob_irq(void)
{
	int wl_host_wake = WL_HOST_WAKE;
#ifdef CUSTOMER_HW_INGENIC
	wl_host_wake = ingenic_sdio_wlan_get_irq();
#endif
	uint host_oob_irq = 0;

	printf("GPIO(WL_HOST_WAKE) = %d\n", wl_host_wake);
	if (gpio_request(wl_host_wake, "bcmdhd") < 0) {
		printf("%s: gpio_request failed\n", __FUNCTION__);
	}
	host_oob_irq = gpio_to_irq(wl_host_wake);
	if (gpio_direction_input(wl_host_wake) < 0 ) {
		printf("%s: gpio_direction_input failed\n", __FUNCTION__);
	}

	printf("host_oob_irq: %d\n", host_oob_irq);

	return host_oob_irq;
}

void bcm_wlan_free_oob_gpio(uint irq_num)
{
	printf("%s: gpio_free(%d)\n", __FUNCTION__, irq_num);
	if (irq_num != -1)
		gpio_free(irq_num);
}

// uint bcm_wlan_get_oob_irq_flags(void)
// {
// 	uint host_oob_irq_flags = 0;

// #ifdef HW_OOB
// #ifdef HW_OOB_LOW_LEVEL
// 	host_oob_irq_flags = IORESOURCE_IRQ | IORESOURCE_IRQ_LOWLEVEL | IORESOURCE_IRQ_SHAREABLE;
// #else
// 	host_oob_irq_flags = IORESOURCE_IRQ | IORESOURCE_IRQ_HIGHLEVEL | IORESOURCE_IRQ_SHAREABLE;
// #endif
// #else
// 	host_oob_irq_flags = IORESOURCE_IRQ | IORESOURCE_IRQ_HIGHEDGE | IORESOURCE_IRQ_SHAREABLE;
// #endif

// 	printf("host_oob_irq_flags=0x%X\n", host_oob_irq_flags);

// 	return host_oob_irq_flags;
// }
#endif



int
bcm_wlan_set_power(bool on
#ifdef CUSTOMER_HW_INTEL
, wifi_adapter_info_t *adapter
#endif
)
{
	int err = 0;
	uint wl_reg_on = WL_REG_ON;

	if (on) {
		printf("======== PULL WL_REG_ON HIGH! %d ========\n", wl_reg_on);

	#ifdef CUSTOMER_HW_INGENIC
		if (poweron_num) {
			bcm_wlan_power_on(RESET);
		} else {
			poweron_num = 1;
			bcm_wlan_power_on(NORMAL);
		}
	#endif
		/* Lets customer power to get stable */
		mdelay(100);
	} else {

	printf("======== PULL WL_REG_ON LOW! %d ========\n", wl_reg_on);

#ifdef CUSTOMER_HW_INGENIC
		if (poweron_num) {
			bcm_wlan_power_off(RESET);
		} else {
			poweroff_num = 1;
			bcm_wlan_power_off(NORMAL);
		}
#endif

	}

	return err;
}

int bcm_wlan_set_carddetect(bool present)
{
	int err = 0;

	if (present) {
		printf("======== Card detection to detect SDIO card! ========\n");
#ifdef CONFIG_MACH_ODROID_4210
		sdhci_s3c_ force_presence_change(&sdmmc_channel, 1);
#endif
	} else {
		printf("======== Card detection to remove SDIO card! ========\n");
#ifdef CONFIG_MACH_ODROID_4210
		sdhci_s3c_force_presence_change(&sdmmc_channel, 0);
#endif
	}

	return err;
}

int bcm_wlan_get_mac_address(unsigned char *buf)
{
	int err = 0;

	printf("======== %s ========\n", __FUNCTION__);
#ifdef EXAMPLE_GET_MAC
	/* EXAMPLE code */
	{
		struct ether_addr ea_example = {{0x00, 0x11, 0x22, 0x33, 0x44, 0xFF}};
		bcopy((char *)&ea_example, buf, sizeof(struct ether_addr));
	}
#endif /* EXAMPLE_GET_MAC */

	return err;
}

#ifdef CONFIG_DHD_USE_STATIC_BUF
extern void *bcmdhd_mem_prealloc(int section, unsigned long size);
void* bcm_wlan_prealloc(int section, unsigned long size)
{
	void *alloc_ptr = NULL;
	alloc_ptr = bcmdhd_mem_prealloc(section, size);
	if (alloc_ptr) {
		printf("success alloc section %d, size %ld\n", section, size);
		if (size != 0L)
			bzero(alloc_ptr, size);
		return alloc_ptr;
	}
	printf("can't alloc section %d\n", section);
	return NULL;
}
#endif

#if !defined(WL_WIRELESS_EXT)
struct cntry_locales_custom {
	char iso_abbrev[WLC_CNTRY_BUF_SZ];	/* ISO 3166-1 country abbreviation */
	char custom_locale[WLC_CNTRY_BUF_SZ];	/* Custom firmware locale */
	int32 custom_locale_rev;		/* Custom local revisin default -1 */
};
#endif

static struct cntry_locales_custom brcm_wlan_translate_custom_table[] = {
	/* Table should be filled out based on custom platform regulatory requirement */
	{"",   "XT", 49},  /* Universal if Country code is unknown or empty */
	{"US", "US", 0},
};

#ifdef CUSTOM_FORCE_NODFS_FLAG
struct cntry_locales_custom brcm_wlan_translate_nodfs_table[] = {
	{"",   "XT", 50},  /* Universal if Country code is unknown or empty */
	{"US", "US", 0},
};
#endif

static void *bcm_wlan_get_country_code(char *ccode
#ifdef CUSTOM_FORCE_NODFS_FLAG
	, u32 flags
#endif
)
{
	struct cntry_locales_custom *locales;
	int size;
	int i;

	if (!ccode)
		return NULL;

#ifdef CUSTOM_FORCE_NODFS_FLAG
	if (flags & WLAN_PLAT_NODFS_FLAG) {
		locales = brcm_wlan_translate_nodfs_table;
		size = ARRAY_SIZE(brcm_wlan_translate_nodfs_table);
	} else {
#endif
		locales = brcm_wlan_translate_custom_table;
		size = ARRAY_SIZE(brcm_wlan_translate_custom_table);
#ifdef CUSTOM_FORCE_NODFS_FLAG
	}
#endif

	for (i = 0; i < size; i++)
		if (strcmp(ccode, locales[i].iso_abbrev) == 0)
			return &locales[i];
	return NULL;
}

extern int bcm_wlan_get_mac_addr_from_sys(unsigned char *buf);
int bcm_wlan_set_plat_data(void) {
	printf("%s\n", __FUNCTION__);
	dhd_wlan_control.set_power = bcm_wlan_set_power;
	dhd_wlan_control.set_carddetect = bcm_wlan_set_carddetect;
	// dhd_wlan_control.get_mac_addr = bcm_wlan_get_mac_address;
	dhd_wlan_control.get_mac_addr = bcm_wlan_get_mac_addr_from_sys;
#ifdef CONFIG_DHD_USE_STATIC_BUF
	dhd_wlan_control.mem_prealloc = bcm_wlan_prealloc;
#endif
	dhd_wlan_control.get_country_code = bcm_wlan_get_country_code;
	return 0;
}

