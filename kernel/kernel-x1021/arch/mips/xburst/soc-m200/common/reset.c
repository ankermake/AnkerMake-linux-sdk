/*
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */
#include <linux/kernel.h>
#include <linux/io.h>
#include <linux/ioport.h>
#include <linux/irq.h>
#include <linux/tty.h>
#include <linux/delay.h>
#include <linux/proc_fs.h>
#include <linux/kthread.h>
#include <linux/syscore_ops.h>
#include <linux/platform_device.h>
#include <linux/sched/rt.h>
#include <linux/seq_file.h>
#include <jz_proc.h>
#include <jz_notifier.h>
#include <soc/base.h>
#include <soc/cpm.h>
#include <soc/extal.h>
#include <soc/tcu.h>
#include <linux/slab.h>
#include <asm/reboot.h>

#define RTC_RTCCR		(0x00)	/* rw, 32, 0x00000081 */
#define RTC_RTCSR		(0x04)	/* rw, 32, 0x???????? */
#define RTC_RTCSAR		(0x08)	/* rw, 32, 0x???????? */
#define RTC_RTCGR		(0x0c)	/* rw, 32, 0x0??????? */
#define RTC_HCR			(0x20)  /* rw, 32, 0x00000000 */
#define RTC_HWFCR		(0x24)  /* rw, 32, 0x0000???0 */
#define RTC_HRCR		(0x28)  /* rw, 32, 0x00000??0 */
#define RTC_HWCR		(0x2c)  /* rw, 32, 0x00000008 */
#define RTC_HWRSR		(0x30)  /* rw, 32, 0x00000000 */
#define RTC_HSPR		(0x34)  /* rw, 32, 0x???????? */
#define RTC_WENR		(0x3c)  /* rw, 32, 0x00000000 */
#define RTC_CKPCR		(0x40)  /* rw, 32, 0x00000010 */
#define RTC_OWIPCR		(0x44)  /* rw, 32, 0x00000010 */
#define RTC_PWRONCR		(0x48)  /* rw, 32, 0x???????? */

#define WDT_TCSR		(0x0c)  /* rw, 32, 0x???????? */
#define WDT_TCER		(0x04)  /* rw, 32, 0x???????? */
#define WDT_TDR			(0x00)  /* rw, 32, 0x???????? */
#define WDT_TCNT		(0x08)  /* rw, 32, 0x???????? */

#define RTCCR_WRDY		BIT(7)
#define WENR_WEN                BIT(31)

#define RECOVERY_SIGNATURE	(0x001a1a)
#define REBOOT_SIGNATURE	(0x003535)
#define UNMSAK_SIGNATURE	(0x7c0000)//do not use these bits
#define FASTBOOT_SIGNATURE      (0x000666)// means "FASTBOOT"

static void wdt_start_count(int msecs)
{
	int time = JZ_EXTAL_RTC / 64 * msecs / 1000;
	if(time > 65535)
		time = 65535;

	outl(1 << 16,TCU_IOBASE + TCU_TSCR);

	outl(0,WDT_IOBASE + WDT_TCNT);		//counter
	outl(time,WDT_IOBASE + WDT_TDR);	//data
	outl((3<<3 | 1<<1),WDT_IOBASE + WDT_TCSR);
	outl(0,WDT_IOBASE + WDT_TCER);
	outl(1,WDT_IOBASE + WDT_TCER);
}

static void wdt_stop_count(void)
{
	outl(1 << 16,TCU_IOBASE + TCU_TSCR);
	outl(0,WDT_IOBASE + WDT_TCNT);		//counter
	outl(65535,WDT_IOBASE + WDT_TDR);	//data
	outl(1 << 16,TCU_IOBASE + TCU_TSSR);
}

