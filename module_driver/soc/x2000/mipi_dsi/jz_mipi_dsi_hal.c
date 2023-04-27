#include <common.h>
#include <soc/base.h>
#include <linux/io.h>
#include <linux/delay.h>
#include <utils/clock.h>

#include <bit_field.h>

#include "jz_mipi_dsi_reg.h"
#include "jz_mipi_dsi.h"

#define MIPI_DSI_IOBASE     0x10075000
#define MIPI_DSI_PHY_IOBASE 0x10077000
#define MIPI_DSI_ADDR(reg)  ((volatile unsigned long *)CKSEG1ADDR(MIPI_DSI_IOBASE + reg))
#define MIPI_DSI_PHY_ADDR(reg) ((volatile unsigned long *)CKSEG1ADDR(MIPI_DSI_PHY_IOBASE + reg))

static void dsi_write_phy_clk(unsigned int val)
{
    *MIPI_DSI_PHY_ADDR(0x64) = val;
}

static inline unsigned int dsi_read_phy_clk(void)
{
    return *MIPI_DSI_PHY_ADDR(0x64);
}

static void dsi_write(unsigned int reg, unsigned int val)
{
    *MIPI_DSI_ADDR(reg) = val;
}

static inline unsigned int dsi_read(unsigned int reg)
{
    return *MIPI_DSI_ADDR(reg);
}

static inline void dsi_set_bit(unsigned int reg, int start, int end, unsigned int val)
{
    set_bit_field(MIPI_DSI_ADDR(reg), start, end, val);
}

static inline unsigned int dsi_get_bit(unsigned int reg, int start, int end)
{
    return get_bit_field(MIPI_DSI_ADDR(reg), start, end);
}

static void dsi_set_power(unsigned int power)
{
    dsi_set_bit(R_DSI_HOST_PWR_UP, SHUTDOWNZ, power);
}

static int dsi_set_dphy_hs2lp_time(unsigned char time)
{
    dsi_set_bit(R_DSI_HOST_PHY_TMR_CFG, PHY_HS2LP_TIME, time);

    return 0;
}

static int dsi_set_dphy_lp2hs_time(unsigned char time)
{
    dsi_set_bit(R_DSI_HOST_PHY_TMR_CFG, PHY_LP2HS_TIME, time);

    return 0;

}

