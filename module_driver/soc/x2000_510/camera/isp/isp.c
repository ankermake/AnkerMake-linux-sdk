/*
 * Copyright (C) 2020 Ingenic Semiconductor Co., Ltd.
 *
 * ISP Driver
 */

#include <linux/miscdevice.h>
#include <linux/interrupt.h>
#include <linux/irq.h>
#include <linux/slab.h>
#include <linux/delay.h>
#include <linux/kthread.h>

#include <common.h>
#include <bit_field.h>
#include <utils/clock.h>

#include "isp-core/inc/tiziano_core.h"
#include "isp-core/inc/tiziano_isp.h"
#include "camera_cpm.h"
#include "isp_regs.h"
#include "isp.h"
#include "vic.h"
#include "mscaler.h"
#include "isp_tuning.h"
#include "version_log.h"
#include "dsys.h"
#include "vic_channel_tiziano.h"


#define IRQ_ISP0                        (21)
#define IRQ_ISP1                        (20)


static struct jz_isp_data jz_isp_dev[2] = {
    {
        .index                  =  0,
        .irq                    = IRQ_INTC_BASE + IRQ_ISP0, /* BASE + 21 */
        .irq_name               = "ISP0",
    },

    {
        .index                  =  1,
        .irq                    = IRQ_INTC_BASE + IRQ_ISP1, /* BASE + 20 */
        .irq_name               = "ISP1",
    },
};


/*
 * ISP Operation
 */
static const unsigned long isp_iobase[] = {
        KSEG1ADDR(ISP0_IOBASE),
        KSEG1ADDR(ISP1_IOBASE),
};

#define ISP_ADDR(index, reg)            ((volatile unsigned long *)((isp_iobase[index]) + (reg)))

static inline void isp_write_reg(int index, unsigned int reg, unsigned int val)
{
    *ISP_ADDR(index, reg) = val;
}

static inline unsigned int isp_read_reg(int index, unsigned int reg)
{
    return *ISP_ADDR(index, reg);
}

static inline void isp_set_bit(int index, unsigned int reg, unsigned int start, unsigned int end, unsigned int val)
{
    set_bit_field(ISP_ADDR(index, reg), start, end, val);
}

static inline unsigned int isp_get_bit(int index, unsigned int reg, unsigned int start, unsigned int end)
{
    return get_bit_field(ISP_ADDR(index, reg), start, end);
}


/*
 * interface used by isp-core
 */
int system_reg_write(void *isp, unsigned int reg, unsigned int value)
{
    struct jz_isp_data *drv = (struct jz_isp_data *)isp;

    isp_write_reg(drv->index, reg, value);

    return 0;
}

unsigned int system_reg_read(void *isp, unsigned int reg)
{
    struct jz_isp_data *drv = (struct jz_isp_data *)isp;

    return isp_read_reg(drv->index, reg);
}

int system_irq_func_set(void *hdl, int irq, void *func, void *data)
{
    struct jz_isp_data *drv = (struct jz_isp_data *)hdl;

    drv->irq_func_cb[irq] = func;
    drv->irq_func_data[irq] = data;

    return 0;
}

EXPORT_SYMBOL(system_irq_func_set);

/*
 * ISP Base Interface
 */
