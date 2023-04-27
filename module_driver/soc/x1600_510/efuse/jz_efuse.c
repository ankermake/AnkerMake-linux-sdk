/*
 * linux/drivers/misc/jz_efuse_x1000.c - Ingenic efuse driver
 *
 * Copyright (C) 2012 Ingenic Semiconductor Co., Ltd.
 * Author: Mick <dongyue.ye@ingenic.com>.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */


#include <linux/module.h>
#include <linux/clk.h>
#include <utils/clock.h>
#include <linux/slab.h>
#include <linux/miscdevice.h>
#include <linux/sched.h>
#include <linux/kernel.h>
#include <linux/string.h>
#include <bit_field.h>
#include <linux/gpio.h>
#include <utils/gpio.h>
#include <linux/init.h>
#include <linux/fs.h>
#include <linux/proc_fs.h>
#include <ingenic_proc.h>

#define EFUSE_CTRL      0x0
#define EFUSE_CFG       0x4
#define EFUSE_STATE     0x8
#define EFUSE_DATA(n)   (0xC + (n)*4)

#define EFUSTATE_WR_DONE    1, 1
#define EFUSTATE_RD_DONE    0, 0

#define EFUSE_STATE_RD_DONE     0
#define EFUSE_STATE_WR_DONE     0

#define EFUSE_REG_BASE      0xB3540000

#define EFUSE_TIMEOUT_MS    100
#define EFUSE_MAX_LENGTH_BYTE   32

#define EFUSE_ADDR(reg) ((volatile unsigned long *)(EFUSE_REG_BASE + reg))

static int gpio_efuse_vddq = -1;
module_param_gpio(gpio_efuse_vddq, 0644);

enum segment_id {
    CHIP_ID,
    CUSTOMER_ID,
    TRIM_DATA,
    SOC_INFO,
    HIDE_BLK,
    PROGRAM_PROTECT,
    CHIP_KEY,
    USER_KEY,
    NKU,
};

struct efuse_segment_info {
    unsigned int seg_start; /* 段在efuse中的起始字节 */
    unsigned int seg_size;  /* 段大小，单位Byte */
    char *segment_name;
};

struct efuse_wr_info_str {
    char *seg_id_name;
    uint32_t size;
    uint32_t start;
    uint8_t *buf;
};

#define CMD_READ                    _IOWR('l', 200, struct efuse_wr_info_str *)
#define CMD_WRITE                   _IOWR('l', 201, struct efuse_wr_info_str *)
#define CMD_READ_SEG_SIZE           _IOWR('l', 202, char *)
#define CMD_SEGMENT_INFORMATION     _IOWR('l', 203, struct efuse_wr_info_str *)
#define CMD_SEGMENT_NUM             _IO('l', 204)

struct jz_efuse {
    struct miscdevice mdev;
    struct mutex lock;
};

#define CHIP_ID_SIZE            16
#define CUSTOMER_ID_SIZE        11
#define TRIM_DATA_SIZE          16
#define SOC_INFO_SIZE           2
#define HIDE_BLK_SIZE           1
#define PROGRAM_PROTECT_SIZE    2
#define CHIP_KEY_SIZE           32
#define USER_KEY_SIZE           32
#define NKU_SIZE                16

#define CHIP_ID_ADDR            (0)
#define CUSTOMER_ID_ADDR        (CHIP_ID_ADDR + CHIP_ID_SIZE)
#define TRIM_DATA_ADDR          (CUSTOMER_ID_ADDR + CUSTOMER_ID_SIZE)
#define SOC_INFO_ADDR           (TRIM_DATA_ADDR + TRIM_DATA_SIZE)
#define HIDE_BLK_ADDR           (SOC_INFO_ADDR + SOC_INFO_SIZE)
#define PROGRAM_PROTECT_ADDR    (HIDE_BLK_ADDR + HIDE_BLK_SIZE)
#define CHIP_KEY_ADDR           (PROGRAM_PROTECT_ADDR + PROGRAM_PROTECT_SIZE)
#define USER_KEY_ADDR           (CHIP_KEY_ADDR + CHIP_KEY_SIZE)
#define NKU_ADDR                (USER_KEY_ADDR + USER_KEY_SIZE)

