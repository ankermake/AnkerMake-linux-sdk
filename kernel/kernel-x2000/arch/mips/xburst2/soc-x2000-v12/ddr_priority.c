#include <linux/kernel.h>
#include <linux/io.h>
#include <linux/ioport.h>
#include <linux/syscore_ops.h>
#include <linux/slab.h>
#include <linux/debugfs.h>
#include <linux/module.h>
#include <linux/moduleparam.h>
#include <linux/spinlock.h>
#include <soc/ddr.h>
#include <soc/cpm.h>


#define MAX_CHANNELS	7

#define GMAC_MSC_CHANNEL 0	//DDR ch0
#define VPU_CHANNEL	 1	//DDR ch1
#define DPU_VPU_CHANNEL	 3	//DDR ch3
#define AHB2_CHANNEL 	 5	//DDR ch5
#define CPU_CHANNEL 	 6	//DDR ch6

struct ddr_pri_chan {
	int chan;
	const char *desc;
};

static struct ddr_pri_chan pri_chans[] = {
	{0, "gmac&msc"},
	{1, "vpu&isp"},
	{3, "dpu&cim"},
	{5, "ahb2&audio&apb"},
	{6, "cpu"}
};

struct ddr_priority {
	struct dentry *root;
	unsigned int pri;
};

static struct ddr_priority ddr_priority;


static ssize_t pri_xport_read(struct file *file, char __user *user_buf, size_t count, loff_t *ppos)
{
	int i;
	unsigned int chan;
	unsigned int xport;

	for (i = 0; i < ARRAY_SIZE(pri_chans); i++) {
		chan = pri_chans[i].chan;
		xport = readl((void *)DDRC_CCHC(chan)) & (DDRC_CCHC_PORT_PRI);
		printk("port priority  chan%d %s : %s\n", chan, pri_chans[i].desc, xport ? "high" : "low");
	}

	return 0;
}

static ssize_t pri_xport_write(struct file *file, const char __user *user_buf, size_t count, loff_t *ppos)
{

	char buf[16];
	char *val = buf;
	char *channel;
	unsigned int chan;
	unsigned int priority;
	unsigned int cchc;

	if (copy_from_user(buf, user_buf, min(count, sizeof(buf) - 1)))
		return -EFAULT;

	channel = strsep(&val, ":");
	chan = simple_strtoul(channel, NULL, 10);
	if ((chan < 0) || (chan > 7)) {
		printk("invalid channel\n");
		return -EINVAL;
	}

	priority = simple_strtoul(val, NULL, 10);
	if ((priority < 0) || (priority > 1)) {
		printk("invalid priority\n");
		return -EINVAL;
	}

	cchc = readl((void *)DDRC_CCHC(chan));
	cchc &= ~DDRC_CCHC_PORT_PRI;
	cchc |= priority << DDRC_CCHC_PORT_PRI_BIT;
	writel(cchc, (void *)DDRC_CCHC(chan));
	printk("set port priority : chan%d  %s\n", chan, priority ? "high" : "low");

	return count;
}

static const struct file_operations pri_xport_fops = {
	.read	= pri_xport_read,
	.write	= pri_xport_write,
	.open	= simple_open,
	.llseek	= default_llseek,
};

static ssize_t pri_master_read(struct file *file, char __user *user_buf, size_t count, loff_t *ppos)
{
	ssize_t ret = 0;
	unsigned int pri;

	pri = cpm_inl(CPM_MESTSEL);
	printk("helix	: %d\n", (pri & PRI_HELIX_MSK) >> PRI_HELIX_BIT);
	printk("felix	: %d\n", (pri & PRI_FELIX_MSK) >> PRI_FELIX_BIT);
	printk("isp0	: %d\n", (pri & PRI_ISP0_MSK) >> PRI_ISP0_BIT);
	printk("isp1	: %d\n", (pri & PRI_ISP1_MSK) >> PRI_ISP1_BIT);
	printk("lcd	: %d\n", (pri & PRI_LCD_MSK) >> PRI_LCD_BIT);
	printk("cim	: %d\n", (pri & PRI_CIM_MSK) >> PRI_CIM_BIT);
	printk("rotate	: %d\n", (pri & PRI_ROTATE_MSK) >> PRI_ROTATE_BIT);

	return ret;
}

