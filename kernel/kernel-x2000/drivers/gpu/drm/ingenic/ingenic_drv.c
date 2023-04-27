#include <drm/drm_atomic.h>
#include <drm/drm_atomic_helper.h>
#include "ingenic_drv.h"
#include "dpu_reg.h"
#include "ingenic_disport.h"
#include "drm_fb_helper.h"
#include "hashtable.h"
#include "reservation.h"

static ssize_t dump_cmp(struct device *dev,
		struct device_attribute *attr, char *buf)
{
	struct drm_device *drm_dev = dev_get_drvdata(dev);
	struct ingenic_drm_private *private = drm_dev->dev_private;
	struct ingenic_crtc *crtc;
	int i;

	for(i = 0; i < DPU_MAX_CRTC_NUM; i++) {
		if(private->crtc[i]) {
			crtc = to_ingenic_crtc(private->crtc[i]);
			if(crtc->data_channel == DPU_CMP_OUTPUT_CHANNEL)
				dpu_dump_cmp_all(crtc);
		}
	}
	return 0;
}

static ssize_t dump_srd(struct device *dev,
		struct device_attribute *attr, char *buf)
{
	struct drm_device *drm_dev = dev_get_drvdata(dev);
	struct ingenic_drm_private *private = drm_dev->dev_private;
	struct ingenic_crtc *crtc;
	int i;

	for(i = 0; i < DPU_MAX_CRTC_NUM; i++) {
		if(private->crtc[i]) {
			crtc = to_ingenic_crtc(private->crtc[i]);
			if(crtc->data_channel == DPU_SIMPLE_READ_CHANNEL)
				dpu_dump_srd_all(crtc);
		}
	}
	return 0;
}
static DEVICE_ATTR(dump_cmp, S_IRUGO|S_IWUSR, dump_cmp, NULL);
static DEVICE_ATTR(dump_srd, S_IRUGO|S_IWUSR, dump_srd, NULL);

static struct attribute *drm_debug_attrs[] = {
	&dev_attr_dump_cmp.attr,
	&dev_attr_dump_srd.attr,
	NULL,
};

const char drm_group_name[] = "debug";
static struct attribute_group drm_debug_attr_group = {
	.name	= drm_group_name,
	.attrs	= drm_debug_attrs,
};
struct ingenic_resobj {
	struct hlist_node hlist_node;
	struct drm_gem_object *obj;
	struct reservation_object resobj;
};

static struct of_device_id ingenic_of_match[];

static DEFINE_HASHTABLE(ingenic_resobj_hlist, 5);

static void ingenic_atomic_schedule(struct ingenic_drm_private *private,
				struct drm_atomic_state *state)
{
	private->commit.state = state;
	schedule_work(&private->commit.work);
}

static void ingenic_atomic_complete(struct ingenic_drm_private *private,
				struct drm_atomic_state *state)
{
	struct drm_device *drm = private->drm_dev;

	/*
	 * Everything below can be run asynchronously without the need to grab
	 * any modeset locks at all under one condition: It must be guaranteed
	 * that the asynchronous work has either been cancelled (if the driver
	 * supports it, which at least requires that the framebuffers get
	 * cleaned up with drm_atomic_helper_cleanup_planes()) or completed
	 * before the new state gets committed on the software side with
	 * drm_atomic_helper_swap_state().
	 *
	 * This scheme allows new atomic state updates to be prepared and
	 * checked in parallel to the asynchronous completion of the previous
	 * update. Which is important since compositors need to figure out the
	 * composition of the next frame right after having submitted the
	 * current layout.
	 */

	drm_atomic_helper_commit_modeset_disables(drm, state);
	drm_atomic_helper_commit_modeset_enables(drm, state);
	drm_atomic_helper_commit_planes(drm, state, false);

	drm_atomic_helper_wait_for_vblanks(drm, state);

	drm_atomic_helper_cleanup_planes(drm, state);
	drm_atomic_state_free(state);
}

static void ingenic_atomic_work(struct work_struct *work)
{
	struct ingenic_drm_private *private = container_of(work,
			struct ingenic_drm_private, commit.work);

	ingenic_atomic_complete(private, private->commit.state);
}

static int ingenic_atomic_commit(struct drm_device *drm,
			     struct drm_atomic_state *state, bool async)
{
	struct ingenic_drm_private *private = drm->dev_private;
	int err;

	err = drm_atomic_helper_prepare_planes(drm, state);
	if (err)
		return err;

	/* serialize outstanding asynchronous commits */
	mutex_lock(&private->commit.lock);
	flush_work(&private->commit.work);

