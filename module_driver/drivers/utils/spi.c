#include <linux/kernel.h>
#include <linux/list.h>
#include <linux/spi/spi.h>
#include <linux/spinlock.h>
#include <utils/spi.h>
#include <linux/device.h>

static DEFINE_MUTEX(mutex);

struct spi_device *spi_register_device(struct spi_board_info *info, int spi_bus_num)
{

    struct spi_master *master = spi_busnum_to_master(spi_bus_num);
    struct spi_device *dev;
    struct device *d;
    int i;
    char dev_name[128];

    mutex_lock(&mutex);

    if (!master) {
        printk(KERN_ERR "error: failed to get spi master %d\n", spi_bus_num);
        dev = NULL;
        goto unlock;
    }

    /**
     * info->chip_select 将不再有意义
     * info->controller_data 将作为 chip select 引脚
     */

    for (i = 0; i < master->num_chipselect + 1; i++) {
        sprintf(dev_name, "spi%d.%d", spi_bus_num, i);
        d = bus_find_device_by_name(&spi_bus_type, NULL, dev_name);
        if (!d) {
            info->chip_select = i;
            break;
        }
    }

    if (i >= master->num_chipselect) {
        dev = NULL;
        goto unlock;
    }

    info->bus_num = spi_bus_num;
    dev = spi_new_device(master, info);

    if (!dev)
        printk(KERN_ERR "can not register spi device to %d busnum!", spi_bus_num);

    spi_master_put(master);

unlock:
    mutex_unlock(&mutex);
    return dev;
}
EXPORT_SYMBOL(spi_register_device);

