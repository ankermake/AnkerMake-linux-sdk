



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

#define OV7740_HTS                          791
#define OV7740_VTS                          252

#define OV7740_CHIP_ID                      (0x7740)
#define OV7740_CHIP_ID_H                    (0x77)
#define OV7740_CHIP_ID_L                    (0x42)

#define OV7740_I2C_ADDER                    0x21

#define SENSOR_VERSION                    "H20200820"


static int power_gpio = -1;
module_param(power_gpio, int, S_IRUGO);
MODULE_PARM_DESC(power_gpio, "Power on GPIO NUM");

static int reset_gpio = -1;
module_param(reset_gpio, int, S_IRUGO);
MODULE_PARM_DESC(reset_gpio, "Sensor reset GPIO NUM");

static int i2c_sel1_gpio = -1;
module_param(i2c_sel1_gpio, int, S_IRUGO);
MODULE_PARM_DESC(i2c_sel1_gpio, "I2C select 1 GPIO NUM");

static int dvp_gpio_func = -1;
module_param(dvp_gpio_func, int, S_IRUGO);
MODULE_PARM_DESC(dvp_gpio_func, "Sensor DVP GPIO function");


struct tx_isp_sensor_attribute ov7740_attr;
static struct sensor_board_info *ov7740_board_info;

static unsigned int ov7740_alloc_again(unsigned int isp_gain,unsigned char shift,unsigned int *sensor_again){

    return 0;
}

static unsigned int ov7740_alloc_dgain(unsigned int isp_gain,unsigned char shift,unsigned int *sensor_dgain){

    return 0;
}

struct tx_isp_sensor_attribute ov7740_attr = {
    .name = "ov7740",
    .chip_id = OV7740_CHIP_ID,
    .cbus_type = TX_SENSOR_CONTROL_INTERFACE_I2C,
    .cbus_mask = V4L2_SBUS_MASK_SAMPLE_8BITS | V4L2_SBUS_MASK_ADDR_8BITS,
    .cbus_device = OV7740_I2C_ADDER,
    .dbus_type = TX_SENSOR_DATA_INTERFACE_DVP,
    .dvp = {
        .mode = SENSOR_DVP_HREF_MODE,
        .blanking = {
            .vblanking = 0,
            .hblanking = 0,
        },
    },
    .max_again = 0xff << (TX_ISP_GAIN_FIXED_POINT - 4),
    .max_dgain = 0,
    .min_integration_time = 1,
    .min_integration_time_native = 1,
    .max_integration_time_native = OV7740_VTS - 4,
    .integration_time_limit = OV7740_VTS - 4,
    .total_width = OV7740_HTS,
    .total_height = OV7740_VTS,
    .max_integration_time = OV7740_VTS - 4,
    .integration_time_apply_delay = 2,
    .again_apply_delay = 1,
    .dgain_apply_delay = 1,
    .sensor_ctrl.alloc_again = ov7740_alloc_again,
    .sensor_ctrl.alloc_dgain = ov7740_alloc_dgain,
    // .one_line_expr_in_us = 44,
    //void priv; /* point to struct tx_isp_sensor_board_info */
};

static struct camera_reg_op ov7740_init_regs_640_480_25fps[] = {

    // {CAMERA_REG_OP_DATA, 0x12, 0x80},    // soft reset
    {CAMERA_REG_OP_DATA, 0x47, 0x02},

    {CAMERA_REG_OP_DATA, 0x17, 0x27},    // AHSTART
    {CAMERA_REG_OP_DATA, 0x04, 0x40},    // analog setting
    {CAMERA_REG_OP_DATA, 0x1B, 0x81},    // pixel shift
    {CAMERA_REG_OP_DATA, 0x29, 0x17},    //
    {CAMERA_REG_OP_DATA, 0x5F, 0x03},    // analog setting
    {CAMERA_REG_OP_DATA, 0x3A, 0x09},
    {CAMERA_REG_OP_DATA, 0x33, 0x44},    // HVSIZEOFF
    {CAMERA_REG_OP_DATA, 0x68, 0x1A},    // BLC

