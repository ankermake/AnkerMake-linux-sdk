/*
 * ar0230.c
 *
 * Copyright (C) 2012 Ingenic Semiconductor Co., Ltd.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */
#define DEBUG

#include <linux/init.h>
#include <linux/module.h>
#include <linux/slab.h>
#include <linux/i2c.h>
#include <linux/delay.h>
#include <linux/gpio.h>
#include <linux/clk.h>
#include <linux/proc_fs.h>
#include <utils/gpio.h>
#include <dvp_gpio_func.h>

#include <tx-isp/tx-isp-common.h>
#include <tx-isp/sensor-common.h>
#include <linux/sensor_board.h>


#define AR0230_CHIP_ID                  0x0056
#define AR0230_REG_END                  0xffff
#define AR0230_REG_DELAY                0xfffe
#define AR0230_SUPPORT_SCLK             (37125000)
#define SENSOR_OUTPUT_MAX_FPS           30
#define SENSOR_OUTPUT_MIN_FPS           5
#define AR0230_MAX_WIDTH                1920
#define AR0230_MAX_HEIGHT               1080

#if defined MD_X1830_AR0230_640X1072
#define AR0230_WIDTH                    640
#define AR0230_HEIGHT                   1072
#elif defined MD_X1830_AR0230_1920X1080
#define AR0230_WIDTH                    1920
#define AR0230_HEIGHT                   1080
#endif

#ifdef MD_X1830_AR0230_HDR
//#define MD_X1830_AR0230_RATIO_T1_T2       8
#define MD_X1830_AR0230_RATIO_T1_T2       16
#define EXPOSURE_RATIO                  MD_X1830_AR0230_RATIO_T1_T2/(MD_X1830_AR0230_RATIO_T1_T2+1)
#define LCG                             0x0
#define HCG                             0x4
int again_mode;
#endif
#define SENSOR_VERSION                  "H20200311b"


static int power_gpio = -1; // GPIO_PB(30)
static int reset_gpio = -1; // GPIO_PA(19)
static int pwdn_gpio = -1; // -1
static int i2c_sel_gpio = -1; // GPIO_PA(18)
static int i2c_bus_num = -1; // 0

module_param_gpio(power_gpio, 0644);

module_param_gpio(reset_gpio, 0644);

module_param_gpio(pwdn_gpio, 0644);

module_param_gpio(i2c_sel_gpio, 0644);

module_param(i2c_bus_num, int, 0644);

static char *dvp_gpio_func_str = "DVP_PA_LOW_10BIT";
module_param(dvp_gpio_func_str, charp, 0644);
MODULE_PARM_DESC(dvp_gpio_func_str, "Sensor GPIO function");

static int dvp_gpio_func = -1;

static struct tx_isp_sensor_attribute ar0230_attr;

struct regval_list {
    unsigned short reg_num;
    unsigned short value;
};

#ifdef MD_X1830_AR0230_HDR
struct again_lut {
    unsigned char mode;
    unsigned int value;
    unsigned int gain;
};

static struct again_lut ar0230_again_lut[] = {
    {LCG, 0x0b, 39587},
    {LCG, 0x0c, 44437},
    {LCG, 0x0d, 49050},
    {LCG, 0x0e, 54517},
    {LCG, 0x0f, 59684},
    {LCG, 0x10, 65535},
    {LCG, 0x12, 71489},
    {LCG, 0x14, 78337},
    {LCG, 0x16, 85107},
    {LCG, 0x18, 92852},
    {HCG, 0x00, 93908},
    {HCG, 0x01, 97009},
    {HCG, 0x02, 100010},
    {HCG, 0x03, 103238},
    {HCG, 0x04, 106665},
    {HCG, 0x05, 109972},
    {HCG, 0x06, 113453},
    {HCG, 0x07, 117358},
    {HCG, 0x08, 121108},
    {HCG, 0x09, 125219},
    {HCG, 0x0a, 129400},
    {HCG, 0x0b, 133634},
    {HCG, 0x0c, 138346},
    {HCG, 0x0d, 143250},
    {HCG, 0x0e, 148307},
    {HCG, 0x0f, 153668},
    {HCG, 0x10, 159443},
    {HCG, 0x12, 165545},
    {HCG, 0x14, 172047},
    {HCG, 0x16, 179130},
    {HCG, 0x18, 186643},
    {HCG, 0x1a, 194935}, //again 2.91x, total again 7.86x
    {HCG, 0x1c, 203881},
    {HCG, 0x1e, 213842},
    {HCG, 0x20, 224978},
    {HCG, 0x22, 231080},
    {HCG, 0x24, 237582},
    {HCG, 0x26, 244594},
    {HCG, 0x28, 252178},
    {HCG, 0x2a, 260410}, //again 5.82x, total again 15.71x
    {HCG, 0x2c, 269416},
    {HCG, 0x2e, 279377},
    {HCG, 0x30, 290513},
    {HCG, 0x32, 296656},
    {HCG, 0x34, 303155},
    {HCG, 0x36, 310164},
    {HCG, 0x38, 317680},
    {HCG, 0x3a, 325975},
};

static unsigned int ar0230_alloc_again(unsigned int isp_gain, unsigned char shift, unsigned int *sensor_again)
{
    struct again_lut *lut = ar0230_again_lut;

    while (lut->gain <= ar0230_attr.max_again) {
        if (isp_gain <= 39587) {
            again_mode = LCG;
            *sensor_again = 0x0b;
            return 39587;
        } else if (isp_gain < lut->gain) {
            again_mode = (lut - 1)->mode;
            *sensor_again = (lut - 1)->value;
            return (lut - 1)->gain;
        } else {
            if ((lut->gain == ar0230_attr.max_again) && (isp_gain >= lut->gain)) {
                again_mode = lut->mode;
                *sensor_again = lut->value;
                return lut->gain;
            }
        }

        lut++;
    }

    return isp_gain;
}

static unsigned int ar0230_alloc_dgain(unsigned int isp_gain, unsigned char shift, unsigned int *sensor_dgain)
{
    return isp_gain;
}

static struct tx_isp_sensor_attribute ar0230_attr={
    .name                                   = "ar0230",
    .chip_id                                = 0x0056,
    .cbus_type                              = TX_SENSOR_CONTROL_INTERFACE_I2C,
    .cbus_mask                              = V4L2_SBUS_MASK_SAMPLE_16BITS | V4L2_SBUS_MASK_ADDR_16BITS,
    .cbus_device                            = 0x10,
    .dbus_type                              = TX_SENSOR_DATA_INTERFACE_DVP,
    .dvp = {
        .mode = SENSOR_DVP_HREF_MODE,
        .blanking = {
            .vblanking = 0,
            .hblanking = 0,
        },
    },
    .max_again                              = 260410,
    .max_dgain                              = 0,
    .min_integration_time                   = 2*MD_X1830_AR0230_RATIO_T1_T2,
    .min_integration_time_native            = 2*MD_X1830_AR0230_RATIO_T1_T2,
    .max_integration_time_native            = 0x0452 * EXPOSURE_RATIO - 4,
    .integration_time_limit                 = 0x0452 * EXPOSURE_RATIO - 4,
    .total_width                            = 0x045E,
    .total_height                           = 0x0452,
    .max_integration_time                   = 0x0452 * EXPOSURE_RATIO - 4,
    .integration_time_apply_delay           = 2,
    .again_apply_delay                      = 2,
    .dgain_apply_delay                      = 1,
    .sensor_ctrl.alloc_again                = ar0230_alloc_again,
    .sensor_ctrl.alloc_dgain                = ar0230_alloc_dgain,
    //    void priv; /* point to struct tx_isp_sensor_board_info */
};