	/*
	 * This is the point of no return - everything below never fails except
	 * when the hw goes bonghits. Which means we can commit the new state on
	 * the software side now.
	 */

	drm_atomic_helper_swap_state(drm, state);

	if (async)
		ingenic_atomic_schedule(private, state);
	else
		ingenic_atomic_complete(private, state);

	mutex_unlock(&private->commit.lock);
	return 0;
}


static struct drm_framebuffer *ingenic_fb_create(struct drm_device *dev,
		struct drm_file *file_priv, struct drm_mode_fb_cmd2 *mode_cmd)
{
	return drm_fb_cma_create(dev, file_priv, mode_cmd);
}

static void ingenic_fb_output_poll_changed(struct drm_device *dev)
{
	struct ingenic_drm_private *priv = dev->dev_private;

	if (priv->fbdev)
		drm_fbdev_cma_hotplug_event(priv->fbdev);
}

static const struct drm_mode_config_funcs mode_config_funcs = {
	.fb_create = ingenic_fb_create,
	.output_poll_changed = ingenic_fb_output_poll_changed,
	.atomic_check = drm_atomic_helper_check,
	.atomic_commit = ingenic_atomic_commit,
};

static int ingenic_modeset_init_properties(struct drm_device *dev)
{
	struct ingenic_drm_private *priv = dev->dev_private;

	priv->props.zorder = drm_property_create_range(dev, 0, "zorder", 0, DPU_SUPPORT_LAYER_NUM-1);
	if (!priv->props.zorder)
		return -ENOMEM;
	priv->props.galpha = drm_property_create_range(dev, 0, "galpha", 0, 0xff);
	if (!priv->props.galpha)
		return -ENOMEM;

	return 0;
}

void modeset_init(struct drm_device *dev)
{
	struct ingenic_drm_private *priv = dev->dev_private;
	dev->mode_config.min_width = DPU_DEFAULT_MIN_WIDTH;
	dev->mode_config.min_height = DPU_DEFAULT_MIN_HEIGHT;
	dev->mode_config.max_width = priv->max_width;
	dev->mode_config.max_height = priv->max_height;
	dev->mode_config.async_page_flip = true;
	dev->mode_config.funcs = &mode_config_funcs;
}

static int ingenic_unload(struct drm_device *dev)
{
	struct ingenic_drm_private *priv = dev->dev_private;

	drm_kms_helper_poll_fini(dev);
	drm_mode_config_cleanup(dev);
	drm_vblank_cleanup(dev);

	if (priv->clk)
		clk_put(priv->clk);

	if (priv->disp_clk)
		clk_put(priv->disp_clk);

	if (priv->mmio)
		iounmap(priv->mmio);

	flush_workqueue(priv->wq);
	destroy_workqueue(priv->wq);

	dev->dev_private = NULL;

	kfree(priv);

	return 0;
}