    {CAMERA_REG_OP_DATA, 0x14, 0x38},    // AGC & LAEC
    {CAMERA_REG_OP_DATA, 0x5F, 0x04},    // analog setting
    {CAMERA_REG_OP_DATA, 0x64, 0x00},    // BLC
    {CAMERA_REG_OP_DATA, 0x67, 0x90},    // BLC
    {CAMERA_REG_OP_DATA, 0x27, 0x80},    // black sun cancellation
    {CAMERA_REG_OP_DATA, 0x45, 0x41},
    {CAMERA_REG_OP_DATA, 0x4B, 0x40},    // analog setting
    {CAMERA_REG_OP_DATA, 0x36, 0x2f},
    {CAMERA_REG_OP_DATA, 0x11, 0x01},    // CLK
    {CAMERA_REG_OP_DATA, 0x36, 0x3f},
    {CAMERA_REG_OP_DATA, 0x0c, 0x12},    // flip & mirror & YUV out swap & max exposure

    {CAMERA_REG_OP_DATA, 0x12, 0x00},    // soft reset vertical skip mode & CC656 mode & sensor raw & output raw data RGB mode
    {CAMERA_REG_OP_DATA, 0x17, 0x25},    // AHSTART
    {CAMERA_REG_OP_DATA, 0x18, 0xa0},    // AHSIZE
    {CAMERA_REG_OP_DATA, 0x1a, 0xf0},    // AVSIZE
    {CAMERA_REG_OP_DATA, 0x31, 0xa0},    // HOUTSIZE
    {CAMERA_REG_OP_DATA, 0x32, 0xf0},    // VOUTSIZE

    {CAMERA_REG_OP_DATA, 0x85, 0x08},    // AGC OFFSET
    {CAMERA_REG_OP_DATA, 0x86, 0x02},    // AGC BASE1
    {CAMERA_REG_OP_DATA, 0x87, 0x01},    // AGC BASE2
    {CAMERA_REG_OP_DATA, 0xd5, 0x10},    // SCALE SMTH CTRL
    {CAMERA_REG_OP_DATA, 0x0d, 0x34},    // analog setting
    {CAMERA_REG_OP_DATA, 0x19, 0x03},    // AVSTART
    {CAMERA_REG_OP_DATA, 0x2b, 0xf8},    // automatically inserted dummy lines in night mode
    {CAMERA_REG_OP_DATA, 0x2c, 0x01},    // row counter end point MSBs

    {CAMERA_REG_OP_DATA, 0x53, 0x00},    // analog setting
    {CAMERA_REG_OP_DATA, 0x89, 0x30},    // LENC CTRL
    {CAMERA_REG_OP_DATA, 0x8d, 0x30},    // LENC RED A1
    {CAMERA_REG_OP_DATA, 0x8f, 0x85},    // LENC RED AB2
    {CAMERA_REG_OP_DATA, 0x93, 0x30},    // LENC GRN A1
    {CAMERA_REG_OP_DATA, 0x95, 0x85},    // LENC GRN AB2
    {CAMERA_REG_OP_DATA, 0x99, 0x30},    // LENC BLUE A1
    {CAMERA_REG_OP_DATA, 0x9b, 0x85},    // LENC BLUE AB2

