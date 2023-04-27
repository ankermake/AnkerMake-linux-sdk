/*
 * ov5693 Camera Driver
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
#include <utils/gpio.h>

#include <tx-isp/tx-isp-common.h>
#include <tx-isp/sensor-common.h>

#define OV5693_CHIP_ID_H                (0x56)
#define OV5693_CHIP_ID_L                (0x90)
#define OV5693_REG_END                  0xffff
#define OV5693_REG_DELAY                0x0000

#ifdef OV5693_BINNING_960P_SUPPORT
#define OV5693_SCLK                     (160000000)
#define SENSOR_OUTPUT_MAX_FPS           30
#else
#define OV5693_SCLK                     (160000000)
#define SENSOR_OUTPUT_MAX_FPS           30
#endif
#define SENSOR_OUTPUT_MIN_FPS           5
#define SENSOR_VERSION                  "H20181220"

static int reset_gpio       = -1;//GPIO_PA(8)
static int pwdn_gpio        = -1;

module_param_gpio(reset_gpio, 0644);
module_param_gpio(pwdn_gpio, 0644);

struct tx_isp_sensor_attribute ov5693_attr;

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

struct again_lut ov5693_again_lut[] = {
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
    {0x22, 71266},
    {0x24, 76671},
    {0x26, 81782},
    {0x28, 86632},
    {0x2a, 91245},
    {0x2c, 95643},
    {0x2e, 99846},
    {0x30, 103870},
    {0x32, 107730},
    {0x34, 111438},
    {0x36, 115006},
    {0x38, 118445},
    {0x3a, 121762},
    {0x3c, 124968},
    {0x3e, 128068},
    {0x40, 131070},
    {0x44, 136801},
    {0x48, 142206},
    {0x4c, 147317},
    {0x50, 152167},
    {0x54, 156780},
    {0x58, 161178},
    {0x5c, 165381},
    {0x60, 169405},
    {0x64, 173265},
    {0x68, 176973},
    {0x6c, 180541},
    {0x70, 183980},
    {0x74, 187297},
    {0x78, 190503},
    {0x7c, 193603},
    {0x80, 196605},
    {0x88, 202336},
    {0x90, 207741},
    {0x98, 212852},
    {0xa0, 217702},
    {0xa8, 222315},
    {0xb0, 226713},
    {0xb8, 230916},
    {0xc0, 234940},
    {0xc8, 238800},
    {0xd0, 242508},
    {0xd8, 246076},
    {0xe0, 249515},
    {0xe8, 252832},
    {0xf0, 256038},
    {0xf8, 259138},
};

static unsigned int ov5693_alloc_again(unsigned int isp_gain, unsigned char shift, unsigned int *sensor_again)
{
    struct again_lut *lut = ov5693_again_lut;

    while(lut->gain <= ov5693_attr.max_again) {
        if(isp_gain == 0) {
            *sensor_again = lut->value;
            return 0;
        }

        else if(isp_gain < lut->gain) {
            *sensor_again = (lut - 1)->value;
            return (lut - 1)->gain;
        }

        else{
            if((lut->gain == ov5693_attr.max_again) && (isp_gain >= lut->gain)) {
                *sensor_again = lut->value;
                return lut->gain;
            }

        }

        lut++;
    }

    return isp_gain;
}

static unsigned int ov5693_alloc_dgain(unsigned int isp_gain, unsigned char shift, unsigned int *sensor_dgain)
{
    return isp_gain;
}


struct tx_isp_sensor_attribute ov5693_attr = {
    .name                           = "ov5693",
    .chip_id                        = 0x5693,
    .cbus_type                      = TX_SENSOR_CONTROL_INTERFACE_I2C,
    .cbus_mask                      = V4L2_SBUS_MASK_SAMPLE_8BITS | V4L2_SBUS_MASK_ADDR_16BITS,
    .cbus_device                    = 0x10,     // 0x36 or 0x10
    .dbus_type                      = TX_SENSOR_DATA_INTERFACE_MIPI,
    .mipi = {
        .clk = 640,
        .lans = 2,
    },
    .max_again                      = 259138,
    .max_dgain                      = 0,
    .min_integration_time           = 2,
    .min_integration_time_native    = 2,
    .max_integration_time_native    = 1984 - 2,         //VTS
    .one_line_expr_in_us            = 17,
    .integration_time_limit         = 1984 - 2,
    .total_width                    = 2688,             //HTS
    .total_height                   = 1984,
    .max_integration_time           = 1984 - 2,
    .integration_time_apply_delay   = 2,
    .again_apply_delay              = 2,
    .dgain_apply_delay              = 0,
    .sensor_ctrl.alloc_again        = ov5693_alloc_again,
    .sensor_ctrl.alloc_dgain        = ov5693_alloc_dgain,
    //void priv; /* point to struct tx_isp_sensor_board_info */
};


/*
 * Interface    : MIPI
 * MCLK         : 24Mhz,
 * MIPI_clcok   : 640MHz(PLL1 multiplier)
 * resolution   : 1280*960
 * FrameRate    : 30fps
 * SYS_CLK      : 160MHz(PLL2_sys_clk),
 * HTS          : 0x0a80
 * VTS          : 0x07c0
 */

static struct regval_list ov5693_init_regs_binning_1280_960_30fps_mipi[] = {

//FQuarter_1280x960_30FPS     24M MCLK

