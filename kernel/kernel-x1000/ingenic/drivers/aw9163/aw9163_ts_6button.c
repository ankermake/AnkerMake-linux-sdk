/**************************************************************************
*  aw9163_ts_3button.c
* 
*  Create Date :
* 
*  Modify Date : 
*
*  Create by   : AWINIC Technology CO., LTD
*
*  Version     : 1.0.0 , 2016/03/22
* 
**************************************************************************/
#include <linux/device.h>
#include <linux/kernel.h>
#include <linux/i2c.h>
#include <linux/input.h>
#include <linux/of.h>
#include <linux/of_address.h>
#include <linux/of_irq.h>
#include <linux/gpio.h>
#include <linux/of_gpio.h>
#include <linux/miscdevice.h>
#include <linux/interrupt.h>
#include <linux/delay.h>
#include <linux/irq.h>
#include <linux/firmware.h>
#include <linux/of_device.h>
#include <linux/workqueue.h>
#include <linux/slab.h>
#include <linux/fs.h>
#include <linux/proc_fs.h>
#include <linux/uaccess.h>
#include <asm/io.h>
#include <linux/init.h>
#include <linux/pci.h>
#include <linux/dma-mapping.h>
#include <linux/gameport.h>
#include <linux/moduleparam.h>
#include <linux/mutex.h>
#include <linux/module.h>
#include "aw9163_reg.h"
#include "aw9163_para.h"
#include <linux/wakelock.h>
#include <linux/atomic.h>

static int keytable[6] = {
    KEY_HOME,
    KEY_VOLUMEDOWN,
    KEY_VOLUMEUP,
    KEY_UP,
    KEY_PHONE,
    -1,
};

/****************************************************************
*
* Marco
*
***************************************************************/
#define AW9163_TS_NAME        "aw9163_ts"

/****************************************************************
*
* Auto Calibration
*
***************************************************************/
#ifdef AW_AUTO_CALI
#define CALI_NUM        4
#define CALI_RAW_MIN    1000
#define CALI_RAW_MAX    3000

#define AW9163_IIC_ADDRESS   0x2C

static unsigned char cali_flag = 0;
static unsigned char cali_num = 0;
static unsigned char cali_cnt = 0;
static unsigned char cali_used = 0;
static unsigned char old_cali_dir[6];    //    0: no cali        1: ofr pos cali        2: ofr neg cali
static unsigned int old_ofr_cfg[6];
static long Ini_sum[6];
#endif

int gpioToIrq;
/****************************************************************
*
* Touch Key Driver
*
***************************************************************/
struct aw9163_ts_data {
    struct input_dev    *input_dev;
    struct work_struct     eint_work;
    struct device_node *irq_node;
    int irq;
};

static struct aw9163_ts_data *aw9163_ts;
static struct i2c_client *aw9163_i2c_client;

/****************************************************************
*
* Touch process variable
*
***************************************************************/
static unsigned char suspend_flag = 0 ; //0: normal; 1: sleep
static int debug_level=0;
static int WorkMode = 1 ; //1: sleep, 2: normal


/****************************************************************
*
* PDN power control
*
***************************************************************/
struct aw9163_io_data {
    int aw9163_pdn;
    int aw9163_int;
    int bus_number;
};

struct pinctrl *aw9163ctrl = NULL;
struct pinctrl_state *aw9163_int_pin = NULL;
struct pinctrl_state *aw9163_pdn_high = NULL;
struct pinctrl_state *aw9163_pdn_low = NULL;

static int aw9163_gpio_pdn = 0;
static int aw9163_gpio_int = 0;

#if 1
static int aw9163_gpio_init(struct platform_device *pdev){
    int err;
    if(gpio_is_valid(aw9163_gpio_pdn)){
        err = gpio_request(aw9163_gpio_pdn, "aw9163_gpio_pdn");

        if(err){
            printk("unable to request aw9163_gpio_pdn \n"); 
        }

        err = gpio_direction_output(aw9163_gpio_pdn, 1);

        if(err){
            printk("unable to set direction aw9163_gpio_pdn \n");
        }
    }else{
        printk("gpio not provided\n");
    }

    //gpio_set_value(aw9163_gpio_pdn, 1);

    printk  ("%s,success\n",__func__);
    return 0;
}
#else
static int aw9163_gpio_init(struct platform_device *pdev)
{
    int ret = 0;

    aw9163ctrl = devm_pinctrl_get(&pdev->dev);
    if (IS_ERR(aw9163ctrl)) {
        dev_err(&pdev->dev, "Cannot find camera pinctrl!");
        ret = PTR_ERR(aw9163ctrl);
        printk("%s devm_pinctrl_get fail!\n", __func__);
    }
    aw9163_pdn_high = pinctrl_lookup_state(aw9163ctrl, "aw9163_pdn_high");
    if (IS_ERR(aw9163_pdn_high)) {
        ret = PTR_ERR(aw9163_pdn_high);
        printk("%s : pinctrl err, aw9163_pdn_high\n", __func__);
    }

    aw9163_pdn_low = pinctrl_lookup_state(aw9163ctrl, "aw9163_pdn_low");
    if (IS_ERR(aw9163_pdn_low)) {
        ret = PTR_ERR(aw9163_pdn_low);
        printk("%s : pinctrl err, aw9163_pdn_low\n", __func__);
    }

    printk("%s success\n", __func__);
    return ret;
}
#endif
static void aw9163_ts_pwron(void)
{
    printk("%s enter\n", __func__);
    //pinctrl_select_state(aw9163ctrl, aw9163_pdn_low);
    if (gpio_is_valid(aw9163_gpio_pdn)) 
        gpio_set_value(aw9163_gpio_pdn, 1);
    msleep(2);
    //pinctrl_select_state(aw9163ctrl, aw9163_pdn_high);
    if (gpio_is_valid(aw9163_gpio_pdn))
        gpio_set_value(aw9163_gpio_pdn, 1);
    msleep(2);
    printk("%s out\n", __func__);
}

static void aw9163_ts_pwroff(void)
{
    printk("%s enter\n", __func__);
    //pinctrl_select_state(aw9163ctrl, aw9163_pdn_low);
    if (gpio_is_valid(aw9163_gpio_pdn))
        gpio_set_value(aw9163_gpio_pdn, 0);
    msleep(2);
    printk("%s out\n", __func__);
}

static void aw9163_ts_config_pins(void)
{
    printk("%s enter\n", __func__);
    aw9163_ts_pwron();
    printk("%s out\n", __func__);
}

