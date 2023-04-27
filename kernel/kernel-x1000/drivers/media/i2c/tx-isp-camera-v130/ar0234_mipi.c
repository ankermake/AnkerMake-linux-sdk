/*
 * ar0234 Camera Driver
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

#include <linux/sensor_board.h>
#include <tx-isp/tx-isp-common.h>
#include <tx-isp/sensor-common.h>

#define AR0234_CHIP_ID                  0xa56
#define AR0234_SUPPORT_30FPS_SCLK      (62000000)

#define SENSOR_OUTPUT_MAX_FPS          30
#define SENSOR_OUTPUT_MIN_FPS          5

#define SENSOR_VERSION                 "H20200824"

#define AR0234_WIDTH                   1920
#define AR0234_HEIGHT                  800

struct regval_list {
    unsigned short reg_num;
    unsigned short value;
};

#define AR0234_REG_END                0xffff
#define AR0234_REG_DELAY              0xfffe     //Tell the Init script to insert a delay

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


struct tx_isp_sensor_attribute ar0234_attr;
#ifndef MODULE
static struct sensor_board_info *ar0234_board_info;
#endif

#define  AR0234_RESET_REGISTER          0x301A
#define  AR0234_VT_PIX_CLK_DIV          0x302A
#define  AR0234_VT_SYS_CLK_DIV          0x302C
#define  AR0234_PRE_PLL_CLK_DIV         0x302E
#define  AR0234_PLL_MULTIPLIER          0x3030
#define  AR0234_OP_PIX_CLK_DIV          0x3036
#define  AR0234_OP_SYS_CLK_DIV          0x3038
#define  AR0234_DIGITAL_TEST            0x30B0
// Sets to 2-Lane MIPI
#define  AR0234_SERIAL_FORMAT           0x31AE
//Configures Sensor Resolution
#define  AR0234_Y_ADDR_START            0x3002
#define  AR0234_X_ADDR_START            0x3004
#define  AR0234_Y_ADDR_END              0x3006
#define  AR0234_X_ADDR_END              0x3008
#define  AR0234_FRAME_LENGTH_LINES      0x300A
#define  AR0234_LINE_LENGTH_PCK         0x300C
#define  AR0234_COARSE_INTEGRATION_TIME 0x3012
//Configures 8-Bit MIPI Mode
#define  AR0234_DATA_FORMAT_BITS        0x31AC
//Configures MIPI Timing Registers for Rev2 Silicon
#define  AR0234_FRAME_PREAMBLE          0x31B0
#define  AR0234_LINE_PREAMBLE           0x31B2
#define  AR0234_MIPI_TIMING_0           0x31B4
#define  AR0234_MIPI_TIMING_1           0x31B6
#define  AR0234_MIPI_TIMING_2           0x31B8
#define  AR0234_MIPI_TIMING_3           0x31BA
#define  AR0234_MIPI_TIMING_4           0x31BC
#define  AR0234_MIPI_CNTRL              0x3354

#define  AR0234_DATAPATH_SELECT         0x306E
#define  AR0234_X_ODD_INC               0x30A2
#define  AR0234_Y_ODD_INC               0x30A6
#define  AR0234_OPERATION_MODE_CTRL     0x3082
#define  AR0234_READ_MODE               0x3040
#define  AR0234_COMPANDING              0x31D0
//Recommended Settings
#define  AR0234_RESERVED_MFR_3096       0x3096
#define  AR0234_SEQ_CTRL_PORT           0x3088
#define  AR0234_SEQ_DATA_PORT           0x3086
#define  AR0234_RESERVED_MFR_3ED2       0x3ED2
#define  AR0234_DELTA_DK_CONTROL        0x3180
#define  AR0234_RESERVED_MFR_3ECC       0x3ECC
#define  AR0234_RESERVED_MFR_3ECC       0x3ECC
#define  AR0234_RESERVED_MFR_30BA       0x30BA

#define  AR0234_RESERVED_MFR_30F0       0x30F0
#define  AR0234_AE_LUMA_TARGET_REG      0x3102

#define  AR0234_ANALOG_GAIN             0x3060
#define  AR0234_RESERVED_MFR_3ED2       0x3ED2
#define  AR0234_RESERVED_MFR_3EEE       0x3EEE
//Defective Pixel Correction
#define  AR0234_RESERVED_MFR_3F4C       0x3F4C
#define  AR0234_RESERVED_MFR_3F4E       0x3F4E
#define  AR0234_RESERVED_MFR_3F50       0x3F50
#define  AR0234_PIX_DEF_ID              0x31E0

//Enables Flash Output
#define  AR0234_LED_FLASH_CONTROL       0x3270


/*
 * the part of driver maybe modify about different sensor and different board.
 */
