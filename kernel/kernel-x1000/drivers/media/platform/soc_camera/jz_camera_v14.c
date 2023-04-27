/*
 * V4L2 Driver for Ingenic jz camera (CIM) host
 *
 * Copyright (C) 2014, Ingenic Semiconductor Inc.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 * This program is support Continuous physical address mapping,
 * not support sg mode now.
 */
#include <linux/clk.h>
#include <linux/device.h>
#include <linux/types.h>
#include <linux/dma-mapping.h>
#include <linux/mutex.h>
#include <linux/version.h>
#include <linux/platform_device.h>
#include <linux/module.h>
#include <linux/moduleparam.h>
#include <linux/device.h>
#include <linux/gpio.h>
#include <linux/interrupt.h>
#include <linux/slab.h>
#include <linux/videodev2.h>
#include <media/v4l2-ioctl.h>
#include <media/v4l2-common.h>
#include <media/v4l2-dev.h>
#include <media/soc_camera.h>
#include <media/soc_mediabus.h>
#include <asm/dma.h>
#include <media/videobuf2-dma-contig.h>
/*kzalloc*/
#include <soc/gpio.h>
#include <soc/base.h>
#include <mach/jz_camera.h>
#include <soc/vic_regs.h>
#ifdef CONFIG_MIPI_SENSOR
#include <soc/csi_regs.h>
#endif

//#define PRINT_CIM_REG
static int frame_size_check_flag = 1;
#define CIM_FRAME_BUF_CNT             2
static void cim_set_img(struct jz_camera_dev *pcdev,  unsigned int width, unsigned int height);

static inline unsigned int bit_field_max(int start, int end)
{
    return (1ul << (end - start + 1)) - 1;
}

static inline unsigned int bit_field_mask(int start, int end)
{
    return bit_field_max(start, end) << start;
}

static inline void set_bit_field(volatile unsigned int *reg, int start, int end, unsigned int val)
{
    unsigned int mask = bit_field_mask(start, end);
    *reg = (*reg & ~mask) | ((val << start) & mask);
}

static inline void cim_dump_reg(struct jz_camera_dev* pcdev)
{
    printk("==========dump vic reg============\n");
    printk("CONTROL(0x%x)         : 0x%08x\n",    VIC_CONTROL, readl(pcdev->vic_base + VIC_CONTROL));
    printk("RESOLUTION(0x%x)      : 0x%08x\n",    VIC_RESOLUTION, readl(pcdev->vic_base + VIC_RESOLUTION));
    printk("INTERFACE_TYPE(0x%x)  : 0x%08x\n",    VIC_INPUT_INTF, readl(pcdev->vic_base + VIC_INPUT_INTF));
    printk("BT_DVP_CFG(0x%x)      : 0x%08x\n",    VIC_INPUT_DVP, readl(pcdev->vic_base + VIC_INPUT_DVP));
    printk("IDI_TYPE_TYPE(0x%x)   : 0x%08x\n",    VIC_INPUT_MIPI, readl(pcdev->vic_base + VIC_INPUT_MIPI));
    printk("GLOBAL_CONFIGURE(0x%x): 0x%08x\n",  VIC_OUTPUT_CFG,  readl(pcdev->vic_base + VIC_OUTPUT_CFG));
    printk("VIC_STATE (0x%x)      : 0x%08x\n",VIC_STATE,    readl(pcdev->vic_base + VIC_STATE));
    printk("ISP_CLK_GATE (0x%x)      : 0x%08x\n",ISP_CLK_GATE,    readl(pcdev->vic_base + ISP_CLK_GATE));

    printk("DMA_CONFIGURE (0x%x)      : 0x%08x\n",DMA_CONFIGURE,    readl(pcdev->vic_base + DMA_CONFIGURE));
    printk("DMA_RESOLUTION (0x%x)      : 0x%08x\n",DMA_RESOLUTION,    readl(pcdev->vic_base + DMA_RESOLUTION));
    printk("DMA_RESET (0x%x)      : 0x%08x\n",DMA_RESET,    readl(pcdev->vic_base + DMA_RESET));
    printk("DMA_Y_CH_BANK_CTRL (0x%x)      : 0x%08x\n",DMA_Y_CH_BANK_CTRL,    readl(pcdev->vic_base + DMA_Y_CH_BANK_CTRL));
    printk("DMA_Y_CH_LINE_STRIDE (0x%x)      : 0x%08x\n",DMA_Y_CH_LINE_STRIDE,    readl(pcdev->vic_base + DMA_Y_CH_LINE_STRIDE));
    printk("DMA_Y_CH_BANK0_ADDR (0x%x)      : 0x%08x\n",DMA_Y_CH_BANK0_ADDR,    readl(pcdev->vic_base + DMA_Y_CH_BANK0_ADDR));
    printk("DMA_Y_CH_BANK1_ADDR (0x%x)      : 0x%08x\n",DMA_Y_CH_BANK1_ADDR,    readl(pcdev->vic_base + DMA_Y_CH_BANK1_ADDR));
    printk("DMA_Y_CH_BANK2_ADDR (0x%x)      : 0x%08x\n",DMA_Y_CH_BANK2_ADDR,    readl(pcdev->vic_base + DMA_Y_CH_BANK2_ADDR));
    printk("DMA_Y_CH_BANK3_ADDR (0x%x)      : 0x%08x\n",DMA_Y_CH_BANK3_ADDR,    readl(pcdev->vic_base + DMA_Y_CH_BANK3_ADDR));
    printk("DMA_Y_CH_BANK4_ADDR (0x%x)      : 0x%08x\n",DMA_Y_CH_BANK4_ADDR,    readl(pcdev->vic_base + DMA_Y_CH_BANK4_ADDR));
    printk("DMA_UV_CH_BANK_CTRL (0x%x)      : 0x%08x\n",DMA_UV_CH_BANK_CTRL,    readl(pcdev->vic_base + DMA_UV_CH_BANK_CTRL));
    printk("DMA_UV_CH_LINE_STRIDE (0x%x)      : 0x%08x\n",DMA_UV_CH_LINE_STRIDE,    readl(pcdev->vic_base + DMA_UV_CH_LINE_STRIDE));
    printk("DMA_UV_CH_BANK0_ADDR (0x%x)      : 0x%08x\n",DMA_UV_CH_BANK0_ADDR,    readl(pcdev->vic_base + DMA_UV_CH_BANK0_ADDR));
    printk("DMA_UV_CH_BANK1_ADDR (0x%x)      : 0x%08x\n",DMA_UV_CH_BANK1_ADDR,    readl(pcdev->vic_base + DMA_UV_CH_BANK1_ADDR));
    printk("DMA_UV_CH_BANK2_ADDR (0x%x)      : 0x%08x\n",DMA_UV_CH_BANK2_ADDR,    readl(pcdev->vic_base + DMA_UV_CH_BANK2_ADDR));
    printk("DMA_UV_CH_BANK3_ADDR (0x%x)      : 0x%08x\n",DMA_UV_CH_BANK3_ADDR,    readl(pcdev->vic_base + DMA_UV_CH_BANK3_ADDR));
    printk("DMA_UV_CH_BANK4_ADDR (0x%x)      : 0x%08x\n",DMA_UV_CH_BANK4_ADDR,    readl(pcdev->vic_base + DMA_UV_CH_BANK4_ADDR));

    printk("IRQ_CNT0 (0x%x)      : 0x%08x\n",IRQ_CNT0,    readl(pcdev->vic_base + IRQ_CNT0));
    printk("IRQ_CNTC1 (0x%x)      : 0x%08x\n",IRQ_CNTC1,    readl(pcdev->vic_base + IRQ_CNTC1));
    printk("IRQ_CNTCA (0x%x)      : 0x%08x\n",IRQ_CNTCA,    readl(pcdev->vic_base + IRQ_CNTCA));
    printk("IRQ_SRC (0x%x)      : 0x%08x\n",IRQ_SRC,    readl(pcdev->vic_base + IRQ_SRC));
    printk("IRQ_CNT_OVF (0x%x)      : 0x%08x\n",IRQ_CNT_OVF,    readl(pcdev->vic_base + IRQ_CNT_OVF));
    printk("IRQ_EN (0x%x)      : 0x%08x\n",IRQ_EN,    readl(pcdev->vic_base + IRQ_EN));
    printk("VIC_IRQ_MSK (0x%x)      : 0x%08x\n",VIC_IRQ_MSK,    readl(pcdev->vic_base + VIC_IRQ_MSK));
    printk("IRQ_CNT1 (0x%x)      : 0x%08x\n",IRQ_CNT1,    readl(pcdev->vic_base + IRQ_CNT1));
    printk("IRQ_CNT2 (0x%x)      : 0x%08x\n",IRQ_CNT2,    readl(pcdev->vic_base + IRQ_CNT2));
}