    {CAMERA_REG_OP_DATA, 0xac, 0x6E},    // reserved
    {CAMERA_REG_OP_DATA, 0xbe, 0xff},    // reserved
    {CAMERA_REG_OP_DATA, 0xbf, 0x00},    // reserved
    {CAMERA_REG_OP_DATA, 0x38, 0x14},
    {CAMERA_REG_OP_DATA, 0xe9, 0x00},    // YAVG
    {CAMERA_REG_OP_DATA, 0x3D, 0x08},    // reserved
    {CAMERA_REG_OP_DATA, 0x3E, 0x80},
    {CAMERA_REG_OP_DATA, 0x3F, 0x40},
    {CAMERA_REG_OP_DATA, 0x40, 0x7F},
    {CAMERA_REG_OP_DATA, 0x41, 0x6A},
    {CAMERA_REG_OP_DATA, 0x42, 0x29},
    {CAMERA_REG_OP_DATA, 0x49, 0x64},
    {CAMERA_REG_OP_DATA, 0x4A, 0xA1},
    {CAMERA_REG_OP_DATA, 0x4E, 0x13},
    {CAMERA_REG_OP_DATA, 0x4D, 0x50},
    {CAMERA_REG_OP_DATA, 0x44, 0x58},
    {CAMERA_REG_OP_DATA, 0x4C, 0x1A},
    {CAMERA_REG_OP_DATA, 0x4E, 0x14},
    {CAMERA_REG_OP_DATA, 0x38, 0x11},
    {CAMERA_REG_OP_DATA, 0x84, 0x70},

    {CAMERA_REG_OP_END, 0x00, 0x00}, /* END MARKER */
};

static struct camera_reg_op ov7740_stream_on[] = {
//    { CAMERA_REG_OP_DATA, 0x80, 0x7F },
    { CAMERA_REG_OP_END, 0x00, 0x00 },        /* END MARKER */
};

static struct camera_reg_op ov7740_stream_off[] = {
//    { CAMERA_REG_OP_DATA, 0x80, 0x00 },
    { CAMERA_REG_OP_END, 0x00, 0x00 },        /* END MARKER */
};

static struct tx_isp_sensor_win_setting ov7740_win_sizes[] = {
    {
        .width = 640,
        .height = 480,
        .fps = 25 << 16 | 1,
        .mbus_code = V4L2_MBUS_FMT_YUYV8_2X8,
        .colorspace = V4L2_COLORSPACE_SRGB,
        .regs = ov7740_init_regs_640_480_25fps,
    }
};

int ov7740_read(struct tx_isp_subdev *sd,unsigned char reg,unsigned char *value)
{
    struct i2c_client *client = tx_isp_get_subdevdata(sd);
    struct i2c_msg msg[2] = {
        [0] ={
            .addr = client->addr,
            .flags = 0,
            .len = 1,
            .buf = &reg,
        },
        [1] =
        {
            .addr = client->addr,
            .flags = I2C_M_RD,
            .len = 1,
            .buf = value,
        }
    };

    int ret;
    ret = private_i2c_transfer(client->adapter, msg, 2);
    if (ret > 0)
        ret = 0;

    return ret;
}

int ov7740_write(struct tx_isp_subdev *sd,unsigned char reg,unsigned char value)
{
    struct i2c_client *client = tx_isp_get_subdevdata(sd);
    unsigned char buf[2] = { reg, value };

    struct i2c_msg msg = {
        .addr    = client->addr,
        .flags    = 0,
        .len    = 2,
        .buf    = buf,
    };

    int ret;

    ret = private_i2c_transfer(client->adapter, &msg, 1);
    if (ret > 0)
        ret = 0;

    return ret;
}


static int ov7740_write_array(struct tx_isp_subdev *sd,
                  struct camera_reg_op *vals)
{
    int ret;

    while (vals->flag != CAMERA_REG_OP_END) {
        if (vals->flag == CAMERA_REG_OP_DATA) {
            ret = ov7740_write(sd, vals->reg, vals->val);
            if (ret < 0) {
                printk("write reg data err [0x%02X - 0x%02X]\n", vals->reg, vals->val);
                return ret;
            }
        }
        else if (vals->flag == CAMERA_REG_OP_DELAY) {
            private_msleep(vals->val);
        }
        else {
            printk("error flag: %d\n", vals->flag);
            return -1;
        }
        vals++;
    }

    return 0;
}

