#include <linux/init.h>
#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/io.h>
#include <linux/device.h>
#include <linux/ctype.h>
#include <linux/fs.h>
#include <asm/device.h>
#include <linux/dma-mapping.h>
#include <linux/platform_device.h>
#include <common.h>
#include <linux/string.h>
#include <linux/list.h>
#include <linux/mutex.h>
#include <linux/of_device.h>
#include <linux/of_reserved_mem.h>
#include <linux/of.h>

#include "rmem_manager.h"
#include "rmem_dev.c"

#define RMEM_PAGE_SIZE 4096

static unsigned long rmem_start = 0;
module_param(rmem_start, ulong, 0644);

static unsigned int rmem_size = 0;
module_param(rmem_size, uint, 0644);

static LIST_HEAD(free_list);
static LIST_HEAD(alloc_list);
static LIST_HEAD(index_list);
static DEFINE_MUTEX(mutex);


static struct device *rmem_src_dev = NULL;

struct rmem {
    struct list_head link;
    unsigned long alloc_addr;
    unsigned long aligned_addr;
    int size;
};

static inline void *m_dma_alloc_coherent(int size)
{
    dma_addr_t dma_handle;
    void *mem = dma_alloc_coherent(rmem_src_dev, size, &dma_handle, GFP_KERNEL);
    if (!mem)
        return NULL;

    return (void *)CKSEG0ADDR(mem);
}

static inline void m_dma_free_coherent(void *mem, int size)
{
    dma_addr_t dma_handle = virt_to_phys(mem);
    dma_free_coherent(rmem_src_dev, size, (void *)CKSEG1ADDR(mem), dma_handle);
}

static void rmem_add_index(struct rmem *rmem)
{
    rmem->alloc_addr = 0;
    rmem->aligned_addr = 0;
    rmem->size = 0;

    list_add_tail(&rmem->link, &index_list);
}

static struct rmem *rmem_get_index(void)
{
    struct rmem *rmem = list_first_entry(&index_list, struct rmem, link);
    if(!rmem)
        printk(KERN_ERR "rmem: index memory is full\n");

    return rmem;
}

void rmem_add_free(struct rmem *rmem)
{
    struct rmem *temp;
    struct list_head *pos;
    list_for_each(pos, &free_list) {
        temp = list_entry(pos, struct rmem, link);

        /*找到rmem 后面的第一个节点（temp）*/
        if (temp->alloc_addr > rmem->alloc_addr) {

            struct rmem *temp_prev = list_entry(temp->link.prev, struct rmem, link);

            /*先判断rmem ---- temp是否相连*/
            if (rmem->alloc_addr + rmem->size == temp->alloc_addr) {
                list_del(&temp->link);
                rmem->size += temp->size;
                rmem_add_index(temp);
            }

            /*判断temp前一个节点是否为头，是：添加到头后面就退出，否继续*/
            if (!temp_prev) {
                list_add(&rmem->link, &free_list);
                return;
            }

            /*判断temp_prev ----- rmem是否相连*/
            if (temp_prev->alloc_addr + temp_prev->size == rmem->alloc_addr) {
                temp_prev->size += rmem->size;
                rmem_add_index(rmem);
            } else {
                list_add(&rmem->link, &temp_prev->link);
            }

            return;
        }

        /*rmem 后面没有节点了*/
        if (list_is_last(pos, &free_list)) {
            if (temp->alloc_addr + temp->size == rmem->alloc_addr) {
                temp->size += rmem->size;
                rmem_add_index(rmem);
                return;
            }
        }
    }

    list_add_tail(&rmem->link, &free_list);
}

void *rmem_alloc_aligned(int size, int align)
{
    int need_size;
    int is_found = 0;
    void *ret;
    struct rmem *rmem;
    unsigned long aligned_addr;

    if (size <= 0)
        return NULL;

    if (align < 4)
        align = 4;

    if (align & (align - 1))
        return NULL;

    mutex_lock(&mutex);

    list_for_each_entry(rmem, &free_list, link) {
        aligned_addr = (unsigned long)ALIGN(rmem->alloc_addr, align);
        need_size = size + (aligned_addr - rmem->alloc_addr);

        if(rmem->size >= need_size) {
            is_found = 1;
            break;
        }
    }

    if (!is_found) {
        ret = m_dma_alloc_coherent(size);
        goto unlock;
    }

    rmem->aligned_addr = aligned_addr;
    if(rmem->size - need_size == 0 || rmem->size - need_size < 4) {
        list_del(&rmem->link);
        list_add_tail(&rmem->link, &alloc_list);
        ret = (void *)rmem->aligned_addr;
        goto unlock;
    }

    struct rmem *rmem_new = rmem_get_index();
    if (!rmem) {
        ret = NULL;
        goto unlock;
    }

    list_del(&rmem_new->link);

    list_del(&rmem->link);

    rmem_new->alloc_addr = rmem->alloc_addr + need_size;
    rmem_new->size = rmem->size - need_size;
    rmem_add_free(rmem_new);

    rmem->aligned_addr = aligned_addr;
    rmem->size = need_size;
    list_add_tail(&rmem->link, &alloc_list);

    ret = (void *)rmem->aligned_addr;

unlock:
    mutex_unlock(&mutex);
    return ret;
}
EXPORT_SYMBOL(rmem_alloc_aligned);

