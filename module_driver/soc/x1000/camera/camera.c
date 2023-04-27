
#include <common.h>
#include <linux/list.h>
#include <linux/clk.h>
#include <linux/miscdevice.h>
#include <linux/gpio.h>
#include <linux/module.h>
#include <linux/ioctl.h>
#include <linux/fs.h>

#include <soc/base.h>
#include <soc/irq.h>
#include <linux/interrupt.h>
#include <linux/irq.h>
#include <linux/kernel.h>
#include <linux/slab.h>

#include <linux/spinlock.h>
#include <linux/mutex.h>
#include <linux/wait.h>
#include <linux/io.h>
#include <linux/dma-mapping.h>
#include <linux/string.h>
#include <linux/delay.h>
#include <linux/sched.h>
#include <bit_field.h>

#include "cim_sensor.h"
#include "cim_regs.h"
#include "cim_gpio.c"

#define DESC_COUNT 2

#define CIM_ADDR(reg)    ((volatile unsigned long *)CKSEG1ADDR(CIM_IOBASE + reg))

static inline void cim_write_reg(unsigned int reg, int val)
{
    *CIM_ADDR(reg) = val;
}

static inline unsigned int cim_read_reg(unsigned int reg)
{
    return *CIM_ADDR(reg);
}

static inline void cim_set_bit(unsigned int reg, int start, int end, unsigned int val)
{
    set_bit_field(CIM_ADDR(reg), start, end, val);
}

static inline unsigned int cim_get_bit(unsigned int reg, int start, int end)
{
    return get_bit_field(CIM_ADDR(reg), start, end);
}

struct frame_desc {
    unsigned long next_desc_addr;
    unsigned long frame_id;
    unsigned long frame_addr;
    unsigned long cim_cmd;
};

struct camera_device {
    struct cim_sensor_config *sensor;
    unsigned char is_power_on;
    unsigned char is_stream_on;
};

static DEFINE_MUTEX(lock);
static DEFINE_SPINLOCK(spinlock);

static struct camera_device m_camera;


enum frm_data_status {
    status_free,
    status_trans,
    status_usable,
    status_user,
};

struct frm_data {
    struct list_head link;
    void *address;
    enum frm_data_status status;
};

struct cim_data {
    struct clk *clk;
    struct clk *mclk;
    wait_queue_head_t waiter;

    void *mem;
    unsigned long frm_size;
    unsigned int mem_cnt;
    struct frame_desc *descs;
    struct frm_data *frms;

    struct frm_data **trans_list;
    struct list_head free_list;
    struct list_head usable_list;
    struct list_head user_list;
    unsigned int frm_cnt;
    unsigned int free_frm_cnt;
    unsigned int frm_stoped;
};

static struct cim_data cim_data;

static int m_mem_cnt;
module_param_named(frame_nums, m_mem_cnt, int, 0644);

static unsigned long rmem_start = 0;
module_param(rmem_start, ulong, 0644);

static unsigned int rmem_size = 0;
module_param(rmem_size, uint, 0644);

static unsigned int enable_debug = 0;
module_param(enable_debug, uint, 0644);

#define debug(x...) \
    do { \
        if (enable_debug) \
            printk(x); \
    } while (0)

static void write_dma_desc(int id)
{
    cim_write_reg(CIMDA, virt_to_phys(&cim_data.descs[id]));
}

static void set_desc_addr(int id, struct frm_data *frm)
{
    struct frame_desc *desc = &cim_data.descs[id];

    desc->frame_addr = virt_to_phys(frm->address);
    dma_cache_sync(NULL, desc, sizeof(*desc), DMA_TO_DEVICE);
}

static void add_to_free_list(struct frm_data *frm)
{
    frm->status = status_free;
    cim_data.free_frm_cnt++;
    list_add_tail(&frm->link, &cim_data.free_list);
}

static struct frm_data *get_free_frm(void)
{
    if (list_empty(&cim_data.free_list))
        return NULL;

    cim_data.free_frm_cnt--;
    struct frm_data *frm = list_first_entry(&cim_data.free_list, struct frm_data, link);
    list_del(&frm->link);
    return frm;
}

