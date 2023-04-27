#ifndef _UTILS_SPI_H_
#define _UTILS_SPI_H_

#include <linux/spi/spi.h>

/**
 * 向指定的spi总线注册spi设备
 * 设备的 chip_select 变量会被自动累加无需赋值
 * 需要提供 controller_data 变量作为cs 的gpio 引脚
 */
struct spi_device *spi_register_device(struct spi_board_info *info, int spi_bus_num);

#endif /* _UTILS_SPI_H_ */