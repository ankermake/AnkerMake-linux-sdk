/*
 * drivers/spi/spi-im501.c
 * (C) Copyright 2014-2018
 * Fortemedia, Inc. <www.fortemedia.com>
 * chenpailin <fuli@fortemedia.com>
 *
 * some simple description for this code
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation; either version 2 of
 * the License, or (at your option) any later version.
 *
 */
#include <linux/init.h>
#include <linux/module.h>
#include <linux/list.h>
#include <linux/input.h>
#include <linux/spi/spi.h>
#include <linux/device.h>
#include <linux/firmware.h>
#include <linux/platform_device.h>
#include <linux/delay.h>
#include <linux/atomic.h>
#include <linux/irq.h>
#include <linux/interrupt.h>
#include <linux/slab.h>
#include <linux/gpio.h>
#include <linux/sched.h>
#include <linux/kthread.h>
#include <linux/uaccess.h>
#include <linux/miscdevice.h>
#include <linux/regulator/consumer.h>
#include <linux/pm_qos.h>
#include <linux/sysfs.h>
#include <linux/clk.h>
#include <linux/kfifo.h>
#include <linux/time.h>
#include <linux/rtc.h>
#include <linux/wait.h>
#include <sound/core.h>
#include <sound/pcm.h>
#include <sound/pcm_params.h>
#include <sound/soc.h>
#include <sound/soc-dapm.h>
#include <sound/initval.h>
#include <sound/tlv.h>
#include <asm/unistd.h>
#include "spi-im501.h"
#include "im501.h"

static DECLARE_WAIT_QUEUE_HEAD(wakeup_work_queue);
static LIST_HEAD(file_pdata_list);
static DEFINE_SPINLOCK(file_pdata_spinlock);

struct file_private_data {
	struct list_head entry;
	struct file * file;
	int available;
};

#if defined(CUBIETRUCK)
#include <mach/sys_config.h>
#elif defined(INGENIC)
#include "im501_priv.h"
#define FIRMWARE_DIR "/lib/firmware/im501"
#else
#include <mach/irqs.h>
#include <linux/of_gpio.h>
#include <linux/hrtimer.h>
#endif


//#define SHOW_DL_TIME
#ifdef SHOW_DL_TIME
struct timex  txc;
struct timex  txc2;
struct timeval tv;
struct rtc_time rt;
#endif

//Fuli 20161215 define spi speed as constant
#define SPI_LOW_SPEED	1000000
#define SPI_HIGH_SPEED	6000000
//
#define FW_RD_CHECK
#define FW_BURST_RD_CHECK		
//#define FW_MEM_RD_CHECK		
	
#define MAX_KFIFO_BUFFER_SIZE		(131072*2) /* >4 seconds */
#ifndef INGENIC
#define CODEC2IM501_PDM_CLKI
#endif

//fuli 20170629 for new protocol of oneshot
#define SM501
//

voice_buf  voice_buf_data;
int im501_dsp_mode_old = -1;
int im501_vbuf_trans_status = 0;
int ap_sleep_flag = 0;
static struct spi_device	*im501_spi;
int current_firmware_type = WAKEUP_FIRMWARE;
u8	firmware_type = 0; //0: wakeup firmware; 1: ultrasound firmware

#define CDEV_COUNT 1
#define IM501_CDEV_NAME "fm_smp"
#define REC_FILE_PATH 			"/sdcard/im501_oneshot.pcm"
#define SPI_REC_FILE_PATH 		"/sdcard/im501_spi_rec.pcm"

struct im501_data_t {
	struct mutex	lock;
	struct mutex	spi_op_lock;
	dev_t			record_chrdev;
	struct cdev		record_cdev;
	struct device	*record_dev;
	struct class	*cdev_class;
	atomic_t		audio_owner;
	int				pdm_clki_gpio;
	int				reset_gpio;
};
struct im501_data_t *im501_data;

int *im501_idx = NULL;
int im501_irq;
static struct work_struct		im501_irq_work;
static struct workqueue_struct	*im501_irq_wq;
static int im501_host_irqstatus = 0;	// To notify the HAL about the incoming irq.
//Fuli 20161216 resolve the issue about can only invoke interupt once
static void im501_irq_handling_work(struct work_struct *work);
//

//Fuli 20170922 to support SPI recording
static char im501_spi_record_started = 0;
struct file *fpdata = NULL;
mm_segment_t oldfs;
unsigned int output_counter = 0;
//

int check_dsp_status(void);
unsigned char request_enter_normal(void);

int im501_spi_read_reg(u8 reg, u8 *val)
{
	struct spi_message message;
	struct spi_transfer x[2];
	int status;
	u8 write_buf[2];
	u8 read_buf[1];

	write_buf[0] = IM501_SPI_CMD_REG_RD;
	write_buf[1] = reg & 0xff;

	spi_message_init(&message);
	memset(x, 0, sizeof(x));

	x[0].len = 2;
	x[0].tx_buf = write_buf;
	spi_message_add_tail(&x[0], &message);

	x[1].len = 1;
	x[1].rx_buf = read_buf;
	spi_message_add_tail(&x[1], &message);

	status = spi_sync(im501_spi, &message);

	*val = read_buf[0];

	return status;
}
EXPORT_SYMBOL_GPL(im501_spi_read_reg);

int im501_spi_write_reg(u8 reg, u8 val)
{
	int status;
	u8 write_buf[3];

	write_buf[0] = IM501_SPI_CMD_REG_WR;
	write_buf[1] = reg;
	write_buf[2] = val;

	status = spi_write(im501_spi, write_buf, sizeof(write_buf));

	if (status)
		dev_err(&im501_spi->dev, "%s error %d\n", __func__, status);

	return status;
}
EXPORT_SYMBOL_GPL(im501_spi_write_reg);

int im501_spi_read_dram(u32 addr, u8 *pdata)
{
	struct spi_message message;
	struct spi_transfer x[2];
	int status, i;
    u8 write_buf[6];
    u8 read_buf[4];
    u8 spi_cmd = IM501_SPI_CMD_DM_RD, addr_msb;
    
	addr_msb = (addr >> 24) & 0xff;
	if (addr_msb == 0x10) {
		spi_cmd = IM501_SPI_CMD_IM_RD;
	}
	else if (addr_msb == 0xF) {
		spi_cmd = IM501_SPI_CMD_DM_RD;
	}
		
    write_buf[0] = spi_cmd;
    write_buf[1] = addr & 0xFF;
    write_buf[2] = (addr>>8) & 0xFF;
    write_buf[3] = (addr>>16) & 0xFF;
    write_buf[4] = 2;
    write_buf[5] = 0;    

	spi_message_init(&message);
	memset(x, 0, sizeof(x));

	x[0].len = 6;
	x[0].tx_buf = write_buf;
	spi_message_add_tail(&x[0], &message);

	x[1].len = 4;
	x[1].rx_buf = read_buf;
	spi_message_add_tail(&x[1], &message);

	status = spi_sync(im501_spi, &message);

	for (i = 0; i < 4; i++) {
		*(pdata + i) = *(read_buf + i);
	}

    return status;
}
EXPORT_SYMBOL_GPL(im501_spi_read_dram);

int im501_spi_write_dram(u32 addr, u8 *pdata)
{
    int status;
    u8 buf[10];
    
    buf[0] =  IM501_SPI_CMD_DM_WR;
    buf[1] =  addr & 0xFF;
    buf[2] =  (addr>>8) & 0xFF;
    buf[3] =  (addr>>16) & 0xFF;
    buf[4] =  2; //2 words
    buf[5] =  0;
    
    buf[6] =  *pdata;
    buf[7] =  *(pdata + 1);
    buf[8] =  *(pdata + 2);
    buf[9] =  *(pdata + 3);

	status = spi_write(im501_spi, buf, sizeof(buf));
    
    return status;
    
}
EXPORT_SYMBOL_GPL(im501_spi_write_dram);

/**
 * im501_spi_burst_read_dram - Read data from SPI by im501 dsp memory address.
 * @addr: Start address.
 * @rxbuf: Data Buffer for reading.
 * @len: Data length, it must be a multiple of 8.
 *
 * Returns true for success.
 */