void *rmem_alloc(int size)
{
    return rmem_alloc_aligned(size, 4);
}
EXPORT_SYMBOL(rmem_alloc);

void rmem_free(void *ptr, int size)
{
    int is_found = 0;
    unsigned long aligned_addr = (unsigned long)ptr;
    struct rmem *alloc;

    unsigned long start = (unsigned long)CKSEG0ADDR(rmem_start);
    unsigned long end = start + rmem_size;

    if (aligned_addr < start || aligned_addr > end) {
        m_dma_free_coherent(ptr, size);
        return;
    }

    mutex_lock(&mutex);

    list_for_each_entry(alloc, &alloc_list, link) {
        if (alloc->aligned_addr == aligned_addr) {
            is_found = 1;
            break;
        }
    }

    if (!is_found) {
        printk(KERN_ERR "rmem: this addr: %p not rmem_alloc\n", ptr);
        mutex_unlock(&mutex);
        return;
    }

    list_del(&alloc->link);

    rmem_add_free(alloc);

    mutex_unlock(&mutex);

    return;
}
EXPORT_SYMBOL(rmem_free);

void rmem_init(void)
{
    if (!rmem_size) {
        printk(KERN_INFO "rmem:no remem set, use kmalloc\n");
        return;
    }

    int i;
    int num;
    struct rmem *new;
    struct rmem *rmem = kmalloc(RMEM_PAGE_SIZE, GFP_KERNEL);

    rmem->alloc_addr = CKSEG0ADDR(rmem_start);
    rmem->size = rmem_size;
    rmem->aligned_addr = rmem->alloc_addr;

    list_add_tail(&rmem->link, &free_list);

    num = RMEM_PAGE_SIZE / sizeof(struct rmem);

    for (i = 1; i < num; i++) {
        new = rmem + i;
        rmem_add_index(new);
    }

    return;
}

static int rmem_src_probe(struct platform_device *pdev)
{
    int ret;
    ret = of_reserved_mem_device_init(&pdev->dev);
    if (ret) {
        dev_warn(&pdev->dev, "failed to init reserved mem\n");
        return ret;
    }

    rmem_src_dev = &pdev->dev;

    return 0;
}


static int rmem_src_remove(struct platform_device *pdev)
{
    return 0;
}

static const struct of_device_id rmem_src_of_match[] = {
    { .compatible = "ingenic,md-rmem-src"},
    {},
};


static struct platform_driver rmem_src_driver = {
    .probe = rmem_src_probe,
    .remove = rmem_src_remove,
    .driver = {
        .name = "rmem_src",
        .of_match_table = rmem_src_of_match,
    },
};

static int rmem_manager_probe(struct platform_device *pdev)
{
    rmem_src_dev = &pdev->dev;

    rmem_init();

    rmem_dev_register();

    return 0;
}

static int rmem_manager_remove(struct platform_device *pdev)
{
    if (rmem_size) {
        struct rmem *first = list_first_entry(&free_list, struct rmem, link);
        assert(first->size == rmem_size);
        kfree(first);
    }

    rmem_dev_unregister();
    return 0;
}

static struct platform_driver rmem_manager_driver = {
    .probe = rmem_manager_probe,
    .remove = rmem_manager_remove,
    .driver = {
        .owner = THIS_MODULE,
        .name = "rmem-manager",
    },
};

/* stop no dev release warning */
static void jz_hash_dev_release(struct device *dev){}

struct platform_device rmem_manager_device = {
    .name = "rmem-manager",
    .dev  = {
        .release = jz_hash_dev_release,
    },
};

static int __init rmem_manager_init(void)
{
    int ret = platform_device_register(&rmem_manager_device);
    if (ret)
        return ret;

    ret = platform_driver_register(&rmem_manager_driver);
    if (ret)
        return ret;

    return  platform_driver_register(&rmem_src_driver);
}
module_init(rmem_manager_init);

static void __exit rmem_manager_exit(void)
{
    platform_driver_unregister(&rmem_src_driver);

    platform_device_unregister(&rmem_manager_device);

    platform_driver_unregister(&rmem_manager_driver);
}
module_exit(rmem_manager_exit);

MODULE_DESCRIPTION("Ingenic rmem_manager driver");
MODULE_LICENSE("GPL");