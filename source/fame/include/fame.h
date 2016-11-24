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

#ifndef __FAME_H__
#define __FAME_H__

#include <stdlib.h>
#include "fame_version.h"

/* Some compilers use a special export keyword */
/* Code taken from SDL                         */
#ifndef DECLSPEC
# ifdef __BEOS__
#  define ASMSYM
#  if defined(__GNUC__)
#   define DECLSPEC     __declspec(dllexport)
#  else
#   define DECLSPEC     __declspec(export)
#  endif
# else
# ifdef WIN32
#  define DECLSPEC      __declspec(dllexport)
#  define ASMSYM  "_"
# else
#  define DECLSPEC
#  define ASMSYM
# endif
# endif
#endif

/* Alignment */
#if defined(__GNUC__)
#ifdef WIN32
#define FAME_ALIGNMENT 16
#else
#define FAME_ALIGNMENT 32
#endif
#define FAME_ALIGNED __attribute__ ((__aligned__(FAME_ALIGNMENT)))
#else
#define FAME_ALIGNED
#endif

/* Error management */
#if defined(__GNUC__)
#define FAME_INFO(format, args...) \
      fprintf(stderr, format, ##args)
#define FAME_WARNING(format, args...) \
      fprintf(stderr, "Warning: " format, ##args)
#define FAME_ERROR(format, args...) \
      fprintf(stderr, "Error: " format, ##args)
#define FAME_FATAL(format, args...) \
      { fprintf(stderr, "Fatal: " format, ##args); exit(-1); }
#else /* not __GNUC__ */
/* No vararg macros */
int FAME_INFO(const char *format, ...);
int FAME_WARNING(const char *format, ...);
int FAME_ERROR(const char *format, ...);
int FAME_FATAL(const char *format, ...);

#endif

#ifndef fame_min
#define fame_min(X,Y) (((X) < (Y)) ? (X) : (Y))
#endif
#ifndef fame_max
#define fame_max(X,Y) (((X) > (Y)) ? (X) : (Y))
#endif

/* object management */
#define FAME_EXTENDS(t) t super
#define FAME_NEW(t) t ## _constructor((t *) malloc(sizeof(t)))
#define FAME_CONSTRUCTOR(t) t * t ## _constructor(t *this)
#define FAME_DELETE(x) free(x)
#define FAME_OVERLOADED(x) super_ ## x

typedef struct _fame_yuv_t_ {
  unsigned int w, h, p;
  unsigned char *y;
  unsigned char *u;
  unsigned char *v;
} fame_yuv_t;

typedef enum { bab_not_coded,
	       bab_all_coded,
	       bab_border_16x16,
	       bab_border_8x8,
	       bab_border_4x4 } fame_bab_t;

typedef struct _fame_box_t_ {
  short x, y;
  unsigned short w, h;
} fame_box_t;

typedef struct _fame_vlc_t_ {
  unsigned long code;
  unsigned long length;
} fame_vlc_t;

#if defined(HAS_MMX)
typedef short dct_t;
#else
typedef float dct_t;
#endif

struct _fame_motion_vector_t_ {
  int dx;
  int dy;
  int error;
  unsigned long deviation;
  unsigned short count;
};
typedef struct _fame_motion_vector_t_ fame_motion_vector_t;

typedef enum { motion_intra, motion_inter } fame_motion_coding_t;

typedef enum { fame_mismatch_local, fame_mismatch_global } fame_mismatch_t;

typedef struct _fame_context_t_ fame_context_t;
typedef struct _fame_frame_statistics_t_ fame_frame_statistics_t;
typedef struct _fame_global_statistics_t_ fame_global_statistics_t;
typedef struct _fame_parameters_t_ fame_parameters_t;

/******************************* object type *********************************/
typedef struct _fame_object_t_ {
  char const *name;
} fame_object_t;

#define FAME_OBJECT(x) ((fame_object_t *) x)

typedef struct _fame_list_t_ {
  char const *type;
  fame_object_t *item;
  struct _fame_list_t_ *next;
} fame_list_t;

/********************************** context **********************************/

struct _fame_context_t_ {
  fame_list_t *type_list;
  fame_object_t *profile;
  struct _fame_private_t_ *priv;
};

/******************************** statistics *********************************/

struct _fame_frame_statistics_t_ {
  unsigned int frame_number;
  char coding;
  signed int target_bits;
  unsigned int actual_bits;
  unsigned int spatial_activity;
  unsigned int quant_scale;
};


struct _fame_global_statistics_t_ {
  unsigned int total_frames;
  unsigned int target_rate;
  unsigned int actual_rate;
  unsigned int mean_spatial_activity;
};


/******************************** parameters *********************************/

struct _fame_parameters_t_ {
  int width;                        /* width of the video sequence */
  int height;                       /* height of the video sequence */
  char const *coding;               /* coding sequence */
  int quality;                      /* video quality */
  int bitrate;                      /* video bitrate (0=VBR)*/
  int slices_per_frame;             /* number of slices per frame */
  unsigned int frames_per_sequence; /* number of frames per sequence */
  int frame_rate_num;               /* numerator of frames per second */
  int frame_rate_den;               /* denominator of frames per second */
  int shape_quality;                /* binary shape quality */
  unsigned int search_range;        /* motion estimation search range */
  unsigned char verbose;            /* verbosity */
  char const *profile;              /* profile name */
  unsigned int total_frames;        /* total number of frames */
  int (* retrieve_cb)(fame_frame_statistics_t *stats);
};

#define FAME_PARAMETERS_INITIALIZER {		                             \
  352,					/* CIF width  */	             \
  288,					/* CIF height */	             \
  "I",					/* I sequence */	             \
  75,					/* average video quality */          \
  0,                                    /* variable bitrate */               \
  1,					/* 1 slice/frame */	             \
  0xffffffff,				/* unlimited length */	             \
  25,                                   /* 25 frames/second */               \
  1,                                    /* /1 */                             \
  100,                                  /* original shape */                 \
  0,                                    /* adaptative search range */        \
  1,                                    /* verbose mode */                   \
  "mpeg4",                              /* profile name */                   \
  0,                                    /* number of frames */               \
  NULL                                  /* stats retrieval callback */       \
}

/***************************** function prototypes ***************************/

#ifdef __cplusplus
extern "C" {
#endif

extern DECLSPEC fame_context_t * fame_open();

extern DECLSPEC void fame_register(fame_context_t * context, 
				   char const *type,
				   fame_object_t *object);

extern DECLSPEC void fame_unregister(fame_context_t * context, 
				     char const *type);

extern DECLSPEC fame_object_t *fame_get_object(fame_context_t * context,
					       char const *type);

extern DECLSPEC void fame_init(fame_context_t * context, 
			       fame_parameters_t *p,
			       unsigned char *buffer,
			       unsigned int size);

extern DECLSPEC void fame_start_frame(fame_context_t *context,
				      fame_yuv_t *yuv,
				      unsigned char *mask);
  
extern DECLSPEC int fame_encode_slice(fame_context_t *context);

extern DECLSPEC void fame_end_frame(fame_context_t *context,
				    fame_frame_statistics_t *stats);

extern DECLSPEC int fame_close(fame_context_t *context);

/* DEPRECATED */
extern DECLSPEC int fame_encode_frame(fame_context_t *context,
				      fame_yuv_t *yuv,
				      unsigned char *mask);

#ifdef __cplusplus
}
#endif

#endif