int im501_spi_burst_read_dram(u32 addr, u8 *rxbuf, size_t len)
{
	int status;
	u8 spi_cmd = IM501_SPI_CMD_DM_RD, addr_msb;
	u8 write_buf[6];
	unsigned int end, offset = 0;

	struct spi_message message;
	struct spi_transfer x[2];

	addr_msb = (addr >> 24) & 0xff;
	if (addr_msb == 0x10) {
		spi_cmd = IM501_SPI_CMD_IM_RD;
	}
	else if (addr_msb == 0xF) {
		spi_cmd = IM501_SPI_CMD_DM_RD;
	}
	
	while (offset < len) {
		if (offset + IM501_SPI_BUF_LEN <= len)
			end = IM501_SPI_BUF_LEN;
		else
			end = len % IM501_SPI_BUF_LEN;

		write_buf[0] = spi_cmd;
		write_buf[1] = ((addr + offset) & 0x000000ff) >> 0;
		write_buf[2] = ((addr + offset) & 0x0000ff00) >> 8;
		write_buf[3] = ((addr + offset) & 0x00ff0000) >> 16;
		write_buf[4] = (end >> 1) & 0xff;
		write_buf[5] = (end >> (1 + 8)) & 0xff;
		
		spi_message_init(&message);
		memset(x, 0, sizeof(x));

		x[0].len = 6;
		x[0].tx_buf = write_buf;
		spi_message_add_tail(&x[0], &message);

		x[1].len = end;
		x[1].rx_buf = rxbuf + offset;
		spi_message_add_tail(&x[1], &message);

		status = spi_sync(im501_spi, &message);
		
		if (status) {
			pr_err("%s: something wrong in spi_sync. going to return failure\n", __func__);
			
			return false;
		}
		
		offset += IM501_SPI_BUF_LEN;
	}

	return 0;
}
EXPORT_SYMBOL_GPL(im501_spi_burst_read_dram);

//Fuli 20161215 extract a invert copy function for burst write
int im501_8byte_invert_copy(u8* source, u8* target) {
	int i = 0;
	
	if(source == NULL || target == NULL) return -1;
	
	for(i = 0; i < 8; i++) {
		target[7 - i] = source[i];
	}
	
	return 0;	
}
//

//Fuli 20161215 extract a copy function for burst write
int im501_8byte_copy(u8* source, u8* target) {
	int i = 0;
	
	if(source == NULL || target == NULL) return -1;
	
	for(i = 0; i < 8; i++) {
		target[i] = source[i];
	}
	
	return 0;	
}
//

/**
 * im501_spi_burst_write - Write data to SPI by im501 dsp memory address.
 * @addr: Start address.
 * @txbuf: Data Buffer for writng.
 * @len: Data length, it must be a multiple of 8.
 * @type: Firmware type, MSB and LSB, the iM501 firmware is in MSB, but the EFT firmware is in LSB.
 *
 * Returns true for success.
 */
int im501_spi_burst_write(u32 addr, const u8 *txbuf, size_t len, int fw_type)
{
	u8 spi_cmd, *write_buf, *local_buf;
	u32 offset = 0;
	int i, local_len, end, status;

	spi_cmd = (addr == 0x10000000) ? IM501_SPI_CMD_IM_WR : IM501_SPI_CMD_DM_WR;
	
	local_len = (len % 8) ? (((len / 8) + 1) * 8) : len;
	local_buf = kzalloc(local_len, GFP_KERNEL);
	if (local_buf == NULL)
		return -ENOMEM;

	memset(local_buf, 0, local_len);
	memcpy(local_buf, txbuf, len);

	write_buf = kzalloc(IM501_SPI_BUF_LEN + 6, GFP_KERNEL);
	if (write_buf == NULL)
		return -ENOMEM;
	
	write_buf[0] = spi_cmd; //Assign the command byte first, since it is fixed.
	while (offset < local_len) {
		if (offset + IM501_SPI_BUF_LEN <= local_len) {
			end = IM501_SPI_BUF_LEN;
		}
		else {
			end = local_len % IM501_SPI_BUF_LEN;
		}	

		write_buf[1] = ((addr + offset) & 0x000000ff) >> 0;		//The memory address
		write_buf[2] = ((addr + offset) & 0x0000ff00) >> 8;
		write_buf[3] = ((addr + offset) & 0x00ff0000) >> 16;
		write_buf[4] = ((end / 2) & 0x00ff) >> 0;				//The word counter low byte.
		write_buf[5] = ((end / 2) & 0xff00) >> 8;				//Assign the high order word counter, since it is fixed.

		if (fw_type == IM501_DSP_FW) {
			for (i = 0; i < end; i += 8) {
				im501_8byte_invert_copy(local_buf + offset + i, write_buf + i + 6);
			}
		} else if (fw_type == IM501_EFT_FW) {
			for (i = 0; i < end; i += 8) {
				im501_8byte_copy(local_buf + offset + i, write_buf + i + 6);
			}
		}
		
		status = spi_write(im501_spi, write_buf, end + 6);

		if (status) {
			dev_err(&im501_spi->dev, "%s error %d\n", __func__, status);
			kfree(write_buf);
			kfree(local_buf);
			return status;
		}

		offset += IM501_SPI_BUF_LEN;
	}

	kfree(write_buf);
	kfree(local_buf);

	return 0;
}
EXPORT_SYMBOL_GPL(im501_spi_burst_write);

//read frame counter to check DSP is working or not
int check_dsp_status(void) {
	u32 framecount1, framecount2;
	int err = ESUCCESS;
	
	err = im501_spi_read_dram(TO_DSP_FRAMECOUNTER_ADDR, (u8*)&framecount1);
	if( err != ESUCCESS ){ 
		pr_err("%s: call im501_spi_read_dram() return error(%d)\n", __func__, err);
		return err;
	}

	mdelay(500);
	
	err = im501_spi_read_dram(TO_DSP_FRAMECOUNTER_ADDR, (u8*)&framecount2);
	if( err != ESUCCESS ){ 
		pr_err("%s: call im501_spi_read_dram() return error(%d)\n", __func__, err);
		return err;
	}

	framecount1 = framecount1 >> 16;
	framecount2 = framecount2 >> 16;
	pr_err("%s: framecount1=%x, framecount2=%x\n", __func__, framecount1, framecount2);
	if(framecount1 != framecount2) {
		pr_err("%s: im501 is working normally.\n", __func__);
	}
	else {
		pr_err("%s: im501 is NOT working.\n", __func__);
		return EDSPNOTWORK;
	}		

	return ESUCCESS;
}
  
/**
 * This function should be implemented by customers as a callback function.
 * The following is just a sample to control the PDM_CLKI on/off through UIF.
 * The parameter @pdm_clki_rate is reserved for future use.
 */
int codec2im501_pdm_clki_set(bool set_flag, u32 pdm_clki_rate)
{
#ifndef INGENIC
	if(set_flag == NORMAL_MODE) {
		pr_err("%s: set the pdm_clki with gpio_number = %d\n", __func__, im501_data->pdm_clki_gpio);
		gpio_set_value(im501_data->pdm_clki_gpio, 1);
		mdelay(10);
	} else {
		pr_err("%s: unset the pdm_clki with gpio_number = %d\n", __func__, im501_data->pdm_clki_gpio);
		gpio_set_value(im501_data->pdm_clki_gpio, 0);
		mdelay(10);
	}
#endif
	return 0;	
}

int im501_reset(void)
{
	if(im501_data->reset_gpio == -1) return 0;
	
	pr_err("%s: reset im501 with gpio_number = %d\n", __func__, im501_data->reset_gpio);

	//pr_err("%s: set reset pin to high.\n", __func__);
	//gpio_set_value(im501_data->reset_gpio, 1);
	//mdelay(200);
	pr_err("%s: set reset pin to low.\n", __func__);
	gpio_set_value(im501_data->reset_gpio, 0);
	mdelay(200);
	pr_err("%s: set reset pin to high.\n", __func__);
	gpio_set_value(im501_data->reset_gpio, 1);
	mdelay(200);

	return 0;	
}

unsigned char fetch_voice_data( unsigned int start_addr, unsigned int data_length )
{
    unsigned char err = ESUCCESS;
    unsigned char *pbuf;
    unsigned int  i,a,b;
    unsigned int  read_size = 2048;	
    //int fifo_len;
   
#ifdef SHOW_DL_TIME
	do_gettimeofday(&(txc.time));
#endif
    a = data_length / read_size ;
    b = data_length % read_size ;
    
	//pr_err("%s: data_length=%d, a=%d, b=%d\n", __func__, data_length, a, b);			
	pbuf = (unsigned char *) kzalloc(read_size, GFP_KERNEL);
	if (!pbuf) {
		pr_err("%s: pbuf allocation failure.\n", __func__);
		return -1;
	}
    
    for( i = 0; i < a; i++ ) {   
        err = im501_spi_burst_read_dram( start_addr, pbuf,  read_size ); 
        if( err != NO_ERR ){ 
			if (pbuf) kfree(pbuf);
			pr_err("%s: im501_spi_burst_read_dram() failure.\n", __func__);
			return err;
        }
        
        start_addr +=  read_size;

		if(fpdata != NULL) {
			fpdata->f_op->write(fpdata, (u8*)(pbuf), read_size, &fpdata->f_pos);
		}
		else {
			pr_err("%s: open data file failed.\n", __func__);
		}
    }
    
    if( b > 0 ) {
        err = im501_spi_burst_read_dram( start_addr,  pbuf,  b ); 
        if( err != ESUCCESS ){ 
			pr_err("%s: im501_spi_burst_read_dram failure with %d.\n", __func__, err);
			if (pbuf) kfree(pbuf);
            return err;
        }

		if(fpdata != NULL) {
			fpdata->f_op->write(fpdata, (u8*)(pbuf), b, &fpdata->f_pos);
		}
		else {
			pr_err("%s: open data file failed.\n", __func__);
		}
    }
    
    if (pbuf) kfree(pbuf);
#ifdef SHOW_DL_TIME
	do_gettimeofday(&(txc2.time));
	pr_err("%s: get one bank used %ld us\n", __func__, 
			(unsigned long)((unsigned long)(txc2.time.tv_sec - txc.time.tv_sec) * 1000000L + 
							(unsigned long)(txc2.time.tv_usec - txc.time.tv_usec)));
#endif
    return err;
    
}
EXPORT_SYMBOL_GPL(fetch_voice_data);