    {0x0103,0x01},    //SOFTWARE_RST
    {0x3001,0x0a},    //system control [0x3001 ~ 0x303F] start
    {0x0100,0x01},
    {0x3002,0x80},
    {0x3006,0x00},
    {0x3011,0x21},
    {0x3012,0x09},
    {0x3013,0x10},
    {0x3014,0x00},
    {0x3015,0x08},
    {0x3016,0xf0},
    {0x3017,0xf0},
    {0x3018,0xf0},
    {0x301b,0xb4},
    {0x301d,0x02},
    {0x3021,0x00},
    {0x3022,0x01},
    {0x3028,0x44},    //system control end
    {0x3098,0x03},    //PLL control [0x3080 ~ 0x30B6] start
    {0x3099,0x1e},
    {0x309a,0x02},
    {0x309b,0x01},
    {0x309c,0x00},
    {0x30a0,0xd2},
    {0x30a2,0x01},
    {0x30b2,0x00},
    {0x30b3,0x50},    //64
    {0x30b4,0x03},
    {0x30b5,0x04},
    {0x30b6,0x01},    //PLL control end
    {0x3104,0x21},    //SCCB control start
    {0x3106,0x00},    //SCCB control end
    {0x3400,0x04},    //manual white balance start
    {0x3401,0x00},
    {0x3402,0x04},
    {0x3403,0x00},
    {0x3404,0x04},
    {0x3405,0x00},
    {0x3406,0x01},    //manual white balance end
    {0x3500,0x00},    //manual exposure control start
    {0x3501,0x3d},
    {0x3502,0x00},
    {0x3503,0x37},
    {0x3504,0x00},
    {0x3505,0x00},
    {0x3506,0x00},
    {0x3507,0x02},
    {0x3508,0x00},
    {0x3509,0x10},    //01 sensor gain;10 real gain
    {0x350a,0x00},
    {0x350b,0x40},    //manual exposure control end
    {0x3601,0x0a},    //ADC and analog start
    {0x3602,0x38},
    {0x3612,0x80},
    {0x3620,0x54},
    {0x3621,0xc7},
    {0x3622,0x0f},
    {0x3625,0x10},
    {0x3630,0x55},
    {0x3631,0xf4},
    {0x3632,0x00},
    {0x3633,0x34},
    {0x3634,0x02},
    {0x364d,0x0d},
    {0x364f,0xdd},
    {0x3660,0x04},
    {0x3662,0x10},
    {0x3663,0xf1},
    {0x3665,0x00},
    {0x3666,0x20},
    {0x3667,0x00},
    {0x366a,0x80},
    {0x3680,0xe0},
    {0x3681,0x00},    //ADC and analog end
    {0x3700,0x42},    //sensor control [0x3700 ~ 0x377F] start
    {0x3701,0x14},
    {0x3702,0xa0},
    {0x3703,0xd8},
    {0x3704,0x78},
    {0x3705,0x02},
    {0x3708,0xe6},
    {0x3709,0xc7},
    {0x370a,0x00},
    {0x370b,0x20},
    {0x370c,0x0c},
    {0x370d,0x11},
    {0x370e,0x00},
    {0x370f,0x40},
    {0x3710,0x00},
    {0x371a,0x1c},
    {0x371b,0x05},
    {0x371c,0x01},
    {0x371e,0xa1},
    {0x371f,0x0c},
    {0x3721,0x00},
    {0x3724,0x10},
    {0x3726,0x00},
    {0x372a,0x01},
    {0x3730,0x10},
    {0x3738,0x22},
    {0x3739,0xe5},
    {0x373a,0x50},
    {0x373b,0x02},
    {0x373c,0x41},
    {0x373f,0x02},
    {0x3740,0x42},
    {0x3741,0x02},
    {0x3742,0x18},
    {0x3743,0x01},
    {0x3744,0x02},
    {0x3747,0x10},
    {0x374c,0x04},
    {0x3751,0xf0},
    {0x3752,0x00},
    {0x3753,0x00},
    {0x3754,0xc0},
    {0x3755,0x00},
    {0x3756,0x1a},
    {0x3758,0x00},
    {0x3759,0x0f},
    {0x376b,0x44},
    {0x375c,0x04},
    {0x3774,0x10},
    {0x3776,0x00},
    {0x377f,0x08},    //sensor control end
    {0x3780,0x22},    //PSRAM control start
    {0x3781,0x0c},
    {0x3784,0x2c},
    {0x3785,0x1e},
    {0x378f,0xf5},
    {0x3791,0xb0},
    {0x3795,0x00},
    {0x3796,0x64},
    {0x3797,0x11},
    {0x3798,0x30},
    {0x3799,0x41},
    {0x379a,0x07},
    {0x379b,0xb0},
    {0x379c,0x0c},    //PSRAM control end
    {0x37c5,0x00},    //FREX control start
    {0x37c6,0x00},
    {0x37c7,0x00},
    {0x37c9,0x00},
    {0x37ca,0x00},
    {0x37cb,0x00},
    {0x37de,0x00},
    {0x37df,0x00},    //FREX control end

