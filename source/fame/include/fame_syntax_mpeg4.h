/*
    libfame - Fast Assembly MPEG Encoder Library
    Copyright (C) 2000-2001 Vivien Chappelier
                            Thomas Cougnard

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

#ifndef __FAME_SYNTAX_MPEG4_H
#define __FAME_SYNTAX_MPEG4_H

#include "fame.h"
#include "fame_syntax.h"
#include "fame_bitbuffer.h"

typedef struct _fame_syntax_cae_t_ {
  unsigned long lower;
  unsigned long range;
  int bits_to_follow;
  int first_bit;
  int nzeros;
  int nonzero;
  unsigned char *buffer;
  unsigned char *sequence;
} fame_syntax_cae_t;

typedef struct _fame_syntax_mpeg4_t_ {
  FAME_EXTENDS(fame_syntax_t);

  fame_bitbuffer_t buffer;    /* bitbuffer */
  int mb_width;               /* width of VOP in macroblocks */
  int mb_height;              /* height of VOP in macroblocks */
  int fps_num, fps_den;       /* framerate */
  short int **y_pred_v[2]; /* AC/DC vertical predictors luminance */
  short int **cr_pred_v;   /* AC/DC vertical predictors chrominance red */
  short int **cb_pred_v;   /* AC/DC vertical predictors chrominance blue */
  short int *y_pred_h[3];  /* AC/DC horizontal predictors luminance */
  short int *cr_pred_h[2]; /* AC/DC horizontal predictors chrominance red */
  short int *cb_pred_h[2]; /* AC/DC horizontal predictors chrominance blue*/
  short int *pred[6];      /* AC/DC predictors in macroblock */
  short int *diff[6];      /* AC/DC predictors in macroblock */
  short int *pred_default;    /* default value of predictor */
  fame_motion_vector_t *motion_pred; /* motion predictors */
  fame_motion_vector_t *mv_pred; /* current motion predictors */
  fame_syntax_cae_t *cae_h;   /* horizontal context arithmetic encoder */
  fame_syntax_cae_t *cae_v;   /* vertical context arithmetic encoder */
  fame_vlc_t *intra_table;    /* variable length table for AC coeffs coding */
  fame_vlc_t *inter_table;    /* variable length table for AC coeffs coding */
  unsigned char y_dc_scaler[32]; /* intra DC scaler for the Y component */
  unsigned char c_dc_scaler[32]; /* intra DC scaler for the C component */
  int *symbol;              /* arithmetic symbol table */
  char profile_and_level_indication;   /* Video Object Sequence */
  char is_visual_object_identifier;    /* Visual Object */
  char visual_object_verid;
  char visual_object_priority;
  char visual_object_type;
  char video_signal_type;              /* Video Signal Type */
  char video_format;
  char video_range;
  char colour_description;
  char colour_primaries;
  char transfer_characteristics;
  char matrix_coefficients;
  char short_video_header;             /* Video Object Layer */
  char random_accessible_vol;
  char video_object_type_indication;
  char is_object_layer_identifier;
  char video_object_layer_verid;
  char video_object_layer_priority;
  char aspect_ratio_info;
  char par_width;
  char par_height;
  char vol_control_parameters;
  char video_object_layer_shape;
  char video_object_layer_shape_extension;
  int  vop_time_increment_resolution;
  char fixed_vop_rate;
  int  fixed_vop_time_increment;
  int  video_object_layer_width;
  int  video_object_layer_height;
  char interlaced;
  char obmc_disable;
  char sprite_enable;
  char sadct_disable;
  char not_8_bit;
  char quant_precision;
  char bits_per_pixel;
  char quant_type;
  char load_intra_quant_mat;
  char load_nonintra_quant_mat;
  unsigned char intra_quant_mat[64];
  unsigned char nonintra_quant_mat[64];
  char quarter_sample;
  char complexity_estimation_disable;
  char resync_marker_disable;
  char data_partitionned;
  char reversible_vlc;
  char newpred_enable;
  char reduced_resolution_vop_enable;
  char scalability;
  char closed_gov;                     /* Group Of VOP */
  char broken_link;
  int vop_time_increment;              /* Video Object Plane */
  char vop_coding_type;
  char vop_rounding_type; 
  char vop_reduced_resolution;
  int vop_width;
  int vop_height;
  int vop_horizontal_mc_spatial_ref;
  int vop_vertical_mc_spatial_ref;
  char change_conv_ratio_disable;
  char vop_constant_alpha;
  char vop_constant_alpha_value;
  char intra_dc_vlc_thr;
  char flag_video_packet_header;
  int vop_quant;
  char vop_fcode_forward;
  char vop_fcode_backward;
  char vop_shape_coding_type;
  int quant_scale;
  char header_extension_code;
  int macroblock_number;
  int macroblock_number_size; 
} fame_syntax_mpeg4_t;

#define FAME_SYNTAX_MPEG4(x) ((fame_syntax_mpeg4_t *) x)

FAME_CONSTRUCTOR(fame_syntax_mpeg4_t);

#endif /* __FAME_SYNTAX_MPEG4_H */
