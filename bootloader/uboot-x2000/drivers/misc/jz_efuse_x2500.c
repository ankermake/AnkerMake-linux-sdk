#include <common.h>
#include <exports.h>
#include <malloc.h>
#include <linux/types.h>
#include <linux/string.h>
#include <linux/err.h>
#include <asm/io.h>
#include <asm/gpio.h>
#include <asm/errno.h>
#include <asm/arch/base.h>
#include <asm/arch/clk.h>
#include <asm/arch/jz_efuse.h>

#ifdef EFU_NO_REG_OPS
#undef writel
#undef readl
#define writel(b, addr) {;}
#define readl(addr)	0
#endif

static int efuse_debug = 0;
static int efuse_gpio = -1;
static int efuse_en_active = 0;

#define WRITE_EFUSE 0
#define READ_EFUSE 1

#define EFUSE_CTRL		0x0
#define EFUSE_CFG		0x4
#define EFUSE_STATE		0x8
#define EFUSE_DATA		0xC
#define EFUSE_SPEEN     0x10
#define EFUSE_SPESEG    0x14



#define OFFSET_TO_ADDR(addr,i,read_bytes) (addr+(i*read_bytes))

#define CHIP_ID_ADDR (0x00)
#define USER_ID_ADDR (0x18)
#define SARADC_CAL (0x20)
#define TRIM_ADDR (0x24)
#define PROGRAM_PROTECT_ADDR (0x26)
#define CPU_ID_ADDR (0x28)
#define SPECIAL_ADDR (0x2c)
#define CUSTOMER_RESV_ADDR (0x30)
uint32_t seg_addr[] = {
	CHIP_ID_ADDR,
	USER_ID_ADDR,
	SARADC_CAL,
	TRIM_ADDR,
	PROGRAM_PROTECT_ADDR,
	CPU_ID_ADDR,
	SPECIAL_ADDR,
	CUSTOMER_RESV_ADDR,
};

