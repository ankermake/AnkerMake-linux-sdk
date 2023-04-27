#ifndef __ingenic_DRV_H__
#define __ingenic_DRV_H__

#include <linux/clk.h>
#include <linux/cpufreq.h>
#include <linux/module.h>
#include <linux/platform_device.h>
#include <linux/pm.h>
#include <linux/pm_runtime.h>
#include <linux/slab.h>
#include <linux/of.h>
#include <linux/of_device.h>
#include <linux/list.h>
#include <linux/reservation.h>

#include <drm/drmP.h>
#include <drm/drm_crtc_helper.h>
#include <drm/drm_gem_cma_helper.h>
#include <drm/drm_fb_cma_helper.h>
#include <drm/drm_flip_work.h>
#include "jz_dsim.h"

/* Defaulting to maximum capability of DPU */
#define DPU_DEFAULT_MAX_PIXELCLOCK 300000
#define DPU_DEFAULT_MAX_WIDTH	2047
#define DPU_DEFAULT_MAX_HEIGHT	2047
#define DPU_DEFAULT_MIN_WIDTH	4
#define DPU_DEFAULT_MIN_HEIGHT	4
#define DPU_DEFAULT_MAX_BANDWIDTH	(2047*2047*60)
#define DPU_DESC_NUM 2
#define DPU_SUPPORT_LAYER_NUM 4
#define DESC_ALIGN 8
#define MAX_BITS_PER_PIX (32)
#define MAX_STRIDE_VALUE (2047)
#define DPU_MAX_CRTC_NUM 2

struct ingenic_drm_plane {
	struct drm_plane base;
	uint32_t index;
	uint32_t nv_en:1;
	uint32_t lay_en:1;
	uint32_t scale_en:1;
	uint32_t zorder:3;
	uint32_t color:3;
	uint32_t format:4;
	uint32_t g_alpha_en:1;
	uint32_t g_alpha_val:8;
	uint32_t src_x;
	uint32_t src_y;
	uint32_t src_w;
	uint32_t src_h;
	uint32_t disp_pos_x;
	uint32_t disp_pos_y;
	uint32_t scale_w;
	uint32_t scale_h;
	uint32_t stride;
	uint32_t uvstride;
	/*NV12, NV21 have two plane*/
	uint32_t addr_offset[2];
	struct drm_framebuffer *pending_fb;
};

struct ingenic_drm_srd_plane {
	struct drm_plane base;
	uint32_t format;
	uint32_t color;
	uint32_t stride;
	uint32_t addr_offset;
	struct drm_framebuffer *pending_fb;
};

typedef enum ingenic_lcd_type {
	LCD_TYPE_NULL = 0,
        LCD_TYPE_TFT = 1,
        LCD_TYPE_SLCD =2,
        LCD_TYPE_MIPI_SLCD = 3,
        LCD_TYPE_MIPI_TFT = 4,
}ingenic_lcd_type_t;


enum smart_lcd_type {
	SMART_LCD_TYPE_6800,
	SMART_LCD_TYPE_8080,
	SMART_LCD_TYPE_SPI_3,
	SMART_LCD_TYPE_SPI_4,
};

/* smart lcd format */
enum smart_lcd_format {
	SMART_LCD_FORMAT_565,
	SMART_LCD_FORMAT_666,
	SMART_LCD_FORMAT_888,
};

/* smart lcd command width */
enum smart_lcd_cwidth {
	SMART_LCD_CWIDTH_8_BIT,
	SMART_LCD_CWIDTH_9_BIT,
	SMART_LCD_CWIDTH_16_BIT,
	SMART_LCD_CWIDTH_18_BIT,
	SMART_LCD_CWIDTH_24_BIT,
};

/* smart lcd data width */
enum smart_lcd_dwidth {
	SMART_LCD_DWIDTH_8_BIT,
	SMART_LCD_DWIDTH_9_BIT,
	SMART_LCD_DWIDTH_16_BIT,
	SMART_LCD_DWIDTH_18_BIT,
	SMART_LCD_DWIDTH_24_BIT,
};

enum smart_config_type {
	SMART_CONFIG_DATA,
	SMART_CONFIG_PRM,
	SMART_CONFIG_CMD,
	SMART_CONFIG_UDELAY,
};

