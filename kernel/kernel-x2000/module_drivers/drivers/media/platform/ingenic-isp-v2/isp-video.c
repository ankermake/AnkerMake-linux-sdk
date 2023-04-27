#include <linux/clk.h>
#include <linux/media-bus-format.h>
#include <linux/media.h>
#include <linux/module.h>
#include <linux/platform_device.h>
#include <linux/of.h>
#include <linux/of_graph.h>
#include <linux/of_platform.h>
#include <linux/pm_runtime.h>
#include <linux/pm_domain.h>
#include <linux/slab.h>
#include <linux/videodev2.h>
#include <linux/component.h>
#include <linux/mutex.h>

#include <media/media-device.h>
#include <media/v4l2-async.h>
#include <media/v4l2-device.h>
#include <media/v4l2-ctrls.h>
#include <media/v4l2-event.h>
#include <media/v4l2-ioctl.h>
#include <media/v4l2-dev.h>
#include <media/videobuf2-dma-contig-ingenic.h>

#include "isp-drv.h"
#define ISP_VIDEO_DRIVER_NAME	"ispvideo"

int isp_max_buffer_num = 3;
module_param(isp_max_buffer_num, int, 0644);
MODULE_PARM_DESC(isp_max_buffer_num, "isp max buffer numer");

int isp_force_img_depth = 0;
module_param(isp_force_img_depth, int, 0644);
MODULE_PARM_DESC(isp_force_img_depth, "force fmt depth, usefull when need mem more than real fmt.");

struct sensor_id_t {
	unsigned int sensor_id;
	bool available;
};

struct sensor_id_t sensor_id_pool[2] = {
	{.sensor_id = 0,
	.available = true},
	{.sensor_id = 1,
	.available = true},
};

static int get_sensor_id(void)
{
	int i;
	for(i=0; i < 2; i++) {
		if(sensor_id_pool[i].available){
			sensor_id_pool[i].available = false;
			return sensor_id_pool[i].sensor_id;
		}
	}
	return -1;
}

static void put_sensor_id(unsigned int sensor_id)
{
	sensor_id_pool[sensor_id].available = true;
}

struct mutex sensor_id_lock;

static struct v4l2_subdev *
isp_video_remote_subdev(struct isp_video_device *ispvideo, u32 *pad)
{
        struct media_pad *remote;

        remote = media_entity_remote_pad(&ispvideo->pad);

        if (remote == NULL ||
            media_entity_type(remote->entity) != MEDIA_ENT_T_V4L2_SUBDEV)
                return NULL;

        if (pad)
                *pad = remote->index;

        return media_entity_to_v4l2_subdev(remote->entity);
}


/* -----------------------------------------------------------------------------
 * V4L2 ioctls
 */

static int
isp_video_querycap(struct file *file, void *fh, struct v4l2_capability *cap)
{
        struct isp_video_device *ispvideo = video_drvdata(file);

        strlcpy(cap->driver, ISP_VIDEO_DRIVER_NAME, sizeof(cap->driver));
        strlcpy(cap->card, ispvideo->video.name, sizeof(cap->card));
	snprintf(cap->bus_info, sizeof(cap->bus_info), "platform:%s", dev_name(ispvideo->dev));

	cap->capabilities = V4L2_CAP_VIDEO_CAPTURE | V4L2_CAP_STREAMING | V4L2_CAP_DEVICE_CAPS;

	cap->device_caps = V4L2_CAP_VIDEO_CAPTURE | V4L2_CAP_STREAMING;

        return 0;
}

static int isp_video_enum_fmt_vid_cap(struct file *file, void *fh,
					    struct v4l2_fmtdesc *f)
{
        struct isp_video_device *ispvideo = video_drvdata(file);
	const struct isp_video_format *fmt = NULL;

	fmt = ispvideo->ops->find_format(NULL, NULL, f->index);
	if(fmt == NULL) {
		return -EINVAL;
	}
	strlcpy(f->description, fmt->name, sizeof(f->description));
	f->pixelformat = fmt->fourcc;

	return 0;
}
static int
isp_video_g_fmt_vid_cap(struct file *file, void *fh, struct v4l2_format *format)
{
        struct isp_video_device *ispvideo = video_drvdata(file);
        struct isp_video_ctx *ctx = ispvideo->ctx;

        //mutex_lock(&ispvideo->mutex);
	if(ctx->format.type == 0) {
		dev_warn(ispvideo->dev, "Format not setted before calling g_fmt\n");
	} else {
        	*format = ctx->format;
	}
        //mutex_unlock(&ispvideo->mutex);

        return 0;
}

static int
__isp_video_try_format(struct isp_video_ctx *ctx, struct v4l2_pix_format *pix,
		const struct isp_video_format **ofmt)
{
	struct isp_video_format *fmt = NULL;
	struct isp_video_device *ispvideo = ctx->ispvideo;
	unsigned int depth = 0;
	unsigned int use_depth = 0;
	int i;

	fmt = ispvideo->ops->find_format(&pix->pixelformat, NULL, -1);
	if(fmt == NULL) {
		dev_err(ispvideo->dev, "Cannot find appropriate format for pix->pixelformat:[%x]\n", pix->pixelformat);
		return -EINVAL;
	}

	/*TODO: handle unsupported fmt using init user fmt*/
#if 0
	pix->width = ((int)pix->width <= 0)?1920:pix->width;
	pix->height = ((int)pix->height <= 0)?1080:pix->height;
#endif

	ispvideo->current_format = fmt;

	for(i = 0; i < fmt->num_planes; i++) {
		depth += fmt->depth[i];
	}

	pix->colorspace = fmt->colorspace;
	pix->field = V4L2_FIELD_NONE;
	pix->pixelformat = fmt->fourcc;
	pix->bytesperline = pix->width * depth / 8;

	use_depth = isp_force_img_depth > depth ? isp_force_img_depth : depth;
	pix->sizeimage = pix->width * use_depth / 8 * pix->height;

	ctx->uv_offset = pix->width * fmt->depth[0] * pix->height / 8;
	ctx->payload_size = pix->bytesperline * pix->height;

	if(ofmt) {
		*ofmt = fmt;
	}

	/*TODO, bound width and height!*/
	//printk("=======%s,%d, pixelformat: %x, widht: %d, height: %d, num_planes: %d, uv_offset: %d\n", __func__, __LINE__, pix->pixelformat, pix->width, pix->height, fmt->num_planes, ctx->uv_offset);

	return 0;
}

