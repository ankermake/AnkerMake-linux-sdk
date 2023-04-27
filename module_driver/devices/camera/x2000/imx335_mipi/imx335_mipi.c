/*
 * Copyright (C) 2020 Ingenic Semiconductor Co., Ltd.
 *
 * IMX335
 *
 */
// #include <linux/sched.h>
// #include <linux/kthread.h>
#include <linux/module.h>
#include <utils/gpio.h>
#include <utils/i2c.h>
#include <common.h>
#include <linux/regulator/consumer.h>
#include "soc/x2000/camera/hal/camera_sensor.h"

// #include "key_log.h"


#define CONFIG_IMX335_1920X1080


#define IMX335_DEVICE_NAME              "imx335"
#define IMX335_DEVICE_I2C_ADDR          0x1a

#define IMX335_CHIP_ID_H                0x08
#define IMX335_CHIP_ID_L                0x00

#define IMX335_REG_END                  0xffff
#define IMX335_REG_DELAY                0xfffe

#define IMX335_SUPPORT_PCLK             (67.5*1000*1000)
#define SENSOR_OUTPUT_MAX_FPS           30
#define SENSOR_OUTPUT_MIN_FPS           5

#define AGAIN_MAX_DB                    0x50
#define DGAIN_MAX_DB                    0x3c
#define LOG2_GAIN_SHIFT                 16


static int power_gpio       = -1;
static int reset_gpio       = -1;    // GPIO_PB(12);
static int pwdn_gpio        = -1;    // GPIO_PB(18);
static int i2c_bus_num      = -1;
static short i2c_addr       = -1;
static int cam_bus_num      = -1;
static char *sensor_name    = NULL;
static char *regulator_name = "";

module_param_gpio(power_gpio, 0644);
module_param_gpio(reset_gpio, 0644);
module_param_gpio(pwdn_gpio, 0644);

module_param(i2c_bus_num, int, 0644);
module_param(i2c_addr, short, 0644);
module_param(cam_bus_num, int, 0644);
module_param(sensor_name, charp, 0644);
module_param(regulator_name, charp, 0644);

static struct i2c_client *i2c_dev;
static struct sensor_attr imx335_sensor_attr;
static struct regulator *imx335_regulator = NULL;

struct again_lut {
    unsigned int value;
    unsigned int gain;
};