    {0x3800,0x00},    //timing control [0x3800 ~ 0x382F] start
    {0x3801,0x00},    //X_ADDR_START
    {0x3802,0x00},
    {0x3803,0x00},    //Y_ADDR_START
    {0x3804,0x0a},
    {0x3805,0x3f},    //X_ADDR_END    0x0a3f 2623
    {0x3806,0x07},
    {0x3807,0xa3},    //Y_ADDR_END    0x07a3 1955
    {0x3808,0x05},
    {0x3809,0x00},    //X_OUTPUT_SIZE 0x0500 1280
    {0x380a,0x03},
    {0x380b,0xC0},    //Y_OUTPUT_SIZE    0x03c0 960
    {0x380c,0x0a},
    {0x380d,0x80},    //HTS 0x0A80    2688
    {0x380e,0x07},
    {0x380f,0xc0},    //VTS 0x07C0    1984
    {0x3810,0x00},
    {0x3811,0x08},
    {0x3812,0x00},
    {0x3813,0x02},
    {0x3814,0x31},
    {0x3815,0x31},
    {0x3820,0x01},    //0x04
    {0x3821,0x1f},    //HDR surport disable
    {0x3823,0x00},
    {0x3824,0x00},
    {0x3825,0x00},
    {0x3826,0x00},
    {0x3827,0x00},
    {0x382a,0x04},    //timing control end
    {0x3a04,0x06},
    {0x3a05,0x14},
    {0x3a06,0x00},
    {0x3a07,0xfe},
    {0x3b00,0x00},    //strobe control start
    {0x3b02,0x00},
    {0x3b03,0x00},
    {0x3b04,0x00},
    {0x3b05,0x00},    //strobe control end
    {0x3e07,0x20},
    {0x4000,0x08},    //BLC control start
    {0x4001,0x04},
    {0x4002,0x45},
    {0x4004,0x08},
    {0x4005,0x18},
    {0x4006,0x20},
    {0x4008,0x24},
    {0x4009,0x10},
    {0x400c,0x00},
    {0x400d,0x00},    //BLC control end
    {0x4058,0x00},    //---
    {0x404e,0x37},
    {0x404f,0x8f},
    {0x4058,0x00},
    {0x4101,0xb2},    //---
    {0x4303,0x00},    //format control start
    {0x4304,0x08},
    {0x4307,0x30},
    {0x4311,0x04},
    {0x4315,0x01},    //format control end
    {0x4511,0x05},
    {0x4512,0x01},    //INPUT_SWAP_MAN_EN
    {0x4806,0x00},    //MIPI control start
    {0x4816,0x52},
    {0x481f,0x30},
    {0x4826,0x2c},
    {0x4831,0x64},    //MIPI control end

    {0x4d00,0x04},    //temperature monitor start
    {0x4d01,0x71},
    {0x4d02,0xfd},
    {0x4d03,0xf5},
    {0x4d04,0x0c},
    {0x4d05,0xcc},    //temperature monitor end
    {0x4837,0x0a},    //MIPI control add
    {0x5000,0x06},    //ISP top start
    {0x5001,0x01},
    {0x5002,0x00},
    {0x5003,0x20},
    {0x5046,0x0a},
    {0x5013,0x00},
    {0x5046,0x0a},    //ISP top end
    {0x5780,0x1c},    //DPC control start
    {0x5786,0x20},
    {0x5787,0x10},
    {0x5788,0x18},
    {0x578a,0x04},
    {0x578b,0x02},
    {0x578c,0x02},
    {0x578e,0x06},
    {0x578f,0x02},
    {0x5790,0x02},
    {0x5791,0xff},    //DPC control end
    {0x5842,0x01},    //LENC start
    {0x5843,0x2b},
    {0x5844,0x01},
    {0x5845,0x92},
    {0x5846,0x01},
    {0x5847,0x8f},
    {0x5848,0x01},
    {0x5849,0x0c},    //LENC end
    {0x5e00,0x00},    //color bar/scalar control start
    {0x5e10,0x0c},    //color bar/scalar control    end
    {0x0100,0x00},    //MODE_SELECT 0:software_standby 1:Streaming

    {OV5693_REG_END, 0x0}, /* END MARKER */
};

/*
 * Interface    : MIPI
 * MCLK         : 24Mhz,
 * MIPI_clcok   : 640MHz(PLL1 multiplier)
 * resolution   : 1280*960
 * FrameRate    : 30fps
 * SYS_CLK      : 160MHz(PLL2_sys_clk),
 * HTS          : 0xa80
 * VTS          : 0x7c0
 *
 *    skipping mode
 *    mode 1 output 2048*1536
 *    mode 2 output 1280*960
 */
static struct regval_list ov5693_init_regs_skipping_1280_960_30fps_mipi[] = {