void openFileForWrite(unsigned char* filePath) {
	oldfs = get_fs();
	set_fs(KERNEL_DS);
	if(fpdata != NULL) {
		filp_close(fpdata, NULL);
		fpdata = NULL;
	}
	fpdata 	= filp_open(filePath, O_CREAT | O_RDWR, 0666);
}

void closeDataFile(void) {
	if(fpdata != NULL) {
		filp_close(fpdata, NULL);
		fpdata = NULL;
	}
	set_fs(oldfs);
}

unsigned char parse_to_host_command( to_host_cmd cmd )
{
	unsigned char err = ESUCCESS;
	unsigned int address = HW_VOICE_BUF_START;
	u8 *pdata;
	//u32 pdm_clki_rate = 2 * 1000 * 1000;
	u32 spi_speed = SPI_HIGH_SPEED;
	int ret;
	//fuli 20170629 for new protocol
	int bank, sync_word_pos, data_size = HW_VOICE_BUF_BANK_SIZE;
	//
	
	//pr_err("%s: cmd_byte = %#x\n", __func__, cmd.cmd_byte);
	if((im501_vbuf_trans_status == 1) && (cmd.status == 1) && (cmd.cmd_byte == TO_HOST_CMD_DATA_BUF_RDY)) { //Reuest host to read To-Host Buffer-Fast
		//pr_err("%s: data ready\n", __func__);
#ifdef SM501 //fuli 20170629 the protocol is different from old version
		data_size = ((cmd.attri >> 13) & 0x7FF) << 1; //16 bits sample, one sample occupies 2 bytes.
		bank = (cmd.attri >> 11) & 0x03;
		sync_word_pos = cmd.attri & 0x7FF;
		if((bank == BANK0) || (bank == BANK0_SYNC)) {
			address = HW_VOICE_BUF_BANK0;
		}
		else if((bank == BANK1) || (bank == BANK1_SYNC)) {
			address = HW_VOICE_BUF_BANK1;
		}
#else //im502BE
		//fuli 20170629 no use		voice_buf_data.index   = (cmd.attri >>8) & 0xFFFF;  //package index
		if( (cmd.attri & 0xFF) == 0xF0 ) {
			address = HW_VOICE_BUF_START; //BANK0 address
		} else if( (cmd.attri & 0xFF) == 0xF9 ) {
			address = HW_VOICE_BUF_START + HW_VOICE_BUF_BANK_SIZE ; //BANK1 address
		}
#endif
 
		if(im501_host_irqstatus == 1) pr_err("%s: data ready at address=%x, data_size=%x\n", __func__, address, data_size);
		err = fetch_voice_data( address, data_size );
		if( err != ESUCCESS ) {
			pr_err("%s: fetch_voice_data() return error.\n", __func__);
			return err;
		}

		//cmd.status = 0;
		//Should be modified in accordance with customers scenario definitions.
		pdata = (u8 *)&cmd;
		pdata[3] &= 0x7F;	//changes the bit-31 to 0 to indicate that the hot has finished with the task.
		//pr_err("%s: pdata[%#x, %#x, %#x, %#x]\n", __func__, pdata[0], pdata[1], pdata[2], pdata[3]);
		err = im501_spi_write_dram(TO_HOST_CMD_ADDR, pdata);
		if( err != ESUCCESS ){ 
			pr_err("%s: im501_spi_write_dram() return error.\n", __func__);
			return err;
		}           
	} 
	else { //treat all other interrupt as keyword detect
		if(cmd.cmd_byte == TO_HOST_CMD_KEYWORD_DET) {
			struct file_private_data * priv;
			spin_lock(&file_pdata_spinlock);
			list_for_each_entry(priv, &file_pdata_list, entry) {
				priv->available = 1;
			}
			spin_unlock(&file_pdata_spinlock);
			wake_up_interruptible_all(&wakeup_work_queue);
		} else 
		if(	(im501_vbuf_trans_status == 0) && (im501_dsp_mode_old == POWER_SAVING_MODE)){ //only available when not transfer voice and in PSM
			im501_vbuf_trans_status = 1;
			im501_host_irqstatus = 1;
			
/*			
#ifdef CODEC2IM501_PDM_CLKI
			//to change the iM501 from PSM to Normal mode, and to keep the same setting
			//as the mode before changing to PSM, the Host just needs to turn on the PDMCLKI,
			//and no additional command is needed. 
			codec2im501_pdm_clki_set(NORMAL_MODE, pdm_clki_rate);
			im501_dsp_mode_old = NORMAL_MODE;
			mdelay(5);
#endif
*/

			request_enter_normal();
			pr_err("%s: keyword detected.\n", __func__);
			//pr_err("%s: ap_sleep_flag = %d, im501_dsp_mode_old = %d\n", __func__, ap_sleep_flag, im501_dsp_mode_old);
			if (ap_sleep_flag) {
				im501_spi->mode = SPI_MODE_0; /* clk active low */
				im501_spi->bits_per_word = 8;
				im501_spi->max_speed_hz = spi_speed; //SPI_HIGH_SPEED

				ret = spi_setup(im501_spi);
				if (ret < 0) {
					dev_err(&im501_spi->dev, "spi_setup() failed\n");
					return -EIO;
				}
				msleep(10);

				ap_sleep_flag = 0;
			}

			//cmd.status = 0;
			//Should be modified in accordance with customers scenario definitions.
			pdata = (u8 *)&cmd;
			pdata[3] &= 0x7F;	//changes the bit-31 to 0 to indicate that the hot has finished with the task.
			//pr_err("%s: pdata[%#x, %#x, %#x, %#x]\n", __func__, pdata[0], pdata[1], pdata[2], pdata[3]);
			err = im501_spi_write_dram(TO_HOST_CMD_ADDR, pdata);
			if( err != ESUCCESS ){ 
				pr_err("%s: 1111 im501_spi_write_dram() return error.\n", __func__);
				return err;
			}

			//pr_err("%s: start to get oneshot and save to file!!!!!!!!!!!!!!!!!!!!!!\n", __func__);			
			openFileForWrite(REC_FILE_PATH);
			request_start_voice_buf_trans();
		}
		else {
			pr_err("%s: Do not process other interrupt during process keyword detection and oneshot, or DSP is in normal mode.\n", __func__);
		}
	}

    return err;
}

unsigned char im501_send_to_dsp_command(to_501_cmd cmd)
{
    unsigned char err;
    unsigned int  i;
    u8 pTempData[8];
    u8 *pdata;
    
	//pr_err("%s: attri=%#x, cmd_byte_ext=%#x, status=%#x, cmd_byte=%#x\n", __func__, cmd.attri, cmd.cmd_byte_ext, cmd.status, cmd.cmd_byte);
	pdata = (u8 *) &cmd;
	//pr_err("%s: pdata[%#x, %#x, %#x, %#x, %#x]\n", __func__, pdata[0], pdata[1], pdata[2], pdata[3], pdata[4]);
	//pr_err("%s: attr=%#x, ext=%#x, status=%#x, cmd=%#x\n", __func__, cmd.attri, cmd.cmd_byte_ext, cmd.status, cmd.cmd_byte);
	
    err = im501_spi_write_dram( TO_DSP_CMD_ADDR, pdata);
    if( err != ESUCCESS ){ 
		pr_err("%s: call im501_spi_write_dram() return error(%d)\n", __func__, err);
        return err;
    }

    err = im501_spi_write_reg( 0x01,  cmd.cmd_byte ); //generate interrupt to DSP
    if( err != ESUCCESS ){ 
		pr_err("%s: call im501_spi_write_reg() return error(%d)\n", __func__, err);
        return err;
    }
    
	memset(pTempData, 0, 8);
	
    for( i = 0; i < 200; i++ ) {   //wait for (50*100us = 5ms) to check if DSP finished 
        err = im501_spi_read_dram( TO_DSP_CMD_ADDR, pTempData);
        if( err != ESUCCESS ){ 
			pr_err("%s: call im501_spi_read_dram() return error(%d)\n", __func__, err);
            return err;
        }
        
        if( (pTempData[3] >> 7) != 0 ) {
            err = TO_501_CMD_ERR;
        } else {
            err = ESUCCESS;
            break;
        }
        //msleep(1);
    }
    
    return err;
}

