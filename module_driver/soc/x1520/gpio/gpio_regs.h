#ifndef _GPIO_REGS_H_
#define _GPIO_REGS_H_

#define GPIO_PORT_OFF    0x100
#define GPIO_SHADOW_OFF  0x700

#define PXPIN            0x00   /* PIN Level Register */
#define PXINT            0x10   /* Port Interrupt Register */
#define PXINTS           0x14   /* Port Interrupt Set Register */
#define PXINTC           0x18   /* Port Interrupt Clear Register */
#define PXMSK            0x20   /* Port Interrupt Mask Reg */
#define PXMSKS           0x24   /* Port Interrupt Mask Set Reg */
#define PXMSKC           0x28   /* Port Interrupt Mask Clear Reg */
#define PXPAT1           0x30   /* Port Pattern 1 Set Reg. */
#define PXPAT1S          0x34   /* Port Pattern 1 Set Reg. */
#define PXPAT1C          0x38   /* Port Pattern 1 Clear Reg. */
#define PXPAT0           0x40   /* Port Pattern 0 Register */
#define PXPAT0S          0x44   /* Port Pattern 0 Set Register */
#define PXPAT0C          0x48   /* Port Pattern 0 Clear Register */
#define PXFLG            0x50   /* Port Flag Register */
#define PXFLGC           0x58   /* Port Flag clear Register */
#define PXPEN            0x60   /* Port Pull Disable Register */
#define PXPENS           0x64   /* Port Pull Disable Set Register */
#define PXPENC           0x68   /* Port Pull Disable Clear Register */
#define PXGFCFG0         0x70   /* port Glitch Filter Configure Register0 */
#define PXGFCFG0S        0x74   /* port Glitch Filter Configure Set Register0 */
#define PXGFCFG0C        0x78   /* port Glitch Filter Configure Clear Register0 */
#define PXGFCFG1         0x80   /* port Glitch Filter Configure Register1 */
#define PXGFCFG1S        0x84   /* port Glitch Filter Configure Set Register1 */
#define PXGFCFG1C        0x88   /* port Glitch Filter Configure Clear Register1 */
#define PXGFCFG2         0x90   /* port Glitch Filter Configure Register2 */
#define PXGFCFG2S        0x94   /* port Glitch Filter Configure Set Register2 */
#define PXGFCFG2C        0x98   /* port Glitch Filter Configure Clear Register2 */
#define PXGFCFG3         0xA0   /* port Glitch Filter Configure Register3 */
#define PXGFCFG3S        0xA4   /* port Glitch Filter Configure Set Register3 */
#define PXGFCFG3C        0xA8   /* port Glitch Filter Configure Clear Register3 */
#define PZGID2LD         0xF0   /* GPIOZ Group ID to load */

#define SHADOW (GPIO_PORT_C + 1)

#endif /* _GPIO_REGS_H_ */
