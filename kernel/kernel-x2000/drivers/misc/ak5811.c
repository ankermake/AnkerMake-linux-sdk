/*
 * linux/drivers/misc/ak5811.c - Ingenic ak5811 driver
 *
 * Copyright (C) 2012 Ingenic Semiconductor Co., Ltd.
 * Author: <chongji.wang@ingenic.com>.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */

#include <linux/clk.h>
#include <linux/device.h>
#include <linux/delay.h>
#include <linux/workqueue.h>
#include <linux/interrupt.h>
#include <linux/gpio/consumer.h>
#include <linux/init.h>
#include <linux/module.h>
#include <linux/pm_runtime.h>
#include <linux/regulator/consumer.h>
#include <linux/sysfs.h>
#include <linux/slab.h>
#include <linux/version.h>
#include <media/media-entity.h>
#include <media/v4l2-async.h>
#include <media/v4l2-ctrls.h>
#include <media/v4l2-subdev.h>
#include <linux/of_graph.h>
#include <linux/spi/spi.h>
#include <linux/of_gpio.h>
#include <linux/of_platform.h>
#include <linux/pinctrl/consumer.h>
#include <linux/hrtimer.h>
#include <linux/miscdevice.h>
#include <linux/fs.h>
#include <linux/dma-mapping.h>
#include <linux/dmaengine.h>
#include <linux/platform_device.h>
#include <asm/uaccess.h>
#include <linux/syscalls.h>


#define AK5811_REG_WRITE	     _IOR('F', 100, unsigned int)
#define AK5811_REG_READ		     _IOR('F', 101, unsigned int)
#define AK5811_PATTERN_OUTPUT        _IOR('F', 102, unsigned int)
//#define AK5811_REG_WRITE	     _IOR('F', 103, unsigned int)

#define	LP_SPI_TEST 		1
#define LP_TRIGGER_REGISTER	1 //enable trigger by register
#define LP_TRIGGER_PIN		0 //enable trigger by pin
#define LP_DARKROOM_TEST	0 //for darkroom test

#if LP_SPI_TEST /* add for test */
#define MAX_SPI_DEV_NUM 6
#define SPI_MAX_SPEED_HZ        12000000

struct spi_test_data {
        struct device   *dev;
        struct spi_device *spi;
        char *rx_buf;
        int rx_len;
        char *tx_buf;
        int tx_len;
};

static struct spi_test_data g_spi_test_data;

#endif
#define DRIVER_VERSION			KERNEL_VERSION(0, 0x01, 0x0)

#define AK5811_DATA_FORMAT 		MEDIA_BUS_FMT_SBGGR12_1X12

#define AK5811_LINK_FREQ		(128 * 1000 * 1000)   /* 128M */
#define AK5811_LANES			4
#define AK5811_BITS_PER_SAMPLE		12

/* pixel rate = link frequency * 2 * lanes / BITS_PER_SAMPLE */
#define AK5811_PIXEL_RATE \
(AK5811_LINK_FREQ * 2 * AK5811_LANES / AK5811_BITS_PER_SAMPLE)


#define CHIP_ID				0x34

/* core register */
#define AK5811_REG_CHIP_ID		0x01


#define REG_NULL			0xFF

#define AK5811_NAME			"ak5811"

#define AK5811_VTS_MAX                  0x7f
#define AK5811_REG_VTS                  0x0e

/* GPIO define */
#define AK5811_GPIO_IRQ               	66 //gpioc2
#define AK5811_GPIO_RESET               64 //gpioc0
#define AK5811_GPIO_PWDN                65 //gpioc1
#define AK5811_GPIO_TRIGGER		68 //gpioc4
#define AK5811_GPIO_AKM_PWR_EN		32 //gpiob0
#define AK5811_GPIO_AKM_DVDD_3V3_EN_N	71 //gpioc7
#define AK5811_GPIO_I2CEN		67 //gpiob1

/* page define */
#define AK5811_PAGE0                0x00    /* page 0 */
#define AK5811_PAGE2                0x02    /* page 2 */
#define AK5811_PAGE3                0x03    /* page 3 */
#define AK5811_PAGE4                0x04    /* page 4 */
#define AK5811_PAGE5                0x05    /* page 5 */
#define AK5811_PAGE6                0x06    /* page 6 */
#define AK5811_PAGE9                0x09    /* page 9 */
#define AK5811_PAGE10               0x0a    /* page 10 */
#define AK5811_PAGE11               0x0b    /* page 11 */
#define AK5811_PAGE12               0x0c    /* page 12 */
#define AK5811_PAGE13               0x0d    /* page 13 */
#define AK5811_PAGE15               0x0f    /* page 15 */
#define AK5811_PAGE17               0x11    /* page 17 */
#define AK5811_PAGE19               0x13    /* page 19 */
#define AK5811_PAGE25               0x19    /* page 25 */

/* reg define */
#define AK5811_REG_SETTING_PAGE		0x02	/* register page setting */
#define AK5811_REG_MONITOR_STATE        0x08    /* AK erro status reg */
#define AK5811_REG_STATE                0x09    /* core register for status */

/* AK5811 STATES */
#define AK5811_STATE_PDN                0x00
#define AK5811_STATE_LPW                0x01
#define AK5811_STATE_TRX                0x04
#define AK5811_STATE_STBY               0x03
#define AK5811_STATE_SLP                0x08

/* define a timer */
struct timer_list timer;

struct regval {
	u8 addr;
	u8 val;
};

struct ak5811_mode {
	u32 width;
	u32 height;
	u32 max_fps;
	u32 hts_def;
	u32 vts_def;
	u32 exp_def;
	const struct regval *reg_list;
};

struct ak5811 {
	struct spi_device	*spi;
        int                     irq_gpio; /* ak5811 irq gpio */
        int                     reset_gpio;
        int                     pwdn_gpio;
        int                     trigger_gpio;
        int                     power1_gpio;
        int                     power2_gpio;
        int                     i2cen_gpio;
	struct pinctrl		*pinctrl;

	struct v4l2_subdev	subdev;
	struct media_pad	pad;
	struct v4l2_ctrl_handler ctrl_handler;
	struct v4l2_ctrl	*exposure;
	struct v4l2_ctrl	*anal_gain;
	struct v4l2_ctrl	*digi_gain;
	struct v4l2_ctrl	*hblank;
	struct v4l2_ctrl	*vblank;
	struct mutex		mutex;
	bool			streaming;
	bool			power_on;
	const struct ak5811_mode *cur_mode;
	u32			module_index;
	const char		*module_name;
	const char		*len_name;
};

/*global paraments define*/
#define TXE		0x1f
#define RXE		0x0f
#define PINASSIGN	0

#define FFIX		0x097200
#define TXG 		0xFF
#define RXG 		0x00


int ak5811_trxset(struct ak5811 *ak5811);
int ak5811_partial_test(struct ak5811 *ak5811);

#define to_ak5811(sd) container_of(sd, struct ak5811, subdev)


/*
 * Xclk 37.125Mhz
 */
static const struct regval ak5811_global_regs[] = {
	{REG_NULL, 0x00},
};

/*
 * Xclk 37.125Mhz
 * max_framerate 30fps
 * LVDS_datarate per lane 222.75Mbps
 */
static const struct regval ak5811_1920x1080_regs[] = {
	{REG_NULL, 0x00},
};

static const struct ak5811_mode supported_modes[] = {
	{
		.width = 1024,
		.height = 512,
		.max_fps = 30,
		.exp_def = 0x0450,
		.hts_def = 0x100 * 4,
		.vts_def = 0x10,
		.reg_list = NULL,
	},

};

static const s64 link_freq_menu_items[] = {
	AK5811_LINK_FREQ
};

int ak5811_spi_write(struct spi_device *spi, const void *tx_buf,size_t len)
{
        struct spi_transfer     t = {
                        .tx_buf         = tx_buf,
                        .len            = len,
                };
        struct spi_message      m;

        spi_message_init(&m);
        spi_message_add_tail(&t, &m);
        return spi_sync(spi, &m);
}

/* Write registers up to 4 at a time */
static int ak5811_write_reg(struct spi_device *spi, u8 reg, u8 val)
{
	int ret = 0;
	u8 txbuf[3] = {0};
#if 0
	printk("!!!!!!!!!!!!!!Write reg 0x%02x , val = 0x%02x\n", reg, val);
#endif
	txbuf[0] = (reg << 1) & 0xff;
	txbuf[1] = val;
	ret = ak5811_spi_write(spi, txbuf, sizeof(txbuf));
	return ret;
}

static int ak5811_write_array(struct spi_device *spi,
			      const struct regval *regs)
{
	u32 i;
	int ret = 0;

	for (i = 0; regs[i].addr != REG_NULL; i++)
		ret = ak5811_write_reg(spi, regs[i].addr,
					regs[i].val);

	return ret;
}

int ak5811_spi_write_and_read(struct spi_device *spi, const void *tx_buf,
                        void *rx_buf, size_t len)
{
        struct spi_transfer     t = {
                        .tx_buf         = tx_buf,
                        .rx_buf         = rx_buf,
                        .len            = len,
                };
        struct spi_message      m;

        spi_message_init(&m);
        spi_message_add_tail(&t, &m);
        return spi_sync(spi, &m);
}


/* Read registers up to 4 at a time */
static int ak5811_read_reg(struct spi_device *spi, u8 reg, u8 *val)
{
	int ret = 0;
	u8 txbuf[3] = {0};
	u8 buf_val[3] = {0};

	txbuf[0] = ((reg << 1) | 0x80 ) & 0xff;
	ret = ak5811_spi_write_and_read(spi, txbuf, buf_val, 3);
	*val = buf_val[1];
#if 0
	printk("Read reg 0x%02x val = 0x%02x\n", reg, *val);
#endif
	return ret;
}

