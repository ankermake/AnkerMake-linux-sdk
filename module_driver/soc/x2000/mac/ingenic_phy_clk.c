#include <linux/kernel.h>
#include <linux/phy.h>
#include <linux/clk.h>
#include <linux/delay.h>

static unsigned int mac_init_flag;
static unsigned int phy_clk_rate;
static unsigned int phy_clk_delay_us;
static struct clk *phy_clk;

static DEFINE_MUTEX(mutex);

module_param_named(phy_clk_rate, phy_clk_rate, uint, 0444);
module_param_named(phy_clk_delay_us, phy_clk_delay_us, uint, 0444);

int mac_enable_phy_clk(void)
{
    if (!phy_clk_rate)
        return 0;

    mutex_lock(&mutex);

    if (!mac_init_flag) {
        phy_clk = clk_get(NULL, "div_macphy");
        if (IS_ERR(phy_clk)) {
            phy_clk = NULL;
            printk(KERN_ERR "%s: phy get clk failure\n", __func__);
            mutex_unlock(&mutex);
            return -1;
        }

        clk_set_rate(phy_clk, phy_clk_rate);

        clk_prepare_enable(phy_clk);

        if (phy_clk_delay_us)
            usleep_range(phy_clk_delay_us, phy_clk_delay_us);
    }

    mac_init_flag++;

    mutex_unlock(&mutex);

    return 0;
}

void mac_disable_phy_clk(void)
{
    if (phy_clk_rate) {
        mutex_lock(&mutex);
        mac_init_flag--;

        if (phy_clk && !mac_init_flag) {
            clk_disable_unprepare(phy_clk);
            clk_put(phy_clk);
            phy_clk = NULL;
        }
        mutex_unlock(&mutex);
    }

}
