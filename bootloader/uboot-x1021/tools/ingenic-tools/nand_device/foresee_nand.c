#include <stdio.h>
#include "nand_common.h"

#define FS_MID			    0xCD
#define FS_NAND_DEVICD_COUNT	    2

static unsigned char fs_xaw[] = {0x5, 0x6,};

static struct device_struct device[] = {
	DEVICE_STRUCT(0xA1, 2048, 2, 4, 3, 2, fs_xaw),
	DEVICE_STRUCT(0xB1, 2048, 2, 4, 3, 2, fs_xaw),
};

static struct nand_desc fs_nand = {

	.id_manufactory = FS_MID,
	.device_counts = FS_NAND_DEVICD_COUNT,
	.device = device,
};

int foresee_nand_register_func(void) {
	return nand_register(&fs_nand);
}