static void add_to_usable_list(struct frm_data *frm)
{
    frm->status = status_usable;
    cim_data.frm_cnt++;
    list_add_tail(&frm->link, &cim_data.usable_list);
}

static struct frm_data *get_usable_frm(void)
{
    if (list_empty(&cim_data.usable_list))
        return NULL;

    cim_data.frm_cnt--;
    struct frm_data *frm = list_first_entry(&cim_data.usable_list, struct frm_data, link);
    list_del(&frm->link);
    return frm;
}

static void set_trans_frm(int index, struct frm_data *frm)
{
    frm->status = status_trans;
    cim_data.trans_list[index] = frm;
    set_desc_addr(index, frm);
}

static void init_frm_lists(void)
{
    INIT_LIST_HEAD(&cim_data.free_list);
    INIT_LIST_HEAD(&cim_data.usable_list);

    cim_data.free_frm_cnt = 0;
    cim_data.frm_cnt = 0;
    cim_data.frm_stoped = 0;

    int i;
    for (i = 0; i < cim_data.mem_cnt; i++) {
        struct frm_data *frm = &cim_data.frms[i];
        frm->address = cim_data.mem + i * cim_data.frm_size;
        add_to_free_list(frm);
    }
}

static void reset_frm_lists(void)
{
    cim_data.free_frm_cnt = 0;
    cim_data.frm_cnt = 0;
    cim_data.frm_stoped = 0;

    int i;
    for (i = 0; i < cim_data.mem_cnt; i++) {
        struct frm_data *frm = &cim_data.frms[i];
        frm->address = cim_data.mem + i * cim_data.frm_size;
        if (frm->status != status_trans)
            list_del(&frm->link);
        add_to_free_list(frm);
    }
}

static void cim_init_setting(struct cim_sensor_config *cfg)
{
    cim_write_reg(CIMST, 0);

    unsigned long cimcfg = 0;
    set_bit_field(&cimcfg, EEOFEN, 0);
    set_bit_field(&cimcfg, DUMMY, 0);
    set_bit_field(&cimcfg, INV_DAT, 0);
    set_bit_field(&cimcfg, PACK, 4); // 设置为4, BS0...BS3生效
    set_bit_field(&cimcfg, BURST_TYPE, 3); // 64byte burst
    set_bit_field(&cimcfg, BS0, cfg->index_byte0);
    set_bit_field(&cimcfg, BS1, cfg->index_byte1);
    set_bit_field(&cimcfg, BS2, cfg->index_byte2);
    set_bit_field(&cimcfg, BS3, cfg->index_byte3);
    set_bit_field(&cimcfg, VSP, cfg->vsync_polarity);
    set_bit_field(&cimcfg, HSP, cfg->hsync_polarity);
    set_bit_field(&cimcfg, PCP, cfg->data_sample_edge);
    set_bit_field(&cimcfg, DSM, cfg->cim_interface);
    cim_write_reg(CIMCFG, cimcfg);

    unsigned long cimcr2 = 0;
    set_bit_field(&cimcr2, FSC, 1); // 使能帧大小检查
    set_bit_field(&cimcr2, ARIF, 1); // 如果帧检查出错，那么自动恢复
    set_bit_field(&cimcr2, OP, 0);
    set_bit_field(&cimcr2, OPE, 0);
    set_bit_field(&cimcr2, APM, 1); // 硬件自动判断dma总线传输优先级
    cim_write_reg(CIMCR2, cimcr2);

    unsigned long cimimr = 0xffffffff;
    set_bit_field(&cimimr, DEOFM, 0);
    set_bit_field(&cimimr, FSEM, 0);
    set_bit_field(&cimimr, DSTPM, 0);
    // set_bit_field(&cimimr, RFOFM, 0);
    cim_write_reg(CIMIMR, cimimr);

    unsigned long cimfs = 0;
    set_bit_field(&cimfs, FVS, cfg->sensor_info.height - 1);
    set_bit_field(&cimfs, FHS, cfg->sensor_info.width - 1);
    if (sensor_fmt_is_YUV422(cfg->sensor_info.fmt))
        set_bit_field(&cimfs, BPP, 1);
    else
        set_bit_field(&cimfs, BPP, 0);
    cim_write_reg(CIMFS, cimfs);

    int i;
    for (i = 0; i < 2; i++) {
        struct frm_data *frm;
        frm = get_free_frm();
        set_trans_frm(i, frm);
    }

    write_dma_desc(0);

    cim_set_bit(CIMCR, RF_RST, 1);
    cim_set_bit(CIMCR, RF_RST, 0);

    unsigned long cimcr = 0;
    set_bit_field(&cimcr, EEOF_LINE, 0);
    set_bit_field(&cimcr, FRC, 0); // 每帧数据都采样
    set_bit_field(&cimcr, WINE, 0); // 暂不使用窗口功能
    set_bit_field(&cimcr, FRM_ALIGN, 1);
    set_bit_field(&cimcr, DMA_SYNC, 1);
    set_bit_field(&cimcr, STP_REQ, 0);
    set_bit_field(&cimcr, SW_RST, 0);
    set_bit_field(&cimcr, DMA_EN, 1); // 开启dma
    set_bit_field(&cimcr, ENA, 1); // 开启cim
    cim_write_reg(CIMCR, cimcr);
}

