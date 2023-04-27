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

#define GC2053_DEVICE_NAME              "gc2053"
#define GC2053_DEVICE_I2C_ADDR          0x37

#define GC2053_CHIP_ID_H                (0x20)
#define GC2053_CHIP_ID_L                (0x53)
#define GC2053_REG_CHIP_ID_HIGH         0xF0
#define GC2053_REG_CHIP_ID_LOW          0xF1

#define GC2053_REG_END                  0xff
#define GC2053_PAGE_REG                 0xfe
#define GC2053_REG_DELAY                0x0

#define GC2053_SUPPORT_MCLK             (24 * 1000 * 1000)
#define GC2033_SUPPORT_WPCLK_FPS_30     (74.25 *1000 *1000)
#define SENSOR_OUTPUT_MAX_FPS           30
#define SENSOR_OUTPUT_MIN_FPS           5
#define GC2053_WIDTH                    1920
#define GC2053_HEIGHT                   1080
#define GC2053_USE_AGAIN_ONLY

static int power_gpio                    = -1;
static int reset_gpio                    = -1;
static int pwdn_gpio                     = -1;
static int i2c_bus_num                   = -1;
static short i2c_addr                    = -1;
static int cam_bus_num                   = -1;
static char *sensor_name                 = NULL;
static char *regulator_name              = "";

module_param_gpio(power_gpio, 0644);
module_param_gpio(reset_gpio, 0644);
module_param_gpio(pwdn_gpio, 0644);

module_param(i2c_bus_num, int, 0644);
module_param(i2c_addr, short, 0644);
module_param(cam_bus_num, int, 0644);
module_param(sensor_name, charp, 0644);
module_param(regulator_name, charp, 0644);

static struct i2c_client *i2c_dev;
static struct sensor_attr gc2053_sensor_attr;
static struct regulator *gc2053_regulator = NULL;

struct regval_list {
    unsigned short reg_num;
    unsigned char value;
};

struct again_lut {
    unsigned int index;
    unsigned char regb4;
    unsigned char regb3;
    unsigned char regb8;
    unsigned char regb9;
    unsigned int gain;
};

static struct again_lut gc2053_again_lut[] = {
    {0, 0x00, 0x00,0x01,0x00, 0},
    {1, 0x00, 0x10,0x01,0x0c, 13726},
    {2, 0x00, 0x20,0x01,0x1b, 31176},
    {3, 0x00, 0x30,0x01,0x2c, 44067},
    {4, 0x00, 0x40,0x01,0x3f, 64793},
    {5, 0x00, 0x50,0x02,0x16, 78620},
    {6, 0x00, 0x60,0x02,0x35, 96179},
    {7, 0x00, 0x70,0x03,0x16, 109137},
    {8, 0x00, 0x80,0x04,0x02, 132535},
    {9, 0x00, 0x90,0x04,0x31, 146065},
    {10, 0x00, 0xa0,0x05,0x32, 163565},
    {11, 0x00, 0xb0,0x06,0x35, 176745},
    {12, 0x00, 0xc0,0x08,0x04, 195116},
    {13, 0x00, 0x5a,0x09,0x19, 208558},
    {14, 0x00, 0x83,0x0b,0x0f, 229100},
    {15, 0x00, 0x93,0x0d,0x12, 242508},
    {16, 0x00, 0x84,0x10,0x00, 262416},
    {17, 0x00, 0x94,0x12,0x3a, 275706},
    {18, 0x01, 0x2c,0x1a,0x02, 292248},
    {19, 0x01, 0x3c,0x1b,0x20, 305567},
    {20, 0x00, 0x8c,0x20,0x0f, 324958},
    {21, 0x00, 0x9c,0x26,0x07, 338276},
    {22, 0x02, 0x64,0x36,0x21, 358918},
    {23, 0x02, 0x74,0x37,0x3a, 372262},
    {24, 0x00, 0xc6,0x3d,0x02, 392095},
    {25, 0x00, 0xdc,0x3f,0x3f, 415409},
    {26, 0x02, 0x85,0x3f,0x3f, 421076},
    {27, 0x02, 0x95,0x3f,0x3f, 440355},
    {28, 0x00, 0xce,0x3f,0x3f, 444858},
};

