#ifndef __XB47XX_AUDIO_PRE__
#define __XB47XX_AUDIO_PRE__

#define AUDIO_WRITE(n) (*(volatile unsigned int*)(n))
#define audio_write(val,addr) AUDIO_WRITE(addr) = (val)
#define I2SCDR1_PRE 	0xb0000070
#define I2SDIV_PRE		0xb0020030

#endif //__XB47XX_AUDIO_PRE__
