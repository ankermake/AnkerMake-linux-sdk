#include <video/ingenic_logo.h>

#if defined(CONFIG_FB_JZ_V12)
  #include "../jz_fb_v12/jz_fb.h"
#elif defined(CONFIG_FB_JZ_V13)
  #include "../jz_fb_v13/jz_fb.h"
#endif

void show_logo(struct jzfb *jzfb)
{
	unsigned int i, j;
	int w, h, bpp;
	int blank_left, blank_top;
	unsigned short *p16;
	unsigned int *p32;
	unsigned int *p_logo;

	w = jzfb->osd.fg0.w;
	h = jzfb->osd.fg0.h;
	bpp = jzfb->osd.fg0.bpp;

	if (!jzfb->vidmem)
		jzfb->vidmem = (void *)phys_to_virt(jzfb->vidmem_phys);

	p16 = (unsigned short *)jzfb->vidmem;

	if (logo_info.bpp == bpp) {
		blank_left = (jzfb->osd.fg0.w - logo_info.width) / 2;
		blank_top = (jzfb->osd.fg0.h - logo_info.height) / 2;
		printk("\033[31m jzfb->osd.fg0.w = %d | logo_info.width = %d |\033[0m\n", jzfb->osd.fg0.w, logo_info.width);
		printk("\033[31m blank_left = %d | blank_top = %d |\033[0m\n", blank_left, blank_top);
		if (bpp == 32) {
			if (blank_top) {
				p32 = (unsigned int *)jzfb->vidmem;
				for (i = 0; i < blank_top * jzfb->osd.fg0.w; ++i) {
					*(p32 + i) = logo_info.background_color;
				}
				p32 = (unsigned int *)jzfb->vidmem + (blank_top + logo_info.height) * jzfb->osd.fg0.w;
				for (i = 0; i < blank_top * jzfb->osd.fg0.w; ++i) {
					*(p32 + i) = logo_info.background_color;
				}
			}
			if (blank_left) {
				for (i = 0; i < logo_info.height; ++i) {
					p32 = (unsigned int *)jzfb->vidmem + (blank_top + i) * jzfb->osd.fg0.w;
					for (j = 0; j < blank_left; ++j) {
						*(p32 + j) = logo_info.background_color;
					}
					p32 = (unsigned int *)jzfb->vidmem + (blank_top + i) * jzfb->osd.fg0.w + logo_info.width + blank_left;
					for (j = 0; j < blank_left; ++j) {
						*(p32 + j) = logo_info.background_color;
					}
				}
			}
			for (i = 0; i < logo_info.height; ++i) {
				p32 = (unsigned int *)jzfb->vidmem + (blank_top + i) * jzfb->osd.fg0.w + blank_left;
				p_logo = (unsigned int*)logo_info.p8 + i * logo_info.width;
				for (j = 0; j < logo_info.width; ++j) {
					*(p32 + j) = *(p_logo + j);
				}
			}
		} else if (bpp == 16) {
			unsigned int background_color = logo_info.background_color << 16 | logo_info.background_color;

			printk("\033[31m jzfb->osd.fg0.h = %d | blank_top = %d | logo_info.height = %d |\033[0m\n", jzfb->osd.fg0.h, blank_top, logo_info.height);
			if (blank_top) {
				int tmp = blank_top * ((jzfb->osd.fg0.w + 1) / 2);
				p32 = (unsigned int *)jzfb->vidmem;
				for (i = 0; i < tmp; i++)
					*(p32++) = background_color;

				tmp = (jzfb->osd.fg0.h - blank_top - logo_info.height) * ((jzfb->osd.fg0.w + 1) / 2);
				p32 = (unsigned int *)jzfb->vidmem + (blank_top + logo_info.height) * ((jzfb->osd.fg0.w + 1) / 2);
				for (i = 0; i < tmp; i++)
					*(p32++) = background_color;
			}

			if (blank_left) {
				int tmp;
				for (i = 0; i < logo_info.height; ++i) {
					tmp = (blank_left + 1) / 2;
					p32 = (unsigned int *)jzfb->vidmem + (blank_top + i) * ((jzfb->osd.fg0.w + 1) / 2);
					for (j = 0; j < tmp; ++j) {
						*(p32 + j) = background_color;
					}

					p32 = (unsigned int *)jzfb->vidmem + (blank_top + i) * ((jzfb->osd.fg0.w + 1) / 2) + (logo_info.width + blank_left) / 2;
					for (j = 0; j < tmp; ++j) {
						*(p32 + j) = background_color;
					}
				}
			}
			if (logo_info.width%2) {
				for (i = 0; i < logo_info.height; ++i) {
					p32 = (unsigned int *)jzfb->vidmem + (blank_top + i) * ((jzfb->osd.fg0.w + 1) / 2) + blank_left / 2;
					p_logo = (unsigned int*)logo_info.p8 + i * logo_info.width/2;
					for (j = 0; j < logo_info.width/2; ++j) {
						*(p32 + j) = *(p_logo + j);
					}
					*(unsigned short*)(p32 + j) = *(unsigned short*)(p_logo + j);
				}
			} else {
				for (i = 0; i < logo_info.height; ++i) {
					p32 = (unsigned int *)jzfb->vidmem + (blank_top + i) * ((jzfb->osd.fg0.w + 1) / 2) + blank_left / 2;
					p_logo = (unsigned int*)logo_info.p8 + i * logo_info.width/2;
					for (j = 0; j < logo_info.width/2; ++j) {
						*(p32 + j) = *(p_logo + j);
					}
				}
			}
		}
	}
}