static void inline rtc_write_reg(int reg,int value)
{
	while(!(inl(RTC_IOBASE + RTC_RTCCR) & RTCCR_WRDY));
	outl(0xa55a,(RTC_IOBASE + RTC_WENR));
	while(!(inl(RTC_IOBASE + RTC_RTCCR) & RTCCR_WRDY));
	while(!(inl(RTC_IOBASE + RTC_WENR) & WENR_WEN));
	while(!(inl(RTC_IOBASE + RTC_RTCCR) & RTCCR_WRDY));
	outl(value,(RTC_IOBASE + reg));
	while(!(inl(RTC_IOBASE + RTC_RTCCR) & RTCCR_WRDY));
}

/*
 * Function: Keep power for CPU core when reset.
 * So that EPC, tcsm and so on can maintain it's status after reset-key pressed.
 */
void reset_keep_power(int keep_pwr)
{
	if (keep_pwr)
		rtc_write_reg(RTC_PWRONCR,
			      inl(RTC_IOBASE + RTC_PWRONCR) & ~(1 << 0));
}

#define HWFCR_WAIT_TIME(x) ((x > 0x7fff ? 0x7fff: (0x7ff*(x)) / 2000) << 5)

void jz_hibernate(void)
{
	local_irq_disable();
	/* Set minimum wakeup_n pin low-level assertion time for wakeup: 1000ms */
	rtc_write_reg(RTC_HWFCR, HWFCR_WAIT_TIME(1000));

	/* Set reset pin low-level assertion time after wakeup: must  > 60ms */
	rtc_write_reg(RTC_HRCR, (60 << 5));

	/* clear wakeup status register */
	rtc_write_reg(RTC_HWRSR, 0x0);

	rtc_write_reg(RTC_HWCR, 0x8);

	/* Put CPU to hibernate mode */
	rtc_write_reg(RTC_HCR, 0x1);

	jz_notifier_call(NOTEFY_PROI_HIGH, JZ_POST_HIBERNATION, NULL);

	mdelay(200);

	while(1)
		printk("We should NOT come here.%08x\n",inl(RTC_IOBASE + RTC_HCR));
}

void jz_wdt_restart(char *command)
{
	printk("Restarting after 4 ms\n");
	if ((command != NULL) && !strcmp(command, "recovery")) {
		while(cpm_inl(CPM_CPPSR) != RECOVERY_SIGNATURE) {
			printk("set RECOVERY_SIGNATURE\n");
			cpm_outl(0x5a5a,CPM_CPSPPR);
			cpm_outl(RECOVERY_SIGNATURE,CPM_CPPSR);
			cpm_outl(0x0,CPM_CPSPPR);
			udelay(100);
		}
	} else if ((command != NULL) && !strcmp(command, "bootloader")) {
		cpm_outl(0x5a5a,CPM_CPSPPR);
		cpm_outl(FASTBOOT_SIGNATURE,CPM_CPPSR);
		cpm_outl(0x0,CPM_CPSPPR);
	} else {
		cpm_outl(0x5a5a,CPM_CPSPPR);
		cpm_outl(REBOOT_SIGNATURE,CPM_CPPSR);
		cpm_outl(0x0,CPM_CPSPPR);
	}

	wdt_start_count(4);
	mdelay(200);
	while(1)
		printk("check wdt.\n");
}

