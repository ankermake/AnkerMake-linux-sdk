#include <linux/videodev2.h>
#include <linux/delay.h>
#include <linux/slab.h>
#include <linux/miscdevice.h>
#include <linux/ioctl.h>

#include "isp_tuning.h"
#include "isp-core/inc/tiziano_core_tuning.h"



static int isp_module_g_attr(struct isp_core_tuning_driver *tuning, struct v4l2_control *ctrl)
{
    tisp_module_control_t module;
    int ret = 0;

    tisp_g_module_control(tuning->core_tuning, &module);
    copy_to_user((void __user*)ctrl->value, &module, sizeof(tisp_module_control_t));

    return ret;
}

static int isp_module_s_attr(struct isp_core_tuning_driver *tuning, struct v4l2_control *ctrl)
{
    tisp_module_control_t module;

    copy_from_user(&module, (const void __user*)ctrl->value, sizeof(tisp_module_control_t));
    tisp_s_module_control(tuning->core_tuning, module);

    return 0;
}

static int isp_day_or_night_s_ctrl(struct isp_core_tuning_driver *tuning, struct v4l2_control *ctrl)
{
    struct jz_isp_data *isp = tuning->parent;
    tisp_core_tuning_t *core_tuning = tuning->core_tuning;

    if(core_tuning->day_night != !!ctrl->value){
        isp->dn_state = !!ctrl->value;
        isp->daynight_change = 1;
    }

    return 0;
}

static int isp_hflip_s_control(struct isp_core_tuning_driver *tuning, struct v4l2_control *ctrl)
{
    struct jz_isp_data *isp = tuning->parent;
    tisp_core_tuning_t *core_tuning = tuning->core_tuning;

    if(core_tuning->hflip != !!ctrl->value){
        isp->hflip_state = !!ctrl->value;
        isp->hflip_change = 1;
    }

    return 0;
}

static int isp_vflip_s_control(struct isp_core_tuning_driver *tuning, struct v4l2_control *ctrl)
{
    struct jz_isp_data *isp = tuning->parent;
    tisp_core_tuning_t *core_tuning = tuning->core_tuning;

    if(core_tuning->vflip != !!ctrl->value){
        isp->vflip_state = !!ctrl->value;
        isp->vflip_change = 1;
    }

    return 0;
}

static int isp_fps_g_control(struct isp_core_tuning_driver *tuning, struct v4l2_control *ctrl)
{
    struct jz_isp_data *isp = tuning->parent;
    struct sensor_attr *attr = isp->camera.sensor;

    ctrl->value = attr->sensor_info.fps;

    return 0;
}

static int isp_fps_s_control(struct isp_core_tuning_driver *tuning, struct v4l2_control *ctrl)
{
    struct jz_isp_data *isp = tuning->parent;
    struct sensor_attr *attr = isp->camera.sensor;
    unsigned int fps = ctrl->value;
    int ret = 0;

    if(attr->ops.set_fps){
        ret = attr->ops.set_fps(fps);
        if (ret != 0) {
            printk(KERN_ERR "Failed to set sensor fps=0x%x, ret=%d\n", fps, ret);
            return ret;
        }
        tisp_set_fps(tuning->core_tuning, fps);
    } else {
        printk(KERN_ERR "attr->ops.set_fps is NULL\n");
        return -EINVAL;
    }

    return 0;
}

static int isp_brightness_g_ctrl(struct isp_core_tuning_driver *tuning, struct v4l2_control *ctrl)
{
    ctrl->value = (int)tisp_get_brightness(tuning->core_tuning);
    return 0;
}

static int isp_brightness_s_ctrl(struct isp_core_tuning_driver *tuning, struct v4l2_control *ctrl)
{
    tisp_set_brightness(tuning->core_tuning, ctrl->value & 0xff);
    return 0;
}

static int isp_contrast_g_ctrl(struct isp_core_tuning_driver *tuning, struct v4l2_control *ctrl)
{
    ctrl->value = (int)tisp_get_contrast(tuning->core_tuning);
    return 0;
}

static int isp_contrast_s_ctrl(struct isp_core_tuning_driver *tuning, struct v4l2_control *ctrl)
{
    tisp_set_contrast(tuning->core_tuning, ctrl->value & 0xff);
    return 0;
}

static int isp_saturation_g_ctrl(struct isp_core_tuning_driver *tuning, struct v4l2_control *ctrl)
{
    ctrl->value = (int)tisp_get_saturation(tuning->core_tuning);
    return 0;
}

static int isp_saturation_s_ctrl(struct isp_core_tuning_driver *tuning, struct v4l2_control *ctrl)
{
    tisp_set_saturation(tuning->core_tuning, ctrl->value & 0xff);
    return 0;
}

static int isp_sharpness_g_ctrl(struct isp_core_tuning_driver *tuning, struct v4l2_control *ctrl)
{
    ctrl->value = (int)tisp_get_sharpness(tuning->core_tuning);
    return 0;
}

static int isp_sharpness_s_ctrl(struct isp_core_tuning_driver *tuning, struct v4l2_control *ctrl)
{
    tisp_set_sharpness(tuning->core_tuning, ctrl->value & 0xff);
    return 0;
}

static int isp_flicker_g_ctrl(struct isp_core_tuning_driver *tuning, struct v4l2_control *ctrl)
{
    tisp_core_tuning_t *core_tuning = tuning->core_tuning;

    switch(core_tuning->flicker_hz){
        case 0:
            ctrl->value = V4L2_CID_POWER_LINE_FREQUENCY_DISABLED;
            break;
        case 50:
            ctrl->value = V4L2_CID_POWER_LINE_FREQUENCY_50HZ;
            break;
        case 60:
            ctrl->value = V4L2_CID_POWER_LINE_FREQUENCY_60HZ;
            break;
        default:
            printk(KERN_ERR "%s: Can not support this val:%d\n", __func__, core_tuning->flicker_hz);
            return -EINVAL;
    }
    return 0;
}