static inline void isp_dump_reg(int index)
{
    printk("==========dump isp%d register============\n", index);

    printk("TOP_CTRL_ADDR_VERSION            :0x%08x\n", isp_read_reg(index, TOP_CTRL_ADDR_VERSION            ));
    printk("TOP_CTRL_ADDR_FM_SIZE            :0x%08x\n", isp_read_reg(index, TOP_CTRL_ADDR_FM_SIZE            ));
    printk("TOP_CTRL_ADDR_BAYER_TYPE         :0x%08x\n", isp_read_reg(index, TOP_CTRL_ADDR_BAYER_TYPE         ));
    printk("TOP_CTRL_ADDR_BYPASS_CON         :0x%08x\n", isp_read_reg(index, TOP_CTRL_ADDR_BYPASS_CON         ));
    printk("TOP_CTRL_ADDR_TOP_CON            :0x%08x\n", isp_read_reg(index, TOP_CTRL_ADDR_TOP_CON            ));
    printk("TOP_CTRL_ADDR_TOP_STATE          :0x%08x\n", isp_read_reg(index, TOP_CTRL_ADDR_TOP_STATE          ));
    printk("TOP_CTRL_ADDR_LINE_SPACE         :0x%08x\n", isp_read_reg(index, TOP_CTRL_ADDR_LINE_SPACE         ));
    printk("TOP_CTRL_ADDR_REG_CON            :0x%08x\n", isp_read_reg(index, TOP_CTRL_ADDR_REG_CON            ));
    printk("TOP_CTRL_ADDR_DMA_RC_TRIG        :0x%08x\n", isp_read_reg(index, TOP_CTRL_ADDR_DMA_RC_TRIG        ));
    printk("TOP_CTRL_ADDR_DMA_RC_CON         :0x%08x\n", isp_read_reg(index, TOP_CTRL_ADDR_DMA_RC_CON         ));
    printk("TOP_CTRL_ADDR_DMA_RC_ADDR        :0x%08x\n", isp_read_reg(index, TOP_CTRL_ADDR_DMA_RC_ADDR        ));
    printk("TOP_CTRL_ADDR_DMA_RC_STATE       :0x%08x\n", isp_read_reg(index, TOP_CTRL_ADDR_DMA_RC_STATE       ));
    printk("TOP_CTRL_ADDR_DMA_RC_APB_WR_DATA :0x%08x\n", isp_read_reg(index, TOP_CTRL_ADDR_DMA_RC_APB_WR_DATA ));
    printk("TOP_CTRL_ADDR_DMA_RC_APB_WR_ADDR :0x%08x\n", isp_read_reg(index, TOP_CTRL_ADDR_DMA_RC_APB_WR_ADDR ));
    printk("TOP_CTRL_ADDR_DMA_RD_CON         :0x%08x\n", isp_read_reg(index, TOP_CTRL_ADDR_DMA_RD_CON         ));
    printk("TOP_CTRL_ADDR_DMA_WR_CON         :0x%08x\n", isp_read_reg(index, TOP_CTRL_ADDR_DMA_WR_CON         ));
    printk("TOP_CTRL_ADDR_DMA_FR_WR_CON      :0x%08x\n", isp_read_reg(index, TOP_CTRL_ADDR_DMA_FR_WR_CON      ));
    printk("TOP_CTRL_ADDR_DMA_STA_WR_CON     :0x%08x\n", isp_read_reg(index, TOP_CTRL_ADDR_DMA_STA_WR_CON     ));
    printk("TOP_CTRL_ADDR_DMA_RD_DEBUG       :0x%08x\n", isp_read_reg(index, TOP_CTRL_ADDR_DMA_RD_DEBUG       ));
    printk("TOP_CTRL_ADDR_DMA_WR_DEBUG       :0x%08x\n", isp_read_reg(index, TOP_CTRL_ADDR_DMA_WR_DEBUG       ));
    printk("TOP_CTRL_ADDR_DMA_FR_WR_DEBUG    :0x%08x\n", isp_read_reg(index, TOP_CTRL_ADDR_DMA_FR_WR_DEBUG    ));
    printk("TOP_CTRL_ADDR_DMA_STA_WR_DEBUG   :0x%08x\n", isp_read_reg(index, TOP_CTRL_ADDR_DMA_STA_WR_DEBUG   ));
    printk("TOP_CTRL_ADDR_INT_EN             :0x%08x\n", isp_read_reg(index, TOP_CTRL_ADDR_INT_EN             ));
    printk("TOP_CTRL_ADDR_INT_REG            :0x%08x\n", isp_read_reg(index, TOP_CTRL_ADDR_INT_REG            ));
    printk("TOP_CTRL_ADDR_INT_CLR            :0x%08x\n", isp_read_reg(index, TOP_CTRL_ADDR_INT_CLR            ));
    printk("TOP_CTRL_ADDR_TP_FREERUN         :0x%08x\n", isp_read_reg(index, TOP_CTRL_ADDR_TP_FREERUN         ));
    printk("TOP_CTRL_ADDR_TP_CON             :0x%08x\n", isp_read_reg(index, TOP_CTRL_ADDR_TP_CON             ));
    printk("TOP_CTRL_ADDR_TP_SIZE            :0x%08x\n", isp_read_reg(index, TOP_CTRL_ADDR_TP_SIZE            ));
    printk("TOP_CTRL_ADDR_TP_FONT            :0x%08x\n", isp_read_reg(index, TOP_CTRL_ADDR_TP_FONT            ));
    printk("TOP_CTRL_ADDR_TP_FLICK           :0x%08x\n", isp_read_reg(index, TOP_CTRL_ADDR_TP_FLICK           ));
    printk("TOP_CTRL_ADDR_TP_CS_TYPE         :0x%08x\n", isp_read_reg(index, TOP_CTRL_ADDR_TP_CS_TYPE         ));
    printk("TOP_CTRL_ADDR_TP_CS_FCLO         :0x%08x\n", isp_read_reg(index, TOP_CTRL_ADDR_TP_CS_FCLO         ));
    printk("TOP_CTRL_ADDR_TP_CS_BCLO         :0x%08x\n", isp_read_reg(index, TOP_CTRL_ADDR_TP_CS_BCLO         ));
}

/*
 * ISP总线复位后mscaler,isp相关模块均被复位
 */
