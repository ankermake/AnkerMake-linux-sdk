#include "gvm8666b_fw.h"
#include <linux/slab.h>

#define GT_FW_CHK_CODE          (0xABCD)
#define GT_FW_CHK_CODE_ADDR     (0x0860)
#define GT_CONTINUE_CAP_MASK    (0x0020)

#define GT_EEPROM_I2C_WRITE_ADDR    (0xA0)
#define GT_EEPROM_I2C_READ_ADDR     (0xA1)
#define GT_EEPROM_SIZE              (64 * 1024)
#define GX_EEPROM_PHASE_SHIFT_SIZE  (0x25) //37

static uint8_t g_eeprom_info[GT_EEPROM_SIZE];
static uint8_t g_eeprom_phase_shift_info[GX_EEPROM_PHASE_SHIFT_SIZE];
static uint8_t g_eeprom_crc_state = 0x0;

struct uid_info
{
    unsigned char lot_id[8];
    unsigned char fab_info;     //bit[3:0]:FAB;bit[7:3]:Wafer_No
    unsigned char x_coordinate;
    unsigned char y_coordinate;
    unsigned char test_time[4]; //bit[0]:reserve;bit[31:1]:test_time;
    unsigned char cp_nvm_info;  //0x0F
    unsigned char test_version[2];
    unsigned char random_num;   //0x12
};

struct efuse_info
{
    struct uid_info uid_info;
    unsigned char reserve_cpA[5];
    unsigned char clk_trim; //0x18
    unsigned char wdt_trim; //0x19
    unsigned char bg_trim;  //0x1A
    unsigned char mbist_repair; //0x1B
    unsigned char vptat_trim;   //0x1C
    unsigned char pixel_record[7];
    unsigned char cp_trim_record_crc; //0x24
    unsigned char cp_reserveB[26];
    unsigned char cp_crc;      //0x3F
};

const unsigned char gvm8666b_crc8[] =
{
    0x00, 0x07, 0x0E, 0x09, 0x1C, 0x1B, 0x12, 0x15, 0x38, 0x3F, 0x36, 0x31,
    0x24, 0x23, 0x2A, 0x2D, 0x70, 0x77, 0x7E, 0x79, 0x6C, 0x6B, 0x62, 0x65,
    0x48, 0x4F, 0x46, 0x41, 0x54, 0x53, 0x5A, 0x5D, 0xE0, 0xE7, 0xEE, 0xE9,
    0xFC, 0xFB, 0xF2, 0xF5, 0xD8, 0xDF, 0xD6, 0xD1, 0xC4, 0xC3, 0xCA, 0xCD,
    0x90, 0x97, 0x9E, 0x99, 0x8C, 0x8B, 0x82, 0x85, 0xA8, 0xAF, 0xA6, 0xA1,
    0xB4, 0xB3, 0xBA, 0xBD, 0xC7, 0xC0, 0xC9, 0xCE, 0xDB, 0xDC, 0xD5, 0xD2,
    0xFF, 0xF8, 0xF1, 0xF6, 0xE3, 0xE4, 0xED, 0xEA, 0xB7, 0xB0, 0xB9, 0xBE,
    0xAB, 0xAC, 0xA5, 0xA2, 0x8F, 0x88, 0x81, 0x86, 0x93, 0x94, 0x9D, 0x9A,
    0x27, 0x20, 0x29, 0x2E, 0x3B, 0x3C, 0x35, 0x32, 0x1F, 0x18, 0x11, 0x16,
    0x03, 0x04, 0x0D, 0x0A, 0x57, 0x50, 0x59, 0x5E, 0x4B, 0x4C, 0x45, 0x42,
    0x6F, 0x68, 0x61, 0x66, 0x73, 0x74, 0x7D, 0x7A, 0x89, 0x8E, 0x87, 0x80,
    0x95, 0x92, 0x9B, 0x9C, 0xB1, 0xB6, 0xBF, 0xB8, 0xAD, 0xAA, 0xA3, 0xA4,
    0xF9, 0xFE, 0xF7, 0xF0, 0xE5, 0xE2, 0xEB, 0xEC, 0xC1, 0xC6, 0xCF, 0xC8,
    0xDD, 0xDA, 0xD3, 0xD4, 0x69, 0x6E, 0x67, 0x60, 0x75, 0x72, 0x7B, 0x7C,
    0x51, 0x56, 0x5F, 0x58, 0x4D, 0x4A, 0x43, 0x44, 0x19, 0x1E, 0x17, 0x10,
    0x05, 0x02, 0x0B, 0x0C, 0x21, 0x26, 0x2F, 0x28, 0x3D, 0x3A, 0x33, 0x34,
    0x4E, 0x49, 0x40, 0x47, 0x52, 0x55, 0x5C, 0x5B, 0x76, 0x71, 0x78, 0x7F,
    0x6A, 0x6D, 0x64, 0x63, 0x3E, 0x39, 0x30, 0x37, 0x22, 0x25, 0x2C, 0x2B,
    0x06, 0x01, 0x08, 0x0F, 0x1A, 0x1D, 0x14, 0x13, 0xAE, 0xA9, 0xA0, 0xA7,
    0xB2, 0xB5, 0xBC, 0xBB, 0x96, 0x91, 0x98, 0x9F, 0x8A, 0x8D, 0x84, 0x83,
    0xDE, 0xD9, 0xD0, 0xD7, 0xC2, 0xC5, 0xCC, 0xCB, 0xE6, 0xE1, 0xE8, 0xEF,
    0xFA, 0xFD, 0xF4, 0xF3
};

