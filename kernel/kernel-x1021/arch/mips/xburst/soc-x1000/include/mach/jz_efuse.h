#ifndef __JZ_EFUSE_H__
#define __JZ_EFUSE_H__

struct jz_efuse_platform_data {
	int gpio_vddq_en_n;	/* supply 2.5V to VDDQ */
};

enum segment_id {
	CHIP_ID,		/* 16 Bytes (read only) */
	RANDOM_ID,		/* 16 Bytes (read only) */
	USER_ID,		/* 16 Bytes (read, write) */
	TRIM3_ID,		/*  2 Bytes (read only) */
	PROTECT_ID,		/*  2 Bytes (read, write) */
};

#define USERID_PRT_BIT (1 << 7)

void jz_efuse_id_read(int is_chip_id, uint32_t *buf);

#endif