static int tiziano_cpm_reset(int index)
{
    int timeout = 0xffffff;
    struct jz_isp_data *drv = &jz_isp_dev[index];

    if (drv->camera.is_power_on != 0) /* 同一ISP的不同通道只需reset一次 */
        return 0;

    switch (index) {
    case 0: {   /* ISP0 */
        /* stop request */
        cpm_set_bit(CPM_SRBC, CPM_SRBC_ISP0_STOP_TRANSFER, 1);
        while ( !cpm_get_bit(CPM_SRBC, CPM_SRBC_ISP0_STOP_ACK) && --timeout );
        if (timeout == 0) {
            printk(KERN_ERR "isp%d wait stop timeout\n", index);
            return  -ETIMEDOUT;
        }

        /* reset */
        unsigned long isp0_reset = cpm_read_reg(CPM_SRBC);

        set_bit_field(&isp0_reset, CPM_SRBC_ISP0_STOP_TRANSFER, 0);
        set_bit_field(&isp0_reset, CPM_SRBC_ISP0_SOFT_RESET, 1);
        cpm_write_reg(CPM_SRBC, isp0_reset);

        udelay(10);

        isp0_reset = cpm_read_reg(CPM_SRBC);
        set_bit_field(&isp0_reset, CPM_SRBC_ISP0_SOFT_RESET, 0);
        cpm_write_reg(CPM_SRBC, isp0_reset);

        break;
    }

    case 1: {   /* ISP1 */
        /* stop request */
        cpm_set_bit(CPM_SRBC, CPM_SRBC_ISP1_STOP_TRANSFER, 1);
        while ( !cpm_get_bit(CPM_SRBC, CPM_SRBC_ISP1_STOP_ACK) && --timeout );
        if (timeout == 0) {
            printk(KERN_ERR "isp%d wait stop timeout\n", index);
            return  -ETIMEDOUT;
        }

        /* reset */
        unsigned long isp1_reset = cpm_read_reg(CPM_SRBC);

        set_bit_field(&isp1_reset, CPM_SRBC_ISP1_STOP_TRANSFER, 0);
        set_bit_field(&isp1_reset, CPM_SRBC_ISP1_SOFT_RESET, 1);
        cpm_write_reg(CPM_SRBC, isp1_reset);

        udelay(10);

        isp1_reset = cpm_read_reg(CPM_SRBC);
        set_bit_field(&isp1_reset, CPM_SRBC_ISP1_SOFT_RESET, 0);
        cpm_write_reg(CPM_SRBC, isp1_reset);
        break;
    }

    default:
        printk(KERN_ERR "isp%d out of range\n", index);
        break;
    }

    return 0;
}

static int isp_firmware_process(void *data)
{
    struct jz_isp_data *drv = (struct jz_isp_data *)data;

    while(!kthread_should_stop()){
        tisp_fw_process(&drv->core);
    }

    return 0;
}

static int tiziano_init(int index)
{
    struct jz_isp_data *drv = &jz_isp_dev[index];
    struct sensor_attr *attr = drv->camera.sensor;
    tisp_init_param_t iparam;
    int ret;

    /*
     * ISP firmware parameter
     */
    strncpy(iparam.sensor, attr->device_name, sizeof(iparam.sensor)); /* IQ file name */
    iparam.width  = attr->sensor_info.width;
    iparam.height = attr->sensor_info.height;
    switch(attr->sensor_info.fmt) {
    case SENSOR_PIXEL_FMT_SBGGR8_1X8:
    case SENSOR_PIXEL_FMT_SBGGR10_1X10:
    case SENSOR_PIXEL_FMT_SBGGR12_1X12:
        iparam.bayer = 2;   /* BGGR */
        break;

    case SENSOR_PIXEL_FMT_SGBRG8_1X8:
    case SENSOR_PIXEL_FMT_SGBRG10_1X10:
    case SENSOR_PIXEL_FMT_SGBRG12_1X12:
        iparam.bayer = 3;   /* GBRG */
        break;

    case SENSOR_PIXEL_FMT_SGRBG8_1X8:
    case SENSOR_PIXEL_FMT_SGRBG10_1X10:
    case SENSOR_PIXEL_FMT_SGRBG12_1X12:
        iparam.bayer = 1;   /* GRBG */
        break;

    case SENSOR_PIXEL_FMT_SRGGB8_1X8:
    case SENSOR_PIXEL_FMT_SRGGB10_1X10:
    case SENSOR_PIXEL_FMT_SRGGB12_1X12:
        iparam.bayer = 0;   /* RGGB */
        break;

    default:
        printk(KERN_ERR "%s, the input format(0x%08x) not support!\n", __func__, attr->sensor_info.fmt);
        return -EINVAL;
    }

    ret = tisp_core_init(&drv->core, &iparam, drv);
    if (ret) {
        printk(KERN_ERR "%s, tisp_core_init failed!\n", __func__);
        return ret;
    }

    /*
     * For event engine
     */
    drv->process_thread = kthread_run(isp_firmware_process, drv, "isp_firmware_process");
    if(IS_ERR_OR_NULL(drv->process_thread)){
        printk(KERN_ERR "%s, kthread_run was failed!\n", __func__);
        return -EINVAL;
    }

    return 0;
}

static void tiziano_deinit(int index)
{
    struct jz_isp_data *drv = &jz_isp_dev[index];

    kthread_stop(drv->process_thread);

    tisp_core_deinit(&drv->core);
}

