#include <linux/init.h>
#include <linux/platform_device.h>
#include <linux/gpio.h>
#include <linux/pwm_backlight.h>
#include <linux/digital_pulse_backlight.h>
#include <linux/delay.h>
#include <linux/lcd.h>

#include <mach/jzfb.h>
#include <soc/gpio.h>
#include <board.h>
#define GPIO_LCD_CS         -1//GPIO_PD(18)
#define GPIO_LCD_RD         -1//GPIO_PD(26)
#define GPIO_LCD_RST        -1//GPIO_PD(19)
#define GPIO_LCD_POWER_EN   -1//GPIO_PB(2)
#define GPIO_BL_PWR_EN      -1//没有背光电源控制

static struct smart_lcd_data_table ili9488_data_table[] = {
    {SMART_CONFIG_CMD   ,0xE0},
    {SMART_CONFIG_PRM   ,0x00},
    {SMART_CONFIG_PRM   ,0x07},
    {SMART_CONFIG_PRM   ,0x0f},
    {SMART_CONFIG_PRM   ,0x0D},
    {SMART_CONFIG_PRM   ,0x1B},
    {SMART_CONFIG_PRM   ,0x0A},
    {SMART_CONFIG_PRM   ,0x3c},
    {SMART_CONFIG_PRM   ,0x78},
    {SMART_CONFIG_PRM   ,0x4A},
    {SMART_CONFIG_PRM   ,0x07},
    {SMART_CONFIG_PRM   ,0x0E},
    {SMART_CONFIG_PRM   ,0x09},
    {SMART_CONFIG_PRM   ,0x1B},
    {SMART_CONFIG_PRM   ,0x1e},
    {SMART_CONFIG_PRM   ,0x0f},

    {SMART_CONFIG_CMD   ,0xE1},
    {SMART_CONFIG_PRM   ,0x00},
    {SMART_CONFIG_PRM   ,0x22},
    {SMART_CONFIG_PRM   ,0x24},
    {SMART_CONFIG_PRM   ,0x06},
    {SMART_CONFIG_PRM   ,0x12},
    {SMART_CONFIG_PRM   ,0x07},
    {SMART_CONFIG_PRM   ,0x36},
    {SMART_CONFIG_PRM   ,0x47},
    {SMART_CONFIG_PRM   ,0x47},
    {SMART_CONFIG_PRM   ,0x06},
    {SMART_CONFIG_PRM   ,0x0a},
    {SMART_CONFIG_PRM   ,0x07},
    {SMART_CONFIG_PRM   ,0x30},
    {SMART_CONFIG_PRM   ,0x37},
    {SMART_CONFIG_PRM   ,0x0f},

    {SMART_CONFIG_CMD   ,0xC0},
    {SMART_CONFIG_PRM   ,0x10},
    {SMART_CONFIG_PRM   ,0x10},

    {SMART_CONFIG_CMD   ,0xC1},
    {SMART_CONFIG_PRM   ,0x41},

    {SMART_CONFIG_CMD   ,0xC5},
    {SMART_CONFIG_PRM   ,0x00},
    {SMART_CONFIG_PRM   ,0x22},
    {SMART_CONFIG_PRM   ,0x80},

    {SMART_CONFIG_CMD   ,0x36},
    {SMART_CONFIG_PRM   ,0x28},

    {SMART_CONFIG_CMD   ,0x3A},//Interface Mode Control
    {SMART_CONFIG_PRM   ,0x66},


    {SMART_CONFIG_CMD   ,0XB0}, //Interface Mode Control
    {SMART_CONFIG_PRM   ,0x00},
    {SMART_CONFIG_CMD   ,0xB1},  //Frame rate 70HZ
    {SMART_CONFIG_PRM   ,0xB0},
    {SMART_CONFIG_PRM   ,0x11},
    {SMART_CONFIG_CMD   ,0xB4},
    {SMART_CONFIG_PRM   ,0x02},
    {SMART_CONFIG_CMD   ,0xB6},//RGB/MCU Interface Control
    {SMART_CONFIG_PRM   ,0x02},
    {SMART_CONFIG_PRM   ,0x02},