static int ak5811_get_reso_dist(const struct ak5811_mode *mode,
				struct v4l2_mbus_framefmt *framefmt)
{
	return abs(mode->width - framefmt->width) +
	       abs(mode->height - framefmt->height);
}

static const struct ak5811_mode *
ak5811_find_best_fit(struct v4l2_subdev_format *fmt)
{
	struct v4l2_mbus_framefmt *framefmt = &fmt->format;
	int dist;
	int cur_best_fit = 0;
	int cur_best_fit_dist = -1;
	unsigned int i;

	for (i = 0; i < ARRAY_SIZE(supported_modes); i++) {
		dist = ak5811_get_reso_dist(&supported_modes[i], framefmt);
		if (cur_best_fit_dist == -1 || dist < cur_best_fit_dist) {
			cur_best_fit_dist = dist;
			cur_best_fit = i;
		}
	}

	return &supported_modes[0];
}

static int ak5811_set_fmt(struct v4l2_subdev *sd,
			  struct v4l2_subdev_pad_config *cfg,
			  struct v4l2_subdev_format *fmt)
{
	struct ak5811 *ak5811 = to_ak5811(sd);
	const struct ak5811_mode *mode;
	s64 h_blank, vblank_def;

	mutex_lock(&ak5811->mutex);

	mode = ak5811_find_best_fit(fmt);
	fmt->format.code = AK5811_DATA_FORMAT;
	fmt->format.width = mode->width;
	fmt->format.height = mode->height;
	fmt->format.field = V4L2_FIELD_NONE;
	if (fmt->which == V4L2_SUBDEV_FORMAT_TRY) {
#ifdef CONFIG_VIDEO_V4L2_SUBDEV_API
		*v4l2_subdev_get_try_format(sd, cfg, fmt->pad) = fmt->format;
#else
		mutex_unlock(&ak5811->mutex);
		return -ENOTTY;
#endif
	} else {
		ak5811->cur_mode = mode;
		h_blank = mode->hts_def - mode->width;
		__v4l2_ctrl_modify_range(ak5811->hblank, h_blank,
					 h_blank, 1, h_blank);
		vblank_def = mode->vts_def - mode->height;
		__v4l2_ctrl_modify_range(ak5811->vblank, vblank_def,
					 AK5811_VTS_MAX - mode->height,
					 1, vblank_def);
	}

	mutex_unlock(&ak5811->mutex);

	return 0;
}

static int ak5811_get_fmt(struct v4l2_subdev *sd,
			  struct v4l2_subdev_pad_config *cfg,
			  struct v4l2_subdev_format *fmt)
{
	struct ak5811 *ak5811 = to_ak5811(sd);
	const struct ak5811_mode *mode = ak5811->cur_mode;

	mutex_lock(&ak5811->mutex);
	if (fmt->which == V4L2_SUBDEV_FORMAT_TRY) {
#ifdef CONFIG_VIDEO_V4L2_SUBDEV_API
		fmt->format = *v4l2_subdev_get_try_format(sd, cfg, fmt->pad);
#else
		mutex_unlock(&ak5811->mutex);
		return -ENOTTY;
#endif
	} else {
		fmt->format.width = mode->width;
		fmt->format.height = mode->height;
		fmt->format.code = AK5811_DATA_FORMAT;
		fmt->format.field = V4L2_FIELD_NONE;
	}
	mutex_unlock(&ak5811->mutex);
	return 0;
}

static int ak5811_enum_mbus_code(struct v4l2_subdev *sd,
				 struct v4l2_subdev_pad_config *cfg,
				 struct v4l2_subdev_mbus_code_enum *code)
{
	if (code->index != 0)
		return -EINVAL;
	code->code = AK5811_DATA_FORMAT;

	return 0;
}

static int ak5811_enum_frame_sizes(struct v4l2_subdev *sd,
				   struct v4l2_subdev_pad_config *cfg,
				   struct v4l2_subdev_frame_size_enum *fse)
{
	if (fse->index >= ARRAY_SIZE(supported_modes))
		return -EINVAL;

	if (fse->code != AK5811_DATA_FORMAT)
		return -EINVAL;

	fse->min_width = supported_modes[fse->index].width;
	fse->max_width = supported_modes[fse->index].width;
	fse->max_height = supported_modes[fse->index].height;
	fse->min_height = supported_modes[fse->index].height;

	return 0;
}

static int ak5811_g_frame_interval(struct v4l2_subdev *sd,
				   struct v4l2_subdev_frame_interval *fi)
{
	struct ak5811 *ak5811 = to_ak5811(sd);
	const struct ak5811_mode *mode = ak5811->cur_mode;

	mutex_lock(&ak5811->mutex);
	fi->interval.numerator = 10000;
	fi->interval.denominator = mode->max_fps * 10000;
	mutex_unlock(&ak5811->mutex);

	return 0;
}


static int __ak5811_start_stream(struct ak5811 *ak5811)
{
	int ret;
	int i = 0;

	/* In case these controls are set before streaming */
	mutex_unlock(&ak5811->mutex);
	ret = v4l2_ctrl_handler_setup(&ak5811->ctrl_handler);
	mutex_lock(&ak5811->mutex);
	if (ret)
		return ret;

        for ( i = 0; i < 1; i++){
		//do_gettimeofday(&tv);
		//printk("time cousumption of before trxset (ms)>>>>tv.tv_sec:%d,  tv.tv_usec%d\n",tv.tv_sec,tv.tv_usec);
                ret = ak5811_trxset(ak5811);
		//do_gettimeofday(&tv);
		//printk("time cousumption of after trxset (ms)>>>>tv.tv_sec:%d,  tv.tv_usec%d\n",tv.tv_sec,tv.tv_usec);

        }

	return ret;
}

static int __ak5811_stop_stream(struct ak5811 *ak5811)
{
        struct regval param_set[] = {
                {AK5811_REG_SETTING_PAGE, AK5811_PAGE0},
                {0x0a, 0x02},
                {REG_NULL, 0x00},
        };

        struct regval sleep_set[] = {
                {AK5811_REG_SETTING_PAGE, AK5811_PAGE0},
                {0x0a, 0x04},
                {REG_NULL, 0x00},
        };


        /* to stby */
        ak5811_write_array(ak5811->spi, param_set);
	udelay(100); /* from datasheet, TRX TO STBY is 100us */
	/* partial test */
	ak5811_partial_test(ak5811);
	/* to sleep */
        ak5811_write_array(ak5811->spi, sleep_set);

	return 0;
}

static int ak5811_s_stream(struct v4l2_subdev *sd, int on)
{
	struct ak5811 *ak5811 = to_ak5811(sd);
	struct spi_device *spi = ak5811->spi;
	int ret = 0;
	//struct timeval tv;

	mutex_lock(&ak5811->mutex);
	on = !!on;
	if (on == ak5811->streaming)
		goto unlock_and_return;

	if (on) {
		//do_gettimeofday(&tv);
		//printk("---------------------->>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>\n");
		//printk("time cousumption of before s_stream (ms)>>>>tv.tv_sec:%d,  tv.tv_usec%d\n",tv.tv_sec,tv.tv_usec);
		ret = pm_runtime_get_sync(&spi->dev);
		if (ret < 0) {
			pm_runtime_put_noidle(&spi->dev);
			goto unlock_and_return;
		}

		ret = __ak5811_start_stream(ak5811);
		if (ret) {
			v4l2_err(sd, "start stream failed while write regs\n");
			pm_runtime_put(&spi->dev);
			goto unlock_and_return;
		}
	} else {
		__ak5811_stop_stream(ak5811);
		pm_runtime_put(&spi->dev);
	}
		//do_gettimeofday(&tv);
		//printk("time cousumption of after s_stream (ms)>>>>tv.tv_sec:%d,  tv.tv_usec%d\n",tv.tv_sec,tv.tv_usec);

	ak5811->streaming = on;

unlock_and_return:
	mutex_unlock(&ak5811->mutex);

	return ret;
}

static int ak5811_s_power(struct v4l2_subdev *sd, int on)
{
	struct ak5811 *ak5811 = to_ak5811(sd);
	struct spi_device *spi = ak5811->spi;
	int ret = 0;

	mutex_lock(&ak5811->mutex);

	/* If the power state is not modified - no work to do. */
	if (ak5811->power_on == !!on)
		goto unlock_and_return;

	if (on) {
		ret = pm_runtime_get_sync(&spi->dev);
		if (ret < 0) {
			pm_runtime_put_noidle(&spi->dev);
			goto unlock_and_return;
		}

		ret = ak5811_write_array(ak5811->spi, ak5811_global_regs);
		if (ret) {
			v4l2_err(sd, "could not set init registers\n");
			pm_runtime_put_noidle(&spi->dev);
			goto unlock_and_return;
		}

		ak5811->power_on = true;
	} else {
		pm_runtime_put(&spi->dev);
		ak5811->power_on = false;
	}

unlock_and_return:
	mutex_unlock(&ak5811->mutex);

	return ret;
}

static int __ak5811_power_on(struct ak5811 *ak5811)
{
        printk("!!!!Enter %s\n",__func__);

        msleep(10);

        gpio_set_value(ak5811->pwdn_gpio, 1);

        msleep(100);

	return 0;

}

static void __ak5811_power_off(struct ak5811 *ak5811)
{

}

