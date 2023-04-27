/*
 * Copyright (C) 2020 Ingenic Semiconductor Co., Ltd.
 *
 * AR0230
 *
 */

#include <linux/module.h>
#include <utils/gpio.h>
#include <utils/i2c.h>
#include <common.h>
#include <camera/hal/camera_sensor.h>
#include <camera/hal/dvp_gpio_func.h>



#define CONFIG_AR0230_1920X1080

#define AR0230_DEVICE_NAME              "ar0230"
#define AR0230_DEVICE_I2C_ADDR          0x10

#define AR0230_CHIP_ID                  0x0056
#define AR0230_REG_END                  0xffff
#define AR0230_REG_DELAY                0xfffe
#define AR0230_SUPPORT_SCLK             (37125000)
#define SENSOR_OUTPUT_MAX_FPS           30
#define SENSOR_OUTPUT_MIN_FPS           5
#define AR0230_MAX_WIDTH                1920
#define AR0230_MAX_HEIGHT               1080
#if defined CONFIG_AR0230_1920X1080
#define AR0230_WIDTH                    1920
#define AR0230_HEIGHT                   1080
#endif

#define CONFIG_AR0230_HDR
#ifdef CONFIG_AR0230_HDR
//#define CONFIG_AR0230_RATIO_T1_T2       8
#define CONFIG_AR0230_RATIO_T1_T2       16
#define EXPOSURE_RATIO                  CONFIG_AR0230_RATIO_T1_T2/(CONFIG_AR0230_RATIO_T1_T2+1)
#define LCG                             0x0
#define HCG                             0x4
int again_mode;
#endif


static int power_gpio   = -1;
static int reset_gpio   = -1;   // GPIO_PE22
static int pwdn_gpio    = -1;
static int i2c_bus_num  = -1;   // 2
static int dvp_gpio_func = -1;

module_param_gpio(power_gpio, 0644);
module_param_gpio(reset_gpio, 0644);
module_param_gpio(pwdn_gpio, 0644);
module_param(i2c_bus_num, int, 0644);

static char *dvp_gpio_func_str = "DVP_PA_LOW_10BIT";
module_param(dvp_gpio_func_str, charp, 0644);
MODULE_PARM_DESC(dvp_gpio_func_str, "Sensor GPIO function");

static struct i2c_client *i2c_dev;
static int vic_index = 0;

static struct sensor_attr ar0230_sensor_attr;

struct regval_list {
    unsigned short reg_num;
    unsigned short value;
};

struct again_lut {
#ifdef CONFIG_AR0230_HDR
    unsigned char mode;
#endif
    unsigned int value;
    unsigned int gain;
};
/* static unsigned char again_mode = LCG; */
struct again_lut ar0230_again_lut[] = {
#ifdef CONFIG_AR0230_HDR
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
#else
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
#endif
};

#ifdef CONFIG_AR0230_HDR
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
#else
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
#endif

static struct regval_list ar0230_regs_stream_on[] = {
    {0x301A, 0x10DC},    //RESET_REGISTER
    {AR0230_REG_END, 0x00},    /* END MARKER */
};

static struct regval_list ar0230_regs_stream_off[] = {
    {0x301A, 0x10D8},    //RESET_REGISTER
    {AR0230_REG_END, 0x00},    /* END MARKER */
};


static int ar0230_write(struct i2c_client *i2c, unsigned short reg, unsigned short value)
{
    unsigned char buf[4] = {reg >> 8, reg & 0xff, value >> 8, value & 0xff};
    struct i2c_msg msg = {
        .addr = i2c->addr,
        .flags  = 0,
        .len    = 4,
        .buf    = buf,
    };

    int ret = i2c_transfer(i2c->adapter, &msg, 1);
    if (ret < 0)
        printk(KERN_ERR "ar0230: failed to write reg: %x\n", (int)reg);

    return ret;
}

