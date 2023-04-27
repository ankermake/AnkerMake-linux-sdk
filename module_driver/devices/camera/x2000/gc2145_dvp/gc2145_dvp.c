/*
 * Copyright (C) 2020 Ingenic Semiconductor Co., Ltd.
 *
 * GC2145 DVP
 *
 */

#include <linux/module.h>
#include <utils/gpio.h>
#include <utils/i2c.h>
#include <common.h>
#include <linux/regulator/consumer.h>
#include <camera/hal/camera_sensor.h>
#include <camera/hal/dvp_gpio_func.h>

#define GC2145_DEVICE_NAME              "gc2145"
#define GC2145_DEVICE_I2C_ADDR          0x3c

#define GC2145_CHIP_ID_H                (0x21)
#define GC2145_CHIP_ID_L                (0x45)

#define GC2145_REG_END                  0xff
#define GC2145_PAGE_REG                 0xfe
#define GC2145_REG_DELAY                0x0

#define GC2145_SUPPORT_MCLK             (12 * 1000 * 1000)
#define GC2033_SUPPORT_WPCLK_FPS_30     (120 *1000 *1000)
#define SENSOR_OUTPUT_MAX_FPS           30
#define SENSOR_OUTPUT_MIN_FPS           5

#define CONFIG_GC2145_1600X1200
#define GC2145_MAX_WIDTH                1600
#define GC2145_MAX_HEIGHT               1200
#if defined CONFIG_GC2145_720X1200
#define GC2145_WIDTH                    720
#define GC2145_HEIGHT                   1200
#elif defined CONFIG_GC2145_1600X1200
#define GC2145_WIDTH                    1600
#define GC2145_HEIGHT                   1200
#else
#define GC2145_WIDTH                    800
#define GC2145_HEIGHT                   480
#endif

#define GC2145_USE_AGAIN_ONLY

static int power_gpio                    = -1;//NULL
static int reset_gpio                    = -1;//GPIO_PA(11)
static int pwdn_gpio                     = -1;//GPIO_PA(08)
static int i2c_bus_num                   = -1;//3
static short i2c_addr                    = -1;//0x3c
static int cam_bus_num                   = -1;//0
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
static struct sensor_attr gc2145_sensor_attr;
static struct regulator *gc2145_regulator = NULL;

struct regval_list {
    unsigned short reg_num;
    unsigned char value;
};

struct again_lut {
    int index;
    unsigned int regb4;
    unsigned int regb3;
    unsigned int regb2;
    unsigned int dpc;
    unsigned int blc;
    unsigned int gain;
};