/*
 * Interface    : DVP - RAW10
 * MCLK         : 24Mhz,
 * PCLK         : 75MHz
 * resolution   : 1920*1080
 * FrameRate    : 30fps
 * HTS          : 2200
 * VTS          : 1125
 * row_time     : 29.629us
 */
static struct regval_list gc2053_init_regs_1920_1080_30fps_dvp[] = {
    /*system*/
    {0xfe, 0x80},
    {0xfe, 0x80},
    {0xfe, 0x80},
    {0xfe, 0x00},
    {0xf2, 0x00}, //[1]I2C_open_ena [0]pwd_dn
    {0xf3, 0x0f}, //00[3]Sdata_pad_io [2:0]Ssync_pad_io
    {0xf4, 0x36}, //[6:4]pll_ldo_set
    {0xf5, 0xc0}, //[7]soc_mclk_enable [6]pll_ldo_en [5:4]cp_clk_sel [3:0]cp_clk_div
    {0xf6, 0x44}, //[7:3]wpllclk_div [2:0]refmp_div
    {0xf7, 0x01}, //[7]refdiv2d5_en [6]refdiv1d5_en [5:4]scaler_mode [3]refmp_enb [1]div2en [0]pllmp_en
    {0xf8, 0x2c}, //38////38//[7:0]pllmp_div
    {0xf9, 0x42}, //[7:3]rpllclk_div [2:1]pllmp_prediv [0]analog_pwc
    {0xfc, 0x8e},