static int isp_flicker_s_ctrl(struct isp_core_tuning_driver *tuning, struct v4l2_control *ctrl)
{
    int hz;

    switch(ctrl->value){
        case V4L2_CID_POWER_LINE_FREQUENCY_DISABLED:
            hz = 0;
            break;
        case V4L2_CID_POWER_LINE_FREQUENCY_50HZ:
            hz = 50;
            break;
        case V4L2_CID_POWER_LINE_FREQUENCY_60HZ:
            hz = 60;
            break;
        default:
            printk(KERN_ERR "%s: Can not support this val:%d\n", __func__, ctrl->value);
            return -EINVAL;
    }

    tisp_s_antiflick(tuning->core_tuning, hz);

    return 0;
}

static int isp_ae_min_g_attr(struct isp_core_tuning_driver *tuning, struct v4l2_control *ctrl)
{
    tisp_ae_ex_min_t ae_min;
    int ret = 0;

    ret = tisp_g_ae_min(tuning->core_tuning, &ae_min);
    if(ret == 0)
        copy_to_user((void __user*)ctrl->value, &ae_min, sizeof(ae_min));

    return ret;
}

static int isp_ae_min_s_attr(struct isp_core_tuning_driver *tuning, struct v4l2_control *ctrl)
{
    tisp_ae_ex_min_t ae_min;

    if (0 == ctrl->value) {
        return -EFAULT;
    } else {
        copy_from_user(&ae_min, (const void __user*)ctrl->value, sizeof(ae_min));
    }

    return tisp_s_ae_min(tuning->core_tuning, ae_min);
}

static int isp_ev_start_s_ctrl(struct isp_core_tuning_driver *tuning, struct v4l2_control *ctrl)
{
    unsigned int ev_start = ctrl->value;

    if(ev_start < 0){
        printk(KERN_ERR "ev_start value overflow!\n");
        return -1;
    }

    tisp_s_ev_start(tuning->core_tuning, ev_start);

    return 0;
}

static int isp_ev_g_attr(struct isp_core_tuning_driver *tuning, struct v4l2_control *ctrl)
{
    struct isp_core_ev_attr ev_attr;
    tisp_ev_attr_t tev_attr;

    tisp_g_ev_attr(tuning->core_tuning, &tev_attr);
    ev_attr.ae_manual = tev_attr.ae_manual;
    ev_attr.ev = tev_attr.ev;
    ev_attr.integration_time = tev_attr.integration_time;
    ev_attr.min_integration_time = tev_attr.min_integration_time;
    ev_attr.max_integration_time = tev_attr.max_integration_time;
    ev_attr.integration_time_us = tev_attr.integration_time_us;
    ev_attr.sensor_again = tev_attr.sensor_again;
    ev_attr.max_sensor_again = tev_attr.max_sensor_again;
    ev_attr.sensor_dgain = tev_attr.sensor_dgain;
    ev_attr.max_sensor_dgain = tev_attr.max_sensor_dgain;
    ev_attr.isp_dgain = tev_attr.isp_dgain;
    ev_attr.max_isp_dgain = tev_attr.max_isp_dgain;
    ev_attr.total_gain = tev_attr.total_gain;

    copy_to_user((void __user*)ctrl->value, &ev_attr, sizeof(ev_attr));

    return 0;
}

static int isp_expr_g_ctrl(struct isp_core_tuning_driver *tuning, struct v4l2_control *ctrl)
{
    struct isp_core_expr_attr expr_attr;
    tisp_ev_attr_t ae_attr;

    copy_from_user(&expr_attr, (const void __user*)ctrl->value, sizeof(expr_attr));

    tisp_g_ev_attr(tuning->core_tuning, &ae_attr);
    if (ae_attr.ae_manual == 0)
        expr_attr.mode = ISP_CORE_EXPR_MODE_AUTO;
    else if (ae_attr.ae_manual == 1)
        expr_attr.mode = ISP_CORE_EXPR_MODE_MANUAL;
    if (expr_attr.integration_time.unit == ISP_CORE_INTEGRATION_TIME_UNIT_LINE)
        expr_attr.integration_time.time = ae_attr.integration_time;
    else if (expr_attr.integration_time.unit == ISP_CORE_INTEGRATION_TIME_UNIT_US)
        expr_attr.integration_time.time = ae_attr.integration_time_us;
    expr_attr.again = ae_attr.sensor_again;

    copy_to_user((void __user*)ctrl->value, &expr_attr, sizeof(expr_attr));

    return 0;
}