static struct efuse_info gvm8666b_efuse_info;
static unsigned char mem_repair_stat = 0xff;


static void gvm8666b_get_efuse_info(struct i2c_client *i2c, struct efuse_info *efuse_info)
{
    unsigned short addr = 0;
    unsigned short efuse_size = 0;
    unsigned short efuse_value = 0;

    efuse_size = sizeof(struct efuse_info) / sizeof(unsigned char);

    gvm8666b_write(i2c, 0x01ce, 0x9527);
    gvm8666b_write(i2c, 0x018a, 0x0004);
    gvm8666b_write(i2c, 0x018c, 0x1000);
    gvm8666b_write(i2c, 0x018c, 0x0000);
    gvm8666b_write(i2c, 0x018c, 0x6000);

    for (addr = 0; addr < efuse_size; addr++) {
        gvm8666b_write(i2c, 0x018c, 0x6000 | addr);
        gvm8666b_write(i2c, 0x018e, 0x0001);
        gvm8666b_write(i2c, 0x018c, 0x1000);
        gvm8666b_read(i2c, 0x0190, (unsigned char *)&efuse_value);
        *(unsigned char *)((unsigned char *)efuse_info + addr) = (unsigned char)efuse_value;
        // printk(KERN_ERR "efuse_info[0x%x] = 0x%x\n", addr, *(unsigned char *)((unsigned char *)efuse_info + addr));
    }

    gvm8666b_write(i2c, 0x018c, 0x3400);
    gvm8666b_write(i2c, 0x01ce, 0x2266);
}


static unsigned char gvm8666b_get_efuse_crc8(struct efuse_info *efuse_info, unsigned short len)
{
    unsigned char index;
    unsigned char addr = 0;
    unsigned char efuse_value = 0;
    unsigned char res_crc = 0;

    while (len--) {
        efuse_value = *(unsigned char *)((unsigned char *)efuse_info + addr);
        index = res_crc ^ efuse_value;
        res_crc = gvm8666b_crc8[index];
        addr++;
    }

    return (~res_crc);
}

static int gvm8666b_update_config(struct i2c_client *i2c)
{
    unsigned char efuse_calc_crc = 0;
    unsigned int cfg_size = 0;
    int i;

    gvm8666b_get_efuse_info(i2c, &gvm8666b_efuse_info);
    efuse_calc_crc = gvm8666b_get_efuse_crc8(&gvm8666b_efuse_info, sizeof(struct efuse_info) - 1);
    if (efuse_calc_crc != gvm8666b_efuse_info.cp_crc) {
        printk(KERN_ERR "efuse crc check failed, please check!");
        return -1;
    } else {
        printk(KERN_ERR "sensor efuse crc check success!\n");
    }

    cfg_size = sizeof(gvm8666b_1088_1488_30fps_mipi_init_regs) / sizeof(struct regval_list);
    for (i = 0; i < cfg_size; i++) {
        if (0x0326 == gvm8666b_1088_1488_30fps_mipi_init_regs[i].reg_num) {
            gvm8666b_1088_1488_30fps_mipi_init_regs[i].value = gvm8666b_efuse_info.clk_trim;
            printk(KERN_ERR "clk_trim = 0x%x\n", gvm8666b_1088_1488_30fps_mipi_init_regs[i].value);
        }
        else if (0x03fa == gvm8666b_1088_1488_30fps_mipi_init_regs[i].reg_num){
            gvm8666b_1088_1488_30fps_mipi_init_regs[i].value = gvm8666b_efuse_info.bg_trim;
            printk(KERN_ERR "bg_trim = 0x%x\n", gvm8666b_1088_1488_30fps_mipi_init_regs[i].value);
        }
        else if (0x0324 == gvm8666b_1088_1488_30fps_mipi_init_regs[i].reg_num) {
            gvm8666b_1088_1488_30fps_mipi_init_regs[i].value = gvm8666b_efuse_info.wdt_trim;
            printk(KERN_ERR "wdt_trim = 0x%x\n", gvm8666b_1088_1488_30fps_mipi_init_regs[i].value);
        }
    }

    return 0;
}