static ssize_t pri_master_write(struct file *file, const char __user *user_buf, size_t count, loff_t *ppos)
{
	char buf[16];
	char *val = buf;
	char *device;
	unsigned int priority;
	unsigned int reg;

	if (copy_from_user(buf, user_buf, min(count, sizeof(buf) - 1)))
		return -EFAULT;

	device = strsep(&val, ":");

	priority = simple_strtoul(val, NULL, 10);
	if ((priority < 0) || (priority > 3)) {
		printk("invalid priority\n");
		return -EINVAL;
	}

	reg = cpm_inl(CPM_MESTSEL);

	if (!strcmp(device, "helix")) {
		reg &= ~(PRI_HELIX_MSK);
		reg |= priority << PRI_HELIX_BIT;
	} else if (!strcmp(device, "felix")) {
		reg &= ~(PRI_FELIX_MSK);
		reg |= priority << PRI_FELIX_BIT;
	} else if (!strcmp(device, "isp0")) {
		reg &= ~(PRI_ISP0_MSK);
		reg |= priority << PRI_ISP0_BIT;
	} else if (!strcmp(device, "isp1")) {
		reg &= ~(PRI_ISP1_MSK);
		reg |= priority << PRI_ISP1_BIT;
	} else if (!strcmp(device, "lcd")) {
		reg &= ~(PRI_LCD_MSK);
		reg |= priority << PRI_LCD_BIT;
	} else if (!strcmp(device, "cim")) {
		reg &= ~(PRI_CIM_MSK);
		reg |= priority << PRI_CIM_BIT;
	} else if (!strcmp(device, "rotate")) {
		reg &= ~(PRI_ROTATE_MSK);
		reg |= priority << PRI_ROTATE_BIT;
	} else {
		printk("invalid device\n");
		return -EINVAL;
	}

	cpm_outl(reg, CPM_MESTSEL);

	return count;
}

static const struct file_operations pri_master_fops = {
	.read	= pri_master_read,
	.write	= pri_master_write,
	.open	= simple_open,
	.llseek	= default_llseek,
};

static ssize_t pri_fix_read(struct file *file, char __user *user_buf, size_t count, loff_t *ppos)
{
	int i;
	unsigned int chan;
	unsigned int fix_en;
	unsigned int fix_pri;

	for (i = 0; i < ARRAY_SIZE(pri_chans); i++) {
		chan = pri_chans[i].chan;
		fix_en = readl((void *)DDRC_CCHC(chan)) & DDRC_CCHC_TR_FIX_PRI_EN;
		if (fix_en) {
			fix_pri = (readl((void *)DDRC_CCHC(chan)) & DDRC_CCHC_TR_FIX_PRI_MSK) >> DDRC_CCHC_TR_FIX_PRI_BIT;
			printk("fix priority  chan%d %s : %d\n", chan, pri_chans[i].desc, fix_pri);

		} else {
			printk("fix priority has no effect on chan%d %s\n", chan, pri_chans[i].desc);
		}

	}

	return 0;
}

