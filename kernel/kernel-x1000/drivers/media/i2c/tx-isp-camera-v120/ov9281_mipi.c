/*
 * ov9281 Camera Driver
 *
 * Copyright (C) 2018, Ingenic Semiconductor Inc.
 *
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */

#include <linux/init.h>
#include <linux/module.h>
#include <linux/slab.h>
#include <linux/i2c.h>
#include <linux/delay.h>
#include <linux/gpio.h>
#include <linux/clk.h>
#include <linux/proc_fs.h>
#include <soc/gpio.h>

#include <tx-isp/tx-isp-common.h>
#include <tx-isp/sensor-common.h>
#include <linux/sensor_board.h>

#define OV9281_CHIP_ID_H               (0x92)
#define OV9281_CHIP_ID_L               (0x81)
#define OV9281_REG_END                  0xffff
#define OV9281_REG_DELAY                0x0000

#if defined CONFIG_OV9281_MIPI_1280X800
#define OV9281_WIDTH			1280
#define OV9281_HEIGHT			800
#define OV9281_HTS				0x2d8
#define OV9281_VTS				0x726
#define SENSOR_OUTPUT_MAX_FPS	60
#elif defined CONFIG_OV9281_MIPI_1280X720
#define OV9281_WIDTH			1280
#define OV9281_HEIGHT			720
#define OV9281_HTS				0x2d8
#define OV9281_VTS				0x38e
#define SENSOR_OUTPUT_MAX_FPS	120
#elif defined CONFIG_OV9281_MIPI_640X480
#define OV9281_WIDTH			640
#define OV9281_HEIGHT			480
#define OV9281_HTS				0x2d8
#define OV9281_VTS				0x393
#define SENSOR_OUTPUT_MAX_FPS	120
#elif defined CONFIG_OV9281_MIPI_640X400
#define OV9281_WIDTH			640
#define OV9281_HEIGHT			400
#define OV9281_HTS				0x2d8
#define OV9281_VTS				0x208
#define SENSOR_OUTPUT_MAX_FPS	210
#endif

#define OV9281_SCLK                     (80000000)
#define SENSOR_OUTPUT_MIN_FPS           5
#define SENSOR_VERSION                  "H20181220"

static int reset_gpio       = -1;
static int pwdn_gpio        = -1;
static int gpio_i2c_sel1    = -1;
static int gpio_i2c_sel2    = -1;

module_param(reset_gpio, int, S_IRUGO);
MODULE_PARM_DESC(reset_gpio, "Reset GPIO NUM");

module_param(pwdn_gpio, int, S_IRUGO);
MODULE_PARM_DESC(pwdn_gpio, "Power down GPIO NUM");

module_param(gpio_i2c_sel1, int, S_IRUGO);
MODULE_PARM_DESC(pwdn_gpio, "I2C select1  GPIO NUM");

module_param(gpio_i2c_sel2, int, S_IRUGO);
MODULE_PARM_DESC(pwdn_gpio, "I2C select2  GPIO NUM");

static struct tx_isp_sensor_attribute ov9281_attr;
static struct sensor_board_info *ov9281_board_info;

struct regval_list {
    unsigned short reg_num;
    unsigned char value;
};

/*
 * the part of driver maybe modify about different sensor and different board.
 */
struct again_lut {
    unsigned int value;
    unsigned int gain;
};

static struct again_lut ov9281_again_lut[] = {
    {0x10, 0},
    {0x11, 5731},
    {0x12, 11136},
    {0x13, 16247},
    {0x14, 21097},
    {0x15, 25710},
    {0x16, 30108},
    {0x17, 34311},
    {0x18, 38335},
    {0x19, 42195},
    {0x1a, 45903},
    {0x1b, 49471},
    {0x1c, 52910},
    {0x1d, 56227},
    {0x1e, 59433},
    {0x1f, 62533},
    {0x20, 65535},
    {0x21, 68444},
    {0x22, 71266},
    {0x23, 74007},
    {0x24, 76671},
    {0x25, 79261},
    {0x26, 81782},
    {0x27, 84238},
    {0x28, 86632},
    {0x29, 88967},
    {0x2a, 91245},
    {0x2b, 93470},
    {0x2c, 95643},
    {0x2d, 97768},
    {0x2e, 99846},
    {0x2f, 101879},
    {0x30, 103870},
    {0x31, 105820},
    {0x32, 107730},
    {0x33, 109602},
    {0x34, 111438},
    {0x35, 113239},
    {0x36, 115006},
    {0x37, 116741},
    {0x38, 118445},
    {0x39, 120118},
    {0x3a, 121762},
    {0x3b, 123379},
    {0x3c, 124968},
    {0x3d, 126530},
    {0x3e, 128068},
    {0x3f, 129581},
    {0x40, 131070},
    {0x41, 132535},
    {0x42, 133979},
    {0x43, 135401},
    {0x44, 136801},
    {0x45, 138182},
    {0x46, 139542},
    {0x47, 140883},
    {0x48, 142206},
    {0x49, 143510},
    {0x4a, 144796},
    {0x4b, 146065},
    {0x4c, 147317},
    {0x4d, 148553},
    {0x4e, 149773},
    {0x4f, 150978},
    {0x50, 152167},
    {0x51, 153342},
    {0x52, 154502},
    {0x53, 155648},
    {0x54, 156780},
    {0x55, 157899},
    {0x56, 159005},
    {0x57, 160098},
    {0x58, 161178},
    {0x59, 162247},
    {0x5a, 163303},
    {0x5b, 164348},
    {0x5c, 165381},
    {0x5d, 166403},
    {0x5e, 167414},
    {0x5f, 168415},
    {0x60, 169405},
    {0x61, 170385},
    {0x62, 171355},
    {0x63, 172314},
    {0x64, 173265},
    {0x65, 174205},
    {0x66, 175137},
    {0x67, 176059},
    {0x68, 176973},
    {0x69, 177878},
    {0x6a, 178774},
    {0x6b, 179662},
    {0x6c, 180541},
    {0x6d, 181412},
    {0x6e, 182276},
    {0x6f, 183132},
    {0x70, 183980},
    {0x71, 184820},
    {0x72, 185653},
    {0x73, 186479},
    {0x74, 187297},
    {0x75, 188109},
    {0x76, 188914},
    {0x77, 189711},
    {0x78, 190503},
    {0x79, 191287},
    {0x7a, 192065},
    {0x7b, 192837},
    {0x7c, 193603},
    {0x7d, 194362},
    {0x7e, 195116},
    {0x7f, 195863},
    {0x80, 196605},
    {0x81, 197340},
    {0x82, 198070},
    {0x83, 198795},
    {0x84, 199514},
    {0x85, 200227},
    {0x86, 200936},
    {0x87, 201639},
    {0x88, 202336},
    {0x89, 203029},
    {0x8a, 203717},
    {0x8b, 204399},
    {0x8c, 205077},
    {0x8d, 205750},
    {0x8e, 206418},
    {0x8f, 207082},
    {0x90, 207741},
    {0x91, 208395},
    {0x92, 209045},
    {0x93, 209690},
    {0x94, 210331},
    {0x95, 210968},
    {0x96, 211600},
    {0x97, 212228},
    {0x98, 212852},
    {0x99, 213472},
    {0x9a, 214088},
    {0x9b, 214700},
    {0x9c, 215308},
    {0x9d, 215912},
    {0x9e, 216513},
    {0x9f, 217109},
    {0xa0, 217702},
    {0xa1, 218291},
    {0xa2, 218877},
    {0xa3, 219458},
    {0xa4, 220037},
    {0xa5, 220611},
    {0xa6, 221183},
    {0xa7, 221751},
    {0xa8, 222315},
    {0xa9, 222876},
    {0xaa, 223434},
    {0xab, 223988},
    {0xac, 224540},
    {0xad, 225088},
    {0xae, 225633},
    {0xaf, 226175},
    {0xb0, 226713},
    {0xb1, 227249},
    {0xb2, 227782},
    {0xb3, 228311},
    {0xb4, 228838},
    {0xb5, 229362},
    {0xb6, 229883},
    {0xb7, 230401},
    {0xb8, 230916},
    {0xb9, 231429},
    {0xba, 231938},
    {0xbb, 232445},
    {0xbc, 232949},
    {0xbd, 233451},
    {0xbe, 233950},
    {0xbf, 234446},
    {0xc0, 234940},
    {0xc1, 235431},
    {0xc2, 235920},
    {0xc3, 236406},
    {0xc4, 236890},
    {0xc5, 237371},
    {0xc6, 237849},
    {0xc7, 238326},
    {0xc8, 238800},
    {0xc9, 239271},
    {0xca, 239740},
    {0xcb, 240207},
    {0xcc, 240672},
    {0xcd, 241134},
    {0xce, 241594},
    {0xcf, 242052},
    {0xd0, 242508},
    {0xd1, 242961},
    {0xd2, 243413},
    {0xd3, 243862},
    {0xd4, 244309},
    {0xd5, 244754},
    {0xd6, 245197},
    {0xd7, 245637},
    {0xd8, 246076},
    {0xd9, 246513},
    {0xda, 246947},
    {0xdb, 247380},
    {0xdc, 247811},
    {0xdd, 248240},
    {0xde, 248667},
    {0xdf, 249091},
    {0xe0, 249515},
    {0xe1, 249936},
    {0xe2, 250355},
    {0xe3, 250772},
    {0xe4, 251188},
    {0xe5, 251602},
    {0xe6, 252014},
    {0xe7, 252424},
    {0xe8, 252832},
    {0xe9, 253239},
    {0xea, 253644},
    {0xeb, 254047},
    {0xec, 254449},
    {0xed, 254848},
    {0xee, 255246},
    {0xef, 255643},
    {0xf0, 256038},
    {0xf1, 256431},
    {0xf2, 256822},
    {0xf3, 257212},
    {0xf4, 257600},
    {0xf5, 257987},
    {0xf6, 258372},
    {0xf7, 258756},
    {0xf8, 259138},
};