struct regval_list {
    unsigned short reg_num;
    unsigned char value;
};
struct again_lut imx335_again_lut[] = {
    /* analog gain */
    {0x0, 0},
    {0x1, 3265},
    {0x2, 6531},
    {0x3, 9796},
    {0x4, 13062},
    {0x5, 16327},
    {0x6, 19593},
    {0x7, 22859},
    {0x8, 26124},
    {0x9, 29390},
    {0xa, 32655},
    {0xb, 35921},
    {0xc, 39187},
    {0xd, 42452},
    {0xe, 45718},
    {0xf, 48983},
    {0x10, 52249},
    {0x11, 55514},
    {0x12, 58780},
    {0x13, 62046},
    {0x14, 65311},
    {0x15, 68577},
    {0x16, 71842},
    {0x17, 75108},
    {0x18, 78374},
    {0x19, 81639},
    {0x1a, 84905},
    {0x1b, 88170},
    {0x1c, 91436},
    {0x1d, 94702},
    {0x1e, 97967},
    {0x1f, 101233},
    {0x20, 104498},
    {0x21, 107764},
    {0x22, 111029},
    {0x23, 114295},
    {0x24, 117561},
    {0x25, 120826},
    {0x26, 124092},
    {0x27, 127357},
    {0x28, 130623},
    {0x29, 133889},
    {0x2a, 137154},
    {0x2b, 140420},
    {0x2c, 143685},
    {0x2d, 146951},
    {0x2e, 150217},
    {0x2f, 153482},
    {0x30, 156748},
    {0x31, 160013},
    {0x32, 163279},
    {0x33, 166544},
    {0x34, 169810},
    {0x35, 173076},
    {0x36, 176341},
    {0x37, 179607},
    {0x38, 182872},
    {0x39, 186138},
    {0x3a, 189404},
    {0x3b, 192669},
    {0x3c, 195935},
    {0x3d, 199200},
    {0x3e, 202466},
    {0x3f, 205732},
    {0x40, 208997},
    {0x41, 212263},
    {0x42, 215528},
    {0x43, 218794},
    {0x44, 222059},
    {0x45, 225325},
    {0x46, 228591},
    {0x47, 231856},
    {0x48, 235122},
    {0x49, 238387},
    {0x4a, 241653},
    {0x4b, 244919},
    {0x4c, 248184},
    {0x4d, 251450},
    {0x4e, 254715},
    {0x4f, 257981},
    {0x50, 261247},
    {0x51, 264512},
    {0x52, 267778},
    {0x53, 271043},
    {0x54, 274309},
    {0x55, 277574},
    {0x56, 280840},
    {0x57, 284106},
    {0x58, 287371},
    {0x59, 290637},
    {0x5a, 293902},
    {0x5b, 297168},
    {0x5c, 300434},
    {0x5d, 303699},
    {0x5e, 306965},
    {0x5f, 310230},
    {0x60, 313496},
    {0x61, 316762},
    {0x62, 320027},
    {0x63, 323293},
    {0x64, 326558},
    /* analog+digital */
    {0x65, 329824},
    {0x66, 333089},
    {0x67, 336355},
    {0x68, 339621},
    {0x69, 342886},
    {0x6a, 346152},
    {0x6b, 349417},
    {0x6c, 352683},
    {0x6d, 355949},
    {0x6e, 359214},
    {0x6f, 362480},
    {0x70, 365745},
    {0x71, 369011},
    {0x72, 372277},
    {0x73, 375542},
    {0x74, 378808},
    {0x75, 382073},
    {0x76, 385339},
    {0x77, 388604},
    {0x78, 391870},
    {0x79, 395136},
    {0x7a, 398401},
    {0x7b, 401667},
    {0x7c, 404932},
    {0x7d, 408198},
    {0x7e, 411464},
    {0x7f, 414729},
    {0x80, 417995},
    {0x81, 421260},
    {0x82, 424526},
    {0x83, 427792},
    {0x84, 431057},
    {0x85, 434323},
    {0x86, 437588},
    {0x87, 440854},
    {0x88, 444119},
    {0x89, 447385},
    {0x8a, 450651},
    {0x8b, 453916},
    {0x8c, 457182},
    {0x8d, 460447},
    {0x8e, 463713},
    {0x8f, 466979},
    {0x90, 470244},
    {0x91, 473510},
    {0x92, 476775},
    {0x93, 480041},
    {0x94, 483307},
    {0x95, 486572},
    {0x96, 489838},
    {0x97, 493103},
    {0x98, 496369},
    {0x99, 499634},
    {0x9a, 502900},/*204x*/
    {0x9b, 506166},
    {0x9c, 509431},
    {0x9d, 512697},
    {0x9e, 515962},
    {0x9f, 519228},
    {0xa0, 522494},
    {0xa1, 525759},
    {0xa2, 529025},
    {0xa3, 532290},
    {0xa4, 535556},
    {0xa5, 538822},
    {0xa6, 542087},
    {0xa7, 545353},
    {0xa8, 548618},
    {0xa9, 551884},
    {0xaa, 555149},
    {0xab, 558415},
    {0xac, 561681},
    {0xad, 564946},
    {0xae, 568212},
    {0xaf, 571477},
    {0xb0, 574743},
    {0xb1, 578009},
    {0xb2, 581274},
    {0xb3, 584540},
    {0xb4, 587805},
    {0xb5, 591071},
    {0xb6, 594337},
    {0xb7, 597602},
    {0xb8, 600868},
    {0xb9, 604133},
    {0xba, 607399},
    {0xbb, 610664},
    {0xbc, 613930},
    {0xbd, 617196},
    {0xbe, 620461},
    {0xbf, 623727},
    {0xc0, 626992},
    {0xc1, 630258},
    {0xc2, 633524},
    {0xc3, 636789},
    {0xc4, 640055},
    {0xc5, 643320},
    {0xc6, 646586},
    {0xc7, 649852},
    {0xc8, 653117},
    {0xc9, 656383},
    {0xca, 659648},
    {0xcb, 662914},
    {0xcc, 666179},
    {0xcd, 669445},
    {0xce, 672711},
    {0xcf, 675976},
    {0xd0, 679242},
    {0xd1, 682507},
    {0xd2, 685773},
    {0xd3, 689039},
    {0xd4, 692304},
    {0xd5, 695570},
    {0xd6, 698835},
    {0xd7, 702101},
    {0xd8, 705367},
    {0xd9, 708632},
    {0xda, 711898},
    {0xdb, 715163},
    {0xdc, 718429},
    {0xdd, 721694},
    {0xde, 724960},
    {0xdf, 728226},
    {0xe0, 731491},
    {0xe1, 734757},
    {0xe2, 738022},
    {0xe3, 741288},
    {0xe4, 744554},
    {0xe5, 747819},
    {0xe6, 751085},
    {0xe7, 754350},
    {0xe8, 757616},
    {0xe9, 760882},
    {0xea, 764147},
    {0xeb, 767413},
    {0xec, 770678},
    {0xed, 773944},
    {0xee, 777209},
    {0xef, 780475},
    {0xf0, 783741},
};