static ssize_t pri_fix_write(struct file *file, const char __user *user_buf, size_t count, loff_t *ppos)
{
	char buf[64];
	char *val = buf;
	char *channel;
	unsigned int chan;
	unsigned int priority;
	unsigned int cchc;

	if (copy_from_user(buf, user_buf, min(count, sizeof(buf) - 1)))
		return -EFAULT;

	channel = strsep(&val, ":");
	chan = simple_strtoul(channel, NULL, 10);
	if ((chan < 0) || (chan > 7)) {
		printk("invalid channel\n");
		return -EINVAL;
	}

	if (!strncmp(val, "disable", count-3)) {
		cchc = readl((void *)DDRC_CCHC(chan));
		cchc &= ~DDRC_CCHC_TR_FIX_PRI_EN;
		cchc &= ~DDRC_CCHC_TR_FIX_PRI_MSK;
		writel(cchc, (void *)DDRC_CCHC(chan));

		printk("chan%d disable fix priority\n", chan);

	} else {

		priority = simple_strtoul(val, NULL, 10);
		if ((priority < 0) || (priority > 4)) {
			printk("invalid priority\n");
			return -EINVAL;
		}

		cchc = readl((void *)DDRC_CCHC(chan));
		cchc |= DDRC_CCHC_TR_FIX_PRI_EN;
		cchc &= ~DDRC_CCHC_TR_FIX_PRI_MSK;
		cchc |= (priority << DDRC_CCHC_TR_FIX_PRI_BIT);
		writel(cchc, (void *)DDRC_CCHC(chan));

		printk("set fix priority : chan%d  pri : %d\n", chan, priority);
	}

	return count;
}

static const struct file_operations pri_fix_fops = {
	.read	= pri_fix_read,
	.write	= pri_fix_write,
	.open	= simple_open,
	.llseek	= default_llseek,
};

static ssize_t pri_wtr_timeout_read(struct file *file, char __user *user_buf, size_t count, loff_t *ppos)
{
	int i;
	unsigned int chan;
	unsigned int w_timeout;

	for (i = 0; i < ARRAY_SIZE(pri_chans); i++) {
		chan = pri_chans[i].chan;
		w_timeout = (readl((void *)DDRC_CCHC(chan)) & DDRC_CCHC_WTR_TIMEOUT_MSK) >> DDRC_CCHC_WTR_TIMEOUT_BIT;
		printk("write timeout of chan%d %s : 0x%x\n", chan, pri_chans[i].desc, w_timeout);
	}

	return 0;
}

static ssize_t pri_wtr_timeout_write(struct file *file, const char __user *user_buf, size_t count, loff_t *ppos)
{
	char buf[16];
	char *val = buf;
	char *channel;
	unsigned int chan;
	unsigned int timeout;
	unsigned int cchc;

	if (copy_from_user(buf, user_buf, min(count, sizeof(buf) - 1)))
		return -EFAULT;

	channel = strsep(&val, ":");
	chan = simple_strtoul(channel, NULL, 10);
	if ((chan < 0) || (chan > 7)) {
		printk("invalid channel\n");
		return -EINVAL;
	}

	timeout = simple_strtoul(val, NULL, 10);
	if ((timeout < 0) || (timeout > 0xf)) {
		printk("invalid itimeout value\n");
		return -EINVAL;
	}

	cchc = readl((void *)DDRC_CCHC(chan));
	cchc &= ~DDRC_CCHC_WTR_TIMEOUT_MSK;
	cchc |= timeout << DDRC_CCHC_WTR_TIMEOUT_BIT;
	writel(cchc, (void *)DDRC_CCHC(chan));
	printk("set write timeout : chan%d  0x%x\n", chan, timeout);



	return count;
}

static const struct file_operations pri_wtr_timeout_fops = {
	.read	= pri_wtr_timeout_read,
	.write	= pri_wtr_timeout_write,
	.open	= simple_open,
	.llseek	= default_llseek,
};


static ssize_t pri_rtr_timeout_read(struct file *file, char __user *user_buf, size_t count, loff_t *ppos)
{
	int i;
	unsigned int chan;
	unsigned int r_timeout;

	for (i = 0; i < ARRAY_SIZE(pri_chans); i++) {
		chan = pri_chans[i].chan;
		r_timeout = (readl((void *)DDRC_CCHC(chan)) & DDRC_CCHC_RTR_TIMEOUT_MSK) >> DDRC_CCHC_RTR_TIMEOUT_BIT;
		printk("read timeout of chan%d %s : 0x%x\n", chan, pri_chans[i].desc, r_timeout);
	}

	return 0;
}

