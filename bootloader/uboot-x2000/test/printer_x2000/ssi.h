#ifndef __SSI_H__
#define __SSI_H__

#define	SSI_DR		 0x00
#define	SSI_CR0		 0x04
#define	SSI_CR1		 0x08
#define	SSI_SR		 0x0C
#define	SSI_ITR		 0x10
#define	SSI_ICR		 0x14
#define	SSI_GR		 0x18

#define SSI_CR0_SSIE		(1 << 15)
#define SSI_CR0_EACLRUN		(1 << 7)
#define SSI_CR0_FSEL		(1 << 6)
#define SSI_CR0_TFLUSH		(1 << 2)
#define SSI_CR0_RFLUSH		(1 << 1)

#define SSI_CR1_PHA		(1 << 1)
#define SSI_CR1_POL		(1 << 0)
#define SSI_CR1_FLEN_8BIT	(0x6 << SSI_CR1_FLEN_BIT)
#define SSI_CR1_FLEN_BIT	3
#define SSI_CR1_FMAT_SPI	(0 << SSI_CR1_FMAT_BIT)
#define SSI_CR1_FMAT_BIT	20

#define SSI_CR1_TCKFI_BIT	26
#define SSI_CR1_TCKFI_3		(3 << SSI_CR1_TCKFI_BIT)

#define SSI_CR1_TFVCK_BIT	28
#define SSI_CR1_TFVCK_1		(1 << SSI_CR1_TFVCK_BIT)
#define SSI_CR1_TFVCK_3		(3 << SSI_CR1_TFVCK_BIT)

#define SSI_CR1_UNFIN		(1 << 23)
#define SSI_GPCHL_LOW           (0 << 2)
#define SSI_GPCHL_HIGH          (1 << 2)

#define SSI_SR_END              (1 << 7)
#define SSI_SR_BUSY             (1 << 6)
#define SSI_SR_RFE		(1 << 4)
#define SSI_SR_TFF		(1 << 5)
#define SSI_SR_UNDR             (1 << 1)

#define SSI_SR_RFIFONUM_BIT	8
#define SSI_SR_RFIFONUM_MASK	(0xff << SSI_SR_RFIFONUM_BIT)

/* SPI Flash Instructions */
#define CMD_WREN 	0x06	/* Write Enable */
#define CMD_WRDI 	0x04	/* Write Disable */
#define CMD_RDSR 	0x05	/* Read Status Register */
#define CMD_RDSR_1 	0x35	/* Read Status1 Register */
#define CMD_RDSR_2 	0x15	/* Read Status2 Register */
#define CMD_WRSR 	0x01	/* Write Status Register */
#define CMD_WRSR_1 	0x31	/* Write Status1 Register */
#define CMD_WRSR_2 	0x11	/* Write Status2 Register */
#define CMD_READ 	0x03	/* Read Data */
#define CMD_FAST_READ 	0x0B	/* Read Data at high speed */
#define CMD_DUAL_READ   0x3b    /* Read Data at QUAD fast speed*/
#define CMD_QUAD_READ   0x6b    /* Read Data at QUAD fast speed*/
#define CMD_QUAD_IO_FAST_READ   0xeb    /* Read Data at QUAD IO fast speed*/
#define CMD_PP 		0x02	/* Page Program(write data) */
#define CMD_QPP 	0x32	/* QUAD Page Program(write data) */
#define CMD_SE 		0xD8	/* Sector Erase */
#define CMD_BE 		0xC7	/* Bulk or Chip Erase */
#define CMD_DP 		0xB9	/* Deep Power-Down */
#define CMD_RES 	0xAB	/* Release from Power-Down and Read Electronic Signature */
#define CMD_RDID 	0x9F	/* Read Identification */
#define CMD_SR_WIP          (1 << 0)
#define CMD_SR_QEP          (1 << 1)
#define CMD_ERASE_4K            0x20
#define CMD_ERASE_32K           0x52
#define CMD_ERASE_64K           0xd8
#define CMD_ERASE_CE            0x60
#define CMD_EN4B				0xB7
#define CMD_EX4B				0xE9
#define SSI_FRMHL_CE0_LOW_CE1_LOW   (0 << 30)
#define SSI_FRMHL_CE0_HIGH_CE1_LOW  (1 << 30)
#define SSI_FRMHL_CE0_LOW_CE1_HIGH  (2 << 30)
#define SSI_FRMHL_CE0_HIGH_CE1_HIGH (3 << 30)
#define SSI_GPCMD           (1 << 25)

#define TRAN_SPI_QUAD 0x5
#define TRAN_SPI_IO_QUAD   (0x6)

/* spi nand gd */
#define CMD_PARD				0x13	/* page read */
#define CMD_PE					0x10	/* program execute*/
#define CMD_PRO_LOAD			0x02	/* program load */
#define CMD_PRO_RDM				0x84	/* program random */
#define CMD_ERASE_128K			0xd8
#define CMD_R_CACHE				0x03	/* read from cache */
#define CMD_FR_CACHE			0x0b	/* fast read from cache */
#define CMD_GET_FEATURE			0x0f

#define FEATURE_ADDR			0xc0

#define P_FAIL			(1 << 3)	/* program fail */
#define E_FAIL			(1 << 2)	/* erase fail */
#define SPINAND_IS_BUSY	(1 << 0)	/* read , write ,erase ops is executing*/
#define ECC_UNCORRECTED	(0x10 << 2) /* ecc uncorrected */

/* 3/4 bytes address */
#define CMD_READ4 	0x13	/* Read Data */
#define CMD_FAST_READ4 	0x0C	/* Read Data at high speed */
#define CMD_PP_4B 		0x12	/* Page Program(write data) */
#define CMD_ERASE_4K_4B            0x21
#define CMD_ERASE_32K_4B           0x5C
#define CMD_ERASE_64K_4B           0xDC




extern void ssi_init(void);
extern int spi_trans_data(unsigned char *data, int size);
extern int spi_send(unsigned char *d,unsigned int count);


#endif /* __SSI_H__ */