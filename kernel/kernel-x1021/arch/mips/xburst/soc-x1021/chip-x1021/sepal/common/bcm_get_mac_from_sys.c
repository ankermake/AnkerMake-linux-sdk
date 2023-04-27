#include <linux/bug.h>
#include <linux/moduleparam.h>
#include <linux/kernel.h>
#include <linux/wlan_plat.h>

static inline int is_hex(char ch)
{
    return (ch >= '0' && ch <= '9') ||
        (ch >= 'A' && ch <= 'F') ||
        (ch >= 'a' && ch <= 'f');
}

static inline int to_hex(char ch)
{
    if (ch >= '0' && ch <= '9')
        return ch - '0';
    else if (ch >= 'A' && ch <= 'F')
        return ch - 'A' + 10;
    else
        return ch - 'a' + 10;
}

static inline int to_hex2(char a[2])
{
    return to_hex(a[0]) * 16 + to_hex(a[1]);
}

static char *bcm_mac_address;

core_param(bcm_mac_address, bcm_mac_address, charp, 0644);

int bcm_wlan_get_mac_addr_from_sys(unsigned char *buf)
{
    int i;
    int len;
    char *mac = bcm_mac_address;

    if (!mac)
        return -1;

    len = strlen(mac);
    
    if (len == 13 && mac[12] == '\n') {
        mac[12] = '\0';
        len = 12;
    }

    if (len == 18 && mac[17] == '\n') {
        mac[17] = '\0';
        len = 17;
    }

    if (len == 12) {
        for (i = 0; i < 12; i++) {
            if (!is_hex(mac[i]))
                return -1;
        }

        for (i = 0; i < 6; i++) {
            buf[i] = to_hex2(&mac[i * 2]);
            /* printk("%02x\n", (int) buf[i]); */
        }

        return 0;
    }

    if (len == 17) {
        i = 0;

        for (i = 0; i < 5; i++) {
            if (!(is_hex(mac[i * 3]) && is_hex(mac[i * 3 + 1]) && mac[i * 3 + 2] == ':'))
                return -1;
        }

        if (!is_hex(mac[15]) || !is_hex(mac[16]))
            return -1;

        for (i = 0; i < 6; i++) {
            buf[i] = to_hex2(&mac[i * 3]);
            /* printk("%02x\n", (int) buf[i]); */
        }

        return 0;
    }

    return -1;
}

struct wifi_platform_data wlan_pdata = {
    .get_mac_addr = bcm_wlan_get_mac_addr_from_sys,
};