static int jz_camera_querycap(struct soc_camera_host *ici, struct v4l2_capability *cap)
{
	strlcpy(cap->card, "jz-Camera", sizeof(cap->card));
	cap->version = VERSION_CODE;
	cap->capabilities = V4L2_CAP_VIDEO_CAPTURE | V4L2_CAP_STREAMING;

	return 0;
}

static unsigned int jz_camera_poll(struct file *file, poll_table *pt)
{
	struct soc_camera_device *icd = file->private_data;

	return vb2_poll(&icd->vb2_vidq, file, pt);
}
static int jz_camera_alloc_desc(struct jz_camera_dev *pcdev, unsigned int count)
{
	struct jz_camera_dma_desc *dma_desc_paddr;
	struct jz_camera_dma_desc *dma_desc;

	pcdev->buf_cnt = count;
	pcdev->desc_vaddr = dma_alloc_coherent(pcdev->soc_host.v4l2_dev.dev,
			sizeof(*pcdev->dma_desc) * pcdev->buf_cnt,
			(dma_addr_t *)&pcdev->dma_desc, GFP_KERNEL);

	dma_desc_paddr = (struct jz_camera_dma_desc *) pcdev->dma_desc;
	dma_desc = (struct jz_camera_dma_desc *) pcdev->desc_vaddr;

	if (!pcdev->dma_desc)
		return -ENOMEM;

	return 0;
}

static void jz_dma_free_desc(struct jz_camera_dev *pcdev)
{
	if(pcdev && pcdev->desc_vaddr) {
		dma_free_coherent(pcdev->soc_host.v4l2_dev.dev,
				sizeof(*pcdev->dma_desc) * pcdev->buf_cnt,
				pcdev->desc_vaddr, (dma_addr_t )pcdev->dma_desc);

		pcdev->desc_vaddr = NULL;
	}
}
static int jz_queue_setup(struct vb2_queue *vq, const struct v4l2_format *fmt,
				unsigned int *nbuffers, unsigned int *nplanes,
				unsigned int sizes[], void *alloc_ctxs[]){
	struct soc_camera_device *icd = soc_camera_from_vb2q(vq);
	struct soc_camera_host *ici = to_soc_camera_host(icd->parent);
	struct jz_camera_dev *pcdev = ici->priv;
	int size;
	size = icd->sizeimage;

	if (!*nbuffers || *nbuffers > MAX_BUFFER_NUM)
		*nbuffers = MAX_BUFFER_NUM;

	if (size * *nbuffers > MAX_VIDEO_MEM)
		*nbuffers = MAX_VIDEO_MEM / size;

	*nplanes = 1;
	sizes[0] = size;
	alloc_ctxs[0] = pcdev->alloc_ctx;

	pcdev->start_streaming_called = 0;
	pcdev->sequence = 0;
	pcdev->active = NULL;

	if(jz_camera_alloc_desc(pcdev, *nbuffers))
		return -ENOMEM;

	dev_dbg(icd->parent, "%s, count=%d, size=%d\n", __func__,
		*nbuffers, size);

	return 0;
}
static int cnt = 0;
static int jz_init_dma(struct vb2_buffer *vb2) {
	struct soc_camera_device *icd = soc_camera_from_vb2q(vb2->vb2_queue);
	struct soc_camera_host *ici = to_soc_camera_host(icd->parent);
	struct jz_camera_dev *pcdev = ici->priv;
	struct jz_camera_dma_desc *dma_desc;
	u32 index = vb2->v4l2_buf.index;

	dma_desc = (struct jz_camera_dma_desc *) pcdev->desc_vaddr;

    dma_desc[index].id = index;
    dma_desc[index].buf = vb2_dma_contig_plane_dma_addr(vb2, 0);

	/* jz_camera_v13 support color format YUV422, RAW8.
	 *
	 * icd->current_fmt->host_fmt->fourcc == V4L2_PIX_FMT_YUY || V4L2_PIX_FMT_SBGGR8
	 * */

	if(index == 0) {
		pcdev->dma_desc_head = (struct jz_camera_dma_desc *)(pcdev->desc_vaddr);
	}

	if(index == (pcdev->buf_cnt - 1)) {
		dma_desc[index].next = 0;

		pcdev->dma_desc_tail = (struct jz_camera_dma_desc *)(&dma_desc[index]);
	} else {
		dma_desc[index].next = (dma_addr_t) (&pcdev->dma_desc[index + 1]);
	}

	return 0;
}

static int jz_buffer_init(struct vb2_buffer *vb2)
{
	struct jz_buffer *buf = container_of(vb2, struct jz_buffer, vb2);

	INIT_LIST_HEAD(&buf->list);

	return 0;
}
static void jz_buffer_queue(struct vb2_buffer *vb2)
{
	struct soc_camera_device *icd = soc_camera_from_vb2q(vb2->vb2_queue);
	struct soc_camera_host *ici = to_soc_camera_host(icd->parent);
	struct jz_camera_dev *pcdev = ici->priv;
	struct jz_buffer *buf = container_of(vb2, struct jz_buffer, vb2);
	unsigned long flags;
	struct jz_camera_dma_desc *dma_desc_vaddr;
	struct jz_camera_dma_desc *dma_desc_paddr;
	int index = vb2->v4l2_buf.index;

    /*** set addr to dma****/

    unsigned int addr[][2] = {
        {DMA_Y_CH_BANK0_ADDR, DMA_UV_CH_BANK0_ADDR},
        {DMA_Y_CH_BANK1_ADDR, DMA_UV_CH_BANK1_ADDR},
        {DMA_Y_CH_BANK2_ADDR, DMA_UV_CH_BANK2_ADDR},
        {DMA_Y_CH_BANK3_ADDR, DMA_UV_CH_BANK3_ADDR},
        {DMA_Y_CH_BANK4_ADDR, DMA_UV_CH_BANK4_ADDR},
    };


	spin_lock_irqsave(&pcdev->lock, flags);

	list_add_tail(&buf->list, &pcdev->video_buffer_list);
	if (pcdev->active == NULL) {
		pcdev->active = buf;
	}

	if(!pcdev->start_streaming_called) {
		goto out;
	}
	/* judged index */
	if((index > pcdev->buf_cnt) || (index < 0)) {
		dev_err(icd->parent,"Warning: %s, %d, index > pcdev->buf_cnt || index < 0, please check index !!!\n",
				 __func__, index);
		goto out;
	}

	dma_desc_vaddr = (struct jz_camera_dma_desc *) pcdev->desc_vaddr;

	if(pcdev->dma_desc_head != pcdev->dma_desc_tail) {
		pcdev->dma_desc_tail->next = (dma_addr_t) (&pcdev->dma_desc[index]);
		pcdev->dma_desc_tail = (struct jz_camera_dma_desc *)(&dma_desc_vaddr[index]);
	} else {
		if(pcdev->dma_stopped) {

			pcdev->dma_desc_head = (struct jz_camera_dma_desc *)(&dma_desc_vaddr[index]);
			pcdev->dma_desc_tail = (struct jz_camera_dma_desc *)(&dma_desc_vaddr[index]);

			dma_desc_paddr = (struct jz_camera_dma_desc *) pcdev->dma_desc;

			writel(dma_desc_vaddr[index].buf, pcdev->vic_base+addr[index][0]);
			writel((dma_desc_vaddr[index].buf + pcdev->uv_data_offset), pcdev->vic_base+addr[index][1]);

			pcdev->dma_stopped = 0;

		} else {

			pcdev->dma_desc_tail->next = (dma_addr_t) (&pcdev->dma_desc[index]);
			pcdev->dma_desc_tail = (struct jz_camera_dma_desc *)(&dma_desc_vaddr[index]);
		}
	}

out:
	spin_unlock_irqrestore(&pcdev->lock, flags);
	return;

}