    //BQuarter(1280x960) 30fps 2lane 10Bit; Center crop from 1296x972
    {0x0103,0x01},
    {0x3001,0x0a},
    {0x0100,0x01},
    {0x3002,0x80},
    {0x3006,0x00},
    {0x3011,0x21},
    {0x3012,0x09},
    {0x3013,0x10},
    {0x3014,0x00},
    {0x3015,0x08},
    {0x3016,0xf0},
    {0x3017,0xf0},
    {0x3018,0xf0},
    {0x301b,0xb4},
    {0x301d,0x02},
    {0x3021,0x00},
    {0x3022,0x01},
    {0x3028,0x44},
    {0x4800,0x04},    //system control end
    {0x3098,0x03},
    {0x3099,0x1e},
    {0x309a,0x02},
    {0x309b,0x01},
    {0x309c,0x00},
    {0x30a0,0xd2},
    {0x30a2,0x01},
    {0x30b2,0x00},
    {0x30b3,0x50},    //pll div 0x64-800M
    {0x30b4,0x03},
    {0x30b5,0x04},
    {0x30b6,0x01},
    {0x3104,0x21},
    {0x3106,0x00},
    {0x3400,0x04},
    {0x3401,0x00},
    {0x3402,0x04},
    {0x3403,0x00},
    {0x3404,0x04},
    {0x3405,0x00},
    {0x3406,0x01},
    {0x3500,0x00},
    {0x3501,0x7b},
    {0x3502,0x00},
    {0x3503,0x07},
    {0x3504,0x00},
    {0x3505,0x00},
    {0x3506,0x00},
    {0x3507,0x02},
    {0x3508,0x00},
    {0x3509,0x10},      // real gain
//    {0x3509,0x01},    //sensor gain
    {0x350a,0x00},
    {0x350b,0x20},
    {0x3600,0xbc},
    {0x3601,0x0a},
    {0x3602,0x38},
    {0x3612,0x80},
    {0x3620,0x44},
    {0x3621,0xb5},
    {0x3622,0x0c},
    {0x3625,0x10},
    {0x3630,0x55},
    {0x3631,0xf4},
    {0x3632,0x00},
    {0x3633,0x34},
    {0x3634,0x02},
    {0x364d,0x0d},
    {0x364f,0xdd},
    {0x3660,0x04},
    {0x3662,0x10},
    {0x3663,0xf1},
    {0x3665,0x00},
    {0x3666,0x20},
    {0x3667,0x00},
    {0x366a,0x80},
    {0x3680,0xe0},
    {0x3681,0x00},
    {0x3700,0x42},
    {0x3701,0x14},
    {0x3702,0xa0},
    {0x3703,0xd8},
    {0x3704,0x78},
    {0x3705,0x02},
    {0x3708,0xe2},
    {0x3709,0xc3},
    {0x370a,0x00},
    {0x370b,0x20},
    {0x370c,0x0c},
    {0x370d,0x11},
    {0x370e,0x00},
    {0x370f,0x40},
    {0x3710,0x00},
    {0x371a,0x1c},
    {0x371b,0x05},
    {0x371c,0x01},
    {0x371e,0xa1},
    {0x371f,0x0c},
    {0x3721,0x00},
    {0x3724,0x10},
    {0x3726,0x00},
    {0x372a,0x01},
    {0x3730,0x10},
    {0x3738,0x22},
    {0x3739,0xe5},
    {0x373a,0x50},
    {0x373b,0x02},
    {0x373c,0x41},
    {0x373f,0x02},
    {0x3740,0x42},
    {0x3741,0x02},
    {0x3742,0x18},
    {0x3743,0x01},
    {0x3744,0x02},
    {0x3747,0x10},
    {0x374c,0x04},
    {0x3751,0xf0},
    {0x3752,0x00},
    {0x3753,0x00},
    {0x3754,0xc0},
    {0x3755,0x00},
    {0x3756,0x1a},
    {0x3758,0x00},
    {0x3759,0x0f},
    {0x376b,0x44},
    {0x375c,0x04},
    {0x3774,0x10},
    {0x3776,0x00},
    {0x377f,0x08},
    {0x3780,0x22},
    {0x3781,0x0c},
    {0x3784,0x2c},
    {0x3785,0x1e},
    {0x378f,0xf5},
    {0x3791,0xb0},
    {0x3795,0x00},
    {0x3796,0x64},
    {0x3797,0x11},
    {0x3798,0x30},
    {0x3799,0x41},
    {0x379a,0x07},
    {0x379b,0xb0},
    {0x379c,0x0c},
    {0x37c5,0x00},
    {0x37c6,0x00},
    {0x37c7,0x00},
    {0x37c9,0x00},
    {0x37ca,0x00},
    {0x37cb,0x00},
    {0x37de,0x00},
    {0x37df,0x00},
    {0x3800,0x00},
    {0x3801,0x20},  // X_ADDR_START
    {0x3802,0x00},
    {0x3803,0x12},  // Y_ADDR_START
    {0x3804,0x0a},
    {0x3805,0x1f},  // X_ADDR_END
    {0x3806,0x07},
    {0x3807,0x91},  // Y_ADDR_END
    {0x3808,0x05},
    {0x3809,0x00},  //X_OUTPUT_SIZE
    {0x380a,0x03},
    {0x380b,0xc0},  //Y_OUTPUT_SIZE
    {0x380c,0x0a},
    {0x380d,0x80},  // HTS
    {0x380e,0x07},
    {0x380f,0xc0},  // VTS
    {0x3810,0x00},
    {0x3811,0x02},
    {0x3812,0x00},
    {0x3813,0x02},
    {0x3814,0x11},
    {0x3815,0x11},
    {0x3820,0x00},
    {0x3821,0x1e},
    {0x3823,0x00},
    {0x3824,0x00},
    {0x3825,0x00},
    {0x3826,0x00},
    {0x3827,0x00},
    {0x382a,0x04},
    {0x3a04,0x06},
    {0x3a05,0x14},
    {0x3a06,0x00},
    {0x3a07,0xfe},
    {0x3b00,0x00},
    {0x3b02,0x00},
    {0x3b03,0x00},
    {0x3b04,0x00},
    {0x3b05,0x00},
    {0x3e07,0x20},
    {0x4000,0x08},
    {0x4001,0x04},
    {0x4002,0x45},
    {0x4004,0x08},
    {0x4005,0x18},
    {0x4006,0x20},
    {0x4008,0x24},
    {0x4009,0x10},
    {0x400c,0x00},
    {0x400d,0x00},
    {0x4058,0x00},
    {0x404e,0x37},
    {0x404f,0x8f},
    {0x4058,0x00},
    {0x4101,0xb2},
    {0x4303,0x00},
    {0x4304,0x08},
    {0x4307,0x30},
    {0x4311,0x04},
    {0x4315,0x01},
    {0x4511,0x05},
    {0x4512,0x01},
    {0x4806,0x00},
    {0x4816,0x52},
    {0x481f,0x30},
    {0x4826,0x2c},
    {0x4831,0x64},
    {0x4d00,0x04},
    {0x4d01,0x71},
    {0x4d02,0xfd},
    {0x4d03,0xf5},
    {0x4d04,0x0c},
    {0x4d05,0xcc},
    {0x4837,0x0a},
    {0x5000,0x06},
    {0x5001,0x01},
    {0x5002,0x80},
    {0x5003,0x20},
    {0x5046,0x0a},
    {0x5013,0x00},
    {0x5046,0x0a},
    {0x5780,0xfc},
    {0x5781,0x13},
    {0x5782,0x03},
    {0x5786,0x20},
    {0x5787,0x40},
    {0x5788,0x08},
    {0x5789,0x08},
    {0x578a,0x02},
    {0x578b,0x01},
    {0x578c,0x01},
    {0x578d,0x0c},
    {0x578e,0x02},
    {0x578f,0x01},
    {0x5790,0x01},
    {0x5791,0xff},
    {0x5842,0x01},
    {0x5843,0x2b},
    {0x5844,0x01},
    {0x5845,0x92},
    {0x5846,0x01},
    {0x5847,0x8f},
    {0x5848,0x01},
    {0x5849,0x0c},
    {0x5e00,0x00},
    {0x5e10,0x0c},
    {0x0100,0x00},    //MODE_SELECT 0:software_standby 1:Streaming