static unsigned int ov9281_alloc_again(unsigned int isp_gain, unsigned char shift, unsigned int *sensor_again)
{
    struct again_lut *lut = ov9281_again_lut;

    while(lut->gain <= ov9281_attr.max_again) {
        if(isp_gain == 0) {
            *sensor_again = lut->value;
            return 0;
        }

        else if(isp_gain < lut->gain) {
            *sensor_again = (lut - 1)->value;
            return (lut - 1)->gain;
        }

        else{
            if((lut->gain == ov9281_attr.max_again) && (isp_gain >= lut->gain)) {
                *sensor_again = lut->value;
                return lut->gain;
            }

        }

        lut++;
    }

    return isp_gain;
}

static unsigned int ov9281_alloc_dgain(unsigned int isp_gain, unsigned char shift, unsigned int *sensor_dgain)
{
    return isp_gain;
}


static struct tx_isp_sensor_attribute ov9281_attr = {
    .name                           = "ov9281",
    .chip_id                        = 0x9281,
    .cbus_type                      = TX_SENSOR_CONTROL_INTERFACE_I2C,
    .cbus_mask                      = V4L2_SBUS_MASK_SAMPLE_8BITS | V4L2_SBUS_MASK_ADDR_16BITS,
    .cbus_device                    = 0x60,
    .dbus_type                      = TX_SENSOR_DATA_INTERFACE_MIPI,
    .mipi = {
        .clk = 800,
        .lans = 2,
    },
    .max_again                      = 259138,
    .max_dgain                      = 0,
    .min_integration_time           = 2,
    .min_integration_time_native    = 2,
    .max_integration_time_native    = OV9281_VTS - 25,
    .one_line_expr_in_us            = 8,
    .integration_time_limit         = OV9281_VTS - 25,
    .total_width                    = OV9281_HTS,
    .total_height                   = OV9281_VTS,
    .max_integration_time           = OV9281_VTS - 25,
    .integration_time_apply_delay   = 2,
    .again_apply_delay              = 2,
    .dgain_apply_delay              = 0,
    .sensor_ctrl.alloc_again        = ov9281_alloc_again,
    .sensor_ctrl.alloc_dgain        = ov9281_alloc_dgain,
    //void priv; /* point to struct tx_isp_sensor_board_info */
};

#if defined CONFIG_OV9281_MIPI_1280X800

/*
 * Interface    : MIPI
 * MCLK         : 24Mhz,
 * MIPI_clcok   : 800MHz(PLL1 multiplier)
 * resolution   : 1280*800
 * FrameRate    : 60fps
 * SYS_CLK      : 80MHz(PLL2_sys_clk),
 * HTS          : 728
 * VTS          : 1830
 */
static struct regval_list ov9281_init_regs_1280_800_60fps_mipi[] = {
    /* software reset */
    {0x0103,0x01},

    /* PLL Control */
    {0x0302,0x32},
    {0x030d,0x50},
    {0x030e,0x02},

    /* system control */
    {0x3001,0x00},
    {0x3004,0x00},
    {0x3005,0x00},
    {0x3006,0x04},
    {0x3011,0x0a},
    {0x3013,0x18},
    {0x301c,0xf0},
    {0x3022,0x01},
    {0x3030,0x10},
    {0x3039,0x32},
    {0x303a,0x00},

    /* manual exposure */
    {0x3500,0x00},
    {0x3501,0x38},
    {0x3502,0x20},
    {0x3503,0x08},
    {0x3505,0x8c},
    {0x3507,0x03},
    {0x3508,0x00},
    {0x3509,0x10},

    /* analog control */
    {0x3610,0x80},
    {0x3611,0xa0},
    {0x3620,0x6e},
    {0x3632,0x56},
    {0x3633,0x78},
    {0x3662,0x05},
    {0x3666,0x00},
    {0x366f,0x5a},
    {0x3680,0x84},

    /* sensor control */
    {0x3712,0x80},
    {0x372d,0x22},
    {0x3731,0x80},
    {0x3732,0x30},
    {0x3778,0x00},
    {0x377d,0x22},
    {0x3788,0x02},
    {0x3789,0xa4},
    {0x378a,0x00},
    {0x378b,0x4a},
    {0x3799,0x20},