static	int jz_buffer_prepare(struct vb2_buffer *vb)
{
	struct soc_camera_device *icd = soc_camera_from_vb2q(vb->vb2_queue);
	int ret = 0;

	dev_vdbg(icd->parent, "%s (vb=0x%p) 0x%p %lu\n", __func__,
		vb, vb2_plane_vaddr(vb, 0), vb2_get_plane_payload(vb, 0));

	vb2_set_plane_payload(vb, 0, icd->sizeimage);
	if (vb2_plane_vaddr(vb, 0) &&
	    vb2_get_plane_payload(vb, 0) > vb2_plane_size(vb, 0)) {
		ret = -EINVAL;
		return ret;
	}

	return 0;

}

static void jz_dma_start(struct jz_camera_dev *pcdev)
{
    unsigned int temp = 0, i;
    struct jz_camera_dma_desc *dma_desc;
    unsigned int addr[][2] = {
        {DMA_Y_CH_BANK0_ADDR, DMA_UV_CH_BANK0_ADDR},
        {DMA_Y_CH_BANK1_ADDR, DMA_UV_CH_BANK1_ADDR},
        {DMA_Y_CH_BANK2_ADDR, DMA_UV_CH_BANK2_ADDR},
        {DMA_Y_CH_BANK3_ADDR, DMA_UV_CH_BANK3_ADDR},
        {DMA_Y_CH_BANK4_ADDR, DMA_UV_CH_BANK4_ADDR},
    };

    writel(0x01ffffff , pcdev->vic_base + IRQ_CNTCA);

    dma_desc = (struct jz_camera_dma_desc *) pcdev->desc_vaddr;
    for(i = 0; i < pcdev->buf_cnt; i++) {
        writel(dma_desc[i].buf, pcdev->vic_base + addr[i][0]);
        writel((dma_desc[i].buf) + pcdev->uv_data_offset, pcdev->vic_base + addr[i][1]);
    }

    temp = readl(pcdev->vic_base + DMA_CONFIGURE);
    set_bit_field(&temp, Dma_en, 1);
    writel(temp, pcdev->vic_base + DMA_CONFIGURE);

}

static void jz_dma_stop(struct jz_camera_dev *pcdev)
{
    unsigned long timeout = 3000;
    unsigned int temp = 0;

    writel(0x01ffffff , pcdev->vic_base + IRQ_CNTCA);
    temp = readl(pcdev->vic_base + DMA_CONFIGURE);
    set_bit_field(&temp, Dma_en, 0);
    writel(temp, pcdev->vic_base + DMA_CONFIGURE);
    writel(DMA_RESET_EN, pcdev->vic_base + DMA_RESET);
    while(readl(pcdev->vic_base + DMA_RESET)) {
        if(--timeout == 0) {
            printk("#################### %s,%d (zxy: dma reset temeout!!!!!!!!! ) \n", __FUNCTION__,__LINE__);
            break;
        }
    }
}


static int jz_start_streaming(struct vb2_queue *q, unsigned int count)
{
    int ret;
    struct soc_camera_device *icd = soc_camera_from_vb2q(q);
    struct soc_camera_host *ici = to_soc_camera_host(icd->parent);
    struct jz_camera_dev *pcdev = ici->priv;
    struct jz_buffer *buf, *node;
    unsigned long flags;

    cim_set_img(pcdev, icd->user_width, icd->user_height);
    list_for_each_entry_safe(buf, node, &pcdev->video_buffer_list, list) {
        ret = jz_init_dma(&buf->vb2);
        if(ret) {
            dev_err(icd->parent,"%s:DMA initialization for Y/RGB failed\n", __func__);
            return ret;
        }
    }
 //   printk("~~~~~jz_stare_streaming vaddr %x ~~~~\n",pcdev->desc_vaddr);
    writel(VIC_RESET_EN, pcdev->vic_base + VIC_CONTROL);
    spin_lock_irqsave(&pcdev->lock, flags);

    enable_irq(pcdev->irq);
    jz_dma_start(pcdev);
    writel(VIC_START_EN, pcdev->vic_base + VIC_CONTROL);
#ifdef PRINT_CIM_REG
    cim_dump_reg(pcdev);
#endif
    pcdev->dma_stopped = 0;
    pcdev->start_streaming_called = 1;

    spin_unlock_irqrestore(&pcdev->lock, flags);
    return 0;
}

static int jz_stop_streaming(struct vb2_queue *q)
{
    struct soc_camera_device *icd = soc_camera_from_vb2q(q);
    struct soc_camera_host *ici = to_soc_camera_host(icd->parent);
    struct jz_camera_dev *pcdev = ici->priv;
    struct jz_buffer *buf, *node;
    unsigned long flags;

    spin_lock_irqsave(&pcdev->lock, flags);
    disable_irq(pcdev->irq);
    cnt++;
//    printk("#################### %s,%d (zxy: cnt = %d  ) \n", __FUNCTION__,__LINE__, cnt);
    jz_dma_stop(pcdev);

    /* Release all active buffers */
    list_for_each_entry_safe(buf, node, &pcdev->video_buffer_list, list) {
        list_del_init(&buf->list);
        vb2_buffer_done(&buf->vb2, VB2_BUF_STATE_ERROR);
    }

    pcdev->start_streaming_called = 0;
    pcdev->dma_stopped = 1;
    pcdev->active = NULL;

    spin_unlock_irqrestore(&pcdev->lock, flags);
    return 0;

}
static struct vb2_ops jz_videobuf2_ops = {
	.buf_init		= jz_buffer_init,
	.queue_setup		= jz_queue_setup,
	.buf_prepare		= jz_buffer_prepare,
	.buf_queue		= jz_buffer_queue,
	.start_streaming	= jz_start_streaming,
	.stop_streaming		= jz_stop_streaming,
	.wait_prepare		= soc_camera_unlock,
	.wait_finish		= soc_camera_lock,
};
static int jz_camera_init_videobuf2(struct vb2_queue *q, struct soc_camera_device *icd) {

	q->type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
	q->io_modes = VB2_MMAP | VB2_USERPTR;
	q->drv_priv = icd;
	q->buf_struct_size = sizeof(struct jz_buffer);
	q->ops = &jz_videobuf2_ops;
	q->mem_ops = &vb2_dma_contig_memops;
	q->timestamp_type = V4L2_BUF_FLAG_TIMESTAMP_MONOTONIC;

	return vb2_queue_init(q);
}

