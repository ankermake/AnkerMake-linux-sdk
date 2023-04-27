#ifndef _GPIO_REGS_H_
#define _GPIO_REGS_H_

#define PXPIN      0x00   /* PIN Level Register */
#define PXINT      0x10   /* Port Interrupt Register */
#define PXINTS     0x14   /* Port Interrupt Set Register */
#define PXINTC     0x18   /* Port Interrupt Clear Register */
#define PXMSK      0x20   /* Port Interrupt Mask Reg */
#define PXMSKS     0x24   /* Port Interrupt Mask Set Reg */
#define PXMSKC     0x28   /* Port Interrupt Mask Clear Reg */
#define PXPAT1     0x30   /* Port Pattern 1 Set Reg. */
#define PXPAT1S    0x34   /* Port Pattern 1 Set Reg. */
#define PXPAT1C    0x38   /* Port Pattern 1 Clear Reg. */
#define PXPAT0     0x40   /* Port Pattern 0 Register */
#define PXPAT0S    0x44   /* Port Pattern 0 Set Register */
#define PXPAT0C    0x48   /* Port Pattern 0 Clear Register */
#define PXFLG      0x50   /* Port Flag Register */
#define PXFLGC     0x58   /* Port Flag clear Register */
#define PXPU       0x80   /* Port PULL-UP State Register */
#define PXPUS      0x84   /* Port PULL-UP State Set Register */
#define PXPUC      0x88   /* Port PULL-UP State Clear Register */
#define PXPD       0x90   /* Port PULL-DOWN State Register */
#define PXPDS      0x94   /* Port PULL-DOWN State Set Register */
#define PXPDC      0x98   /* Port PULL-DOWN State Clear Register */

#define PZGID2LD   0xF0   /* GPIOZ Group ID to load */

#define GPIO_PORT_OFF    0x100
#define GPIO_SHADOW_OFF  0x700

#endif /* _GPIO_REGS_H_ */