    /* timing control */
    {0x3800,0x00},
    {0x3801,0x00},
    {0x3802,0x00},
    {0x3803,0x00},
    {0x3804,0x05},
    {0x3805,0x0f},
    {0x3806,0x03},
    {0x3807,0x2f},
    {0x3808,0x05},
    {0x3809,0x00},
    {0x380a,0x03},
    {0x380b,0x20},
    {0x380c,0x02},      //HTS_H : 728
    {0x380d,0xd8},      //HTS_L
    {0x380e,0x07},      //VTS_H : 1830
    {0x380f,0x26},      //VTS_L
    {0x3810,0x00},
    {0x3811,0x08},
    {0x3812,0x00},
    {0x3813,0x08},
    {0x3814,0x11},
    {0x3815,0x11},
    {0x3820,0x40},
    {0x3821,0x00},
    {0x382c,0x05},
    {0x382d,0xb0},

    /* Global shutter control */
    {0x3881,0x42},
    {0x3882,0x01},
    {0x3883,0x00},
    {0x3885,0x02},
    {0x389d,0x00},
    {0x38a8,0x02},
    {0x38a9,0x80},
    {0x38b1,0x00},
    {0x38b3,0x02},
    {0x38c4,0x00},
    {0x38c5,0xc0},
    {0x38c6,0x04},
    {0x38c7,0x80},

    /* PWM and strobe control */
    {0x3920,0xff},

    /* BLC control */
    {0x4003,0x40},
    {0x4008,0x04},
    {0x4009,0x0b},
    {0x400c,0x00},
    {0x400d,0x07},
    {0x4010,0x40},
    {0x4043,0x40},

    /* Format control */
    {0x4307,0x30},
    {0x4317,0x00},

    /* Read out control */
    {0x4501,0x00},
    {0x4507,0x00},
    {0x4509,0x00},
    {0x450a,0x08},

    /* VFIFO control */
    {0x4601,0x04},

    /* DVP control */
    {0x470f,0x00},


    /* ISP Top control */
    {0x5000,0x87},
    {0x5001,0x00},
    {0x5e00,0x00},
    {0x5d00,0x07},
    {0x5d01,0x00},

    /* low power mode control */
    {0x4f00,0x04},
    {0x4f07,0x00},
    {0x4f10,0x00},
    {0x4f11,0x98},
    {0x4f12,0x0f},
    {0x4f13,0xc4},


    /* MIPI Top control */
    /* mipi LPX timing:      0x0012 + Tui * 0xFF = 190ns */
    {0x4800,0x01},      /* T-lpx select        bit[0]=0:auto calculate   =1:Use lpx_p_min */
    {0x4824,0x00},
    {0x4825,0x12},      /* lpx p min */
    {0x4830,0xff},      /* lpx_P min UI */

    {0x4802,0x84},      /* T-hs_prepare_sel     bit[7]=0:auto calculate  =1:Use hs_prepare_min */
                        /* T-hs_zero    sel     bit[2]=0:auto calculate  =1:Use hs_zero_min */

    /* mipi HS-prepare timing:  0x05 = 27ns */
    {0x4826,0x05},      /* hs_prepare min:ns */
    {0x4827,0xf5},      /* hs_prepare max:ns */
    {0x4831,0x00},      /* ui_hs_prepare_max[7:4] min[3:0]  权重低作用不大*/

    /* mipi HS-zero timing:  0x0018 + Tui * 0xFF  = 240ns */
    {0x4818,0x00},
    {0x4819,0x18},
    {0x482a,0xff},

    {OV9281_REG_END, 0x0},
};
	
#elif defined CONFIG_OV9281_MIPI_1280X720

/*
 * Interface    : MIPI
 * MCLK         : 24Mhz,
 * MIPI_clcok   : 800MHz(PLL1 multiplier)
 * resolution   : 1280*720
 * FrameRate    : 120fps
 * SYS_CLK      : 80MHz(PLL2_sys_clk),
 * HTS          : 728
 * VTS          : 910
 */
static struct regval_list ov9281_init_regs_1280_720_120fps_mipi[] = {
    /* software reset */
    {0x0103,0x01},

    /* PLL Control */
    //{0x0300,0x02},//pll 1 div
    {0x0302,0x32},//loop 倍频
    //{0x030b,0x05},//PLL2 div
    {0x030d,0x50},//PLL2 loop 倍频
    {0x030e,0x02},

    /* system control */
    {0x3001,0x00},
    {0x3004,0x00},
    {0x3005,0x00},
    {0x3006,0x04},
    {0x3011,0x0a},
    {0x3013,0x18},
    {0x301c,0xf0},
    {0x3022,0x01},
    {0x3030,0x10},
    {0x3039,0x32},
    {0x303a,0x00},

    /* manual exposure */
    {0x3500,0x00},
    {0x3501,0x38},
    {0x3502,0x20},
    {0x3503,0x08},
    {0x3505,0x8c},
    {0x3507,0x03},
    {0x3508,0x00},
    {0x3509,0x10},

    /* analog control */
    {0x3610,0x80},
    {0x3611,0xa0},
    {0x3620,0x6e},
    {0x3632,0x56},
    {0x3633,0x78},
    {0x3662,0x05},
    {0x3666,0x00},
    {0x366f,0x5a},
    {0x3680,0x84},

    /* sensor control */
    {0x3712,0x80},
    {0x372d,0x22},
    {0x3731,0x80},
    {0x3732,0x30},
    {0x3778,0x00},
    {0x377d,0x22},
    {0x3788,0x02},
    {0x3789,0xa4},
    {0x378a,0x00},
    {0x378b,0x4a},
    {0x3799,0x20},

    /* timing control */
    {0x3800,0x00},
    {0x3801,0x00},
    {0x3802,0x00},
    {0x3803,0x00},
    {0x3804,0x05},
    {0x3805,0x0f},
    {0x3806,0x03},
    {0x3807,0x2f},
    {0x3808,0x05},
    {0x3809,0x00},
    {0x380a,0x02},
    {0x380b,0xd0},
    {0x380c,0x02},      //HTS_H : 728
    {0x380d,0xd8},      //HTS_L
    {0x380e,0x03},      //VTS_H : 910
    {0x380f,0x8e},      //VTS_L
    {0x3810,0x00},
    {0x3811,0x08},
    {0x3812,0x00},
    {0x3813,0x08},
    {0x3814,0x11},
    {0x3815,0x11},
    {0x3820,0x40},
    {0x3821,0x00},
    {0x382c,0x05},
    {0x382d,0xb0},

    /* Global shutter control */
    {0x3881,0x42},
    {0x3882,0x01},
    {0x3883,0x00},
    {0x3885,0x02},
    {0x389d,0x00},
    {0x38a8,0x02},
    {0x38a9,0x80},
    {0x38b1,0x00},
    {0x38b3,0x02},
    {0x38c4,0x00},
    {0x38c5,0xc0},
    {0x38c6,0x04},
    {0x38c7,0x80},

    /* PWM and strobe control */
    {0x3920,0xff},