static struct regval_list ar0230_init_regs_1920_1080_30fps_HDR_dvp[] = {
    {0x301A, 0x0001},
    {AR0230_REG_DELAY, 200},
    {0x301A, 0x10D8}, //reset_register
    {0x3088, 0x8000},
    {0x3086, 0x4558},
    {0x3086, 0x729B},
    {0x3086, 0x4A31},
    {0x3086, 0x4342},
    {0x3086, 0x8E03},
    {0x3086, 0x2A14},
    {0x3086, 0x4578},
    {0x3086, 0x7B3D},
    {0x3086, 0xFF3D},
    {0x3086, 0xFF3D},
    {0x3086, 0xEA2A},
    {0x3086, 0x043D},
    {0x3086, 0x102A},
    {0x3086, 0x052A},
    {0x3086, 0x1535},
    {0x3086, 0x2A05},
    {0x3086, 0x3D10},
    {0x3086, 0x4558},
    {0x3086, 0x2A04},
    {0x3086, 0x2A14},
    {0x3086, 0x3DFF},
    {0x3086, 0x3DFF},
    {0x3086, 0x3DEA},
    {0x3086, 0x2A04},
    {0x3086, 0x622A},
    {0x3086, 0x288E},
    {0x3086, 0x0036},
    {0x3086, 0x2A08},
    {0x3086, 0x3D64},
    {0x3086, 0x7A3D},
    {0x3086, 0x0444},
    {0x3086, 0x2C4B},
    {0x3086, 0x8F00},
    {0x3086, 0x430C},
    {0x3086, 0x2D63},
    {0x3086, 0x4316},
    {0x3086, 0x8E03},
    {0x3086, 0x2AFC},
    {0x3086, 0x5C1D},
    {0x3086, 0x5754},
    {0x3086, 0x495F},
    {0x3086, 0x5305},
    {0x3086, 0x5307},
    {0x3086, 0x4D2B},
    {0x3086, 0xF810},
    {0x3086, 0x164C},
    {0x3086, 0x0855},
    {0x3086, 0x562B},
    {0x3086, 0xB82B},
    {0x3086, 0x984E},
    {0x3086, 0x1129},
    {0x3086, 0x0429},
    {0x3086, 0x8429},
    {0x3086, 0x9460},
    {0x3086, 0x5C19},
    {0x3086, 0x5C1B},
    {0x3086, 0x4548},
    {0x3086, 0x4508},
    {0x3086, 0x4588},
    {0x3086, 0x29B6},
    {0x3086, 0x8E01},
    {0x3086, 0x2AF8},
    {0x3086, 0x3E02},
    {0x3086, 0x2AFA},
    {0x3086, 0x3F09},
    {0x3086, 0x5C1B},
    {0x3086, 0x29B2},
    {0x3086, 0x3F0C},
    {0x3086, 0x3E02},
    {0x3086, 0x3E13},
    {0x3086, 0x5C13},
    {0x3086, 0x3F11},
    {0x3086, 0x3E0B},
    {0x3086, 0x5F2B},
    {0x3086, 0x902A},
    {0x3086, 0xF22B},
    {0x3086, 0x803E},
    {0x3086, 0x043F},
    {0x3086, 0x0660},
    {0x3086, 0x29A2},
    {0x3086, 0x29A3},
    {0x3086, 0x5F4D},
    {0x3086, 0x192A},
    {0x3086, 0xFA29},
    {0x3086, 0x8345},
    {0x3086, 0xA83E},
    {0x3086, 0x072A},
    {0x3086, 0xFB3E},
    {0x3086, 0x2945},
    {0x3086, 0x8821},
    {0x3086, 0x3E08},
    {0x3086, 0x2AFA},
    {0x3086, 0x5D29},
    {0x3086, 0x9288},
    {0x3086, 0x102B},
    {0x3086, 0x048B},
    {0x3086, 0x1685},
    {0x3086, 0x8D48},
    {0x3086, 0x4D4E},
    {0x3086, 0x2B80},
    {0x3086, 0x4C0B},
    {0x3086, 0x603F},
    {0x3086, 0x282A},
    {0x3086, 0xF23F},
    {0x3086, 0x0F29},
    {0x3086, 0x8229},
    {0x3086, 0x8329},
    {0x3086, 0x435C},
    {0x3086, 0x155F},
    {0x3086, 0x4D19},
    {0x3086, 0x2AFA},
    {0x3086, 0x4558},
    {0x3086, 0x8E00},
    {0x3086, 0x2A98},
    {0x3086, 0x3F06},
    {0x3086, 0x1244},
    {0x3086, 0x4A04},
    {0x3086, 0x4316},
    {0x3086, 0x0543},
    {0x3086, 0x1658},
    {0x3086, 0x4316},
    {0x3086, 0x5A43},
    {0x3086, 0x1606},
    {0x3086, 0x4316},
    {0x3086, 0x0743},
    {0x3086, 0x168E},
    {0x3086, 0x032A},
    {0x3086, 0x9C45},
    {0x3086, 0x787B},
    {0x3086, 0x3F07},
    {0x3086, 0x2A9D},
    {0x3086, 0x3E2E},
    {0x3086, 0x4558},
    {0x3086, 0x253E},
    {0x3086, 0x068E},
    {0x3086, 0x012A},
    {0x3086, 0x988E},
    {0x3086, 0x0012},
    {0x3086, 0x444B},
    {0x3086, 0x0343},
    {0x3086, 0x2D46},
    {0x3086, 0x4316},
    {0x3086, 0xA343},
    {0x3086, 0x165D},
    {0x3086, 0x0D29},
    {0x3086, 0x4488},
    {0x3086, 0x102B},
    {0x3086, 0x0453},
    {0x3086, 0x0D8B},
    {0x3086, 0x1685},
    {0x3086, 0x448E},
    {0x3086, 0x032A},
    {0x3086, 0xFC5C},
    {0x3086, 0x1D8D},
    {0x3086, 0x6057},
    {0x3086, 0x5449},
    {0x3086, 0x5F53},
    {0x3086, 0x0553},
    {0x3086, 0x074D},
    {0x3086, 0x2BF8},
    {0x3086, 0x1016},
    {0x3086, 0x4C08},
    {0x3086, 0x5556},
    {0x3086, 0x2BB8},
    {0x3086, 0x2B98},
    {0x3086, 0x4E11},
    {0x3086, 0x2904},
    {0x3086, 0x2984},
    {0x3086, 0x2994},
    {0x3086, 0x605C},
    {0x3086, 0x195C},
    {0x3086, 0x1B45},
    {0x3086, 0x4845},
    {0x3086, 0x0845},
    {0x3086, 0x8829},
    {0x3086, 0xB68E},
    {0x3086, 0x012A},
    {0x3086, 0xF83E},
    {0x3086, 0x022A},
    {0x3086, 0xFA3F},
    {0x3086, 0x095C},
    {0x3086, 0x1B29},
    {0x3086, 0xB23F},
    {0x3086, 0x0C3E},
    {0x3086, 0x023E},
    {0x3086, 0x135C},
    {0x3086, 0x133F},
    {0x3086, 0x113E},
    {0x3086, 0x0B5F},
    {0x3086, 0x2B90},
    {0x3086, 0x2AF2},
    {0x3086, 0x2B80},
    {0x3086, 0x3E04},
    {0x3086, 0x3F06},
    {0x3086, 0x6029},
    {0x3086, 0xA229},
    {0x3086, 0xA35F},
    {0x3086, 0x4D1C},
    {0x3086, 0x2AFA},
    {0x3086, 0x2983},
    {0x3086, 0x45A8},
    {0x3086, 0x3E07},
    {0x3086, 0x2AFB},
    {0x3086, 0x3E29},
    {0x3086, 0x4588},
    {0x3086, 0x243E},
    {0x3086, 0x082A},
    {0x3086, 0xFA5D},
    {0x3086, 0x2992},
    {0x3086, 0x8810},
    {0x3086, 0x2B04},
    {0x3086, 0x8B16},
    {0x3086, 0x868D},
    {0x3086, 0x484D},
    {0x3086, 0x4E2B},
    {0x3086, 0x804C},
    {0x3086, 0x0B60},
    {0x3086, 0x3F28},
    {0x3086, 0x2AF2},
    {0x3086, 0x3F0F},
    {0x3086, 0x2982},
    {0x3086, 0x2983},
    {0x3086, 0x2943},
    {0x3086, 0x5C15},
    {0x3086, 0x5F4D},
    {0x3086, 0x1C2A},
    {0x3086, 0xFA45},
    {0x3086, 0x588E},
    {0x3086, 0x002A},
    {0x3086, 0x983F},
    {0x3086, 0x064A},
    {0x3086, 0x739D},
    {0x3086, 0x0A43},
    {0x3086, 0x160B},
    {0x3086, 0x4316},
    {0x3086, 0x8E03},
    {0x3086, 0x2A9C},
    {0x3086, 0x4578},
    {0x3086, 0x3F07},
    {0x3086, 0x2A9D},
    {0x3086, 0x3E12},
    {0x3086, 0x4558},
    {0x3086, 0x3F04},
    {0x3086, 0x8E01},
    {0x3086, 0x2A98},
    {0x3086, 0x8E00},
    {0x3086, 0x9176},
    {0x3086, 0x9C77},
    {0x3086, 0x9C46},
    {0x3086, 0x4416},
    {0x3086, 0x1690},
    {0x3086, 0x7A12},
    {0x3086, 0x444B},
    {0x3086, 0x4A00},
    {0x3086, 0x4316},
    {0x3086, 0x6343},
    {0x3086, 0x1608},
    {0x3086, 0x4316},
    {0x3086, 0x5043},
    {0x3086, 0x1665},
    {0x3086, 0x4316},
    {0x3086, 0x6643},
    {0x3086, 0x168E},
    {0x3086, 0x032A},
    {0x3086, 0x9C45},
    {0x3086, 0x783F},
    {0x3086, 0x072A},
    {0x3086, 0x9D5D},
    {0x3086, 0x0C29},
    {0x3086, 0x4488},
    {0x3086, 0x102B},
    {0x3086, 0x0453},
    {0x3086, 0x0D8B},
    {0x3086, 0x1686},
    {0x3086, 0x3E1F},
    {0x3086, 0x4558},
    {0x3086, 0x283E},
    {0x3086, 0x068E},
    {0x3086, 0x012A},
    {0x3086, 0x988E},
    {0x3086, 0x008D},
    {0x3086, 0x6012},
    {0x3086, 0x444B},
    {0x3086, 0x2C2C},
    {0x3086, 0x2C2C},