static ssize_t pri_rtr_timeout_write(struct file *file, const char __user *user_buf, size_t count, loff_t *ppos)
{
	char buf[16];
	char *val = buf;
	char *channel;
	unsigned int chan;
	unsigned int timeout;
	unsigned int cchc;

	if (copy_from_user(buf, user_buf, min(count, sizeof(buf) - 1)))
		return -EFAULT;

	channel = strsep(&val, ":");
	chan = simple_strtoul(channel, NULL, 10);
	if ((chan < 0) || (chan > 7)) {
		printk("invalid channel\n");
		return -EINVAL;
	}

	timeout = simple_strtoul(val, NULL, 10);
	if ((timeout < 0) || (timeout > 0xf)) {
		printk("invalid itimeout value\n");
		return -EINVAL;
	}

	cchc = readl((void *)DDRC_CCHC(chan));
	cchc &= ~DDRC_CCHC_RTR_TIMEOUT_MSK;
	cchc |= timeout << DDRC_CCHC_RTR_TIMEOUT_BIT;
	writel(cchc, (void *)DDRC_CCHC(chan));
	printk("set read timeout : chan%d  0x%x\n", chan, timeout);



	return count;
}

static const struct file_operations pri_rtr_timeout_fops = {
	.read	= pri_rtr_timeout_read,
	.write	= pri_rtr_timeout_write,
	.open	= simple_open,
	.llseek	= default_llseek,
};

static ssize_t pri_trqos_read(struct file *file, char __user *user_buf, size_t count, loff_t *ppos)
{
	int i;
	unsigned int chan;
	unsigned int qos;

	for (i = 0; i < ARRAY_SIZE(pri_chans); i++) {
		chan = pri_chans[i].chan;
		qos = (readl((void *)DDRC_CCHC(chan)) & DDRC_CCHC_TR_QOS_COMP_MSK) >> DDRC_CCHC_TR_QOS_COMP_BIT;
		printk("transaction qos  chan%d %s : transaction qos is %d\n", chan, pri_chans[i].desc, qos);
	}

	return 0;
}

static ssize_t pri_trqos_write(struct file *file, const char __user *user_buf, size_t count, loff_t *ppos)
{

	char buf[16];
	char *val = buf;
	char *channel;
	unsigned int chan;
	unsigned int qos;
	unsigned int cchc;

	if (copy_from_user(buf, user_buf, min(count, sizeof(buf) - 1)))
		return -EFAULT;

	channel = strsep(&val, ":");
	chan = simple_strtoul(channel, NULL, 10);
	if ((chan < 0) || (chan > 7)) {
		printk("invalid channel\n");
		return -EINVAL;
	}

	qos = simple_strtoul(val, NULL, 10);
	if ((qos < 0) || (qos > 3)) {
		printk("invalid qos\n");
		return -EINVAL;
	}

	cchc = readl((void *)DDRC_CCHC(chan));
	cchc &= ~DDRC_CCHC_TR_QOS_COMP_MSK;
	cchc |= qos << DDRC_CCHC_TR_QOS_COMP_BIT;
	writel(cchc, (void *)DDRC_CCHC(chan));
	printk("set transaction qos : chan%d  %d\n", chan, qos);

	return count;
}

static const struct file_operations pri_trqos_fops = {
	.read	= pri_trqos_read,
	.write	= pri_trqos_write,
	.open	= simple_open,
	.llseek	= default_llseek,
};