static struct regval_list gc2145_init_regs[] = {
    { 0xfe , 0xf0},
    { 0xfe , 0xf0},
    { 0xfe , 0xf0},
    { 0xfc , 0x06},
    { 0xf6 , 0x00},
    { 0xf7 , 0x1d},
    { 0xf8 , 0x84},
    { 0xfa , 0x00},
    { 0xf9 , 0xfe},
    { 0xf2 , 0x00},
        /////////////////////////////////////////////////
        //////////////////ISP reg//////////////////////
        ////////////////////////////////////////////////////
    { 0xfe , 0x00},
    { 0x03 , 0x04},
    { 0x04 , 0xe2},
    { 0x09 , 0x00},
    { 0x0a , 0x00},
    { 0x0b , 0x00},
    { 0x0c , 0x00},
    { 0x0d , 0x04},
    { 0x0e , 0xc0},
    { 0x0f , 0x06},
    { 0x10 , 0x52},
    { 0x12 , 0x2e},
    { 0x17 , 0x14}, //mirror
    { 0x18 , 0x22},
    { 0x19 , 0x0e},
    { 0x1a , 0x01},
    { 0x1b , 0x4b},
    { 0x1c , 0x07},
    { 0x1d , 0x10},
    { 0x1e , 0x88},
    { 0x1f , 0x78},
    { 0x20 , 0x03},
    { 0x21 , 0x40},
    { 0x22 , 0xa0},
    { 0x24 , 0x16},
    { 0x25 , 0x01},
    { 0x26 , 0x10},
    { 0x2d , 0x60},
    { 0x30 , 0x01},
    { 0x31 , 0x90},
    { 0x33 , 0x06},
    { 0x34 , 0x01},
        /////////////////////////////////////////////////
        //////////////////ISP reg////////////////////
        /////////////////////////////////////////////////
    { 0xfe , 0x00},
    { 0x80 , 0x7f},
    { 0x81 , 0x26},
    { 0x82 , 0xfa},
    { 0x83 , 0x00},
    { 0x84 , 0x02},
    { 0x86 , 0x02},
    { 0x88 , 0x03},
    { 0x89 , 0x03},
    { 0x85 , 0x08},
    { 0x8a , 0x00},
    { 0x8b , 0x00},
    { 0xb0 , 0x55},
    { 0xc3 , 0x00},
    { 0xc4 , 0x80},
    { 0xc5 , 0x90},
    { 0xc6 , 0x3b},
    { 0xc7 , 0x46},
    { 0xec , 0x06},
    { 0xed , 0x04},
    { 0xee , 0x60},
    { 0xef , 0x90},
    { 0xb6 , 0x01},
    { 0x90 , 0x01},
    {0x91, (GC2145_MAX_HEIGHT-GC2145_HEIGHT)/2/256}, //out_win_y1_h
    {0x92, 8+(GC2145_MAX_HEIGHT-GC2145_HEIGHT)/2%256}, //out_win_y1_l
    {0x93, (GC2145_MAX_WIDTH-GC2145_WIDTH)/2/256}, //out_win_x1_h
    {0x94, 8+(GC2145_MAX_WIDTH-GC2145_WIDTH)/2%256}, //out_win_x1_l
    {0x95, GC2145_HEIGHT/256}, //out_win_height_h
    {0x96, GC2145_HEIGHT%256}, //out_win_height_l
    {0x97, GC2145_WIDTH/256}, //out_win_width_h
    {0x98, GC2145_WIDTH%256}, //out_win_width_l
        /////////////////////////////////////////
        /////////// BLK ////////////////////////
        /////////////////////////////////////////
    { 0xfe , 0x00},
    { 0x40 , 0x42},
    { 0x41 , 0x00},
    { 0x43 , 0x5b},
    { 0x5e , 0x00},
    { 0x5f , 0x00},
    { 0x60 , 0x00},
    { 0x61 , 0x00},
    { 0x62 , 0x00},
    { 0x63 , 0x00},
    { 0x64 , 0x00},
    { 0x65 , 0x00},
    { 0x66 , 0x20},
    { 0x67 , 0x20},
    { 0x68 , 0x20},
    { 0x69 , 0x20},
    { 0x76 , 0x00},
    { 0x6a , 0x08},
    { 0x6b , 0x08},
    { 0x6c , 0x08},
    { 0x6d , 0x08},
    { 0x6e , 0x08},
    { 0x6f , 0x08},
    { 0x70 , 0x08},
    { 0x71 , 0x08},
    { 0x76 , 0x00},
    { 0x72 , 0xf0},
    { 0x7e , 0x3c},
    { 0x7f , 0x00},
    { 0xfe , 0x02},
    { 0x48 , 0x15},
    { 0x49 , 0x00},
    { 0x4b , 0x0b},
    { 0xfe , 0x00},
        ////////////////////////////////////////
        /////////// AEC ////////////////////////
        ////////////////////////////////////////
    { 0xfe , 0x01},
    { 0x01 , 0x04},
    { 0x02 , 0xc0},
    { 0x03 , 0x04},
    { 0x04 , 0x90},
    { 0x05 , 0x30},
    { 0x06 , 0x90},
    { 0x07 , 0x30},
    { 0x08 , 0x80},
    { 0x09 , 0x00},
    { 0x0a , 0x82},
    { 0x0b , 0x11},
    { 0x0c , 0x10},
    { 0x11 , 0x10},
    { 0x13 , 0x7b},
    { 0x17 , 0x00},
    { 0x1c , 0x11},
    { 0x1e , 0x61},
    { 0x1f , 0x35},
    { 0x20 , 0x40},
    { 0x22 , 0x40},
    { 0x23 , 0x20},
    { 0xfe , 0x02},
    { 0x0f , 0x04},
    { 0xfe , 0x01},
    { 0x12 , 0x35},
    { 0x15 , 0xb0},
    { 0x10 , 0x31},
    { 0x3e , 0x28},
    { 0x3f , 0xb0},
    { 0x40 , 0x90},
    { 0x41 , 0x0f},