int isp_stream_on(int index, struct sensor_attr *attr)
{
    struct jz_isp_data *drv = &jz_isp_dev[index];
    int ret;

    isp_write_reg(index, TOP_CTRL_ADDR_INT_EN, 0x81703ff);
    enable_irq(drv->irq);

    drv->tuning->event(drv->tuning, TISP_EVENT_ACTIVATE_MODULE, NULL);

    ret = vic_tiziano_stream_on(index, attr);  /* VIC Interface Stream ON */
    if (ret) {
        printk(KERN_ERR "%s[%d] vic tiziano stream on failed!\n",__func__,__LINE__);
        ret = -EINVAL;
        goto tiziano_stream_on_failed;
    }

    drv->camera.is_stream_on = 1;

    return 0;

tiziano_stream_on_failed:
    disable_irq(drv->irq);
    drv->tuning->event(drv->tuning, TISP_EVENT_SLAVE_MODULE, NULL);
    return ret;
}

int isp_stream_off(int index, struct sensor_attr *attr)
{
    struct jz_isp_data *drv = &jz_isp_dev[index];

    vic_tiziano_stream_off(index, attr);

    /* disable irq */
    disable_irq(drv->irq);
    isp_write_reg(index, TOP_CTRL_ADDR_INT_EN, 0);

    /* isp tirger */
    isp_write_reg(index, INPUT_CTRL_ADDR_IP_TRIG, 0x0);

    drv->tuning->event(drv->tuning, TISP_EVENT_SLAVE_MODULE, NULL);

    drv->camera.is_stream_on = 0;

    return 0;
}

int isp_power_on(int index)
{
    struct jz_isp_data *drv = &jz_isp_dev[index];
    int ret = 0;

    mutex_lock(&drv->lock);

    if (drv->camera.is_power_on == 0) {
        if(!vic_tiziano_enable_clk(index))
            tiziano_cpm_reset(index);

        ret = vic_tiziano_power_on(index);
        if (ret)
            goto unlock;

        ret = tiziano_init(index);
        if (ret)
            goto unlock;
    }

    if (!ret)
        drv->camera.is_power_on++;

unlock:
    mutex_unlock(&drv->lock);

    return ret;
}

void isp_power_off(int index)
{
    struct jz_isp_data *drv = &jz_isp_dev[index];

    mutex_lock(&drv->lock);

    vic_tiziano_power_off(index);

    tiziano_deinit(index);

    drv->camera.is_power_on--;

    mutex_unlock(&drv->lock);
}

static irqreturn_t isp_irq_handler(int irq, void *data)
{
    struct jz_isp_data *drv = (struct jz_isp_data *)data;
    int index = drv->index;
    int irqret = IRQ_HANDLED;
    unsigned int irqstatus;
    int ret;

    irqstatus = isp_read_reg(index, TOP_CTRL_ADDR_INT_REG);
    isp_write_reg(index, TOP_CTRL_ADDR_INT_CLR, irqstatus);
#if 0
    unsigned int irqen;
    unsigned int ispstatus;
    int retry = 0;
    irqen = isp_read_reg(index, TOP_CTRL_ADDR_INT_EN);
    isp_write_reg(index, TOP_CTRL_ADDR_INT_EN, irqen & (~irqstatus));

    //err cnt
    if (irqstatus & 0x3f8){
        printk(KERN_WARNING "ispcore: irq-status %08x\n",irqstatus);
        drv->isp_err++;
    }
    if (irqstatus & 0xf8){
        drv->isp_err1++;
    }
    if (irqstatus & 0x200){
        drv->isp_overflow++;
    }
    if (irqstatus & 0x100){
        drv->isp_breakfrm++;
    }

    //breakfrm & overflow
    if (irqstatus & 0x300){
        ispstatus = isp_read_reg(index, TOP_CTRL_ADDR_TOP_CON);
        isp_write_reg(index, TOP_CTRL_ADDR_TOP_CON, ispstatus | 0x2);//[1]:1
        while(retry < 60){
            ret = isp_read_reg(index, TOP_CTRL_ADDR_TOP_STATE);
            if(ret == 1)
                break;
            retry++;
        }
        if(retry < 60){
            //reset isp
            isp_write_reg(index, TOP_CTRL_ADDR_TOP_CON, ((ispstatus & 0xfffffffd) | 0x10));//[1]:0 [4]:1
            isp_write_reg(index, TOP_CTRL_ADDR_TOP_CON, (ispstatus & 0xffffffed));//[4]:0
            isp_write_reg(index, TOP_CTRL_ADDR_INT_CLR, 0x300);
            isp_write_reg(index, 0x100, 0x1);
        } else {
            printk(KERN_WARNING "retry failed!!!!\n");
        }
    }
#endif

    //frmdone isp set
    if (irqstatus & 0x07) {
        if (1 == drv->daynight_change) {
            drv->daynight_change = 0;
            tisp_day_or_night_s_ctrl(&drv->core.core_tuning, drv->dn_state);
        }
        if(1 == drv->hflip_change){
            drv->hflip_change = 0;
            tisp_mirror_enable(&drv->core.core_tuning, drv->hflip_state);
        }
        if(1 == drv->vflip_change){
            drv->vflip_change = 0;
            tisp_flip_enable(&drv->core.core_tuning, drv->vflip_state);
        }
    }

    /*
     * 1. mscaler irq callback
     */
    ret = mscaler_interrupt_service_routine(index, irqstatus);
    if(ret < 0)
        printk(KERN_ERR "mscaler interrupt handle error. ret=%d\n", ret);

    /*
     * 2. isp-core irq callbacks
     */
    int i;
    for (i = 0; i < 32; i++) {
        if ( (irqstatus & (1 << i)) && drv->irq_func_cb[i]) {
            ret = drv->irq_func_cb[i](drv->irq_func_data[i]);
            if (ret != IRQ_HANDLED) {
                irqret = ret;
            }
        }
    }

#if 0
    isp_write_reg(index, TOP_CTRL_ADDR_INT_EN, irqen);
#endif

    return irqret;
}

