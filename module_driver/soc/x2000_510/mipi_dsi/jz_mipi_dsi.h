#ifndef _X2000_JZ_MIPI_DSI_H
#define _X2000_JZ_MIPI_DSI_H

#include "soc/x2000/fb/lcdc_data.h"

struct video_config {
    unsigned char virtual_channel;
    enum dsih_video_mode video_mode;
    unsigned int receive_ack_packets;

    enum lcdc_signal_polarity data_en_polarity;

    enum lcdc_signal_level h_polarity;
    unsigned short h_active_pixels;  /* hadr */
    unsigned short hs;
    unsigned short hbp;  /* hbp */
    unsigned short h_total_pixels;  /* h_total */

    enum lcdc_signal_level v_polarity;
    unsigned short v_active_lines;  /* vadr */
    unsigned short vs;
    unsigned short vbp;  /* vbp */
    unsigned short vfp;  /* v_total */

    unsigned int byte_clock;
    unsigned int pixel_clock;

    unsigned int chunk;
    unsigned int null_size;
    unsigned int video_size;

    unsigned int bpp_info;
};

struct dsi_config {
    unsigned char num_of_lanes;
    unsigned char max_hs_to_lp_cycles;
    unsigned char max_lp_to_hs_cycles;
    unsigned short max_bta_cycles;
    enum lcdc_signal_polarity color_mode_polarity;
    enum lcdc_signal_polarity shut_down_polarity;
    enum dsih_color_coding color_coding;
    enum mipi_dsi_18bit_type color_type_18bit;
    int te_mipi_en;
};

int dsi_write_cmd(struct dsi_cmd_packet *cmd_data);
int dsi_read_cmd(struct dsi_cmd_packet *cmd_data, int bytes, unsigned char *rd_buf);


#endif