#include <linux/errno.h>
#include <utils/string.h>
#include <linux/kernel.h>

int strtol_h(const char *str, long *res)
{
    char *end = (char *)str;
    int len = strlen(str);

    if (len == 0)
        return -EINVAL;

    long tmp = simple_strtol(str, &end, 0);

    if (end == str)
        return -EINVAL;

    if (end - str != len) {
        if (end - str == len - 1) {
            char c = tolower(str[len - 1]);
            if (c == 'k') {
                if (abs(tmp) > LONG_MAX / 1000)
                    return -ERANGE;
                tmp *= 1000;
                goto out;
            }

            if (c == 'm') {
                if (abs(tmp) > LONG_MAX / 1000000)
                    return -ERANGE;
                tmp *= 1000000;
                goto out;
            }
        }

        return -EINVAL;
    }

out:
    if (res)
        *res = tmp;

    return 0;
}

int strtoul_h(const char *str, unsigned long *res)
{
    char *end = (char *)str;
    int len = strlen(str);

    if (len == 0)
        return -EINVAL;

    unsigned long tmp = simple_strtoul(str, &end, 0);

    if (end == str)
        return -EINVAL;

    if (end - str != len) {
        if (end - str == len - 1) {
            char c = tolower(str[len - 1]);
            if (c == 'k') {
                if (abs(tmp) > ULONG_MAX / 1000)
                    return -ERANGE;
                tmp *= 1000;
                goto out;
            }

            if (c == 'm') {
                if (abs(tmp) > ULONG_MAX / 1000000)
                    return -ERANGE;
                tmp *= 1000000;
                goto out;
            }
        }

        return -EINVAL;
    }

out:
    if (res)
        *res = tmp;

    return 0;
}