static int jz_camera_try_fmt(struct soc_camera_device *icd, struct v4l2_format *f) {
	struct v4l2_subdev *sd = soc_camera_to_subdev(icd);
	const struct soc_camera_format_xlate *xlate;
	struct v4l2_pix_format *pix = &f->fmt.pix;
	struct v4l2_mbus_framefmt mf;
	__u32 pixfmt = pix->pixelformat;
	int ret;
	/* TODO: limit to jz hardware capabilities */

	xlate = soc_camera_xlate_by_fourcc(icd, pix->pixelformat);
	if (!xlate) {
		dev_err(icd->parent,"Format %x not found\n", pix->pixelformat);
		return -EINVAL;
	}

	v4l_bound_align_image(&pix->width, 48, 2048, 1,
			&pix->height, 32, 2048, 0,
			pixfmt == V4L2_PIX_FMT_YUV422P ? 4 : 0);


	mf.width	= pix->width;
	mf.height	= pix->height;
	mf.field	= pix->field;
	mf.colorspace	= pix->colorspace;
	mf.code		= xlate->code;


	/* limit to sensor capabilities */
	ret = v4l2_subdev_call(sd, video, try_mbus_fmt, &mf);
	if (ret < 0)
		return ret;

	pix->width	= mf.width;
	pix->height	= mf.height;
	pix->field	= mf.field;
	pix->colorspace	= mf.colorspace;

	return 0;
}

static int jz_camera_set_fmt(struct soc_camera_device *icd, struct v4l2_format *f) {
	struct v4l2_subdev *sd = soc_camera_to_subdev(icd);
	const struct soc_camera_format_xlate *xlate;
	struct v4l2_pix_format *pix = &f->fmt.pix;
	struct v4l2_mbus_framefmt mf;
	int ret;
	xlate = soc_camera_xlate_by_fourcc(icd, pix->pixelformat);
	if (!xlate) {
		dev_err(icd->parent, "Format %x not found\n", pix->pixelformat);
		return -EINVAL;
	}

	mf.width        = pix->width;
	mf.height       = pix->height;
	mf.field        = pix->field;
	mf.colorspace   = pix->colorspace;
	mf.code         = xlate->code;

	ret = v4l2_subdev_call(sd, video, s_mbus_fmt, &mf); // set sensor size register
	if (ret < 0)
		return ret;

	if (mf.code != xlate->code)
		return -EINVAL;

	pix->width              = mf.width;
	pix->height             = mf.height;
	pix->field              = mf.field;
	pix->colorspace         = mf.colorspace;

	icd->current_fmt        = xlate;

	return ret;
}

static int jz_camera_set_crop(struct soc_camera_device *icd, const struct v4l2_crop *a) {
	struct v4l2_subdev *sd = soc_camera_to_subdev(icd);

	return v4l2_subdev_call(sd, video, s_crop, a);
}


static void cim_set_img(struct jz_camera_dev *pcdev,  unsigned int width, unsigned int height)
{
    struct vic_sensor_config* pcfg = pcdev->cfg;

    if (fmt_is_NV12(pcfg->info.data_fmt)) {
//        cim->frame_size = width * height;
//        cim->frame_size = ALIGN(cim->frame_size, VIC_ALIGN_SIZE);
        pcdev->uv_data_offset = 0;//cim->frame_size;
//        cim->frame_size += cim->frame_size / 2;
    } else {
//        cim->frame_size = width * 2 * height;
        pcdev->uv_data_offset = 0;
    }

//    cim->frame_size = ALIGN(cim->frame_size, VIC_ALIGN_SIZE);
//    cim->uv_data_offset = pcfg->info.uv_data_offset;
}

static void init_dvp_dma(struct soc_camera_device *icd)
{
	struct soc_camera_host *ici = to_soc_camera_host(icd->parent);
	struct jz_camera_dev *pcdev = ici->priv;
	unsigned int temp = 0;
	struct vic_sensor_config *pcfg = pcdev->cfg;

	unsigned int base_mode = 0;
	unsigned int y_stride = 0;
	unsigned int uv_stride = 0;
	switch (pcfg->dvp_cfg_info.dvp_data_fmt) {
    case DVP_RAW8:
    case DVP_RAW10:
    case DVP_RAW12:
        base_mode = 0;
        y_stride = icd->user_width * 2;
        break;

    case DVP_YUV422:
        if (pcfg->info.data_fmt == fmt_NV12) {
            base_mode = 6;
            uv_stride = 0;
            y_stride = icd->user_width;
        } else if (pcfg->info.data_fmt == fmt_NV21) {
            base_mode = 7;
            uv_stride = icd->user_width;
            y_stride = icd->user_width;
        } else {
            base_mode = 3;
            y_stride = icd->user_width * 2;
        }
        break;

    default:
        break;
    }

    writel(y_stride, pcdev->vic_base + DMA_Y_CH_LINE_STRIDE);
    writel(uv_stride, pcdev->vic_base + DMA_UV_CH_LINE_STRIDE);

    temp = 0;
//    set_bit_field(&temp, Dma_en, 1);
    set_bit_field(&temp, Buffer_number, pcdev->buf_cnt - 1);
    set_bit_field(&temp, Base_mode, base_mode);
    set_bit_field(&temp, Yuv422_order, 3);
    writel(temp, pcdev->vic_base + DMA_CONFIGURE);
}

static void init_mipi_dma(struct soc_camera_device *icd)
{
	struct soc_camera_host *ici = to_soc_camera_host(icd->parent);
	struct jz_camera_dev *pcdev = ici->priv;
	unsigned int temp = 0;
	struct vic_sensor_config *pcfg = pcdev->cfg;
	unsigned int base_mode = 0;
	unsigned int y_stride = 0;
	unsigned int uv_stride = 0;

    switch (pcfg->mipi_cfg_info.data_fmt) {
    case MIPI_RAW8:
            base_mode = 3;
            y_stride = pcfg->info.xres;
            break;
    case MIPI_RAW10:
    case MIPI_RAW12:
        base_mode = 0;
        y_stride = pcfg->info.xres * 2;
        break;

    case MIPI_YUV422:
        if (pcfg->info.data_fmt == fmt_NV12) {
            base_mode = 6;
            uv_stride = pcfg->info.xres;
            y_stride = pcfg->info.xres;
        } else if (pcfg->info.data_fmt == fmt_NV21) {
            base_mode = 7;
            uv_stride = pcfg->info.xres;
            y_stride = pcfg->info.xres;
        } else {
            base_mode = 3;
            y_stride = pcfg->info.xres * 2;
        }
        break;

    default:
        break;
    }

    writel(y_stride, pcdev->vic_base + DMA_Y_CH_LINE_STRIDE);
    writel(uv_stride, pcdev->vic_base + DMA_UV_CH_LINE_STRIDE);

    temp = 0;
//    set_bit_field(&temp, Dma_en, 1);
    set_bit_field(&temp, Buffer_number, pcdev->buf_cnt - 1);
    set_bit_field(&temp, Base_mode, base_mode);
    set_bit_field(&temp, Yuv422_order, 3);
    writel(temp, pcdev->vic_base + DMA_CONFIGURE);
}

static void set_irq_regs(struct jz_camera_dev *pcdev)
{
    unsigned long temp;
    temp = 0;
    set_bit_field(&temp, DMA_DVP_OVF, 1);
    set_bit_field(&temp, DMA_FRD, 1);
    set_bit_field(&temp, CORE_FIFO_FAIL, 1);
    set_bit_field(&temp, VIC_HVF, 1);
    set_bit_field(&temp, VIC_IMVE, 1);
    set_bit_field(&temp, VIC_IMHE, 1);
    set_bit_field(&temp, VIC_DVP_OVF, 1);
    set_bit_field(&temp, VIC_FRD, 1);
    writel(temp, pcdev->vic_base + IRQ_EN);
}