static int ar0230_read(struct i2c_client *i2c, unsigned short reg, unsigned short *value)
{
    unsigned char buf[2] = {reg >> 8, reg & 0xff};
    unsigned char buf2[2];

    struct i2c_msg msg[2] = {
        [0] = {
            .addr = i2c->addr,
            .flags  = 0,
            .len    = 2,
            .buf    = buf,
        },
        [1] = {
            .addr = i2c->addr,
            .flags  = I2C_M_RD,
            .len    = 2,
            .buf    = buf2,
        }
    };

    int ret = i2c_transfer(i2c->adapter, msg, 2);
    if (ret < 0)
        printk(KERN_ERR "ar0230: failed to read reg: %x\n", (int)reg);
    else
        *value = (buf2[0] << 8) | buf2[1];

    return ret;
}

static inline int ar0230_read_array(struct i2c_client *i2c, struct regval_list *vals)
{
    int ret;
    unsigned short val;
    while (vals->reg_num != AR0230_REG_END) {
        if (vals->reg_num == AR0230_REG_DELAY) {
                m_msleep(vals->value);
        } else {
            ret = ar0230_read(i2c, vals->reg_num, &val);
            if (ret < 0)
                return ret;
        }
        printk(KERN_ERR "vals->reg_num:%x, vals->value:%x\n", vals->reg_num, val);
        vals++;
    }
    return 0;
}

static int ar0230_write_array(struct i2c_client *i2c, struct regval_list *vals)
{
    int ret;
    while (vals->reg_num != AR0230_REG_END) {
        if (vals->reg_num == AR0230_REG_DELAY) {
                m_msleep(vals->value);
        } else {
            ret = ar0230_write(i2c, vals->reg_num, vals->value);
            if (ret < 0)
                return ret;
        }
        vals++;
    }

    return 0;
}


static int ar0230_detect(struct i2c_client *i2c)
{
    unsigned short val;
    int ret;

    ret = ar0230_read(i2c, 0x3000, &val);
    if (ret < 0)
        return ret;
    if (val != AR0230_CHIP_ID)
        return -ENODEV;

    return 0;
}

static int init_gpio(void)
{
    int ret;
    char gpio_str[10];

    printk(KERN_ERR "ar0230 reset_gpio %d pwdn_gpio %d power_gpio %d\n", reset_gpio,pwdn_gpio,power_gpio);

    if (reset_gpio != -1) {
        ret = gpio_request(reset_gpio, "ar0230_reset");
        if (ret) {
            printk(KERN_ERR "ar0230: failed to request rst pin: %s\n", gpio_to_str(reset_gpio, gpio_str));
            goto err_reset_gpio;
        }
    }

    if (pwdn_gpio != -1) {
        ret = gpio_request(pwdn_gpio, "ar0230_pwdn");
        if (ret) {
            printk(KERN_ERR "ar0230: failed to request pwdn pin: %s\n", gpio_to_str(pwdn_gpio, gpio_str));
            goto err_pwdn_gpio;
        }
    }

    if (power_gpio != -1) {
        ret = gpio_request(power_gpio, "ar0230_power");
        if (ret) {
            printk(KERN_ERR "ar0230: failed to request power pin: %s\n", gpio_to_str(power_gpio, gpio_str));
            goto err_power_gpio;
        }
    }

    return 0;

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

    dvp_deinit_gpio();
}

static void ar0230_power_off(void)
{
    if (reset_gpio != -1)
        gpio_direction_output(reset_gpio, 0);

    if (pwdn_gpio != -1)
        gpio_direction_output(pwdn_gpio, 1);

    if (power_gpio != -1)
        gpio_direction_output(power_gpio, 0);

    camera_disable_sensor_mclk(vic_index);
}

