#ifndef _UTILS_STRING_H_
#define _UTILS_STRING_H_

#include <linux/string.h>
#include <linux/ctype.h>

/**
 * @brief 忽略大小写判断字符串是否相同
 */
extern int stricmp(const char *s1, const char *s2);

/**
 * @brief 忽略大小写判断字符串 "s1" 是否带有前缀 "prefix"
 */
extern int strimatch(const char *s1, const char *prefix);

/*
 * 第一个space的位置
 * "123 ..." -> " ..."
 * "123"     -> ""
 */
const char *str_first_space(const char *s);

/*
 * 第一个非space的位置
 * "  123..." -> "123..."
 * "123..."   -> "123..."
 */
const char *str_first_not_space(const char *s);

/*
 * 尾部的连续space的起始位置
 * "...123   " -> "   "
 * "...123"    -> ""
 */
const char *str_first_tail_space(const char *s);

/**
 * 指明首尾两端应该strip的位置
 * return:  str_first_not_space(s)
 * right_p: right_p = str_first_tail_space(s)
 */
const char *strip(const char *s, const char **right_p);

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
char **str_to_words(const char *str, int *word_count);

/**
 * 释放 str_to_words() 函数返回的字符串数组
 */
void str_free_words(char **words);

/**
 * 将字符串转换为 long,可以支持单位为 k/K m/M
 * @param str 待转换的字符串
 * @param res 返回转换的值,如果转换成功
 * @return 0 转换成功 -ERANGE 超出范围 -EINVAL 转换失败
 */
int strtol_h(const char *str, long *res);

/**
 * 将字符串转换为 unsigned long,可以支持单位为 k/K m/M
 * @param str 待转换的字符串
 * @param res 返回转换的值,如果转换成功
 * @return 0 转换成功 -ERANGE 超出范围 -EINVAL 转换失败
 */
int strtoul_h(const char *str, unsigned long *res);

#endif /* _UTILS_STRING_H_ */