static struct regval_list imx335_init_regs_2040_1940_30fps_mipi_2lan[] = {
   /* inclk 37.25M clk 1080p@30fps Sunnic */
    {0x3002, 0x01},
    {0x3004, 0x00},
    {0x300C, 0x5B}, // BCWAIT_TIME[7:0]
    {0x300D, 0x40}, // CPWAIT_TIME[7:0]
    {0x3018, 0x04}, // WINMODE[3:0]
    {0x302C, 0x50}, // HTRIMMING_START[11:0]
    {0x302D, 0x01}, //
    {0x302E, 0xF8}, // HNUM[11:0]
    {0x302F, 0x07}, //  2040
    {0x3050, 0x00}, // ADBIT[0]
    {0x3056, 0x94}, // Y_OUT_SIZE[12:0] 1940
    {0x3074, 0xC8}, // AREA3_ST_ADR_1[12:0]
    {0x3076, 0x28}, // AREA3_WIDTH_1[12:0] 1940
    {0x315A, 0x02}, // INCKSEL2[1:0]
    {0x316A, 0x7E}, // INCKSEL4[1:0]
    {0x319D, 0x00}, // MDBIT
    {0x31A1, 0x00}, // XVS_DRV[1:0]
    {0x3288, 0x21}, // -
    {0x328A, 0x02}, // -
    {0x3414, 0x05}, // -
    {0x3416, 0x18}, // -
    {0x341C, 0xFF}, // ADBIT1[8:0]
    {0x341D, 0x01}, //
    {0x3648, 0x01}, // -
    {0x364A, 0x04}, // -
    {0x364C, 0x04}, // -
    {0x3678, 0x01}, // -
    {0x367C, 0x31}, // -
    {0x367E, 0x31}, // -
    {0x3706, 0x10}, // -
    {0x3708, 0x03}, // -
    {0x3714, 0x02}, // -
    {0x3715, 0x02}, // -
    {0x3716, 0x01}, // -
    {0x3717, 0x03}, // -
    {0x371C, 0x3D}, // -
    {0x371D, 0x3F}, // -
    {0x372C, 0x00}, // -
    {0x372D, 0x00}, // -
    {0x372E, 0x46}, // -
    {0x372F, 0x00}, // -
    {0x3730, 0x89}, // -
    {0x3731, 0x00}, // -
    {0x3732, 0x08}, // -
    {0x3733, 0x01}, // -
    {0x3734, 0xFE}, // -
    {0x3735, 0x05}, // -
    {0x3740, 0x02}, // -
    {0x375D, 0x00}, // -
    {0x375E, 0x00}, // -
    {0x375F, 0x11}, // -
    {0x3760, 0x01}, // -
    {0x3768, 0x1A}, // -
    {0x3769, 0x1A}, // -
    {0x376A, 0x1A}, // -
    {0x376B, 0x1A}, // -
    {0x376C, 0x1A}, // -
    {0x376D, 0x17}, // -
    {0x376E, 0x0F}, // -
    {0x3776, 0x00}, // -
    {0x3777, 0x00}, // -
    {0x3778, 0x46}, // -
    {0x3779, 0x00}, // -
    {0x377A, 0x89}, // -
    {0x377B, 0x00}, // -
    {0x377C, 0x08}, // -
    {0x377D, 0x01}, // -
    {0x377E, 0x23}, // -
    {0x377F, 0x02}, // -
    {0x3780, 0xD9}, // -
    {0x3781, 0x03}, // -
    {0x3782, 0xF5}, // -
    {0x3783, 0x06}, // -
    {0x3784, 0xA5}, // -
    {0x3788, 0x0F}, // -
    {0x378A, 0xD9}, // -
    {0x378B, 0x03}, // -
    {0x378C, 0xEB}, // -
    {0x378D, 0x05}, // -
    {0x378E, 0x87}, // -
    {0x378F, 0x06}, // -
    {0x3790, 0xF5}, // -
    {0x3792, 0x43}, // -
    {0x3794, 0x7A}, // -
    {0x3796, 0xA1}, // -
    {0x3A01, 0x01}, // LANEMODE[2:0]