/****************************************************************
*
* i2c write and read
*
***************************************************************/
static unsigned int i2c_write_reg(unsigned char addr, unsigned int reg_data)
{
    int ret;
    u8 wdbuf[512] = {0};

    struct i2c_msg msgs[] = {
        {
            .addr    = aw9163_i2c_client->addr,
            .flags    = 0,
            .len    = 3,
            .buf    = wdbuf,
        },
    };

    if(NULL == aw9163_i2c_client) {
        pr_err("msg %s aw9163_i2c_client is NULL\n", __func__);
        return -1;
    }
    wdbuf[0] = addr;
    wdbuf[2] = (unsigned char)(reg_data & 0x00ff);
    wdbuf[1] = (unsigned char)((reg_data & 0xff00)>>8);

    ret = i2c_transfer(aw9163_i2c_client->adapter, msgs, 1);
    if (ret < 0)
    pr_err("msg %s i2c read error: %d\n", __func__, ret);

    return ret;

}


static unsigned int i2c_read_reg(unsigned char addr)
{
    int ret;
    u8 rdbuf[512] = {0};
    unsigned int getdata;

    struct i2c_msg msgs[] = {
        {
            .addr    = aw9163_i2c_client->addr,
            .flags    = 0,
            .len    = 1,
            .buf    = rdbuf,
        },
        {
            .addr    = aw9163_i2c_client->addr,
            .flags    = I2C_M_RD,
            .len    = 2,
            .buf    = rdbuf,
        },
    };

    if(NULL == aw9163_i2c_client) {
        pr_err("msg %s aw9163_i2c_client is NULL\n", __func__);
        return -1;
    }
    rdbuf[0] = addr;

    ret = i2c_transfer(aw9163_i2c_client->adapter, msgs, 2);
    if (ret < 0)
    pr_err("msg %s i2c read error: %d\n", __func__, ret);

    getdata=rdbuf[0] & 0x00ff;
    getdata<<= 8;
    getdata |=rdbuf[1];

    return getdata;
}


/****************************************************************
*
* aw9163 initial register @ mobile active
*
***************************************************************/
static void aw9163_normal_mode_cfg(void)
{
    i2c_write_reg(GCR,0x0000);            // disable chip

    // LED config
    i2c_write_reg(LER1,N_LER1);            // LED enable
    i2c_write_reg(LER2,N_LER2);            // LED enable

    //i2c_write_reg(CTRS1,0x0000);        // LED control RAM or I2C
    //i2c_write_reg(CTRS2,0x0000);        // LED control RAM or I2C
    i2c_write_reg(IMAX1,N_IMAX1);        // LED MAX light setting
    i2c_write_reg(IMAX2,N_IMAX2);        // LED MAX light setting
    i2c_write_reg(IMAX3,N_IMAX3);        // LED MAX light setting
    i2c_write_reg(IMAX4,N_IMAX4);        // LED MAX light setting
    i2c_write_reg(IMAX5,N_IMAX5);        // LED MAX light setting

    i2c_write_reg(LCR,N_LCR);            // LED effect control
    i2c_write_reg(IDLECR,N_IDLECR);        // IDLE time setting

    // cap-touch config
    i2c_write_reg(SLPR,N_SLPR);            // touch key enable
    i2c_write_reg(SCFG1,N_SCFG1);        // scan time setting
    i2c_write_reg(SCFG2,N_SCFG2);        // bit0~3 is sense seting

    i2c_write_reg(OFR1,0x1010);            // offset
    i2c_write_reg(OFR2,0x1010);            // offset
    i2c_write_reg(OFR3,0x1010);            // offset
    //i2c_write_reg(OFR1,N_OFR1);            // offset
    //i2c_write_reg(OFR2,N_OFR2);            // offset
    //i2c_write_reg(OFR3,N_OFR3);            // offset

    i2c_write_reg(THR0, N_THR0);        // S1 press thred setting
    i2c_write_reg(THR1, N_THR1);        // S2 press thred setting
    i2c_write_reg(THR2, N_THR2);        // S3 press thred setting
    i2c_write_reg(THR3, N_THR3);        // S4 press thred setting
    i2c_write_reg(THR4, N_THR4);        // S5 press thred setting
    i2c_write_reg(THR5, N_THR5);        // S6 press thred setting

    i2c_write_reg(SETCNT,N_SETCNT);        // debounce
    i2c_write_reg(BLCTH,N_BLCTH);        // base trace rate 

    //i2c_write_reg(AKSR,N_AKSR);            // AKS 
    i2c_write_reg(AKSR,0x001f);            // AKS 
    #ifndef AW_AUTO_CALI
    i2c_write_reg(INTER,N_INTER);         // signel click interrupt 
    #else
    //i2c_write_reg(INTER,0x0080);         // frame interrupt 
    i2c_write_reg(INTER,0x001f);         // frame interrupt 
    #endif

    i2c_write_reg(MPTR,N_MPTR);            // Long Press Time 
    i2c_write_reg(GDTR,N_GDTR);            // gesture time setting
    i2c_write_reg(GDCFGR,N_GDCFGR);        // gesture key select
    i2c_write_reg(TAPR1,N_TAPR1);        // double click 1
    i2c_write_reg(TAPR2,N_TAPR2);        // double click 2
    i2c_write_reg(TDTR,N_TDTR);            // double click time

#ifndef AW_AUTO_CALI
i2c_write_reg(GIER,N_GIER);            // gesture and double click enable
#else
i2c_write_reg(GIER,0x0000);            // gesture and double click disable
#endif    

    // Chip Enbale
    i2c_write_reg(GCR,N_GCR);            // LED enable and touch scan enable

    WorkMode = 2;
    printk("%s Finish\n", __func__);

}