static void to_v4l2_mbus_framefmt(struct isp_video_device *ispvideo, struct v4l2_format *format, struct v4l2_mbus_framefmt *fmt)
{
	struct v4l2_pix_format *pix = &format->fmt.pix;
	struct isp_video_format *ispfmt = NULL;

	ispfmt = ispvideo->ops->find_format(&pix->pixelformat, NULL, -1);

	fmt->width = pix->width;
	fmt->height = pix->height;
	fmt->code = ispfmt->mbus_code;
	fmt->field = pix->field;
	fmt->colorspace = pix->colorspace;
	fmt->ycbcr_enc = pix->ycbcr_enc;
	fmt->quantization = pix->quantization;
	fmt->xfer_func = pix->xfer_func;
}

static int
isp_video_s_fmt_vid_cap(struct file *file, void *fh, struct v4l2_format *format)
{
        struct isp_video_device *ispvideo = video_drvdata(file);
	struct isp_video_ctx *ctx = ispvideo->ctx;
	struct v4l2_subdev *sd = NULL;
	struct v4l2_subdev_format subdev_fmt;
	unsigned int pad = 0;
	int ret = 0;

	if(ispvideo->prestart)
		return 0;


	if(vb2_is_busy(&ctx->queue)) {
		return -EBUSY;
	}

	ret = __isp_video_try_format(ctx, &format->fmt.pix, NULL);
	if(ret < 0) {
		dev_err(ispvideo->dev, "Failed to set format cap!\n");
		return -EINVAL;
	}

	sd = isp_video_remote_subdev(ispvideo, &pad);
	if(sd == NULL) {
		return -EINVAL;
	}
	v4l2_set_subdev_hostdata(sd, ispvideo);

	ctx->format = *format;

	to_v4l2_mbus_framefmt(ispvideo, format, &subdev_fmt.format);

	subdev_fmt.pad = pad;
	subdev_fmt.which = V4L2_SUBDEV_FORMAT_ACTIVE;

	ret = v4l2_subdev_call(sd, pad, set_fmt, NULL, &subdev_fmt);
	if(ret < 0) {
		dev_err(ispvideo->dev, "Failed to set subdev format\n");
		return -EINVAL;
	}

	return 0;
}

static int
isp_video_try_format_cap(struct file *file, void *fh, struct v4l2_format *format)
{
        struct isp_video_device *ispvideo = video_drvdata(file);
	struct isp_video_ctx *ctx = ispvideo->ctx;
	int ret = 0;

	ret = __isp_video_try_format(ctx, &format->fmt.pix, NULL);
	if(ret < 0) {
		dev_err(ispvideo->dev, "Try format cap error!\n");
		return -EINVAL;
	}
	return 0;
}

static int
isp_video_cropcap(struct file *file, void *fh, struct v4l2_cropcap *cropcap)
{
	struct isp_video_device *ispvideo = video_drvdata(file);
	unsigned int pad;
	struct v4l2_subdev *sd = isp_video_remote_subdev(ispvideo, &pad);

	if(cropcap->type == V4L2_BUF_TYPE_VIDEO_CAPTURE || cropcap->type == V4L2_BUF_TYPE_VIDEO_CAPTURE_MPLANE)
		return  v4l2_subdev_call(sd, video, cropcap, cropcap);
	else {
		dev_warn(ispvideo->dev, "unsupported cropcap type!\n");
		return -EINVAL;
	}
}

static int
isp_video_get_crop(struct file *file, void *fh, struct v4l2_crop *crop)
{
	struct isp_video_device *ispvideo = video_drvdata(file);
	unsigned int pad;
	struct v4l2_subdev *sd = isp_video_remote_subdev(ispvideo, &pad);

	if(crop->type == V4L2_BUF_TYPE_VIDEO_CAPTURE || crop->type == V4L2_BUF_TYPE_VIDEO_CAPTURE_MPLANE)
		return v4l2_subdev_call(sd, video, g_crop, crop);
	else {
		dev_warn(ispvideo->dev, "unsupported crop type!\n");
		return -EINVAL;
	}
}

static int
isp_video_set_crop(struct file *file, void *fh, const struct v4l2_crop *crop)
{
	struct isp_video_device *ispvideo = video_drvdata(file);
	unsigned int pad;
	struct v4l2_subdev *sd = isp_video_remote_subdev(ispvideo, &pad);

	if(crop->type == V4L2_BUF_TYPE_VIDEO_CAPTURE || crop->type == V4L2_BUF_TYPE_VIDEO_CAPTURE_MPLANE)
		return v4l2_subdev_call(sd, video, s_crop, crop);
	else {
		dev_warn(ispvideo->dev, "unsupported crop type!\n");
		return -EINVAL;
	}
}


