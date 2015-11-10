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

#ifndef __FAME_MOTION_NONE_H
#define __FAME_MOTION_NONE_H

#include "fame.h"
#include "fame_motion.h"

typedef struct _fame_motion_none_t_ {
  FAME_EXTENDS(fame_motion_t);
} fame_motion_none_t;

#define FAME_MOTION_NONE(x) ((fame_motion_none_t *) x)

extern FAME_CONSTRUCTOR(fame_motion_none_t);

#endif
