/*
 * kernel/drivers/video/jz_vfb/vfb.c
 *
 * Copyright (c) 2015 Ingenic Semiconductor Co., Ltd.
 *              http://www.ingenic.com/
 *
 * Core file for Ingenic Display Controller driver
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */

#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/errno.h>
#include <linux/mutex.h>
#include <linux/poll.h>
#include <linux/wait.h>
#include <linux/fs.h>
#include <linux/irq.h>
#include <linux/mm.h>
#include <linux/fb.h>
#include <linux/delay.h>
#include <linux/ctype.h>
#include <linux/dma-mapping.h>
#include <linux/platform_device.h>
#include <linux/io.h>
#include <linux/memory.h>
#include <linux/suspend.h>
#include <linux/string.h>
#include <linux/kthread.h>
#include <asm/cacheflush.h>

#include "vfb.h"
#include "vsync.h"
#include "./ingenic_sfc_oled_v2/sfc.h"

extern struct sfc_flash *g_flash;

//struct fb_info *fb;
static  struct jzfb *jzfb;

void *video_buf = NULL;
size_t videomem_size = 0;
size_t xy_size = 0;

static u64 jz_fb_dmamask = ~(u64) 0;

struct platform_device jz_vfb_device = {
	.name = "vfb",
	.id = 0,
	.dev = {
		.dma_mask = &jz_fb_dmamask,
		.coherent_dma_mask = 0xffffffff,
	},
};
#if 1
//CONFIG_VFB_XRES=194
//CONFIG_VFB_YRES=368

struct fb_videomode vlcd_videomode = {
	.name = "vfb_mode",
	.refresh = 60,
	.xres = 368,//VFB_XRES,
	.yres = 194,//VFB_YRES,
	.pixclock = KHZ2PICOS(36000),
	.left_margin = 70,
	.right_margin = 70,
	.upper_margin = 2,
	.lower_margin = 2,
	.hsync_len = 42,
	.vsync_len = 11,
	.sync = ~FB_SYNC_HOR_HIGH_ACT & ~FB_SYNC_VERT_HIGH_ACT,
	.vmode = FB_VMODE_NONINTERLACED,
	.flag = 0,
};

struct jzfb_platform_data jzvfb_pdata = {
	.num_modes = 1,
	.modes = &vlcd_videomode,
	.bpp = VFB_BPP,
	.width = 45,
	.height = 75,

};
#endif
static const struct fb_fix_screeninfo jzfb_fix  = {
	.id = "jzvfb",
	.type = FB_TYPE_PACKED_PIXELS,
	.visual = FB_VISUAL_TRUECOLOR,
	.xpanstep = 0,
	.ypanstep = 1,
	.ywrapstep = 0,
	.accel = FB_ACCEL_NONE,
};

static void
jzfb_videomode_to_var(struct fb_var_screeninfo *var,
		      const struct fb_videomode *mode)
{
	var->xres = mode->xres;
	var->yres = mode->yres;
	var->xres_virtual = mode->xres;
	var->yres_virtual = mode->yres * NUM_FRAME_BUFFERS;
	var->xoffset = 0;
	var->yoffset = 0;
	var->left_margin = mode->left_margin;
	var->right_margin = mode->right_margin;
	var->upper_margin = mode->upper_margin;
	var->lower_margin = mode->lower_margin;
	var->hsync_len = mode->hsync_len;
	var->vsync_len = mode->vsync_len;
	var->sync = mode->sync;
	var->vmode = mode->vmode & FB_VMODE_MASK;
	var->pixclock = mode->pixclock;
}

static int jzfb_get_controller_bpp(struct jzfb *jzfb)
{
	switch (jzfb->pdata->bpp) {
	case 18:
	case 24:
		printk("***********case 24:return 32\n\n");
		return 32;
	case 15:
		return 16;
	default:
		return jzfb->pdata->bpp;
	}
}

static struct fb_videomode *jzfb_get_mode(struct fb_var_screeninfo *var,
					  struct fb_info *info)
{
	size_t i;
	struct jzfb *jzfb = info->par;
	struct fb_videomode *mode = jzfb->pdata->modes;

	for (i = 0; i < jzfb->pdata->num_modes; ++i, ++mode) {
		if (mode->xres == var->xres && mode->yres == var->yres
			&& mode->vmode == var->vmode
			&& mode->right_margin == var->right_margin) {
					return mode;
			}
	}

