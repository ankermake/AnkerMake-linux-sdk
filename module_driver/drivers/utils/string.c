#include <linux/kernel.h>

#include <utils/string.h>

/**
 * @brief 忽略大小写判断字符串是否相同
 */
int stricmp(const char *s1, const char *s2)
{
    int c1, c2;

    do {
        c1 = tolower(*s1++);
        c2 = tolower(*s2++);
    } while (c1 == c2 && c1 != 0);
    return c1 - c2;
}
EXPORT_SYMBOL(stricmp);

/**
 * @brief 判断字符串 "s1" 是否带有前缀 "prefix"
 */
int strimatch(const char *s1, const char *prefix)
{
    int c1, c2;

    const char *s2 = prefix;

    do {
        c1 = tolower(*s1++);
        c2 = tolower(*s2++);
    } while (c1 == c2 && c2 != 0);

    return c2 == 0 ? 0 : c2 - c1;
}
EXPORT_SYMBOL(strimatch);

const char *str_first_space(const char *s)
{
    while (*s && !isspace(*s)) {
        s++;
    }

    return s;
}
EXPORT_SYMBOL(str_first_space);

const char *str_first_not_space(const char *s)
{
    while (*s && isspace(*s)) {
        s++;
    }

    return s;
}
EXPORT_SYMBOL(str_first_not_space);

const char *str_first_tail_space(const char *s)
{
    const char *e = s + strlen(s) - 1;

    while (e >= s && isspace(*e)) {
        e--;
    }

    return e + 1;
}
EXPORT_SYMBOL(str_first_tail_space);

const char *strip(const char *s, const char **right_p)
{
    while (*s && isspace(*s)) {
        s++;
    }

    const char *e = s + strlen(s) - 1;

    while (e >= s && isspace(*e)) {
        e--;
    }

    *right_p = e + 1;

    return s;
}
EXPORT_SYMBOL(strip);
