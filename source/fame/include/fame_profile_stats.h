/*
    libfame - Fast Assembly MPEG Encoder Library
    Copyright (C) 2000-2001 Vivien Chappelier

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

#ifndef __FAME_PROFILE_STATS_H
#define __FAME_PROFILE_STATS_H

#include "fame.h"
#include "fame_profile.h"
#include "fame_monitor.h"

typedef struct _fame_profile_stats_t_ {
  FAME_EXTENDS(fame_profile_t);
  /* protected data */
  int width;
  int height;
  char *coding;
  unsigned int total_frames;
  int frame_number;
  unsigned char *ref_shape;
  fame_yuv_t *ref[2];
  int current;
  unsigned int monitor_flags;
  fame_monitor_t *monitor;
  fame_frame_statistics_t *frame_stats;
} fame_profile_stats_t;

#define FAME_PROFILE_STATS(x) ((fame_profile_stats_t *) x)

extern FAME_CONSTRUCTOR(fame_profile_stats_t);

#endif