    {SMART_CONFIG_CMD   ,0xB7},
    {SMART_CONFIG_PRM   ,0xC6},

    {SMART_CONFIG_CMD   ,0XBE},
    {SMART_CONFIG_PRM   ,0x00},
    {SMART_CONFIG_PRM   ,0x04},

    {SMART_CONFIG_CMD   ,0xE9},
    {SMART_CONFIG_PRM   ,0x00},

    {SMART_CONFIG_CMD   ,0XF7},
    {SMART_CONFIG_PRM   ,0xA9},
    {SMART_CONFIG_PRM   ,0x51},
    {SMART_CONFIG_PRM   ,0x2C},
    {SMART_CONFIG_PRM   ,0x82},

    //-----display window 320*480---------//
    {SMART_CONFIG_CMD   ,0x2B}, //Display function control     480
    //{SMART_CONFIG_CMD    , 0x2A},
    {SMART_CONFIG_PRM   ,0x00},
    {SMART_CONFIG_PRM   ,0x00},
    {SMART_CONFIG_PRM   ,0x01},
    {SMART_CONFIG_PRM   ,0x3f},

    {SMART_CONFIG_CMD   ,0x2A}, //Frame rate control    320
   // {SMART_CONFIG_CMD    , 0x2B},
    {SMART_CONFIG_PRM   ,0x00},
    {SMART_CONFIG_PRM   ,0x00},
    {SMART_CONFIG_PRM   ,0x01},
    {SMART_CONFIG_PRM   ,0xdf},
    //--------end display window --------------//
    {SMART_CONFIG_CMD    ,0x11},
    {SMART_CONFIG_UDELAY,120000},
    {SMART_CONFIG_CMD    ,0x29},

    {SMART_CONFIG_CMD    ,0x2C},


};

static struct fb_videomode jzfb_ili9488_videomode = {
    .name = "480x320",
    .xres = 480,
    .yres = 320,
    .pixclock = KHZ2PICOS(30000),
};

static struct jzfb_smart_config ili9488_cfg = {
    .frm_md = 0,        //帧刷新模式。0:单帧模式（默认）1： 连续帧模式
    .rdy_switch = 0,    //0:不要等待RDY，立即发送命令或数据；（默认）1： 等待RDY，然后发送命令或数据。
    .rdy_dp = 0,        //RDY的默认极性 0：默认（无效）级别低；1： 默认（无效）级别为高；（默认）
    .rdy_anti_jit = 0,  //RDY的抗抖动。 0:具有1个pixclk周期的示例RDY（默认）1： 具有3个pixclk周期的用于抗抖动的样品RDY。
#ifndef CONFIG_LCD_V14_SLCD_TE
    .te_switch = 0, //0:不等待TE，SLCD启动后发送pix U数据；1： 等待TE，然后发送pix_数据。（默认值)
    .te_dp = 0,     //TE的默认极性 0:默认（无效）级别低；（默认）1： 默认（无效）级别高；
    .te_md = 0,     //TE的活动边。 0：前边缘（默认）1： 后缘
    .te_anti_jit = 0,       //TE的抗抖动。0:1个pixclk周期的样品TE；1： 使用3个pixclk周期采样TE以防抖动（默认）。
#else
    .te_switch = 1,
    .te_dp = 0,
    .te_md = 0,
    .te_anti_jit = 0,
#endif
    .cs_en = 0,    // CS控制启用。0:CS引脚由GPIO控制。（默认）1： CS引脚由显示控制器控制。
    .cs_dp = 0,     //CS的默认极性 0：默认（无效）级别低；1： 默认（无效）级别为高；（默认）
    .dc_md = 0,     //DC的定义。0：命令DC=低，数据DC=高（默认）1： 命令DC=高，数据DC=低
    .wr_md = 1,     //WR的定义。0:posedge驱动（驱动级别高）负边缘取样（取样水平低）；一：negedge驱动器（驱动器级别低）
    //posedge上的样本（样本级别很高）。（默认值）注：C型时，WR引脚为SPI_CLK。
    .smart_type = SMART_LCD_TYPE_8080, //lcd 接口型号
    .pix_fmt = SMART_LCD_FORMAT_888,    //颜色格式   该屏幕使用的是565 16bpp；
    .dwidth = SMART_LCD_DWIDTH_8_BIT,
    .cwidth = SMART_LCD_CWIDTH_8_BIT,

