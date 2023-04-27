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
#include <linux/slab.h>
#include <linux/kernel.h>
#include <linux/gpio.h>
#include <linux/init.h>
#include <linux/miscdevice.h>
#include <linux/sched.h>
#include <linux/sched/clock.h>
#include <linux/ctype.h>
#include <linux/fs.h>
#include <linux/timer.h>
#include <linux/jiffies.h>
#include <linux/clk.h>
#include <linux/proc_fs.h>
#include <linux/seq_file.h>
#include <linux/delay.h>
#include <soc/base.h>
#include "bit_field.h"
#include <utils/gpio.h>
#include <utils/clock.h>
#include <ingenic_proc.h>
#include <linux/string.h>

#include <gpio_def.h>
#define EFUSE_CTRL      0x0
#define EFUSE_CFG       0x4
#define EFUSE_STATE     0x8
#define EFUSE_DATA(n)   (0xC + (n)*4)

#define EFUSTATE_WR_DONE    1, 1
#define EFUSTATE_RD_DONE    0, 0

#define EFUSE_STATE_RD_DONE     0
#define EFUSE_STATE_WR_DONE     0

#define EFUSE_REG_BASE      0xB3540000

#define EFUSE_TIMEOUT_MS  200

#define EFUSE_ADDR(reg) ((volatile unsigned long *)(EFUSE_REG_BASE + reg))

#define CMD_READ                                    _IOWR('l', 200, struct efuse_wr_info_str *)
#define CMD_WRITE                                   _IOWR('l', 201, struct efuse_wr_info_str *)
#define CMD_READ_SEG_SIZE                           _IOWR('l', 202, char *)
#define CMD_SEGMENT_INFORMATION                     _IOWR('l', 203, struct efuse_segment_list *)
#define CMD_SEGMENT_NUM                             _IO('l', 204)

struct proc_dir_entry *efuse_dir;

static int gpio_efuse_vddq = -1;
module_param_gpio(gpio_efuse_vddq, 0644);

enum segment_id {
    CHIP_ID,
    CUSTOMER_ID0,
    CUSTOMER_ID1,
    CUSTOMER_ID2,
    TRIM_DATA0,
    TRIM_DATA1,
    TRIM_DATA2,
    SOC_INFO,
    PROTECT_BIT,
    HIDE_BLOCK,
};

static char *segment_id_name[10] = {
    "CHIP_ID",
    "CUSTOMER_ID0",
    "CUSTOMER_ID1",
    "CUSTOMER_ID2",
    "TRIM_DATA0",
    "TRIM_DATA1",
    "TRIM_DATA2",
    "SOC_INFO",
    "PROTECT_BIT",
    "HIDE_BLOCK",
};

struct efuse_segment_info {
    unsigned int seg_start;
    unsigned int seg_size;
    char *segment_name;
};

struct efuse_wr_info_str {
    char *seg_id_name;
    uint32_t size;
    uint32_t start;
    uint8_t *buf;
};

struct jz_efuse {
    struct miscdevice mdev;
    struct mutex lock;
    struct timer_list vddq_protect_timer;
};

struct seg_info {
    unsigned int seg_start; /* 段在efuse中的起始字节 */
    unsigned int seg_size;  /* 段大小，单位Byte */
};