static int isp_expr_s_ctrl(struct isp_core_tuning_driver *tuning, struct v4l2_control *ctrl)
{
    struct jz_isp_data *isp = tuning->parent;
    struct sensor_info *sensor_info = &isp->camera.sensor->sensor_info;
    struct isp_core_expr_attr expr_attr;
    tisp_ev_attr_t ae_attr, ae_attr_get;

    copy_from_user(&expr_attr, (const void __user*)ctrl->value, sizeof(expr_attr));

    if (expr_attr.mode == ISP_CORE_EXPR_MODE_AUTO) {
        ae_attr.ae_manual = 0;
    } else if (expr_attr.mode == ISP_CORE_EXPR_MODE_MANUAL) {
        ae_attr.ae_manual = 1;
        ae_attr.sensor_again = expr_attr.again;
        if ((expr_attr.integration_time.unit == ISP_CORE_INTEGRATION_TIME_UNIT_LINE)) {
            ae_attr.integration_time = expr_attr.integration_time.time;
        } else if((expr_attr.integration_time.unit == ISP_CORE_INTEGRATION_TIME_UNIT_US)) {
#if 0
            //value overflow
            ae_attr.integration_time = expr_attr.integration_time.time * (sensor_info->fps >> 16)/(sensor_info->fps & 0xffff) *
                                        sensor_info->total_height / 1000000;
#else
            unsigned long long t1 = (unsigned long long)expr_attr.integration_time.time * sensor_info->total_height * (sensor_info->fps >> 16);
            do_div(t1, 1000000);
            ae_attr.integration_time = (unsigned int)t1;
            ae_attr.integration_time =  ae_attr.integration_time / (sensor_info->fps & 0xffff);
#endif
        } else {
            printk(KERN_ERR "%s, can not support this unit: %d\n", __func__, expr_attr.integration_time.unit);
            return -EINVAL;
        }

        tisp_g_ev_attr(tuning->core_tuning, &ae_attr_get);
        if (ae_attr.integration_time < ae_attr_get.min_integration_time ||
            ae_attr.integration_time > ae_attr_get.max_integration_time) {
            printk(KERN_ERR "%s, integration_time:%u out of range[%u - %u] line\n", __func__, ae_attr.integration_time,
                                                ae_attr_get.min_integration_time, ae_attr_get.max_integration_time);
            return -EINVAL;
        }
        if (ae_attr.sensor_again > ae_attr_get.max_sensor_again) {
            printk(KERN_ERR "%s, again:%u out of range[1024 - %u]\n", __func__, ae_attr.sensor_again, ae_attr_get.max_sensor_again);
            return -EINVAL;
        }
    } else {
        printk(KERN_ERR "%s, can not support this mode: %d\n", __func__, expr_attr.mode);
        return -EINVAL;
    }

    tisp_s_ae_attr(tuning->core_tuning, ae_attr);

    return 0;
}

static int isp_max_again_g_ctrl(struct isp_core_tuning_driver *tuning, struct v4l2_control *ctrl)
{
    tisp_ev_attr_t ev_attr;
    int ret = 0;

    ret = tisp_g_ev_attr(tuning->core_tuning, &ev_attr);
    if(ret != 0){
        printk(KERN_ERR "tisp_g_ev_attr failed!\n");
        return ret;
    }

    ctrl->value = ev_attr.max_sensor_again;

    return 0;
}

static int isp_max_again_s_ctrl(struct isp_core_tuning_driver *tuning, struct v4l2_control *ctrl)
{
    unsigned int max_again = ctrl->value;

    tisp_s_max_again(tuning->core_tuning, max_again);

    return 0;
}

static int isp_max_dgain_g_ctrl(struct isp_core_tuning_driver *tuning, struct v4l2_control *ctrl)
{
    tisp_ev_attr_t ev_attr;
    int ret = 0;

    ret = tisp_g_ev_attr(tuning->core_tuning, &ev_attr);
    if(ret != 0){
        printk(KERN_ERR "tisp_g_ev_attr failed!\n");
        return ret;
    }

    ctrl->value = ev_attr.max_isp_dgain;

    return 0;
}

static int isp_max_dgain_s_ctrl(struct isp_core_tuning_driver *tuning, struct v4l2_control *ctrl)
{
    unsigned int max_isp_dgain = ctrl->value;

    tisp_s_max_isp_dgain(tuning->core_tuning, max_isp_dgain);

    return 0;
}

/* the format of return value is 8.8 */
static int isp_tgain_g_ctrl(struct isp_core_tuning_driver *tuning, struct v4l2_control *ctrl)
{
    tisp_ev_attr_t ev_attr;
    int ret = 0;

    ret = tisp_g_ev_attr(tuning->core_tuning, &ev_attr);
    if(ret != 0){
        printk(KERN_ERR "tisp_g_ev_attr failed!\n");
        return ret;
    }

    ctrl->value = ev_attr.total_gain;

    return 0;
}

static int isp_ae_luma_g_ctrl(struct isp_core_tuning_driver *tuning, struct v4l2_control *ctrl)
{
    unsigned char luma;

    tisp_g_ae_luma(tuning->core_tuning, &luma);
    ctrl->value = luma;

    return 0;
}

static int isp_hi_light_depress_g_ctrl(struct isp_core_tuning_driver *tuning, struct v4l2_control *ctrl)
{
    unsigned int strength;
    int ret = 0;

    ret = tisp_g_Hilightdepress(tuning->core_tuning, &strength);
    if(ret != 0){
        printk(KERN_ERR "tisp_g_Hilightdepress failed!\n");
        return ret;
    }
    ctrl->value = strength;

    return 0;
}

static int isp_hi_light_depress_s_ctrl(struct isp_core_tuning_driver *tuning, struct v4l2_control *ctrl)
{
    unsigned int strength = ctrl->value;
    int ret = 0;

    ret = tisp_s_Hilightdepress(tuning->core_tuning, strength);
    if(ret != 0)
        printk(KERN_ERR "tisp_s_Hilightdepress failed!\n");

    return ret;
}

