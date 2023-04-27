#include <linux/videodev2.h>
#include <linux/delay.h>
#include <linux/slab.h>
#include "isp-drv.h"

static inline int tx_isp_isp_hflip_s_control(struct isp_device *isp, struct v4l2_control *control)
{
	int hvflip;
	int value = control->value;

	if(value != 0 && value !=1){
		dev_err(isp->dev, "the available range of hflip is 0-1\n");
		return -EINVAL;
	}

	isp->ctrls.hflip = value;

	hvflip = (isp->ctrls.hflip << 0)|(isp->ctrls.vflip << 1);
	tisp_hv_flip_enable(isp->sensor_id, hvflip);

	return 0;
}

static inline int tx_isp_isp_hflip_g_control(struct isp_device *isp, struct v4l2_control *control)
{
	int ret = 0;
	control->value = isp->ctrls.hflip;
	return ret;
}

static inline int tx_isp_isp_vflip_s_control(struct isp_device *isp, struct v4l2_control *control)
{
	int hvflip;
	int value = control->value;

	if(value != 0 && value !=1){
		dev_err(isp->dev, "the available range of vflip is 0-1\n");
		return -EINVAL;
	}

	isp->ctrls.vflip = value;

	hvflip = (isp->ctrls.hflip << 0)|(isp->ctrls.vflip << 1);
	tisp_hv_flip_enable(isp->sensor_id, hvflip);

	return 0;
}

static inline int tx_isp_isp_vflip_g_control(struct isp_device *isp, struct v4l2_control *control)
{
	int ret = 0;
	control->value = isp->ctrls.vflip;
	return ret;
}

static inline int tx_isp_isp_sat_s_control(struct isp_device *isp, struct v4l2_control *control)
{
	int value = control->value;

	if(value < 0 || value > 255){
		dev_err(isp->dev, "the available range of saturation is 0-255\n");
		return -EINVAL;
	}

	tisp_set_saturation(isp->sensor_id, value);
	isp->ctrls.saturation = value;
	return 0;
}

static inline int tx_isp_isp_sat_g_control(struct isp_device *isp, struct v4l2_control *control)
{
	control->value = isp->ctrls.saturation;
	return 0;
}

static inline int tx_isp_isp_bright_s_control(struct isp_device *isp, struct v4l2_control *control)
{
	int value = control->value;

	if(value < 0 || value > 255){
		dev_err(isp->dev, "the available range of brightness is 0-255\n");
		return -EINVAL;
	}

	tisp_set_brightness(isp->sensor_id, value);
	isp->ctrls.brightness = value;

	return 0;
}

static inline int tx_isp_isp_bright_g_control(struct isp_device *isp, struct v4l2_control *control)
{
	control->value = isp->ctrls.brightness;

	return 0;
}

static inline int tx_isp_isp_contrast_s_control(struct isp_device *isp, struct v4l2_control *control)
{
	int value = control->value;

	if(value < 0 || value > 255){
		dev_err(isp->dev, "the available range of contrast is 0-255\n");
		return -EINVAL;
	}

	tisp_set_contrast(isp->sensor_id, value);
	isp->ctrls.contrast = value;

	return 0;
}

static inline int tx_isp_isp_contrast_g_control(struct isp_device *isp, struct v4l2_control *control)
{
	control->value = isp->ctrls.contrast;

	return 0;
}

static inline int tx_isp_isp_sharp_s_control(struct isp_device *isp, struct v4l2_control *control)
{
	int value = control->value;

	if(value < 0 || value > 255){
		dev_err(isp->dev, "the available range of sharpness is 0-255\n");
		return -EINVAL;
	}

	tisp_set_sharpness(isp->sensor_id, value);
	isp->ctrls.sharpness = value;

	return 0;
}

static inline int tx_isp_isp_sharp_g_control(struct isp_device *isp, struct v4l2_control *control)
{
	control->value = isp->ctrls.sharpness;

	return 0;
}


