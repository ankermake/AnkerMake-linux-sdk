/*
 * ar0144 Camera Driver
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

#define AR0144_CHIP_ID_H                (0x03)
#define AR0144_CHIP_ID_L                (0x56)
#define AR0144_CHIP_ID                  (0x0356)
#define AR0144_REG_END                  0xffff
#define AR0144_REG_DELAY                0xfffe

#define AR0144_SCLK                     (80000000)
#define SENSOR_OUTPUT_MAX_FPS           60
#define SENSOR_OUTPUT_MIN_FPS           5
#define SENSOR_VERSION                  "H20200818"

static int reset_gpio                   = GPIO_PA(9);
static int pwdn_gpio                    = -1;
static int gpio_i2c_sel1                = -1;
static int gpio_i2c_sel2                = -1;

module_param(reset_gpio, int, S_IRUGO);
MODULE_PARM_DESC(reset_gpio, "Reset GPIO NUM");

module_param(pwdn_gpio, int, S_IRUGO);
MODULE_PARM_DESC(pwdn_gpio, "Power down GPIO NUM");

module_param(gpio_i2c_sel1, int, S_IRUGO);
MODULE_PARM_DESC(pwdn_gpio, "I2C select1  GPIO NUM");

module_param(gpio_i2c_sel2, int, S_IRUGO);
MODULE_PARM_DESC(pwdn_gpio, "I2C select2  GPIO NUM");

static struct tx_isp_sensor_attribute ar0144_attr;
static struct sensor_board_info *ar0144_board_info;

struct regval_list {
    unsigned short reg_num;
    unsigned short value;
};

/*
 * the part of driver maybe modify about different sensor and different board.
 */
struct again_lut {
    unsigned int value;
    unsigned int gain;
};

static struct again_lut ar0144_again_lut[] = {
    { 0x0, 0 },
    { 0x1, 2794 },
    { 0x2, 6397 },
    { 0x3, 9011 },
    { 0x4, 12388 },
    { 0x5, 16447 },
    { 0x6, 19572 },
    { 0x7, 23340 },
    { 0x8, 26963 },
    { 0x9, 31135 },
    { 0xa, 35780 },
    { 0xb, 39588 },
    { 0xc, 44438 },
    { 0xd, 49051 },
    { 0xe, 54517 },
    { 0xf, 59685 },
    { 0x10, 65536 },
    { 0x12, 71490 },
    { 0x14, 78338 },
    { 0x16, 85108 },
    { 0x18, 92854 },
    { 0x1a, 100992 },
    { 0x1c, 109974 },
    { 0x1e, 120053 },
    { 0x20, 131072 },
    { 0x22, 137247 },
    { 0x24, 143667 },
    { 0x26, 150644 },
    { 0x28, 158212 },
    { 0x2a, 166528 },
    { 0x2c, 175510 },
    { 0x2e, 185457 },
    { 0x30, 196608 },
    { 0x32, 202783 },
    { 0x34, 209203 },
    { 0x36, 216276 },
    { 0x38, 223748 },
    { 0x3a, 232064 },
    { 0x3c, 241046 },
    { 0x3e, 250993 },
    { 0x40, 262144 },
};

static unsigned int ar0144_alloc_again(unsigned int isp_gain, unsigned char shift, unsigned int *sensor_again)
{
    struct again_lut *lut = ar0144_again_lut;

    while(lut->gain <= ar0144_attr.max_again) {
        if(isp_gain == 0) {
            *sensor_again = lut->value;
            return 0;
        }

        else if(isp_gain < lut->gain) {
            *sensor_again = (lut - 1)->value;
            return (lut - 1)->gain;
        }

        else{
            if((lut->gain == ar0144_attr.max_again) && (isp_gain >= lut->gain)) {
                *sensor_again = lut->value;
                return lut->gain;
            }

        }

        lut++;
    }

    return isp_gain;
}

static unsigned int ar0144_alloc_dgain(unsigned int isp_gain, unsigned char shift, unsigned int *sensor_dgain)
{
    return isp_gain;
}


static struct tx_isp_sensor_attribute ar0144_attr = {
    .name                           = "ar0144",
    .chip_id                        = 0x0356,
    .cbus_type                      = TX_SENSOR_CONTROL_INTERFACE_I2C,
    .cbus_mask                      = V4L2_SBUS_MASK_SAMPLE_16BITS | V4L2_SBUS_MASK_ADDR_16BITS,
    .cbus_device                    = 0x10,
    .dbus_type                      = TX_SENSOR_DATA_INTERFACE_MIPI,
    .mipi = {
        .clk = 444,                    // mipi data throughput per lane, Mbps/lane = VTS * HTS * FPS * BPP / Lanes
        .lans = 2,
    },
    .max_again                      = 262144,
    .max_dgain                      = 0,
    .min_integration_time           = 4,
    .min_integration_time_native    = 4,
    .max_integration_time_native    = 0x033B - 4,
    .one_line_expr_in_us            = 8,
    .integration_time_limit         = 0x033B - 4,
    .total_width                    = 0x05D0,            // HTS, register value in 0x300C
    .total_height                   = 0x033B,            // VTS, register value in 0x300A
    .max_integration_time           = 0x033B - 4,
    .integration_time_apply_delay   = 1,
    .again_apply_delay              = 1,
    .dgain_apply_delay              = 0,
    .sensor_ctrl.alloc_again        = ar0144_alloc_again,
    .sensor_ctrl.alloc_dgain        = ar0144_alloc_dgain,
    //void priv; /* point to struct tx_isp_sensor_board_info */
};

