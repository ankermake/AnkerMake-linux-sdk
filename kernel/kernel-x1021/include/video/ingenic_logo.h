#ifndef __LOGO_H__
#define __LOGO_H__

struct _logo_info {
	int width;
	int height;
	int bpp;
	unsigned char *p8;
	unsigned int background_color;
};

extern struct _logo_info logo_info;

#endif /* __LOGO_H__ */
