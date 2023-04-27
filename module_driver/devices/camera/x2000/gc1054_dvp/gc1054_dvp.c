/*
 * Copyright (C) 2020 Ingenic Semiconductor Co., Ltd.
 *
 * GC2053 DVP
 *
 */

#include <linux/module.h>
#include <utils/gpio.h>
#include <utils/i2c.h>
#include <common.h>
#include <linux/regulator/consumer.h>
#include <camera/hal/camera_sensor.h>
#include <camera/hal/dvp_gpio_func.h>

#define GC1054_DEVICE_NAME              "gc1054"
#define GC1054_DEVICE_I2C_ADDR          0x21
#define GC1054_CHIP_ID_H                0x10
#define GC1054_CHIP_ID_L                0x54

#define GC1054_REG_END                  0x00
#define GC1054_REG_DELAY                0xff
#define GC1054_PAGE_SELECT              0xfe

#define GC1054_SUPPORT_MCLK             (24 * 1000 * 1000)
#define GC1054_SUPPORT_PCLK             (39 * 1000 * 1000)
#define SENSOR_OUTPUT_MAX_FPS           30
#define SENSOR_OUTPUT_MIN_FPS           5
#define GC1054_MAX_WIDTH                1280
#define GC1054_MAX_HEIGHT               720
#define GC1054_WIDTH                    1080
#define GC1054_HEIGHT                   720


static char *sensor_name                 = NULL;
static int cam_bus_num                   = -1;
static int i2c_bus_num                   = -1;
static unsigned short i2c_addr           = GC1054_DEVICE_I2C_ADDR;
static int power_gpio                    = -1;
static int reset_gpio                    = -1;
static int pwdn_gpio                     = -1;
static char *regulator_name              = "";
static int dvp_gpio_func                 = -1;


module_param(sensor_name, charp, 0644);
module_param(cam_bus_num, int, 0644);
module_param(i2c_bus_num, int, 0644);
module_param(i2c_addr, short, 0644);
module_param_gpio(power_gpio, 0644);
module_param_gpio(reset_gpio, 0644);
module_param_gpio(pwdn_gpio, 0644);
module_param(regulator_name, charp, 0644);
module_param(dvp_gpio_func, int, 0644);


static struct i2c_client *i2c_dev;
static struct sensor_attr gc1054_sensor_attr;
static struct regulator *gc1054_regulator = NULL;

struct regval_list {
    unsigned char reg_num;
    unsigned char value;
};


/*
 * Interface    : DVP - RAW10
 * MCLK         : 24Mhz,
 * PCLK         : 39MHz
 * resolution   : 1280*720
 * FrameRate    : 25fps
 * HTS          : 1726
 * VTS          : 903
 */