struct smart_lcd_data_table {
	enum smart_config_type type;
	uint32_t value;
};

enum tft_lcd_color_even {
	TFT_LCD_COLOR_EVEN_RGB,
	TFT_LCD_COLOR_EVEN_RBG,
	TFT_LCD_COLOR_EVEN_BGR,
	TFT_LCD_COLOR_EVEN_BRG,
	TFT_LCD_COLOR_EVEN_GBR,
	TFT_LCD_COLOR_EVEN_GRB,
};

enum tft_lcd_color_odd {
	TFT_LCD_COLOR_ODD_RGB,
	TFT_LCD_COLOR_ODD_RBG,
	TFT_LCD_COLOR_ODD_BGR,
	TFT_LCD_COLOR_ODD_BRG,
	TFT_LCD_COLOR_ODD_GBR,
	TFT_LCD_COLOR_ODD_GRB,
};

enum tft_lcd_mode {
	TFT_LCD_MODE_PARALLEL_888,
	TFT_LCD_MODE_PARALLEL_666,
	TFT_LCD_MODE_PARALLEL_565,
	TFT_LCD_MODE_SERIAL_RGB,
	TFT_LCD_MODE_SERIAL_RGBD,
};

typedef enum display_data_channel {
	DPU_SIMPLE_READ_CHANNEL,
	DPU_CMP_OUTPUT_CHANNEL,
} data_channel_t;

struct tft_config {
	uint32_t pix_clk_inv:1;
	uint32_t de_dl:1;
	uint32_t sync_dl:1;
	enum tft_lcd_color_even color_even;
	enum tft_lcd_color_odd color_odd;
	enum tft_lcd_mode mode;
};

struct smart_config {
	uint32_t te_switch:1;
	uint32_t te_mipi_switch:1;
	uint32_t te_md:1;
	uint32_t te_dp:1;
	uint32_t te_anti_jit:1;
	uint32_t dc_md:1;
	uint32_t wr_md:1;
	enum smart_lcd_type smart_type;
	enum smart_lcd_format pix_fmt;
	enum smart_lcd_dwidth dwidth;
	enum smart_lcd_cwidth cwidth;
	uint32_t bus_width;

	unsigned long write_gram_cmd;
	uint32_t length_cmd;
	struct smart_lcd_data_table *data_table;
	uint32_t length_data_table;
	int (*init) (void);
	int (*gpio_for_slcd) (void);
};

struct ingenic_srd_priv {
	struct ingenicfb_sreadesc *sreadesc[2];
	dma_addr_t sreadesc_phys[2];
	struct ingenic_drm_srd_plane plane;
};

struct ingenic_cmp_priv {
	struct ingenicfb_framedesc *framedesc[2];
	dma_addr_t framedesc_phys[2];
	struct ingenicfb_layerdesc *layerdesc[2][DPU_SUPPORT_LAYER_NUM];
	dma_addr_t layerdesc_phys[2][DPU_SUPPORT_LAYER_NUM];
	struct ingenic_drm_plane plane[DPU_SUPPORT_LAYER_NUM];
	unsigned dither_enable;
	struct {
		unsigned dither_red;
		unsigned dither_green;
		unsigned dither_blue;
	} dither;
};

struct ingenic_crtc {
	struct drm_crtc base;

	struct drm_pending_vblank_event *event;

	ingenic_lcd_type_t lcd_type;
	bool enable;
	int slcd_continua;
	int frame_current;
	int frame_next;
	struct ingenic_dpu_ops *dpu_ops;
	int tft_under_cnt;
	int wover_cnt;
	int pipe;

	data_channel_t data_channel;
	void *crtc_priv;
};

struct ingenic_dpu_ops {
	void (*enable)(struct ingenic_crtc *crtc);
	void (*disable)(struct ingenic_crtc *crtc);
	void (*enable_vblank)(struct ingenic_crtc *crtc, bool enable);
	void (*mode_set)(struct ingenic_crtc *);
	void (*commit)(struct ingenic_crtc *);
	irqreturn_t (*irq_handler)(struct ingenic_crtc *crtc);
};

struct ingenic_disport {
	struct drm_encoder encoder;
	struct drm_connector connector;

	struct device_node *panel_node;
	struct drm_panel *panel;

	struct device *dev;

	int dpms;

	ingenic_lcd_type_t lcd_type;
	struct dpu_dp_ops *dp_ops;

