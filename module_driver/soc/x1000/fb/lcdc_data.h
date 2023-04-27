#ifndef _LCDC_DATA_H_
#define _LCDC_DATA_H_

enum fb_fmt {
    fb_fmt_RGB555,
    fb_fmt_RGB565,
    fb_fmt_RGB888,
};

enum smart_config_type {
    SMART_CONFIG_DATA,
    SMART_CONFIG_PRM,
    SMART_CONFIG_CMD,
    SMART_CONFIG_UDELAY,
};

struct smart_lcd_data_table {
    enum smart_config_type type;
    unsigned int value;
};

enum lcdc_out_format {
    OUT_FORMAT_565,
    OUT_FORMAT_666,
    OUT_FORMAT_888,
};

enum lcdc_mcu_data_width {
    MCU_WIDTH_8BITS,
    MCU_WIDTH_9BITS,
    MCU_WIDTH_16BITS,
    MCU_WIDTH_18BITS,
    MCU_WIDTH_24BITS,
};

enum lcdc_signal_polarity {
    AT_FALLING_EDGE,
    AT_RISING_EDGE,
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

enum lcdc_out_order {
    ORDER_RGB,
    ORDER_RBG,
    ORDER_GRB,
    ORDER_BRG,
    ORDER_GBR,
    ORDER_BGR,
};

enum lcdc_signal_level {
    AT_LOW_LEVEL,
    AT_HIGH_LEVEL,
};

enum lcdc_lcd_mode {
    SLCD_6800 = 1,
    SLCD_8080,
};

struct lcdc_data {
    const char *name;
    unsigned int refresh;
    unsigned int xres;
    unsigned int yres;
    unsigned int pixclock;
    unsigned int pixclock_when_init;
    unsigned int left_margin;
    unsigned int right_margin;
    unsigned int upper_margin;
    unsigned int lower_margin;
    unsigned int hsync_len;
    unsigned int vsync_len;

    enum fb_fmt fb_format;
    enum lcdc_out_format out_format;
    enum lcdc_lcd_mode lcd_mode;

    struct {
        int te_gpio;
        enum lcdc_out_order out_order;
        enum lcdc_mcu_data_width mcu_data_width;
        enum lcdc_mcu_data_width mcu_cmd_width;
        enum lcdc_data_trans_mode data_trans_mode;
        enum lcdc_data_trans_mode cmd_trans_mode;

        enum lcdc_signal_polarity wr_data_sample_edge;
        enum lcdc_dc_pin dc_pin;

        enum lcdc_signal_polarity te_data_transfered_edge;
        enum lcdc_te_type te_pin_mode;

        enum lcdc_signal_level rdy_cmd_send_level;
        int enable_rdy_pin:1;
    }slcd;

    int height; // height of picture in mm
    int width;  // width of picture in mm

    unsigned int cmd_of_start_frame;
    struct smart_lcd_data_table *slcd_data_table;
    unsigned int slcd_data_table_length;
    int (*power_on)(void);
    int (*power_off)(void);
};

int jzfb_register_lcd(struct lcdc_data *data);

void jzfb_unregister_lcd(struct lcdc_data *data);

#endif