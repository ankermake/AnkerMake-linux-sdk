//#define DEBUG
#include <common.h>
#include <errno.h>
#include <asm/io.h>
#include <spl.h>
#include <asm/arch/clk.h>
#include <asm/arch/sfc.h>
#include <asm/arch/spinand.h>
#include <generated/sfc_timing_val.h>
#include <generated/sfc_nand_params.h>
#include "spl_ota.h"
#include "spl_rtos.h"

#define SPINAND_PARAM_SIZE	1024

//#define  CONFIG_SPI_STANDARD
static struct spl_nand_param *curr_device;

static inline void sfc_writel(unsigned int value, unsigned short offset)
{
	writel(value, SFC_BASE + offset);
}

static inline unsigned int sfc_readl(unsigned short offset)
{
	return readl(SFC_BASE + offset);
}

static inline void sfc_transfer_direction(unsigned int value)
{
	unsigned int val;

	val = sfc_readl(SFC_GLB);
	val &= ~(1 << GLB_TRAN_DIR_OFFSET);
	val |= value << GLB_TRAN_DIR_OFFSET;
	sfc_writel(val, SFC_GLB);
}

static inline void sfc_set_length(unsigned int value)
{
	sfc_writel(value, SFC_TRAN_LEN);
}

static inline void sfc_dev_addr(unsigned int channel, unsigned int value)
{
	sfc_writel(value, SFC_DEV_ADDR(channel));
}

static inline void sfc_dev_addr_plus(unsigned int channel, unsigned int value)
{
	sfc_writel(value, SFC_DEV_ADDR_PLUS(channel));
}

static inline void set_flash_timing(void)
{
	sfc_writel(DEF_TIM_VAL, SFC_DEV_CONF);
}

static inline void sfc_tranconf_init(struct jz_sfc *sfc, unsigned int channel)
{
	sfc_writel(sfc->tranconf.d32, SFC_TRAN_CONF(channel));
}

static void sfc_set_transfer(struct jz_sfc *sfc, unsigned int dir)
{
	sfc_tranconf_init(sfc, 0);
	sfc_transfer_direction(dir);
	sfc_set_length(sfc->len);
	sfc_dev_addr(0, sfc->addr);
}
static void clear_end(void)
{
	while(!(sfc_readl(SFC_SR) & END));
        sfc_writel(CLR_END, SFC_SCR);

}

static void sfc_send_cmd(struct jz_sfc *sfc, unsigned char dir)
{
	sfc_writel(1 << 1, SFC_TRIG);
	sfc_set_transfer(sfc, dir);
	sfc_writel(1 << 2, SFC_TRIG);
	sfc_writel(TRIG_START, SFC_TRIG);
}

static void sfc_write_data(unsigned int *data, unsigned int length)
{
	while (!(sfc_readl(SFC_SR) & TRAN_REQ));
	sfc_writel(CLR_TREQ,SFC_SCR);
	sfc_writel(*data, SFC_RM_DR);
	clear_end();
}

static int sfc_read_data(unsigned int *data, unsigned int length)
{
	unsigned int tmp_len = 0;
	unsigned int fifo_num = 0;
	unsigned int reg_tmp = 0;
	unsigned int len = (length + 3) / 4;
	int i;

	while(1){
		reg_tmp = sfc_readl(SFC_SR);
		if (reg_tmp & RECE_REQ) {
			sfc_writel(CLR_RREQ, SFC_SCR);
			if ((len - tmp_len) > THRESHOLD)
				fifo_num = THRESHOLD;
			else
				fifo_num = len - tmp_len;

			for (i = 0; i < fifo_num; i++) {
				*data++ = sfc_readl(SFC_RM_DR);
				tmp_len++;
			}
		}
		if (tmp_len == len)
			break;
	}
	clear_end();

	return 0;
}

static void sfc_controler_init(void)
{
	unsigned int tmp;

	clk_set_rate(SFC, CONFIG_SFC_NAND_RATE);

	tmp = sfc_readl(SFC_GLB);
	tmp &= ~(GLB_THRESHOLD_MSK);
	tmp |= (THRESHOLD << GLB_THRESHOLD_OFFSET);
	sfc_writel(tmp, SFC_GLB);

	set_flash_timing();
}