void dump_dsi_reg(void)
{
    printk("===========>dump dsi reg\n");
    printk("VERSION------------:%08x\n",
           dsi_read(R_DSI_HOST_VERSION));
    printk("PWR_UP:------------:%08x\n",
           dsi_read(R_DSI_HOST_PWR_UP));
    printk("CLKMGR_CFG---------:%08x\n",
           dsi_read(R_DSI_HOST_CLKMGR_CFG));
    printk("DPI_VCID-----------:%08x\n",
           dsi_read(R_DSI_HOST_DPI_VCID));
    printk("DPI_COLOR_CODING---:%08x\n",
           dsi_read(R_DSI_HOST_DPI_COLOR_CODING));
    printk("DPI_CFG_POL--------:%08x\n",
           dsi_read(R_DSI_HOST_DPI_CFG_POL));
    printk("DPI_LP_CMD_TIM-----:%08x\n",
           dsi_read(R_DSI_HOST_DPI_LP_CMD_TIM));
    printk("DBI_VCID-----------:%08x\n",
           dsi_read(R_DSI_HOST_DBI_VCID));
    printk("DBI_CFG------------:%08x\n",
           dsi_read(R_DSI_HOST_DBI_CFG));
    printk("DBI_PARTITIONING_EN:%08x\n",
           dsi_read(R_DSI_HOST_DBI_PARTITIONING_EN));
    printk("DBI_CMDSIZE--------:%08x\n",
           dsi_read(R_DSI_HOST_DBI_CMDSIZE));
    printk("PCKHDL_CFG---------:%08x\n",
           dsi_read(R_DSI_HOST_PCKHDL_CFG));
    printk("GEN_VCID-----------:%08x\n",
           dsi_read(R_DSI_HOST_GEN_VCID));
    printk("MODE_CFG-----------:%08x\n",
           dsi_read(R_DSI_HOST_MODE_CFG));
    printk("VID_MODE_CFG-------:%08x\n",
           dsi_read(R_DSI_HOST_VID_MODE_CFG));
    printk("VID_PKT_SIZE-------:%08x\n",
           dsi_read(R_DSI_HOST_VID_PKT_SIZE));
    printk("VID_NUM_CHUNKS-----:%08x\n",
           dsi_read(R_DSI_HOST_VID_NUM_CHUNKS));
    printk("VID_NULL_SIZE------:%08x\n",
           dsi_read(R_DSI_HOST_VID_NULL_SIZE));
    printk("VID_HSA_TIME-------:%08x\n",
           dsi_read(R_DSI_HOST_VID_HSA_TIME));
    printk("VID_HBP_TIME-------:%08x\n",
           dsi_read(R_DSI_HOST_VID_HBP_TIME));
    printk("VID_HLINE_TIME-----:%08x\n",
           dsi_read(R_DSI_HOST_VID_HLINE_TIME));
    printk("VID_VSA_LINES------:%08x\n",
           dsi_read(R_DSI_HOST_VID_VSA_LINES));
    printk("VID_VBP_LINES------:%08x\n",
           dsi_read(R_DSI_HOST_VID_VBP_LINES));
    printk("VID_VFP_LINES------:%08x\n",
           dsi_read(R_DSI_HOST_VID_VFP_LINES));
    printk("VID_VACTIVE_LINES--:%08x\n",
        dsi_read(R_DSI_HOST_VID_VACTIVE_LINES));
    printk("EDPI_CMD_SIZE------:%08x\n",
           dsi_read(R_DSI_HOST_EDPI_CMD_SIZE));
    printk("CMD_MODE_CFG-------:%08x\n",
           dsi_read(R_DSI_HOST_CMD_MODE_CFG));
    printk("GEN_HDR------------:%08x\n",
           dsi_read(R_DSI_HOST_GEN_HDR));
    printk("GEN_PLD_DATA-------:%08x\n",
           dsi_read(R_DSI_HOST_GEN_PLD_DATA));
    printk("CMD_PKT_STATUS-----:%08x\n",
           dsi_read(R_DSI_HOST_CMD_PKT_STATUS));
    printk("TO_CNT_CFG---------:%08x\n",
           dsi_read(R_DSI_HOST_TO_CNT_CFG));
    printk("HS_RD_TO_CNT-------:%08x\n",
           dsi_read(R_DSI_HOST_HS_RD_TO_CNT));
    printk("LP_RD_TO_CNT-------:%08x\n",
           dsi_read(R_DSI_HOST_LP_RD_TO_CNT));
    printk("HS_WR_TO_CNT-------:%08x\n",
           dsi_read(R_DSI_HOST_HS_WR_TO_CNT));
    printk("LP_WR_TO_CNT_CFG---:%08x\n",
           dsi_read(R_DSI_HOST_LP_WR_TO_CNT));
    printk("BTA_TO_CNT---------:%08x\n",
           dsi_read(R_DSI_HOST_BTA_TO_CNT));
    printk("SDF_3D-------------:%08x\n",
           dsi_read(R_DSI_HOST_SDF_3D));
    printk("LPCLK_CTRL---------:%08x\n",
           dsi_read(R_DSI_HOST_LPCLK_CTRL));
    printk("PHY_TMR_LPCLK_CFG--:%08x\n",
           dsi_read(R_DSI_HOST_PHY_TMR_LPCLK_CFG));
    printk("PHY_TMR_CFG--------:%08x\n",
           dsi_read(R_DSI_HOST_PHY_TMR_CFG));
    printk("PHY_RSTZ-----------:%08x\n",
           dsi_read(R_DSI_HOST_PHY_RSTZ));
    printk("PHY_IF_CFG---------:%08x\n",
           dsi_read(R_DSI_HOST_PHY_IF_CFG));
    printk("PHY_ULPS_CTRL------:%08x\n",
           dsi_read(R_DSI_HOST_PHY_ULPS_CTRL));
    printk("PHY_TX_TRIGGERS----:%08x\n",
           dsi_read(R_DSI_HOST_PHY_TX_TRIGGERS));
    printk("PHY_STATUS---------:%08x\n",
           dsi_read(R_DSI_HOST_PHY_STATUS));
    printk("PHY_TST_CTRL0------:%08x\n",
           dsi_read(R_DSI_HOST_PHY_TST_CTRL0));
    printk("PHY_TST_CTRL1------:%08x\n",
           dsi_read(R_DSI_HOST_PHY_TST_CTRL1));
    printk("INT_ST0------------:%08x\n",
           dsi_read(R_DSI_HOST_INT_ST0));
    printk("INT_ST1------------:%08x\n",
           dsi_read(R_DSI_HOST_INT_ST1));
    printk("INT_MSK0-----------:%08x\n",
           dsi_read(R_DSI_HOST_INT_MSK0));
    printk("INT_MSK1-----------:%08x\n",
           dsi_read(R_DSI_HOST_INT_MSK1));
    printk("INT_FORCE0---------:%08x\n",
           dsi_read(R_DSI_HOST_INT_FORCE0));
    printk("INT_FORCE1---------:%08x\n",
           dsi_read(R_DSI_HOST_INT_FORCE1));
    printk("VID_SHADOW_CTRL----:%08x\n",
           dsi_read(R_DSI_HOST_VID_SHADOW_CTRL));
    printk("DPI_VCID_ACT-------:%08x\n",
           dsi_read(R_DSI_HOST_DPI_VCID_ACT));
    printk("DPI_COLOR_CODING_AC:%08x\n",
           dsi_read(R_DSI_HOST_DPI_COLOR_CODING_ACT));
    printk("DPI_LP_CMD_TIM_ACT-:%08x\n",
           dsi_read(R_DSI_HOST_DPI_LP_CMD_TIM_ACT));
    printk("VID_MODE_CFG_ACT---:%08x\n",
           dsi_read(R_DSI_HOST_VID_MODE_CFG_ACT));
    printk("VID_PKT_SIZE_ACT---:%08x\n",
           dsi_read(R_DSI_HOST_VID_PKT_SIZE_ACT));
    printk("VID_NUM_CHUNKS_ACT-:%08x\n",
           dsi_read(R_DSI_HOST_VID_NUM_CHUNKS_ACT));
    printk("VID_HSA_TIME_ACT---:%08x\n",
           dsi_read(R_DSI_HOST_VID_HSA_TIME_ACT));
    printk("VID_HBP_TIME_ACT---:%08x\n",
           dsi_read(R_DSI_HOST_VID_HBP_TIME_ACT));
    printk("VID_HLINE_TIME_ACT-:%08x\n",
           dsi_read(R_DSI_HOST_VID_HLINE_TIME_ACT));
    printk("VID_VSA_LINES_ACT--:%08x\n",
           dsi_read(R_DSI_HOST_VID_VSA_LINES_ACT));
    printk("VID_VBP_LINES_ACT--:%08x\n",
           dsi_read(R_DSI_HOST_VID_VBP_LINES_ACT));
    printk("VID_VFP_LINES_ACT--:%08x\n",
           dsi_read(R_DSI_HOST_VID_VFP_LINES_ACT));
    printk("VID_VACTIVE_LINES_ACT:%08x\n",
           dsi_read(R_DSI_HOST_VID_VACTIVE_LINES_ACT));
    printk("SDF_3D_ACT---------:%08x\n",
           dsi_read(R_DSI_HOST_SDF_3D_ACT));
}