static int ov7740_read_array(struct tx_isp_subdev *sd,
                 struct camera_reg_op *vals)
{
    int ret;
    unsigned char val;

    while (vals->flag != CAMERA_REG_OP_END) {
        if (vals->flag == CAMERA_REG_OP_DATA) {
            ret = ov7740_read(sd, vals->reg, &val);
            printk("read reg 0x%02X,value 0x%02X\n", vals->reg, val);
            if (ret < 0)
                return ret;
        }
        else if (vals->flag == CAMERA_REG_OP_DELAY) {
            private_msleep(vals->val);
        }
        else {
            printk("error flag: %d\n", vals->flag);
            return -1;
        }
        vals++;
    }

    return 0;
}

static int ov7740_set_integration_time(struct tx_isp_subdev *sd, int value){

    return 0;
}

static int ov7740_set_analog_gain(struct tx_isp_subdev *sd, int value){

    return 0;
}

static int ov7740_set_digital_gain(struct tx_isp_subdev *sd, int value){

    return 0;
}

static int ov7740_get_black_pedestal(struct tx_isp_subdev *sd, int value){

    return 0;
}

static int ov7740_set_mode(struct tx_isp_subdev *sd, int value)
{
    struct tx_isp_sensor *sensor = sd_to_sensor_device(sd);
    struct tx_isp_sensor_win_setting *wsize = NULL;
    int ret = ISP_SUCCESS;

    if (value == TX_ISP_SENSOR_FULL_RES_MAX_FPS) {
        wsize = &ov7740_win_sizes[0];
    } else if (value == TX_ISP_SENSOR_PREVIEW_RES_MAX_FPS) {
        wsize = &ov7740_win_sizes[0];
    }

    if (wsize) {
        sensor->video.mbus.width = wsize->width;
        sensor->video.mbus.height = wsize->height;
        sensor->video.mbus.code = wsize->mbus_code;
        sensor->video.mbus.field = V4L2_FIELD_NONE;
        sensor->video.mbus.colorspace = wsize->colorspace;
        sensor->video.fps = wsize->fps;
        ret = tx_isp_call_subdev_notify(
            sd, TX_ISP_EVENT_SYNC_SENSOR_ATTR, &sensor->video);
    }

    return ret;
}

static int ov7740_set_fps(struct  tx_isp_subdev *sd, int fps)
{
    int ret;
    struct tx_isp_sensor *sensor = sd_to_sensor_device(sd);

    sensor->video.fps = fps;

    ret = tx_isp_call_subdev_notify(sd, TX_ISP_EVENT_SYNC_SENSOR_ATTR, &sensor->video);
    return ret;
}

static int ov7740_set_vflip(struct tx_isp_subdev *sd, int enable){

    return 0;
}

static int ov7740_sensor_ops_ioctl(struct tx_isp_subdev *sd, unsigned int cmd, void *arg)
{
    long ret = 0;

    if (IS_ERR_OR_NULL(sd)) {
        printk(KERN_ERR "[%s.%d]The pointer is invalid!\n", __func__, __LINE__);
        return -EINVAL;
    }

    switch (cmd) {
    case TX_ISP_EVENT_SENSOR_INT_TIME:
        if (arg)
            ret = ov7740_set_integration_time(sd, *(int *)arg);
        break;
    case TX_ISP_EVENT_SENSOR_AGAIN:
        if (arg)
            ret = ov7740_set_analog_gain(sd, *(int *)arg);
        break;
    case TX_ISP_EVENT_SENSOR_DGAIN:
        if (arg)
            ret = ov7740_set_digital_gain(sd, *(int *)arg);
        break;
    case TX_ISP_EVENT_SENSOR_BLACK_LEVEL:
        if (arg)
            ret = ov7740_get_black_pedestal(sd, *(int *)arg);
        break;
    case TX_ISP_EVENT_SENSOR_RESIZE:
        if (arg)
            ret = ov7740_set_mode(sd, *(int *)arg);
        break;
    case TX_ISP_EVENT_SENSOR_PREPARE_CHANGE:
        if (arg)
            ret = ov7740_write_array(sd, ov7740_stream_off);
        break;
    case TX_ISP_EVENT_SENSOR_FINISH_CHANGE:
        if (arg)
            ret = ov7740_write_array(sd, ov7740_stream_on);
        break;
    case TX_ISP_EVENT_SENSOR_FPS:
        if (arg)
            ret = ov7740_set_fps(sd, *(int *)arg);
        break;
    case TX_ISP_EVENT_SENSOR_VFLIP:
        if (arg)
            ret = ov7740_set_vflip(sd, *(int *)arg);
        break;
    default:
        break;
    }

    return 0;
}