static irqreturn_t isp_irq_thread_handler(int irq, void *data)
{
    struct jz_isp_data *isp = (struct jz_isp_data *)data;
    struct sensor_ctrl_ops *ops = &isp->camera.sensor->ops;
    int i = 0;

    if(!isp || !ops)
        return 0;

    for(i = 0; i < TISP_I2C_SET_BUTTON; i++){
        if(isp->i2c_msgs[i].flag == 0)
            continue;
        isp->i2c_msgs[i].flag = 0;
        switch(i){
            case TISP_I2C_SET_INTEGRATION:
                if (ops->set_integration_time)
                    ops->set_integration_time(isp->i2c_msgs[i].value);
                else
                    printk(KERN_ERR "sensor_attr->ops.set_integration_time is NULL!\n");
                break;
            case TISP_I2C_SET_AGAIN:
                if (ops->set_analog_gain)
                    ops->set_analog_gain(isp->i2c_msgs[i].value);
                else
                    printk(KERN_ERR "sensor_attr->ops.set_analog_gain is NULL!\n");
                break;
            case TISP_I2C_SET_DGAIN:
                if (ops->set_digital_gain)
                    ops->set_digital_gain(isp->i2c_msgs[i].value);
                else
                    printk(KERN_ERR "sensor_attr->ops.set_digital_gain is NULL!\n");
                break;
            default:
                break;
        }
    }
    return 0;
}


