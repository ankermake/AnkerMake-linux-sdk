#include <linux/spi/spi.h>
#include <linux/init.h>

struct spi_board_info example = {
	.modalias = "",		    //device name, match driver name
	.platform_data = NULL,	//device private data
	.max_speed_hz = 0, 	    //set bus max rate
	.bus_num = 0,		    //bus num
	.chip_select = 0,	    //chip select
	.mode = 0,
};

static int __init example_bd_info_init(void)
{
	return spi_register_board_info(&example, 1);
}
arch_initcall(example_bd_info_init);