     /*cisctl&analog*/
    {0xfe, 0x00},
    {0x87, 0x18}, //[6]aec_delay_mode
    {0xee, 0x30}, //[5:4]dwen_sramen
    {0xd0, 0xb7}, //ramp_en
    {0x03, 0x04},
    {0x04, 0x60},
    {0x05, 0x04}, //05
    {0x06, 0x4c}, //60 //[11:0]hb
    {0x41, 0x05},
    {0x42, 0x46}, /*vts for 25fps dvp*/
    {0x07, 0x00},
    {0x08, 0x11}, //19
    {0x09, 0x00},
    {0x0a, 0x02}, //cisctl row start
    {0x0b, 0x00},
    {0x0c, 0x02}, //cisctl col start
    {0x0d, 0x04},
    {0x0e, 0x40},
    {0x12, 0xe2}, //vsync_ahead_mode
    {0x13, 0x16},
    {0x19, 0x0a}, //ad_pipe_num
    {0x21, 0x1c}, //eqc1fc_eqc2fc_sw
    {0x28, 0x0a}, //16//eqc2_c2clpen_sw
    {0x29, 0x24}, //eq_post_width
    {0x2b, 0x04}, //c2clpen --eqc2
    {0x32, 0xf8}, //[5]txh_en ->avdd28
    {0x37, 0x03}, //[3:2]eqc2sel=0
    {0x39, 0x15}, //17 //[3:0]rsgl
    {0x43, 0x07}, //vclamp
    {0x44, 0x40}, //0e//post_tx_width
    {0x46, 0x0b},
    {0x4b, 0x20}, //rst_tx_width
    {0x4e, 0x08}, //12//ramp_t1_width
    {0x55, 0x20}, //read_tx_width_pp
    {0x66, 0x05}, //18//stspd_width_r1
    {0x67, 0x05}, //40//5//stspd_width_r
    {0x77, 0x01}, //dacin offset x31
    {0x78, 0x00}, //dacin offset
    {0x7c, 0x93}, //[1:0] co1comp
    {0x8c, 0x12}, //12 ramp_t1_ref
    {0x8d, 0x92}, //90
    {0x90, 0x01},
    {0x9d, 0x10},
    {0xce, 0x7c}, //70//78//[4:2]c1isel
    {0xd2, 0x41}, //[5:3]c2clamp
    {0xd3, 0xdc}, //ec//0x39[7]=0, 0xd3[3]=1 rsgh=vref
    {0xe6, 0x50}, //ramps offset
     /*gain*/
    {0xb6, 0xc0},
    {0xb0, 0x70},
    {0xb1, 0x01},
    {0xb2, 0x00},
    {0xb3, 0x00},
    {0xb4, 0x00},
    {0xb8, 0x01},
    {0xb9, 0x00},
     /*blk*/
    {0x26, 0x30},
    {0xfe, 0x01},
    {0x40, 0x23},
    {0x55, 0x07},
    {0x60, 0x40}, //[7:0]WB_offset
    {0xfe, 0x04},
    {0x14, 0x78}, //g1 ratio
    {0x15, 0x78}, //r ratio
    {0x16, 0x78}, //b ratio
    {0x17, 0x78}, //g2 ratio
     /*window*/
    {0xfe, 0x01},
    {0x92, 0x00}, //win y1
    {0x94, 0x03}, //win x1
    {0x95, 0x04},
    {0x96, 0x38}, //[10:0]out_height
    {0x97, 0x07},
    {0x98, 0x80}, //[11:0]out_width
     /*ISP*/
    {0xfe, 0x01},
    {0x01, 0x05}, //03//[3]dpc blending mode [2]noise_mode [1:0]center_choose 2b'11:median 2b'10:avg 2'b00:near
    {0x02, 0x89}, //[7:0]BFF_sram_mode
    {0x04, 0x01}, //[0]DD_en
    {0x07, 0xa6},
    {0x08, 0xa9},
    {0x09, 0xa8},
    {0x0a, 0xa7},
    {0x0b, 0xff},
    {0x0c, 0xff},
    {0x0f, 0x00},
    {0x50, 0x1c},
    {0x89, 0x03},
    {0xfe, 0x04},
    {0x28, 0x86},
    {0x29, 0x86},
    {0x2a, 0x86},
    {0x2b, 0x68},
    {0x2c, 0x68},
    {0x2d, 0x68},
    {0x2e, 0x68},
    {0x2f, 0x68},
    {0x30, 0x4f},
    {0x31, 0x68},
    {0x32, 0x67},
    {0x33, 0x66},
    {0x34, 0x66},
    {0x35, 0x66},
    {0x36, 0x66},
    {0x37, 0x66},
    {0x38, 0x62},
    {0x39, 0x62},
    {0x3a, 0x62},
    {0x3b, 0x62},
    {0x3c, 0x62},
    {0x3d, 0x62},
    {0x3e, 0x62},
    {0x3f, 0x62},
     /*DVP & MIPI*/
    {0xfe, 0x01},
    {0x9a, 0x06}, //[5]OUT_gate_mode [4]hsync_delay_half_pclk [3]data_delay_half_pclk [2]vsync_polarity [1]hsync_polarity [0]pclk_out_polarity
    {0xfe, 0x00},
    {0x7b, 0x2a}, //[7:6]updn [5:4]drv_high_data [3:2]drv_low_data [1:0]drv_pclk
    {0x23, 0x2d}, //[3]rst_rc [2:1]drv_sync [0]pwd_rc
    {0xfe, 0x03},
    {0x01, 0x20}, //27[6:5]clkctr [2]phy-lane1_en [1]phy-lane0_en [0]phy_clk_en
    {0x02, 0x56}, //[7:6]data1ctr [5:4]data0ctr [3:0]mipi_diff
    {0x03, 0xb2}, //b6[7]clklane_p2s_sel [6:5]data0hs_ph [4]data0_delay1s [3]clkdelay1s [2]mipi_en [1:0]clkhs_ph
    {0x12, 0x80},
    {0x13, 0x07}, //LWC

    {0xfe, 0x00},
    {0x3e, 0x00}, //91[7]lane_ena [6]DVPBUF_ena [5]ULPEna [4]MIPI_ena [3]mipi_set_auto_disable [2]RAW8_mode [1]ine_sync_mode [0]double_lane_en
    {GC2053_REG_DELAY, 100},    /* END MARKER */
    {GC2053_REG_END, 0x00},    /* END MARKER */
};