static void set_vic_input_dvp(struct soc_camera_device *icd)
{
    struct soc_camera_host *ici = to_soc_camera_host(icd->parent);
    struct jz_camera_dev *pcdev = ici->priv;
    unsigned int temp = 0;
    struct vic_sensor_config *pcfg = pcdev->cfg;

    set_bit_field(&temp, DVP_DATA_FORMAT, pcfg->dvp_cfg_info.dvp_data_fmt);

    set_bit_field(&temp, YUV_DATA_ORDER, pcfg->dvp_cfg_info.dvp_yuv_data_order);
    set_bit_field(&temp, DVP_TIMING_MODE, pcfg->dvp_cfg_info.dvp_timing_mode);
    set_bit_field(&temp, HSYNC_POLAR, pcfg->dvp_cfg_info.dvp_hsync_polarity);
    set_bit_field(&temp, VSYNC_POLAR, pcfg->dvp_cfg_info.dvp_vsync_polarity);
    set_bit_field(&temp, INTERLACE_EN, pcfg->dvp_cfg_info.dvp_img_scan_mode);

    if (pcfg->dvp_cfg_info.dvp_gpio_mode == DVP_PA_HIGH_8BIT ||
    pcfg->dvp_cfg_info.dvp_gpio_mode == DVP_PA_HIGH_10BIT)
        set_bit_field(&temp, DVP_RAW_ALIGN, 1);
    else
        set_bit_field(&temp, DVP_RAW_ALIGN, 0);

    writel(temp, pcdev->vic_base + VIC_INPUT_DVP);

}

static void set_vic_dma(struct soc_camera_device *icd)
{
    struct soc_camera_host *ici = to_soc_camera_host(icd->parent);
    struct jz_camera_dev *pcdev = ici->priv;
    unsigned int temp = 0;
    struct vic_sensor_config *pcfg = pcdev->cfg;

    /* reset dma */
    writel(DMA_RESET_EN, pcdev->vic_base + DMA_RESET);
#ifdef CONFIG_MIPI_SENSOR
    if(pcfg->mipi_cfg_info.data_fmt == MIPI_RAW8)
        set_bit_field(&temp, DMA_HORIZONTAL_RESOLUTION, icd->user_width / 2);
    else
        set_bit_field(&temp, DMA_HORIZONTAL_RESOLUTION, icd->user_width);
#else
        set_bit_field(&temp, DMA_HORIZONTAL_RESOLUTION, icd->user_width);
#endif

    set_bit_field(&temp, DMA_VERTICAL_RESOLUTION, icd->user_height);
    writel(temp, pcdev->vic_base + DMA_RESOLUTION);
    if(pcfg->vic_interface == VIC_dvp)
        init_dvp_dma(icd);
    else if(pcfg->vic_interface == VIC_mipi_csi)
        init_mipi_dma(icd);
}

#ifdef CONFIG_MIPI_SENSOR
static inline void csi_set_bit(struct jz_camera_dev* pcdev, unsigned int reg, int start, int end, unsigned int val)
{
    set_bit_field(pcdev->mipi_base + reg, start, end, val);
}

static inline void mipi_csi_dphy_test_clear(struct jz_camera_dev *pcdev, int value)
{
    csi_set_bit(pcdev, MIPI_PHY_TST_CTRL0, CSI_PHY_test_ctrl0_testclr, value);
}

static inline void mipi_csi_dphy_set_lanes(struct jz_camera_dev *pcdev, int lanes)
{
    csi_set_bit(pcdev, MIPI_N_LANES, CSI_PHY_n_lanes, lanes-1);
}

static inline void mipi_csi_dphy_phy_shutdown(struct jz_camera_dev *pcdev, int enable)
{
    csi_set_bit(pcdev, MIPI_PHY_SHUTDOWNZ, CSI_PHY_phy_shutdown, enable);
}


static inline void mipi_csi_dphy_dphy_reset(struct jz_camera_dev *pcdev, int state)
{
    csi_set_bit(pcdev, MIPI_DPHY_RSTZ, CSI_PHY_dphy_reset, state);
}

static inline void mipi_csi_dphy_csi2_reset(struct jz_camera_dev *pcdev, int state)
{
    csi_set_bit(pcdev, MIPI_CSI2_RESETN, CSI_PHY_csi2_reset, state);
}

static inline void csi_write_reg(struct jz_camera_dev *pcdev, unsigned int reg, int val)
{
    writel(val, pcdev->mipi_base + reg);
}

static inline unsigned int csi_read_reg(struct jz_camera_dev *pcdev, unsigned int reg)
{
    return readl(pcdev->mipi_base + reg);
}

static unsigned char mipi_csi_event_disable(struct jz_camera_dev *pcdev, unsigned int  mask, unsigned char err_reg_no)
{
    switch (err_reg_no) {
    case CSI_ERR_MASK_REGISTER1:
        csi_write_reg(pcdev, MIPI_MASK1, mask | csi_read_reg(pcdev, MIPI_MASK1));
        break;
    case CSI_ERR_MASK_REGISTER2:
        csi_write_reg(pcdev, MIPI_MASK2, mask | csi_read_reg(pcdev, MIPI_MASK2));
        break;
    default:
        return CSI_ERR_OUT_OF_BOUND;
    }
    return 0;
}


static inline int mipi_csi_phy_configure(struct jz_camera_dev *pcdev, int lanes)
{
    mipi_csi_dphy_test_clear(pcdev, 1);
    mdelay(10);

    mipi_csi_dphy_set_lanes(pcdev, lanes);

    /*reset phy*/
    mipi_csi_dphy_phy_shutdown(pcdev, 0);
    mipi_csi_dphy_dphy_reset(pcdev, 0);
    mipi_csi_dphy_csi2_reset(pcdev, 0);
#if 0
    /* test phy loop */
    unsigned char data[4];

    udelay(15);

    mipi_csi_dphy_test_clear(0);

    mipi_csi_dphy_test_data_in(0);

    mipi_csi_dphy_test_en(0);
    mipi_csi_dphy_test_clock(0);

    data[0]=0x00;
    mipi_csi_dphy_test_loop(0x44,data, 1);

    data[0]=0x1e;
    mipi_csi_dphy_test_loop(0xb0,data, 1);

    data[0]=0x1;
    mipi_csi_dphy_test_loop(0xb1,data, 1);
#endif
    mdelay(10);
    mipi_csi_dphy_phy_shutdown(pcdev, 1);
    mipi_csi_dphy_dphy_reset(pcdev, 1);
    mipi_csi_dphy_csi2_reset(pcdev, 1);
    mipi_csi_event_disable(pcdev, 0xffffffff, CSI_ERR_MASK_REGISTER1);
    mipi_csi_event_disable(pcdev, 0xffffffff, CSI_ERR_MASK_REGISTER2);
    mdelay(10);

    //mipi_csi_dump_regs();
    /* wait for phy ready */
    return 0;
}

static inline int mipi_csi_dphy_get_state(struct jz_camera_dev *pcdev)
{
    return csi_read_reg(pcdev, MIPI_PHY_STATE);
}

static int mipi_csi_phy_ready(struct jz_camera_dev *pcdev, int lanes)
{
    int ready;
    int ret = 0;

    ready = mipi_csi_dphy_get_state(pcdev);
    ret = ready & (1 << CSI_PHY_STATE_CLK_LANE_STOP );
    switch (lanes) {
    case 2:
        ret = ret && (ready & (1 << CSI_PHY_STATE_DATA_LANE1));
    case 1:
        ret = ret && (ready & (1 << CSI_PHY_STATE_DATA_LANE0));
        break;
    default:
        printk("Do not support lane num more than 2!\n");
    }

    return !!ret;
}


static int mipi_csi_phy_initialization(struct jz_camera_dev *pcdev, struct mipi_csi_bus_info *mipi_info)
{
    int retries = 30;
    int ret = 0;
    int i;

    mipi_csi_phy_configure(pcdev, mipi_info->lanes);

    /* wait for phy ready */
    for (i = 0; i < retries; i++) {
        if (mipi_csi_phy_ready(pcdev, mipi_info->lanes))
            break;

        udelay(2);
    }

    if (i >= retries) {
        ret = -1;
        printk("csi init failure!\n");
        return ret;
    }

    return ret;
}
#endif