static void dsi_set_dphy_bta_time(unsigned short time)
{
    dsi_set_bit(R_DSI_HOST_PHY_TMR_CFG, MAX_RD_TIME, time);
}

static int dsi_get_cmd_full(void)
{
    return dsi_get_bit(R_DSI_HOST_CMD_PKT_STATUS, GEN_CMD_FULL);
}

static int dsi_get_pld_w_full(void)
{
    return dsi_get_bit(R_DSI_HOST_CMD_PKT_STATUS, GEN_PLD_W_FUL);
}

static int dsi_get_rd_cmd_busy(void)
{
    return dsi_get_bit(R_DSI_HOST_CMD_PKT_STATUS, GEN_RD_CMD_BUSY);
}

static int dsi_get_rd_empty(void)
{
    return dsi_get_bit(R_DSI_HOST_CMD_PKT_STATUS, GEN_PLD_R_EMPTY);
}

static int dsi_wait_pld_w_not_full(int count)
{
    int status;
    status = dsi_get_pld_w_full();

    while (count-- && status) {
        status = dsi_get_pld_w_full();
    }

    return status;
}

static int dsi_wait_cmd_not_full(int count)
{
    int status;
    status = dsi_get_cmd_full();

    while (count-- && status) {
        status = dsi_get_cmd_full();
    }

    return status;
}