extern int tisp_ae_main_manual_set(tisp_ae_ctrls_t ae_manual);
extern int tisp_ae_main_manual_get(tisp_ae_ctrls_t *ae_manual);
extern int tisp_ae_sec_manual_set(tisp_ae_ctrls_t ae_manual);
extern int tisp_ae_sec_manual_get(tisp_ae_ctrls_t *ae_manual);

static int ispcore_exp_auto_s_control(struct isp_device *isp, struct v4l2_control *control)
{
	tisp_ae_ctrls_t ae_manual;
	int ret = 0;
	/*
	 * control->value.
	 *	0: 自动曝光.
	 *	1: 手动曝光.
	 *	2. 快门优先.
	 *	3. 光圈优先.
	 * */

	if(isp->sensor_id == 0)
		ret = tisp_ae_main_manual_get(&ae_manual);
	else if(isp->sensor_id == 1)
		ret = tisp_ae_sec_manual_get(&ae_manual);
	else
		ret = -EINVAL;
	if(ret)
		return ret;

	if(control->value == V4L2_EXPOSURE_AUTO) {
		ae_manual.tisp_ae_it_manual = 0;
	} else if(control->value == V4L2_EXPOSURE_MANUAL) {
		ae_manual.tisp_ae_it_manual = 1;
	} else {
		dev_err(isp->dev, "the available range of exp_auto is 0-1\n");
		return -EINVAL;
	}

	if(isp->sensor_id == 0)
		ret = tisp_ae_main_manual_set(ae_manual);
	else if(isp->sensor_id == 1)
		ret = tisp_ae_sec_manual_set(ae_manual);
	else
		ret = -EINVAL;

	return ret;
}

/* 获取当前曝光方式.*/
static int ispcore_exp_auto_g_control(struct isp_device *isp, struct v4l2_control *control)
{
	tisp_ae_ctrls_t ae_manual;
	int ret = 0;

	if(isp->sensor_id == 0)
		ret = tisp_ae_main_manual_get(&ae_manual);
	else if(isp->sensor_id == 1)
		ret = tisp_ae_sec_manual_get(&ae_manual);
	else
		ret = -EINVAL;

	control->value = ae_manual.tisp_ae_it_manual == 0 ? V4L2_EXPOSURE_AUTO : V4L2_EXPOSURE_MANUAL;

	return ret;
}

/* 设置曝光等级. eg: [-4, 4]
 *
 *  相对曝光时间. 从 [-4, 4] -> [inte_min, inte_max]
 * */
static int ispcore_exp_s_control(struct isp_device *isp, struct v4l2_control *control)
{
	return 0;
}

static int ispcore_exp_g_control(struct isp_device *isp, struct v4l2_control *control)
{
	return 0;
}

/* 设置曝光时间. */
static int ispcore_exp_abs_s_control(struct isp_device *isp, struct v4l2_control *control)
{
	tisp_ae_ctrls_t ae_manual;
	int ret = 0;

	if(control->value < 0){
		dev_err(isp->dev, "the available range of exposure is greater than 0\n");
		return -EINVAL;
	}

	if(isp->sensor_id == 0)
		ret = tisp_ae_main_manual_get(&ae_manual);
	else if(isp->sensor_id == 1)
		ret = tisp_ae_sec_manual_get(&ae_manual);
	else
		ret = -EINVAL;
	if(ret)
		return ret;

	if(ae_manual.tisp_ae_it_manual) {
		ae_manual.tisp_ae_sensor_integration_time = control->value;
		if(isp->sensor_id == 0)
			ret = tisp_ae_main_manual_set(ae_manual);
		else if(isp->sensor_id == 1)
			ret = tisp_ae_sec_manual_set(ae_manual);
		else
			ret = -EINVAL;
	} else {
		dev_warn(isp->dev, "set exp_abs while in exposure [auto] mode.\n");
	}

	return ret;

}

static int ispcore_exp_abs_g_control(struct isp_device *isp, struct v4l2_control *control)
{
	tisp_ae_ctrls_t ae_manual;
	int ret = 0;

	if(isp->sensor_id == 0)
		ret = tisp_ae_main_manual_get(&ae_manual);
	else if(isp->sensor_id == 1)
		ret = tisp_ae_sec_manual_get(&ae_manual);
	else
		ret = -EINVAL;

	control->value = ae_manual.tisp_ae_sensor_integration_time;

	return ret;
}