        /////////////////////////////
        //////// INTPEE /////////////
        /////////////////////////////
    { 0xfe , 0x02},
    { 0x90 , 0x6c},
    { 0x91 , 0x03},
    { 0x92 , 0xcb},
    { 0x94 , 0x33},
    { 0x95 , 0x84},
    { 0x97 , 0x65},
    { 0xa2 , 0x11},
    { 0xfe , 0x00},
        /////////////////////////////
        //////// DNDD///////////////
        /////////////////////////////
    { 0xfe , 0x02},
    { 0x80 , 0xc1},
    { 0x81 , 0x08},
    { 0x82 , 0x05},
    { 0x83 , 0x08},
    { 0x84 , 0x0a},
    { 0x86 , 0xf0},
    { 0x87 , 0x50},
    { 0x88 , 0x15},
    { 0x89 , 0xb0},
    { 0x8a , 0x30},
    { 0x8b , 0x10},
        /////////////////////////////////////////
        /////////// ASDE ////////////////////////
        /////////////////////////////////////////
    { 0xfe , 0x01},
    { 0x21 , 0x04},
    { 0xfe , 0x02},
    { 0xa3 , 0x50},
    { 0xa4 , 0x20},
    { 0xa5 , 0x40},
    { 0xa6 , 0x80},
    { 0xab , 0x40},
    { 0xae , 0x0c},
    { 0xb3 , 0x46},
    { 0xb4 , 0x64},
    { 0xb6 , 0x38},
    { 0xb7 , 0x01},
    { 0xb9 , 0x2b},
    { 0x3c , 0x04},
    { 0x3d , 0x15},
    { 0x4b , 0x06},
    { 0x4c , 0x20},
    { 0xfe , 0x00},
        /////////////////////////////////////////
        /////////// GAMMA   ////////////////////////
        /////////////////////////////////////////
        ///////////////////gamma1////////////////////
#if 1
    { 0xfe , 0x02},
    { 0x10 , 0x09},
    { 0x11 , 0x0d},
    { 0x12 , 0x13},
    { 0x13 , 0x19},
    { 0x14 , 0x27},
    { 0x15 , 0x37},
    { 0x16 , 0x45},
    { 0x17 , 0x53},
    { 0x18 , 0x69},
    { 0x19 , 0x7d},
    { 0x1a , 0x8f},
    { 0x1b , 0x9d},
    { 0x1c , 0xa9},
    { 0x1d , 0xbd},
    { 0x1e , 0xcd},
    { 0x1f , 0xd9},
    { 0x20 , 0xe3},
    { 0x21 , 0xea},
    { 0x22 , 0xef},
    { 0x23 , 0xf5},
    { 0x24 , 0xf9},
    { 0x25 , 0xff},
#else
    { 0xfe , 0x02},
    { 0x10 , 0x0a},
    { 0x11 , 0x12},
    { 0x12 , 0x19},
    { 0x13 , 0x1f},
    { 0x14 , 0x2c},
    { 0x15 , 0x38},
    { 0x16 , 0x42},
    { 0x17 , 0x4e},
    { 0x18 , 0x63},
    { 0x19 , 0x76},
    { 0x1a , 0x87},
    { 0x1b , 0x96},
    { 0x1c , 0xa2},
    { 0x1d , 0xb8},
    { 0x1e , 0xcb},
    { 0x1f , 0xd8},
    { 0x20 , 0xe2},
    { 0x21 , 0xe9},
    { 0x22 , 0xf0},
    { 0x23 , 0xf8},
    { 0x24 , 0xfd},
    { 0x25 , 0xff},
    { 0xfe , 0x00},
#endif
    { 0xfe , 0x00},
    { 0xc6 , 0x20},
    { 0xc7 , 0x2b},
        ///////////////////gamma2////////////////////
#if 1
    { 0xfe , 0x02},
    { 0x26 , 0x0f},
    { 0x27 , 0x14},
    { 0x28 , 0x19},
    { 0x29 , 0x1e},
    { 0x2a , 0x27},
    { 0x2b , 0x33},
    { 0x2c , 0x3b},
    { 0x2d , 0x45},
    { 0x2e , 0x59},
    { 0x2f , 0x69},
    { 0x30 , 0x7c},
    { 0x31 , 0x89},
    { 0x32 , 0x98},
    { 0x33 , 0xae},
    { 0x34 , 0xc0},
    { 0x35 , 0xcf},
    { 0x36 , 0xda},
    { 0x37 , 0xe2},
    { 0x38 , 0xe9},
    { 0x39 , 0xf3},
    { 0x3a , 0xf9},
    { 0x3b , 0xff},
#else
        ////Gamma outdoor
    { 0xfe , 0x02},
    { 0x26 , 0x17},
    { 0x27 , 0x18},
    { 0x28 , 0x1c},
    { 0x29 , 0x20},
    { 0x2a , 0x28},
    { 0x2b , 0x34},
    { 0x2c , 0x40},
    { 0x2d , 0x49},
    { 0x2e , 0x5b},
    { 0x2f , 0x6d},
    { 0x30 , 0x7d},
    { 0x31 , 0x89},
    { 0x32 , 0x97},
    { 0x33 , 0xac},
    { 0x34 , 0xc0},
    { 0x35 , 0xcf},
    { 0x36 , 0xda},
    { 0x37 , 0xe5},
    { 0x38 , 0xec},
    { 0x39 , 0xf8},
    { 0x3a , 0xfd},
    { 0x3b , 0xff},
#endif
        ///////////////////////////////////////////////
        ///////////YCP ///////////////////////
        ///////////////////////////////////////////////
    { 0xfe , 0x02},
    { 0xd1 , 0x32},
    { 0xd2 , 0x32},
    { 0xd3 , 0x40},
    { 0xd6 , 0xf0},
    { 0xd7 , 0x10},
    { 0xd8 , 0xda},
    { 0xdd , 0x14},
    { 0xde , 0x86},
    { 0xed , 0x80},
    { 0xee , 0x00},
    { 0xef , 0x3f},
    { 0xd8 , 0xd8},
        ///////////////////abs/////////////////
    { 0xfe , 0x01},
    { 0x9f , 0x40},
        /////////////////////////////////////////////
        //////////////////////// LSC ///////////////
        //////////////////////////////////////////
    { 0xfe , 0x01},
    { 0xc2 , 0x14},
    { 0xc3 , 0x0d},
    { 0xc4 , 0x0c},
    { 0xc8 , 0x15},
    { 0xc9 , 0x0d},
    { 0xca , 0x0a},
    { 0xbc , 0x24},
    { 0xbd , 0x10},
    { 0xbe , 0x0b},
    { 0xb6 , 0x25},
    { 0xb7 , 0x16},
    { 0xb8 , 0x15},
    { 0xc5 , 0x00},
    { 0xc6 , 0x00},
    { 0xc7 , 0x00},
    { 0xcb , 0x00},
    { 0xcc , 0x00},
    { 0xcd , 0x00},
    { 0xbf , 0x07},
    { 0xc0 , 0x00},
    { 0xc1 , 0x00},
    { 0xb9 , 0x00},
    { 0xba , 0x00},
    { 0xbb , 0x00},
    { 0xaa , 0x01},
    { 0xab , 0x01},
    { 0xac , 0x00},
    { 0xad , 0x05},
    { 0xae , 0x06},
    { 0xaf , 0x0e},
    { 0xb0 , 0x0b},
    { 0xb1 , 0x07},
    { 0xb2 , 0x06},
    { 0xb3 , 0x17},
    { 0xb4 , 0x0e},
    { 0xb5 , 0x0e},
    { 0xd0 , 0x09},
    { 0xd1 , 0x00},
    { 0xd2 , 0x00},
    { 0xd6 , 0x08},
    { 0xd7 , 0x00},
    { 0xd8 , 0x00},
    { 0xd9 , 0x00},
    { 0xda , 0x00},
    { 0xdb , 0x00},
    { 0xd3 , 0x0a},
    { 0xd4 , 0x00},
    { 0xd5 , 0x00},
    { 0xa4 , 0x00},
    { 0xa5 , 0x00},
    { 0xa6 , 0x77},
    { 0xa7 , 0x77},
    { 0xa8 , 0x77},
    { 0xa9 , 0x77},
    { 0xa1 , 0x80},
    { 0xa2 , 0x80},
    { 0xfe , 0x01},
    { 0xdf , 0x0d},
    { 0xdc , 0x25},
    { 0xdd , 0x30},
    { 0xe0 , 0x77},
    { 0xe1 , 0x80},
    { 0xe2 , 0x77},
    { 0xe3 , 0x90},
    { 0xe6 , 0x90},
    { 0xe7 , 0xa0},
    { 0xe8 , 0x90},
    { 0xe9 , 0xa0},
    { 0xfe , 0x00},
        ///////////////////////////////////////////////
        /////////// AWB////////////////////////
        ///////////////////////////////////////////////
    { 0xfe , 0x01},
    { 0x4f , 0x00},
    { 0x4f , 0x00},
    { 0x4b , 0x01},
    { 0x4f , 0x00},
    { 0x4c , 0x01}, // D75
    { 0x4d , 0x71},
    { 0x4e , 0x01},
    { 0x4c , 0x01},
    { 0x4d , 0x91},
    { 0x4e , 0x01},
    { 0x4c , 0x01},
    { 0x4d , 0x70},
    { 0x4e , 0x01},
    { 0x4c , 0x01}, // D65
    { 0x4d , 0x90},
    { 0x4e , 0x02},
    { 0x4c , 0x01},
    { 0x4d , 0xb0},
    { 0x4e , 0x02},
    { 0x4c , 0x01},
    { 0x4d , 0x8f},
    { 0x4e , 0x02},
    { 0x4c , 0x01},
    { 0x4d , 0x6f},
    { 0x4e , 0x02},
    { 0x4c , 0x01},
    { 0x4d , 0xaf},
    { 0x4e , 0x02},
    { 0x4c , 0x01},
    { 0x4d , 0xd0},
    { 0x4e , 0x02},
    { 0x4c , 0x01},
    { 0x4d , 0xf0},
    { 0x4e , 0x02},
    { 0x4c , 0x01},
    { 0x4d , 0xcf},
    { 0x4e , 0x02},
    { 0x4c , 0x01},
    { 0x4d , 0xef},
    { 0x4e , 0x02},
    { 0x4c , 0x01},//D50
    { 0x4d , 0x6e},
    { 0x4e , 0x03},
    { 0x4c , 0x01},
    { 0x4d , 0x8e},
    { 0x4e , 0x03},
    { 0x4c , 0x01},
    { 0x4d , 0xae},
    { 0x4e , 0x03},
    { 0x4c , 0x01},
    { 0x4d , 0xce},
    { 0x4e , 0x03},
    { 0x4c , 0x01},
    { 0x4d , 0x4d},
    { 0x4e , 0x03},
    { 0x4c , 0x01},
    { 0x4d , 0x6d},
    { 0x4e , 0x03},
    { 0x4c , 0x01},
    { 0x4d , 0x8d},
    { 0x4e , 0x03},
    { 0x4c , 0x01},
    { 0x4d , 0xad},
    { 0x4e , 0x03},
    { 0x4c , 0x01},
    { 0x4d , 0xcd},
    { 0x4e , 0x03},
    { 0x4c , 0x01},
    { 0x4d , 0x4c},
    { 0x4e , 0x03},
    { 0x4c , 0x01},
    { 0x4d , 0x6c},
    { 0x4e , 0x03},
    { 0x4c , 0x01},
    { 0x4d , 0x8c},
    { 0x4e , 0x03},
    { 0x4c , 0x01},
    { 0x4d , 0xac},
    { 0x4e , 0x03},
    { 0x4c , 0x01},
    { 0x4d , 0xcc},
    { 0x4e , 0x03},
    { 0x4c , 0x01},
    { 0x4d , 0xcb},
    { 0x4e , 0x03},
    { 0x4c , 0x01},
    { 0x4d , 0x4b},
    { 0x4e , 0x03},
    { 0x4c , 0x01},
    { 0x4d , 0x6b},
    { 0x4e , 0x03},
    { 0x4c , 0x01},
    { 0x4d , 0x8b},
    { 0x4e , 0x03},
    { 0x4c , 0x01},
    { 0x4d , 0xab},
    { 0x4e , 0x03},
    { 0x4c , 0x01},//CWF
    { 0x4d , 0x8a},
    { 0x4e , 0x04},
    { 0x4c , 0x01},
    { 0x4d , 0xaa},
    { 0x4e , 0x04},
    { 0x4c , 0x01},
    { 0x4d , 0xca},
    { 0x4e , 0x04},
    { 0x4c , 0x01},
    { 0x4d , 0xca},
    { 0x4e , 0x04},
    { 0x4c , 0x01},
    { 0x4d , 0xc9},
    { 0x4e , 0x04},
    { 0x4c , 0x01},
    { 0x4d , 0x8a},
    { 0x4e , 0x04},
    { 0x4c , 0x01},
    { 0x4d , 0x89},
    { 0x4e , 0x04},
    { 0x4c , 0x01},
    { 0x4d , 0xa9},
    { 0x4e , 0x04},
    { 0x4c , 0x02},//tl84
    { 0x4d , 0x0b},
    { 0x4e , 0x05},
    { 0x4c , 0x02},
    { 0x4d , 0x0a},
    { 0x4e , 0x05},
    { 0x4c , 0x01},
    { 0x4d , 0xeb},
    { 0x4e , 0x05},
    { 0x4c , 0x01},
    { 0x4d , 0xea},
    { 0x4e , 0x05},
    { 0x4c , 0x02},
    { 0x4d , 0x09},
    { 0x4e , 0x05},
    { 0x4c , 0x02},
    { 0x4d , 0x29},
    { 0x4e , 0x05},
    { 0x4c , 0x02},
    { 0x4d , 0x2a},
    { 0x4e , 0x05},
    { 0x4c , 0x02},
    { 0x4d , 0x4a},
    { 0x4e , 0x05},
    { 0x4c , 0x02},
    { 0x4d , 0x8a},
    { 0x4e , 0x06},
    { 0x4c , 0x02},
    { 0x4d , 0x49},
    { 0x4e , 0x06},
    { 0x4c , 0x02},
    { 0x4d , 0x69},
    { 0x4e , 0x06},
    { 0x4c , 0x02},
    { 0x4d , 0x89},
    { 0x4e , 0x06},
    { 0x4c , 0x02},
    { 0x4d , 0xa9},
    { 0x4e , 0x06},
    { 0x4c , 0x02},
    { 0x4d , 0x48},
    { 0x4e , 0x06},
    { 0x4c , 0x02},
    { 0x4d , 0x68},
    { 0x4e , 0x06},
    { 0x4c , 0x02},
    { 0x4d , 0x69},
    { 0x4e , 0x06},
    { 0x4c , 0x02},//H
    { 0x4d , 0xca},
    { 0x4e , 0x07},
    { 0x4c , 0x02},
    { 0x4d , 0xc9},
    { 0x4e , 0x07},
    { 0x4c , 0x02},
    { 0x4d , 0xe9},
    { 0x4e , 0x07},
    { 0x4c , 0x03},
    { 0x4d , 0x09},
    { 0x4e , 0x07},
    { 0x4c , 0x02},
    { 0x4d , 0xc8},
    { 0x4e , 0x07},
    { 0x4c , 0x02},
    { 0x4d , 0xe8},
    { 0x4e , 0x07},
    { 0x4c , 0x02},
    { 0x4d , 0xa7},
    { 0x4e , 0x07},
    { 0x4c , 0x02},
    { 0x4d , 0xc7},
    { 0x4e , 0x07},
    { 0x4c , 0x02},
    { 0x4d , 0xe7},
    { 0x4e , 0x07},
    { 0x4c , 0x03},
    { 0x4d , 0x07},
    { 0x4e , 0x07},
    { 0x4f , 0x01},
    { 0x50 , 0x80},
    { 0x51 , 0xa8},
    { 0x52 , 0x47},
    { 0x53 , 0x38},
    { 0x54 , 0xc7},
    { 0x56 , 0x0e},
    { 0x58 , 0x08},
    { 0x5b , 0x00},
    { 0x5c , 0x74},
    { 0x5d , 0x8b},
    { 0x61 , 0xdb},
    { 0x62 , 0xb8},
    { 0x63 , 0x86},
    { 0x64 , 0xc0},
    { 0x65 , 0x04},
    { 0x67 , 0xa8},
    { 0x68 , 0xb0},
    { 0x69 , 0x00},
    { 0x6a , 0xa8},
    { 0x6b , 0xb0},
    { 0x6c , 0xaf},
    { 0x6d , 0x8b},
    { 0x6e , 0x50},
    { 0x6f , 0x18},
    { 0x73 , 0xf0},
    { 0x70 , 0x0d},
    { 0x71 , 0x60},
    { 0x72 , 0x80},
    { 0x74 , 0x01},
    { 0x75 , 0x01},
    { 0x7f , 0x0c},
    { 0x76 , 0x70},
    { 0x77 , 0x58},
    { 0x78 , 0xa0},
    { 0x79 , 0x5e},
    { 0x7a , 0x54},
    { 0x7b , 0x58},
    { 0xfe , 0x00},
        //////////////////////////////////////////
        ///////////CC////////////////////////
        //////////////////////////////////////////
    { 0xfe , 0x02},
    { 0xc0 , 0x01},
    { 0xc1 , 0x44},
    { 0xc2 , 0xfd},
    { 0xc3 , 0x04},
    { 0xc4 , 0xF0},
    { 0xc5 , 0x48},
    { 0xc6 , 0xfd},
    { 0xc7 , 0x46},
    { 0xc8 , 0xfd},
    { 0xc9 , 0x02},
    { 0xca , 0xe0},
    { 0xcb , 0x45},
    { 0xcc , 0xec},
    { 0xcd , 0x48},
    { 0xce , 0xf0},
    { 0xcf , 0xf0},
    { 0xe3 , 0x0c},
    { 0xe4 , 0x4b},
    { 0xe5 , 0xe0},
        //////////////////////////////////////////
        ///////////ABS ////////////////////
        //////////////////////////////////////////
    { 0xfe , 0x01},
    { 0x9f , 0x40},
    { 0xfe , 0x00},
        //////////////////////////////////////
        ///////////  OUTPUT   ////////////////
        //////////////////////////////////////
    { 0xfe, 0x00},
    { 0xf2, 0x0f}, 	///////////////dark sun////////////////////
    { 0xfe , 0x02},
    { 0x40 , 0xbf},
    { 0x46 , 0xcf},
    { 0xfe , 0x00}, 	//////////////frame rate 50Hz/////////
    { 0xfe , 0x00},
    { 0x05 , 0x01},
    { 0x06 , 0x56},
    { 0x07 , 0x00},
    { 0x08 , 0x32},
    { 0xfe , 0x01},
    { 0x25 , 0x00},
    { 0x26 , 0xfa},
    { 0x27 , 0x04},
    { 0x28 , 0xe2}, //20fps
    { 0x29 , 0x06},
    { 0x2a , 0xd6}, //14fps
    { 0x2b , 0x07},
    { 0x2c , 0xd0}, //12fps
    { 0x2d , 0x0b},
    { 0x2e , 0xb8}, //8fps
    { 0xfe , 0x00},
    {GC2145_REG_END, 0x00},    /* END MARKER */
};