static int dsi_wait_rd_cmd_busy(int count)
{
    int status;
    status = dsi_get_rd_cmd_busy();

    while (count-- && status) {
        status = dsi_get_rd_cmd_busy();
    }

    return status;
}

static int dsi_wait_rd_fifo_empty(int count)
{
    int status;
    status = dsi_get_rd_empty();

    while (count-- && status) {
        status = dsi_get_rd_empty();
    }

    return status;
}

static void dsi_set_transfer_mode(int mode)
{
    unsigned long cmd_mode_cfg = dsi_read(R_DSI_HOST_CMD_MODE_CFG);
    set_bit_field(&cmd_mode_cfg, MAX_RD_PKT_SIZE, mode);
    set_bit_field(&cmd_mode_cfg, DCS_SW_0P_TX, mode);
    set_bit_field(&cmd_mode_cfg, DCS_SW_1P_TX, mode);
    set_bit_field(&cmd_mode_cfg, DCS_SR_0P_TX, mode);
    set_bit_field(&cmd_mode_cfg, DCS_LW_TX, mode);

    set_bit_field(&cmd_mode_cfg, GEN_SW_0P_TX, mode);
    set_bit_field(&cmd_mode_cfg, GEN_SW_1P_TX, mode);
    set_bit_field(&cmd_mode_cfg, GEN_SW_2P_TX, mode);
    set_bit_field(&cmd_mode_cfg, GEN_LW_TX, mode);
    set_bit_field(&cmd_mode_cfg, GEN_SR_0P_TX, mode);
    set_bit_field(&cmd_mode_cfg, GEN_SR_1P_TX, mode);
    set_bit_field(&cmd_mode_cfg, GEN_SR_2P_TX, mode);

    dsi_write(R_DSI_HOST_CMD_MODE_CFG, cmd_mode_cfg);
}

static void dsi_set_cmd_mode(void)
{
    dsi_set_bit(R_DSI_HOST_MODE_CFG, CMD_VIDEO_MODE, 1);
}

static void dsi_set_video_mode(void)
{
    dsi_set_bit(R_DSI_HOST_MODE_CFG, CMD_VIDEO_MODE, 0);
}

static void dsi_set_edpi_cmd_size(unsigned short size)
{
    dsi_set_bit(R_DSI_HOST_EDPI_CMD_SIZE, EDPI_ALLOWED_CMD_SIZE, size);
}

struct dphy_pll_range {
    unsigned int start_clk_sel;
    unsigned int output_freq0;	/*start freq in same resolution*/
    unsigned int output_freq1;	/*end freq in same resolution*/
    unsigned int resolution;
};