struct again_lut {
    unsigned char value;
    unsigned int gain;
};

struct again_lut ar0234_again_lut[] = {
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

struct tx_isp_sensor_attribute ar0234_attr;

static unsigned int ar0234_alloc_again(unsigned int isp_gain, unsigned char shift, unsigned int *sensor_again)
{
    struct again_lut *lut = ar0234_again_lut;

    while(lut->gain <= ar0234_attr.max_again) {
        if(isp_gain == 0) {
            *sensor_again = 0;
            return 0;

        } else if (isp_gain < lut->gain) {
            *sensor_again = (lut - 1)->value;
            return (lut - 1)->gain;

        } else {
            if((lut->gain == ar0234_attr.max_again) && (isp_gain >= lut->gain)) {
                *sensor_again = lut->value;
                return lut->gain;
            }
        }
        lut++;
    }

    return isp_gain;
}

unsigned int ar0234_alloc_dgain(unsigned int isp_gain, unsigned char shift, unsigned int *sensor_dgain)
{
    return isp_gain;
}


struct tx_isp_sensor_attribute ar0234_attr={

    .name = "ar0234",
    .chip_id = 0x0a56,
    .cbus_type = TX_SENSOR_CONTROL_INTERFACE_I2C,
    .cbus_mask = V4L2_SBUS_MASK_SAMPLE_16BITS | V4L2_SBUS_MASK_ADDR_16BITS,
    .cbus_device = 0x18,
    .dbus_type = TX_SENSOR_DATA_INTERFACE_MIPI,
    .mipi = {
        .clk = 504,
        .lans = 2,
    },
    .max_again = 262144,
    .max_dgain = 0,
    .min_integration_time = 2,
    .min_integration_time_native = 2,

    .max_integration_time_native = 0x348 - 4,
    .integration_time_limit = 0x348 - 4,
    .total_width = 0x264,
    .total_height = 0x348,
    .max_integration_time = 0x348 - 4,
    .integration_time_apply_delay = 2,
    .again_apply_delay = 2,
    .dgain_apply_delay = 2,
    .sensor_ctrl.alloc_again = ar0234_alloc_again,
    .sensor_ctrl.alloc_dgain = ar0234_alloc_dgain,
    //    void priv; /* point to struct tx_isp_sensor_board_info */
};

static struct regval_list ar0234_init_regs_mipi_1920_800[] = {

    { AR0234_REG_DELAY, 200 }, // delay 200ms
    { AR0234_RESET_REGISTER, 0xD9 },
    { AR0234_REG_DELAY, 200 }, // delay 200ms

    //Configures PLL for 24MHz input 63MHz PXL_CLK _384Mbps/lane
    /* 24MHz input, target 24 MHz, 2 mipi lane, 5ms integration  */
    { AR0234_VT_PIX_CLK_DIV, 0x8 },
    { AR0234_VT_SYS_CLK_DIV, 0x2 },
    { AR0234_PRE_PLL_CLK_DIV, 0x2 },
    { AR0234_PLL_MULTIPLIER, 0x2a },
    { AR0234_OP_PIX_CLK_DIV, 0x8 },
    { AR0234_OP_SYS_CLK_DIV, 0x4 },

    { AR0234_DIGITAL_TEST, 0x00 },
    // Sets to 2-Lane MIPI
    { AR0234_SERIAL_FORMAT, 0x202 },

