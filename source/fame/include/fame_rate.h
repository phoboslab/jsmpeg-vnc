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

#ifndef __FAME_RATE_H
#define __FAME_RATE_H

#include "fame.h"
#include "fame_monitor.h"

#define FAME_RATE_SECOND_PASS 1

typedef struct _fame_rate_t_ {
  FAME_EXTENDS(fame_object_t);

  void (* init)(struct _fame_rate_t_ *rate,
		int mb_width,
		int mb_height,
		int bitrate,
		char *coding,
		fame_frame_statistics_t *stats_list,
		fame_global_statistics_t *global_stats,
		unsigned int flags);
  void (* close)(struct _fame_rate_t_ *rate);
  void (* enter)(struct _fame_rate_t_ *rate,
		 fame_yuv_t **ref,
		 fame_yuv_t *current,
		 unsigned char *shape,
		 char coding,
		 fame_frame_statistics_t *frame_stats);
  int (* global_estimation)(struct _fame_rate_t_ *rate);
  int (* local_estimation)(struct _fame_rate_t_ *rate,
			   int mb_x, int mb_y,
			   short blocks[6][64]);
  void (* leave)(struct _fame_rate_t_ *rate,
		 int spent);

  int mb_width;
  int mb_height;
  fame_yuv_t **ref;
  fame_yuv_t *current;
  unsigned char *shape;
  char coding;
  int bitrate;
  int available;
  int spent;
  int global_scale;
  float coeff1, coeff2;
  int total_frames;
  fame_frame_statistics_t *stats_list;
  unsigned int flags;
} fame_rate_t;

#define FAME_RATE(x) ((fame_rate_t *) x)

extern FAME_CONSTRUCTOR(fame_rate_t);

#endif