static int ov7740_s_stream(struct tx_isp_subdev *sd, int enable)
{
    int ret = 0;

    if (enable) {
        ret = ov7740_write_array(sd, ov7740_stream_on);
        printk("ov7740 stream on (%d)\n", ret);
    } else {
        ret = ov7740_write_array(sd, ov7740_stream_off);
        printk("ov7740 stream off (%d)\n", ret);
    }

    return ret;
}

static int ov7740_g_register(struct tx_isp_subdev *sd,
                 struct tx_isp_dbg_register *reg)
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
    ret = ov7740_read(sd, reg->reg & 0xff, &val);
    reg->val = val;
    reg->size = 1;

    return ret;

}


static int ov7740_s_register(struct tx_isp_subdev *sd,
                 const struct tx_isp_dbg_register *reg)
{
    int len = 0;

    len = strlen(sd->chip.name);
    if(len && strncmp(sd->chip.name, reg->name, len)){
        return -EINVAL;
    }
    if (!private_capable(CAP_SYS_ADMIN))
        return -EPERM;
    ov7740_write(sd, reg->reg & 0xff, reg->val & 0xff);

    return 0;

}

static int ov7740_init(struct tx_isp_subdev *sd, int enable)
{
    struct tx_isp_sensor *sensor =
        (container_of(sd, struct tx_isp_sensor, sd));
    struct tx_isp_sensor_win_setting *wsize = &ov7740_win_sizes[0];
    int ret = 0;

    if (!enable)
        return ISP_SUCCESS;

    sensor->video.mbus.width = wsize->width;
    sensor->video.mbus.height = wsize->height;
    sensor->video.mbus.code = wsize->mbus_code;
    sensor->video.mbus.field = V4L2_FIELD_NONE;
    sensor->video.mbus.colorspace = wsize->colorspace;
    sensor->video.fps = wsize->fps;

    ret = ov7740_write_array(sd, wsize->regs);
    if (ret) {
        printk("init write reg array err(%d)\n", ret);
        return ret;
    }

    ret = tx_isp_call_subdev_notify(
        sd, TX_ISP_EVENT_SYNC_SENSOR_ATTR, &sensor->video);
    sensor->priv = wsize;

    return 0;
}

static int ov7740_reset(struct tx_isp_subdev *sd, int val)
{
    return 0;
}

static int ov7740_detect(struct tx_isp_subdev *sd, unsigned int *ident)
{
    unsigned char val;
    int ret;

    ret = ov7740_read(sd, 0x0A, &val);

    if (ret < 0)
        return ret;
    if (val != OV7740_CHIP_ID_H)
        return -ENODEV;

    *ident = val;

    ret = ov7740_read(sd, 0x0B, &val);

    if (ret < 0)
        return ret;
    if (val != OV7740_CHIP_ID_L)
        return -ENODEV;

    *ident = (*ident << 8) | val;

    return 0;
}

static int ov7740_g_chip_ident(struct tx_isp_subdev *sd,
                   struct tx_isp_chip_ident *chip)
{
    struct i2c_client *client = tx_isp_get_subdevdata(sd);
    unsigned int ident = 0;
    int ret = ISP_SUCCESS;