    //Configures Sensor Resolution
    { AR0234_Y_ADDR_START, 0xD0 },
    { AR0234_X_ADDR_START, 0x8 },
    { AR0234_Y_ADDR_END, 0x3EF },
    { AR0234_X_ADDR_END, 0x787 },
    { AR0234_FRAME_LENGTH_LINES, 0x348 },
    { AR0234_LINE_LENGTH_PCK, 0x264 }, //  (increased from 0x264 when width > 1600 pixels)
//  { AR0234_LINE_LENGTH_PCK, 0x2E4 }, //  (increased from 0x264 when width > 1600 pixels)

    { AR0234_COARSE_INTEGRATION_TIME, 0x118 },

    //Configures 8-Bit MIPI Mode
    { AR0234_DATA_FORMAT_BITS, 0xA08 },

    //Configures MIPI Timing Registers for Rev2 Silicon
    { AR0234_FRAME_PREAMBLE, 0x62 },
    { AR0234_LINE_PREAMBLE, 0x4A },
    { AR0234_MIPI_TIMING_0, 0x5186 },
    { AR0234_MIPI_TIMING_1, 0x51D4 },
    { AR0234_MIPI_TIMING_2, 0x60CB },
    { AR0234_MIPI_TIMING_3, 0x208 },
    { AR0234_MIPI_TIMING_4, 0xA87 },
    { AR0234_MIPI_CNTRL, 0x002A },

    { AR0234_RESET_REGISTER, 0x2058 },
    { AR0234_DATAPATH_SELECT, 0x9010 },
    { AR0234_X_ODD_INC, 0x1 },
    { AR0234_Y_ODD_INC, 0x1 },
    { AR0234_OPERATION_MODE_CTRL, 0x3 },
    { AR0234_READ_MODE, 0x0 },
    { AR0234_COMPANDING, 0x0 },
//  { AR0234_TEMPSENS_CTRL, 0x11},

    //Recommended Settings
    { AR0234_RESERVED_MFR_3096, 0x280 },
    { AR0234_SEQ_CTRL_PORT, 0x81BA },
    { AR0234_SEQ_DATA_PORT, 0x3D02 },
    { AR0234_RESERVED_MFR_3ED2, 0xFA86 },
    { AR0234_DELTA_DK_CONTROL, 0x824F },
    { AR0234_RESERVED_MFR_3ECC, 0x6D42 },
    { AR0234_RESERVED_MFR_3ECC, 0xD42 },
    { AR0234_RESERVED_MFR_30BA, 0x7622 },

    { AR0234_RESERVED_MFR_30F0, 0x2283 },
    { AR0234_AE_LUMA_TARGET_REG, 0x5000 },

    { AR0234_ANALOG_GAIN, 0xD },
    { AR0234_RESERVED_MFR_3ED2, 0xAA86 },
    { AR0234_RESERVED_MFR_3EEE, 0xA4AA },

    //Defective Pixel Correction
    { AR0234_RESERVED_MFR_3F4C, 0x121F },
    { AR0234_RESERVED_MFR_3F4E, 0x121F },
    { AR0234_RESERVED_MFR_3F50, 0xB81 },
    { AR0234_PIX_DEF_ID, 0x3 },
    //Enables Flash Output
    { AR0234_LED_FLASH_CONTROL, 0x100 },
    { AR0234_RESET_REGISTER, 0x2058 },

