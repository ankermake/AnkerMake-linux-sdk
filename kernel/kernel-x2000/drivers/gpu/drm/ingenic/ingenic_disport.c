#include <drm/drmP.h>
#include <drm/drm_crtc_helper.h>
#include <drm/drm_panel.h>
#include <drm/drm_atomic_helper.h>
#include <video/of_videomode.h>
#include <video/videomode.h>
#include "ingenic_drv.h"
#include "ingenic_disport.h"
#include "jz_dsim.h"
#include "dpu_reg.h"

#define connector_to_disport(c) container_of(c, struct ingenic_disport, connector)

static inline struct ingenic_disport *encoder_to_disport(struct drm_encoder *e)
{
	return container_of(e, struct ingenic_disport, encoder);
}

static enum drm_connector_status
ingenic_disport_detect(struct drm_connector *connector, bool force)
{
	struct ingenic_disport *ctx = connector_to_disport(connector);

	if (ctx->panel && !ctx->panel->connector)
		drm_panel_attach(ctx->panel, &ctx->connector);

	return connector_status_connected;
}

static void ingenic_disport_connector_destroy(struct drm_connector *connector)
{
	drm_connector_unregister(connector);
	drm_connector_cleanup(connector);
}

static struct drm_connector_funcs ingenic_disport_connector_funcs = {
	.dpms = drm_atomic_helper_connector_dpms,
	.detect = ingenic_disport_detect,
	.fill_modes = drm_helper_probe_single_connector_modes,
	.destroy = ingenic_disport_connector_destroy,
	.reset = drm_atomic_helper_connector_reset,
	.atomic_duplicate_state = drm_atomic_helper_connector_duplicate_state,
	.atomic_destroy_state = drm_atomic_helper_connector_destroy_state,
};

static int ingenic_disport_get_modes(struct drm_connector *connector)
{
	struct ingenic_disport *ctx = connector_to_disport(connector);

	if (ctx->panel)
		return ctx->panel->funcs->get_modes(ctx->panel);

	return 0;
}

static struct drm_encoder *
ingenic_disport_best_encoder(struct drm_connector *connector)
{
	struct ingenic_disport *ctx = connector_to_disport(connector);

	return &ctx->encoder;
}

static struct drm_connector_helper_funcs ingenic_disport_connector_helper_funcs = {
	.get_modes = ingenic_disport_get_modes,
	.best_encoder = ingenic_disport_best_encoder,
};

static int ingenic_disport_create_connector(struct drm_encoder *encoder)
{
	struct ingenic_disport *ctx = encoder_to_disport(encoder);
	struct drm_connector *connector = &ctx->connector;
	int ret;

	connector->polled = DRM_CONNECTOR_POLL_HPD;
	connector->dpms = DRM_MODE_DPMS_OFF;

	ret = drm_connector_init(encoder->dev, connector,
				 &ingenic_disport_connector_funcs,
				 DRM_MODE_CONNECTOR_DisplayPort);
	if (ret) {
		DRM_ERROR("failed to initialize connector with drm\n");
		return ret;
	}

	drm_connector_helper_add(connector, &ingenic_disport_connector_helper_funcs);
	drm_connector_register(connector);
	drm_mode_connector_attach_encoder(connector, encoder);
	drm_panel_attach(ctx->panel, connector);

	return 0;
}

static bool ingenic_disport_mode_fixup(struct drm_encoder *encoder,
				  const struct drm_display_mode *mode,
				  struct drm_display_mode *adjusted_mode)
{
	struct drm_device *dev = encoder->dev;
	struct ingenic_disport *ctx = encoder_to_disport(encoder);
	struct drm_connector *connector = &ctx->connector;
	const struct drm_display_mode *panel_mode;

	if (list_empty(&connector->modes)) {
		dev_dbg(dev->dev, "mode_fixup: empty modes list\n");
		return false;
	}