static int isp_ae_zone_weight_g_attr(struct isp_core_tuning_driver *tuning, struct v4l2_control *ctrl)
{
    struct isp_core_weight_attr attr;
    int ret = 0;
    int cols,rows;

    tisp_3a_weight_t *zone_weight = kmalloc(sizeof(tisp_3a_weight_t), GFP_KERNEL);
    if(!zone_weight){
        printk(KERN_ERR "Failed to kmalloc ae_zone_weight\n");
        return -ENOMEM;
    }

    ret = tisp_g_aezone_weight(tuning->core_tuning, zone_weight);
    if(ret != 0){
        printk(KERN_ERR "tisp_g_aezone_weight failed!\n");
        goto err_get_zone_weight;
    }

    for(cols = 0;cols < 15;cols++){
        for(rows = 0;rows < 15;rows++){
            attr.weight[cols][rows] = (unsigned char)(zone_weight->weight[rows+cols*15] & 0xff);
        }
    }

    copy_to_user((void __user*)ctrl->value, &attr, sizeof(attr));

err_get_zone_weight:
    kfree(zone_weight);
    return ret;
}

static int isp_ae_zone_weight_s_attr(struct isp_core_tuning_driver *tuning, struct v4l2_control *ctrl)
{
    struct isp_core_weight_attr attr;
    int ret = 0;
    int cols,rows;

    tisp_3a_weight_t *zone_weight = kmalloc(sizeof(tisp_3a_weight_t), GFP_KERNEL);
    if(!zone_weight){
        printk(KERN_ERR "Failed to kmalloc ae_zone_weight\n");
        return -ENOMEM;
    }

    if (0 == ctrl->value) {
        ret =  -EFAULT;
        goto err_zone_weight_control;
    } else {
        copy_from_user(&attr, (const void __user*)ctrl->value, sizeof(attr));
    }

    for(cols = 0;cols < 15;cols++){
        for(rows = 0;rows < 15;rows++){
            if(attr.weight[cols][rows] > 8){
                printk(KERN_ERR "%s:ae zone weight overflow!\n", __func__);
                ret = -1;
                goto err_zone_weight_overflow;
            }
            zone_weight->weight[rows+cols*15] = (unsigned int)attr.weight[cols][rows];
        }
    }

    ret = tisp_s_aezone_weight(tuning->core_tuning, zone_weight);
    if(ret != 0)
        printk(KERN_ERR "tisp_s_aezone_weight failed!\n");

err_zone_weight_overflow:
err_zone_weight_control:
    kfree(zone_weight);
    return ret;

    return 0;
}

static int isp_ae_g_roi(struct isp_core_tuning_driver *tuning, struct v4l2_control *ctrl)
{
    struct isp_core_weight_attr attr;
    int ret = 0;
    int cols,rows;

    tisp_3a_weight_t *roi_weight = kmalloc(sizeof(tisp_3a_weight_t), GFP_KERNEL);
    if(!roi_weight){
        printk(KERN_ERR "Failed to private_kmalloc roi_weight\n");
        return -ENOMEM;
    }

    ret = tisp_g_aeroi_weight(tuning->core_tuning, roi_weight);
    if(ret != 0){
        printk(KERN_ERR "tisp_g_aeroi_weight failed!\n");
        goto err_get_roi_weight;
    }

    for(cols = 0;cols < 15;cols++){
        for(rows = 0;rows < 15;rows++){
            attr.weight[cols][rows] = (unsigned char)(roi_weight->weight[rows+cols*15] & 0xff);
        }
    }

    copy_to_user((void __user*)ctrl->value, &attr, sizeof(attr));

err_get_roi_weight:
    kfree(roi_weight);
    return ret;
}

static int isp_ae_s_roi(struct isp_core_tuning_driver *tuning, struct v4l2_control *ctrl)
{
    struct isp_core_weight_attr attr;
    int ret = 0;
    int cols,rows;

    tisp_3a_weight_t *roi_weight = kmalloc(sizeof(tisp_3a_weight_t), GFP_KERNEL);
    if(!roi_weight){
        printk(KERN_ERR "Failed to private_kmalloc roi_weight\n");
        return -ENOMEM;
    }

    if (0 == ctrl->value) {
        ret =  -EFAULT;
        goto err_roi_weight_control;
    } else {
        copy_from_user(&attr, (const void __user*)ctrl->value, sizeof(attr));
    }

    for(cols = 0;cols < 15;cols++){
        for(rows = 0;rows < 15;rows++){
            if(attr.weight[cols][rows] > 8){
                printk(KERN_ERR "%s:ae weight overflow!\n", __func__);
                ret = -1;
                goto err_roi_weight_overflow;
            }
            roi_weight->weight[rows+cols*15] = (unsigned int)attr.weight[cols][rows];
        }
    }

    ret = tisp_s_aeroi_weight(tuning->core_tuning, roi_weight);
    if(ret != 0)
        printk(KERN_ERR "tisp_s_aeroi_weight failed!\n");

err_roi_weight_overflow:
err_roi_weight_control:
    kfree(roi_weight);
    return ret;
}

static int isp_ae_zone_g_ctrl(struct isp_core_tuning_driver *tuning, struct v4l2_control *ctrl)
{
    tisp_zone_info_t ae_zone;

    tisp_g_ae_zone(tuning->core_tuning, &ae_zone);
    copy_to_user((void __user*)ctrl->value, &ae_zone, sizeof(tisp_zone_info_t));

    return 0;
}