    {0x3ED6, 0x34B3},
    {0x2436, 0x000E},
    {0x320C, 0x0180},
    {0x320E, 0x0300},
    {0x3210, 0x0500},
    {0x3204, 0x0B6D},
    {0x30FE, 0x0080},
    {0x3ED8, 0x7B99},
    {0x3EDC, 0x9BA8},
    {0x3EDA, 0x9B9B},
    {0x3092, 0x006F},
    {0x3EEC, 0x1C04},
    {0x3EF6, 0xA70F},
    {0x3044, 0x0410},
    {0x3ED0, 0xFF44},
    {0x3ED4, 0x031F},
    {0x30FE, 0x0080},
    {0x3EE2, 0x8866},
    {0x3EE4, 0x6623},
    {0x3EE6, 0x2263},
    {0x30E0, 0x4283},
    {0x30F0, 0x1283},
    {0x30B0, 0x0118}, //digital_test
    {0x30BA, 0x769C}, //digital_ctrl,
    {0x31AC, 0x100C}, //data_format_bits, raw bit-width
    {0x302A, 0x0006}, //vt_pix_clk_div
    {0x302C, 0x0002}, //vt_sys_clk_div
    {0x302E, 0x0001}, //pre_pll_clk_div
    {0x3030, 0x003C}, //pll_multiplier
    {0x3036, 0x000C}, //op_pix_clk_div
    {0x3038, 0x0002}, //op_sys_clk_div
    {0x3002, 4+(AR0230_MAX_HEIGHT-AR0230_HEIGHT)/2},  //y_addr_start
    {0x3004, 8+(AR0230_MAX_WIDTH-AR0230_WIDTH)/2},  //x_addr_start
    {0x3006, 4+(AR0230_MAX_HEIGHT-AR0230_HEIGHT)/2 + AR0230_HEIGHT - 1},  //y_addr_end
    {0x3008, 8+(AR0230_MAX_WIDTH-AR0230_WIDTH)/2 + AR0230_WIDTH - 1},   //x_addr_end
    {0x300A, 0x0452}, //frame_length_lines: vts
    {0x300C, 0x045E}, //line_length_pck: hts
    {0x3012, 0x01E0}, //coarse_integration_time
    {0x30A2, 0x0001}, //x_odd_inc
    {0x30A6, 0x0001}, //y_odd_inc
    {0x3040, 0x0000}, //read_mode: flip
    {0x31AE, 0x0301}, //serial_format: HiSPi lane
    {0x3082, 0x0008}, //operation_mode_ctrl: HDR mode, 16x
    {0x305E, 0x0080}, //global_gain
    {0x3176, 0x0080},
    {0x3178, 0x0080},
    {0x317A, 0x0080},
    {0x317C, 0x0080},
    {0x318A, 0x0E10}, //hdr_mc_ctrl1
    {0x318E, 0x0000}, //hdr_mc_ctrl3: HDR data
    {0x3190, 0x0000}, //hdr_mc_ctrl4
    {0x318C, 0xC000}, //hdr_mc_ctrl2
    {0x3192, 0x0400}, //hdr_mc_ctrl5
    {0x3198, 0x0C11}, //hdr_mc_ctrl8
    {0x30FE, 0x0000},
    {0x2400, 0x0002}, //altm_control: enable
#if 0
    {0x2410, 0x0020}, //altm_power_gain, power = power_gain * x + power_offset
    {0x2412, 0x0019}, //altm_power_offset
    {0x2420, 0x0010}, //altm_fsharp_v
    {0x2442, 0x00D0}, //altm_control_key_k0
    {0x2444, 0x0000}, //altm_control_key_k0_lo
    {0x2446, 0x0004}, //altm_control_key_k0_hi
    {0x2440, 0x0002}, //altm_control_damper
    {0x2450, 0x0000}, //altm_out_pedestal
    {0x2438, 0x0010}, //altm_control_min_factor
    {0x243A, 0x0020}, //altm_control_max_factor
    {0x243C, 0x0080}, //altm_control_dark_floor
    {0x243E, 0x0200}, //altm_control_bright_floor
#else
    {0x2410, 0x0028}, //altm_power_gain, power = power_gain * x + power_offset
    {0x2412, 0x0013}, //altm_power_offset
    {0x2420, 0x0013}, //altm_fsharp_v
    {0x2442, 0x0080}, //altm_control_key_k0
    {0x2444, 0x0000}, //altm_control_key_k01_lo
    {0x2446, 0x0004}, //altm_control_key_k01_hi
    {0x2440, 0x0004}, //altm_control_damper
    {0x2450, 0x0000}, //altm_out_pedestal
    {0x2438, 0x0008}, //altm_control_min_factor
    {0x243A, 0x0020}, //altm_control_max_factor
    {0x243C, 0x0020}, //altm_control_dark_floor
    {0x243E, 0x0200}, //altm_control_bright_floor
#endif
    {0x2422, 8+(AR0230_MAX_WIDTH-AR0230_WIDTH)/2}, //altm_stats_ex_win_x_start
    {0x2424, AR0230_WIDTH},  //altm_stats_ex_win_x_width
    {0x2426, 4+(AR0230_MAX_HEIGHT-AR0230_HEIGHT)/2}, //altm_stats_ex_win_y_start
    {0x2428, AR0230_HEIGHT}, //altm_stats_ex_win_y_height
    {0x31D0, 0x0000}, //companding: compand disable
    {0x301E, 0x00A8}, //data_pedestal
    //{0x301E, 0x0028},
    {0x3200, 0x0002}, //adacd_control: adacd enable
    {0x3202, 0x00CF}, //adacd_noise_model1
    {0x3206, 0x0A06}, //adacd_noise_floor1
    {0x3208, 0x1A12}, //adacd_noise_floor2
    {0x320A, 0x0080}, //adacd_pedestal
    {0x31E0, 0x0200}, //pix_def_id
    {0x3060, 0x0000}, //analog_gain
    {0x3064, 0x1882}, //smia_test
    {0x306E, 0xF010}, //datapath_select
    {0x3180, 0x8089}, //delta_dk_control
    {0x3ED2, 0x1F46},
    {0x302E, 0x0003},
    {0x3030, 0x004A},
    {0x302C, 0x0002},
    {0x302A, 0x0004},
    {AR0230_REG_END, 0x00},    /* END MARKER */
};