static int gvm8666b_continue_write(struct i2c_client *i2c, unsigned short reg, int len, unsigned char *val)
{
    unsigned char *buf = kmalloc(len+2, GFP_KERNEL);
    // unsigned char buf[0x800 + 2] = {0};
    buf[0] = reg >> 8;
    buf[1] = reg & 0xff;
    memcpy(buf+2, val, len);
    struct i2c_msg msg = {
        .addr   = i2c->addr,
        .flags  = 0,
        .len    = len + 2,
        .buf    = buf,
    };

    int ret = i2c_transfer(i2c->adapter, &msg, 1);
    if (ret > 0)
        ret = 0;

    kfree(buf);

    return ret;
}

static int gvm8666b_memory_repair(struct i2c_client *i2c, unsigned int mem_err_stat)
{
    int ret = 0;
    unsigned short mcu_mem_stat = 0x0f00;
    unsigned short sram_mem_stat = 0x0000;
    unsigned short mipi_mem_stat = 0x0002;

    gvm8666b_write(i2c, 0x01ce, 0x9527);
    gvm8666b_write(i2c, 0x01d0, 0x0001);

    if ((mem_err_stat & 0x0001) == 0x0001) {
        gvm8666b_write(i2c, 0x02D0, 0x0010);
        gvm8666b_write(i2c, 0x017E, 0x1000);
        m_msleep(3);
        gvm8666b_write(i2c, 0x017E, 0x0000);
        gvm8666b_write(i2c, 0x02D0, 0x0410);
        gvm8666b_write(i2c, 0x02D0, 0x0010);
        gvm8666b_write(i2c, 0x017E, 0x1000);
        m_msleep(3);

        gvm8666b_write(i2c, 0x02D0, 0x0020);
        gvm8666b_write(i2c, 0x017E, 0x1000);
        m_msleep(3);
        gvm8666b_write(i2c, 0x017E, 0x0000);
        gvm8666b_write(i2c, 0x02D0, 0x0820);
        gvm8666b_write(i2c, 0x02D0, 0x0020);
        gvm8666b_write(i2c, 0x017E, 0x1000);
        m_msleep(3);

        gvm8666b_write(i2c, 0x02D0, 0x0030);
        gvm8666b_write(i2c, 0x017E, 0x1000);
        m_msleep(3);
        gvm8666b_write(i2c, 0x017E, 0x0000);
        gvm8666b_write(i2c, 0x02D0, 0x1030);
        gvm8666b_write(i2c, 0x02D0, 0x0030);
        gvm8666b_write(i2c, 0x017E, 0x1000);
        m_msleep(3);
        gvm8666b_read(i2c, 0x0180, (unsigned char *)&mcu_mem_stat);
    }

    if ((mem_err_stat & 0x0002) == 0x0002) {
        gvm8666b_write(i2c, 0x02D0, 0x0040);
        gvm8666b_write(i2c, 0x017E, 0x0003);
        m_msleep(3);
        gvm8666b_write(i2c, 0x017E, 0x0000);
        gvm8666b_write(i2c, 0x02D0, 0x0100);
        gvm8666b_write(i2c, 0x02D0, 0x0000);
        gvm8666b_write(i2c, 0x017E, 0x0001);
        m_msleep(3);

        gvm8666b_write(i2c, 0x02D0, 0x0050);
        gvm8666b_write(i2c, 0x017E, 0x0003);
        m_msleep(3);
        gvm8666b_write(i2c, 0x017E, 0x0000);
        gvm8666b_write(i2c, 0x02D0, 0x0200);
        gvm8666b_write(i2c, 0x02D0, 0x0000);
        gvm8666b_write(i2c, 0x017E, 0x0002);
        m_msleep(3);
        gvm8666b_read(i2c, 0x0182, (unsigned char *)&sram_mem_stat);
    }

    if ((mem_err_stat & 0x0004) == 0x0004) {
        gvm8666b_write(i2c, 0x7860, 0x0000);
        gvm8666b_write(i2c, 0x7298, 0x0001);
        m_msleep(3);
        gvm8666b_write(i2c, 0x7298, 0x0000);
        gvm8666b_write(i2c, 0x7298, 0x0010);
        gvm8666b_write(i2c, 0x7298, 0x0000);
        gvm8666b_write(i2c, 0x7298, 0x0001);
        m_msleep(3);
        gvm8666b_read(i2c, 0x729C, (unsigned char *)&mipi_mem_stat);
    }

    gvm8666b_write(i2c, 0x017E, 0x0000);
    gvm8666b_write(i2c, 0x7298, 0x0000);

    if ((mcu_mem_stat == 0x0f00) && (sram_mem_stat == 0x0000)
        && (mipi_mem_stat == 0x0002)) {
            printk(KERN_ERR "success to repair mcu & adc sram & mipi memory space!\n");
    } else {
        printk(KERN_ERR "fail to repair mcu & adc sram & mipi memory space!\n");
        ret = -1;
    }
    return ret;
}