/* 保护位虽然有32位，但实际有作用的是后16位
*/
static struct seg_info segments[10] = {
    [CHIP_ID] = {0, 17},
    [CUSTOMER_ID0] = {17, 17},
    [CUSTOMER_ID1] = {34, 29},
    [CUSTOMER_ID2] = {63, 29},
    [TRIM_DATA0] = {92, 5},
    [TRIM_DATA1] = {97, 5},
    [TRIM_DATA2] = {102, 5},
    [SOC_INFO] = {107, 5},
    [PROTECT_BIT] = {112, 4},
    [HIDE_BLOCK] = {116, 4}
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

static int get_segment_id(char *name)
{
    int i;

    for(i = 0; i < ARRAY_SIZE(segment_id_name); i++){
        if (strcmp(segment_id_name[i], name) == 0) {
            return i;
        }
    }

    return -1;
}

static void jz_efuse_enable_read(unsigned int addr)
{
    unsigned int val;

    efuse_write_reg(EFUSE_STATE, 0);

    val = addr << 21 | ((1 - 1) << 16);
    efuse_write_reg(EFUSE_CTRL, val);
    val |= 1;
    efuse_write_reg(EFUSE_CTRL, val);

}

static unsigned int jz_efuse_read_word(unsigned int start)
{
    unsigned int addr = start / 4;

    jz_efuse_enable_read(addr);

    while (!efuse_get_bit(EFUSE_STATE, EFUSTATE_RD_DONE));

    return efuse_read_reg(EFUSE_DATA(0));
}

static int soc_efuse_read_segment(int seg_id, unsigned char *buf, int len)
{
    unsigned int data = 0;
    int start;

    if (len != segments[seg_id].seg_size) {
        printk(KERN_ERR "EFUSE: operate segment %d data length != %d Byte\n", seg_id, segments[seg_id].seg_size);
        return -1;
    }

    start = segments[seg_id].seg_start;

    if (start % 4) {
        int offset = start % 4;
        int n = 4 - offset;

        data = jz_efuse_read_word(start);

        char *tmp_buf = (void *)&data;

        if (len < n)
            n = len;

        memcpy(buf, tmp_buf + offset, n);

        start += n;
        buf += n;
        len -= n;
    }

    while (len >= 4) {
        data = jz_efuse_read_word(start);

        char *tmp_buf = (void *)&data;

        memcpy(buf, tmp_buf, 4);

        start += 4;
        len -= 4;
        buf += 4;
    }

    if (len) {
        data = jz_efuse_read_word(start);

        char *tmp_buf = (void *)&data;

        memcpy(buf, tmp_buf, len);
    }

    efuse_write_reg(EFUSE_STATE, 0);

    return 0;
}

static int soc_efuse_segment_information(struct efuse_segment_info *info)
{
    int n;
    int lenth;
    mutex_lock(&efuse.lock);

    n = info->seg_start;
    lenth = info->seg_size;
    strncpy(info->segment_name, segment_id_name[n], lenth);
    info->seg_size = segments[n].seg_size;
    info->seg_start = segments[n].seg_start;

    mutex_unlock(&efuse.lock);
    return 0;
}

static int soc_efuse_segment_num(void)
{
    return ARRAY_SIZE(segment_id_name);
}

int soc_efuse_read(char *seg_id_name, unsigned char *buf, int start, int size)
{
    int len, i;
    int ret = 0;
    int seg_id;
    unsigned char save_buf[32] = {0};

    mutex_lock(&efuse.lock);

    seg_id = get_segment_id(seg_id_name);
    if (seg_id < CHIP_ID || seg_id > HIDE_BLOCK) {
        printk(KERN_ERR "EFUSE: have not %s segment\n", seg_id_name);
        ret = -1;
        goto unlock;
    }

    len = segments[seg_id].seg_size;

    if ((start + size) > len) {
        printk(KERN_ERR "EFUSE: operate segment %d data length size should <= %d Byte", seg_id, len);
        ret = -1;
        goto unlock;
    }

    ret = soc_efuse_read_segment(seg_id, save_buf, len);
    if(ret < 0)
        goto unlock;

    for (i = start; i < (start + size); i++)
        *buf++ = save_buf[i];

unlock:
    mutex_unlock(&efuse.lock);

    return ret;
}

static void jz_efuse_write_word(unsigned int start, unsigned int data)
{
    unsigned int val = 0;

    unsigned int addr = start / 4;

    efuse_write_reg(EFUSE_DATA(0), data);

    /* Programming EFUSE enable, set addr and length */
    val = efuse_read_reg(EFUSE_CTRL);
    val = addr << 21 | ((1 - 1) << 16) | 1 << 15;
    val |= 1 << 9;         // pass 1.8V power to internal for program
    val &= ~(1 << 8);      // power up
    efuse_write_reg(EFUSE_CTRL, val);

    /* Connect VDDQ pin from 1.8V */
    gpio_set_value(gpio_efuse_vddq, 0);
    uint64_t time = local_clock_ms();

    /* Write EFUSE enable */
    val |= 2;
    efuse_write_reg(EFUSE_CTRL, val);

    /* Wait write EFUSE */
    while (!efuse_get_bit(EFUSE_STATE, EFUSTATE_WR_DONE)) {
        if (local_clock_ms() - time > EFUSE_TIMEOUT_MS) {
            printk(KERN_ERR "EFUSE write word timeout\n");
            break;
        }
    }
    /* Disconnect VDDQ pin from 1.8V. */
    gpio_set_value(gpio_efuse_vddq, 1);

    efuse_write_reg(EFUSE_CTRL, 0);
    efuse_write_reg(EFUSE_STATE, 0);

    val = 1 << 8;      // power down
    efuse_write_reg(EFUSE_CTRL, val);
}

static int soc_efuse_write_segment(int seg_id, unsigned char *buf, int len)
{
    int i;
    unsigned int start, data = 0;

    if (len != segments[seg_id].seg_size) {
        printk(KERN_ERR "EFUSE: operate segment %d data length != %d Byte\n", seg_id, segments[seg_id].seg_size);
        return -1;
    }

    if (gpio_efuse_vddq == -1) {
        printk(KERN_ERR "EFUSE: efuse vddq gpio should be set !\n");
        return -1;
    }

    start = segments[seg_id].seg_start;

    if (start % 4) {
        /* 写操作每次要写一个字，由于每段的第一个字节可能不是字对齐的，
         * 就会出现该段与上一段处交叉在同一个字中的情况。
         * 这样在写该段的第一个字时，要将这个32位中不属于当该段的位清0(写0到efuse 相当于等于不操作)
        */
        data = 0;

        int offset = start % 4;
        int n = 4 - offset;

        if (len < n)
            n = len;

        for (i = offset; i < 4; i++) {
            data |= (*buf << (i * 8));
            buf++;
        }

        jz_efuse_write_word(start, data);

        start += n;
        len -= n;
    }

    while (len >= 4) {
        data = 0;
        for (i = 0; i < 4; i++) {
            data |= (*buf << (i * 8));
            buf++;
        }

        jz_efuse_write_word(start, data);

        start += 4;
        len -= 4;
    }

    if (len) {
        data = 0;
        for (i = 0; i < len; i++) {
            data |= (*buf << (i * 8));
            buf++;
        }

        jz_efuse_write_word(start, data);
    }

    efuse_write_reg(EFUSE_CTRL, 0);
    efuse_write_reg(EFUSE_STATE, 0);

    return 0;
}

int soc_efuse_write(char *seg_id_name, unsigned char *buf, int start, int size)
{
    int len = 0, ret = 0;
    int seg_id, i;
    unsigned char save_buf[40] = {0};

    mutex_lock(&efuse.lock);

    seg_id = get_segment_id(seg_id_name);
    if (seg_id < CHIP_ID || seg_id > HIDE_BLOCK) {
        printk(KERN_ERR "EFUSE: have not %s segment\n", seg_id_name);
        ret = -1;
        goto unlock;
    }

    len = segments[seg_id].seg_size;

    if ((start + size) > len) {
        printk(KERN_ERR "EFUSE: operate segment %d data length size should <= %d Byte", seg_id, len);
        ret = -1;
        goto unlock;
    }

    ret = soc_efuse_read_segment(seg_id, save_buf, len);

    for (i = start; i < (start + size); i++)
        save_buf[i] = *buf++;

    ret = soc_efuse_write_segment(seg_id, save_buf, len);
    if (ret < 0)
        goto unlock;

unlock:
    mutex_unlock(&efuse.lock);

    return ret;
}

int soc_efuse_read_seg_size(char *name)
{
    int seg_id;

    seg_id = get_segment_id(name);
    if (seg_id < CHIP_ID || seg_id > HIDE_BLOCK) {
        printk(KERN_ERR "EFUSE: have not %s segment\n", name);
        return -1;
    }

    return segments[seg_id].seg_size;
}

static long efuse_ioctl(struct file *filp, unsigned int cmd, unsigned long arg)
{
    int ret;
    char *name;
    struct efuse_wr_info_str *wr_info;

    wr_info = (struct efuse_wr_info_str *)arg;
    name = (char *)arg;

    switch (cmd) {
    case CMD_READ:
        ret = soc_efuse_read(wr_info->seg_id_name, wr_info->buf, wr_info->start, wr_info->size);
        break;
    case CMD_WRITE:
        ret = soc_efuse_write(wr_info->seg_id_name, wr_info->buf, wr_info->start, wr_info->size);
        break;
    case CMD_READ_SEG_SIZE:
        ret = soc_efuse_read_seg_size(name);
        break;
    case CMD_SEGMENT_NUM:
        ret = soc_efuse_segment_num();
        break;
    case CMD_SEGMENT_INFORMATION:
        ret = soc_efuse_segment_information((struct efuse_segment_info *)arg);
        break;
    default:
        ret = -1;
        printk(KERN_ERR "no support your cmd:%x\n", cmd);
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

static int efuse_read_chip_id_proc(struct seq_file *m, void *v)
{
    int n, ret = 0;
    unsigned char buf[17];

    ret = soc_efuse_read("CHIP_ID", buf, 0, 16);
    seq_printf(m,"CHIP_ID: ");
    for(n = 0; n < 16; n++)
        seq_printf(m,"%02x",buf[n]);

    seq_printf(m,"\n");

    return ret;
}

static int efuse_read_user_id_proc(struct seq_file *m, void *v)
{
    int n, ret = 0;
    unsigned char buf[30];

    ret |= soc_efuse_read("CUSTOMER_ID0", buf, 0, 17);
    seq_printf(m,"CUSTOMER_ID0: ");
    for(n = 0; n < 17; n++)
        seq_printf(m,"%02x",buf[n]);

    seq_printf(m,"\n");

    ret |= soc_efuse_read("CUSTOMER_ID1", buf, 0, 29);
    seq_printf(m,"CUSTOMER_ID1: ");
    for(n = 0; n < 29; n++)
        seq_printf(m,"%02x",buf[n]);

    seq_printf(m,"\n");

    ret |= soc_efuse_read("CUSTOMER_ID2", buf, 0, 29);
    seq_printf(m,"CUSTOMER_ID2: ");
    for(n = 0; n < 29; n++)
        seq_printf(m,"%02x",buf[n]);

    seq_printf(m,"\n");

    return ret;
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

static int __init jz_efuse_init(void)
{
    int ret;
    unsigned int tmp;
    unsigned long rate, ns;
    int rd_strobe, wr_strobe;
    struct proc_dir_entry *res;
    unsigned int rd_adj, wr_adj;

    h2clk = clk_get(NULL, "div_ahb2");
    if (IS_ERR(h2clk)) {
        printk(KERN_ERR "get h2clk rate fail!\n");
        return -1;
    }

    rate = clk_get_rate(h2clk);
    ns = 1000000000 / rate;

    if (gpio_efuse_vddq != -1) {
        gpio_request(gpio_efuse_vddq, "efuse_vddq");
        gpio_direction_output(gpio_efuse_vddq, 1);
    }

    mutex_init(&efuse.lock);

    rd_adj = 4 / ns;
    if ((rd_adj + 1) * ns <= 4) {
        printk(KERN_ERR "EFUSE: get efuse cfg rd_adj fail!\n");
        return -1;
    }

    wr_adj = 4/ ns;
    if ((wr_adj + 1) * ns <= 4) {
        printk(KERN_ERR "EFUSE: get efuse cfg wr_adj fail!\n");
        return -1;
    }

    rd_strobe = 100 / ns - rd_adj - 29;
    if (rd_strobe < 0)
        rd_strobe = 0;
    if ((rd_adj + rd_strobe + 30) * ns <= 100) {
        printk(KERN_ERR "EFUSE: get efuse cfg rd_strobe fail!\n");
        return -1;
    }

    wr_strobe = 12000 / ns - wr_adj - 2999;
    tmp = (wr_adj + 3000 + wr_strobe) * ns;
    if (tmp > 13000 || tmp < 11000) {
        printk(KERN_ERR "EFUSE: get efuse cfg wr_strobe fail!\n");
        return -1;
    }

    tmp = rd_adj << 24 | rd_strobe << 16 | wr_adj << 12 | wr_strobe;

    clk = clk_get(NULL, "gate_efuse");
    if (IS_ERR(clk)) {
        printk(KERN_ERR "get efuse clk fail!\n");
        return -1;
    }
    clk_prepare_enable(clk);

    efuse_write_reg(EFUSE_CFG, tmp);

    efuse_dir = jz_proc_mkdir("efuse");
    if (!efuse_dir) {
        printk(KERN_ERR "create_proc_entry for common efuse failed.\n");
        return -ENODEV;
    }

    res = proc_create("efuse_chip_id", 0444, efuse_dir, &efuse_proc_read_chipID_fops);
    if(!res){
        printk(KERN_ERR "create proc of efuse_chip_id error!!!!\n");
    }

    res = proc_create("efuse_user_id",0444, efuse_dir, &efuse_proc_read_userID_fops);
    if(!res){
        printk(KERN_ERR "create proc of efuse_user_id error!!!!\n");
    }

    efuse.mdev.minor = MISC_DYNAMIC_MINOR;
    efuse.mdev.name = "efuse-string-version";
    efuse.mdev.fops = &efuse_misc_fops;

    ret = misc_register(&efuse.mdev);
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

MODULE_DESCRIPTION("JZ x2000 EFUSE driver");
MODULE_LICENSE("GPL");