/*设置自动增益控制方式
 * 0: 自动增益控制
 * 1: 手动增益控制.
 * */
static inline int ispcore_auto_gain_s_control(struct isp_device *isp, struct v4l2_control *control)
{
	tisp_ae_ctrls_t ae_manual;
	int ret = 0;


	if(isp->sensor_id == 0)
		ret = tisp_ae_main_manual_get(&ae_manual);
	else if(isp->sensor_id == 1)
		ret = tisp_ae_sec_manual_get(&ae_manual);
	else
		ret = -EINVAL;
	if(ret)
		return ret;

	ae_manual.tisp_ae_ag_manual = control->value;

	if(isp->sensor_id == 0)
		ret = tisp_ae_main_manual_set(ae_manual);
	else if(isp->sensor_id == 1)
		ret = tisp_ae_sec_manual_set(ae_manual);
	else {
		dev_err(isp->dev, "the available range of gain_auto is 0-1\n");
		ret = -EINVAL;
	}

	return ret;
}

/*获取自动增益控制方式.*/
static inline int ispcore_auto_gain_g_control(struct isp_device *isp, struct v4l2_control *control)
{
	tisp_ae_ctrls_t ae_manual;
	int ret = 0;

	if(isp->sensor_id == 0)
		ret = tisp_ae_main_manual_get(&ae_manual);
	else if(isp->sensor_id == 1)
		ret = tisp_ae_sec_manual_get(&ae_manual);
	else
		ret = -EINVAL;

	control->value = ae_manual.tisp_ae_ag_manual;

	return ret;
}

/*手动增益控制模式下，设置当前增益.*/
static inline int ispcore_gain_s_control(struct isp_device *isp, struct v4l2_control *control)
{
	tisp_ae_ctrls_t ae_manual;
	int ret = 0;

	if(control->value < 0){
		dev_err(isp->dev, "the available range of again is greater than 0\n");
		return -EINVAL;
	}

	if(isp->sensor_id == 0)
		ret = tisp_ae_main_manual_get(&ae_manual);
	else if(isp->sensor_id == 1)
		ret = tisp_ae_sec_manual_get(&ae_manual);
	else
		ret = -EINVAL;
	if(ret)
		return ret;

	if(ae_manual.tisp_ae_ag_manual) {
		ae_manual.tisp_ae_sensor_again = control->value;
		if(isp->sensor_id == 0)
			ret = tisp_ae_main_manual_set(ae_manual);
		else if(isp->sensor_id == 1)
			ret = tisp_ae_sec_manual_set(ae_manual);
		else
			ret = -EINVAL;
	} else {
		dev_warn(isp->dev, "set exp_abs while in exposure [auto] mode.\n");
	}

	return ret;
}
static inline int ispcore_gain_g_control(struct isp_device *isp, struct v4l2_control *control)
{
	tisp_ae_ctrls_t ae_manual;
	int ret = 0;

	if(isp->sensor_id == 0)
		ret = tisp_ae_main_manual_get(&ae_manual);
	else if(isp->sensor_id == 1)
		ret = tisp_ae_sec_manual_get(&ae_manual);
	else
		ret = -EINVAL;

	control->value = ae_manual.tisp_ae_sensor_again;

	return ret;
}

static inline int tx_isp_isp_flicker_s_attr(struct isp_device *isp, struct v4l2_control *control)
{
	tisp_anfiflicker_attr_t attr;
	int ret = 0;

	if(control->value == V4L2_CID_POWER_LINE_FREQUENCY_DISABLED) {
		attr.mode = ISP_ANTIFLICKER_DISABLE_MODE;
		attr.freq = 0;
	} else if(control->value == V4L2_CID_POWER_LINE_FREQUENCY_50HZ) {
		attr.mode = ISP_ANTIFLICKER_NORMAL_MODE;
		attr.freq = 50;
	} else if(control->value == V4L2_CID_POWER_LINE_FREQUENCY_60HZ) {
		attr.mode = ISP_ANTIFLICKER_NORMAL_MODE;
		attr.freq = 60;
	} else {
		dev_err(isp->dev, "the available range of antiflicker is 0-2\n");
		return -EINVAL;
	}

	ret = tisp_s_antiflick(isp->sensor_id, &attr);
	if(ret != 0)
		ISP_WARNING("[ %s:%d ] set control failed!!!\n", __func__, __LINE__);

	return ret;
}

