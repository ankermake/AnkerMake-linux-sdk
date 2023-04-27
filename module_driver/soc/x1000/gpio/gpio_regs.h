#ifndef _GPIO_REGS_H_
#define _GPIO_REGS_H_

#define GPIO_PORT_OFF    0x100
#define GPIO_SHADOW_OFF  0x700

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
#define PXOENS     0x64   /* Port Output Disable Set Register */
#define PXOENC     0x68   /* Port Output Disable Clear Register */
#define PXPEN      0x70   /* Port Pull Disable Register */
#define PXPENS     0x74   /* Port Pull Disable Set Register */
#define PXPENC     0x78   /* Port Pull Disable Clear Register */
#define PZGID2LD   0xF0   /* GPIOZ Group ID to load */

#define PXGFCFG0   0x800  /* PORT C Glitch Filter Configure register */
#define PXGFCFG0S  0x804  /* PORT C Glitch Filter Configure register */
#define PXGFCFG0C  0x808  /* PORT C Glitch Filter Configure register */
#define PXGFCFG1   0x810  /* PORT C Glitch Filter Configure register */
#define PXGFCFG1S  0x814  /* PORT C Glitch Filter Configure register */
#define PXGFCFG1C  0x818  /* PORT C Glitch Filter Configure register */
#define PXGFCFG2   0x820  /* PORT C Glitch Filter Configure register */
#define PXGFCFG2S  0x824  /* PORT C Glitch Filter Configure register */
#define PXGFCFG2C  0x828  /* PORT C Glitch Filter Configure register */
#define PXGFCFG3   0x830  /* PORT C Glitch Filter Configure register */
#define PXGFCFG3S  0x834  /* PORT C Glitch Filter Configure register */
#define PXGFCFG3C  0x838  /* PORT C Glitch Filter Configure register */

#define SHADOW (GPIO_PORT_D + 1)

#endif /* _GPIO_REGS_H_ */
