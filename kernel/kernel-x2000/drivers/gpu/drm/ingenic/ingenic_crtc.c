#include <fence.h>
#include <drm/drm_flip_work.h>
#include <drm/drm_plane_helper.h>
#include <drm/drm_fourcc.h>
#include <drm/drm_atomic.h>
#include <drm/drm_atomic_helper.h>
#include "ingenic_drv.h"
#include "dpu_reg.h"

void enable_vblank(struct drm_crtc *crtc, bool enable)
{
	struct ingenic_crtc *ingenic_crtc = to_ingenic_crtc(crtc);
	ingenic_crtc->dpu_ops->enable_vblank(ingenic_crtc, enable);
}

irqreturn_t ingenic_crtc_irq(int irq, void *arg)
{
	struct drm_crtc *crtc = arg;
	struct ingenic_crtc *ingenic_crtc = to_ingenic_crtc(crtc);
	return ingenic_crtc->dpu_ops->irq_handler(ingenic_crtc);
}

static void ingenic_crtc_enable(struct drm_crtc *crtc)
{
	struct ingenic_crtc *ingenic_crtc = to_ingenic_crtc(crtc);
	ingenic_crtc->dpu_ops->enable(ingenic_crtc);
}

static void ingenic_crtc_disable(struct drm_crtc *crtc)
{
	struct ingenic_crtc *ingenic_crtc = to_ingenic_crtc(crtc);
	ingenic_crtc->dpu_ops->disable(ingenic_crtc);
}

static int ingenic_alloc_srd_desc(struct ingenic_crtc *crtc)
{
	struct ingenic_srd_priv *srd_priv = crtc->crtc_priv;
	struct drm_device *dev = crtc->base.dev;
	uint32_t buff_size;
	uint8_t *addr;
	dma_addr_t addr_phy;
	int i;

	buff_size = sizeof(struct ingenicfb_sreadesc);
	buff_size = ALIGN(buff_size, DESC_ALIGN);
	addr = dma_alloc_coherent(dev->dev, buff_size * 2,
				  &addr_phy, GFP_KERNEL);
	if(addr == NULL) {
		return -ENOMEM;
	}
	for(i = 0; i < 2; i++) {
		srd_priv->sreadesc[i] =
			(struct ingenicfb_sreadesc *)(addr + i * buff_size);
		srd_priv->sreadesc_phys[i] = addr_phy + i * buff_size;
	}
	return 0;
}

static int ingenic_alloc_cmp_desc(struct ingenic_crtc *crtc)
{
	struct ingenic_cmp_priv *cmp_priv = crtc->crtc_priv;
	struct drm_device *dev = crtc->base.dev;
	uint32_t buff_size;
	uint8_t *addr;
	dma_addr_t addr_phy;
	int i, j;

	buff_size = sizeof(struct ingenicfb_framedesc);
	buff_size = ALIGN(buff_size, DESC_ALIGN);
	addr = dma_alloc_coherent(dev->dev, buff_size * 2,
				  &addr_phy, GFP_KERNEL);
	if(addr == NULL) {
		return -ENOMEM;
	}
	for(i = 0; i < 2; i++) {
		cmp_priv->framedesc[i] =
			(struct ingenicfb_framedesc *)(addr + i * buff_size);
		cmp_priv->framedesc_phys[i] = addr_phy + i * buff_size;
	}

	buff_size = sizeof(struct ingenicfb_layerdesc);
	buff_size = ALIGN(buff_size, DESC_ALIGN);
	addr = dma_alloc_coherent(dev->dev, buff_size * 2 * DPU_SUPPORT_LAYER_NUM,
				  &addr_phy, GFP_KERNEL);
	if(addr == NULL) {
		return -ENOMEM;
	}
	for(j = 0; j < 2; j++) {
		for(i = 0; i < DPU_SUPPORT_LAYER_NUM; i++) {
			cmp_priv->layerdesc[j][i] = (struct ingenicfb_layerdesc *)
				(addr + i * buff_size + j * buff_size * DPU_SUPPORT_LAYER_NUM);
			cmp_priv->layerdesc_phys[j][i] =
				addr_phy + i * buff_size + j * buff_size * DPU_SUPPORT_LAYER_NUM;
		}
	}

	return 0;
}

static void ingenic_crtc_destroy(struct drm_crtc *crtc)
{
	struct ingenic_crtc *ingenic_crtc = to_ingenic_crtc(crtc);
	struct drm_device *dev = crtc->dev;
	struct drm_pending_vblank_event *event;
	bool vblank_put = false;
	unsigned long flags;

	spin_lock_irqsave(&dev->event_lock, flags);

	event = ingenic_crtc->event;
	ingenic_crtc->event = NULL;

	spin_unlock_irqrestore(&dev->event_lock, flags);

	if (event)
		event->base.destroy(&event->base);

	if (vblank_put)
		drm_vblank_put(dev, 0);

	drm_crtc_cleanup(crtc);

	kfree(ingenic_crtc);
}

static bool ingenic_crtc_mode_fixup(struct drm_crtc *crtc,
		const struct drm_display_mode *mode,
		struct drm_display_mode *adjusted_mode)
{
	return true;
}

static void ingenic_crtc_mode_set_nofb(struct drm_crtc *crtc)
{
//	struct ingenic_crtc *ingenic_crtc = to_ingenic_crtc(crtc);

}

static void ingenic_crtc_atomic_begin(struct drm_crtc *crtc,
                struct drm_crtc_state *old_crtc_state)
{
	struct ingenic_crtc *ingenic_crtc = to_ingenic_crtc(crtc);