static int gvm8666b_memory_check(struct i2c_client *i2c)
{
    int ret = 0;

    mem_repair_stat = gvm8666b_efuse_info.mbist_repair;
    printk(KERN_ERR "mem_repair_stat = 0x%x\n", mem_repair_stat);

    if ((mem_repair_stat & 0x0001) == 0x0001) {
        printk(KERN_ERR "an error in mcu memory space!\n");
    }

    if ((mem_repair_stat & 0x0002) == 0x0002) {
        printk(KERN_ERR "an error in sram memory space!\n");
    }

    if ((mem_repair_stat & 0x0004) == 0x0004) {
        printk(KERN_ERR "an error in mipi memory space!\n");
    }

    if ((mem_repair_stat & 0x0007) == 0x0007) {
        ret = gvm8666b_memory_repair(i2c, mem_repair_stat);
    } else {
        printk(KERN_ERR "mcu & sram & mipi memory space is normal!\n");
    }

    return ret;
}

static int gvm8666b_download_fw(struct i2c_client *i2c)
{
    int ret = 0;

    ret = gvm8666b_continue_write(i2c, 0x2000, 0x800, g_sensor_fw);
    if (ret < 0) {
        printk(KERN_ERR "fail to update firmware's first block");
        return ret;
    }

    ret = gvm8666b_continue_write(i2c, 0x2800, 0x800, g_sensor_fw + 0x800);
    if (ret < 0) {
        printk(KERN_ERR "fail to update firmware's second block");
        return ret;
    }

    ret = gvm8666b_continue_write(i2c, 0x3000, 0x800, g_sensor_fw + 0x1000);
    if (ret < 0) {
        printk(KERN_ERR "fail to update firmware's third block");
        return ret;
    }

    ret = gvm8666b_continue_write(i2c, 0x3800, 0x600, g_sensor_fw + 0x1800);
    if (ret < 0) {
        printk(KERN_ERR "fail to update firmware's first block");
        return ret;
    }

    ret = gvm8666b_continue_write(i2c, 0x3e00, 0x200, g_sensor_fw + 0x1e00);
    if (ret < 0) {
        printk(KERN_ERR "fail to update firmware's first block");
        return ret;
    }

    return ret;
}

static int gvm8666b_check_fw_valid(struct i2c_client *i2c)
{
    int ret = 0;
    unsigned short fw_check_code = 0;

    ret = gvm8666b_read(i2c, 0x0860, (unsigned char *)&fw_check_code);
    if (ret < 0) {
        printk(KERN_ERR "fail to read image sensor firmware's block\n");
        return -1;
    }
    printk(KERN_ERR "image sensor firmware check code 0x%x\n", fw_check_code);
    if (fw_check_code != GT_FW_CHK_CODE) {
        printk(KERN_ERR "image sensor firmware check code is invalid\n");
        ret = -1;
    } else {
        printk(KERN_ERR "image sensor firmware check success\n");
    }
    return ret;
}