static void cim_stream_on(struct cim_sensor_config *cfg)
{
    clk_enable(cim_data.clk);

    reset_frm_lists();

    cim_init_setting(cfg);

    enable_irq(IRQ_CIM);
}

static void cim_stream_off(void)
{
    disable_irq(IRQ_CIM);

    unsigned long cimcr = cim_read_reg(CIMCR);
    set_bit_field(&cimcr, SW_RST, 1); // reset rxfifo
    set_bit_field(&cimcr, DMA_EN, 0); // 开启dma
    set_bit_field(&cimcr, ENA, 0); // 开启cim
    cim_write_reg(CIMCR, cimcr);

    cim_set_bit(CIMCR, RF_RST, 0);

    clk_disable(cim_data.clk);
}

static void init_frame_desc(struct frame_desc *desc, int id, void *addr, unsigned int len, struct frame_desc *next)
{
    unsigned long cim_cmd = 0;
    set_bit_field(&cim_cmd, SOFINTE, 0);
    set_bit_field(&cim_cmd, EOFINTE, 1);
    set_bit_field(&cim_cmd, STOP, 0);
    set_bit_field(&cim_cmd, OFAR, 1);
    set_bit_field(&cim_cmd, LEN, len / 4);
    desc->frame_id = id;
    desc->frame_addr = virt_to_phys(addr);
    desc->next_desc_addr = virt_to_phys(next);
    desc->cim_cmd = cim_cmd;
}

void cim_dump_status(const char *tag)
{
    printk(KERN_ERR "cim: %s iid:%x da:%x fid:%x fdr:%x\n",
            tag,
            cim_read_reg(CIMIID),
            cim_read_reg(CIMDA),
            cim_read_reg(CIMFID),
            cim_read_reg(CIMFA));
}

struct frm_data *try_get_free_frm(void)
{
    struct frm_data *frm;

    frm = get_free_frm();
    if (frm)
        return frm;

    if (cim_data.frm_cnt <= 1)
        return NULL;

    return get_usable_frm();
}