static struct regval_list gc1054_init_regs_1280_720_dvp[] = {
    /////////////////////////////////////////////////////
    //////////////////////   SYS   //////////////////////
    /////////////////////////////////////////////////////
	{0xf2,0x00},
	{0xf6,0x00},
	{0xfc,0x04},
	{0xf7,0x01},
	{0xf8,0x0c},
	{0xf9,0x00},
	{0xfa,0x80},
	{0xfc,0x0e},
	///////////////////////////////////
	/// ANALOG & CISCTL   /////////////
	///////////////////////////////////
	{0xfe,0x00},
	{0x03,0x02},
	{0x04,0xa6},
	{0x05,0x02}, //HB
	{0x06,0x07},
	{0x07,0x00}, //VB
	{0x08,0x0a},
	{0x09,0x00},
	{0x0a,0x04}, //row start
	{0x0b,0x00},
	{0x0c,0x00}, //col start
	{0x0d,0x02},
	{0x0e,0xd4}, //height 724
	{0x0f,0x05},
	{0x10,0x08}, //width 1288
	{0x17,0xc0},
	{0x18,0x02},
	{0x19,0x08},
	{0x1a,0x18},
	{0x1d,0x12},
	{0x1e,0x50},
	{0x1f,0x80},
	{0x21,0x30},
	{0x23,0xf8},
	{0x25,0x10},
	{0x28,0x20},
	{0x34,0x08}, //data low
	{0x3c,0x10},
	{0x3d,0x0e},
	{0xcc,0x8e},
	{0xcd,0x9a},
	{0xcf,0x70},
	{0xd0,0xa9},
	{0xd1,0xc5},
	{0xd2,0xed}, //data high
	{0xd8,0x3c}, //dacin offset
	{0xd9,0x7a},
	{0xda,0x12},
	{0xdb,0x50},
	{0xde,0x0c},
	{0xe3,0x60},
	{0xe4,0x78},
	{0xfe,0x01},
	{0xe3,0x01},
	{0xe6,0x10}, //ramps offset
	///////////////////////////////////
	////   ISP   //////////////////////
	///////////////////////////////////
	{0xfe,0x01},
	{0x80,0x50},
	{0x88,0xf3},
	{0x89,0x03},
	{0x90,0x01},
    {0x91,(2+(GC1054_MAX_HEIGHT-GC1054_HEIGHT)/2) >> 8},
    {0x92,(2+(GC1054_MAX_HEIGHT-GC1054_HEIGHT)/2) & 0xff}, //crop win 2<=y<=4
    {0x93,(3+(GC1054_MAX_WIDTH-GC1054_WIDTH)/2) >> 8},
    {0x94,(3+(GC1054_MAX_WIDTH-GC1054_WIDTH)/2) & 0xff}, //crop win 2<=x<=5
    {0x95,GC1054_HEIGHT >> 8}, //crop win height
    {0x96,GC1054_HEIGHT & 0xff},
    {0x97,GC1054_WIDTH >> 8}, //crop win width
    {0x98,GC1054_WIDTH & 0xff},
	///////////////////////////////////
	////   BLK   //////////////////////
	///////////////////////////////////
	{0xfe,0x01},
	{0x40,0x22},
	{0x43,0x03},
	{0x4e,0x3c},
	{0x4f,0x00},
	{0x60,0x00},
	{0x61,0x80},
	///////////////////////////////////
	////   GAIN   /////////////////////
	///////////////////////////////////
	{0xfe,0x01},
	{0xb0,0x48},
	{0xb1,0x01},
	{0xb2,0x00},
	{0xb6,0x00},
	{0xfe,0x02},
	{0x01,0x00},
	{0x02,0x01},
	{0x03,0x02},
	{0x04,0x03},
	{0x05,0x04},
	{0x06,0x05},
	{0x07,0x06},
	{0x08,0x0e},
	{0x09,0x16},
	{0x0a,0x1e},
	{0x0b,0x36},
	{0x0c,0x3e},
	{0x0d,0x56},
	{0xfe,0x02},
	{0xb0,0x00}, //col_gain[11:8]
	{0xb1,0x00},
	{0xb2,0x00},
	{0xb3,0x11},
	{0xb4,0x22},
	{0xb5,0x54},
	{0xb6,0xb8},
	{0xb7,0x60},
	{0xb9,0x00}, //col_gain[12]
	{0xba,0xc0},
	{0xc0,0x20}, //col_gain[7:0]
	{0xc1,0x2d},
	{0xc2,0x40},
	{0xc3,0x5b},
	{0xc4,0x80},
	{0xc5,0xb5},
	{0xc6,0x00},
	{0xc7,0x6a},
	{0xc8,0x00},
	{0xc9,0xd4},
	{0xca,0x00},
	{0xcb,0xa8},
	{0xcc,0x00},
	{0xcd,0x50},
	{0xce,0x00},
	{0xcf,0xa1},
	///////////////////////////////////
	//   DARKSUN   ////////////////////
	///////////////////////////////////
	{0xfe,0x02},
	{0x54,0xf7},
	{0x55,0xf0},
	{0x56,0x00},
	{0x57,0x00},
	{0x58,0x00},
	{0x5a,0x04},
	///////////////////////////////////
	/////   DD   //////////////////////
	///////////////////////////////////
	{0xfe,0x04},
	{0x81,0x8a},
	//////////////////////////////////
	////	 MIPI	/////////////////////
	///////////////////////////////////
	{0xfe,0x03},
	{0x01,0x00},
	{0x02,0x00},
	{0x03,0x00},
	{0x10,0x11},
	{0x15,0x00},
	{0x40,0x01},
	{0x41,0x00},
    {0x42,GC1054_WIDTH & 0xff}, //buf_win_width
    {0x43,GC1054_WIDTH >> 8},
	///////////////////////////////////
	////   pad enable   ///////////////
	///////////////////////////////////
	{0xfe,0x00},
	{0xf2,0x0f},

	{GC1054_REG_END, 0x00},
};

static struct regval_list gc1054_regs_stream_on[] = {
    // {0xfe, 0x00},
    // {0x3e, 0x40},
    {GC1054_REG_END, 0x00},     /* END MARKER */
};

static struct regval_list gc1054_regs_stream_off[] = {
    // {0xfe, 0x00},
    // {0x3e, 0x00},
    {GC1054_REG_END, 0x00},     /* END MARKER */
};

static int gc1054_write(struct i2c_client *i2c, unsigned char reg, unsigned char value)
{
    unsigned char buf[2] = {reg, value};
    struct i2c_msg msg = {
        .addr   = i2c->addr,
        .flags  = 0,
        .len    = 2,
        .buf    = buf,
    };

    int ret = i2c_transfer(i2c->adapter, &msg, 1);
    if (ret < 0) {
        printk(KERN_ERR "gc1054: failed to write reg: %x\n", (int)reg);
    }

    return ret;
}

