#include "aes_hal.h"
#include <bit_field.h>
#include <linux/slab.h>

#define ASCR                0x00
#define ASSR                0x04
#define ASINTM              0x08
#define ASSA                0x0c
#define ASDA                0x10
#define ASTC                0x14
#define ASDI                0x18
#define ASDO                0x1c
#define ASKY                0x20
#define ASIV                0x24

#define ASCR_PS             28, 31
#define ASCR_Reserved       12, 27
#define ASCR_ENDIAN         11, 11
#define ASCR_CLR            10, 10
#define ASCR_DMAS           9, 9
#define ASCR_DMAE           8, 8
#define ASCR_KEYL           6, 7
#define ASCR_MODE           5, 5
#define ASCR_DECE           4, 4
#define ASCR_AESS           3, 3
#define ASCR_KEYS           2, 2
#define ASCR_INIT_IV        1, 1
#define ASCR_EN             0, 0

#define ASSR_Reserved       3, 31
#define ASSR_DMAD           2, 2
#define ASSR_AESD           1, 1
#define ASSR_KEYD           0, 0

#define ASINTM_Reserved     3, 31
#define ASINTM_DMA_INT_M    2, 2
#define ASINTM_AES_INT_M    1, 1
#define ASINTM_KEY_INT_M    0, 0

#define ASSA_SA             0, 31
#define ASDA_DA             0, 31
#define ASTC_TC             0, 31
#define ASDI_DIN            0, 31
#define ADSO_DOUT           0, 31
#define ASKY_ASKY           0, 31
#define ASIV_ASIV           0, 31

#define AES_IOBASE          0x13430000

#define AES_ADDR(reg)       ((volatile unsigned long *)((KSEG1ADDR(AES_IOBASE)) + (reg)))

static inline void aes_write_reg(unsigned long reg, unsigned int value)
{
    *AES_ADDR(reg) = value;
}

static inline unsigned int aes_read_reg(unsigned long reg)
{
    return *AES_ADDR(reg);
}

static inline void aes_set_bit(unsigned long reg, int bit, unsigned int val)
{
    set_bit_field(AES_ADDR(reg), bit, bit, val);
}

static inline unsigned int aes_get_bit(unsigned long reg, int bit)
{
    return get_bit_field(AES_ADDR(reg), bit, bit);
}

static inline void aes_set_bits(unsigned long reg, int start, int end, unsigned int val)
{
    set_bit_field(AES_ADDR(reg), start, end, val);
}

static inline unsigned int aes_get_bits(unsigned long reg, int start, int end)
{
    return get_bit_field(AES_ADDR(reg), start, end);
}

//////////////////////////////////////////////////////////////

/* AES Control Register (ASCR) */
/* 0~5: don't set these bits, and keep them default value(3:4KB) */
inline void aes_hal_set_page_size(unsigned int page_size)
{
    aes_set_bits(ASCR, ASCR_PS, page_size);
}

inline unsigned int aes_hal_get_page_size(void)
{
    return aes_get_bits(ASCR, ASCR_PS);
}

/* 1: big-endian  0: little-endian */
inline void aes_hal_set_data_input_endian(unsigned int endian)
{
    aes_set_bits(ASCR, ASCR_ENDIAN, endian);
}

inline unsigned int aes_hal_get_data_input_endian(void)
{
    return aes_get_bits(ASCR, ASCR_ENDIAN);
}

/* 此位必须由两个原因设置:
    (1) CBC模式解码后;
    (2) 出于安全考虑, 清除IV/钥匙/输入数据/输出数据; */
/* RWS:read and write, set to 1 by write 1, write 0 has no effect */
inline void aes_hal_clear_iv_keys(void)
{
    aes_set_bits(ASCR, ASCR_CLR, 1);
}

inline void aes_hal_start_dma(void)
{
    aes_set_bits(ASCR, ASCR_DMAS, 1);
}

inline void aes_hal_enable_dma(void)
{
    aes_set_bits(ASCR, ASCR_DMAE, 1);
}

inline void aes_hal_disable_dma(void)
{
    aes_set_bits(ASCR, ASCR_DMAE, 0);
}