/*
 * Interface    : MIPI
 * MCLK         : 24Mhz,
 * MIPI_clcok   : 443 Mbps/lane
 * MIPI_lane    : 2
 * resolution   : 1280*800
 * FrameRate    : 60fps
 * PCLK            : 74MHz
 * HTS          : 827
 * VTS          : 1488
 * BPP            : 12
 */
static struct regval_list ar0144_init_regs_1280_800_60fps_mipi_2lane_12bit[] = {
    {0x302A, 0x0006},        //VT_PIX_CLK_DIV = 6
    {0x302C, 0x0001},        //VT_SYS_CLK_DIV = 1
    {0x302E, 0x0004},        //PRE_PLL_CLK_DIV = 4
    {0x3030, 0x0042},        //PLL_MULTIPLIER = 66
    {0x3036, 0x000C},        //OP_PIX_CLK_DIV = 12
    {0x3038, 0x0001},        //OP_SYS_CLK_DIV = 1
    {0x0000, 0x0001},       //udelay(50)
    {0x31B0, 0x0042},        //FRAME_PREAMBLE = 66
    {0x31B2, 0x002E},        //LINE_PREAMBLE = 46
    {0x31B4, 0x1665},        //MIPI_TIMING_0 = 5733
    {0x31B6, 0x110E},        //MIPI_TIMING_1 = 4366
    {0x31B8, 0x2047},        //MIPI_TIMING_2 = 8263
    {0x31BA, 0x0105},        //MIPI_TIMING_3 = 261
    {0x31BC, 0x0004},        //MIPI_TIMING_4 = 4
    {0x31AE, 0x0202},        //SERIAL_FORMAT = 514
    {0x3002, 0x0000},        //Y_ADDR_START = 0
    {0x3004, 0x0004},        //X_ADDR_START = 4
    {0x3006, 0x031F},        //Y_ADDR_END = 799
    {0x3008, 0x0503},        //X_ADDR_END = 1283
    {0x300A, 0x033B},        //FRAME_LENGTH_LINES = 827
    {0x300C, 0x05D0},        //LINE_LENGTH_PCK = 1488
    {0x3012, 0x03E0},        //COARSE_INTEGRATION_TIME = 826
    {0x3270, 0x01FC},        //led_flash_en
    {0x3786, 0x00c6},        //mipi enter LP11 for ingenic ISP
    {0x3060, 0x002D},
    {0x31AC, 0x0C0C},        //DATA_FORMAT_BITS = 3084
    {0x306E, 0x9010},        //DATAPATH_SELECT = 36880
    {0x30A2, 0x0001},        //X_ODD_INC = 1
    {0x30A6, 0x0001},        //Y_ODD_INC = 1
    {0x3082, 0x0003},        //OPERATION_MODE_CTRL = 3
    //{0x3084, 0x0003},        //OPERATION_MODE_CTRL_CB = 3
    //{0x308C, 0x0028},        //Y_ADDR_START_CB = 40
    //{0x308A, 0x0004},        //X_ADDR_START_CB = 4
    //{0x3090, 0x02F7},        //Y_ADDR_END_CB = 759
    //{0x308E, 0x0503},        //X_ADDR_END_CB = 1283
    //{0x30AA, 0x033A},        //FRAME_LENGTH_LINES_CB = 826
    //{0x303E, 0x05D0},        //LINE_LENGTH_PCK_CB = 1488
    //{0x3016, 0x0339},        //COARSE_INTEGRATION_TIME_CB = 825
    //{0x30AE, 0x0001},        //X_ODD_INC_CB = 1
    //{0x30A8, 0x0001},        //Y_ODD_INC_CB = 1
    {0x3040, 0x0000},        //READ_MODE = 0
    {0x31D0, 0x0001},        //COMPANDING = 1
    //{0x31D8, 0x0F10},        //Enable All Test Lanes, Choose LP11 on Data & CLK Lane
    //{0x31C6, 0x8080},        //Enable Serial Test Mode 
    {0x301A, 0x0058},        //RESET_REGISTER = 92

    {AR0144_REG_END, 0x0}, /* END MARKER */
};

/*
 * Interface    : MIPI
 * MCLK         : 24Mhz,
 * MIPI_clcok   : 444 Mbps/lane
 * MIPI_lane    : 2
 * resolution   : 1280*800
 * FrameRate    : 60fps
 * PCLK            : 74MHz
 * HTS          : 827
 * VTS          : 1488
 * BPP            : 12
 */