static int isp_ae_hist_g_attr(struct isp_core_tuning_driver *tuning, struct v4l2_control *ctrl)
{
    struct isp_core_ae_sta_info info;
    tisp_ae_sta_t *ae_hist = kmalloc(sizeof(tisp_ae_sta_t), GFP_KERNEL);
    if(!ae_hist){
        printk(KERN_ERR "Failed to kmalloc ae_hist\n");
        return -1;
    }

    tisp_g_ae_hist(tuning->core_tuning, ae_hist);

    info.ae_histhresh[0] = ae_hist->ae_hist_nodes[0];
    info.ae_histhresh[1] = ae_hist->ae_hist_nodes[1];
    info.ae_histhresh[2] = ae_hist->ae_hist_nodes[2];
    info.ae_histhresh[3] = ae_hist->ae_hist_nodes[3];

    info.ae_hist[0] = ae_hist->ae_hist_5bin[0];
    info.ae_hist[1] = ae_hist->ae_hist_5bin[1];
    info.ae_hist[2] = ae_hist->ae_hist_5bin[2];
    info.ae_hist[3] = ae_hist->ae_hist_5bin[3];
    info.ae_hist[4] = ae_hist->ae_hist_5bin[4];

    info.ae_stat_nodeh = ae_hist->ae_hist_hv[0];
    info.ae_stat_nodev = ae_hist->ae_hist_hv[1];

    copy_to_user((void __user*)ctrl->value, &info, sizeof(info));
    kfree(ae_hist);

    return 0;
}

static int isp_ae_hist_s_attr(struct isp_core_tuning_driver *tuning, struct v4l2_control *ctrl)
{
    struct isp_core_ae_sta_info info;
    tisp_ae_sta_t *ae_hist = kmalloc(sizeof(tisp_ae_sta_t), GFP_KERNEL);
    if(!ae_hist){
        printk(KERN_ERR "Failed to kmalloc ae_hist\n");
        return -1;
    }

    copy_from_user(&info, (const void __user*)ctrl->value, sizeof(info));

    ae_hist->ae_hist_nodes[0] = info.ae_histhresh[0];
    ae_hist->ae_hist_nodes[1] = info.ae_histhresh[1];
    ae_hist->ae_hist_nodes[2] = info.ae_histhresh[2];
    ae_hist->ae_hist_nodes[3] = info.ae_histhresh[3];

    tisp_s_ae_hist(tuning->core_tuning, *ae_hist);
    kfree(ae_hist);

    return 0;
}

static int isp_wb_g_ctrl(struct isp_core_tuning_driver *tuning, struct v4l2_control *ctrl)
{
    struct isp_core_wb_attr wb_attr;
    tisp_wb_attr_t twb_attr;
    int ret = 0;

    ret = tisp_g_wb_attr(tuning->core_tuning, &twb_attr);
    if(ret != 0){
        printk(KERN_ERR "tisp_g_wb_attr failed!\n");
        return ret;
    }

    switch(twb_attr.tisp_wb_manual){
        case 0:
            wb_attr.mode = ISP_CORE_WB_MODE_AUTO;
            break;
        case 1:
            wb_attr.mode = ISP_CORE_WB_MODE_MANUAL;
            break;
        case 2:
            wb_attr.mode = ISP_CORE_WB_MODE_DAY_LIGHT;
            break;
        case 3:
            wb_attr.mode = ISP_CORE_WB_MODE_CLOUDY;
            break;
        case 4:
            wb_attr.mode = ISP_CORE_WB_MODE_INCANDESCENT;
            break;
        case 5:
            wb_attr.mode = ISP_CORE_WB_MODE_FLOURESCENT;
            break;
        case 6:
            wb_attr.mode = ISP_CORE_WB_MODE_TWILIGHT;
            break;
        case 7:
            wb_attr.mode = ISP_CORE_WB_MODE_SHADE;
            break;
        case 8:
            wb_attr.mode = ISP_CORE_WB_MODE_WARM_FLOURESCENT;
            break;
        case 9:
            wb_attr.mode = ISP_CORE_WB_MODE_CUSTOM;
            break;
        default:
            return -EINVAL;
    }

    wb_attr.rgain = twb_attr.tisp_wb_rg;
    wb_attr.bgain = twb_attr.tisp_wb_bg;

    copy_to_user((void __user*)ctrl->value, &wb_attr, sizeof(wb_attr));

    return 0;
}

static int isp_wb_s_ctrl(struct isp_core_tuning_driver *tuning, struct v4l2_control *ctrl)
{
    struct isp_core_wb_attr wb_attr;
    int manual = 0;
    tisp_wb_attr_t tisp_wb_attr;

    copy_from_user(&wb_attr, (const void __user*)ctrl->value, sizeof(wb_attr));

    switch(wb_attr.mode){
        case ISP_CORE_WB_MODE_AUTO:
            manual = 0;
            break;
        case ISP_CORE_WB_MODE_MANUAL:
            manual = 1;
            break;
        case ISP_CORE_WB_MODE_DAY_LIGHT:
            manual = 2;
            break;
        case ISP_CORE_WB_MODE_CLOUDY:
            manual = 3;
            break;
        case ISP_CORE_WB_MODE_INCANDESCENT:
            manual = 4;
            break;
        case ISP_CORE_WB_MODE_FLOURESCENT:
            manual = 5;
            break;
        case ISP_CORE_WB_MODE_TWILIGHT:
            manual = 6;
            break;
        case ISP_CORE_WB_MODE_SHADE:
            manual = 7;
            break;
        case ISP_CORE_WB_MODE_WARM_FLOURESCENT:
            manual = 8;
            break;
        case ISP_CORE_WB_MODE_CUSTOM:
            manual = 9;
            break;
        default:
            printk(KERN_ERR "%s:Can not support this mode:%d\n", __func__, wb_attr.mode);
            return -EINVAL;
        }

    tisp_wb_attr.tisp_wb_manual = manual;
    tisp_wb_attr.tisp_wb_rg = wb_attr.rgain;
    tisp_wb_attr.tisp_wb_bg = wb_attr.bgain;
    tisp_s_wb_attr(tuning->core_tuning, tisp_wb_attr);

    return 0;
}

