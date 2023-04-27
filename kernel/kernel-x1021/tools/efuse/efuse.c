#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/ioctl.h>
#include <fcntl.h>
#include <string.h>


/* ************************************************************* */
/* keep sync with kernel/drivers/misc/jz_efuse_v13.c */

#define DRV_NAME "jz-efuse-v13"

#define CMD_GET_CHIPID  _IOW('E', 0, unsigned int)
#define CMD_GET_RANDOM  _IOW('E', 1, unsigned int)
#define CMD_GET_USERID  _IOR('E', 2, unsigned int)
#define CMD_GET_PROTECTID  _IOR('E', 3, unsigned int)
#define CMD_WRITE       _IOR('E', 4, unsigned int)
#define CMD_READ        _IOR('E', 5, unsigned int)

enum segment_id {
	CHIP_ID,		/* 16 Bytes (read only) */
	RANDOM_ID,		/* 16 Bytes (read only) */
	USER_ID,		/* 16 Bytes (read, write) */
	TRIM3_ID,		/*  2 Bytes (read only) */
	PROTECT_ID,		/*  2 Bytes (read, write) */
};

#define USERID_PRT_BIT (1 << 7)




typedef unsigned int __u32;
typedef		__u32		uint32_t;

struct efuse_wr_info {
	uint32_t seg_id;
	uint32_t data_length;
	uint32_t *buf;		/* 4bytes aligned */
};
/* ************************************************************* */


#define EFUSE_DEV	"/dev/"DRV_NAME

uint32_t buf[8] = {0};		/* 4bytes aligned */
uint32_t user_id[8] = {0};		/* 4bytes aligned */

/* length in byte */
static int dump_buf_in_hexl_mode(unsigned char *buf, int length)
{
	int iii;
	for(iii=0;iii<length;iii++)
		printf(" %02x", (unsigned char)*buf++);\

	return length;
}

/* length in byte */
static int merge_old_new_user_id(unsigned char *old, unsigned char *new, int length)
{
	int iii;
	for(iii=0;iii<length;iii++) {
		*old++ |= *new++; /* merge old and new */
	}
	return length;
}

/* length in byte */
static int create_user_id(unsigned char *id, int length, int offset)
{
	int iii;
	for(iii=0;iii<length;iii++) {
		//*id++ = 1<<((iii+offset)%8);
		*id++ = 1<<((offset)%8);
	}
	return length;
}

static int usage(int argc, char * argv[])
{
	printf("-------------------------------------\n");
	printf(" %s [id offset] [protect]\n", argv[0]);
	printf("-------------------------------------\n");
	printf("\n");

	return 0;
}

