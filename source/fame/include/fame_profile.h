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

#ifndef __FAME_PROFILE_H
#define __FAME_PROFILE_H

#include "fame.h"

typedef struct _fame_profile_private_data_ fame_profile_private_data_t;

typedef struct _fame_profile_t_ {
  FAME_EXTENDS(fame_object_t);
  void (* init)(struct _fame_profile_t_ *profile,
		fame_context_t *context,
		fame_parameters_t *params,
		unsigned char *buffer,
		unsigned int size);
  void (* enter)(struct _fame_profile_t_ *profile,
		 fame_yuv_t *yuv,
		 unsigned char *shape);		 
  int (* encode)(struct _fame_profile_t_ *profile);
  void (* leave)(struct _fame_profile_t_ *profile,
		 fame_frame_statistics_t *stats);
  int (* close)(struct _fame_profile_t_ *profile);
  fame_profile_private_data_t *data;
} fame_profile_t;

#define FAME_PROFILE(x) ((fame_profile_t *) x)

#endif