struct dphy_pll_range dphy_pll_table[] = {
    {0, 63750000, 93125000, 312500},
    {95, 93750000, 186250000, 625000},
    {244, 187500000, 372500000, 1250000},
    {393, 375000000, 745000000, 2500000},
    {542, 750000000, 2750000000UL, 5000000},
};

static int jz_dsih_dphy_configure_x2000(unsigned int output_freq)
{
    int i;
    struct dphy_pll_range *pll;
    unsigned int pll_clk_sel = 0xffffffff;

    for(i = 0; i < ARRAY_SIZE(dphy_pll_table); i++) {
        pll = &dphy_pll_table[i];
        if(output_freq >= pll->output_freq0 && output_freq <= pll->output_freq1) {
            pll_clk_sel = pll->start_clk_sel + (output_freq - pll->output_freq0) / pll->resolution;
            break;
        }
    }
    if(pll_clk_sel == 0xffffffff) {
        printk("can not find appropriate pll freq set for dsi phy! output_freq: %u\n", output_freq);
        return -1;
    }
    dsi_write_phy_clk(pll_clk_sel);
    return 0;
}

static int jz_dsi_set_clock(unsigned int clk)
{
    int ret;
    ret = jz_dsih_dphy_configure_x2000(clk * 1000 * 8);
    if (ret < 0) {
        printk("set dphy clock error!!\n");
        return -1;
    }

    dsi_set_bit(R_DSI_HOST_PHY_IF_CFG, PHY_STOP_WAIT_TIME, 0x1C);

    dsi_set_bit(R_DSI_HOST_PHY_RSTZ, PHY_ENABLECLK, 1);
    dsi_set_bit(R_DSI_HOST_PHY_RSTZ, PHY_SHUTDOWMZ, 1);
    dsi_set_bit(R_DSI_HOST_PHY_RSTZ, PHY_RSTZ, 1);

    //why 7????
    dsi_set_bit(R_DSI_HOST_CLKMGR_CFG, TX_ESC_CLK_DIV, 7);

    return 0;

}

static void jz_dsi_dphy_init(struct dsi_config *dsi_config)
{
    dsi_set_bit(R_DSI_HOST_PHY_RSTZ, PHY_RSTZ, 0);

    dsi_set_bit(R_DSI_HOST_PHY_IF_CFG, PHY_STOP_WAIT_TIME, 0x1c);

    dsi_set_bit(R_DSI_HOST_PHY_IF_CFG, N_LANES, dsi_config->num_of_lanes - 1);

    dsi_set_bit(R_DSI_HOST_PHY_RSTZ, PHY_ENABLECLK, 1);

    dsi_set_bit(R_DSI_HOST_PHY_RSTZ, PHY_SHUTDOWMZ, 1);
    dsi_set_bit(R_DSI_HOST_PHY_RSTZ, PHY_RSTZ, 1);
}

static void jz_dsi_gen_init(struct dsi_config *dsi_config)
{
    dsi_set_bit(R_DSI_HOST_DPI_CFG_POL, COLORM_ACTIVE_LOW, !dsi_config->color_mode_polarity);
    dsi_set_bit(R_DSI_HOST_DPI_CFG_POL, SHUTD_ACTIVE_LOW, !dsi_config->shut_down_polarity);

    dsi_set_dphy_hs2lp_time(dsi_config->max_hs_to_lp_cycles);
    dsi_set_dphy_lp2hs_time(dsi_config->max_lp_to_hs_cycles);
    dsi_set_dphy_bta_time(dsi_config->max_bta_cycles);

    dsi_set_bit(R_DSI_HOST_GEN_VCID, GEN_VICD_RX, 0);

    unsigned long pckhdl_cfg = dsi_read(R_DSI_HOST_PCKHDL_CFG);

    set_bit_field(&pckhdl_cfg, ETOP_RX_EN, 1);
    set_bit_field(&pckhdl_cfg, ETOP_TX_EN, 0);
    set_bit_field(&pckhdl_cfg, BAT_EN, 1);
    set_bit_field(&pckhdl_cfg, ECC_RX_EN, 1);
    set_bit_field(&pckhdl_cfg, CRC_RX_EN, 1);

    dsi_write(R_DSI_HOST_PCKHDL_CFG, pckhdl_cfg);

    dsi_set_bit(R_DSI_HOST_DPI_COLOR_CODING, DIP_COLOR_CODING, dsi_config->color_coding);
    dsi_set_bit(R_DSI_HOST_DPI_COLOR_CODING, LOOSELY18_EN, dsi_config->color_type_18bit);
}