	return NULL;
}

static int jzfb_check_var(struct fb_var_screeninfo *var, struct fb_info *info)
{
	struct jzfb *jzfb = info->par;
	struct fb_videomode *mode;

	if (var->bits_per_pixel != jzfb_get_controller_bpp(jzfb) &&
	    var->bits_per_pixel != jzfb->pdata->bpp)
		return -EINVAL;
	mode = jzfb_get_mode(var, info);
	if (mode == NULL) {
		dev_err(info->dev, "%s get video mode failed\n", __func__);
		return -EINVAL;
	}

	jzfb_videomode_to_var(var, mode);
//	printk("#################file=%s,func=%s,line=%d,bpp=%d\n\n",__FILE__,__func__,__LINE__,jzfb->pdata->bpp);
	switch (jzfb->pdata->bpp) {
	case 16:
		var->red.offset = 11;
		var->red.length = 5;
		var->green.offset = 5;
		var->green.length = 6;
		var->blue.offset = 0;
		var->blue.length = 5;
		break;
	case 17 ... 32:
		if (jzfb->fmt_order == FORMAT_X8B8G8R8) {
			var->red.offset = 0;
			var->green.offset = 8;
			var->blue.offset = 16;
		} else {
			/* default: FORMAT_X8R8G8B8 */
			var->red.offset = 16;
			var->green.offset = 8;
			var->blue.offset = 0;
		}

//		var->transp.offset = 24;
//		var->transp.length = 8;
		var->transp.offset = 0;
		var->transp.length = 0;
		var->red.length = 8;
		var->green.length = 8;
		var->blue.length = 8;
		var->bits_per_pixel = 32;
		break;
	default:
		dev_err(jzfb->dev, "Not support for %d bpp\n",
			jzfb->pdata->bpp);
		break;
	}

	return 0;
}

static int jzfb_set_par(struct fb_info *info)
{
	struct fb_var_screeninfo *var = &info->var;
	struct fb_videomode *mode;

	mode = jzfb_get_mode(var, info);
	if (mode == NULL) {
		dev_err(info->dev, "%s get video mode failed\n", __func__);
		return -EINVAL;
	}
	info->mode = mode;
	return 0;
}

static int jzfb_alloc_devmem(struct jzfb *jzfb)
{
	unsigned int videosize = 0;
	struct fb_videomode *mode;
	void *page;

	mode = jzfb->pdata->modes;
	if (!mode) {
		dev_err(jzfb->dev, "Checkout video mode fail\n");
		return -EINVAL;
	}

//	videosize = ALIGN(mode->xres, PIXEL_ALIGN) * mode->yres;
	videosize = mode->xres * mode->yres;
// add :
	xy_size = videosize;

	videosize *= jzfb_get_controller_bpp(jzfb) >> 3;
	videosize *= NUM_FRAME_BUFFERS;

//	jzfb->vidmem_size = PAGE_ALIGN(videosize);
	jzfb->vidmem_size = videosize;

	/**
	 * Use the dma alloc coherent has waste some space,
	 * If you need to alloc buffer for dma, open it,
	 * else close it and use the Kmalloc.
	 * And in jzfb_free_devmem() function is also set.
	 */
	jzfb->vidmem = dma_alloc_coherent(jzfb->dev,
					  jzfb->vidmem_size,
					  &jzfb->vidmem_phys, GFP_KERNEL);
	if (!jzfb->vidmem)
		return -ENOMEM;
	for (page = jzfb->vidmem;
	     page < jzfb->vidmem + PAGE_ALIGN(jzfb->vidmem_size);
	     page += PAGE_SIZE) {
		SetPageReserved(virt_to_page(page));
	}
	//add for export:
	videomem_size = jzfb->vidmem_size;
	video_buf = jzfb->vidmem;

	printk("#######line=%d======>video_buf=%p,vidmem_size=%d\n",__LINE__,video_buf,videomem_size);

	dev_dbg(jzfb->dev, "Frame buffer size: %d bytes\n", jzfb->vidmem_size);

	return 0;
}

static void jzfb_free_devmem(struct jzfb *jzfb)
{
	dma_free_coherent(jzfb->dev, jzfb->vidmem_size,
			  jzfb->vidmem, jzfb->vidmem_phys);
}