static int gc1054_read(struct i2c_client *i2c, unsigned char reg, unsigned char *value)
{
    unsigned char buf[1] = {reg & 0xff};
    struct i2c_msg msg[2] = {
        [0] = {
            .addr  = i2c->addr,
            .flags = 0,
            .len   = 1,
            .buf   = buf,
        },
        [1] = {
            .addr  = i2c->addr,
            .flags = I2C_M_RD,
            .len   = 1,
            .buf   = value,
        }
    };

    int ret = i2c_transfer(i2c->adapter, msg, 2);
    if (ret < 0) {
        printk(KERN_ERR "gc1054(%x): failed to read reg: %x\n", i2c->addr, (int)reg);
    }

    return ret;
}

static int gc1054_write_array(struct i2c_client *i2c, struct regval_list *vals)
{
    int ret;

    while (vals->reg_num != GC1054_REG_END) {
        if (vals->reg_num == GC1054_REG_DELAY) {
            m_msleep(vals->value);
        } else {
            ret = gc1054_write(i2c, vals->reg_num, vals->value);
            if (ret < 0)
                return ret;
        }
        // printk(KERN_ERR "vals->reg_num:%x, vals->value:%x\n", vals->reg_num, vals->value);
        vals++;
    }

    return 0;
}

static inline int gc1054_read_array(struct i2c_client *i2c, struct regval_list *vals)
{
    int ret;
    unsigned char val;

    while (vals->reg_num != GC1054_REG_END) {
        if (vals->reg_num == GC1054_REG_DELAY) {
            m_msleep(vals->value);
        } else {
            ret = gc1054_read(i2c, vals->reg_num, &val);
            if (ret < 0)
                return ret;
        }
        printk(KERN_ERR "vals->reg_num:0x%x, vals->value:0x%x\n", vals->reg_num, val);
        vals++;
    }

    return 0;
}

static int gc1054_detect(struct i2c_client *i2c)
{
    unsigned char h = 1;
    unsigned char l = 1;
    int ret;

    ret = gc1054_read(i2c, 0xf0, &h);
    if (ret < 0)
        return ret;
    if (h != GC1054_CHIP_ID_H) {
        printk(KERN_ERR "gc1054 read chip id high failed:0x%x != 0x%x\n", h, GC1054_CHIP_ID_H);
        return -ENODEV;
    }

    ret = gc1054_read(i2c, 0xf1, &l);
    if (ret < 0)
        return ret;
    if (l != GC1054_CHIP_ID_L){
        printk(KERN_ERR "gc1054 read chip id low failed:0x%x != 0x%x\n", l, GC1054_CHIP_ID_L);
        return -ENODEV;
    }
    printk(KERN_DEBUG "gc1054 get chip id = %02x%02x\n", h, l);

    return 0;
}

static int init_gpio(void)
{
    int ret;
    char gpio_str[10];

    switch (dvp_gpio_func) {
        case DVP_PA_LOW_10BIT:
        case DVP_PA_HIGH_10BIT:
            gc1054_sensor_attr.dvp.data_fmt = DVP_RAW10;
            break;
        case DVP_PA_12BIT:
            gc1054_sensor_attr.dvp.data_fmt = DVP_RAW12;
            break;
        case DVP_PA_LOW_8BIT:
        case DVP_PA_HIGH_8BIT:
            gc1054_sensor_attr.dvp.data_fmt = DVP_RAW8;
            break;
        default:
            printk(KERN_ERR "gc1054: unknown dvp gpio mode: %d\n", dvp_gpio_func);
            return -1;
    };
    gc1054_sensor_attr.dvp.gpio_mode = dvp_gpio_func;

    ret = dvp_init_select_gpio(&gc1054_sensor_attr.dvp, gc1054_sensor_attr.dvp.gpio_mode);
    if(ret){
        printk(KERN_ERR "gc1054: failed to init dvp pins\n");
        return ret;
    }

    if (power_gpio != -1) {
        ret = gpio_request(power_gpio, "gc1054_power");
        if (ret) {
            printk(KERN_ERR "gc1054: failed to request power pin: %s\n", gpio_to_str(power_gpio, gpio_str));
            goto err_power_gpio;
        }
    }

    if (strcmp(regulator_name, "-1") && strlen(regulator_name)) {
        gc1054_regulator = regulator_get(NULL, regulator_name);
        if (IS_ERR(gc1054_regulator)) {
            printk(KERN_ERR "regulator_get error!\n");
            ret = -1;
            goto err_regulator;
        }
    }

    if (reset_gpio != -1) {
        ret = gpio_request(reset_gpio, "gc1054_reset");
        if (ret) {
            printk(KERN_ERR "gc1054: failed to request rst pin: %s\n", gpio_to_str(reset_gpio, gpio_str));
            goto err_reset_gpio;
        }
    }

    if (pwdn_gpio != -1) {
        ret = gpio_request(pwdn_gpio, "gc1054_pwdn");
        if (ret) {
            printk(KERN_ERR "gc1054: failed to request pwdn pin: %s\n", gpio_to_str(pwdn_gpio, gpio_str));
            goto err_pwdn_gpio;
        }
    }


    return 0;

err_pwdn_gpio:
    if (reset_gpio != -1)
        gpio_free(reset_gpio);
err_reset_gpio:
    if (gc1054_regulator)
        regulator_put(gc1054_regulator);
err_regulator:
	if (power_gpio != -1)
        gpio_free(power_gpio);
err_power_gpio:
    dvp_deinit_gpio();
    return ret;
}