static int ak5811_runtime_resume(struct device *dev)
{
	struct spi_device *spi = to_spi_device(dev);
	struct v4l2_subdev *sd = spi_get_drvdata(spi);
	struct ak5811 *ak5811 = to_ak5811(sd);

	return __ak5811_power_on(ak5811);
}

static int ak5811_runtime_suspend(struct device *dev)
{
	struct spi_device *spi = to_spi_device(dev);
	struct v4l2_subdev *sd = spi_get_drvdata(spi);
	struct ak5811 *ak5811 = to_ak5811(sd);

	__ak5811_power_off(ak5811);

	return 0;
}

#ifdef CONFIG_VIDEO_V4L2_SUBDEV_API
static int ak5811_open(struct v4l2_subdev *sd, struct v4l2_subdev_fh *fh)
{
	struct ak5811 *ak5811 = to_ak5811(sd);
	struct v4l2_mbus_framefmt *try_fmt =
				v4l2_subdev_get_try_format(sd, fh->pad, 0);
	const struct ak5811_mode *def_mode = &supported_modes[0];

	mutex_lock(&ak5811->mutex);
	/* Initialize try_fmt */
	try_fmt->width = def_mode->width;
	try_fmt->height = def_mode->height;
	try_fmt->code = AK5811_DATA_FORMAT;
	try_fmt->field = V4L2_FIELD_NONE;

	mutex_unlock(&ak5811->mutex);
	/* No crop or compose */

	return 0;
}
#endif

static const struct dev_pm_ops ak5811_pm_ops = {
	SET_RUNTIME_PM_OPS(ak5811_runtime_suspend,
			   ak5811_runtime_resume, NULL)
};

#ifdef CONFIG_VIDEO_V4L2_SUBDEV_API
static const struct v4l2_subdev_internal_ops ak5811_internal_ops = {
	.open = ak5811_open,
};
#endif

static const struct v4l2_subdev_core_ops ak5811_core_ops = {
	.s_power = ak5811_s_power,
};

static const struct v4l2_subdev_video_ops ak5811_video_ops = {
	.s_stream = ak5811_s_stream,
	.g_frame_interval = ak5811_g_frame_interval,
};

static const struct v4l2_subdev_pad_ops ak5811_pad_ops = {
	.enum_mbus_code = ak5811_enum_mbus_code,
	.enum_frame_size = ak5811_enum_frame_sizes,
	.get_fmt = ak5811_get_fmt,
	.set_fmt = ak5811_set_fmt,
};

static const struct v4l2_subdev_ops ak5811_subdev_ops = {
	.core	= &ak5811_core_ops,
	.video	= &ak5811_video_ops,
	.pad	= &ak5811_pad_ops,
};

static int ak5811_set_ctrl(struct v4l2_ctrl *ctrl)
{
	struct ak5811 *ak5811 = container_of(ctrl->handler,
					     struct ak5811, ctrl_handler);
	struct spi_device *spi = ak5811->spi;
	int ret = 0;

	/* Propagate change of current control to all related controls */

	if (pm_runtime_get(&spi->dev) <= 0)
		return 0;

	pm_runtime_put(&spi->dev);

	return ret;
}

static const struct v4l2_ctrl_ops ak5811_ctrl_ops = {
	.s_ctrl = ak5811_set_ctrl,
};

static int ak5811_initialize_controls(struct ak5811 *ak5811)
{
	const struct ak5811_mode *mode;
	struct v4l2_ctrl_handler *handler;
	struct v4l2_ctrl *ctrl;
	s64 vblank_def;
	u32 h_blank;
	int ret;

	handler = &ak5811->ctrl_handler;
	mode = ak5811->cur_mode;
	ret = v4l2_ctrl_handler_init(handler, 8);
	if (ret)
		return ret;
	handler->lock = &ak5811->mutex;

	ctrl = v4l2_ctrl_new_int_menu(handler, NULL, V4L2_CID_LINK_FREQ,
				      0, 0, link_freq_menu_items);
	if (ctrl)
		ctrl->flags |= V4L2_CTRL_FLAG_READ_ONLY;

	v4l2_ctrl_new_std(handler, NULL, V4L2_CID_PIXEL_RATE,
			  0, AK5811_PIXEL_RATE, 1, AK5811_PIXEL_RATE);

	h_blank = mode->hts_def - mode->width;
	ak5811->hblank = v4l2_ctrl_new_std(handler, NULL, V4L2_CID_HBLANK,
				h_blank, h_blank, 1, h_blank);
	if (ak5811->hblank)
		ak5811->hblank->flags |= V4L2_CTRL_FLAG_READ_ONLY;

	vblank_def = mode->vts_def - mode->height;
	ak5811->vblank = v4l2_ctrl_new_std(handler, &ak5811_ctrl_ops,
				V4L2_CID_VBLANK, vblank_def,
				AK5811_VTS_MAX - mode->height,
				1, vblank_def);

/*
	exposure_max = mode->vts_def - 4;
	ak5811->exposure = v4l2_ctrl_new_std(handler, &ak5811_ctrl_ops,
				V4L2_CID_EXPOSURE, AK5811_EXPOSURE_MIN,
				exposure_max, AK5811_EXPOSURE_STEP,
				mode->exp_def);

	ak5811->anal_gain = v4l2_ctrl_new_std(handler, &ak5811_ctrl_ops,
				V4L2_CID_ANALOGUE_GAIN, AK5811_GAIN_MIN,
				AK5811_GAIN_MAX, AK5811_GAIN_STEP,
				AK5811_GAIN_DEFAULT);

*/
	if (handler->error) {
		ret = handler->error;
		dev_err(&ak5811->spi->dev,
			"Failed to init controls(%d)\n", ret);
		goto err_free_handler;
	}

	ak5811->subdev.ctrl_handler = handler;

	return 0;

err_free_handler:
	v4l2_ctrl_handler_free(handler);

	return ret;
}


static int ak5811_check_sensor_id(struct spi_device *spi)
{
	struct device *dev = &spi->dev;
	int ret = 0;
	u8 id = 0;

	ret = ak5811_read_reg(spi,
		AK5811_REG_CHIP_ID & 0xff, &id);
	if (id != CHIP_ID) {
		dev_err(dev, "Unexpected sensor id(%02x), ret(%d)\n", id, ret);
		return -ENODEV;
	}
	dev_info(dev, "Detected ak5811 id:%02x\n", id);
	return 0;
}

/*
 * func: to get current chip status
 * @mode: which stat you want to check
 *      01: AK5811_STATE_LPW
 * */
static int ak5811_check_state(struct ak5811 *ak5811, u8 mode)
{
        u8 val = 0;

        ak5811_read_reg(ak5811->spi, AK5811_REG_STATE, &val);
        printk("!!!!!Read chip status, register=0x09, val=%02x\n",val);
        if ( (val & 0x0f) == mode){
                return 0;
        }else{
                return -EIO;
        }
}

/*
 * func: ak5811_check_monitor status
 * reg: core register 0x08, Monitoring errors indication
*/
static int ak5811_check_monitor_stat(struct ak5811 *ak5811)
{
        u8 val =0;

        ak5811_read_reg(ak5811->spi, AK5811_REG_MONITOR_STATE, &val);
        printk("!!!!!Read monitoring erro status, register=0x08, val=%02x\n",val);
        if ( val == 0){
                return 0;
        }else{
                return val;
        }
}

/*
 * func: spi communication errors indication
*/
static int ak5811_spi_check(struct ak5811 *ak5811)
{
	u8 val = 0;
	ak5811_read_reg(ak5811->spi, 0x07, &val);
	if (val == 0){
		return 0;
	}else{
		printk("Spi communication error indication, val = 0x%02x\n",val);
		return val;
	}
}

static int ak5811_iref_check(struct ak5811 *ak5811)
{
	u8 val = 0;

	/* set to page 0 */
	ak5811_write_reg(ak5811->spi, AK5811_REG_SETTING_PAGE, AK5811_PAGE0);
	/* Crystal frequency setting and Internal LO path disable */
	ak5811_write_reg(ak5811->spi, 0x06, 0x22);
	ak5811_write_reg(ak5811->spi, 0x05, 0x03); //CTCHK_REFCLK_E and CTCHK_E
	ak5811_write_reg(ak5811->spi, 0x0f, 0x02); //IREFCAL set
	ak5811_write_reg(ak5811->spi, 0x0f, 0x03); //IREFCAL start
	msleep(100);
	ak5811_read_reg(ak5811->spi, 0x0f, &val);
	if (val == 0x12){
		return 0;
	}else{
		return -EIO;
	}
}

/*
 * VCOTGTSEL (TX output center frequency)
*/

void ak5811_vcotgtsel(struct ak5811 *ak5811)
{

	/* set to page 11 */
	ak5811_write_reg(ak5811->spi, AK5811_REG_SETTING_PAGE, AK5811_PAGE11);
	ak5811_write_reg(ak5811->spi, 0x0c, 0x05); // tx output center frequency

	/* set to page 3 */
	ak5811_write_reg(ak5811->spi, AK5811_REG_SETTING_PAGE, AK5811_PAGE3);
	ak5811_write_reg(ak5811->spi, 0x1a, ((FFIX & 0x0F0000) >> 16)); // CW frequency setting
	ak5811_write_reg(ak5811->spi, 0x1b, ((FFIX & 0x00FF00) >> 8)); // Chip1 SFFIX(CW frequency setting)
	ak5811_write_reg(ak5811->spi, 0x1c, ((FFIX & 0x0000FF))); // Chip1 SFFIX(CW frequency setting)
	ak5811_write_reg(ak5811->spi, 0x1e, 0x98); // Chip1 SFLIMITHI(Frequency chiirp upper limit)
	ak5811_write_reg(ak5811->spi, 0x1f, 0x77); // Chip1 SFLIMITHI(Frequency chiirp upper limit)
	ak5811_write_reg(ak5811->spi, 0x20, 0x83); // Chip1 SFLIMITHI(Frequency chiirp low limit)
	ak5811_write_reg(ak5811->spi, 0x21, 0x09); // Chip1 SFLIMITHI(Frequency chiirp low limit)
}

