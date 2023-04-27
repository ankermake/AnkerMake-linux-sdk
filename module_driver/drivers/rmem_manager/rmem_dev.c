
#include <linux/version.h>
#include <linux/miscdevice.h>
#include <linux/ioctl.h>
#include <linux/module.h>
#include <linux/mutex.h>
#include <soc/base.h>
#include <linux/slab.h>
#include<linux/pid.h>
#include<linux/sched.h>
#include <linux/dmaengine.h>
#include <linux/vmalloc.h>
#include <linux/list.h>

#include "rmem_manager.h"

#define CMD_RMEM_ALLOC                _IOWR('R', 120, struct rmem_alloc_data *)
#define CMD_RMEM_FREE                 _IOWR('R', 121, struct rmem_alloc_data *)
#define CMD_RMEM_CACHE                _IOWR('W', 122, void *)

struct rmem_private_data {
    struct list_head alloc_list;
    struct mutex lock;
};

struct rmem_alloc_data {
    unsigned int size;
    void *mem;
};

struct rmem_alloc_data_list {
    struct list_head link;
    unsigned int size;
    void *mem;
};

enum rmem_cache_type {
    rmem_cache_dev_to_mem,
    rmem_cache_mem_to_dev,
};

static void add_alloc_data(struct rmem_private_data *pdata, void *mem, int size)
{
    struct rmem_alloc_data_list *node = vmalloc(sizeof(*node));

    node->size = size;
    node->mem = mem;

    mutex_lock(&pdata->lock);

    list_add_tail(&node->link, &pdata->alloc_list);

    mutex_unlock(&pdata->lock);
}

static int delete_alloc_data(struct rmem_private_data *pdata, void *mem)
{
    struct rmem_alloc_data_list *pos;
    int is_found = 0;

    mutex_lock(&pdata->lock);

    list_for_each_entry(pos, &pdata->alloc_list, link) {
        if(pos->mem == mem) {
            is_found = 1;
            break;
        }
    }

    if (is_found) {
        list_del(&pos->link);
        vfree(pos);
    } else {
        pr_warn("RMEM: can not find this mem = %p\n", mem);
    }

    mutex_unlock(&pdata->lock);
    return is_found;
}

static int rmem_open(struct inode *inode, struct file *filp)
{
    struct rmem_private_data *pdata = vmalloc(sizeof(*pdata));

    INIT_LIST_HEAD(&pdata->alloc_list);

    mutex_init(&pdata->lock);

    filp->private_data = pdata;
    return 0;
}

static int rmem_release(struct inode *inode, struct file *filp)
{
    struct rmem_private_data *pdata = filp->private_data;
    struct rmem_alloc_data_list *pos, *next;

    mutex_lock(&pdata->lock);

    list_for_each_entry_safe(pos, next, &pdata->alloc_list, link) {
        pr_warn("RMEM: source not free, pid = %d, name = %s, mem = %p, size = %d\n",
                   task_pid_nr(current), current->comm, pos->mem, pos->size);

        rmem_free(pos->mem, pos->size);

        list_del(&pos->link);
        vfree(pos);
    };

    mutex_unlock(&pdata->lock);

    vfree(pdata);

    return 0;
}

static long rmem_ioctl(struct file *filp, unsigned int cmd, unsigned long arg)
{
    int ret = 0;
    unsigned int size;
    void *mem;

    struct rmem_alloc_data *data;
    data = (struct rmem_alloc_data *)arg;

    struct rmem_private_data *pdata = filp->private_data;

    switch (cmd) {
    case CMD_RMEM_ALLOC:
        size = ALIGN(data->size, PAGE_SIZE);
        mem = rmem_alloc_aligned(size, PAGE_SIZE);
        if (!mem) {
            printk(KERN_ERR "RMEM: alloc memery err\n");
            ret = -1;
        }

        data->mem = (void *)virt_to_phys(mem);

        add_alloc_data(pdata, mem, size);
        break;

    case CMD_RMEM_FREE:
        mem = (void *)CKSEG0ADDR(data->mem);
        size = ALIGN(data->size, PAGE_SIZE);

        if (delete_alloc_data(pdata, mem))
            rmem_free(mem,size);

        break;

    case CMD_RMEM_CACHE: {
        unsigned long *array = (void *)arg;
        void *mem = (void *)array[0];
        int size = array[1];
        enum rmem_cache_type type = array[2];
#if LINUX_VERSION_CODE >= KERNEL_VERSION(5,10, 0)
        if (type == rmem_cache_dev_to_mem)
            dma_cache_inv((unsigned long)mem, size);
        else
            dma_cache_wback((unsigned long)mem, size);
#else
        if (type == rmem_cache_dev_to_mem)
            dma_cache_sync(NULL, mem, size, DMA_DEV_TO_MEM);
        else
            dma_cache_sync(NULL, mem, size, DMA_MEM_TO_DEV);
#endif
        break;
    }

    default:
        printk(KERN_ERR "RMEM: not support this cmd:%d\n", cmd);
        ret = -1;
        break;
    }

    return ret;
}

int rmem_mmap(struct file *file, struct vm_area_struct *vma)
{
    size_t size = vma->vm_end - vma->vm_start;
    phys_addr_t offset = (phys_addr_t)vma->vm_pgoff << PAGE_SHIFT;

    if (offset + (phys_addr_t)size - 1 < offset)
        return -EINVAL;

    vma->vm_pgoff = offset >> PAGE_SHIFT;

    vma->vm_flags |= VM_IO;
    pgprot_val(vma->vm_page_prot) &= ~_CACHE_MASK;
    pgprot_val(vma->vm_page_prot) |= 0;

    if (io_remap_pfn_range(vma,
                vma->vm_start,
                vma->vm_pgoff,
                size,
                vma->vm_page_prot)) {
        return -EAGAIN;
    }
    return 0;
}


static struct file_operations rmem_misc_fops = {
    .open = rmem_open,
    .release = rmem_release,
    .unlocked_ioctl = rmem_ioctl,
    .mmap = rmem_mmap,
};

static struct miscdevice rmem_mdev = {
    .minor = MISC_DYNAMIC_MINOR,
    .name = "rmem_manager",
    .fops = &rmem_misc_fops,
};

void rmem_dev_register(void)
{
    int ret;
    ret = misc_register(&rmem_mdev);
    BUG_ON(ret < 0);
}

void rmem_dev_unregister(void)
{
    misc_deregister(&rmem_mdev);
}