static int isp_video_get_selection(struct file *file, void *fh, struct v4l2_selection *s)
{
	struct isp_video_device *ispvideo = video_drvdata(file);
	unsigned int pad;
	struct v4l2_subdev *sd = isp_video_remote_subdev(ispvideo, &pad);
	struct v4l2_subdev_selection sel = {0};
	int ret = 0;

	if(s->type == V4L2_BUF_TYPE_VIDEO_CAPTURE || s->type == V4L2_BUF_TYPE_VIDEO_CAPTURE_MPLANE) {
		sel.target = s->target;
		ret = v4l2_subdev_call(sd, pad, get_selection, NULL, &sel);
		memcpy(&s->r, &sel.r, sizeof(struct v4l2_rect));
		return ret;
	} else {
		dev_warn(ispvideo->dev, "unsupported selection type!\n");
		return -EINVAL;
	}
}

static int isp_video_set_selection(struct file *file, void *fh, struct v4l2_selection *s)
{
	struct isp_video_device *ispvideo = video_drvdata(file);
	unsigned int pad;
	struct v4l2_subdev *sd = isp_video_remote_subdev(ispvideo, &pad);
	struct isp_video_ctx *ctx = ispvideo->ctx;
	struct v4l2_format *format = &ctx->format;
	struct v4l2_subdev_selection sel;
	int ret = 0;

	if(s->type == V4L2_BUF_TYPE_VIDEO_CAPTURE || s->type == V4L2_BUF_TYPE_VIDEO_CAPTURE_MPLANE) { 
		sel.flags = s->flags;
		sel.target = s->target;
		memcpy(&sel.r, &s->r, sizeof(struct v4l2_rect));
		ret = v4l2_subdev_call(sd, pad, set_selection, NULL, &sel);
		if(ret)
			return ret;

		format->fmt.pix.width = sel.r.width;
		format->fmt.pix.height = sel.r.height;
		ret = __isp_video_try_format(ctx, &format->fmt.pix, NULL);
		return ret;
	} else {
		dev_warn(ispvideo->dev, "unsupported selection type!\n");
		return -EINVAL;
	}
}

static int isp_video_enum_input(struct file *file, void *priv,
				 struct v4l2_input *inp)
{
        struct isp_video_device *ispvideo = video_drvdata(file);

	if (inp->index != 0)
		return -EINVAL;

	/* default is camera */
	inp->type = V4L2_INPUT_TYPE_CAMERA;
	inp->std = ispvideo->video.tvnorms;
	strcpy(inp->name, "Camera");

	return 0;
}

static int isp_video_g_input(struct file *file, void *priv, unsigned int *i)
{
	*i = 0;

	return 0;
}

static int isp_video_s_input(struct file *file, void *priv, unsigned int i)
{
	if (i > 0)
		return -EINVAL;

	return 0;
}

int isp_video_enum_framesizes(struct file *file, void *fh, struct v4l2_frmsizeenum *fsize)
{
	struct isp_video_device *ispvideo = video_drvdata(file);
	struct ispcam_device 	*ispcam = ispvideo->ispcam;
	struct mscaler_device 	*mscaler= ispcam->mscaler;
	struct v4l2_subdev_frame_size_enum fse;
	int ret = 0;

	fse.code = fsize->pixel_format;
	fse.index = fsize->index;

	ret = v4l2_subdev_call(&mscaler->sd, pad, enum_frame_size, NULL, &fse);
	if (ret < 0)
		return ret;

	if(!ispvideo->bypass) {
		fsize->type = V4L2_FRMSIZE_TYPE_STEPWISE;
		fsize->stepwise.min_width = fse.min_width;
		fsize->stepwise.max_width = fse.max_width;
		fsize->stepwise.min_height = fse.min_height;
		fsize->stepwise.max_height = fse.max_height;
		fsize->stepwise.step_width = 2;
		fsize->stepwise.step_height = 2;
	} else {
		fsize->type = V4L2_FRMSIZE_TYPE_DISCRETE;
		fsize->discrete.width = fse.min_width;
		fsize->discrete.height = fse.min_height;
	}
	return 0;
}

int isp_video_g_parm(struct file *file, void *fh, struct v4l2_streamparm *a){
	struct isp_video_device *ispvideo = video_drvdata(file);
	struct ispcam_device 	*ispcam = ispvideo->ispcam;
	struct isp_async_device	*isd	= &ispcam->isd[0];
	int ret = 0;

	ret = v4l2_subdev_call(isd->sd, video, g_parm, a);
	if(ret < 0) {
		return ret;
	}
	return 0;
}

extern int isp_video_s_ctrl(struct file *file, void *fh, struct v4l2_control *a);
extern int isp_video_g_ctrl(struct file *file, void *fh, struct v4l2_control *a);

extern int bypass_video_s_ctrl(struct file *file, void *fh, struct v4l2_control *a);
extern int bypass_video_g_ctrl(struct file *file, void *fh, struct v4l2_control *a);

static int vidioc_reqbufs(struct file *file, void *priv, struct v4l2_requestbuffers *p)
{
	struct isp_video_device *ispvideo = video_drvdata(file);
	struct ispcam_device 	*ispcam = ispvideo->ispcam;
	struct isp_prestart_info *p_info = ispcam->p_info;
	int ret = 0;

	if(ispvideo->prestart) {
		p->count = p_info->buf_cnt;
		return ret;
	} else
		return vb2_ioctl_reqbufs(file, priv, p);

}

static int vidioc_qbuf(struct file *file, void *priv, struct v4l2_buffer *p)
{
	struct isp_video_device *ispvideo = video_drvdata(file);
	struct ispcam_device 	*ispcam = ispvideo->ispcam;
	struct isp_prestart_info *p_info = ispcam->p_info;

	if(ispvideo->prestart && !atomic_read(&p_info->usr_streaming))
		return 0;
	else
		return vb2_ioctl_qbuf(file, priv, p);
}

