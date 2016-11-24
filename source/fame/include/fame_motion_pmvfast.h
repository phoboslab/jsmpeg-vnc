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

#ifndef __FAME_MOTION_PMVFAST_H
#define __FAME_MOTION_PMVFAST_H

#include "fame.h"
#include "fame_motion.h"

/**** Capabilites of the motion object ****/

#define MOTIONCAP_HALF      0x0001
#define MOTIONCAP_QUARTER   0x0002
#define MOTIONCAP_BLOCK8x8  0x0004
#define MOTIONCAP_USEPREDICTED 0x0008

typedef struct _fame_motion_pmvfast_t_ {
  FAME_EXTENDS(fame_motion_t);

  void (* FAME_OVERLOADED(init))(fame_motion_t *motion,
				 int mb_width,
				 int mb_height,
				 unsigned int flags);
  void (* FAME_OVERLOADED(close))(fame_motion_t *motion);
  void (* FAME_OVERLOADED(enter))(fame_motion_t *motion,
				  fame_yuv_t **ref,
				  fame_yuv_t *current,
				  unsigned char *shape,
				  int search_range);
  void (* FAME_OVERLOADED(leave))(fame_motion_t *motion);

  fame_motion_vector_t *vectors[2];
} fame_motion_pmvfast_t;

extern FAME_CONSTRUCTOR(fame_motion_pmvfast_t);

#define FAME_MOTION_PMVFAST(x) ((fame_motion_pmvfast_t *) x)

#endif