static int isp_wb_statis_g_ctrl(struct isp_core_tuning_driver *tuning, struct v4l2_control *ctrl)
{
    tisp_wb_attr_t wb_attr;

    tisp_g_wb_attr(tuning->core_tuning, &wb_attr);
    ctrl->value = ((wb_attr.tisp_wb_rg_sta_weight & 0xffff) << 16) +
        (wb_attr.tisp_wb_bg_sta_weight & 0xffff);

    return 0;
}

static int isp_wb_statis_global_g_ctrl(struct isp_core_tuning_driver *tuning, struct v4l2_control *ctrl)
{
    tisp_wb_attr_t wb_attr;

    tisp_g_wb_attr(tuning->core_tuning, &wb_attr);
    ctrl->value = ((wb_attr.tisp_wb_rg_sta_global & 0xffff) << 16) +
        (wb_attr.tisp_wb_bg_sta_global & 0xffff);

    return 0;
}

static int isp_adr_strength_g_ctrl(struct isp_core_tuning_driver *tuning, struct v4l2_control *ctrl)
{
    unsigned int strength;
    int ret = 0;

    ret = tisp_g_adr_strength(tuning->core_tuning, &strength);
    if(ret != 0){
        printk(KERN_ERR "tisp_g_adr_strength failed!\n");
        return ret;
    }
    ctrl->value = strength;

    return 0;
}

static int isp_adr_strength_s_ctrl(struct isp_core_tuning_driver *tuning, struct v4l2_control *ctrl)
{
    unsigned int strength = ctrl->value;
    int ret = 0;

    ret = tisp_s_adr_strength(tuning->core_tuning, strength);
    if(ret != 0)
        printk(KERN_ERR "tisp_s_adr_strength failed!\n");

    return ret;
}

static int isp_gamma_g_attr(struct isp_core_tuning_driver *tuning, struct v4l2_control *ctrl)
{
    struct isp_core_gamma_attr attr;
    tisp_gamma_lut_t tgamma;
    int nodes_num = 0;
    int ret = 0;

    ret = tisp_g_Gamma(tuning->core_tuning, &tgamma);
    if(ret != 0){
        printk(KERN_ERR "tisp_g_Gamma failed!\n");
        return ret;
    }
    for(nodes_num = 0; nodes_num < 129;nodes_num++)
        attr.gamma[nodes_num] = (unsigned short)(tgamma.gamma[nodes_num] & 0xffff);

    copy_to_user((void __user*)ctrl->value, (const void *)&attr, sizeof(attr));

    return 0;
}

static int isp_gamma_s_attr(struct isp_core_tuning_driver *tuning, struct v4l2_control *ctrl)
{
    struct isp_core_gamma_attr attr;
    tisp_gamma_lut_t tgamma;
    int nodes_num = 0;
    int ret = 0;

    if (0 == ctrl->value) {
        return -1;
    } else {
        copy_from_user(&attr, (const void __user*)ctrl->value, sizeof(attr));
    }
    for(nodes_num = 0; nodes_num < 129;nodes_num++)
        tgamma.gamma[nodes_num] = (unsigned int)attr.gamma[nodes_num];

    ret = tisp_s_Gamma(tuning->core_tuning, &tgamma);
    if(ret != 0)
        printk(KERN_ERR "tisp_s_Gamma failed!\n");

    return ret;
}

static int isp_core_ops_v4l2_g_ctrl(struct isp_core_tuning_driver *tuning, struct v4l2_control *ctrl)
{
    tisp_core_tuning_t *core_tuning = tuning->core_tuning;
    int ret = 0;

    switch(ctrl->id){
        case V4L2_CID_HFLIP:
            ctrl->value = core_tuning->hflip;
            break;
        case V4L2_CID_VFLIP:
            ctrl->value = core_tuning->vflip;
            break;
        case V4L2_CID_BRIGHTNESS:
            ret = isp_brightness_g_ctrl(tuning, ctrl);
            break;
        case V4L2_CID_CONTRAST:
            ret = isp_contrast_g_ctrl(tuning, ctrl);
            break;
        case V4L2_CID_SATURATION:
            ret = isp_saturation_g_ctrl(tuning, ctrl);
            break;
        case V4L2_CID_SHARPNESS:
            ret = isp_sharpness_g_ctrl(tuning, ctrl);
            break;
        case V4L2_CID_POWER_LINE_FREQUENCY:
            ret = isp_flicker_g_ctrl(tuning, ctrl);
            break;
        default:
            printk(KERN_ERR "%s, unknown ctrl cmd: %d !\n", __func__, ctrl->id);
            ret = -ENOIOCTLCMD;
    }
    return ret;
}

static int isp_core_ops_v4l2_s_ctrl(struct isp_core_tuning_driver *tuning, struct v4l2_control *ctrl)
{
    int ret = 0;

    switch (ctrl->id) {
        case V4L2_CID_HFLIP:
            ret = isp_hflip_s_control(tuning, ctrl);
            break;
        case V4L2_CID_VFLIP:
            ret = isp_vflip_s_control(tuning, ctrl);
            break;
        case V4L2_CID_BRIGHTNESS:
            ret = isp_brightness_s_ctrl(tuning, ctrl);
            break;
        case V4L2_CID_CONTRAST:
            ret = isp_contrast_s_ctrl(tuning, ctrl);
            break;
        case V4L2_CID_SATURATION:
            ret = isp_saturation_s_ctrl(tuning, ctrl);
            break;
        case V4L2_CID_SHARPNESS:
            ret = isp_sharpness_s_ctrl(tuning, ctrl);
            break;
        case V4L2_CID_POWER_LINE_FREQUENCY:
            ret = isp_flicker_s_ctrl(tuning, ctrl);
            break;
        default:
            printk(KERN_ERR "%s, unknown ctrl cmd: %d !\n", __func__, ctrl->id);
            ret = -ENOIOCTLCMD;
    }
    return ret;
}

