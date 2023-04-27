#include <linux/slab.h>
#include <linux/kernel.h>
#include <linux/string.h>
#include <utils/string.h>
#include <assert.h>

static const char *str_get_first_word(const char *str, char *word)
{
    char q = 0;

    str = str_first_not_space(str);

    for (;;str++) {
        if (*str == '\0')
            goto out;

        if (!q) {
            if (isspace(*str))
                goto out;

            if (*str == '\\') {
                if (str[1] == '"') {
                    if (word)
                        *word++ = '"';
                    str++;
                    continue;
                }
            }

            if (*str == '\'') {
                q = '\'';
                continue;
            }

            if (*str == '"') {
                q = '"';
                continue;
            }
        } else {
            if (*str == '\\') {
                if (str[1] == '"') {
                    if (word) {
                        if (q == '\'')
                            *word++ = '\\';
                        *word++ = '"';
                    }

                    str++;
                    continue;
                }
            }

            if (*str == q) {
                q = 0;
                continue;
            }
        }

        if (word)
            *word++ = *str;
    }

out:
    if (word)
        *word = '\0';

    return str;
}

/**
 * 支持 "... ..." 以及 '... ...' 类型的word
 * 支持 \" 转义为 ",注意如下区别
 *    "... ..." 中 \" 会被转义 为 "
 *    '... ...' 中 \" 不会被转义 为 "
 * @param str 输入的字符串
 * @param word_count 返回 word 的个数,可以为NULL
 * @return words[count+1] 字符串数组, 数组的成员按顺序存放每个 word, words[count] 为 NULL 表示结束
 * @attention 返回值需要调用 str_free_words() 释放
 */
char **str_to_words(const char *str, int *word_count)
{
    int i, count = 0, len = 0;
    const char *save = str;

    while (1) {
        str = str_first_not_space(str);
        if (!*str)
            break;
        const char *s = str_get_first_word(str, NULL);
        len += s - str + 1;
        str = s;
        count++;
    }

    char **array = kmalloc((count + 1) * sizeof(char *) + len, GFP_KERNEL);
    char *p = (char *)&array[count + 1];
    assert(array);

    str = save;
    for (i = 0; i < count; i++) {
        array[i] = p;
        str = str_get_first_word(str, p);
        p += strlen(p) + 1;
    }

    array[count] = NULL;

    if (word_count)
        *word_count = count;

    return array;
}
EXPORT_SYMBOL(str_to_words);

/**
 * 释放 str_to_words() 函数返回的字符串数组
 */
void str_free_words(char **words)
{
    kfree(words);
}
EXPORT_SYMBOL(str_free_words);
