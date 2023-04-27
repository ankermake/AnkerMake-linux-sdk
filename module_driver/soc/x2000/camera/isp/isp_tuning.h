#ifndef __ISP_TUNING_H__
#define __ISP_TUNING_H__

#include <linux/sched.h>
#include <media/v4l2-ioctl.h>
#include <media/v4l2-ctrls.h>
#include "isp.h"



#define VIDIOC_PRIVATE_G_CTRL     _IOWR('V', BASE_VIDIOC_PRIVATE + 1, struct v4l2_control)
#define VIDIOC_PRIVATE_S_CTRL     _IOWR('V', BASE_VIDIOC_PRIVATE + 2, struct v4l2_control)




typedef enum isp_core_module_ops_mode {
    ISPCORE_MODULE_DISABLE,
    ISPCORE_MODULE_ENABLE,
    ISPCORE_MODULE_BUTT,            /**< 用于判断参数的有效性，参数大小必须小于这个值 */
} ISPMODULE_OPS_MODE_E;

typedef enum isp_core_module_ops_type {
    ISPCORE_MODULE_AUTO,
    ISPCORE_MODULE_MANUAL,
} ISPMODULE_OPS_TYPE_E;

/* the defination of mode of isp during the day or night */
typedef enum isp_core_mode_day_and_night {
    ISP_CORE_RUNING_MODE_DAY_MODE,
    ISP_CORE_RUNING_MODE_NIGHT_MODE,
    ISP_CORE_RUNING_MODE_BUTT,
} ISP_CORE_MODE_DN_E;

struct isp_core_weight_attr{
    unsigned char weight[15][15];
};

struct isp_core_ae_sta_info{
    unsigned char ae_histhresh[4];
    unsigned short ae_hist[5];
    unsigned char ae_stat_nodeh;
    unsigned char ae_stat_nodev;
};

/* ev */
struct isp_core_ev_attr {
    unsigned int ae_manual;
    unsigned int ev;
    unsigned int integration_time;
    unsigned int min_integration_time;
    unsigned int max_integration_time;
    unsigned int integration_time_us;
    unsigned int sensor_again;
    unsigned int max_sensor_again;
    unsigned int sensor_dgain;
    unsigned int max_sensor_dgain;
    unsigned int isp_dgain;
    unsigned int max_isp_dgain;
    unsigned int total_gain;
};

/* expr */
enum isp_core_expr_mode {
    ISP_CORE_EXPR_MODE_AUTO = 0,
    ISP_CORE_EXPR_MODE_MANUAL,
};

enum isp_core_integration_time_unit {
    ISP_CORE_INTEGRATION_TIME_UNIT_LINE,
    ISP_CORE_INTEGRATION_TIME_UNIT_US,
};

struct isp_core_integration_time {
    enum isp_core_integration_time_unit unit;
    unsigned int time;
};

struct isp_core_expr_attr {
    enum isp_core_expr_mode mode;
    struct isp_core_integration_time integration_time;
    unsigned int again;
};

/* awb */
enum isp_core_wb_mode {
    ISP_CORE_WB_MODE_AUTO = 0,
    ISP_CORE_WB_MODE_MANUAL,
    ISP_CORE_WB_MODE_DAY_LIGHT,
    ISP_CORE_WB_MODE_CLOUDY,
    ISP_CORE_WB_MODE_INCANDESCENT,
    ISP_CORE_WB_MODE_FLOURESCENT,
    ISP_CORE_WB_MODE_TWILIGHT,
    ISP_CORE_WB_MODE_SHADE,
    ISP_CORE_WB_MODE_WARM_FLOURESCENT,
    ISP_CORE_WB_MODE_CUSTOM,
};

struct isp_core_wb_attr {
    enum isp_core_wb_mode mode;
    unsigned short rgain;
    unsigned short bgain;
};

struct isp_core_gamma_attr{
    unsigned short gamma[129];
};