/****************************************************************
*
* aw9163 initial register @ mobile sleep
*
***************************************************************/
static void aw9163_sleep_mode_cfg(void)
{
    i2c_write_reg(GCR,0x0000);           // disable chip

    // LED config
    i2c_write_reg(LER1,S_LER1);            // LED enable
    i2c_write_reg(LER2,S_LER2);            // LED enable

    i2c_write_reg(LCR,S_LCR);            // LED effect control
    i2c_write_reg(IDLECR,S_IDLECR);        // IDLE time setting

    i2c_write_reg(IMAX1,S_IMAX1);        // LED MAX light setting
    i2c_write_reg(IMAX2,S_IMAX2);        // LED MAX light setting
    i2c_write_reg(IMAX3,S_IMAX3);        // LED MAX light setting
    i2c_write_reg(IMAX4,S_IMAX4);        // LED MAX light setting
    i2c_write_reg(IMAX5,S_IMAX5);        // LED MAX light setting

    // cap-touch config
    i2c_write_reg(SLPR,S_SLPR);            // touch key enable
    i2c_write_reg(SCFG1,S_SCFG1);        // scan time setting
    i2c_write_reg(SCFG2,S_SCFG2);        // bit0~3 is sense seting

    i2c_write_reg(OFR1,S_OFR1);            // offset
    i2c_write_reg(OFR2,S_OFR2);            // offset
    i2c_write_reg(OFR3,S_OFR3);            // offset

    i2c_write_reg(THR0, S_THR0);        // S1 press thred setting
    i2c_write_reg(THR1, S_THR1);        // S2 press thred setting
    i2c_write_reg(THR2, S_THR2);        // S3 press thred setting
    i2c_write_reg(THR3, S_THR3);        // S4 press thred setting
    i2c_write_reg(THR4, S_THR4);        // S5 press thred setting
    i2c_write_reg(THR5, S_THR5);        // S6 press thred setting

    i2c_write_reg(SETCNT,S_SETCNT);        // debounce
    i2c_write_reg(IDLECR, S_IDLECR);    // idle mode
    i2c_write_reg(BLCTH,S_BLCTH);        // base speed setting

    i2c_write_reg(AKSR,S_AKSR);            // AKS

    #ifndef AW_AUTO_CALI
    i2c_write_reg(INTER,S_INTER);         // signel click interrupt 
    #else
    //i2c_write_reg(INTER,0x0080);         // signel click interrupt 
    i2c_write_reg(INTER,0x001f);         // signel click interrupt 
    #endif

    i2c_write_reg(GDCFGR,S_GDCFGR);        // gesture key select
    i2c_write_reg(TAPR1,S_TAPR1);        // double click 1
    i2c_write_reg(TAPR2,S_TAPR2);        // double click 2

    i2c_write_reg(TDTR,S_TDTR);            // double click time
    #ifndef AW_AUTO_CALI
    i2c_write_reg(GIER,S_GIER);            // gesture and double click enable
#else
i2c_write_reg(GIER,0x0000);            // gesture and double click disable
#endif

    //Chip Enable
    i2c_write_reg(GCR, S_GCR);           // enable chip sensor function

    WorkMode = 1;
    printk("%s Finish\n", __func__);
    }


/****************************************************************
*
* aw9163 led 
*
***************************************************************/
static void aw9163_led_off(void)
{
    //Disable LED Module
    unsigned int reg;
    reg = i2c_read_reg(GCR);
    reg &= 0xFFFE;
    i2c_write_reg(GCR, reg);         // GCR-Disable LED Module
}

static void aw9163_led_on(void)
{
    //Disable LED Module
    unsigned int reg;
    reg = i2c_read_reg(GCR);
    reg &= 0xFFFE;
    i2c_write_reg(GCR, reg);         // GCR-Disable LED Module

    //LED Config
    i2c_write_reg(IMAX1,0x0100);     // IMAX1-LED1 Current
    i2c_write_reg(IMAX2,0x0101);     // IMAX2-LED2~LED3 Current
    i2c_write_reg(LER1,0x0054);     // LER1-LED1~LED3 Enable
    i2c_write_reg(CTRS1,0x0054);     // CTRS1-LED1~LED3: i2c Control

    //Enable LED Module
    reg |= 0x0001;
    i2c_write_reg(GCR,reg);         // GCR-Enable LED Module

    // LED Control
    i2c_write_reg(CMDR,0xBFFF);     // CMDR-LED1~LED3 PWM=0xFF
}


/****************************************************************
*
* aw9xxx Auto Calibration
*
***************************************************************/
#ifdef AW_AUTO_CALI
static unsigned char aw9xxx_auto_cali(void)
{
    unsigned char i;
    unsigned char cali_dir[6];

    unsigned int buf[6];
    unsigned int ofr_cfg[6];
    unsigned int sen_num;

    if(cali_num == 0){
        ofr_cfg[0] = i2c_read_reg(0x13);
        ofr_cfg[1] = i2c_read_reg(0x14);
        ofr_cfg[2] = i2c_read_reg(0x15);
    }else{
        for(i=0; i<3; i++){
            ofr_cfg[i] = old_ofr_cfg[i];
        }
    }

    i2c_write_reg(0x1e,0x3);
    for(i=0; i<6; i++){
        buf[i] = i2c_read_reg(0x36+i);
    }
    sen_num = i2c_read_reg(0x02);        // SLPR

    for(i=0; i<6; i++) 
    Ini_sum[i] = (cali_cnt==0)? (0) : (Ini_sum[i] + buf[i]);

    if(cali_cnt==4){
        for(i=0; i<6; i++){
            if((sen_num & (1<<i)) == 0)    {    // sensor used
                if((Ini_sum[i]>>2) < CALI_RAW_MIN){
                    if((i%2) && ((ofr_cfg[i>>1]&0xFF00)==0x1000)){                    // 0x10** -> 0x00**
                        ofr_cfg[i>>1] = ofr_cfg[i>>1] & 0x00FF;
                        cali_dir[i] = 2;
                    }else if((i%2) && ((ofr_cfg[i>>1]&0xFF00)==0x0000)){            // 0x00**    no calibration
                        cali_dir[i] = 0;
                    }else if (((i%2)==0) && ((ofr_cfg[i>>1]&0x00FF)==0x0010)){        // 0x**10 -> 0x**00
                        ofr_cfg[i>>1] = ofr_cfg[i>>1] & 0xFF00;
                        cali_dir[i] = 2;
                    }else if (((i%2)==0) && ((ofr_cfg[i>>1]&0x00FF)==0x0000)){        // 0x**00 no calibration
                        cali_dir[i] = 0;
                    }else{
                        ofr_cfg[i>>1] = ofr_cfg[i>>1] - ((i%2)? (1<<8):1);
                        cali_dir[i] = 2;
                    }
                }else if((Ini_sum[i]>>2) > CALI_RAW_MAX){
                    if((i%2) && ((ofr_cfg[i>>1]&0xFF00)==0x1F00)){    // 0x1F** no calibration
                        cali_dir[i] = 0;
                    }else if (((i%2)==0) && ((ofr_cfg[i>>1]&0x00FF)==0x001F)){    // 0x**1F no calibration
                        cali_dir[i] = 0;
                    }else{
                        ofr_cfg[i>>1] = ofr_cfg[i>>1] + ((i%2)? (1<<8):1);
                        cali_dir[i] = 1;
                    }
                }else{
                    cali_dir[i] = 0;
                }

                if(cali_num > 0){
                    if(cali_dir[i] != old_cali_dir[i]){
                        cali_dir[i] = 0;
                        ofr_cfg[i>>1] = old_ofr_cfg[i>>1];
                    }
                }
            }
        }

        cali_flag = 0;
        for(i=0; i<6; i++){
            if((sen_num & (1<<i)) == 0)    {    // sensor used
                if(cali_dir[i] != 0){
                    cali_flag = 1;
                }
            }
        }
        if((cali_flag==0) && (cali_num==0)){
            cali_used = 0;
        }else{
            cali_used = 1;
        }

        if(cali_flag == 0){
            cali_num = 0;
            cali_cnt = 0;
            return 0;
        }

        i2c_write_reg(GCR, 0x0000);
        for(i=0; i<3; i++){
            i2c_write_reg(OFR1+i, ofr_cfg[i]);
        }
        i2c_write_reg(GCR, 0x0003);

        if(cali_num == (CALI_NUM -1)){    // no calibration
            cali_flag = 0;
            cali_num = 0;
            cali_cnt = 0;
            return 0;
        }

        for(i=0; i<6; i++){
            old_cali_dir[i] = cali_dir[i];
        }

        for(i=0; i<3; i++){
            old_ofr_cfg[i] = ofr_cfg[i];
        }

        cali_num ++;
        }

    if(cali_cnt < 4){
        cali_cnt ++;
    }else{
        cali_cnt = 0;
    }

    return 1;
}
#endif

