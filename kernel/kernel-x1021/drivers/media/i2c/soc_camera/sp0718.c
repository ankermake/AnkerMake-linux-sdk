/*
 * A Camer driver for SuperPix SP0718 cameras on A20.
 * Author:Yuanjianping@superpix.com.cn,20130311
 */
#include <linux/init.h>
#include <linux/module.h>
#include <linux/slab.h>
#include <linux/i2c.h>
#include <linux/delay.h>
#include <linux/videodev2.h>
#include <linux/clk.h>
#include <media/v4l2-device.h>
#include <media/v4l2-chip-ident.h>
#include <media/v4l2-mediabus.h>
#include <media/soc_camera.h>
#include <media/soc_mediabus.h>
#include <linux/io.h>

//for internel driver debug
//#define DEV_DBG_EN
#ifdef DEV_DBG_EN
#define csi_dev_dbg(x,arg...) printk(KERN_INFO"[CSI_DEBUG][SP0718]"x,##arg)
#else
#define csi_dev_dbg(x,arg...)
#endif
#define csi_dev_err(x,arg...) printk(KERN_INFO"[CSI_ERR][SP0718]"x,##arg)
#define csi_dev_print(x,arg...) printk(KERN_INFO"[CSI][SP0718]"x,##arg)

//define the voltage level of control signal
#define CSI_STBY_ON		1
#define CSI_STBY_OFF		0
#define CSI_RST_ON		0
#define CSI_RST_OFF		1
#define CSI_PWR_ON		1
#define CSI_PWR_OFF		0

#define V4L2_IDENT_SENSOR	0x0718

#define REG_ADDR_STEP		1
#define REG_DATA_STEP		1
#define REG_STEP 		(REG_ADDR_STEP+REG_DATA_STEP)

/*
 * Basic window sizes.  These probably belong somewhere more globally
 * useful.
 */
#define VGA_WIDTH		640
#define VGA_HEIGHT		480
#define QVGA_WIDTH		320
#define QVGA_HEIGHT		240

#define SP0718_NORMAL_Y0ffset  0x20
#define SP0718_LOWLIGHT_Y0ffset  0x25
//AE target
#define  SP0718_P1_0xeb  0x78
#define  SP0718_P1_0xec  0x6c
#define  SP0718_P0_0xeb  0x78
#define  SP0718_P0_0xec  0x6c
//HEQ
#define  SP0718_P1_0x10  0x00//outdoor
#define  SP0718_P1_0x14  0x20
#define  SP0718_P1_0x11  0x00//nr
#define  SP0718_P1_0x15  0x18
#define  SP0718_P1_0x12  0x00//dummy
#define  SP0718_P1_0x16  0x10
#define  SP0718_P1_0x13  0x00//low
#define  SP0718_P1_0x17  0x00

/*
 * Our nominal (default) frame rate.
 */
#define SENSOR_FRAME_RATE 10

/*
 * The sp0718 sits on i2c with ID 0x42
 */
#define	ID_REG			0x02
#define	SP0718_CHIP_ID		0x71

enum v4l2_whiteblance {
	V4L2_WB_AUTO,
	V4L2_WB_CLOUD,
	V4L2_WB_DAYLIGHT,
	V4L2_WB_INCANDESCENCE,
	V4L2_WB_FLUORESCENT,
	V4L2_WB_TUNGSTEN,
};

struct sensor_info {
	struct v4l2_subdev sd;
	struct v4l2_ctrl_handler hdl;
	struct sensor_format_struct *fmt;  /* Current format */
	int width;
	int height;
	int brightness;
	int contrast;
	int saturation;
	int hue;
	int hflip;
	int vflip;
	int gain;
	int autogain;
	int exp;
	enum v4l2_exposure_auto_type autoexp;
	int autowb;
	enum v4l2_whiteblance wb;
	enum v4l2_colorfx clrfx;
	u8 clkrc;			/* Clock divider value */
};

static inline struct sensor_info *to_state(struct v4l2_subdev *sd)
{
	return container_of(sd, struct sensor_info, sd);
}

struct regval_list {
	unsigned char reg_num[REG_ADDR_STEP];
	unsigned char value[REG_DATA_STEP];
};

static struct regval_list sensor_default_regs[] =
{
	{{0xfd},{0x00}},
	{{0x1C},{0x3c}}, //0x00 modify by sp_yjp for test,20130311
	{{0x31},{0x00}},//0x00 modify by sp_yjp for test,20130311
	{{0x27},{0xb3}},///0xb3	///2x gain
	{{0x1b},{0x17}},
	{{0x26},{0xaa}},
	{{0x37},{0x02}},
	{{0x28},{0x8f}},
	{{0x1a},{0x73}},
	{{0x1e},{0x1b}},
	{{0x21},{0x06}},  ///blackout voltage
	{{0x22},{0x2a}},  ///colbias
	{{0x0f},{0x3f}},
	{{0x10},{0x3e}},
	{{0x11},{0x00}},
	{{0x12},{0x01}},
	{{0x13},{0x3f}},
	{{0x14},{0x04}},
	{{0x15},{0x30}},
	{{0x16},{0x31}},
	{{0x17},{0x01}},
	{{0x69},{0x31}},
	{{0x6a},{0x2a}},
	{{0x6b},{0x33}},
	{{0x6c},{0x1a}},
	{{0x6d},{0x32}},
	{{0x6e},{0x28}},
	{{0x6f},{0x29}},
	{{0x70},{0x34}},
	{{0x71},{0x18}},
	{{0x36},{0x00}},//02 delete badframe
	{{0xfd},{0x01}},
	{{0x5d},{0x51}},//position
	{{0xf2},{0x19}},

	//Blacklevel
	{{0x1f},{0x10}},
	{{0x20},{0x1f}},
	//pregain
	{{0xfd},{0x02}},
	{{0x00},{0x88}},
	{{0x01},{0x88}},

	/***30 - 8 fps best**/
	{{0xfd},{0x00}},
	{{0x03},{0x03}},
	{{0x04},{0x96}},
	{{0x06},{0x00}},
	{{0x09},{0x00}},
	{{0x0a},{0x00}},
	{{0xfd},{0x01}},
	{{0xef},{0x99}},
	{{0xf0},{0x00}},
	{{0x02},{0x0c}},
	{{0x03},{0x01}},
	{{0x06},{0x93}},
	{{0x07},{0x00}},
	{{0x08},{0x01}},
	{{0x09},{0x00}},
	//Status
	{{0xfd},{0x02}},
	{{0xbe},{0x2c}},
	{{0xbf},{0x07}},
	{{0xd0},{0x2c}},
	{{0xd1},{0x07}},
	{{0xfd},{0x01}},
	{{0x5b},{0x07}},
	{{0x5c},{0x2c}},

	//rpc
	{{0xfd},{0x01}},
	{{0xe0},{0x40}},////24//4c//48//4c//44//4c//3e//3c//3a//38//rpc_1base_max
	{{0xe1},{0x30}},////24//3c//38//3c//36//3c//30//2e//2c//2a//rpc_2base_max
	{{0xe2},{0x2e}},////24//34//30//34//2e//34//2a//28//26//26//rpc_3base_max
	{{0xe3},{0x2a}},////24//2a//2e//2c//2e//2a//2e//26//24//22//rpc_4base_max
	{{0xe4},{0x2a}},////24//2a//2e//2c//2e//2a//2e//26//24//22//rpc_5base_max
	{{0xe5},{0x28}},////24//2c//2a//2c//28//2c//24//22//20//rpc_6base_max
	{{0xe6},{0x28}},////24//2c//2a//2c//28//2c//24//22//20//rpc_7base_max
	{{0xe7},{0x26}},////24//2a//28//2a//26//2a//22//20//20//1e//rpc_8base_max
	{{0xe8},{0x26}},////24//2a//28//2a//26//2a//22//20//20//1e//rpc_9base_max
	{{0xe9},{0x26}},////24//2a//28//2a//26//2a//22//20//20//1e//rpc_10base_max
	{{0xea},{0x26}},////24//28//26//28//24//28//20//1f//1e//1d//rpc_11base_max
	{{0xf3},{0x26}},////24//28//26//28//24//28//20//1f//1e//1d//rpc_12base_max
	{{0xf4},{0x26}},////24//28//26//28//24//28//20//1f//1e//1d//rpc_13base_max
	//ae gain &status
	{{0xfd},{0x01}},
	{{0x04},{0xe0}},//rpc_max_indr
	{{0x05},{0x26}},//1e//rpc_min_indr
	{{0x0a},{0xa0}},//rpc_max_outdr
	{{0x0b},{0x26}},//rpc_min_outdr
	{{0x5a},{0x40}},//dp rpc
	{{0xfd},{0x02}},
	{{0xbc},{0xa0}},//rpc_heq_low
	{{0xbd},{0x80}},//rpc_heq_dummy
	{{0xb8},{0x80}},//mean_normal_dummy
	{{0xb9},{0x90}},//mean_dummy_normal

	//ae target
	{{0xfd},{0x01}},
	{{0xeb},{SP0718_P1_0xeb}},//78
	{{0xec},{SP0718_P1_0xec}},//78
	{{0xed},{0x0a}},
	{{0xee},{0x10}},

	//lsc
	{{0xfd},{0x01}},
	{{0x26},{0x30}},
	{{0x27},{0x2c}},
	{{0x28},{0x07}},
	{{0x29},{0x08}},
	{{0x2a},{0x40}},
	{{0x2b},{0x03}},
	{{0x2c},{0x00}},
	{{0x2d},{0x00}},