static int isp_core_ops_private_g_ctrl(struct isp_core_tuning_driver *tuning, struct v4l2_control *ctrl)
{
    int ret = 0;

    switch(ctrl->id){
        case ISP_TUNING_CID_MODULE_CONTROL:
            ret = isp_module_g_attr(tuning, ctrl);
            break;
        case ISP_TUNING_CID_DAY_OR_NIGHT:
            ctrl->value = tuning->core_tuning->day_night;
            break;
        case ISP_TUNING_CID_CONTROL_FPS:
            ret = isp_fps_g_control(tuning, ctrl);
            break;
        case ISP_TUNING_CID_AE_ZONE:
            ret = isp_ae_zone_g_ctrl(tuning, ctrl);
            break;
        case ISP_TUNING_CID_AE_HIST:
            ret = isp_ae_hist_g_attr(tuning, ctrl);
            break;
        case ISP_TUNING_CID_AE_ROI:
            ret = isp_ae_g_roi(tuning, ctrl);
            break;
        case ISP_TUNING_CID_AE_WEIGHT:
            ret = isp_ae_zone_weight_g_attr(tuning, ctrl);
            break;
        case ISP_TUNING_CID_EV_ATTR:
            ret = isp_ev_g_attr(tuning, ctrl);
            break;
        case ISP_TUNING_CID_EXPR_ATTR:
            ret = isp_expr_g_ctrl(tuning, ctrl);
            break;
        case ISP_TUNING_CID_MAX_AGAIN:
            ret = isp_max_again_g_ctrl(tuning, ctrl);
            break;
        case ISP_TUNING_CID_MAX_DGAIN:
            ret = isp_max_dgain_g_ctrl(tuning, ctrl);
            break;
        case ISP_TUNING_CID_AE_COMP:
            ret = isp_brightness_g_ctrl(tuning, ctrl);
            break;
        case ISP_TUNING_CID_AE_MIN:
            ret = isp_ae_min_g_attr(tuning, ctrl);
            break;
        case ISP_TUNING_CID_TOTAL_GAIN:
            ret = isp_tgain_g_ctrl(tuning, ctrl);
            break;
        case ISP_TUNING_CID_AE_LUMA:
            ret = isp_ae_luma_g_ctrl(tuning, ctrl);
            break;
        case ISP_TUNING_CID_HILIGHT_DEPRESS_STRENGTH:
            ret = isp_hi_light_depress_g_ctrl(tuning, ctrl);
            break;
        case ISP_TUNING_CID_WB_ATTR:
            ret = isp_wb_g_ctrl(tuning, ctrl);
            break;
        case ISP_TUNING_CID_WB_STATIS_ATTR:
            ret = isp_wb_statis_g_ctrl(tuning, ctrl);
            break;
        case ISP_TUNING_CID_WB_STATIS_GOL_ATTR:
            ret = isp_wb_statis_global_g_ctrl(tuning, ctrl);
            break;
        case ISP_TUNING_CID_GAMMA_ATTR:
            ret = isp_gamma_g_attr(tuning, ctrl);
            break;
        case ISP_TUNING_CID_ADR_RATIO:
            ret = isp_adr_strength_g_ctrl(tuning, ctrl);
            break;
        default:
            printk(KERN_ERR "%s, unknown ctrl cmd: %d !\n", __func__, ctrl->id);
            ret = -ENOIOCTLCMD;
    }
    return ret;
}

static int isp_core_ops_private_s_ctrl(struct isp_core_tuning_driver *tuning, struct v4l2_control *ctrl)
{
    int ret = 0;

    switch (ctrl->id) {
        case ISP_TUNING_CID_MODULE_CONTROL:
            ret = isp_module_s_attr(tuning, ctrl);
            break;
        case ISP_TUNING_CID_DAY_OR_NIGHT:
            ret = isp_day_or_night_s_ctrl(tuning, ctrl);
            break;
        case ISP_TUNING_CID_CONTROL_FPS:
            ret = isp_fps_s_control(tuning, ctrl);
            break;
        case ISP_TUNING_CID_AE_ROI:
            ret = isp_ae_s_roi(tuning, ctrl);
            break;
        case ISP_TUNING_CID_AE_WEIGHT:
            ret = isp_ae_zone_weight_s_attr(tuning, ctrl);
            break;
        case ISP_TUNING_CID_AE_HIST:
            ret = isp_ae_hist_s_attr(tuning, ctrl);
            break;
        case ISP_TUNING_CID_EXPR_ATTR:
            ret = isp_expr_s_ctrl(tuning, ctrl);
            break;
        case ISP_TUNING_CID_MAX_AGAIN:
            ret = isp_max_again_s_ctrl(tuning, ctrl);
            break;
        case ISP_TUNING_CID_MAX_DGAIN:
            ret = isp_max_dgain_s_ctrl(tuning, ctrl);
            break;
        case ISP_TUNING_CID_AE_COMP:
            ret = isp_brightness_s_ctrl(tuning, ctrl);
            break;
        case ISP_TUNING_CID_AE_MIN:
            ret = isp_ae_min_s_attr(tuning, ctrl);
            break;
        case ISP_TUNING_CID_EV_START:
            ret = isp_ev_start_s_ctrl(tuning, ctrl);
            break;
        case ISP_TUNING_CID_HILIGHT_DEPRESS_STRENGTH:
            ret = isp_hi_light_depress_s_ctrl(tuning, ctrl);
            break;
        case ISP_TUNING_CID_WB_ATTR:
            ret = isp_wb_s_ctrl(tuning, ctrl);
            break;
        case ISP_TUNING_CID_GAMMA_ATTR:
            ret = isp_gamma_s_attr(tuning, ctrl);
            break;
        case ISP_TUNING_CID_ADR_RATIO:
            ret = isp_adr_strength_s_ctrl(tuning, ctrl);
            break;
        default:
            printk(KERN_ERR "%s, unknown ctrl cmd: %d !\n", __func__, ctrl->id);
            ret = -ENOIOCTLCMD;
    }
    return ret;
}