static irqreturn_t cim_irq_handler(int irq, void *data)
{
    struct frm_data *frm;
    int index, id;
    unsigned long state = cim_read_reg(CIMST);

    if (get_bit_field(&state, DEOF)) {
        // cim_dump_status("DEOF");
        id = cim_read_reg(CIMIID);
        assert(id < cim_data.mem_cnt);

        debug("id: %d %d %d\n", id, cim_data.free_frm_cnt,
                                cim_data.frm_cnt);

        if (cim_data.trans_list[0] != cim_data.trans_list[1]) {
            index = !id;
            frm = cim_data.trans_list[index];
            add_to_usable_list(frm);

            frm = try_get_free_frm();
            if (!frm)
                frm = cim_data.trans_list[id];

            set_trans_frm(index, frm);
        }

        cim_set_bit(CIMST, DEOF, 0);

        wake_up_all(&cim_data.waiter);

        return IRQ_HANDLED;
    }

    if (get_bit_field(&state, DSTOP)) {
        // cim_dump_status("DSTOP");

        cim_set_bit(CIMST, DSTOP, 0);

        return IRQ_HANDLED;
    }

    if (get_bit_field(&state, FSE)) {
        printk(KERN_ERR "cim err: %lx\n", state);
        cim_set_bit(CIMST, FSE, 0);
        return IRQ_HANDLED;
    }

    printk(KERN_ERR "cim err: %lx\n", state);
    cim_write_reg(CIMST, 0);

    return IRQ_HANDLED;
}

void cim_enable_sensor_mclk(unsigned long clk_rate)
{
    clk_set_rate(cim_data.mclk, clk_rate);
    clk_enable(cim_data.mclk);
}

void cim_disable_sensor_mclk(void)
{
    clk_disable(cim_data.mclk);
    assert(!clk_is_enabled(cim_data.mclk));
}

static int rmem_is_used;

static inline void *m_dma_alloc_coherent(int size)
{
    if (!rmem_is_used && rmem_size >= size && rmem_start) {
        rmem_is_used = 1;
        return (void *)CKSEG0ADDR(rmem_start);
    }

    dma_addr_t dma_handle;
    void *mem = dma_alloc_coherent(NULL, size, &dma_handle, GFP_KERNEL);
    if (!mem)
        return mem;

    return (void *)CKSEG0ADDR(mem);
}

static inline void m_dma_free_coherent(void *mem, int size)
{
    dma_addr_t dma_handle = virt_to_phys(mem);
    if (dma_handle != rmem_start)
        dma_free_coherent(NULL, size, (void *)CKSEG1ADDR(mem), dma_handle);
    else
        rmem_is_used = 0;
}

static int cim_alloc_mem(struct cim_sensor_config *cfg)
{
    unsigned int bytes_pixel = sensor_fmt_is_YUV422(cfg->sensor_info.fmt) ? 2 : 1;
    unsigned int real_frm_size = cfg->sensor_info.width * cfg->sensor_info.height * bytes_pixel;
    unsigned int frame_align_size = ALIGN(real_frm_size, PAGE_SIZE);

    unsigned int mem_cnt = m_mem_cnt;

    assert(mem_cnt >= 1);

    void *mem = m_dma_alloc_coherent(mem_cnt * frame_align_size);
    if (mem == NULL) {
        printk(KERN_ERR "camera: failed to alloc mem: %u\n", frame_align_size * mem_cnt);
        return -ENOMEM;
    }

    cim_data.mem = mem;
    cim_data.mem_cnt = mem_cnt;
    cim_data.descs = m_dma_alloc_coherent(DESC_COUNT * sizeof(cim_data.descs[0]));
    assert(cim_data.descs);

    cim_data.frms = kmalloc(mem_cnt * sizeof(cim_data.frms[0]), GFP_KERNEL);
    assert(cim_data.frms);

    cim_data.trans_list = kmalloc(DESC_COUNT * sizeof(cim_data.trans_list[0]), GFP_KERNEL);
    assert(cim_data.trans_list);

    cim_data.frm_size = frame_align_size;
    cfg->info.fps = (cfg->sensor_info.fps >> 16) / (cfg->sensor_info.fps & 0xFFFF);
    cfg->info.frame_size = real_frm_size;
    cfg->info.frame_align_size = frame_align_size;
    cfg->info.frame_nums = mem_cnt;
    cfg->info.line_length = bytes_pixel * cfg->info.width;
    cfg->info.phys_mem = virt_to_phys(cim_data.mem);

    init_frame_desc(&cim_data.descs[0], 0, mem, real_frm_size, &cim_data.descs[1]);
    init_frame_desc(&cim_data.descs[1], 1, mem, real_frm_size, &cim_data.descs[0]);

    dma_cache_sync(NULL, cim_data.descs, sizeof(cim_data.descs[0]) * DESC_COUNT, DMA_TO_DEVICE);

    init_frm_lists();

    return 0;
}

