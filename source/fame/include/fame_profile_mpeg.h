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

#ifndef __FAME_PROFILE_MPEG_H
#define __FAME_PROFILE_MPEG_H

#include "fame.h"
#include "fame_profile.h"
#include "fame_encoder.h"
#include "fame_decoder.h"
#include "fame_motion.h"
#include "fame_syntax.h"
#include "fame_shape.h"
#include "fame_rate.h"
#include "fame_monitor.h"

typedef struct _fame_profile_mpeg_t_ {
  FAME_EXTENDS(fame_profile_t);
  /* protected data */
  int width;
  int height;
  char *coding;
  unsigned char quant_scale;
  int bitrate;
  int slices_per_frame;
  int frames_per_gop;
  unsigned int frames_per_sequence;
  unsigned int total_frames;
  int lines_per_slice;
  int slice_number;
  int frame_number;
  int gop_number;
  int fps_num;
  int fps_den;
  int alpha_th;
  unsigned int search_range;
  unsigned int search_range_adaptive;
  unsigned char intra_dc_y_scale_table[32];
  unsigned char intra_dc_c_scale_table[32];
  unsigned char *intra_matrix;
  unsigned char *inter_matrix;
  int rounding;
  fame_mismatch_t mismatch;
  unsigned char verbose;
  unsigned char *ref_shape;
  unsigned char *bab_map;
  fame_yuv_t *ref[2][4];
  unsigned int past, current, future;
  unsigned char *buffer;
  unsigned int size;
  unsigned int dirty;
  unsigned int total;
  unsigned int decoder_flags;
  unsigned int encoder_flags;
  unsigned int motion_flags;
  unsigned int syntax_flags;
  unsigned int shape_flags;
  unsigned int rate_flags;
  unsigned int monitor_flags;
  fame_decoder_t *decoder;
  fame_encoder_t *encoder;
  fame_motion_t *motion;
  fame_syntax_t *syntax;
  fame_shape_t *shape;
  fame_rate_t *rate;
  fame_monitor_t *monitor;
  char current_coding, next_coding;
  int intra, inter;
  fame_frame_statistics_t *frame_stats;
  int slice_start;
  fame_box_t bounding_box;
} fame_profile_mpeg_t;

#define FAME_PROFILE_MPEG(x) ((fame_profile_mpeg_t *) x)

extern FAME_CONSTRUCTOR(fame_profile_mpeg_t);

#endif