//Fuli 20171022 to support general message interface
unsigned char im501_send_message_to_dsp( unsigned char cmd_index, unsigned int para )
{
    unsigned char err = ESUCCESS;
	to_501_cmd    cmd;
        
	cmd.attri    		= para & 0xFFFFFF ;
	cmd.cmd_byte_ext 	= (para >> 24) & 0x7F;
	cmd.status   		= 1;

	cmd.cmd_byte = cmd_index;//((cmd_index & 0x3F) << 2) | 0x01; //D[1] : "1", interrupt DSP. This bit generates NMI (non-mask-able interrupt), D[0]: "1" generate mask-able interrupt

	pr_err("%s: attri=%#x, cmd_byte_ext=%#x, status=%#x, cmd_byte=%#x\n", __func__, cmd.attri, cmd.cmd_byte_ext, cmd.status, cmd.cmd_byte);
	err = im501_send_to_dsp_command( cmd );
	
	if(err != ESUCCESS) {
		pr_err("%s: fail to send message(%02x) to dsp. err=%d\n", __func__, cmd_index >> 2, err);
	}
	
    return err;
}
//
#if defined(CUBIETRUCK)
static int im501_gpio_init(int *gpio_number, int dt_gpio_idx)
{
    *gpio_number = GPIOB(dt_gpio_idx);
	//pr_err("%s: gpio_number = %d @ %d\n", __func__, *gpio_number, dt_gpio_idx);
    
	if (!gpio_is_valid(*gpio_number)) return -1;

	if(gpio_request(*gpio_number, "pdm_clk")) {
		pr_err("%s: %s request port error!\n", __func__, "pdm_clk");
		return -1;
	} else {
		gpio_direction_output(*gpio_number, 0);
		mdelay(10);
		gpio_free(*gpio_number);
	}
	return ESUCCESS;
}
#elif defined(INGENIC)
#else
static int im501_gpio_init(struct device_node *np, int *gpio_number, int dt_gpio_idx)
{
    *gpio_number = of_get_gpio(np, dt_gpio_idx);
	//pr_err("%s: gpio_number = %d @ %d\n", __func__, *gpio_number, dt_gpio_idx);
    
	if (!gpio_is_valid(*gpio_number)) return -1;

	if(gpio_request(*gpio_number, "pdm_clk")) {
		pr_err("%s: %s request port error!\n", __func__, "pdm_clk");
		return -1;
	} else {
		gpio_direction_output(*gpio_number, 0);
		mdelay(10);

		gpio_free(*gpio_number);
	}
	return ESUCCESS;
}
#endif

unsigned char request_start_voice_buf_trans( void )
{
    unsigned char err = ESUCCESS;
	err = im501_send_message_to_dsp( TO_DSP_CMD_REQ_START_BUF_TRANS, 0 );
	
    return err;
}

unsigned char request_stop_voice_buf_trans( void )
{
    unsigned char err = ESUCCESS;

	err = im501_send_message_to_dsp( TO_DSP_CMD_REQ_STOP_BUF_TRANS, 0 );
	im501_vbuf_trans_status = 0;

    return err;
}

unsigned char request_enter_psm( void )
{
    unsigned char err = ESUCCESS;
        
	if (im501_vbuf_trans_status == 1) {
		pr_err("%s: im501_vbuf_trans_status = %d, stop it.\n", __func__, im501_vbuf_trans_status);
		request_stop_voice_buf_trans(); //Call this function to guarantee there is no voice transferring.
	}
	
	im501_vbuf_trans_status = 0;

	if (im501_dsp_mode_old != POWER_SAVING_MODE) { //The current dsp mode is not PSM.
		pr_err("%s-1: ap_sleep_flag = %d, im501_dsp_mode_old = %d. iM501 is going to sleep.\n", __func__, ap_sleep_flag, im501_dsp_mode_old);
		pr_err("%s: the command is %#x\n", __func__, TO_DSP_CMD_REQ_ENTER_PSM);
		//fuli 20170629		err = write_cmd_to_im501( TO_DSP_CMD_REQ_ENTER_PSM, 0xc0000000);
		err = im501_send_message_to_dsp( TO_DSP_CMD_REQ_ENTER_PSM, 0);
		//
		
		if(ESUCCESS != err) {
			pr_err("%s: error occurs when switch to power saving mode, error = %d\n", __func__, err);
		}
	 
#ifdef CODEC2IM501_PDM_CLKI
		mdelay(100);
		codec2im501_pdm_clki_set(POWER_SAVING_MODE, 0);
		mdelay(5);
#endif
		ap_sleep_flag = 1;
		im501_dsp_mode_old = POWER_SAVING_MODE;
		pr_err("%s-2: ap_sleep_flag = %d, im501_dsp_mode_old = %d\n", __func__, ap_sleep_flag, im501_dsp_mode_old);
	}

	return err;
}

unsigned char request_enter_normal(void)
{
    unsigned char err = ESUCCESS;
        
	if (im501_dsp_mode_old != NORMAL_MODE) { //The current dsp mode is not NORMAL
		pr_err("%s: entering...\n", __func__);
#ifdef CODEC2IM501_PDM_CLKI
		mdelay(5);
		codec2im501_pdm_clki_set(NORMAL_MODE, 0);
		mdelay(100);
#endif

		//fuli 20170629
		err = im501_send_message_to_dsp( TO_DSP_CMD_REQ_ENTER_NORMAL, 0);
		if(ESUCCESS != err) {
			pr_err("%s: error occurs when switch to normal mode, error = %d\n", __func__, err);
		}
		//
		
		ap_sleep_flag = 0;
		im501_dsp_mode_old = NORMAL_MODE;
		pr_err("%s: ap_sleep_flag = %d, im501_dsp_mode_old = %d\n", __func__, ap_sleep_flag, im501_dsp_mode_old);
	}
	
	return err;
}

int im501_spi_burst_read_check(u32 start_addr, u8 *buf, u32 data_length )
{
    int err;
    unsigned char *pbuf;
    unsigned int  i,a,b;
    
	pr_err("%s: entering...\n", __func__);
    a = data_length / IM501_SPI_BUF_LEN ;
    b = data_length % IM501_SPI_BUF_LEN ;
    
	pbuf = (unsigned char *) kzalloc(IM501_SPI_BUF_LEN, GFP_KERNEL);
	if (!pbuf) {
		pr_err("%s: pbuf allocation failure.\n", __func__);
		return -1;
	}
    
    for( i = 0; i < a; i++ ) {
		pr_err("%s: start_addr = %#x\n", __func__, start_addr);
        err = im501_spi_burst_read_dram( start_addr, pbuf,  IM501_SPI_BUF_LEN ); 
        if( err != ESUCCESS ){ 
            return err;
        }
        memcpy(buf + i * IM501_SPI_BUF_LEN, pbuf, IM501_SPI_BUF_LEN);

        start_addr +=  IM501_SPI_BUF_LEN;
    }
    
    if( b > 0 ) {
        err = im501_spi_burst_read_dram( start_addr,  pbuf,  b ); 
        if( err != ESUCCESS ){ 
            return err;
        }
        memcpy(buf + i * IM501_SPI_BUF_LEN, pbuf, b);
    }
    
    if (pbuf) kfree(pbuf);
    return err;
}
EXPORT_SYMBOL_GPL(im501_spi_burst_read_check);

static void im501_8byte_swap(u8 *rxbuf, u32 len)
{
	u8 local_buf[8];
	int i;
	
	for (i = 0; i < len / 8 * 8; i += 8) {
		local_buf[0] = rxbuf[i + 0];
		local_buf[1] = rxbuf[i + 1];
		local_buf[2] = rxbuf[i + 2];
		local_buf[3] = rxbuf[i + 3];
		local_buf[4] = rxbuf[i + 4];
		local_buf[5] = rxbuf[i + 5];
		local_buf[6] = rxbuf[i + 6];
		local_buf[7] = rxbuf[i + 7];

		rxbuf[i + 0] = local_buf[7];
		rxbuf[i + 1] = local_buf[6];
		rxbuf[i + 2] = local_buf[5];
		rxbuf[i + 3] = local_buf[4];
		rxbuf[i + 4] = local_buf[3];
		rxbuf[i + 5] = local_buf[2];
		rxbuf[i + 6] = local_buf[1];
		rxbuf[i + 7] = local_buf[0];
	}
}

static size_t im501_read_file(char *file_path, const u8 **buf)
{
	loff_t pos = 0;
	unsigned int file_size = 0;
	struct file *fp;

	pr_err("%s: file_path = %s\n", __func__, file_path);
	fp = filp_open(file_path, O_RDONLY, 0);
	if (!IS_ERR(fp)) {
		pr_err("%s: The file is present.\n", __func__);
		
		file_size = vfs_llseek(fp, pos, SEEK_END);

		*buf = kzalloc(file_size, GFP_KERNEL);
		if (*buf == NULL) {
			filp_close(fp, 0);
			return 0;
		}

		kernel_read(fp, pos, (char *)*buf, file_size);
		filp_close(fp, 0);

		return file_size;
	}

	return 0;
}