/****************************************************************
*
* aw9163 touch key report
*
***************************************************************/
static void aw9163_left_slip(void)
{
    printk("aw9163 left slip \n");
    input_report_key(aw9163_ts->input_dev, KEY_F1, 1);
    input_sync(aw9163_ts->input_dev);
    input_report_key(aw9163_ts->input_dev, KEY_F1, 0);
    input_sync(aw9163_ts->input_dev);
}

static void aw9163_right_slip(void)
{
    printk("aw9163 right slip \n");
    input_report_key(aw9163_ts->input_dev, KEY_F2, 1);
    input_sync(aw9163_ts->input_dev);
    input_report_key(aw9163_ts->input_dev, KEY_F2, 0);
    input_sync(aw9163_ts->input_dev);
}

static void aw9163_left_double(void)
{
    printk("aw9163 Left double click \n");
    input_report_key(aw9163_ts->input_dev, KEY_PREVIOUSSONG, 1);
    input_sync(aw9163_ts->input_dev);
    input_report_key(aw9163_ts->input_dev, KEY_PREVIOUSSONG, 0);
    input_sync(aw9163_ts->input_dev);
}

static void aw9163_center_double(void)
{
    printk("aw9163 Center double click \n");
    input_report_key(aw9163_ts->input_dev, KEY_F3, 1);
    input_sync(aw9163_ts->input_dev);
    input_report_key(aw9163_ts->input_dev, KEY_F3, 0);
    input_sync(aw9163_ts->input_dev);
}

static void aw9163_right_double(void)
{
    printk("aw9163 Right_double click \n");
    input_report_key(aw9163_ts->input_dev, KEY_NEXTSONG, 1);
    input_sync(aw9163_ts->input_dev);
    input_report_key(aw9163_ts->input_dev, KEY_NEXTSONG, 0);
    input_sync(aw9163_ts->input_dev);
}

static void aw9163_left_press(void)
{
    input_report_key(aw9163_ts->input_dev, KEY_MENU, 1);
    input_sync(aw9163_ts->input_dev);
    printk("aw9163 left press \n");
}

static void aw9163_center_press(void)
{
    printk("aw9163 center press \n");
    input_report_key(aw9163_ts->input_dev, KEY_HOMEPAGE, 1);
    input_sync(aw9163_ts->input_dev);
}

static void aw9163_right_press(void)
{
    input_report_key(aw9163_ts->input_dev, KEY_BACK, 1);
    input_sync(aw9163_ts->input_dev);
    printk("aw9163 right press \n");
}

static void aw9163_left_release(void)
{
    input_report_key(aw9163_ts->input_dev, KEY_MENU, 0);
    input_sync(aw9163_ts->input_dev);
    printk("aw9163 left release\n");
}

static void aw9163_center_release(void)
{
    input_report_key(aw9163_ts->input_dev, KEY_HOMEPAGE, 0);
    input_sync(aw9163_ts->input_dev);
    printk("aw9163 center release \n");
}

static void aw9163_right_release(void)
{
    input_report_key(aw9163_ts->input_dev, KEY_BACK, 0);
    input_sync(aw9163_ts->input_dev);
    printk("aw9163 right release \n");
}


/****************************************************************
*
* Function : Cap-touch main program @ mobile sleep 
*            wake up after double-click/right_slip/left_slip
*
***************************************************************/
static void aw9163_sleep_mode_proc(void)
{
    unsigned int buff1;

    printk("%s Enter\n", __func__);

    #ifdef AW_AUTO_CALI
    if(cali_flag){
        aw9xxx_auto_cali();
        if(cali_flag == 0){    
            if(cali_used){
                i2c_write_reg(GCR,0x0000);     // disable chip
            }
            i2c_write_reg(INTER,S_INTER);
            i2c_write_reg(GIER,S_GIER);
            if(cali_used){
                i2c_write_reg(GCR,S_GCR);     // enable chip
            }
        }
        return ;
    }
    #endif

    if(debug_level == 0){
        buff1=i2c_read_reg(0x2e);            //read gesture interupt status
        if(buff1 == 0x10){
            aw9163_center_double();
        }else if(buff1 == 0x01){
            aw9163_right_slip();
        }else if (buff1 == 0x02){
            aw9163_left_slip();
        }
    }
}

/****************************************************************
*
* Function : Cap-touch main pragram @ mobile normal state
*            press/release
*
***************************************************************/

