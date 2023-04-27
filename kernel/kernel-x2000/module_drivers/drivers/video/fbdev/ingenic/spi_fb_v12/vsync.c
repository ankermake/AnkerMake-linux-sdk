/*
 * Copyright (C) 2005-2014, Ingenic Semiconductor Inc.
 * http://www.ingenic.cn/
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 *
 */

#include <linux/mm.h>
#include <linux/delay.h>
#include <linux/fb.h>
#include <linux/init.h>
#include <linux/wakelock.h>
#include <linux/kthread.h>
#include <linux/clk.h>
#include <asm/uaccess.h>
#include <linux/gpio.h>
#include <linux/interrupt.h>

#include "vfb.h"
#include "vsync.h"
#include "./ingenic_sfc_oled_v2/sfc.h"



#if 0
#define dprintk printk
#else
#define dprintk(sss, aaa...)						\
	do {								\
	} while (0)
#endif




extern struct sfc_flash *g_flash;
extern void *video_buf;
extern size_t videomem_size;
extern size_t xy_size;

static struct vsync_wq_t vsync_data;
static void *video_buf_dst = NULL;
static int volatile current_frm = 0;
static int volatile next_frm = 0;
static int volatile vsync_pan = 0;
static int volatile te_irq_count = 0;
extern int oled_te_gpio;

struct slcd_te {
	int te_gpio;
	int te_gpio_level;
	int te_irq_no;
	int refresh_request;
	int auto_refresh;
} slcd_te;

static int vsync_pan_yoffset(struct fb_info *info, int yoffset);

int hand_state =1;

// 194 * 368
#define TFT_HEIGHT 194
#define TFT_WIDTH  368
#define SKIP_WIDTH 0

int rotmemcpy32to24(void * src, void *dst, int cnt)
{
	unsigned int *p_src, *p_temp_src;
	unsigned char *p_dst, *p_temp_dst;

	unsigned int tmp_data;
	int i,j;

	p_src = (unsigned int *) src;
	p_dst = (unsigned char *) dst;


	if (hand_state == 1)
	{	//right hand, default
		p_src = p_src;	//first line
		p_dst = p_dst  + ((TFT_WIDTH-1) *TFT_HEIGHT)*3;	// last line

		for (i=0; i< TFT_HEIGHT; i++)	//126
		{
			p_temp_src = p_src;
			p_temp_dst = p_dst; 

			for (j=0; j<TFT_WIDTH; j++)	//294
			{
				tmp_data = *p_temp_src;

				*p_temp_dst = (tmp_data>>16)&0xff;
				*(p_temp_dst+1) = (tmp_data>>8)&0xff;
				*(p_temp_dst+2) = tmp_data&0xff;

				p_temp_src++;
				p_temp_dst -= TFT_HEIGHT*3;
			}
			p_dst += 3;	//one bit 3 byte
			p_src += (TFT_WIDTH+SKIP_WIDTH);
		}

	}else
	{	//left hand
		p_src = p_src;	//first line
		p_dst = p_dst + (TFT_HEIGHT-1)*3;	//last line 294-1*126

		for (i=0; i< TFT_HEIGHT; i++)
		{
			p_temp_src = p_src;
			p_temp_dst = p_dst; 

			for (j=0; j<TFT_WIDTH; j++)	//294
			{
				tmp_data = *p_temp_src;

				*p_temp_dst = (tmp_data>>16)&0xff;
				*(p_temp_dst+1) = (tmp_data>>8)&0xff;
				*(p_temp_dst+2) = tmp_data&0xff;

				p_temp_src++;
				p_temp_dst += TFT_HEIGHT*3;	//126
			}
//	ingenic_oled_update_video_buffer(g_flash,(char *)dst,cnt);
//msleep(200);
			p_dst -= 3;		//3 byte
			p_src += (TFT_WIDTH+SKIP_WIDTH);
		}
	}
}


/* ---------------------------------- te irq ------------------------------------- */

void request_te_vsync_refresh(struct jzfb *jzfb)
{
	struct slcd_te *te;
	te = &slcd_te;
	/* irq spin lock ? */

	//printk(KERN_DEBUG " %s te->refresh_request=%d\n", __FUNCTION__, te->refresh_request);

	if(te->refresh_request == 0) {
		te->refresh_request = 1;
		/* cleare gpio irq pendding */
		//enable_irq(te->te_irq_no);
	}

	return ;
}