    if (power_gpio != -1) {
        ret = gpio_request(power_gpio, "ov7740_pwdn");
        if (!ret) {
            gpio_direction_output(power_gpio, 1);
            msleep(150);

        } else {
            printk("gpio requrest fail %d\n", power_gpio);
            return -1;
        }
    }

    if (reset_gpio != -1) {
        ret = gpio_request(reset_gpio, "ov7740_reset");
        if (!ret) {
            gpio_direction_output(reset_gpio, 1);
            msleep(20);
            gpio_direction_output(reset_gpio, 0);
            msleep(20);
            gpio_direction_output(reset_gpio, 1);
            msleep(1);
        } else {
            printk("gpio requrest fail %d\n", reset_gpio);
            return -1;
        }
    }

    if (i2c_sel1_gpio != -1) {
        ret = gpio_request(i2c_sel1_gpio, "ov7740_i2c_sel1");
        if (!ret)
            private_gpio_direction_output(i2c_sel1_gpio, 1);
        else
            printk("gpio requrest fail %d\n", i2c_sel1_gpio);
    }

    ret = ov7740_detect(sd, &ident);
    if (ret) {
        printk("chip found @ 0x%x (%s) is not an ov7740 chip.\n",
            client->addr,
            client->adapter->name);
    }

    printk("ov7740 chip found @ 0x%02x (%s)\n",
         client->addr,
         client->adapter->name);

    if (chip) {
        memcpy(chip->name, "ov7740", sizeof("ov7740"));
        chip->ident = ident;
        chip->revision = SENSOR_VERSION;
    }

    return 0;
}

static struct tx_isp_subdev_core_ops ov7740_core_ops = {
    .g_chip_ident = ov7740_g_chip_ident,
    .reset = ov7740_reset,
    .init = ov7740_init,
    .g_register = ov7740_g_register,
    .s_register = ov7740_s_register,
};

static struct tx_isp_subdev_video_ops ov7740_video_ops = {
    .s_stream = ov7740_s_stream,
};

static struct tx_isp_subdev_sensor_ops ov7740_sensor_ops = {
    .ioctl = ov7740_sensor_ops_ioctl,
};

static struct tx_isp_subdev_ops ov7740_ops = {
    .core = &ov7740_core_ops,
    .video = &ov7740_video_ops,
    .sensor = &ov7740_sensor_ops,
};

static u64 tx_isp_module_dma_mask = ~(u64)0;

struct platform_device ov7740_platform_device = {
    .name = "ov7740",
    .id = -1,
    .dev =
        {
            .dma_mask = &tx_isp_module_dma_mask,
            .coherent_dma_mask = 0xffffffff,
            .platform_data = NULL,
        },
    .num_resources = 0,
};

