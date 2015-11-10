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

#ifndef __FAME_VERSION_H__
#define __FAME_VERSION_H__

#ifndef LIBFAME_MAJOR_VERSION
#define LIBFAME_MAJOR_VERSION (0)
#define LIBFAME_MINOR_VERSION (9)
#define LIBFAME_MICRO_VERSION (0)
#define LIBFAME_VERSION "0.9.0"
#endif

extern const unsigned int libfame_major_version,
                          libfame_minor_version,
                          libfame_micro_version;
extern const char libfame_version[];

#endif