struct efuse_wr_info {
	uint32_t seg_id;
	uint32_t bytes;
	uint32_t offset;
	uint32_t *buf;
};
static uint32_t efuse_readl(uint32_t reg_off)
{
	return readl(EFUSE_BASE + reg_off);
}
static void efuse_writel(uint32_t val, uint32_t reg_off)
{
	writel(val, EFUSE_BASE + reg_off);
}
static int adjust_efuse(void)
{
    uint32_t val, ns;
	int i, rd_strobe, wr_strobe;
	uint32_t rd_adj, wr_adj;
	int h2clk = clk_get_rate(H2CLK);
	ns = 1000000000 / h2clk;
	printf("rate = %d, ns = %d\n", h2clk, ns);
	for(i = 0; i < 0xf; i++)
		if((( i + 1) * ns) > 25)
			break;
	if(i == 0xf) {
		printf("get efuse cfg rd_adj fail!\n");
		return -1;
	}
	rd_adj = i;

	for(i = 0; i < 0xf; i++)
		if(((i + 1) * ns) > 20)
			break;
	if(i == 0xf) {
		printf("get efuse cfg wr_adj fail!\n");
		return -1;
	}
	wr_adj = i;

	for(i = 0; i < 0xf; i++)
		if(((rd_adj + i + 1) * ns) > 20)
			break;
	if(i == 0xf) {
		printf("get efuse cfg rd_strobe fail!\n");
		return -1;
	}
	rd_strobe = i;

	for(i = 1; i < 0xfff; i++) {
		val = (wr_adj + i + 2000) * ns;
		//if( val > (11000 * 1000) && val < (13000 * 1000))
		/*if(val > 10000)*/
		if( val > (9.8 * 1000) && val < (10.2 * 1000))
			break;
	}
	if(i >= 0xfff) {
		printf("get efuse cfg wd_strobe fail!\n");
		return -1;
	}
	wr_strobe = i;

	printf("rd_adj = %d | rd_strobe = %d | "
		 "wr_adj = %d | wr_strobe = %d\n", rd_adj, rd_strobe,
		 wr_adj, wr_strobe);
	/*set configer register*/
	/*val = 1 << 31 | rd_adj << 20 | rd_strobe << 16 | wr_adj << 12 | wr_strobe;*/
	val = rd_adj << 23 | rd_strobe << 18 | wr_adj << 14 | wr_strobe;
	efuse_writel(val, EFUSE_CFG);
    printf("h2clk is %d, efucfg_reg 0x%x\n",h2clk,efuse_readl(EFUSE_CFG));
	return 0;
}
static void boost_vddq(int gpio)
{
	int val;
	printf("boost vddq\n");
	gpio_direction_output(gpio, efuse_en_active);
	do {
		val = gpio_get_value(gpio);
		printf("gpio %d level %d\n",gpio,val);
	} while (val != efuse_en_active);
	mdelay(10);		/*  mdelay(10) wait for EFUSE VDDQ setup. */
}
static void reduce_vddq(int gpio)
{
	int val;
	printf("reduce vddq\n");
	gpio_direction_output(gpio, !efuse_en_active);
	do {
		val = gpio_get_value(gpio);
		printf("gpio %d level %d\n",gpio,val);
	} while (val == efuse_en_active);
	mdelay(10);		/*  mdelay(10) wait for EFUSE VDDQ fall down. */
}
static int jz_efuse_check_arg(uint32_t seg_id, uint32_t bit_num)
{
	if(seg_id == CHIP_ID) {
		if(bit_num > 96) {
			printf("read segment %d data length %d > 96 bit\n", seg_id, bit_num);
			return -1;
		}
	} else if(seg_id == USER_ID) {
		if(bit_num > 32) {
			printf("read segment %d data length %d > 32 bit\n", seg_id, bit_num);
			return -1;
		}
	} else if(seg_id == SARADC_CAL_DAT) {
		if(bit_num > 16) {
			printf("read segment %d data length %d > 16 bit\n", seg_id, bit_num);
			return -1;
		}
	} else if(seg_id == TRIM_DATA) {
		if(bit_num > 8) {
			printf("read segment %d data length %d > 16 bit\n", seg_id, bit_num);
			return -1;
		}
	} else if(seg_id == PROGRAM_PROTECT) {
		if(bit_num > 8) {
			printf("read segment %d data length %d > 8 bit\n", seg_id, bit_num);
			return -1;
		}
	} else if(seg_id == CPU_ID) {
		if(bit_num > 16) {
			printf("read segment %d data length %d > 16 bit\n", seg_id, bit_num);
			return -1;
		}
	} else if(seg_id == SPECIAL_USE) {
		if(bit_num > 16) {
			printf("read segment %d data length %d > 16 bit\n", seg_id, bit_num);
			return -1;
		}
	} else if(seg_id == CUSTOMER_RESV) {
		if(bit_num > 320) {
			printf("read segment %d data length %d > 320 bit\n", seg_id, bit_num);
			return -1;
		}
	} else {
		printf("read segment num is error(0 ~ 7)");
		return -1;
	}
	return 0;

}
int jz_efuse_read(uint32_t seg_id, uint32_t r_bytes, uint32_t offset, uint32_t *buf)
{
	unsigned long flags;
	unsigned int val, addr = 0, bit_num, remainder;
	unsigned int count = r_bytes;
	unsigned char *save_buf = (unsigned char *)buf;
	unsigned int data = 0;
	unsigned int i;
	/* check the bit_num  */

	bit_num = (r_bytes + offset) * 8;
	if (jz_efuse_check_arg(seg_id, bit_num) == -1)
	{
	    printf("efuse arg check error \n");
		return -1;
	}
	/* First word reading */
	//efuse->id2addr = seg_addr;
	addr = (seg_addr[seg_id] + offset) / 4;
	remainder = (seg_addr[seg_id] + offset) % 4;
	efuse_writel(0, EFUSE_STATE);
	val = addr << 21;
	efuse_writel(val, EFUSE_CTRL);
	val |= 1;
	efuse_writel(val, EFUSE_CTRL);

	while(!(efuse_readl(EFUSE_STATE) & 1));
	data = efuse_readl(EFUSE_DATA);

	if ((count + remainder) <= 4) {
		data = data >> (8 * remainder);
		while(count){
			*(save_buf) = data & 0xff;
			data = data >> 8;
			count--;
			save_buf++;
		}
		goto end;
	}else {
		data = data >> (8 * remainder);
		for (i = 0; i < (4 - remainder); i++) {
			*(save_buf) = data & 0xff;
			data = data >> 8;
			count--;
			save_buf++;
		}
	}
	/* Middle word reading */
again:
	if (count > 4) {
		addr++;
		efuse_writel(0, EFUSE_STATE);

		val = addr << 21;
		efuse_writel(val, EFUSE_CTRL);
		val |= 1;
		efuse_writel(val, EFUSE_CTRL);

		while(!(efuse_readl(EFUSE_STATE) & 1));
		data = efuse_readl(EFUSE_DATA);

		for (i = 0; i < 4; i++) {
			*(save_buf) = data & 0xff;
			data = data >> 8;
			count--;
			save_buf++;
		}

		goto again;
	}

	/* Final word reading */
	addr++;
	efuse_writel(0, EFUSE_STATE);

	val = addr << 21;
	efuse_writel(val, EFUSE_CTRL);
	val |= 1;
	efuse_writel(val, EFUSE_CTRL);

	while(!(efuse_readl(EFUSE_STATE) & 1));
	data = efuse_readl(EFUSE_DATA);

	while(count) {
		*(save_buf) = data & 0xff;
		data = data >> 8;
		count--;
		save_buf++;
	}
	efuse_writel(0, EFUSE_STATE);
end:
	return 0;
}
//EXPORT_SYMBOL_GPL(jz_efuse_read);
int jz_efuse_write(uint32_t seg_id, uint32_t w_bytes, uint32_t offset, uint32_t *buf)
{
	unsigned long flags;
	unsigned int val, addr = 0, bit_num, remainder;
	unsigned int count = w_bytes;
	unsigned char *save_buf = (unsigned char *)buf;
	unsigned char data[4] = {0};
	unsigned int i;

	bit_num = (w_bytes + offset) * 8;
	if (jz_efuse_check_arg(seg_id, bit_num) == -1)
	{
		printf("efuse arg check error \n");
		return -1;
	}
	/* First word writing */
	addr = (seg_addr[seg_id] + offset) / 4;
	remainder = (seg_addr[seg_id] + offset) % 4;

	if ((count + remainder) <= 4) {
		for (i = 0; i < remainder; i++)
			data[i] = 0;
		while(count) {
			data[i] = *save_buf;
			save_buf++;
			i++;
			count--;
		}
		while(i < 4) {
			data[i] = 0;
			i++;
		}
		efuse_writel(0x20, EFUSE_CTRL);
		val = data[0] | (data[1] << 8) | (data[2] << 16) | (data[3] << 24);
		efuse_writel(val, EFUSE_DATA);
		val = addr << 21 | 1 << 15 | 1 << 5;
		efuse_writel(val, EFUSE_CTRL);

		//spin_lock_irqsave(&efuse->lock, flags);

		//ef/////
		udelay(10);
		val |= 2;
		efuse_writel(val, EFUSE_CTRL);

		while(!(efuse_readl(EFUSE_STATE) & 2));
		//efuse_vddq_set(0);

		efuse_writel(0, EFUSE_CTRL);
		efuse_writel(0, EFUSE_STATE);

		//spin_unlock_irqrestore(&efuse->lock, flags);

		goto end;
	}else {
		for (i = 0; i < remainder; i++)
			data[i] = 0;
		for (i = remainder; i < 4; i++) {
			data[i] = *save_buf;
			save_buf++;
			count--;
		}
		efuse_writel(0x20, EFUSE_CTRL);
		val = data[0] | (data[1] << 8) | (data[2] << 16) | (data[3] << 24);
		efuse_writel(val, EFUSE_DATA);
		val = addr << 21 | 1 << 15 | 1 << 5;
		efuse_writel(val, EFUSE_CTRL);

		//spin_lock_irqsave(&efuse->lock, flags);

		//efuse_vddq_set(1);
		udelay(10);
		val |= 2;
		efuse_writel(val, EFUSE_CTRL);

		while(!(efuse_readl(EFUSE_STATE) & 2));
		//efuse_vddq_set(0);

		efuse_writel(0, EFUSE_CTRL);
		efuse_writel(0, EFUSE_STATE);

		//spin_unlock_irqrestore(&efuse->lock, flags);
	}
	/* Middle word writing */
again:
	if (count > 4) {
		addr++;
		for (i = 0; i < 4; i++) {
			data[i] = *save_buf;
			save_buf++;
			count--;
		}
		efuse_writel(0x20, EFUSE_CTRL);
		val = data[0] | (data[1] << 8) | (data[2] << 16) | (data[3] << 24);
		efuse_writel(val, EFUSE_DATA);
		val = addr << 21 | 1 << 15 | 1 << 5;
		efuse_writel(val, EFUSE_CTRL);

		//spin_lock_irqsave(&efuse->lock, flags);

		//efuse_vddq_set(1);
		udelay(10);
		val |= 2;
		efuse_writel(val, EFUSE_CTRL);

		while(!(efuse_readl(EFUSE_STATE) & 2));
		//efuse_vddq_set(0);

		efuse_writel(0, EFUSE_CTRL);
		efuse_writel(0, EFUSE_STATE);

		//spin_unlock_irqrestore(&efuse->lock, flags);

		goto again;
	}

	/* Final word writing */
	addr++;
	for (i = 0; i < 4; i++) {
		if (count) {
			data[i] = *save_buf;
			save_buf++;
			count--;
		}else {
			data[i] = 0;
		}
	}
	efuse_writel(0x20, EFUSE_CTRL);
	val = data[0] | (data[1] << 8) | (data[2] << 16) | (data[3] << 24);
	efuse_writel(val, EFUSE_DATA);
	val = addr << 21 | 1 << 15 | 1 << 5;
	efuse_writel(val, EFUSE_CTRL);

	//spin_lock_irqsave(&efuse->lock, flags);
	//efuse_vddq_set(1);
	udelay(10);
	val |= 2;
	efuse_writel(val, EFUSE_CTRL);

	while(!(efuse_readl(EFUSE_STATE) & 2));
	//efuse_vddq_set(0);

	//spin_unlock_irqrestore(&efuse->lock, flags);

	efuse_writel(0, EFUSE_CTRL);
	efuse_writel(0, EFUSE_STATE);
end:
	save_buf = NULL;

	return 0;
}
void special_segment_efuse_read(unsigned int* value)
{
	unsigned int val = 0, addr = 0;

	efuse_writel(0, EFUSE_STATE);

	val = 0xa55aa55a;
	efuse_writel(val, EFUSE_SPEEN);

	/* x2500 address needs to be changed,aligned to word */
	addr = SPECIAL_ADDR / 4;
	val = addr << 21;
	efuse_writel(val, EFUSE_CTRL);

	val |= 1;
	efuse_writel(val, EFUSE_CTRL);

	while(!(efuse_readl(EFUSE_STATE) & 0x1));
	*value = efuse_readl(EFUSE_DATA);
}
//EXPORT_SYMBOL_GPL(jz_efuse_write);
void special_segment_efuse_write(unsigned int value)
{
	unsigned int val;
	unsigned long flags;
	unsigned int addr = 0;

	boost_vddq(efuse_gpio);
	efuse_writel(0, EFUSE_STATE);

	val = 0xa55aa55a;
	efuse_writel(val, EFUSE_SPEEN);

	efuse_writel(0x20, EFUSE_CTRL);
	val = value;
	efuse_writel(val, EFUSE_DATA);

	/* x2500 address needs to be changed,aligned to word */
	addr = SPECIAL_ADDR / 4;
	val = addr << 21 | 1 << 15 | 1 << 5;
	efuse_writel(val, EFUSE_CTRL);

	//spin_lock_irqsave(&efuse->lock, flags);
	//efuse_vddq_set(1);
	udelay(10);

	val |= 1 << 1;
	efuse_writel(val, EFUSE_CTRL);

	while(!(efuse_readl(EFUSE_STATE) & 0x2));

	//efuse_vddq_set(0);
	//spin_unlock_irqrestore(&efuse->lock, flags);

	efuse_writel(0, EFUSE_CTRL);
	efuse_writel(0, EFUSE_STATE);
	efuse_writel(0, EFUSE_SPEEN);

	special_segment_efuse_read(&val);
	reduce_vddq(efuse_gpio);
}
static int  efuse_read_user_id_proc(void)
{
	int len = 4;
	unsigned char buf[4]={1,2,3,4};
	jz_efuse_read(1, len, 0,(uint32_t *)buf);
        printf("%02x%02x%02x%02x\n",buf[0],buf[1],buf[2],buf[3]);
	return buf;
}
static int efuse_write_custer_test(void)
{
        int len = 2;
	unsigned char buf[2]={99,99};
	jz_efuse_write(7, len, 32,(uint32_t *)buf);
        // printf("%02x%02x%02x%02x\n",buf[0],buf[1]);
	return buf;
}
static int  efuse_read_custer()
{
	int len = 2;
	unsigned char buf[2]={1,2};
	jz_efuse_read(7, len, 32,(uint32_t *)buf);
        printf("%02x%02x\n",buf[0],buf[1]);
	return buf;
}
int efuse_read_id(void *buf, uint32_t length, uint32_t seg_id)
{
	int len = 2;
	unsigned char bef[2]={1,2};
	jz_efuse_read(seg_id, len, 34,(uint32_t *)bef);
	//printf("***********************************************************************testtest\n");
        printf("%02x%02x\n",bef[0],bef[1]);
	return 0;
}
/*
 *String to hexadecimal number, burning tool input through/ drivers/usb/gadget/cloner/cloner_ module_ The data
 *passed from the write function in eFuse is a string, so it needs to be converted to hexadecimal
*/
static int convert_string2hex(unsigned char *in_data, int in_data_len, unsigned char *out_data, int *out_data_len)
{
       int i;
       int loop_count;
       int convert_point = 0;
       int mem_point = 0;
       unsigned char convert_result;
       unsigned char temp[3] = {0};
       /* Check parameter validity */
       if (in_data == NULL || in_data_len <= 0 || out_data == NULL || out_data_len == NULL || (in_data_len % 2) != 0) {
               printf("invalid parameters\n");
               return -1;
       }
	/* Determine whether the hexadecimal range is exceeded 0 ~ F */
       for (i = 0; i < in_data_len; i++) {
       if ((in_data[i] < '0') || (in_data[i] > 'f') || ((in_data[i] > '9') &&(in_data[i] < 'A'))) {
              printf("out of range\n");
              return -1;
           }
       }
       loop_count = in_data_len / 2;
       memset(out_data, 0x00, *out_data_len);
       *out_data_len = 0;

       for (i = 0; i < loop_count; i++) {
              memset(temp, 0x00, sizeof(temp));
              memcpy(temp, in_data + convert_point, 2);
              convert_point += 2;
              convert_result = simple_strtoull(temp, NULL, 16);
              memcpy(out_data + mem_point, &convert_result, sizeof(unsigned char));
              mem_point += sizeof(unsigned char);
              *out_data_len += sizeof(unsigned char);
       }
       return 0;
}
/*
 *buf is the input of the burning tool
 *length is the length of the input string
 *seg_id is segmentid such as chip_id
 * Subsequent interface parameters may change
 *See for details ./drivers/usb/gadget/cloner/cloner_module_efuse.c write read function
*/
int efuse_write(void *buf, int length, int seg_id)
{
       //efuse_write_custer_test();
       int len = length;
       unsigned char in_data[len];
       int in_data_len = len;
       unsigned char out_data[len];
       int out_data_len = len;
       /*input buf(type string) save to array*/
       memcpy(in_data,(unsigned char *)buf,len);
       in_data_len = strlen(in_data);
       /*input buf(type string) change to hex*/
       convert_string2hex(in_data, in_data_len, out_data, &out_data_len);
       jz_efuse_write(seg_id, len, 34,(uint32_t *)out_data);
       return 0;
}
int efuse_init(int gpio_pin,int active)
{
      if(gpio_pin >= 0){
		if(efuse_gpio >= 0) gpio_free(efuse_gpio);
		efuse_gpio = gpio_request(gpio_pin, "VDDQ");
		if(efuse_gpio < 0) return efuse_gpio;
		efuse_en_active = active;
	}else{
		efuse_gpio = -1;
	}
	if(adjust_efuse() < 0)
		return -1;
	return 0;
}