static struct regval_list gc2053_regs_stream_on[] = {

    {0xfe, 0x00},
    {0x3e, 0x40},
    {GC2053_REG_END, 0x00},     /* END MARKER */
};

static struct regval_list gc2053_regs_stream_off[] = {

    {0xfe, 0x00},
    {0x3e, 0x00},
    {GC2053_REG_END, 0x00},     /* END MARKER */
};

static int gc2053_write(struct i2c_client *i2c, unsigned char reg, unsigned char value)
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
        printk(KERN_ERR "gc2053: failed to write reg: %x\n", (int)reg);
    }

    return ret;
}

static int gc2053_read(struct i2c_client *i2c, unsigned char reg, unsigned char *value)
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
        printk(KERN_ERR "gc2053(%x): failed to read reg: %x\n", i2c->addr, (int)reg);
    }

    return ret;
}

static int gc2053_write_array(struct i2c_client *i2c, struct regval_list *vals)
{
    int ret;

    while (vals->reg_num != GC2053_REG_END) {
        if (vals->reg_num == GC2053_REG_DELAY) {
            m_msleep(vals->value);
        } else {
            ret = gc2053_write(i2c, vals->reg_num, vals->value);
            if (ret < 0)
                return ret;
        }
        // printk(KERN_ERR "vals->reg_num:%x, vals->value:%x\n", vals->reg_num, vals->value);
        vals++;
    }

    return 0;
}

static inline int gc2053_read_array(struct i2c_client *i2c, struct regval_list *vals)
{
    int ret;
    unsigned char val;

    while (vals->reg_num != GC2053_REG_END) {
        if (vals->reg_num == GC2053_REG_DELAY) {
            m_msleep(vals->value);
        } else {
            ret = gc2053_read(i2c, vals->reg_num, &val);
            if (ret < 0)
                return ret;
        }
        printk(KERN_ERR "vals->reg_num:%x, vals->value:%x\n", vals->reg_num, val);
        vals++;
    }

    return 0;
}

static int gc2053_detect(struct i2c_client *i2c)
{
    unsigned char h = 1;
    unsigned char l = 1;
    int ret;

    ret = gc2053_read(i2c, GC2053_REG_CHIP_ID_HIGH, &h);
    if (ret < 0)
        return ret;
    if (h != GC2053_CHIP_ID_H) {
        printk(KERN_ERR "gc2053 read chip id high failed:0x%x\n", h);
        return -ENODEV;
    }

    ret = gc2053_read(i2c, GC2053_REG_CHIP_ID_LOW, &l);
    if (ret < 0)
        return ret;

    if (l != GC2053_CHIP_ID_L) {
        printk(KERN_ERR "gc2053 read chip id low failed:0x%x\n", l);
        return -ENODEV;
    }
    printk(KERN_DEBUG "gc2053 get chip id = %02x%02x\n", h, l);

    return 0;
}

static int init_gpio(void)
{
    int ret;
    char gpio_str[10];

    if (reset_gpio != -1) {
        ret = gpio_request(reset_gpio, "gc2053_reset");
        if (ret) {
            printk(KERN_ERR "gc2053: failed to request rst pin: %s\n", gpio_to_str(reset_gpio, gpio_str));
            goto err_reset_gpio;
        }
    }

    if (pwdn_gpio != -1) {
        ret = gpio_request(pwdn_gpio, "gc2053_pwdn");
        if (ret) {
            printk(KERN_ERR "gc2053: failed to request pwdn pin: %s\n", gpio_to_str(pwdn_gpio, gpio_str));
            goto err_pwdn_gpio;
        }
    }

    if (power_gpio != -1) {
        ret = gpio_request(power_gpio, "gc2053_power");
        if (ret) {
            printk(KERN_ERR "gc2053: failed to request power pin: %s\n", gpio_to_str(power_gpio, gpio_str));
            goto err_power_gpio;
        }
    }

    if (strcmp(regulator_name, "-1") && strlen(regulator_name)) {
        gc2053_regulator = regulator_get(NULL, regulator_name);
        if (IS_ERR(gc2053_regulator)) {
            printk(KERN_ERR "regulator_get error!\n");
            ret = -1;
            goto err_regulator;
        }
    }
    return 0;

err_regulator:
    if (power_gpio != -1)
        gpio_free(power_gpio);
err_power_gpio:
    if (pwdn_gpio != -1)
        gpio_free(pwdn_gpio);
err_pwdn_gpio:
    if (reset_gpio != -1)
        gpio_free(reset_gpio);
err_reset_gpio:
    return ret;
}