static int dsi_write_gen_data(unsigned int data)
{
    if (dsi_wait_pld_w_not_full(500)) {
        printk("pld_w_fifo full!\n");
        return -1;
    }

    dsi_write(R_DSI_HOST_GEN_PLD_DATA, data);

    return 0;
}

static int dsi_write_short_packet(unsigned char vc, unsigned char packet_type, unsigned short cmd_data)
{
    if (dsi_wait_cmd_not_full(10*1000)) {
        printk("cmd fifo full!\n");
        return -1;
    }

    unsigned char data[2];
    data[0] = cmd_data;
    data[1] = cmd_data >> 8;

    unsigned long gen_hdr = 0;
    set_bit_field(&gen_hdr, GEN_DT, packet_type);
    set_bit_field(&gen_hdr, GEN_VC, vc);
    set_bit_field(&gen_hdr, GEN_WC_LSBYTE, data[0]);
    set_bit_field(&gen_hdr, GEN_WC_MSBYTE, data[1]);
    dsi_write(R_DSI_HOST_GEN_HDR, gen_hdr);

    return 0;

}

static int dsi_write_long_packet(unsigned char vc, unsigned char packet_type, unsigned char *cmd_data, unsigned short word_count)
{
    int ret;
    int i;

    unsigned int gen_pld_data = 0;
    for (i = 0; i < word_count; i++) {
        gen_pld_data |= cmd_data[i] << (i % 4) * 8;
        if ((i + 1) % 4 == 0 && i != 0) {
            ret = dsi_write_gen_data(gen_pld_data);
            if (ret < 0)
                return -1;

            gen_pld_data = 0;
        }
    }

    if (word_count % 4 != 0) {
        ret = dsi_write_gen_data(gen_pld_data);
        if (ret < 0)
            return -1;
    }

    ret = dsi_write_short_packet(vc, packet_type, word_count);
    if (ret < 0)
        return -1;


    return 0;
}

static int dsi_read_packet(int bytes, char *rd_buf)
{
    int i;
    int off = 0;
    if (dsi_wait_rd_cmd_busy(10*1000)) {
        printk("read cmd busy!\n");
        return -1;
    }

    if (dsi_wait_rd_fifo_empty(10*1000)) {
        printk("read fifo is empty!\n");
        return -1;
    }

    for (i = 0; i < bytes; i++) {
        rd_buf[i] = dsi_get_bit(R_DSI_HOST_GEN_PLD_DATA, off, off + 7);
        off += 8;

        if ((i + 1) % 4 == 0 && i != 0) {
            off = 0;
        }
    }

    return 0;
}