static int vidioc_streamon(struct file *file, void *priv, enum v4l2_buf_type i)
{
	struct isp_video_device *ispvideo = video_drvdata(file);
	struct ispcam_device 	*ispcam = ispvideo->ispcam;
	struct isp_prestart_info *p_info = ispcam->p_info;

	if(ispvideo->prestart){
		atomic_set(&p_info->usr_streaming, 1);
		wait_event(p_info->wq, !atomic_read(&p_info->driver_streaming));
		return 0;
	} else
		return vb2_ioctl_streamon(file, priv, i);
}

static int vidioc_streamoff(struct file *file, void *priv, enum v4l2_buf_type i)
{
	struct isp_video_device *ispvideo = video_drvdata(file);
	struct ispcam_device 	*ispcam = ispvideo->ispcam;
	struct isp_prestart_info *p_info = ispcam->p_info;

	if(ispvideo->prestart) {
		atomic_set(&p_info->usr_streaming, 0);
	}
	return vb2_ioctl_streamoff(file, priv, i);
}

static const struct v4l2_ioctl_ops isp_video_ioctl_ops = {
        .vidioc_querycap                = isp_video_querycap,
	.vidioc_enum_fmt_vid_cap	= isp_video_enum_fmt_vid_cap,
        .vidioc_g_fmt_vid_cap    	= isp_video_g_fmt_vid_cap,
        .vidioc_s_fmt_vid_cap   	= isp_video_s_fmt_vid_cap,
        .vidioc_try_fmt_vid_cap  	= isp_video_try_format_cap,
        .vidioc_cropcap                 = isp_video_cropcap,
        .vidioc_g_crop                  = isp_video_get_crop,
        .vidioc_s_crop                  = isp_video_set_crop,
        .vidioc_g_selection             = isp_video_get_selection,
        .vidioc_s_selection             = isp_video_set_selection,
        .vidioc_reqbufs                 = vidioc_reqbufs,
        .vidioc_querybuf                = vb2_ioctl_querybuf,
        .vidioc_qbuf                    = vidioc_qbuf,
        .vidioc_dqbuf                   = vb2_ioctl_dqbuf,
	.vidioc_expbuf			= vb2_ioctl_expbuf,
        .vidioc_streamon                = vidioc_streamon,
        .vidioc_streamoff               = vidioc_streamoff,
	.vidioc_enum_input		= isp_video_enum_input,
	.vidioc_g_input			= isp_video_g_input,
	.vidioc_s_input			= isp_video_s_input,
	.vidioc_enum_framesizes		= isp_video_enum_framesizes,
	.vidioc_g_parm			= isp_video_g_parm,
	.vidioc_s_ctrl			= isp_video_s_ctrl,
	.vidioc_g_ctrl			= isp_video_g_ctrl,
};

static const struct v4l2_ioctl_ops bypass_video_ioctl_ops = {
        .vidioc_querycap                = isp_video_querycap,	/**/
	.vidioc_enum_fmt_vid_cap	= isp_video_enum_fmt_vid_cap,
        .vidioc_g_fmt_vid_cap    	= isp_video_g_fmt_vid_cap,
        .vidioc_s_fmt_vid_cap   	= isp_video_s_fmt_vid_cap,
        .vidioc_try_fmt_vid_cap  	= isp_video_try_format_cap,
        .vidioc_reqbufs                 = vb2_ioctl_reqbufs,
        .vidioc_querybuf                = vb2_ioctl_querybuf,
        .vidioc_qbuf                    = vb2_ioctl_qbuf,
        .vidioc_dqbuf                   = vb2_ioctl_dqbuf,
	.vidioc_expbuf			= vb2_ioctl_expbuf,
        .vidioc_streamon                = vb2_ioctl_streamon,
        .vidioc_streamoff               = vb2_ioctl_streamoff,
	.vidioc_enum_input		= isp_video_enum_input,	/**/
	.vidioc_g_input			= isp_video_g_input,
	.vidioc_s_input			= isp_video_s_input,
	.vidioc_enum_framesizes		= isp_video_enum_framesizes,
	.vidioc_g_parm			= isp_video_g_parm,  /**/
//	.vidioc_s_ctrl			= bypass_video_s_ctrl,
//	.vidioc_g_ctrl			= bypass_video_g_ctrl,
};

/* -----------------------------------------------------------------------------
 * Video queue operations
 */

static int isp_video_queue_setup(struct vb2_queue *queue,
                                 const void *parg,
                                 unsigned int *count, unsigned int *num_planes,
                                 unsigned int sizes[], void *alloc_ctxs[])
{
        struct isp_video_ctx *ctx = vb2_get_drv_priv(queue);
        struct isp_video_device *ispvideo = ctx->ispvideo;
	struct v4l2_format *format = &ctx->format;

	*num_planes = 1;

	sizes[0] = format->fmt.pix.sizeimage;
	if(sizes[0] == 0) {
		dev_err(ispvideo->dev, "queue setup 0 sizeimage\n");
		return -EINVAL;
	}

	alloc_ctxs[0] = ispvideo->alloc_ctx;

	/*MAX_BUFFER_NUMS:*/
	if(ispvideo->bypass)
		isp_max_buffer_num = 5;
	if(*count >= isp_max_buffer_num) {
		*count = isp_max_buffer_num;
	}

	ispvideo->max_buffer_num = *count;

        return 0;
}

