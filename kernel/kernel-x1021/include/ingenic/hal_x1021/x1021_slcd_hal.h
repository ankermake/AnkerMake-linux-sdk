#ifndef _X1021_SLCD_HAL_H_
#define _X1021_SLCD_HAL_H_

enum jzfb_src_format {
    SRC_FORMAT_555,
    SRC_FORMAT_565,
    SRC_FORMAT_888,
    SRC_FORMAT_reserve, // reserve, do not remove
    SRC_FORMAT_NV12,
    SRC_FORMAT_NV21,
};

enum jzfb_out_format {
    OUT_FORMAT_565,
    OUT_FORMAT_666,
    OUT_FORMAT_888,
};

enum jzfb_mcu_data_width {
    MCU_WIDTH_8BITS,
    MCU_WIDTH_9BITS,
    MCU_WIDTH_16BITS,
    MCU_WIDTH_18BITS,
    MCU_WIDTH_24BITS,
};

enum jzfb_out_order {
    ORDER_RGB,
    ORDER_RBG,
    ORDER_GRB,
    ORDER_BRG,
    ORDER_GBR,
    ORDER_BGR,
};

enum jzfb_refresh_mode {
    REFRESH_PAN_DISPLAY,
    REFRESH_CONTINUOUS,
};

enum jzfb_signal_polarity {
    AT_FALLING_EDGE,
    AT_RISING_EDGE,
};

enum jzfb_signal_level {
    AT_LOW_LEVEL,
    AT_HIGH_LEVEL,
};

enum jzfb_dc_pin {
    CMD_LOW_DATA_HIGH,
    CMD_HIGH_DATA_LOW,
};

enum jzfb_te_type {
    TE_NOT_EANBLE,
    TE_GPIO_IRQ_TRIGGER,
    TE_LCDC_TRIGGER,
};

struct jzfb_config_data {
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

    unsigned int pixclock_when_init;

    enum jzfb_src_format fb_format;
    enum jzfb_out_format out_format;
    enum jzfb_out_order out_order;
    enum jzfb_mcu_data_width mcu_data_width;
    enum jzfb_mcu_data_width mcu_cmd_width;
    enum jzfb_refresh_mode refresh_mode;

    enum jzfb_signal_polarity wr_data_sample_edge;
    enum jzfb_dc_pin dc_pin;

    enum jzfb_signal_level te_data_transfered_level;
    enum jzfb_te_type te_pin;

    enum jzfb_signal_level rdy_cmd_send_level;
    int enable_rdy_pin:1;
};

struct jzfb_frame_desc {
    unsigned int next_desc_addr;
    unsigned int buffer_addr_rgb;
    unsigned int stride_rgb;
    unsigned int chain_end;
    unsigned int eof_mask;
    unsigned int buffer_addr_uv;
    unsigned int stride_uv;
    unsigned int reserve;
};

enum jzfb_interrupt_type {
    JZFB_irq_frame_end = 1,
    JZFB_irq_dma_end = 2,
    JZFB_irq_general_stop = 5,
    JZFB_irq_quick_stop = 6,
};

void jzfb_hal_init(struct jzfb_config_data *config);

void jzfb_slcd_send_cmd(unsigned int cmd);

void jzfb_slcd_send_param(unsigned int data);

void jzfb_slcd_send_data(unsigned int data);

int jzfb_slcd_wait_busy(unsigned int count_udelay_10);

void jzfb_disable_slcd_fmt_convert(void);

void jzfb_enable_slcd_fmt_convert(void);

unsigned int jzfb_line_stride_pixels(struct jzfb_config_data *config, unsigned int xres);

unsigned int jzfb_bytes_per_pixel(struct jzfb_config_data *config);

void jzfb_set_frame_desc(unsigned int desc_addr);

void jzfb_scld_start_dma(void);

void jzfb_general_stop(void);

void jzfb_quick_stop(void);

void jzfb_disable_all_interrupt(void);

void jzfb_enable_interrupt(enum jzfb_interrupt_type irq_type);

void jzfb_disable_interrupt(enum jzfb_interrupt_type irq_type);

unsigned int jzfb_get_interrupts(void);

void jzfb_clear_interrupt(enum jzfb_interrupt_type irq_type);

int jzfb_check_interrupt(unsigned int flags, enum jzfb_interrupt_type irq_type);

unsigned int jzfb_get_current_frame_desc(void);

unsigned int jzfb_user_read_reg(unsigned int reg);

void jzfb_user_write_reg(unsigned int reg, unsigned int value);

void jzfb_user_set_bit_field(unsigned int reg, int start, int end, unsigned int val);

unsigned int jzfb_user_get_bit_field(unsigned int reg, int start, int end);

#endif /* _X1021_SLCD_HAL_H_ */