static int ingenic_load(struct drm_device *dev, unsigned long flags)
{
	struct platform_device *pdev = dev->platformdev;
	struct ingenic_drm_private *priv;
	struct drm_crtc *crtc;
	struct resource *res;
	int ret;

	priv = kzalloc(sizeof(*priv), GFP_KERNEL);
	if (!priv) {
		DRM_DEBUG_DRIVER("failed to allocate private data\n");
		return -ENOMEM;
	}

	dev->dev_private = priv;
	priv->drm_dev = dev;

	priv->wq = alloc_ordered_workqueue("ingenic", 0);
	mutex_init(&priv->commit.lock);
	INIT_WORK(&priv->commit.work, ingenic_atomic_work);

	res = platform_get_resource(pdev, IORESOURCE_MEM, 0);
	if (!res) {
		DRM_ERROR("failed to get memory resource\n");
		ret = -EINVAL;
		goto fail;
	}

	priv->mmio = ioremap_nocache(res->start, resource_size(res));
	if (!priv->mmio) {
		DRM_ERROR("failed to ioremap\n");
		ret = -ENOMEM;
		goto fail;
	}

	priv->clk = clk_get(dev->dev, "gate_lcd");
	if (IS_ERR(priv->clk)) {
		DRM_ERROR("failed to get lcd clock\n");
		ret = -ENODEV;
		goto fail;
	}

	clk_prepare_enable(priv->clk);

	priv->disp_clk = clk_get(dev->dev, "div_lcd");
	if (IS_ERR(priv->disp_clk)) {
		DRM_ERROR("failed to get pixel clock\n");
		ret = -ENODEV;
		goto fail;
	}

	priv->irq = platform_get_irq(pdev, 0);

	clk_prepare_enable(priv->disp_clk);

	priv->max_bandwidth = DPU_DEFAULT_MAX_BANDWIDTH;

	priv->max_width = DPU_DEFAULT_MAX_WIDTH;

	priv->max_height = DPU_DEFAULT_MAX_HEIGHT;

	priv->max_pixelclock = DPU_DEFAULT_MAX_PIXELCLOCK;

	drm_mode_config_init(dev);

	ret = ingenic_modeset_init_properties(dev);
	if (ret < 0)
		goto fail;

	priv->encoders = ingenic_disport_probe(dev->dev);
	if (IS_ERR(priv->encoders)) {
		DRM_ERROR("disport probe failed\n");
		ret = PTR_ERR(priv->encoders);
		goto fail;
	}

	modeset_init(dev);

#ifdef CONFIG_DRM_INGENIC_SRD_CRTC
	crtc = ingenic_srd_crtc_create(dev);
	ret = request_irq(priv->irq, ingenic_crtc_irq,
			  IRQF_SHARED, "srd_crtc", crtc);
#endif
#ifdef CONFIG_DRM_INGENIC_CMP_CRTC
	crtc = ingenic_cmp_crtc_create(dev);
	ret = request_irq(priv->irq, ingenic_crtc_irq,
			  IRQF_SHARED, "cmp_crtc", crtc);
#endif
	if(!priv->pipe) {
		DRM_ERROR("no crtc controller\n");
		goto fail;
	}
	dev->irq_enabled = true;

	ingenic_disport_bind(dev, priv->encoders);

	ret = drm_vblank_init(dev, priv->pipe);
	if (ret < 0) {
		DRM_ERROR("failed to initialize vblank\n");
		goto fail;
	}

	platform_set_drvdata(pdev, dev);

	priv->fbdev = drm_fbdev_cma_init(dev, 32,
			dev->mode_config.num_crtc,
			dev->mode_config.num_connector);

	drm_kms_helper_poll_init(dev);

	drm_mode_config_reset(dev);

	sysfs_create_group(&pdev->dev.kobj, &drm_debug_attr_group);

//	drm_fbdev_cma_restore_mode(priv->fbdev);
	return 0;

fail:
	ingenic_unload(dev);
	return ret;
}

static void ingenic_preclose(struct drm_device *dev, struct drm_file *file)
{
//	struct ingenic_drm_private *priv = dev->dev_private;

//	ingenic_crtc_cancel_page_flip(priv->crtc, file);
}

static void ingenic_lastclose(struct drm_device *dev)
{
	struct ingenic_drm_private *priv = dev->dev_private;

	drm_fbdev_cma_restore_mode(priv->fbdev);
}

static int ingenic_enable_vblank(struct drm_device *dev, uint32_t pipe)
{
	struct ingenic_drm_private *priv = dev->dev_private;
	enable_vblank(priv->crtc[pipe], true);
	return 0;
}

static void ingenic_disable_vblank(struct drm_device *dev, uint32_t pipe)
{
	struct ingenic_drm_private *priv = dev->dev_private;
	enable_vblank(priv->crtc[pipe], false);
}

static struct ingenic_resobj *ingenic_resobj_lookup(struct drm_gem_object *obj)
{
	struct ingenic_resobj *ingenic_resobj_cursor;
	struct ingenic_resobj *ingenic_resobj = NULL;

	hash_for_each_possible(ingenic_resobj_hlist, ingenic_resobj_cursor,
					hlist_node, (unsigned long)obj) {
		if (ingenic_resobj_cursor->obj == obj) {
			ingenic_resobj = ingenic_resobj_cursor;
			break;
		}
	}

	return ingenic_resobj;
}

struct reservation_object *ingenic_drv_lookup_resobj(struct drm_gem_object *obj)
{
	struct ingenic_resobj *ingenic_resobj = ingenic_resobj_lookup(obj);

	if (!ingenic_resobj) {
		ingenic_resobj = kzalloc(sizeof(*ingenic_resobj), GFP_KERNEL);
		if (!ingenic_resobj)
			return NULL;

		ingenic_resobj->obj = obj;
		INIT_HLIST_NODE(&ingenic_resobj->hlist_node);
		reservation_object_init(&ingenic_resobj->resobj);

		hash_add(ingenic_resobj_hlist, &ingenic_resobj->hlist_node,
							(unsigned long)obj);
	}

	return &ingenic_resobj->resobj;
}