#ifdef SOC_CAMERA_DEBUG
static int dsysfs_isp_show_info(struct device *dev, struct device_attribute *attr, char *buf)
{
    struct jz_isp_data *drv = container_of((struct kobject *)dev, struct jz_isp_data, dsysfs_kobj);
    struct sensor_attr *sensor = drv->camera.sensor;
    tisp_core_tuning_t *core_tuning = &drv->core.core_tuning;
    char *p = buf;

    char *colorspace = NULL;
    tisp_ev_attr_t ev_attr;
    tisp_wb_attr_t wb_attr;
    tisp_ae_ex_min_t ae_min;

    p += sprintf(p ,"************** ISP INFO **************\n");
    if(!drv->camera.is_stream_on){
        p += sprintf(p ,"sensor doesn't work, please stream on\n");
        return p - buf;
    }

    tisp_g_ev_attr(core_tuning, &ev_attr);
    tisp_g_ae_min(core_tuning, &ae_min);
    tisp_g_wb_attr(core_tuning, &wb_attr);

    switch(sensor->sensor_info.fmt){
    case SENSOR_PIXEL_FMT_SBGGR8_1X8:
    case SENSOR_PIXEL_FMT_SBGGR10_1X10:
    case SENSOR_PIXEL_FMT_SBGGR12_1X12:
        colorspace = "BGGR";
        break;
    case SENSOR_PIXEL_FMT_SGBRG8_1X8:
    case SENSOR_PIXEL_FMT_SGBRG10_1X10:
    case SENSOR_PIXEL_FMT_SGBRG12_1X12:
        colorspace = "GBRG";
        break;
    case SENSOR_PIXEL_FMT_SGRBG8_1X8:
    case SENSOR_PIXEL_FMT_SGRBG10_1X10:
    case SENSOR_PIXEL_FMT_SGRBG12_1X12:
        colorspace = "GRBG";
        break;
    case SENSOR_PIXEL_FMT_SRGGB8_1X8:
    case SENSOR_PIXEL_FMT_SRGGB10_1X10:
    case SENSOR_PIXEL_FMT_SRGGB12_1X12:
        colorspace = "RGGB";
        break;
#if 0
    case SENSOR_PIXEL_FMT_SRGIB_8BIT:
    case SENSOR_PIXEL_FMT_SRGIB_10BIT:
    case SENSOR_PIXEL_FMT_SRGIB_12BIT:
        colorspace = "RGIB";
        break;
    case SENSOR_PIXEL_FMT_SBGIR_8BIT:
    case SENSOR_PIXEL_FMT_SBGIR_10BIT:
    case SENSOR_PIXEL_FMT_SBGIR_12BIT:
        colorspace = "BGIR";
        break;
    case SENSOR_PIXEL_FMT_SRIGB_8BIT:
    case SENSOR_PIXEL_FMT_SRIGB_10BIT:
    case SENSOR_PIXEL_FMT_SRIGB_12BIT:
        colorspace = "RIGB";
        break;
    case SENSOR_PIXEL_FMT_SBIGR_8BIT:
    case SENSOR_PIXEL_FMT_SBIGR_10BIT:
    case SENSOR_PIXEL_FMT_SBIGR_12BIT:
        colorspace = "BIGR";
        break;
    case SENSOR_PIXEL_FMT_SGRBI_8BIT:
    case SENSOR_PIXEL_FMT_SGRBI_10BIT:
    case SENSOR_PIXEL_FMT_SGRBI_12BIT:
        colorspace = "GRBI";
        break;
    case SENSOR_PIXEL_FMT_SGBRI_8BIT:
    case SENSOR_PIXEL_FMT_SGBRI_10BIT:
    case SENSOR_PIXEL_FMT_SGBRI_12BIT:
        colorspace = "GBRI";
        break;
    case SENSOR_PIXEL_FMT_SIRBG_8BIT:
    case SENSOR_PIXEL_FMT_SIRBG_10BIT:
    case SENSOR_PIXEL_FMT_SIRBG_12BIT:
        colorspace = "IRBG";
        break;
    case SENSOR_PIXEL_FMT_SIBRG_8BIT:
    case SENSOR_PIXEL_FMT_SIBRG_10BIT:
    case SENSOR_PIXEL_FMT_SIBRG_12BIT:
        colorspace = "IBRG";
        break;
    case SENSOR_PIXEL_FMT_SRGGI_8BIT:
    case SENSOR_PIXEL_FMT_SRGGI_10BIT:
    case SENSOR_PIXEL_FMT_SRGGI_12BIT:
        colorspace = "RGGI";
        break;
    case SENSOR_PIXEL_FMT_SBGGI_8BIT:
    case SENSOR_PIXEL_FMT_SBGGI_10BIT:
    case SENSOR_PIXEL_FMT_SBGGI_12BIT:
        colorspace = "BGGI";
        break;
    case SENSOR_PIXEL_FMT_SGRIG_8BIT:
    case SENSOR_PIXEL_FMT_SGRIG_10BIT:
    case SENSOR_PIXEL_FMT_SGRIG_12BIT:
        colorspace = "GRIG";
        break;
    case SENSOR_PIXEL_FMT_SGBIG_8BIT:
    case SENSOR_PIXEL_FMT_SGBIG_10BIT:
    case SENSOR_PIXEL_FMT_SGBIG_12BIT:
        colorspace = "GBIG";
        break;
    case SENSOR_PIXEL_FMT_SGIRG_8BIT:
    case SENSOR_PIXEL_FMT_SGIRG_10BIT:
    case SENSOR_PIXEL_FMT_SGIRG_12BIT:
        colorspace = "GIRG";
        break;
    case SENSOR_PIXEL_FMT_SGIBG_8BIT:
    case SENSOR_PIXEL_FMT_SGIBG_10BIT:
    case SENSOR_PIXEL_FMT_SGIBG_12BIT:
        colorspace = "SGIBG";
        break;
    case SENSOR_PIXEL_FMT_SIGGR_8BIT:
    case SENSOR_PIXEL_FMT_SIGGR_10BIT:
    case SENSOR_PIXEL_FMT_SIGGR_12BIT:
        colorspace = "IGGR";
        break;
    case SENSOR_PIXEL_FMT_SIGGB_8BIT:
    case SENSOR_PIXEL_FMT_SIGGB_10BIT:
    case SENSOR_PIXEL_FMT_SIGGB_12BIT:
        colorspace = "IGGB";
        break;
#endif
    default:
        colorspace = "The format of isp input is YUV422 or RGB";
        break;
    }

    p += sprintf(p ,"TISP Core Version : %s\n", TISP_CORE_VERSION);
    p += sprintf(p ,"Driver Version : %s\n", DRIVER_VERSION);
    p += sprintf(p ,"SENSOR NAME : %s\n", sensor->device_name);
    p += sprintf(p ,"SENSOR WIDTH : %d\n", sensor->sensor_info.width);
    p += sprintf(p ,"SENSOR HEIGHT : %d\n", sensor->sensor_info.height);
    p += sprintf(p ,"SENSOR RAW PATTERN : %s\n", colorspace);
    p += sprintf(p ,"SENSOR FPS : %d / %d\n", sensor->sensor_info.fps >> 16, sensor->sensor_info.fps & 0xffff);
    p += sprintf(p ,"ISP Top Value : 0x%x\n", tisp_top_read(core_tuning));
    p += sprintf(p ,"ISP Runing Mode : %s\n", ((tisp_day_or_night_g_ctrl(core_tuning) == TISP_MODE_NIGHT_MODE) ? "Night" : "Day"));
    p += sprintf(p ,"ISP AE: %s\n", ev_attr.ae_manual ? "MANUAL" : "AUTO");
    p += sprintf(p ,"ISP EV value: %d\n", ev_attr.ev);
    p += sprintf(p ,"SENSOR Integration Time : %d lines\n", ev_attr.integration_time);
    p += sprintf(p ,"MIN SENSOR Integration Time : %d lines\n", ev_attr.min_integration_time);
    p += sprintf(p ,"MAX SENSOR Integration Time : %d lines\n", ev_attr.max_integration_time);
    p += sprintf(p ,"SENSOR Integration Time : %d us\n", ev_attr.integration_time_us);
    p += sprintf(p ,"SENSOR analog gain : %d\n", ev_attr.sensor_again);
    p += sprintf(p ,"MAX SENSOR analog gain : %d\n", ev_attr.max_sensor_again);
    p += sprintf(p ,"SENSOR digital gain : %d\n", ev_attr.sensor_dgain);
    p += sprintf(p ,"MAX SENSOR digital gain : %d\n",ev_attr.max_sensor_dgain);
    p += sprintf(p ,"ISP digital gain : %d\n", ev_attr.isp_dgain);
    p += sprintf(p ,"MAX ISP digital gain : %d\n", ev_attr.max_isp_dgain);
    p += sprintf(p ,"ISP Tgain : %d\n", ev_attr.total_gain);
    p += sprintf(p ,"ISP WB weighted rgain: %d\n", (unsigned int)(65536/(wb_attr.tisp_wb_rg_sta_weight)));
    p += sprintf(p ,"ISP WB weighted bgain: %d\n", (unsigned int)(65536/(wb_attr.tisp_wb_bg_sta_weight)));
    p += sprintf(p ,"Brightness : %d\n", tisp_get_brightness(core_tuning));
    p += sprintf(p ,"Contrast : %d\n", tisp_get_contrast(core_tuning));
    p += sprintf(p ,"Saturation : %d\n", tisp_get_saturation(core_tuning));
    p += sprintf(p ,"Sharpness : %d\n", tisp_get_sharpness(core_tuning));
    p += sprintf(p ,"hflip : %s\n", core_tuning->hflip ? "enable" : "disable");
    p += sprintf(p ,"vflip : %s\n", core_tuning->vflip ? "enable" : "disable");
    p += sprintf(p ,"Antiflicker : %d\n", core_tuning->flicker_hz);
    p += sprintf(p ,"debug : %d,%d,%d,%d\n", drv->isp_err,drv->isp_err1,drv->isp_overflow,drv->isp_breakfrm);

    return p - buf;
}