static ssize_t pri_bwlimit_w_read(struct file *file, char __user *user_buf, size_t count, loff_t *ppos)
{
	int i;
	unsigned int chan;
	unsigned int bw_en;
	unsigned int bw_pri;

	for (i = 0; i < ARRAY_SIZE(pri_chans); i++) {
		chan = pri_chans[i].chan;
		bw_en = readl((void *)DDRC_CCHC(chan)) & DDRC_CCHC_BW_LIMIT_WEN;
		if (bw_en) {
			bw_pri = (readl((void *)DDRC_CCHC(chan)) & DDRC_CCHC_BW_LIMIT_WCNT_MSK) >> DDRC_CCHC_BW_LIMIT_WCNT_BIT;
			printk("write channel bandwidth limit count of chan%d %s : %d\n", chan, pri_chans[i].desc, bw_pri);

		} else {
			printk("write channel bandwidth limit count has no effect on chan%d %s\n", chan, pri_chans[i].desc);
		}

	}

	return 0;
}

static ssize_t pri_bwlimit_w_write(struct file *file, const char __user *user_buf, size_t count, loff_t *ppos)
{
	char buf[64];
	char *val = buf;
	char *channel;
	unsigned int chan;
	unsigned int cnt;
	unsigned int cchc;

	if (copy_from_user(buf, user_buf, min(count, sizeof(buf) - 1)))
		return -EFAULT;

	channel = strsep(&val, ":");
	chan = simple_strtoul(channel, NULL, 10);
	if ((chan < 0) || (chan > 7)) {
		printk("invalid channel\n");
		return -EINVAL;
	}

	if (!strncmp(val, "disable", count-3)) {
		cchc = readl((void *)DDRC_CCHC(chan));
		cchc &= ~DDRC_CCHC_BW_LIMIT_WEN;
		cchc &= ~DDRC_CCHC_BW_LIMIT_WCNT_MSK;
		writel(cchc, (void *)DDRC_CCHC(chan));

		printk("chan%d disable write channel bandwidth limit\n", chan);

	} else {

		cnt = simple_strtoul(val, NULL, 10);
		if ((cnt < 0) || (cnt  > 0xff)) {
			printk("invalid bwlimit count\n");
			return -EINVAL;
		}

		cchc = readl((void *)DDRC_CCHC(chan));
		cchc |= DDRC_CCHC_BW_LIMIT_WEN;
		cchc &= ~DDRC_CCHC_BW_LIMIT_WCNT_MSK;
		cchc |= (cnt << DDRC_CCHC_BW_LIMIT_WCNT_BIT);
		writel(cchc, (void *)DDRC_CCHC(chan));

		printk("set write channel bandwidth limit : chan%d  count : %d\n", chan, cnt);
	}

	return count;
}

static const struct file_operations pri_bwlimit_w_fops = {
	.read	= pri_bwlimit_w_read,
	.write	= pri_bwlimit_w_write,
	.open	= simple_open,
	.llseek	= default_llseek,
};


static ssize_t pri_bwlimit_r_read(struct file *file, char __user *user_buf, size_t count, loff_t *ppos)
{
	int i;
	unsigned int chan;
	unsigned int bw_en;
	unsigned int bw_pri;

	for (i = 0; i < ARRAY_SIZE(pri_chans); i++) {
		chan = pri_chans[i].chan;
		bw_en = readl((void *)DDRC_CCHC(chan)) & DDRC_CCHC_BW_LIMIT_REN;
		if (bw_en) {
			bw_pri = (readl((void *)DDRC_CCHC(chan)) & DDRC_CCHC_BW_LIMIT_RCNT_MSK) >> DDRC_CCHC_BW_LIMIT_RCNT_BIT;
			printk("read channel bandwidth limit count of chan%d %s : %d\n", chan, pri_chans[i].desc, bw_pri);

		} else {
			printk("read channel bandwidth limit count has no effect on chan%d %s\n", chan, pri_chans[i].desc);
		}

	}

	return 0;
}