	{{0xa1},{0x24}},
	{{0xa2},{0x27}},
	{{0xa3},{0x27}},
	{{0xa4},{0x2b}},
	{{0xa5},{0x1c}},
	{{0xa6},{0x1a}},
	{{0xa7},{0x1a}},
	{{0xa8},{0x1a}},
	{{0xa9},{0x18}},
	{{0xaa},{0x1c}},
	{{0xab},{0x17}},
	{{0xac},{0x17}},
	{{0xad},{0x08}},
	{{0xae},{0x08}},
	{{0xaf},{0x08}},
	{{0xb0},{0x00}},
	{{0xb1},{0x00}},
	{{0xb2},{0x00}},
	{{0xb3},{0x00}},
	{{0xb4},{0x00}},
	{{0xb5},{0x02}},
	{{0xb6},{0x06}},
	{{0xb7},{0x00}},
	{{0xb8},{0x00}},


	//DP
	{{0xfd},{0x01}},
	{{0x48},{0x09}},
	{{0x49},{0x99}},

	//awb
	{{0xfd},{0x01}},
	{{0x32},{0x05}},
	{{0xfd},{0x00}},
	{{0xe7},{0x03}},
	{{0xfd},{0x02}},
	{{0x26},{0xc8}},
	{{0x27},{0xb6}},
	{{0xfd},{0x00}},
	{{0xe7},{0x00}},
	{{0xfd},{0x02}},
	{{0x1b},{0x80}},
	{{0x1a},{0x80}},
	{{0x18},{0x26}},
	{{0x19},{0x28}},
	{{0xfd},{0x02}},
	{{0x2a},{0x00}},
	{{0x2b},{0x08}},
	{{0x28},{0xef}},//0xa0//f8
	{{0x29},{0x20}},

	//d65 90  e2 93
	{{0x66},{0x42}},//0x59//0x60////0x58//4e//0x48
	{{0x67},{0x62}},//0x74//0x70//0x78//6b//0x69
	{{0x68},{0xee}},//0xd6//0xe3//0xd5//cb//0xaa
	{{0x69},{0x18}},//0xf4//0xf3//0xf8//ed
	{{0x6a},{0xa6}},//0xa5
	//indoor 91
	{{0x7c},{0x3b}},//0x45//30//41//0x2f//0x44
	{{0x7d},{0x5b}},//0x70//60//55//0x4b//0x6f
	{{0x7e},{0x15}},//0a//0xed
	{{0x7f},{0x39}},//23//0x28
	{{0x80},{0xaa}},//0xa6
	//cwf   92
	{{0x70},{0x3e}},//0x38//41//0x3b
	{{0x71},{0x59}},//0x5b//5f//0x55
	{{0x72},{0x31}},//0x30//22//0x28
	{{0x73},{0x4f}},//0x54//44//0x45
	{{0x74},{0xaa}},
	//tl84  93
	{{0x6b},{0x1b}},//0x18//11
	{{0x6c},{0x3a}},//0x3c//25//0x2f
	{{0x6d},{0x3e}},//0x3a//35
	{{0x6e},{0x59}},//0x5c//46//0x52
	{{0x6f},{0xaa}},
	//f    94
	{{0x61},{0xea}},//0x03//0x00//f4//0xed
	{{0x62},{0x03}},//0x1a//0x25//0f//0f
	{{0x63},{0x6a}},//0x62//0x60//52//0x5d
	{{0x64},{0x8a}},//0x7d//0x85//70//0x75//0x8f
	{{0x65},{0x6a}},//0xaa//6a

	{{0x75},{0x80}},
	{{0x76},{0x20}},
	{{0x77},{0x00}},
	{{0x24},{0x25}},

	//针对室内调偏不过灯箱测试使用//针对人脸调偏
	{{0x20},{0xd8}},
	{{0x21},{0xa3}},//82//a8偏暗照度还有调偏
	{{0x22},{0xd0}},//e3//bc
	{{0x23},{0x86}},

	//outdoor r\b range
	{{0x78},{0xc3}},//d8
	{{0x79},{0xba}},//82
	{{0x7a},{0xa6}},//e3
	{{0x7b},{0x99}},//86


	//skin
	{{0x08},{0x15}},//
	{{0x09},{0x04}},//
	{{0x0a},{0x20}},//
	{{0x0b},{0x12}},//
	{{0x0c},{0x27}},//
	{{0x0d},{0x06}},//
	{{0x0e},{0x63}},//

	//wt th
	{{0x3b},{0x10}},
	//gw
	{{0x31},{0x60}},
	{{0x32},{0x60}},
	{{0x33},{0xc0}},
	{{0x35},{0x6f}},

	// sharp
	{{0xfd},{0x02}},
	{{0xde},{0x0f}},
	{{0xd2},{0x02}},//6//控制黑白边；0-边粗，f-变细
	{{0xd3},{0x06}},
	{{0xd4},{0x06}},
	{{0xd5},{0x06}},
	{{0xd7},{0x20}},//10//2x根据增益判断轮廓阈值
	{{0xd8},{0x30}},//24//1A//4x
	{{0xd9},{0x38}},//28//8x
	{{0xda},{0x38}},//16x
	{{0xdb},{0x08}},//
	{{0xe8},{0x58}},//48//轮廓强度
	{{0xe9},{0x48}},
	{{0xea},{0x30}},
	{{0xeb},{0x20}},
	{{0xec},{0x48}},//60//80
	{{0xed},{0x48}},//50//60
	{{0xee},{0x30}},
	{{0xef},{0x20}},
	//平坦区域锐化力度
	{{0xf3},{0x50}},
	{{0xf4},{0x10}},
	{{0xf5},{0x10}},
	{{0xf6},{0x10}},
	//dns
	{{0xfd},{0x01}},
	{{0x64},{0x44}},//沿方向边缘平滑力度  //0-最强，8-最弱
	{{0x65},{0x22}},
	{{0x6d},{0x04}},//8//强平滑（平坦）区域平滑阈值
	{{0x6e},{0x06}},//8
	{{0x6f},{0x10}},
	{{0x70},{0x10}},
	{{0x71},{0x08}},//0d//弱平滑（非平坦）区域平滑阈值
	{{0x72},{0x12}},//1b
	{{0x73},{0x1c}},//20
	{{0x74},{0x24}},
	{{0x75},{0x44}},//[7:4]平坦区域强度，[3:0]非平坦区域强度；0-最强，8-最弱；
	{{0x76},{0x02}},//46
	{{0x77},{0x02}},//33
	{{0x78},{0x02}},
	{{0x81},{0x10}},//18//2x//根据增益判定区域阈值，低于这个做强平滑、大于这个做弱平滑；
	{{0x82},{0x20}},//30//4x
	{{0x83},{0x30}},//40//8x
	{{0x84},{0x48}},//50//16x
	{{0x85},{0x0c}},//12/8+reg0x81 第二阈值，在平坦和非平坦区域做连接
	{{0xfd},{0x02}},
	{{0xdc},{0x0f}},

	//gamma
	{{0xfd},{0x01}},
	{{0x8b},{0x00}},//00//00
	{{0x8c},{0x0a}},//0c//09
	{{0x8d},{0x16}},//19//17
	{{0x8e},{0x1f}},//25//24
	{{0x8f},{0x2a}},//30//33
	{{0x90},{0x3c}},//44//47
	{{0x91},{0x4e}},//54//58
	{{0x92},{0x5f}},//61//64
	{{0x93},{0x6c}},//6d//70
	{{0x94},{0x82}},//80//81
	{{0x95},{0x94}},//92//8f
	{{0x96},{0xa6}},//a1//9b
	{{0x97},{0xb2}},//ad//a5
	{{0x98},{0xbf}},//ba//b0
	{{0x99},{0xc9}},//c4//ba
	{{0x9a},{0xd1}},//cf//c4
	{{0x9b},{0xd8}},//d7//ce
	{{0x9c},{0xe0}},//e0//d7
	{{0x9d},{0xe8}},//e8//e1
	{{0x9e},{0xef}},//ef//ea
	{{0x9f},{0xf8}},//f7//f5
	{{0xa0},{0xff}},//ff//ff
	//CCM
	{{0xfd},{0x02}},
	{{0x15},{0xd0}},//b>th
	{{0x16},{0x95}},//r<th
	//gc镜头照人脸偏黄
	//!F
	{{0xa0},{0x80}},//80
	{{0xa1},{0x00}},//00
	{{0xa2},{0x00}},//00
	{{0xa3},{0x00}},//06
	{{0xa4},{0x8c}},//8c
	{{0xa5},{0xf4}},//ed
	{{0xa6},{0x0c}},//0c
	{{0xa7},{0xf4}},//f4
	{{0xa8},{0x80}},//80
	{{0xa9},{0x00}},//00
	{{0xaa},{0x30}},//30
	{{0xab},{0x0c}},//0c
	//F
	{{0xac},{0x8c}},
	{{0xad},{0xf4}},
	{{0xae},{0x00}},
	{{0xaf},{0xed}},
	{{0xb0},{0x8c}},
	{{0xb1},{0x06}},
	{{0xb2},{0xf4}},
	{{0xb3},{0xf4}},
	{{0xb4},{0x99}},
	{{0xb5},{0x0c}},
	{{0xb6},{0x03}},
	{{0xb7},{0x0f}},

	//sat u
	{{0xfd},{0x01}},
	{{0xd3},{0x9c}},//0x88//50
	{{0xd4},{0x98}},//0x88//50
	{{0xd5},{0x8c}},//50
	{{0xd6},{0x84}},//50
	//sat v
	{{0xd7},{0x9c}},//0x88//50
	{{0xd8},{0x98}},//0x88//50
	{{0xd9},{0x8c}},//50
	{{0xda},{0x84}},//50
	//auto_sat
	{{0xdd},{0x30}},
	{{0xde},{0x10}},
	{{0xd2},{0x01}},//autosa_en
	{{0xdf},{0xff}},//a0//y_mean_th

	//uv_th
	{{0xfd},{0x01}},
	{{0xc2},{0xaa}},
	{{0xc3},{0xaa}},
	{{0xc4},{0x66}},
	{{0xc5},{0x66}},