static irqreturn_t jzfb_te_irq_handler(int irq, void *data)
{
	struct jzfb *jzfb = (struct jzfb *)data;
	struct slcd_te *te;

	te = &slcd_te;

	te_irq_count++;
	dprintk(KERN_DEBUG " %s() te_irq_count=%d, te->refresh_request=%d\n",
	       __FUNCTION__, te_irq_count, te->refresh_request);

	/* irq spin lock ? */
	if (1 || te_irq_count&1) {
		if (1 || te->refresh_request > 0) {
			//te->refresh_request = 0;
			/* wakeup draw thread... */
			vsync_pan = 1;
			wake_up_interruptible(&vsync_data.vsync_wq);
		}
	}

	/* disable te irq */
	//disable_irq_nosync(te->te_irq_no);

	return IRQ_HANDLED;
}

int jzfb_te_irq_register(struct jzfb *jzfb)
{
	int ret = 0;
	struct slcd_te *te;
	unsigned int te_irq_no;
	char * irq_name;

	int te_gpio = oled_te_gpio;
	int te_irq_level = IRQF_TRIGGER_RISING | IRQF_ONESHOT;// | IRQF_DISABLED;
	//int te_irq_level = IRQF_TRIGGER_FALLING | IRQF_ONESHOT;// | IRQF_DISABLED;

	te = &slcd_te;
	te->te_gpio = te_gpio;
	te->te_gpio_level = te_irq_level;
	te->te_irq_no = 0;
	te->refresh_request=1;
	te->auto_refresh = 1;

	printk(KERN_DEBUG " %s te_gpio=%d te_irq_level=%d\n", __FUNCTION__, te_gpio, te_irq_level);
	if(!gpio_is_valid(te_gpio)) {
		printk(" %s !gpio_is_valid(oled_te_gpio), te_gpio=%d te_irq_level=%d\n", __FUNCTION__, te_gpio, te_irq_level);
	}
#if 0
	if (te_gpio && gpio_request_one(te_gpio, GPIOF_DIR_IN, "slcd_te_gpio")) {
		dev_err(jzfb->dev, "gpio request slcd te gpio faile, te_gpio=%d\n", te_gpio);
		return -EBUSY;
	}
#endif
	te_irq_no = gpio_to_irq(te_gpio);
	printk(KERN_DEBUG " %s te_irq_no=%d\n", __FUNCTION__, te_irq_no);


	irq_name = (char *) kzalloc(30, GFP_KERNEL);
	sprintf(irq_name, "slcd_te_irq(%d:%d)", te_gpio, te_irq_level);

	if (request_irq(te_irq_no, jzfb_te_irq_handler,
			te_irq_level,
			irq_name, jzfb)) {
		dev_err(jzfb->dev,"slcd te request irq failed\n");
		ret = -EINVAL;
		goto err;
	}

	//disable_irq_nosync(te_irq_no);

	te->te_irq_no = te_irq_no;
err:

	return ret;
}

/* ---------------------------------- te irq ------------------------------------- */



int vsync_wait_irq(struct fb_info *info,unsigned long arg);


int jzfb_vsync_thread(void *data)
{
	int ret = 0;
	unsigned long delay = 1;
	struct jzfb * jzfb = (struct jzfb *)data;

	current_frm = -1;
	next_frm = 0;
	video_buf_dst = (void *)kmalloc(videomem_size*NUM_FRAME_BUFFERS,GFP_KERNEL);
	if(IS_ERR_OR_NULL(video_buf_dst)) {
		printk("%s(): alloc buffer failed\n", __func__);
		return -ENOMEM;
	}
	while (!kthread_should_stop()) {
		void * video_buf;
		ret = vsync_wait_irq(jzfb->fb,delay);
		if (unlikely(ret)){
			printk("%s() vsync_wait_irq error\n", __func__);
			//return -EFAULT;
		}

		if(slcd_te.refresh_request) {
			video_buf = video_buf_dst + videomem_size*next_frm;
			current_frm = next_frm;
			dprintk(KERN_DEBUG "ingenic_oled_update_video_buffer() start... next_frm=%d\n", next_frm);
			if(g_flash != NULL)
				ingenic_oled_update_video_buffer(g_flash,(char *)video_buf,xy_size*3);
			dprintk(KERN_DEBUG "ingenic_oled_update_video_buffer() finish... te->refresh_request = 0;\n");
			current_frm = -1;
			slcd_te.refresh_request = 0;
			//need_refresh = 0;
		}
		wake_up_interruptible(&vsync_data.pan_wq);

		if(slcd_te.auto_refresh && (te_irq_count&1)) {
			vsync_pan_yoffset(jzfb->fb, 0);
		}
	}
//				kfree(video_buf_dst);
	return 0;
}