static ssize_t pri_bwlimit_r_write(struct file *file, const char __user *user_buf, size_t count, loff_t *ppos)
{
	char buf[64];
	char *val = buf;
	char *channel;
	unsigned int chan;
	unsigned int cnt;
	unsigned int cchc;

	if (copy_from_user(buf, user_buf, min(count, sizeof(buf) - 1)))
		return -EFAULT;

	channel = strsep(&val, ":");
	chan = simple_strtoul(channel, NULL, 10);
	if ((chan < 0) || (chan > 7)) {
		printk("invalid channel\n");
		return -EINVAL;
	}

	if (!strncmp(val, "disable", count-3)) {
		cchc = readl((void *)DDRC_CCHC(chan));
		cchc &= ~DDRC_CCHC_BW_LIMIT_REN;
		cchc &= ~DDRC_CCHC_BW_LIMIT_RCNT_MSK;
		writel(cchc, (void *)DDRC_CCHC(chan));

		printk("chan%d disable read channel bandwidth limit\n", chan);

	} else {

		cnt = simple_strtoul(val, NULL, 10);
		if ((cnt < 0) || (cnt  > 0xff)) {
			printk("invalid bwlimit count\n");
			return -EINVAL;
		}

		cchc = readl((void *)DDRC_CCHC(chan));
		cchc |= DDRC_CCHC_BW_LIMIT_REN;
		cchc &= ~DDRC_CCHC_BW_LIMIT_RCNT_MSK;
		cchc |= (cnt << DDRC_CCHC_BW_LIMIT_RCNT_BIT);
		writel(cchc, (void *)DDRC_CCHC(chan));

		printk("set read channel bandwidth limit : chan%d  count : %d\n", chan, cnt);
	}

	return count;
}

static const struct file_operations pri_bwlimit_r_fops = {
	.read	= pri_bwlimit_r_read,
	.write	= pri_bwlimit_r_write,
	.open	= simple_open,
	.llseek	= default_llseek,
};

static int __init ddr_priority_init(void)
{
	struct dentry * d;
	struct ddr_priority *pri = &ddr_priority;

	d = debugfs_create_dir("ddr_priority", NULL);
	if (IS_ERR(d)){
		pr_err("create debugfs for ddr_priority failed.\n");
		return PTR_ERR(d);
	}

	pri->root = d;

	d = debugfs_create_file("master", S_IWUSR | S_IRUGO, pri->root, pri, &pri_master_fops);
	if (IS_ERR_OR_NULL(d))
		goto err_node;

	d = debugfs_create_file("xport", S_IWUSR | S_IRUGO, pri->root, pri, &pri_xport_fops);
	if (IS_ERR_OR_NULL(d))
		goto err_node;

	d = debugfs_create_file("fix", S_IWUSR | S_IRUGO, pri->root, pri, &pri_fix_fops);
	if (IS_ERR_OR_NULL(d))
		goto err_node;

	d = debugfs_create_file("w_timeout", S_IWUSR | S_IRUGO, pri->root, pri, &pri_wtr_timeout_fops);
	if (IS_ERR_OR_NULL(d))
		goto err_node;

	d = debugfs_create_file("r_timeout", S_IWUSR | S_IRUGO, pri->root, pri, &pri_rtr_timeout_fops);
	if (IS_ERR_OR_NULL(d))
		goto err_node;

	d = debugfs_create_file("trans_qos", S_IWUSR | S_IRUGO, pri->root, pri, &pri_trqos_fops);
	if (IS_ERR_OR_NULL(d))
		goto err_node;

	d = debugfs_create_file("w_bw_limit", S_IWUSR | S_IRUGO, pri->root, pri, &pri_bwlimit_w_fops);
	if (IS_ERR_OR_NULL(d))
		goto err_node;

	d = debugfs_create_file("r_bw_limit", S_IWUSR | S_IRUGO, pri->root, pri, &pri_bwlimit_r_fops);
	if (IS_ERR_OR_NULL(d))
		goto err_node;


	return 0;
err_node:
	debugfs_remove_recursive(pri->root);

	return -1;
}

static void __exit ddr_priority_deinit(void)
{
	struct ddr_priority *pri = &ddr_priority;
	debugfs_remove_recursive(pri->root);
}

late_initcall(ddr_priority_init);
module_exit(ddr_priority_deinit);