static int gvm8666b_img_sensor_update_fw(struct i2c_client *i2c)
{
    int ret = 0;
    int try_time = 0;
    int read_cnt;

    gvm8666b_write(i2c, 0x014e, 0x0001);
    printk(KERN_ERR "the first time start to repair MCU,SRAM and MIPI memory space !!!\n");
    ret = gvm8666b_memory_check(i2c);
    if (ret) {
        printk(KERN_ERR "memory check failed\n");
        return ret;
    }

    for (try_time = 0; try_time < 3; try_time++) {
        printk(KERN_ERR "the %d time to update sensor firmware !!!", (try_time + 1));
        gvm8666b_write(i2c, 0x01ce, 0x9527);
        gvm8666b_write(i2c, 0x1016, 0x2000);
        gvm8666b_write(i2c, 0x01d0, 0x0003);

        ret = gvm8666b_download_fw(i2c);
        if (ret < 0) {
            printk(KERN_ERR "gvm8666b download firmware fail.\n");
            return ret;
        }
        gvm8666b_write(i2c, 0x01d0, 0x0000);
        gvm8666b_write(i2c, 0x1016, 0x0000);

        for (read_cnt = 0; read_cnt < 3; read_cnt++) {
            m_msleep(3);
            ret = gvm8666b_check_fw_valid(i2c);
            if (ret) {
                continue;
            } else {
                break;
            }
        }
        gvm8666b_write(i2c, 0x01ce, 0x2266);
        if (ret < 0) {
            printk(KERN_ERR "Fail to the %d time update firmware !!!\n", (try_time + 1));
        } else {
            printk(KERN_ERR "success to update firmware\n");
            break;
        }
    }

    return ret;
}

//read eeprom
static int gt_sensor_read_block_otp_a16_d8(struct i2c_client *client,
					unsigned char dev_addr,
					unsigned short eeprom_addr,
					unsigned char *buf, int len)
{
    unsigned char reg[3];
    struct i2c_msg msg[2];

    reg[0] = (eeprom_addr & 0xff00) >> 8;
    reg[1] = eeprom_addr & 0xff;
    reg[2] = 0xee;

    msg[0].addr = dev_addr >> 1;
    msg[0].flags = 0;
    msg[0].len = 2;
    msg[0].buf = reg;

    msg[1].addr = dev_addr >> 1;
    msg[1].flags = I2C_M_RD;
    msg[1].len = len;
    msg[1].buf = buf;

    return (i2c_transfer(client->adapter, msg, 2) == 2);
}

/**
 * gx_crc8 - crc logic
 * @eeprom_info: pointer of the eeprom_info which has been read from EEPROM
 * @eeprom_size: total Bytes of specific eeprom_info sector
 */
static uint8_t gx_crc8(uint8_t *eeprom_info, uint16_t eeprom_length)
{
    uint8_t index    = 0x0;
    uint16_t addr     = 0x0;
    uint8_t chkValue = 0x0;
    uint8_t resCRC   = 0x0;

    while(eeprom_length--) {
        chkValue = *(uint8_t *)((uint8_t *) eeprom_info + addr);
        index    = resCRC ^ chkValue;
        resCRC   = gvm8666b_crc8[index];
        addr++;
    }

    return (~resCRC);
}

static int gx_eeprom_phase_shift_crc_chk(uint8_t *eeprom_info, uint16_t eeprom_size)
{
    int rc = 0;
    uint8_t phase_expo_crc = 0x00;

    phase_expo_crc = gx_crc8(eeprom_info, eeprom_size -1);
    if (eeprom_info[eeprom_size-1] != phase_expo_crc) {
        printk(KERN_ERR "phase_expo_crc check fail !\n");
        return -EFAULT;
    } else {
        printk("[GX] phase_expo_crc chk success !\n");
        g_eeprom_crc_state = 0x1;
    }

    return rc;
}

/**
 * gx_update_exphase - update the initial phase_shift config and initial best_exp_time for sensor
 * @eeprom_info: pointer of the phase_shift_data which has been read from EEPROM
 * @eeprom_size: total Bytes of phase_shift_data sector (0x25  = 37Bypte)
 *
 * Must be called  before sensor streamimg On triggered
 */