static int isp_video_buffer_prepare(struct vb2_buffer *buf)
{
	struct isp_video_ctx *ctx = vb2_get_drv_priv(buf->vb2_queue);

	//vb2_set_plane_payload(buf, 0, format->fmt.pix.sizeimage);
	vb2_set_plane_payload(buf, 0, ctx->payload_size);
        return 0;
}

static void isp_video_buffer_queue(struct vb2_buffer *buf)
{
        struct vb2_v4l2_buffer *vbuf = to_vb2_v4l2_buffer(buf);
	struct isp_video_ctx *ctx = vb2_get_drv_priv(buf->vb2_queue);
	struct isp_video_buffer *isp_buffer = to_isp_buffer(vbuf);
	struct isp_video_device *ispvideo = ctx->ispvideo;
	int ret = 0;

	isp_buffer->uv_offset = ctx->uv_offset;

	ret = ispvideo->ops->qbuf(ispvideo, isp_buffer);
	if(ret < 0) {
		dev_err(ispvideo->dev, "failed to queue buf!\n");
	}
}

static int isp_video_subdev_init(struct isp_video_device *ispvideo)
{
	struct media_entity *entity = &ispvideo->video.entity;
	struct v4l2_subdev *sd = NULL;
	struct media_pad *pad = NULL;
	struct ispcam_device 	*ispcam = ispvideo->ispcam;
	int ret = 0;

	/*subdevs reset.*/
	while(1) {
		pad = &entity->pads[0];
		/*直到找到该pipeline上的仅有一个FL_SOURCE pad设备.*/
		if(!(pad->flags & MEDIA_PAD_FL_SINK))
			break;

		pad = media_entity_remote_pad(pad);
		if (pad == NULL ||
				media_entity_type(pad->entity) != MEDIA_ENT_T_V4L2_SUBDEV)
			break;
		entity = pad->entity;
		sd = media_entity_to_v4l2_subdev(entity);

		/*stream on*/
		v4l2_set_subdev_hostdata(sd, ispvideo);
		ret = v4l2_subdev_call(sd, core, init, ispcam->sensor_id);
		if(ret < 0 && ret != -ENOIOCTLCMD) {
			dev_err(ispvideo->dev, "failed to init subdev[%s]\n", entity->name);
			break;
		} else {
			ret = 0;
		}
	}

	return ret;
}

static int isp_video_subdev_reset(struct isp_video_device *ispvideo)
{
	struct media_entity *entity = &ispvideo->video.entity;
	struct v4l2_subdev *sd = NULL;
	struct media_pad *pad = NULL;
	int ret = 0;

	/*subdevs reset.*/
	while(1) {
		pad = &entity->pads[0];
		/*直到找到该pipeline上的仅有一个FL_SOURCE pad设备.*/
		if(!(pad->flags & MEDIA_PAD_FL_SINK))
			break;

		pad = media_entity_remote_pad(pad);
		if (pad == NULL ||
				media_entity_type(pad->entity) != MEDIA_ENT_T_V4L2_SUBDEV)
			break;
		entity = pad->entity;
		sd = media_entity_to_v4l2_subdev(entity);

		/*stream on*/
		v4l2_set_subdev_hostdata(sd, ispvideo);
		ret = v4l2_subdev_call(sd, core, reset, 1);
		if(ret < 0 && ret != -ENOIOCTLCMD) {
			dev_err(ispvideo->dev, "failed to reset subdev[%s]\n", entity->name);
		} else {
			ret = 0;
		}
	}

	return ret;

}


static int isp_video_subdev_streamon(struct isp_video_device *ispvideo)
{
	struct ispcam_device 	*ispcam = ispvideo->ispcam;
	struct mscaler_device 	*mscaler= ispcam->mscaler;
	struct isp_device 	*isp	= ispcam->isp;
	struct vic_device	*vic	= ispcam->vic;
	struct csi_device 	*csi	= ispcam->csi;
	struct isp_async_device	*isd	= &ispcam->isd[0];
	int ret = 0;

	if(!ispvideo->bypass) {
		v4l2_set_subdev_hostdata(&mscaler->sd, ispvideo);
		ret = v4l2_subdev_call(&mscaler->sd, video, s_stream, 1);
		if(ret < 0) {
			goto mscaler_streamon_err;
		}

		ret = v4l2_subdev_call(&isp->sd, video, s_stream, 1);
		if(ret < 0) {
			goto isp_streamon_err;
		}
	}

	if(csi && (isd->bus_type == V4L2_MBUS_CSI2)) {
		ret = v4l2_subdev_call(&csi->sd, video, s_stream, 1);
		if(ret < 0) {
			goto csi_streamon_err;
		}
	}

	if(!isd->enabled) {
		ret = v4l2_subdev_call(isd->sd, video, s_stream, 1);
		if(ret < 0) {
			goto isd_streamon_err;
		}
	}
	isd->enabled++;

	ret = v4l2_subdev_call(&vic->sd, video, s_stream, 1);
	if(ret < 0) {
		goto vic_streamon_err;
	}

	return ret;
vic_streamon_err:
	isd->enabled--;
	if(!isd->enabled) {
		ret = v4l2_subdev_call(isd->sd, video, s_stream, 0);
	}
isd_streamon_err:
	if(csi && (isd->bus_type == V4L2_MBUS_CSI2))
		v4l2_subdev_call(&csi->sd, video, s_stream, 0);
csi_streamon_err:
	if(!ispvideo->bypass)
		v4l2_subdev_call(&isp->sd, video, s_stream, 0);
isp_streamon_err:
	if(!ispvideo->bypass)
		v4l2_subdev_call(&mscaler->sd, video, s_stream, 0);
mscaler_streamon_err:
	dev_err(ispvideo->dev, "start stream error\n");
	return ret;
}

