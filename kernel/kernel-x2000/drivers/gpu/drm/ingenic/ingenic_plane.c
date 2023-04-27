#include <drm/drmP.h>
#include <drm/drm_plane_helper.h>
#include <drm/drm_atomic_helper.h>
#include "ingenic_drv.h"
#include "dpu_reg.h"

static void ingenic_srd_plane_atomic_update(struct drm_plane *plane,
				       struct drm_plane_state *old_state)
{
	struct drm_plane_state *state = plane->state;
	struct drm_crtc *crtc = state->crtc;
	struct drm_display_mode *mode = &crtc->mode;
	struct ingenic_drm_srd_plane *ingenic_plane = to_ingenic_srd_plane(plane);
	struct drm_framebuffer *fb = state->fb;
	struct drm_gem_cma_object *gem;
	uint32_t bpp;
	int x, y;

	if (!state->crtc)
		return;

	switch(fb->pixel_format) {
		case DRM_FORMAT_XRGB8888:
			ingenic_plane->format = LAYER_CFG_FORMAT_RGB888;
			ingenic_plane->color = LAYER_CFG_COLOR_RGB;
			break;
		case DRM_FORMAT_XBGR8888:
			ingenic_plane->format = LAYER_CFG_FORMAT_RGB888;
			ingenic_plane->color = LAYER_CFG_COLOR_BGR;
			break;
		case DRM_FORMAT_ARGB1555:
			ingenic_plane->format = LAYER_CFG_FORMAT_ARGB1555;
			ingenic_plane->color = LAYER_CFG_COLOR_RGB;
			break;
		case DRM_FORMAT_XRGB1555:
			ingenic_plane->format = LAYER_CFG_FORMAT_RGB555;
			ingenic_plane->color = LAYER_CFG_COLOR_RGB;
			break;
		case DRM_FORMAT_RGB565:
			ingenic_plane->format = LAYER_CFG_FORMAT_RGB565;
			ingenic_plane->color = LAYER_CFG_COLOR_RGB;
			break;
		default:
			printk("Err: plane not support formates!\n");
			break;
	}

#if 0
	if((state->crtc_x != 0) || (state->crtc_y != 0) ||
		(state->crtc_w != mode->hdisplay) ||
		(state->crtc_h != mode->vdisplay) ||
		(state->src_w >> 16 != mode->hdisplay) ||
		(state->src_h >> 16 != mode->vdisplay)) {
		printk("Err: plane set not support!\n");
	}
#endif

	gem = drm_fb_cma_get_gem_obj(fb, 0);
	bpp = fb->bits_per_pixel;
	x = state->src_x >> 16;
	y = state->src_y >> 16;
	ingenic_plane->addr_offset = gem->paddr + fb->offsets[0]
		       + y * fb->pitches[0] + x * (bpp >> 3);
	ingenic_plane->stride = fb->pitches[0] / (bpp >> 3);

	ingenic_plane->pending_fb = state->fb;
}

static void ingenic_srd_plane_atomic_disable(struct drm_plane *plane,
					struct drm_plane_state *old_state)
{
}

static const struct drm_plane_helper_funcs srd_plane_helper_funcs = {
	.atomic_update = ingenic_srd_plane_atomic_update,
	.atomic_disable = ingenic_srd_plane_atomic_disable,
};


static int ingenic_cmp_plane_get_size(int start, unsigned length, unsigned last)
{
	int end = start + length;
	int size = 0;

	if (start <= 0) {
		if (end > 0)
			size = min_t(unsigned, end, last);
	} else if (start <= last) {
		size = min_t(unsigned, last - start, length);
	}

	return size;
}

static void ingenic_cmp_plane_compute_base(struct drm_plane *plane,
					 struct drm_framebuffer *fb,
					 int x, int y)
{
	struct ingenic_drm_plane *ingenic_plane = to_ingenic_plane(plane);
	struct drm_gem_cma_object *gem;
	uint32_t bpp;

	/* one framebufer may have 4 plane */
	gem = drm_fb_cma_get_gem_obj(fb, 0);
	bpp = fb->bits_per_pixel;
	bpp = ingenic_plane->nv_en ? 8 : bpp;
	ingenic_plane->addr_offset[0] = gem->paddr + fb->offsets[0]
		       + y * fb->pitches[0] + x * (bpp >> 3);

	if (ingenic_plane->nv_en) {
		ingenic_plane->addr_offset[1] = gem->paddr + fb->offsets[1]
			       + y  * fb->pitches[1]
			       + x * (bpp >> 3);
	}
	ingenic_plane->stride = fb->pitches[0]/(bpp >> 3);
	ingenic_plane->uvstride = fb->pitches[1]/(bpp >> 3);
}