static void aw9163_normal_mode_proc(void)
{
    unsigned int key_state, key_event;

    printk("%s Enter\n", __func__);

    #ifdef AW_AUTO_CALI    
    if(0 && cali_flag){
        printk("cali ...\n");
        aw9xxx_auto_cali();
        if(cali_flag == 0){    
            if(cali_used){
                i2c_write_reg(GCR,0x0000);     // disable chip
            }
            i2c_write_reg(INTER,N_INTER);
            i2c_write_reg(GIER,N_GIER);
            if(cali_used){
                i2c_write_reg(GCR,N_GCR);     // enable chip
            }
        }
        return ;
    }
    #endif
    // printk("debug_level=%d\n", debug_level);
    if(debug_level == 0){
        key_event = i2c_read_reg(0x32);        //read key interupt status
        key_state = i2c_read_reg(0x31);
        // printk("key_event: %02x, key_state: %02x\n", key_event, key_state);

        for (int i = 0; i < 6; i++) {
            if (keytable[i] < 0) {
                continue;
            }
            int bit = 1 << i;
            if (key_event & bit) {
                int value = !!(key_state & bit);
                // printk("report: %d, value: %d\n", keytable[i], value);
                input_report_key(aw9163_ts->input_dev, keytable[i], value);
            }
        }
        input_sync(aw9163_ts->input_dev);

#if 0
        if(buff2 & 0x10){                        //S3 click
            if(buff1 == 0x00){
                aw9163_left_release();
            }else if(buff1 == 0x10){
                aw9163_left_press();
            }
        }else if(buff2 & 0x08){                    //S2 click
            if(buff1 == 0x00){
                aw9163_center_release();
            }else if (buff1 == 0x08){
                aw9163_center_press();
            }
        }else if(buff2 & 0x04){                    //S1 click
            if(buff1 == 0x00){
                aw9163_right_release();
            }else if(buff1 == 0x04){
                aw9163_right_press();
            }
        }
#endif
    }
}

static int aw9163_ts_clear_intr(struct i2c_client *client) 
{
    int res;
    res = i2c_read_reg(0x32);
    printk("%s: reg0x32=0x%x\n", __func__, res);

    return 0;
}

/****************************************************************
*
* Function : Interrupt sub-program work in
*            aw9163_sleep_mode__proc() or 
*            aw9163_normal_mode_proc()
*
***************************************************************/
static void aw9163_ts_eint_work(struct work_struct *work)
{
    printk("%s Enter\n", __func__);
    printk("%s ==============aw9163_ts_eint_work WorkMode = %d\n", __func__,WorkMode);

    switch(WorkMode){
        case 1:
        aw9163_sleep_mode_proc();
        break;
        case 2:
        aw9163_normal_mode_proc();
        break;
        default:
        break;
    }

    aw9163_ts_clear_intr(aw9163_i2c_client);

    enable_irq(aw9163_ts->irq);
}


static irqreturn_t aw9163_ts_eint_func(int irq, void *desc)
{
    printk("%s Enter\n", __func__);
    printk("===========================touch 123456\n");
    disable_irq_nosync(irq);

    if(aw9163_ts == NULL){
        printk("%s: aw9163_ts == NULL", __func__);
        return  IRQ_NONE;
    }

    schedule_work(&aw9163_ts->eint_work);

    return IRQ_HANDLED;
}


/****************************************************************
*
* aw9163 eint set
*
***************************************************************/
static int aw9163_ts_setup_eint(void)
{
    int ret = 0;
    u32 ints[2] = {0, 0};
    int err;

    #if 1
    if(gpio_is_valid(aw9163_gpio_int)){
        printk("==========================reuqest_irq\n");
        aw9163_ts->irq = gpio_to_irq(aw9163_gpio_int);
        err = gpio_request(aw9163_gpio_int, "aw9163_gpio_int");
        gpio_direction_input(aw9163_gpio_int);
        if (request_irq(aw9163_ts->irq, aw9163_ts_eint_func, IRQ_TYPE_EDGE_FALLING, "aw9163-eint", NULL)) {
            printk("%s IRQ LINE NOT AVAILABLE!!\n", __func__);
            return -EINVAL;
        }
        if(err){
            printk("unable to request aw9163_gpio_int \n");
        }
        // enable_irq(aw9163_ts->irq);
    }
    #else
    aw9163_int_pin = pinctrl_lookup_state(aw9163ctrl, "aw9163_pin_int");
    if (IS_ERR(aw9163_int_pin)) {
        ret = PTR_ERR(aw9163_int_pin);
        printk("%s : pinctrl err, aw9163_int_pin=%d\n", __func__, aw9163_int_pin);
        return -EINVAL;
    }
    #endif

    #if 0
    if (aw9163_ts->irq_node) {
        of_property_read_u32_array(aw9163_ts->irq_node, "debounce", ints, ARRAY_SIZE(ints));
        gpio_set_debounce(ints[0], ints[1]);
        //pinctrl_select_state(aw9163ctrl, aw9163_int_pin);
        gpio_direction_output(aw9163_gpio_int, 1);
        printk("%s ints[0] = %d, ints[1] = %d!!\n", __func__, ints[0], ints[1]);

        aw9163_ts->irq = irq_of_parse_and_map(aw9163_ts->irq_node, 0);
        printk("%s irq = %d\n", __func__, aw9163_ts->irq);
        if (!aw9163_ts->irq) {
            printk("%s irq_of_parse_and_map fail!!\n", __func__);
            return -EINVAL;
        }
        if (request_irq(aw9163_ts->irq, aw9163_ts_eint_func, IRQ_TYPE_LEVEL_LOW, "aw9163-eint", NULL)) {
            printk("%s IRQ LINE NOT AVAILABLE!!\n", __func__);
            return -EINVAL;
        }
        //enable_irq(aw9163_ts->irq);
    } else {
        printk("null irq node!!\n");
        return -EINVAL;
    }
    #endif

    return 0;
}

/****************************************************************
*
* aw9163 syspend and resume
*
***************************************************************/
#ifdef CONFIG_PM
static int aw9163_ts_suspend(struct device *dev)
{
    if(WorkMode != 1){
        aw9163_sleep_mode_cfg();
        aw9163_ts_pwroff();
        suspend_flag = 1;
        #ifdef AW_AUTO_CALI
        cali_flag = 1;
        cali_num = 0;
        cali_cnt = 0;
        #endif
    }
    printk("%s Finish\n", __func__);

    return 0;
}

static int aw9163_ts_resume(struct device *dev)
{    
    if(WorkMode != 2){
        aw9163_normal_mode_cfg();
        aw9163_ts_pwron();
        suspend_flag = 0;
        #ifdef AW_AUTO_CALI
        cali_flag = 1;
        cali_num = 0;
        cali_cnt = 0;
        #endif
    }
    printk("%s Finish\n", __func__);

    return 0;
}

static const struct dev_pm_ops aw9163_pm_ops = {
    .suspend = aw9163_ts_suspend,
    .resume = aw9163_ts_resume,
};
#endif


/****************************************************************
*
* for adb shell and APK debug
*
***************************************************************/
static ssize_t aw9163_show_debug(struct device* cd,struct device_attribute *attr, char* buf)
{
    ssize_t ret = 0;

    sprintf(buf, "aw9163 Debug %d\n",debug_level);

    ret = strlen(buf) + 1;

    return ret;
}