static void jz_dsi_dpi_config(struct video_config *video_config)
{
    unsigned int hs_timeout;
    int counter;

    unsigned int temp;

    temp = video_config->byte_clock * 1000 / (video_config->pixel_clock);

    dsi_write(R_DSI_HOST_VID_HBP_TIME, video_config->hbp * temp / 1000);
    dsi_write(R_DSI_HOST_VID_HSA_TIME, video_config->hs * temp / 1000);
    dsi_write(R_DSI_HOST_VID_HLINE_TIME, video_config->h_total_pixels * temp / 1000);

    dsi_write(R_DSI_HOST_VID_VBP_LINES, video_config->vbp);
    dsi_write(R_DSI_HOST_VID_VFP_LINES, video_config->vfp);
    dsi_write(R_DSI_HOST_VID_VSA_LINES, video_config->vs);
    dsi_write(R_DSI_HOST_VID_VACTIVE_LINES, video_config->v_active_lines);

    unsigned long dpi_cfg_pol = dsi_read(R_DSI_HOST_DPI_CFG_POL);
    set_bit_field(&dpi_cfg_pol, DATAEN_ACTIVE_LOW, !video_config->data_en_polarity);
    set_bit_field(&dpi_cfg_pol, VSYNC_ACTIVE_LOW, 1);
    set_bit_field(&dpi_cfg_pol, HSYNC_ACTIVE_LOW, 1);
    dsi_write(R_DSI_HOST_DPI_CFG_POL, dpi_cfg_pol);

    //timeout ???
    hs_timeout = (video_config->h_total_pixels * video_config->v_active_lines) \
                  + (2 * (video_config->bpp_info * 100 / 8) / 100);

    for (counter = 0x80; (counter < hs_timeout) && (counter > 2); counter--) {
        if ((hs_timeout % counter) == 0) {
            dsi_set_bit(R_DSI_HOST_CLKMGR_CFG, TO_CLK_DIV, counter);
            dsi_set_bit(R_DSI_HOST_TO_CNT_CFG, HSTX_TO_CNT, (unsigned short)(hs_timeout / counter));
            dsi_set_bit(R_DSI_HOST_TO_CNT_CFG, LPRX_TO_CNT, (unsigned short)(hs_timeout / counter));
            break;
        }
    }

}


int jz_dsi_video_init(struct video_config *video_config)
{
    //high speed
    dsi_set_transfer_mode(0);

    //set video mode
    dsi_set_video_mode();

    unsigned long vid_mode_cfg = dsi_read(R_DSI_HOST_VID_MODE_CFG);

    set_bit_field(&vid_mode_cfg, VID_MODE_TYPE, video_config->video_mode);
    set_bit_field(&vid_mode_cfg, LP_VSA_EN, 1);
    set_bit_field(&vid_mode_cfg, LP_VBP_EN, 1);
    set_bit_field(&vid_mode_cfg, LP_VFP_EN, 1);
    set_bit_field(&vid_mode_cfg, LP_VACT_EN, 1);
    set_bit_field(&vid_mode_cfg, LP_HBP_EN, 1);
    set_bit_field(&vid_mode_cfg, LP_HFP_EN, 1);

    set_bit_field(&vid_mode_cfg, VPG_EN, 0);
    set_bit_field(&vid_mode_cfg, VPG_MODE, 0);
    set_bit_field(&vid_mode_cfg, VPG_ORIENTATION, 1);

    dsi_write(R_DSI_HOST_VID_MODE_CFG, vid_mode_cfg);

    //dpi config
    jz_dsi_dpi_config(video_config);


    //set R_DSI_HOST_CLKMGR_CFG tx_esc_clk_division????
    dsi_set_bit(R_DSI_HOST_CLKMGR_CFG, TX_ESC_CLK_DIV, 7);


    //pack_size chun_no null_size????
    dsi_set_bit(R_DSI_HOST_VID_NUM_CHUNKS, VID_NUM_CHUNKS, video_config->chunk);
    dsi_set_bit(R_DSI_HOST_VID_PKT_SIZE, VID_PKT_SIZE, video_config->video_size);
    dsi_set_bit(R_DSI_HOST_VID_NULL_SIZE, VID_NULL_SIZE, video_config->null_size);

    //set dpi channel
    dsi_set_bit(R_DSI_HOST_DPI_VCID, DPI_VCID, video_config->virtual_channel);

    //enable hs clk
    dsi_set_bit(R_DSI_HOST_LPCLK_CTRL, PHY_TXREQUESTCLKHS, 1);

    return 0;

}
