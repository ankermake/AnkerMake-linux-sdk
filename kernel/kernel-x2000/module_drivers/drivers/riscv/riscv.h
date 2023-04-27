#ifndef __RISCV_H__
#define __RISCV_H__


struct riscv_device {
	struct device *dev;
	int irq;
	unsigned int iobase;

	struct mutex	mutex;
};


#define CCU_CCSR                    0x0
//bit31~0:reset entry/rw
#define CCU_CRER                    0x4
// bit31~0:host to riscv mailbox, gen intrrrupt/rw
#define CCU_FROM_HOST               0x8
// bit31~0:riscv to host mailbox, gen intrrrupt/rw
#define CCU_TO_HOST                 0xc
#define CCU_TIME_L                  0x10
#define CCU_TIME_H                  0x14
#define CCU_TIME_CMP_L              0x18
#define CCU_TIME_CMP_H              0x1c
#define CCU_INTC_MASK_L             0x20
#define CCU_INTC_MASK_H             0x24
#define CCU_INTC_PEND_L             0x28
#define CCU_INTC_PEND_H             0x2c
#define CCU_CATCH                   0x34
#define CCU_RAND_INT                0x38

#define CCU_PMA_CNT                 4
#define CCU_PMA_ADR_0               0x40
#define CCU_PMA_ADR_1               0x44
#define CCU_PMA_ADR_2               0x48
#define CCU_PMA_ADR_3               0x4c

#define CCU_PMA_CFG_0               0x60
#define CCU_PMA_CFG_1               0x64
#define CCU_PMA_CFG_2               0x68
#define CCU_PMA_CFG_3               0x6c

enum message_type {
	MESSAGE_INIT_SENSOR0,
	MESSAGE_INIT_SENSOR1,
	MESSAGE_INIT_SENSOR2,
	MESSAGE_ISP_PRESTART,
	MESSAGE_END,
};

struct message {
	enum message_type type;
	void *data;
	int data_size;
};

#define REPLY	0x01

#endif