static ssize_t aw9163_store_debug(struct device* cd, struct device_attribute *attr,
                                  const char* buf, size_t len)
{
    unsigned long on_off = simple_strtoul(buf, NULL, 10);
    debug_level = on_off;

    printk("%s: debug_level=%d\n",__func__, debug_level);

    return len;
}

static ssize_t aw9163_get_reg(struct device* cd,struct device_attribute *attr, char* buf)
{
    unsigned int reg_val[1];
    ssize_t len = 0;
    u8 i;
    disable_irq(aw9163_ts->irq);
    for(i=1;i<0x7F;i++) {
        reg_val[0] = i2c_read_reg(i);
        len += snprintf(buf+len, PAGE_SIZE-len, "reg%02x = 0x%04x, ", i,reg_val[0]);
    }
    len += snprintf(buf+len, PAGE_SIZE-len, "\n");
    enable_irq(aw9163_ts->irq);
    return len;
}

static ssize_t aw9163_write_reg(struct device* cd, struct device_attribute *attr,
                                const char* buf, size_t len)
{

    unsigned int databuf[2];
    disable_irq(aw9163_ts->irq);
    if(2 == sscanf(buf,"%x %x",&databuf[0], &databuf[1])) {
        i2c_write_reg((u8)databuf[0],databuf[1]);
    }
    enable_irq(aw9163_ts->irq);
    return len;
}

static ssize_t aw9163_get_adbBase(struct device* cd,struct device_attribute *attr, char* buf)
{
    unsigned int dataS1,dataS2,dataS3,dataS4,dataS5,dataS6;
    ssize_t len = 0;

    disable_irq(aw9163_ts->irq);
    len += snprintf(buf+len, PAGE_SIZE-len, "base: \n");
    i2c_write_reg(MCR,0x0003);

    dataS1=i2c_read_reg(0x36);
    dataS2=i2c_read_reg(0x37);
    dataS3=i2c_read_reg(0x38);
    dataS4=i2c_read_reg(0x39);
    dataS5=i2c_read_reg(0x3a);
    dataS6=i2c_read_reg(0x3b);
    len += snprintf(buf+len, PAGE_SIZE-len, "%d, ",dataS1);
    len += snprintf(buf+len, PAGE_SIZE-len, "%d, ",dataS2);
    len += snprintf(buf+len, PAGE_SIZE-len, "%d, ",dataS3);
    len += snprintf(buf+len, PAGE_SIZE-len, "%d, ",dataS4);
    len += snprintf(buf+len, PAGE_SIZE-len, "%d, ",dataS5);
    len += snprintf(buf+len, PAGE_SIZE-len, "%d, ",dataS6);

    len += snprintf(buf+len, PAGE_SIZE-len, "\n");

    enable_irq(aw9163_ts->irq);
    return len;
}

static ssize_t aw9163_get_rawdata(struct device* cd,struct device_attribute *attr, char* buf)
{
    unsigned int dataS1,dataS2,dataS3,dataS4,dataS5,dataS6;
    ssize_t len = 0;

    disable_irq(aw9163_ts->irq);
    len += snprintf(buf+len, PAGE_SIZE-len, "base: \n");
    i2c_write_reg(MCR,0x0003);

    dataS1=i2c_read_reg(0x36);
    dataS2=i2c_read_reg(0x37);
    dataS3=i2c_read_reg(0x38);
    dataS4=i2c_read_reg(0x39);
    dataS5=i2c_read_reg(0x3a);
    dataS6=i2c_read_reg(0x3b);
    len += snprintf(buf+len, PAGE_SIZE-len, "%d, ",dataS1);
    len += snprintf(buf+len, PAGE_SIZE-len, "%d, ",dataS2);
    len += snprintf(buf+len, PAGE_SIZE-len, "%d, ",dataS3);
    len += snprintf(buf+len, PAGE_SIZE-len, "%d, ",dataS4);
    len += snprintf(buf+len, PAGE_SIZE-len, "%d, ",dataS5);
    len += snprintf(buf+len, PAGE_SIZE-len, "%d, ",dataS6);

    len += snprintf(buf+len, PAGE_SIZE-len, "\n");

    enable_irq(aw9163_ts->irq);
    return len;
}

static ssize_t aw9163_get_delta(struct device* cd,struct device_attribute *attr, char* buf)
{
    unsigned int deltaS1,deltaS2,deltaS3,deltaS4,deltaS5,deltaS6;
    ssize_t len = 0;

    disable_irq(aw9163_ts->irq);
    len += snprintf(buf+len, PAGE_SIZE-len, "delta: \n");
    i2c_write_reg(MCR,0x0001);

    deltaS1=i2c_read_reg(0x36);if((deltaS1 & 0x8000) == 0x8000) { deltaS1 = 0; }
    deltaS2=i2c_read_reg(0x37);if((deltaS2 & 0x8000) == 0x8000) { deltaS2 = 0; }
    deltaS3=i2c_read_reg(0x38);if((deltaS3 & 0x8000) == 0x8000) { deltaS3 = 0; }
    deltaS4=i2c_read_reg(0x39);if((deltaS4 & 0x8000) == 0x8000) { deltaS4 = 0; }
    deltaS5=i2c_read_reg(0x3a);if((deltaS5 & 0x8000) == 0x8000) { deltaS5 = 0; }
    deltaS6=i2c_read_reg(0x3b);if((deltaS6 & 0x8000) == 0x8000) { deltaS6 = 0; }
    len += snprintf(buf+len, PAGE_SIZE-len, "%d, ",deltaS1);
    len += snprintf(buf+len, PAGE_SIZE-len, "%d, ",deltaS2);
    len += snprintf(buf+len, PAGE_SIZE-len, "%d, ",deltaS3);
    len += snprintf(buf+len, PAGE_SIZE-len, "%d, ",deltaS4);
    len += snprintf(buf+len, PAGE_SIZE-len, "%d, ",deltaS5);
    len += snprintf(buf+len, PAGE_SIZE-len, "%d, ",deltaS6);

    len += snprintf(buf+len, PAGE_SIZE-len, "\n");

    enable_irq(aw9163_ts->irq);
    return len;
}