static struct regval_list ar0144_init_regs_1280_800_60fps_mipi_2lane_12bit_new[] = {
    {0x302A, 0x0006}, // VT_PIX_CLK_DIV
    {0x302C, 0x0001}, // VT_SYS_CLK_DIV
    {0x302E, 0x0004}, // PRE_PLL_CLK_DIV
    {0x3030, 0x004A}, // PLL_MULTIPLIER
    {0x3036, 0x000C}, // OP_PIX_CLK_DIV
    {0x3038, 0x0001}, // OP_SYS_CLK_DIV
    {0x0000, 0x0001}, //udelay(50)
    {0x30B0, 0x0028}, // DIGITAL_TEST
    {0x31B0, 0x0047}, // FRAME_PREAMBLE
    {0x31B2, 0x0033}, // LINE_PREAMBLE
    {0x31B4, 0x2634}, // MIPI_TIMING_0
    {0x31B6, 0x210E}, // MIPI_TIMING_1
    {0x31B8, 0x20C7}, // MIPI_TIMING_2
    {0x31BA, 0x0185}, // MIPI_TIMING_3
    {0x31BC, 0x0004}, // MIPI_TIMING_4
    {0x31AE, 0x0202}, // SERIAL_FORMAT
    {0x3002, 0x0000}, // Y_ADDR_START
    {0x3004, 0x0004}, // X_ADDR_START
    {0x3006, 0x031F}, // Y_ADDR_END
    {0x3008, 0x0503}, // X_ADDR_END
    {0x300A, 0x033B}, // FRAME_LENGTH_LINES
    {0x300C, 0x05D0}, // LINE_LENGTH_PCK
    {0x3012, 0x033A}, // COARSE_INTEGRATION_TIME
    {0x3786, 0x00c6}, //mipi enter LP11 for ingenic ISP
    {0x31AC, 0x0C0C}, // DATA_FORMAT_BITS
    {0x306E, 0x9010}, // DATAPATH_SELECT
    {0x30A2, 0x0001}, // X_ODD_INC
    {0x30A6, 0x0001}, // Y_ODD_INC
    {0x3082, 0x0003}, // OPERATION_MODE_CTRL
    //{0x3084, 0x0003}, // OPERATION_MODE_CTRL_CB
    //{0x308C, 0x0028}, // Y_ADDR_START_CB
    //{0x308A, 0x0004}, // X_ADDR_START_CB
    //{0x3090, 0x02F7}, // Y_ADDR_END_CB
    //{0x308E, 0x0503}, // X_ADDR_END_CB
    //{0x30AA, 0x0337}, // FRAME_LENGTH_LINES_CB
    //{0x303E, 0x05D0}, // LINE_LENGTH_PCK_CB
    //{0x3016, 0x0336}, // COARSE_INTEGRATION_TIME_CB
    //{0x30AE, 0x0001}, // X_ODD_INC_CB
    //{0x30A8, 0x0001}, // Y_ODD_INC_CB
    {0x3040, 0x0000}, // READ_MODE
    {0x31D0, 0x0001}, // COMPANDING
    {0x301A, 0x0058}, // RESET_REGISTER

    {AR0144_REG_END, 0x0}, /* END MARKER */
};

/*
 * Interface    : MIPI
 * MCLK         : 24Mhz,
 * MIPI_clcok   : 589 Mbps/lane
 * MIPI_lane    : 1
 * resolution   : 1280*800
 * FrameRate    : 30fps
 * PCLK            : 74MHz
 * HTS          : 1654
 * VTS          : 1488
 * BPP            : 8
 */
static struct regval_list ar0144_init_regs_1280_800_30fps_mipi_1lane_8bit[] = {
    //{0x301A, 0x00D9}, // RESET_REGISTER
    //{0x301A, 0x3058}, // RESET_REGISTER
    //{AR0144_REG_DELAY,    100},
    //{0x3F4C, 0x003F}, // RESERVED_MFR_3F4C
    //{0x3F4E, 0x0018}, // RESERVED_MFR_3F4E
    //{0x3F50, 0x17DF}, // RESERVED_MFR_3F50
    //{0x30B0, 0x0028}, // DIGITAL_TEST
    //{0x3060, 0x001E}, // ANALOG_GAIN
    //{0x30FE, 0x00A8}, // NOISE_PEDESTAL
    //{0x306E, 0x4810}, // DATAPATH_SELECT
    //{0x3064, 0x1802}, // SMIA_TEST
    //{AR0144_REG_DELAY,    200},
    {0x302A, 0x0004}, // VT_PIX_CLK_DIV
    {0x302C, 0x0002}, // VT_SYS_CLK_DIV
    {0x302E, 0x0008}, // PRE_PLL_CLK_DIV
    {0x3030, 0x00C6}, // PLL_MULTIPLIER
    {0x3036, 0x0008}, // OP_PIX_CLK_DIV
    {0x3038, 0x0001}, // OP_SYS_CLK_DIV
    {0x30B0, 0x0028}, // DIGITAL_TEST
    {0x31B0, 0x0066}, // FRAME_PREAMBLE
    {0x31B2, 0x0045}, // LINE_PREAMBLE
    {0x31B4, 0x4C67}, // MIPI_TIMING_0
    {0x31B6, 0x4218}, // MIPI_TIMING_1
    {0x31B8, 0x41CB}, // MIPI_TIMING_2
    {0x31BA, 0x030A}, // MIPI_TIMING_3
    {0x31BC, 0x0008}, // MIPI_TIMING_4
    {0x3354, 0x002A}, // MIPI_CNTRL
    {0x31AE, 0x0201}, // SERIAL_FORMAT
    {0x3002, 0x0000}, // Y_ADDR_START
    {0x3004, 0x0004}, // X_ADDR_START
    {0x3006, 0x031F}, // Y_ADDR_END
    {0x3008, 0x0503}, // X_ADDR_END
    {0x300A, 0x0676}, // FRAME_LENGTH_LINES
    {0x300C, 0x05D0}, // LINE_LENGTH_PCK
    {0x3012, 0x0064}, // COARSE_INTEGRATION_TIME
    {0x31AC, 0x0C0C}, // DATA_FORMAT_BITS
    {0x306E, 0x9010}, // DATAPATH_SELECT
    {0x30A2, 0x0001}, // X_ODD_INC
    {0x30A6, 0x0001}, // Y_ODD_INC
    {0x3040, 0x0000}, // READ_MODE
    {0x3786, 0x00c6}, //mipi enter LP11 for ingenic ISP
    //{0x31D8, 0x0F10}, //Enable All Test Lanes, Choose LP11 on Data & CLK Lane
    //{0x31C6, 0x8080}, //Enable Serial Test Mode
    {0x311C, 0x033B}, // AE_MAX_EXPOSURE_REG
    {0x31AC, 0x0808}, // DATA_FORMAT_BITS
    {0x3082, 0x0003}, // OPERATION_MODE_CTRL
    {0x31D0, 0x0001}, // COMPANDING
    {0x301A, 0x0058}, // RESET_REGISTER