static struct efuse_segment_info segments[9] = {
    [CHIP_ID]         = {CHIP_ID_ADDR,         CHIP_ID_SIZE,         "CHIP_ID"},
    [CUSTOMER_ID]     = {CUSTOMER_ID_ADDR,     CUSTOMER_ID_SIZE,     "CUSTOMER_ID"},
    [TRIM_DATA]       = {TRIM_DATA_ADDR,       TRIM_DATA_SIZE,       "TRIM_DATA"},
    [SOC_INFO]        = {SOC_INFO_ADDR,        SOC_INFO_SIZE,        "SOC_INFO"},
    [HIDE_BLK]        = {HIDE_BLK_ADDR,        HIDE_BLK_SIZE,        "HIDE_BLK"},
    [PROGRAM_PROTECT] = {PROGRAM_PROTECT_ADDR, PROGRAM_PROTECT_SIZE, "PROGRAM_PROTECT"},
    [CHIP_KEY]        = {CHIP_KEY_ADDR,        CHIP_KEY_SIZE,        "CHIP_KEY"},
    [USER_KEY]        = {USER_KEY_ADDR,        USER_KEY_SIZE,        "USER_KEY"},
    [NKU]             = {NKU_ADDR,             NKU_SIZE,             "NKU"},
};

static struct jz_efuse efuse;

static inline void efuse_write_reg(unsigned int reg, unsigned int value)
{
    *EFUSE_ADDR(reg) = value;
}

static inline unsigned int efuse_read_reg(unsigned int reg)
{
    return *EFUSE_ADDR(reg);
}

static inline void efuse_set_bit(unsigned int reg, int start, int end, unsigned int val)
{
    set_bit_field(EFUSE_ADDR(reg), start, end, val);
}

static inline unsigned int efuse_get_bit(unsigned int reg, int start, int end)
{
    return get_bit_field(EFUSE_ADDR(reg), start, end);
}

static int soc_efuse_segment_num(void)
{
    return ARRAY_SIZE(segments);
}

static int get_segment_id(char *name)
{
    int i;
    int segment_num = soc_efuse_segment_num();

    for(i = 0; i < segment_num; i++) {
        if (strcmp(segments[i].segment_name, name) == 0) {
            return i;
        }
    }

    printk(KERN_ERR "EFUSE: can not find %s segment_id\n", name);

    return -1;
}

int soc_efuse_read_segment_size(char *name, int *seg_id)
{
    int id = get_segment_id(name);
    if (id < CHIP_ID || id > NKU) {
        printk(KERN_ERR "EFUSE: can not get %s segment_size\n", name);
        return -1;
    }

    if (seg_id != NULL)
        *seg_id = id;

    return segments[id].seg_size;
}

static void jz_efuse_enable_read(unsigned int addr, int len)
{
    efuse_write_reg(EFUSE_CTRL, 0);
    efuse_write_reg(EFUSE_STATE, 0);

    unsigned int val = addr << 21 | ((len - 1) << 16);
    efuse_write_reg(EFUSE_CTRL, val);
    val |= 1;
    efuse_write_reg(EFUSE_CTRL, val);
}

static int soc_efuse_read_segment(int seg_id, unsigned char *buf, int len)
{
    if (len != segments[seg_id].seg_size) {
        printk(KERN_ERR "EFUSE: %d segment_size should be equal to %d Byte\n", seg_id, segments[seg_id].seg_size);
        return -1;
    }

    unsigned int addr = segments[seg_id].seg_start;

    while (len > 0) {
        int data_n = 0;
        int N = len > EFUSE_MAX_LENGTH_BYTE ? EFUSE_MAX_LENGTH_BYTE : len;
        int size = N;

        jz_efuse_enable_read(addr, size);
        while (!efuse_get_bit(EFUSE_STATE, EFUSTATE_RD_DONE));

        while (size > 0) {
            int n = size > 4 ? 4 : size;
            unsigned int data = efuse_read_reg(EFUSE_DATA(data_n));
            memcpy(buf, &data, n);
            data_n ++;
            size -= n;
            buf += n;
        }

        efuse_write_reg(EFUSE_STATE, 0);
        len -= N;
        addr += N;
    }

    return 0;
}