static int im501_dsp_load_single_fw_file(u8 *fw_name, u32 addr, int fw_type)
{
	u8 *local_buf, *data;
	int i;
	size_t size = 0;

	//pr_err("%s: entering...\n", __func__);
#ifdef SHOW_DL_TIME
	do_gettimeofday(&(txc.time));
	rtc_time_to_tm(txc.time.tv_sec,&rt);
	pr_err("%s: start time: %d-%d-%d %d:%d:%d \n", __func__, rt.tm_year+1900,rt.tm_mon, rt.tm_mday,rt.tm_hour,rt.tm_min,rt.tm_sec); 			
#endif

	size = im501_read_file(fw_name, (const u8**)&data);
	if (!size) {
		dev_err(&im501_spi->dev, "%s: firmware %s open failed.\n", __func__, fw_name);
		return -1;
	}
	if (size) {
		pr_err("%s: firmware %s size = %d, addr = %#x\n", __func__, fw_name, size, addr);
		im501_spi_burst_write(addr, data, size, fw_type);

#ifdef FW_RD_CHECK
		local_buf = (u8 *)kzalloc(size, GFP_KERNEL);
		if (!local_buf)
			return -ENOMEM;
#ifdef FW_BURST_RD_CHECK
		im501_spi_burst_read_dram(addr, local_buf, size);
		if (fw_type == IM501_DSP_FW)
			im501_8byte_swap(local_buf, size);
		
		for (i = 0; i < size; i++) {
			if (local_buf[i] != data[i]) {
				pr_err("%s: fw read %#x vs write %#x @ %#x\n", __func__, local_buf[i], data[i], addr+i);
			}
		}
#endif
		kfree (local_buf);
#endif
		kfree (data);
	}

#ifdef SHOW_DL_TIME
	do_gettimeofday(&(txc.time));
	rtc_time_to_tm(txc.time.tv_sec,&rt);
	pr_err("%s: end   time: %d-%d-%d %d:%d:%d \n", __func__, rt.tm_year+1900,rt.tm_mon, rt.tm_mday,rt.tm_hour,rt.tm_min,rt.tm_sec); 			
#endif
	return 0;
}
EXPORT_SYMBOL_GPL(im501_dsp_load_single_fw_file);

static void im501_dsp_load_fw(u8 firmware_type) { //0: wakeup firmware; 1: ultrasound firmware

	const struct firmware *fw = NULL;
	u8 *local_buf;
	int fw_type = IM501_DSP_FW;
	int i;
	
pr_err("%s: firmware_type=%d\n", __func__, firmware_type);
	//pr_err("%s: entering...\n", __func__);
#ifdef SHOW_DL_TIME
	do_gettimeofday(&(txc.time));
	rtc_time_to_tm(txc.time.tv_sec,&rt);
	pr_err("%s: start time: %d-%d-%d %d:%d:%d \n", __func__, rt.tm_year+1900,rt.tm_mon, rt.tm_mday,rt.tm_hour,rt.tm_min,rt.tm_sec); 			
#endif

#if defined(INGENIC)
	request_firmware(&fw, FIRMWARE_DIR "/iram0.bin", &im501_spi->dev);
#else
	if(firmware_type == WAKEUP_FIRMWARE) request_firmware(&fw, "iram0.bin", &im501_spi->dev);
	else request_firmware(&fw, "../firmware1/iram0.bin", &im501_spi->dev);
#endif
	
	if (!fw) {
		dev_err(&im501_spi->dev, "%s: iram0.bin firmware request failed.\n", __func__);
	}
	if (fw) {
		pr_err("%s: firmware iram0.bin size = %d \n", __func__, fw->size);
		im501_spi_burst_write(0x10000000, fw->data, fw->size, fw_type);

#ifdef FW_RD_CHECK
		local_buf = (u8 *)kzalloc(fw->size, GFP_KERNEL);
		if (!local_buf)
			return;// -ENOMEM;
#ifdef FW_BURST_RD_CHECK
		im501_spi_burst_read_dram(0x10000000, local_buf, fw->size);
		if (fw_type == IM501_DSP_FW)
			im501_8byte_swap(local_buf, fw->size);
		
		for (i = 0; i < fw->size; i++) {
			if (local_buf[i] != fw->data[i]) {
				pr_err("%s: fw read %#x vs write %#x @ %#x\n", __func__, local_buf[i], fw->data[i], 0x10000000+i);
				break;
			}
		}
#endif
#ifdef FW_MEM_RD_CHECK		
		for (i = 0; i < fw->size / 4; i++) {
			im501_spi_read_dram(0x10000000 + i * 4, local_buf + i * 4);
			if (local_buf[i * 4] != fw->data[i * 4])
				pr_err("%s: fw read %#x vs write %#x @ %#x\n", __func__, local_buf[i * 4], fw->data[i * 4], 0x10000000 + i * 4);
			if (local_buf[i * 4 + 1] != fw->data[i * 4 + 1])
				pr_err("%s: fw read %#x vs write %#x @ %#x\n", __func__, local_buf[i * 4 + 1], fw->data[i * 4 + 1], 0x10000000 + i * 4 + 1);
			if (local_buf[i * 4 + 2] != fw->data[i * 4 + 2])
				pr_err("%s: fw read %#x vs write %#x @ %#x\n", __func__, local_buf[i * 4 + 2], fw->data[i * 4 + 2], 0x10000000 + i * 4 + 2);
			if (local_buf[i * 4 + 3] != fw->data[i * 4 + 3])
				pr_err("%s: fw read %#x vs write %#x @ %#x\n", __func__, local_buf[i * 4 + 3], fw->data[i * 4 + 3], 0x10000000 + i * 4 + 3);
		}
#endif
		kfree (local_buf);
#endif
		release_firmware(fw);
		fw = NULL;
	}

#if defined(INGENIC)
	request_firmware(&fw, FIRMWARE_DIR "/dram0.bin", &im501_spi->dev);
#else
	if(firmware_type == WAKEUP_FIRMWARE) request_firmware(&fw, "dram0.bin", &im501_spi->dev);
	else request_firmware(&fw, "../firmware1/dram0.bin", &im501_spi->dev);
#endif
	if (!fw) {
		dev_err(&im501_spi->dev, "%s: dram0.bin firmware request failed.\n", __func__);
	}
	if (fw) {
		pr_err("%s: firmware dram0.bin size = %d\n", __func__, fw->size);
		im501_spi_burst_write(0x0ffc0000, fw->data, fw->size, fw_type);

#ifdef FW_RD_CHECK
		local_buf = (u8 *)kzalloc(fw->size, GFP_KERNEL);
		if (!local_buf)
			return;// -ENOMEM;
	
#ifdef FW_BURST_RD_CHECK		
		im501_spi_burst_read_dram(0x0ffc0000, local_buf, fw->size);
		if (fw_type == IM501_DSP_FW)
			im501_8byte_swap(local_buf, fw->size);
		
		for (i = 0; i < fw->size; i++) {
			if (local_buf[i] != fw->data[i]) {
				pr_err("%s: fw read %#x vs write %#x @ %#x\n", __func__, local_buf[i], fw->data[i], 0x0ffc0000+i);
				break;
			}
		}
#endif
		kfree (local_buf);
#endif

		release_firmware(fw);
		fw = NULL;
	}
#if defined(INGENIC)
	request_firmware(&fw, FIRMWARE_DIR "/dram1.bin", &im501_spi->dev);
#else
	if(firmware_type == WAKEUP_FIRMWARE) request_firmware(&fw, "dram1.bin", &im501_spi->dev);
	else request_firmware(&fw, "../firmware1/dram1.bin", &im501_spi->dev);
#endif
	if (!fw) {
		dev_err(&im501_spi->dev, "%s: dram1.bin firmware request failed.\n", __func__);
	}
	if (fw) {
		pr_err("%s: firmware dram1.bin size=%d\n", __func__, fw->size);
		im501_spi_burst_write(0x0ffe0000, fw->data, fw->size, fw_type);

#ifdef FW_RD_CHECK
		local_buf = (u8 *)kzalloc(fw->size, GFP_KERNEL);
		if (!local_buf)
			return;// -ENOMEM;
		
#ifdef FW_BURST_RD_CHECK		
		im501_spi_burst_read_dram(0x0ffe0000, local_buf, fw->size);
		if (fw_type == IM501_DSP_FW)
			im501_8byte_swap(local_buf, fw->size);
		
		for (i = 0; i < fw->size; i++) {
			if (local_buf[i] != fw->data[i]) {
				pr_err("%s: fw read %#x vs write %#x @ %#x\n", __func__, local_buf[i], fw->data[i], 0x0ffe0000+i);
				break;
			}
		}
#endif
		kfree (local_buf);
#endif
		release_firmware(fw);
		fw = NULL;
	}
		
	
#ifdef SHOW_DL_TIME
	do_gettimeofday(&(txc.time));
	rtc_time_to_tm(txc.time.tv_sec,&rt);
	pr_err("%s: end   time: %d-%d-%d %d:%d:%d \n", __func__, rt.tm_year+1900,rt.tm_mon, rt.tm_mday,rt.tm_hour,rt.tm_min,rt.tm_sec); 			
#endif
	pr_err("%s: firmware is loaded\n", __func__);
}