    {AR0144_REG_END, 0x0}, /* END MARKER */
};

/*
 * Interface    : MIPI
 * MCLK         : 24Mhz,
 * MIPI_clcok   : 670 Mbps/lane
 * MIPI_lane    : 1
 * resolution   : 1280*800
 * FrameRate    : 30fps
 * PCLK            : 56MHz
 * HTS          : 1250
 * VTS          : 1488
 * BPP            : 12
 */

static struct regval_list ar0144_init_regs_1280_800_30fps_mipi_1lane_12bit[] = {
    /*{0x301A, 0x00D9}, // RESET_REGISTER
    {0x301A, 0x3058}, // RESET_REGISTER
    {AR0144_REG_DELAY, 100         },
    {0x3F4C, 0x003F}, // PIX_DEF_1D_DDC_LO_DEF
    {0x3F4E, 0x0018}, // PIX_DEF_1D_DDC_HI_DEF
    {0x3F50, 0x17DF}, // PIX_DEF_1D_DDC_EDGE
    {0x30B0, 0x0028}, // DIGITAL_TEST
    {0x3ED6, 0x3CB5}, // DAC_LD_10_11
    {0x3ED8, 0x8765}, // DAC_LD_12_13
    {0x3EDA, 0x8888}, // DAC_LD_14_15
    {0x3EDC, 0x97FF}, // DAC_LD_16_17
    {0x3EF8, 0x6522}, // DAC_LD_44_45
    {0x3EFA, 0x2222}, // DAC_LD_46_47
    {0x3EFC, 0x6666}, // DAC_LD_48_49
    {0x3F00, 0xAA05}, // DAC_LD_52_53
    {0x3EE2, 0x180E}, // DAC_LD_22_23
    {0x3EE4, 0x0808}, // DAC_LD_24_25
    {0x3EEA, 0x2A09}, // DAC_LD_30_31
    {0x3060, 0x000D}, // ANALOG_GAIN
    {0x30FE, 0x00A8}, // NOISE_PEDESTAL
    {0x3092, 0x00CF}, // ROW_NOISE_CONTROL
    {0x3268, 0x0030}, // SEQUENCER_CONTROL
    {0x3786, 0x0006}, // DIGITAL_CTRL_1
    {0x3F4A, 0x0F70}, // DELTA_DK_PIX_THRES
    {0x306E, 0x4810}, // DATAPATH_SELECT
    {0x3064, 0x1802}, // SMIA_TEST
    {0x3EF6, 0x804D}, // DAC_LD_42_43
    {0x3180, 0xC08F}, // DELTA_DK_CONTROL
    {0x30BA, 0x7623}, // DIGITAL_CTRL
    {0x3176, 0x0480}, // DELTA_DK_ADJUST_GREENR
    {0x3178, 0x0480}, // DELTA_DK_ADJUST_RED
    {0x317A, 0x0480}, // DELTA_DK_ADJUST_BLUE
    {0x317C, 0x0480}, // DELTA_DK_ADJUST_GREENB
    {AR0144_REG_DELAY, 200         },*/
    {0x302A, 0x0006}, // VT_PIX_CLK_DIV
    {0x302C, 0x0002}, // VT_SYS_CLK_DIV
    {0x302E, 0x0002}, // PRE_PLL_CLK_DIV
    {0x3030, 0x0038}, // PLL_MULTIPLIER
    {0x3036, 0x000C}, // OP_PIX_CLK_DIV
    {0x3038, 0x0001}, // OP_SYS_CLK_DIV
    {0x30B0, 0x0028}, // DIGITAL_TEST
    {0x31B0, 0x005A}, // FRAME_PREAMBLE
    {0x31B2, 0x003A}, // LINE_PREAMBLE
    {0x31B4, 0x3945}, // MIPI_TIMING_0
    {0x31B6, 0x31D4}, // MIPI_TIMING_1
    {0x31B8, 0x30C9}, // MIPI_TIMING_2
    {0x31BA, 0x0208}, // MIPI_TIMING_3
    {0x31BC, 0x0007}, // MIPI_TIMING_4
    {0x3354, 0x002C}, // MIPI_CNTRL
    {0x31AE, 0x0201}, // SERIAL_FORMAT
    {0x3002, 0x0000}, // Y_ADDR_START
    {0x3004, 0x0004}, // X_ADDR_START
    {0x3006, 0x031F}, // Y_ADDR_END
    {0x3008, 0x0503}, // X_ADDR_END
    {0x300A, 0x04E2}, // FRAME_LENGTH_LINES
    {0x300C, 0x05D0}, // LINE_LENGTH_PCK
    {0x3012, 0x0064}, // COARSE_INTEGRATION_TIME
    {0x31AC, 0x0C0C}, // DATA_FORMAT_BITS
    {0x306E, 0x9010}, // DATAPATH_SELECT
    {0x30A2, 0x0001}, // X_ODD_INC
    {0x30A6, 0x0001}, // Y_ODD_INC
    {0x3040, 0x0000}, // READ_MODE
    {0x3786, 0x00c6}, //mipi enter LP11 for ingenic ISP
    //{0x31D8, 0x0F10}, //Enable All Test Lanes, Choose LP11 on Data & CLK Lane
    //{0x31C6, 0x8080}, //Enable Serial Test Mode
    {0x311C, 0x0460}, // AE_MAX_EXPOSURE_REG
    {0x3082, 0x0003}, // OPERATION_MODE_CTRL
    {0x301A, 0x0058}, // RESET_REGISTER