static int spinand_bad_block_check(int len, unsigned char *check_buf)
{
	int i;

	for(i = 0; i < len; i++)
		if(check_buf[i] != 0xff)
			return 1;
	return 0;
}

static int spinand_read_page(unsigned int page, unsigned int column, unsigned char *dst_addr,
							 unsigned int len, unsigned int pagesize)
{
	struct jz_sfc sfc;
	unsigned int read_buf = 0;
	int oob_flag = 0;
	unsigned char i;
	unsigned char checklen = 1;

read_oob:
	if (oob_flag) {
		column = pagesize;
		len = 4;
		dst_addr = (unsigned char *)&read_buf;
	}

	SFC_SEND_COMMAND(&sfc, SPINAND_CMD_PARD, 0, page, 3, 0, 0, 0);
	clear_end();
	do {
		SFC_SEND_COMMAND(&sfc, SPINAND_CMD_GET_FEATURE, 1, SPINAND_ADDR_STATUS, 1, 0, 1, 0);
		sfc_read_data(&read_buf, 1);
	}while(read_buf & 0x1);

	/* ecc check */
	for(i = 0; i < curr_device->eccstat_count; i++) {
		read_buf = read_buf & 0xff;
		if(((read_buf >> curr_device->ecc_bit) &
		(~(0xff << curr_device->bit_counts))) == curr_device->eccerrstatus[i]) {
		    /* ecc Error and not corrected */
			return 1;
		}
	}

	if(curr_device->device_id == 0x20 || curr_device->device_id == 0x22 || (curr_device->device_id == 0x72))/*MXIC 2G plane select*/
		column |= (((page >> 6) & 1) << 12);

#ifndef CONFIG_SPI_STANDARD
	SFC_SEND_COMMAND(&sfc, SPINAND_CMD_RDCH_X4, len, column, curr_device->addrlen, 8, 1, 0);
#else
	SFC_SEND_COMMAND(&sfc, SPINAND_CMD_FRCH, len, column, curr_device->addrlen, 8, 1, 0);
#endif
	sfc_read_data((unsigned int *)dst_addr, len);

	if (!oob_flag && !(page % CONFIG_SPI_NAND_PPB)) {
		oob_flag = 1;
		goto read_oob;
	} else if (oob_flag) {
#if NAND_BUSWIDTH == NAND_BUSWIDTH_16
		checklen = 2;
#endif
		if (spinand_bad_block_check(checklen, (unsigned char *)&read_buf))
			return 1;
	}

	return 0;
}

static int probe_id_list(unsigned char *id)
{
	unsigned char i;

	for (i = 0; i < ARRAY_SIZE(nand_param); i++) {
		if (nand_param[i].id_manufactory == id[0] &&
			    nand_param[i].device_id == id[1]) {
			curr_device = &nand_param[i];
			break;
		}
	}

	if (i == ARRAY_SIZE(nand_param))
		return -ENODEV;

	return 0;
}

static int spinand_probe_id(struct jz_sfc *sfc)
{
	/*
	 * cmd-->addr-->pid
	 */
	unsigned char addrlen[] = {0, 1};
	unsigned char id[2] = {0};
	unsigned char i;

	for(i = 0; i < sizeof(addrlen); i++) {
		SFC_SEND_COMMAND(sfc, SPINAND_CMD_RDID, 2, 0, addrlen[i], 0, 1, 0);
		sfc_read_data((unsigned int *)id, 2);
		if (!probe_id_list(id))
			    break;
	}
	if(i == sizeof(addrlen)) {
		debug("ERR: don`t support this kind of nand device, \
			please add it\n");
		return -ENODEV;
	}
	return 0;
}

static int spinand_init(void)
{
	struct jz_sfc sfc;
	unsigned int x;

	/*
	 * Probe nand vid/pid
	 */
	if(spinand_probe_id(&sfc))
		return -ENODEV;

	/* disable write protect */
	x = 0;
	SFC_SEND_COMMAND(&sfc, SPINAND_CMD_SET_FEATURE, 1, SPINAND_ADDR_PROTECT, 1, 0, 1, 1);
	sfc_write_data(&x, 1);

	x = BITS_QUAD_EN | BITS_ECC_EN | BITS_BUF_EN;
	SFC_SEND_COMMAND(&sfc, SPINAND_CMD_SET_FEATURE, 1, SPINAND_ADDR_FEATURE, 1, 0, 1, 1);
	sfc_write_data(&x, 1);

	return 0;
}