/*
 * Tx output magnitude setting
*/
void ak5811_tx_gain_set(struct ak5811 *ak5811)
{
        struct regval tx_gain_set[] = {
                {AK5811_REG_SETTING_PAGE, AK5811_PAGE11},
#if LP_DARKROOM_TEST
                {0x16, 0x00},
                {0x17, 0x66}, //TXGCAL1_TGT
                {0x18, 0x00},
                {0x19, 0xcc},//tx gain ch2
                {0x1A, 0x01},
                {0x1B, 0x33},//tx gain ch3
                {0x1C, 0x01},
                {0x1D, 0x99},//tx gain ch4
#endif
                {0x20, 0x7E},
                {0x21, 0x00},
                {0x22, 0x7E},
                {0x23, 0x00},
                {0x24, 0x7E},
                {0x25, 0x00},
                {0x26, 0x7E},
                {0x27, 0x00},
                {REG_NULL, 0x00},
        };
        ak5811_write_array(ak5811->spi, tx_gain_set);

}

/*
 * Tx output phase setting
*/
void ak5811_tx_phase_set(struct ak5811 *ak5811)
{
        struct regval phase_set[] = {
                {AK5811_REG_SETTING_PAGE, AK5811_PAGE11},
                {0x28, 0x80},
                {REG_NULL, 0x00},
        };

	ak5811_write_array(ak5811->spi, phase_set);
}

/* activate channel setting */
void ak5811_activate_channel_set(struct ak5811 *ak5811)
{
	/* set to page 3 */
	ak5811_write_reg(ak5811->spi, AK5811_REG_SETTING_PAGE, AK5811_PAGE3);
	ak5811_write_reg(ak5811->spi, 0x0a, TXE); //TXE(TX output channel select) ALL channles
	ak5811_write_reg(ak5811->spi, AK5811_REG_SETTING_PAGE, AK5811_PAGE0);
	ak5811_write_reg(ak5811->spi, 0x19, RXE); //RXE(RX input channel select) ALL channles
}

/*
 * RX path gain setting
*/
void ak5811_rx_gain_set(struct ak5811 *ak5811)
{
	struct regval gain_set[] = {
                {AK5811_REG_SETTING_PAGE, AK5811_PAGE0},
		{0x1B, RXG}, // T_RXGCAL_PGAGAIN
                {AK5811_REG_SETTING_PAGE, AK5811_PAGE10},
		{0x0B, 0x66},
		{0x0C, 0x00}, //RX gain calibration reference level setting 1
		{0x0D, 0x66},
		{0x0E, 0x00}, //RX gain calibration reference level setting 2
		{0x0F, 0x66},
		{0x10, 0x00}, // set level to 3
		{0x11, 0x66},
		{0x12, 0x00}, // set level to 4
                {REG_NULL, 0x00},
	};

	printk("RX path gain setting\n");
	ak5811_write_array(ak5811->spi, gain_set);
}

/*
 * Tx path gain setting
*/
void ak5811_tx_path_gain_set(struct ak5811 *ak5811)
{
	struct regval gain_set[] = {
                {AK5811_REG_SETTING_PAGE, AK5811_PAGE11},
		{0x20, 0x7E},
		{0x21, 0x00},
		{0x22, 0x7E},
		{0x23, 0x00},
		{0x24, 0x7E},
		{0x25, 0x00},
		{0x26, 0x7E},
		{0x27, 0x00},
                {REG_NULL, 0x00},
	};

	return;
	printk("TX path gain setting\n");
	ak5811_write_array(ak5811->spi, gain_set);
}

/*
 * RX path High Pass Filter setting
*/
void ak5811_rx_filter_set(struct ak5811 *ak5811)
{
	int hpfset = 0xe4;

	printk("RX path High Pass Filter setting \n");
	ak5811_write_reg(ak5811->spi, AK5811_REG_SETTING_PAGE, AK5811_PAGE0);
	ak5811_write_reg(ak5811->spi, 0x1A, hpfset); //RX HPF select

}

/*
 * TD MIMO setting
*/
void ak5811_td_mimo_set(struct ak5811 *ak5811)
{
        struct regval mimo_set[] = {
                {AK5811_REG_SETTING_PAGE, AK5811_PAGE3},
                {0x0B, 0x03},
                {0x0C, 0x11},
                {0x0D, 0x11},
                {0x0E, 0x22},
                {0x0F, 0x22},
                {0x10, 0x44},
                {0x11, 0x44},
                {0x12, 0x88},
                {0x13, 0x88},
                {REG_NULL, 0x00},
        };

	printk("TD MIMO setting\n");
	ak5811_write_array(ak5811->spi, mimo_set);
	msleep(10);
}

void test_erro_check(struct ak5811 *ak5811)
{
	u8 val  = 0;

	ak5811_write_reg(ak5811->spi, 0x08, 0x20); //Synthesizer error clear
	/* set to page 5 */
	ak5811_write_reg(ak5811->spi, AK5811_REG_SETTING_PAGE, AK5811_PAGE5);
	ak5811_read_reg(ak5811->spi, 0x0A, &val); //Error check
	if (val != 0){
		ak5811_read_reg(ak5811->spi, 0x2F, &val);
		if (val != 0){
			printk("Error in RXG calibration (0x2F). Value: 0x%02x\n", val);
		}

		ak5811_read_reg(ak5811->spi, 0x30, &val);
		if (val != 0){
			printk("Error in RXG calibration (0x30). Value: 0x%02x\n", val);
		}

		ak5811_read_reg(ak5811->spi, 0x31, &val);
		if (val != 0){
			printk("Error in RXG calibration (0x31). Value: 0x%02x\n", val);
		}

		ak5811_read_reg(ak5811->spi, 0x32, &val);
		if (val != 0){
			printk("Error in RXG calibration (0x32). Value: 0x%02x\n", val);
		}

		ak5811_read_reg(ak5811->spi, 0x33, &val);
		if (val != 0){
			printk("Error in RXG calibration (0x33). Value: 0x%02x\n", val);
		}

		ak5811_read_reg(ak5811->spi, 0x34, &val);
		if (val != 0){
			printk("Error in RXPH calibration (0x34). Value: 0x%02x\n", val);
		}

		ak5811_read_reg(ak5811->spi, 0x35, &val);
		if (val != 0){
			printk("Error in RXPH calibration (0x35). Value: 0x%02x\n", val);
		}

		ak5811_read_reg(ak5811->spi, 0x36, &val);
		if (val != 0){
			printk("Error in RXPH calibration (0x36). Value: 0x%02x\n", val);
		}

		ak5811_read_reg(ak5811->spi, 0x37, &val);
		if (val != 0){
			printk("Error in RXPH calibration (0x37). Value: 0x%02x\n", val);
		}
	}
	ak5811_write_reg(ak5811->spi, 0x0A, 0x80); //Error clear
}

/* partial calibration test */
int ak5811_partial_test(struct ak5811 *ak5811)
{
	u8 val  = 0;

	printk("Partial calibration start\n");
	ak5811_write_reg(ak5811->spi, AK5811_REG_SETTING_PAGE, AK5811_PAGE0);
	/* Partial calibration LO supply prohibited items */
	ak5811_write_reg(ak5811->spi, 0x11, 0x21); //STBCHK=1
	msleep(5);
	ak5811_read_reg(ak5811->spi, 0x11, &val); //complete check
	if (val != 0x20){
		printk("Error: Partial diagnosis (LO prohibited) hasn't been completed\n");
	}
	test_erro_check(ak5811);

	ak5811_write_reg(ak5811->spi, AK5811_REG_SETTING_PAGE, AK5811_PAGE0);
	/* Partial calibration LO supply requirement items */
	ak5811_write_reg(ak5811->spi, 0x11, 0x31); //STBCHK=1
	msleep(5);
	ak5811_read_reg(ak5811->spi, 0x11, &val); //complete check
	if (val != 0x30){
		printk("Error: Partial diagnosis (LO requirement) hasn't been completed\n");
	}
	test_erro_check(ak5811);
	return 0;
}