    {0x3002, 0x00},
    {0x3000, 0x00},

    {IMX335_REG_END, 0x00},    /* END MARKER */
};

static struct regval_list imx335_regs_stream_on[] = {
    {0x3000, 0x00},             /* RESET_REGISTER */
    {IMX335_REG_END, 0x00},     /* END MARKER */
};

static struct regval_list imx335_regs_stream_off[] = {
    {0x3000, 0x01},             /* RESET_REGISTER */
    {IMX335_REG_END, 0x00},     /* END MARKER */
};

static int imx335_write(struct i2c_client *i2c, unsigned short reg, unsigned char value)
{
    unsigned char buf[3] = {reg >> 8, reg & 0xff, value};
    struct i2c_msg msg = {
        .addr   = i2c->addr,
        .flags  = 0,
        .len    = 3,
        .buf    = buf,
    };

    int ret = i2c_transfer(i2c->adapter, &msg, 1);
    if (ret < 0)
        printk(KERN_ERR "imx335: failed to write reg: %x\n", (int)reg);

    return ret;
}

static int imx335_read(struct i2c_client *i2c, unsigned short reg, unsigned char *value)
{
    unsigned char buf[2] = {reg >> 8, reg & 0xff};
    struct i2c_msg msg[2] = {
        [0] = {
            .addr   = i2c->addr,
            .flags  = 0,
            .len    = 2,
            .buf    = buf,
        },
        [1] = {
            .addr   = i2c->addr,
            .flags  = I2C_M_RD,
            .len    = 1,
            .buf    = value,
        }
    };

    int ret = i2c_transfer(i2c->adapter, msg, 2);
    if (ret < 0)
        printk(KERN_ERR "imx335(%x): failed to read reg: %x\n", i2c->addr, (int)reg);

    return ret;
}

static int imx335_write_array(struct i2c_client *i2c, struct regval_list *vals)
{
    int ret;
    while (vals->reg_num != IMX335_REG_END) {
        if (vals->reg_num == IMX335_REG_DELAY) {
                m_msleep(vals->value);
        } else {
            ret = imx335_write(i2c, vals->reg_num, vals->value);
            if (ret < 0)
                return ret;
        }
        vals++;
    }

    return 0;
}