static struct regval_list gc2145_regs_stream_on[] = {

    {0xfe, 0x00},
    {0xf2, 0x0f},
    {GC2145_REG_END, 0x00},     /* END MARKER */
};

static struct regval_list gc2145_regs_stream_off[] = {

    {0xfe, 0x00},
    {0xf2, 0x00},
    {GC2145_REG_END, 0x00},     /* END MARKER */
};

static int gc2145_write(struct i2c_client *i2c, unsigned char reg, unsigned char value)
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
        printk(KERN_ERR "gc2145: failed to write reg: %x\n", (int)reg);
    }

    return ret;
}

static int gc2145_read(struct i2c_client *i2c, unsigned char reg, unsigned char *value)
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
        printk(KERN_ERR "gc2145(%x): failed to read reg: %x\n", i2c->addr, (int)reg);
    }

    return ret;
}

static int gc2145_write_array(struct i2c_client *i2c, struct regval_list *vals)
{
    int ret;

    while (vals->reg_num != GC2145_REG_END) {
        if (vals->reg_num == GC2145_REG_DELAY) {
                msleep(vals->value);
        } else {
            ret = gc2145_write(i2c, vals->reg_num, vals->value);
            if (ret < 0)
                return ret;
        }
        vals++;
    }

    return 0;
}