static int jz_camera_set_bus_param(struct soc_camera_device *icd) {
	struct soc_camera_host *ici = to_soc_camera_host(icd->parent);
	struct jz_camera_dev *pcdev = ici->priv;
	unsigned int temp = 0;
	struct vic_sensor_config *pcfg = pcdev->cfg;
	int timeout = 3000;

	printk("#################### %s,%d,  pcdev->vic_base:0x%x, w:%d, %d  !!!!\n",
	__FUNCTION__, __LINE__, pcdev->vic_base,icd->user_width,icd->user_height);


	/* reset vic 控制器 */
	writel(VIC_RESET_EN, pcdev->vic_base + VIC_CONTROL);

	temp = 0;
#ifdef CONFIG_MIPI_SENSOR
        if(pcfg->mipi_cfg_info.data_fmt == MIPI_RAW8)
            set_bit_field(&temp, HORIZONTAL_RESOLUTION, icd->user_width/2);
        else
            set_bit_field(&temp, HORIZONTAL_RESOLUTION, icd->user_width);
#else
            set_bit_field(&temp, HORIZONTAL_RESOLUTION, icd->user_width);
#endif

	set_bit_field(&temp, VERTICAL_RESOLUTION, icd->user_height);
	writel(temp, pcdev->vic_base + VIC_RESOLUTION);

	writel(pcfg->vic_interface, pcdev->vic_base + VIC_INPUT_INTF);
    if(pcfg->vic_interface == VIC_dvp)
        set_vic_input_dvp(icd);
#ifdef CONFIG_MIPI_SENSOR
    else if(pcfg->vic_interface == VIC_mipi_csi)
        if(pcfg->mipi_cfg_info.data_fmt == MIPI_RAW8)
            writel(MIPI_YUV422, pcdev->vic_base + VIC_INPUT_MIPI);
        else
            writel(pcfg->mipi_cfg_info.data_fmt, pcdev->vic_base + VIC_INPUT_MIPI);
#endif

	temp = 0;
	if(pcfg->vic_interface == VIC_dvp)
		set_bit_field(&temp, ISP_PORT_MOD, pcfg->isp_port_mode);
	else if (pcfg->vic_interface == VIC_mipi_csi)
		set_bit_field(&temp, ISP_PORT_MOD, 0);
    set_bit_field(&temp, VCKE_ENA_BLE, 0);
    set_bit_field(&temp, BLANK_ENABLE, 0);
    set_bit_field(&temp, AB_MODE_SELECT, 0);
    writel(temp, pcdev->vic_base + VIC_OUTPUT_CFG);

    temp = 0;
    set_bit_field(&temp, DMA_CLK_GATE_EN, 0);
    set_bit_field(&temp, VPCLK_GATE_EN, 0);
    writel(temp, pcdev->vic_base + ISP_CLK_GATE);

    writel(REG_ENABLE_EN, pcdev->vic_base + VIC_CONTROL);
    while(readl(pcdev->vic_base + VIC_CONTROL) & REG_ENABLE_EN)
    {
        if(--timeout == 0)
        {
            printk("timeout while wait vic_reg_enable: %x\n", readl(pcdev->vic_base + VIC_CONTROL));
            break;
        }
    }

    set_vic_dma(icd);
    set_irq_regs(pcdev);

#ifdef CONFIG_MIPI_SENSOR
    mipi_csi_phy_initialization(pcdev, &pcfg->mipi_cfg_info);
#endif
    writel(readl(pcdev->vic_base + VIC_CONTROL) | VIC_START_EN, pcdev->vic_base + VIC_CONTROL);
    while(readl(pcdev->vic_base + VIC_CONTROL))
    {
        if(--timeout == 0)
        {
            printk("timeout while wait vic_reg_enable: %x\n", readl(pcdev->vic_base + VIC_CONTROL));
            break;
        }
    }
	return 0;
}
static void jz_camera_activate(struct jz_camera_dev *pcdev) { // sxyzhang needed modified
	int ret = -1;
    unsigned long tmp;

	if(pcdev->mclk) {
		ret = clk_set_rate(pcdev->mclk, pcdev->mclk_freq);
		ret = clk_enable(pcdev->mclk);
	}
	if(pcdev->isp_clk) {
		ret = clk_enable(pcdev->isp_clk);
	}

	if(pcdev->cgu_isp_clk) {
		ret = clk_set_rate(pcdev->cgu_isp_clk,100000000);
		ret = clk_enable(pcdev->cgu_isp_clk);
	}

	if(pcdev->csi_clk) {
		ret = clk_enable(pcdev->csi_clk);
	}

    tmp = clk_get_rate(pcdev->mclk);
    printk("#################### %s,%d (zxy:mclk: %ld   ) \n", __FUNCTION__,__LINE__, tmp);
    tmp = clk_get_rate(pcdev->cgu_isp_clk);
    printk("#################### %s,%d (zxy:cgu_isp_clk: %ld   ) \n", __FUNCTION__,__LINE__, tmp);

	if(ret) {
		dev_err(NULL, "enable clock failed!\n");
	}

	msleep(10);
}
static void jz_camera_deactivate(struct jz_camera_dev *pcdev) {
    unsigned int temp = 0, timeout = 3000;

	if(pcdev->mclk) {
		clk_disable(pcdev->mclk);
	}
	if(pcdev->isp_clk) {
		clk_disable(pcdev->isp_clk);
	}

	if(pcdev->cgu_isp_clk) {
		clk_disable(pcdev->cgu_isp_clk);
	}

	if(pcdev->csi_clk) {
		clk_disable(pcdev->csi_clk);
	}
    writel(VIC_GLB_SAFE_RST_EN, pcdev->vic_base + VIC_CONTROL);
    temp = readl(pcdev->vic_base + DMA_CONFIGURE);
    set_bit_field(&temp, Dma_en, 0);
    writel(temp, pcdev->vic_base + DMA_CONFIGURE);
    writel(DMA_RESET_EN, pcdev->vic_base + DMA_RESET);
    while(readl(pcdev->vic_base + DMA_RESET)) {
        if(--timeout == 0) {
            printk("#################### %s,%d (zxy: dma reset temeout!!!!!!!!! ) \n", __FUNCTION__,__LINE__);
            break;
        }
    }
}

static int jz_camera_add_device(struct soc_camera_device *icd) {
	struct soc_camera_host *ici = to_soc_camera_host(icd->parent);
	struct jz_camera_dev *pcdev = ici->priv;

	int icd_index = icd->devnum;

	if (pcdev->icd[icd_index])
		return -EBUSY;

	dev_dbg(icd->parent, "jz Camera driver attached to camera %d\n",
			icd->devnum);

	pcdev->icd[icd_index] = icd;

	jz_camera_activate(pcdev);

	return 0;
}
static void jz_camera_remove_device(struct soc_camera_device *icd) {
	struct soc_camera_host *ici = to_soc_camera_host(icd->parent);
	struct jz_camera_dev *pcdev = ici->priv;

	int icd_index = icd->devnum;

	BUG_ON(icd != pcdev->icd[icd_index]);

	jz_camera_deactivate(pcdev);
	dev_dbg(icd->parent, "jz Camera driver detached from camera %d\n",
			icd->devnum);

	jz_dma_free_desc(pcdev);
	pcdev->icd[icd_index] = NULL;
}