	ingenic_crtc->event = crtc->state->event;

}
static void ingenic_crtc_atomic_flush(struct drm_crtc *crtc,
                struct drm_crtc_state *old_crtc_state)
{
	struct ingenic_crtc *ingenic_crtc = to_ingenic_crtc(crtc);

	if (ingenic_crtc->dpu_ops->commit)
		ingenic_crtc->dpu_ops->commit(ingenic_crtc);
}

static const struct drm_crtc_funcs ingenic_crtc_funcs = {
	.destroy = ingenic_crtc_destroy,
	.reset = drm_atomic_helper_crtc_reset,
	.set_config = drm_atomic_helper_set_config,
	.page_flip = drm_atomic_helper_page_flip,
	.reset = drm_atomic_helper_crtc_reset,
	.atomic_duplicate_state = drm_atomic_helper_crtc_duplicate_state,
	.atomic_destroy_state = drm_atomic_helper_crtc_destroy_state,
};

static const struct drm_crtc_helper_funcs ingenic_crtc_helper_funcs = {
	.mode_fixup = ingenic_crtc_mode_fixup,
	.disable = ingenic_crtc_disable,
	.enable = ingenic_crtc_enable,
	.mode_set_nofb = ingenic_crtc_mode_set_nofb,
	.atomic_begin = ingenic_crtc_atomic_begin,
	.atomic_flush = ingenic_crtc_atomic_flush,
};

struct drm_crtc * ingenic_cmp_crtc_create(struct drm_device *dev)
{
	struct ingenic_drm_private *priv = dev->dev_private;
	struct ingenic_crtc *cmp_crtc;
	struct drm_crtc *crtc;
	struct ingenic_cmp_priv *cmp_priv;
	struct ingenic_drm_plane *primary_plane;
	enum drm_plane_type type;
	uint32_t index;
	int ret;

	cmp_crtc = kzalloc(sizeof(*cmp_crtc)+sizeof(*cmp_priv), GFP_KERNEL);
	if (!cmp_crtc) {
		dev_err(dev->dev, "allocation failed\n");
		return NULL;
	}

	cmp_priv = (struct ingenic_cmp_priv *)(cmp_crtc + 1);

	cmp_crtc->pipe = priv->pipe++;
	cmp_register_crtc(cmp_crtc);

	for (index = 0; index < DPU_SUPPORT_LAYER_NUM; index++) {
		const uint32_t *formats;
		int arry_size;
		ingenic_cmp_plane_get_formats(index, &formats, &arry_size);
		type = ingenic_cmp_plane_get_type(index);
		ret = ingenic_cmp_plane_init(dev, &cmp_priv->plane[index],
					(1 << cmp_crtc->pipe), type, formats,
					arry_size, index);
		if (ret)
			return NULL;
	}

	crtc = &cmp_crtc->base;
	primary_plane = &cmp_priv->plane[0];
	ret = drm_crtc_init_with_planes(dev, crtc, &primary_plane->base, NULL,
					&ingenic_crtc_funcs);
	if (ret < 0)
		goto fail;

	drm_crtc_helper_add(crtc, &ingenic_crtc_helper_funcs);

	cmp_crtc->crtc_priv = cmp_priv;
	cmp_crtc->frame_current = 0;
	cmp_crtc->frame_next = 0;
	priv->crtc[cmp_crtc->pipe] = crtc;

	ingenic_alloc_cmp_desc(cmp_crtc);

	return crtc;

fail:
	ingenic_crtc_destroy(crtc);
	return NULL;
}

struct drm_crtc * ingenic_srd_crtc_create(struct drm_device *dev)
{
	struct ingenic_drm_private *priv = dev->dev_private;
	struct ingenic_crtc *srd_crtc;
	struct drm_crtc *crtc;
	struct ingenic_srd_priv *srd_priv;
	struct ingenic_drm_srd_plane *primary_plane;
	enum drm_plane_type type;
	const uint32_t *formats;
	int arry_size;
	int ret;

	srd_crtc = kzalloc(sizeof(*srd_crtc)+sizeof(*srd_priv), GFP_KERNEL);
	if (!srd_crtc) {
		dev_err(dev->dev, "allocation failed\n");
		return NULL;
	}

	srd_priv = (struct ingenic_srd_priv *)(srd_crtc + 1);

	srd_register_crtc(srd_crtc);

	srd_crtc->pipe = priv->pipe++;
	type = DRM_PLANE_TYPE_PRIMARY;
	ingenic_srd_plane_get_formats(&formats, &arry_size);

	ret = ingenic_srd_plane_init(dev, &srd_priv->plane,
				(1 << srd_crtc->pipe), type, formats,
				arry_size, 0);
	if (ret)
		return NULL;

	crtc = &srd_crtc->base;
	primary_plane = &srd_priv->plane;
	ret = drm_crtc_init_with_planes(dev, crtc, &primary_plane->base, NULL,
					&ingenic_crtc_funcs);
	if (ret < 0)
		goto fail;

	drm_crtc_helper_add(crtc, &ingenic_crtc_helper_funcs);

	srd_crtc->crtc_priv = srd_priv;
	srd_crtc->frame_current = 0;
	srd_crtc->frame_next = 0;
	priv->crtc[srd_crtc->pipe] = crtc;

	ingenic_alloc_srd_desc(srd_crtc);

	return crtc;

fail:
	ingenic_crtc_destroy(crtc);
	return NULL;
}