    {OV5693_REG_END, 0x0}, /* END MARKER */
};


static struct tx_isp_sensor_win_setting ov5693_win_sizes[] = {
    {
        .width      = 1280,
        .height     = 960,
        .fps        = 30 << 16 | 1,
        .mbus_code  = V4L2_MBUS_FMT_SBGGR10_1X10,
        .colorspace = V4L2_COLORSPACE_SRGB,
        .regs       = ov5693_init_regs_binning_1280_960_30fps_mipi,
    },
    {
        .width      = 1280,
        .height     = 960,
        .fps        = 30 << 16 | 1,
        .mbus_code  = V4L2_MBUS_FMT_SBGGR10_1X10,
        .colorspace = V4L2_COLORSPACE_SRGB,
        .regs       = ov5693_init_regs_skipping_1280_960_30fps_mipi,
    },
};

/*
 * the part of driver was fixed.
 */

static struct regval_list ov5693_stream_on_mipi[] = {
    {0x0100,0x01},
    {OV5693_REG_END, 0x0}, /* END MARKER */
};

static struct regval_list ov5693_stream_off_mipi[] = {
    {0x0100,0x00},
    {OV5693_REG_END, 0x0}, /* END MARKER */
};

static int ov5693_read(struct v4l2_subdev *sd, u16 reg, u8 *val)
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

static int ov5693_write(struct v4l2_subdev *sd, u16 reg, u8 val)
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

static inline int ov5693_read_array(struct v4l2_subdev *sd, struct regval_list *vals)
{
    int ret;
    u8 val;

    while (vals->reg_num != OV5693_REG_END) {
       if (vals->reg_num == OV5693_REG_DELAY) {
            msleep(vals->value);
        } else {
          ret = ov5693_read(sd, vals->reg_num, &val);
          printk("reg 0x%x, value 0x%x\n",vals->reg_num,val);
            if (ret < 0)
                return ret;
        }
        vals++;
    }

    return 0;
}

static int ov5693_write_array(struct v4l2_subdev *sd, struct regval_list *vals)
{
    int ret;

    while (vals->reg_num != OV5693_REG_END) {
        if (vals->reg_num == OV5693_REG_DELAY) {
                msleep(vals->value);
        } else {
            ret = ov5693_write(sd, vals->reg_num, vals->value);
            if (ret < 0)
                return ret;
        }
        //printk("vals->reg_num:%x, vals->value:%x\n",vals->reg_num, vals->value);
        vals++;
    }

    return 0;
}


static int ov5693_reset(struct v4l2_subdev *sd, unsigned int val)
{
    return 0;
}

static int ov5693_detect(struct v4l2_subdev *sd, unsigned int *ident)
{
    unsigned char v;
    int ret;

    ret = ov5693_read(sd, 0x300A, &v);
    pr_debug("-----%s: %d ret = %d, v = 0x%02x\n", __func__, __LINE__, ret,v);
    if (ret < 0)
        return ret;

    if (v != OV5693_CHIP_ID_H)
        return -ENODEV;

    *ident = v;
    ret = ov5693_read(sd, 0x300B, &v);
    pr_debug("-----%s: %d ret = %d, v = 0x%02x\n", __func__, __LINE__, ret,v);
    if (ret < 0)
        return ret;

    if (v != OV5693_CHIP_ID_L)
        return -ENODEV;

    *ident = (*ident << 8) | v;

    return 0;
}