static void cim_free_mem(void)
{
    m_dma_free_coherent(cim_data.mem, cim_data.frm_size * cim_data.mem_cnt);
    m_dma_free_coherent(cim_data.descs, sizeof(cim_data.descs[0]) * DESC_COUNT);
    kfree(cim_data.frms);
    kfree(cim_data.trans_list);
    cim_data.mem = NULL;
    cim_data.frms = NULL;
    cim_data.descs = NULL;
}

static int camera_power_on(void)
{
    int ret = 0;

    mutex_lock(&lock);

    if (!m_camera.is_power_on) {
        ret = m_camera.sensor->ops.power_on();
        if (!ret)
            m_camera.is_power_on = 1;
    }

    mutex_unlock(&lock);

    return ret;
}

static void camera_power_off(void)
{
    mutex_lock(&lock);

    if (m_camera.is_stream_on) {
        cim_stream_off();
        m_camera.sensor->ops.stream_off();
        m_camera.is_stream_on = 0;
    }

    if (m_camera.is_power_on) {
        m_camera.sensor->ops.power_off();
        m_camera.is_power_on = 0;
    }

    mutex_unlock(&lock);
}

static int camera_stream_on(void)
{
    int ret = 0;

    mutex_lock(&lock);

    if (!m_camera.is_power_on) {
        printk(KERN_ERR "camera can't stream on when not power on\n");
        ret = -EINVAL;
        goto unlock;
    }

    if (m_camera.is_stream_on)
        goto unlock;

    ret = m_camera.sensor->ops.stream_on();
    if (ret)
        goto unlock;

    m_camera.is_stream_on = 1;

    cim_stream_on(m_camera.sensor);

unlock:
    mutex_unlock(&lock);

    return ret;
}

static void camera_stream_off(void)
{
    mutex_lock(&lock);

    if (m_camera.is_stream_on) {
        cim_stream_off();
        m_camera.sensor->ops.stream_off();
        m_camera.is_stream_on = 0;
    }

    mutex_unlock(&lock);
}

static int camera_wait_frame(struct list_head *list, void **mem_p)
{
    int ret = 0;
    unsigned long flags;
    struct frm_data *frm;
    void *mem = NULL;

    mutex_lock(&lock);

    spin_lock_irqsave(&spinlock, flags);
    frm = get_usable_frm();
    if (!frm) {
        spin_unlock_irqrestore(&spinlock, flags);
        ret = wait_event_interruptible_timeout(
            cim_data.waiter, cim_data.frm_cnt, msecs_to_jiffies(3000));

        spin_lock_irqsave(&spinlock, flags);
        frm = get_usable_frm();
        if (ret <= 0 && !frm) {
            ret = -ETIMEDOUT;
            printk(KERN_ERR "camera: wait frame time out\n");
        }
    }
    spin_unlock_irqrestore(&spinlock, flags);

    if (frm) {
        ret = 0;
        frm->status = status_user;
        mem = frm->address;
        list_add_tail(&frm->link, list);
        *mem_p = mem;
    }

    mutex_unlock(&lock);

    return ret;
}

static void do_put_frame(struct frm_data *frm)
{
    unsigned long flags;
    unsigned int index = frm - cim_data.frms;

    spin_lock_irqsave(&spinlock, flags);

    list_del(&frm->link);

    if (cim_data.trans_list[0] == cim_data.trans_list[1]) {
        int id = cim_read_reg(CIMIID);
        index = !id;

        set_trans_frm(index, frm);
    } else {
        add_to_free_list(frm);
    }

    spin_unlock_irqrestore(&spinlock, flags);
}

static void camera_put_frame(void *buf)
{
    unsigned int size = buf - cim_data.mem;
    unsigned int index = size / cim_data.frm_size;
    struct frm_data *frm = &cim_data.frms[index];

    assert(!(size % cim_data.frm_size));
    assert(index < cim_data.mem_cnt);

    mutex_lock(&lock);

    if (frm->status != status_user) {
        printk(KERN_ERR "camera: double free of vic frame %p\n", buf);
        goto unlock;
    }

    do_put_frame(frm);

unlock:
    mutex_unlock(&lock);
}

