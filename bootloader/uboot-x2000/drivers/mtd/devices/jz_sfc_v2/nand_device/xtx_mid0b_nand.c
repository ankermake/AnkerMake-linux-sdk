#include <errno.h>
#include <malloc.h>
#include <linux/mtd/partitions.h>
#include "../jz_sfc_common.h"
#include "nand_common.h"
#include <ubi_uboot.h>

#define XTX_MID0B_DEVICES_NUM         4
#define TSETUP		5
#define THOLD		5
#define	TSHSL_R		20
#define	TSHSL_W		20

#define TRD		260
#define TPP		350
#define TBE		3

static struct jz_sfcnand_device *xtx_mid0b_nand;

static struct jz_sfcnand_base_param xtx_mid0b_param[XTX_MID0B_DEVICES_NUM] = {

	[0] = {
		/*XT26G01A */
		.pagesize = 2 * 1024,
		.blocksize = 2 * 1024 * 64,
		.oobsize = 64,
		.flashsize = 2 * 1024 * 64 * 1024,

		.tSETUP  = TSETUP,
		.tHOLD   = THOLD,
		.tSHSL_R = TSHSL_R,
		.tSHSL_W = TSHSL_W,

		.tRD = TRD,
		.tPP = TPP,
		.tBE = TBE,

		.plane_select = 0,
		.ecc_max = 0x8,
		.need_quad = 1,
	},

	[1] = {
		/*XT26G02B */
		.pagesize = 2 * 1024,
		.blocksize = 2 * 1024 * 64,
		.oobsize = 64,
		.flashsize = 2 * 1024 * 64 * 2048,

		.tSETUP  = TSETUP,
		.tHOLD   = THOLD,
		.tSHSL_R = TSHSL_R,
		.tSHSL_W = TSHSL_W,

		.tRD = TRD,
		.tPP = TPP,
		.tBE = TBE,

		.plane_select = 0,
		.ecc_max = 0x4,
		.need_quad = 1,
	},

	[2] = {
		/*XT26G01C */
		.pagesize = 2 * 1024,
		.blocksize = 2 * 1024 * 64,
		.oobsize = 128,
		.flashsize = 2 * 1024 * 64 * 1024,

		.tSETUP  = TSETUP,
		.tHOLD   = THOLD,
		.tSHSL_R = TSHSL_R,
		.tSHSL_W = TSHSL_W,

		.tRD = 150,
		.tPP = 450,
		.tBE = 4,

		.plane_select = 0,
		.ecc_max = 0x8,
		.need_quad = 1,
	},

	[3] = {
		/*XT26G02C */
		.pagesize = 2 * 1024,
		.blocksize = 2 * 1024 * 64,
		.oobsize = 128,
		.flashsize = 2 * 1024 * 64 * 2048,

		.tSETUP  = TSETUP,
		.tHOLD   = THOLD,
		.tSHSL_R = TSHSL_R,
		.tSHSL_W = TSHSL_W,

		.tRD = 125,
		.tPP = 350,
		.tBE = 3,

		.plane_select = 0,
		.ecc_max = 0x8,
		.need_quad = 1,
	},
};

static struct device_id_struct device_id[XTX_MID0B_DEVICES_NUM] = {
	DEVICE_ID_STRUCT(0xE1, "XT26G01A ", &xtx_mid0b_param[0]),
	DEVICE_ID_STRUCT(0xF2, "XT26G02B ", &xtx_mid0b_param[1]),
	DEVICE_ID_STRUCT(0x11, "XT26G01C ", &xtx_mid0b_param[2]),
	DEVICE_ID_STRUCT(0x12, "XT26G02C ", &xtx_mid0b_param[3]),
};


static cdt_params_t *xtx_mid0b_get_cdt_params(struct sfc_flash *flash, uint8_t device_id)
{
	CDT_PARAMS_INIT(xtx_mid0b_nand->cdt_params);

	switch(device_id) {
	    case 0xE1:
	    case 0xF2:
	    case 0x11:
	    case 0x12:
		    break;
	    default:
		    pr_err("device_id err, please check your  device id: device_id = 0x%02x\n", device_id);
		    return NULL;
	}

	return &xtx_mid0b_nand->cdt_params;
}


static inline int deal_ecc_status(struct sfc_flash *flash, uint8_t device_id, uint8_t ecc_status)
{
	int ret = 0;

	switch(device_id) {
		case 0xE1:
			switch((ecc_status >> 2) & 0xf) {
			    case 0x12:
			    case 0x8:
				ret = -EBADMSG;
				break;
			    case 0x0 ... 0x7:
			    default:
				ret = 0;
				break;
			}
			break;

		case 0xF2:
			switch((ecc_status >> 4) & 0x3) {
			    case 0x02:
				    ret = -EBADMSG;
				    break;
			    case 0x03:
				    ret = 0x8;
				    break;
			    default:
				    ret = 0;
			}
			break;

		case 0x11:
		case 0x12:
			switch((ecc_status >> 4) & 0xf) {
				case 0x01:
					ret = 0x1;
					break;
				case 0x02:
					ret = 0x2;
					break;
				case 0x03:
					ret = 0x3;
					break;
				case 0x04:
					ret = 0x4;
					break;
				case 0x05:
					ret = 0x5;
					break;
				case 0x06:
					ret = 0x6;
					break;
				case 0x07:
					ret = 0x7;
					break;
				case 0x08:
					ret = 0x8;
					break;
				case 0x0f:
					ret = -EBADMSG;
					break;
				default:
					ret = 0;
					break;
			}
			break;

		default:
			printf("device_id err, it maybe don`t support this device, check your device id: device_id = 0x%02x\n", device_id);
			ret = -EIO;

	}
	return ret;
}


static int xtx_mid0b_nand_init(void) {

	xtx_mid0b_nand = kzalloc(sizeof(*xtx_mid0b_nand), GFP_KERNEL);
	if(!xtx_mid0b_nand) {
		pr_err("alloc xtx_mid0b_nand struct fail\n");
		return -ENOMEM;
	}

	xtx_mid0b_nand->id_manufactory = 0x0B;
	xtx_mid0b_nand->id_device_list = device_id;
	xtx_mid0b_nand->id_device_count = XTX_MID0B_DEVICES_NUM;

	xtx_mid0b_nand->ops.get_cdt_params = xtx_mid0b_get_cdt_params;
	xtx_mid0b_nand->ops.deal_ecc_status = deal_ecc_status;

	/* use private get feature interface, please define it in this document */
	xtx_mid0b_nand->ops.get_feature = NULL;

	return jz_sfcnand_register(xtx_mid0b_nand);
}

SPINAND_MOUDLE_INIT(xtx_mid0b_nand_init);
