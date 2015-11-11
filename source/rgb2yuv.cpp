
#include "stdlib.h"
#include "rgb2yuv.h"


static float RGBYUV02990[256], RGBYUV05870[256], RGBYUV01140[256];
static float RGBYUV01684[256], RGBYUV03316[256];
static float RGBYUV04187[256], RGBYUV00813[256];

void InitLookupTable();

/************************************************************************
 *
 *  int RGB2YUV (int x_dim, int y_dim, void *bmp, YUV *yuv)
 *
 *	Purpose :	It takes a 24-bit RGB bitmap and convert it into
 *				YUV (4:2:0) format
 *
 *  Input :		x_dim	the x dimension of the bitmap
 *				y_dim	the y dimension of the bitmap
 *				bmp		pointer to the buffer of the bitmap
 *				yuv		pointer to the YUV structure
 *
 *  Output :	0		OK
 *				1		wrong dimension
 *				2		memory allocation error
 *
 *	Side Effect :
 *				None
 *
 *	Date :		09/28/2000
 *
 *  Contacts:
 *
 *  Adam Li
 *
 *  DivX Advance Research Center <darc@projectmayo.com>
 *
 ************************************************************************/

int RGB2YUV (int x_dim, int y_dim, void *bmp, void *y_out, void *u_out, void *v_out, int flip)
{
	static int init_done = 0;

	long i, j, size;
	unsigned char *r, *g, *b;
	unsigned char *y, *u, *v;
	unsigned char *pu1, *pu2, *pv1, *pv2, *psu, *psv;
	unsigned char *y_buffer, *u_buffer, *v_buffer;
	unsigned char *sub_u_buf, *sub_v_buf;

	if (init_done == 0)
	{
		InitLookupTable();
		init_done = 1;
	}

	// check to see if x_dim and y_dim are divisible by 2
	if ((x_dim % 2) || (y_dim % 2)) return 1;
	size = x_dim * y_dim;

	// allocate memory
	y_buffer = (unsigned char *)y_out;
	sub_u_buf = (unsigned char *)u_out;
	sub_v_buf = (unsigned char *)v_out;
	u_buffer = (unsigned char *)malloc(size * sizeof(unsigned char));
	v_buffer = (unsigned char *)malloc(size * sizeof(unsigned char));
	if (!(u_buffer && v_buffer))
	{
		if (u_buffer) free(u_buffer);
		if (v_buffer) free(v_buffer);
		return 2;
	}

	b = (unsigned char *)bmp;
	y = y_buffer;
	u = u_buffer;
	v = v_buffer;

	// convert RGB to YUV
	if (!flip) {
		for (j = 0; j < y_dim; j ++)
		{
			y = y_buffer + (y_dim - j - 1) * x_dim;
			u = u_buffer + (y_dim - j - 1) * x_dim;
			v = v_buffer + (y_dim - j - 1) * x_dim;

			for (i = 0; i < x_dim; i ++) {
				g = b + 1;
				r = b + 2;
				*y = (unsigned char)(  RGBYUV02990[*r] + RGBYUV05870[*g] + RGBYUV01140[*b]);
				*u = (unsigned char)(- RGBYUV01684[*r] - RGBYUV03316[*g] + (*b)/2          + 128);
				*v = (unsigned char)(  (*r)/2          - RGBYUV04187[*g] - RGBYUV00813[*b] + 128);
				b += 4;
				y ++;
				u ++;
				v ++;
			}
		}
	} else {
		for (i = 0; i < size; i++)
		{
			g = b + 1;
			r = b + 2;
			*y = (unsigned char)(  RGBYUV02990[*r] + RGBYUV05870[*g] + RGBYUV01140[*b]);
			*u = (unsigned char)(- RGBYUV01684[*r] - RGBYUV03316[*g] + (*b)/2          + 128);
			*v = (unsigned char)(  (*r)/2          - RGBYUV04187[*g] - RGBYUV00813[*b] + 128);
			b += 4;
			y ++;
			u ++;
			v ++;
		}
	}

	// subsample UV
	for (j = 0; j < y_dim/2; j ++)
	{
		psu = sub_u_buf + j * x_dim / 2;
		psv = sub_v_buf + j * x_dim / 2;
		pu1 = u_buffer + 2 * j * x_dim;
		pu2 = u_buffer + (2 * j + 1) * x_dim;
		pv1 = v_buffer + 2 * j * x_dim;
		pv2 = v_buffer + (2 * j + 1) * x_dim;
		for (i = 0; i < x_dim/2; i ++)
		{
			*psu = (*pu1 + *(pu1+1) + *pu2 + *(pu2+1)) / 4;
			*psv = (*pv1 + *(pv1+1) + *pv2 + *(pv2+1)) / 4;
			psu ++;
			psv ++;
			pu1 += 2;
			pu2 += 2;
			pv1 += 2;
			pv2 += 2;
		}
	}

	free(u_buffer);
	free(v_buffer);

	return 0;
}


void InitLookupTable()
{
	int i;

	for (i = 0; i < 256; i++) RGBYUV02990[i] = (float)0.2990 * i;
	for (i = 0; i < 256; i++) RGBYUV05870[i] = (float)0.5870 * i;
	for (i = 0; i < 256; i++) RGBYUV01140[i] = (float)0.1140 * i;
	for (i = 0; i < 256; i++) RGBYUV01684[i] = (float)0.1684 * i;
	for (i = 0; i < 256; i++) RGBYUV03316[i] = (float)0.3316 * i;
	for (i = 0; i < 256; i++) RGBYUV04187[i] = (float)0.4187 * i;
	for (i = 0; i < 256; i++) RGBYUV00813[i] = (float)0.0813 * i;
}