extern uint32_t tisp_log2_fixed_to_fixed(const uint32_t val, const int in_fix_point, const uint8_t out_fix_point);
static int ar0230_power_on(void)
{
    int ret;

    camera_enable_sensor_mclk(vic_index, 24 * 1000 * 1000);

    if (power_gpio != -1) {
        gpio_direction_output(power_gpio, 1);
        m_msleep(50);
    }

    if (pwdn_gpio != -1){
        gpio_direction_output(pwdn_gpio, 1);
        m_msleep(50);
        gpio_direction_output(pwdn_gpio, 0);
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

    ret = ar0230_detect(i2c_dev);
    if (ret) {
        printk(KERN_ERR "ar0230: failed to detect\n");
       ar0230_power_off();
        return ret;
    }

    ret = ar0230_write_array(i2c_dev, ar0230_sensor_attr.sensor_info.private_init_setting);
#ifdef CONFIG_AR0230_HDR
    //ret += ar0230_write(i2c_dev, 0x3082, (tisp_log2_fixed_to_fixed(CONFIG_AR0230_RATIO_T1_T2, 0, 0)-2) << 2);
#endif
    if (ret) {
        printk(KERN_ERR "ar0230: failed to init regs\n");
        ar0230_power_off();
        return ret;
    }

    return 0;
}

static int ar0230_stream_on(void)
{
    int ret = ar0230_write_array(i2c_dev, ar0230_regs_stream_on);
    if (ret)
        printk(KERN_ERR "ar0230: failed to stream on\n");

    return ret;
}

static void ar0230_stream_off(void)
{
    int ret = ar0230_write_array(i2c_dev, ar0230_regs_stream_off);
    if (ret)
        printk(KERN_ERR "ar0230: failed to stream on\n");
}

static int ar0230_g_register(struct sensor_dbg_register *reg)
{
    unsigned short val;
    int ret;

    ret = ar0230_read(i2c_dev, reg->reg & 0xffff, &val);
    reg->val = val;
    reg->size = 2;
    return ret;
}

static int ar0230_s_register(struct sensor_dbg_register *reg)
{
    return ar0230_write(i2c_dev, reg->reg & 0xffff, reg->val & 0xffff);
}

static int ar0230_set_integration_time(int value)
{
    int ret;

#ifdef CONFIG_AR0230_HDR
#if 0
    static int integration_time_last = 0;
    if (16 == CONFIG_AR0230_RATIO_T1_T2) {
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
    ret = ar0230_write(i2c_dev, 0x3012, value);
    if (ret < 0)
        return ret;

    return 0;
}


unsigned int ar0230_alloc_again(unsigned int isp_gain, unsigned char shift, unsigned int *sensor_again)
{
    struct again_lut *lut = ar0230_again_lut;

    while(lut->gain <= ar0230_sensor_attr.sensor_info.max_again) {
#ifdef CONFIG_AR0230_HDR
        if(isp_gain <= 39587) {
            again_mode = LCG;
            *sensor_again = 0x0b;
            return 39587;
        }
#else
        if(isp_gain == 0) {
            *sensor_again = 0;
            return 0;
        }
#endif
        else if(isp_gain < lut->gain) {
#ifdef CONFIG_AR0230_HDR
            again_mode = (lut - 1)->mode;
#endif
            *sensor_again = (lut - 1)->value;
            return (lut - 1)->gain;
        } else {
            if((lut->gain == ar0230_sensor_attr.sensor_info.max_again) && (isp_gain >= lut->gain)) {
#ifdef CONFIG_AR0230_HDR
                again_mode = lut->mode;
#endif
                *sensor_again = lut->value;
                return lut->gain;
            }
        }

        lut++;
    }

    return isp_gain;
}


static int ar0230_set_analog_gain(int value)
{
    int ret = 0;

#ifdef CONFIG_AR0230_HDR
    static int again_mode_last = LCG;

    if (again_mode != again_mode_last) {
        if (again_mode == LCG){
            ret += ar0230_write(i2c_dev,0x3100, 0x00);
            ret += ar0230_write(i2c_dev,0x3096, 0x780);
            ret += ar0230_write(i2c_dev,0x3098, 0x780);
            ret += ar0230_write(i2c_dev,0x3206, 0x0B08);
            ret += ar0230_write(i2c_dev,0x3208, 0x1E13);
            ret += ar0230_write(i2c_dev,0x3202, 0x0080);

            again_mode_last = again_mode;
        } else if (again_mode == HCG){
            ret += ar0230_write(i2c_dev,0x3100, 0x04);
            ret += ar0230_write(i2c_dev,0x3096, 0x480);
            ret += ar0230_write(i2c_dev,0x3098, 0x480);
            ret += ar0230_write(i2c_dev,0x3206, 0x1C0E);
            ret += ar0230_write(i2c_dev,0x3208, 0x4E39);
            ret += ar0230_write(i2c_dev,0x3202, 0x00B0);

            again_mode_last = again_mode;
        } else {
            printk(KERN_ERR "Do not support this Again: %d(%x)\n", again_mode, value);
        }
    }
#endif
    ret = ar0230_write(i2c_dev, 0x3060, value);
    if (ret < 0)
        return ret;

    return 0;
}

unsigned int ar0230_alloc_dgain(unsigned int isp_gain, unsigned char shift, unsigned int *sensor_dgain)
{
    return isp_gain;
}

static int ar0230_set_digital_gain(int value)
{
    return 0;
}

static int ar0230_set_fps(int fps)
{
    struct sensor_info *sensor_info = &ar0230_sensor_attr.sensor_info;
    unsigned int pclk = AR0230_SUPPORT_SCLK;
    unsigned short hts, vts;
    unsigned int newformat; //the format is 24.8
    int ret = 0;
    /* the format of fps is 16/16. for example 25 << 16 | 2, the value is 25/2 fps. */
    newformat = (((fps >> 16) / (fps & 0xffff)) << 8) + ((((fps >> 16) % (fps & 0xffff)) << 8) / (fps & 0xffff));
    if(newformat > (SENSOR_OUTPUT_MAX_FPS << 8) || newformat < (SENSOR_OUTPUT_MIN_FPS << 8))
        return -1;

    ret += ar0230_read(i2c_dev, 0x300C, &hts);
    if(ret < 0)
        return -1;

    vts = pclk * (fps & 0xffff) / hts / ((fps & 0xffff0000) >> 16);
    ret = ar0230_write(i2c_dev, 0x300A, vts);
    if(ret < 0)
        return -1;

    sensor_info->fps = fps;
    sensor_info->total_height = vts;
#ifdef CONFIG_AR0230_HDR
    vts = vts * EXPOSURE_RATIO;
#endif
    sensor_info->max_integration_time = vts - 4;

    return ret;
}


static struct sensor_attr ar0230_sensor_attr = {
    .device_name        = AR0230_DEVICE_NAME,
    .cbus_addr          = AR0230_DEVICE_I2C_ADDR,

    .dma_mode           = SENSOR_DATA_DMA_MODE_RAW,
    .dbus_type          = SENSOR_DATA_BUS_DVP,
    .dvp = {
        .data_fmt       = DVP_RAW10,
        .gpio_mode      = DVP_PA_LOW_10BIT,
        .timing_mode    = DVP_HREF_MODE,
        .yuv_data_order = 0,
        .hsync_polarity = POLARITY_HIGH_ACTIVE,
        .vsync_polarity = POLARITY_HIGH_ACTIVE,
        .img_scan_mode  = DVP_IMG_SCAN_PROGRESS,
    },

    .isp_clk_rate       = 150 * 1000 * 1000,

    .sensor_info = {
#ifdef CONFIG_AR0230_HDR
        .private_init_setting   = ar0230_init_regs_1920_1080_30fps_HDR_dvp,

        .width                  = AR0230_WIDTH,
        .height                 = AR0230_HEIGHT,
        .fmt                    = SENSOR_PIXEL_FMT_SGRBG12_1X12,

        .fps                    = 30 << 16 | 1,
        .total_width            = 0x045E,
        .total_height           = 0x0452,

        .min_integration_time   = 2*CONFIG_AR0230_RATIO_T1_T2,
        .max_integration_time   = 0x0452 * EXPOSURE_RATIO - 4,
        //.one_line_expr_in_us = ,
        .max_again              = 260410,
        .max_dgain              = 0,
#else
        .private_init_setting   = ar0230_init_regs_1920_1080_30fps_linear_dvp,

        .width                  = AR0230_WIDTH,
        .height                 = AR0230_HEIGHT,
        .fmt                    = SENSOR_PIXEL_FMT_SGRBG12_1X12,

        .fps                    = 25 << 16 | 1,
        .total_width            = 0x044c,
        .total_height           = 0x0546,

        .min_integration_time   = 4,
        .max_integration_time   = 0x0546-4,
        //.one_line_expr_in_us = ,
        .max_again              = 262144,
        .max_dgain              = 0,
#endif
    },

    .ops = {
        .power_on               = ar0230_power_on,
        .power_off              = ar0230_power_off,
        .stream_on              = ar0230_stream_on,
        .stream_off             = ar0230_stream_off,
        .get_register           = ar0230_g_register,
        .set_register           = ar0230_s_register,

        .set_integration_time   = ar0230_set_integration_time,
        .alloc_again            = ar0230_alloc_again,
        .set_analog_gain        = ar0230_set_analog_gain,
        .alloc_dgain            = ar0230_alloc_dgain,
        .set_digital_gain       = ar0230_set_digital_gain,
        .set_fps                = ar0230_set_fps,
    },
};

static int ar0230_probe(struct i2c_client *client,
        const struct i2c_device_id *id)
{
    int i;

    for (i = 0; i < ARRAY_SIZE(dvp_gpio_func_array); i++) {
        if (!strcmp(dvp_gpio_func_str, dvp_gpio_func_array[i])) {
            dvp_gpio_func = i;
            break;
        }
    }

    if (i == ARRAY_SIZE(dvp_gpio_func_array)){
        printk(KERN_ERR "sensor dvp_gpio_func set error!\n");
        return -EINVAL;
    }

    int ret = init_gpio();
    if (ret)
        goto err_init_gpio;

    ret = dvp_init_select_gpio(&ar0230_sensor_attr.dvp, dvp_gpio_func);
    if (ret)
        goto err_dvp_select_gpio;

    ret = camera_register_sensor(vic_index, &ar0230_sensor_attr);
    if (ret) 
        goto err_camera_register;

    return 0;

err_camera_register:
    dvp_deinit_gpio();
err_dvp_select_gpio:
    deinit_gpio();

err_init_gpio:

    return ret;
}

static int ar0230_remove(struct i2c_client *client)
{
    camera_unregister_sensor(vic_index, &ar0230_sensor_attr);
    dvp_deinit_gpio();
    deinit_gpio();
    return 0;
}

static const struct i2c_device_id ar0230_id[] = {
    { AR0230_DEVICE_NAME, 0 },
    { }
};
MODULE_DEVICE_TABLE(i2c, ar0230_id);

static struct i2c_driver ar0230_driver = {
    .driver = {
        .owner          = THIS_MODULE,
        .name           = AR0230_DEVICE_NAME,
    },
    .probe              = ar0230_probe,
    .remove             = ar0230_remove,
    .id_table           = ar0230_id,
};

static struct i2c_board_info sensor_ar0230_info = {
    .type               = AR0230_DEVICE_NAME,
    .addr               = AR0230_DEVICE_I2C_ADDR,
};

static __init int init_ar0230(void)
{
    if (i2c_bus_num < 0) {
        printk(KERN_ERR "ar0230: i2c_bus_num must be set\n");
        return -EINVAL;
    }

    int ret = i2c_add_driver(&ar0230_driver);
    if (ret) {
        printk(KERN_ERR "ar0230: failed to register i2c driver\n");
        return ret;
    }

    i2c_dev = i2c_register_device(&sensor_ar0230_info, i2c_bus_num);
    if (i2c_dev == NULL) {
        printk(KERN_ERR "ar0230: failed to register i2c device\n");
        i2c_del_driver(&ar0230_driver);
        return -EINVAL;
    }

    return 0;
}

static __exit void exit_ar0230(void)
{
    i2c_unregister_device(i2c_dev);
    i2c_del_driver(&ar0230_driver);
}

module_init(init_ar0230);
module_exit(exit_ar0230);

MODULE_DESCRIPTION("x2000 ar0230 driver");
MODULE_LICENSE("GPL");