static irqreturn_t jz_camera_irq_handler(int irq, void *data) {
	struct jz_camera_dev *pcdev = (struct jz_camera_dev *)data;
	unsigned long status = 0;
	unsigned long flags = 0;
	int index = 0;
	struct jz_camera_dma_desc *dma_desc_paddr;
	for (index = 0; index < ARRAY_SIZE(pcdev->icd); index++) {
		if (pcdev->icd[index]) {
			break;
		}
	}

	if(index == MAX_SOC_CAM_NUM)
		return IRQ_HANDLED;

	/* judged pcdev->dma_desc_head->id */
	if((pcdev->dma_desc_head->id > pcdev->buf_cnt) || (pcdev->dma_desc_head->id < 0)) {
		dev_dbg(NULL, "Warning: %s, %d, pcdev->dma_desc_head->id >pcdev->buf_cnt ||pcdev->dma_desc_head->id < 0, please check pcdev->dma_desc_head->id !!!\n",__func__, pcdev->dma_desc_head->id);
		return IRQ_NONE;
	}

	spin_lock_irqsave(&pcdev->lock, flags);

	/* read interrupt status register */
	status = readl(pcdev->vic_base + IRQ_SRC);
	if (!status) {
		dev_err(NULL, "status is NULL! \n");
		spin_unlock_irqrestore(&pcdev->lock, flags);

		return IRQ_NONE;
	}

    if (status & ST_DMA_DVP_OVF)
        printk( "DMA's async fifo overflow error occurs interrupt!\n");
    if (status & ST_CORE_FIFO_FAIL)
        printk("ISP_core dma fifo empty fail or full fail occurs interrupt!\n");
    if (status & ST_VIC_HVF)
        printk("VIC BT656/1120 hvf error occurs interrupt!\n");
    if (status & ST_VIC_IMVE)
        printk("VIC image vertical error interrupt!\n");
    if (status & ST_VIC_IMHE)
        printk("VIC image horizon error interrupt!\n");
    if (status & ST_VIC_MIPI_OVF)
        printk("VIC's mipi csi channel assync fifo overflow error occurs interrupt!\n");
    if (status & ST_VIC_DVP_OVF)
        printk("VIC's dvp channel assync fifo overflow error occurs interrupt!\n");

	if(status & ST_DMA_FRD)
	{
		writel(ST_DMA_FRD , pcdev->vic_base + IRQ_CNTCA);
            printk("dma done interrupt!\n");
		if(pcdev->active) {

			struct vb2_buffer *vb2 = &pcdev->active->vb2;
			struct jz_buffer *buf = container_of(vb2, struct jz_buffer, vb2);

			list_del_init(&buf->list);
			v4l2_get_timestamp(&vb2->v4l2_buf.timestamp);
			vb2->v4l2_buf.sequence = pcdev->sequence++;
			vb2_buffer_done(vb2, VB2_BUF_STATE_DONE);

			if((vb2->v4l2_buf.sequence % 60) == 0)  {
				pcdev->debug_ms_start = ktime_to_ms(ktime_get_real());
			} else if((vb2->v4l2_buf.sequence % 60) == 59) {
				long long debug_ms_end = ktime_to_ms(ktime_get_real());
				long long ms = debug_ms_end - pcdev->debug_ms_start;

				long long fps = 60 * 1000;
				do_div(fps, ms);

				printk("===fps: %lld, start: %lld, end: %lld, sequence: %d\n",\
						fps, pcdev->debug_ms_start, debug_ms_end, pcdev->sequence);
			}
		}

		if (list_empty(&pcdev->video_buffer_list)) {
			pcdev->active = NULL;
			spin_unlock_irqrestore(&pcdev->lock, flags);
			return IRQ_HANDLED;
		}

		if(pcdev->dma_desc_head != pcdev->dma_desc_tail) {
			pcdev->dma_desc_head =
				(struct jz_camera_dma_desc *)UNCAC_ADDR(phys_to_virt(pcdev->dma_desc_head->next));
		}
		if(pcdev->dma_stopped && !list_empty(&pcdev->video_buffer_list)) {
			/* dma stop condition:
			 * 1. dma desc reach the end, and there is no more desc to be transferd.
			 *    dma need to stop.
			 * 2. if the capture list not empty, we should restart dma here.
			 *
			 * */

			dma_desc_paddr = (struct jz_camera_dma_desc *) pcdev->dma_desc;
			pcdev->dma_stopped = 0;
		}
		/* start next dma frame. */
		pcdev->active = list_entry(pcdev->video_buffer_list.next, struct jz_buffer, list);

		spin_unlock_irqrestore(&pcdev->lock, flags);

		return IRQ_HANDLED;

	}
        writel(status, pcdev->vic_base + IRQ_CNTCA);
	spin_unlock_irqrestore(&pcdev->lock, flags);

	return IRQ_HANDLED;
}

static ssize_t frame_size_check_r(struct device *dev, struct device_attribute *attr, char *buf)
{
	snprintf(buf, 100, " 0: disable frame size check.\n 1: enable frame size check.\n");
	return 100;
}

static ssize_t frame_size_check_w(struct device *dev, struct device_attribute *attr, const char *buf, size_t count)
{
        if (count != 2)
                return -EINVAL;

        if (*buf == '0') {
                frame_size_check_flag = 0;
        } else if (*buf == '1') {
                frame_size_check_flag = 1;
        } else {
                return -EINVAL;
        }

        return count;
}

/**********************cim_debug***************************/
static DEVICE_ATTR(dump_cim_reg, S_IRUGO|S_IWUSR, cim_dump_reg, NULL);
static DEVICE_ATTR(frame_size_check, S_IRUGO|S_IWUSR, frame_size_check_r, frame_size_check_w);

static struct attribute *cim_debug_attrs[] = {
	&dev_attr_dump_cim_reg.attr,
	&dev_attr_frame_size_check.attr,
	NULL,
};

const char cim_group_name[] = "debug";
static struct attribute_group cim_debug_attr_group = {
	.name	= cim_group_name,
	.attrs	= cim_debug_attrs,
};



static struct soc_camera_host_ops jz_soc_camera_host_ops = {
	.owner = THIS_MODULE,
	.add = jz_camera_add_device,
	.remove = jz_camera_remove_device,
	.set_fmt = jz_camera_set_fmt,
	.try_fmt = jz_camera_try_fmt,
	.init_videobuf2 = jz_camera_init_videobuf2,
	.poll = jz_camera_poll,
	.querycap = jz_camera_querycap,
	.set_crop = jz_camera_set_crop,
	.set_bus_param = jz_camera_set_bus_param,
};

