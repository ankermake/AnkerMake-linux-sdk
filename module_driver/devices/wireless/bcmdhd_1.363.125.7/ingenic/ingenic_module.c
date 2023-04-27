int bt_power_dev_init(void);
int bt_power_dev_exit(void);

int bt_power_init(void);
int bt_power_exit(void);

int wlan_device_init(void);
void wlan_device_exit(void);

int bcm_wlan_deinit(void);

int bcm_ingenic_moudle_init(void)
{
    int ret;

    ret = bt_power_dev_init();
    if (ret)
        return ret;

    ret = bt_power_init();
    if (ret) {
        bt_power_dev_exit();
        return ret;
    }

    ret = wlan_device_init();
    if (ret) {
        bt_power_dev_exit();
        bt_power_exit();
        return ret;
    }

    return 0;
}

void bcm_ingenic_moudle_exit(void)
{
    wlan_device_exit();
    bt_power_exit();
    bt_power_dev_exit();
    bcm_wlan_deinit();
}