static int check_frame_mem(void *mem, void *base)
{
    unsigned int size = mem - base;

    if (size % cim_data.frm_size)
        return -1;

    if (size / cim_data.frm_size >= cim_data.mem_cnt)
        return -1;

    return 0;
}

static unsigned int camera_get_available_frame_count(void)
{
    return cim_data.frm_cnt;
}

static void camera_skip_frames(unsigned int frames)
{
    unsigned long flags;

    mutex_lock(&lock);

    spin_lock_irqsave(&spinlock, flags);

    while (frames--) {
        struct frm_data *frm = get_usable_frm();
        if (frm == NULL)
            break;
        add_to_free_list(frm);
    }

    spin_unlock_irqrestore(&spinlock, flags);

    mutex_unlock(&lock);
}

static void camera_get_info(struct camera_info *info)
{
    mutex_unlock(&lock);

    *info = m_camera.sensor->info;

    mutex_unlock(&lock);
}

#define CMD_get_info                _IOWR('C', 120, struct camera_info)
#define CMD_power_on                _IO('C', 121)
#define CMD_power_off               _IO('C', 122)
#define CMD_stream_on               _IO('C', 123)
#define CMD_stream_off              _IO('C', 124)
#define CMD_wait_frame              _IO('C', 125)
#define CMD_put_frame               _IO('C', 126)
#define CMD_get_frame_count         _IO('C', 127)
#define CMD_skip_frames             _IO('C', 128)

struct m_private_data {
    struct list_head list;
    void *map_mem;
};

static long cim_ioctl(struct file *filp, unsigned int cmd, unsigned long arg)
{
    int ret;
    struct m_private_data *data = filp->private_data;
    void *map_mem = data->map_mem;

    switch (cmd) {
    case CMD_get_info:{
        struct camera_info *info = (void *)arg;
        if (!info)
            return -EINVAL;
        camera_get_info(info);
        return 0;
    }

    case CMD_power_on:
        return camera_power_on();

    case CMD_power_off:
        camera_power_off();
        return 0;

    case CMD_stream_on:
        return camera_stream_on();

    case CMD_stream_off:
        camera_stream_off();
        return 0;

    case CMD_wait_frame: {
        struct list_head *list = &data->list;
        void **mem_p = (void *)arg;
        if (!mem_p)
            return -EINVAL;

        if (!map_mem) {
            printk(KERN_ERR "camera: please mmap first\n");
            return -EINVAL;
        }

        void *mem;
        ret = camera_wait_frame(list, &mem);
        if (ret)
            return ret;

        mem = map_mem + (mem - cim_data.mem);
        dma_cache_sync(NULL, mem, cim_data.frm_size, DMA_FROM_DEVICE);
        *mem_p = mem;
        return 0;
    }

    case CMD_put_frame: {
        void *mem = (void *)arg;
        if (!mem)
            return -EINVAL;
        if (check_frame_mem(mem, map_mem))
            return -EINVAL;

        dma_cache_sync(NULL, mem, cim_data.frm_size, DMA_FROM_DEVICE);
        mem = cim_data.mem + (mem - map_mem);
        camera_put_frame(mem);
        return 0;
    }

    case CMD_get_frame_count: {
        unsigned int *count_p = (void *)arg;
        if (!count_p)
            return -EINVAL;
        *count_p = camera_get_available_frame_count();
        return 0;
    }

    case CMD_skip_frames: {
        unsigned int frames = arg;
        camera_skip_frames(frames);
        return 0;
    }

    default:
        printk(KERN_ERR "camera: %x not support this cmd\n", cmd);
        return -EINVAL;
    }

    return 0;
}

