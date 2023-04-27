#include <asm/barrier.h>

static inline unsigned int *add_off(unsigned int *base, int offset)
{
	return (unsigned int *) ((char *)base + offset);
}

static inline unsigned int *del_off(unsigned int *base, int offset)
{
	return (unsigned int *) ((char *)base - offset);
}

void rotate_90_1(unsigned int *dst, unsigned int *src, int xres, int yres, int src_stride, int dst_stride)
{
	int i, j, src_del, dst_del;
	src_del = src_stride - xres * 4;
	dst_del = 0 - (dst_stride * xres + 4);
	dst = dst + yres - 1;

	for (j = 0; j < yres; j++) {
		for (i = 0; i < xres; i++) {
			*dst = *src++;
			dst = add_off(dst, dst_stride);
		}
		src = add_off(src, src_del);
		dst = add_off(dst, dst_del);
	}
}

void rotate_90_2_8(unsigned int *dst, unsigned int *src, int xres, int yres, int src_stride, int dst_stride)
{
	int i, j, src_del, dst_del;
	src_del = src_stride - xres * 4;
	dst_del = 0 - (dst_stride * xres + 8 * 4);
	dst = dst + yres - 8;
	unsigned int *src1 = add_off(src, src_stride);
	unsigned int *src2 = add_off(src1, src_stride);
	unsigned int *src3 = add_off(src2, src_stride);
	unsigned int *src4 = add_off(src3, src_stride);
	unsigned int *src5 = add_off(src4, src_stride);
	unsigned int *src6 = add_off(src5, src_stride);
	unsigned int *src7 = add_off(src6, src_stride);

	for (j = 0; j < yres / 8; j++) {
		for (i = 0; i < xres / 2; i++) {
			register unsigned int a0 = *src++;
			register unsigned int a1 = *src++;
			register unsigned int b0 = *src1++;
			register unsigned int b1 = *src1++;
			register unsigned int c0 = *src2++;
			register unsigned int c1 = *src2++;
			register unsigned int d0 = *src3++;
			register unsigned int d1 = *src3++;
			register unsigned int e0 = *src4++;
			register unsigned int e1 = *src4++;
			register unsigned int f0 = *src5++;
			register unsigned int f1 = *src5++;
			register unsigned int g0 = *src6++;
			register unsigned int g1 = *src6++;
			register unsigned int k0 = *src7++;
			register unsigned int k1 = *src7++;

			dst[0] = k0;
			dst[1] = g0;
			dst[2] = f0;
			dst[3] = e0;
			dst[4] = d0;
			dst[5] = c0;
			dst[6] = b0;
			dst[7] = a0;
			dst = add_off(dst, dst_stride);
			dst[0] = k1;
			dst[1] = g1;
			dst[2] = f1;
			dst[3] = e1;
			dst[4] = d1;
			dst[5] = c1;
			dst[6] = b1;
			dst[7] = a1;
			dst = add_off(dst, dst_stride);
		}
		src = add_off(src7, src_del);
		src1 = add_off(src, src_stride);
		src2 = add_off(src1, src_stride);
		src3 = add_off(src2, src_stride);
		src4 = add_off(src3, src_stride);
		src5 = add_off(src4, src_stride);
		src6 = add_off(src5, src_stride);
		src7 = add_off(src6, src_stride);
		dst = add_off(dst, dst_del);
	}
}

void rotate_90(unsigned int *dst, unsigned int *src, int xres, int yres, int src_stride, int dst_stride)
{
	int xdel = xres % 2;
	int ydel = yres % 8;
	int x0 = xres - xdel;
	int y0 = yres - ydel;

	rotate_90_2_8(dst + ydel, src, x0, y0, src_stride, dst_stride);
	if (xdel)
		rotate_90_1(add_off(dst, x0 * dst_stride), src + x0, xdel, yres, src_stride, dst_stride);
	if (ydel)
		rotate_90_1(dst, add_off(src, y0 * src_stride), x0, ydel, src_stride, dst_stride);
}

void rotate_270_1(unsigned int *dst, unsigned int *src, int xres, int yres, int src_stride, int dst_stride)
{
	int i, j, src_del, dst_del;
	src_del = src_stride - xres * 4;
	dst_del = dst_stride * xres + 4;
	dst = add_off(dst, (xres - 1) * dst_stride);

	for (j = 0; j < yres; j++) {
		for (i = 0; i < xres; i++) {
			*dst = *src++;
			dst = del_off(dst , dst_stride);
		}
		src = add_off(src, src_del);
		dst = add_off(dst, dst_del);
	}
}