	//heq
	{{0xfd},{0x01}},
	{{0x0f},{0xff}},
	{{0x10},{SP0718_P1_0x10}}, //out
	{{0x14},{SP0718_P1_0x14}},
	{{0x11},{SP0718_P1_0x11}}, //nr
	{{0x15},{SP0718_P1_0x15}},
	{{0x12},{SP0718_P1_0x12}}, //dummy
	{{0x16},{SP0718_P1_0x16}},
	{{0x13},{SP0718_P1_0x13}}, //low
	{{0x17},{SP0718_P1_0x17}},

	{{0xfd},{0x01}},
	{{0xcd},{0x20}},
	{{0xce},{0x1f}},
	{{0xcf},{0x20}},
	{{0xd0},{0x55}},

	///auto
	{{0xfd},{0x01}},
	{{0xfb},{0x33}},
	{{0x32},{0x15}},
	{{0x33},{0xff}},
	{{0x34},{0xe7}},

	{{0x35},{0x40}},	  	//add by sp_yjp,20121210
	{{0xff},{0xff}}

};

/*
 * The white balance settings
 * Here only tune the R G B channel gain.
 * The white balance enalbe bit is modified in sensor_s_autowb and sensor_s_wb
 */
static struct regval_list sensor_wb_auto_regs[] = {
	{{0xfd},{0x02}},
	{{0x26},{0xc8}},
	{{0x27},{0xb6}},
	{{0xfd},{0x01}},
	{{0x32},{0x15}},   //awb & ae  opened
	{{0xfd},{0x00}},
};

static struct regval_list sensor_wb_cloud_regs[] = {
	{{0xfd},{0x01}},
	{{0x32},{0x05}},
	{{0xfd},{0x02}},
	{{0x26},{0xc8}},
	{{0x27},{0x89}},
	{{0xfd},{0x00}},
};

static struct regval_list sensor_wb_daylight_regs[] = {
	//tai yang guang
	{{0xfd},{0x01}},
	{{0x32},{0x05}},
	{{0xfd},{0x02}},
	{{0x26},{0xaa}},
	{{0x27},{0xce}},
	{{0xfd},{0x00}},
};

static struct regval_list sensor_wb_incandescence_regs[] = {
	//bai re guang
	{{0xfd},{0x01}},
	{{0x32},{0x05}},
	{{0xfd},{0x02}},
	{{0x26},{0x75}},
	{{0x27},{0xe2}},
	{{0xfd},{0x00}},
};

static struct regval_list sensor_wb_fluorescent_regs[] = {
	//ri guang deng
	{{0xfd},{0x01}},
	{{0x32},{0x05}},
	{{0xfd},{0x02}},
	{{0x26},{0x91}},
	{{0x27},{0xc8}},
	{{0xfd},{0x00}},
};

static struct regval_list sensor_wb_tungsten_regs[] = {
	//wu si deng
	{{0xfd},{0x01}},
	{{0x32},{0x05}},
	{{0xfd},{0x02}},
	{{0x26},{0x75}},
	{{0x27},{0xe2}},
	{{0xfd},{0x00}},
};

/*
 * The color effect settings
 */
static struct regval_list sensor_colorfx_none_regs[] = {
	{{0xfd},{0x01}},
	{{0x66},{0x00}},
	{{0x67},{0x80}},
	{{0x68},{0x80}},

};

static struct regval_list sensor_colorfx_bw_regs[] = {
	{{0xfd},{0x01}},
	{{0x66},{0x20}},
	{{0x67},{0x80}},
	{{0x68},{0x80}},
};

static struct regval_list sensor_colorfx_sepia_regs[] = {
	{{0xfd},{0x01}},
	{{0x66},{0x10}},
	{{0x67},{0xc0}},
	{{0x68},{0x20}},
};

static struct regval_list sensor_colorfx_negative_regs[] = {
	{{0xfd},{0x01}},
	{{0x66},{0x04}},
	{{0x67},{0x80}},
	{{0x68},{0x80}},
};

static struct regval_list sensor_colorfx_emboss_regs[] = {
	{{0xfd},{0x01}},
	{{0x66},{0x01}},
	{{0x67},{0x80}},
	{{0x68},{0x80}}
};

static struct regval_list sensor_colorfx_sketch_regs[] = {
	{{0xfd},{0x01}},
	{{0x66},{0x02}},
	{{0x67},{0x80}},
	{{0x68},{0x80}}
};

static struct regval_list sensor_colorfx_sky_blue_regs[] = {
	{{0xfd},{0x01}},
	{{0x66},{0x10}},
	{{0x67},{0x20}},
	{{0x68},{0xf0}}
};

static struct regval_list sensor_colorfx_grass_green_regs[] = {
	{{0xfd},{0x01}},
	{{0x66},{0x10}},
	{{0x67},{0x20}},
	{{0x68},{0x20}}
};

static struct regval_list sensor_colorfx_skin_whiten_regs[] = {
	//NULL
};

static struct regval_list sensor_colorfx_vivid_regs[] = {
	//NULL
};

/*
 * The brightness setttings
 */
static struct regval_list sensor_brightness_neg4_regs[] = {
	{{0xfd},{0x01}},
	{{0xdb},{0xc0}}
};

static struct regval_list sensor_brightness_neg3_regs[] = {
	{{0xfd},{0x01}},
	{{0xdb},{0xd0}}
};

static struct regval_list sensor_brightness_neg2_regs[] = {
	{{0xfd},{0x01}},
	{{0xdb},{0xe0}}
};

static struct regval_list sensor_brightness_neg1_regs[] = {
	{{0xfd},{0x01}},
	{{0xdb},{0xf0}}
};

static struct regval_list sensor_brightness_zero_regs[] = {
	{{0xfd},{0x01}},
	{{0xdb},{0x00}}
};

static struct regval_list sensor_brightness_pos1_regs[] = {
	{{0xfd},{0x01}},
	{{0xdb},{0x10}}
};

static struct regval_list sensor_brightness_pos2_regs[] = {
	{{0xfd},{0x01}},
	{{0xdb},{0x20}}
};

static struct regval_list sensor_brightness_pos3_regs[] = {
	{{0xfd},{0x01}},
	{{0xdb},{0x30}}
};

static struct regval_list sensor_brightness_pos4_regs[] = {
	{{0xfd},{0x01}},
	{{0xdb},{0x40}}
};

/* 640X480 VGA */
static struct regval_list sensor_vga[] =
{
	{{0xfd}, {0x01}},
	{{0x4a}, {0x00}},
	{{0x4b}, {0x01}},
	{{0x4c}, {0xe0}},
	{{0x4d}, {0x00}},
	{{0x4e}, {0x02}},
	{{0x4f}, {0x80}},
	{{0xfd}, {0x02}},
	{{0x0f}, {0x00}},	// 1/4 subsample enable
	{{0xfd}, {0x00}},
	{{0x30}, {0x00}},
};

/* 320*240 QVGA */
static  struct regval_list sensor_qvga[] =
{
	{{0xfd}, {0x01}},
	{{0x4a}, {0x00}},
	{{0x4b}, {0x01}},
	{{0x4c}, {0xe0}},
	{{0x4d}, {0x00}},
	{{0x4e}, {0x02}},
	{{0x4f}, {0x80}},
	{{0xfd}, {0x02}},
	{{0x0f}, {0x01}},	// 1/4 subsample enable
	{{0xfd}, {0x00}},
	{{0x30}, {0x10}},
};

/*
 * The contrast setttings
 */
static struct regval_list sensor_contrast_neg4_regs[] = {
	//NULL
};

static struct regval_list sensor_contrast_neg3_regs[] = {
	//NULL
};

static struct regval_list sensor_contrast_neg2_regs[] = {
	//NULL
};

static struct regval_list sensor_contrast_neg1_regs[] = {
	//NULL
};

static struct regval_list sensor_contrast_zero_regs[] = {
	//NULL
};

static struct regval_list sensor_contrast_pos1_regs[] = {
	//NULL
};

static struct regval_list sensor_contrast_pos2_regs[] = {
	//NULL
};

static struct regval_list sensor_contrast_pos3_regs[] = {
	//NULL
};

static struct regval_list sensor_contrast_pos4_regs[] = {
	//NULL
};

/*
 * The saturation setttings
 */
static struct regval_list sensor_saturation_neg4_regs[] = {
	//NULL
};

static struct regval_list sensor_saturation_neg3_regs[] = {
	//NULL
};

static struct regval_list sensor_saturation_neg2_regs[] = {
	//NULL
};

static struct regval_list sensor_saturation_neg1_regs[] = {
	//NULL
};

static struct regval_list sensor_saturation_zero_regs[] = {
	//NULL
};

static struct regval_list sensor_saturation_pos1_regs[] = {
	//NULL
};

static struct regval_list sensor_saturation_pos2_regs[] = {
	//NULL
};

static struct regval_list sensor_saturation_pos3_regs[] = {
	//NULL
};

static struct regval_list sensor_saturation_pos4_regs[] = {
	//NULL
};

/*
 * The exposure target setttings
 */
static struct regval_list sensor_ev_neg4_regs[] = {
	{{0xfd},{0x01}},
	{{0xeb},{SP0718_P0_0xeb-0x20}},
	{{0xec},{SP0718_P0_0xec-0x20}},
	{{0xdb},{0x00}}
};

static struct regval_list sensor_ev_neg3_regs[] = {
	{{0xfd},{0x01}},
	{{0xeb},{SP0718_P0_0xeb-0x18}},
	{{0xec},{SP0718_P0_0xec-0x18}},
	{{0xdc},{0x00}}
};

