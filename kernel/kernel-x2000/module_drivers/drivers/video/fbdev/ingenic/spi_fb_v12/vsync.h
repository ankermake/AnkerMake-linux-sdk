#ifndef __VSYNC_H
#define __VSYNC_H

#include <linux/wait.h>
#include <linux/sched.h>


struct vsync_wq_t {
	wait_queue_head_t vsync_wq;
	wait_queue_head_t pan_wq;
	struct task_struct *vsync_thread;
	unsigned int vsync_skip_map;	/* 10 bits width */
	int vsync_skip_ratio;
#define TIMESTAMP_CAP	16
	struct {
		volatile int wp; /* write position */
		int rp;	/* read position */
		u64 value[TIMESTAMP_CAP];
	} timestamp;
};

extern void request_te_vsync_refresh(struct jzfb *jzfb);
extern int jzfb_te_irq_register(struct jzfb *jzfb);
extern int vsync_pan_display(struct fb_info *info, struct fb_var_screeninfo *var);
extern int vsync_set_auto_refresh(struct jzfb *jzfb, int auto_refresh);
extern int jzfb_vsync_thread(void *data);
extern int vsync_soft_set(void *data);
extern void vsync_soft_stop(void);
extern int vsync_wait(struct fb_info *info,unsigned long arg);
extern int vsync_wake(struct fb_info *info);

#endif /* __VSYNC_H */