/*
 * self test
 * Full chip diagnosis and increment calibration
*/
int ak5811_self_test(struct ak5811 *ak5811)
{
	u8 val  = 0;
	struct regval diag_cal_set[] = {
		{AK5811_REG_SETTING_PAGE, AK5811_PAGE9},
		{0x0A, 0x0F},
		{0x0B, 0x3B},
		{0x0C, 0x62},
		{0x0E, 0x0F},
		{0x0F, 0xFF},
		{0x10, 0x7D},
		{0x16, 0x07},
		{0x17, 0xf6},//{0x17, 0xff}
		{0x18, 0x7c},
                {REG_NULL, 0x00},
	};
	printk("Full-chip diagnosis & Increment calibration\n");
	ak5811_write_array(ak5811->spi, diag_cal_set);
	msleep(10);
	printk("Full-chip diagnosis (w/o LO input) start\n");
	/* set to page 0 */
	ak5811_write_reg(ak5811->spi, AK5811_REG_SETTING_PAGE, AK5811_PAGE0);
	ak5811_write_reg(ak5811->spi, 0x11, 0x01); //STBCHK
	msleep(300);
	ak5811_read_reg(ak5811->spi, 0x11, &val);
	if (val != 0){
		printk("Erro: Rull-chip diagnosis (w/o LO input) hasn't been completed\n");
		return -EIO;
	}
	ak5811_write_reg(ak5811->spi, 0x08, 0x20); //Synthesizer error clear
	/* set to page 5 */
	ak5811_write_reg(ak5811->spi, AK5811_REG_SETTING_PAGE, AK5811_PAGE5);
	ak5811_read_reg(ak5811->spi, 0x0A, &val);
	if (val != 0){
		ak5811_read_reg(ak5811->spi, 0x0B, &val);
		if(val != 0){
			printk("Error in Synthesizer calibration. Value: 0x%02x\n", val);
		}
		ak5811_read_reg(ak5811->spi, 0x0C, &val);
		if (val != 0){
			printk("Error in DAC/IREF/CVCO/LIPWD/RIPWD/RLPWD calibration. Value: 0x%02x\n", val);
		}
		ak5811_read_reg(ak5811->spi, 0x0D, &val);
		if (val != 0){
			printk("Error in ADQ/AD/PGA/CPG/RRC calibration. Value: 0x%02x\n", val);
		}
		ak5811_read_reg(ak5811->spi, 0x0E, &val);
		if (val == 1){
			/* set to page 6 */
			ak5811_write_reg(ak5811->spi, AK5811_REG_SETTING_PAGE, AK5811_PAGE6);
			ak5811_read_reg(ak5811->spi, 0x10, &val); //Error detail read
			if ((val != 4) && (val != 6)){
				printk("Error in TXPH calibration\n. Value: 0x%02x\n", val);
			}

		}else if (val != 0){
			printk("ERROR in VCO/TDC/LIN/LOPWD/TPWD calibration. Value: 0x%02x\n", val);
		}
	}

	/* set to page 5 */
	ak5811_write_reg(ak5811->spi, AK5811_REG_SETTING_PAGE, AK5811_PAGE5);
	ak5811_write_reg(ak5811->spi, 0x0A, 0x80); //Error clear
	printk("Full-chip diagnosis (LO input) start\n");
	/* set to page 0 */
	ak5811_write_reg(ak5811->spi, AK5811_REG_SETTING_PAGE, AK5811_PAGE0);
	ak5811_write_reg(ak5811->spi, 0x06, 0x22); //internal LO path active
	msleep(10);
	ak5811_write_reg(ak5811->spi, 0x11, 0x11); //STBCHK=1
	msleep(10);
	ak5811_read_reg(ak5811->spi, 0x11, &val); //complete check
	if (val != 0x10){
		printk("Error: Full-chip diagnosis (LO input) hasn't been completed\n");
	}
	ak5811_write_reg(ak5811->spi, 0x08, 0x20); //Synthesizer error clear
	/* set to page 5 */
	ak5811_write_reg(ak5811->spi, AK5811_REG_SETTING_PAGE, AK5811_PAGE5);
	ak5811_read_reg(ak5811->spi, 0x0A, &val); //Error check
	if (val != 0){
		ak5811_read_reg(ak5811->spi, 0x2F, &val);
		if (val != 0){
			printk("Error in RXG calibration (0x2F). Value: 0x%02x\n", val);
		}

		ak5811_read_reg(ak5811->spi, 0x30, &val);
		if (val != 0){
			printk("Error in RXG calibration (0x30). Value: 0x%02x\n", val);
		}

		ak5811_read_reg(ak5811->spi, 0x31, &val);
		if (val != 0){
			printk("Error in RXG calibration (0x31). Value: 0x%02x\n", val);
		}

		ak5811_read_reg(ak5811->spi, 0x32, &val);
		if (val != 0){
			printk("Error in RXG calibration (0x32). Value: 0x%02x\n", val);
		}

		ak5811_read_reg(ak5811->spi, 0x33, &val);
		if (val != 0){
			printk("Error in RXG calibration (0x33). Value: 0x%02x\n", val);
		}

		ak5811_read_reg(ak5811->spi, 0x34, &val);
		if (val != 0){
			printk("Error in RXPH calibration (0x34). Value: 0x%02x\n", val);
		}

		ak5811_read_reg(ak5811->spi, 0x35, &val);
		if (val != 0){
			printk("Error in RXPH calibration (0x35). Value: 0x%02x\n", val);
		}

		ak5811_read_reg(ak5811->spi, 0x36, &val);
		if (val != 0){
			printk("Error in RXPH calibration (0x36). Value: 0x%02x\n", val);
		}

		ak5811_read_reg(ak5811->spi, 0x37, &val);
		if (val != 0){
			printk("Error in RXPH calibration (0x37). Value: 0x%02x\n", val);
		}
	}

	ak5811_write_reg(ak5811->spi, 0x0A, 0x80); // Error clear

	return 0;
}

/*
 * tx output max, txphcal, rxphcal set
*/
void ak5811_txrx_set(struct ak5811 *ak5811)
{
	struct regval tx_output[] = {
		{AK5811_REG_SETTING_PAGE, AK5811_PAGE17},
		{0x0A, TXG},
		{0x0B, TXG},
		{0x0C, TXG},
		{0x0D, TXG},
		{REG_NULL, 0x00},
	};
	struct regval ffix_set[] = {
		{AK5811_REG_SETTING_PAGE, AK5811_PAGE3},
		{0x1E, 0x97},
		{0x1F, 0x20},
		{0x20, 0x84},
		{0x21, 0x60},
                {REG_NULL, 0x00},
	};
	struct regval chirp_form[] = {
		{AK5811_REG_SETTING_PAGE, AK5811_PAGE4},
		{0x0A, 0x10},
		{0x0B, 0x00},
		{0x0C, 0x01},
		{0x15, 0xff},
		{0x16, 0xff},
		{0x17, 0xf1},
		{0x18, 0x8b},
		{0x1E, 0x00},
		{0x1F, 0x00},
		{0x20, 0x78},
		{0x21, 0x00},
		{0x10, 0x00},
		{0x11, 0xa0},
		{0x12, 0x00},
		{0x13, 0x05},
		{0x14, 0x30},
		{0x19, 0x00},
		{0x1A, 0x00},
		{0x1B, 0x00},
		{0x1C, 0x00},
		{0x1D, 0xa0},
		{0x0E, 0x02},
		{0x0F, 0x00},
                {REG_NULL, 0x00},
	};
	ak5811_write_array(ak5811->spi, tx_output);
	printk("Chirp Form setting\n");
	ak5811_write_array(ak5811->spi, ffix_set);
	ak5811_write_array(ak5811->spi, chirp_form);
}

/*
 * CSI 2 output setting
*/

void ak5811_csi2_set(struct ak5811 *ak5811)
{
	struct regval csi2_set[] = {
		{AK5811_REG_SETTING_PAGE, AK5811_PAGE0},
		{0x17, PINASSIGN},
		{0x20, 0x00},
		{0x1c, 0x02}, //20M RAD_RATE,01:53.3M
		{0x0D, 0x04},
		{0x18, 0xF7},
		{0x21, 0x78},
		{0x22, 0x01},
		{0x23, 0x00},
		{0x24, 0x00},
		{0x25, 0x9c},
		{0x26, 0x02},
		{0x27, 0x00},
		{0x33, 0x22},  //no sopport line sync
		{0x34, 0xFF},
		{0x35, 0xFF},
		{0x36, 0xFF},
		{0x37, 0xFF},
		{0x33, 0x32},  //no sopport line sync
		{0x0C, 0x02},
//		{0x20, 0x04}, //Continuous mode
		{0x20, 0x00}, //Inermittent mode
		{0x0C, 0x03},
                {REG_NULL, 0x00},
	};
	printk("CSI-2 output setting\n");
	ak5811_write_array(ak5811->spi, csi2_set);
	msleep(1);
	ak5811_write_reg(ak5811->spi, AK5811_REG_SETTING_PAGE, AK5811_PAGE0);
	ak5811_write_reg(ak5811->spi, 0x06, 0x22);
}


void ak5811_stby2trx_dur_set(struct ak5811 *ak5811)
{
        struct regval param_set[] = {
                {AK5811_REG_SETTING_PAGE, AK5811_PAGE0},
		{0x12, 0x00},
                {REG_NULL, 0x00},
        };

	printk("Set STBY2TRX DUR\n");
        ak5811_write_array(ak5811->spi, param_set);

}

/* loop filter setting */
void ak5811_loop_filter_set(struct ak5811 *ak5811)
{

        struct regval param_set[] = {
                {AK5811_REG_SETTING_PAGE, AK5811_PAGE19},
		{0x2a, 0x00},
		{0x2b, 0xd0},
		{0x2c, 0x00},
		{0x2d, 0x0a},
		{0x2e, 0x00},
		{0x2f, 0x29},
		{0x30, 0x0d},
		{0x31, 0xe8},
		{0x32, 0x06},
		{0x33, 0x54},
                {REG_NULL, 0x00},
        };
	printk("loop filter setting\n");
        ak5811_write_array(ak5811->spi, param_set);
}

