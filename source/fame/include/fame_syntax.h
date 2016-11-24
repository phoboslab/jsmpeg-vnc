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

#ifndef __FAME_SYNTAX_H
#define __FAME_SYNTAX_H

#include "fame.h"

#define FAME_SYNTAX_ARBITRARY_SHAPE 1 /* encode shape information */
#define FAME_SYNTAX_LOSSLESS_SHAPE  2 /* encode shape at full quality */

typedef struct _fame_syntax_t_ {
  FAME_EXTENDS(fame_object_t);
  void (* init)            (struct _fame_syntax_t_ *syntax,
			    int mb_width,
			    int mb_height,
			    unsigned char **intra_default_matrix,
			    unsigned char **inter_default_matrix,
			    unsigned char *intra_dc_y_scale_table,
			    unsigned char *intra_dc_c_scale_table,
			    fame_mismatch_t *mismatch_type,
			    unsigned int flags);
  void (* use)             (struct _fame_syntax_t_ *syntax,
			    unsigned char *buffer,
			    int size);
  int  (* flush)           (struct _fame_syntax_t_ *syntax);
  void (* start_sequence)  (struct _fame_syntax_t_ *syntax,
			    int width,
			    int height,
			    int fps_num,
			    int fps_den,
			    int size,
			    int bitrate);
  void (* start_GOP)       (struct _fame_syntax_t_ *syntax,
			    int frame);
  void (* start_picture)   (struct _fame_syntax_t_ *syntax,
			    char frame_type,
			    int frame_number,
			    fame_box_t *box,
			    int rounding_control,
			    int search_range);
  void (* start_slice)     (struct _fame_syntax_t_ *syntax,
			    int vpos,
			    int length,
			    unsigned char qscale);
  void (* end_slice)       (struct _fame_syntax_t_ *syntax);
  void (* end_sequence)    (struct _fame_syntax_t_ *syntax);
  void (* predict_vector)  (struct _fame_syntax_t_ *syntax,
			    int mb_x,
			    int mb_y,
			    int k,
			    fame_motion_vector_t *mv);
  void (* compute_chrominance_vectors)(struct _fame_syntax_t_ *syntax,
				       struct _fame_motion_vector_t_ *vectors,
				       unsigned char pattern);
  void (* write_intra_mb)  (struct _fame_syntax_t_ *syntax,
			    int mb_x,
			    int mb_y,
			    short *blocks[6],
			    unsigned char *bab,
			    unsigned char *bab_map,
			    fame_bab_t bab_type,
			    unsigned char dquant,
			    unsigned char pattern);
  void (* write_inter_mb)  (struct _fame_syntax_t_ *syntax,
			    int mb_x,
			    int mb_y,
			    short *blocks[6],
			    unsigned char *bab,
			    unsigned char *bab_map,
			    fame_bab_t bab_type,
			    unsigned char dquant,
			    unsigned char pattern,
			    fame_motion_vector_t *forward,
			    fame_motion_vector_t *backward,
			    fame_motion_coding_t motion_coding);
  void (* close)           (struct _fame_syntax_t_ *syntax);
} fame_syntax_t;

#define FAME_SYNTAX(x) ((fame_syntax_t *) x)

#endif
