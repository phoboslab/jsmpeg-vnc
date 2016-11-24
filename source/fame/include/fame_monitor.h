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

#ifndef __FAME_MONITOR_H
#define __FAME_MONITOR_H

#include "fame.h"

#define FAME_MONITOR_LOAD_STATS 1


unsigned int activity(fame_yuv_t *frame,
		      unsigned char *shape,
		      unsigned int mb_width,
		      unsigned int mb_height);

unsigned int activity2(fame_yuv_t *ref,
		       fame_yuv_t *frame,
		       unsigned char *shape,
		       unsigned int mb_width,
		       unsigned int mb_height);

typedef struct _fame_monitor_t_ {
  FAME_EXTENDS(fame_object_t);

  void (* init)(struct _fame_monitor_t_ *monitor,
		int (* retrieve_cb)(fame_frame_statistics_t *stats),
		unsigned int mb_width,
		unsigned int mb_height,
		unsigned int total_frames,
		unsigned int flags);
  void (* close)(struct _fame_monitor_t_ *monitor);
  void (* enter)(struct _fame_monitor_t_ *monitor,
		 unsigned int frame_number,
		 fame_yuv_t **ref,
		 fame_yuv_t *frame,
		 unsigned char *shape,
		 char *coding);
  fame_frame_statistics_t* (* leave)(struct _fame_monitor_t_ *monitor,
					unsigned int spent,
					unsigned int quant_scale);

  fame_global_statistics_t global_stats;
  fame_frame_statistics_t *current_frame_stats;
  fame_frame_statistics_t *frame_stats_list;
  int (* retrieve_stats_callback)(fame_frame_statistics_t *frame_stats);
  int keyframe;
  unsigned int mb_width;
  unsigned int mb_height;
  unsigned int old_activity;
  unsigned int flags;
} fame_monitor_t;

#define FAME_MONITOR(x) ((fame_monitor_t *) x)

extern FAME_CONSTRUCTOR(fame_monitor_t);

#endif