static void soc_efuse_segment_information(struct efuse_segment_info *info)
{
    mutex_lock(&efuse.lock);

    int seg_id = info->seg_start;
    int name_len = info->seg_size;

    strncpy(info->segment_name, segments[seg_id].segment_name, name_len);
    info->seg_size = segments[seg_id].seg_size;
    info->seg_start = segments[seg_id].seg_start;

    mutex_unlock(&efuse.lock);
}

int soc_efuse_read(struct efuse_wr_info_str *seg_info)
{
    int ret = 0;
    unsigned char seg_buf[128] = {0};

    char *seg_name = seg_info->seg_id_name;
    unsigned char *buf = seg_info->buf;
    int start = seg_info->start;
    int size = seg_info->size;

    mutex_lock(&efuse.lock);

    int seg_id;
    int len = soc_efuse_read_segment_size(seg_name, &seg_id);
    if (len < 0) {
        ret = -1;
        goto unlock;
    }

    if ((start + size) > len) {
        printk(KERN_ERR "EFUSE: %d segment_size should be less than %d Byte", seg_id, len);
        ret = -1;
        goto unlock;
    }

    ret = soc_efuse_read_segment(seg_id, seg_buf, len);
    if(ret < 0)
        goto unlock;

    memcpy(buf, &seg_buf[start], size);

unlock:
    mutex_unlock(&efuse.lock);

    return ret;
}

static int soc_efuse_write_segment(enum segment_id seg_id, unsigned char *buf, int len)
{
    if (len != segments[seg_id].seg_size) {
        printk(KERN_ERR "EFUSE: %d segment_size should be equal to %d Byte\n", seg_id, segments[seg_id].seg_size);
        return -1;
    }

    if (gpio_efuse_vddq == -1) {
        printk(KERN_ERR "EFUSE: efuse vddq gpio should be set when write segment!\n");
        return -1;
    }

    unsigned int addr = segments[seg_id].seg_start;

    while (len > 0) {
        int data_n = 0;
        int N = len > EFUSE_MAX_LENGTH_BYTE ? EFUSE_MAX_LENGTH_BYTE : len;
        int size = N;

        while (size > 0) {
            int data = 0;
            int n = size > 4 ? 4 : size;

            memcpy(&data, buf, n);

            efuse_write_reg(EFUSE_DATA(data_n), data);
            // printk(KERN_ERR "EFUSE: segment_id: %d, data_n%d = [%08x]\n", seg_id, data_n, data);
            data_n ++;
            size -= n;
            buf += n;
        }

        efuse_write_reg(EFUSE_CTRL, 0);
        efuse_write_reg(EFUSE_STATE, 0);

        unsigned int reg = addr << 21 | (N - 1) << 16 | 1 << 15;
        efuse_write_reg(EFUSE_CTRL, reg);

        uint64_t time = local_clock_ms();

        // Please confirm the circuit carefully before using or the chip will burn out
        // Connect AVDEFUSE pin to 2.5V(not EN_AVDEFUSE)
        gpio_set_value(gpio_efuse_vddq, 0);

        reg = efuse_read_reg(EFUSE_CTRL);
        reg |= 1 << 1;
        efuse_write_reg(EFUSE_CTRL, reg);

        while (!efuse_get_bit(EFUSE_STATE, EFUSTATE_WR_DONE)) {
            if (local_clock_ms() - time > EFUSE_TIMEOUT_MS) {
                printk(KERN_ERR "EFUSE: write efuse timeout\n");
                break;
            }
        }

        // Disconnect AVDEFUSE pin from 2.5V(not EN_AVDEFUSE)
        gpio_set_value(gpio_efuse_vddq, 1);

        len -= N;
        addr += N;
    }

    return 0;
}