static int cim_mmap(struct file *file, struct vm_area_struct *vma)
{
    unsigned long start;
    unsigned long off;
    u32 len;

    off = vma->vm_pgoff << PAGE_SHIFT;

    if (off) {
        printk(KERN_ERR "camera: off must be 0\n");
        return -EINVAL;
    }

    len = cim_data.frm_size * cim_data.mem_cnt;
    if ((vma->vm_end - vma->vm_start) != len) {
        printk(KERN_ERR "camera: size must be total size\n");
        return -EINVAL;
    }

    /* frame buffer memory */
    start = virt_to_phys(cim_data.mem);
    off += start;

    vma->vm_pgoff = off >> PAGE_SHIFT;
    vma->vm_flags |= VM_IO;

    /* 0: cachable,write through (cache + cacheline对齐写穿)
     * 1: uncachable,write Acceleration (uncache + 硬件写加速)
     * 2: uncachable
     * 3: cachable
     */
    pgprot_val(vma->vm_page_prot) &= ~_CACHE_MASK;
    pgprot_val(vma->vm_page_prot) |= _CACHE_CACHABLE_NO_WA;

    if (io_remap_pfn_range(vma, vma->vm_start, off >> PAGE_SHIFT,
                           vma->vm_end - vma->vm_start, vma->vm_page_prot))
    {
        return -EAGAIN;
    }

    struct m_private_data *data = file->private_data;
    data->map_mem = (void *)vma->vm_start;

    return 0;
}

static int cim_open(struct inode *inode, struct file *filp)
{
    struct m_private_data *data = kmalloc(sizeof(*data), GFP_KERNEL);

    INIT_LIST_HEAD(&data->list);
    filp->private_data = data;
    return 0;
}

static int cim_release(struct inode *inode, struct file *filp)
{
    struct list_head *pos, *n;
    struct m_private_data *data = filp->private_data;
    struct list_head *list = &data->list;

    mutex_lock(&lock);

    list_for_each_safe(pos, n, list) {
        struct frm_data *frm = list_entry(pos, struct frm_data, link);
        do_put_frame(frm);
    }

    mutex_unlock(&lock);

    kfree(data);

    return 0;
}

static struct file_operations cim_misc_fops = {
    .open = cim_open,
    .release = cim_release,
    .mmap = cim_mmap,
    .unlocked_ioctl = cim_ioctl,
};

static struct miscdevice cim_mdev = {
    .minor = MISC_DYNAMIC_MINOR,
    .name = "camera",
    .fops = &cim_misc_fops,
};

/*
 * 格式信息转换
 * sensor format : Sensor格式在sensor driver中根据setting指定
 * camera format : Camera格式在camera driver中使用,并暴露给应用
 *
 *    sensor格式                    Camera 格式
 * [成员0 sensor format]  <--->  [成员1 camera format]
 *
 */
struct fmt_pair {
    sensor_pixel_fmt sensor_fmt;
    camera_pixel_fmt camera_fmt;
};

static struct fmt_pair fmts[] = {
    {SENSOR_PIXEL_FMT_Y8_1X8,     CAMERA_PIX_FMT_GREY},
    {SENSOR_PIXEL_FMT_UYVY8_2X8,  CAMERA_PIX_FMT_UYVY},
    {SENSOR_PIXEL_FMT_VYUY8_2X8,  CAMERA_PIX_FMT_VYUY},
    {SENSOR_PIXEL_FMT_YUYV8_2X8,  CAMERA_PIX_FMT_YUYV},
    {SENSOR_PIXEL_FMT_YVYU8_2X8,  CAMERA_PIX_FMT_YVYU},

    {SENSOR_PIXEL_FMT_SBGGR8_1X8, CAMERA_PIX_FMT_SBGGR8},
    {SENSOR_PIXEL_FMT_SGBRG8_1X8, CAMERA_PIX_FMT_SGBRG8},
    {SENSOR_PIXEL_FMT_SGRBG8_1X8, CAMERA_PIX_FMT_SGRBG8},
    {SENSOR_PIXEL_FMT_SRGGB8_1X8, CAMERA_PIX_FMT_SRGGB8},
};

