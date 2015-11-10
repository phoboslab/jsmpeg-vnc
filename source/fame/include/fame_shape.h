/*
    libfame - Fast Assembly MPEG Encoder Library
    Copyright (C) 2000-2001 Vivien Chappelier
                            Damien Vincent

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

#ifndef __SHAPE_INT_H
#define __SHAPE_INT_H

#include "fame.h"

#define FAME_SHAPE_LOSSLESS 1 /* no downsampling */

typedef struct _fame_shape_t_ {
  FAME_EXTENDS(fame_object_t);
  void  (* init)(struct _fame_shape_t_ *shape,
		 int mb_width,
		 int mb_height,
		 unsigned int flags);
  void  (* close)(struct _fame_shape_t_ *shape);
  void  (* enter)(struct _fame_shape_t_ *shape,
		  unsigned char *mask,
		  unsigned char *ref,
		  unsigned char alpha_th);
  fame_bab_t (* encode_intra_shape)(struct _fame_shape_t_ *shape,
				    int mb_x,
				    int mb_y,
				    unsigned char **bab,
				    unsigned char *pattern);

  int mb_width;
  int mb_height;
  int pitch;
  unsigned char *input;
  unsigned char *recon;
  unsigned char bab16x16[20][20];
  unsigned char bab8x8[12][12];
  unsigned char bab4x4[8][8];
  unsigned char alpha_th;
  unsigned int flags;
} fame_shape_t;

#define FAME_SHAPE(x) ((fame_shape_t *) x)

extern FAME_CONSTRUCTOR(fame_shape_t);

#endif