    {AR0144_REG_END, 0x0}, /* END MARKER */
};

static struct tx_isp_sensor_win_setting ar0144_win_sizes[] = {
    {
        .width      = 1280,
        .height     = 800,
        .fps        = 60 << 16 | 1,
        .mbus_code  = V4L2_MBUS_FMT_SGRBG12_1X12,
        .colorspace = V4L2_COLORSPACE_SRGB,
        .regs       = ar0144_init_regs_1280_800_60fps_mipi_2lane_12bit,

    },
    {
        .width      = 1280,
        .height     = 800,
        .fps        = 60 << 16 | 1,
        .mbus_code  = V4L2_MBUS_FMT_SGRBG12_1X12,
        .colorspace = V4L2_COLORSPACE_SRGB,
        .regs       = ar0144_init_regs_1280_800_60fps_mipi_2lane_12bit_new,
    },
    {
        .width      = 1280,
        .height     = 800,
        .fps        = 30 << 16 | 1,
        .mbus_code  = V4L2_MBUS_FMT_SGRBG8_1X8,
        .colorspace = V4L2_COLORSPACE_SRGB,
        .regs       = ar0144_init_regs_1280_800_30fps_mipi_1lane_8bit,
    },
    {
        .width      = 1280,
        .height     = 800,
        .fps        = 30 << 16 | 1,
        .mbus_code  = V4L2_MBUS_FMT_SGRBG12_1X12,
        .colorspace = V4L2_COLORSPACE_SRGB,
        .regs       = ar0144_init_regs_1280_800_30fps_mipi_1lane_12bit,
    },
};

/*
 * the part of driver was fixed.
 */

static struct regval_list ar0144_stream_on_mipi[] = {
    {0x301A, 0x005C},
    {AR0144_REG_END, 0x00},    /* END MARKER */
};

static struct regval_list ar0144_stream_off_mipi[] = {
    {0x301A, 0x0058},
    {AR0144_REG_END, 0x00},    /* END MARKER */
};

int ar0144_read(struct v4l2_subdev *sd, u16 reg, u16 *val)
{
    int ret;
    u8 buf[2] = {reg >> 8, reg & 0xff};
    u8 buf2[2];

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
            .len    = 2,
            .buf    = buf2,
        }
    };

    ret = i2c_transfer(client->adapter, msg, 2);
    if (ret > 0)
        ret = 0;
    
    *val = (buf2[0] << 8) | buf2[1];

    return ret;
}

int ar0144_write(struct v4l2_subdev *sd, u16 reg, unsigned short val)
{
    struct i2c_client *client = v4l2_get_subdevdata(sd);

    u8 buf[4] = {reg >> 8, reg & 0xff, val >> 8, val & 0xff};
    struct i2c_msg msg = {
        .addr   = client->addr,
        .flags  = 0,
        .len    = 4,
        .buf    = buf,
    };

    int ret;
    ret = i2c_transfer(client->adapter, &msg, 1);
    if (ret > 0)
        ret = 0;

    return ret;
}

static int ar0144_read_array(struct v4l2_subdev *sd, struct regval_list *vals)
{
    int ret;
    u16 val;
    
    while (vals->reg_num != AR0144_REG_END) {
       if (vals->reg_num == AR0144_REG_DELAY) {
            msleep(vals->value);
        } else {
            ret = ar0144_read(sd, vals->reg_num, &val);
            if (ret < 0)
                return ret;
        }
        printk("reg_num:0x%x, val:0x%x.\n",vals->reg_num,val);
        vals++;
    }

    return 0;
}