static inline int gc2145_read_array(struct i2c_client *i2c, struct regval_list *vals)
{
    int ret;
    unsigned char val;

    while (vals->reg_num != GC2145_REG_END) {
        if (vals->reg_num == GC2145_REG_DELAY) {
                msleep(vals->value);
        } else {
            ret = gc2145_read(i2c, vals->reg_num, &val);
            if (ret < 0)
                return ret;
            if (vals->reg_num == GC2145_PAGE_REG){
                val &= 0xf8;
                val |= (vals->value & 0x07);
                ret = gc2145_write(i2c, vals->reg_num, val);
                ret = gc2145_read(i2c, vals->reg_num, &val);
            }
        }
        vals++;
    }

    return 0;
}

static int gc2145_detect(struct i2c_client *i2c)
{
    unsigned char h = 1;
    unsigned char l = 1;
    int ret;

    ret = gc2145_read(i2c, 0xf0, &h);
    if (ret < 0)
        return ret;

    if (h != GC2145_CHIP_ID_H) {
        printk(KERN_ERR "gc2145 read chip id high failed:0x%x\n", h);
        return -ENODEV;
    }

    ret = gc2145_read(i2c, 0xf1, &l);
    if (ret < 0)
        return ret;

    if (l != GC2145_CHIP_ID_L) {
        printk(KERN_ERR "gc2145 read chip id low failed:0x%x\n", l);
        return -ENODEV;
    }
    printk(KERN_DEBUG "gc2145 get chip id = %02x%02x\n", h, l);

    return 0;
}