void rotate_270_2_8(unsigned int *dst, unsigned int *src, int xres, int yres, int src_stride, int dst_stride)
{
	int i, j, src_del, dst_del;
	src_del = src_stride - xres * 4;
	dst_del = dst_stride * xres + 8 * 4;
	dst = add_off(dst, (xres - 1) * dst_stride);
	unsigned int *src1 = add_off(src, src_stride);
	unsigned int *src2 = add_off(src1, src_stride);
	unsigned int *src3 = add_off(src2, src_stride);
	unsigned int *src4 = add_off(src3, src_stride);
	unsigned int *src5 = add_off(src4, src_stride);
	unsigned int *src6 = add_off(src5, src_stride);
	unsigned int *src7 = add_off(src6, src_stride);

	for (j = 0; j < yres / 8; j++) {
		for (i = 0; i < xres / 2; i++) {
			register unsigned int a0 = *src++;
			register unsigned int a1 = *src++;
			register unsigned int b0 = *src1++;
			register unsigned int b1 = *src1++;
			register unsigned int c0 = *src2++;
			register unsigned int c1 = *src2++;
			register unsigned int d0 = *src3++;
			register unsigned int d1 = *src3++;
			register unsigned int e0 = *src4++;
			register unsigned int e1 = *src4++;
			register unsigned int f0 = *src5++;
			register unsigned int f1 = *src5++;
			register unsigned int g0 = *src6++;
			register unsigned int g1 = *src6++;
			register unsigned int k0 = *src7++;
			register unsigned int k1 = *src7++;

			dst[7] = k0;
			dst[6] = g0;
			dst[5] = f0;
			dst[4] = e0;
			dst[3] = d0;
			dst[2] = c0;
			dst[1] = b0;
			dst[0] = a0;
			dst = del_off(dst , dst_stride);
			dst[7] = k1;
			dst[6] = g1;
			dst[5] = f1;
			dst[4] = e1;
			dst[3] = d1;
			dst[2] = c1;
			dst[1] = b1;
			dst[0] = a1;
			dst = del_off(dst , dst_stride);
		}
		src = add_off(src7, src_del);
		src1 = add_off(src, src_stride);
		src2 = add_off(src1, src_stride);
		src3 = add_off(src2, src_stride);
		src4 = add_off(src3, src_stride);
		src5 = add_off(src4, src_stride);
		src6 = add_off(src5, src_stride);
		src7 = add_off(src6, src_stride);
		dst = add_off(dst, dst_del);
	}
}

void rotate_270(unsigned int *dst, unsigned int *src, int xres, int yres, int src_stride, int dst_stride)
{
	int xdel = xres % 2;
	int ydel = yres % 8;
	int x0 = xres - xdel;
	int y0 = yres - ydel;

	rotate_270_2_8(add_off(dst, xdel * dst_stride), src, x0, y0, src_stride, dst_stride);
	if (xdel)
		rotate_270_1(dst, src + x0, xdel, yres, src_stride, dst_stride);
	if (ydel)
		rotate_270_1(add_off(dst, xdel * dst_stride + y0 * 4), add_off(src, y0 * src_stride), x0, ydel, src_stride, dst_stride);
}

void rotate_180_8(unsigned int *dst, unsigned int *src, int xres, int yres, int src_stride, int dst_stride)
{
	int i, j, src_del, dst_del;
	src_del = src_stride - xres * 4;
	dst_del = 0 - (dst_stride - xres * 4);
	dst = add_off(dst, (yres - 1) * dst_stride + (xres - 8) * 4);

	for (j = 0; j < yres; j++) {
		for (i = 0; i < xres / 8; i++) {
			register unsigned a0 = *src++;
			register unsigned a1 = *src++;
			register unsigned a2 = *src++;
			register unsigned a3 = *src++;
			register unsigned a4 = *src++;
			register unsigned a5 = *src++;
			register unsigned a6 = *src++;
			register unsigned a7 = *src++;

			dst[7] = a0;
			dst[6] = a1;
			dst[5] = a2;
			dst[4] = a3;
			dst[3] = a4;
			dst[2] = a5;
			dst[1] = a6;
			dst[0] = a7;
			dst = dst - 8;
		}
		src = add_off(src, src_del);
		dst = add_off(dst, dst_del);
	}
}

void rotate_180_1(unsigned int *dst, unsigned int *src, int xres, int yres, int src_stride, int dst_stride)
{
	int i, j, src_del, dst_del;
	src_del = src_stride - xres * 4;
	dst_del = 0 - (dst_stride - xres * 4);
	dst = add_off(dst, (yres - 1) * dst_stride + (xres - 1) * 4);

	for (j = 0; j < yres; j++) {
		for (i = 0; i < xres; i++) {
			*dst-- = *src++;
		}
		src = add_off(src, src_del);
		dst = add_off(dst, dst_del);
	}
}

void rotate_180(unsigned int *dst, unsigned int *src, int xres, int yres, int src_stride, int dst_stride)
{
	int xdel = xres % 8;
	int x0 = xres - xdel;
	
	rotate_180_8(dst + xdel, src, x0, yres, src_stride, dst_stride);
	if (xdel)
		rotate_180_1(dst, src + x0, xdel, yres, src_stride, dst_stride);
}