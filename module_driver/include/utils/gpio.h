#ifndef _UTILS_GPIO_H_
#define _UTILS_GPIO_H_

#include <linux/module.h>
#include <soc/gpio.h>
#include <linux/gpio.h>

/**
 * 设置gpio port 中部分引脚的 func
 * @param port 0:PA 1:PB ,,,
 * @param pins 用bit来表示port 对应的某些pin脚
 * @param func 参见不同平台的定义
 * @return 0 表示成功
 */
extern int gpio_port_set_func(int port, unsigned long pins, enum gpio_function func);

/**
 * 设置gpio func
 * @param gpio 某个gpio
 * @param func 参见不同平台的定义
 * @return 0 表示成功
 */
extern int gpio_set_func(int gpio, enum gpio_function func);

/**
 * @brief 字符串 转 gpio
 * 字符串形式如下所示,不限大小写
 * gpio_pXN, gpio_pXNN, pXN, pXNN
 *     X: a,b,c,d,e,f,g
 *     N: 0,1,2,3,4,5,6,7,8,9
 * 例如: gpio_pa02 GPIO_PA02 gpio_pa2 PA02 pa02 pa2
 * @return -EINVAL 如果转换失败
 *         如果 str 是 “-1” 则返回 -1， -1 一般表示gpio未定义
 */
extern int str_to_gpio(const char *str);

/**
 * @brief gpio 转 字符串
 * @param gpio gpio
 * @param buf 字符串数组,如果buf为NULL,那么会调用kmalloc分配一个(需要用户kfree)
 * @return 返回gpio字符串，如果buf为NULL，则返回kmalloc分配的(需要用户kfree), 如果buf不为NULL，则返回buf,
 * 字符串形式如下所示
 *   1 PXNN (例如 PA02)
 *       X: A,B,C,D,E,F,G
 *       N: 0,1,2,3,4,5,6,7,8,9
 *   2 -1 (如果gpio 为 -1)
 *   3 error (如果gpio不是有效值也不是-1)
 */
extern char *gpio_to_str(int gpio, char *buf);

/**
 * @brief 字符串转gpio
 * 只要从开始匹配到gpio字符串成功即可
 * 匹配成功之后的位置通过传入的指针写回,方便继续解析接下来的字符串
 * @return 正数 表示匹配到一个有效的gpio,str_p 里的值会被指向接下来的字符串
 *         -1  表示匹配到 -1, -1 一般表示gpio未定义,str_p 里的值会被指向接下来的字符串
 *         -EINVAL 没有匹配到gpio或者-1, str_p里的值不会改变
 */
extern int str_match_gpio(const char **str_p);

/**
 * @brief module_param_gpio_named - module param for gpio
 * @param name: a valid C identifier which is the parameter name.
 * @param value: the actual lvalue to alter.
 * @param perm: visibility in sysfs.
 *
 */
#define module_param_gpio_named(name, value, perm) \
    param_check_int(name, &(value));               \
    module_param_cb(name, &param_gpio_ops, &(value), perm)

/**
 * @brief module_param_gpio - module param for gpio
 * @param value: the variable to alter, and exposed parameter name.
 * @param perm: visibility in sysfs.
 *
 */
#define module_param_gpio(value, perm)           \
    module_param_gpio_named(value, value, perm)

/*
 * used by module_param_gpio_named, module_param_gpio
 */
extern struct kernel_param_ops param_gpio_ops;

#endif /* _UTILS_GPIO_H_ */

