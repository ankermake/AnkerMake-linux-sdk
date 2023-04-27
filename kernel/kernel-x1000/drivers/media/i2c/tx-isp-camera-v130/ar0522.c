/*
 * ar0522 Camera Driver
 *
 * Copyright (C) 2019, Ingenic Semiconductor Inc.
 *
 * Authors: Jeff <jifu.liu@ingenic.com>
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
#include <linux/sensor_board.h>
#include <tx-isp/tx-isp-common.h>
#include <tx-isp/sensor-common.h>
#include <tx-isp/apical-isp/apical_math.h>

#define AR0522_CHIP_ID_H                       (0x14)
#define AR0522_CHIP_ID_L                       (0x57)

#define AR0522_SUPPORT_30FPS_SCLK              (154000000)
#define SENSOR_VERSION                         "H20191224"

#if defined CONFIG_AR0522_2592X1944
#define AR0522_WIDTH                           2592
#define AR0522_HEIGHT                          1944
#define SENSOR_OUTPUT_MAX_FPS                  25
#elif defined CONFIG_AR0522_2560X1080
#define AR0522_WIDTH                           2560
#define AR0522_HEIGHT                          1080
#define SENSOR_OUTPUT_MAX_FPS                  30
#endif

#define SENSOR_OUTPUT_MIN_FPS                  5

#define AR0522_REG_END                         0xffff
#define AR0522_REG_DELAY                       0xfffe


struct regval_list {
    unsigned short reg_num;
    unsigned short value;
};

static int power_gpio = -1;
module_param(power_gpio, int, S_IRUGO);
MODULE_PARM_DESC(power_gpio, "Power on GPIO NUM");

static int pwdn_gpio = -1;
module_param(pwdn_gpio, int, S_IRUGO);
MODULE_PARM_DESC(pwdn_gpio, "Power down GPIO NUM");

static int reset_gpio = -1;
module_param(reset_gpio, int, S_IRUGO);
MODULE_PARM_DESC(reset_gpio, "Reset GPIO NUM");

static int i2c_sel1_gpio = -1;
module_param(i2c_sel1_gpio, int, S_IRUGO);
MODULE_PARM_DESC(i2c_sel1_gpio, "i2c select1  GPIO NUM");


struct tx_isp_sensor_attribute ar0522_attr;

#ifndef MODULE
static struct sensor_board_info *ar0522_board_info;
#endif


struct again_lut {
    unsigned int value;
    unsigned int gain;
};

/* static unsigned char again_mode = LCG; */
struct again_lut ar0522_again_lut[] = {
    {0x0, 0},
    {0x1, 2794},
    {0x2, 6397},
    {0x3, 9011},
    {0x4, 12388},
    {0x5, 16447},
    {0x6, 19572},
    {0x7, 23340},
    {0x8, 26963},
    {0x9, 31135},
    {0xa, 35780},
    {0xb, 39588},
    {0xc, 44438},
    {0xd, 49051},
    {0xe, 54517},
    {0xf, 59685},
    {0x10, 65536},
    {0x12, 71490},
    {0x14, 78338},
    {0x16, 85108},
    {0x18, 92854},
    {0x1a, 100992},
    {0x1c, 109974},
    {0x1e, 120053},
    {0x20, 131072},
    {0x22, 137247},
    {0x24, 143667},
    {0x26, 150644},
    {0x28, 158212},
    {0x2a, 166528},
    {0x2c, 175510},
    {0x2e, 185457},
    {0x30, 196608},
    {0x32, 202783},
    {0x34, 209203},
    {0x36, 216276},
    {0x38, 223748},
    {0x3a, 232064},
    {0x3c, 241046},
    {0x3e, 250993},
    {0x40, 262144},

};

static unsigned int ar0522_alloc_again(unsigned int isp_gain, unsigned char shift, unsigned int *sensor_again)
{
    struct again_lut *lut = ar0522_again_lut;

    while(lut->gain <= ar0522_attr.max_again) {
        if(isp_gain == 0) {
            *sensor_again = 0;
            /* again_mode = LCG; */
            return 0;
        }
        else if(isp_gain < lut->gain) {
            *sensor_again = (lut - 1)->value;
            /* again_mode = (lut - 1)->mode; */
            return (lut - 1)->gain;
        }
        else{
            if((lut->gain == ar0522_attr.max_again) && (isp_gain >= lut->gain)) {
                *sensor_again = lut->value;
                /* again_mode = lut->mode; */
                return lut->gain;
            }
        }

        lut++;
    }

    return isp_gain;

}

static unsigned int ar0522_alloc_dgain(unsigned int isp_gain, unsigned char shift, unsigned int *sensor_dgain)
{
    return isp_gain;
}

struct tx_isp_sensor_attribute ar0522_attr={
    .name                                  = "ar0522",
    .chip_id                               = 0x1457,
    .cbus_type                             = TX_SENSOR_CONTROL_INTERFACE_I2C,
    .cbus_mask                             = V4L2_SBUS_MASK_SAMPLE_16BITS | V4L2_SBUS_MASK_ADDR_16BITS,
    .cbus_device                           = 0x18,
    .dbus_type                             = TX_SENSOR_DATA_INTERFACE_MIPI,
    .mipi = {
        .clk = 600,
        .lans = 2,
    },
    .max_again                             = 262144,
    .max_dgain                             = 0,
    .min_integration_time                  = 2,
    .min_integration_time_native           = 2,
#if defined CONFIG_AR0522_2592X1944
    .max_integration_time_native           = 0x0890,
    .integration_time_limit                = 0x0890,
    .total_width                           = 0x15D4,
    .total_height                          = 0x0890,
    .max_integration_time                  = 0x0890,
#elif defined CONFIG_AR0522_2560X1080
    .max_integration_time_native           = 0x0890,
    .integration_time_limit                = 0x0890,
    .total_width                           = 0x15D4,
    .total_height                          = 0x0890,
    .max_integration_time                  = 0x0890,
#endif

    .integration_time_apply_delay          = 2,
    .again_apply_delay                     = 2,
    .dgain_apply_delay                     = 2,
    .sensor_ctrl.alloc_again               = ar0522_alloc_again,
    .sensor_ctrl.alloc_dgain               = ar0522_alloc_dgain,
    //    void priv; /* point to struct tx_isp_sensor_board_info */
};