static void deinit_gpio(void)
{
    if (power_gpio != -1)
        gpio_free(power_gpio);

    if (pwdn_gpio != -1)
        gpio_free(pwdn_gpio);

    if (reset_gpio != -1)
        gpio_free(reset_gpio);

    if (gc2053_regulator)
        regulator_put(gc2053_regulator);
}

static void gc2053_power_off(void)
{
    if (reset_gpio != -1)
        gpio_direction_output(reset_gpio, 0);

    if (pwdn_gpio != -1)
        gpio_direction_output(pwdn_gpio, 1);

    if (power_gpio != -1)
        gpio_direction_output(power_gpio, 0);

    if (gc2053_regulator)
        regulator_disable(gc2053_regulator);

    camera_disable_sensor_mclk(cam_bus_num);
}

static int gc2053_power_on(void)
{
    int ret;

    camera_enable_sensor_mclk(cam_bus_num, 24 * 1000 * 1000);

    if (gc2053_regulator)
        regulator_enable(gc2053_regulator);

    if (power_gpio != -1) {
        gpio_direction_output(power_gpio, 1);
        m_msleep(50);
    }

    if (pwdn_gpio != -1){
        gpio_direction_output(pwdn_gpio, 1);
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

    ret = gc2053_detect(i2c_dev);
    if (ret) {
        printk(KERN_ERR "gc2053: failed to detect\n");
        gc2053_power_off();
        return ret;
    }

    ret = gc2053_write_array(i2c_dev, gc2053_sensor_attr.sensor_info.private_init_setting);
    // ret += gc2053_read_array(i2c_dev, gc2053_sensor_attr.sensor_info.private_init_setting);

    if (ret) {
        printk(KERN_ERR "gc2053: failed to init regs\n");
        gc2053_power_off();
        return ret;
    }

    return 0;
}

static int gc2053_stream_on(void)
{
    int ret = gc2053_write_array(i2c_dev, gc2053_regs_stream_on);
    if (ret)
        printk(KERN_ERR "gc2053: failed to stream on\n");

    return ret;
}

static void gc2053_stream_off(void)
{
    int ret = gc2053_write_array(i2c_dev, gc2053_regs_stream_off);
    if (ret)
        printk(KERN_ERR "gc2053: failed to stream on\n");
}

static int gc2053_g_register(struct sensor_dbg_register *reg)
{
    unsigned char val;
    int ret;

    ret = gc2053_read(i2c_dev, reg->reg & 0xffff, &val);
    reg->val = val;
    reg->size = 2;
    return ret;
}

static int gc2053_s_register(struct sensor_dbg_register *reg)
{
    return gc2053_write(i2c_dev, reg->reg & 0xffff, reg->val & 0xffff);
}

static int gc2053_set_integration_time(int value)
{
    int ret = 0;
    unsigned int expo = value;

    ret = gc2053_write(i2c_dev, 0xfe, 0x00);
    ret += gc2053_write(i2c_dev, 0x04, (unsigned char)(expo & 0xff));
    ret += gc2053_write(i2c_dev, 0x03, (unsigned char)((expo >> 8) & 0x3f));
    if (ret < 0) {
        printk(KERN_ERR "gc2053: failed to set integration time\n");
        return ret;
    }

    return 0;
}

static unsigned int gc2053_alloc_again(unsigned int isp_gain, unsigned char shift, unsigned int *sensor_again)
{
    struct again_lut *lut = gc2053_again_lut;

    while (lut->gain <= gc2053_sensor_attr.sensor_info.max_again) {
        if (isp_gain <= 0) {
            *sensor_again = lut[0].index;
            return lut[0].gain;
        }
        else if (isp_gain < lut->gain) {
            *sensor_again = (lut - 1)->index;
            return (lut - 1)->gain;
        } else {
            if ((lut->gain == gc2053_sensor_attr.sensor_info.max_again) && (isp_gain >= lut->gain)) {
                *sensor_again = lut->index;
                return lut->gain;
            }
        }

        lut++;
    }

    return isp_gain;
}

static int gc2053_set_analog_gain(int value)
{
    int ret = 0;
    struct again_lut *val_lut = gc2053_again_lut;

    ret = gc2053_write(i2c_dev, 0xfe, 0x00);
    ret += gc2053_write(i2c_dev, 0xb4, val_lut[value].regb4);
    ret += gc2053_write(i2c_dev, 0xb3, val_lut[value].regb3);
    ret += gc2053_write(i2c_dev, 0xb8, val_lut[value].regb8);
    ret += gc2053_write(i2c_dev, 0xb9, val_lut[value].regb9);
    if (ret < 0) {
        printk(KERN_ERR "gc2053: failed to set analog gain\n");
        return ret;
    }
    return 0;
}

static unsigned int gc2053_alloc_dgain(unsigned int isp_gain, unsigned char shift, unsigned int *sensor_dgain)
{

    return isp_gain;
}

static int gc2053_set_digital_gain(int value)
{
    return 0;
}

static int gc2053_set_fps(int fps)
{
    struct sensor_info *sensor_info = &gc2053_sensor_attr.sensor_info;
    unsigned int wpclk = 0;
    unsigned short vts = 0;
    unsigned short hts=0;
    unsigned char tmp;
    unsigned int newformat = 0;
    int ret = 0;

    newformat = (((fps >> 16) / (fps & 0xffff)) << 8) + ((((fps >> 16) % (fps & 0xffff)) << 8) / (fps & 0xffff));
    if (newformat > (SENSOR_OUTPUT_MAX_FPS << 8) || newformat <(SENSOR_OUTPUT_MIN_FPS) << 8)
        return -1;

    wpclk = GC2033_SUPPORT_WPCLK_FPS_30;
    ret = gc2053_write(i2c_dev, 0xfe, 0x0);
    ret += gc2053_read(i2c_dev, 0x05, &tmp);
    hts = tmp;
    ret += gc2053_read(i2c_dev, 0x06, &tmp);
    if(ret < 0)
        return -1;
    hts = ((hts << 8) + tmp) << 1;

    vts = wpclk * (fps & 0xffff) / hts / ((fps & 0xffff0000) >> 16);
    ret = gc2053_write(i2c_dev, 0x41, (unsigned char)((vts & 0x3f00) >> 8));
    ret += gc2053_write(i2c_dev, 0x42, (unsigned char)(vts & 0xff));
    if(ret < 0)
        return -1;

    sensor_info->fps = fps;
    sensor_info->total_height = vts;
    sensor_info->max_integration_time = vts - 4;

    return 0;
}


static struct sensor_attr gc2053_sensor_attr = {
    .device_name            = GC2053_DEVICE_NAME,
    .cbus_addr              = GC2053_DEVICE_I2C_ADDR,

    .dbus_type              = SENSOR_DATA_BUS_DVP,
    .dvp = {
        .data_fmt           = DVP_RAW10,
        .gpio_mode          = DVP_PA_LOW_10BIT,
        .timing_mode        = DVP_HREF_MODE,
        .yuv_data_order = 0,
        .hsync_polarity = POLARITY_HIGH_ACTIVE,
        .vsync_polarity = POLARITY_HIGH_ACTIVE,
        .img_scan_mode  = DVP_IMG_SCAN_PROGRESS,
    },

    .isp_clk_rate           = 150 * 1000 * 1000,

    .sensor_info = {
        .private_init_setting   = gc2053_init_regs_1920_1080_30fps_dvp,
        .width                  = GC2053_WIDTH,
        .height                 = GC2053_HEIGHT,
        .fmt                    = SENSOR_PIXEL_FMT_SRGGB10_1X10,

        .fps                    = 30 << 16 | 1,
        .total_width            = 0x44c * 2,
        .total_height           = 0x546,

        .min_integration_time   = 3,
        .max_integration_time   = 0x546 - 4,
        .one_line_expr_in_us    = 30,
        .max_again              = 444858,
        .max_dgain              = 0,
    },

    .ops = {
        .power_on               = gc2053_power_on,
        .power_off              = gc2053_power_off,
        .stream_on              = gc2053_stream_on,
        .stream_off             = gc2053_stream_off,
        .get_register           = gc2053_g_register,
        .set_register           = gc2053_s_register,

        .set_integration_time   = gc2053_set_integration_time,
        .alloc_again            = gc2053_alloc_again,
        .set_analog_gain        = gc2053_set_analog_gain,
        .alloc_dgain            = gc2053_alloc_dgain,
        .set_digital_gain       = gc2053_set_digital_gain,
        .set_fps                = gc2053_set_fps,
    },
};

static int gc2053_probe(struct i2c_client *client,
        const struct i2c_device_id *id)
{
    int ret = init_gpio();
    if (ret)
        return ret;

    ret = dvp_init_select_gpio(&gc2053_sensor_attr.dvp, gc2053_sensor_attr.dvp.gpio_mode);
    if (ret) {
        deinit_gpio();
        return ret;
    }

    ret = camera_register_sensor(cam_bus_num, &gc2053_sensor_attr);
    if (ret) {
        dvp_deinit_gpio();
        deinit_gpio();
        return ret;
    }

    return 0;
}

static int gc2053_remove(struct i2c_client *client)
{
    camera_unregister_sensor(cam_bus_num, &gc2053_sensor_attr);
    dvp_deinit_gpio();
    deinit_gpio();

    return 0;
}

static struct i2c_device_id gc2053_id[] = {
    { GC2053_DEVICE_NAME, 0 },
    { }
};
MODULE_DEVICE_TABLE(i2c, gc2053_id);

static struct i2c_driver gc2053_driver = {
    .driver = {
        .owner          = THIS_MODULE,
        .name           = GC2053_DEVICE_NAME,
    },
    .probe              = gc2053_probe,
    .remove             = gc2053_remove,
    .id_table           = gc2053_id,
};

static struct i2c_board_info sensor_gc2053_info = {
    .type               = GC2053_DEVICE_NAME,
    .addr               = GC2053_DEVICE_I2C_ADDR,
};

static __init int init_gc2053(void)
{
    if (i2c_bus_num < 0) {
        printk(KERN_ERR "gc2053: i2c_bus_num must be set\n");
        return -EINVAL;
    }
    gc2053_driver.driver.name = sensor_name;
    strcpy(gc2053_id[0].name, sensor_name);
    if (i2c_addr != -1)
        sensor_gc2053_info.addr = i2c_addr;
    strcpy(sensor_gc2053_info.type, sensor_name);
    gc2053_sensor_attr.device_name = sensor_name;

    int ret = i2c_add_driver(&gc2053_driver);
    if (ret) {
        printk(KERN_ERR "gc2053: failed to register i2c driver\n");
        return ret;
    }

    i2c_dev = i2c_register_device(&sensor_gc2053_info, i2c_bus_num);
    if (i2c_dev == NULL) {
        printk(KERN_ERR "gc2053: failed to register i2c device\n");
        i2c_del_driver(&gc2053_driver);
        return -EINVAL;
    }

    return 0;
}

static __exit void exit_gc2053(void)
{
    i2c_unregister_device(i2c_dev);
    i2c_del_driver(&gc2053_driver);
}

module_init(init_gc2053);
module_exit(exit_gc2053);

MODULE_DESCRIPTION("x2000 gc2053 driver");
MODULE_LICENSE("GPL");