static inline int tx_isp_isp_flicker_g_attr(struct isp_device *isp, struct v4l2_control *control)
{
	tisp_anfiflicker_attr_t attr;
	int ret = 0;

	ret = tisp_g_antiflick(isp->sensor_id, &attr);
	if(ret != 0){
		ISP_ERROR("[ %s:%d ] get control failed!!!\n", __func__, __LINE__);
		goto err_get_flicker;
	}

	switch(attr.freq) {
		case 0:
			control->value = V4L2_CID_POWER_LINE_FREQUENCY_DISABLED;
			break;
		case 50:
			control->value = V4L2_CID_POWER_LINE_FREQUENCY_50HZ;
			break;
		case 60:
			control->value = V4L2_CID_POWER_LINE_FREQUENCY_60HZ;
			break;
		default:
			ISP_ERROR("[ %s:%d ] get control failed!!!\n", __func__, __LINE__);
	}

err_get_flicker:
	return ret;
}

static int tx_isp_isp_module_s_attr(struct isp_device *isp, struct v4l2_control *control)
{
	tisp_module_control_t module;
	int ret = 0;

	module.key = control->value;
	tisp_s_module_control(isp->sensor_id, module);

	return ret;
}

static int tx_isp_isp_module_g_attr(struct isp_device *isp, struct v4l2_control *control)
{
	tisp_module_control_t module;
	int ret = 0;

	tisp_g_module_control(isp->sensor_id, &module);
	control->value = module.key;

	return ret;
}

static inline int tx_isp_isp_day_or_night_g_ctrl(struct isp_device *isp, struct v4l2_control *control)
{
	control->value = isp->ctrls.daynight;
	return 0;
}

static inline int tx_isp_isp_day_or_night_s_ctrl(struct isp_device *isp, struct v4l2_control *control)
{
	int ret = 0;
	TISP_MODE_DN_E dn = control->value;

	if(dn < 0 || dn > 1){
		dev_err(isp->dev, "the available range of daynight mode is 0-1\n");
		return -EINVAL;
	}

	if(dn != isp->ctrls.daynight){
		tisp_day_or_night_s_ctrl(isp->sensor_id, dn);
		isp->ctrls.daynight = dn;
	}

	return ret;
}

static int tx_isp_isp_ae_luma_g_ctrl(struct isp_device *isp, struct v4l2_control *control)
{
#if 0
	unsigned char luma;
	struct v4l2_subdev_format *input_fmt = &isp->formats[ISP_PAD_SINK];
	int width = input_fmt->format.width;
	int height = input_fmt->format.height;

	tisp_g_ae_luma(&core->core_tuning, &luma, width, height);
	control->value = luma;
#endif
	return 0;
}

static int tx_isp_bcsh_hue_s_ctrl(struct isp_device *isp, struct v4l2_control *control)
{
	int value = control->value;
	int ret = 0;

	if(value < 0 || value > 255){
		dev_err(isp->dev, "the available range of hue is 0-255\n");
		return -EINVAL;
	}

	tisp_set_bcsh_hue(isp->sensor_id, value);
	isp->ctrls.hue = value;

	return ret;
}

static int tx_isp_bcsh_hue_g_ctrl(struct isp_device *isp, struct v4l2_control *control)
{
	int ret = 0;

	control->value = isp->ctrls.hue;

	return ret;
}