void ak5811_extc_check(struct ak5811 *ak5811)
{
	u8 val = 0;
	/* set to page 0 */
	ak5811_write_reg(ak5811->spi, AK5811_REG_SETTING_PAGE, AK5811_PAGE0);
	ak5811_write_reg(ak5811->spi, 0x0f, 0x0d); //LPCHK_EXTC0_CAL1
	msleep(2);
	ak5811_read_reg(ak5811->spi, 0x0f, &val);
	if (val != 0x3c){
		printk("Erro: EXTC CAL hasn't been completed");
	}
	msleep(2);
	/* set to page 5 */
	ak5811_write_reg(ak5811->spi, AK5811_REG_SETTING_PAGE, AK5811_PAGE5);
	ak5811_read_reg(ak5811->spi, 0x0a, &val);
	if (val != 0x00){
		printk("Erro: read from page5 0x0a = 0x%02x\n", val);
		ak5811_write_reg(ak5811->spi, 0x0a, 0x80); //erro reset
	}
	ak5811_write_reg(ak5811->spi, AK5811_REG_SETTING_PAGE, AK5811_PAGE0);
	ak5811_write_reg(ak5811->spi, 0x0f, 0x00); //tdc reset
	ak5811_write_reg(ak5811->spi, 0x05, 0x03); //continuous error check on
	ak5811_write_reg(ak5811->spi, 0x08, 0x40); //clock error clear
}

static int ak5811_init(struct ak5811 *ak5811)
{
	int ret = 0;
	u8 val = 0;
#define ak_print(ret, str) \
        if(ret){ \
                printk(str); \
                return -EIO;    \
        };

	/* LPW state check */
	ret = ak5811_check_state(ak5811, AK5811_STATE_LPW);
	ak_print(ret, "SIF Error: IC is not in Low Power state\n");

	/* Monitoring errors indication check */
	ret = ak5811_check_monitor_stat(ak5811);
	ak_print(ret, "Erro detected in Power\n");

	/* Put reset pin high */
	gpio_set_value(ak5811->reset_gpio, 1);

	/* spi communication errors indication */
	ret = ak5811_spi_check(ak5811);
	ak_print(ret, "\n");

	ak5811_write_reg(ak5811->spi, 0x04, 0x5a); //T_MAGIC
	msleep(10);

	/* set to page3 */
	ak5811_write_reg(ak5811->spi, AK5811_REG_SETTING_PAGE, AK5811_PAGE3);
	ak5811_write_reg(ak5811->spi, 0x22, 0x03);
	ak5811_write_reg(ak5811->spi, 0x23, 0x76);
	/* loop filter setting */
	ak5811_loop_filter_set(ak5811);
	ak5811_write_reg(ak5811->spi, 0x05, 0x00); //CTCHK_E

	/* STB->TRX time setting */
	ak5811_stby2trx_dur_set(ak5811);
	/* set to page 19 */
	ak5811_write_reg(ak5811->spi, AK5811_REG_SETTING_PAGE, AK5811_PAGE19);
	ak5811_write_reg(ak5811->spi, 0x3c, 0x01); //Error reset (TRXREJECT_DIS)
	printk("Will do Lowpower check IREF\n");
	ret = ak5811_iref_check(ak5811);
	printk("Do Lowpower check IREF done\n");
	ak_print(ret, "ERROR: IREFCAL hasn't been completed\n");
	msleep(10);
	/* set to page 5 for reading error*/
	ak5811_write_reg(ak5811->spi, AK5811_REG_SETTING_PAGE, AK5811_PAGE5);
	ak5811_read_reg(ak5811->spi, 0x0a, &val);
	if (val != 0){
		ak5811_write_reg(ak5811->spi, 0x0a, 0x80); //error reset
		/* set to page 13 */
		ak5811_write_reg(ak5811->spi, AK5811_REG_SETTING_PAGE, AK5811_PAGE13);
		ak5811_write_reg(ak5811->spi, 0x10, 0x41); //IREFCAL set
		ak5811_write_reg(ak5811->spi, 0x11, 0x41); //IREFCAL set
	}

	/* lowpower extc check */
	printk("Will do EXTC check\n");
	ak5811_extc_check(ak5811);

	printk("TX output frequency range & initial frequency settings\n");
	ak5811_vcotgtsel(ak5811);

	printk("Tx output magnitude setting\n");
	ak5811_tx_gain_set(ak5811);

	printk("Tx output phase setting\n");
	ak5811_tx_phase_set(ak5811);
	printk("Active channel setting\n");
	ak5811_activate_channel_set(ak5811);

	/* rx path gain setting */
	ak5811_rx_gain_set(ak5811);

	/* RX path high pass filter setting */
	ak5811_rx_filter_set(ak5811);

	/* td mimo setting */
	ak5811_td_mimo_set(ak5811);
	/* trigger set */
	printk("Trigger setting\n");
	/* set to page 0 */
	ak5811_write_reg(ak5811->spi, AK5811_REG_SETTING_PAGE, AK5811_PAGE0);
#if LP_TRIGGER_REGISTER
	/* triggered by register */
	ak5811_write_reg(ak5811->spi, 0x13, 0x00);

#else
	/* triggered by pin */
	ak5811_write_reg(ak5811->spi, 0x13, 0x04);
#endif
	msleep(10);

	/* go to STBY */
	printk("Go to STBY\n");
	ak5811_write_reg(ak5811->spi, 0x0A, 0x02);
	msleep(10);
	ak5811_read_reg(ak5811->spi, 0x09, &val); //state check
	if (val != 0x03){
		printk("Error: not in standby state\n");
		return -EIO;
	}

	/* self test */
	if (ak5811_self_test(ak5811)){
		return -EIO;
	}

#if LP_TRIGGER_PIN
	/*Configure IRQ to output 10MHz frequency as synchronous clock*/
	ak5811_write_reg(ak5811->spi, AK5811_REG_SETTING_PAGE, AK5811_PAGE25);
	ak5811_write_reg(ak5811->spi, 0x0b, 0x14);
#endif

	/* tx and rx setting */
	ak5811_txrx_set(ak5811);
	/* csi2 set */
	ak5811_csi2_set(ak5811);
	printk("Ak5811 init done\n");
	return 0;
}

/* trigger by pin
 * 0:trigger by pin, 1:trigger by register
 */
void ak5811_trigger_set(struct ak5811 *ak5811, bool type)
{
	gpio_set_value(ak5811->trigger_gpio, 0);
	udelay(1);
	gpio_set_value(ak5811->trigger_gpio, 1);
	udelay(1);
	gpio_set_value(ak5811->trigger_gpio, 0);
}

/* transfer from STBY to TRX */
int ak5811_trxset(struct ak5811 *ak5811)
{
        int ret = 0;
        u8 val = 0;
        struct regval param_set[] = {
                {AK5811_REG_SETTING_PAGE, AK5811_PAGE0},
                {0x0a, 0x01},
                {REG_NULL, 0x00},
        };

#if LP_TRIGGER_REGISTER
        struct regval register_trigger[] = {
                {AK5811_REG_SETTING_PAGE, AK5811_PAGE0},
                {0x13, 0x01},
                {REG_NULL, 0x00},
        };
#endif
	/* check current state */
	ak5811_read_reg(ak5811->spi, 0x09, &val);
	if (val == 0x08){/* currnet is in sleep state */
		ak5811_write_reg(ak5811->spi, AK5811_REG_SETTING_PAGE, AK5811_PAGE0);
		ak5811_write_reg(ak5811->spi, 0x0A, 0x02);
		udelay(200); /* SLEEP TO STBY is 200us */
	}

        /* to trx */
        ret = ak5811_write_array(ak5811->spi, param_set);
        if (ret){
                printk("Erro to transfer from STBY to TRX\n");
                return -EIO;
        }
	mdelay(140);
        ret = ak5811_read_reg(ak5811->spi, AK5811_REG_STATE, &val);
        if (ret || (val != 0x04)){
                printk("Set to TRX erro, val = %02x\n",val);
                return -EIO;
        }

#if LP_TRIGGER_REGISTER
        /* register trigger start */
#if 0
        ret = ak5811_write_array(ak5811->spi, register_trigger);
        if (ret){
                printk("Erro to do register trigger\n");
                return -EIO;
        }
#endif
#else
	ak5811_trigger_set(ak5811, 0); //0:trigger by pin,1:trigger by register
#endif

        return 0;
}

static ssize_t ak5811_trig(struct device *dev,struct device_attribute *attr, const char *buf)
{

	struct regval register_trigger[] = {
                {AK5811_REG_SETTING_PAGE, AK5811_PAGE0},
                {0x13, 0x01},
                {REG_NULL, 0x00},
        };
	struct mutex lock;
	int ret = 0;
	int i = 0;
	int val = 2;

	mutex_init(&lock);
	printk(">>>>**********************************************trig start\n");
	ret = ak5811_write_array(g_spi_test_data.spi, register_trigger);
	if (ret){
		printk("Erro to do register trigger\n");
		return -EIO;
	}
	msleep(100);
	ret = ak5811_write_reg(g_spi_test_data.spi, AK5811_REG_SETTING_PAGE, AK5811_PAGE0);
	ret |= ak5811_read_reg(g_spi_test_data.spi, 0x13, &val);
	if (ret){
		printk("Erro to do register trigger\n");
		return -EIO;
	}
	printk(">>>>**********************************************%d\n",val);
	mutex_unlock(&lock);
	return ret;
}

static DEVICE_ATTR(trig,S_IRUGO|S_IWUSR, ak5811_trig,NULL);
static struct attribute *ak5811_trig_attrs[] = {
	&dev_attr_trig.attr,
	NULL,
};

const char ak5811_group_name[] = "ak5811_test";
static struct attribute_group ak5811_trig_attr_group = {
	.name   = ak5811_group_name,
	.attrs  = ak5811_trig_attrs,
};