static struct regval_list sensor_ev_neg2_regs[] = {
	{{0xfd},{0x01}},
	{{0xeb},{SP0718_P0_0xeb-0x10}},
	{{0xec},{SP0718_P0_0xec-0x10}},
	{{0xdc},{0x00}}
};

static struct regval_list sensor_ev_neg1_regs[] = {
	{{0xfd},{0x01}},
	{{0xeb},{SP0718_P0_0xeb-0x08}},
	{{0xec},{SP0718_P0_0xec-0x08}},
	{{0xdc},{0x00}}
};

static struct regval_list sensor_ev_zero_regs[] = {
	{{0xfd},{0x01}},
	{{0xeb},{SP0718_P0_0xeb}},
	{{0xec},{SP0718_P0_0xec}},
	{{0xdc},{0x00}}
};

static struct regval_list sensor_ev_pos1_regs[] = {
	{{0xfd},{0x01}},
	{{0xeb},{SP0718_P0_0xeb+0x08}},
	{{0xec},{SP0718_P0_0xec+0x08}},
	{{0xdc},{0x00}}
};

static struct regval_list sensor_ev_pos2_regs[] = {
	{{0xfd},{0x01}},
	{{0xeb},{SP0718_P0_0xeb+0x10}},
	{{0xec},{SP0718_P0_0xec+0x10}},
	{{0xdc},{0x00}}
};

static struct regval_list sensor_ev_pos3_regs[] = {
	{{0xfd},{0x01}},
	{{0xeb},{SP0718_P0_0xeb+0x18}},
	{{0xec},{SP0718_P0_0xec+0x18}},
	{{0xdc},{0x00}}
};

static struct regval_list sensor_ev_pos4_regs[] = {
	{{0xfd},{0x01}},
	{{0xeb},{SP0718_P0_0xeb+0x20}},
	{{0xec},{SP0718_P0_0xec+0x20}},
	{{0xdc},{0x00}}
};


/*
 * Here we'll try to encapsulate the changes for just the output
 * video format.
 *
 */


static struct regval_list sensor_fmt_yuv422_yuyv[] = {
	{{0xfd},{0x01}},//Page 0
	{{0x35},{0x40}}	//YCbYCr
};


static struct regval_list sensor_fmt_yuv422_yvyu[] = {
	{{0xfd},{0x01}},//Page 0
	{{0x35},{0x41}}	//YCrYCb
};

static struct regval_list sensor_fmt_yuv422_vyuy[] = {
	{{0xfd},{0x01}},//Page 0
	{{0x35},{0x01}}	//CrYCbY
};

static struct regval_list sensor_fmt_yuv422_uyvy[] = {
	{{0xfd},{0x01}},//Page 0
	{{0x35},{0x00}}	//CbYCrY
};

static struct regval_list sensor_fmt_rgb565[] = {
	{{0xfd},{0x01}},//Page 0
	{{0x35},{0x44}}	//RGB565
};


/*
 * Low-level register I/O.
 *
 */


/*
 * On most platforms, we'd rather do straight i2c I/O.
 */
static int sensor_read(struct v4l2_subdev *sd, unsigned char *reg,
		unsigned char *value)
{
	struct i2c_client *client = v4l2_get_subdevdata(sd);
	u8 data[REG_STEP];
	struct i2c_msg msg;
	int ret,i;

	for(i = 0; i < REG_ADDR_STEP; i++)
		data[i] = reg[i];

	data[REG_ADDR_STEP] = 0xff;
	/*
	 * Send out the register address...
	 */
	msg.addr = client->addr;
	msg.flags = 0;
	msg.len = REG_ADDR_STEP;
	msg.buf = data;
	ret = i2c_transfer(client->adapter, &msg, 1);
	if (ret < 0) {
		csi_dev_err("Error %d on register write\n", ret);
		return ret;
	}
	/*
	 * ...then read back the result.
	 */

	msg.flags = I2C_M_RD;
	msg.len = REG_DATA_STEP;
	msg.buf = &data[REG_ADDR_STEP];

	ret = i2c_transfer(client->adapter, &msg, 1);
	if (ret >= 0) {
		for(i = 0; i < REG_DATA_STEP; i++)
			value[i] = data[i+REG_ADDR_STEP];
		ret = 0;
	}
	else {
		csi_dev_err("Error %d on register read\n", ret);
	}
	return ret;
}


static int sensor_write(struct v4l2_subdev *sd, unsigned char *reg,
		unsigned char *value)
{
	struct i2c_client *client = v4l2_get_subdevdata(sd);
	struct i2c_msg msg;
	unsigned char data[REG_STEP];
	int ret,i;

	for(i = 0; i < REG_ADDR_STEP; i++)
		data[i] = reg[i];
	for(i = REG_ADDR_STEP; i < REG_STEP; i++)
		data[i] = value[i-REG_ADDR_STEP];

	msg.addr = client->addr;
	msg.flags = 0;
	msg.len = REG_STEP;
	msg.buf = data;


	ret = i2c_transfer(client->adapter, &msg, 1);
	if (ret > 0) {
		ret = 0;
	}
	else if (ret < 0) {
		csi_dev_err("sensor_write error!\n");
	}
	return ret;
}


/*
 * Write a list of register settings;
 */
static int sensor_write_array(struct v4l2_subdev *sd, struct regval_list *vals , uint size)
{
	int i,ret;

	if (size == 0)
		return -EINVAL;

	for(i = 0; i < size ; i++)
	{
		if(vals->reg_num[0] == 0xff) {
			mdelay(vals->value[0]);
		}	else {
			ret = sensor_write(sd, vals->reg_num, vals->value);
			if (ret < 0)
			{
				csi_dev_err("sensor_write_err!\n");
				return ret;
			}
		}
		udelay(500);
		vals++;
	}

	return 0;
}

static int sensor_detect(struct v4l2_subdev *sd)
{
	int ret;
	struct regval_list regs;

	regs.reg_num[0] = 0xfd;
	regs.value[0] = 0x00; //PAGE 0x00
	ret = sensor_write(sd, regs.reg_num, regs.value);
	if (ret < 0) {
		csi_dev_err("sensor_write err at sensor_detect!\n");
		return ret;
	}

	regs.reg_num[0] = ID_REG;
	ret = sensor_read(sd, regs.reg_num, regs.value);
	printk("@@@SP:SP0718 SENSOR ID=0x%x\n",regs.value[0] );
	if (ret < 0) {
		csi_dev_err("sensor_read err at sensor_detect!\n");
		return ret;
	}

	if(regs.value[0] != SP0718_CHIP_ID)
		return -ENODEV;

	return 0;
}

static int sensor_init(struct v4l2_subdev *sd, u32 val)
{
	int ret;
	csi_dev_dbg("sensor_init\n");

	/*Make sure it is a target sensor*/
	ret = sensor_detect(sd);
	if (ret) {
		csi_dev_err("chip found is not an target chip.\n");
		return ret;
	}

	return sensor_write_array(sd, sensor_default_regs , ARRAY_SIZE(sensor_default_regs));
}

static int sensor_power(struct v4l2_subdev *sd, int on)
{
	struct i2c_client *client = v4l2_get_subdevdata(sd);
	struct soc_camera_subdev_desc *ssdd = soc_camera_i2c_to_desc(client);
	int ret;

	if (!on)
		return soc_camera_power_off(&client->dev, ssdd);

	ret = soc_camera_power_on(&client->dev, ssdd);
	if (ret < 0)
		return ret;

	return sensor_init(sd, on);
}

/*
 * Store information about the video data format.
 */
static struct sensor_format_struct {
	__u8 *desc;
	enum v4l2_mbus_pixelcode mbus_code;
	struct regval_list *regs;
	int	regs_size;
	int bpp;   /* Bytes per pixel */
} sensor_formats[] = {
	{
		.desc		= "YUYV 4:2:2",
		.mbus_code	= V4L2_MBUS_FMT_YUYV8_2X8,
		.regs 		= sensor_fmt_yuv422_yuyv,
		.regs_size = ARRAY_SIZE(sensor_fmt_yuv422_yuyv),
		.bpp		= 2,
	},
	{
		.desc		= "YVYU 4:2:2",
		.mbus_code	= V4L2_MBUS_FMT_YVYU8_2X8,
		.regs 		= sensor_fmt_yuv422_yvyu,
		.regs_size = ARRAY_SIZE(sensor_fmt_yuv422_yvyu),
		.bpp		= 2,
	},
	{
		.desc		= "UYVY 4:2:2",
		.mbus_code	= V4L2_MBUS_FMT_UYVY8_2X8,
		.regs 		= sensor_fmt_yuv422_uyvy,
		.regs_size = ARRAY_SIZE(sensor_fmt_yuv422_uyvy),
		.bpp		= 2,
	},
	{
		.desc		= "VYUY 4:2:2",
		.mbus_code	= V4L2_MBUS_FMT_VYUY8_2X8,
		.regs 		= sensor_fmt_yuv422_vyuy,
		.regs_size = ARRAY_SIZE(sensor_fmt_yuv422_vyuy),
		.bpp		= 2,
	},
	{
		.desc		= "RGB 565",
		.mbus_code	= V4L2_MBUS_FMT_RGB565_2X8_LE,
		.regs 		= sensor_fmt_rgb565,
		.regs_size = ARRAY_SIZE(sensor_fmt_rgb565),
		.bpp		= 2,
	},
	//	{
	//		.desc		= "Raw RGB Bayer",
	//		.mbus_code	= V4L2_MBUS_FMT_SBGGR8_1X8,
	//		.regs 		= sensor_fmt_raw,
	//		.regs_size = ARRAY_SIZE(sensor_fmt_raw),
	//		.bpp		= 1
	//	},
};
#define N_FMTS ARRAY_SIZE(sensor_formats)


/*
 * Then there is the issue of window sizes.  Try to capture the info here.
 */

