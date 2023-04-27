#ifndef __PDMAC_H__
#define __PDMAC_H__

#define PDMA_CHAN_OFF   0x20

#define DSA		(0x00)
#define DTA		(0x04)
#define DTC		(0x08)
#define DRT		(0x0C)
#define DCCS	(0x10)
#define DCMD	(0x14)
#define DDA		(0x18)
#define DSD		(0x1C)

#define DMAC	(0x1000)
#define DIRQP	(0x1004)
#define DDB		(0x1008)
#define DDBS	(0x100C)
#define DCKE	(0x1010)
#define DCKES	(0x1014)
#define DCKEC	(0x1018)
#define DMACP	(0x101C)
#define DSIRQP	(0x1020)
#define DSIRQM	(0x1024)
#define DCIRQP	(0x1028)
#define DCIRQM	(0x102C)

#define DMNMB	(0x1034)
#define DMSMB	(0x1038)
#define DMINT	(0x103C)

#define DCCS_NDES		(1 << 31)
#define DCCS_DES8		(1 << 30)
#define DCCS_DES4		(0 << 30)
#define DCCS_TOC_BIT	16
#define DCCS_TOC_MASK	(0x3fff << DCCS_TOC_BIT)
#define DCCS_CDOA_BIT	8
#define DCCS_CDOA_MASK	(0xff << DCCS_CDOA_BIT)
#define DCCS_AR			(1 << 4)
#define DCCS_TT			(1 << 3)
#define DCCS_HLT		(1 << 2)
#define DCCS_TOE		(1 << 1)
#define DCCS_CTE		(1 << 0)

#define DCMD_SAI			(1 << 23)
#define DCMD_DAI			(1 << 22)
#define DCMD_RDIL_BIT		16
#define DCMD_RDIL_MASK		(0xf << DCMD_RDIL_BIT)
#define DCMD_RDIL_1BYTE		(1 << DCMD_RDIL_BIT)
#define DCMD_RDIL_2BYTE		(2 << DCMD_RDIL_BIT)
#define DCMD_RDIL_3BYTE		(3 << DCMD_RDIL_BIT)
#define DCMD_RDIL_4BYTE		(4 << DCMD_RDIL_BIT)
#define DCMD_RDIL_8BYTE		(5 << DCMD_RDIL_BIT)
#define DCMD_RDIL_16BYTE	(6 << DCMD_RDIL_BIT)
#define DCMD_RDIL_32BYTE	(7 << DCMD_RDIL_BIT)
#define DCMD_RDIL_64BYTE	(8 << DCMD_RDIL_BIT)
#define DCMD_RDIL_128BYTE	(9 << DCMD_RDIL_BIT)
#define DCMD_SWDH_BIT		14
#define DCMD_SWDH_MASK		(0x3 << DCMD_SWDH_BIT)
#define DCMD_SWDH_32		(0 << DCMD_SWDH_BIT)
#define DCMD_SWDH_8			(1 << DCMD_SWDH_BIT)
#define DCMD_SWDH_16		(2 << DCMD_SWDH_BIT)
#define DCMD_DWDH_BIT		12
#define DCMD_DWDH_MASK		(0x03 << DCMD_DWDH_BIT)
#define DCMD_DWDH_32		(0 << DCMD_DWDH_BIT)
#define DCMD_DWDH_8			(1 << DCMD_DWDH_BIT)
#define DCMD_DWDH_16		(2 << DCMD_DWDH_BIT)
#define DCMD_TSZ_BIT		8
#define DCMD_TSZ_MASK		(0x7 << DCMD_TSZ_BIT)
#define DCMD_TSZ_32BIT		(0 << DCMD_TSZ_BIT)
#define DCMD_TSZ_8BIT		(1 << DCMD_TSZ_BIT)
#define DCMD_TSZ_16BIT		(2 << DCMD_TSZ_BIT)
#define DCMD_TSZ_16BYTE		(3 << DCMD_TSZ_BIT)
#define DCMD_TSZ_32BYTE		(4 << DCMD_TSZ_BIT)
#define DCMD_TSZ_64BYTE		(5 << DCMD_TSZ_BIT)
#define DCMD_TSZ_128BYTE	(6 << DCMD_TSZ_BIT)
#define DCMD_TSZ_AUTO		(7 << DCMD_TSZ_BIT)
#define DCMD_STDE			(1 << 2)
#define DCMD_TIE			(1 << 1)
#define DCMD_LINK			(1 << 0)

#define DRT_RT_BIT		0
#define DRT_RT_MASK		(0x3f << DRT_RT_BIT)
#define DRT_RT_I2S1OUT	(0x04 << DRT_RT_BIT)
#define DRT_RT_I2S1IN	(0x05 << DRT_RT_BIT)
#define DRT_RT_I2S0OUT	(0x06 << DRT_RT_BIT)
#define DRT_RT_I2S0IN	(0x07 << DRT_RT_BIT)
#define DRT_RT_AUTO		(0x08 << DRT_RT_BIT)
#define DRT_RT_SADCIN	(0x09 << DRT_RT_BIT)

#define DMINT_S_IP		(1 << 17)
#define DMINT_N_IP		(1 << 16)
#define DMINT_S_IMSK	(1 << 1)
#define DMINT_N_IMSK	(1 << 0)

#endif