static int ar0144_write_array(struct v4l2_subdev *sd, struct regval_list *vals)
{
    int ret;

    while (vals->reg_num != AR0144_REG_END) {
        if (vals->reg_num == AR0144_REG_DELAY) {
                msleep(vals->value);
        } else {
            ret = ar0144_write(sd, vals->reg_num, vals->value);
            if (ret < 0)
                return ret;
        }
        pr_debug("vals->reg_num:%x, vals->value:%x\n",vals->reg_num, vals->value);
        vals++;
    }
    return 0;
}


static int ar0144_reset(struct v4l2_subdev *sd, u32 val)
{
    int ret = ISP_SUCCESS;

    printk("Set gpio:%d, %d\n", reset_gpio, pwdn_gpio);

    if(reset_gpio != -1){
       ret = gpio_request(reset_gpio,"ar0144_reset");
        if (!ret) {
            gpio_direction_output(reset_gpio, 1);
            msleep(5);
            // the datasheet point out the hard reset to be hold for 1ms at least
            gpio_direction_output(reset_gpio, 0);
            msleep(5);
            // the datasheet point out the internal init to be 160000mclk at least
            // and for 24M mclk, it is ~6.7ms
            gpio_direction_output(reset_gpio, 1);
            msleep(10);
            printk("ar0144 reset done\n");
        } else {
            printk("gpio requrest fail %d\n", reset_gpio);
        }
    }

    if(pwdn_gpio != -1){
        ret = gpio_request(pwdn_gpio,"ar0144_pwdn");
        if(!ret){
            gpio_direction_output(pwdn_gpio, 1);
            msleep(10);
            gpio_direction_output(pwdn_gpio, 0);
            msleep(50);
        }else{
            printk("gpio requrest fail %d\n",pwdn_gpio);
        }
    }
    
    return 0;
}

static int ar0144_detect(struct v4l2_subdev *sd, unsigned int *ident)
{
    u16 val;
    int ret;
    
    ret = ar0144_read(sd, 0x3000, &val);
    if(ret < 0)
        return -ENODEV;
    
    if(val != AR0144_CHIP_ID)
        return -ENODEV;

    return 0;  
}

static int ar0144_set_integration_time(struct v4l2_subdev *sd, int value)
{
    int ret;

    ret = ar0144_write(sd,0x3012, value);
    pr_debug("ar0144_set_integration_time value 0x%x.\n", value);
    if (ret < 0)
        return ret;

    return 0;
}

static int ar0144_set_analog_gain(struct v4l2_subdev *sd, int gain)
{
    int ret;

    ret = ar0144_write(sd, 0x3060, gain);
    pr_debug("ar0144_set_analog_gain value 0x%x.\n", gain);
    if (ret < 0)
        return ret;

    return 0;
}

static int ar0144_set_digital_gain(struct v4l2_subdev *sd, int value)
{

    return 0;
}

static int ar0144_get_black_pedestal(struct v4l2_subdev *sd, int value)
{

    return 0;
}

static int ar0144_init(struct v4l2_subdev *sd, u32 enable)
{
    struct tx_isp_sensor *sensor = (container_of(sd, struct tx_isp_sensor, sd));
    struct tx_isp_notify_argument arg;
    struct tx_isp_sensor_win_setting *wsize = &ar0144_win_sizes[1];;
    int ret = 0;
    if(!enable)
        return ISP_SUCCESS;
    
    sensor->video.mbus.width      = wsize->width;
    sensor->video.mbus.height     = wsize->height;
    sensor->video.mbus.code       = wsize->mbus_code;
    sensor->video.mbus.field      = V4L2_FIELD_NONE;
    sensor->video.mbus.colorspace = wsize->colorspace;
    sensor->video.fps             = wsize->fps;

    ret = ar0144_write_array(sd, wsize->regs);
    ret += ar0144_read_array(sd, wsize->regs);
    
    if (ret)
        return ret;
    
    arg.value = (int)&sensor->video;
    sd->v4l2_dev->notify(sd, TX_ISP_NOTIFY_SYNC_VIDEO_IN, &arg);
    sensor->priv = wsize;

    return 0;
}

static int ar0144_g_parm(struct v4l2_subdev *sd, struct v4l2_streamparm *parms)
{

    return 0;
}

static int ar0144_s_parm(struct v4l2_subdev *sd, struct v4l2_streamparm *parms)
{

    return 0;
}

static int ar0144_s_stream(struct v4l2_subdev *sd, int enable)
{
    int ret = 0;
    if (enable) {
        ret = ar0144_write_array(sd, ar0144_stream_on_mipi);
        pr_debug("ar0144 stream on\n");

    }
    else {
        ret = ar0144_write_array(sd, ar0144_stream_off_mipi);
        pr_debug("ar0144 stream off\n");
    }

    return ret;
}

