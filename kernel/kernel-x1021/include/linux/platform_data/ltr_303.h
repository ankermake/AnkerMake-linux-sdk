#ifndef __LTR_303ALS_H__
#define __LTR_303ALS_H__

struct ltr303_platform_data {
	/* ALS */
	uint16_t pfd_levels[5];
	uint16_t pfd_als_lowthresh;
	uint16_t pfd_als_highthresh;
	int pfd_disable_als_on_suspend;

	/* Interrupt */
	int pfd_gpio_int_no;
};
#endif /* __LTR_303ALS_H__ */