    /* BLC control */
    {0x4003,0x40},
    {0x4008,0x04},
    {0x4009,0x0b},
    {0x400c,0x00},
    {0x400d,0x07},
    {0x4010,0x40},
    {0x4043,0x40},

    /* Format control */
    {0x4307,0x30},
    {0x4317,0x00},

    /* Read out control */
    {0x4501,0x00},
    {0x4507,0x00},
    {0x4509,0x00},
    {0x450a,0x08},

    /* VFIFO control */
    {0x4601,0x04},

    /* DVP control */
    {0x470f,0x00},

    /* ISP Top control */
    {0x5000,0x9f},
    {0x5001,0x00},
    {0x5e00,0x00},
    {0x5d00,0x07},
    {0x5d01,0x00},

    /* low power mode control */
    {0x4f00,0x04},
    {0x4f07,0x00},
    {0x4f10,0x00},
    {0x4f11,0x98},
    {0x4f12,0x0f},
    {0x4f13,0xc4},

    {0x5000,0x87},

    /* MIPI Top control */
    /* mipi LPX timing:      0x0012 + Tui * 0xFF = 190ns */
    {0x4800,0x01},      /* T-lpx select        bit[0]=0:auto calculate   =1:Use lpx_p_min */
    {0x4824,0x00},
    {0x4825,0x12},      /* lpx p min */
    {0x4830,0xff},      /* lpx_P min UI */

    {0x4802,0x84},      /* T-hs_prepare_sel     bit[7]=0:auto calculate  =1:Use hs_prepare_min */
                        /* T-hs_zero    sel     bit[2]=0:auto calculate  =1:Use hs_zero_min */

    /* mipi HS-prepare timing:  0x05 = 27ns */
    {0x4826,0x05},      /* hs_prepare min:ns */
    {0x4827,0xf5},      /* hs_prepare max:ns */
    {0x4831,0x00},      /* ui_hs_prepare_max[7:4] min[3:0]  权重低作用不大*/

    /* mipi HS-zero timing:  0x0018 + Tui * 0xFF  = 240ns */
    {0x4818,0x00},
    {0x4819,0x18},
    {0x482a,0xff},

    {OV9281_REG_END, 0x0}, /* END MARKER */
};
	
#elif defined CONFIG_OV9281_MIPI_640X480

/*
 * Interface    : MIPI
 * MCLK         : 24Mhz,
 * MIPI_clcok   : 800MHz(PLL1 multiplier)
 * resolution   : 640*480
 * FrameRate    : 120fps
 * SYS_CLK      : 80MHz(PLL2_sys_clk),
 * HTS          : 728
 * VTS          : 915
 */
static struct regval_list ov9281_init_regs_640_480_120fps_mipi[] = {
    /* software reset */
    {0x0103,0x01},

    /* PLL Control */
    //{0x0300,0x02},//pll 1 div
    {0x0302,0x32},//loop 倍频
    //{0x030b,0x05},//PLL2 div
    {0x030d,0x50},//PLL2 loop 倍频
    {0x030e,0x02},

    /* system control */
    {0x3001,0x00},
    {0x3004,0x00},
    {0x3005,0x00},
    {0x3006,0x04},
    {0x3011,0x0a},
    {0x3013,0x18},
    {0x301c,0xf0},
    {0x3022,0x01},
    {0x3030,0x10},
    {0x3039,0x32},
    {0x303a,0x00},

    /* manual exposure */
    {0x3500,0x00},
    {0x3501,0x38},
    {0x3502,0x20},
    {0x3503,0x08},
    {0x3505,0x8c},
    {0x3507,0x03},
    {0x3508,0x00},
    {0x3509,0x10},

    /* analog control */
    {0x3610,0x80},
    {0x3611,0xa0},
    {0x3620,0x6e},
    {0x3632,0x56},
    {0x3633,0x78},
    {0x3662,0x05},
    {0x3666,0x00},
    {0x366f,0x5a},
    {0x3680,0x84},

    /* sensor control */
    {0x3712,0x80},
    {0x372d,0x22},
    {0x3731,0x80},
    {0x3732,0x30},
    {0x3778,0x00},
    {0x377d,0x22},
    {0x3788,0x02},
    {0x3789,0xa4},
    {0x378a,0x00},
    {0x378b,0x4a},
    {0x3799,0x20},

    /* timing control */
    {0x3800,0x00},
    {0x3801,0x00},
    {0x3802,0x00},
    {0x3803,0x00},
    {0x3804,0x05},
    {0x3805,0x0f},
    {0x3806,0x03},
    {0x3807,0x2f},
    {0x3808,0x02},
    {0x3809,0x80},
    {0x380a,0x01},
    {0x380b,0xe0},
    {0x380c,0x02},      //HTS_H : 728
    {0x380d,0xd8},      //HTS_L
    {0x380e,0x03},      //VTS_H : 915
    {0x380f,0x93},      //VTS_L
    {0x3810,0x01},
    {0x3811,0x48},
    {0x3812,0x00},
    {0x3813,0xa8},
    {0x3814,0x11},
    {0x3815,0x11},
    {0x3820,0x40},
    {0x3821,0x00},
    {0x382c,0x05},
    {0x382d,0xb0},

    /* Global shutter control */
    {0x3881,0x42},
    {0x3882,0x01},
    {0x3883,0x00},
    {0x3885,0x02},
    {0x389d,0x00},
    {0x38a8,0x02},
    {0x38a9,0x80},
    {0x38b1,0x00},
    {0x38b3,0x02},
    {0x38c4,0x00},
    {0x38c5,0xc0},
    {0x38c6,0x04},
    {0x38c7,0x80},

    /* PWM and strobe control */
    {0x3920,0xff},

    /* BLC control */
    {0x4003,0x40},
    {0x4008,0x04},
    {0x4009,0x0b},
    {0x400c,0x00},
    {0x400d,0x07},
    {0x4010,0x40},
    {0x4043,0x40},

    /* Format control */
    {0x4307,0x30},
    {0x4317,0x00},

    /* Read out control */
    {0x4501,0x00},
    {0x4507,0x00},
    {0x4509,0x00},
    {0x450a,0x08},

    /* VFIFO control */
    {0x4601,0x04},

    /* DVP control */
    {0x470f,0x00},

    /* ISP Top control */
    {0x5000,0x87},
    {0x5001,0x00},
    {0x5e00,0x00},
    {0x5d00,0x07},
    {0x5d01,0x00},

    /* low power mode control */
    {0x4f00,0x04},
    {0x4f07,0x00},
    {0x4f10,0x00},
    {0x4f11,0x98},
    {0x4f12,0x0f},
    {0x4f13,0xc4},

    /* MIPI Top control */
    {0x4800,0x00},

    /* mipi LPX timing:      0x0012 + Tui * 0xFF = 190ns */
    {0x4800,0x01},      /* T-lpx select        bit[0]=0:auto calculate   =1:Use lpx_p_min */
    {0x4824,0x00},
    {0x4825,0x12},      /* lpx p min */
    {0x4830,0xff},      /* lpx_P min UI */