void ingenic_cmp_plane_set_formt(struct drm_plane *plane,  struct drm_framebuffer *fb)
{
	struct ingenic_drm_plane *ingenic_plane = to_ingenic_plane(plane);
	switch(fb->pixel_format) {
		case DRM_FORMAT_XRGB8888:
			ingenic_plane->format = LAYER_CFG_FORMAT_RGB888;
			ingenic_plane->color = LAYER_CFG_COLOR_RGB;
			break;
		case DRM_FORMAT_XBGR8888:
			ingenic_plane->format = LAYER_CFG_FORMAT_RGB888;
			ingenic_plane->color = LAYER_CFG_COLOR_BGR;
			break;
		case DRM_FORMAT_ARGB8888:
			ingenic_plane->format = LAYER_CFG_FORMAT_ARGB8888;
			ingenic_plane->color = LAYER_CFG_COLOR_RGB;
			break;
		case DRM_FORMAT_ABGR8888:
			ingenic_plane->format = LAYER_CFG_FORMAT_ARGB8888;
			ingenic_plane->color = LAYER_CFG_COLOR_BGR;
			break;
		case DRM_FORMAT_ARGB1555:
			ingenic_plane->format = LAYER_CFG_FORMAT_ARGB1555;
			ingenic_plane->color = LAYER_CFG_COLOR_RGB;
			break;
		case DRM_FORMAT_XRGB1555:
			ingenic_plane->format = LAYER_CFG_FORMAT_RGB555;
			ingenic_plane->color = LAYER_CFG_COLOR_RGB;
			break;
		case DRM_FORMAT_RGB565:
			ingenic_plane->format = LAYER_CFG_FORMAT_RGB565;
			ingenic_plane->color = LAYER_CFG_COLOR_RGB;
			break;
		case DRM_FORMAT_NV12:
			ingenic_plane->nv_en = 1;
			ingenic_plane->format = LAYER_CFG_FORMAT_NV12;
			ingenic_plane->color = 0;
			break;
		case DRM_FORMAT_NV21:
			ingenic_plane->nv_en = 1;
			ingenic_plane->format = LAYER_CFG_FORMAT_NV21;
			ingenic_plane->color = 0;
			break;
		case DRM_FORMAT_YUV422:
			ingenic_plane->format = LAYER_CFG_FORMAT_YUV422;
			ingenic_plane->color = 0;
			break;
		default:
			printk("Err: plane not support formates!\n");
			break;
	}
}

static void ingenic_cmp_plane_mode_set(struct drm_plane *plane,
				  struct drm_crtc *crtc,
				  struct drm_framebuffer *fb,
				  int crtc_x, int crtc_y,
				  uint32_t crtc_w, uint32_t crtc_h,
				  uint32_t src_x, uint32_t src_y,
				  uint32_t src_w, uint32_t src_h)
{
	struct ingenic_drm_plane *ingenic_plane = to_ingenic_plane(plane);
	struct drm_display_mode *mode = &crtc->mode;
	uint32_t actual_w;
	uint32_t actual_h;

	actual_w = ingenic_cmp_plane_get_size(crtc_x, crtc_w, mode->hdisplay);
	actual_h = ingenic_cmp_plane_get_size(crtc_y, crtc_h, mode->vdisplay);

	if (crtc_x < 0) {
		if (actual_w)
			src_x -= crtc_x;
		crtc_x = 0;
	}

	if (crtc_y < 0) {
		if (actual_h)
			src_y -= crtc_y;
		crtc_y = 0;
	}

	/* set drm framebuffer data. */
	ingenic_plane->src_x = src_x;
	ingenic_plane->src_y = src_y;
	ingenic_plane->src_w = src_w;
	ingenic_plane->src_h = src_h;

	/* set plane range to be displayed. */
	ingenic_plane->disp_pos_x = crtc_x;
	ingenic_plane->disp_pos_y = crtc_y;
	if((ingenic_plane->src_w != actual_w) || (ingenic_plane->src_h != actual_h)) {
		ingenic_plane->scale_en = 1;
		ingenic_plane->scale_w = actual_w;
		ingenic_plane->scale_h = actual_h;
	} else {
		ingenic_plane->scale_en = 0;
		ingenic_plane->scale_w = 0;
		ingenic_plane->scale_h = 0;
	}

	ingenic_plane->nv_en = 0;
	ingenic_cmp_plane_set_formt(plane, fb);

	ingenic_cmp_plane_compute_base(plane, fb,
			ingenic_plane->src_x,
			ingenic_plane->src_y);

	ingenic_plane->lay_en = 1;
	plane->crtc = crtc;
}

static void ingenic_cmp_plane_atomic_update(struct drm_plane *plane,
				       struct drm_plane_state *old_state)
{
	struct drm_plane_state *state = plane->state;
	struct ingenic_drm_plane *ingenic_plane = to_ingenic_plane(plane);

	if (!state->crtc)
		return;

	ingenic_cmp_plane_mode_set(plane, state->crtc, state->fb,
			      state->crtc_x, state->crtc_y,
			      state->crtc_w, state->crtc_h,
			      state->src_x >> 16, state->src_y >> 16,
			      state->src_w >> 16, state->src_h >> 16);