static inline int imx335_read_array(struct i2c_client *i2c, struct regval_list *vals)
{
    int ret;
    unsigned char val;
    while (vals->reg_num != IMX335_REG_END) {
        if (vals->reg_num == IMX335_REG_DELAY) {
                m_msleep(vals->value);
        } else {
            ret = imx335_read(i2c, vals->reg_num, &val);
            if (ret < 0)
                return ret;
        }

        printk("reg = 0x%02x, val = 0x%02x\n",vals->reg_num, val);
        vals++;
    }

    return 0;
}

static int imx335_detect(struct i2c_client *i2c)
{
    int ret;
    unsigned char h = 1;
    unsigned char l = 1;
    unsigned char v;

    ret = imx335_read(i2c, 0x3112, &v);
    if (ret < 0)
        return ret;
    if (v != IMX335_CHIP_ID_H) {
        printk(KERN_ERR "imx335 read chip id high failed:0x%x\n", h);
        return -ENODEV;
    }

    ret = imx335_read(i2c, 0x3113, &v);
    if (ret < 0)
        return ret;

    if (v != IMX335_CHIP_ID_L) {
        printk(KERN_ERR "imx335 read chip id low failed:0x%x\n", l);
        return -ENODEV;
    }
    printk(KERN_DEBUG "imx335 get chip id = %02x%02x\n", h, l);

    return 0;
}