    {0x4802,0x84},      /* T-hs_prepare_sel     bit[7]=0:auto calculate  =1:Use hs_prepare_min */
                        /* T-hs_zero    sel     bit[2]=0:auto calculate  =1:Use hs_zero_min */

    /* mipi HS-prepare timing:  0x05 = 27ns */
    {0x4826,0x05},      /* hs_prepare min:ns */
    {0x4827,0xf5},      /* hs_prepare max:ns */
    {0x4831,0x00},      /* ui_hs_prepare_max[7:4] min[3:0]  权重低作用不大*/

    /* mipi HS-zero timing:  0x0018 + Tui * 0xFF  = 240ns */
    {0x4818,0x00},
    {0x4819,0x18},
    {0x482a,0xff},

    {OV9281_REG_END, 0x0}, /* END MARKER */
};

#elif defined CONFIG_OV9281_MIPI_640X400

/*
 * Interface    : MIPI
 * MCLK         : 24Mhz,
 * MIPI_clcok   : 640MHz(PLL1 multiplier)
 * resolution   : 640*400
 * FrameRate    : 210fps
 * SYS_CLK      : 80MHz(PLL2_sys_clk),
 * HTS          : 728
 * VTS          : 520
 */
static struct regval_list ov9281_init_regs_640_400_210fps_mipi[] = {
    /* software reset */
    {0x0103,0x01},

    /* PLL Control */
    {0x0302,0x28},
    {0x030d,0x50},
    {0x030e,0x02},

    /* system control */
    {0x3001,0x00},
    {0x3004,0x00},
    {0x3005,0x00},
    {0x3006,0x04},
    {0x3011,0x0a},
    {0x3013,0x18},
    {0x301c,0xf0},
    {0x3022,0x01},
    {0x3030,0x10},
    {0x3039,0x32},
    {0x303a,0x00},

    /* manual exposure */
    {0x3500,0x00},
    {0x3501,0x20},
    {0x3502,0x20},
    {0x3503,0x08},
    {0x3505,0x8c},
    {0x3507,0x03},      /* <<3 gain */
    {0x3508,0x00},
    {0x3509,0x10},

    /* analog control */
    {0x3610,0x80},
    {0x3611,0xa0},
    {0x3620,0x6e},
    {0x3632,0x56},
    {0x3633,0x78},
    {0x3662,0x05},
    {0x3666,0x00},
    {0x366f,0x5a},
    {0x3680,0x84},

    /* sensor control */
    {0x3712,0x80},
    {0x372d,0x22},
    {0x3731,0x80},
    {0x3732,0x30},
    {0x3778,0x10},
    {0x377d,0x22},
    {0x3788,0x02},
    {0x3789,0xa4},
    {0x378a,0x00},
    {0x378b,0x4a},
    {0x3799,0x20},

    /* timing control */
    {0x3800,0x00},
    {0x3801,0x00},
    {0x3802,0x00},
    {0x3803,0x00},
    {0x3804,0x05},
    {0x3805,0x0f},
    {0x3806,0x03},
    {0x3807,0x2f},
    {0x3808,0x02},
    {0x3809,0x80},
    {0x380a,0x01},
    {0x380b,0x90},
    {0x380c,0x02},      //HTS_H : 728
    {0x380d,0xd8},      //HTS_L
    {0x380e,0x02},      //VTS_H : 520
    {0x380f,0x08},      //VTS_L
    {0x3810,0x00},
    {0x3811,0x04},
    {0x3812,0x00},
    {0x3813,0x04},
    {0x3814,0x31},
    {0x3815,0x22},
    {0x3820,0x60},
    {0x3821,0x01},
    {0x382c,0x05},
    {0x382d,0xb0},

    /* Global shutter control */
    {0x3881,0x42},
    {0x3882,0x01},
    {0x3883,0x00},
    {0x3885,0x02},
    {0x38a8,0x02},
    {0x389d,0x00},
    {0x38a9,0x80},
    {0x38b1,0x00},
    {0x38b3,0x02},
    {0x38c4,0x00},
    {0x38c5,0xc0},
    {0x38c6,0x04},
    {0x38c7,0x80},

    /* PWM and strobe control */
    {0x3920,0xff},

    /* BLC control */
    {0x4003,0x40},
    {0x4008,0x02},
    {0x4009,0x05},
    {0x400c,0x00},
    {0x400d,0x03},
    {0x4010,0x40},
    {0x4043,0x40},

    /* Format control */
    {0x4307,0x30},
    {0x4317,0x00},

    /* Read out control */
    {0x4501,0x00},
    {0x4507,0x03},
    {0x4509,0x80},
    {0x450a,0x08},

    /* VFIFO control */
    {0x4601,0x04},

    /* DVP control */
    {0x470f,0x00},
    {0x4f07,0x00},


    /* ISP Top control */
    {0x5000,0x9f},
    {0x5001,0x00},
    {0x5e00,0x00},
    {0x5d00,0x07},
    {0x5d01,0x00},

    /* low power mode control */
    {0x4f00,0x04},
    {0x4f10,0x00},
    {0x4f11,0x98},
    {0x4f12,0x0f},
    {0x4f13,0xc4},

    /* MIPI Top control */
    {0x4800,0x00},
    /* mipi LPX timing: time 未测量 */
    {0x4800,0x01},      /* T-lpx select        bit[0]=0:auto calculate   =1:Use lpx_p_min */
    {0x4824,0x00},
    {0x4825,0x20},      /* lpx p min */
    {0x4830,0xff},      /* lpx_P min UI */

    {0x4802,0x84},      /* T-hs_prepare_sel     bit[7]=0:auto calculate  =1:Use hs_prepare_min */
                        /* T-hs_zero    sel     bit[2]=0:auto calculate  =1:Use hs_zero_min */

    /* mipi HS-prepare timing:time 未测量 */
    {0x4826,0x05},      /* hs_prepare min:ns */
    {0x4827,0xf5},      /* hs_prepare max:ns */
    {0x4831,0x00},      /* ui_hs_prepare_max[7:4] min[3:0]  权重低作用不大*/

    /* mipi HS-zero timing: time 未测量 */
    {0x4818,0x00},
    {0x4819,0x18},
    {0x482a,0xff},

    {OV9281_REG_END, 0x0}, /* END MARKER */
};
	
#endif

static struct tx_isp_sensor_win_setting ov9281_win_sizes[] = {
    {
        .width      = OV9281_WIDTH,
        .height     = OV9281_HEIGHT,
        .fps        = SENSOR_OUTPUT_MAX_FPS << 16 | 1,
        .mbus_code  = V4L2_MBUS_FMT_SGRBG10_1X10,
        .colorspace = V4L2_COLORSPACE_SRGB,
#if defined CONFIG_OV9281_MIPI_1280X800
		.regs       = ov9281_init_regs_1280_800_60fps_mipi,
#elif defined CONFIG_OV9281_MIPI_1280X720
		.regs		= ov9281_init_regs_1280_720_120fps_mipi,
#elif defined CONFIG_OV9281_MIPI_640X480
		.regs		= ov9281_init_regs_640_480_120fps_mipi,
#elif defined CONFIG_OV9281_MIPI_640X400
		.regs		= ov9281_init_regs_640_400_210fps_mipi,
#endif
    },
};