static int ar0144_set_fps(struct tx_isp_sensor *sensor, int fps)
{
    struct v4l2_subdev *sd = &sensor->sd;
    struct tx_isp_notify_argument arg;
    unsigned int sclk = 0;
    unsigned short vts = 0;
    unsigned short hts=0;
    unsigned int max_fps = 0;
    u16 tmp[2];
    unsigned int newformat = 0; //the format is 24.8
    int ret = 0;

    sclk = AR0144_SCLK;
    max_fps = SENSOR_OUTPUT_MAX_FPS;

    /* the format of fps is 16/16. for example 25 << 16 | 2, the value is 25/2 fps. */
    newformat = (((fps >> 16) / (fps & 0xffff)) << 8) + ((((fps >> 16) % (fps & 0xffff)) << 8) / (fps & 0xffff));
    if(newformat > (max_fps << 8) || newformat < (SENSOR_OUTPUT_MIN_FPS << 8)){
        printk("warn: fps(%x) no in range\n", fps);
        return -1;
    }

    //HTS
    ret = ar0144_read(sd, 0x300C, tmp);
    hts=tmp[0];
    if(ret < 0)
        return -1;
    
    hts = (hts << 8) + tmp[1];
    //VTS
    vts = sclk * (fps & 0xffff) / hts / ((fps & 0xffff0000) >> 16);
    ret = ar0144_write(sd, 0x300A, vts);
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

static int ar0144_set_mode(struct tx_isp_sensor *sensor, int value)
{

    return 0;
}

static int ar0144_set_vflip(struct tx_isp_sensor *sensor, int enable)
{

    return 0;
}

static int ar0144_g_chip_ident(struct v4l2_subdev *sd,
        struct v4l2_dbg_chip_ident *chip)
{
    struct i2c_client *client = v4l2_get_subdevdata(sd);
    unsigned int ident = 0;
    int ret = ISP_SUCCESS;

    if(reset_gpio != -1){
       ret = gpio_request(reset_gpio,"ar0144_reset");
        if (!ret) {
            gpio_direction_output(reset_gpio, 1);
            msleep(5);
            gpio_direction_output(reset_gpio, 0);
            msleep(5);
            gpio_direction_output(reset_gpio, 1);
            msleep(10);
            printk("ar0144 reset done\n");
        } else {
            printk("gpio requrest fail %d\n", reset_gpio);
        }
    }

    if(pwdn_gpio != -1){
        ret = gpio_request(pwdn_gpio,"ar0144_pwdn");
        if(!ret){
            gpio_direction_output(pwdn_gpio, 1);
            msleep(10);
            gpio_direction_output(pwdn_gpio, 0);
            msleep(50);
        }else{
            printk("gpio requrest fail %d\n",pwdn_gpio);
        }
    }

    ret = ar0144_detect(sd, &ident);
    if (ret) {
        printk("chip found @ 0x%x (%s) is not an ar0144 chip.\n",
                client->addr, client->adapter->name);
        return ret;
    }

    v4l_info(client, "ar0144 chip found @ 0x%02x (%s)\n",
         client->addr, client->adapter->name);

    return v4l2_chip_ident_i2c_client(client, chip, ident, 0);
}
static int ar0144_s_power(struct v4l2_subdev *sd, int on)
{
    return 0;
}

static int ar0144_ops_private_ioctl(struct tx_isp_sensor *sensor, struct isp_private_ioctl *ctrl)
{
    long ret = 0;

    struct v4l2_subdev *sd = &sensor->sd;
    //printk("%d %s %d\n", __LINE__, __func__, ctrl->cmd);
    switch(ctrl->cmd){
    case TX_ISP_PRIVATE_IOCTL_SENSOR_INT_TIME:
        ret = ar0144_set_integration_time(sd, ctrl->value);
        break;
    case TX_ISP_PRIVATE_IOCTL_SENSOR_AGAIN:
        ret = ar0144_set_analog_gain(sd, ctrl->value);
        break;
    case TX_ISP_PRIVATE_IOCTL_SENSOR_DGAIN:
        ret = ar0144_set_digital_gain(sd, ctrl->value);
        break;
    case TX_ISP_PRIVATE_IOCTL_SENSOR_BLACK_LEVEL:
        ret = ar0144_get_black_pedestal(sd, ctrl->value);
        break;
    case TX_ISP_PRIVATE_IOCTL_SENSOR_RESIZE:
        ret = ar0144_set_mode(sensor, ctrl->value);
        break;
    case TX_ISP_PRIVATE_IOCTL_SUBDEV_PREPARE_CHANGE:
        ret = ar0144_write_array(sd, ar0144_stream_off_mipi);
        break;
    case TX_ISP_PRIVATE_IOCTL_SUBDEV_FINISH_CHANGE:
        ret = ar0144_write_array(sd, ar0144_stream_on_mipi);
        break;
    case TX_ISP_PRIVATE_IOCTL_SENSOR_FPS:
        ret = ar0144_set_fps(sensor, ctrl->value);
        break;
    //case TX_ISP_PRIVATE_IOCTL_SENSOR_VFLIP:
        //ret = ar0144_set_vflip(sd, ctrl->value);
        //break;
    default:
        break;;
    }

    return 0;
}

static long ar0144_ops_ioctl(struct v4l2_subdev *sd, unsigned int cmd, void *arg)
{
    struct tx_isp_sensor *sensor =container_of(sd, struct tx_isp_sensor, sd);
    int ret;

    switch (cmd) {
    case VIDIOC_ISP_PRIVATE_IOCTL:
        ret = ar0144_ops_private_ioctl(sensor, arg);
        break;
    default:
        return -1;
        break;
    }

    return 0;
}

static int ar0144_g_register(struct v4l2_subdev *sd, struct v4l2_dbg_register *reg)
{
    struct i2c_client *client = v4l2_get_subdevdata(sd);
    u16 val = 0;
    int ret;

    if (!v4l2_chip_match_i2c_client(client, &reg->match))
        return -EINVAL;

    if (!capable(CAP_SYS_ADMIN))
        return -EPERM;

    ret = ar0144_read(sd, reg->reg & 0xffff, &val);
    reg->val = val;
    reg->size = 2;

    return ret;
}

static int ar0144_s_register(struct v4l2_subdev *sd, const struct v4l2_dbg_register *reg)
{
    struct i2c_client *client = v4l2_get_subdevdata(sd);


    if (!v4l2_chip_match_i2c_client(client, &reg->match))
        return -EINVAL;

    if (!capable(CAP_SYS_ADMIN))
        return -EPERM;

    ar0144_write(sd, reg->reg & 0xffff, reg->val & 0xffff);

    return 0;
}

static struct v4l2_subdev_core_ops ar0144_core_ops = {
    .g_chip_ident   = ar0144_g_chip_ident,
    .reset          = ar0144_reset,
    .init           = ar0144_init,
    .s_power        = ar0144_s_power,
    .ioctl          = ar0144_ops_ioctl,
#ifdef CONFIG_VIDEO_ADV_DEBUG
    .g_register     = ar0144_g_register,
    .s_register     = ar0144_s_register,
#endif
};

static struct v4l2_subdev_video_ops ar0144_video_ops = {
    .s_stream       = ar0144_s_stream,
    .s_parm         = ar0144_s_parm,
    .g_parm         = ar0144_g_parm,
};

static struct v4l2_subdev_ops ar0144_ops = {
    .core  = &ar0144_core_ops,
    .video = &ar0144_video_ops,
};

/* It's the sensor device */
static u64 tx_isp_module_dma_mask = ~(u64)0;
struct platform_device sensor_platform_device = {
    .name          = "ar0144",
    .id            = -1,
    .dev = {
        .dma_mask = &tx_isp_module_dma_mask,
        .coherent_dma_mask = 0xffffffff,
        .platform_data = NULL,
    },
    .num_resources = 0,
};

static int ar0144_probe(struct i2c_client *client, const struct i2c_device_id *id)
{
    struct v4l2_subdev *sd;
    struct tx_isp_video_in *video;
    struct tx_isp_sensor *sensor;
    struct tx_isp_sensor_win_setting *wsize=&ar0144_win_sizes[1];
    int ret = 0;
    pr_debug("probe ok ----start--->ar0144\n");
    sensor = (struct tx_isp_sensor *)kzalloc(sizeof(*sensor), GFP_KERNEL);
    if(!sensor){
        printk("Failed to allocate sensor subdev.\n");
        return -ENOMEM;
    }

#ifndef MODULE
    ar0144_board_info = get_sensor_board_info(sensor_platform_device.name);
    if (ar0144_board_info) {
        pwdn_gpio       = ar0144_board_info->gpios.gpio_power;
        reset_gpio      = ar0144_board_info->gpios.gpio_sensor_rst;
        gpio_i2c_sel1   = ar0144_board_info->gpios.gpio_i2c_sel1;
        gpio_i2c_sel2   = ar0144_board_info->gpios.gpio_i2c_sel2;
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

    ar0144_attr.max_again = 262144;
    ar0144_attr.max_dgain = 0;
    sd = &sensor->sd;
    video = &sensor->video;
    sensor->video.attr = &ar0144_attr;
    sensor->video.vi_max_width = wsize->width;
    sensor->video.vi_max_height = wsize->height;
    v4l2_i2c_subdev_init(sd, client, &ar0144_ops);
    v4l2_set_subdev_hostdata(sd, sensor);

    pr_debug("probe ok ------->ar0144\n");

    return 0;

#if 0
    private_clk_disable(sensor->mclk);
    private_clk_put(sensor->mclk);
#endif

err_get_mclk:
    kfree(sensor);
    return -1;
}

static int ar0144_remove(struct i2c_client *client)
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

static const struct i2c_device_id ar0144_id[] = {
    { "ar0144", 0 },
    { }
};
MODULE_DEVICE_TABLE(i2c, ar0144_id);

static struct i2c_driver ar0144_driver = {
    .driver = {
        .owner  = THIS_MODULE,
        .name   = "ar0144",
    },
    .probe      = ar0144_probe,
    .remove     = ar0144_remove,
    .id_table   = ar0144_id,
};

static __init int init_ar0144(void)
{
    return i2c_add_driver(&ar0144_driver);
}

static __exit void exit_ar0144(void)
{
    i2c_del_driver(&ar0144_driver);
}

module_init(init_ar0144);
module_exit(exit_ar0144);

MODULE_DESCRIPTION("A low-level driver for Onsemi ar0144 sensors");
MODULE_LICENSE("GPL");