/* isp core tuning */
enum isp_tuning_private_cmd_id {
    ISP_TUNING_CID_MODULE_CONTROL       = V4L2_CID_PRIVATE_BASE,
    ISP_TUNING_CID_DAY_OR_NIGHT,
    ISP_TUNING_CID_CONTROL_FPS,
    ISP_TUNING_CID_AE                   = V4L2_CID_PRIVATE_BASE + 0x40 * 1,
    ISP_TUNING_CID_AE_ZONE,
    ISP_TUNING_CID_AE_HIST,
    ISP_TUNING_CID_AE_ROI,
    ISP_TUNING_CID_AE_WEIGHT,
    ISP_TUNING_CID_EV_ATTR,
    ISP_TUNING_CID_EXPR_ATTR,
    ISP_TUNING_CID_AGAIN_ATTR,
    ISP_TUNING_CID_MAX_AGAIN,
    ISP_TUNING_CID_MAX_DGAIN,
    ISP_TUNING_CID_TOTAL_GAIN,
    ISP_TUNING_CID_AE_COMP,
    ISP_TUNING_CID_AE_MIN,
    ISP_TUNING_CID_EV_START,
    ISP_TUNING_CID_AE_LUMA,
    ISP_TUNING_CID_HILIGHT_DEPRESS_STRENGTH,
    ISP_TUNING_CID_AWB                  = V4L2_CID_PRIVATE_BASE + 0x40 * 2,
    ISP_TUNING_CID_WB_ATTR,
    ISP_TUNING_CID_WB_STATIS_GOL_ATTR,
    ISP_TUNING_CID_WB_STATIS_ATTR,
    ISP_TUNING_CID_AWB_WEIGHT,
    ISP_TUNING_CID_AWB_HIST,
    ISP_TUNING_CID_AF                   = V4L2_CID_PRIVATE_BASE + 0x40 * 3,
    ISP_TUNING_CID_AF_HIST,
    ISP_TUNING_CID_AF_WEIGHT,
    ISP_TUNING_CID_AF_METRIC,
    ISP_TUNING_CID_GAMMA                = V4L2_CID_PRIVATE_BASE + 0x40 * 4,
    ISP_TUNING_CID_GAMMA_ATTR,
    ISP_TUNING_CID_DMSC                 = V4L2_CID_PRIVATE_BASE + 0x40 * 5,
    ISP_TUNING_CID_DMSC_ATTR,
    ISP_TUNING_CID_FC_ATTR,
    ISP_TUNING_CID_MDNS                 = V4L2_CID_PRIVATE_BASE + 0x40 * 6,
    ISP_TUNING_CID_GET_NCU_INFO,
    ISP_TUNING_CID_3DNS_RATIO,
    ISP_TUNING_CID_ADR                  = V4L2_CID_PRIVATE_BASE + 0x40 * 7,
    ISP_TUNING_CID_ADR_RATIO,
    ISP_TUNING_CID_SDNS                 = V4L2_CID_PRIVATE_BASE + 0x40 * 8,
    ISP_TUNING_CID_DPC                  = V4L2_CID_PRIVATE_BASE + 0x40 * 9,
    ISP_TUNING_CID_BLC                  = V4L2_CID_PRIVATE_BASE + 0x40 * 10,
    ISP_TUNING_CID_LSC                  = V4L2_CID_PRIVATE_BASE + 0x40 * 11,
    ISP_TUNING_CID_CCM                  = V4L2_CID_PRIVATE_BASE + 0x40 * 12,
    ISP_TUNING_CID_WDR                  = V4L2_CID_PRIVATE_BASE + 0x40 * 13,
};


struct isp_core_tuning_driver {
    char device_name[16];
    struct miscdevice mdev;

    struct jz_isp_data *parent;        // which is that the driver belongs to.
    tisp_core_tuning_t *core_tuning;

    struct mutex mlock;
    isp_state_t state;
    int (*event)(struct isp_core_tuning_driver *tuning, unsigned int event, void *data);
};

#define mdev_to_tuningdriver(dev) (container_of(dev, struct isp_core_tuning_driver, mdev))

int isp_core_tuning_init(struct jz_isp_data *parent);
void isp_core_tuning_deinit(struct jz_isp_data *parent);


#endif //__ISP_TUNING_H__
