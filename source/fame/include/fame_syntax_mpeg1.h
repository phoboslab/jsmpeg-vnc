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

#ifndef __FAME_SYNTAX_MPEG1_H
#define __FAME_SYNTAX_MPEG1_H

#include "fame.h"
#include "fame_syntax.h"
#include "fame_bitbuffer.h"

typedef enum { frame_type_I,
	       frame_type_P } frame_type_t;

typedef struct _fame_syntax_mpeg1_t_ {
  FAME_EXTENDS(fame_syntax_t);
  fame_bitbuffer_t buffer;                     /* bitbuffer */
  int fps_num, fps_den;                        /* framerate */
  short int y_dc_pred, cr_dc_pred, cb_dc_pred; /* DC predictors */
  fame_motion_vector_t mv_pred;
  frame_type_t frame_type;
  unsigned char f_code;
  unsigned int prev_mb_addr;
  unsigned int slice_start;
  unsigned int slice_length;
  int mb_width, mb_height;
  fame_motion_coding_t previous_coding;
  fame_vlc_t *vlc_table;
} fame_syntax_mpeg1_t;

#define FAME_SYNTAX_MPEG1(x) ((fame_syntax_mpeg1_t *) x)

FAME_CONSTRUCTOR(fame_syntax_mpeg1_t);

#endif /* __FAME_SYNTAX_MPEG1_H */