int gx_update_exphase(struct i2c_client *client, uint8_t *eeprom_info, uint16_t size)
{
    int rc = 0;
    int i  = 0;
    unsigned short start_addr = 0x82A;
    unsigned short reg_value  = 0x0;
    unsigned char expo_hf[2];
    unsigned char expo_lf[2];

    if (NULL == eeprom_info) {
        return -1;
    }

    for (i = 0; i < 0x14;) {
        if (3 == i || 8 == i || 13 == i || 18 == i) {
            reg_value = (uint32_t)(eeprom_info[i] | eeprom_info[i+1] <<8);
            i += 2;
        } else {
            reg_value = (uint32_t)eeprom_info[i];
            i++;
        }

        rc = gvm8666b_write(client, start_addr, reg_value);
        if ( rc < 0) {
            printk(KERN_ERR "[GX] Failed to write I2C settings:----settings:0x%x", start_addr);
            return rc;
        }
        start_addr += 2;
    }

    //need to check endian 20211011
    reg_value = (uint32_t)(eeprom_info[i] | (eeprom_info[i+1] << 8));
    printk("[GX] expo_time = 0x%x. ----comes from eeprom\n", reg_value);

    gvm8666b_write(client, 0x81a, (0x8000 | reg_value));
    if ( rc < 0) {
        printk(KERN_ERR "[GX] failed to update expohf");
        return rc;
    }
    rc = gvm8666b_write(client, 0x81a,  (0x4000 | reg_value));
    if ( rc < 0) {
        printk(KERN_ERR "[GX] failed to update expolf");
        return rc;
    }
    msleep(30);
    gvm8666b_read(client, 0x81C, expo_hf);
    gvm8666b_read(client, 0x81E, expo_lf);

    i += 6;
    reg_value = (uint32_t)(eeprom_info[i] | (eeprom_info[i+1] << 8));
    rc = gvm8666b_write(client, 0x483C, reg_value);
    if ( rc < 0) {
        printk(KERN_ERR "[GX] failed to write I2C settings:----settings:0x483C");
        return rc;
    }
    i += 2;

    reg_value = (uint32_t)(eeprom_info[i] | (eeprom_info[i+1] << 8));
    rc = gvm8666b_write(client, 0x4838, reg_value);
    if ( rc < 0) {
        printk(KERN_ERR "[GX] failed to write I2C settings:----settings:0x4838");
        return rc;
    }

    start_addr = 0x82A;
    for (i = 0; i < 0x10;i++) {
        unsigned char tmp[2];
        gvm8666b_read(client, start_addr, tmp);
        reg_value = tmp[1] << 8 | tmp[0];
        start_addr += 2;
    }

    return 0;
}

static int gvm8666b_update_phase_shift(struct i2c_client *i2c_dev)
{
    //read phase_shift and best exposureTime from eeprom
    int ret = gt_sensor_read_block_otp_a16_d8(i2c_dev, 
            GT_EEPROM_I2C_READ_ADDR, 0x143, 
            g_eeprom_phase_shift_info, 
            GX_EEPROM_PHASE_SHIFT_SIZE);
    if (ret < 0) {
        printk(KERN_ERR "%s: fail to read g_eeprom_phase_shift_info\n", __func__);
        return ret;
    }

    //CRC Check
    gx_eeprom_phase_shift_crc_chk(g_eeprom_phase_shift_info, GX_EEPROM_PHASE_SHIFT_SIZE);
    if (ret < 0) {
        printk(KERN_ERR "%s: fail to phase_expo_crc check\n", __func__);
        return ret;
    }

    //Update initial phase_shift and initial best_exp_time
    gx_update_exphase(i2c_dev, g_eeprom_phase_shift_info, 0x25);
    if (ret < 0) {
        printk(KERN_ERR "%s: fail to update the initial phase_shift config and initial best_exp_time\n", __func__);
        return ret;
    }

    return 0;
}

static int gvm8666b_read_eeprom_info(struct i2c_client *i2c_dev)
{
    //read eeprom data first 32KB
    int ret = gt_sensor_read_block_otp_a16_d8(i2c_dev,
            GT_EEPROM_I2C_READ_ADDR, 0x00,
            g_eeprom_info,
            32*1024);
    //read eeprom data last 32KB
    ret = gt_sensor_read_block_otp_a16_d8(i2c_dev,
            GT_EEPROM_I2C_READ_ADDR, 0x8000,
            g_eeprom_info + 0x8000,
            32*1024);

    if (ret < 0) {
        printk(KERN_ERR "%s: fail to read eeprom info\n", __func__);
        return ret;
    }

    struct file *fd = filp_open("/tmp/eeprom.bin", O_CREAT | O_WRONLY | O_TRUNC, 0766);
    if (fd < 0) {
        printk(KERN_ERR "Failed to open /tmp/snap.raw\n");
        ret = -1;
        return ret;
    }

    mm_segment_t old_fs = get_fs();
    set_fs(KERNEL_DS);
    loff_t *pos = &(fd->f_pos);
    vfs_write(fd, g_eeprom_info, GT_EEPROM_SIZE, pos);
    filp_close(fd, NULL);
    set_fs(old_fs);

    return 0;
}
