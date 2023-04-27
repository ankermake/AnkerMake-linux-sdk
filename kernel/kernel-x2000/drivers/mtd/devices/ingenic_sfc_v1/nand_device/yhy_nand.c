#include <linux/init.h>
#include <linux/module.h>
#include <linux/errno.h>
#include <linux/device.h>
#include <linux/slab.h>
#include <linux/mtd/partitions.h>
#include "../spinand.h"
#include "../ingenic_sfc_common.h"
#include "nand_common.h"

#define	YHY_MIDC9_DEVICES_NUM         2
#define TSETUP		5
#define THOLD		5
#define	TSHSL_R		20
#define	TSHSL_W		50

#define TRD		200
#define TPP		800
#define TBE		10


static struct ingenic_sfcnand_base_param yhy_midc9_param[YHY_MIDC9_DEVICES_NUM] = {

	[0] = {
		/*HYF1GQ4U */
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

		.ecc_max = 0x1,
		.need_quad = 1,
	},

	[1] = {
		/*HYF2GQ4U */
		.pagesize = 2 * 1024,
		.blocksize = 2 * 1024 * 64,
		.oobsize = 128,
		.flashsize = 2 * 1024 * 64 * 2048,

		.tSETUP  = TSETUP,
		.tHOLD   = THOLD,
		.tSHSL_R = TSHSL_R,
		.tSHSL_W = TSHSL_W,

		.tRD = TRD,
		.tPP = TPP,
		.tBE = TBE,

		.ecc_max = 0x1,
		.need_quad = 1,
	},

};

static struct device_id_struct device_id[YHY_MIDC9_DEVICES_NUM] = {
	DEVICE_ID_STRUCT(0x21, "HYF1GQ4U", &yhy_midc9_param[0]),
	DEVICE_ID_STRUCT(0x52, "HYF2GQ4U", &yhy_midc9_param[1]),
};
static int32_t yhy_midc9_get_read_feature(struct flash_operation_message *op_info) {

	struct sfc_flash *flash = op_info->flash;
	struct ingenic_sfcnand_flashinfo *nand_info = flash->flash_info;
	struct sfc_transfer transfer;
	uint8_t device_id = nand_info->id_device;
	uint8_t ecc_status = 0;
	int32_t ret = 0;

retry:
	ecc_status = 0;
	memset(&transfer, 0, sizeof(transfer));
	sfc_list_init(&transfer);

	transfer.cmd_info.cmd = SPINAND_CMD_GET_FEATURE;
	transfer.sfc_mode = TM_STD_SPI;

	transfer.addr = SPINAND_ADDR_STATUS;
	transfer.addr_len = 1;

	transfer.cmd_info.dataen = ENABLE;
	transfer.data = &ecc_status;
	transfer.len = 1;
	transfer.direction = GLB_TRAN_DIR_READ;

	transfer.data_dummy_bits = 0;
	transfer.ops_mode = CPU_OPS;

	if(sfc_sync(flash->sfc, &transfer)) {
		dev_err(flash->dev,"sfc_sync error ! %s %s %d\n",__FILE__,__func__,__LINE__);
		return -EIO;
	}

	if(ecc_status & SPINAND_IS_BUSY)
		goto retry;

	switch(device_id) {
		case 0x21:
		case 0x52:
			switch((ecc_status >> 4) & 0x3) {
				case 0x00:
					ret = 0;
					break;
				case 0x01:
					ret = 0x1;
					break;
				default:
					ret = -EBADMSG;
			}
			break;
		default:
			dev_err(flash->dev,"device_id err, it maybe don`t support this device, check your device id: device_id = 0x%02x\n", device_id);
			ret = -EIO;
	}
	return ret;
}
static int yhy_midc9_nand_init(void) {
	struct ingenic_sfcnand_device *yhy_midc9_nand;
	yhy_midc9_nand = kzalloc(sizeof(*yhy_midc9_nand), GFP_KERNEL);
	if(!yhy_midc9_nand) {
		pr_err("alloc yhy_nand struct fail\n");
		return -ENOMEM;
	}
	yhy_midc9_nand->id_manufactory = 0xC9;
	yhy_midc9_nand->id_device_list = device_id;
	yhy_midc9_nand->id_device_count = YHY_MIDC9_DEVICES_NUM;
	yhy_midc9_nand->ops.nand_read_ops.get_feature = yhy_midc9_get_read_feature;
	return ingenic_sfcnand_register(yhy_midc9_nand);
}

fs_initcall(yhy_midc9_nand_init);