static int jzfb_pan_display(struct fb_var_screeninfo *var, struct fb_info *info)
{
	int ret = 0;
	struct jzfb *jzfb = info->par;
	//printk(KERN_DEBUG "%s() var->xoffset=%d,info->var.xoffset=%d, var->yres_virtual=%d, var->yres=%d, var->yoffset=%d\n",
	//__func__, var->xoffset,info->var.xoffset, var->yres_virtual, var->yres, var->yoffset);

	if (var->xoffset - info->var.xoffset) {
		dev_err(info->dev, "No support for X panning for now\n");
		//return -EINVAL;
	}

	ret = vsync_pan_display(info, var);

	return 0;
}

static int jzfb_mmap(struct fb_info *info, struct vm_area_struct *vma)
{
	struct jzfb *jzfb = info->par;
	unsigned long start;
	unsigned long off;
	u32 len;

	off = vma->vm_pgoff << PAGE_SHIFT;

	/* frame buffer memory */
	start = jzfb->fb->fix.smem_start;
	len = PAGE_ALIGN((start & ~PAGE_MASK) + jzfb->fb->fix.smem_len);
	start &= PAGE_MASK;

	if ((vma->vm_end - vma->vm_start + off) > len)
		return -EINVAL;
	off += start;

	vma->vm_pgoff = off >> PAGE_SHIFT;
	vma->vm_flags |= VM_IO;

	/* default is _CACHE_CACHABLE_NONCOHERENT: ->vm_page_prot=613 */
	//printk("%s(), vma->vm_page_prot=%x\n", __func__, pgprot_val(vma->vm_page_prot));
	pgprot_val(vma->vm_page_prot) &= ~_CACHE_MASK;
	//pgprot_val(vma->vm_page_prot) |= _CACHE_CACHABLE_WA; /* Write-Acceleration, vm_page_prot=213 */
	pgprot_val(vma->vm_page_prot) |= _CACHE_CACHABLE_NONCOHERENT; /* CACHABLE, vm_page_prot=613 */
	//printk("%s(), vma->vm_page_prot=%x\n", __func__, pgprot_val(vma->vm_page_prot));

	if (io_remap_pfn_range(vma, vma->vm_start, off >> PAGE_SHIFT,
			       vma->vm_end - vma->vm_start, vma->vm_page_prot))
	{
		return -EAGAIN;
	}

	return 0;
}

static int jzfb_ioctl(struct fb_info *info, unsigned int cmd, unsigned long arg)
{
	void __user *argp = (void __user *)arg;
	struct jzfb *jzfb = info->par;
	int ret;
	switch (cmd) {
		case JZFB_SET_VSYNCINT:
			break;
		case FBIO_WAITFORVSYNC:
			ret = vsync_wait(info,arg);
			if (unlikely(ret))
				return -EFAULT;
			break;
		case JZFB_GET_LCDTYPE:
			break;

		default:
			dev_info(info->dev, "Unknown ioctl 0x%x\n", cmd);
			return -1;
	}

	return 0;
}

static int jzfb_open(struct fb_info *info, int user)
{
	struct jzfb *jzfb = info->par;

	dev_dbg(info->dev, "%s()\n", __func__);

	return 0;
}

static int jzfb_release(struct fb_info *info, int user)
{
	struct jzfb *jzfb = info->par;
	dev_dbg(info->dev, "%s()\n", __func__);

	vsync_set_auto_refresh(jzfb, 1);	/* enable auto refresh */
	return 0;
}

static struct fb_ops jzfb_ops = {
	.owner = THIS_MODULE,
	.fb_open = jzfb_open,
	.fb_release = jzfb_release,
	.fb_check_var = jzfb_check_var,
	.fb_set_par = jzfb_set_par,
	.fb_pan_display = jzfb_pan_display,
	.fb_fillrect = cfb_fillrect,
	.fb_copyarea = cfb_copyarea,
	.fb_imageblit = cfb_imageblit,
	.fb_ioctl = jzfb_ioctl,
	.fb_mmap = jzfb_mmap,
};

