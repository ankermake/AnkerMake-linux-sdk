#ifndef __TX_SENSOR_COMMON_H__
#define __TX_SENSOR_COMMON_H__
#include <soc/gpio.h>

#include <tx-isp/txx-funcs.h>
#include "tx-isp-common.h"


#define SENSOR_R_BLACK_LEVEL	0
#define SENSOR_GR_BLACK_LEVEL	1
#define SENSOR_GB_BLACK_LEVEL	2
#define SENSOR_B_BLACK_LEVEL	3

/* External v4l2 format info. */
#define V4L2_I2C_REG_MAX		(150)
#define V4L2_I2C_ADDR_16BIT		(0x0002)
#define V4L2_I2C_DATA_16BIT		(0x0004)
#define V4L2_SBUS_MASK_SAMPLE_8BITS	0x01
#define V4L2_SBUS_MASK_SAMPLE_16BITS	0x02
#define V4L2_SBUS_MASK_SAMPLE_32BITS	0x04
#define V4L2_SBUS_MASK_ADDR_8BITS	0x08
#define V4L2_SBUS_MASK_ADDR_16BITS	0x10
#define V4L2_SBUS_MASK_ADDR_32BITS	0x20
#define V4L2_SBUS_MASK_ADDR_STEP_16BITS 0x40
#define V4L2_SBUS_MASK_ADDR_STEP_32BITS 0x80
#define V4L2_SBUS_MASK_SAMPLE_SWAP_BYTES 0x100
#define V4L2_SBUS_MASK_SAMPLE_SWAP_WORDS 0x200
#define V4L2_SBUS_MASK_ADDR_SWAP_BYTES	0x400
#define V4L2_SBUS_MASK_ADDR_SWAP_WORDS	0x800
#define V4L2_SBUS_MASK_ADDR_SKIP	0x1000
#define V4L2_SBUS_MASK_SPI_READ_MSB_SET 0x2000
#define V4L2_SBUS_MASK_SPI_INVERSE_DATA 0x4000
#define V4L2_SBUS_MASK_SPI_HALF_ADDR	0x8000
#define V4L2_SBUS_MASK_SPI_LSB		0x10000



#define dual_camera_do(ret, sensor, msdelay ,func, args...)                         \
    do {                                                                            \
        int i, j, cur_chn;                                                          \
        struct dual_camera_frame_info *info = &sensor->video.dc_param->frame_info;  \
        cur_chn = info->frame_array[info->frame_cursor];                            \
        for (i=0; i<2; i++) {                                                       \
            ret += func(args);                                                      \
            /* printk("dual_camera_do(%d) -> %s(%s)\n",cur_chn, #func, #args); */   \
            for (j=0; j<info->frame_num; j++) {                                     \
                if (cur_chn != info->frame_array[j]) {                              \
                    cur_chn = info->frame_array[j];                                 \
                    break;                                                          \
                }                                                                   \
            }                                                                       \
            dual_camera_i2c_switch(cur_chn);                                        \
            if (!i)mdelay(msdelay);                                                 \
        }                                                                           \
    } while(0)

#define dual_camera_do_lock(ret, sensor, msdelay ,func, args...)        \
    do {                                                                \
        unsigned long flag;                                             \
        spin_lock_irqsave(&sensor->video.dc_param->lock, flag);         \
        dual_camera_do(ret, sensor, msdelay ,func, args);               \
        spin_unlock_irqrestore(&sensor->video.dc_param->lock, flag);    \
    } while(0)