static ssize_t aw9163_get_irqstate(struct device* cd,struct device_attribute *attr, char* buf)
{
    unsigned int keytouch,keyS1,keyS2,keyS3,keyS4,keyS5,keyS6;
    unsigned int gesture,slide1,slide2,slide3,slide4,doubleclick1,doubleclick2;
    ssize_t len = 0;

    disable_irq(aw9163_ts->irq);
    len += snprintf(buf+len, PAGE_SIZE-len, "keytouch: \n");

    keytouch=i2c_read_reg(0x31);
    if((keytouch&0x1) == 0x1) keyS1=1;else keyS1=0;
    if((keytouch&0x2) == 0x2) keyS2=1;else keyS2=0;
    if((keytouch&0x4) == 0x4) keyS3=1;else keyS3=0;
    if((keytouch&0x8) == 0x8) keyS4=1;else keyS4=0;
    if((keytouch&0x10) == 0x10) keyS5=1;else keyS5=0;
    if((keytouch&0x20) == 0x20) keyS6=1;else keyS6=0;

    len += snprintf(buf+len, PAGE_SIZE-len, "%d, ",keyS1);
    len += snprintf(buf+len, PAGE_SIZE-len, "%d, ",keyS2);
    len += snprintf(buf+len, PAGE_SIZE-len, "%d, ",keyS3);
    len += snprintf(buf+len, PAGE_SIZE-len, "%d, ",keyS4);
    len += snprintf(buf+len, PAGE_SIZE-len, "%d, ",keyS5);
    len += snprintf(buf+len, PAGE_SIZE-len, "%d, ",keyS6);
    len += snprintf(buf+len, PAGE_SIZE-len, "\n");

    len += snprintf(buf+len, PAGE_SIZE-len, "gesture: \n");            
    gesture=i2c_read_reg(0x2e);
    if(gesture == 0x1) slide1=1;else slide1=0;
    if(gesture == 0x2) slide2=1;else slide2=0;
    if(gesture == 0x4) slide3=1;else slide3=0;
    if(gesture == 0x8) slide4=1;else slide4=0;
    if(gesture == 0x10) doubleclick1=1;else doubleclick1=0;
    if(gesture == 0x200) doubleclick2=1;else doubleclick2=0;

    len += snprintf(buf+len, PAGE_SIZE-len, "%d, ",slide1);
    len += snprintf(buf+len, PAGE_SIZE-len, "%d, ",slide2);
    len += snprintf(buf+len, PAGE_SIZE-len, "%d, ",slide3);
    len += snprintf(buf+len, PAGE_SIZE-len, "%d, ",slide4);
    len += snprintf(buf+len, PAGE_SIZE-len, "%d, ",doubleclick1);
    len += snprintf(buf+len, PAGE_SIZE-len, "%d, ",doubleclick2);

    len += snprintf(buf+len, PAGE_SIZE-len, "\n");

    enable_irq(aw9163_ts->irq);
    return len;
}

static ssize_t aw9163_show_led(struct device* cd,struct device_attribute *attr, char* buf)
{
    ssize_t len = 0;
    len += snprintf(buf+len, PAGE_SIZE-len, "aw9163_led_off(void)\n");
    len += snprintf(buf+len, PAGE_SIZE-len, "echo 0 > led\n");
    len += snprintf(buf+len, PAGE_SIZE-len, "aw9163_led_on(void)\n");
    len += snprintf(buf+len, PAGE_SIZE-len, "echo 1 > led\n");

    return len;
}

static ssize_t aw9163_store_led(struct device* cd, struct device_attribute *attr,
                                const char* buf, size_t len)
{
    unsigned int databuf[1];

    sscanf(buf,"%d",&databuf[0]);
    if(databuf[0] == 0) {                // OFF
                         aw9163_led_off();
                        } else if(databuf[0] == 1){            // ON
                                                   aw9163_led_on();
                                                  } else {
                                                      aw9163_led_off();
                                                  }

    return len;
}

static DEVICE_ATTR(debug, 0664, aw9163_show_debug, aw9163_store_debug);
static DEVICE_ATTR(getreg, 0664, aw9163_get_reg, aw9163_write_reg);
static DEVICE_ATTR(adbbase, 0664, aw9163_get_adbBase, NULL);
static DEVICE_ATTR(rawdata, 0664, aw9163_get_rawdata, NULL);
static DEVICE_ATTR(delta, 0664, aw9163_get_delta, NULL);
static DEVICE_ATTR(getstate, 0664, aw9163_get_irqstate, NULL);
static DEVICE_ATTR(led, 0664, aw9163_show_led, aw9163_store_led);

static struct attribute *aw9163_attributes[] = {
    &dev_attr_debug.attr,
    &dev_attr_getreg.attr,
    &dev_attr_adbbase.attr,
    &dev_attr_rawdata.attr,
    &dev_attr_delta.attr,
    &dev_attr_getstate.attr,
    &dev_attr_led.attr,
    NULL
};

static struct attribute_group aw9163_attribute_group = {
    .attrs = aw9163_attributes
};


/****************************************************************
*
* aw9163 i2c driver
*
***************************************************************/
static int aw9163_i2c_probe(struct i2c_client *client, const struct i2c_device_id *id)
{
    struct input_dev *input_dev;
    int err = 0;
    int count =0;
    unsigned int reg_value; 

    printk("%s Enter\n", __func__);

    if (!i2c_check_functionality(client->adapter, I2C_FUNC_I2C)) {
        err = -ENODEV;
        goto exit_check_functionality_failed;
    }

    aw9163_ts = kzalloc(sizeof(*aw9163_ts), GFP_KERNEL);
    if (!aw9163_ts)    {
        err = -ENOMEM;
        printk("%s: kzalloc failed\n", __func__);
        goto exit_alloc_data_failed;
    }

    aw9163_ts_config_pins();

    client->addr = 0x2C;                            // chip  I2C address
    //client->timing= 400;
    aw9163_i2c_client = client;
    i2c_set_clientdata(client, aw9163_ts);

    for(count = 0; count < 5; count++){
        reg_value = i2c_read_reg(0x00);                //read chip ID
        printk("%s: chip id = 0x%04x", __func__, reg_value);

        if (reg_value == 0xb223)
        break;

        msleep(5);

        if(count == 4) {
            err = -ENODEV;
            goto exit_create_singlethread;
        }
    }

    aw9163_ts->irq_node = of_find_compatible_node(NULL, NULL, AW9163_TS_NAME);

    INIT_WORK(&aw9163_ts->eint_work, aw9163_ts_eint_work);

    input_dev = input_allocate_device();
    if (!input_dev){
        err = -ENOMEM;
        dev_err(&client->dev, "failed to allocate input device\n");
        goto exit_input_dev_alloc_failed;
    }

    aw9163_ts->input_dev = input_dev;


    __set_bit(EV_KEY, input_dev->evbit);
    __set_bit(EV_SYN, input_dev->evbit);

    for (int i = 0; i < 6; i++) {
        if (keytable[i] >= 0) {
            __set_bit(keytable[i], input_dev->keybit);
        }
    }

    input_dev->name = AW9163_TS_NAME;        //dev_name(&client->dev)
    err = input_register_device(input_dev);
    if (err) {
        dev_err(&client->dev,
                "%s: failed to register input device: %s\n",
                __func__, dev_name(&client->dev));
        goto exit_input_register_device_failed;
    }    

    err = sysfs_create_group(&client->dev.kobj, &aw9163_attribute_group);
    if (err < 0) {
        dev_err(&client->dev, "%s error creating sysfs attr files\n", __func__);
        goto exit_sysfs_creat_failed;
    }   

    WorkMode = 2;
    aw9163_normal_mode_cfg();

#ifdef AW_AUTO_CALI
cali_flag = 1;
cali_num = 0;
cali_cnt = 0;
#endif

    reg_value = i2c_read_reg(0x01);
    printk("%s: aw9163 GCR = 0x%04x\n", __func__, reg_value);

    aw9163_ts_setup_eint();
    //unsigned int buff2 = i2c_read_reg("0x32");

    //printk("=============================0x32=0x%04x\n",buff2);
    printk("%s Over\n", __func__);
    return 0;

exit_sysfs_creat_failed:
    exit_input_register_device_failed:
        input_free_device(input_dev);
        exit_input_dev_alloc_failed:
            //free_irq(client->irq, aw9163_ts);
            cancel_work_sync(&aw9163_ts->eint_work);
            exit_create_singlethread:
                printk("==singlethread error =\n");
                i2c_set_clientdata(client, NULL);
                kfree(aw9163_ts);
                exit_alloc_data_failed:
                    exit_check_functionality_failed:
                        return err;
}