/* 0:128  1:192  2:256 */
inline void aes_hal_set_key_length(enum aes_keyl keyl)
{
    aes_set_bits(ASCR, ASCR_KEYL, keyl);
}

inline enum aes_keyl aes_hal_get_key_length(void)
{
    return aes_get_bits(ASCR, ASCR_KEYL);
}

inline void aes_hal_select_mode(enum aes_mode mode)
{
    aes_set_bits(ASCR, ASCR_MODE, mode);
}

inline enum aes_mode aes_hal_get_mode(void)
{
    return aes_get_bits(ASCR, ASCR_MODE);
}

inline void aes_hal_select_dece(enum aes_dece mode)
{
    aes_set_bits(ASCR, ASCR_DECE, mode);
}

inline enum aes_dece aes_hal_get_dece(void)
{
    return aes_get_bits(ASCR, ASCR_DECE);
}

inline void aes_hal_start_encrypt(void)
{
    aes_set_bits(ASCR, ASCR_AESS, 1);
}

inline void aes_hal_start_key_expansion(void)
{
    aes_set_bits(ASCR, ASCR_KEYS, 1);
}

inline void aes_hal_initial_iv(void)
{
    aes_set_bits(ASCR, ASCR_INIT_IV, 1);
}

inline void aes_hal_enable_encrypt(void)
{
    aes_set_bits(ASCR, ASCR_EN, 1);
}

inline void aes_hal_disable_encrypt(void)
{
    aes_set_bits(ASCR, ASCR_EN, 0);
}

/* AES Status Register (ASSR) */
/* RWC:read and write, clear to 0 by write 1, write 0 has no effect */
inline void aes_hal_clear_dma_done_status(void)
{
    aes_set_bits(ASSR, ASSR_DMAD, 1);
}

inline unsigned int aes_hal_get_dma_done_status(void)
{
    return aes_get_bits(ASSR, ASSR_DMAD);
}

inline void aes_hal_clear_aes_done_status(void)
{
    aes_set_bits(ASSR, ASSR_AESD, 1);
}

inline unsigned int aes_hal_get_aes_done_status(void)
{
    return aes_get_bits(ASSR, ASSR_AESD);
}

inline void aes_hal_clear_key_expansion_done_status(void)
{
    aes_set_bits(ASSR, ASSR_KEYD, 1);
}

inline unsigned int aes_hal_get_key_expansion_done_status(void)
{
    return aes_get_bits(ASSR, ASSR_KEYD);
}

inline void aes_hal_clear_all_done_status(void)
{
    aes_write_reg(ASSR, 0x7);
}

inline unsigned int aes_hal_get_all_done_status(void)
{
    return aes_read_reg(ASSR);
}

/* AES Interrupt Mask Register (ASINTM) */
inline void aes_hal_enable_dma_done_interrupt(void)
{
    aes_set_bits(ASINTM, ASINTM_DMA_INT_M, 1);
}

inline void aes_hal_mask_dma_done_interrupt(void)
{
    aes_set_bits(ASINTM, ASINTM_DMA_INT_M, 0);
}

inline void aes_hal_enable_aes_done_interrupt(void)
{
    aes_set_bits(ASINTM, ASINTM_AES_INT_M, 1);
}

inline void aes_hal_mask_aes_done_interrupt(void)
{
    aes_set_bits(ASINTM, ASINTM_AES_INT_M, 0);
}

inline void aes_hal_enable_key_expansion_done_interrupt(void)
{
    aes_set_bits(ASINTM, ASINTM_KEY_INT_M, 1);
}

inline void aes_hal_mask_key_expansion_done_interrupt(void)
{
    aes_set_bits(ASINTM, ASINTM_KEY_INT_M, 0);
}

inline void aes_hal_enable_all_interrupt(void)
{
    aes_write_reg(ASINTM, 0x7);
}

inline void aes_hal_mask_all_interrupt(void)
{
    aes_write_reg(ASINTM, 0);
}

inline unsigned int aes_hal_get_all_interrupt(void)
{
    return aes_read_reg(ASINTM);
}