    {AR0234_REG_END, 0x00}, /* END MARKER */
};

/*
 * the order of the ar0234_win_sizes is [full_resolution, preview_resolution].
 */
static struct tx_isp_sensor_win_setting ar0234_win_sizes[] = {
    {
        .width        = AR0234_WIDTH,
        .height       = AR0234_HEIGHT,
        .fps          = SENSOR_OUTPUT_MAX_FPS << 16 | 1,
        .mbus_code    = V4L2_MBUS_FMT_SRGGB8_1X8,
        .colorspace   = V4L2_COLORSPACE_SRGB,
        .regs         = ar0234_init_regs_mipi_1920_800,
    }
};

/*
 * the part of driver was fixed.
 */

static struct regval_list ar0234_stream_on[] = {
    {0x301A,0x205C},
    {AR0234_REG_END,0x0},    /* END MARKER */
};

static struct regval_list ar0234_stream_off[] = {
    {0x301A,0x2058},
    {AR0234_REG_END,0x0},    /* END MARKER */
};


static int ar0234_read(struct tx_isp_subdev *sd, unsigned short reg,
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


static int ar0234_write(struct tx_isp_subdev *sd, uint16_t reg,
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

static inline int ar0234_read_array(struct tx_isp_subdev *sd, struct regval_list *vals)
{
    int ret;
    unsigned short val;
    while (vals->reg_num != AR0234_REG_END) {
        if (vals->reg_num == AR0234_REG_DELAY) {
                private_msleep(vals->value);
                printk("msleep %d.\n",vals->value);
        } else {
            ret = ar0234_read(sd, vals->reg_num, &val);
            if (ret < 0)
                return ret;

        printk("reg_num 0x%x,value 0x%x.\n",vals->reg_num,val);
        }
        vals++;
    }
    return 0;
}

static int ar0234_write_array(struct tx_isp_subdev *sd, struct regval_list *vals)
{
    int ret;
    while (vals->reg_num != AR0234_REG_END) {
        if (vals->reg_num == AR0234_REG_DELAY) {
             private_msleep(vals->value);
//             printk("msleep %d.\n",vals->value);
        } else {
            ret = ar0234_write(sd, vals->reg_num, vals->value);
//            printk("reg_num 0x%x,value 0x%x.\n",vals->reg_num,vals->value);
            if (ret < 0)
                return ret;
        }
        vals++;
    }
    return 0;
}

static int ar0234_reset(struct tx_isp_subdev *sd, int val)
{
    return 0;
}

static int ar0234_detect(struct tx_isp_subdev *sd, unsigned int *ident)
{
    unsigned short val;
    int ret;

    ret = ar0234_read(sd, 0x3000, &val);

    if (ret < 0)
        return ret;
    if (val != AR0234_CHIP_ID)
        return -ENODEV;

    return 0;

}

static int ar0234_set_integration_time(struct tx_isp_subdev *sd, int value)
{
    int ret;

    ret = ar0234_write(sd,0x3012, value);

    if (ret < 0)
        return ret;

    return 0;

}

static int ar0234_set_analog_gain(struct tx_isp_subdev *sd, u16 value)
{
    int ret;
    ret = ar0234_write(sd,0x3060, value);
    if (ret < 0)
        return ret;

    return 0;
}

static int ar0234_set_digital_gain(struct tx_isp_subdev *sd, int value)
{
    return 0;
}

static int ar0234_get_black_pedestal(struct tx_isp_subdev *sd, int value)
{
    return 0;
}

static int ar0234_init(struct tx_isp_subdev *sd, int enable)
{
    struct tx_isp_sensor *sensor = (container_of(sd, struct tx_isp_sensor, sd));
    struct tx_isp_sensor_win_setting *wsize = &ar0234_win_sizes[0];
    int ret = 0;
//    unsigned char v[2]={0};

    if(!enable)
        return ISP_SUCCESS;

    sensor->video.mbus.width = wsize->width;
    sensor->video.mbus.height = wsize->height;
    sensor->video.mbus.code = wsize->mbus_code;
    sensor->video.mbus.field = V4L2_FIELD_NONE;
    sensor->video.mbus.colorspace = wsize->colorspace;
    sensor->video.fps = wsize->fps;
    ret = ar0234_write_array(sd, wsize->regs);
    if (ret)
        return ret;

//    ret = ar0234_read_array(sd,wsize->regs);
//    ret = ar0234_read(sd, 0x301A, v);
//    printk("read value :0x%x%x.\n",v[0],v[1]);

    ret = tx_isp_call_subdev_notify(sd, TX_ISP_EVENT_SYNC_SENSOR_ATTR, &sensor->video);
    sensor->priv = wsize;

    return 0;
}

static int ar0234_s_stream(struct tx_isp_subdev *sd, int enable)
{
    int ret = 0;

    if (enable) {
        ret = ar0234_write_array(sd, ar0234_stream_on);
        printk("ar0234 stream on\n");
    }
    else {
        ret = ar0234_write_array(sd, ar0234_stream_off);
        printk("ar0234 stream off\n");
    }

    return ret;
}

static int ar0234_set_fps(struct tx_isp_subdev *sd, int fps)
{
    struct tx_isp_sensor *sensor = sd_to_sensor_device(sd);
    unsigned int wpclk = 0;
    unsigned short vts = 0;
    unsigned short hts=0;
    unsigned int max_fps = 0;
    unsigned int newformat = 0; //the format is 24.8
    int ret = 0;

    wpclk = AR0234_SUPPORT_30FPS_SCLK;
    max_fps = SENSOR_OUTPUT_MAX_FPS;

    newformat = (((fps >> 16) / (fps & 0xffff)) << 8) + ((((fps >> 16) % (fps & 0xffff)) << 8) / (fps & 0xffff));
    if(newformat > (max_fps << 8) || newformat < (SENSOR_OUTPUT_MIN_FPS << 8)){
        printk("warn: fps(%x) no in range\n", fps);
        return -1;
    }
    ret += ar0234_read(sd, 0x300C, &hts);
    if(ret < 0)
        return -1;

    hts *= 4;
    vts = wpclk * (fps & 0xffff) / hts / ((fps & 0xffff0000) >> 16);
    ret = ar0234_write(sd, 0x300A, vts);
    if(ret < 0)
        return -1;

    sensor->video.fps = fps;
    sensor->video.attr->max_integration_time_native = vts - 4;
    sensor->video.attr->integration_time_limit = vts - 4;
    sensor->video.attr->total_height = vts;
    sensor->video.attr->max_integration_time = vts - 4;
    ret = tx_isp_call_subdev_notify(sd, TX_ISP_EVENT_SYNC_SENSOR_ATTR, &sensor->video);

    return ret;
}

static int ar0234_set_mode(struct tx_isp_subdev *sd, int value)
{
    struct tx_isp_sensor *sensor = sd_to_sensor_device(sd);
    struct tx_isp_sensor_win_setting *wsize = NULL;
    int ret = ISP_SUCCESS;

    if(value == TX_ISP_SENSOR_FULL_RES_MAX_FPS){
        wsize = &ar0234_win_sizes[0];
    }else if(value == TX_ISP_SENSOR_PREVIEW_RES_MAX_FPS){
        wsize = &ar0234_win_sizes[0];
    }

    if(wsize){
        sensor->video.mbus.width = wsize->width;
        sensor->video.mbus.height = wsize->height;
        sensor->video.mbus.code = wsize->mbus_code;
        sensor->video.mbus.field = V4L2_FIELD_NONE;
        sensor->video.mbus.colorspace = wsize->colorspace;
        sensor->video.fps = wsize->fps;
        ret = tx_isp_call_subdev_notify(sd, TX_ISP_EVENT_SYNC_SENSOR_ATTR, &sensor->video);
    }

    return ret;
}

static int ar0234_set_vflip(struct tx_isp_subdev *sd, int enable)
{
    return 0;
}

static int ar0234_g_chip_ident(struct tx_isp_subdev *sd,
        struct tx_isp_chip_ident *chip)
{
    struct i2c_client *client = tx_isp_get_subdevdata(sd);
    unsigned int ident = 0;
    int ret = ISP_SUCCESS;

    if (power_gpio != -1) {
        ret = gpio_request(power_gpio, "ar0234_power");
        if (!ret) {
            private_gpio_direction_output(power_gpio, 1);
            private_msleep(50);
        }
        else
            printk("gpio requrest fail %d\n", power_gpio);
    }
    if(pwdn_gpio != -1){
        ret = private_gpio_request(pwdn_gpio,"ar0234_pwdn");
        if(!ret){
            private_gpio_direction_output(pwdn_gpio, 1);
            private_msleep(150);
        }else{
            printk("gpio requrest fail %d\n",pwdn_gpio);
        }
    }
    if(reset_gpio != -1){
        ret = private_gpio_request(reset_gpio,"ar0234_reset");
        if(!ret){
            private_gpio_direction_output(reset_gpio, 0);
            private_msleep(20);
            private_gpio_direction_output(reset_gpio, 1);
            private_msleep(20);
            private_gpio_direction_output(reset_gpio, 0);
            private_msleep(86);
        }else{
            printk("gpio requrest fail %d\n",reset_gpio);
        }
    }

    private_msleep(70);
    ret = ar0234_detect(sd, &ident);
    if (ret) {
        printk("chip found @ 0x%x (%s) is not an ar0234 chip.\n",
                client->addr, client->adapter->name);
        return ret;
    }
    printk("ar0234 chip found @ 0x%02x (%s)\n", client->addr, client->adapter->name);
    if(chip){
        memcpy(chip->name, "ar0234", sizeof("ar0234"));
        chip->ident = ident;
        chip->revision = SENSOR_VERSION;
    }
    return 0;
}

static int ar0234_sensor_ops_ioctl(struct tx_isp_subdev *sd, unsigned int cmd, void *arg)
{
    long ret = 0;

    if(IS_ERR_OR_NULL(sd)){
        printk("[%d]The pointer is invalid!\n", __LINE__);
        return -EINVAL;
    }
    switch(cmd){
        case TX_ISP_EVENT_SENSOR_INT_TIME:
            if(arg)
                ret = ar0234_set_integration_time(sd, *(int*)arg);
            break;
        case TX_ISP_EVENT_SENSOR_AGAIN:
            if(arg)
                ret = ar0234_set_analog_gain(sd, *(u16*)arg);
            break;
        case TX_ISP_EVENT_SENSOR_DGAIN:
            if(arg)
                ret = ar0234_set_digital_gain(sd, *(int*)arg);
            break;
        case TX_ISP_EVENT_SENSOR_BLACK_LEVEL:
            if(arg)
                ret = ar0234_get_black_pedestal(sd, *(int*)arg);
            break;
        case TX_ISP_EVENT_SENSOR_RESIZE:
            if(arg)
                ret = ar0234_set_mode(sd, *(int*)arg);
            break;
        case TX_ISP_EVENT_SENSOR_PREPARE_CHANGE:
            ret = ar0234_write_array(sd, ar0234_stream_off);
            break;
        case TX_ISP_EVENT_SENSOR_FINISH_CHANGE:
            ret = ar0234_write_array(sd, ar0234_stream_on);
            break;
        case TX_ISP_EVENT_SENSOR_FPS:
            if(arg)
                ret = ar0234_set_fps(sd, *(int*)arg);
            break;
        case TX_ISP_EVENT_SENSOR_VFLIP:
            if(arg)
                ret = ar0234_set_vflip(sd, *(int*)arg);
            break;
        default:
            break;;
    }

    return 0;
}

static int ar0234_g_register(struct tx_isp_subdev *sd, struct tx_isp_dbg_register *reg)
{
    unsigned short val = 0;
    int len = 0;
    int ret = 0;

    len = strlen(sd->chip.name);
    if(len && strncmp(sd->chip.name, reg->name, len)){
        return -EINVAL;
    }
    if (!private_capable(CAP_SYS_ADMIN))
        return -EPERM;
    ret = ar0234_read(sd, reg->reg & 0xffff, &val);
    reg->val = val;
    reg->size = 2;

    return ret;
}

static int ar0234_s_register(struct tx_isp_subdev *sd, const struct tx_isp_dbg_register *reg)
{
    int len = 0;

    len = strlen(sd->chip.name);
    if(len && strncmp(sd->chip.name, reg->name, len)){
        return -EINVAL;
    }
    if (!private_capable(CAP_SYS_ADMIN))
        return -EPERM;
    ar0234_write(sd, reg->reg & 0xffff, reg->val & 0xffff);

    return 0;
}

static struct tx_isp_subdev_core_ops ar0234_core_ops = {
    .g_chip_ident = ar0234_g_chip_ident,
    .reset = ar0234_reset,
    .init = ar0234_init,
    /*.ioctl = ar0234_ops_ioctl,*/
    .g_register = ar0234_g_register,
    .s_register = ar0234_s_register,
};

static struct tx_isp_subdev_video_ops ar0234_video_ops = {
    .s_stream = ar0234_s_stream,
};

static struct tx_isp_subdev_sensor_ops ar0234_sensor_ops = {
    .ioctl    = ar0234_sensor_ops_ioctl,
};

static struct tx_isp_subdev_ops ar0234_ops = {
    .core = &ar0234_core_ops,
    .video = &ar0234_video_ops,
    .sensor = &ar0234_sensor_ops,
};

/* It's the sensor device */
static u64 tx_isp_module_dma_mask = ~(u64)0;
struct platform_device ar0234_platform_device = {
    .name = "ar0234",
    .id = -1,
    .dev = {
        .dma_mask = &tx_isp_module_dma_mask,
        .coherent_dma_mask = 0xffffffff,
        .platform_data = NULL,
    },
    .num_resources = 0,
};

static int ar0234_probe(struct i2c_client *client, const struct i2c_device_id *id)
{
    struct tx_isp_subdev *sd;
    struct tx_isp_sensor *sensor;
    struct tx_isp_sensor_win_setting *wsize = &ar0234_win_sizes[0];

    printk("probe ok ----start-w %d h %d -->ar0234\n",AR0234_WIDTH, AR0234_HEIGHT);
    sensor = (struct tx_isp_sensor *)kzalloc(sizeof(*sensor), GFP_KERNEL);
    if(!sensor){
        printk("Failed to allocate sensor subdev.\n");
        return -ENOMEM;
    }
    memset(sensor, 0 ,sizeof(*sensor));

#ifndef MODULE
    ar0234_board_info = get_sensor_board_info(ar0234_platform_device.name);
    if (ar0234_board_info) {
        power_gpio    = ar0234_board_info->gpios.gpio_power;
        reset_gpio    = ar0234_board_info->gpios.gpio_sensor_rst;
        i2c_sel1_gpio = ar0234_board_info->gpios.gpio_i2c_sel1;
        pwdn_gpio      = ar0234_board_info->gpios.gpio_sensor_pwdn;
    }
#endif

#if 1
    sensor->mclk = clk_get(NULL, "cgu_cim");
    if (IS_ERR(sensor->mclk)) {
        printk("Cannot get sensor input clock cgu_cim\n");
        goto err_get_mclk;
    }
    private_clk_set_rate(sensor->mclk, 24000000);
    private_clk_enable(sensor->mclk);
#endif
     /*
        convert sensor-gain into isp-gain,
     */
    ar0234_attr.max_again = 262144;
    ar0234_attr.max_dgain = 0;
    sd = &sensor->sd;
    sensor->video.attr = &ar0234_attr;
    sensor->video.vi_max_width = wsize->width;
    sensor->video.vi_max_height = wsize->height;
    sensor->video.mbus.width = wsize->width;
    sensor->video.mbus.height = wsize->height;
    sensor->video.mbus.code = wsize->mbus_code;
    sensor->video.mbus.field = V4L2_FIELD_NONE;
    sensor->video.mbus.colorspace = wsize->colorspace;
    sensor->video.fps = wsize->fps;
    sensor->video.dc_param = NULL;

    tx_isp_subdev_init(&ar0234_platform_device, sd, &ar0234_ops);

    tx_isp_set_subdevdata(sd, client);
    tx_isp_set_subdev_hostdata(sd, sensor);
    private_i2c_set_clientdata(client, sd);
    pr_debug("probe ok ------->ar0234\n");

    return 0;
#if 1
err_get_mclk:
    kfree(sensor);
#endif

    return -1;
}

static int ar0234_remove(struct i2c_client *client)
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

static const struct i2c_device_id ar0234_id[] = {
    { "ar0234", 0 },
    { }
};
MODULE_DEVICE_TABLE(i2c, ar0234_id);

static struct i2c_driver ar0234_driver = {
    .driver = {
        .owner    = THIS_MODULE,
        .name    = "ar0234",
    },
    .probe        = ar0234_probe,
    .remove        = ar0234_remove,
    .id_table    = ar0234_id,
};

static __init int init_ar0234(void)
{
    int ret = 0;
    ret = private_driver_get_interface();
    if(ret){
        printk("Failed to init ar0234 dirver.\n");
        return -1;
    }

    return private_i2c_add_driver(&ar0234_driver);
}

static __exit void exit_ar0234(void)
{
    private_i2c_del_driver(&ar0234_driver);
}

module_init(init_ar0234);
module_exit(exit_ar0234);

MODULE_DESCRIPTION("A low-level driver for Onsemi ar0234 sensors");
MODULE_LICENSE("GPL");