static struct tx_isp_sensor_win_setting ar0230_win_sizes[] = {
    {
        .width                      = AR0230_WIDTH,
        .height                     = AR0230_HEIGHT,
        .fps                        = 30 << 16 | 1,
        .mbus_code                  = V4L2_MBUS_FMT_SGRBG12_1X12,
        .colorspace                 = V4L2_COLORSPACE_SRGB,
        .regs                       = ar0230_init_regs_1920_1080_30fps_HDR_dvp,
    },
};

#else
/*
 * the part of driver maybe modify about different sensor and different board.
 */
struct again_lut {
    unsigned int value;
    unsigned int gain;
};
/* static unsigned char again_mode = LCG; */
static struct again_lut ar0230_again_lut[] = {
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

static unsigned int ar0230_alloc_again(unsigned int isp_gain, unsigned char shift, unsigned int *sensor_again)
{
    struct again_lut *lut = ar0230_again_lut;

    while (lut->gain <= ar0230_attr.max_again) {
        if (isp_gain == 0) {
            *sensor_again = 0;
            return 0;
        } else if (isp_gain < lut->gain) {
            *sensor_again = (lut - 1)->value;
            return (lut - 1)->gain;
        } else {
            if ((lut->gain == ar0230_attr.max_again) && (isp_gain >= lut->gain)) {
                *sensor_again = lut->value;
                return lut->gain;
            }
        }

        lut++;
    }

    return isp_gain;
}

static unsigned int ar0230_alloc_dgain(unsigned int isp_gain, unsigned char shift, unsigned int *sensor_dgain)
{
    return isp_gain;
}

static struct tx_isp_sensor_attribute ar0230_attr={
    .name                                   = "ar0230",
    .chip_id                                = 0x0056,
    .cbus_type                              = TX_SENSOR_CONTROL_INTERFACE_I2C,
    .cbus_mask                              = V4L2_SBUS_MASK_SAMPLE_16BITS | V4L2_SBUS_MASK_ADDR_16BITS,
    .cbus_device                            = 0x10,
    .dbus_type                              = TX_SENSOR_DATA_INTERFACE_DVP,
    .dvp = {
        .mode = SENSOR_DVP_HREF_MODE,
        .blanking = {
            .vblanking = 0,
            .hblanking = 0,
        },
    },
    .max_again                              = 262144,
    .max_dgain                              = 0,
    .min_integration_time                   = 4,
    .min_integration_time_native            = 4,
    .max_integration_time_native            = 0x0546-4,
    .integration_time_limit                 = 0x0546-4,
    .total_width                            = 0x44c,
    .total_height                           = 0x0546,
    .max_integration_time                   = 0x0546-4,
    .integration_time_apply_delay           = 2,
    .again_apply_delay                      = 2,
    .dgain_apply_delay                      = 1,
    .sensor_ctrl.alloc_again                = ar0230_alloc_again,
    .sensor_ctrl.alloc_dgain                = ar0230_alloc_dgain,
    //    void priv; /* point to struct tx_isp_sensor_board_info */
};

static struct regval_list ar0230_init_regs_1920_1080_30fps_linear_dvp[] = {
    {0x301A, 0x0001},    // RESET_REGISTER
    {AR0230_REG_DELAY,200},
    {0x301A, 0x10D8},    // RESET_REGISTER
    {AR0230_REG_DELAY,200},
    {0x3088, 0x8242},
    {0x3086, 0x4558},
    {0x3086, 0x729B},
    {0x3086, 0x4A31},
    {0x3086, 0x4342},
    {0x3086, 0x8E03},
    {0x3086, 0x2A14},
    {0x3086, 0x4578},
    {0x3086, 0x7B3D},
    {0x3086, 0xFF3D},
    {0x3086, 0xFF3D},
    {0x3086, 0xEA2A},
    {0x3086, 0x043D},
    {0x3086, 0x102A},
    {0x3086, 0x052A},
    {0x3086, 0x1535},
    {0x3086, 0x2A05},
    {0x3086, 0x3D10},
    {0x3086, 0x4558},
    {0x3086, 0x2A04},
    {0x3086, 0x2A14},
    {0x3086, 0x3DFF},
    {0x3086, 0x3DFF},
    {0x3086, 0x3DEA},
    {0x3086, 0x2A04},
    {0x3086, 0x622A},
    {0x3086, 0x288E},
    {0x3086, 0x0036},
    {0x3086, 0x2A08},
    {0x3086, 0x3D64},
    {0x3086, 0x7A3D},
    {0x3086, 0x0444},
    {0x3086, 0x2C4B},
    {0x3086, 0x8F03},
    {0x3086, 0x430D},
    {0x3086, 0x2D46},
    {0x3086, 0x4316},
    {0x3086, 0x5F16},
    {0x3086, 0x530D},
    {0x3086, 0x1660},
    {0x3086, 0x3E4C},
    {0x3086, 0x2904},
    {0x3086, 0x2984},
    {0x3086, 0x8E03},
    {0x3086, 0x2AFC},
    {0x3086, 0x5C1D},
    {0x3086, 0x5754},
    {0x3086, 0x495F},
    {0x3086, 0x5305},
    {0x3086, 0x5307},
    {0x3086, 0x4D2B},
    {0x3086, 0xF810},
    {0x3086, 0x164C},
    {0x3086, 0x0955},
    {0x3086, 0x562B},
    {0x3086, 0xB82B},
    {0x3086, 0x984E},
    {0x3086, 0x1129},
    {0x3086, 0x9460},
    {0x3086, 0x5C19},
    {0x3086, 0x5C1B},
    {0x3086, 0x4548},
    {0x3086, 0x4508},
    {0x3086, 0x4588},
    {0x3086, 0x29B6},
    {0x3086, 0x8E01},
    {0x3086, 0x2AF8},
    {0x3086, 0x3E02},
    {0x3086, 0x2AFA},
    {0x3086, 0x3F09},
    {0x3086, 0x5C1B},
    {0x3086, 0x29B2},
    {0x3086, 0x3F0C},
    {0x3086, 0x3E03},
    {0x3086, 0x3E15},
    {0x3086, 0x5C13},
    {0x3086, 0x3F11},
    {0x3086, 0x3E0F},
    {0x3086, 0x5F2B},
    {0x3086, 0x902A},
    {0x3086, 0xF22B},
    {0x3086, 0x803E},
    {0x3086, 0x063F},
    {0x3086, 0x0660},
    {0x3086, 0x29A2},
    {0x3086, 0x29A3},
    {0x3086, 0x5F4D},
    {0x3086, 0x1C2A},
    {0x3086, 0xFA29},
    {0x3086, 0x8345},
    {0x3086, 0xA83E},
    {0x3086, 0x072A},
    {0x3086, 0xFB3E},
    {0x3086, 0x2945},
    {0x3086, 0x8824},
    {0x3086, 0x3E08},
    {0x3086, 0x2AFA},
    {0x3086, 0x5D29},
    {0x3086, 0x9288},
    {0x3086, 0x102B},
    {0x3086, 0x048B},
    {0x3086, 0x1686},
    {0x3086, 0x8D48},
    {0x3086, 0x4D4E},
    {0x3086, 0x2B80},
    {0x3086, 0x4C0B},
    {0x3086, 0x603F},
    {0x3086, 0x302A},
    {0x3086, 0xF23F},
    {0x3086, 0x1029},
    {0x3086, 0x8229},
    {0x3086, 0x8329},
    {0x3086, 0x435C},
    {0x3086, 0x155F},
    {0x3086, 0x4D1C},
    {0x3086, 0x2AFA},
    {0x3086, 0x4558},
    {0x3086, 0x8E00},
    {0x3086, 0x2A98},
    {0x3086, 0x3F0A},
    {0x3086, 0x4A0A},
    {0x3086, 0x4316},
    {0x3086, 0x0B43},
    {0x3086, 0x168E},
    {0x3086, 0x032A},
    {0x3086, 0x9C45},
    {0x3086, 0x783F},
    {0x3086, 0x072A},
    {0x3086, 0x9D3E},
    {0x3086, 0x305D},
    {0x3086, 0x2944},
    {0x3086, 0x8810},
    {0x3086, 0x2B04},
    {0x3086, 0x530D},
    {0x3086, 0x4558},
    {0x3086, 0x3E08},
    {0x3086, 0x8E01},
    {0x3086, 0x2A98},
    {0x3086, 0x8E00},
    {0x3086, 0x769C},
    {0x3086, 0x779C},
    {0x3086, 0x4644},
    {0x3086, 0x1616},
    {0x3086, 0x907A},
    {0x3086, 0x1244},
    {0x3086, 0x4B18},
    {0x3086, 0x4A04},
    {0x3086, 0x4316},
    {0x3086, 0x0643},
    {0x3086, 0x1605},
    {0x3086, 0x4316},
    {0x3086, 0x0743},
    {0x3086, 0x1658},
    {0x3086, 0x4316},
    {0x3086, 0x5A43},
    {0x3086, 0x1645},
    {0x3086, 0x588E},
    {0x3086, 0x032A},
    {0x3086, 0x9C45},
    {0x3086, 0x787B},
    {0x3086, 0x3F07},
    {0x3086, 0x2A9D},
    {0x3086, 0x530D},
    {0x3086, 0x8B16},
    {0x3086, 0x863E},
    {0x3086, 0x2345},
    {0x3086, 0x5825},
    {0x3086, 0x3E10},
    {0x3086, 0x8E01},
    {0x3086, 0x2A98},
    {0x3086, 0x8E00},
    {0x3086, 0x3E10},
    {0x3086, 0x8D60},
    {0x3086, 0x1244},
    {0x3086, 0x4B2C},
    {0x3086, 0x2C2C},
    {0x2436, 0x000E},
    {0x320C, 0x0180},
    {0x320E, 0x0300},
    {0x3210, 0x0500},
    {0x3204, 0x0B6D},
    {0x30FE, 0x0080},
    {0x3ED8, 0x7B99},
    {0x3EDC, 0x9BA8},
    {0x3EDA, 0x9B9B},
    {0x3092, 0x006F},
    {0x3EEC, 0x1C04},
    {0x30BA, 0x779C},
    {0x3EF6, 0xA70F},
    {0x3044, 0x0410},
    {0x3ED0, 0xFF44},
    {0x3ED4, 0x031F},
    {0x30FE, 0x0080},
    {0x3EE2, 0x8866},
    {0x3EE4, 0x6623},
    {0x3EE6, 0x2263},
    {0x30E0, 0x4283},
    {0x30F0, 0x1283},
    {0x301A, 0x10D8},    //RESET_REGISTER
    {0x30B0, 0x0118},    //DIGITAL_TEST enable low power
    {0x31AC, 0x0C0C},
    {0x302A, 0x0008},    //VT_PIX_CLK_DIV
    {0x302C, 0x0001},    //VT_SYS_CLK_DIV
    {0x302E, 0x0002},    //PRE_PLL_CLK_DIV
    {0x3030, 0x002C},    //PLL_MULTIPLIER
    {0x3036, 0x000C},    //OP_PIX_CLK_DIV
    {0x3038, 0x0001},    //OP_SYS_CLK_DIV
    {0x3002, 4+(AR0230_MAX_HEIGHT-AR0230_HEIGHT)/2},  //Y_ADDR_START
    {0x3004, 8+(AR0230_MAX_WIDTH-AR0230_WIDTH)/2},  //X_ADDR_START
    {0x3006, 4+(AR0230_MAX_HEIGHT-AR0230_HEIGHT)/2 + AR0230_HEIGHT - 1},  //Y_ADDR_END
    {0x3008, 8+(AR0230_MAX_WIDTH-AR0230_WIDTH)/2 + AR0230_WIDTH - 1},   //X_ADDR_END
    {0x300A, 0x0546},    //1125 vts
    {0x300C, 0x044c},    //1100 hts
    {0x3012, 0x0416},    //1046
    {0x30A2, 0x0001},
    {0x30A6, 0x0001},
    {0x3040, 0x0000},
    {0x31AE, 0x0301},
    {0x3082, 0x0009},
    {0x30BA, 0x769C},
    {0x3096, 0x0080},
    {0x3098, 0x0080},
    {0x31E0, 0x0200},
    {0x318C, 0x0000},
    {0x2400, 0x0003},
    {0x301E, 0x00A8},
    {0x2450, 0x0000},
    {0x320A, 0x0080},
    {0x31D0, 0x0001},
    {0x3200, 0x0000},
    {0x31D0, 0x0000},
    {0x3176, 0x0080},
    {0x3178, 0x0080},
    {0x317A, 0x0080},
    {0x317C, 0x0080},
    {0x3060, 0x000B},    // ANALOG_GAIN 1.5x Minimum analog Gain for LCG
    {0x3206, 0x0B08},
    {0x3208, 0x1E13},
    {0x3202, 0x0080},
    {0x3200, 0x0002},
    {0x3100, 0x0000},
    {0x3064, 0x1802},    //Disable Embedded Data and Stats
    {0x31C6, 0x0400},    //HISPI_CONTROL_STATUS: HispiSP
    {0x306E, 0x9210},    //DATAPATH_SELECT[9]=1 VDD_SLVS=1.8V
    {0x3060, 0x000f},
    {0x305e, 0x0080},
    {0x3012, 0xffff},
    /* {0x3070, 0x0002},       //color bar */
    {AR0230_REG_DELAY,33},
    {AR0230_REG_END, 0x00},    /* END MARKER */
};

/*
 * the order of the ar0230_win_sizes is [full_resolution, preview_resolution].
 */
static struct tx_isp_sensor_win_setting ar0230_win_sizes[] = {
    {
        .width                      = AR0230_WIDTH,
        .height                     = AR0230_HEIGHT,
        .fps                        = 25 << 16 | 1,
        .mbus_code                  = V4L2_MBUS_FMT_SGRBG12_1X12,
        .colorspace                 = V4L2_COLORSPACE_SRGB,
        .regs                       = ar0230_init_regs_1920_1080_30fps_linear_dvp,
    }
};
#endif

static enum v4l2_mbus_pixelcode ar0230_mbus_code[] = {
    V4L2_MBUS_FMT_SGRBG12_1X12,
    V4L2_MBUS_FMT_SBGGR12_1X12, //vflip on
    V4L2_MBUS_FMT_SGRBG10_1X10,
    V4L2_MBUS_FMT_SBGGR10_1X10, //vflip on
};

/*
 * the part of driver was fixed.
 */

static struct regval_list ar0230_stream_on[] = {
    {0x301A, 0x10DC},    //RESET_REGISTER
    {AR0230_REG_END, 0x00},    /* END MARKER */
};

static struct regval_list ar0230_stream_off[] = {
    {0x301A, 0x10D8},    //RESET_REGISTER
    {AR0230_REG_END, 0x00},    /* END MARKER */
};


static int ar0230_read(struct tx_isp_subdev *sd, unsigned short reg,
        unsigned short *value)
{
    struct i2c_client *client = tx_isp_get_subdevdata(sd);
    unsigned char buf[2] = {reg >> 8, reg & 0xff};
    unsigned char buf2[2];

    struct i2c_msg msg[2] = {
        [0] = {
            .addr    = client->addr,
            .flags    = 0,
            .len    = 2,
            .buf    = buf,
        },
        [1] = {
            .addr    = client->addr,
            .flags    = I2C_M_RD,
            .len    = 2,
            .buf    = buf2,
        }
    };

    int ret;
    ret = private_i2c_transfer(client->adapter, msg, 2);
    if (ret > 0)
        ret = 0;

    *value = (buf2[0] << 8) | buf2[1];

    return ret;
}

static int ar0230_write(struct tx_isp_subdev *sd, unsigned short reg,
        unsigned short value)
{
    struct i2c_client *client = tx_isp_get_subdevdata(sd);
    unsigned char buf[4] = {reg >> 8, reg & 0xff, value >> 8, value & 0xff};
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

static inline int ar0230_read_array(struct tx_isp_subdev *sd, struct regval_list *vals)
{
    int ret;
    unsigned short val;

    while (vals->reg_num != AR0230_REG_END) {
        if (vals->reg_num == AR0230_REG_DELAY) {
                private_msleep(vals->value);
        } else {
            ret = ar0230_read(sd, vals->reg_num, &val);
            if (ret < 0)
                return ret;
        }
        vals++;
    }
    return 0;
}
static int ar0230_write_array(struct tx_isp_subdev *sd, struct regval_list *vals)
{
    int ret;

    while (vals->reg_num != AR0230_REG_END) {
        if (vals->reg_num == AR0230_REG_DELAY) {
            private_msleep(vals->value);
        } else {
            ret = ar0230_write(sd, vals->reg_num, vals->value);
            if (ret < 0)
                return ret;
        }
        vals++;
    }

    return 0;
}

static int ar0230_reset(struct tx_isp_subdev *sd, int val)
{
    return 0;
}

static int ar0230_detect(struct tx_isp_subdev *sd, unsigned int *ident)
{
    unsigned short val;
    int ret;

    ret = ar0230_read(sd, 0x3000, &val);
    if (ret < 0)
        return ret;

    if (val != AR0230_CHIP_ID)
        return -ENODEV;

    return 0;
}

static int ar0230_set_integration_time(struct tx_isp_subdev *sd, int value)
{
    int ret;

#ifdef MD_X1830_AR0230_HDR
#if 0
    static int integration_time_last = 0;
    if (16 == MD_X1830_AR0230_RATIO_T1_T2) {
        if ((value - integration_time_last) > 64) {
            integration_time_last += 64;
        } else if ((integration_time_last - value) > 64) {
            integration_time_last -= 64;
        } else {
            integration_time_last = value;
        }
        value = integration_time_last;
    }
    integration_time_last = value;
#endif
#endif
    ret = ar0230_write(sd, 0x3012, value);
    if (ret < 0)
        return ret;

    return 0;
}

static int ar0230_set_analog_gain(struct tx_isp_subdev *sd, int value)
{
    int ret = 0;
#ifdef MD_X1830_AR0230_HDR
    static int again_mode_last = LCG;

    if (again_mode != again_mode_last) {
        if (again_mode == LCG) {
            ret += ar0230_write(sd,0x3100, 0x00);
            ret += ar0230_write(sd,0x3096, 0x780);
            ret += ar0230_write(sd,0x3098, 0x780);
            ret += ar0230_write(sd,0x3206, 0x0B08);
            ret += ar0230_write(sd,0x3208, 0x1E13);
            ret += ar0230_write(sd,0x3202, 0x0080);

            again_mode_last = again_mode;
        } else if (again_mode == HCG) {
            ret += ar0230_write(sd,0x3100, 0x04);
            ret += ar0230_write(sd,0x3096, 0x480);
            ret += ar0230_write(sd,0x3098, 0x480);
            ret += ar0230_write(sd,0x3206, 0x1C0E);
            ret += ar0230_write(sd,0x3208, 0x4E39);
            ret += ar0230_write(sd,0x3202, 0x00B0);

            again_mode_last = again_mode;
        } else {
            printk(KERN_ERR "Do not support this Again: %d(%x)\n", again_mode, value);
        }
    }
#endif
    ret = ar0230_write(sd, 0x3060, value);
    if (ret < 0)
        return ret;

    return 0;
}

static int ar0230_set_digital_gain(struct tx_isp_subdev *sd, int value)
{
    return 0;
}

static int ar0230_get_black_pedestal(struct tx_isp_subdev *sd, int value)
{
    return 0;
}

static int ar0230_init(struct tx_isp_subdev *sd, int enable)
{
    struct tx_isp_sensor *sensor = sd_to_sensor_device(sd);
    struct tx_isp_sensor_win_setting *wsize = &ar0230_win_sizes[0];
    int ret = 0;

    if(!enable)
        return ISP_SUCCESS;
    sensor->video.mbus.width                = wsize->width;
    sensor->video.mbus.height               = wsize->height;
    sensor->video.mbus.code                 = wsize->mbus_code;
    sensor->video.mbus.field                = V4L2_FIELD_NONE;
    sensor->video.mbus.colorspace           = wsize->colorspace;
    sensor->video.fps                       = wsize->fps;

    ret = ar0230_write_array(sd, wsize->regs);
#ifdef MD_X1830_AR0230_HDR
    ret += ar0230_write(sd, 0x3082, (private_log2_fixed_to_fixed(MD_X1830_AR0230_RATIO_T1_T2, 0, 0)-2) << 2);
#endif
    if (ret)
        return ret;

    ret = tx_isp_call_subdev_notify(sd, TX_ISP_EVENT_SYNC_SENSOR_ATTR, &sensor->video);
    sensor->priv = wsize;

    return 0;
}


static int ar0230_s_stream(struct tx_isp_subdev *sd, int enable)
{
    int ret = 0;

    if (enable) {
        ret = ar0230_write_array(sd, ar0230_stream_on);
        pr_debug("ar0230 stream on\n");
    } else {
        ret = ar0230_write_array(sd, ar0230_stream_off);
        pr_debug("ar0230 stream off\n");
    }

    return ret;
}

static int ar0230_set_fps(struct tx_isp_subdev *sd, int fps)
{
    struct tx_isp_sensor *sensor = sd_to_sensor_device(sd);
    unsigned int pclk = AR0230_SUPPORT_SCLK;
    unsigned short hts, vts;
    unsigned int newformat; //the format is 24.8
    int ret = 0;
    /* the format of fps is 16/16. for example 25 << 16 | 2, the value is 25/2 fps. */
    newformat = (((fps >> 16) / (fps & 0xffff)) << 8) + ((((fps >> 16) % (fps & 0xffff)) << 8) / (fps & 0xffff));
    if (newformat > (SENSOR_OUTPUT_MAX_FPS << 8) || newformat < (SENSOR_OUTPUT_MIN_FPS << 8))
        return -1;

    ret += ar0230_read(sd, 0x300C, &hts);
    if (ret < 0)
        return -1;

    vts = pclk * (fps & 0xffff) / hts / ((fps & 0xffff0000) >> 16);
    ret = ar0230_write(sd, 0x300A, vts);
    if (ret < 0)
        return -1;

    sensor->video.fps = fps;
    sensor->video.attr->total_height = vts;
#ifdef MD_X1830_AR0230_HDR
    vts = vts * EXPOSURE_RATIO;
#endif
    sensor->video.attr->max_integration_time_native     = vts - 4;
    sensor->video.attr->integration_time_limit          = vts - 4;
    sensor->video.attr->max_integration_time            = vts - 4;

    ret = tx_isp_call_subdev_notify(sd, TX_ISP_EVENT_SYNC_SENSOR_ATTR, &sensor->video);

    return ret;
}

static int ar0230_set_mode(struct tx_isp_subdev *sd, int value)
{
    struct tx_isp_sensor *sensor = sd_to_sensor_device(sd);
    struct tx_isp_sensor_win_setting *wsize = NULL;
    int ret = ISP_SUCCESS;

    if (value == TX_ISP_SENSOR_FULL_RES_MAX_FPS) {
        wsize = &ar0230_win_sizes[0];
    } else if (value == TX_ISP_SENSOR_PREVIEW_RES_MAX_FPS) {
        wsize = &ar0230_win_sizes[0];
    }

    if (wsize) {
        sensor->video.mbus.width        = wsize->width;
        sensor->video.mbus.height       = wsize->height;
        sensor->video.mbus.code         = wsize->mbus_code;
        sensor->video.mbus.field        = V4L2_FIELD_NONE;
        sensor->video.mbus.colorspace   = wsize->colorspace;
        sensor->video.fps               = wsize->fps;
        ret = tx_isp_call_subdev_notify(sd, TX_ISP_EVENT_SYNC_SENSOR_ATTR, &sensor->video);
    }

    return ret;
}

static int ar0230_set_vflip(struct tx_isp_subdev *sd, int enable)
{
    struct tx_isp_sensor *sensor = sd_to_sensor_device(sd);
    int ret = 0;
    unsigned short val = 0;

    ret = ar0230_read(sd, 0x3040, &val);
    val = (val & 0xc000) >> 14;
    if (enable) {
        val |= 0x02;
        switch (dvp_gpio_func) {
            case DVP_PA_LOW_10BIT:
            case DVP_PA_HIGH_10BIT:
                sensor->video.mbus.code = ar0230_mbus_code[3];
                break;
            case DVP_PA_12BIT:
                sensor->video.mbus.code = ar0230_mbus_code[1];
                break;
            default:
                printk(KERN_ERR "vfilp fail, unsupport code\n");
        }
    } else {
        val &= 0xfd;
        switch (dvp_gpio_func) {
            case DVP_PA_LOW_10BIT:
            case DVP_PA_HIGH_10BIT:
                sensor->video.mbus.code = ar0230_mbus_code[2];
                break;
            case DVP_PA_12BIT:
                sensor->video.mbus.code = ar0230_mbus_code[0];
                break;
            default:
                printk(KERN_ERR "vfilp fail, unsupport code\n");
        }
    }

    sensor->video.mbus_change = 0;

    ret += ar0230_write(sd, 0x301c, val);
    ret += ar0230_write_array(sd, ar0230_stream_on);
    if(!ret)
        ret = tx_isp_call_subdev_notify(sd, TX_ISP_EVENT_SYNC_SENSOR_ATTR, &sensor->video);

    return ret;
}

static int ar0230_g_chip_ident(struct tx_isp_subdev *sd,
        struct tx_isp_chip_ident *chip)
{
    struct i2c_client *client = tx_isp_get_subdevdata(sd);
    unsigned int ident = 0;
    int ret = ISP_SUCCESS;

    if (power_gpio != -1) {
        ret = gpio_request(power_gpio, "ar0230_power");
        if (!ret) {
            private_gpio_direction_output(power_gpio, 1);
            private_msleep(50);
        } else {
            printk(KERN_ERR "ar0230: gpio requrest fail %d\n", power_gpio);
            return -1;
        }
    }

    if (pwdn_gpio != -1) {
        ret = private_gpio_request(pwdn_gpio,"ar0230_pwdn");
        if (!ret) {
            private_gpio_direction_output(pwdn_gpio, 1);
            private_msleep(150);
            private_gpio_direction_output(pwdn_gpio, 0);
            private_msleep(10);
        } else {
            printk(KERN_ERR "ar0230: gpio requrest fail %d\n",pwdn_gpio);
            return -1;
        }
    }

    if (reset_gpio != -1) {
        ret = private_gpio_request(reset_gpio,"ar0230_reset");
        if (!ret) {
            private_gpio_direction_output(reset_gpio, 1);
            private_msleep(20);
            private_gpio_direction_output(reset_gpio, 0);
            private_msleep(20);
            private_gpio_direction_output(reset_gpio, 1);
            private_msleep(20);
        } else {
            printk(KERN_ERR "ar0230: gpio requrest fail %d\n",reset_gpio);
            return -1;
        }
    }

    if (i2c_sel_gpio != -1) {
        int ret = gpio_request(i2c_sel_gpio, "ar0230_i2c_sel");
        if (!ret) {
            private_gpio_direction_output(i2c_sel_gpio, 1);
        } else {
            printk(KERN_ERR "ar0230: gpio requrest fail %d\n", i2c_sel_gpio);
            return -1;
        }
    }

    ret = ar0230_detect(sd, &ident);
    if (ret) {
        printk(KERN_ERR "ar0230: chip found @ 0x%x (%s) is not an ar0230 chip.\n",
                client->addr, client->adapter->name);
        return ret;
    }
    printk(KERN_ERR "ar0230: ar0230 chip found @ 0x%02x (%s)\n", client->addr, client->adapter->name);
    if (chip) {
        memcpy(chip->name, "ar0230", sizeof("ar0230"));
        chip->ident = ident;
        chip->revision = SENSOR_VERSION;
    }

    return 0;
}

static int ar0230_sensor_ops_ioctl(struct tx_isp_subdev *sd, unsigned int cmd, void *arg)
{
    long ret = 0;

    if (IS_ERR_OR_NULL(sd)) {
        printk(KERN_ERR "ar0230: [%d]The pointer is invalid!\n", __LINE__);
        return -EINVAL;
    }

    switch (cmd) {
        case TX_ISP_EVENT_SENSOR_INT_TIME:
            if (arg)
                ret = ar0230_set_integration_time(sd, *(int*)arg);
            break;
        case TX_ISP_EVENT_SENSOR_AGAIN:
            if (arg)
                ret = ar0230_set_analog_gain(sd, *(int*)arg);
            break;
        case TX_ISP_EVENT_SENSOR_DGAIN:
            if (arg)
                ret = ar0230_set_digital_gain(sd, *(int*)arg);
            break;
        case TX_ISP_EVENT_SENSOR_BLACK_LEVEL:
            if (arg)
                ret = ar0230_get_black_pedestal(sd, *(int*)arg);
            break;
        case TX_ISP_EVENT_SENSOR_RESIZE:
            if (arg)
                ret = ar0230_set_mode(sd, *(int*)arg);
            break;
        case TX_ISP_EVENT_SENSOR_PREPARE_CHANGE:
            ret = ar0230_write_array(sd, ar0230_stream_off);
            break;
        case TX_ISP_EVENT_SENSOR_FINISH_CHANGE:
            ret = ar0230_write_array(sd, ar0230_stream_on);
            break;
        case TX_ISP_EVENT_SENSOR_FPS:
            if (arg)
                ret = ar0230_set_fps(sd, *(int*)arg);
            break;
        case TX_ISP_EVENT_SENSOR_VFLIP:
            if (arg)
                ret = ar0230_set_vflip(sd, *(int*)arg);
            break;
        default:
            break;;
    }

    return 0;
}

static int ar0230_g_register(struct tx_isp_subdev *sd, struct tx_isp_dbg_register *reg)
{
    unsigned short val = 0;
    int len = 0;
    int ret = 0;

    len = strlen(sd->chip.name);
    if (len && strncmp(sd->chip.name, reg->name, len)) {
        return -EINVAL;
    }

    if (!private_capable(CAP_SYS_ADMIN))
        return -EPERM;

    ret = ar0230_read(sd, reg->reg & 0xffff, &val);
    reg->val = val;
    reg->size = 2;

    return ret;
}

static int ar0230_s_register(struct tx_isp_subdev *sd, const struct tx_isp_dbg_register *reg)
{
    int len = 0;

    len = strlen(sd->chip.name);
    if (len && strncmp(sd->chip.name, reg->name, len)) {
        return -EINVAL;
    }

    if (!private_capable(CAP_SYS_ADMIN))
        return -EPERM;

    ar0230_write(sd, reg->reg & 0xffff, reg->val & 0xffff);

    return 0;
}

static struct tx_isp_subdev_core_ops ar0230_core_ops = {
    .g_chip_ident               = ar0230_g_chip_ident,
    .reset                      = ar0230_reset,
    .init                       = ar0230_init,
    .g_register                 = ar0230_g_register,
    .s_register                 = ar0230_s_register,
};

static struct tx_isp_subdev_video_ops ar0230_video_ops = {
    .s_stream                   = ar0230_s_stream,
};

static struct tx_isp_subdev_sensor_ops    ar0230_sensor_ops = {
    .ioctl                      = ar0230_sensor_ops_ioctl,
};

static struct tx_isp_subdev_ops ar0230_ops = {
    .core                       = &ar0230_core_ops,
    .video                      = &ar0230_video_ops,
    .sensor                     = &ar0230_sensor_ops,
};

/* It's the sensor device */
static u64 tx_isp_module_dma_mask = ~(u64)0;
struct platform_device ar0230_platform_device = {
    .name                   = "ar0230",
    .id                     = -1,
    .dev = {
        .dma_mask = &tx_isp_module_dma_mask,
        .coherent_dma_mask = 0xffffffff,
        .platform_data = NULL,
    },
    .num_resources          = 0,
};


static int ar0230_probe(struct i2c_client *client,
        const struct i2c_device_id *id)
{
    struct tx_isp_subdev *sd;
    struct tx_isp_video_in *video;
    struct tx_isp_sensor *sensor;
    struct tx_isp_sensor_win_setting *wsize = &ar0230_win_sizes[0];
    enum v4l2_mbus_pixelcode mbus;
    int i, ret;

    for (i = 0; i < ARRAY_SIZE(dvp_gpio_func_array); i++) {
        if (!strcmp(dvp_gpio_func_str, dvp_gpio_func_array[i])) {
            dvp_gpio_func = i;
            break;
        }
    }

    if (i == ARRAY_SIZE(dvp_gpio_func_array))
        printk(KERN_ERR "sensor dvp_gpio_func set error!\n");

    sensor = (struct tx_isp_sensor *)kzalloc(sizeof(*sensor), GFP_KERNEL);
    if (!sensor) {
        printk(KERN_ERR "ar0230: Failed to allocate sensor subdev.\n");
        return -ENOMEM;
    }

    ret = tx_isp_clk_set(MD_X1830_AR0230_ISPCLK);
    if (ret < 0) {
        printk(KERN_ERR "ar0230: Cannot set isp clock\n");
        goto err_get_mclk;
    }

    /* request mclk of sensor */
    sensor->mclk = clk_get(NULL, "cgu_cim");
    if (IS_ERR(sensor->mclk)) {
        printk(KERN_ERR "ar0230: Cannot get sensor input clock cgu_cim\n");
        goto err_get_mclk;
    }

    clk_set_rate(sensor->mclk, 24000000);
    clk_enable(sensor->mclk);

    ret = set_sensor_gpio_function(dvp_gpio_func);
    if (ret < 0)
        goto err_set_sensor_gpio;

    ar0230_attr.dvp.gpio = dvp_gpio_func;

    switch (dvp_gpio_func) {
        case DVP_PA_LOW_10BIT:
        case DVP_PA_HIGH_10BIT:
            mbus = ar0230_mbus_code[2];
            break;
        case DVP_PA_12BIT:
            mbus = ar0230_mbus_code[0];
            break;
        default:
            goto err_set_sensor_gpio;
    }

    for (i = 0; i < ARRAY_SIZE(ar0230_win_sizes); i++)
        ar0230_win_sizes[i].mbus_code = mbus;

    sd = &sensor->sd;
    video = &sensor->video;

    sensor->video.attr                  = &ar0230_attr;
    sensor->video.mbus_change           = 1;
    sensor->video.vi_max_width          = wsize->width;
    sensor->video.vi_max_height         = wsize->height;
    sensor->video.mbus.width            = wsize->width;
    sensor->video.mbus.height           = wsize->height;
    sensor->video.mbus.code             = wsize->mbus_code;
    sensor->video.mbus.field            = V4L2_FIELD_NONE;
    sensor->video.mbus.colorspace       = wsize->colorspace;
    sensor->video.fps                   = wsize->fps;
    sensor->video.dc_param              = NULL;

    tx_isp_subdev_init(&ar0230_platform_device, sd, &ar0230_ops);
    tx_isp_set_subdevdata(sd, client);
    tx_isp_set_subdev_hostdata(sd, sensor);
    private_i2c_set_clientdata(client, sd);

    pr_debug("probe ok ------->ar0230\n");

    return 0;
err_set_sensor_gpio:
    private_clk_disable(sensor->mclk);
    private_clk_put(sensor->mclk);
err_get_mclk:
    kfree(sensor);

    return -1;
}

static int ar0230_remove(struct i2c_client *client)
{
    struct tx_isp_subdev *sd = private_i2c_get_clientdata(client);
    struct tx_isp_sensor *sensor = tx_isp_get_subdev_hostdata(sd);

    if (power_gpio != -1) {
        private_gpio_direction_output(power_gpio, 0);
        private_gpio_free(power_gpio);
    }

    if (i2c_sel_gpio != -1)
        private_gpio_free(i2c_sel_gpio);

    if (reset_gpio != -1)
        private_gpio_free(reset_gpio);

    if (pwdn_gpio != -1)
        private_gpio_free(pwdn_gpio);

    private_clk_disable(sensor->mclk);
    private_clk_put(sensor->mclk);
    tx_isp_subdev_deinit(sd);
    kfree(sensor);

    return 0;
}

static const struct i2c_device_id ar0230_id[] = {
    { "ar0230", 0 },
    { }
};
MODULE_DEVICE_TABLE(i2c, ar0230_id);

static struct i2c_driver ar0230_driver = {
    .driver = {
        .owner          = THIS_MODULE,
        .name           = "ar0230",
    },
    .probe              = ar0230_probe,
    .remove             = ar0230_remove,
    .id_table           = ar0230_id,
};

static __init int init_ar0230(void)
{
    int ret = 0;

    ret = private_driver_get_interface();
    if (ret) {
        printk(KERN_ERR "ar0230: Failed to init ar0230 dirver.\n");
        return -1;
    }

    return private_i2c_add_driver(&ar0230_driver);
}

static __exit void exit_ar0230(void)
{
    private_i2c_del_driver(&ar0230_driver);
}

module_init(init_ar0230);
module_exit(exit_ar0230);

MODULE_DESCRIPTION("x1830 ar0230 driver depend on isp");
MODULE_LICENSE("GPL");
