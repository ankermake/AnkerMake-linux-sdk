#include <linux/list.h>
#include <linux/spinlock.h>
#include <linux/fs.h>

#define BUFFER_SIZE (1536)
#define BUFFER_CNT (16)

struct db_node {
	struct list_head node;
	char *buffer;
	size_t size;
	int pos;
};

static struct db_node db_nodes[BUFFER_CNT];
static LIST_HEAD(full_head);
static LIST_HEAD(free_head);
static DEFINE_SPINLOCK(list_lock);
static volatile int db_opened = false;
static DECLARE_WAIT_QUEUE_HEAD(db_waitq);


static int db_open (struct inode *inode, struct file *file)
{
	struct db_node *pn;
	int d = 0;
	spin_lock_bh(&list_lock);
	list_for_each_entry(pn, &free_head, node)
		d++;
	printk("before free = %d\n", d);
	d = 0;
	list_for_each_entry(pn, &full_head, node) {
		pn->size = 0;
		pn->pos = 0;
		d++;
	}
	printk("before used = %d\n", d);
	d = 0;
	list_splice_tail_init(&full_head, &free_head);
	list_for_each_entry(pn, &free_head, node)
		d++;
	printk("now free = %d\n", d);
	db_opened = true;
	spin_unlock_bh(&list_lock);
	return 0;
}

static ssize_t db_read (struct file *filp, char __user *buf, size_t length, loff_t *loff)
{
	struct db_node *pn;
	int count = length;
	int size, pos = 0;

	while (count) {
		spin_lock_bh(&list_lock);
		if (!list_empty(&full_head)) {
			pn = list_first_entry(&full_head, struct db_node, node);
			list_del(&pn->node);
			spin_unlock_bh(&list_lock);
		} else {
			spin_unlock_bh(&list_lock);
			if (!(filp->f_flags & O_NONBLOCK)) {
				wait_event_interruptible(db_waitq, !list_empty_careful(&full_head));
				continue;
			} else
				break;
		}
		if (pn->size < count)
			size = pn->size;
		else
			size = count;
		copy_to_user(buf + pos, pn->buffer + pn->pos, size);
		count -= size;
		pos += size;
		pn->pos += size;
		pn->pos %= BUFFER_SIZE;
		pn->size -= size;

		//printk(KERN_DEBUG"read [%x]: pn->size = %x\n",(unsigned)pn, pn->size);
		spin_lock_bh(&list_lock);
		if (!pn->size) {
			pn->pos = 0;
			list_add_tail(&pn->node, &free_head);
		} else {
			list_add(&pn->node, &full_head);
		}
		spin_unlock_bh(&list_lock);
	}
	return length - count;
}

static unsigned int db_poll(struct file *file, struct poll_table_struct *wait)
{
	unsigned int mask = 0;

	poll_wait(file, &db_waitq, wait);

	if (list_empty_careful(&full_head)) {
		mask = POLLIN | POLLRDNORM;
	}

	return mask;
}

static int db_release (struct inode *inode, struct file *file)
{
	db_opened = false;
	return 0;
}

static struct file_operations db_ops = {
	.open = db_open,
	.read = db_read,
	.poll = db_poll,
	.release = db_release,
};

void send_data_back_to_user(char *addr, size_t length)
{
	struct db_node *pn;
	int count = length;
	int size, pos = 0;
	char *buf = addr;


	if (!db_opened)
		return;

	while (count) {
		spin_lock(&list_lock);
		if (!list_empty(&free_head))
			pn = list_first_entry(&free_head, struct db_node, node);
		else {	/*overrun*/
			WARN(list_empty(&full_head), "No node ???\n");
			pn = list_first_entry(&full_head, struct db_node, node);
			pn->pos = 0;
			pn->size = 0;
			printk(KERN_DEBUG"###underrun happen!!\n");
		}
		list_del(&pn->node);
		spin_unlock(&list_lock);

		if (count > (BUFFER_SIZE - pn->pos))
			size = BUFFER_SIZE - pn->pos;
		else
			size = count;

		memcpy(pn->buffer + pn->pos, buf + pos, size);

		count -= size;
		pos += size;
		pn->pos += size;
		pn->pos %= BUFFER_SIZE;
		pn->size += size;
		//printk(KERN_DEBUG"[%x]pn->pos = %d, pn->size = %d %d", (unsigned)pn, pn->pos, pn->size, BUFFER_SIZE);

		spin_lock(&list_lock);
		if (pn->size == BUFFER_SIZE) {
			if (list_empty(&full_head)) {
				list_add_tail(&pn->node, &full_head);
				wake_up_interruptible(&db_waitq);
			} else
				list_add_tail(&pn->node, &full_head);
		} else {
			list_add(&pn->node, &free_head);
		}
		spin_unlock(&list_lock);
	}
	return;
}

int send_data_back_to_user_init(void)
{
	int i;
	int major;
	dev_t devno;
	struct class *cls;
	struct device *dev;

	for (i = 0; i < BUFFER_CNT; i++) {
		db_nodes[i].buffer = kmalloc(BUFFER_SIZE, GFP_KERNEL);
		INIT_LIST_HEAD(&db_nodes[i].node);
		db_nodes[i].size = 0;
		db_nodes[i].pos = 0;;
		list_add(&db_nodes[i].node, &free_head);
	}

	cls = class_create(THIS_MODULE, "audio-class");
	if (IS_ERR(cls))
		return PTR_ERR(cls);

	major = register_chrdev(0, "iaudio", &db_ops);
	if (major < 0)
		return major;

	devno = MKDEV(major, 0);
	dev = device_create(cls, NULL, devno, NULL, "iaudio");
	if (IS_ERR(dev))
		return PTR_ERR(dev);

	printk("=====> %s %d\n", __func__, __LINE__);
	return 0;
}