static struct regval_list ar0522_init_regs_mipi_2592_1944_30fps[] ={
/*
 *
 *  Register Log created on Tuesday, December 17, 2019 : 10:03:18
 *
 *  [Register Log 12/17/19 10:03:18]
 *
 *   XMCLK = 24000000
 *
 */
    {AR0522_REG_DELAY,100},
    {0x301A, 0x00D9},
    {AR0522_REG_DELAY,100},
    {AR0522_REG_DELAY,100},
    {0x301A, 0x30D8},
    {0x3042, 0x0004},    // DARK_CONTROL2
    {0x3044, 0x4580},    // DARK_CONTROL
    {0x30EE, 0x1136},    // DARK_CONTROL3
    {0x3120, 0x0001},    // DITHER_CONTROL
    {0x3F2C, 0x442E},    // GTH_THRES_RTN
    {0x30D2, 0x0000},   // CRM_CONTROL
    {0x30D4, 0x0000},    // COLUMN_CORRECTION
    {0x30D6, 0x2FFF},    // COLUMN_CORRECTION2
    {0x30DA, 0x0FFF},    // COLUMN_CORRECTION_CLIP2
    {0x30DC, 0x0FFF},    // COLUMN_CORRECTION_CLIP3
    {0x30DE, 0x0000},    // COLUMN_GROUP_CORRECTION
    {0x31E0, 0x0781},    // PIX_DEF_ID
    {0x3180, 0x9434},    // FINE_DIG_CORRECTION_CONTROL
    {0x3172, 0x0206},    // ANALOG_CONTROL2
    {0x3F00, 0x0017},    // BM_T0
    {0x3F02, 0x02DD},    // BM_T1
    {0x3F04, 0x0020},    // NOISE_GAIN_THRESHOLD0
    {0x3F06, 0x0040},    // NOISE_GAIN_THRESHOLD1
    {0x3F08, 0x0070},    // NOISE_GAIN_THRESHOLD2
    {0x3F0A, 0x0101},    // NOISE_FLOOR10
    {0x3F0C, 0x0302},    // NOISE_FLOOR32
    {0x3F1E, 0x0022},    // NOISE_COEF
    {0x3F1A, 0x0103},    // CROSSFACTOR2
    {0x3F14, 0x0505},    // SINGLE_K_FACTOR2
    {0x3F44, 0x1515},    // COUPLE_K_FACTOR2
    {0x3F18, 0x0103},    // CROSSFACTOR1
    {0x3F12, 0x0505},    // SINGLE_K_FACTOR1
    {0x3F42, 0x1515},    // COUPLE_K_FACTOR1
    {0x3F16, 0x0103},    // CROSSFACTOR0
    {0x3F10, 0x0505},    // SINGLE_K_FACTOR0
    {0x3F40, 0x1515},    // COUPLE_K_FACTOR0
    {0x3D00, 0x043E},
    {0x3D02, 0x4760},
    {0x3D04, 0xFFFF},
    {0x3D06, 0xFFFF},
    {0x3D08, 0x8000},
    {0x3D0A, 0x0510},
    {0x3D0C, 0xAF08},
    {0x3D0E, 0x0252},
    {0x3D10, 0x486F},
    {0x3D12, 0x5D5D},
    {0x3D14, 0x8056},
    {0x3D16, 0x8313},
    {0x3D18, 0x0087},
    {0x3D1A, 0x6A48},
    {0x3D1C, 0x6982},
    {0x3D1E, 0x0280},
    {0x3D20, 0x8359},
    {0x3D22, 0x8D02},
    {0x3D24, 0x8020},
    {0x3D26, 0x4882},
    {0x3D28, 0x4269},
    {0x3D2A, 0x6A95},
    {0x3D2C, 0x5988},
    {0x3D2E, 0x5A83},
    {0x3D30, 0x5885},
    {0x3D32, 0x6280},
    {0x3D34, 0x6289},
    {0x3D36, 0x6097},
    {0x3D38, 0x5782},
    {0x3D3A, 0x605C},
    {0x3D3C, 0xBF18},
    {0x3D3E, 0x0961},
    {0x3D40, 0x5080},
    {0x3D42, 0x2090},
    {0x3D44, 0x4390},
    {0x3D46, 0x4382},
    {0x3D48, 0x5F8A},
    {0x3D4A, 0x5D5D},
    {0x3D4C, 0x9C63},
    {0x3D4E, 0x8063},
    {0x3D50, 0xA960},
    {0x3D52, 0x9757},
    {0x3D54, 0x8260},
    {0x3D56, 0x5CFF},
    {0x3D58, 0xBF10},
    {0x3D5A, 0x1681},
    {0x3D5C, 0x0802},
    {0x3D5E, 0x8000},
    {0x3D60, 0x141C},
    {0x3D62, 0x6000},
    {0x3D64, 0x6022},
    {0x3D66, 0x4D80},
    {0x3D68, 0x5C97},
    {0x3D6A, 0x6A69},
    {0x3D6C, 0xAC6F},
    {0x3D6E, 0x4645},
    {0x3D70, 0x4400},
    {0x3D72, 0x0513},
    {0x3D74, 0x8069},
    {0x3D76, 0x6AC6},
    {0x3D78, 0x5F95},
    {0x3D7A, 0x5F70},
    {0x3D7C, 0x8040},
    {0x3D7E, 0x4A81},
    {0x3D80, 0x0300},
    {0x3D82, 0xE703},
    {0x3D84, 0x0088},
    {0x3D86, 0x4A83},
    {0x3D88, 0x40FF},
    {0x3D8A, 0xFFFF},
    {0x3D8C, 0xFD70},
    {0x3D8E, 0x8040},
    {0x3D90, 0x4A85},
    {0x3D92, 0x4FA8},
    {0x3D94, 0x4F8C},
    {0x3D96, 0x0070},
    {0x3D98, 0xBE47},
    {0x3D9A, 0x8847},
    {0x3D9C, 0xBC78},
    {0x3D9E, 0x6B89},
    {0x3DA0, 0x6A80},
    {0x3DA2, 0x6986},
    {0x3DA4, 0x6B8E},
    {0x3DA6, 0x6B80},
    {0x3DA8, 0x6980},
    {0x3DAA, 0x6A88},
    {0x3DAC, 0x7C9F},
    {0x3DAE, 0x866B},
    {0x3DB0, 0x8765},
    {0x3DB2, 0x46FF},
    {0x3DB4, 0xE365},
    {0x3DB6, 0xA679},
    {0x3DB8, 0x4A40},
    {0x3DBA, 0x4580},
    {0x3DBC, 0x44BC},
    {0x3DBE, 0x7000},
    {0x3DC0, 0x8040},
    {0x3DC2, 0x0802},
    {0x3DC4, 0x10EF},
    {0x3DC6, 0x0104},
    {0x3DC8, 0x3860},
    {0x3DCA, 0x5D5D},
    {0x3DCC, 0x5682},
    {0x3DCE, 0x1300},
    {0x3DD0, 0x8648},
    {0x3DD2, 0x8202},
    {0x3DD4, 0x8082},
    {0x3DD6, 0x598A},
    {0x3DD8, 0x0280},
    {0x3DDA, 0x2048},
    {0x3DDC, 0x3060},
    {0x3DDE, 0x8042},
    {0x3DE0, 0x9259},
    {0x3DE2, 0x865A},
    {0x3DE4, 0x8258},
    {0x3DE6, 0x8562},
    {0x3DE8, 0x8062},
    {0x3DEA, 0x8560},
    {0x3DEC, 0x9257},
    {0x3DEE, 0x8221},
    {0x3DF0, 0x10FF},
    {0x3DF2, 0xB757},
    {0x3DF4, 0x9361},
    {0x3DF6, 0x1019},
    {0x3DF8, 0x8020},
    {0x3DFA, 0x9043},
    {0x3DFC, 0x8E43},
    {0x3DFE, 0x845F},
    {0x3E00, 0x835D},
    {0x3E02, 0x805D},
    {0x3E04, 0x8163},
    {0x3E06, 0x8063},
    {0x3E08, 0xA060},
    {0x3E0A, 0x9157},
    {0x3E0C, 0x8260},
    {0x3E0E, 0x5CFF},
    {0x3E10, 0xFFFF},
    {0x3E12, 0xFFE5},
    {0x3E14, 0x1016},
    {0x3E16, 0x2048},
    {0x3E18, 0x0802},
    {0x3E1A, 0x1C60},
    {0x3E1C, 0x0014},
    {0x3E1E, 0x0060},
    {0x3E20, 0x2205},
    {0x3E22, 0x8120},
    {0x3E24, 0x908F},
    {0x3E26, 0x6A80},
    {0x3E28, 0x6982},
    {0x3E2A, 0x5F9F},
    {0x3E2C, 0x6F46},
    {0x3E2E, 0x4544},
    {0x3E30, 0x0005},
    {0x3E32, 0x8013},
    {0x3E34, 0x8069},
    {0x3E36, 0x6A80},
    {0x3E38, 0x7000},
    {0x3E3A, 0x0000},
    {0x3E3C, 0x0000},
    {0x3E3E, 0x0000},
    {0x3E40, 0x0000},
    {0x3E42, 0x0000},
    {0x3E44, 0x0000},
    {0x3E46, 0x0000},
    {0x3E48, 0x0000},
    {0x3E4A, 0x0000},
    {0x3E4C, 0x0000},
    {0x3E4E, 0x0000},
    {0x3E50, 0x0000},
    {0x3E52, 0x0000},
    {0x3E54, 0x0000},
    {0x3E56, 0x0000},
    {0x3E58, 0x0000},
    {0x3E5A, 0x0000},
    {0x3E5C, 0x0000},
    {0x3E5E, 0x0000},
    {0x3E60, 0x0000},
    {0x3E62, 0x0000},
    {0x3E64, 0x0000},
    {0x3E66, 0x0000},
    {0x3E68, 0x0000},
    {0x3E6A, 0x0000},
    {0x3E6C, 0x0000},
    {0x3E6E, 0x0000},
    {0x3E70, 0x0000},
    {0x3E72, 0x0000},
    {0x3E74, 0x0000},
    {0x3E76, 0x0000},
    {0x3E78, 0x0000},
    {0x3E7A, 0x0000},
    {0x3E7C, 0x0000},
    {0x3E7E, 0x0000},
    {0x3E80, 0x0000},
    {0x3E82, 0x0000},
    {0x3E84, 0x0000},
    {0x3E86, 0x0000},
    {0x3E88, 0x0000},
    {0x3E8A, 0x0000},
    {0x3E8C, 0x0000},
    {0x3E8E, 0x0000},
    {0x3E90, 0x0000},
    {0x3E92, 0x0000},
    {0x3E94, 0x0000},
    {0x3E96, 0x0000},
    {0x3E98, 0x0000},
    {0x3E9A, 0x0000},
    {0x3E9C, 0x0000},
    {0x3E9E, 0x0000},
    {0x3EA0, 0x0000},
    {0x3EA2, 0x0000},
    {0x3EA4, 0x0000},
    {0x3EA6, 0x0000},
    {0x3EA8, 0x0000},
    {0x3EAA, 0x0000},
    {0x3EAC, 0x0000},
    {0x3EAE, 0x0000},
    {0x3EB0, 0x0000},
    {0x3EB2, 0x0000},
    {0x3EB4, 0x0000},
    {0x3EB6, 0x004C},    // DAC_LD_0_1
    {0x3EBA, 0xAAAA},    // DAC_LD_4_5
    {0x3EBC, 0x0086},    // DAC_LD_6_7
    {0x3EC0, 0x1E00},    // DAC_LD_10_11
    {0x3EC2, 0x100B},    // DAC_LD_12_13
    {0x3EC4, 0x3300},    // DAC_LD_14_15
    {0x3EC6, 0xEA44},    // DAC_LD_16_17
    {0x3EC8, 0x6F6F},    // DAC_LD_18_19
    {0x3ECA, 0x2F4A},    // DAC_LD_20_21
    {0x3ECC, 0x0506},    // DAC_LD_22_23
    {0x3ECE, 0x203B},    // DAC_LD_24_25
    {0x3ED0, 0x13F0},    // DAC_LD_26_27
    {0x3ED2, 0x9A3D},    // DAC_LD_28_29
    {0x3ED4, 0x862F},    // DAC_LD_30_31
    {0x3ED6, 0x4081},    // DAC_LD_32_33
    {0x3ED8, 0x4003},    // DAC_LD_34_35
    {0x3EDA, 0x9A80},    // DAC_LD_36_37
    {0x3EDC, 0xC000},    // DAC_LD_38_39
    {0x3EDE, 0xC303},    // DAC_LD_40_41
    {0x3426, 0x1600},    // AD_RESERVE_1_0
    {0x342A, 0x0038},    // PULSE_CONFIG
    {0x3F3E, 0x0001},    // ANALOG_CONTROL10
    {0x341A, 0x6051},    // AD_SH1_0
    {0x3420, 0x6051},    // AD_SH2_0
    {0x3EC2, 0x100A},    // DAC_LD_12_13
    {0x3ED8, 0x8003},    // DAC_LD_34_35
    {0x341A, 0x4735},    // AD_SH1_0
    {0x3420, 0x4735},    // AD_SH2_0
    {0x3426, 0x8A1A},    // AD_RESERVE_1_0
    {0x342A, 0x0018},    // PULSE_CONFIG
    {0x3ED2, 0xA53D},    // DAC_LD_28_29
    {0x3EDA, 0xA580},    // DAC_LD_36_37
    {0x3EBA, 0xAAAD},    // DAC_LD_4_5
    {0x3EB6, 0x004C},    // DAC_LD_0_1
    {0x3EDE, 0xC403},    // DAC_LD_40_41
    {0x3F3E, 0x0000},    // ANALOG_CONTROL10
    {0x31AE, 0x0202},
    {0x31B0, 0x0051},    // FRAME_PREAMBLE = 81
    {0x31B2, 0x0029},    // LINE_PREAMBLE = 41
    {0x31B4, 0x2350},    // MIPI_TIMING_0 = 9040
    {0x31B6, 0x1368},    // MIPI_TIMING_1 = 4968
    {0x31B8, 0x2012},    // MIPI_TIMING_2 = 8210
    {0x31BA, 0x1863},    // MIPI_TIMING_3 = 6243
    {0x31BC, 0x8589},    // MIPI_TIMING_4 = 34185
    {0x0112, 0x0A0A},    // CCP_DATA_FORMAT
    {0x0300, 0x0005},    // VT_PIX_CLK_DIV
    {0x0302, 0x0001},    // VT_SYS_CLK_DIV
    {0x0304, 0x0303},    // PRE_PLL_CLK_DIV
    {0x0306, 0x7359},    // PLL_MULTIPLIER
    {0x0308, 0x000A},    // OP_PIX_CLK_DIV
    {0x030A, 0x0001},    // OP_SYS_CLK_DIV
    {0x3016, 0x0101},    // ROW_SPEED
    {0x3004, 0x0008},    // X_ADDR_START_
    {0x3008, 0x0A27},    // X_ADDR_END_
    {0x3002, 0x0008},    // Y_ADDR_START_
    {0x3006, 0x079F},    // Y_ADDR_END_
    {0x3040, 0x0041},    // READ_MODE
    {0x317A, 0x416E},    // ANALOG_CONTROL6
    {0x3F3C, 0x0003},    // ANALOG_CONTROL9
    {0x034C, 0x0A20},    // X_OUTPUT_SIZE
    {0x034E, 0x0798},    // Y_OUTPUT_SIZE
    {0x300C, 0x1178},    // LINE_LENGTH_PCK_4472
    {0x300A, 0x092A},    // FRAME_LENGTH_LINES_2346
    {0x301A, 0x0210},    // RESET_REGISTER
    {0x301E, 0x00A8},    // DATA_PEDESTAL_
    {0x301A, 0x0218},    // RESET_REGISTER
    {0x30EE, 0x1136},    // DARK_CONTROL3
    {0x3F2C, 0x442E},    // GTH_THRES_RTN
    {0x301A, 0x0210},    // RESET_REGISTER
    {0x301E, 0x00AA},    // DATA_PEDESTAL_
    {0x301A, 0x0218},    // RESET_REGISTER
    {0x3120, 0x0005},    // DITHER_CONTROL
    {0x300C, 0x15D4},    // LINE_LENGTH_PCK_5588
    {0x300A, 0x0890},    // FRAME_LENGTH_LINES_2192
    {0x301A, 0x0218},    // RESET_REGISTER
    {AR0522_REG_END, 0x00},    /* END MARKER */
};