static struct sensor_win_size {
	int	width;
	int	height;
	int	hstart;		/* Start/stop values for the camera.  Note */
	int	hstop;		/* that they do not always make complete */
	int	vstart;		/* sense to humans, but evidently the sensor */
	int	vstop;		/* will do the right thing... */
	struct regval_list *regs; /* Regs to tweak */
	int regs_size;
	int (*set_size) (struct v4l2_subdev *sd);
} sensor_win_sizes[] = {
	/* VGA */
	{
		.width		= VGA_WIDTH,
		.height		= VGA_HEIGHT,
		.regs 		= sensor_vga,
		.regs_size	= ARRAY_SIZE(sensor_vga),
		.set_size	= NULL,
	},
	/* QVGA */
	{
		.width		= QVGA_WIDTH,
		.height		= QVGA_HEIGHT,
		.regs 		= sensor_qvga,
		.regs_size	= ARRAY_SIZE(sensor_qvga),
		.set_size	= NULL,
	}
};

#define N_WIN_SIZES (ARRAY_SIZE(sensor_win_sizes))

static int sensor_enum_fmt(struct v4l2_subdev *sd, unsigned index,
		enum v4l2_mbus_pixelcode *code)
{
	if (index >= N_FMTS)
		return -EINVAL;

	*code = sensor_formats[index].mbus_code;

	return 0;
}

static int sensor_try_fmt_internal(struct v4l2_subdev *sd,
		struct v4l2_mbus_framefmt *fmt,
		struct sensor_format_struct **ret_fmt,
		struct sensor_win_size **ret_wsize)
{
	int index;
	struct sensor_win_size *wsize;
	csi_dev_dbg("sensor_try_fmt_internal\n");
	for (index = 0; index < N_FMTS; index++)
		if (sensor_formats[index].mbus_code == fmt->code)
			break;

	if (index >= N_FMTS) {
		/* default to first format */
		index = 0;
		fmt->code = sensor_formats[0].mbus_code;
	}

	if (ret_fmt != NULL)
		*ret_fmt = sensor_formats + index;

	/*
	 * Fields: the sensor devices claim to be progressive.
	 */
	fmt->field = V4L2_FIELD_NONE;


	/*
	 * Round requested image size down to the nearest
	 * we support, but not below the smallest.
	 */
	for (wsize = sensor_win_sizes; wsize < sensor_win_sizes + N_WIN_SIZES;
			wsize++)
		if (fmt->width >= wsize->width && fmt->height >= wsize->height)
			break;

	if (wsize >= sensor_win_sizes + N_WIN_SIZES)
		wsize--;   /* Take the smallest one */
	if (ret_wsize != NULL)
		*ret_wsize = wsize;
	/*
	 * Note the size we'll actually handle.
	 */
	fmt->width = wsize->width;
	fmt->height = wsize->height;

	return 0;
}

static int sensor_try_fmt(struct v4l2_subdev *sd,
		struct v4l2_mbus_framefmt *fmt)
{
	return sensor_try_fmt_internal(sd, fmt, NULL, NULL);
}

/*
 * Set a format.
 */
static int sensor_s_fmt(struct v4l2_subdev *sd,
		struct v4l2_mbus_framefmt *fmt)
{
	int ret;
	struct sensor_format_struct *sensor_fmt;
	struct sensor_win_size *wsize;
	struct sensor_info *info = to_state(sd);

	csi_dev_dbg("sensor_s_fmt\n");

	ret = sensor_try_fmt_internal(sd, fmt, &sensor_fmt, &wsize);
	if (ret)
		return ret;


	sensor_write_array(sd, sensor_fmt->regs , sensor_fmt->regs_size);

	ret = 0;
	if (wsize->regs)
	{
		ret = sensor_write_array(sd, wsize->regs , wsize->regs_size);
		if (ret < 0)
			return ret;
	}

	if (wsize->set_size)
	{
		ret = wsize->set_size(sd);
		if (ret < 0)
			return ret;
	}

	info->fmt = sensor_fmt;
	info->width = wsize->width;
	info->height = wsize->height;

	return 0;
}

/*
 * Implement G/S_PARM.  There is a "high quality" mode we could try
 * to do someday; for now, we just do the frame rate tweak.
 */
static int sensor_g_parm(struct v4l2_subdev *sd, struct v4l2_streamparm *parms)
{
	struct v4l2_captureparm *cp = &parms->parm.capture;
	//struct sensor_info *info = to_state(sd);

	if (parms->type != V4L2_BUF_TYPE_VIDEO_CAPTURE)
		return -EINVAL;

	memset(cp, 0, sizeof(struct v4l2_captureparm));
	cp->capability = V4L2_CAP_TIMEPERFRAME;
	cp->timeperframe.numerator = 1;
	cp->timeperframe.denominator = SENSOR_FRAME_RATE;

	return 0;
}

static int sensor_s_parm(struct v4l2_subdev *sd, struct v4l2_streamparm *parms)
{
	//	struct v4l2_captureparm *cp = &parms->parm.capture;
	//struct v4l2_fract *tpf = &cp->timeperframe;
	//struct sensor_info *info = to_state(sd);
	//int div;

	//	if (parms->type != V4L2_BUF_TYPE_VIDEO_CAPTURE)
	//		return -EINVAL;
	//	if (cp->extendedmode != 0)
	//		return -EINVAL;

	//	if (tpf->numerator == 0 || tpf->denominator == 0)
	//		div = 1;  /* Reset to full rate */
	//	else
	//		div = (tpf->numerator*SENSOR_FRAME_RATE)/tpf->denominator;
	//
	//	if (div == 0)
	//		div = 1;
	//	else if (div > CLK_SCALE)
	//		div = CLK_SCALE;
	//	info->clkrc = (info->clkrc & 0x80) | div;
	//	tpf->numerator = 1;
	//	tpf->denominator = sensor_FRAME_RATE/div;
	//sensor_write(sd, REG_CLKRC, info->clkrc);
	return 0;
}


/*
 * Code for dealing with controls.
 * fill with different sensor module
 * different sensor module has different settings here
 * if not support the follow function ,retrun -EINVAL
 */

//static int sensor_queryctrl(struct v4l2_subdev *sd,
//		struct v4l2_queryctrl *qc)
//{
//	/* Fill in min, max, step and default value for these controls. */
//	/* see include/linux/videodev2.h for details */
//	/* see sensor_s_parm and sensor_g_parm for the meaning of value */
//
//	switch (qc->id) {
//	//	case V4L2_CID_BRIGHTNESS:
//	//		return v4l2_ctrl_query_fill(qc, -4, 4, 1, 1);
//	//	case V4L2_CID_CONTRAST:
//	//		return v4l2_ctrl_query_fill(qc, -4, 4, 1, 1);
//	//	case V4L2_CID_SATURATION:
//	//		return v4l2_ctrl_query_fill(qc, -4, 4, 1, 1);
//	//	case V4L2_CID_HUE:
//	//		return v4l2_ctrl_query_fill(qc, -180, 180, 5, 0);
//		case V4L2_CID_VFLIP:
//		case V4L2_CID_HFLIP:
//			return v4l2_ctrl_query_fill(qc, 0, 1, 1, 0);
//	//	case V4L2_CID_GAIN:
//	//		return v4l2_ctrl_query_fill(qc, 0, 255, 1, 128);
//	//	case V4L2_CID_AUTOGAIN:
//	//		return v4l2_ctrl_query_fill(qc, 0, 1, 1, 1);
//		case V4L2_CID_EXPOSURE:
//			return v4l2_ctrl_query_fill(qc, -4, 4, 1, 0);
//		case V4L2_CID_EXPOSURE_AUTO:
//			return v4l2_ctrl_query_fill(qc, 0, 1, 1, 0);
//		case V4L2_CID_DO_WHITE_BALANCE:
//			return v4l2_ctrl_query_fill(qc, 0, 5, 1, 0);
//		case V4L2_CID_AUTO_WHITE_BALANCE:
//			return v4l2_ctrl_query_fill(qc, 0, 1, 1, 1);
//		case V4L2_CID_COLORFX:
//			return v4l2_ctrl_query_fill(qc, 0, 9, 1, 0);
//	}
//	return -EINVAL;
//}

static int sensor_g_hflip(struct v4l2_subdev *sd, __s32 *value)
{
	int ret;
	struct sensor_info *info = to_state(sd);
	struct regval_list regs;

	regs.reg_num[0] = 0xfd;
	regs.value[0] = 0x00;		//page 0
	ret = sensor_write(sd, regs.reg_num, regs.value);
	if (ret < 0) {
		csi_dev_err("sensor_write err at sensor_g_hflip!\n");
		return ret;
	}

	regs.reg_num[0] = 0x31;
	ret = sensor_read(sd, regs.reg_num, regs.value);
	if (ret < 0) {
		csi_dev_err("sensor_read err at sensor_g_hflip!\n");
		return ret;
	}

	regs.value[0] &= (1<<5);
	regs.value[0] = regs.value[0]>>5;		//0x31 bit5 is mirror

	*value = regs.value[0];

	info->hflip = *value;
	return 0;
}

static int sensor_s_hflip(struct v4l2_subdev *sd, int value)
{
	int ret;
	struct sensor_info *info = to_state(sd);
	struct regval_list regs;

	regs.reg_num[0] = 0xfd;
	regs.value[0] = 0x00;		//page 0
	ret = sensor_write(sd, regs.reg_num, regs.value);
	if (ret < 0) {
		csi_dev_err("sensor_write err at sensor_s_hflip!\n");
		return ret;
	}

	regs.reg_num[0] = 0x31;
	ret = sensor_read(sd, regs.reg_num, regs.value);
	if (ret < 0) {
		csi_dev_err("sensor_read err at sensor_s_hflip!\n");
		return ret;
	}

	switch (value) {
		case 0:
			regs.value[0] &= 0xdf;
			break;
		case 1:
			regs.value[0] |= 0x20;
			break;
		default:
			return -EINVAL;
	}
	ret = sensor_write(sd, regs.reg_num, regs.value);
	if (ret < 0) {
		csi_dev_err("sensor_write err at sensor_s_hflip!\n");
		return ret;
	}

	mdelay(100);

	info->hflip = value;
	return 0;
}