static void deinit_gpio(void)
{
    dvp_deinit_gpio();

    if (reset_gpio != -1)
        gpio_free(reset_gpio);

    if (pwdn_gpio != -1)
        gpio_free(pwdn_gpio);

    if (gc1054_regulator)
        regulator_put(gc1054_regulator);

    if (power_gpio != -1)
        gpio_free(power_gpio);
}

static void gc1054_power_off(void)
{
    if (reset_gpio != -1)
        gpio_direction_output(reset_gpio, 0);

    if (pwdn_gpio != -1)
        gpio_direction_output(pwdn_gpio, 1);

    if (power_gpio != -1)
        gpio_direction_output(power_gpio, 0);

    if (gc1054_regulator)
        regulator_disable(gc1054_regulator);

    camera_disable_sensor_mclk(cam_bus_num);
}

static int gc1054_power_on(void)
{
    int ret;

    camera_enable_sensor_mclk(cam_bus_num, 24 * 1000 * 1000);

    if (gc1054_regulator) {
        regulator_enable(gc1054_regulator);
        m_msleep(10);
    }

    if (power_gpio != -1) {
        gpio_direction_output(power_gpio, 1);
        m_msleep(50);
    }

    if (pwdn_gpio != -1){
        gpio_direction_output(pwdn_gpio, 1);
        m_msleep(10);
        gpio_direction_output(pwdn_gpio, 0);
        m_msleep(10);
    }

    if (reset_gpio != -1){
        gpio_direction_output(reset_gpio, 1);
        m_msleep(20);
        gpio_direction_output(reset_gpio, 0);
        m_msleep(20);
        gpio_direction_output(reset_gpio, 1);
        m_msleep(10);
    }

    ret = gc1054_detect(i2c_dev);
    if (ret) {
        printk(KERN_ERR "gc1054: failed to detect\n");
        goto err;
    }

    ret = gc1054_write_array(i2c_dev, gc1054_sensor_attr.sensor_info.private_init_setting);
    // ret += gc1054_read_array(i2c_dev, gc1054_sensor_attr.sensor_info.private_init_setting);
    if (ret) {
        printk(KERN_ERR "gc1054: failed to init regs\n");
        goto err;
    }

    return 0;

err:
    gc1054_power_off();
    deinit_gpio();

    return ret;
}

static int gc1054_stream_on(void)
{
    int ret = gc1054_write_array(i2c_dev, gc1054_regs_stream_on);
    if (ret)
        printk(KERN_ERR "gc1054: failed to stream on\n");

    return ret;
}

static void gc1054_stream_off(void)
{
    int ret = gc1054_write_array(i2c_dev, gc1054_regs_stream_off);
    if (ret)
        printk(KERN_ERR "gc1054: failed to stream on\n");
}

static int gc1054_g_register(struct sensor_dbg_register *reg)
{
    unsigned char val;
    int ret;

    ret = gc1054_read(i2c_dev, reg->reg & 0xff, &val);
    reg->val = val;
    reg->size = 1;
    return ret;
}

static int gc1054_s_register(struct sensor_dbg_register *reg)
{
    return gc1054_write(i2c_dev, reg->reg & 0xff, reg->val & 0xff);
}

static int gc1054_set_integration_time(int value)
{
    int ret = 0;

    ret = gc1054_write(i2c_dev, 0xfe, 0x00);
	ret += gc1054_write(i2c_dev, 0x4, value & 0xff);
	ret += gc1054_write(i2c_dev, 0x3, (value & 0x1f00)>>8);
    if (ret < 0) {
        printk(KERN_ERR "gc1054: failed to set integration time\n");
        return ret;
    }

    return 0;
}