	ingenic_plane->pending_fb = state->fb;
}

static void ingenic_cmp_plane_atomic_disable(struct drm_plane *plane,
					struct drm_plane_state *old_state)
{
	struct ingenic_drm_plane *ingenic_plane = to_ingenic_plane(plane);

	if (!old_state->crtc)
		return;

	ingenic_plane->lay_en = 0;
}

static int ingenic_plane_atomic_set_property(struct drm_plane *plane,
					  struct drm_plane_state *state,
					  struct drm_property *property,
					  uint64_t val)
{
	struct ingenic_drm_private *priv = plane->dev->dev_private;
	struct ingenic_drm_plane *ingenic_plane = to_ingenic_plane(plane);

	if (property == priv->props.zorder) {
		ingenic_plane->zorder = val;
	} else if(property == priv->props.galpha) {
		ingenic_plane->g_alpha_val = val;
		if(ingenic_plane->g_alpha_val == 0xff)
			ingenic_plane->g_alpha_en = 0;
		else
			ingenic_plane->g_alpha_en = 1;
	} else
		return -EINVAL;

	return 0;
}

static int ingenic_plane_atomic_get_property(struct drm_plane *plane,
					  const struct drm_plane_state *state,
					  struct drm_property *property,
					  uint64_t *val)
{
	struct ingenic_drm_private *priv = plane->dev->dev_private;
	struct ingenic_drm_plane *ingenic_plane = to_ingenic_plane(plane);

	if (property == priv->props.zorder)
		*val = ingenic_plane->zorder;
	else if(property == priv->props.galpha)
		*val = ingenic_plane->g_alpha_val;
	else
		return -EINVAL;

	return 0;
}

static void ingenic_plane_install_properties(struct drm_plane *plane,
		struct drm_mode_object *obj)
{
	struct ingenic_drm_plane *ingenic_plane = to_ingenic_plane(plane);
	struct drm_device *dev = plane->dev;
	struct ingenic_drm_private *priv = dev->dev_private;

	drm_object_attach_property(obj, priv->props.zorder, ingenic_plane->index);
	drm_object_attach_property(obj, priv->props.galpha, 0xff);
	ingenic_plane->zorder = ingenic_plane->index;
	ingenic_plane->g_alpha_val = 0xff;
	ingenic_plane->g_alpha_en = 0;
}

static const struct drm_plane_helper_funcs cmp_plane_helper_funcs = {
	.atomic_update = ingenic_cmp_plane_atomic_update,
	.atomic_disable = ingenic_cmp_plane_atomic_disable,
};

static struct drm_plane_funcs ingenic_plane_funcs = {
	.update_plane	= drm_atomic_helper_update_plane,
	.disable_plane	= drm_atomic_helper_disable_plane,
	.destroy	= drm_plane_cleanup,
	.set_property = drm_atomic_helper_plane_set_property,
	.reset = drm_atomic_helper_plane_reset,
	.atomic_duplicate_state = drm_atomic_helper_plane_duplicate_state,
	.atomic_destroy_state = drm_atomic_helper_plane_destroy_state,
	.atomic_set_property = ingenic_plane_atomic_set_property,
	.atomic_get_property = ingenic_plane_atomic_get_property,
};

enum drm_plane_type ingenic_cmp_plane_get_type(uint32_t index)
{
		if (index == 0)
			return DRM_PLANE_TYPE_PRIMARY;
		else
			return DRM_PLANE_TYPE_OVERLAY;
}

int ingenic_cmp_plane_init(struct drm_device *dev,
		      struct ingenic_drm_plane *ingenic_plane,
		      unsigned long possible_crtcs, enum drm_plane_type type,
		      const uint32_t *formats, uint32_t fcount,
		      uint32_t index)
{
	struct drm_plane *plane = &ingenic_plane->base;
	int err;

	err = drm_universal_plane_init(dev, plane, possible_crtcs,
				       &ingenic_plane_funcs, formats, fcount,
				       type);
	if (err) {
		DRM_ERROR("failed to initialize plane\n");
		return err;
	}

	drm_plane_helper_add(plane, &cmp_plane_helper_funcs);

	ingenic_plane->index = index;

	ingenic_plane_install_properties(plane, &plane->base);

	return 0;
}

int ingenic_srd_plane_init(struct drm_device *dev,
		      struct ingenic_drm_srd_plane *ingenic_plane,
		      unsigned long possible_crtcs, enum drm_plane_type type,
		      const uint32_t *formats, uint32_t fcount,
		      uint32_t index)
{
	struct drm_plane *plane = &ingenic_plane->base;
	int err;

	err = drm_universal_plane_init(dev, plane, possible_crtcs,
				       &ingenic_plane_funcs, formats, fcount,
				       type);
	if (err) {
		DRM_ERROR("failed to initialize plane\n");
		return err;
	}

	drm_plane_helper_add(plane, &srd_plane_helper_funcs);

	return 0;
}