static int ov5693_set_integration_time(struct v4l2_subdev *sd, int value)
{
    int ret = 0;
    unsigned int expo = value<<4;

    ret = ov5693_write(sd, 0x3502, (unsigned char)(expo & 0xff));
    if (ret < 0)
        return ret;
    ret = ov5693_write(sd, 0x3501, (unsigned char)((expo >> 8) & 0xff));
    if (ret < 0)
        return ret;
    ret = ov5693_write(sd, 0x3500, (unsigned char)((expo >> 16) & 0x7));
    if (ret < 0)
        return ret;

    return 0;

}

/**************************************
  ov5693 sensor gain control by [0x350A] [0x350B]
    value from 0x10 to 0xF8 represents gain of 1x to 15.5x
**************************************/
static int ov5693_set_analog_gain(struct v4l2_subdev *sd, u16 value)
{
    int ret = 0;

    ret = ov5693_write(sd, 0x350B, (unsigned char)value);
    if (ret < 0) {
        return ret;
        }

    return 0;
}

static int ov5693_set_digital_gain(struct v4l2_subdev *sd, int value)
{
    return 0;
}

static int ov5693_get_black_pedestal(struct v4l2_subdev *sd, int value)
{
    return 0;
}

static int ov5693_init(struct v4l2_subdev *sd, unsigned int enable)
{
    struct tx_isp_sensor *sensor = (container_of(sd, struct tx_isp_sensor, sd));
    struct tx_isp_notify_argument arg;
    struct tx_isp_sensor_win_setting *wsize;
    int ret = 0;

    if(!enable)
        return ISP_SUCCESS;
    //printk("%d %s\n", __LINE__, __func__);

#ifdef OV5693_BINNING_960P_SUPPORT
    wsize = &ov5693_win_sizes[0];
#else
    wsize = &ov5693_win_sizes[1];
#endif

    sensor->video.mbus.width      = wsize->width;
    sensor->video.mbus.height     = wsize->height;
    sensor->video.mbus.code       = wsize->mbus_code;
    sensor->video.mbus.field      = V4L2_FIELD_NONE;
    sensor->video.mbus.colorspace = wsize->colorspace;
    sensor->video.fps             = wsize->fps;

    ret = ov5693_write_array(sd, wsize->regs);

    if (ret)
        return ret;

 //   ret = ov5693_read_array(sd, wsize->regs);
//    while(1);
    arg.value = (int)&sensor->video;
    sd->v4l2_dev->notify(sd, TX_ISP_NOTIFY_SYNC_VIDEO_IN, &arg);
    sensor->priv = wsize;

    return 0;
}

static int ov5693_g_parm(struct v4l2_subdev *sd, struct v4l2_streamparm *parms)
{
    return 0;
}

static int ov5693_s_parm(struct v4l2_subdev *sd, struct v4l2_streamparm *parms)
{

    return 0;
}

static int ov5693_s_stream(struct v4l2_subdev *sd, int enable)
{
    int ret = 0;

    if (enable) {
        ret = ov5693_write_array(sd, ov5693_stream_on_mipi);
        pr_debug("ov5693 stream on\n");

    }
    else {
        ret = ov5693_write_array(sd, ov5693_stream_off_mipi);
        pr_debug("ov5693 stream off\n");
    }

    return ret;
}

static int ov5693_set_fps(struct tx_isp_sensor *sensor, int fps)
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

    sclk = OV5693_SCLK;
    max_fps = SENSOR_OUTPUT_MAX_FPS;

    /* the format of fps is 16/16. for example 25 << 16 | 2, the value is 25/2 fps. */
    newformat = (((fps >> 16) / (fps & 0xffff)) << 8) + ((((fps >> 16) % (fps & 0xffff)) << 8) / (fps & 0xffff));
    if(newformat > (max_fps << 8) || newformat < (SENSOR_OUTPUT_MIN_FPS << 8)){
        printk(KERN_ERR "ov5693: fps(%x) no in range\n", fps);
        return -1;
    }

    //HTS
    ret = ov5693_read(sd, 0x380c, &tmp);
    hts = tmp << 8;
    ret += ov5693_read(sd, 0x380d, &tmp);
    if(ret < 0)
        return -1;

    hts |= tmp;
    //VTS
    vts = sclk * (fps & 0xffff) / hts / ((fps & 0xffff0000) >> 16);

    ret = ov5693_write(sd, 0x380e, (unsigned char)(vts >> 8));
    ret += ov5693_write(sd, 0x380f, (unsigned char)(vts & 0xff));
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

