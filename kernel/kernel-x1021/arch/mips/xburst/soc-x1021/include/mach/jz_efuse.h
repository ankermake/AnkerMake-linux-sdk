#ifndef __JZ_EFUSE_H__
#define __JZ_EFUSE_H__

struct jz_efuse_platform_data {
    int gpio_vddq_en_n; /* supply 1.5V to VDDQ */
    int gpio_en_level;
};

enum segment_id {
    CHIP_ID,
    USER_ID,
    SARADC_CAL_DAT,
    TRIM_DATA,
    PROGRAM_PROTECT,
    CPU_ID,
    SPECIAL_USE,
    CUSTOMER_RESV,
};

int jz_efuse_read(unsigned int seg_id, unsigned int r_bytes, unsigned int offset, unsigned char* buf);

#endif