static int jzfb_probe(struct platform_device *pdev)
{
	int ret = 0;
	int delay;
	struct fb_info *fb;
	struct jzfb_platform_data *pdata = pdev->dev.platform_data;
	struct fb_videomode *video_mode;

//	if (!pdata) {
//		dev_err(&pdev->dev, "Missing platform data\n");
//		return -ENXIO;
//	}
	fb = framebuffer_alloc(sizeof(struct jzfb), &pdev->dev);
	if (!fb) {
		dev_err(&pdev->dev, "Failed to allocate framebuffer device\n");
		return -ENOMEM;
	}

	fb->fbops = &jzfb_ops;
	fb->flags = FBINFO_DEFAULT;

	jzfb = fb->par;
	jzfb->fb = fb;
	jzfb->dev = &pdev->dev;
	jzfb->pdata = pdata;

	platform_set_drvdata(pdev, jzfb);

	fb_videomode_to_modelist(pdata->modes, pdata->num_modes, &fb->modelist);
	video_mode = jzfb->pdata->modes;
	if (!video_mode)
		goto err_framebuffer_release;
	jzfb_videomode_to_var(&fb->var, video_mode);
	fb->var.width = pdata->width;
	fb->var.height = pdata->height;
	fb->var.bits_per_pixel = pdata->bpp;
	/* Android generic FrameBuffer format is A8B8G8R8(B3B2B1B0), so we set A8B8G8R8 as default */
//    jzfb->fmt_order = FORMAT_X8B8G8R8;
    jzfb->fmt_order = FORMAT_X8R8G8B8;

	jzfb_check_var(&fb->var, fb);

	ret = jzfb_alloc_devmem(jzfb);
	if (ret) {
		dev_err(&pdev->dev, "Failed to allocate video memory\n");
		goto err_framebuffer_release;
	}
	fb->fix = jzfb_fix;
//	fb->fix.line_length = fb->var.bits_per_pixel * ALIGN(fb->var.xres,
//							     PIXEL_ALIGN) >> 3;
	fb->fix.line_length = fb->var.bits_per_pixel * fb->var.xres  >> 3;
	fb->fix.smem_start = jzfb->vidmem_phys;
	fb->fix.smem_len = jzfb->vidmem_size;
	fb->screen_base = jzfb->vidmem;
	fb->pseudo_palette = (void *)(fb + 1);

	jzfb_te_irq_register(jzfb);

	delay = 1000 / video_mode->refresh;
	if(vsync_soft_set(jzfb)){
		dev_err(&pdev->dev, "Failed to run vsync thread");
		goto err_free_devmem;
	}

	ret = register_framebuffer(fb);
	if (ret) {
		dev_err(&pdev->dev, "Failed to register framebuffer: %d\n",
			ret);
		goto err_kthread_shop;
	}

	return 0;
err_kthread_shop:
	vsync_soft_stop();
err_free_devmem:
	jzfb_free_devmem(jzfb);
err_framebuffer_release:
	framebuffer_release(fb);
	return ret;
}

static int jzfb_remove(struct platform_device *pdev)
{
	struct jzfb *jzfb = platform_get_drvdata(pdev);

	vsync_soft_stop();
	jzfb_free_devmem(jzfb);
	platform_set_drvdata(pdev, NULL);
	framebuffer_release(jzfb->fb);
	return 0;
}

static const struct of_device_id ingenicvfb_of_match[] = {
	{ .compatible = "ingenic,x2000-vfb"},
	{},
};

static struct platform_driver jzfb_driver = {
	.probe = jzfb_probe,
	.remove = jzfb_remove,
	.driver = {
		   .name = "vfb",
//		   .of_match_table = ingenicvfb_of_match,
		   },
};

static int __init jzvfb_init(void)
{
	int ret = 0;
	ret = platform_driver_register(&jzfb_driver);
	if(ret)
		return ret;
	ret = platform_device_add_data(&jz_vfb_device,&jzvfb_pdata,sizeof(jzvfb_pdata));
	if(ret)
		goto driver_unregister;
	ret = platform_device_register(&jz_vfb_device);
	if(ret)
		goto driver_unregister;
	return 0;
driver_unregister:
			platform_driver_unregister(&jzfb_driver);
	return ret;
}

static void __exit jzvfb_cleanup(void)
{
	platform_device_unregister(&jz_vfb_device);
	platform_driver_unregister(&jzfb_driver);
}

late_initcall(jzvfb_init);

module_exit(jzvfb_cleanup);

MODULE_DESCRIPTION("JZ virtual framebuffer driver");
MODULE_LICENSE("GPL");