static int ov5693_set_mode(struct tx_isp_sensor *sensor, int value)
{
    struct tx_isp_notify_argument arg;
    struct v4l2_subdev *sd = &sensor->sd;
    struct tx_isp_sensor_win_setting *wsize = NULL;
    int ret = ISP_SUCCESS;

#ifdef OV5693_BINNING_960P_SUPPORT
    if(value == TX_ISP_SENSOR_FULL_RES_MAX_FPS){
        wsize = &ov5693_win_sizes[0];
    }else if(value == TX_ISP_SENSOR_PREVIEW_RES_MAX_FPS){
        wsize = &ov5693_win_sizes[0];
    }
#else
    if(value == TX_ISP_SENSOR_FULL_RES_MAX_FPS){
        wsize = &ov5693_win_sizes[1];
    }else if(value == TX_ISP_SENSOR_PREVIEW_RES_MAX_FPS){
        wsize = &ov5693_win_sizes[1];
    }
#endif

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

static inline int ov5693_set_vflip(struct tx_isp_sensor *sensor, int enable)
{
    struct tx_isp_notify_argument arg;
    struct v4l2_subdev *sd = &sensor->sd;
    int ret = 0;
    u8 val = 0;

    ret = ov5693_read(sd, 0x3820, &val);
    if (enable){
        val |= (1<<2);
        //sensor->video.mbus.code = V4L2_MBUS_FMT_SBGGR10_1X10;
    } else {
        val &= (1<<2);
        //sensor->video.mbus.code = V4L2_MBUS_FMT_SGRBG10_1X10;
    }
    ret += ov5693_write(sd, 0x3820, val);
    arg.value = (int)&sensor->video;

    if(!ret)
        sd->v4l2_dev->notify(sd, TX_ISP_NOTIFY_SYNC_VIDEO_IN, &arg);

    return ret;
}

static int ov5693_g_chip_ident(struct v4l2_subdev *sd,
        struct v4l2_dbg_chip_ident *chip)
{
    struct i2c_client *client = v4l2_get_subdevdata(sd);
    unsigned int ident = 0;
    int ret = ISP_SUCCESS;

    if (reset_gpio != -1) {
       ret = gpio_request(reset_gpio,"ov5693_reset");
        if (!ret) {
            gpio_direction_output(reset_gpio, 1);
            msleep(5);
            gpio_direction_output(reset_gpio, 0);
            msleep(30);
            gpio_direction_output(reset_gpio, 1);
            msleep(35);
        } else {
            printk(KERN_ERR "ov5693: gpio requrest fail %d\n", reset_gpio);
            return -1;
        }
    }

    if (pwdn_gpio != -1) {
        ret = gpio_request(pwdn_gpio,"ov5693_pwdn");
        if(!ret){
            gpio_direction_output(pwdn_gpio, 1);
            msleep(10);
            gpio_direction_output(pwdn_gpio, 0);
            msleep(50);
        }else{
            printk(KERN_ERR "ov5693: gpio requrest fail %d\n",pwdn_gpio);
            return -1;
        }
    }

    ret = ov5693_detect(sd, &ident);
    if (ret) {
        printk(KERN_ERR "ov5693: chip found @ 0x%x (%s) is not an ov5693 chip.\n",
                client->addr, client->adapter->name);
        return ret;
    }

    v4l_info(client, "ov5693 chip found @ 0x%02x (%s)\n",
         client->addr, client->adapter->name);

    return v4l2_chip_ident_i2c_client(client, chip, ident, 0);
}

static int ov5693_s_power(struct v4l2_subdev *sd, int on)
{
    return 0;
}

static int ov5693_ops_private_ioctl(struct tx_isp_sensor *sensor, struct isp_private_ioctl *ctrl)
{
    long ret = 0;

    struct v4l2_subdev *sd = &sensor->sd;
    //printk("%d %s %d\n", __LINE__, __func__, ctrl->cmd);
    switch(ctrl->cmd){
    case TX_ISP_PRIVATE_IOCTL_SENSOR_INT_TIME:
        ret = ov5693_set_integration_time(sd, ctrl->value);
        break;
    case TX_ISP_PRIVATE_IOCTL_SENSOR_AGAIN:
        ret = ov5693_set_analog_gain(sd, ctrl->value);
        break;
    case TX_ISP_PRIVATE_IOCTL_SENSOR_DGAIN:
        ret = ov5693_set_digital_gain(sd, ctrl->value);
        break;
    case TX_ISP_PRIVATE_IOCTL_SENSOR_BLACK_LEVEL:
        ret = ov5693_get_black_pedestal(sd, ctrl->value);
        break;
    case TX_ISP_PRIVATE_IOCTL_SENSOR_RESIZE:
        ret = ov5693_set_mode(sensor, ctrl->value);
        break;
    case TX_ISP_PRIVATE_IOCTL_SUBDEV_PREPARE_CHANGE:
        ret = ov5693_write_array(sd, ov5693_stream_off_mipi);
        break;
    case TX_ISP_PRIVATE_IOCTL_SUBDEV_FINISH_CHANGE:
        ret = ov5693_write_array(sd, ov5693_stream_on_mipi);
        break;
    case TX_ISP_PRIVATE_IOCTL_SENSOR_FPS:
        ret = ov5693_set_fps(sensor, ctrl->value);
        break;
    //case TX_ISP_PRIVATE_IOCTL_SENSOR_VFLIP:
        //ret = ov5693_set_vflip(sd, ctrl->value);
        //break;
    default:
        break;;
    }

    return 0;
}

static long ov5693_ops_ioctl(struct v4l2_subdev *sd, unsigned int cmd, void *arg)
{
    struct tx_isp_sensor *sensor =container_of(sd, struct tx_isp_sensor, sd);
    int ret;

    switch (cmd) {
    case VIDIOC_ISP_PRIVATE_IOCTL:
        ret = ov5693_ops_private_ioctl(sensor, arg);
        break;
    default:
        return -1;
        break;
    }

    return 0;
}

#ifdef CONFIG_VIDEO_ADV_DEBUG
static int ov5693_g_register(struct v4l2_subdev *sd, struct v4l2_dbg_register *reg)
{
    struct i2c_client *client = v4l2_get_subdevdata(sd);
    unsigned char val = 0;
    int ret;

    if (!v4l2_chip_match_i2c_client(client, &reg->match))
        return -EINVAL;

    if (!capable(CAP_SYS_ADMIN))
        return -EPERM;

    ret = ov5693_read(sd, reg->reg & 0xffff, &val);
    reg->val = val;
    reg->size = 1;

    return ret;
}

static int ov5693_s_register(struct v4l2_subdev *sd, const struct v4l2_dbg_register *reg)
{
    struct i2c_client *client = v4l2_get_subdevdata(sd);

    if (!v4l2_chip_match_i2c_client(client, &reg->match))
        return -EINVAL;

    if (!capable(CAP_SYS_ADMIN))
        return -EPERM;

    ov5693_write(sd, reg->reg & 0xffff, reg->val & 0xff);

    return 0;
}
#endif

static struct v4l2_subdev_core_ops ov5693_core_ops = {
    .g_chip_ident   = ov5693_g_chip_ident,
    .reset          = ov5693_reset,
    .init           = ov5693_init,
    .s_power        = ov5693_s_power,
    .ioctl          = ov5693_ops_ioctl,
#ifdef CONFIG_VIDEO_ADV_DEBUG
    .g_register     = ov5693_g_register,
    .s_register     = ov5693_s_register,
#endif
};

static struct v4l2_subdev_video_ops ov5693_video_ops = {
    .s_stream       = ov5693_s_stream,
    .s_parm         = ov5693_s_parm,
    .g_parm         = ov5693_g_parm,
};

static struct v4l2_subdev_ops ov5693_ops = {
    .core  = &ov5693_core_ops,
    .video = &ov5693_video_ops,
};

/* It's the sensor device */
static u64 tx_isp_module_dma_mask = ~(u64)0;
struct platform_device sensor_platform_device = {
    .name          = "ov5693",
    .id            = -1,
    .dev = {
        .dma_mask = &tx_isp_module_dma_mask,
        .coherent_dma_mask = 0xffffffff,
        .platform_data = NULL,
    },
    .num_resources = 0,
};

static int ov5693_probe(struct i2c_client *client, const struct i2c_device_id *id)
{
    struct v4l2_subdev *sd;
    struct tx_isp_video_in *video;
    struct tx_isp_sensor *sensor;
    struct tx_isp_sensor_win_setting *wsize;
    struct clk *isp_clk=NULL;
    int ret = 0;

    pr_debug("probe ok ----start--->ov5693\n");
    sensor = (struct tx_isp_sensor *)kzalloc(sizeof(*sensor), GFP_KERNEL);
    if(!sensor){
        printk(KERN_ERR "ov5693: Failed to allocate sensor subdev.\n");
        return -ENOMEM;
    }

    ret = tx_isp_clk_set(MD_X1520_OV5693_ISPCLK);
    if (ret < 0) {
        printk(KERN_ERR "ov5693: Cannot set isp clock\n");
        goto err_get_mclk;
    }
    /* request mclk of sensor */
    sensor->mclk = clk_get(NULL, "cgu_cim");
    if (IS_ERR(sensor->mclk)) {
        printk(KERN_ERR "ov5693: Cannot get sensor input clock cgu_cim\n");
        goto err_get_mclk;
    }

    clk_set_rate(sensor->mclk, 24000000);
    clk_enable(sensor->mclk);

    /* request isp clk */
    isp_clk = clk_get(NULL, "cgu_isp");
    if (IS_ERR(isp_clk)) {
        printk(KERN_ERR "ov5693: Cannot get input clock cgu_isp\n");
        goto err_get_isp_clk;
    }
    clk_set_rate(isp_clk, 200*1000000);    //200M

#ifdef OV5693_BINNING_960P_SUPPORT
    wsize = &ov5693_win_sizes[0];
#else
    wsize = &ov5693_win_sizes[1];
#endif

    ov5693_attr.max_again = 259138;
    ov5693_attr.max_dgain = 0;
    sd = &sensor->sd;
    video = &sensor->video;
    sensor->video.attr = &ov5693_attr;
    sensor->video.vi_max_width = wsize->width;
    sensor->video.vi_max_height = wsize->height;
    v4l2_i2c_subdev_init(sd, client, &ov5693_ops);
    v4l2_set_subdev_hostdata(sd, sensor);

    pr_debug("probe ok ------->ov5693\n");

    return 0;

err_get_mclk:
    kfree(sensor);
err_get_isp_clk:
    kfree(sensor);

    return -1;
}

static int ov5693_remove(struct i2c_client *client)
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

static const struct i2c_device_id ov5693_id[] = {
    { "ov5693", 0 },
    { }
};
MODULE_DEVICE_TABLE(i2c, ov5693_id);

static struct i2c_driver ov5693_driver = {
    .driver = {
        .owner  = THIS_MODULE,
        .name   = "ov5693",
    },
    .probe      = ov5693_probe,
    .remove     = ov5693_remove,
    .id_table   = ov5693_id,
};

static __init int init_ov5693(void)
{
    return i2c_add_driver(&ov5693_driver);
}

static __exit void exit_ov5693(void)
{
    i2c_del_driver(&ov5693_driver);
}

module_init(init_ov5693);
module_exit(exit_ov5693);

MODULE_DESCRIPTION("x1520 ov5693 driver depend on isp");
MODULE_LICENSE("GPL");