/*
 * the part of driver was fixed.
 */

static struct regval_list ov9281_stream_on_mipi[] = {
    {0x0100,0x01},
    {OV9281_REG_END, 0x0}, /* END MARKER */
};

static struct regval_list ov9281_stream_off_mipi[] = {
    {0x0100,0x00},
    {OV9281_REG_END, 0x0}, /* END MARKER */
};

int ov9281_read(struct v4l2_subdev *sd, u16 reg, u8 *val)
{
    int ret;
    u8 buf[2] = {reg >> 8, reg & 0xff};

    struct i2c_client *client = v4l2_get_subdevdata(sd);
    struct i2c_msg msg[2] = {
        [0] = {
            .addr   = client->addr,
            .flags  = 0,
            .len    = 2,
            .buf    = buf,
        },
        [1] = {
            .addr   = client->addr,
            .flags  = I2C_M_RD,
            .len    = 1,
            .buf    = val,
        }
    };

    ret = i2c_transfer(client->adapter, msg, 2);
    if (ret > 0)
        ret = 0;

    return ret;
}

int ov9281_write(struct v4l2_subdev *sd, u16 reg, u8 val)
{
    struct i2c_client *client = v4l2_get_subdevdata(sd);

    u8 buf[3] = {reg >> 8, reg & 0xff, val};
    struct i2c_msg msg = {
        .addr   = client->addr,
        .flags  = 0,
        .len    = 3,
        .buf    = buf,
    };

    int ret;
    ret = i2c_transfer(client->adapter, &msg, 1);
    if (ret > 0)
        ret = 0;

    return ret;
}

static int ov9281_read_array(struct v4l2_subdev *sd, struct regval_list *vals)
{
    int ret;
    u8 val;
    while (vals->reg_num != OV9281_REG_END) {
       if (vals->reg_num == OV9281_REG_DELAY) {
            msleep(vals->value);
        } else {
            ret = ov9281_read(sd, vals->reg_num, &val);
            if (ret < 0)
                return ret;
        }
        vals++;
    }

    return 0;
}

static int ov9281_write_array(struct v4l2_subdev *sd, struct regval_list *vals)
{
    int ret;

    while (vals->reg_num != OV9281_REG_END) {
        if (vals->reg_num == OV9281_REG_DELAY) {
                msleep(vals->value);
        } else {
            ret = ov9281_write(sd, vals->reg_num, vals->value);
            if (ret < 0)
                return ret;
        }
        //printk("vals->reg_num:%x, vals->value:%x\n",vals->reg_num, vals->value);
        vals++;
    }
    return 0;
}


static int ov9281_reset(struct v4l2_subdev *sd, int val)
{
    return 0;
}

static int ov9281_detect(struct v4l2_subdev *sd, unsigned int *ident)
{
    unsigned char v;
    int ret;
    ret = ov9281_read(sd, 0x300A, &v);
    pr_debug("-----%s: %d ret = %d, v = 0x%02x\n", __func__, __LINE__, ret,v);
    if (ret < 0)
        return ret;

    if (v != OV9281_CHIP_ID_H)
        return -ENODEV;

    *ident = v;
    ret = ov9281_read(sd, 0x300B, &v);
    pr_debug("-----%s: %d ret = %d, v = 0x%02x\n", __func__, __LINE__, ret,v);
    if (ret < 0)
        return ret;

    if (v != OV9281_CHIP_ID_L)
        return -ENODEV;

    *ident = (*ident << 8) | v;

    return 0;
}

static int ov9281_set_integration_time(struct v4l2_subdev *sd, int value)
{
    int ret = 0;
    unsigned int expo = value;
    unsigned char tmp;
    unsigned short vts;
    unsigned int max_expo;

    ret = ov9281_read(sd, 0x380e, &tmp);
    vts = tmp << 8;
    ret += ov9281_read(sd, 0x380f, &tmp);
    if(ret < 0)
        return ret;

    vts |= tmp;
    max_expo = vts - 25;
    if (expo > max_expo)
        expo = max_expo;

    //0x3500, [19:16]
    //0x3501, [15:8]
    //0x3502, [7:0], low 4 bits are fraction bits
    ret  = ov9281_write(sd, 0x3500, (unsigned char)((expo & 0xffff) >> 12));
    ret += ov9281_write(sd, 0x3501, (unsigned char)((expo & 0x0fff) >> 4));
    ret += ov9281_write(sd, 0x3502, (unsigned char)((expo & 0x000f) << 4));

    if (ret < 0) {
        printk("ov9281_write error  %d\n" ,__LINE__ );
        return ret;
    }

    return 0;
}

/**************************************
  ov9281 sensor gain control by [0x3509]
    value from 0x10 to 0xF8 represents gain of 1x to 15.5x
**************************************/
static int ov9281_set_analog_gain(struct v4l2_subdev *sd, u16 gain)
{
    int ret = 0;
    //printk("%d %s\n", __LINE__, __func__);
    ret = ov9281_write(sd, 0x3509, gain);
    if (ret < 0) {
        printk("ov9281_write error  %d" ,__LINE__ );
        return ret;
    }

    return 0;
}

static int ov9281_set_digital_gain(struct v4l2_subdev *sd, int value)
{
    return 0;
}

static int ov9281_get_black_pedestal(struct v4l2_subdev *sd, int value)
{
    return 0;
}

static int ov9281_init(struct v4l2_subdev *sd, int enable)
{
    struct tx_isp_sensor *sensor = (container_of(sd, struct tx_isp_sensor, sd));
    struct tx_isp_notify_argument arg;
    struct tx_isp_sensor_win_setting *wsize = &ov9281_win_sizes[0];
    int ret = 0;
    if(!enable)
        return ISP_SUCCESS;
    //printk("%d %s\n", __LINE__, __func__);

    sensor->video.mbus.width      = wsize->width;
    sensor->video.mbus.height     = wsize->height;
    sensor->video.mbus.code       = wsize->mbus_code;
    sensor->video.mbus.field      = V4L2_FIELD_NONE;
    sensor->video.mbus.colorspace = wsize->colorspace;
    sensor->video.fps             = wsize->fps;

    ret = ov9281_write_array(sd, wsize->regs);

    if (ret)
        return ret;
    arg.value = (int)&sensor->video;
    sd->v4l2_dev->notify(sd, TX_ISP_NOTIFY_SYNC_VIDEO_IN, &arg);
    sensor->priv = wsize;

    return 0;
}

static int ov9281_g_parm(struct v4l2_subdev *sd, struct v4l2_streamparm *parms)
{
    return 0;
}

static int ov9281_s_parm(struct v4l2_subdev *sd, struct v4l2_streamparm *parms)
{

    return 0;
}

static int ov9281_s_stream(struct v4l2_subdev *sd, int enable)
{
    int ret = 0;
    if (enable) {
        ret = ov9281_write_array(sd, ov9281_stream_on_mipi);
        pr_debug("ov9281 stream on\n");

    }
    else {
        ret = ov9281_write_array(sd, ov9281_stream_off_mipi);
        pr_debug("ov9281 stream off\n");
    }

    return ret;
}