#define TX_ISP_GAIN_FIXED_POINT 16
const unsigned int  ANALOG_GAIN_1 =		(1<<TX_ISP_GAIN_FIXED_POINT)|(unsigned int)((0.0*(1<<TX_ISP_GAIN_FIXED_POINT)));
const unsigned int  ANALOG_GAIN_2 =		(1<<TX_ISP_GAIN_FIXED_POINT)|(unsigned int)((0.42*(1<<TX_ISP_GAIN_FIXED_POINT)));
const unsigned int  ANALOG_GAIN_3 =		(1<<TX_ISP_GAIN_FIXED_POINT)|(unsigned int)((0.99*(1<<TX_ISP_GAIN_FIXED_POINT)));
const unsigned int  ANALOG_GAIN_4 =		(2<<TX_ISP_GAIN_FIXED_POINT)|(unsigned int)((0.85*(1<<TX_ISP_GAIN_FIXED_POINT)));
const unsigned int  ANALOG_GAIN_5 =		(4<<TX_ISP_GAIN_FIXED_POINT)|(unsigned int)((0.03*(1<<TX_ISP_GAIN_FIXED_POINT)));
const unsigned int  ANALOG_GAIN_6 =		(5<<TX_ISP_GAIN_FIXED_POINT)|(unsigned int)((0.77*(1<<TX_ISP_GAIN_FIXED_POINT)));
const unsigned int  ANALOG_GAIN_7 =		(8<<TX_ISP_GAIN_FIXED_POINT)|(unsigned int)((0.06*(1<<TX_ISP_GAIN_FIXED_POINT)));
const unsigned int  ANALOG_GAIN_8 =		(11<<TX_ISP_GAIN_FIXED_POINT)|(unsigned int)((0.53*(1<<TX_ISP_GAIN_FIXED_POINT)));
const unsigned int  ANALOG_GAIN_9 =		(16<<TX_ISP_GAIN_FIXED_POINT)|(unsigned int)((0.12*(1<<TX_ISP_GAIN_FIXED_POINT)));

static unsigned int fix_point_mult2(unsigned int a, unsigned int b)
{
	unsigned int x1,x2,x;
	unsigned int a1,a2,b1,b2;
	unsigned int mask = (((unsigned int)0xffffffff)>>(32-TX_ISP_GAIN_FIXED_POINT));
	a1 = a>>TX_ISP_GAIN_FIXED_POINT;
	a2 = a&mask;
	b1 = b>>TX_ISP_GAIN_FIXED_POINT;
	b2 = b&mask;

	x1 = a1*b1;
	x1 += (a1*b2)>>TX_ISP_GAIN_FIXED_POINT;
	x1 += (a2*b1)>>TX_ISP_GAIN_FIXED_POINT;

	x2 = (a1*b2)&mask;
	x2 += (a2*b1)&mask;
	x2 += (a2*b2)>>TX_ISP_GAIN_FIXED_POINT;

	x = (x1<<TX_ISP_GAIN_FIXED_POINT)+x2;

	return x;
        }

