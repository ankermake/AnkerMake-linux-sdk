#include <linux/kernel.h>
#include <linux/i2c.h>
#include <linux/version.h>

struct i2c_client *i2c_register_device(struct i2c_board_info *info, int i2c_bus_num)
{
    struct i2c_adapter *adapter;
    struct i2c_client *client;

    adapter = i2c_get_adapter(i2c_bus_num);
    if (!adapter) {
        printk(KERN_ERR "error: failed to get i2c adapter %d\n", i2c_bus_num);
        return NULL;
    }
#if LINUX_VERSION_CODE >= KERNEL_VERSION(5,10, 0)
    client = i2c_new_client_device(adapter, info);
#else
    client = i2c_new_device(adapter, info);
#endif
    i2c_put_adapter(adapter);

    return client;
}
EXPORT_SYMBOL(i2c_register_device);