static int init_gpio(void)
{
    int ret;
    char gpio_str[10];

    if (reset_gpio != -1) {
        ret = gpio_request(reset_gpio, "imx335_reset");
        if (ret) {
            printk(KERN_ERR "imx335: failed to request rst pin: %s\n", gpio_to_str(reset_gpio, gpio_str));
            goto err_reset_gpio;
        }
    }

    if (pwdn_gpio != -1) {
        ret = gpio_request(pwdn_gpio, "imx335_pwdn");
        if (ret) {
            printk(KERN_ERR "imx335: failed to request pwdn pin: %s\n", gpio_to_str(pwdn_gpio, gpio_str));
            goto err_pwdn_gpio;
        }
    }

    if (power_gpio != -1) {
        ret = gpio_request(power_gpio, "imx335_power");
        if (ret) {
            printk(KERN_ERR "imx335: failed to request power pin: %s\n", gpio_to_str(power_gpio, gpio_str));
            goto err_power_gpio;
        }
    }

    if (strcmp(regulator_name, "-1") && strlen(regulator_name)) {
        imx335_regulator = regulator_get(NULL, regulator_name);
        if (IS_ERR(imx335_regulator)) {
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

    if (imx335_regulator)
        regulator_put(imx335_regulator);
}

static void imx335_power_off(void)
{
    if (reset_gpio != -1)
        gpio_direction_output(reset_gpio, 0);

    if (pwdn_gpio != -1)
        gpio_direction_output(pwdn_gpio, 1);

    if (power_gpio != -1)
        gpio_direction_output(power_gpio, 0);

    if (imx335_regulator)
        regulator_disable(imx335_regulator);

    camera_disable_sensor_mclk(cam_bus_num);
}

static int imx335_power_on(void)
{
    int ret;

    camera_enable_sensor_mclk(cam_bus_num, 37125000);

    if (imx335_regulator)
        regulator_enable(imx335_regulator);

    if (power_gpio != -1) {
        gpio_direction_output(power_gpio, 1);
        m_msleep(50);
    }

    if (pwdn_gpio != -1){
        gpio_direction_output(pwdn_gpio, 0);
        m_msleep(50);
        gpio_direction_output(pwdn_gpio, 1);
        m_msleep(10);
    }

    if (reset_gpio != -1){
        gpio_direction_output(reset_gpio, 1);
        m_msleep(5);
        gpio_direction_output(reset_gpio, 0);
        m_msleep(10);
        gpio_direction_output(reset_gpio, 1);
        m_msleep(30);
    }

    ret = imx335_detect(i2c_dev);
    if (ret) {
        printk(KERN_ERR "imx335: failed to detect\n");
       imx335_power_off();
        return ret;
    }

    ret = imx335_write_array(i2c_dev, imx335_sensor_attr.sensor_info.private_init_setting);
    if (ret) {
        printk(KERN_ERR "imx335: failed to init regs\n");
        imx335_power_off();
        return ret;
    }

    return 0;
}

static int imx335_stream_on(void)
{
    int ret = imx335_write_array(i2c_dev, imx335_regs_stream_on);
    if (ret)
        printk(KERN_ERR "imx335: failed to stream on\n");

    return ret;
}

static void imx335_stream_off(void)
{
    int ret = imx335_write_array(i2c_dev, imx335_regs_stream_off);
    if (ret)
        printk(KERN_ERR "imx335: failed to stream on\n");
}

static int imx335_g_register(struct sensor_dbg_register *reg)
{
    unsigned char val;
    int ret;

    ret = imx335_read(i2c_dev, reg->reg & 0xffff, &val);
    reg->val = val;
    reg->size = 2;
    return ret;
}

static int imx335_s_register(struct sensor_dbg_register *reg)
{
    return imx335_write(i2c_dev, reg->reg & 0xffff, reg->val & 0xffff);
}

static int imx335_set_integration_time(int value)
{
    int ret = 0;
    unsigned short shr0 = 0;
    unsigned short vmax = 0;

    vmax = imx335_sensor_attr.sensor_info.total_height;
    shr0 = vmax - value;

    ret = imx335_write(i2c_dev, 0x3058, (unsigned char)(shr0 & 0xff));
    ret += imx335_write(i2c_dev, 0x3059, (unsigned char)((shr0 >> 8) & 0xff));
    ret += imx335_write(i2c_dev, 0x305a, (unsigned char)((shr0 >> 16) & 0x0f));

    if(0 != ret){
        return -1;
    }

    return 0;
}

unsigned int imx335_alloc_again(unsigned int isp_gain, unsigned char shift, unsigned int *sensor_again)
{
#if 0
    /* again_reg value = gain[dB]*10 */
    again_reg = (isp_gain*20)>>LOG2_GAIN_SHIFT;
    // Limit Max gain
    printk("isp_gain in = %d, again_reg = %d\n",isp_gain,again_reg);
    if(again_reg > (AGAIN_MAX_DB + DGAIN_MAX_DB)*10)
        again_reg = (AGAIN_MAX_DB + DGAIN_MAX_DB)*10;
    /* p_ctx->again_reg=again_reg; */
    *sensor_again=again_reg;
    isp_gain= (((int32_t)again_reg)<<LOG2_GAIN_SHIFT)/200;
    return isp_gain;
#endif
    struct again_lut *lut = imx335_again_lut;

    while (lut->gain <= imx335_sensor_attr.sensor_info.max_again) {
        if (isp_gain <= imx335_again_lut[0].gain) {
            *sensor_again = lut[0].value;
            return lut[0].gain;
        } else if (isp_gain < lut->gain) {
            *sensor_again = (lut - 1)->value;
            return (lut - 1)->gain;
        } else {
            if((lut->gain == imx335_sensor_attr.sensor_info.max_again) && (isp_gain >= lut->gain)) {
                *sensor_again = lut->value;
                return lut->gain;
            }
        }

        lut++;
    }

    return isp_gain;
}

static int imx335_set_analog_gain(int value)
{
    int ret = 0;

    ret = imx335_write(i2c_dev, 0x30e8, (unsigned char)(value & 0xff));
    ret += imx335_write(i2c_dev, 0x30e9, (unsigned char)((value >> 8) & 0x07));
    if (0 != 0)
        return ret;

    return 0;

}

unsigned int imx335_alloc_dgain(unsigned int isp_gain, unsigned char shift, unsigned int *sensor_dgain)
{
    return isp_gain;
}

static int imx335_set_digital_gain(int value)
{
    return 0;
}

static int imx335_set_fps(int fps)
{
    int ret = 0;
    unsigned int pclk = 0;
    unsigned short hmax = 0;
    unsigned short vmax = 0;
    unsigned short cur_int = 0;
    unsigned short shr0 = 0;
    unsigned char value = 0;
    unsigned int newformat = 0;
    struct sensor_info *sensor_info = &imx335_sensor_attr.sensor_info;

    newformat = (((fps >> 16) / (fps & 0xffff)) << 8) + ((((fps >> 16) % (fps & 0xffff)) << 8) / (fps & 0xffff));
    if(newformat > (SENSOR_OUTPUT_MAX_FPS << 8) || newformat < (SENSOR_OUTPUT_MIN_FPS << 8)) {
        printk("warn: fps(%d) no in range\n", fps);
        return -1;
    }
    pclk = IMX335_SUPPORT_PCLK;

#if 1
    /*method 1 change hts*/
    ret = imx335_read(i2c_dev, 0x3030, &value);
    vmax = value;
    ret += imx335_read(i2c_dev, 0x3031, &value);
    vmax |= value << 8;
    ret += imx335_read(i2c_dev, 0x3032, &value);
    vmax |= (value|0x7) << 16;

    hmax = pclk * (fps & 0xffff) / vmax / ((fps & 0xffff0000) >> 16);
    ret += imx335_write(i2c_dev, 0x3034, hmax & 0xff);
    ret += imx335_write(i2c_dev, 0x3035, (hmax >> 8) & 0xff);
    if (0 != ret) {
        printk("err: imx335_write err\n");
        return ret;
    }
    sensor_info->total_height = hmax;

#else
    /*method 2 change vts*/
    ret = imx335_read(sd, 0x3034, &value);
    hmax = value;
    ret += imx335_read(sd, 0x3035, &value);
    hmax = (((value & 0x3f) << 8) | hmax) >> 1;

    vmax = pclk * (fps & 0xffff) / hmax / ((fps & 0xffff0000) >> 16);
    ret += imx335_write(sd, 0x3030, vmax & 0xff);
    ret += imx335_write(sd, 0x3031, (vmax >> 8) & 0xff);
    ret += imx335_write(sd, 0x3032, (vmax >> 16) & 0x0f);
#endif

    /*record current integration time*/
    ret = imx335_read(i2c_dev, 0x3058, &value);
    shr0 = value;
    ret += imx335_read(i2c_dev, 0x3059, &value);
    shr0 = (value << 8) | shr0;
    ret += imx335_read(i2c_dev, 0x305a, &value);
    shr0 = ((value & 0x0f) << 16) | shr0;
    cur_int = sensor_info->total_height - shr0;

    sensor_info->fps = fps;
    sensor_info->total_height = vmax;
    sensor_info->max_integration_time = vmax - 9;

    ret = imx335_set_integration_time(cur_int);
    if(ret < 0)
        return -1;

    return ret;
}


static struct sensor_attr imx335_sensor_attr = {
    .device_name            = IMX335_DEVICE_NAME,
    .cbus_addr              = IMX335_DEVICE_I2C_ADDR,

    .dbus_type              = SENSOR_DATA_BUS_MIPI,
    .mipi = {
        .mipi_crop = {
            .enable             = 1,
            .sensor_ctrl = {
                .mipi_vcomp_en   = 1,
                .mipi_hcomp_en   = 1,
                .line_sync_mode  = 0,
                .work_start_flag = 0,
                .data_type_en    = 1,
                .data_type_value = MIPI_CTRL_RAW10,
                .del_start       = 0,
            },
            .hcrop_start        = 0,
            .vcrop_start        = 0,
            .output_width       = 0x7F8,
            .output_height      = 0x794,
        },

        .data_fmt               = MIPI_RAW10,
        .lanes                  = 2,
        .clk_settle_time        = 120,  /* ns */
        .data_settle_time       = 130,  /* ns */
    },

    .isp_clk_rate           = 200 * 1000 * 1000,

    .sensor_info = {
        .private_init_setting   = imx335_init_regs_2040_1940_30fps_mipi_2lan,
        .width                  = 0x7F8,
        .height                 = 0x794,
        .fmt                    = SENSOR_PIXEL_FMT_SRGGB10_1X10,

        .fps                    = 30 << 16 | 1,
        .total_width            = 0x226,
        .total_height           = 0x1194,

        .min_integration_time   = 1,
        .max_integration_time   = 0x1194 - 9,
        .one_line_expr_in_us    = 28,
        .max_again              = 460447,
        .max_dgain              = 0,
    },

    .ops = {
        .power_on               = imx335_power_on,
        .power_off              = imx335_power_off,
        .stream_on              = imx335_stream_on,
        .stream_off             = imx335_stream_off,
        .get_register           = imx335_g_register,
        .set_register           = imx335_s_register,

        .set_integration_time   = imx335_set_integration_time,
        .alloc_again            = imx335_alloc_again,
        .set_analog_gain        = imx335_set_analog_gain,
        .alloc_dgain            = imx335_alloc_dgain,
        .set_digital_gain       = imx335_set_digital_gain,
        .set_fps                = imx335_set_fps,
    },
};

static int imx335_probe(struct i2c_client *client,
        const struct i2c_device_id *id)
{
    int ret = init_gpio();
    if (ret)
        return ret;

    ret = camera_register_sensor(cam_bus_num, &imx335_sensor_attr);
    if (ret) {
        deinit_gpio();
        return ret;
    }

    return 0;
}

static int imx335_remove(struct i2c_client *client)
{
    camera_unregister_sensor(cam_bus_num, &imx335_sensor_attr);
    deinit_gpio();
    return 0;
}

static struct i2c_device_id imx335_id[] = {
    { IMX335_DEVICE_NAME, 0 },
    { }
};
MODULE_DEVICE_TABLE(i2c, imx335_id);

static struct i2c_driver imx335_driver = {
    .driver = {
        .owner          = THIS_MODULE,
        .name           = IMX335_DEVICE_NAME,
    },
    .probe              = imx335_probe,
    .remove             = imx335_remove,
    .id_table           = imx335_id,
};

static struct i2c_board_info sensor_imx335_info = {
    .type               = IMX335_DEVICE_NAME,
    .addr               = IMX335_DEVICE_I2C_ADDR,
};

static __init int init_imx335(void)
{

    if (i2c_bus_num < 0) {
        printk(KERN_ERR "imx335: i2c_bus_num must be set\n");
        return -EINVAL;
    }
    imx335_driver.driver.name = sensor_name;
    strcpy(imx335_id[0].name, sensor_name);
    if (i2c_addr != -1)
        sensor_imx335_info.addr = i2c_addr;
    strcpy(sensor_imx335_info.type, sensor_name);
    imx335_sensor_attr.device_name = sensor_name;

    int ret = i2c_add_driver(&imx335_driver);
    if (ret) {
        printk(KERN_ERR "imx335: failed to register i2c driver\n");
        return ret;
    }

    i2c_dev = i2c_register_device(&sensor_imx335_info, i2c_bus_num);
    if (i2c_dev == NULL) {
        printk(KERN_ERR "imx335: failed to register i2c device\n");
        i2c_del_driver(&imx335_driver);
        return -EINVAL;
    }

    return 0;
}

static __exit void exit_imx335(void)
{
    i2c_unregister_device(i2c_dev);
    i2c_del_driver(&imx335_driver);
}

module_init(init_imx335);
module_exit(exit_imx335);

MODULE_DESCRIPTION("x2000 imx335 driver");
MODULE_LICENSE("GPL");