static unsigned int gc1054_gainone_to_reg(unsigned int gain_one, unsigned int *regs)
{
	unsigned int gain_one1 = 0;
	unsigned int gain_tmp = 0;
	unsigned char regb6 = 0;
	unsigned char regb1 =0x1;
	unsigned char regb2 = 0;
	int i,j;
	unsigned int gain_one_max = fix_point_mult2(ANALOG_GAIN_9, (0xf<<TX_ISP_GAIN_FIXED_POINT) + (0x3f<<(TX_ISP_GAIN_FIXED_POINT-6)));

	if (gain_one < ANALOG_GAIN_1) {
		gain_one1 = ANALOG_GAIN_1;
		regb6 = 0x00;
		regb1 = 0x01;
		regb2 = 0x00;
		goto done;
	} else if (gain_one < (ANALOG_GAIN_2)) {
		gain_one1 = gain_tmp = ANALOG_GAIN_1;
		regb1 = 0;
		regb2 = 0;
		for (i = 1; i <= 0xf; i++ )
			for (j = 0; j <= 0x3f; j++) {
				gain_tmp = fix_point_mult2(ANALOG_GAIN_1, (i<<TX_ISP_GAIN_FIXED_POINT)+(j<<(TX_ISP_GAIN_FIXED_POINT-6)));
				if (gain_one < gain_tmp) {
					goto done;
				}
				gain_one1 = gain_tmp;
				regb6 = 0x00;
				regb1 = i;
				regb2 = j;
			}
	} else if (gain_one < ANALOG_GAIN_3) {
		gain_one1 = gain_tmp = ANALOG_GAIN_2;
		regb1 = 0;
		regb2 = 0;
		for (i = 1; i <= 0xf; i++ )
			for (j = 0; j <= 0x3f; j++) {
				gain_tmp = fix_point_mult2(ANALOG_GAIN_2, (i<<TX_ISP_GAIN_FIXED_POINT)+(j<<(TX_ISP_GAIN_FIXED_POINT-6)));
				if (gain_one < gain_tmp) {
					goto done;
				}
				gain_one1 = gain_tmp;
				regb6 = 0x01;
				regb1 = i;
				regb2 = j;
			}
	} else if (gain_one < ANALOG_GAIN_4) {
		gain_one1 = gain_tmp = ANALOG_GAIN_3;
		regb1 = 0;
		regb2 = 0;
		for (i = 1; i <= 0xf; i++ )
			for (j = 0; j <= 0x3f; j++) {
				gain_tmp = fix_point_mult2(ANALOG_GAIN_3, (i<<TX_ISP_GAIN_FIXED_POINT)+(j<<(TX_ISP_GAIN_FIXED_POINT-6)));
				if (gain_one < gain_tmp) {
					goto done;
				}
				gain_one1 = gain_tmp;
				regb6 = 0x02;
				regb1 = i;
				regb2 = j;
			}
	} else if (gain_one < ANALOG_GAIN_5) {
		gain_one1 = gain_tmp = ANALOG_GAIN_4;
		regb1 = 0;
		regb2 = 0;
		for (i = 1; i <= 0xf; i++ )
			for (j = 0; j <= 0x3f; j++) {
				gain_tmp = fix_point_mult2(ANALOG_GAIN_4, (i<<TX_ISP_GAIN_FIXED_POINT)+(j<<(TX_ISP_GAIN_FIXED_POINT-6)));
				if (gain_one < gain_tmp) {
					goto done;
				}
				gain_one1 = gain_tmp;
				regb6 = 0x03;
				regb1 = i;
				regb2 = j;
			}
	} else if (gain_one < ANALOG_GAIN_6) {
		gain_one1 = gain_tmp = ANALOG_GAIN_5;
		regb1 = 0;
		regb2 = 0;
		for (i = 1; i <= 0xf; i++ )
			for (j = 0; j <= 0x3f; j++) {
				gain_tmp = fix_point_mult2(ANALOG_GAIN_5, (i<<TX_ISP_GAIN_FIXED_POINT)+(j<<(TX_ISP_GAIN_FIXED_POINT-6)));
				if (gain_one < gain_tmp) {
					goto done;
				}
				gain_one1 = gain_tmp;
				regb6 = 0x04;
				regb1 = i;
				regb2 = j;
			}
	} else if (gain_one < ANALOG_GAIN_7) {
		gain_one1 = gain_tmp = ANALOG_GAIN_6;
		regb1 = 0;
		regb2 = 0;
		for (i = 1; i <= 0xf; i++ )
			for (j = 0; j <= 0x3f; j++) {
				gain_tmp = fix_point_mult2(ANALOG_GAIN_6, (i<<TX_ISP_GAIN_FIXED_POINT)+(j<<(TX_ISP_GAIN_FIXED_POINT-6)));
				if (gain_one < gain_tmp) {
					goto done;
				}
				gain_one1 = gain_tmp;
				regb6 = 0x05;
				regb1 = i;
				regb2 = j;
			}
	} else if (gain_one < ANALOG_GAIN_8) {
		gain_one1 = gain_tmp = ANALOG_GAIN_7;
		regb1 = 0;
		regb2 = 0;
		for (i = 1; i <= 0xf; i++ )
			for (j = 0; j <= 0x3f; j++) {
				gain_tmp = fix_point_mult2(ANALOG_GAIN_7, (i<<TX_ISP_GAIN_FIXED_POINT)+(j<<(TX_ISP_GAIN_FIXED_POINT-6)));
				if (gain_one < gain_tmp) {
					goto done;
				}
				gain_one1 = gain_tmp;
				regb6 = 0x06;
				regb1 = i;
				regb2 = j;
			}
	} else if (gain_one < ANALOG_GAIN_9) {
		gain_one1 = gain_tmp = ANALOG_GAIN_8;
		regb1 = 0;
		regb2 = 0;
		for (i = 1; i <= 0xf; i++ )
			for (j = 0; j <= 0x3f; j++) {
				gain_tmp = fix_point_mult2(ANALOG_GAIN_8, (i<<TX_ISP_GAIN_FIXED_POINT)+(j<<(TX_ISP_GAIN_FIXED_POINT-6)));
				if (gain_one < gain_tmp) {
					goto done;
				}
				gain_one1 = gain_tmp;
				regb6 = 0x07;
				regb1 = i;
				regb2 = j;
			}
	} else if (gain_one < gain_one_max) {
		gain_one1 = gain_tmp = ANALOG_GAIN_9;
		regb1 = 0;
		regb2 = 0;
		for (i = 1; i <= 0xf; i++ )
			for (j = 0; j <= 0x3f; j++) {
				gain_tmp = fix_point_mult2(ANALOG_GAIN_9, (i<<TX_ISP_GAIN_FIXED_POINT)+(j<<(TX_ISP_GAIN_FIXED_POINT-6)));
				if (gain_one < gain_tmp) {
					goto done;
				}
				gain_one1 = gain_tmp;
				regb6 = 0x08;
				regb1 = i;
				regb2 = j;
			}
	} else {
		gain_one1 = gain_one_max;
		regb6 = 0x08;
		regb1 = 0xf;
		regb2 = 0x3f;
		goto done;
	}
	gain_one1 = ANALOG_GAIN_1;

done:
	*regs = (regb6<<12)|(regb1<<8)|(regb2);

	return gain_one1;
}