	/* The flat panel mode is fixed, just copy it to the adjusted mode. */
	panel_mode = list_first_entry(&connector->modes,
				      struct drm_display_mode, head);
	drm_mode_copy(adjusted_mode, panel_mode);

	refresh_pixclock_auto_adapt(ctx, adjusted_mode);

	return true;
}

static void ingenic_disport_enable(struct drm_encoder *encoder)
{
	struct ingenic_disport *ctx = encoder_to_disport(encoder);
	struct drm_crtc_state *crtc_state = encoder->crtc->state;
	struct drm_display_mode *adjusted_mode =
			&crtc_state->adjusted_mode;

	if(ctx->dpms != DRM_MODE_DPMS_OFF) {
		return;
	}

	if(ctx->mipi_if && !ctx->mipi_init) {
		struct lcd_panel *lcd_panel =
			(struct lcd_panel *)adjusted_mode->private;
		struct jzdsi_data *dsi_data = lcd_panel->dsi_pdata;

		dsi_data->mode = adjusted_mode;
		ctx->dsi_dev->master_ops->mode_init(ctx->dsi_dev, dsi_data);
		ctx->mipi_init = true;
	}

	if(ctx->mipi_if) {
		ctx->dsi_dev->master_ops->set_blank_mode(ctx->dsi_dev, 0);
	}

	drm_panel_prepare(ctx->panel);

	drm_panel_enable(ctx->panel);

	ctx->dp_ops->mode_set(ctx, adjusted_mode);

	if(ctx->mipi_if) {
		ctx->dsi_dev->master_ops->mode_cfg(ctx->dsi_dev,
			(ctx->lcd_type == LCD_TYPE_MIPI_SLCD) ? 0 : 1);
	}
	ctx->dpms = DRM_MODE_DPMS_ON;
}

static void ingenic_disport_disable(struct drm_encoder *encoder)
{
	struct ingenic_disport *ctx = encoder_to_disport(encoder);

	if(ctx->dpms != DRM_MODE_DPMS_OFF) {
		drm_panel_unprepare(ctx->panel);
		drm_panel_disable(ctx->panel);
		if(ctx->mipi_if) {
			ctx->dsi_dev->master_ops->set_blank_mode(ctx->dsi_dev, 4);
		}
		ctx->dpms = DRM_MODE_DPMS_OFF;
	}
}

int ingenic_dsi_write_comand(struct drm_connector *connector,
			struct dsi_cmd_packet *cmd)
{
	struct ingenic_disport *ctx = connector_to_disport(connector);
	if(ctx->mipi_if) {
		return ctx->dsi_dev->master_ops->cmd_write(ctx->dsi_dev, cmd);
	}

	return 0;
}
EXPORT_SYMBOL(ingenic_dsi_write_comand);

static void ingenic_disport_mode_set(struct drm_encoder *encoder,
				struct drm_display_mode *mode,
				struct drm_display_mode *adjusted_mode)
{
//	struct ingenic_disport *disport = encoder_to_disport(encoder);
}

static struct drm_encoder_helper_funcs ingenic_disport_encoder_helper_funcs = {
	.mode_fixup = ingenic_disport_mode_fixup,
	.mode_set = ingenic_disport_mode_set,
	.enable = ingenic_disport_enable,
	.disable = ingenic_disport_disable,
};

static struct drm_encoder_funcs ingenic_disport_encoder_funcs = {
	.destroy = drm_encoder_cleanup,
};

static struct device_node *
of_graph_get_remote_port_parent(const struct device_node *node)
{
	struct device_node *np;
	unsigned int depth;

	np = of_parse_phandle(node, "remote-endpoint", 0);

	/* Walk 3 levels up only if there is 'ports' node. */
	for (depth = 2; depth && np; depth--) {
		np = of_get_next_parent(np);
	}
	return np;
}

static struct device_node *ingenic_disport_of_find_panel_node(struct device *dev)
{
	struct device_node *port, *np, *ep;

