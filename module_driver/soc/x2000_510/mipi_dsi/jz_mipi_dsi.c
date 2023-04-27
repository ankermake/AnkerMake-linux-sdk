#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/delay.h>
#include <linux/gpio.h>
#include <linux/clk.h>
#include <linux/slab.h>
#include <linux/irq.h>
#include <linux/interrupt.h>
#include <linux/dma-mapping.h>
#include <linux/platform_device.h>
#include <linux/mutex.h>
#include <linux/wait.h>
#include <linux/spinlock.h>
#include <linux/list.h>
#include <linux/string.h>
#include <linux/delay.h>
#include <linux/vmalloc.h>
#include <linux/sched.h>
#include <linux/ioctl.h>
#include <soc/base.h>

#include "common.h"
#include "jz_mipi_dsi_hal.c"

static struct {
    struct video_config video_config;
    struct dsi_config dsi_config;
    struct clk *clk_dsi;
    struct mutex mutex;
    int dsi_en;
}jz_dsi;

static inline int mipi_bpp_per_pixel(struct lcdc_data *data)
{
    switch (data->mipi.color_coding) {
    case COLOR_CODE_16BIT_CONFIG1:
    case COLOR_CODE_16BIT_CONFIG2:
    case COLOR_CODE_16BIT_CONFIG3:
        return 16;

    case COLOR_CODE_18BIT_CONFIG1:
    case COLOR_CODE_18BIT_CONFIG2:
        return 18;

    case COLOR_CODE_24BIT:
        return 24;
    default:
        break;
    }

    printk(KERN_ERR "no support this color coding\n");
    return 0;
}

int dsi_write_cmd(struct dsi_cmd_packet *cmd_data)
{
    unsigned int packet_type;
    unsigned short word_count = 0;
    unsigned int ret;
    /*word count*/
    packet_type = cmd_data->packet_type;
    word_count = ((cmd_data->cmd1_or_wc_msb << 8 ) | cmd_data->cmd0_or_wc_lsb);

    if (packet_type == 0x39) {
        ret = dsi_write_long_packet(jz_dsi.video_config.virtual_channel, packet_type, cmd_data->cmd_data, word_count);
        if (ret < 0)
            return -1;
    }

    if (packet_type == 0x05 || packet_type == 0x15) {
        ret = dsi_write_short_packet(jz_dsi.video_config.virtual_channel, packet_type, word_count);
        if (ret < 0)
            return -1;
    }

    mdelay(1);
    return 0;
}EXPORT_SYMBOL(dsi_write_cmd);

int dsi_read_cmd(struct dsi_cmd_packet *cmd_data, int bytes, unsigned char *rd_buf)
{
    unsigned int packet_type;
    unsigned short short_data = 0;
    unsigned int ret;

    dsi_set_bit(R_DSI_HOST_GEN_VCID, GEN_VICD_RX, 0);

    packet_type = cmd_data->packet_type;
    short_data = ((cmd_data->cmd1_or_wc_msb << 8 ) | cmd_data->cmd0_or_wc_lsb);

    if (packet_type != 0x04 && packet_type != 0x14 && packet_type != 0x24 && packet_type != 0x06) {
        printk(KERN_ERR "dsi_read :not suport this packet_type = %x\n", packet_type);
        return -1;
    }

    ret = dsi_write_short_packet(jz_dsi.video_config.virtual_channel, packet_type, short_data);
    if (ret < 0)
        return -1;

    ret = dsi_read_packet(bytes, rd_buf);
    if (ret < 0)
        return -1;

    return 0;
}EXPORT_SYMBOL(dsi_read_cmd);

static void calculate_video_mode_chunck(void)
{
    assert(jz_dsi.video_config.video_mode == VIDEO_BURST_WITH_SYNC_PULSES);

    //burst mode
    if (jz_dsi.video_config.video_mode == VIDEO_BURST_WITH_SYNC_PULSES) {
        jz_dsi.video_config.null_size = 0;
        jz_dsi.video_config.chunk = 0;
        jz_dsi.video_config.video_size = jz_dsi.video_config.h_active_pixels;

        if((jz_dsi.dsi_config.color_coding == COLOR_CODE_18BIT_CONFIG1 ||
            jz_dsi.dsi_config.color_coding == COLOR_CODE_18BIT_CONFIG2) &&
            jz_dsi.dsi_config.color_type_18bit != LOOSELY18) {

            jz_dsi.video_config.video_size = ALIGN(jz_dsi.video_config.video_size, 4);
        }

        return;
    }

    //not burst mode

}