static void ingenic_gem_cma_free_object(struct drm_gem_object *obj)
{
	struct ingenic_resobj *ingenic_resobj = ingenic_resobj_lookup(obj);

	if (ingenic_resobj) {
		hash_del(&ingenic_resobj->hlist_node);

		reservation_object_wait_timeout_rcu(&ingenic_resobj->resobj,
					true, false, MAX_SCHEDULE_TIMEOUT);

		reservation_object_fini(&ingenic_resobj->resobj);

		kfree(ingenic_resobj);
	}

	drm_gem_cma_free_object(obj);
}

static struct dma_buf *ingenic_gem_prime_export(struct drm_device *dev,
					struct drm_gem_object *obj, int flags)
{
	/* Read/write access required */
	flags |= O_RDWR;
	return drm_gem_prime_export(dev, obj, flags);
}

static const struct file_operations fops = {
	.open               = drm_open,
	.release            = drm_release,
	.unlocked_ioctl     = drm_ioctl,
#ifdef CONFIG_COMPAT
	.compat_ioctl       = drm_compat_ioctl,
#endif
	.poll               = drm_poll,
	.read               = drm_read,
	.llseek             = no_llseek,
	.mmap               = drm_gem_cma_mmap,
};

static struct drm_driver ingenic_driver = {
	.driver_features    = DRIVER_HAVE_IRQ | DRIVER_GEM | DRIVER_MODESET |
				DRIVER_PRIME | DRIVER_ATOMIC,
	.load               = ingenic_load,
	.unload             = ingenic_unload,
	.preclose           = ingenic_preclose,
	.lastclose          = ingenic_lastclose,
	.set_busid          = drm_platform_set_busid,
	.get_vblank_counter = drm_vblank_count,
	.enable_vblank      = ingenic_enable_vblank,
	.disable_vblank     = ingenic_disable_vblank,
	.gem_free_object    = ingenic_gem_cma_free_object,
	.gem_vm_ops         = &drm_gem_cma_vm_ops,
	.dumb_create        = drm_gem_cma_dumb_create,
	.dumb_map_offset    = drm_gem_cma_dumb_map_offset,
	.dumb_destroy       = drm_gem_dumb_destroy,
	.prime_handle_to_fd = drm_gem_prime_handle_to_fd,
	.prime_fd_to_handle = drm_gem_prime_fd_to_handle,
	.gem_prime_import   = drm_gem_prime_import,
	.gem_prime_export   = ingenic_gem_prime_export,
	.gem_prime_get_sg_table = drm_gem_cma_prime_get_sg_table,
	.gem_prime_import_sg_table = drm_gem_cma_prime_import_sg_table,
	.gem_prime_vmap     = drm_gem_cma_prime_vmap,
	.gem_prime_vunmap   = drm_gem_cma_prime_vunmap,
	.gem_prime_mmap     = drm_gem_cma_prime_mmap,
	.gem_prime_res_obj  = ingenic_drv_lookup_resobj,
	.fops               = &fops,
	.name               = "ingenic",
	.desc               = "Ingenic DPU DRM",
	.date               = "20191212",
	.major              = 1,
	.minor              = 0,
};

static int ingenic_pdev_probe(struct platform_device *pdev)
{
	if (!pdev->dev.of_node) {
		DRM_DEBUG_DRIVER("device-tree data is missing\n");
		return -ENXIO;
	}

	return drm_platform_init(&ingenic_driver, pdev);
}

static int ingenic_pdev_remove(struct platform_device *pdev)
{
	drm_put_dev(platform_get_drvdata(pdev));
	return 0;
}

static struct of_device_id ingenic_of_match[] = {
		{ .compatible = "ingenic,x2000-dpu", },
		{ },
};
MODULE_DEVICE_TABLE(of, ingenic_of_match);

static struct platform_driver ingenic_platform_driver = {
	.probe      = ingenic_pdev_probe,
	.remove     = ingenic_pdev_remove,
	.driver     = {
		.name   = "x2000-dpu",
		.of_match_table = ingenic_of_match,
	},
};

static int __init ingenic_drm_init(void)
{
	return platform_driver_register(&ingenic_platform_driver);
}

static void __exit ingenic_drm_fini(void)
{
	platform_driver_unregister(&ingenic_platform_driver);
}
late_initcall(ingenic_drm_init);
module_init(ingenic_drm_init);
module_exit(ingenic_drm_fini);

MODULE_AUTHOR("chunleiwang <chunlei.wang@ingenic.com>");
MODULE_DESCRIPTION("Ingenic X2000 dpu driver");
MODULE_LICENSE("GPL");
