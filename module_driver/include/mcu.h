#ifndef _MD_MCU_H_
#define _MD_MCU_H_

void mcu_shutdown(void);

void mcu_bootup(void);

void mcu_reset(void);

int mcu_write_data(void *buf, unsigned int size);

int mcu_read_data(void *buf, unsigned int size);

#endif