    .write_gram_cmd = 0x2c,
    .data_table = ili9488_data_table,
    .length_data_table = ARRAY_SIZE(ili9488_data_table),
    .fb_copy_type =     FB_COPY_TYPE_ROTATE_270,
};

static unsigned int lcd_power_inited;

static int ili9488_gpio_init(void)
{
    int ret;

    ret = gpio_request(GPIO_LCD_RST, "lcd rst");
    if (ret) {
        printk(KERN_ERR "can's request lcd rst gpio\n");
        return ret;
    }

    ret = gpio_request(GPIO_LCD_CS, "lcd cs");
    if (ret) {
        printk(KERN_ERR "can's request lcd cs gpio\n");
        return ret;
    }

    ret = gpio_request(GPIO_LCD_RD, "lcd rd");
    if (ret) {
        printk(KERN_ERR "can's request lcd rd gpio\n");
        return ret;
    }

    ret = gpio_request(GPIO_LCD_POWER_EN, "lcd power en");
    if (ret) {
        printk(KERN_ERR "can's request lcd power gpio\n");
        return ret;
    }

    lcd_power_inited = 1;

    return 0;
}

static int ili9488_power_on(void *data)
{
    printk("ili9488_power_on\n");
    if (!lcd_power_inited && ili9488_gpio_init())
        return -EFAULT;

    gpio_direction_output(GPIO_LCD_POWER_EN, 1);
    gpio_direction_output(GPIO_LCD_CS, 1);
    // gpio_direction_output(GPIO_LCD_RD, 1);
    gpio_direction_output(GPIO_LCD_RST, 0);
    mdelay(20);
    gpio_direction_output(GPIO_LCD_RST, 1);
    mdelay(120);
    gpio_direction_output(GPIO_LCD_CS, 0);

    return 0;
}

static int ili9488_power_off(void *data)
{
    if (!lcd_power_inited && ili9488_gpio_init())
        return -EFAULT;

    gpio_direction_output(GPIO_LCD_CS, 1);
    // gpio_direction_output(GPIO_LCD_RD, 1);
    gpio_direction_output(GPIO_LCD_RST, 0);
    gpio_direction_output(GPIO_LCD_POWER_EN, 0);

    return 0;
}

static struct lcd_callback_ops lcd_callback = {
    .lcd_power_on_begin = ili9488_power_on,
    .lcd_power_off_begin = ili9488_power_off,
};

struct jzfb_platform_data jzfb_pdata = {
    .num_modes = 1,
    .modes = &jzfb_ili9488_videomode,
    .lcd_type = LCD_TYPE_SLCD,
    .width = 480,
    .height = 320,

    .smart_config = &ili9488_cfg,
    .lcd_callback_ops = &lcd_callback,
};

/****************power and  backlight*********************/

static struct platform_pwm_backlight_data backlight_data = {
    .pwm_id = 3,
    .pwm_active_level = 1,
    .max_brightness = 255,
    .dft_brightness = 120,
    .pwm_period_ns = 30000,
};

struct platform_device backlight_device = {
    .name = "pwm-backlight",
    .dev = {
        .platform_data = &backlight_data,
    },
};

static int __init jzfb_backlight_init(void)
{
    platform_device_register(&backlight_device);
    return 0;
}

static void __exit jzfb_backlight_exit(void)
{
    platform_device_unregister(&backlight_device);
}

module_init(jzfb_backlight_init);
module_exit(jzfb_backlight_exit)