static struct regval_list ar0522_init_regs_mipi_2560_1080_30fps[] ={
/*
 *
 *  Register Log created on Tuesday, December 17, 2019 : 10:03:18
 *
 * [Register Log 12/17/19 10:03:18]
 *
 *  XMCLK = 24000000
 *
 */
    {AR0522_REG_DELAY,100},
    {0x301A, 0x00D9},
    {AR0522_REG_DELAY,100},
    {AR0522_REG_DELAY,100},
    {0x301A, 0x30D8},
    {0x3042, 0x0004},    // DARK_CONTROL2
    {0x3044, 0x4580},    // DARK_CONTROL
    {0x30EE, 0x1136},    // DARK_CONTROL3
    {0x3120, 0x0001},    // DITHER_CONTROL
    {0x3F2C, 0x442E},    // GTH_THRES_RTN
    {0x30D2, 0x0000},   // CRM_CONTROL
    {0x30D4, 0x0000},    // COLUMN_CORRECTION
    {0x30D6, 0x2FFF},    // COLUMN_CORRECTION2
    {0x30DA, 0x0FFF},    // COLUMN_CORRECTION_CLIP2
    {0x30DC, 0x0FFF},    // COLUMN_CORRECTION_CLIP3
    {0x30DE, 0x0000},    // COLUMN_GROUP_CORRECTION
    {0x31E0, 0x0781},    // PIX_DEF_ID
    {0x3180, 0x9434},    // FINE_DIG_CORRECTION_CONTROL
    {0x3172, 0x0206},    // ANALOG_CONTROL2
    {0x3F00, 0x0017},    // BM_T0
    {0x3F02, 0x02DD},    // BM_T1
    {0x3F04, 0x0020},    // NOISE_GAIN_THRESHOLD0
    {0x3F06, 0x0040},    // NOISE_GAIN_THRESHOLD1
    {0x3F08, 0x0070},    // NOISE_GAIN_THRESHOLD2
    {0x3F0A, 0x0101},    // NOISE_FLOOR10
    {0x3F0C, 0x0302},    // NOISE_FLOOR32
    {0x3F1E, 0x0022},    // NOISE_COEF
    {0x3F1A, 0x0103},    // CROSSFACTOR2
    {0x3F14, 0x0505},    // SINGLE_K_FACTOR2
    {0x3F44, 0x1515},    // COUPLE_K_FACTOR2
    {0x3F18, 0x0103},    // CROSSFACTOR1
    {0x3F12, 0x0505},    // SINGLE_K_FACTOR1
    {0x3F42, 0x1515},    // COUPLE_K_FACTOR1
    {0x3F16, 0x0103},    // CROSSFACTOR0
    {0x3F10, 0x0505},    // SINGLE_K_FACTOR0
    {0x3F40, 0x1515},    // COUPLE_K_FACTOR0
    {0x3D00, 0x043E},
    {0x3D02, 0x4760},
    {0x3D04, 0xFFFF},
    {0x3D06, 0xFFFF},
    {0x3D08, 0x8000},
    {0x3D0A, 0x0510},
    {0x3D0C, 0xAF08},
    {0x3D0E, 0x0252},
    {0x3D10, 0x486F},
    {0x3D12, 0x5D5D},
    {0x3D14, 0x8056},
    {0x3D16, 0x8313},
    {0x3D18, 0x0087},
    {0x3D1A, 0x6A48},
    {0x3D1C, 0x6982},
    {0x3D1E, 0x0280},
    {0x3D20, 0x8359},
    {0x3D22, 0x8D02},
    {0x3D24, 0x8020},
    {0x3D26, 0x4882},
    {0x3D28, 0x4269},
    {0x3D2A, 0x6A95},
    {0x3D2C, 0x5988},
    {0x3D2E, 0x5A83},
    {0x3D30, 0x5885},
    {0x3D32, 0x6280},
    {0x3D34, 0x6289},
    {0x3D36, 0x6097},
    {0x3D38, 0x5782},
    {0x3D3A, 0x605C},
    {0x3D3C, 0xBF18},
    {0x3D3E, 0x0961},
    {0x3D40, 0x5080},
    {0x3D42, 0x2090},
    {0x3D44, 0x4390},
    {0x3D46, 0x4382},
    {0x3D48, 0x5F8A},
    {0x3D4A, 0x5D5D},
    {0x3D4C, 0x9C63},
    {0x3D4E, 0x8063},
    {0x3D50, 0xA960},
    {0x3D52, 0x9757},
    {0x3D54, 0x8260},
    {0x3D56, 0x5CFF},
    {0x3D58, 0xBF10},
    {0x3D5A, 0x1681},
    {0x3D5C, 0x0802},
    {0x3D5E, 0x8000},
    {0x3D60, 0x141C},
    {0x3D62, 0x6000},
    {0x3D64, 0x6022},
    {0x3D66, 0x4D80},
    {0x3D68, 0x5C97},
    {0x3D6A, 0x6A69},
    {0x3D6C, 0xAC6F},
    {0x3D6E, 0x4645},
    {0x3D70, 0x4400},
    {0x3D72, 0x0513},
    {0x3D74, 0x8069},
    {0x3D76, 0x6AC6},
    {0x3D78, 0x5F95},
    {0x3D7A, 0x5F70},
    {0x3D7C, 0x8040},
    {0x3D7E, 0x4A81},
    {0x3D80, 0x0300},
    {0x3D82, 0xE703},
    {0x3D84, 0x0088},
    {0x3D86, 0x4A83},
    {0x3D88, 0x40FF},
    {0x3D8A, 0xFFFF},
    {0x3D8C, 0xFD70},
    {0x3D8E, 0x8040},
    {0x3D90, 0x4A85},
    {0x3D92, 0x4FA8},
    {0x3D94, 0x4F8C},
    {0x3D96, 0x0070},
    {0x3D98, 0xBE47},
    {0x3D9A, 0x8847},
    {0x3D9C, 0xBC78},
    {0x3D9E, 0x6B89},
    {0x3DA0, 0x6A80},
    {0x3DA2, 0x6986},
    {0x3DA4, 0x6B8E},
    {0x3DA6, 0x6B80},
    {0x3DA8, 0x6980},
    {0x3DAA, 0x6A88},
    {0x3DAC, 0x7C9F},
    {0x3DAE, 0x866B},
    {0x3DB0, 0x8765},
    {0x3DB2, 0x46FF},
    {0x3DB4, 0xE365},
    {0x3DB6, 0xA679},
    {0x3DB8, 0x4A40},
    {0x3DBA, 0x4580},
    {0x3DBC, 0x44BC},
    {0x3DBE, 0x7000},
    {0x3DC0, 0x8040},
    {0x3DC2, 0x0802},
    {0x3DC4, 0x10EF},
    {0x3DC6, 0x0104},
    {0x3DC8, 0x3860},
    {0x3DCA, 0x5D5D},
    {0x3DCC, 0x5682},
    {0x3DCE, 0x1300},
    {0x3DD0, 0x8648},
    {0x3DD2, 0x8202},
    {0x3DD4, 0x8082},
    {0x3DD6, 0x598A},
    {0x3DD8, 0x0280},
    {0x3DDA, 0x2048},
    {0x3DDC, 0x3060},
    {0x3DDE, 0x8042},
    {0x3DE0, 0x9259},
    {0x3DE2, 0x865A},
    {0x3DE4, 0x8258},
    {0x3DE6, 0x8562},
    {0x3DE8, 0x8062},
    {0x3DEA, 0x8560},
    {0x3DEC, 0x9257},
    {0x3DEE, 0x8221},
    {0x3DF0, 0x10FF},
    {0x3DF2, 0xB757},
    {0x3DF4, 0x9361},
    {0x3DF6, 0x1019},
    {0x3DF8, 0x8020},
    {0x3DFA, 0x9043},
    {0x3DFC, 0x8E43},
    {0x3DFE, 0x845F},
    {0x3E00, 0x835D},
    {0x3E02, 0x805D},
    {0x3E04, 0x8163},
    {0x3E06, 0x8063},
    {0x3E08, 0xA060},
    {0x3E0A, 0x9157},
    {0x3E0C, 0x8260},
    {0x3E0E, 0x5CFF},
    {0x3E10, 0xFFFF},
    {0x3E12, 0xFFE5},
    {0x3E14, 0x1016},
    {0x3E16, 0x2048},
    {0x3E18, 0x0802},
    {0x3E1A, 0x1C60},
    {0x3E1C, 0x0014},
    {0x3E1E, 0x0060},
    {0x3E20, 0x2205},
    {0x3E22, 0x8120},
    {0x3E24, 0x908F},
    {0x3E26, 0x6A80},
    {0x3E28, 0x6982},
    {0x3E2A, 0x5F9F},
    {0x3E2C, 0x6F46},
    {0x3E2E, 0x4544},
    {0x3E30, 0x0005},
    {0x3E32, 0x8013},
    {0x3E34, 0x8069},
    {0x3E36, 0x6A80},
    {0x3E38, 0x7000},
    {0x3E3A, 0x0000},
    {0x3E3C, 0x0000},
    {0x3E3E, 0x0000},
    {0x3E40, 0x0000},
    {0x3E42, 0x0000},
    {0x3E44, 0x0000},
    {0x3E46, 0x0000},
    {0x3E48, 0x0000},
    {0x3E4A, 0x0000},
    {0x3E4C, 0x0000},
    {0x3E4E, 0x0000},
    {0x3E50, 0x0000},
    {0x3E52, 0x0000},
    {0x3E54, 0x0000},
    {0x3E56, 0x0000},
    {0x3E58, 0x0000},
    {0x3E5A, 0x0000},
    {0x3E5C, 0x0000},
    {0x3E5E, 0x0000},
    {0x3E60, 0x0000},
    {0x3E62, 0x0000},
    {0x3E64, 0x0000},
    {0x3E66, 0x0000},
    {0x3E68, 0x0000},
    {0x3E6A, 0x0000},
    {0x3E6C, 0x0000},
    {0x3E6E, 0x0000},
    {0x3E70, 0x0000},
    {0x3E72, 0x0000},
    {0x3E74, 0x0000},
    {0x3E76, 0x0000},
    {0x3E78, 0x0000},
    {0x3E7A, 0x0000},
    {0x3E7C, 0x0000},
    {0x3E7E, 0x0000},
    {0x3E80, 0x0000},
    {0x3E82, 0x0000},
    {0x3E84, 0x0000},
    {0x3E86, 0x0000},
    {0x3E88, 0x0000},
    {0x3E8A, 0x0000},
    {0x3E8C, 0x0000},
    {0x3E8E, 0x0000},
    {0x3E90, 0x0000},
    {0x3E92, 0x0000},
    {0x3E94, 0x0000},
    {0x3E96, 0x0000},
    {0x3E98, 0x0000},
    {0x3E9A, 0x0000},
    {0x3E9C, 0x0000},
    {0x3E9E, 0x0000},
    {0x3EA0, 0x0000},
    {0x3EA2, 0x0000},
    {0x3EA4, 0x0000},
    {0x3EA6, 0x0000},
    {0x3EA8, 0x0000},
    {0x3EAA, 0x0000},
    {0x3EAC, 0x0000},
    {0x3EAE, 0x0000},
    {0x3EB0, 0x0000},
    {0x3EB2, 0x0000},
    {0x3EB4, 0x0000},
    {0x3EB6, 0x004C},    // DAC_LD_0_1
    {0x3EBA, 0xAAAA},    // DAC_LD_4_5
    {0x3EBC, 0x0086},    // DAC_LD_6_7
    {0x3EC0, 0x1E00},    // DAC_LD_10_11
    {0x3EC2, 0x100B},    // DAC_LD_12_13
    {0x3EC4, 0x3300},    // DAC_LD_14_15
    {0x3EC6, 0xEA44},    // DAC_LD_16_17
    {0x3EC8, 0x6F6F},    // DAC_LD_18_19
    {0x3ECA, 0x2F4A},    // DAC_LD_20_21
    {0x3ECC, 0x0506},    // DAC_LD_22_23
    {0x3ECE, 0x203B},    // DAC_LD_24_25
    {0x3ED0, 0x13F0},    // DAC_LD_26_27
    {0x3ED2, 0x9A3D},    // DAC_LD_28_29
    {0x3ED4, 0x862F},    // DAC_LD_30_31
    {0x3ED6, 0x4081},    // DAC_LD_32_33
    {0x3ED8, 0x4003},    // DAC_LD_34_35
    {0x3EDA, 0x9A80},    // DAC_LD_36_37
    {0x3EDC, 0xC000},    // DAC_LD_38_39
    {0x3EDE, 0xC303},    // DAC_LD_40_41
    {0x3426, 0x1600},    // AD_RESERVE_1_0
    {0x342A, 0x0038},    // PULSE_CONFIG
    {0x3F3E, 0x0001},    // ANALOG_CONTROL10
    {0x341A, 0x6051},    // AD_SH1_0
    {0x3420, 0x6051},    // AD_SH2_0
    {0x3EC2, 0x100A},    // DAC_LD_12_13
    {0x3ED8, 0x8003},    // DAC_LD_34_35
    {0x341A, 0x4735},    // AD_SH1_0
    {0x3420, 0x4735},    // AD_SH2_0
    {0x3426, 0x8A1A},    // AD_RESERVE_1_0
    {0x342A, 0x0018},    // PULSE_CONFIG
    {0x3ED2, 0xA53D},    // DAC_LD_28_29
    {0x3EDA, 0xA580},    // DAC_LD_36_37
    {0x3EBA, 0xAAAD},    // DAC_LD_4_5
    {0x3EB6, 0x004C},    // DAC_LD_0_1
    {0x3EDE, 0xC403},    // DAC_LD_40_41
    {0x3F3E, 0x0000},    // ANALOG_CONTROL10
    {0x31AE, 0x0202},
    {0x31B0, 0x0051},    // FRAME_PREAMBLE = 81
    {0x31B2, 0x0029},    // LINE_PREAMBLE = 41
    {0x31B4, 0x2350},    // MIPI_TIMING_0 = 9040
    {0x31B6, 0x1368},    // MIPI_TIMING_1 = 4968
    {0x31B8, 0x2012},    // MIPI_TIMING_2 = 8210
    {0x31BA, 0x1863},    // MIPI_TIMING_3 = 6243
    {0x31BC, 0x8589},    // MIPI_TIMING_4 = 34185
    {0x0110, 0x00 },
    {0x0112, 0x0808},    // CCP_DATA_FORMAT
    {0x0300, 0x0004},    // VT_PIX_CLK_DIV
    {0x0302, 0x0001},    // VT_SYS_CLK_DIV
    {0x0304, 0x0303},    // PRE_PLL_CLK_DIV
    {0x0306, 0x5C59},    // PLL_MULTIPLIER
    {0x0308, 0x0008},    // OP_PIX_CLK_DIV
    {0x030A, 0x0001},    // OP_SYS_CLK_DIV
    {0x3016, 0x0101},    // ROW_SPEED
    {0x3004, 0x0008},    // X_ADDR_START_
    {0x3008, 0x0A07},    // X_ADDR_END_
    {0x3002, 0x0008},    // Y_ADDR_START_
    {0x3006, 0x043F},    // Y_ADDR_END_
    {0x3040, 0x0041},    // READ_MODE
    {0x317A, 0x416E},    // ANALOG_CONTROL6
    {0x3F3C, 0x0003},    // ANALOG_CONTROL9
    {0x034C, 0x0A00},    // X_OUTPUT_SIZE
    {0x034E, 0x0438},    // Y_OUTPUT_SIZE
    {0x300C, 0x1178},    // LINE_LENGTH_PCK_4472
    {0x300A, 0x092A},    // FRAME_LENGTH_LINES_2346
    {0x301A, 0x0210},    // RESET_REGISTER
    {0x301E, 0x00A8},    // DATA_PEDESTAL_
    {0x301A, 0x0218},    // RESET_REGISTER
    {0x30EE, 0x1136},    // DARK_CONTROL3
    {0x3F2C, 0x442E},    // GTH_THRES_RTN
    {0x301A, 0x0210},    // RESET_REGISTER
    {0x301E, 0x00AA},    // DATA_PEDESTAL_
    {0x301A, 0x0218},    // RESET_REGISTER
    {0x3120, 0x0005},    // DITHER_CONTROL
    {0x300C, 0x15D4},    // LINE_LENGTH_PCK_
    {0x300A, 0x0890},    // FRAME_LENGTH_LINES_
    {0x301A, 0x0218},    // RESET_REGISTER
    {AR0522_REG_END, 0x00},    /* END MARKER */
};