void im501_write_check_spi_reg(u8 reg, u8 val) {
	u8 ret_val;

	im501_spi_write_reg(reg, val);
	im501_spi_read_reg(0x00, (u8*)&ret_val);
	if(ret_val != val) pr_err("%s: read back value(%x) of reg%02x is different from the written value(%x). \n", __func__, ret_val, reg, val); 			

	return;
}

static void im501_fw_loaded(const struct firmware *fw, void *context)
{
	u32 pdm_clki_rate = 2 * 1000 * 1000;
	u8* temp_data;

    pr_err("%s: entering...\n", __func__);
    release_firmware(fw);

#ifdef CODEC2IM501_PDM_CLKI    
    //Commented out temporarily
    codec2im501_pdm_clki_set(NORMAL_MODE, pdm_clki_rate);
    mdelay(5); //Apply PDM_CLKI, and then wait for 1024 clock cycles
#endif
	mdelay(500); //make sure PDMCLKI is ready for external PDMCLKI

	//reset DSP
	im501_reset();
	
	im501_write_check_spi_reg(0x00, 0x07);
	im501_write_check_spi_reg(0x00, 0x05);
	
    mdelay(1);
    
    current_firmware_type = WAKEUP_FIRMWARE;
	if(context != NULL) {
		temp_data = context;
		current_firmware_type = (u8)(*temp_data);
	}
	pr_err("%s: firmware_type = %d\n", __func__, current_firmware_type);

	im501_dsp_load_fw(current_firmware_type);
    mdelay(1);

	im501_write_check_spi_reg(0x00, 0x04);

	//check DSP working or not
	check_dsp_status();
	
	im501_dsp_mode_old = NORMAL_MODE;

	//Here after download the firmware, force DSP enter PSM mode for testing.
	//Should be changed according to the real requirement.
	//request_enter_psm();

	return;
}

static void im501_spi_lock(struct mutex *lock)
{
	mutex_lock(lock);
}

static void im501_spi_unlock(struct mutex *lock)
{
	mutex_unlock(lock);
}

//Fuli 20171012 to support SPI recording
unsigned char im501_switch_to_spi_recording_mode( void )
{
	return im501_send_message_to_dsp( TO_DSP_CMD_REQ_SWITCH_SPI_REC, 1 );
}

unsigned char im501_switch_to_normal_mode( void )
{
	return im501_send_message_to_dsp( TO_DSP_CMD_REQ_SWITCH_SPI_REC, 0 );
}
//

static void im501_irq_handling_work(struct work_struct *work)
{
    unsigned char err;
    to_host_cmd   cmd;
    u8 *pdata;
    u32 spi_speed = SPI_LOW_SPEED;
    int ret;
    

	//pr_err("%s: entering...\n", __func__);
	if (im501_dsp_mode_old == POWER_SAVING_MODE) {
		pr_err("%s: iM501 is in PSM, set the spi speed to %d\n", __func__, spi_speed);
		im501_spi->mode = SPI_MODE_0; /* clk active low */
		im501_spi->bits_per_word = 8;
		im501_spi->max_speed_hz = spi_speed;

		ret = spi_setup(im501_spi);
		if (ret < 0) {
			dev_err(&im501_spi->dev, "spi_setup() failed\n");
			return;// -EIO;
		}
		msleep(10);
	}

    pdata = (u8 *)&cmd;
	err = im501_spi_read_dram( TO_HOST_CMD_ADDR, pdata);
	if( err != ESUCCESS ){
		pr_err("%s: error occurs when get data from DRAM, error=%d\n", __func__, err);
		return ;
	}
	
	if((im501_host_irqstatus == 1) || ((output_counter % 100) == 0)) {
		pr_err("%s: pdata[%#x, %#x, %#x, %#x]\n", __func__, pdata[0], pdata[1], pdata[2], pdata[3]);
	}
	output_counter++;
	
	cmd.status = pdata[3] >> 7;
	cmd.cmd_byte = pdata[3] & 0x7F;
	cmd.attri = pdata[0] | (pdata[1] * 256) | (pdata[2] * 256 * 256);
    //pr_err("%s: cmd[%#x, %#x, %#x]\n", __func__, cmd.status, cmd.cmd_byte, cmd.attri);
	
	if(cmd.status != 1) {
		pr_err("%s: got wrong command from DSP.\n", __func__);
	}
	
	im501_spi_lock(&im501_data->spi_op_lock);
	err = parse_to_host_command( cmd );
	im501_spi_unlock(&im501_data->spi_op_lock);

	if( err != ESUCCESS ){ 
		pr_err("%s: error occurs when calling parse_to_host_command(), error=%d\n", __func__, err);
	}

	return ;
}

static irqreturn_t im501_irq_handler(int irq, void *para)
{
	//pr_err("%s: entering...\n", __func__);
	queue_work(im501_irq_wq, &im501_irq_work);
	
	return IRQ_HANDLED;
}

/* Access to the audio buffer is controlled through "audio_owner". Either the
 * character device can be opened. */
static int im501_record_open(struct inode *inode, struct file *file)
{
	pr_err("%s: entering...\n", __func__);
	if (!atomic_add_unless(&im501_data->audio_owner, 1, 1))
		return -EBUSY;
	pr_err("%s: im501_data->audio_owner.counter = %d\n", __func__, im501_data->audio_owner.counter);

	file->private_data = im501_data;

	return 0;
}

static int im501_record_release(struct inode *inode, struct file *file)
{
	pr_err("%s: decrease audio_owner.\n", __func__);
	atomic_dec(&im501_data->audio_owner);
	pr_err("%s: im501_data->audio_owner.counter = %d\n", __func__, im501_data->audio_owner.counter);

	return 0;
}

/* The write function is a hack to load the A-model on systems where the
 * firmware files are not accesible to the user. */
static ssize_t im501_record_write(struct file *file,
				  const char __user *buf,
				  size_t count_want,
				  loff_t *f_pos)
{
	pr_err("%s: entering...\n", __func__);
	return count_want;
}

/* Read out of the kfifo (as long as data is available). */
static ssize_t im501_record_read(struct file *file,
				 char __user *buf,
				 size_t count_want,
				 loff_t *f_pos)
{
	return 0;
}

static const struct file_operations record_fops = {
	.owner   = THIS_MODULE,
	.open    = im501_record_open,
	.release = im501_record_release,
	.read    = im501_record_read,
	.write   = im501_record_write,
};

static int im501_create_cdev(struct spi_device *spi)
{
    int ret = 0, err = -1;
    struct device *dev = &spi->dev;
    int cdev_major, dev_no;

	pr_err("%s: entering...\n", __func__);
	atomic_set(&im501_data->audio_owner, 0);
	pr_err("%s: im501_data->audio_owner.counter = %d\n", __func__, im501_data->audio_owner.counter);
	 
	ret = alloc_chrdev_region(&im501_data->record_chrdev, 0, 1, IM501_CDEV_NAME);
	if (ret) {
		dev_err(dev, "%s: failed to allocate character device\n", __func__);
		return ret;
	}
	pr_err("%s: alloc_chrdev_region ok...\n", __func__);

	cdev_major = MAJOR(im501_data->record_chrdev);
	pr_err("%s: char dev major = %d", __func__, cdev_major);
		
	im501_data->cdev_class = class_create(THIS_MODULE, IM501_CDEV_NAME);
	if (IS_ERR(im501_data->cdev_class)) {
		dev_err(dev, "%s: failed to create class\n", __func__);
		return err;
	}
	pr_err("%s: cdev_class create ok...\n", __func__);

	dev_no = MKDEV(cdev_major, 1);
	
	cdev_init(&im501_data->record_cdev, &record_fops);
	pr_err("%s: cdev_init ok...\n", __func__);

	im501_data->record_cdev.owner = THIS_MODULE;

	ret = cdev_add(&im501_data->record_cdev, dev_no, 1);
	if (ret) {
		dev_err(dev, "%s: failed to add character device\n", __func__);
		return ret;
	}
	pr_err("%s: cdev_add ok...\n", __func__);

	im501_data->record_dev = device_create(im501_data->cdev_class, NULL,
					 dev_no, NULL, "%s%d", IM501_CDEV_NAME, 1);
	if (IS_ERR(im501_data->record_dev)) {
		dev_err(&im501_spi->dev, "%s: could not create device\n", __func__);
		return err;
	}
 	pr_err("%s: device_create ok...\n", __func__);

    return ret; 
}