static int tx_isp_switch_bin(struct isp_device *isp, struct v4l2_control *control)
{
	tisp_bin_t attr;
	int ret = 0;

	ret = private_copy_from_user(&attr, (const void __user*)control->value, sizeof(attr));
	if(ret != 0){
		ISP_ERROR("[ %s:%d ] copy error!!!\n", __func__, __LINE__);
	} else {
		//printk("\n------------------>> Test:%d-%d-%s <<------------------\n", attr.enable, attr.mode, attr.bname);
		ret = tisp_switch_bin(isp->sensor_id, &attr);
		if(ret != 0)
			ISP_WARNING("[ %s:%d ] switch bin failed!!!\n", __func__, __LINE__);
	}
	return ret;
}

static int tx_isp_faceae_s_attr(struct isp_video_device *ispvideo, struct v4l2_control *control)
{

	struct ispcam_device *ispcam = ispvideo->ispcam;
	struct mscaler_device *mscaler = ispcam->mscaler;
	struct isp_device *isp = ispcam->isp;
        struct media_pad *remote = media_entity_remote_pad(&ispvideo->pad);
	struct v4l2_subdev_format *input_fmt = &mscaler->formats[MSCALER_PAD_SINK];
	struct v4l2_subdev_format *output_fmt = &mscaler->formats[remote->index];

	int in_width = input_fmt->format.width;
	int in_height = input_fmt->format.height;
	int out_width = output_fmt->format.width;
	int out_height = output_fmt->format.height;

	tisp_face_t attr;
	int ret = 0;

	ret = copy_from_user(&attr, (const void __user*)control->value, sizeof(attr));
	if(ret != 0) {
		ISP_ERROR("[ %s:%d ] copy error!!!\n", __func__, __LINE__);
		return ret;
	}

	if(attr.top < 0 || attr.top > out_height || attr.bottom < 0 || attr.bottom > out_height
			|| attr.left < 0 || attr.left > out_width || attr.right < 0 || attr.right > out_width
			|| attr.top > attr.bottom || attr.left > attr.right) {
		dev_err(isp->dev, "face ae attr not available\n");
		return -EINVAL;
	} else {
		attr.top = (attr.top * in_height)/out_height;
		attr.bottom = (attr.bottom * in_height)/out_height;
		attr.left = (attr.left * in_width)/out_width;
		attr.right = (attr.right * in_width)/out_width;
		ret = tisp_ae_face_set(isp->sensor_id, &attr);
		if(ret != 0)
			ISP_WARNING("[ %s:%d ] set ae face failed!!!\n", __func__, __LINE__);

	}
	return ret;
}

static int tx_isp_faceae_g_attr(struct isp_device *isp, struct v4l2_control *control)
{
	int ret = 0;
#if 0
	tisp_face_t attr = {0};

	ret = tisp_ae_face_get(isp->sensor_id, &attr); //empty function
	if(ret != 0){
		ISP_WARNING("[ %s:%d ] get ae smart failed!!!\n", __func__, __LINE__);
		goto err_get_aesmart;
	}

	ret = copy_to_user((void __user*)control->value, (const void *)&attr, sizeof(attr));
	if(ret != 0)
		ISP_ERROR("[ %s:%d ] copy error!!!\n", __func__, __LINE__);

err_get_aesmart:
#endif
	return ret;
}

static int tx_isp_faceawb_s_attr(struct isp_video_device *ispvideo, struct v4l2_control *control)
{
	int ret = 0;
#if 0 //empty function
	struct ispcam_device *ispcam = ispvideo->ispcam;
	struct mscaler_device *mscaler = ispcam->mscaler;
	struct isp_device *isp = ispcam->isp;
        struct media_pad *remote = media_entity_remote_pad(&ispvideo->pad);
	struct v4l2_subdev_format *input_fmt = &mscaler->formats[MSCALER_PAD_SINK];
	struct v4l2_subdev_format *output_fmt = &mscaler->formats[remote->index];

	int in_width = input_fmt->format.width;
	int in_height = input_fmt->format.height;
	int out_width = output_fmt->format.width;
	int out_height = output_fmt->format.height;

	tisp_face_t attr;

	ret = copy_from_user(&attr, (const void __user*)control->value, sizeof(attr));
	if(ret != 0){
		ISP_ERROR("[ %s:%d ] copy error!!!\n", __func__, __LINE__);
		return ret;
	}
	if(attr.top < 0 || attr.top > out_height || attr.bottom < 0 || attr.bottom > out_height
			|| attr.left < 0 || attr.left > out_width || attr.right < 0 || attr.right > out_width
			|| attr.top > attr.bottom || attr.left > attr.right) {
		dev_err(isp->dev, "face awb attr not available\n");
		return -EINVAL;
	} else {
		attr.top = (attr.top * in_height)/out_height;
		attr.bottom = (attr.bottom * in_height)/out_height;
		attr.left = (attr.left * in_width)/out_width;
		attr.right = (attr.right * in_width)/out_width;
		ret = tisp_awb_face_set(isp->sensor_id, &attr);
		if(ret != 0)
			ISP_WARNING("[ %s:%d ] set awb face failed!!!\n", __func__, __LINE__);
	}
#endif
	return ret;
}