static int ak5811_probe(struct spi_device *spi)
{
	struct device *dev = &spi->dev;
	struct ak5811 *ak5811;
	struct v4l2_subdev *sd;
	int ret;

	dev_info(dev, "driver version: %02x.%02x.%02x",
		DRIVER_VERSION >> 16,
		(DRIVER_VERSION & 0xff00) >> 8,
		DRIVER_VERSION & 0x00ff);

	ak5811 = devm_kzalloc(dev, sizeof(struct ak5811), GFP_KERNEL);
	if (!ak5811)
		return -ENOMEM;

//	spi->mode = SPI_MODE_3 | SPI_LSB_FIRST;
//	spi->irq = -1;
//	spi->max_speed_hz = 400000;
	spi->bits_per_word = 8;

#if LP_SPI_TEST /* add for test */
	g_spi_test_data.spi = spi;
	g_spi_test_data.dev = dev;
#endif
	ret = spi_setup(spi);
	if (ret < 0) {
		dev_err(dev, "could not setup spi!\n");
		return -EINVAL;
	}

	printk("%s:name=%s,bus_num=%d,cs=%d,mode=%d,speed=%d\n", __func__, spi->modalias, spi->master->bus_num, spi->chip_select, spi->mode, spi->max_speed_hz);

	ak5811->spi = spi;
	ak5811->cur_mode = &supported_modes[0];

	printk("!!!!reset gpio = %d, pwdn gpio = %d\n", AK5811_GPIO_RESET, AK5811_GPIO_PWDN);
	ak5811->irq_gpio = AK5811_GPIO_IRQ;
	ak5811->reset_gpio = AK5811_GPIO_RESET;
	ak5811->pwdn_gpio = AK5811_GPIO_PWDN;
	ak5811->trigger_gpio = AK5811_GPIO_TRIGGER;
	ak5811->power1_gpio = AK5811_GPIO_AKM_PWR_EN;
	ak5811->power2_gpio = AK5811_GPIO_AKM_DVDD_3V3_EN_N;
	ak5811->i2cen_gpio = AK5811_GPIO_I2CEN;

#if 0
	ret = gpio_request(ak5811->irq_gpio, "irq-gpio");
	if (ret){
		printk("!!!!!!Erro to request ak5811 irq gpio\n");
		return -EINVAL;
	}
	ret = gpio_direction_input(ak5811->irq_gpio);
	if (ret) {
		printk("!!!!!!Erro to set ak5811 irq gpio direction\n");
		goto err_gpio_irq;
	}
#endif

	ret = gpio_request(ak5811->reset_gpio, "reset-gpio");
	if (ret){
		printk("!!!!!!Erro to request reset gpio\n");
		return -EINVAL;
	}
	ret = gpio_direction_output(ak5811->reset_gpio, 0);
	if (ret) {
		printk("!!!!!!Erro to set reset gpio direction\n");
		goto err_gpio_reset;
	}

	ret = gpio_request(ak5811->pwdn_gpio, "pwdn-gpio");
	if (ret){
		printk("!!!!!Erro to request pwdn gpio\n");
		return -EINVAL;
	}

	ret = gpio_direction_output(ak5811->pwdn_gpio, 0);
	if (ret) {
		printk("!!!!Erro to set pwdn gpio direction\n");
		goto err_gpio_pwdn;
	}

#if 0
	ret = gpio_request(ak5811->trigger_gpio, "trigger-gpio");
	if (ret){
		printk("!!!!!Erro to request trigger gpio\n");
		return -EINVAL;
	}
	ret = gpio_direction_output(ak5811->trigger_gpio, 0);
	if (ret) {
		printk("!!!!Erro to set trigger gpio direction\n");
		goto err_gpio_trigger;
	}
#endif

	ret = gpio_request(ak5811->power1_gpio, "power1-gpio");
	if (ret){
		printk("!!!!!Erro to request power1(AKM_PWR_EN) gpio\n");
		return -EINVAL;
	}
	ret = gpio_direction_output(ak5811->power1_gpio, 1);
	if (ret) {
		printk("!!!!Erro to set power1(AKM_PWR_EN) gpio direction\n");
		goto err_gpio_power1_gpio;
	}

	ret = gpio_request(ak5811->power2_gpio, "power2-gpio");
	if (ret){
		printk("!!!!!Erro to request power2(AKM_DVDD_3V3_EN_N) gpio\n");
		return -EINVAL;
	}
	ret = gpio_direction_output(ak5811->power2_gpio, 0);
	if (ret) {
		printk("!!!!Erro to set power2(AKM_DVDD_3V3_EN_N) gpio direction\n");
		goto err_gpio_power2_gpio;
	}

	ret = gpio_request(ak5811->i2cen_gpio, "i2cen-gpio");
	if (ret){
		printk("!!!!!Erro to request i2cen gpio\n");
		return -EINVAL;
	}
	ret = gpio_direction_output(ak5811->i2cen_gpio, 0);
	if (ret) {
		printk("!!!!Erro to set i2cen gpio direction\n");
		goto err_gpio_i2cen_gpio;
	}

	mutex_init(&ak5811->mutex);

	sd = &ak5811->subdev;
	v4l2_spi_subdev_init(sd, spi, &ak5811_subdev_ops);
	ret = ak5811_initialize_controls(ak5811);
	if (ret)
		goto err_destroy_mutex;

	ret = __ak5811_power_on(ak5811);
	printk("------------------------>>>>>>>>>ret:%d\n",ret);
	if (ret)
		goto err_free_handler;

	ret = ak5811_check_sensor_id(spi);
	if (ret)
		goto err_power_off;

	/* init ak5811, transfer to STBY */
	ret = ak5811_init(ak5811);
	if (ret)
		goto err_power_off;
#ifdef CONFIG_VIDEO_V4L2_SUBDEV_API
	sd->internal_ops = &ak5811_internal_ops;
	sd->flags |= V4L2_SUBDEV_FL_HAS_DEVNODE;
#endif

#if defined(CONFIG_MEDIA_CONTROLLER)
	ak5811->pad.flags = MEDIA_PAD_FL_SOURCE;
	sd->entity.type = MEDIA_ENT_T_V4L2_SUBDEV_SENSOR;
	ret = media_entity_init(&sd->entity, 1, &ak5811->pad, 0);
	if (ret < 0)
		goto err_power_off;
#endif

	snprintf(sd->name, sizeof(sd->name), "m%02d_%s %s",
			ak5811->module_index,
			AK5811_NAME, dev_name(sd->dev));
	ret = v4l2_async_register_subdev(sd);
	if (ret) {
		dev_err(dev, "v4l2 async register subdev failed\n");
		goto err_clean_entity;
	}
	ret = sysfs_create_group(&spi->dev.kobj, &ak5811_trig_attr_group);
	if (ret) {
		dev_err(dev, "device create sysfs group failed\n");
		ret = -EINVAL;
		goto err_clean_entity;
	}

	pm_runtime_set_active(dev);
	pm_runtime_enable(dev);
	pm_runtime_idle(dev);

	return 0;

err_clean_entity:
#if defined(CONFIG_MEDIA_CONTROLLER)
	media_entity_cleanup(&sd->entity);
#endif
err_power_off:
	__ak5811_power_off(ak5811);
err_free_handler:
	v4l2_ctrl_handler_free(&ak5811->ctrl_handler);
err_destroy_mutex:
	mutex_destroy(&ak5811->mutex);
err_gpio_i2cen_gpio:
	gpio_free(ak5811->i2cen_gpio);
err_gpio_power2_gpio:
	gpio_free(ak5811->power2_gpio);
err_gpio_power1_gpio:
	gpio_free(ak5811->power1_gpio);
err_gpio_pwdn:
	gpio_free(ak5811->pwdn_gpio);
err_gpio_reset:
	gpio_free(ak5811->reset_gpio);

	return ret;
}

static int ak5811_remove(struct spi_device *spi)
{
	struct v4l2_subdev *sd = spi_get_drvdata(spi);
	struct ak5811 *ak5811 = to_ak5811(sd);

	v4l2_async_unregister_subdev(sd);
#if defined(CONFIG_MEDIA_CONTROLLER)
	media_entity_cleanup(&sd->entity);
#endif
	v4l2_ctrl_handler_free(&ak5811->ctrl_handler);
	mutex_destroy(&ak5811->mutex);

	pm_runtime_disable(&spi->dev);
	if (!pm_runtime_status_suspended(&spi->dev))
		__ak5811_power_off(ak5811);
	pm_runtime_set_suspended(&spi->dev);

	return 0;
}

static const struct spi_device_id ak5811_match_id[] = {
	{ "ak,ak5811", 0 },
	{ }
};
MODULE_DEVICE_TABLE(spi, ak5811_match_id);

#if IS_ENABLED(CONFIG_OF)
static const struct of_device_id ak5811_of_match[] = {
	{ .compatible = "ak,ak5811" },
	{ },
};
MODULE_DEVICE_TABLE(of, ak5811_of_match);
#endif

static struct spi_driver ak5811_driver = {
	.driver = {
		.name = AK5811_NAME,
		.pm = &ak5811_pm_ops,
		.of_match_table = of_match_ptr(ak5811_of_match),
	},
	.probe		= &ak5811_probe,
	.remove		= &ak5811_remove,
	.id_table	= ak5811_match_id,
};

#if LP_SPI_TEST /* add for test */
int spi_write_slt(int id, const void *txbuf, size_t n)
{
        int ret = -1;
        struct spi_device *spi = NULL;

        if (id >= MAX_SPI_DEV_NUM)
                return -1;
        if (!g_spi_test_data.spi) {
                pr_err("g_spi.%d is NULL\n", id);
                return -1;
        } else {
                spi = g_spi_test_data.spi;
        }

        ret = spi_write(spi, txbuf, n);
        return ret;
}