int vsync_soft_set(void* data)
{
	static int delay;
	delay= data;
	init_waitqueue_head(&vsync_data.vsync_wq);
	init_waitqueue_head(&vsync_data.pan_wq);
	vsync_data.timestamp.rp = 0;
	vsync_data.timestamp.wp = 0;
	vsync_data.vsync_thread = kthread_run(jzfb_vsync_thread,data,"spi-fb-vsync");
	if(vsync_data.vsync_thread == ERR_PTR(-ENOMEM)){
		return -1;
	}
	return 0;
}

void vsync_soft_stop(void)
{
	kthread_stop(vsync_data.vsync_thread);
}

int vsync_wait_irq(struct fb_info *info,unsigned long arg)
{
	int ret;
//	printk("--->file=%s,func=%s,line=%d,vsync_pan=%d\n",__FILE__,__func__,__LINE__,vsync_pan);
	//unlock_fb_info(info);
	vsync_pan = 0 ;
	//wait_event_interruptible(vsync_data.vsync_wq,vsync_pan);
	ret = wait_event_interruptible_timeout(vsync_data.vsync_wq,vsync_pan, CONFIG_HZ*3);
	//printk(KERN_DEBUG "func=%s,line=%d, wait_event_interruptible_timeout() ret=%d\n",
	//       __func__,__LINE__, ret);
	if (!ret) {
		printk("func=%s,line=%d, wait_event_interruptible_timeout() HZ*3, ret=%d\n",
		       __func__,__LINE__, ret);
	}
	//lock_fb_info(info);
//	printk("--->file=%s,func=%s,line=%d,vsync_pan=%d\n",__FILE__,__func__,__LINE__,vsync_pan);
	return 0;
}

int vsync_wait(struct fb_info *info,unsigned long arg)
{
	return 0;
}
int vsync_wake(struct fb_info *info)
{
	//vsync_pan = 1;
	//wake_up_interruptible(&vsync_data.vsync_wq);
//	printk("--->file=%s,func=%s,line=%d,vsync_pan=%d\n",__FILE__,__func__,__LINE__,vsync_pan);
	return 0;
}

/*  */

static inline void * mem_cacheable_check(void *mem)
{
	u32 p = (u32)mem;
	p &= ~(1<<29);
	mem = (void*)p;
	return mem;
}

static int vsync_pan_yoffset(struct fb_info *info, int yoffset)
{
	struct slcd_te *te;
	struct jzfb *jzfb = info->par;
	void * vbuf;
	void * video_buf;

	te = &slcd_te;
	/* irq spin lock ? */
	vbuf = jzfb->vidmem;
	vbuf = mem_cacheable_check(vbuf);

	vbuf += yoffset*info->fix.line_length;

	next_frm = yoffset / info->var.yres;
	next_frm = min(next_frm, NUM_FRAME_BUFFERS-1);

	dprintk(KERN_DEBUG "%s() ~~~~~~~~videomem_size=%d,xy_size*3=%d,vidmem=%p,vbuf=%p,var->yoffset=%d\n",
	       __func__, videomem_size,xy_size*3,jzfb->vidmem,vbuf, yoffset);
//	printk(KERN_DEBUG "wait_event_interruptible() current_frm==%d, next_frm=%d\n", current_frm,next_frm);
//	if (current_frm == next_frm) {
//		wait_event_interruptible(vsync_data.pan_wq, (current_frm != next_frm));
//	}

	video_buf = video_buf_dst + videomem_size*next_frm;
	//dprintk(KERN_DEBUG "%s() memcpy32to24 start...\n", __func__);
	//memcpy32to24(vbuf,video_buf,xy_size*3);
	rotmemcpy32to24(vbuf,video_buf,xy_size*3);
	//dprintk(KERN_DEBUG "%s() memcpy32to24 end...\n", __func__);

	request_te_vsync_refresh(jzfb);

	return 0;
}


int vsync_pan_display(struct fb_info *info, struct fb_var_screeninfo *var)
{
	int ret;
	struct slcd_te *te;
	te = &slcd_te;
	te->auto_refresh = 0;	/* disable auto refresh */
	ret = vsync_pan_yoffset(info, var->yoffset);

	ret = wait_event_interruptible_timeout(vsync_data.pan_wq, (te->refresh_request==0), CONFIG_HZ*3);
	if (!ret) {
		printk("%s() wait_event_interruptible_timeout 3*HZ ...\n", __func__);
	}
	return ret;
}


/* auto refresh, 0: disable, 1: enable */
int vsync_set_auto_refresh(struct jzfb *jzfb, int auto_refresh)
{
	struct slcd_te *te;
	te = &slcd_te;
	te->auto_refresh = auto_refresh;

	return 0;
}