extern uint32_t tisp_math_exp2(uint32_t val, const unsigned char shift_in, const unsigned char shift_out);
extern uint32_t tisp_log2_fixed_to_fixed(const uint32_t val, const int in_fix_point, const uint8_t out_fix_point);
static unsigned int gc1054_alloc_again(unsigned int isp_gain, unsigned char shift, unsigned int *sensor_again)
{
	unsigned int gain_one = 0;
	unsigned int gain_one1 = 0;
	unsigned int regs = 0;
	unsigned int isp_gain1 = 0;

	gain_one = tisp_math_exp2(isp_gain, shift, TX_ISP_GAIN_FIXED_POINT);
	gain_one1 = gc1054_gainone_to_reg(gain_one, &regs);
	isp_gain1 = tisp_log2_fixed_to_fixed(gain_one1, TX_ISP_GAIN_FIXED_POINT, shift);
	*sensor_again = regs;

	return isp_gain1;
}

static int gc1054_set_analog_gain(int value)
{
    int ret = 0;

	ret = gc1054_write(i2c_dev, 0xfe, 0x01);
	ret += gc1054_write(i2c_dev, 0xb6, (value >> 12) & 0xf);
	ret += gc1054_write(i2c_dev, 0xb1, (value >> 8) & 0xf);
	ret += gc1054_write(i2c_dev, 0xb2, (value << 2) & 0xff);
    if (ret < 0) {
        printk(KERN_ERR "gc1054: failed to set analog gain\n");
        return ret;
    }
    return 0;
}

static unsigned int gc1054_alloc_dgain(unsigned int isp_gain, unsigned char shift, unsigned int *sensor_dgain)
{
    return isp_gain;
}

static int gc1054_set_digital_gain(int value)
{
    return 0;
}

static int gc1054_set_fps(int fps)
{
    struct sensor_info *sensor_info = &gc1054_sensor_attr.sensor_info;
    unsigned int pclk = GC1054_SUPPORT_PCLK;
	unsigned short win_width=0;
	unsigned short win_high=0;
	unsigned short vts = 0;
	unsigned short hb=0;
	unsigned short sh_delay=0;
	unsigned short vb = 0;
	unsigned short hts=0;
    unsigned char tmp;
    unsigned int newformat = 0;
    int ret = 0;

    newformat = (((fps >> 16) / (fps & 0xffff)) << 8) + ((((fps >> 16) % (fps & 0xffff)) << 8) / (fps & 0xffff));
    if (newformat > (SENSOR_OUTPUT_MAX_FPS << 8) || newformat <(SENSOR_OUTPUT_MIN_FPS) << 8)
        return -1;

    ret = gc1054_write(i2c_dev, 0xfe, 0x00);

    ret = gc1054_read(i2c_dev, 0x5, &tmp);
	hb = tmp;
	ret += gc1054_read(i2c_dev, 0x6, &tmp);
	hb = (hb << 8) + tmp;
	ret += gc1054_read(i2c_dev, 0xf, &tmp);
	win_width = tmp;
	ret += gc1054_read(i2c_dev, 0x10, &tmp);
	win_width = (win_width << 8) + tmp;
	ret += gc1054_read(i2c_dev, 0x2c, &tmp);
	if(ret < 0)
		return -1;
	sh_delay = tmp;
    hts=(hb+16+(win_width+sh_delay)/4)*2;
	ret = gc1054_read(i2c_dev, 0xd, &tmp);
	win_high = tmp;
	ret += gc1054_read(i2c_dev, 0xe, &tmp);
	if(ret < 0)
		return -1;
	win_high = (win_high << 8) + tmp;
    vts = pclk * (fps & 0xffff) / hts / (fps >> 16);
	vb = vts - win_high - 16;
	ret = gc1054_write(i2c_dev, 0x8, (unsigned char)(vb & 0xff));
	ret += gc1054_write(i2c_dev, 0x7, (unsigned char)(vb >> 8));
	if(ret < 0)
		return -1;

    sensor_info->fps = fps;
    sensor_info->total_height = vts;
    sensor_info->max_integration_time = vts - 4;

    return 0;
}