static int __init jz_camera_probe(struct platform_device *pdev) {
	int err = 0, ret = 0;
	unsigned int irq;
	struct resource *vic_res;
	void __iomem *vic_base;
#ifdef CONFIG_MIPI_SENSOR
    	struct resource *mipi_res;
	void __iomem *mipi_base;
#endif
	struct jz_camera_dev *pcdev;
    struct vic_sensor_config* pcfg;
    struct vic_sensor_config* (*pget_sensor_data)(void);

    pget_sensor_data = pdev->dev.platform_data;
    pcfg = pget_sensor_data();

	/* malloc */
	pcdev = kzalloc(sizeof(*pcdev), GFP_KERNEL);
	if (!pcdev) {
		dev_err(&pdev->dev, "Could not allocate pcdev\n");
		err = -ENOMEM;
		goto err_kzalloc;
	}

    if(pcfg)
	pcdev->cfg = pcfg;

	/* resource */
	vic_res = platform_get_resource(pdev, IORESOURCE_MEM, 0);
	if(!vic_res) {
		dev_err(&pdev->dev, "Could not get resource!\n");
		err = -ENODEV;
		goto err_get_resource;
	}
#ifdef CONFIG_MIPI_SENSOR
	mipi_res = platform_get_resource(pdev, IORESOURCE_MEM, 1);
	if(!mipi_res) {
		dev_err(&pdev->dev, "Could not get resource!\n");
		err = -ENODEV;
		goto err_get_mipi_resource;
	}
#endif

	/* irq */
	irq = platform_get_irq(pdev, 0);
	if (irq <= 0) {
		dev_err(&pdev->dev, "Could not get irq!\n");
		err = -ENODEV;
		goto err_get_irq;
	}

	/*get cim1 clk*/
	pcdev->mclk = clk_get(NULL, "cgu_cim");
	if (IS_ERR(pcdev->mclk)) {
		err = PTR_ERR(pcdev->mclk);
		dev_err(&pdev->dev, "%s:can't get clk %s\n", __func__, "cgu_cim");
		goto err_clk_get_mclk;
	}

	pcdev->isp_clk = clk_get(NULL, "isp");//GATE
	if (IS_ERR(pcdev->isp_clk)) {
		err = PTR_ERR(pcdev->isp_clk);
		dev_err(&pdev->dev, "%s:can't get clk %s\n",
				__func__, "isp");
		goto err_clk_get_isp_clk;
	}

	pcdev->cgu_isp_clk = clk_get(NULL, "cgu_isp");
	if (IS_ERR(pcdev->cgu_isp_clk)) {
		err = PTR_ERR(pcdev->cgu_isp_clk);
		dev_err(&pdev->dev, "%s:can't get clk %s\n",
				__func__, "cgu_isp");
		goto err_clk_get_cgu_isp_clk;
	}

#ifdef CONFIG_MIPI_SENSOR
	pcdev->csi_clk = clk_get(NULL, "csi");
	if (IS_ERR(pcdev->csi_clk)) {
		err = PTR_ERR(pcdev->csi_clk);
		dev_err(&pdev->dev, "%s:can't get clk %s\n",
		__func__, "csi");
		goto err_clk_get_csi_clk;
	}
#endif

	pcdev->dev = &pdev->dev;

	pcdev->mclk_freq = 24000000;


    clk_set_rate(pcdev->mclk, pcdev->mclk_freq);

	/* Request the vic regions. */
	if (!request_mem_region(vic_res->start, resource_size(vic_res), VIC_ONLY_NAME)) {
		err = -EBUSY;
		goto err_request_vic_mem_region;
	}
	vic_base = ioremap(vic_res->start, resource_size(vic_res));
	if (!vic_base) {
		err = -ENOMEM;
		goto err_vic_ioremap;
	}

#ifdef CONFIG_MIPI_SENSOR
	/* Request the mipi regions. */
	if (!request_mem_region(mipi_res->start, resource_size(mipi_res), MIPI_ONLY_NAME)) {
		err = -EBUSY;
		goto err_request_mipi_mem_region;
	}
	mipi_base = ioremap(mipi_res->start, resource_size(mipi_res));
	if (!mipi_base) {
		err = -ENOMEM;
		goto err_mipi_ioremap;
	}
#endif

	spin_lock_init(&pcdev->lock);
	INIT_LIST_HEAD(&pcdev->video_buffer_list);

	pcdev->vic_res = vic_res;
	pcdev->vic_base = vic_base;
	pcdev->irq = irq;

#ifdef CONFIG_MIPI_SENSOR
	pcdev->mipi_res = mipi_res;
	pcdev->mipi_base = mipi_base;
#endif
	pcdev->active = NULL;
    pcdev->buf_cnt = CIM_FRAME_BUF_CNT;

	/* request irq */
	err = request_irq(pcdev->irq, jz_camera_irq_handler, IRQF_DISABLED,
			dev_name(&pdev->dev), pcdev);
	if(err) {
		dev_err(&pdev->dev, "request irq failed!\n");
		goto err_request_irq;
	}

	disable_irq(pcdev->irq);

	pcdev->alloc_ctx = vb2_dma_contig_init_ctx(&pdev->dev);
	if (IS_ERR(pcdev->alloc_ctx)) {
		ret = PTR_ERR(pcdev->alloc_ctx);
		goto err_alloc_ctx;
	}
	pcdev->soc_host.drv_name        = VIC_ONLY_NAME;
	pcdev->soc_host.ops             = &jz_soc_camera_host_ops;
	pcdev->soc_host.priv            = pcdev;
	pcdev->soc_host.v4l2_dev.dev    = &pdev->dev;
	pcdev->soc_host.nr              = 0; /* bus_id */

	err = soc_camera_host_register(&pcdev->soc_host);
	if (err)
		goto err_soc_camera_host_register;

	ret = sysfs_create_group(&pcdev->dev->kobj, &cim_debug_attr_group);
	if (ret) {
		dev_err(&pdev->dev, "device create sysfs group failed\n");

		ret = -EINVAL;
		goto err_free_file;
	}

	dev_dbg(&pdev->dev, "jz Camera driver loaded!\n");

	return 0;

err_free_file:
	sysfs_remove_group(&pcdev->dev->kobj, &cim_debug_attr_group);
err_soc_camera_host_register:
	vb2_dma_contig_cleanup_ctx(pcdev->alloc_ctx);
err_alloc_ctx:
	free_irq(pcdev->irq, pcdev);
err_request_irq:
#ifdef CONFIG_MIPI_SENSOR
        iounmap(mipi_base);
err_mipi_ioremap:
       release_mem_region(mipi_res->start, resource_size(mipi_res));
err_request_mipi_mem_region:
#endif
        iounmap(vic_base);
err_vic_ioremap:
	release_mem_region(vic_res->start, resource_size(vic_res));
err_request_vic_mem_region:
#ifdef CONFIG_MIPI_SENSOR
	clk_put(pcdev->csi_clk);
err_clk_get_csi_clk:
#endif
	clk_put(pcdev->cgu_isp_clk);
err_clk_get_cgu_isp_clk:
	clk_put(pcdev->isp_clk);
err_clk_get_isp_clk:
	clk_put(pcdev->mclk);
err_clk_get_mclk:
err_get_irq:
#ifdef CONFIG_MIPI_SENSOR
err_get_mipi_resource:
#endif
err_get_resource:
	kfree(pcdev);
err_kzalloc:
	return err;

}

static int __exit jz_camera_remove(struct platform_device *pdev)
{
	struct soc_camera_host *soc_host = to_soc_camera_host(&pdev->dev);
	struct jz_camera_dev *pcdev = container_of(soc_host,
					struct jz_camera_dev, soc_host);
	struct resource *vic_res;
#ifdef CONFIG_MIPI_SENSOR
	struct resource *mipi_res;
#endif
	free_irq(pcdev->irq, pcdev);

	vb2_dma_contig_cleanup_ctx(pcdev->alloc_ctx);

	clk_put(pcdev->mclk);
	clk_put(pcdev->isp_clk);
	clk_put(pcdev->cgu_isp_clk);
#ifdef CONFIG_MIPI_SENSOR
    clk_put(pcdev->csi_clk);
#endif
	soc_camera_host_unregister(soc_host);

	sysfs_remove_group(&pcdev->dev->kobj, &cim_debug_attr_group);

	iounmap(pcdev->vic_base);
	vic_res = pcdev->vic_res;
	release_mem_region(vic_res->start, resource_size(vic_res));
#ifdef CONFIG_MIPI_SENSOR
    iounmap(pcdev->mipi_base);
    mipi_res = pcdev->mipi_res;
    release_mem_region(mipi_res->start, resource_size(mipi_res));
#endif

	kfree(pcdev);

	dev_dbg(&pdev->dev, "jz Camera driver unloaded\n");

	return 0;
}

static struct platform_driver jz_camera_driver = {
	.remove		= __exit_p(jz_camera_remove),
	.driver		= {
		.name	= VIC_ONLY_NAME,
		.owner	= THIS_MODULE,
	},
};

static int __init jz_camera_init(void) {
	/*
	 * platform_driver_probe() can save memory,
	 * but this Driver can bind to one device only.
	 */
	return platform_driver_probe(&jz_camera_driver, jz_camera_probe);
}

static void __exit jz_camera_exit(void) {
	return platform_driver_unregister(&jz_camera_driver);
}

late_initcall(jz_camera_init);
module_exit(jz_camera_exit);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("sxyzhang <xiaoyan.zhang@ingenic.com>");
MODULE_DESCRIPTION("jz Soc Camera Host Driver");
MODULE_ALIAS("a jz-cim platform");