	port = of_get_child_by_name(dev->of_node, "port");
	if (!port)
		return NULL;

	ep = of_get_child_by_name(port, "endpoint");
	if (!ep)
		return NULL;

	np = of_graph_get_remote_port_parent(ep);

	return np;
}

static int ingenic_disport_parse_dt(struct ingenic_disport *ctx)
{
	struct device *dev = ctx->dev;

	ctx->panel_node = ingenic_disport_of_find_panel_node(dev);
	if (!ctx->panel_node)
		return -EINVAL;

	return 0;
}

int ingenic_disport_bind(struct drm_device *dev, struct drm_encoder *encoder)
{
	int ret;

	drm_encoder_init(dev, encoder, &ingenic_disport_encoder_funcs,
			 DRM_MODE_ENCODER_TMDS);

	drm_encoder_helper_add(encoder, &ingenic_disport_encoder_helper_funcs);
	encoder->possible_crtcs = (1 << DPU_MAX_CRTC_NUM) - 1;

	ret = ingenic_disport_create_connector(encoder);
	if (ret) {
		DRM_ERROR("failed to create connector ret = %d\n", ret);
		drm_encoder_cleanup(encoder);
		return ret;
	}

	return 0;
}

static int ingenic_disport_get_lcd_type(struct ingenic_disport *ctx)
{
	ctx->lcd_type = LCD_TYPE_NULL;
	ctx->mipi_if = false;
#ifdef CONFIG_DRM_INGENIC_DISPLAY_INTERFACE_TFT
	ctx->lcd_type = LCD_TYPE_TFT;
#endif
#ifdef CONFIG_DRM_INGENIC_DISPLAY_INTERFACE_SLCD
	ctx->lcd_type = LCD_TYPE_SLCD;
#endif
#ifdef CONFIG_DRM_INGENIC_DISPLAY_INTERFACE_MIPI_SLCD
	ctx->lcd_type = LCD_TYPE_MIPI_SLCD;
	ctx->mipi_if = true;
#endif
#ifdef CONFIG_DRM_INGENIC_DISPLAY_INTERFACE_MIPI_TFT
	ctx->lcd_type = LCD_TYPE_MIPI_TFT;
	ctx->mipi_if = true;
#endif
	return 0;
}

struct drm_encoder *ingenic_disport_probe(struct device *dev)
{
	struct ingenic_disport *ctx;
	int ret;

	ctx = devm_kzalloc(dev, sizeof(*ctx), GFP_KERNEL);
	if (!ctx)
		return ERR_PTR(-ENOMEM);

	ctx->dev = dev;

	ret = ingenic_disport_parse_dt(ctx);
	if (ret < 0) {
		devm_kfree(dev, ctx);
		return NULL;
	}

	if (ctx->panel_node) {
		ctx->panel = of_drm_find_panel(ctx->panel_node);
		if (!ctx->panel)
			return ERR_PTR(-EPROBE_DEFER);
	}

	ingenic_disport_get_lcd_type(ctx);
	if(ctx->lcd_type == LCD_TYPE_NULL)
		return ERR_PTR(-ENODEV);

	dpu_register_disport(ctx);

	if(ctx->mipi_if) {
		ctx->dsi_dev = jzdsi_init();
		if (!ctx->dsi_dev)
			return ERR_PTR(-ENOMEM);
	}
	ctx->dpms = DRM_MODE_DPMS_OFF;

	return &ctx->encoder;
}

int ingenic_disport_remove(struct drm_encoder *encoder)
{
	struct ingenic_disport *ctx = encoder_to_disport(encoder);

	ingenic_disport_disable(&ctx->encoder);

	if (ctx->panel)
		drm_panel_detach(ctx->panel);
	if(ctx->mipi_if) {
		jzdsi_remove(ctx->dsi_dev);
	}

	return 0;
}