static ssize_t dsysfs_isp_ctrl(struct file *file, struct kobject *kobj, struct bin_attribute *attr, char *buf, loff_t pos, size_t count)
{
    struct jz_isp_data *drv = container_of(kobj, struct jz_isp_data, dsysfs_kobj);

    if (!strncmp(buf, "r_isp_reg", sizeof("r_isp_reg")-1)) {
        char *s, *tmp;
        int i = 0;
        unsigned int reg, reg_list[2] = {0, 0}; //[0]:start, [1]:stop
        unsigned int val;

        s = buf+sizeof("r_isp_reg");
        tmp = strsep(&s, " ");
        while (tmp != NULL) {
            if (*tmp != '\0') {
                //printk("-%s\n", tmp);
                reg_list[i] = simple_strtoul(tmp, NULL, 0);
                i++;
                if (i >= 2)
                    break;
            }
            tmp = strsep(&s, " ");
        }

        if (reg_list[0] % 4 || reg_list[1] % 4) {
            printk(KERN_ERR "err reg list: 0x%08x - 0x%08x", reg_list[0], reg_list[1]);
            return count;
        }

        if (reg_list[1] == 0) {
            //read one reg
            val = isp_read_reg(drv->index, reg_list[0]);
            printk(KERN_ERR "r_isp_reg: 0x%08x 0x%08x \n", reg_list[0], val);
        } else if (reg_list[1] > reg_list[0]) {
            //read reg list
            printk(KERN_ERR "r_isp_reg_list: 0x%08x - 0x%08x \n", reg_list[0], reg_list[1]);
            reg = reg_list[0];
            while (1) {
                val = isp_read_reg(drv->index, reg);
                printk(KERN_ERR "0x%08x 0x%08x \n", reg, val);

                reg += 4;
                if (reg >= reg_list[1])
                    break;
            }
        } else {
            printk(KERN_ERR "err reg list: 0x%08x - 0x%08x", reg_list[0], reg_list[1]);
        }

        return count;
    } else if (!strncmp(buf, "w_isp_reg", sizeof("w_isp_reg")-1)) {
        unsigned int reg;
        unsigned int val;
        char *p = 0;
        reg = simple_strtoul(buf+sizeof("w_isp_reg"), &p, 0);
        val = simple_strtoul(p+1, NULL, 0);
        isp_write_reg(drv->index, reg, val);
        printk(KERN_ERR "w_isp_reg: 0x%08x 0x%08x \n", reg, val);
    }

    return count;
}