static int ov9281_set_fps(struct tx_isp_sensor *sensor, int fps)
{
    struct v4l2_subdev *sd = &sensor->sd;
    struct tx_isp_notify_argument arg;
    unsigned int sclk = 0;
    unsigned short vts = 0;
    unsigned short hts=0;
    unsigned int max_fps = 0;
    unsigned char tmp;
    unsigned int newformat = 0; //the format is 24.8
    int ret = 0;

    sclk = OV9281_SCLK;
    max_fps = SENSOR_OUTPUT_MAX_FPS;

    /* the format of fps is 16/16. for example 25 << 16 | 2, the value is 25/2 fps. */
    newformat = (((fps >> 16) / (fps & 0xffff)) << 8) + ((((fps >> 16) % (fps & 0xffff)) << 8) / (fps & 0xffff));
    if(newformat > (max_fps << 8) || newformat < (SENSOR_OUTPUT_MIN_FPS << 8)){
        printk("warn: fps(%x) no in range\n", fps);
        return -1;
    }

    //HTS
    ret = ov9281_read(sd, 0x380c, &tmp);
    hts = tmp << 8;
    ret += ov9281_read(sd, 0x380d, &tmp);
    if(ret < 0)
        return -1;

    hts |= tmp;
    //VTS
    vts = sclk * (fps & 0xffff) / hts / ((fps & 0xffff0000) >> 16);

    ret = ov9281_write(sd, 0x380e, (unsigned char)(vts >> 8));
    ret += ov9281_write(sd, 0x380f, (unsigned char)(vts & 0xff));
    if(ret < 0)
        return -1;

    sensor->video.fps                               = fps;
    sensor->video.attr->max_integration_time_native = vts - 4;
    sensor->video.attr->integration_time_limit      = vts - 4;
    sensor->video.attr->total_height                = vts;
    sensor->video.attr->max_integration_time        = vts - 4;

    arg.value = (int)&sensor->video;
    sd->v4l2_dev->notify(sd, TX_ISP_NOTIFY_SYNC_VIDEO_IN, &arg);

    return ret;
}

static int ov9281_set_mode(struct tx_isp_sensor *sensor, int value)
{
    struct tx_isp_notify_argument arg;
    struct v4l2_subdev *sd = &sensor->sd;
    struct tx_isp_sensor_win_setting *wsize = NULL;
    int ret = ISP_SUCCESS;

    if(value == TX_ISP_SENSOR_FULL_RES_MAX_FPS){
        wsize = &ov9281_win_sizes[0];
    }else if(value == TX_ISP_SENSOR_PREVIEW_RES_MAX_FPS){
        wsize = &ov9281_win_sizes[0];
    }

    if(wsize){
        sensor->video.mbus.width      = wsize->width;
        sensor->video.mbus.height     = wsize->height;
        sensor->video.mbus.code       = wsize->mbus_code;
        sensor->video.mbus.field      = V4L2_FIELD_NONE;
        sensor->video.mbus.colorspace = wsize->colorspace;
        sensor->video.fps             = wsize->fps;

        arg.value = (int)&sensor->video;
        sd->v4l2_dev->notify(sd, TX_ISP_NOTIFY_SYNC_VIDEO_IN, &arg);
    }

    return ret;
}

static int ov9281_set_vflip(struct tx_isp_sensor *sensor, int enable)
{
    struct tx_isp_notify_argument arg;
    struct v4l2_subdev *sd = &sensor->sd;
    int ret = 0;
    u8 val = 0;

    ret = ov9281_read(sd, 0x3820, &val);
    if (enable){
        val |= (1<<2);
        //sensor->video.mbus.code = V4L2_MBUS_FMT_SBGGR10_1X10;
    } else {
        val &= (1<<2);
        //sensor->video.mbus.code = V4L2_MBUS_FMT_SGRBG10_1X10;
    }
    ret += ov9281_write(sd, 0x3820, val);
    arg.value = (int)&sensor->video;

    if(!ret)
        sd->v4l2_dev->notify(sd, TX_ISP_NOTIFY_SYNC_VIDEO_IN, &arg);

    return ret;
}

static int ov9281_g_chip_ident(struct v4l2_subdev *sd,
        struct v4l2_dbg_chip_ident *chip)
{
    struct i2c_client *client = v4l2_get_subdevdata(sd);
    unsigned int ident = 0;
    int ret = ISP_SUCCESS;

    if(reset_gpio != -1){
       ret = gpio_request(reset_gpio,"ov9281_reset");
        if (!ret) {
            gpio_direction_output(reset_gpio, 1);
            msleep(5);
            gpio_direction_output(reset_gpio, 0);
            msleep(30);
            gpio_direction_output(reset_gpio, 1);
            msleep(35);
        } else {
            printk("gpio requrest fail %d\n", reset_gpio);
        }
    }

    if(pwdn_gpio != -1){
        ret = gpio_request(pwdn_gpio,"ov9281_pwdn");
        if(!ret){
            gpio_direction_output(pwdn_gpio, 1);
            msleep(10);
            gpio_direction_output(pwdn_gpio, 0);
            msleep(50);
        }else{
            printk("gpio requrest fail %d\n",pwdn_gpio);
        }
    }

    ret = ov9281_detect(sd, &ident);
    if (ret) {
        printk("chip found @ 0x%x (%s) is not an ov9281 chip.\n",
                client->addr, client->adapter->name);
        return ret;
    }

    v4l_info(client, "ov9281 chip found @ 0x%02x (%s)\n",
         client->addr, client->adapter->name);

    return v4l2_chip_ident_i2c_client(client, chip, ident, 0);
}
static int ov9281_s_power(struct v4l2_subdev *sd, int on)
{
    return 0;
}

static int ov9281_ops_private_ioctl(struct tx_isp_sensor *sensor, struct isp_private_ioctl *ctrl)
{
    long ret = 0;

    struct v4l2_subdev *sd = &sensor->sd;
    //printk("%d %s %d\n", __LINE__, __func__, ctrl->cmd);
    switch(ctrl->cmd){
    case TX_ISP_PRIVATE_IOCTL_SENSOR_INT_TIME:
        ret = ov9281_set_integration_time(sd, ctrl->value);
        break;
    case TX_ISP_PRIVATE_IOCTL_SENSOR_AGAIN:
        ret = ov9281_set_analog_gain(sd, ctrl->value);
        break;
    case TX_ISP_PRIVATE_IOCTL_SENSOR_DGAIN:
        ret = ov9281_set_digital_gain(sd, ctrl->value);
        break;
    case TX_ISP_PRIVATE_IOCTL_SENSOR_BLACK_LEVEL:
        ret = ov9281_get_black_pedestal(sd, ctrl->value);
        break;
    case TX_ISP_PRIVATE_IOCTL_SENSOR_RESIZE:
        ret = ov9281_set_mode(sensor, ctrl->value);
        break;
    case TX_ISP_PRIVATE_IOCTL_SUBDEV_PREPARE_CHANGE:
        ret = ov9281_write_array(sd, ov9281_stream_off_mipi);
        break;
    case TX_ISP_PRIVATE_IOCTL_SUBDEV_FINISH_CHANGE:
        ret = ov9281_write_array(sd, ov9281_stream_on_mipi);
        break;
    case TX_ISP_PRIVATE_IOCTL_SENSOR_FPS:
        ret = ov9281_set_fps(sensor, ctrl->value);
        break;
    //case TX_ISP_PRIVATE_IOCTL_SENSOR_VFLIP:
        //ret = ov9281_set_vflip(sd, ctrl->value);
        //break;
    default:
        break;;
    }

    return 0;
}