static int isp_video_subdev_streamoff(struct isp_video_device *ispvideo)
{
	struct ispcam_device 	*ispcam = ispvideo->ispcam;
	struct mscaler_device 	*mscaler= ispcam->mscaler;
	struct isp_device 	*isp	= ispcam->isp;
	struct vic_device	*vic	= ispcam->vic;
	struct csi_device 	*csi	= ispcam->csi;
	struct isp_async_device	*isd	= &ispcam->isd[0];
	int ret = 0;

	//global ispcam device mutex.
	if(!ispvideo->bypass) {
		v4l2_set_subdev_hostdata(&mscaler->sd, ispvideo);
		ret = v4l2_subdev_call(&mscaler->sd, video, s_stream, 0);
		if(ret < 0) {
			goto err;
		}

		ret = v4l2_subdev_call(&isp->sd, video, s_stream, 0);
		if(ret < 0) {
			goto err;
		}
	}

	if(csi && (isd->bus_type == V4L2_MBUS_CSI2)) {
		ret = v4l2_subdev_call(&csi->sd, video, s_stream, 0);
		if(ret < 0) {
			goto err;
		}
	}

	isd->enabled--;
	if(!isd->enabled) {
		ret = v4l2_subdev_call(isd->sd, video, s_stream, 0);
		if(ret < 0) {
			goto err;
		}
	}

	ret = v4l2_subdev_call(&vic->sd, video, s_stream, 0);
	if(ret < 0) {
		goto err;
	}

	return ret;
err:
	dev_err(ispvideo->dev, "start stream error\n");
	return ret;

}

static int isp_video_subdev_stream(struct isp_video_device *ispvideo, int enable)
{
	int ret = 0;

	//mutex lock.

	if(enable) {
		ret = isp_video_subdev_streamon(ispvideo);
	} else {
		ret = isp_video_subdev_streamoff(ispvideo);
	}

	return ret;
}

static int isp_video_start_streaming(struct vb2_queue *q, unsigned int count)
{
	struct isp_video_ctx *ctx = vb2_get_drv_priv(q);
	struct isp_video_device *ispvideo = ctx->ispvideo;
	struct media_entity *entity = &ispvideo->video.entity;
	struct isp_pipeline *pipe = &ispvideo->pipe;
	struct ispcam_device 	*ispcam = ispvideo->ispcam;
	int ret = 0;

	mutex_lock(&ispcam->mutex);

	mutex_lock(&ispvideo->stream_lock);

	if(ispcam->enabled++ == 0) {
		mutex_lock(&sensor_id_lock);
		ispcam->sensor_id = get_sensor_id();
		//printk("~~~~~~~~~~~~~~sensor_id = %d\n", ispcam->sensor_id);
		if(ispcam->sensor_id < 0)
			goto err_get_sensor_id;
		mutex_unlock(&sensor_id_lock);
		ret = media_entity_pipeline_start(entity, &pipe->pipeline);
		if(ret < 0) {
			dev_err(ispvideo->dev, "Failed to start pipeline\n");
			goto err_pipeline_start;
		}

		ret = isp_video_subdev_reset(ispvideo);
		ret = isp_video_subdev_init(ispvideo);
		if(ret) {
			dev_err(ispvideo->dev, "Failed to init subdev\n");
			goto err_subdev_init;
		}
	}

	ret = isp_video_subdev_stream(ispvideo, 1);

	if(ret < 0) {
		goto err_start_stream;
	}

	mutex_unlock(&ispvideo->stream_lock);

	mutex_unlock(&ispcam->mutex);
	return 0;

err_start_stream:
err_subdev_init:
	media_entity_pipeline_stop(entity);
err_pipeline_start:
	put_sensor_id(ispcam->sensor_id);
err_get_sensor_id:
	ispcam->enabled--;
	mutex_unlock(&ispvideo->stream_lock);
	mutex_unlock(&ispcam->mutex);
	return ret;
}
static void isp_video_stop_streaming(struct vb2_queue *q)
{
	struct isp_video_ctx *ctx = vb2_get_drv_priv(q);
	struct isp_video_device *ispvideo = ctx->ispvideo;
	//struct isp_pipeline *pipe = &ispvideo->pipe;
	struct ispcam_device 	*ispcam = ispvideo->ispcam;
	unsigned int is_streaming = 0;
	int ret = 0;

	mutex_lock(&ispcam->mutex);

	mutex_lock(&ispvideo->stream_lock);



	is_streaming = vb2_is_streaming(q);

	if(!is_streaming)
		goto done;

	ret = isp_video_subdev_stream(ispvideo, 0);

	if(--ispcam->enabled > 0) {
		goto finish;
	}

	mutex_lock(&sensor_id_lock);
	put_sensor_id(ispcam->sensor_id);
	mutex_unlock(&sensor_id_lock);
	media_entity_pipeline_stop(&ispvideo->video.entity);

finish:
done:
	mutex_unlock(&ispvideo->stream_lock);
	mutex_unlock(&ispcam->mutex);
}

static const struct vb2_ops isp_video_queue_ops = {
        .queue_setup = isp_video_queue_setup,
        .buf_prepare = isp_video_buffer_prepare,
        .buf_queue = isp_video_buffer_queue,
	.start_streaming = isp_video_start_streaming,
	.stop_streaming = isp_video_stop_streaming,
};