/* AES DMA Source Address (ASSA) */
inline void aes_hal_set_dma_src_addr(void *virt_addr)
{
    unsigned int phys_addr = virt_to_phys(virt_addr);
    aes_write_reg(ASSA, phys_addr);
}

/* AES DMA Destination Address (ASDA) */
inline void aes_hal_set_dma_dst_addr(void *virt_addr)
{
    unsigned int phys_addr = virt_to_phys(virt_addr);
    aes_write_reg(ASDA, phys_addr);
}

/* AES Transfer Count (ASTC) */
/* 1Block == 128bit */
inline void aes_hal_set_transfer_count(unsigned int blocks)
{
    aes_write_reg(ASTC, blocks);
}

/* AES Data Input Register (ASDI) */
inline void aes_hal_data_input(unsigned char *data, enum aes_endian endian)
{
    aes_write_reg(ASDI, GETU32(data, endian));
    aes_write_reg(ASDI, GETU32(data + 4, endian));
    aes_write_reg(ASDI, GETU32(data + 8, endian));
    aes_write_reg(ASDI, GETU32(data + 12, endian));
}

/* AES Data Output Register (ASDO) */
inline void aes_hal_data_output(unsigned char *data, enum aes_endian endian)
{
    int i, dout;
    for (i = 0; i < 4; i++) {
        dout = aes_read_reg(ASDO);
        if (endian == ENDIAN_BIG) {
            data[i * 4 + 3] = (dout & 0xff000000) >> 24;
            data[i * 4 + 2] = (dout & 0x00ff0000) >> 16;
            data[i * 4 + 1] = (dout & 0x0000ff00) >> 8;
            data[i * 4 + 0] = (dout & 0x000000ff) >> 0;
        } else {
            data[i * 4 + 0] = (dout & 0xff000000) >> 24;
            data[i * 4 + 1] = (dout & 0x00ff0000) >> 16;
            data[i * 4 + 2] = (dout & 0x0000ff00) >> 8;
            data[i * 4 + 3] = (dout & 0x000000ff) >> 0;
        }
    }
}

/* AES KEY FIFO (ASKY) */
inline void aes_hal_set_keys(unsigned int *key, enum aes_keyl keyl)
{
    if (keyl < AES128 || keyl > AES256) {
        printk(KERN_ERR "AES: keyl is error!\n");
        return ;
    }

    aes_write_reg(ASKY, key[0]);
    aes_write_reg(ASKY, key[1]);
    aes_write_reg(ASKY, key[2]);
    aes_write_reg(ASKY, key[3]);

    if (keyl >= AES192) {
        aes_write_reg(ASKY, key[4]);
        aes_write_reg(ASKY, key[5]);
    }

    if (keyl == AES256) {
        aes_write_reg(ASKY, key[6]);
        aes_write_reg(ASKY, key[7]);
    }
}

/* AES IV FIFO (ASIV) */
inline void aes_hal_set_iv(unsigned char *iv, enum aes_endian endian)
{
    aes_write_reg(ASIV, GETU32(iv, endian));
    aes_write_reg(ASIV, GETU32(iv + 4, endian));
    aes_write_reg(ASIV, GETU32(iv + 8, endian));
    aes_write_reg(ASIV, GETU32(iv + 12, endian));
}

void aes_dump_regs(void)
{
    printk(KERN_ERR "ASCR: %x\n", aes_read_reg(ASCR));
    printk(KERN_ERR "ASSR: %x\n", aes_read_reg(ASSR));
    printk(KERN_ERR "ASINTM: %x\n", aes_read_reg(ASINTM));
    printk(KERN_ERR "ASSA: %x\n", aes_read_reg(ASSA));
    printk(KERN_ERR "ASDA: %x\n", aes_read_reg(ASDA));
    printk(KERN_ERR "ASTC: %x\n", aes_read_reg(ASTC));
    printk(KERN_ERR "ASKY: %x\n", aes_read_reg(ASKY));
    printk(KERN_ERR "ASIV: %x\n", aes_read_reg(ASIV));
}