int spi_read_slt(int id, void *rxbuf, size_t n)
{
        int ret = -1;
        struct spi_device *spi = NULL;

        if (id >= MAX_SPI_DEV_NUM)
                return ret;
        if (!g_spi_test_data.spi) {
                pr_err("g_spi.%d is NULL\n", id);
                return ret;
        } else {
                spi = g_spi_test_data.spi;
        }

        ret = spi_read(spi, rxbuf, n);
        return ret;
}

int spi_write_then_read_slt(int id, const void *txbuf, unsigned n_tx,
                void *rxbuf, unsigned n_rx)
{
        int ret = -1;
        struct spi_device *spi = NULL;

        if (id >= MAX_SPI_DEV_NUM)
                return ret;
        if (!g_spi_test_data.spi) {
                pr_err("g_spi.%d is NULL\n", id);
                return ret;
        } else {
                spi = g_spi_test_data.spi;
        }

        ret = spi_write_then_read(spi, txbuf, n_tx, rxbuf, n_rx);
        return ret;
}

int spi_write_and_read_slt(int id, const void *tx_buf,
                        void *rx_buf, size_t len)
{
        int ret = -1;
        struct spi_device *spi = NULL;
        struct spi_transfer     t = {
                        .tx_buf         = tx_buf,
                        .rx_buf         = rx_buf,
                        .len            = len,
                };
        struct spi_message      m;

        if (id >= MAX_SPI_DEV_NUM)
                return ret;
        if (!g_spi_test_data.spi) {
                pr_err("g_spi.%d is NULL\n", id);
                return ret;
        } else {
                spi = g_spi_test_data.spi;
        }

        spi_message_init(&m);
        spi_message_add_tail(&t, &m);
        return spi_sync(spi, &m);
}


static ssize_t spi_test_write(struct file *file,
                        const char __user *buf, size_t n, loff_t *offset)
{
        int argc = 0, i;
        char tmp[64];
        char *argv[16];
        char *cmd, *data;
        unsigned int id = 0, times = 0, size = 0;
        u8 reg = 0, val = 0;
        unsigned long us = 0, bytes = 0;
        char *txbuf = NULL, *rxbuf = NULL;
        ktime_t start_time;
        ktime_t end_time;
        ktime_t cost_time;

        memset(tmp, 0, sizeof(tmp));
        if (copy_from_user(tmp, buf, n))
                return -EFAULT;
        cmd = tmp;
        data = tmp;

        while (data < (tmp + n)) {
                data = strstr(data, " ");
                if (!data)
                        break;
                *data = 0;
                argv[argc] = ++data;
                argc++;
                if (argc >= 16)
                        break;
        }

        tmp[n - 1] = 0;

        if (!strcmp(cmd, "setspeed")) {
                int id = 0, val;
                struct spi_device *spi = NULL;

                sscanf(argv[0], "%d", &id);
                sscanf(argv[1], "%d", &val);

                if (id >= MAX_SPI_DEV_NUM)
                        return n;
                if (!g_spi_test_data.spi) {
                        pr_err("g_spi.%d is NULL\n", id);
                        return n;
                } else {
                        spi = g_spi_test_data.spi;
                }
                spi->max_speed_hz = val;

        } else if (!strcmp(cmd, "write")) {
                sscanf(argv[0], "%hhu", &reg);
                sscanf(argv[1], "%hhu", &val);
		printk("Write reg 0x%02x with val 0x%02x\n", reg, val);
		ak5811_write_reg(g_spi_test_data.spi, reg, val);
        } else if (!strcmp(cmd, "read")) {
                sscanf(argv[0], "%hhu", &reg);

		ak5811_read_reg(g_spi_test_data.spi, reg, &val);
		printk("Read reg 0x%02x val %02x\n", reg, val);
#if 0
		for ( i = 0; i < times; i++)
			ak5811_check_sensor_id(g_spi_test_data.spi);
#endif
	} else if (!strcmp(cmd, "loop")) {
                sscanf(argv[0], "%d", &id);
                sscanf(argv[1], "%d", &times);
                sscanf(argv[2], "%d", &size);

                txbuf = kzalloc(size, GFP_KERNEL);
                if (!txbuf) {
                        printk("spi tx alloc buf size %d fail\n", size);
                        return n;
                }

                rxbuf = kzalloc(size, GFP_KERNEL);
                if (!rxbuf) {
                        kfree(txbuf);
                        printk("spi rx alloc buf size %d fail\n", size);
                        return n;
                }

                for (i = 0; i < size; i++)
                        txbuf[i] = i % 256;

                start_time = ktime_get();
                for (i = 0; i < times; i++)
                        spi_write_and_read_slt(id, txbuf, rxbuf, size);

                end_time = ktime_get();
                cost_time = ktime_sub(end_time, start_time);
                us = ktime_to_us(cost_time);

                if (memcmp(txbuf, rxbuf, size))
                        printk("spi loop test fail\n");

                bytes = size * times;
                bytes = bytes * 1000 / us;
                printk("spi loop %d*%d cost %ldus speed:%ldKB/S\n", size, times, us, bytes);

                kfree(txbuf);
                kfree(rxbuf);
        } else {
                printk("echo id number size > /dev/spi_misc_test\n");
                printk("echo write 0 10 255 > /dev/spi_misc_test\n");
                printk("echo write 0 10 255 init.rc > /dev/spi_misc_test\n");
                printk("echo read 0 10 255 > /dev/spi_misc_test\n");
                printk("echo loop 0 10 255 > /dev/spi_misc_test\n");
                printk("echo setspeed 0 1000000 > /dev/spi_misc_test\n");
        }

        return n;
}

struct ak5811_reg_info {

	unsigned char page;
	unsigned char reg;
	unsigned char val;
};

/* ak5811 dev ioctl */
static long ak5811_spi_ioctl(struct file *file, unsigned int cmd, unsigned long arg)
{
	struct ak5811_reg_info ak5811_reg;
	int ret = 0;
	u8 val_before,val_after = 0;
	u8 val = 0;

	struct regval patter_enable[] = {
              {AK5811_REG_SETTING_PAGE, AK5811_PAGE2},
              {0x30, 0x2F},
	      {REG_NULL, 0x00},
	};

	struct regval patter_disable[] = {
              {AK5811_REG_SETTING_PAGE, AK5811_PAGE2},
              {0x30, 0x00},
	      {REG_NULL, 0x00},
	};


	switch (cmd){
#if 1
		case AK5811_REG_WRITE:
		{
			//printk("------------------------AK5811_REG_WRITE_WITH_PAGE:%d\n",AK5811_REG_WRITE_WITH_PAGE);
			ret = copy_from_user(&ak5811_reg, (struct ak5811_reg_info *)arg, sizeof(ak5811_reg));
			if (!ret){
				ret = ak5811_write_reg(g_spi_test_data.spi, AK5811_REG_SETTING_PAGE, ak5811_reg.page);
				ret |= ak5811_write_reg(g_spi_test_data.spi, ak5811_reg.reg, ak5811_reg.val);
			}
			else{
				printk("Faile to get infos from user, ret = %d\n", ret);
			}
				break;
		}
#endif
		case AK5811_REG_READ:
		{
			ret = copy_from_user(&ak5811_reg, (struct ak5811_reg_info *)arg, sizeof(ak5811_reg));
			if (!ret){
				ret = ak5811_write_reg(g_spi_test_data.spi, AK5811_REG_SETTING_PAGE, ak5811_reg.page);
				ret |= ak5811_read_reg(g_spi_test_data.spi, ak5811_reg.reg, &val);
				if (!ret) {
					ak5811_reg.val = val;
					ret = copy_to_user((struct ak5811_reg_info *)arg, &ak5811_reg, sizeof(ak5811_reg));
				}else{
					printk("Erro: Get reg 0x%02x val: 0x%02x ret:%d\n", ak5811_reg.reg, val, ret);
				}
			}else{

				printk("Faile to get infos from user, ret = %d\n", ret);
			}
			break;
		}
		case AK5811_PATTERN_OUTPUT:
		{
			ret = copy_from_user(&ak5811_reg, (struct ak5811_reg_info *)arg, sizeof(ak5811_reg));
			if (ak5811_reg.val){/* enable pattern output */
				ak5811_write_array(g_spi_test_data.spi, patter_enable);
			}else{
				ak5811_write_array(g_spi_test_data.spi, patter_disable);
			}
			break;
		}
		default:
		{
			ret = -ENOIOCTLCMD;
			break;
		}
	}
	return ret;
}

static const struct file_operations spi_test_fops = {
        .write = spi_test_write,
	.unlocked_ioctl = ak5811_spi_ioctl,
	.compat_ioctl = ak5811_spi_ioctl,
};

static struct miscdevice spi_test_misc = {
        .minor = MISC_DYNAMIC_MINOR,
        .name = "spi_misc_test",
        .fops = &spi_test_fops,
};
#endif

static int __init ak5811_mod_init(void)
{
#if LP_SPI_TEST
	misc_register(&spi_test_misc);
#endif

	return spi_register_driver(&ak5811_driver);
}

static void __exit ak5811_mod_exit(void)
{

#if LP_SPI_TEST
	misc_deregister(&spi_test_misc);
#endif
	spi_unregister_driver(&ak5811_driver);
}

late_initcall(ak5811_mod_init);
module_exit(ak5811_mod_exit);

MODULE_DESCRIPTION("Ingenic ak5811 radar driver");
MODULE_LICENSE("GPL v2");
MODULE_VERSION("20210426");