int main(int argc, char * argv[])
{
	int write_userid, write_protect;
	int offset;
	int ret;
	int efuse_fd;
	struct efuse_wr_info wr;

	usage(argc, argv);

	write_userid = 0;
	write_protect = 0;


	if ( argc > 1 ) {
		write_userid = 1;
		offset = strtol(argv[1], NULL, 0);
	}

	if ( argc > 2 ) {
		write_protect = 1;
		//offset = strtol(argv[1], NULL, 0);
	}

	efuse_fd = open(EFUSE_DEV, O_RDWR);
	if (efuse_fd<0) {
		printf("open %s failed, return %d\n", EFUSE_DEV, efuse_fd);
		return -1;
	}

	/* CMD_GET_PROTECTID */
	memset((void*)&buf[0], 0, 16);
	wr.seg_id = PROTECT_ID;
	wr.data_length = 2;
	wr.buf = &buf[0];

	ret = ioctl(efuse_fd, CMD_READ, &wr);
	printf("PROTECT_ID:\n");
	dump_buf_in_hexl_mode((unsigned char *)&buf[0], 2);
	printf("\nPROTECTID: %04x\n", buf[0]);


	/* CMD_GET_CHIPID */
	memset((void*)&buf[0], 0, 16);
	wr.seg_id = CHIP_ID;
	wr.data_length = 16;
	wr.buf = &buf[0];

	ret = ioctl(efuse_fd, CMD_READ, &wr);
	printf("CMD_GET_CHIPID:\n");
	dump_buf_in_hexl_mode((unsigned char *)&buf[0], 16);
	printf("\n");

	/* CMD_GET_RANDOM */
	memset((void*)&buf[0], 0, 16);
	wr.seg_id = RANDOM_ID;
	wr.data_length = 16;
	wr.buf = &buf[0];

	ret = ioctl(efuse_fd, CMD_READ, &wr);
	printf("CMD_GET_RANDOM:\n");
	dump_buf_in_hexl_mode((unsigned char *)&buf[0], 16);
	printf("\n");

	if ( write_userid ) {
		/* CMD_GET_USERID */
		memset((void*)&buf[0], 0, 16);
		wr.seg_id = USER_ID;
		wr.data_length = 16;
		wr.buf = &buf[0];

		ret = ioctl(efuse_fd, CMD_READ, &wr);
		printf("CMD_GET_USERID:\n");
		dump_buf_in_hexl_mode((unsigned char *)&buf[0], 16);
		printf("\n");

		/* ************************************************** */

		/* Write USER_ID */
		memset((void*)&buf[0], 0, 16);
		create_user_id((unsigned char *)&buf[0], 8, offset);
		//merge_old_new_user_id((unsigned char *)&buf[0], (unsigned char *)&user_id[0], 16);
		wr.seg_id = USER_ID;
		wr.data_length = 16;
		wr.buf = &buf[0];

		printf("WRITE USERID:\n");
		dump_buf_in_hexl_mode((unsigned char *)&buf[0], 16);
		printf("\n");
		ret = ioctl(efuse_fd, CMD_WRITE, &wr);
		if (ret<0)
			printf("CMD_WRITE USER_ID: return failed(%d)\n", ret);
		printf("WRITE USERID:\n");
		dump_buf_in_hexl_mode((unsigned char *)&buf[0], 16);
		printf("\n");
	}

	/* CMD_GET_USERID */
	memset((void*)&buf[0], 0, 16);
	wr.seg_id = USER_ID;
	wr.data_length = 16;
	wr.buf = &buf[0];

	ret = ioctl(efuse_fd, CMD_READ, &wr);
	printf("CMD_GET_USERID:\n");
	dump_buf_in_hexl_mode((unsigned char *)&buf[0], 16);
	printf("\n");



	/* Write PROTECT_ID. 
	   NOTE: protect bit write only one chance
	*/
	if ( write_protect ) {
		int protect;

		protect = USERID_PRT_BIT;
		wr.seg_id = PROTECT_ID;
		wr.data_length = 2;
		wr.buf = &protect;

		printf("WRITE PROTECT_ID:\n");
		dump_buf_in_hexl_mode((unsigned char *)&protect, 2);
		printf("\n");
		ret = ioctl(efuse_fd, CMD_WRITE, &wr);
	}




	return 0;
}




/* 
# cat /proc/jz/efuse/efuse_chip_id 
--------> chip id(in 4 bytes): 45e94d2d1f51080700000000845b940f

# /efuse 0
PROTECT_ID:
 40 c0
PROTECTID: c040
CMD_GET_CHIPID:
 2d 4d e9 45 07 08 51 1f 00 00 00 00 0f 94 5b 84
CMD_GET_RANDOM:
 43 06 94 72 aa 3e 58 3d b8 e7 8c 6f 0b 18 34 54
CMD_GET_USERID:
 07 0e 16 2e 56 a6 47 86 07 0e 16 2e 56 a6 47 86
WRITE USERID:
 01 01 01 01 01 01 01 01 01 01 01 01 01 01 01 01
WRITE USERID:
 07 0f 17 2f 57 a7 47 87 07 0f 17 2f 57 a7 47 87
CMD_GET_USERID:
 07 0f 17 2f 57 a7 47 87 07 0f 17 2f 57 a7 47 87
# 

 */
