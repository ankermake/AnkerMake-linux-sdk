#include "ovisp-base.h"
#include "ovisp-isp.h"

#define OVISP_REG_END	0xffff

struct isp_reg_t ov2710_setting[] = {

	{0x65400, 0x01},
	{0x65401, 0x00},
	{0x65402, 0x40},//minlowlevel = 1.5 * blc
	{0x65403, 0x00},
	{0x65404, 0xa0},//maxlowlevel
	{0x65405, 0x0f},
	{0x65406, 0xff},//high level = 255, disable high level
	{0x65409, 0x01},//psthres1
	{0x6540a, 0x02},//psthres2
	{0x6540b, 0x02},//minnum
	{0x6540c, 0x00},
	{0x6540d, 0x01},//target_offset
	{OVISP_REG_END, 0X00},
};

void isp_setting_init(struct isp_device *isp)
{
	int i = 0;
	struct isp_reg_t* isp_setting = ov2710_setting;
	for (i = 0; isp_setting[i].reg != OVISP_REG_END; i++) {
		if( isp_setting[i].reg & 0x10000 )
			isp_firmware_writeb(isp, isp_setting[i].value, isp_setting[i].reg);
		else if( isp_setting[i].reg & 0x60000 )
			isp_reg_writeb(isp, isp_setting[i].value, isp_setting[i].reg);
	}
}