static inline int set_sensor_gpio_function(int func_set)
{
	int ret = 0;
#if (defined(CONFIG_SOC_T10) || defined(CONFIG_SOC_T20) || defined(CONFIG_SOC_T30) || defined(CONFIG_SOC_X1830)) || defined(CONFIG_SOC_X1630)
	switch (func_set) {
	case DVP_PA_LOW_8BIT:
		ret = private_jzgpio_set_func(GPIO_PORT_A, GPIO_FUNC_1, 0x000340ff);
		pr_info("set sensor gpio as PA-low-8bit\n");
		break;
	case DVP_PA_HIGH_8BIT:
		ret = private_jzgpio_set_func(GPIO_PORT_A, GPIO_FUNC_1, 0x00034ff0);
		pr_info("set sensor gpio as PA-high-8bit\n");
		break;
	case DVP_PA_LOW_10BIT:
		ret = private_jzgpio_set_func(GPIO_PORT_A, GPIO_FUNC_1, 0x000343ff);
		pr_info("set sensor gpio as PA-low-10bit\n");
		break;
	case DVP_PA_HIGH_10BIT:
		ret = private_jzgpio_set_func(GPIO_PORT_A, GPIO_FUNC_1, 0x00034ffc);
		pr_info("set sensor gpio as PA-high-10bit\n");
		break;
	case DVP_PA_12BIT:
		ret = private_jzgpio_set_func(GPIO_PORT_A, GPIO_FUNC_1, 0x00034fff);
		pr_info("set sensor gpio as PA-12bit\n");
		break;
	default:
		pr_err("set sensor gpio error: unknow function %d\n", func_set);
		ret = -1;
		break;
	}
#else
	ret = -1;
#endif
	return ret;
}

static inline int set_mclk_drive(int strength)
{
    return private_jzgpio_set_func(GPIO_PORT_A, strength, 0x8000);
}

static inline void interpolation_frame_init(struct dual_camera_frame_info *info)
{
    unsigned int frame_num_of_group;
    unsigned int frame_num_of_group_cnt = 0;
    unsigned int camera1_frame_num_of_group;
    unsigned int ratio, remain;

    int i;

    if (info->camera1_frame_rate == 0 || info->camera2_frame_rate == 0) {
        info->frame_num = 1;
        info->frame_array = (unsigned char *)kmalloc(info->frame_num, GFP_KERNEL);
        if (info->camera1_frame_rate != 0) {
            info->frame_array[0] = info->camera1_chn_type;
        } else if (info->camera2_frame_rate != 0) {
            info->frame_array[0] = info->camera2_chn_type;
        } else {
            info->frame_array[0] = info->camera1_chn_type;
        }
    } else if (info->camera1_frame_rate == info->camera2_frame_rate) {
        info->frame_num = 2;
        info->frame_array = (unsigned char *)kmalloc(info->frame_num, GFP_KERNEL);
        info->frame_array[0] = info->camera1_chn_type;
        info->frame_array[1] = info->camera2_chn_type;
    } else {
        info->frame_num = info->camera1_frame_rate+info->camera2_frame_rate;
        info->frame_array = (unsigned char *)kmalloc(info->frame_num, GFP_KERNEL);

        ratio = max(info->camera1_frame_rate, info->camera2_frame_rate) / min(info->camera1_frame_rate, info->camera2_frame_rate);
        remain = max(info->camera1_frame_rate, info->camera2_frame_rate) % min(info->camera1_frame_rate, info->camera2_frame_rate);
        frame_num_of_group = ratio+2;
        for (i=0; i<info->frame_num; i++) {
            frame_num_of_group_cnt++;
            if (frame_num_of_group_cnt > frame_num_of_group) {
                frame_num_of_group_cnt = 1;
                if (remain > 0) {
                    remain--;
                }
            }
            if (remain > 0) {
                frame_num_of_group = ratio+2;
            } else {
                frame_num_of_group = ratio+1;
            }
            if (info->camera1_frame_rate > info->camera2_frame_rate) {
                if (remain > 0) {
                    camera1_frame_num_of_group = ratio+1;
                } else {
                    camera1_frame_num_of_group = ratio;
                }
            } else if (info->camera1_frame_rate < info->camera2_frame_rate) {
                camera1_frame_num_of_group = 1;
            }

            if (frame_num_of_group_cnt <= camera1_frame_num_of_group) {
                info->frame_array[i] = info->camera1_chn_type;
            } else {
                info->frame_array[i] = info->camera2_chn_type;
            }
        }
    }

    info->frame_cursor = 0;

#if 0
    for (i=0; i<info->frame_num; i++) {
        printk("%d - ", info->frame_array[i]);
    }
    printk("[%d]\n", info->frame_num);
#endif
}

static inline void interpolation_frame_deinit(struct dual_camera_frame_info *info)
{
    if (info->frame_array)
        kfree(info->frame_array);
    memset(info, 0, sizeof(struct dual_camera_frame_info));
}


#endif// __TX_SENSOR_COMMON_H__