static int sensor_g_vflip(struct v4l2_subdev *sd, __s32 *value)
{
	int ret;
	struct sensor_info *info = to_state(sd);
	struct regval_list regs;

	regs.reg_num[0] = 0xfd;
	regs.value[0] = 0x00;		//page 0
	ret = sensor_write(sd, regs.reg_num, regs.value);
	if (ret < 0) {
		csi_dev_err("sensor_write err at sensor_g_vflip!\n");
		return ret;
	}

	regs.reg_num[0] = 0x31;
	ret = sensor_read(sd, regs.reg_num, regs.value);
	if (ret < 0) {
		csi_dev_err("sensor_read err at sensor_g_vflip!\n");
		return ret;
	}

	regs.value[0] &= (1<<6);
	regs.value[0] = regs.value[0]>>6;		//0x31 bit6 is upsidedown

	*value = regs.value[0];

	info->vflip = *value;
	return 0;
}

static int sensor_s_vflip(struct v4l2_subdev *sd, int value)
{
	int ret;
	struct sensor_info *info = to_state(sd);
	struct regval_list regs;

	regs.reg_num[0] = 0xfd;
	regs.value[0] = 0x00;		//page 0
	ret = sensor_write(sd, regs.reg_num, regs.value);
	if (ret < 0) {
		csi_dev_err("sensor_write err at sensor_s_vflip!\n");
		return ret;
	}

	regs.reg_num[0] = 0x31;
	ret = sensor_read(sd, regs.reg_num, regs.value);
	if (ret < 0) {
		csi_dev_err("sensor_read err at sensor_s_vflip!\n");
		return ret;
	}

	switch (value) {
		case 0:
			regs.value[0] &= 0xbf;
			break;
		case 1:
			regs.value[0] |= 0x40;
			break;
		default:
			return -EINVAL;
	}
	ret = sensor_write(sd, regs.reg_num, regs.value);
	if (ret < 0) {
		csi_dev_err("sensor_write err at sensor_s_vflip!\n");
		return ret;
	}

	mdelay(100);

	info->vflip = value;
	return 0;
}

static int sensor_g_autogain(struct v4l2_subdev *sd, __s32 *value)
{
	return -EINVAL;
}

static int sensor_s_autogain(struct v4l2_subdev *sd, int value)
{
	return -EINVAL;
}

static int sensor_g_autoexp(struct v4l2_subdev *sd, __s32 *value)
{
	int ret;
	struct sensor_info *info = to_state(sd);
	struct regval_list regs;

	regs.reg_num[0] = 0xfd;
	regs.value[0] = 0x00;		//page 0
	ret = sensor_write(sd, regs.reg_num, regs.value);
	if (ret < 0) {
		csi_dev_err("sensor_write err at sensor_g_autoexp!\n");
		return ret;
	}

	regs.reg_num[0] = 0x32;
	ret = sensor_read(sd, regs.reg_num, regs.value);
	if (ret < 0) {
		csi_dev_err("sensor_read err at sensor_g_autoexp!\n");
		return ret;
	}

	regs.value[0] &= 0x01;
	if (regs.value[0] == 0x01) {
		*value = V4L2_EXPOSURE_AUTO;
	}
	else
	{
		*value = V4L2_EXPOSURE_MANUAL;
	}

	info->autoexp = *value;
	return 0;
}

static int sensor_s_autoexp(struct v4l2_subdev *sd,
		enum v4l2_exposure_auto_type value)
{
	int ret;
	struct sensor_info *info = to_state(sd);
	struct regval_list regs;

	regs.reg_num[0] = 0xfd;
	regs.value[0] = 0x00;		//page 0
	ret = sensor_write(sd, regs.reg_num, regs.value);
	if (ret < 0) {
		csi_dev_err("sensor_write err at sensor_s_autoexp!\n");
		return ret;
	}

	regs.reg_num[0] = 0x32;
	ret = sensor_read(sd, regs.reg_num, regs.value);
	if (ret < 0) {
		csi_dev_err("sensor_read err at sensor_s_autoexp!\n");
		return ret;
	}

	switch (value) {
		case V4L2_EXPOSURE_AUTO:
			regs.value[0] |= 0x01;
			break;
		case V4L2_EXPOSURE_MANUAL:
			regs.value[0] &= 0xfe;
			break;
		case V4L2_EXPOSURE_SHUTTER_PRIORITY:
			return -EINVAL;
		case V4L2_EXPOSURE_APERTURE_PRIORITY:
			return -EINVAL;
		default:
			return -EINVAL;
	}

	ret = sensor_write(sd, regs.reg_num, regs.value);
	if (ret < 0) {
		csi_dev_err("sensor_write err at sensor_s_autoexp!\n");
		return ret;
	}

	mdelay(10);

	info->autoexp = value;
	return 0;
}

static int sensor_g_autowb(struct v4l2_subdev *sd, int *value)
{
	int ret;
	struct sensor_info *info = to_state(sd);
	struct regval_list regs;

	regs.reg_num[0] = 0xfd;
	regs.value[0] = 0x00;		//page 0
	ret = sensor_write(sd, regs.reg_num, regs.value);
	if (ret < 0) {
		csi_dev_err("sensor_write err at sensor_g_autowb!\n");
		return ret;
	}

	regs.reg_num[0] = 0x32;
	ret = sensor_read(sd, regs.reg_num, regs.value);
	if (ret < 0) {
		csi_dev_err("sensor_read err at sensor_g_autowb!\n");
		return ret;
	}

	regs.value[0] &= (1<<4);
	regs.value[0] = regs.value[0]>>4;		//0x32 bit4 is awb enable

	*value = regs.value[0];
	info->autowb = *value;

	return 0;
}

static int sensor_s_autowb(struct v4l2_subdev *sd, int value)
{
	int ret;
	struct sensor_info *info = to_state(sd);
	struct regval_list regs;

	ret = sensor_write_array(sd, sensor_wb_auto_regs, ARRAY_SIZE(sensor_wb_auto_regs));
	if (ret < 0) {
		csi_dev_err("sensor_write_array err at sensor_s_autowb!\n");
		return ret;
	}

	regs.reg_num[0] = 0xfd;
	regs.value[0] = 0x00;		//page 0
	ret = sensor_write(sd, regs.reg_num, regs.value);
	if (ret < 0) {
		csi_dev_err("sensor_write err at sensor_s_autowb!\n");
		return ret;
	}

	regs.reg_num[0] = 0x32;
	ret = sensor_read(sd, regs.reg_num, regs.value);
	if (ret < 0) {
		csi_dev_err("sensor_read err at sensor_s_autowb!\n");
		return ret;
	}

	switch(value) {
		case 0:
			regs.value[0] &= 0xef;
			break;
		case 1:
			regs.value[0] |= 0x10;
			break;
		default:
			break;
	}
	ret = sensor_write(sd, regs.reg_num, regs.value);
	if (ret < 0) {
		csi_dev_err("sensor_write err at sensor_s_autowb!\n");
		return ret;
	}

	mdelay(10);

	info->autowb = value;
	return 0;
}

static int sensor_g_hue(struct v4l2_subdev *sd, __s32 *value)
{
	return -EINVAL;
}

static int sensor_s_hue(struct v4l2_subdev *sd, int value)
{
	return -EINVAL;
}

static int sensor_g_gain(struct v4l2_subdev *sd, __s32 *value)
{
	return -EINVAL;
}

static int sensor_s_gain(struct v4l2_subdev *sd, int value)
{
	return -EINVAL;
}

static int sensor_g_brightness(struct v4l2_subdev *sd, __s32 *value)
{
	struct sensor_info *info = to_state(sd);

	*value = info->brightness;
	return 0;
}

static int sensor_s_brightness(struct v4l2_subdev *sd, int value)
{
	int ret;
	struct sensor_info *info = to_state(sd);

	switch (value) {
		case -4:
			ret = sensor_write_array(sd, sensor_brightness_neg4_regs, ARRAY_SIZE(sensor_brightness_neg4_regs));
			break;
		case -3:
			ret = sensor_write_array(sd, sensor_brightness_neg3_regs, ARRAY_SIZE(sensor_brightness_neg3_regs));
			break;
		case -2:
			ret = sensor_write_array(sd, sensor_brightness_neg2_regs, ARRAY_SIZE(sensor_brightness_neg2_regs));
			break;
		case -1:
			ret = sensor_write_array(sd, sensor_brightness_neg1_regs, ARRAY_SIZE(sensor_brightness_neg1_regs));
			break;
		case 0:
			ret = sensor_write_array(sd, sensor_brightness_zero_regs, ARRAY_SIZE(sensor_brightness_zero_regs));
			break;
		case 1:
			ret = sensor_write_array(sd, sensor_brightness_pos1_regs, ARRAY_SIZE(sensor_brightness_pos1_regs));
			break;
		case 2:
			ret = sensor_write_array(sd, sensor_brightness_pos2_regs, ARRAY_SIZE(sensor_brightness_pos2_regs));
			break;
		case 3:
			ret = sensor_write_array(sd, sensor_brightness_pos3_regs, ARRAY_SIZE(sensor_brightness_pos3_regs));
			break;
		case 4:
			ret = sensor_write_array(sd, sensor_brightness_pos4_regs, ARRAY_SIZE(sensor_brightness_pos4_regs));
			break;
		default:
			return -EINVAL;
	}

	if (ret < 0) {
		csi_dev_err("sensor_write_array err at sensor_s_brightness!\n");
		return ret;
	}

	mdelay(10);

	info->brightness = value;
	return 0;
}

