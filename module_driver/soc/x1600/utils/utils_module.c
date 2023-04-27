#include <linux/module.h>

void rtc32k_init(void);
void clk24m_init(void);

static int utils_init(void)
{
#ifdef MD_X1600_UTILS_RTC32K
    rtc32k_init();
#endif

#ifdef MD_X1600_UTILS_CLK24M
    clk24m_init();
#endif

    return 0;
}

module_init(utils_init);

MODULE_LICENSE("GPL");