static int isp_video_open(struct file *file)
{
        struct isp_video_device *ispvideo = video_drvdata(file);
	struct video_device *vdev = &ispvideo->video;
	struct isp_prestart_info *p_info = ispvideo->ispcam->p_info;
	struct isp_video_ctx *ctx = NULL;
	struct vb2_queue *queue = NULL;
	int ret = 0;

	if(ispvideo->prestart) {
		if(IS_ERR_OR_NULL(p_info)) {
			dev_err(ispvideo->ispcam->dev, "prestart thread not runing\n");
			return -ENOMEM;
		}
		return 0;
	}

	v4l2_fh_open(file);
	if(v4l2_fh_is_singular(file->private_data))
	{
		mutex_lock(&ispvideo->mutex);
		ctx = kzalloc(sizeof(struct isp_video_ctx), GFP_KERNEL);
		if(IS_ERR_OR_NULL(ctx)) {
			dev_err(ispvideo->ispcam->dev, "Failed to alloc ctx for isp_video_ctx\n");
			ret = -ENOMEM;
			goto err_ctx_alloc;
		}

		queue = &ctx->queue;
		queue->type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
		queue->io_modes = VB2_MMAP | VB2_USERPTR | VB2_DMABUF;
		queue->drv_priv = ctx;
		queue->ops = &isp_video_queue_ops;
		queue->mem_ops = &ingenic_vb2_dma_contig_memops;
		queue->buf_struct_size = sizeof(struct isp_video_buffer);
		queue->timestamp_flags = V4L2_BUF_FLAG_TIMESTAMP_MONOTONIC;
		queue->lock = &ispvideo->queue_lock;

		ret = vb2_queue_init(&ctx->queue);
		if(ret < 0) {
			goto err_vb2_queue_init;
		}

		ctx->ispvideo = ispvideo;
		ispvideo->ctx = ctx;
		vdev->queue = ispvideo->queue = queue;
		ctx->fh = file->private_data;

		/*TODO:init user fmt*/
#if 0
		ctx->format.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
		ctx->format.fmt.pix.width = 1920;
		ctx->format.fmt.pix.height = 1080;
		ctx->format.fmt.pix.pixelformat = V4L2_PIX_FMT_NV12;
		ctx->format.fmt.pix.bytesperline = 1920*12/8;
		ctx->format.fmt.pix.sizeimage = 1920*1080*1.5;
		ctx->format.fmt.pix.colorspace = V4L2_COLORSPACE_SRGB;
		ctx->format.fmt.pix.field = V4L2_FIELD_NONE;
#endif

		mutex_unlock(&ispvideo->mutex);
	}

	return 0;
err_vb2_queue_init:
	kfree(ctx);
err_ctx_alloc:
	mutex_unlock(&ispvideo->mutex);
	v4l2_fh_release(file);
	return ret;
}
static int isp_video_release(struct file *file)
{
        struct isp_video_device *ispvideo = video_drvdata(file);
	struct video_device *vdev = &ispvideo->video;
	struct isp_video_ctx *ctx = ispvideo->ctx;

	if(ispvideo->prestart)
	{
		ispvideo->prestart = 0;
		kfree(ispvideo->ispcam->p_info);
		ispvideo->ispcam->p_info = NULL;
	}

	if(v4l2_fh_is_singular(file->private_data)) {
		mutex_lock(&ispvideo->mutex);

		vb2_queue_release(&ctx->queue);
		kfree(ctx);
		vdev->queue = ispvideo->queue = NULL;

		mutex_unlock(&ispvideo->mutex);
	}

	v4l2_fh_release(file);
	file->private_data = NULL;

	return 0;
}

const static struct v4l2_file_operations isp_video_fops = {
        .owner = THIS_MODULE,
        .unlocked_ioctl = video_ioctl2,
        .open = isp_video_open,
        .release = isp_video_release,
        .poll = vb2_fop_poll, /*isp_video_poll,*/
        .mmap = vb2_fop_mmap, /*isp_video_mmap,*/
};


int isp_video_init(struct isp_video_device *ispvideo, char *name, const struct isp_video_ops *video_ops)
{
	int ret = 0;
	struct video_device *video = &ispvideo->video;


	/*创建buffer分配器*/
	ispvideo->alloc_ctx = ingenic_vb2_dma_contig_init_ctx(ispvideo->ispcam->dev);
	if(IS_ERR(ispvideo)) {
		return -ENOMEM;
	}

	/*初始化一个sink pad*/
	ispvideo->pad.flags = MEDIA_PAD_FL_SINK | MEDIA_PAD_FL_MUST_CONNECT;
	ret = media_entity_init(&video->entity, 1, &ispvideo->pad, 0);
	if(ret < 0) {
		/*TODO: error case*/
	}

	mutex_init(&ispvideo->queue_lock);
	mutex_init(&ispvideo->mutex);
	mutex_init(&ispvideo->stream_lock);

	snprintf(video->name, sizeof(video->name), "isp-%s", name);
	video->fops = &isp_video_fops;

	if(ispvideo->bypass)
		video->ioctl_ops = &bypass_video_ioctl_ops;
        else
		video->ioctl_ops = &isp_video_ioctl_ops;

	video->release = video_device_release_empty;

	ispvideo->name = video->name;
	ispvideo->ops = video_ops;
	ispvideo->dev = ispvideo->ispcam->dev;

	video_set_drvdata(&ispvideo->video, ispvideo);

	return 0;

}

int isp_video_cleanup(struct isp_video_device *ispvideo)
{

	return 0;
}