static DSYSFS_DEV_ATTR(show_isp_info, S_IRUGO|S_IWUSR, dsysfs_isp_show_info, NULL);
static struct attribute *dsysfs_isp_dev_attrs[] = {
    &dsysfs_dev_attr_show_isp_info.attr,
    NULL,
};

static DSYSFS_BIN_ATTR(ctrl, S_IRUGO|S_IWUSR, NULL, dsysfs_isp_ctrl, 0);
static struct bin_attribute *dsysfs_isp_bin_attrs[] = {
    &dsysfs_bin_attr_ctrl,
    NULL,
};

static const struct attribute_group dsysfs_isp_attr_group = {
    .attrs  = dsysfs_isp_dev_attrs,
    .bin_attrs = dsysfs_isp_bin_attrs,
};
#endif


int isp_component_bind_sensor(int index, struct sensor_attr *sensor)
{
    struct jz_isp_data *drv = &jz_isp_dev[index];
    int ret = 0;

    assert(drv->is_finish > 0);
    assert(!drv->camera.sensor);

    mutex_lock(&drv->lock);

    ret = mscaler_register_sensor(index, sensor);
    if (ret != 0) {
        printk(KERN_ERR "Failed to register sensor!\n");
        goto failed_to_register_sensor;
    }

    //tuning init
    ret = isp_core_tuning_init(drv);
    if (ret != 0) {
        printk(KERN_ERR "Failed to init tuning module!\n");
        ret = -EINVAL;
        goto failed_to_tuning;
    }

    drv->camera.sensor = sensor;
    drv->camera.is_power_on = 0;
    drv->camera.is_stream_on = 0;

    mutex_unlock(&drv->lock);

    return 0;

failed_to_tuning:
    mscaler_unregister_sensor(index, sensor);
failed_to_register_sensor:
    mutex_unlock(&drv->lock);
    return ret;
}

void isp_component_unbind_sensor(int index, struct sensor_attr *sensor)
{
    struct jz_isp_data *drv = &jz_isp_dev[index];

    assert(drv->is_finish > 0);
    assert(drv->camera.sensor);
    assert(sensor == drv->camera.sensor);

    mutex_lock(&drv->lock);

    isp_core_tuning_deinit(drv);

    mscaler_unregister_sensor(index, sensor);

    drv->camera.sensor = NULL;

    mutex_unlock(&drv->lock);
}

int jz_isp_drv_init(int index)
{
    struct jz_isp_data *drv = &jz_isp_dev[index];
    int ret;

    mutex_init(&drv->lock);

    ret = request_threaded_irq(drv->irq, isp_irq_handler, isp_irq_thread_handler, IRQF_ONESHOT, drv->irq_name, (void *)drv);
    if (ret) {
        printk(KERN_ERR "camera: isp%d failed to request irq\n", index);
        goto error_request_irq;
    }
    disable_irq(drv->irq);

    ret = jz_mscaler_drv_init(index);
    if (ret) {
        printk(KERN_ERR "camera: failed to init mscaler\n");
        goto error_mscaler_init;
    }

#ifdef SOC_CAMERA_DEBUG
    ret = dsysfs_create_group(&drv->dsysfs_kobj, dsysfs_get_root_dir(index), "isp", &dsysfs_isp_attr_group);
    if (ret) {
        printk(KERN_ERR "isp%d dsysfs create sub dir isp fail\n", index);
        goto error_dsys_create_isp;
    }
#endif

    drv->isp_err = 0;
    drv->isp_err1 = 0;
    drv->isp_overflow = 0;
    drv->isp_breakfrm = 0;

    drv->camera.is_stream_on = 0;
    drv->is_finish = 1;

    printk(KERN_DEBUG "isp%d initialization successfully\n", index);

    return 0;

#ifdef SOC_CAMERA_DEBUG
error_dsys_create_isp:
    jz_mscaler_drv_deinit(index);
#endif
error_mscaler_init:
    free_irq(drv->irq, drv);
error_request_irq:
    return ret;
}

void jz_isp_drv_deinit(int index)
{
    struct jz_isp_data *drv = &jz_isp_dev[index];

    if (!drv->is_finish)
        return ;

    drv->is_finish = 0;
    free_irq(drv->irq, drv);

    jz_mscaler_drv_deinit(index);

#ifdef SOC_CAMERA_DEBUG
    dsysfs_remove_group(&drv->dsysfs_kobj, &dsysfs_isp_attr_group);
#endif

    return ;
}


EXPORT_SYMBOL(system_reg_write);
EXPORT_SYMBOL(system_reg_read);
