#include <linux/init.h>
#include <linux/module.h>
#include <linux/errno.h>
#include <linux/device.h>
#include <linux/slab.h>
#include <linux/mtd/partitions.h>
#include "../spinand.h"
#include "../ingenic_sfc_common.h"
#include "nand_common.h"

#define WINBOND_DEVICES_NUM         1
#define TSETUP		5
#define THOLD		5
#define	TSHSL_R		10
#define	TSHSL_W		50

#define TRD		60
#define TPP		700
#define TBE		10

static struct ingenic_sfcnand_device *winbond_nand;

static struct ingenic_sfcnand_base_param winbond_param = {

	.pagesize = 2 * 1024,
	.oobsize = 64,
	.blocksize = 2 * 1024 * 64,
	.flashsize = 2 * 1024 * 64 * 1024,

	.tSETUP = TSETUP,
	.tHOLD  = THOLD,
	.tSHSL_R = TSHSL_R,
	.tSHSL_W = TSHSL_W,

	.tRD = TRD,
	.tPP = TPP,
	.tBE = TBE,

	.plane_select = 0,
	.ecc_max = 0x3,//0x3,
	.need_quad = 1,

};

static struct device_id_struct device_id[WINBOND_DEVICES_NUM] = {
	DEVICE_ID_STRUCT(0xAA, "25N01GVZEIR", &winbond_param),
};


static cdt_params_t *winbond_get_cdt_params(struct sfc_flash *flash, uint8_t device_id)
{
	CDT_PARAMS_INIT(winbond_nand->cdt_params);

	switch(device_id) {
		case 0xAA:
		    break;
		default:
		    dev_err(flash->dev, "device_id err, please check your  device id: device_id = 0x%02x\n", device_id);
		    return NULL;
	}

	return &winbond_nand->cdt_params;
}


static inline int deal_ecc_status(struct sfc_flash *flash, uint8_t device_id, uint8_t ecc_status)
{
	int ret = 0;
	switch(device_id) {
		case 0xAA:
			switch((ecc_status >> 4) & 0x3)
			{
				case 0x0:
					ret = 0;
					break;
				case 0x01:
					ret = 0x1;
					break;
				case 0x02:
				case 0x03:
					ret =0x2;
					break;
				default:
					ret = -EIO;
				}
			break;

		default:
			dev_err(flash->dev, "device_id err,it maybe don`t support this device, please check your device id: device_id = 0x%02x\n", device_id);
			return -EIO;   //notice!!!
	}
	return ret;
}


static int __init winbond_nand_init(void) {

	winbond_nand = kzalloc(sizeof(*winbond_nand), GFP_KERNEL);
	if(!winbond_nand) {
		pr_err("alloc ato_nand struct fail\n");
		return -ENOMEM;
	}

	winbond_nand->id_manufactory = 0xEF;
	winbond_nand->id_device_list = device_id;
	winbond_nand->id_device_count = WINBOND_DEVICES_NUM;
	winbond_nand->ops.get_cdt_params = winbond_get_cdt_params;
	winbond_nand->ops.deal_ecc_status = deal_ecc_status;

	/* use private get feature interface, please define it in this document */
	winbond_nand->ops.get_feature = NULL;

	return ingenic_sfcnand_register(winbond_nand);
}

fs_initcall(winbond_nand_init);