static void hibernate_restart(void) {
	uint32_t rtc_rtcsr,rtc_rtccr;

	while(!(inl(RTC_IOBASE + RTC_RTCCR) & RTCCR_WRDY));
	rtc_rtcsr = inl(RTC_IOBASE + RTC_RTCSR);
	rtc_rtccr = inl(RTC_IOBASE + RTC_RTCCR);

	rtc_write_reg(RTC_RTCSAR,rtc_rtcsr + 5);
	rtc_rtccr &= ~(1 << 4);
	rtc_write_reg(RTC_RTCCR,rtc_rtccr | 0x3<<2);

	/* Clear reset status */
	cpm_outl(0,CPM_RSR);

	/* Set minimum wakeup_n pin low-level assertion time for wakeup: 1000ms */
	rtc_write_reg(RTC_HWFCR, HWFCR_WAIT_TIME(1000));

	/* Set reset pin low-level assertion time after wakeup: must  > 60ms */
	rtc_write_reg(RTC_HRCR, (60 << 5));

	/* clear wakeup status register */
	rtc_write_reg(RTC_HWRSR, 0x0);

	rtc_write_reg(RTC_HWCR, 0x9);
	/* Put CPU to hibernate mode */
	rtc_write_reg(RTC_HCR, 0x1);

	mdelay(200);
	while(1)
		printk("We should NOT come here.%08x\n",inl(RTC_IOBASE + RTC_HCR));

}
#ifdef CONFIG_HIBERNATE_RESET
void jz_hibernate_restart(char *command)
{
	local_irq_disable();

	if ((command != NULL)
	     &&(!strcmp(command, "recovery")
	     || !strcmp(command, "bootloader")) {
		jz_wdt_restart(command);
	}

	hibernate_restart();
}
#endif
static void jz_burner_boot(void)
{
	unsigned int val = 'b' << 24 | 'u' << 16 | 'r' << 8 | 'n';

	cpm_outl(val, CPM_SLPC);

	wdt_start_count(2);
	while(1);
}

int __init reset_init(void)
{
	pm_power_off = jz_hibernate;
#ifdef CONFIG_HIBERNATE_RESET
	_machine_restart = jz_hibernate_restart;
#else
	_machine_restart = jz_wdt_restart;
#endif
	return 0;
}
arch_initcall(reset_init);


struct wdt_reset {
	unsigned stop;
	unsigned msecs;
	struct task_struct *task;
	unsigned count;
};

static int reset_task(void *data) {
	struct wdt_reset *wdt = data;
	const struct sched_param param = {
		.sched_priority = MAX_RT_PRIO-1,
	};
	sched_setscheduler(current,SCHED_RR,&param);

	wdt_start_count(wdt->msecs + 1000);
	while (1) {
		if(kthread_should_stop()) {
			wdt_stop_count();
			break;
		}
		outl(0,WDT_IOBASE + WDT_TCNT);
		msleep(wdt->msecs);
	}

	return 0;
}

/* ============================wdt control proc=================================== */
static int wdt_time_show(struct seq_file *filq, void *v)
{
	int len = 0;
	struct wdt_reset *wdt = filq->private;

	len += seq_printf(filq, "%d msecs\n", wdt->msecs);

	return len;
}

static int wdt_time_write(struct file *file, const char __user *buffer,
						  size_t usize, loff_t *off)
{
	unsigned msecs= 0;
	struct wdt_reset *wdt = ((struct seq_file *)file->private_data)->private;

	if(!wdt->stop)
		return -EBUSY;

	sscanf(buffer,"%d\n",&msecs);
	if(msecs < 1000) msecs = 1000;
	if(msecs > 30000) msecs = 30000;

	wdt->msecs = msecs;
	return usize;
}
static struct jz_single_file_ops wdt_time_proc_fops = {
	.read = wdt_time_show,
	.write = wdt_time_write,
};
/* ============================wdt time proc end================================== */
/* ============================wdt control proc=================================== */
static int wdt_control_show(struct seq_file *filq, void *v)
{
	int len = 0;
	struct wdt_reset *wdt = filq->private;

	len += seq_printf(filq, wdt->stop?">off<on\n":"off>on<\n");

	return len;
}

static int wdt_control_write(struct file *file, const char __user *buffer,
							 size_t usize, loff_t *off)
{
	struct wdt_reset *wdt = ((struct seq_file *)file->private_data)->private;

	if(!strncmp(buffer,"on",2) && (wdt->stop == 1)) {
		wdt->task = kthread_run(reset_task, wdt, "reset_task%d",wdt->count++);
		wdt->stop = 0;
	} else if(!strncmp(buffer,"off",3) && (wdt->stop == 0)) {
		kthread_stop(wdt->task);
		wdt->stop = 1;
	}

	return usize;
}
static struct jz_single_file_ops wdt_control_proc_fops = {
	.read = wdt_control_show,
	.write = wdt_control_write,
};
/* ============================wdt control proc end =============================== */
/* ============================reset proc=================================== */
static char *reset_command[] = {"wdt","hibernate","recovery","bootloader", "burnerboot"};
static int reset_show(struct seq_file *filq, void *v)
{
	int len = 0, i;

	for(i = 0; i < ARRAY_SIZE(reset_command); i++)
		len += seq_printf(filq, "%s\t", reset_command[i]);
	len += seq_printf(filq,"\n");

	return len;
}

static int reset_write(struct file *file, const char __user *buffer,
					   size_t usize, loff_t *off)
{
	int command_size = 0;
	int i;

	if(usize == 0)
		return -EINVAL;

	command_size = ARRAY_SIZE(reset_command);
	for(i = 0;i < command_size; i++) {
		if(!strncmp(buffer, reset_command[i], strlen(reset_command[i])))
			break;
	}
	if(i == command_size)
		return -EINVAL;

	local_irq_disable();
	switch(i) {
	case 0:
		jz_wdt_restart(NULL);
		break;
	case 1:
		hibernate_restart();
		break;
	case 2:
		jz_wdt_restart("recovery");
		break;
	case 3:
		jz_wdt_restart("bootloader");
		break;
	case 4:
		jz_burner_boot();
		break;
	default:
		printk("not support command %d\n", i);
	}

	return usize;
}
static struct jz_single_file_ops reset_proc_fops = {
	.read = reset_show,
	.write = reset_write,
};
/* ============================reset proc end=============================== */

static int wdt_probe(struct platform_device *pdev)
{
	struct wdt_reset *wdt;
	struct proc_dir_entry *p, *res;

	wdt = kmalloc(sizeof(struct wdt_reset),GFP_KERNEL);
	if(!wdt) {
		return -ENOMEM;
	}

	wdt->count = 0;
	wdt->msecs = 3000;

	dev_set_drvdata(&pdev->dev,wdt);
#ifdef CONFIG_SUSPEND_WDT
	wdt->stop = 0;
	wdt->task = kthread_run(reset_task, wdt, "reset_task%d",wdt->count++);
#else
	wdt->stop = 1;
#endif
	p = jz_proc_mkdir("reset");
	if (!p) {
		pr_warning("create_proc_entry for common reset failed.\n");
		return -ENODEV;
	}

	res = jz_proc_create_data("reset", 0444, p, &reset_proc_fops, wdt);
	res = jz_proc_create_data("wdt_control", 0444, p, &wdt_control_proc_fops, wdt);
	res = jz_proc_create_data("wdt_time", 0444, p, &wdt_time_proc_fops, wdt);
	if (!res) {
		pr_warning("proc_create_data for reset wdt failed.\n");
		return -ENODEV;
	}

	return 0;
}
int wdt_suspend(struct platform_device *pdev, pm_message_t state)
{
	struct wdt_reset *wdt = dev_get_drvdata(&pdev->dev);
	if(wdt->stop)
		return 0;
	kthread_stop(wdt->task);
	return 0;
}

void wdt_shutdown(struct platform_device *pdev)
{
	wdt_stop_count();
}

int wdt_resume(struct platform_device *pdev)
{
	struct wdt_reset *wdt = dev_get_drvdata(&pdev->dev);
	if(wdt->stop)
		return 0;
	wdt->task = kthread_run(reset_task, wdt, "reset_task%d",wdt->count++);
	return 0;
}

static struct platform_device wdt_pdev = {
	.name		= "wdt_reset",
};

static struct platform_driver wdt_pdrv = {
	.probe		= wdt_probe,
	.shutdown	= wdt_shutdown,
	.suspend	= wdt_suspend,
	.resume		= wdt_resume,
	.driver		= {
		.name	= "wdt_reset",
		.owner	= THIS_MODULE,
	},
};

static int __init init_reset(void)
{
	platform_driver_register(&wdt_pdrv);
	platform_device_register(&wdt_pdev);
	return 0;
}
module_init(init_reset);