	bool mipi_if;
	bool mipi_init;
	struct dsi_device *dsi_dev;
	int data_channel;
};

struct dpu_dp_ops {
	void (*enable)(struct ingenic_disport *disport);
	void (*disable)(struct ingenic_disport *disport);
	void (*mode_set)(struct ingenic_disport *, struct drm_display_mode *);
};

struct ingenic_drm_private {
	void __iomem *mmio;

	struct clk *disp_clk;    /* display dpll */
	struct clk *clk;         /* functional clock */
//	struct kref clk_refcount;
	int irq;

	/* don't attempt resolutions w/ higher W * H * Hz: */
	uint32_t max_bandwidth;
	/*
	 * Pixel Clock will be restricted to some value as
	 * defined in the device datasheet measured in KHz
	 */
	uint32_t max_pixelclock;
	/*
	 * Max allowable width is limited on a per device basis
	 * measured in pixels
	 */
	uint32_t max_width;
	/*
	 * Max allowable height is limited on a per device basis
	 * measured in pixels
	 */
	uint32_t max_height;

	struct workqueue_struct *wq;

	struct drm_fbdev_cma *fbdev;

	struct drm_crtc *crtc[DPU_MAX_CRTC_NUM];
	int pipe;

	struct drm_encoder *encoders;

	struct drm_connector *connectors;
	struct {
		struct drm_atomic_state *state;
		struct work_struct work;
		struct mutex lock;
	} commit;
	struct drm_device *drm_dev;

	struct {
		struct drm_property *galpha;
		struct drm_property *zorder;
	} props;
};

struct lcd_panel {
	ingenic_lcd_type_t lcd_type;
	struct jzdsi_data *dsi_pdata;
	struct smart_config *smart_config;
	struct tft_config *tft_config;
};

#define to_ingenic_plane(x)	container_of(x, struct ingenic_drm_plane, base)
#define to_ingenic_srd_plane(x)	container_of(x, struct ingenic_drm_srd_plane, base)
#define to_ingenic_crtc(x)	container_of(x, struct ingenic_crtc, base)


enum drm_plane_type ingenic_cmp_plane_get_type(uint32_t index);
void ingenic_cmp_plane_get_formats(uint32_t index, const uint32_t **formats, int *size);
void ingenic_srd_plane_get_formats(const uint32_t **formats, int *size);
int ingenic_srd_plane_init(struct drm_device *dev,
		      struct ingenic_drm_srd_plane *ingenic_plane,
		      unsigned long possible_crtcs, enum drm_plane_type type,
		      const uint32_t *formats, uint32_t fcount,
		      uint32_t index);
int ingenic_cmp_plane_init(struct drm_device *dev,
		      struct ingenic_drm_plane *ingenic_plane,
		      unsigned long possible_crtcs, enum drm_plane_type type,
		      const uint32_t *formats, uint32_t fcount,
		      uint32_t index);

void dpu_dump_cmp_all(struct ingenic_crtc *crtc);
void dpu_dump_srd_all(struct ingenic_crtc *crtc);
struct drm_crtc *ingenic_cmp_crtc_create(struct drm_device *dev);
struct drm_crtc *ingenic_srd_crtc_create(struct drm_device *dev);
void ingenic_crtc_cancel_page_flip(struct drm_crtc *crtc,
				  struct drm_file *file);
extern irqreturn_t ingenic_crtc_irq(int irq, void *arg);
extern void ingenic_crtc_update_clk(struct drm_crtc *crtc);
int ingenic_crtc_mode_valid(struct drm_crtc *crtc,
			   struct drm_display_mode *mode);
struct reservation_object *ingenic_drv_lookup_resobj(struct drm_gem_object *obj);
extern int cmp_register_crtc(struct ingenic_crtc *crtc);
extern int srd_register_crtc(struct ingenic_crtc *crtc);
extern int dpu_register_disport(struct ingenic_disport *);
extern void enable_vblank(struct drm_crtc *crtc, bool enable);
extern int ingenic_dsi_write_comand(struct drm_connector *,
				struct dsi_cmd_packet *cmd);

extern int refresh_pixclock_auto_adapt(struct ingenic_disport *ctx,
			struct drm_display_mode *mode);
#endif /* __ingenic_DRV_H__ */