static ssize_t im501_spi_device_read(struct file *file, char __user * buffer,
	size_t length, loff_t * offset)
{
	char str[5];
	size_t ret = 0;
	char *local_buffer;
    dev_cmd_mode_gs* get_mode_ret_data;
    dev_cmd_reg_rw*  get_reg_ret_data;
    dev_cmd_long*    get_addr_ret_data;
    dev_cmd_short*	 get_irq_status;
    dev_cmd_fw_type* get_firmware_type_ret_data;
    int temp = 0;

	local_buffer = (char *)kzalloc(length * sizeof(char), GFP_KERNEL);
	if (!local_buffer) {
		pr_err("%s: local_buffer allocation failure.\n", __func__);
		goto out;
	}
	temp = copy_from_user(local_buffer, buffer, length);

    //pr_err("local_buffer = %d:, length = %d\n", local_buffer[0], length);
	switch(local_buffer[0]) {
	case FM_SMVD_REG_READ:
        get_reg_ret_data = (dev_cmd_reg_rw*)local_buffer;
        im501_spi_read_reg(get_reg_ret_data->reg_addr, (u8*)&get_reg_ret_data->reg_val);
        ret = sizeof(dev_cmd_reg_rw);
		break;

	case FM_SMVD_DSP_ADDR_READ:
        get_addr_ret_data = (dev_cmd_long*)local_buffer;
		im501_spi_read_dram(get_addr_ret_data->addr, (u8*)&get_addr_ret_data->val);
        ret = sizeof(dev_cmd_long);
        break;

	case FM_SMVD_MODE_GET:
        get_mode_ret_data = (dev_cmd_mode_gs*)local_buffer;
        get_mode_ret_data->dsp_mode = (char)im501_dsp_mode_old;
        ret = sizeof(dev_cmd_mode_gs);
        break;

	case FM_SMVD_HOST_IRQQUERY:
		get_irq_status = (dev_cmd_short*)local_buffer;
		get_irq_status->val = im501_host_irqstatus;
		if (im501_host_irqstatus == 1) {
			im501_host_irqstatus = 0;
			pr_err("%s: iM501 irq is coming..\n", __func__);
		}
		ret = sizeof(dev_cmd_short);
		break;

	case FM_SMVD_GET_FW_TYPE:
        get_firmware_type_ret_data 					= (dev_cmd_fw_type*)local_buffer;
        get_firmware_type_ret_data->firmware_type 	= (char)current_firmware_type;
        ret = sizeof(dev_cmd_fw_type);
        break;
	case FM_WAIT_VOICE_TRIGGER:
	{
		struct file_private_data * priv = file->private_data;
		wait_event_interruptible(wakeup_work_queue, priv->available != 0);
		priv->available = 0;
		ret = 0;
	}
	default:
        ret = sprintf(str, "0");            
		break;
	}

	if (copy_to_user(buffer, local_buffer, ret))
		return -EFAULT;

out:
	if (local_buffer) kfree(local_buffer);
	return ret;
}

static ssize_t im501_spi_device_write(struct file *file,
	const char __user * buffer, size_t length, loff_t * offset)
{
	dev_cmd_long *local_dev_cmd;
	dev_cmd_fwdl *local_dev_cmd_fwdl;
	/*if spi speed is fast enough, we can get total data from SPI together
	dev_cmd_start_rec *local_rec_cmd;
	*/
	dev_cmd_message *local_msg_cmd;
	
	unsigned int cmd_name, cmd_addr, cmd_val;
	int dsp_mode;
	int new_firmware_type;
	int temp = 0;
	unsigned char err = ESUCCESS;
	
	pr_err("%s: entering...\n", __func__);
	local_dev_cmd = (dev_cmd_long *)kzalloc(sizeof(dev_cmd_long), GFP_KERNEL);
	if (!local_dev_cmd) {
		pr_err("%s: local_dev_cmd allocation failure.\n", __func__);
		goto out;
	}
	temp = copy_from_user(local_dev_cmd, buffer, length);

    pr_err("local_dev_cmd->cmd_name = %d, length = %d\n", local_dev_cmd->cmd_name, length);	
	cmd_name = local_dev_cmd->cmd_name;
	
	switch(cmd_name) {
	//The short commands
	case FM_SMVD_REG_WRITE:		//Command #1
		cmd_addr = local_dev_cmd->addr;
		cmd_val = local_dev_cmd->val;
		im501_spi_write_reg(cmd_addr, cmd_val);        
		break;

	case FM_SMVD_DSP_ADDR_WRITE:	//Command #3
		cmd_addr = local_dev_cmd->addr;
		cmd_val = local_dev_cmd->val;
		im501_spi_write_dram(cmd_addr, (u8*)&cmd_val);      
		break;

	case FM_SMVD_MODE_SET:		//Command #4
		pr_err("%s: FM_SMVD_MODE_SET dsp_mode = %d\n", __func__, local_dev_cmd->addr);
		dsp_mode = local_dev_cmd->addr;
		// Temporarily set to PSM mode.
		if (dsp_mode == 0)
			request_enter_psm();
		else if (dsp_mode == 1)
			request_enter_normal();
		else if (dsp_mode == 4) {
			request_stop_voice_buf_trans();
			msleep(2000);
			closeDataFile();
			output_counter = 0;
		}
		//im501_dsp_mode_old = dsp_mode;
		break;

	case FM_SMVD_DL_EFT_FW:
		local_dev_cmd_fwdl = (dev_cmd_fwdl *)local_dev_cmd;
		im501_dsp_load_single_fw_file(local_dev_cmd_fwdl->buf, local_dev_cmd_fwdl->dsp_addr, IM501_EFT_FW);
		break;

	case FM_SMVD_SWITCH_FW:		
		pr_err("%s: FM_SMVD_SWITCH_FW firmware_type = %d\n", __func__, local_dev_cmd->addr);
		new_firmware_type = local_dev_cmd->addr;
		
		if((new_firmware_type == 0) || (new_firmware_type == 1)) {
			#ifndef INGENIC
			if(new_firmware_type == current_firmware_type) {
				pr_err("%s: requested firmware type is the same as current firmware type, not need to change it.\n", __func__);
			} else 
			#endif
			{
				firmware_type = new_firmware_type;
				im501_fw_loaded(NULL, (void*)&firmware_type);
			}
			
		}
		else {
			pr_err("%s: requested firmware type is not supported.\n", __func__);
		}
		break;

	//Fuli 20170922 to support SPI recording
	//do the same as one shot, app and lib will get sample data then generate pcm and wav by channel
	case FM_SMVD_START_SPI_REC:
		openFileForWrite(SPI_REC_FILE_PATH);

		im501_spi_record_started 	= 1;
		im501_host_irqstatus		= 0; //not wakeup by keywords
		
		//it will be set when im501 wakeup from PSM, set it here for SPI recording
		im501_vbuf_trans_status = 1;
		
		im501_switch_to_spi_recording_mode();
		request_start_voice_buf_trans();
		
		break;
		
	case FM_SMVD_STOP_SPI_REC:
		request_stop_voice_buf_trans();
		im501_spi_record_started = 0;
		
		//not sure it is needed or not
		im501_switch_to_normal_mode();
		//
		
		msleep(2000);
		closeDataFile();
		output_counter = 0;
		break;
	//
	
	//Fuli 20171022 to support general message interface
	case FM_SMVD_SEND_MESSAGE:
		local_msg_cmd	= (dev_cmd_message *) local_dev_cmd;
		pr_err("%s: will send message 0x%02X to DSP with data 0x%08X.\n", __func__, local_msg_cmd->message_index, local_msg_cmd->message_data);		
		err = im501_send_message_to_dsp( ((local_msg_cmd->message_index & 0x3F) << 2) | 0x01, local_msg_cmd->message_data );
		break;
	//
		
	default:
		break;
	}
out:
	if (local_dev_cmd) kfree(local_dev_cmd);
	return length;
}

static unsigned int im501_spi_device_poll(struct file * file, struct poll_table_struct * wait)
{
	struct file_private_data * priv = file->private_data;
	pr_err("%s: in\n", __func__);
	poll_wait(file, &wakeup_work_queue, wait);
	if (priv->available) {
		pr_err("%s: out PULLIN\n", __func__);
		return POLLIN;
	}
	pr_err("%s: out 0\n", __func__);
	return 0;
}

static int im501_spi_device_open(struct inode * inode, struct file * file)
{
	struct file_private_data * priv = kzalloc(sizeof(struct file_private_data), GFP_KERNEL);
	if (priv) {
		spin_lock(&file_pdata_spinlock);
		list_add_tail(&priv->entry, &file_pdata_list);
		file->private_data = priv;
		priv->file = file;
		priv->available = 0;
		spin_unlock(&file_pdata_spinlock);
	}
	return priv ? 0 : -ENOMEM;
}

static int im501_spi_device_release(struct inode * inode, struct file * file)
{
	struct file_private_data * priv = file->private_data;

	spin_lock(&file_pdata_spinlock);
	list_del(&priv->entry);
	spin_unlock(&file_pdata_spinlock);
	kzfree(priv);
}


struct file_operations im501_spi_fops = {
	.owner = THIS_MODULE,
	.open = im501_spi_device_open,
	.read = im501_spi_device_read,
	.write = im501_spi_device_write,
	.poll = im501_spi_device_poll,
	.release = im501_spi_device_release,
};

static struct miscdevice im501_spi_dev = {
	.minor = MISC_DYNAMIC_MINOR,
	.name = "im501_spi",
	.fops = &im501_spi_fops
};

