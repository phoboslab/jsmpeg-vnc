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

#ifndef __FAME_DECODER_H
#define __FAME_DECODER_H

typedef struct _fame_decoder_t_ {
  FAME_EXTENDS(fame_object_t);
  void (* init)                (struct _fame_decoder_t_ *decoder,
				int width,
				int height,
				unsigned char *intra_quantisation_table,
				unsigned char *inter_quantisation_table,
				unsigned char *intra_dc_y_scale_table,
				unsigned char *intra_dc_c_scale_table,
				fame_mismatch_t mismatch_type);
  void (* enter)               (struct _fame_decoder_t_ *decoder,
				fame_yuv_t **past_ref,
				fame_yuv_t **new_ref,
				fame_yuv_t **future_ref,
				fame_yuv_t *yuv,
				unsigned char *shape);
  void (* reconstruct_intra_mb)(struct _fame_decoder_t_ *decoder,
				short x,
				short y,
				short *blocks[6],
				unsigned char q,
				fame_bab_t bab_type);
  void (* reconstruct_inter_mb)(struct _fame_decoder_t_ *decoder,
				short x,
				short y,
				short *blocks[6],
				fame_motion_vector_t *forward,
				fame_motion_vector_t *backward,
				fame_motion_coding_t motion_coding,
				unsigned char q,
				fame_bab_t bab_type);
  void (* pad)                 (struct _fame_decoder_t_ *decoder,
				unsigned char *bab_map,
				fame_box_t *box);
  void (* interpolate)         (struct _fame_decoder_t_ *decoder,
				int rounding);
  void (* leave)               (struct _fame_decoder_t_ *decoder);
  void (* close)               (struct _fame_decoder_t_ *decoder);
} fame_decoder_t;

#define FAME_DECODER(x) ((fame_decoder_t *) x)

#endif