static int tx_isp_faceawb_g_attr(struct isp_device *isp, struct v4l2_control *control)
{
	int ret = 0;
#if 0
	tisp_face_t attr = {0};

	ret = tisp_awb_face_get(isp->sensor_id, &attr); //empty function
	if(ret != 0){
		ISP_WARNING("[ %s:%d ] get awb face attr failed!!!\n", __func__, __LINE__);
		goto err_get_aesmart;
	}

	ret = copy_to_user((void __user*)control->value, (const void *)&attr, sizeof(attr));
	if(ret != 0)
		ISP_ERROR("[ %s:%d ] copy error!!!\n", __func__, __LINE__);

err_get_aesmart:
#endif
	return ret;
}

static int wdr_enable(struct isp_device *isp, struct v4l2_control *control)
{
	struct isp_async_device	*isd = &isp->ispcam->isd[0];
	//int enable = control->value;
	int ret = 0;

	if(control->value != 0 && control->value !=1){
		dev_err(isp->dev, "the available range of wdr is 0-1\n");
		return -EINVAL;
	}

	ret = v4l2_subdev_call(isd->sd, core, s_ctrl, control);
	return ret;
}

static int tx_isp_set_default_bin_path(struct isp_device *isp, struct v4l2_control *control)
{
	int ret = 0;
	ret = copy_from_user(&isp->bpath, (const void __user*)control->value, sizeof(isp->bpath));
	return ret;
}

int isp_video_g_ctrl(struct file *file, void *fh, struct v4l2_control *a)
{
	struct isp_video_device *ispvideo = video_drvdata(file);
	struct ispcam_device 	*ispcam = ispvideo->ispcam;
	struct isp_device 	*isp	= ispcam->isp;
	int ret = 0;

	switch(a->id){
		case V4L2_CID_HFLIP:
			ret = tx_isp_isp_hflip_g_control(isp, a);
			break;
		case V4L2_CID_VFLIP:
			ret = tx_isp_isp_vflip_g_control(isp, a);
			break;
		case V4L2_CID_SATURATION:
			ret = tx_isp_isp_sat_g_control(isp, a);
			break;
		case V4L2_CID_BRIGHTNESS:
			ret = tx_isp_isp_bright_g_control(isp, a);
			break;
		case V4L2_CID_CONTRAST:
			ret = tx_isp_isp_contrast_g_control(isp, a);
			break;
		case V4L2_CID_SHARPNESS:
			ret = tx_isp_isp_sharp_g_control(isp, a);
			break;
		case V4L2_CID_EXPOSURE_AUTO:
			ret = ispcore_exp_auto_g_control(isp, a);
			break;
		case V4L2_CID_EXPOSURE_ABSOLUTE:
			ret = ispcore_exp_abs_g_control(isp, a);
			break;
		case V4L2_CID_EXPOSURE:
			ret = ispcore_exp_g_control(isp, a);
			break;
		case V4L2_CID_AUTOGAIN:
			ret = ispcore_auto_gain_g_control(isp, a);
			break;
		case V4L2_CID_GAIN:
			ret = ispcore_gain_g_control(isp, a);
			break;
		case V4L2_CID_HUE:
			ret = tx_isp_bcsh_hue_g_ctrl(isp, a);
			break;
		case V4L2_CID_POWER_LINE_FREQUENCY:
			ret = tx_isp_isp_flicker_g_attr(isp, a);
			break;
		case IMAGE_TUNING_CID_MODULE_CONTROL:
			ret = tx_isp_isp_module_g_attr(isp, a);
			break;
		case IMAGE_TUNING_CID_DAY_OR_NIGHT:
			ret = tx_isp_isp_day_or_night_g_ctrl(isp, a);
			break;
		case IMAGE_TUNING_CID_AE_LUMA:
			ret = tx_isp_isp_ae_luma_g_ctrl(isp, a);
			break;
		case IMAGE_TUNING_CID_FACE_AE_CONTROL:
			ret = tx_isp_faceae_g_attr(isp, a);
			break;
		case IMAGE_TUNING_CID_FACE_AWB_CONTROL:
			ret = tx_isp_faceawb_g_attr(isp, a);
			break;
		default:
			ret = -EINVAL;
			break;
	}

	return ret;
}