//Henry add for try
static const struct spi_device_id im501_spi_id[] = {
	{ "im501_spi", 0 },
	{ }
};
MODULE_DEVICE_TABLE(spi, im501_spi_id);
//End


#ifdef CONFIG_OF
static struct of_device_id im501_spi_dt_ids[] = {
	{ .compatible = "fortemedia,im501_spi"},
	{ },
};
MODULE_DEVICE_TABLE(of, im501_spi_dt_ids);
#endif

static int im501_spi_probe(struct spi_device *spi)
{
	int ret;
#if defined(CUBIETRUCK)
	enum gpio_eint_trigtype 	trigtype;
	u32 						trigenabled = 0;
	script_item_value_type_e  	type;
	script_item_u 				val;
#elif defined(INGENIC)
	struct im501_pdata * pdata = spi->dev.platform_data;
#else
	struct device *dev 		= &spi->dev;
	struct device_node *np 	= dev->of_node;
#endif
	int irq_gpio;			/* irq gpio define */
	u32 spi_speed;
	
	im501_spi = spi;
	pr_err("%s: max_speed = %d, chip_select = %d, mode = %d, modalias = %s\n", __func__, spi->max_speed_hz, spi->chip_select, spi->mode, spi->modalias);
	
	im501_data = (struct im501_data_t *)kzalloc(sizeof(struct im501_data_t), GFP_KERNEL);
	if (im501_data == NULL)
		return -ENOMEM;
	pr_err("%s: im501_data allocated successfully\n", __func__);

	mutex_init(&im501_data->spi_op_lock);
	pr_err("%s: im501_data mutex lock init successfully\n", __func__);

	//Prepare GPIO for PDM_CLKI on/off control
#if defined(CUBIETRUCK)
	im501_gpio_init(&im501_data->pdm_clki_gpio, 2);
	//request pdm_clki_gpio for global usage.
	if(gpio_request(im501_data->pdm_clki_gpio, "pdm_clk")) {
		pr_err("%s: %s request port error!\n", __func__, "pdm_clk");
		return -1;
	}
	im501_data->reset_gpio = -1;
#elif defined(INGENIC)
	// im501_data->pdm_clki_gpio = im501_spi_dev->gpio_pdm_clki;
	im501_data->pdm_clki_gpio = -1;
	im501_data->reset_gpio = -1;
#else
	im501_gpio_init(np, &im501_data->pdm_clki_gpio, 1);
	//request pdm_clki_gpio for global usage.
	if(gpio_request(im501_data->pdm_clki_gpio, "pdm_clk")) {
		pr_err("%s: %s request port error!\n", __func__, "pdm_clk");
		return -1;
	}

	im501_gpio_init(np, &im501_data->reset_gpio, 2);
	//request reset_gpio for global usage.
	if(gpio_request(im501_data->reset_gpio, "reset_im501")) {
		pr_err("%s: %s request port error!\n", __func__, "reset_im501");
		return -1;
	}
	pr_err("%s: set reset pin to high.\n", __func__);
	gpio_set_value(im501_data->reset_gpio, 1);
	mdelay(200);
#endif
	

#ifdef CODEC2IM501_PDM_CLKI    
    //Commented out temporarily
    codec2im501_pdm_clki_set(POWER_SAVING_MODE, 0);
    mdelay(5);
#endif

#if defined(CUBIETRUCK)
	type = script_get_item("spi_board0", "max_speed_hz", &val);
	if (SCIRPT_ITEM_VALUE_TYPE_INT != type) {
        pr_err("%s: request max_speed_hz error!\n", __func__);
		spi_speed = SPI_HIGH_SPEED;
    }
    else {
		spi_speed = val.val;
	}
#elif defined(INGENIC)
	spi_speed = SPI_HIGH_SPEED;
#else
	ret = of_property_read_u32(np, "spi-max-frequency", &spi_speed);
	if (ret && ret != -EINVAL)
		spi_speed = SPI_HIGH_SPEED;
#endif

	spi->mode = SPI_MODE_0; // clk active low 
	spi->bits_per_word = 8;
	spi->max_speed_hz = spi_speed;

	ret = spi_setup(spi);
	if (ret < 0) {
		dev_err(&spi->dev, "spi_setup() failed\n");
		return -EIO;
	}
	pr_err("%s: setup spi, spi_speed=%d \n", __func__, spi_speed);

#if defined(INGENIC)
	current_firmware_type = -1;
#else
	firmware_type = WAKEUP_FIRMWARE;
	request_firmware_nowait(THIS_MODULE, FW_ACTION_HOTPLUG,
		"im501_fw", &spi->dev, GFP_KERNEL, (void*)&firmware_type, im501_fw_loaded);
#endif
	im501_create_cdev(spi);
	
	pr_err("%s: misc_register.\n", __func__);
	ret = misc_register(&im501_spi_dev);
	if (ret)
		dev_err(&spi->dev, "Couldn't register control device\n");

	im501_vbuf_trans_status = 0;

	im501_idx = (int *)kzalloc(sizeof(int*), GFP_KERNEL);
	*im501_idx = 1;
	
	INIT_WORK(&im501_irq_work, im501_irq_handling_work);
	im501_irq_wq = create_singlethread_workqueue("im501_irq_wq");


#if defined(CUBIETRUCK)
    irq_gpio = GPIOH(16);
	im501_irq = sw_gpio_irq_request(irq_gpio, TRIG_EDGE_POSITIVE, (peint_handle)im501_irq_handler, (int *)im501_idx);
	pr_err("%s: im501_irq = %d\n", __func__, im501_irq);
	if (im501_irq == 0) {
		pr_err("%s: IM501 sw_gpio_irq_request failed\n", __func__);
		return 0;
	}

	ret = sw_gpio_eint_get_trigtype(irq_gpio, &trigtype);
	if(ret != 0) {
		pr_err("%s: sw_gpio_eint_get_trigtype() failed.\n", __func__);
	}else {
		if(trigtype == TRIG_EDGE_POSITIVE) {
			pr_err("%s: trigger type is TRIG_EDGE_POSITIVE.\n", __func__);
		}
		else {
			pr_err("%s: trigger type is %d\n", __func__, trigtype);
		}
	}
	ret = sw_gpio_eint_get_enable(irq_gpio, &trigenabled);
	if(ret != 0) {
		pr_err("%s: sw_gpio_eint_get_enable() failed.\n", __func__);
	}else {
		pr_err("%s: trigger enabled is %d\n", __func__, trigenabled);
	}
#else
#if defined(INGENIC)
	irq_gpio = pdata->gpio_irq;
#else
	irq_gpio = of_get_gpio(np, 0);
#endif
	pr_err("%s: irq_gpio = %d\n", __func__, irq_gpio);
	if (!gpio_is_valid(irq_gpio)) return 0;
	if(irq_gpio) im501_irq	= gpio_to_irq(irq_gpio);
	pr_err("%s: im501_irq = %d\n", __func__, im501_irq);
	irq_set_irq_type(im501_irq, IRQ_TYPE_EDGE_RISING);
	gpio_request(irq_gpio, "im501_irq");
	if((ret = request_irq(im501_irq, im501_irq_handler, IRQF_TRIGGER_RISING, "im501_irq", im501_idx))) {
		pr_err("%s: irq %d request fail!\n", __func__, im501_irq);
		return ret;
	}
#endif

	return 0;
}

static int im501_spi_remove(struct spi_device *spi)
{
	pr_err("%s: entering...\n", __func__);

#if defined(CUBIETRUCK)
	sw_gpio_irq_free(im501_irq);
#else
	free_irq(im501_irq, im501_idx);
	gpio_free(im501_data->reset_gpio);
#endif

	flush_workqueue(im501_irq_wq);
	destroy_workqueue(im501_irq_wq);
#ifndef INGENIC
	gpio_free(im501_data->pdm_clki_gpio);
#endif
	return 0;
}

static struct spi_driver im501_spi_driver = {
	.driver = {
		.name = "im501_spi",
		.owner = THIS_MODULE,
#ifdef CONFIG_OF
		.of_match_table = of_match_ptr(im501_spi_dt_ids),
#endif		
	},
	.probe = im501_spi_probe,
	.remove = im501_spi_remove,
	//Henry add for try
	.id_table = im501_spi_id,
	//End of try
};
//module_spi_driver(im501_spi_driver);

static int __init im501_spi_init(void)
{
	int status;

	status = spi_register_driver(&im501_spi_driver);
	if (status < 0) {
		pr_err("%s: im501_spi_driver failure. status = %d\n", __func__, status);
	}
	pr_err("%s: im501_spi_driver success. status = %d\n", __func__, status);
	return status;
}
module_init(im501_spi_init);

static void __exit im501_spi_exit(void)
{
	spi_unregister_driver(&im501_spi_driver);
}
module_exit(im501_spi_exit);

MODULE_DESCRIPTION("IM501 SPI driver");
MODULE_AUTHOR("sample code <fuli@fortemedia.com>");
MODULE_LICENSE("GPL v2");