int sfc_nand_load(unsigned int src_addr, unsigned int count, unsigned int dst_addr)
{
	unsigned int pageaddr, columnaddr, rlen;
	int ret, retry_count = 5;
	unsigned char *buf = (unsigned char *)dst_addr;
	unsigned int pagesize = curr_device->pagesize;

	while (count) {
		pageaddr = src_addr / pagesize;
		columnaddr = src_addr % pagesize;
		rlen = (pagesize - columnaddr) < count ? (pagesize - columnaddr) : count;

		ret = spinand_read_page(pageaddr, columnaddr, buf, rlen, pagesize);
		if (ret > 0) {
			debug("bad block %d\n", pageaddr / CONFIG_SPI_NAND_PPB);
			src_addr += CONFIG_SPI_NAND_PPB * pagesize;
			continue;
		}

		if(ret < 0 && retry_count--) {
			continue;
		}
#if 0
		if(retry_count < 0) {
			debug("read page ecc error, pageaddr = %d, columnaddr = %d\n", pageaddr,
				columnaddr);
			return -1;
		}
#endif
		buf += rlen;
		src_addr += rlen;
		count -= rlen;
		retry_count = 5;
	}

	return 0;
}

#ifdef CONFIG_SPL_OS_BOOT
#define HAS_GET_PARTITION
#endif

#ifdef CONFIG_SPL_OS_OTA_BOOT
#define HAS_GET_PARTITION
#endif

#ifdef HAS_GET_PARTITION
static struct jz_sfcnand_partition_param *get_partitions(void)
{
	struct jz_sfcnand_burner_param *burn_param;
	static struct jz_sfcnand_partition_param partitions;

	/*read burn param*/
	sfc_nand_load(CONFIG_SPIFLASH_PART_OFFSET, SPINAND_PARAM_SIZE, CONFIG_SYS_TEXT_BASE);
	burn_param = (void *)(CONFIG_SYS_TEXT_BASE);
	partitions.num_partition = burn_param->partition_num;
	partitions.partition = (struct jz_sfcnand_partition *)&burn_param->partition;

	return &partitions;
}

static unsigned int get_part_offset_by_name(struct jz_sfcnand_partition_param *partitions, const char *name)
{
	int i = 0;

	for(i = 0; i < partitions->num_partition; i++) {
		if (!strncmp(partitions->partition[i].name, name, strlen(name))) {
			return partitions->partition[i].offset;
		}
	}
	return -1;
}
#endif

void spl_load_kernel(long offset)
{
	struct image_header *header;
	header = (struct image_header *)(CONFIG_SYS_TEXT_BASE);

	sfc_nand_load(offset, sizeof(struct image_header), CONFIG_SYS_TEXT_BASE);
	header->ih_name[IH_NMLEN - 1] = 0;
	spl_parse_image_header(header);
	printf("offset = %d, spl_image.load_addr = %x\n", offset, spl_image.load_addr);
	sfc_nand_load(offset, spl_image.size, spl_image.load_addr);
}

static void sfc_init(void)
{
	sfc_controler_init();
	spinand_init();
}

#ifdef CONFIG_SPL_OS_BOOT
void spl_sfc_os_boot(void)
{
	struct jz_sfcnand_partition_param *partitions;
	unsigned int bootimg_addr = 0;

	sfc_init();

	partitions = get_partitions();
	bootimg_addr = get_part_offset_by_name(partitions, CONFIG_SPL_OS_NAME);
	if (bootimg_addr == -1) {
		printf("[ERROR%s:%d] cannot find (%s) partition\n", __func__, __LINE__, CONFIG_SPL_OS_NAME);
		printf("TODO: check burner spinand partition config\n");
	}

	spl_load_kernel(bootimg_addr);
}
#endif