static int sensor_g_contrast(struct v4l2_subdev *sd, __s32 *value)
{
	struct sensor_info *info = to_state(sd);

	*value = info->contrast;
	return 0;
}

static int sensor_s_contrast(struct v4l2_subdev *sd, int value)
{
	int ret;
	struct sensor_info *info = to_state(sd);

	switch (value) {
		case -4:
			ret = sensor_write_array(sd, sensor_contrast_neg4_regs, ARRAY_SIZE(sensor_contrast_neg4_regs));
			break;
		case -3:
			ret = sensor_write_array(sd, sensor_contrast_neg3_regs, ARRAY_SIZE(sensor_contrast_neg3_regs));
			break;
		case -2:
			ret = sensor_write_array(sd, sensor_contrast_neg2_regs, ARRAY_SIZE(sensor_contrast_neg2_regs));
			break;
		case -1:
			ret = sensor_write_array(sd, sensor_contrast_neg1_regs, ARRAY_SIZE(sensor_contrast_neg1_regs));
			break;
		case 0:
			ret = sensor_write_array(sd, sensor_contrast_zero_regs, ARRAY_SIZE(sensor_contrast_zero_regs));
			break;
		case 1:
			ret = sensor_write_array(sd, sensor_contrast_pos1_regs, ARRAY_SIZE(sensor_contrast_pos1_regs));
			break;
		case 2:
			ret = sensor_write_array(sd, sensor_contrast_pos2_regs, ARRAY_SIZE(sensor_contrast_pos2_regs));
			break;
		case 3:
			ret = sensor_write_array(sd, sensor_contrast_pos3_regs, ARRAY_SIZE(sensor_contrast_pos3_regs));
			break;
		case 4:
			ret = sensor_write_array(sd, sensor_contrast_pos4_regs, ARRAY_SIZE(sensor_contrast_pos4_regs));
			break;
		default:
			return -EINVAL;
	}

	if (ret < 0) {
		csi_dev_err("sensor_write_array err at sensor_s_contrast!\n");
		return ret;
	}

	mdelay(10);

	info->contrast = value;
	return 0;
}

static int sensor_g_saturation(struct v4l2_subdev *sd, __s32 *value)
{
	struct sensor_info *info = to_state(sd);

	*value = info->saturation;
	return 0;
}

static int sensor_s_saturation(struct v4l2_subdev *sd, int value)
{
	int ret;
	struct sensor_info *info = to_state(sd);

	switch (value) {
		case -4:
			ret = sensor_write_array(sd, sensor_saturation_neg4_regs, ARRAY_SIZE(sensor_saturation_neg4_regs));
			break;
		case -3:
			ret = sensor_write_array(sd, sensor_saturation_neg3_regs, ARRAY_SIZE(sensor_saturation_neg3_regs));
			break;
		case -2:
			ret = sensor_write_array(sd, sensor_saturation_neg2_regs, ARRAY_SIZE(sensor_saturation_neg2_regs));
			break;
		case -1:
			ret = sensor_write_array(sd, sensor_saturation_neg1_regs, ARRAY_SIZE(sensor_saturation_neg1_regs));
			break;
		case 0:
			ret = sensor_write_array(sd, sensor_saturation_zero_regs, ARRAY_SIZE(sensor_saturation_zero_regs));
			break;
		case 1:
			ret = sensor_write_array(sd, sensor_saturation_pos1_regs, ARRAY_SIZE(sensor_saturation_pos1_regs));
			break;
		case 2:
			ret = sensor_write_array(sd, sensor_saturation_pos2_regs, ARRAY_SIZE(sensor_saturation_pos2_regs));
			break;
		case 3:
			ret = sensor_write_array(sd, sensor_saturation_pos3_regs, ARRAY_SIZE(sensor_saturation_pos3_regs));
			break;
		case 4:
			ret = sensor_write_array(sd, sensor_saturation_pos4_regs, ARRAY_SIZE(sensor_saturation_pos4_regs));
			break;
		default:
			return -EINVAL;
	}

	if (ret < 0) {
		csi_dev_err("sensor_write_array err at sensor_s_saturation!\n");
		return ret;
	}

	mdelay(10);

	info->saturation = value;
	return 0;
}

static int sensor_g_exp(struct v4l2_subdev *sd, __s32 *value)
{
	struct sensor_info *info = to_state(sd);

	*value = info->exp;
	return 0;
}

static int sensor_s_exp(struct v4l2_subdev *sd, int value)
{
	int ret;
	struct sensor_info *info = to_state(sd);

	switch (value) {
		case -4:
			ret = sensor_write_array(sd, sensor_ev_neg4_regs, ARRAY_SIZE(sensor_ev_neg4_regs));
			break;
		case -3:
			ret = sensor_write_array(sd, sensor_ev_neg3_regs, ARRAY_SIZE(sensor_ev_neg3_regs));
			break;
		case -2:
			ret = sensor_write_array(sd, sensor_ev_neg2_regs, ARRAY_SIZE(sensor_ev_neg2_regs));
			break;
		case -1:
			ret = sensor_write_array(sd, sensor_ev_neg1_regs, ARRAY_SIZE(sensor_ev_neg1_regs));
			break;
		case 0:
			ret = sensor_write_array(sd, sensor_ev_zero_regs, ARRAY_SIZE(sensor_ev_zero_regs));
			break;
		case 1:
			ret = sensor_write_array(sd, sensor_ev_pos1_regs, ARRAY_SIZE(sensor_ev_pos1_regs));
			break;
		case 2:
			ret = sensor_write_array(sd, sensor_ev_pos2_regs, ARRAY_SIZE(sensor_ev_pos2_regs));
			break;
		case 3:
			ret = sensor_write_array(sd, sensor_ev_pos3_regs, ARRAY_SIZE(sensor_ev_pos3_regs));
			break;
		case 4:
			ret = sensor_write_array(sd, sensor_ev_pos4_regs, ARRAY_SIZE(sensor_ev_pos4_regs));
			break;
		default:
			return -EINVAL;
	}

	if (ret < 0) {
		csi_dev_err("sensor_write_array err at sensor_s_exp!\n");
		return ret;
	}

	mdelay(10);

	info->exp = value;
	return 0;
}

static int sensor_g_wb(struct v4l2_subdev *sd, int *value)
{
	struct sensor_info *info = to_state(sd);
	enum v4l2_whiteblance *wb_type = (enum v4l2_whiteblance*)value;

	*wb_type = info->wb;

	return 0;
}

static int sensor_s_wb(struct v4l2_subdev *sd,
		enum v4l2_whiteblance value)
{
	int ret;
	struct sensor_info *info = to_state(sd);

	if (value == V4L2_WB_AUTO) {
		ret = sensor_s_autowb(sd, 1);
		return ret;
		ret = sensor_write_array(sd, sensor_wb_auto_regs, ARRAY_SIZE(sensor_wb_auto_regs));
		info->autowb = 1;
	}
	else {
		ret = sensor_s_autowb(sd, 0);
		if(ret < 0) {
			csi_dev_err("sensor_s_autowb error, return %x!\n",ret);
			return ret;
		}
		switch (value) {
			case V4L2_WB_CLOUD:
				ret = sensor_write_array(sd, sensor_wb_cloud_regs, ARRAY_SIZE(sensor_wb_cloud_regs));
				break;
			case V4L2_WB_DAYLIGHT:
				ret = sensor_write_array(sd, sensor_wb_daylight_regs, ARRAY_SIZE(sensor_wb_daylight_regs));
				break;
			case V4L2_WB_INCANDESCENCE:
				ret = sensor_write_array(sd, sensor_wb_incandescence_regs, ARRAY_SIZE(sensor_wb_incandescence_regs));
				break;
			case V4L2_WB_FLUORESCENT:
				ret = sensor_write_array(sd, sensor_wb_fluorescent_regs, ARRAY_SIZE(sensor_wb_fluorescent_regs));
				break;
			case V4L2_WB_TUNGSTEN:
				ret = sensor_write_array(sd, sensor_wb_tungsten_regs, ARRAY_SIZE(sensor_wb_tungsten_regs));
				break;
			default:
				return -EINVAL;
		}
		info->autowb = 0;
	}

	if (ret < 0) {
		csi_dev_err("sensor_s_wb error, return %x!\n",ret);
		return ret;
	}

	mdelay(10);

	info->wb = value;
	return 0;
}

static int sensor_g_colorfx(struct v4l2_subdev *sd,
		__s32 *value)
{
	struct sensor_info *info = to_state(sd);
	enum v4l2_colorfx *clrfx_type = (enum v4l2_colorfx*)value;

	*clrfx_type = info->clrfx;
	return 0;
}

static int sensor_s_colorfx(struct v4l2_subdev *sd,
		enum v4l2_colorfx value)
{
	int ret;
	struct sensor_info *info = to_state(sd);

