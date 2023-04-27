#ifndef _LCD_DATA_H_
#define _LCD_DATA_H_

struct lcdc;

enum fb_fmt {
    fb_fmt_RGB555,
    fb_fmt_RGB565,
    fb_fmt_RGB888,
    fb_fmt_ARGB8888,
    fb_fmt_NV12,
    fb_fmt_NV21,
};

struct lcdc;

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
    TFT_8BITS_SERIAL = 2,
    TFT_8BITS_DUMMY_SERIAL,

    SLCD_6800,
    SLCD_8080,
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
        enum lcdc_signal_level hsync_active_level;
        enum lcdc_signal_level vsync_active_level;
    } tft;

    struct {
        unsigned int pixclock_when_init;
        int te_gpio;
        int cmd_of_start_frame;

        enum lcdc_mcu_data_width mcu_data_width;
        enum lcdc_mcu_data_width mcu_cmd_width;

        enum lcdc_signal_polarity wr_data_sample_edge;
        enum lcdc_dc_pin dc_pin;

        enum lcdc_signal_polarity te_data_transfered_edge;
        enum lcdc_te_type te_pin_mode;

        enum lcdc_signal_level rdy_cmd_send_level;
        int enable_rdy_pin:1;
    } slcd;

    int height; // 屏的物理高度,单位毫米
    int width;  // 屏的物理宽度,单位毫米
    struct smart_lcd_data_table *slcd_data_table;
    unsigned int slcd_data_table_length;
    int (*power_on)(struct lcdc *lcdc);
    int (*power_off)(struct lcdc *lcdc);
};

struct srdma_cfg {
    enum fb_fmt fb_fmt;
    void *fb_mem;
    int is_video;
    int stride;
};

int jzfb_register_lcd(struct lcdc_data *pdata);

void jzfb_unregister_lcd(struct lcdc_data *pdata);


#endif