#define error_if(_cond) \
    do { \
        if (_cond) { \
            printk(KERN_ERR "camera: failed to check: %s\n", #_cond); \
            ret = -1; \
            goto unlock; \
        } \
    } while (0)

static int sensor_attribute_check_init(struct cim_sensor_config *sensor)
{
    int ret = -1;

    error_if(m_camera.sensor);
    error_if(!sensor->device_name);
    error_if(sensor->sensor_info.width < 128 || sensor->sensor_info.width > 8192);
    error_if(sensor->sensor_info.height < 128 || sensor->sensor_info.height > 8192);
    error_if(!sensor->ops.power_on);
    error_if(!sensor->ops.power_off);
    error_if(!sensor->ops.stream_on);
    error_if(!sensor->ops.stream_off);

    error_if(sensor->cim_interface != CIM_sync_mode);
    error_if(!(sensor_fmt_is_YUV422(sensor->sensor_info.fmt) ||
            sensor_fmt_is_8BIT(sensor->sensor_info.fmt)));

    memset(sensor->info.name, 0x00, sizeof(sensor->info.name));
    memcpy(sensor->info.name, sensor->device_name, strlen(sensor->device_name));

    sensor->info.width =  sensor->sensor_info.width;
    sensor->info.height =  sensor->sensor_info.height;
    sensor->info.data_fmt = 0;

    int i = 0;
    int size = ARRAY_SIZE(fmts);
    for (i = 0; i < size; i++) {
        if (sensor->sensor_info.fmt == fmts[i].sensor_fmt)
            break;
    }

    if (i >= size) {
        printk(KERN_ERR "sensor format(0x%x) is not Support\n", sensor->sensor_info.fmt);
        goto unlock;
    }

    sensor->info.data_fmt = fmts[i].camera_fmt;
    return 0;

unlock:
    return ret;
}

int cim_register_sensor(struct cim_sensor_config *sensor)
{
    int ret = 0;

    mutex_lock(&lock);

    ret = sensor_attribute_check_init(sensor);
    assert(ret == 0);

    ret = cim_alloc_mem(sensor);
    if (ret)
        goto unlock;

    ret = misc_register(&cim_mdev);
    assert(!ret);

    m_camera.sensor = sensor;
    m_camera.is_power_on = 0;
    m_camera.is_stream_on = 0;

unlock:
    mutex_unlock(&lock);
    return ret;
}

void cim_unregister_sensor(struct cim_sensor_config *sensor)
{
    int ret;

    mutex_lock(&lock);

    assert(m_camera.sensor);
    assert(sensor == m_camera.sensor);

    ret = misc_deregister(&cim_mdev);
    assert(!ret);

    if (m_camera.is_power_on)
        cim_stream_off();

    cim_free_mem();

    m_camera.sensor = NULL;

    mutex_unlock(&lock);
}

static int cim_init(void)
{
    if (m_mem_cnt < 1) {
        m_mem_cnt = 2;
        printk(KERN_ERR "camera: mem cnt fix to 2\n");
    }

    if (cim_init_gpio())
        return -1;

    cim_data.mclk = clk_get(NULL, "cgu_cim");
    assert(!IS_ERR(cim_data.mclk));

    cim_data.clk = clk_get(NULL, "cim");
    assert(!IS_ERR(cim_data.clk));

    init_waitqueue_head(&cim_data.waiter);

    int ret = request_irq(IRQ_CIM, cim_irq_handler, 0, "cim", NULL);
    assert(!ret);

    disable_irq(IRQ_CIM);

    return 0;
}

static void cim_exit(void)
{
    free_irq(IRQ_CIM, NULL);

    clk_put(cim_data.mclk);
    clk_put(cim_data.clk);

    cim_deinit_gpio();
}

module_init(cim_init);

module_exit(cim_exit);

EXPORT_SYMBOL(cim_register_sensor);
EXPORT_SYMBOL(cim_unregister_sensor);

EXPORT_SYMBOL(cim_enable_sensor_mclk);
EXPORT_SYMBOL(cim_disable_sensor_mclk);

MODULE_DESCRIPTION("JZ x1000 cim driver");
MODULE_LICENSE("GPL");