int isp_video_s_ctrl(struct file *file, void *fh, struct v4l2_control *a)
{
	struct isp_video_device *ispvideo = video_drvdata(file);
	struct ispcam_device 	*ispcam = ispvideo->ispcam;
	struct isp_device 	*isp	= ispcam->isp;
	int ret = 0;

	switch (a->id) {
		case V4L2_CID_HFLIP:
			ret = tx_isp_isp_hflip_s_control(isp, a);
			break;
		case V4L2_CID_VFLIP:
			ret = tx_isp_isp_vflip_s_control(isp, a);
			break;
		case V4L2_CID_SATURATION:
			ret = tx_isp_isp_sat_s_control(isp, a);
			break;
		case V4L2_CID_BRIGHTNESS:
			ret = tx_isp_isp_bright_s_control(isp, a);
			break;
		case V4L2_CID_CONTRAST:
			ret = tx_isp_isp_contrast_s_control(isp, a);
			break;
		case V4L2_CID_SHARPNESS:
			ret = tx_isp_isp_sharp_s_control(isp, a);
			break;
		case V4L2_CID_EXPOSURE_AUTO:
			ret = ispcore_exp_auto_s_control(isp, a);
			break;
		case V4L2_CID_EXPOSURE_ABSOLUTE:
			ret = ispcore_exp_abs_s_control(isp, a);
			break;
		case V4L2_CID_EXPOSURE:
			ret = ispcore_exp_s_control(isp, a);
			break;
		case V4L2_CID_AUTOGAIN:
			ret = ispcore_auto_gain_s_control(isp, a);
			break;
		case V4L2_CID_GAIN:
			ret = ispcore_gain_s_control(isp, a);
			break;
		case V4L2_CID_HUE:
			ret = tx_isp_bcsh_hue_s_ctrl(isp, a);
			break;
		case IMAGE_TUNING_CID_SWITCH_BIN:
			ret = tx_isp_switch_bin(isp, a);
			break;
		case V4L2_CID_POWER_LINE_FREQUENCY:
			ret = tx_isp_isp_flicker_s_attr(isp, a);
			break;
		case IMAGE_TUNING_CID_MODULE_CONTROL:
			ret = tx_isp_isp_module_s_attr(isp, a);
			break;
		case IMAGE_TUNING_CID_DAY_OR_NIGHT:
			ret = tx_isp_isp_day_or_night_s_ctrl(isp, a);
			break;
		case IMAGE_TUNING_CID_FACE_AE_CONTROL:
			ret = tx_isp_faceae_s_attr(ispvideo, a);
			break;
		case IMAGE_TUNING_CID_FACE_AWB_CONTROL:
			ret = tx_isp_faceawb_s_attr(ispvideo, a);
			break;
		case V4L2_CID_WIDE_DYNAMIC_RANGE:
			ret = wdr_enable(isp, a);
			break;
		case IMAGE_TUNING_CID_SET_DEFAULT_BIN_PATH:
			ret = tx_isp_set_default_bin_path(isp, a);
			break;
		default:
			ret = -EINVAL;
			break;
	}
	return ret;
}