static long isp_core_tunning_unlocked_ioctl(struct file *file, unsigned int cmd, unsigned long arg)
{
    struct miscdevice *dev = file->private_data;
    struct isp_core_tuning_driver *tuning = mdev_to_tuningdriver(dev);
    struct v4l2_control ctrl;
    long ret = 0;

    mutex_lock(&tuning->mlock);
    /* the file must be opened firstly. */
    if(tuning->state < STATE_OPEN){
        ret = -EPERM;
        goto done;
    }

    if(copy_from_user(&ctrl, (void __user *)arg, sizeof(ctrl))) {
        ret = -EFAULT;
        goto done;
    }

    /* printk(KERN_ERR "##### %s cmd=0x%08x,id=0x%08x #####\n", __func__, cmd, ctrl.id); */
    switch(cmd){
        case VIDIOC_S_CTRL:
            ret = isp_core_ops_v4l2_s_ctrl(tuning, &ctrl);
            break;
        case VIDIOC_G_CTRL:
            ret = isp_core_ops_v4l2_g_ctrl(tuning, &ctrl);
            if(ret != 0)
                break;
            if (copy_to_user((void __user *)arg, &ctrl, sizeof(ctrl)))
                ret = -EFAULT;
            break;
        case VIDIOC_PRIVATE_S_CTRL:
            ret = isp_core_ops_private_s_ctrl(tuning, &ctrl);
            break;
        case VIDIOC_PRIVATE_G_CTRL:
            ret = isp_core_ops_private_g_ctrl(tuning, &ctrl);
            if(ret != 0)
                break;
            if (copy_to_user((void __user *)arg, &ctrl, sizeof(ctrl)))
                ret = -EFAULT;
            break;

        default:
            printk(KERN_ERR "%s, unknown ctrl cmd: %d !\n", __func__, cmd);
            ret = -ENOIOCTLCMD;
            break;
    }

done:
    mutex_unlock(&tuning->mlock);

    return ret;
}

static int isp_core_tunning_open(struct inode *inode, struct file *file)
{
#if 0
    struct miscdevice *dev = file->private_data;
    struct isp_core_tuning_driver *tuning = mdev_to_tuningdriver(dev);

    /* the module must be actived firstly. */
    if(tuning->state <= STATE_CLOSE){
        printk(KERN_ERR "isp tuning open fail, please stream on first\n");
        return -EPERM;
    }
#endif

    return 0;
}

static int isp_core_tunning_release(struct inode *inode, struct file *file)
{
    return 0;
}

static struct file_operations isp_core_tunning_fops = {
    .open = isp_core_tunning_open,
    .release = isp_core_tunning_release,
    .unlocked_ioctl = isp_core_tunning_unlocked_ioctl,
};

static int isp_core_tuning_activate(struct isp_core_tuning_driver *tuning)
{
    mutex_lock(&tuning->mlock);
    tuning->state = STATE_OPEN;
    mutex_unlock(&tuning->mlock);
    return 0;
}

static int isp_core_tuning_slake(struct isp_core_tuning_driver *tuning)
{
    mutex_lock(&tuning->mlock);
    tuning->state = STATE_CLOSE;
    mutex_unlock(&tuning->mlock);
    return 0;
}

static int isp_core_tuning_event(struct isp_core_tuning_driver *tuning, unsigned int event, void *data)
{
    int ret = 0;
    switch(event){
        case TISP_EVENT_ACTIVATE_MODULE:
            ret = isp_core_tuning_activate(tuning);
            break;
        case TISP_EVENT_SLAVE_MODULE:
            ret = isp_core_tuning_slake(tuning);
            break;
        default:
            break;
    }
    return ret;
}


static struct isp_core_tuning_driver tuning_dev[] = {
    {
        .mdev = {
            .minor = MISC_DYNAMIC_MINOR,
            .name  = "isp-tuning0",
            .fops  = &isp_core_tunning_fops,
        },
        .event      = isp_core_tuning_event,
        .state      = STATE_CLOSE,
    },
    {
        .mdev = {
            .minor = MISC_DYNAMIC_MINOR,
            .name  = "isp-tuning1",
            .fops  = &isp_core_tunning_fops,
        },
        .event      = isp_core_tuning_event,
        .state      = STATE_CLOSE,
    }
};

int isp_core_tuning_init(struct jz_isp_data *parent)
{
    int ret = misc_register(&tuning_dev[parent->index].mdev);
    if (ret != 0) {
        printk(KERN_ERR "Failed to register tuning dev!\n");
        return -1;
    }

    tuning_dev[parent->index].parent = parent;
    tuning_dev[parent->index].core_tuning = &parent->core.core_tuning;
    mutex_init(&tuning_dev[parent->index].mlock);

    parent->tuning = &tuning_dev[parent->index];

    return 0;
}

void isp_core_tuning_deinit(struct jz_isp_data *parent)
{
    misc_deregister(&tuning_dev[parent->index].mdev);
    tuning_dev[parent->index].state = STATE_CLOSE;
    parent->tuning = NULL;
}