/*
 * the order of the ar0522_win_sizes is [full_resolution, preview_resolution].
 */
static struct tx_isp_sensor_win_setting ar0522_win_sizes[] = {
    {
        .width                     = AR0522_WIDTH,
        .height                    = AR0522_HEIGHT,
        .fps                       = SENSOR_OUTPUT_MAX_FPS << 16 | 1,
        .mbus_code                 = V4L2_MBUS_FMT_SGRBG8_1X8,
        .colorspace                = V4L2_COLORSPACE_SRGB,
//      .regs                    = ar0522_init_regs_mipi_2592_1944_30fps,
        .regs                      = ar0522_init_regs_mipi_2560_1080_30fps,
    }
};

/*
 * the part of driver was fixed.
 */

static struct regval_list ar0522_stream_on[] = {
    {0x301A, 0x021C},    //RESET_REGISTER
    {AR0522_REG_END,0x0},    /* END MARKER */
};

static struct regval_list ar0522_stream_off[] = {
    {0x301A, 0x0218},    //RESET_REGISTER
    {AR0522_REG_END,0x0},    /* END MARKER */
};

static int ar0522_read(struct tx_isp_subdev *sd, uint16_t reg,
        unsigned char *value)
{
//  uint16_t val;
    struct i2c_client *client = tx_isp_get_subdevdata(sd);
    unsigned char buf[2] = {reg >> 8, reg & 0xff};

    struct i2c_msg msg[2] = {
        [0] = {
            .addr     = client->addr,
            .flags    = 0,
            .len      = 2,
            .buf      = buf,
        },
        [1] = {
            .addr     = client->addr,
            .flags    = I2C_M_RD,
            .len      = 2,
            .buf      = value,
        }
    };

    int ret;
//    val=(buf[0]<<8)|buf[1];
//    printk("reg_num:0x%2x, val:0x%2x.\n",reg,val);
    ret = private_i2c_transfer(client->adapter, msg, 2);
    if (ret > 0)
        ret = 0;

    return ret;

}