int isp_video_register(struct isp_video_device *ispvideo, struct v4l2_device *v4l2_dev, int nr)
{
	struct video_device *video = &ispvideo->video;
	int ret = 0;

	video->v4l2_dev = v4l2_dev;

	ret = video_register_device(video, VFL_TYPE_GRABBER, nr);
	if(ret < 0) {
	/*TODO: error case*/
	}
	set_bit(_IOC_NR(VIDIOC_STREAMOFF), video->disable_locking);

	dev_info(ispvideo->dev, "register video device %s @ /dev/video%d ok\n", video->name, video->num);
	mutex_init(&sensor_id_lock);

	return ret;
}
int isp_video_unregister(struct isp_video_device *ispvideo)
{

	return 0;
}


/*prestart*/
static struct isp_video_ctx *isp_prestart_open(struct isp_video_device *ispvideo)
{

	struct video_device *vdev = &ispvideo->video;
	struct vb2_queue *queue = NULL;
	struct isp_video_ctx *ctx = NULL;
	int ret = 0;

        mutex_lock(&ispvideo->mutex);
	if(ispvideo->queue != NULL) {
		mutex_unlock(&ispvideo->mutex);
		return NULL;
	}

	ctx = kzalloc(sizeof(struct isp_video_ctx), GFP_KERNEL);
	if(IS_ERR_OR_NULL(ctx)) {
		dev_err(ispvideo->ispcam->dev, "Failed to alloc ctx for isp_video_ctx\n");
		ret = -ENOMEM;
		goto err_ctx_alloc;
	}

	queue = &ctx->queue;
	queue->type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
	queue->io_modes = VB2_MMAP | VB2_USERPTR;
	queue->drv_priv = ctx;
	queue->ops = &isp_video_queue_ops;
	queue->mem_ops = &ingenic_vb2_dma_contig_memops;
	queue->buf_struct_size = sizeof(struct isp_video_buffer);
	queue->timestamp_flags = V4L2_BUF_FLAG_TIMESTAMP_MONOTONIC;
	queue->lock = &ispvideo->queue_lock;

	ret = vb2_queue_init(&ctx->queue);
	if(ret < 0) {
		goto err_vb2_queue_init;
	}

	ctx->ispvideo = ispvideo;
	ispvideo->ctx = ctx;
	vdev->queue = ispvideo->queue = queue;

	mutex_unlock(&ispvideo->mutex);

	return ctx;
err_vb2_queue_init:
	kfree(ctx);
err_ctx_alloc:
	mutex_unlock(&ispvideo->mutex);
	return NULL;
}

static int isp_prestart_video_s_fmt(struct isp_video_ctx *ctx, struct v4l2_format *format)
{
        struct isp_video_device *ispvideo = ctx->ispvideo;
	struct v4l2_subdev *sd = NULL;
	struct v4l2_subdev_format subdev_fmt;
	unsigned int pad = 0;
	int ret = 0;

	if(vb2_is_busy(&ctx->queue)) {
		return -EBUSY;
	}

	ret = __isp_video_try_format(ctx, &format->fmt.pix, NULL);
	if(ret < 0) {
		dev_err(ispvideo->dev, "Failed to set format cap!\n");
		return -EINVAL;
	}

	sd = isp_video_remote_subdev(ispvideo, &pad);
	if(sd == NULL) {
		return -EINVAL;
	}

	ctx->format = *format;

	to_v4l2_mbus_framefmt(ispvideo, format, &subdev_fmt.format);

	subdev_fmt.pad = pad;
	subdev_fmt.which = V4L2_SUBDEV_FORMAT_ACTIVE;

	ret = v4l2_subdev_call(sd, pad, set_fmt, NULL, &subdev_fmt);
	if(ret < 0) {
		dev_err(ispvideo->dev, "Failed to set subdev format\n");
		return -EINVAL;
	}

	return 0;
}

int isp_prestart(void *data)
{
	struct ispcam_device *ispcam = data;
	struct isp_prestart_info *p_info = ispcam->p_info;
	struct isp_prestart_message *p_message = p_info->p_message;
	struct isp_video_device *ispvideo = &ispcam->mscaler->ispvideo[p_message->ch_nr];
	struct video_device *vdev = &ispvideo->video;
	struct isp_video_ctx *ctx = NULL;
	int ret = 0;
	int miss_count = 0;
	struct v4l2_format format = {0};
	int count = 3;
	struct v4l2_buffer buf = {0};
	int i = 0;

	ispvideo->prestart = 1;
	init_waitqueue_head(&p_info->wq);

	/*open*/
	ctx = isp_prestart_open(ispvideo);

	/*S_FMT*/

	format.fmt.pix.width = p_message->width;
	format.fmt.pix.height = p_message->height;
	format.fmt.pix.pixelformat = p_message->pixel_format;
	isp_prestart_video_s_fmt(ctx, &format);

	/*REQBUFS*/

	ret = vb2_core_reqbufs(vdev->queue, V4L2_MEMORY_MMAP, &count);
	if(ret)
	{
		goto err;
	}
	p_info->buf_cnt = count;

	/*QBUF*/
	for(i = 0; i < count; i++)
	{

		buf.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
		buf.memory = V4L2_MEMORY_MMAP;
		buf.index = i;
		vb2_qbuf(vdev->queue, &buf);
	}

	/*STREAMON*/
	ret = vb2_streamon(vdev->queue, V4L2_BUF_TYPE_VIDEO_CAPTURE);
	atomic_set(&p_info->driver_streaming, 1);

	while(!atomic_read(&p_info->usr_streaming));

	atomic_set(&p_info->driver_streaming, 0);
	wake_up(&p_info->wq);

	miss_count = p_info->frame_cnt - p_info->buf_cnt;
	dev_dbg(ispcam->dev, "%d frames passed\n",miss_count);

	return 0;
err:
	dev_err(ispcam->dev, "isp prestart err = %d\n", ret);
	return ret;
}