static struct sensor_attr gc1054_sensor_attr = {
    .device_name        = GC1054_DEVICE_NAME,
    .cbus_addr          = GC1054_DEVICE_I2C_ADDR,

    .dbus_type          = SENSOR_DATA_BUS_DVP,
    .dvp = {
        .data_fmt           = DVP_RAW8,
        .gpio_mode          = DVP_PA_LOW_8BIT,
        .timing_mode        = DVP_HREF_MODE,
        .yuv_data_order = 0,
        .hsync_polarity = POLARITY_HIGH_ACTIVE,
        .vsync_polarity = POLARITY_HIGH_ACTIVE,
        .img_scan_mode  = DVP_IMG_SCAN_PROGRESS,
    },

    .isp_clk_rate       = 150 * 1000 * 1000,

    .sensor_info = {
        .private_init_setting   = gc1054_init_regs_1280_720_dvp,
        .width                  = GC1054_WIDTH,
        .height                 = GC1054_HEIGHT,
        .fmt                    = SENSOR_PIXEL_FMT_SRGGB10_1X10,

        .fps                    = 25 << 16 | 1,
        .total_width            = 1726,
        .total_height           = 903,

        .min_integration_time   = 4,
        .max_integration_time   = 903 - 4,
        .max_again              = 262850,
        .max_dgain              = 0,
    },

    .ops = {
        .power_on               = gc1054_power_on,
        .power_off              = gc1054_power_off,
        .stream_on              = gc1054_stream_on,
        .stream_off             = gc1054_stream_off,
        .get_register           = gc1054_g_register,
        .set_register           = gc1054_s_register,

        .set_integration_time   = gc1054_set_integration_time,
        .alloc_again            = gc1054_alloc_again,
        .set_analog_gain        = gc1054_set_analog_gain,
        .alloc_dgain            = gc1054_alloc_dgain,
        .set_digital_gain       = gc1054_set_digital_gain,
        .set_fps                = gc1054_set_fps,
    },
};

static int gc1054_probe(struct i2c_client *client,
        const struct i2c_device_id *id)
{
    int ret = init_gpio();
    if (ret)
        return ret;

    ret = dvp_init_select_gpio(&gc1054_sensor_attr.dvp, gc1054_sensor_attr.dvp.gpio_mode);
    if (ret) {
        deinit_gpio();
        return ret;
    }

    ret = camera_register_sensor(cam_bus_num, &gc1054_sensor_attr);
    if (ret) {
        dvp_deinit_gpio();
        deinit_gpio();
        return ret;
    }

    return 0;
}

static int gc1054_remove(struct i2c_client *client)
{
    camera_unregister_sensor(cam_bus_num, &gc1054_sensor_attr);
    dvp_deinit_gpio();
    deinit_gpio();

    return 0;
}

static struct i2c_device_id gc1054_id[] = {
    { GC1054_DEVICE_NAME, 0 },
    { }
};
MODULE_DEVICE_TABLE(i2c, gc1054_id);

static struct i2c_driver gc1054_driver = {
    .driver = {
        .owner          = THIS_MODULE,
        .name           = GC1054_DEVICE_NAME,
    },
    .probe              = gc1054_probe,
    .remove             = gc1054_remove,
    .id_table           = gc1054_id,
};

static struct i2c_board_info sensor_gc1054_info = {
    .type               = GC1054_DEVICE_NAME,
    .addr               = GC1054_DEVICE_I2C_ADDR,
};

static __init int init_gc1054(void)
{
    if (i2c_bus_num < 0) {
        printk(KERN_ERR "gc1054: i2c_bus_num must be set\n");
        return -EINVAL;
    }
    gc1054_driver.driver.name = sensor_name;
    strcpy(gc1054_id[0].name, sensor_name);
    if (i2c_addr != -1)
        sensor_gc1054_info.addr = i2c_addr;
    strcpy(sensor_gc1054_info.type, sensor_name);
    gc1054_sensor_attr.device_name = sensor_name;

    int ret = i2c_add_driver(&gc1054_driver);
    if (ret) {
        printk(KERN_ERR "gc1054: failed to register i2c driver\n");
        return ret;
    }

    i2c_dev = i2c_register_device(&sensor_gc1054_info, i2c_bus_num);
    if (i2c_dev == NULL) {
        printk(KERN_ERR "gc1054: failed to register i2c device\n");
        i2c_del_driver(&gc1054_driver);
        return -EINVAL;
    }

    return 0;
}

static __exit void exit_gc1054(void)
{
    i2c_unregister_device(i2c_dev);
    i2c_del_driver(&gc1054_driver);
}

module_init(init_gc1054);
module_exit(exit_gc1054);

MODULE_DESCRIPTION("x2000 gc1054 driver");
MODULE_LICENSE("GPL");
