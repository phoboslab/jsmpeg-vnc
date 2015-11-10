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

#ifndef __FAME_ENCODER_MPEG_H
#define __FAME_ENCODER_MPEG_H

#include "fame.h"
#include "fame_encoder.h"

typedef struct _fame_encoder_mpeg_t_ {
  FAME_EXTENDS(fame_encoder_t);
  unsigned char quant_scale;               /* quantization scale */
  dct_t yiqmatrixes[32][64];            /* Y intra quantization matrixes     */
  dct_t ciqmatrixes[32][64];            /* C intra quantization matrixes     */
  dct_t niqmatrixes[32][64];            /* non-intra quantization matrixes   */
  dct_t yiqround[32][64];               /* Y intra quantization rounding     */
  dct_t ciqround[32][64];               /* C intra quantization rounding     */
  dct_t niqround[32][64];               /* non-intra quantization rounding   */
  dct_t tmpblock[64];                   /* temporary block                   */
  short blocks[6][64];                  /* DCT, quantised blocks             */
  int width;                            /* width of frames                   */
  int height;                           /* height of frames                  */
  fame_yuv_t *input;                    /* input frame                       */
  fame_yuv_t **past_ref;                /* past reference frame              */
  fame_yuv_t **new_ref;                 /* reconstructed reference frame     */
  fame_yuv_t **future_ref;              /* future reference frame            */
  unsigned char *shape;                 /* shape mask                        */
  unsigned char *padded;                /* buffer for shape padding          */
  fame_mismatch_t mismatch;             /* mismatch type for dequantisation  */
} fame_encoder_mpeg_t;

#define FAME_ENCODER_MPEG(x) ((fame_encoder_mpeg_t *) x)

extern FAME_CONSTRUCTOR(fame_encoder_mpeg_t);

#endif