int soc_efuse_write(struct efuse_wr_info_str *seg_info)
{
    int ret = 0;
    // EFuse writing 0 has no effect
    unsigned char seg_buf[128] = {0};

    char *seg_name = seg_info->seg_id_name;
    unsigned char *buf = seg_info->buf;
    int start = seg_info->start;
    int size = seg_info->size;

    mutex_lock(&efuse.lock);

    int seg_id;
    int len = soc_efuse_read_segment_size(seg_name, &seg_id);
    if (len < 0) {
        ret = -1;
        goto unlock;
    }

    if ((start + size) > len) {
        printk(KERN_ERR "EFUSE: %d segment_size should be less than %d Byte", seg_id, len);
        ret = -1;
        goto unlock;
    }

    memcpy(&seg_buf[start], buf, size);

    ret = soc_efuse_write_segment(seg_id, seg_buf, len);

unlock:
    mutex_unlock(&efuse.lock);

    return ret;
}

static long efuse_ioctl(struct file *filp, unsigned int cmd, unsigned long arg)
{
    int ret = 0;
    char *name = (char *)arg;
    struct efuse_wr_info_str *info = (struct efuse_wr_info_str *)arg;

    switch (cmd)
    {
    case CMD_READ:
        ret = soc_efuse_read(info);
        break;

    case CMD_WRITE:
        ret = soc_efuse_write(info);
        break;

    case CMD_READ_SEG_SIZE:
        ret = soc_efuse_read_segment_size(name, NULL);
        break;

    case CMD_SEGMENT_NUM:
        ret = soc_efuse_segment_num();
        break;

    case CMD_SEGMENT_INFORMATION:
        soc_efuse_segment_information((struct efuse_segment_info *)arg);
        break;

    default:
        printk(KERN_ERR "EFUSE: do not support this cmd: %x\n", cmd);
        return -1;
    }

    return ret;
}

static int efuse_open(struct inode *inode, struct file *filp)
{
    return 0;
}

static int efuse_release(struct inode *inode, struct file *filp)
{
    return 0;
}

static struct file_operations efuse_misc_fops = {
    .open = efuse_open,
    .release = efuse_release,
    .unlocked_ioctl = efuse_ioctl,
};

static int efuse_read_segment_proc(struct seq_file *m, void *v, unsigned char *seg_name)
{
    unsigned char buf[128] = {0};

    int len = soc_efuse_read_segment_size(seg_name, NULL);
    if (len < 0)
        return -1;

    struct efuse_wr_info_str seg_info;
    seg_info.seg_id_name = seg_name;
    seg_info.buf = buf;
    seg_info.start = 0;
    seg_info.size = len;

    int ret = soc_efuse_read(&seg_info);
    if (ret < 0) {
        printk(KERN_ERR "EFUSE: failed to read %s segment\n", seg_name);
        return ret;
    }

    int n;
    seq_printf(m, "%s: ", seg_name);
    for (n = 0; n < len; n++)
        seq_printf(m, "%02x", buf[n]);

    seq_printf(m, "\n");

    return 0;
}

static int efuse_read_chip_id_proc(struct seq_file *m, void *v)
{
    return efuse_read_segment_proc(m, v, "CHIP_ID");
}

static int efuse_read_user_id_proc(struct seq_file *m, void *v)
{
    return efuse_read_segment_proc(m, v, "CUSTOMER_ID");
}

static int efuse_read_chipID_proc_open(struct inode *inode, struct file *file)
{
    return single_open(file, efuse_read_chip_id_proc, PDE_DATA(inode));
}

static int efuse_read_userID_proc_open(struct inode *inode,struct file *file)
{
    return single_open(file, efuse_read_user_id_proc, PDE_DATA(inode));
}

static const struct proc_ops efuse_proc_read_chipID_fops = {
    .proc_open = efuse_read_chipID_proc_open,
    .proc_read = seq_read,
    .proc_lseek = seq_lseek,
    .proc_release = single_release,
};

static const struct proc_ops efuse_proc_read_userID_fops = {
    .proc_open = efuse_read_userID_proc_open,
    .proc_read = seq_read,
    .proc_lseek = seq_lseek,
    .proc_release = single_release,
};