	switch (value) {
		case V4L2_COLORFX_NONE:
			ret = sensor_write_array(sd, sensor_colorfx_none_regs, ARRAY_SIZE(sensor_colorfx_none_regs));
			break;
		case V4L2_COLORFX_BW:
			ret = sensor_write_array(sd, sensor_colorfx_bw_regs, ARRAY_SIZE(sensor_colorfx_bw_regs));
			break;
		case V4L2_COLORFX_SEPIA:
			ret = sensor_write_array(sd, sensor_colorfx_sepia_regs, ARRAY_SIZE(sensor_colorfx_sepia_regs));
			break;
		case V4L2_COLORFX_NEGATIVE:
			ret = sensor_write_array(sd, sensor_colorfx_negative_regs, ARRAY_SIZE(sensor_colorfx_negative_regs));
			break;
		case V4L2_COLORFX_EMBOSS:
			ret = sensor_write_array(sd, sensor_colorfx_emboss_regs, ARRAY_SIZE(sensor_colorfx_emboss_regs));
			break;
		case V4L2_COLORFX_SKETCH:
			ret = sensor_write_array(sd, sensor_colorfx_sketch_regs, ARRAY_SIZE(sensor_colorfx_sketch_regs));
			break;
		case V4L2_COLORFX_SKY_BLUE:
			ret = sensor_write_array(sd, sensor_colorfx_sky_blue_regs, ARRAY_SIZE(sensor_colorfx_sky_blue_regs));
			break;
		case V4L2_COLORFX_GRASS_GREEN:
			ret = sensor_write_array(sd, sensor_colorfx_grass_green_regs, ARRAY_SIZE(sensor_colorfx_grass_green_regs));
			break;
		case V4L2_COLORFX_SKIN_WHITEN:
			ret = sensor_write_array(sd, sensor_colorfx_skin_whiten_regs, ARRAY_SIZE(sensor_colorfx_skin_whiten_regs));
			break;
		case V4L2_COLORFX_VIVID:
			ret = sensor_write_array(sd, sensor_colorfx_vivid_regs, ARRAY_SIZE(sensor_colorfx_vivid_regs));
			break;
		default:
			return -EINVAL;
	}

	if (ret < 0) {
		csi_dev_err("sensor_s_colorfx error, return %x!\n",ret);
		return ret;
	}

	mdelay(10);

	info->clrfx = value;
	return 0;
}

static int sensor_g_ctrl(struct v4l2_ctrl *ctrl)
{
	struct sensor_info *sensor = container_of(ctrl->handler, struct sensor_info, hdl);
	struct v4l2_subdev *sd = &sensor->sd;

	switch (ctrl->id) {
		case V4L2_CID_BRIGHTNESS:
			return sensor_g_brightness(sd, &ctrl->val);
		case V4L2_CID_CONTRAST:
			return sensor_g_contrast(sd, &ctrl->val);
		case V4L2_CID_SATURATION:
			return sensor_g_saturation(sd, &ctrl->val);
		case V4L2_CID_HUE:
			return sensor_g_hue(sd, &ctrl->val);
		case V4L2_CID_VFLIP:
			return sensor_g_vflip(sd, &ctrl->val);
		case V4L2_CID_HFLIP:
			return sensor_g_hflip(sd, &ctrl->val);
		case V4L2_CID_GAIN:
			return sensor_g_gain(sd, &ctrl->val);
		case V4L2_CID_AUTOGAIN:
			return sensor_g_autogain(sd, &ctrl->val);
		case V4L2_CID_EXPOSURE:
			return sensor_g_exp(sd, &ctrl->val);
		case V4L2_CID_EXPOSURE_AUTO:
			return sensor_g_autoexp(sd, &ctrl->val);
		case V4L2_CID_DO_WHITE_BALANCE:
			return sensor_g_wb(sd, &ctrl->val);
		case V4L2_CID_AUTO_WHITE_BALANCE:
			return sensor_g_autowb(sd, &ctrl->val);
		case V4L2_CID_COLORFX:
			return sensor_g_colorfx(sd, &ctrl->val);
	}
	return -EINVAL;
}

static int sensor_s_ctrl(struct v4l2_ctrl *ctrl)
{
	struct sensor_info *sensor = container_of(ctrl->handler, struct sensor_info, hdl);
	struct v4l2_subdev *sd = &sensor->sd;
	switch (ctrl->id) {
		case V4L2_CID_BRIGHTNESS:
			return sensor_s_brightness(sd, ctrl->val);
		case V4L2_CID_CONTRAST:
			return sensor_s_contrast(sd, ctrl->val);
		case V4L2_CID_SATURATION:
			return sensor_s_saturation(sd, ctrl->val);
		case V4L2_CID_HUE:
			return sensor_s_hue(sd, ctrl->val);
		case V4L2_CID_VFLIP:
			return sensor_s_vflip(sd, ctrl->val);
		case V4L2_CID_HFLIP:
			return sensor_s_hflip(sd, ctrl->val);
		case V4L2_CID_GAIN:
			return sensor_s_gain(sd, ctrl->val);
		case V4L2_CID_AUTOGAIN:
			return sensor_s_autogain(sd, ctrl->val);
		case V4L2_CID_EXPOSURE:
			return sensor_s_exp(sd, ctrl->val);
		case V4L2_CID_EXPOSURE_AUTO:
			return sensor_s_autoexp(sd,
					(enum v4l2_exposure_auto_type) ctrl->val);
		case V4L2_CID_DO_WHITE_BALANCE:
			return sensor_s_wb(sd,
					(enum v4l2_whiteblance) ctrl->val);
		case V4L2_CID_AUTO_WHITE_BALANCE:
			return sensor_s_autowb(sd, ctrl->val);
		case V4L2_CID_COLORFX:
			return sensor_s_colorfx(sd,
					(enum v4l2_colorfx) ctrl->val);
	}
	return -EINVAL;
}

static int sensor_g_chip_ident(struct v4l2_subdev *sd,
		struct v4l2_dbg_chip_ident *chip)
{
	struct i2c_client *client = v4l2_get_subdevdata(sd);

	return v4l2_chip_ident_i2c_client(client, chip, V4L2_IDENT_SENSOR, 0);
}

static int sensor_g_mbus_config(struct v4l2_subdev *sd,
		struct v4l2_mbus_config *cfg)
{

	struct i2c_client *client = v4l2_get_subdevdata(sd);
	struct soc_camera_subdev_desc *ssdd = soc_camera_i2c_to_desc(client);

	cfg->flags = V4L2_MBUS_PCLK_SAMPLE_RISING | V4L2_MBUS_MASTER |
		V4L2_MBUS_VSYNC_ACTIVE_HIGH | V4L2_MBUS_HSYNC_ACTIVE_HIGH |
		V4L2_MBUS_DATA_ACTIVE_HIGH;
	cfg->type = V4L2_MBUS_PARALLEL;

	cfg->flags = soc_camera_apply_board_flags(ssdd, cfg);

	return 0;
}

static const struct v4l2_ctrl_ops sensor_ctrl_ops = {
	.g_volatile_ctrl = sensor_g_ctrl,
	.s_ctrl = sensor_s_ctrl,
};

static const struct v4l2_subdev_core_ops sensor_core_ops = {
	.s_power = sensor_power,
	.g_chip_ident = sensor_g_chip_ident,
//	.queryctrl = sensor_queryctrl,
	.init = sensor_init,
};

static const struct v4l2_subdev_video_ops sensor_video_ops = {
	.enum_mbus_fmt = sensor_enum_fmt,
	.try_mbus_fmt = sensor_try_fmt,
	.s_mbus_fmt = sensor_s_fmt,
	.g_mbus_config  = sensor_g_mbus_config,
	.s_parm = sensor_s_parm,
	.g_parm = sensor_g_parm,
};

static const struct v4l2_subdev_ops sensor_ops = {
	.core = &sensor_core_ops,
	.video = &sensor_video_ops,
};

static int sensor_probe(struct i2c_client *client,
		const struct i2c_device_id *id)
{
	struct v4l2_subdev *sd;
	struct sensor_info *info;

	info = kzalloc(sizeof(struct sensor_info), GFP_KERNEL);
	if (info == NULL)
		return -ENOMEM;
	sd = &info->sd;
	v4l2_i2c_subdev_init(sd, client, &sensor_ops);

	v4l2_ctrl_handler_init(&info->hdl, 10);
	v4l2_ctrl_new_std(&info->hdl, &sensor_ctrl_ops,
			V4L2_CID_VFLIP, 0, 1, 1, 0);
	v4l2_ctrl_new_std(&info->hdl, &sensor_ctrl_ops,
			V4L2_CID_HFLIP, 0, 1, 1, 0);
	v4l2_ctrl_new_std(&info->hdl, &sensor_ctrl_ops,
			V4L2_CID_EXPOSURE, -4, 4, 1, 0);
	v4l2_ctrl_new_std(&info->hdl, &sensor_ctrl_ops,
			V4L2_CID_DO_WHITE_BALANCE, 0, 5, 1, 0);
	v4l2_ctrl_new_std(&info->hdl, &sensor_ctrl_ops,
			V4L2_CID_AUTO_WHITE_BALANCE, 0, 1, 1, 1);
	info->sd.ctrl_handler = &info->hdl;

	if (info->hdl.error) {
		return info->hdl.error;
	}

	info->fmt = &sensor_formats[0];

	info->brightness = 0;
	info->contrast = 0;
	info->saturation = 0;
	info->hue = 0;
	info->hflip = 0;
	info->vflip = 0;
	info->gain = 0;
	info->autogain = 1;
	info->exp = 0;
	info->autoexp = 0;
	info->autowb = 1;
	info->wb = 0;
	info->clrfx = 0;

	return 0;
}


static int sensor_remove(struct i2c_client *client)
{
	struct v4l2_subdev *sd = i2c_get_clientdata(client);

	v4l2_device_unregister_subdev(sd);
	kfree(to_state(sd));
	return 0;
}

static const struct i2c_device_id sensor_id[] = {
	{ "sp0718_back", 0 },
	{ }
};
MODULE_DEVICE_TABLE(i2c, sensor_id);

static struct i2c_driver sensor_driver = {
	.driver = {
		.owner = THIS_MODULE,
		.name = "sp0718_back",
	},
	.probe = sensor_probe,
	.remove = sensor_remove,
	.id_table = sensor_id,
};

static __init int init_sensor(void)
{
	return i2c_add_driver(&sensor_driver);
}

static __exit void exit_sensor(void)
{
	i2c_del_driver(&sensor_driver);
}

module_init(init_sensor);
module_exit(exit_sensor);

MODULE_DESCRIPTION("camera sensor sp0817 driver");
MODULE_LICENSE("GPL");
