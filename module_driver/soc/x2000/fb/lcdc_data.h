#ifndef _LCDC_DATA_H_
#define _LCDC_DATA_H_

enum fb_fmt {
    fb_fmt_RGB555,
    fb_fmt_RGB565,
    fb_fmt_RGB888,
    fb_fmt_ARGB8888,
    fb_fmt_NV12,
    fb_fmt_NV21,
    fb_fmt_yuv422,
};

enum lcdc_out_format {
    OUT_FORMAT_RGB565,
    OUT_FORMAT_RGB666,
    OUT_FORMAT_RGB888,
    OUT_FORMAT_RGB444,
    OUT_FORMAT_RGB555,
};

enum lcdc_mcu_data_width {
    MCU_WIDTH_8BITS,
    MCU_WIDTH_9BITS,
    MCU_WIDTH_16BITS,

    // x2000 只有16个data 引脚
    // MCU_WIDTH_18BITS,
    // MCU_WIDTH_24BITS,
};

enum lcdc_out_order {
    ORDER_RGB,
    ORDER_RBG,
    ORDER_GRB,
    ORDER_BRG,
    ORDER_GBR,
    ORDER_BGR,
};

enum lcdc_lcd_mode {
    TFT_24BITS,
    TFT_18BITS,
    TFT_16BITS,
    TFT_8BITS_SERIAL,
    TFT_8BITS_DUMMY_SERIAL,
    TFT_MIPI,

    SLCD_6800,
    SLCD_8080,
    SLCD_MIPI,
    SLCD_SPI_3LINE,
    SLCD_SPI_4LINE,
};

enum lcdc_signal_polarity {
    AT_FALLING_EDGE,
    AT_RISING_EDGE,
};

enum lcdc_signal_level {
    AT_LOW_LEVEL,
    AT_HIGH_LEVEL,
};

enum lcdc_dc_pin {
    CMD_LOW_DATA_HIGH,
    CMD_HIGH_DATA_LOW,
};

enum lcdc_data_trans_mode {
    LCD_PARALLEL_MODE,
    LCD_SERIAL_MODE,
};

enum lcdc_te_type {
    TE_NOT_EANBLE,
    TE_GPIO_IRQ_TRIGGER,
    TE_LCDC_TRIGGER,
};

struct lcdc;

enum smart_config_type {
    SMART_CONFIG_DATA,
    SMART_CONFIG_PRM = SMART_CONFIG_DATA,
    SMART_CONFIG_CMD,
    SMART_CONFIG_UDELAY,
};

struct smart_lcd_data_table {
    enum smart_config_type type;
    unsigned int value;
};

enum dsih_video_mode {
    VIDEO_NON_BURST_WITH_SYNC_PULSES = 0,
    VIDEO_NON_BURST_WITH_SYNC_EVENTS,
    VIDEO_BURST_WITH_SYNC_PULSES
};

enum dsih_color_coding {
    COLOR_CODE_16BIT_CONFIG1,
    COLOR_CODE_16BIT_CONFIG2,
    COLOR_CODE_16BIT_CONFIG3,
    COLOR_CODE_18BIT_CONFIG1,
    COLOR_CODE_18BIT_CONFIG2,
    COLOR_CODE_24BIT
};

enum mipi_dsi_18bit_type {
    PACKED18,
    LOOSELY18,
};

enum tft_pix_clk_type {
    INVERT_DISABLE,
    INVERT_ENABLE,
};

struct dsi_cmd_packet {
    unsigned char packet_type;
    unsigned char cmd0_or_wc_lsb;
    unsigned char cmd1_or_wc_msb;
    unsigned char *cmd_data;
};

struct lcdc_data {
    /* video mode */
    const char *name;
    unsigned int refresh;
    unsigned int xres;
    unsigned int yres;
    unsigned int pixclock;
    unsigned int left_margin;
    unsigned int right_margin;
    unsigned int upper_margin;
    unsigned int lower_margin;
    unsigned int hsync_len;
    unsigned int vsync_len;

    enum fb_fmt fb_fmt;
    enum lcdc_lcd_mode lcd_mode;
    enum lcdc_out_format out_format;

    struct {
        enum lcdc_out_order even_line_order;
        enum lcdc_out_order odd_line_order;
        enum lcdc_signal_polarity pix_clk_polarity;
        enum lcdc_signal_level de_active_level;
        enum lcdc_signal_level hsync_vsync_active_level;

        enum tft_pix_clk_type pix_clk_inv;
    } tft;

    struct {
        unsigned int pixclock_when_init;
        int te_gpio;
        int rd_gpio;
        int cmd_of_start_frame;

        enum lcdc_mcu_data_width mcu_data_width;
        enum lcdc_mcu_data_width mcu_cmd_width;

        enum lcdc_signal_polarity wr_data_sample_edge;
        enum lcdc_signal_polarity rd_data_sample_edge;
        enum lcdc_dc_pin dc_pin;

        enum lcdc_signal_polarity te_data_transfered_edge;
        enum lcdc_te_type te_pin_mode;

        enum lcdc_signal_level rdy_cmd_send_level;
        int enable_rdy_pin:1;
    } slcd;

    struct {
        unsigned char num_of_lanes;
        unsigned char virtual_channel;
        unsigned int byte_clock;
        unsigned char max_hs_to_lp_cycles;
        unsigned char max_lp_to_hs_cycles;
        unsigned short max_bta_cycles;
        enum dsih_color_coding color_coding;
        enum lcdc_signal_polarity data_en_polarity;

        enum lcdc_signal_level hsync_active_level;
        enum lcdc_signal_level vsync_active_level;

        enum dsih_video_mode video_mode;
        enum lcdc_signal_polarity color_mode_polarity;
        enum lcdc_signal_polarity shut_down_polarity;

        enum mipi_dsi_18bit_type color_type_18bit;

        enum lcdc_te_type slcd_te_pin_mode;
        enum lcdc_signal_polarity slcd_te_data_transfered_edge;

    } mipi;


    int height; // 屏的物理高度,单位毫米
    int width;  // 屏的物理宽度,单位毫米
    struct smart_lcd_data_table *slcd_data_table;
    unsigned int slcd_data_table_length;
    int (*power_on)(struct lcdc *lcdc);
    int (*power_off)(struct lcdc *lcdc);
    void (*lcd_init)(void);
};

enum lcdc_layer_order {
    lcdc_layer_top,
    lcdc_layer_bottom,
    lcdc_layer_0,
    lcdc_layer_1,
    lcdc_layer_2,
    lcdc_layer_3,
};

struct lcdc_layer {
    enum fb_fmt fb_fmt;

    unsigned int xres;
    unsigned int yres;

    unsigned int xpos;
    unsigned int ypos;

    enum lcdc_layer_order layer_order;
    int layer_enable;

    struct {
        void *mem;
        unsigned int stride; // 单位： 字节
    } rgb;

    struct {
        void *mem;
        unsigned int stride; // 单位： 字节
    } y;

    struct {
        void *mem;
        unsigned int stride; // 单位： 字节
    } uv;

    struct {
        unsigned char enable;
        unsigned char value;
    } alpha;

    struct {
        unsigned char enable;
        unsigned int xres;
        unsigned int yres;
    } scaling;
};

struct srdma_cfg {
    enum fb_fmt fb_fmt;
    void *fb_mem;
    int is_video;
    int stride;
};

int jzfb_register_lcd(struct lcdc_data *pdata);

void jzfb_unregister_lcd(struct lcdc_data *pdata);

int slcd_read_data_only(int reg, int count, char *buffer);

#endif /* _LCDC_DATA_H_ */