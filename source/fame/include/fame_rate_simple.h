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

#ifndef __FAME_RATE_SIMPLE_H
#define __FAME_RATE_SIMPLE_H

#include "fame.h"
#include "fame_rate.h"
#include "fame_monitor.h"

#define FAME_RATE_2PASS_POWER 0.5

typedef struct _fame_rate_simple_t_ {
  FAME_EXTENDS(fame_rate_t);
  void (* FAME_OVERLOADED(init))(struct _fame_rate_t_ *rate,
				 int mb_width,
				 int mb_height,
				 int bitrate,
				 char *coding,
				 fame_frame_statistics_t *stats_list,
				 fame_global_statistics_t *global_stats,
				 unsigned int flags);
  void (* FAME_OVERLOADED(enter))(struct _fame_rate_t_ *rate,
				  fame_yuv_t **ref,
				  fame_yuv_t *current,
				  unsigned char *shape,
				  char coding,
				  fame_frame_statistics_t *frame_stats);
  void (* FAME_OVERLOADED(leave))(struct _fame_rate_t_ *rate,
				  int spent);

  
  int I_bits, P_bits;
  int activity;
  float I_coeff1;
} fame_rate_simple_t;

#define FAME_RATE_SIMPLE(x) ((fame_rate_simple_t *) x)

extern FAME_CONSTRUCTOR(fame_rate_simple_t);

#endif