static long ov9281_ops_ioctl(struct v4l2_subdev *sd, unsigned int cmd, void *arg)
{
    struct tx_isp_sensor *sensor =container_of(sd, struct tx_isp_sensor, sd);
    int ret;

    switch (cmd) {
    case VIDIOC_ISP_PRIVATE_IOCTL:
        ret = ov9281_ops_private_ioctl(sensor, arg);
        break;
    default:
        return -1;
        break;
    }

    return 0;
}

static int ov9281_g_register(struct v4l2_subdev *sd, struct v4l2_dbg_register *reg)
{
    struct i2c_client *client = v4l2_get_subdevdata(sd);
    unsigned char val = 0;
    int ret;

    if (!v4l2_chip_match_i2c_client(client, &reg->match))
        return -EINVAL;

    if (!capable(CAP_SYS_ADMIN))
        return -EPERM;

    ret = ov9281_read(sd, reg->reg & 0xffff, &val);
    reg->val = val;
    reg->size = 1;

    return ret;
}

static int ov9281_s_register(struct v4l2_subdev *sd, const struct v4l2_dbg_register *reg)
{
    struct i2c_client *client = v4l2_get_subdevdata(sd);


    if (!v4l2_chip_match_i2c_client(client, &reg->match))
        return -EINVAL;

    if (!capable(CAP_SYS_ADMIN))
        return -EPERM;

    ov9281_write(sd, reg->reg & 0xffff, reg->val & 0xff);

    return 0;
}

static struct v4l2_subdev_core_ops ov9281_core_ops = {
    .g_chip_ident   = ov9281_g_chip_ident,
    .reset          = ov9281_reset,
    .init           = ov9281_init,
    .s_power        = ov9281_s_power,
    .ioctl          = ov9281_ops_ioctl,
#ifdef CONFIG_VIDEO_ADV_DEBUG
    .g_register     = ov9281_g_register,
    .s_register     = ov9281_s_register,
#endif
};

static struct v4l2_subdev_video_ops ov9281_video_ops = {
    .s_stream       = ov9281_s_stream,
    .s_parm         = ov9281_s_parm,
    .g_parm         = ov9281_g_parm,
};

static struct v4l2_subdev_ops ov9281_ops = {
    .core  = &ov9281_core_ops,
    .video = &ov9281_video_ops,
};

/* It's the sensor device */
static u64 tx_isp_module_dma_mask = ~(u64)0;
struct platform_device sensor_platform_device = {
    .name          = "ov9281",
    .id            = -1,
    .dev = {
        .dma_mask = &tx_isp_module_dma_mask,
        .coherent_dma_mask = 0xffffffff,
        .platform_data = NULL,
    },
    .num_resources = 0,
};

static int ov9281_probe(struct i2c_client *client, const struct i2c_device_id *id)
{
    struct v4l2_subdev *sd;
    struct tx_isp_video_in *video;
    struct tx_isp_sensor *sensor;
    struct tx_isp_sensor_win_setting *wsize=&ov9281_win_sizes[0];
    int ret = 0;
    pr_debug("probe ok ----start--->ov9281\n");
    sensor = (struct tx_isp_sensor *)kzalloc(sizeof(*sensor), GFP_KERNEL);
    if(!sensor){
        printk("Failed to allocate sensor subdev.\n");
        return -ENOMEM;
    }

#ifndef MODULE
        ov9281_board_info = get_sensor_board_info(sensor_platform_device.name);
        if (ov9281_board_info) {
            pwdn_gpio       = ov9281_board_info->gpios.gpio_power;
            reset_gpio      = ov9281_board_info->gpios.gpio_sensor_rst;
            gpio_i2c_sel1   = ov9281_board_info->gpios.gpio_i2c_sel1;
            gpio_i2c_sel2   = ov9281_board_info->gpios.gpio_i2c_sel2;
        }
#endif
    ret = tx_isp_clk_set(CONFIG_ISP_CLK);
    if (ret < 0) {
        printk("Cannot set isp clock\n");
        goto err_get_mclk;
    }
    /* request mclk of sensor */
    sensor->mclk = clk_get(NULL, "cgu_cim");
    if (IS_ERR(sensor->mclk)) {
        printk("Cannot get sensor input clock cgu_cim\n");
        goto err_get_mclk;
    }

    clk_set_rate(sensor->mclk, 24000000);
    clk_enable(sensor->mclk);

    ov9281_attr.max_again = 259138;
    ov9281_attr.max_dgain = 0;
    sd = &sensor->sd;
    video = &sensor->video;
    sensor->video.attr = &ov9281_attr;
    sensor->video.vi_max_width = wsize->width;
    sensor->video.vi_max_height = wsize->height;
    v4l2_i2c_subdev_init(sd, client, &ov9281_ops);
    v4l2_set_subdev_hostdata(sd, sensor);

    pr_debug("probe ok ------->ov9281\n");

    return 0;

#if 0
    private_clk_disable(sensor->mclk);
    private_clk_put(sensor->mclk);
#endif
err_get_mclk:
    kfree(sensor);
    return -1;
}

static int ov9281_remove(struct i2c_client *client)
{
    struct v4l2_subdev *sd = i2c_get_clientdata(client);
    struct tx_isp_sensor *sensor = v4l2_get_subdev_hostdata(sd);

    if(reset_gpio != -1)
        gpio_free(reset_gpio);

    if(pwdn_gpio != -1)
        gpio_free(pwdn_gpio);

    clk_disable(sensor->mclk);
    clk_put(sensor->mclk);
    v4l2_device_unregister_subdev(sd);
    kfree(sensor);

    return 0;
}

static const struct i2c_device_id ov9281_id[] = {
    { "ov9281", 0 },
    { }
};
MODULE_DEVICE_TABLE(i2c, ov9281_id);

static struct i2c_driver ov9281_driver = {
    .driver = {
        .owner  = THIS_MODULE,
        .name   = "ov9281",
    },
    .probe      = ov9281_probe,
    .remove     = ov9281_remove,
    .id_table   = ov9281_id,
};

static __init int init_ov9281(void)
{

    return i2c_add_driver(&ov9281_driver);
}

static __exit void exit_ov9281(void)
{
    i2c_del_driver(&ov9281_driver);
}

module_init(init_ov9281);
module_exit(exit_ov9281);

MODULE_DESCRIPTION("A low-level driver for Gcoreinc ov9281 sensors");
MODULE_LICENSE("GPL");