static int ar0522_write(struct tx_isp_subdev *sd, uint16_t reg,
        unsigned short value)
{
    struct i2c_client *client = tx_isp_get_subdevdata(sd);
    uint8_t buf[4] = {reg >> 8, reg & 0xff, value >> 8, value & 0xff};
    struct i2c_msg msg = {
        .addr    = client->addr,
        .flags    = 0,
        .len    = 4,
        .buf    = buf,
    };

    int ret;
    ret = private_i2c_transfer(client->adapter, &msg, 1);
    if (ret > 0)
        ret = 0;

    return ret;
}

static inline int ar0522_read_array(struct tx_isp_subdev *sd, struct regval_list *vals)
{
    int ret;
    unsigned char val[2];
//    uint16_t value;
//    printk("=======ar0522_read_array=========.\n");

    while (vals->reg_num != AR0522_REG_END) {
        if (vals->reg_num == AR0522_REG_DELAY) {
        private_msleep(vals->value);
//        printk("ar0522 meleep %d.\n",vals->value);
        } else {
            ret = ar0522_read(sd, vals->reg_num, val);
//        value=(val[0]<<8)|val[1];
//        printk("reg_num:0x%x, val:0x%x.\n",vals->reg_num,value);
            if (ret < 0)
                return ret;
        }
        vals++;
    }

    return 0;
}

