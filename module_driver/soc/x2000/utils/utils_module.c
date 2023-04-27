#include <linux/module.h>

void rtc32k_init(void);

static int utils_init(void)
{
#ifdef MD_X2000_UTILS_RTC32K
    rtc32k_init();
#endif

    return 0;
}

module_init(utils_init);

MODULE_LICENSE("GPL");