static int init_gpio(void)
{
    int ret;
    char gpio_str[10];

    if (reset_gpio != -1) {
        ret = gpio_request(reset_gpio, "gc2145_reset");
        if (ret) {
            printk(KERN_ERR "gc2145: failed to request rst pin: %s\n", gpio_to_str(reset_gpio, gpio_str));
            goto err_reset_gpio;
        }
    }

    if (pwdn_gpio != -1) {
        ret = gpio_request(pwdn_gpio, "gc2145_pwdn");
        if (ret) {
            printk(KERN_ERR "gc2145: failed to request pwdn pin: %s\n", gpio_to_str(pwdn_gpio, gpio_str));
            goto err_pwdn_gpio;
        }
    }

    if (power_gpio != -1) {
        ret = gpio_request(power_gpio, "gc2145_power");
        if (ret) {
            printk(KERN_ERR "gc2145: failed to request power pin: %s\n", gpio_to_str(power_gpio, gpio_str));
            goto err_power_gpio;
        }
    }

    if (strcmp(regulator_name, "-1") && strlen(regulator_name)) {
        gc2145_regulator = regulator_get(NULL, regulator_name);
        if (IS_ERR(gc2145_regulator)) {
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

    if (gc2145_regulator)
        regulator_put(gc2145_regulator);
}

static void gc2145_power_off(void)
{
    if (reset_gpio != -1)
        gpio_direction_output(reset_gpio, 0);

    if (pwdn_gpio != -1)
        gpio_direction_output(pwdn_gpio, 1);

    if (power_gpio != -1)
        gpio_direction_output(power_gpio, 0);

    if (gc2145_regulator)
        regulator_disable(gc2145_regulator);

    camera_disable_sensor_mclk(cam_bus_num);
}

static int gc2145_power_on(void)
{
    int ret;

    camera_enable_sensor_mclk(cam_bus_num, 12 * 1000 * 1000);

    if (gc2145_regulator)
        regulator_enable(gc2145_regulator);

    if (power_gpio != -1) {
        gpio_direction_output(power_gpio, 1);
        m_msleep(50);
    }

    if (pwdn_gpio != -1){
        gpio_direction_output(pwdn_gpio, 1);
        msleep(10);
        gpio_direction_output(pwdn_gpio, 0);
        msleep(10);
    }

    if (reset_gpio != -1){
        gpio_direction_output(reset_gpio, 1);
        m_msleep(20);
        gpio_direction_output(reset_gpio, 0);
        m_msleep(20);
        gpio_direction_output(reset_gpio, 1);
        m_msleep(10);
    }

    ret = gc2145_detect(i2c_dev);
    if (ret) {
        printk(KERN_ERR "gc2145: failed to detect\n");
        gc2145_power_off();
        return ret;
    }

    ret = gc2145_write_array(i2c_dev, gc2145_sensor_attr.sensor_info.private_init_setting);
    // ret += gc2145_read_array(i2c_dev, gc2145_sensor_attr.sensor_info.private_init_setting);

    if (ret) {
        printk(KERN_ERR "gc2145: failed to init regs\n");
        gc2145_power_off();
        return ret;
    }

    return 0;
}

static int gc2145_stream_on(void)
{
    int ret = gc2145_write_array(i2c_dev, gc2145_regs_stream_on);
    if (ret)
        printk(KERN_ERR "gc2145: failed to stream on\n");

    return ret;
}

static void gc2145_stream_off(void)
{
    int ret = gc2145_write_array(i2c_dev, gc2145_regs_stream_off);
    if (ret)
        printk(KERN_ERR "gc2145: failed to stream on\n");
}

static int gc2145_g_register(struct sensor_dbg_register *reg)
{
    unsigned char val;
    int ret;

    ret = gc2145_read(i2c_dev, reg->reg & 0xffff, &val);
    reg->val = val;
    reg->size = 2;
    return ret;
}

static int gc2145_s_register(struct sensor_dbg_register *reg)
{
    return gc2145_write(i2c_dev, reg->reg & 0xffff, reg->val & 0xffff);
}

static int gc2145_set_integration_time(int value)
{
    int ret = 0;

    ret = gc2145_write(i2c_dev, 0x04, value&0xff);
    ret += gc2145_write(i2c_dev, 0x03, (value&0x3f00)>>8);
    if (ret < 0) {
        printk("gc2145_write error  %d\n" ,__LINE__ );
        return ret;
    }

    return 0;
}

static unsigned int gc2145_alloc_dgain(unsigned int isp_gain, unsigned char shift, unsigned int *sensor_dgain)
{
    return isp_gain;
}

static int gc2145_set_digital_gain(int value)
{
    return 0;
}

static int gc2145_set_fps(int fps)
{
    struct sensor_info *sensor_info = &gc2145_sensor_attr.sensor_info;
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
    ret = gc2145_write(i2c_dev, 0xfe, 0x0);
    ret += gc2145_read(i2c_dev, 0x05, &tmp);
    hts = tmp;
    ret += gc2145_read(i2c_dev, 0x06, &tmp);
    if(ret < 0)
        return -1;
    hts = ((hts << 8) + tmp) << 1;

    vts = wpclk * (fps & 0xffff) / hts / ((fps & 0xffff0000) >> 16);
    ret = gc2145_write(i2c_dev, 0x41, (unsigned char)((vts & 0x3f00) >> 8));
    ret += gc2145_write(i2c_dev, 0x42, (unsigned char)(vts & 0xff));
    if(ret < 0)
        return -1;

    sensor_info->fps = fps;
    sensor_info->total_height = vts;
    sensor_info->max_integration_time = vts - 4;

    return 0;
}


static struct sensor_attr gc2145_sensor_attr = {
    .device_name            = GC2145_DEVICE_NAME,
    .cbus_addr              = GC2145_DEVICE_I2C_ADDR,

    .dbus_type              = SENSOR_DATA_BUS_DVP,
    .dvp = {
        .data_fmt           = DVP_YUV422,
        .gpio_mode          = DVP_PA_LOW_8BIT,
        .timing_mode        = DVP_HREF_MODE,
        .yuv_data_order = order_1_2_3_4,
        .hsync_polarity = POLARITY_HIGH_ACTIVE,
        .vsync_polarity = POLARITY_LOW_ACTIVE,
        .img_scan_mode  = DVP_IMG_SCAN_PROGRESS,
    },

    .isp_clk_rate           = 90 * 1000 * 1000,

    .sensor_info = {
        .private_init_setting   = gc2145_init_regs,
        .width                  = GC2145_WIDTH,
        .height                 = GC2145_HEIGHT,
        .fmt                    = SENSOR_PIXEL_FMT_YUYV8_2X8,
        .fps                    = 25 << 16 | 1,
    },

    .ops = {
        .power_on               = gc2145_power_on,
        .power_off              = gc2145_power_off,
        .stream_on              = gc2145_stream_on,
        .stream_off             = gc2145_stream_off,
        .get_register           = gc2145_g_register,
        .set_register           = gc2145_s_register,

        .set_integration_time   = gc2145_set_integration_time,
        .alloc_dgain            = gc2145_alloc_dgain,
        .set_digital_gain       = gc2145_set_digital_gain,
        .set_fps                = gc2145_set_fps,
    },
};

static int gc2145_probe(struct i2c_client *client,
        const struct i2c_device_id *id)
{
    int ret = init_gpio();
    if (ret)
        return ret;

    ret = dvp_init_select_gpio(&gc2145_sensor_attr.dvp, gc2145_sensor_attr.dvp.gpio_mode);
    if (ret) {
        deinit_gpio();
        return ret;
    }

    ret = camera_register_sensor(cam_bus_num, &gc2145_sensor_attr);
    if (ret) {
        dvp_deinit_gpio();
        deinit_gpio();
        return ret;
    }

    return 0;
}

static int gc2145_remove(struct i2c_client *client)
{
    camera_unregister_sensor(cam_bus_num, &gc2145_sensor_attr);
    dvp_deinit_gpio();
    deinit_gpio();

    return 0;
}

static struct i2c_device_id gc2145_id[] = {
    { GC2145_DEVICE_NAME, 0 },
    { }
};
MODULE_DEVICE_TABLE(i2c, gc2145_id);

static struct i2c_driver gc2145_driver = {
    .driver = {
        .owner          = THIS_MODULE,
        .name           = GC2145_DEVICE_NAME,
    },
    .probe              = gc2145_probe,
    .remove             = gc2145_remove,
    .id_table           = gc2145_id,
};

static struct i2c_board_info sensor_gc2145_info = {
    .type               = GC2145_DEVICE_NAME,
    .addr               = GC2145_DEVICE_I2C_ADDR,
};

static __init int init_gc2145(void)
{
    if (i2c_bus_num < 0) {
        printk(KERN_ERR "gc2145: i2c_bus_num must be set\n");
        return -EINVAL;
    }
    gc2145_driver.driver.name = sensor_name;
    strcpy(gc2145_id[0].name, sensor_name);
    if (i2c_addr != -1)
        sensor_gc2145_info.addr = i2c_addr;
    strcpy(sensor_gc2145_info.type, sensor_name);
    gc2145_sensor_attr.device_name = sensor_name;

    int ret = i2c_add_driver(&gc2145_driver);
    if (ret) {
        printk(KERN_ERR "gc2145: failed to register i2c driver\n");
        return ret;
    }

    i2c_dev = i2c_register_device(&sensor_gc2145_info, i2c_bus_num);
    if (i2c_dev == NULL) {
        printk(KERN_ERR "gc2145: failed to register i2c device\n");
        i2c_del_driver(&gc2145_driver);
        return -EINVAL;
    }

    return 0;
}

static __exit void exit_gc2145(void)
{
    i2c_unregister_device(i2c_dev);
    i2c_del_driver(&gc2145_driver);
}

module_init(init_gc2145);
module_exit(exit_gc2145);

MODULE_DESCRIPTION("x2000 gc2145 driver");
MODULE_LICENSE("GPL");