static int ar0522_write_array(struct tx_isp_subdev *sd, struct regval_list *vals)
{
    int ret;

    while (vals->reg_num != AR0522_REG_END) {
        if (vals->reg_num == AR0522_REG_DELAY) {
             private_msleep(vals->value);
        } else {
            ret = ar0522_write(sd, vals->reg_num, vals->value);
//            printk("reg_num 0x%x,value 0x%x.\n",vals->reg_num,vals->value);
            if (ret < 0)
                return ret;
        }

        vals++;
    }

    return 0;
}

static int ar0522_reset(struct tx_isp_subdev *sd, int val)
{
    return 0;
}

static int ar0522_detect(struct tx_isp_subdev *sd, unsigned int *ident)
{
    unsigned char v[2]={0};
    int ret;

    ret = ar0522_read(sd, 0x0000, v);
    printk("addr H 0x%x,addr L 0x%x.\n",v[0],v[1]);
    if (ret < 0)
        return ret;

    if (v[0] != AR0522_CHIP_ID_H)
        return -ENODEV;

    if (v[1] != AR0522_CHIP_ID_L)
        return -ENODEV;

    return 0;

}

static int ar0522_set_integration_time(struct tx_isp_subdev *sd, int value)
{
    int ret;

    ret = ar0522_write(sd,0x3012, value);
    if (ret < 0)
        return ret;

    return 0;

}
/**************************************
  analog gain P0:0x3060
**************************************/
static int ar0522_set_analog_gain(struct tx_isp_subdev *sd, u16 value)
{
    int ret;

    ret = ar0522_write(sd,0x3056, value);
    if (ret < 0)
        return ret;

    return 0;
}

static int ar0522_set_digital_gain(struct tx_isp_subdev *sd, int value)
{
    return 0;
}

static int ar0522_get_black_pedestal(struct tx_isp_subdev *sd, int value)
{
    return 0;
}

static int ar0522_init(struct tx_isp_subdev *sd, int enable)
{
    struct tx_isp_sensor *sensor = (container_of(sd, struct tx_isp_sensor, sd));
    struct tx_isp_sensor_win_setting *wsize = &ar0522_win_sizes[0];
    int ret = 0;

    if(!enable)
        return ISP_SUCCESS;

    sensor->video.mbus.width                  = wsize->width;
    sensor->video.mbus.height                 = wsize->height;
    sensor->video.mbus.code                   = wsize->mbus_code;
    sensor->video.mbus.field                  = V4L2_FIELD_NONE;
    sensor->video.mbus.colorspace             = wsize->colorspace;
    sensor->video.fps                         = wsize->fps;

    ret = ar0522_write_array(sd, wsize->regs);
    if (ret)
        return ret;

    ret = ar0522_read_array(sd,wsize->regs);
    ret = tx_isp_call_subdev_notify(sd, TX_ISP_EVENT_SYNC_SENSOR_ATTR, &sensor->video);
    sensor->priv = wsize;

    return 0;
}