static int aw9163_i2c_remove(struct i2c_client *client)
{
    struct aw9163_ts_data *aw9163_ts = i2c_get_clientdata(client);

    printk("%s enter\n", __func__);

    input_unregister_device(aw9163_ts->input_dev);
    kfree(aw9163_ts);
    i2c_set_clientdata(client, NULL);

    return 0;
}

static const struct i2c_device_id aw9163_i2c_id[] = {
    { AW9163_TS_NAME, 0 },{ }
};
MODULE_DEVICE_TABLE(i2c, aw9163_ts_id);


#ifdef CONFIG_OF
//static const struct of_device_id aw9163_of_match[] = {
//    {.compatible = "awinic,aw9163_i2c"},
//    {},
//};
#endif
static struct i2c_driver aw9163_i2c_driver = {
    .probe        = aw9163_i2c_probe,
    .remove        = aw9163_i2c_remove,
    .id_table    = aw9163_i2c_id,
    .driver    = {
        .name    = AW9163_TS_NAME,
        .owner    = THIS_MODULE,
        #ifdef CONFIG_OF
        //        .of_match_table = aw9163_of_match,
        #endif    
    },
};
/****************************************************************
*
* adb platform driver
*
***************************************************************/
static int aw9163_ts_remove(struct platform_device *pdev)
{
    printk("%s start!\n", __func__);
    i2c_del_driver(&aw9163_i2c_driver);
    return 0;
}

static int aw9163_ts_probe(struct platform_device *pdev)
{
    int ret;
    int addr;
    struct i2c_adapter *adapter;
    struct i2c_client *client;
    int bus_type = -1;

    printk("%s start!\n", __func__);

    struct aw9163_io_data *pdata = pdev->dev.platform_data;
    aw9163_gpio_pdn = pdata->aw9163_pdn;
    aw9163_gpio_int = pdata->aw9163_int;
    bus_type = pdata->bus_number;

    ret = aw9163_gpio_init(pdev);
    if (ret != 0) {
        printk("[%s] failed to init aw9163 pinctrl.\n", __func__);
        return ret;
    } else {
        printk("[%s] Success to init aw9163 pinctrl.\n", __func__);
    }

    adapter = i2c_get_adapter(bus_type);
    if (!adapter) {
        printk("wrong i2c adapter:%d\n", bus_type);
        return -ENODEV;
    }

    struct i2c_board_info board_info ={
        I2C_BOARD_INFO(AW9163_TS_NAME, 0x2C),
        //.of_node = child,
    };

    client = i2c_new_device(adapter, &board_info);
    if (!client) {
        printk("%s, allocate i2c_client failed\n", __func__);
        return -ENODEV;
    }
    addr = 44;
    // if(addr == AW9163_IIC_ADDRESS)
    //   aw9163_i2c_client = client;

    printk("=============================================aw9163========success\n");

    ret = i2c_add_driver(&aw9163_i2c_driver);
    if (ret != 0) {
        printk("[%s] failed to register aw9163 i2c driver.\n", __func__);
        return ret;
    } else {
        printk("[%s] Success to register aw9163 i2c driver.\n", __func__);
    }

    printk("%s exit!\n", __func__);

    return 0;
}

static struct platform_device_id id_tables[] = {
    {"aw9163-ts", 0},
    {}
};

#ifdef CONFIG_OF
//static const struct of_device_id aw9163plt_of_match[] = {
//    {.compatible = "awinic,aw9163_ts"},
//    {},
//};
#endif

static struct platform_driver aw9163_ts_driver = {
    .probe     = aw9163_ts_probe,
    .remove     = aw9163_ts_remove,
    .driver = {
        .name   = "aw9163-ts",
        //#ifdef CONFIG_OF
        //        .of_match_table = aw9163plt_of_match,
        //#endif
        #ifdef CONFIG_PM
        .pm = &aw9163_pm_ops,
        #endif
    },
    .id_table = id_tables,
};

static int __init aw9163_ts_init(void)
{
    int ret;
    printk("%s Enter\n", __func__);

    ret = platform_driver_register(&aw9163_ts_driver);
    if (ret) {
        printk("****[%s] Unable to register driver (%d)\n", __func__, ret);
        return ret;
    }        

    printk("%s Exit\n", __func__);
    
    return ret;
}

static void __exit aw9163_ts_exit(void)
{
    printk("%s Enter\n", __func__);
    platform_driver_unregister(&aw9163_ts_driver);
    printk("%s Exit\n", __func__);
}

module_init(aw9163_ts_init);
module_exit(aw9163_ts_exit);

MODULE_AUTHOR("<liweilei@awinic.com.cn>");
MODULE_DESCRIPTION("AWINIC aw9163 Touch driver");
MODULE_LICENSE("GPL v2");