static int ov7740_probe(struct i2c_client *client,
            const struct i2c_device_id *id)
{
    struct tx_isp_subdev *sd;
    struct tx_isp_video_in *video;
    struct tx_isp_sensor *sensor;
    struct tx_isp_sensor_win_setting *wsize = &ov7740_win_sizes[0];
    int ret = -1;

    sensor = (struct tx_isp_sensor *)kzalloc(sizeof(*sensor), GFP_KERNEL);
    if (!sensor) {
        printk(KERN_ERR "Failed to allocate sensor(ov7740) subdev.\n");
        return -ENOMEM;
    }
    memset(sensor, 0, sizeof(*sensor));

#ifndef MODULE
    ov7740_board_info = get_sensor_board_info(ov7740_platform_device.name);
    if (ov7740_board_info) {
        power_gpio = ov7740_board_info->gpios.gpio_power;
        reset_gpio = ov7740_board_info->gpios.gpio_sensor_rst;

        i2c_sel1_gpio = ov7740_board_info->gpios.gpio_i2c_sel1;
        dvp_gpio_func = ov7740_board_info->dvp_gpio_func;
    }
#endif

    /* request mclk of sensor */
    sensor->mclk = clk_get(NULL, "cgu_cim");
    if (IS_ERR(sensor->mclk)) {
        printk(KERN_ERR "Cannot get sensor ov4770 input clock cgu_cim\n");
        goto err_get_mclk;
    }
    private_clk_set_rate(sensor->mclk, 24000000);
    private_clk_enable(sensor->mclk);

    ret = set_sensor_gpio_function(dvp_gpio_func);
    if (ret < 0) {
        printk(KERN_ERR "Cannot get sensor ov4770 gpio function\n");
        goto err_set_sensor_gpio;
    }
    ov7740_attr.dvp.gpio = dvp_gpio_func;

    /*
        convert sensor-gain into isp-gain,
     */
    ov7740_attr.max_again = log2_fixed_to_fixed(ov7740_attr.max_again,
                            TX_ISP_GAIN_FIXED_POINT,
                            LOG2_GAIN_SHIFT);
    ov7740_attr.max_dgain = ov7740_attr.max_dgain;

    sd = &sensor->sd;
    video = &sensor->video;
    sensor->video.attr = &ov7740_attr;
    sensor->video.mbus_change = 0;
    sensor->video.vi_max_width = wsize->width;
    sensor->video.vi_max_height = wsize->height;
    sensor->video.mbus.width = wsize->width;
    sensor->video.mbus.height = wsize->height;
    sensor->video.mbus.code = wsize->mbus_code;
    sensor->video.mbus.field = V4L2_FIELD_NONE;
    sensor->video.mbus.colorspace = wsize->colorspace;
    sensor->video.fps = wsize->fps;
    sensor->video.dc_param = NULL;

    tx_isp_subdev_init(&ov7740_platform_device, sd, &ov7740_ops);
    tx_isp_set_subdevdata(sd, client);
    tx_isp_set_subdev_hostdata(sd, sensor);
    private_i2c_set_clientdata(client, sd);

    return 0;
err_set_sensor_gpio:
    clk_disable(sensor->mclk);
    clk_put(sensor->mclk);
err_get_mclk:
    kfree(sensor);

    return -1;
}

static int ov7740_remove(struct i2c_client *client)
{
    struct tx_isp_subdev *sd = private_i2c_get_clientdata(client);
    struct tx_isp_sensor *sensor = tx_isp_get_subdev_hostdata(sd);

    if (power_gpio != -1) {
        private_gpio_direction_output(power_gpio, 0);
        private_gpio_free(power_gpio);
    }

    if (reset_gpio != -1)
        private_gpio_free(reset_gpio);

    if (i2c_sel1_gpio != -1)
        private_gpio_free(i2c_sel1_gpio);

    private_clk_disable(sensor->mclk);
    private_clk_put(sensor->mclk);
    tx_isp_subdev_deinit(sd);
    kfree(sensor);

    printk("remove ov7740 driver\n");

    return 0;
}

static const struct i2c_device_id ov7740_id[] = {
    { "ov7740", 0 },
    {}
};
MODULE_DEVICE_TABLE(i2c, ov7740_id);

static struct i2c_driver ov7740_driver = {
    .driver =
        {
            .owner = THIS_MODULE,
            .name = "ov7740",
        },
    .probe = ov7740_probe,
    .remove = ov7740_remove,
    .id_table = ov7740_id,
};

static __init int init_ov7740(void)
{
    int ret = 0;
    ret = private_driver_get_interface();
    if (ret) {
        printk(KERN_ERR "Failed to init ov7740 dirver.\n");
        return -1;
    }

    return private_i2c_add_driver(&ov7740_driver);
}

static __exit void exit_ov7740(void)
{
    i2c_del_driver(&ov7740_driver);
}

module_init(init_ov7740);
module_exit(exit_ov7740);

MODULE_DESCRIPTION("A low-level driver for OmniVision ov7740 sensors");
MODULE_LICENSE("GPL");