static int ar0522_s_stream(struct tx_isp_subdev *sd, int enable)
{
    int ret = 0;

    if (enable) {
        ret = ar0522_write_array(sd, ar0522_stream_on);
        printk("ar0522 stream on\n");
    }
    else {
        ret = ar0522_write_array(sd, ar0522_stream_off);
        printk("ar0522 stream off\n");
    }

    return ret;
}

static int ar0522_set_fps(struct tx_isp_subdev *sd, int fps)
{
    struct tx_isp_sensor *sensor = sd_to_sensor_device(sd);
    unsigned int wpclk = 0;
    unsigned short vts = 0;
    unsigned short hts=0;
    unsigned int max_fps = 0;
    unsigned char tmp[2];
    unsigned int newformat = 0;
    int ret = 0;

    wpclk = AR0522_SUPPORT_30FPS_SCLK;
    max_fps = SENSOR_OUTPUT_MAX_FPS;
    newformat = (((fps >> 16) / (fps & 0xffff)) << 8) + ((((fps >> 16) % (fps & 0xffff)) << 8) / (fps & 0xffff));

    if(newformat > (max_fps << 8) || newformat < (SENSOR_OUTPUT_MIN_FPS << 8)){
        printk("warn: fps(%x) no in range\n", fps);
        return -1;
    }

    ret += ar0522_read(sd, 0x300C, tmp);
    hts =tmp[0];
    if(ret < 0)
        return -1;

    hts = (hts << 8) + tmp[1];
    vts = wpclk * (fps & 0xffff) / hts / ((fps & 0xffff0000) >> 16);
    ret = ar0522_write(sd, 0x300A, vts);
    if(ret < 0)
        return -1;

    sensor->video.fps                                    = fps;
    sensor->video.attr->max_integration_time_native      = vts - 4;
    sensor->video.attr->integration_time_limit           = vts - 4;
    sensor->video.attr->total_height                     = vts;
    sensor->video.attr->max_integration_time             = vts - 4;

    ret = tx_isp_call_subdev_notify(sd, TX_ISP_EVENT_SYNC_SENSOR_ATTR, &sensor->video);

    return ret;
}

static int ar0522_set_mode(struct tx_isp_subdev *sd, int value)
{
    struct tx_isp_sensor *sensor = sd_to_sensor_device(sd);
    struct tx_isp_sensor_win_setting *wsize = NULL;
    int ret = ISP_SUCCESS;

    if(value == TX_ISP_SENSOR_FULL_RES_MAX_FPS){
        wsize = &ar0522_win_sizes[0];
    }else if(value == TX_ISP_SENSOR_PREVIEW_RES_MAX_FPS){
        wsize = &ar0522_win_sizes[0];
    }

    if(wsize){
        sensor->video.mbus.width          = wsize->width;
        sensor->video.mbus.height         = wsize->height;
        sensor->video.mbus.code           = wsize->mbus_code;
        sensor->video.mbus.field          = V4L2_FIELD_NONE;
        sensor->video.mbus.colorspace     = wsize->colorspace;
        sensor->video.fps                 = wsize->fps;
        ret = tx_isp_call_subdev_notify(sd, TX_ISP_EVENT_SYNC_SENSOR_ATTR, &sensor->video);
    }

    return ret;
}

static int ar0522_set_vflip(struct tx_isp_subdev *sd, int enable)
{
    struct tx_isp_sensor *sensor = sd_to_sensor_device(sd);
    int ret = 0;
    unsigned char val = 0xc0;

    ret = ar0522_write(sd, 0xfe, 0x00);
    if (enable){
        val &= ~0x3;
        val |= 0x2;
        sensor->video.mbus.code = V4L2_MBUS_FMT_SGBRG10_1X10;
    } else {
        val &= 0xfd;
        sensor->video.mbus.code = V4L2_MBUS_FMT_SRGGB10_1X10;
    }
    ret += ar0522_write(sd, 0x17, val);

    if(!ret)
        ret = tx_isp_call_subdev_notify(sd, TX_ISP_EVENT_SYNC_SENSOR_ATTR, &sensor->video);

    return ret;
}

static int ar0522_g_chip_ident(struct tx_isp_subdev *sd,
        struct tx_isp_chip_ident *chip)
{
    struct i2c_client *client = tx_isp_get_subdevdata(sd);
    unsigned int ident = 0;
    int ret = ISP_SUCCESS;

    if (power_gpio != -1) {
        ret = gpio_request(power_gpio, "ar0522_power");
        if (!ret) {
            private_gpio_direction_output(power_gpio, 1);
            private_msleep(50);
        }
        else
            printk("gpio requrest fail %d\n", power_gpio);
    }

    if(pwdn_gpio != -1){
        ret = private_gpio_request(pwdn_gpio,"ar0522_pwdn");
        if(!ret){
//            private_gpio_direction_output(pwdn_gpio, 0);
//            private_msleep(10);
        private_gpio_direction_output(pwdn_gpio, 1);
            private_msleep(150);
        }else{
            printk("gpio requrest fail %d\n",pwdn_gpio);
        }
    }

    if(reset_gpio != -1){
        ret = private_gpio_request(reset_gpio,"ar0522_reset");
        if(!ret){
            private_gpio_direction_output(reset_gpio, 1);
            private_msleep(20);
            private_gpio_direction_output(reset_gpio, 0);
            private_msleep(20);
            private_gpio_direction_output(reset_gpio, 1);
            private_msleep(20);
        }else{
            printk("gpio requrest fail %d\n",reset_gpio);
        }
    }

    if (i2c_sel1_gpio != -1) {
        int ret = gpio_request(i2c_sel1_gpio, "ar0522_i2c_sel1");
        if (!ret) {
            private_gpio_direction_output(i2c_sel1_gpio, 1);
        } else {
            printk("gpio requrest fail %d\n", i2c_sel1_gpio);
        }
    }

    private_msleep(50);
    ret = ar0522_detect(sd, &ident);

    if (ret) {
        printk("chip found @ 0x%x (%s) is not an ar0522 chip.\n",
                client->addr, client->adapter->name);
        return ret;
    }

    printk("ar0522 chip found @ 0x%02x (%s)\n", client->addr, client->adapter->name);
    if(chip){
        memcpy(chip->name, "ar0522", sizeof("ar0522"));
        chip->ident = ident;
        chip->revision = SENSOR_VERSION;
    }

    return 0;
}

static int ar0522_sensor_ops_ioctl(struct tx_isp_subdev *sd, unsigned int cmd, void *arg)
{
    long ret = 0;

    if(IS_ERR_OR_NULL(sd)){
        printk("[%d]The pointer is invalid!\n", __LINE__);
        return -EINVAL;
    }

    switch(cmd){
        case TX_ISP_EVENT_SENSOR_INT_TIME:
            if(arg)
                ret = ar0522_set_integration_time(sd, *(int*)arg);
            break;
        case TX_ISP_EVENT_SENSOR_AGAIN:
            if(arg)
                ret = ar0522_set_analog_gain(sd, *(u16*)arg);
            break;
        case TX_ISP_EVENT_SENSOR_DGAIN:
            if(arg)
                ret = ar0522_set_digital_gain(sd, *(int*)arg);
            break;
        case TX_ISP_EVENT_SENSOR_BLACK_LEVEL:
            if(arg)
                ret = ar0522_get_black_pedestal(sd, *(int*)arg);
            break;
        case TX_ISP_EVENT_SENSOR_RESIZE:
            if(arg)
                ret = ar0522_set_mode(sd, *(int*)arg);
            break;
        case TX_ISP_EVENT_SENSOR_PREPARE_CHANGE:
            ret = ar0522_write_array(sd, ar0522_stream_off);
            break;
        case TX_ISP_EVENT_SENSOR_FINISH_CHANGE:
            ret = ar0522_write_array(sd, ar0522_stream_on);
            break;
        case TX_ISP_EVENT_SENSOR_FPS:
            if(arg)
                ret = ar0522_set_fps(sd, *(int*)arg);
            break;
        case TX_ISP_EVENT_SENSOR_VFLIP:
            if(arg)
                ret = ar0522_set_vflip(sd, *(int*)arg);
            break;
        default:
            break;;
    }

    return 0;
}