#define mipi_error_if(_cond) \
    do { \
        if (_cond) { \
            printk(KERN_ERR "jz_mipi_dsi: failed to check: %s\n", #_cond); \
            ret = -1; \
            return ret; \
        } \
    } while (0)

static int jz_mipi_dsi_data_init(struct lcdc_data *lcd_data)
{
    int ret;

    mipi_error_if(lcd_data == NULL);
    mipi_error_if(lcd_data->mipi.num_of_lanes > 2);
    mipi_error_if(lcd_data->mipi.num_of_lanes < 1);
    mipi_error_if(lcd_data->mipi.video_mode > VIDEO_BURST_WITH_SYNC_PULSES);
    mipi_error_if(lcd_data->mipi.color_coding > COLOR_CODE_24BIT);

    int hs = lcd_data->hsync_len;
    int hbp = lcd_data->left_margin;
    int hfp = lcd_data->right_margin;
    int vs = lcd_data->vsync_len;
    int vbp = lcd_data->upper_margin;
    int vfp = lcd_data->lower_margin;

    int v_total_lines;

    jz_dsi.dsi_config.color_coding = lcd_data->mipi.color_coding;
    jz_dsi.dsi_config.num_of_lanes = lcd_data->mipi.num_of_lanes;
    jz_dsi.dsi_config.color_mode_polarity = lcd_data->mipi.color_mode_polarity;
    jz_dsi.dsi_config.shut_down_polarity = lcd_data->mipi.shut_down_polarity;
    jz_dsi.dsi_config.max_bta_cycles = lcd_data->mipi.max_bta_cycles;
    jz_dsi.dsi_config.max_hs_to_lp_cycles = lcd_data->mipi.max_hs_to_lp_cycles;
    jz_dsi.dsi_config.max_lp_to_hs_cycles = lcd_data->mipi.max_lp_to_hs_cycles;
    jz_dsi.dsi_config.color_type_18bit = lcd_data->mipi.color_type_18bit;

    jz_dsi.video_config.virtual_channel = lcd_data->mipi.virtual_channel;
    jz_dsi.video_config.video_mode = lcd_data->mipi.video_mode;
    jz_dsi.video_config.data_en_polarity = lcd_data->mipi.data_en_polarity;

    jz_dsi.video_config.h_active_pixels = lcd_data->xres;
    jz_dsi.video_config.hs = hs;
    jz_dsi.video_config.hbp = hbp;
    jz_dsi.video_config.h_total_pixels = hs + hbp + lcd_data->xres + hfp;

    jz_dsi.video_config.vs = vs;
    jz_dsi.video_config.vbp = vbp;
    jz_dsi.video_config.v_active_lines = lcd_data->yres;
    jz_dsi.video_config.vfp = vfp;

    v_total_lines = vs + vbp + lcd_data->yres + vfp;

    jz_dsi.video_config.h_polarity = lcd_data->mipi.hsync_active_level;
    jz_dsi.video_config.v_polarity = lcd_data->mipi.vsync_active_level;
    jz_dsi.video_config.pixel_clock = lcd_data->pixclock / 1000;
    jz_dsi.video_config.bpp_info = mipi_bpp_per_pixel(lcd_data);


    jz_dsi.video_config.byte_clock = (jz_dsi.video_config.h_total_pixels * v_total_lines / 1000)
                                     * lcd_data->refresh * (jz_dsi.video_config.bpp_info / 8)
                                     * 6 / 5 / jz_dsi.dsi_config.num_of_lanes;


    if (lcd_data->mipi.slcd_te_pin_mode != TE_NOT_EANBLE)
        jz_dsi.dsi_config.te_mipi_en = 1;

    calculate_video_mode_chunck();

    return 0;
}

static void jz_dsi_command_cfg(void)
{
    dsi_set_edpi_cmd_size(1024);
    dsi_set_bit(R_DSI_HOST_LPCLK_CTRL, PHY_TXREQUESTCLKHS, 1);
    dsi_set_bit(R_DSI_HOST_LPCLK_CTRL, AUTO_CLKLANE_CTRL, 1);

    // high speed
    dsi_set_transfer_mode(0);

    dsi_set_bit(R_DSI_HOST_VID_MODE_CFG, FRAME_BTA_ACK_EN, 0);
    dsi_set_bit(R_DSI_HOST_PCKHDL_CFG, BAT_EN, 0);

    if (jz_dsi.dsi_config.te_mipi_en)
        dsi_set_bit(R_DSI_HOST_CMD_MODE_CFG, TEAR_FX_EN, 1);
    else
        dsi_set_bit(R_DSI_HOST_CMD_MODE_CFG, TEAR_FX_EN, 0);

    dsi_set_cmd_mode();
}

static void jz_dsi_video_cfg(void)
{
    dsi_set_power(0);
    jz_dsi_video_init(&jz_dsi.video_config);
    dsi_set_power(1);
}

static void jz_enable_mipi_dsi(void)
{
    clk_prepare_enable(jz_dsi.clk_dsi);
    jz_dsi_dphy_init(&jz_dsi.dsi_config);

    //LP mode
    dsi_set_transfer_mode(1);
    dsi_set_cmd_mode();
    dsi_set_edpi_cmd_size(0x6);

    jz_dsi_set_clock(jz_dsi.video_config.byte_clock);

    dsi_set_power(0);
    dsi_set_power(1);

    jz_dsi_gen_init(&jz_dsi.dsi_config);

    dsi_set_power(0);
    dsi_set_power(1);

}

static void jz_disable_mipi_dsi(void)
{
    dsi_set_bit(R_DSI_HOST_PHY_RSTZ, PHY_ENABLECLK, 0);
    dsi_set_bit(R_DSI_HOST_PHY_RSTZ, PHY_SHUTDOWMZ, 0);
    dsi_set_power(0);
    clk_disable_unprepare(jz_dsi.clk_dsi);
}

static void jz_mipi_dsi_init(void)
{
    jz_dsi.clk_dsi = clk_get(NULL, "gate_dsi");
    assert(!IS_ERR(jz_dsi.clk_dsi));
}

static void jz_mipi_dsi_exit(void)
{
    clk_put(jz_dsi.clk_dsi);
}