#ifdef CONFIG_SPL_OS_OTA_BOOT
static char *spl_sfc_os_ota_boot(void)
{
	struct jz_sfcnand_partition_param *partitions;
	unsigned int img_addr = 0;
	int is_kernel2=0;
	const char *kernel_name=CONFIG_SPL_OS_NAME;

	sfc_init();

	partitions = get_partitions();
	img_addr = get_part_offset_by_name(partitions, CONFIG_SPL_OTA_NAME);
	if (img_addr != -1) {
		char buf[128];
		const char kernel2[] = "ota:"CONFIG_SPL_OS_NAME2;
		sfc_nand_load(img_addr, sizeof(buf), (unsigned int)buf);
		buf[sizeof(buf) - 1] = 0;
		if (!strncmp(kernel2, buf, sizeof(kernel2)-1)) {
			is_kernel2 = 1;
			kernel_name=CONFIG_SPL_OS_NAME2;
		}
	}

	img_addr = get_part_offset_by_name(partitions, kernel_name);
	if (img_addr == -1) {
		printf("[ERROR%s:%d] cannot find (%s) partition\n", __func__, __LINE__, kernel_name);
		printf("TODO: check burner spinand partition config\n");
	}

	debug("kernel:%s %x\n", kernel_name, img_addr);

	spl_load_kernel(img_addr);

	if (is_kernel2)
		return CONFIG_SYS_SPL_ARGS_ADDR2;
	else
		return CONFIG_SYS_SPL_ARGS_ADDR;
}
#endif

#ifdef CONFIG_SPL_RTOS_BOOT
static void spl_sfc_rtos_boot(void)
{
	struct rtos_header rtos;

	sfc_init();

	sfc_nand_load(CONFIG_RTOS_OFFSET, sizeof(rtos), (unsigned int)&rtos);
	if (rtos_check_header(&rtos))
		hang();
	sfc_nand_load(CONFIG_RTOS_OFFSET, rtos.img_end - rtos.img_start, rtos.img_start);
	rtos_start(&rtos);
}
#endif

#ifdef CONFIG_SPL_RTOS_OTA_BOOT
static void spl_sfc_rtos_ota_boot(void)
{
	struct rtos_header rtos;
	struct rtos_ota_header ota;
	int id;
	unsigned int os_offset;

	sfc_init();

	sfc_nand_load(CONFIG_RTOS_OTA_TAG_OFFSET, sizeof(ota), (unsigned int)&ota);
	id = rtos_ota_check_header(&ota);
	if (id < 0)
		hang();

	os_offset = id ? CONFIG_RTOS_OTA_OS1_OFFSET : CONFIG_RTOS_OTA_OS0_OFFSET;
	sfc_nand_load(os_offset, sizeof(rtos), (unsigned int)&rtos);
	if (rtos_check_header(&rtos))
		hang();

	sfc_nand_load(os_offset, rtos.img_end - rtos.img_start, rtos.img_start);
	rtos_start(&rtos);
}
#endif

#ifdef CONFIG_OTA_VERSION20
static struct ota_ops ota_ops = {
	.flash_init = sfc_init,
	.flash_read = sfc_nand_load,
	.flash_load_kernel = spl_load_kernel,
	.flash_get_partitions = get_partitions,
	.flash_get_part_offset_by_name = get_part_offset_by_name,
};
#endif

char* spl_sfc_nand_load_image(void)
{
#if defined(CONFIG_OTA_VERSION20)
	register_ota_ops(&ota_ops);
	return spl_ota_load_image();
#elif defined(CONFIG_SPL_OS_OTA_BOOT)
	return spl_sfc_os_ota_boot();
#elif defined(CONFIG_SPL_OS_BOOT)
	spl_sfc_os_boot();
#elif defined(CONFIG_SPL_RTOS_BOOT)
	spl_sfc_rtos_boot();
#elif defined(CONFIG_SPL_RTOS_OTA_BOOT)
	spl_sfc_rtos_ota_boot();
#else
	{
		struct image_header *header;
		header = (struct image_header *)(CONFIG_SYS_TEXT_BASE);

		sfc_init();
		sfc_nand_load(CONFIG_UBOOT_OFFSET, CONFIG_SYS_MONITOR_LEN, CONFIG_SYS_TEXT_BASE);
		spl_parse_image_header(header);
	}
#endif

	return NULL;
}