static int ar0522_g_register(struct tx_isp_subdev *sd, struct tx_isp_dbg_register *reg)
{
    unsigned char val = 0;
    int len = 0;
    int ret = 0;

    len = strlen(sd->chip.name);
    if(len && strncmp(sd->chip.name, reg->name, len)){
        return -EINVAL;
    }

    if (!private_capable(CAP_SYS_ADMIN))
        return -EPERM;

    ret = ar0522_read(sd, reg->reg & 0xff, &val);
    reg->val = val;
    reg->size = 1;

    return ret;
}

static int ar0522_s_register(struct tx_isp_subdev *sd, const struct tx_isp_dbg_register *reg)
{
    int len = 0;

    len = strlen(sd->chip.name);
    if(len && strncmp(sd->chip.name, reg->name, len)){
        return -EINVAL;
    }

    if (!private_capable(CAP_SYS_ADMIN))
        return -EPERM;

    ar0522_write(sd, reg->reg & 0xff, reg->val & 0xff);

    return 0;
}

static struct tx_isp_subdev_core_ops ar0522_core_ops = {
    .g_chip_ident              = ar0522_g_chip_ident,
    .reset                     = ar0522_reset,
    .init                      = ar0522_init,
    /*.ioctl                   = ar0522_ops_ioctl,*/
    .g_register                = ar0522_g_register,
    .s_register                = ar0522_s_register,
};

static struct tx_isp_subdev_video_ops ar0522_video_ops = {
    .s_stream                  = ar0522_s_stream,
};

static struct tx_isp_subdev_sensor_ops ar0522_sensor_ops = {
    .ioctl                     = ar0522_sensor_ops_ioctl,
};

static struct tx_isp_subdev_ops ar0522_ops = {
    .core                      = &ar0522_core_ops,
    .video                     = &ar0522_video_ops,
    .sensor                    = &ar0522_sensor_ops,
};

/* It's the sensor device */
static u64 tx_isp_module_dma_mask = ~(u64)0;
struct platform_device ar0522_platform_device = {
    .name                     = "ar0522",
    .id                       = -1,
    .dev = {
        .dma_mask = &tx_isp_module_dma_mask,
        .coherent_dma_mask = 0xffffffff,
        .platform_data = NULL,
    },
    .num_resources            = 0,
};

static int ar0522_probe(struct i2c_client *client, const struct i2c_device_id *id)
{
    struct tx_isp_subdev *sd;
    struct tx_isp_sensor *sensor;
    struct tx_isp_sensor_win_setting *wsize = &ar0522_win_sizes[0];

    printk("probe ok ----start-w %d h %d -->ar0522\n",AR0522_WIDTH, AR0522_HEIGHT);
    sensor = (struct tx_isp_sensor *)kzalloc(sizeof(*sensor), GFP_KERNEL);
    if(!sensor){
        printk("Failed to allocate sensor subdev.\n");
        return -ENOMEM;
    }

    memset(sensor, 0 ,sizeof(*sensor));

#ifndef MODULE
    ar0522_board_info = get_sensor_board_info(ar0522_platform_device.name);
    if (ar0522_board_info) {
        power_gpio     = ar0522_board_info->gpios.gpio_power;
        reset_gpio     = ar0522_board_info->gpios.gpio_sensor_rst;
        i2c_sel1_gpio  = ar0522_board_info->gpios.gpio_i2c_sel1;
        pwdn_gpio      = ar0522_board_info->gpios.gpio_sensor_pwdn;
    }
#endif
    //printk("detect gpio reset :%d,power :%d,pdwn:%d i2c_sel1_gpio:%d.\n",reset_gpio,power_gpio,pwdn_gpio,i2c_sel1_gpio);
    sensor->mclk = clk_get(NULL, "cgu_cim");

    if (IS_ERR(sensor->mclk)) {
        printk("Cannot get sensor input clock cgu_cim\n");

        goto err_get_mclk;
    }

    private_clk_set_rate(sensor->mclk, 20000000);
    private_clk_enable(sensor->mclk);
    printk("mclk request ok.\n");

     /*
        convert sensor-gain into isp-gain,
     */
    ar0522_attr.max_again = 262144;
    ar0522_attr.max_dgain = 0;
    sd = &sensor->sd;

    sensor->video.attr                   = &ar0522_attr;
    sensor->video.vi_max_width           = wsize->width;
    sensor->video.vi_max_height          = wsize->height;
    sensor->video.mbus.width             = wsize->width;
    sensor->video.mbus.height            = wsize->height;
    sensor->video.mbus.code              = wsize->mbus_code;
    sensor->video.mbus.field             = V4L2_FIELD_NONE;
    sensor->video.mbus.colorspace        = wsize->colorspace;
    sensor->video.fps                    = wsize->fps;
    sensor->video.dc_param               = NULL;

    tx_isp_subdev_init(&ar0522_platform_device, sd, &ar0522_ops);
    tx_isp_set_subdevdata(sd, client);
    tx_isp_set_subdev_hostdata(sd, sensor);
    private_i2c_set_clientdata(client, sd);
    pr_debug("probe ok ------->ar0522\n");

    return 0;
err_get_mclk:
    kfree(sensor);

    return -1;
}

static int ar0522_remove(struct i2c_client *client)
{
    struct tx_isp_subdev *sd = private_i2c_get_clientdata(client);
    struct tx_isp_sensor *sensor = tx_isp_get_subdev_hostdata(sd);

    if(power_gpio != -1) {
        private_gpio_direction_output(power_gpio, 0);
        private_gpio_free(power_gpio);
    }

    if(i2c_sel1_gpio != -1)
        private_gpio_free(i2c_sel1_gpio);

    if(pwdn_gpio != -1)
        private_gpio_free(pwdn_gpio);

    if(reset_gpio != -1)
        private_gpio_free(reset_gpio);

    private_clk_disable(sensor->mclk);
    private_clk_put(sensor->mclk);
    tx_isp_subdev_deinit(sd);
    kfree(sensor);

    return 0;
}

static const struct i2c_device_id ar0522_id[] = {
    { "ar0522", 0 },
    { }
};

MODULE_DEVICE_TABLE(i2c, ar0522_id);

static struct i2c_driver ar0522_driver = {
    .driver = {
        .owner            = THIS_MODULE,
        .name             = "ar0522",
    },
    .probe                = ar0522_probe,
    .remove               = ar0522_remove,
    .id_table             = ar0522_id,
};

static __init int init_ar0522(void)
{
    int ret = 0;
    ret = private_driver_get_interface();
    if(ret){
        printk("Failed to init ar0522 dirver.\n");
        return -1;
    }

    return private_i2c_add_driver(&ar0522_driver);
}

static __exit void exit_ar0522(void)
{
    private_i2c_del_driver(&ar0522_driver);
}

module_init(init_ar0522);
module_exit(exit_ar0522);

MODULE_DESCRIPTION("A low-level driver for Gcoreinc ar0522 sensors");
MODULE_LICENSE("GPL");

