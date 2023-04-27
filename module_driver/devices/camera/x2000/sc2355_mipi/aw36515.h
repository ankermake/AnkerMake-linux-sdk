/*
 * aw36515 Common Driver Header
 *
 */
#ifndef _LEDS_AW36515_H
#define _LEDS_AW36515_H



/* aw36515 mode */
enum aw_mode {
    AW_MODE_IR = 0,
    AW_MODE_TORCH,
    AW_MODE_SIZE,
};



int aw36515_power_on(enum aw_mode mode, int which_led, unsigned char rate);
int aw36515_led_ctrl(enum aw_mode mode, int which_led, unsigned char rate);
int aw36515_set_current_rate(int which_led, unsigned char rate);

int init_aw36515(void);
void exit_aw36515(void);


#endif /* _LEDS_AW36515_H */