static struct clk *clk;
static struct clk *h2clk;
struct proc_dir_entry *efuse_dir;

static int __init jz_efuse_init(void)
{
    h2clk = clk_get(NULL, "div_ahb2");
    if (IS_ERR(h2clk)) {
        printk(KERN_ERR "EFUSE: failed to get h2clk!\n");
        return -1;
    }

    unsigned long rate = clk_get_rate(h2clk);
    unsigned long ns = 1000000000 / rate;

    if (gpio_efuse_vddq != -1) {
        gpio_request(gpio_efuse_vddq, "efuse_vddq");
        gpio_direction_output(gpio_efuse_vddq, 1);
    }

    mutex_init(&efuse.lock);

    // cannot have floating point calculation so used 7 instead of 6.5
    unsigned int rd_adj = 7 / ns;
    if ((rd_adj + 1) * ns <= 7) {
        printk(KERN_ERR "EFUSE: failed to get efuse cfg rd_adj!\n");
        return -1;
    }

    // cannot have floating point calculation so used 7 instead of 6.5
    unsigned int wr_adj = 7 / ns;
    if ((wr_adj + 1) * ns <= 7) {
        printk(KERN_ERR "EFUSE: failed to get efuse cfg wr_adj!\n");
        return -1;
    }

    int rd_strobe = 35 / ns - rd_adj - 4;
    if (rd_strobe < 0)
        rd_strobe = 0;
    if ((rd_adj + rd_strobe + 5) * ns <= 35) {
        printk(KERN_ERR "EFUSE: failed to get efuse cfg rd_strobe!\n");
        return -1;
    }

    int wr_strobe = 10000 / ns - wr_adj - 1666;
    if (wr_strobe < 0)
        wr_strobe = 0;
    wr_strobe = wr_strobe + 1;
    unsigned int tmp = (wr_adj + 1666 + wr_strobe) * ns;
    if (tmp < 9000 || tmp > 11000) {
        printk(KERN_ERR "EFUSE: failed to get efuse cfg wr_strobe!\n");
        return -1;
    }

    tmp = rd_adj << 20 | rd_strobe << 16 | wr_adj << 12 | wr_strobe;

    clk = clk_get(NULL, "gate_efuse");
    if (IS_ERR(clk)) {
        printk(KERN_ERR "EFUSE: failed to get efuse clk!\n");
        return -1;
    }
    clk_prepare_enable(clk);

    efuse_write_reg(EFUSE_CFG, tmp);

    efuse_dir = jz_proc_mkdir("efuse");
    if (!efuse_dir) {
        printk(KERN_ERR "EFUSE: failed to create_proc_entry for common efuse.\n");
        return -ENODEV;
    }

    struct proc_dir_entry *res = proc_create("efuse_chip_id", 0444, efuse_dir, &efuse_proc_read_chipID_fops);
    if (!res)
        printk(KERN_ERR "EFUSE: create proc of efuse_chip_id error!!!\n");

    res = proc_create("efuse_user_id", 0444, efuse_dir, &efuse_proc_read_userID_fops);
    if (!res)
        printk(KERN_ERR "EFUSE: create proc of efuse_user_id error!!!\n");

    efuse.mdev.minor = MISC_DYNAMIC_MINOR;
    efuse.mdev.name = "efuse-string-version";
    efuse.mdev.fops = &efuse_misc_fops;

    int ret = misc_register(&efuse.mdev);
    BUG_ON(ret < 0);

    return 0;
}
module_init(jz_efuse_init);

static void __exit jz_efuse_exit(void)
{
    misc_deregister(&efuse.mdev);
    proc_remove(efuse_dir);
    clk_disable_unprepare(clk);
    clk_put(clk);

    if (gpio_efuse_vddq != -1)
        gpio_free(gpio_efuse_vddq);

    clk_put(h2clk);

    return;
}
module_exit(jz_efuse_exit);

MODULE_DESCRIPTION("JZ x1600_510 EFUSE driver");
MODULE_LICENSE("GPL");