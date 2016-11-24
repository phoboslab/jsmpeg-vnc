/*
    libfame - Fast Assembly MPEG Encoder Library
    Copyright (C) 2000-2001  Damien Vincent

    This library is free software; you can redistribute it and/or
    modify it under the terms of the GNU Library General Public
    License as published by the Free Software Foundation; either
    version 2 of the License, or (at your option) any later version.

    This library is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
    Library General Public License for more details.

    You should have received a copy of the GNU Library General Public
    License along with this library; if not, write to the Free
    Software Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
*/

#ifndef __FAME_MOTION_H
#define __FAME_MOTION_H

#include "fame.h"

#define FAME_MOTION_SUBPEL_SEARCH 1        /* support subpixel search */
#define FAME_MOTION_BLOCK_SEARCH  2        /* support 8x8 block search */
#define FAME_MOTION_UNRESTRICTED_SEARCH  4 /* support out of frame MVs */
#define FAME_MOTION_FLIP_ROUNDING  8       /* support rouding control */

typedef unsigned int (*compute_error_t) (unsigned char *ref,
					 unsigned char *input,
					 unsigned char *shape,
					 int pitch);

typedef struct _fame_motion_t_ {
  FAME_EXTENDS(fame_object_t);

  void (* init)(struct _fame_motion_t_ *motion,
		int mb_width,
		int mb_height,
		unsigned int flags);
  void (* close)(struct _fame_motion_t_ *motion);
  void (* enter)(struct _fame_motion_t_ *motion,
		 fame_yuv_t **ref,
		 fame_yuv_t *current,
		 unsigned char *shape,
		 int search_range);
  fame_motion_coding_t (* estimation)(struct _fame_motion_t_ *motion,
				      int mb_x,
				      int mb_y,
				      fame_motion_vector_t *vectors,
				      unsigned char quant);
  void (* leave)(struct _fame_motion_t_ *motion);

  int mb_width;
  int mb_height;
  fame_yuv_t **ref;
  fame_yuv_t *current;
  unsigned char *shape;
  int search_range;
  int fcode;
  unsigned int flags;
  compute_error_t MAE8x8;
} fame_motion_t;

#define FAME_MOTION(x) ((fame_motion_t *) x)

extern FAME_CONSTRUCTOR(fame_motion_t);

#endif
