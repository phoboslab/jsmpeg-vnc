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

#ifndef __FAME_BITBUFFER_H__
#define __FAME_BITBUFFER_H__

typedef struct _fame_bitbuffer_t_
{
  unsigned char * base;
  unsigned char * data;
  unsigned long shift;
} fame_bitbuffer_t;

#if defined(HAS_BSWAP)
/* Note: This code will work on >i486 only as i386 doesn't support the bswap */
/* instruction. Alternatives (using xchg for example) could be written for   */
/* i386.                                                                     */
#define fast_bitbuffer_write(data, shift, c, l)				\
{									\
  int d;								\
									\
  asm("add %1, %%ecx\n"            /* ecx = shift + length */		\
      "shrd %%cl, %2, %3\n"        /* adjust code to fit in */		\
      "shr %%cl, %2\n"             /* adjust code to fit in */		\
      "mov %%ecx, %1\n"            /* shift += length */		\
      "bswap %2\n"                 /* reverse byte order of code */	\
      "shr $5, %%ecx\n"            /* get dword increment */            \
      "or %2, (%0)\n"              /* put first 32 bits */		\
      "bswap %3\n"                 /* reverse byte order of code */	\
      "lea   (%0, %%ecx, 4), %0\n" /* data += (ecx>32) */		\
      "andl $31, %1\n"             /* mask shift */			\
      "orl %3, (%0)\n"             /* put last 32 bits */		\
      : "=r"(data), "=r"(shift), "=a"(d), "=d"(d), "=c"(d)	\
      : "0"(data), "1"(shift), "2"((unsigned long) c), "3"(0), "c"((unsigned long) l) \
      : "memory");							\
}
#else
#define fast_bitbuffer_write(data, shift, d, size)               \
{							\
  /* assume size != 0 */				\
  unsigned char * ptr;					\
  unsigned char left;					\
  unsigned long c;					\
  							\
  ptr = data + ((shift) >> 3); 	        \
  left = 8 - ((shift) & 7);			        \
							\
  /* left align */					\
  c = (((unsigned long) (d)) << (32 - size));           \
							\
  *ptr++ |= (c >> (32 - left));				\
  c <<= left;						\
  *ptr++ |= (c >> 24);					\
  c <<= 8;						\
  *ptr++ |= (c >> 24);					\
  c <<= 8;						\
  *ptr++ |= (c >> 24);					\
  c <<= 8;						\
  *ptr++ |= (c >> 24);					\
  							\
  shift += (size);				\
  data += (((shift) >> 5) << 2);	        \
  shift &= 31;				        \
}
#endif

#define bitbuffer_write(bb, c, l) fast_bitbuffer_write((bb)->data, (bb)->shift, c, l)

#define bitbuffer_init(bb, d, size)			\
{						        \
  (bb)->base = d;			                \
  (bb)->data = d;			                \
  (bb)->shift = 0;				        \
  memset((bb)->data, 0, size);                          \
}

#define bitbuffer_flush(bb) (((((bb)->data - (bb)->base) << 3) + ((bb)->shift + 7)) >> 3)

#define bitbuffer_length(bb) ((((bb)->data - (bb)->base) << 3) + ((bb)->shift))

static void inline bitbuffer_cat(fame_bitbuffer_t *bb1, fame_bitbuffer_t *bb2)
{
  unsigned char *ptr = bb2->base;
  int length = bitbuffer_length(bb2);

  while(length > 32) {
    bitbuffer_write(bb1, *ptr, 32);
    ptr += 4;
    length -= 32;
  }
  bitbuffer_write(bb1, ((*ptr) >> (32 - length)), length);
}

static unsigned long inline bitbuffer_padding(fame_bitbuffer_t *bb)
{
  return((8 - (bb->shift & 7)) & 7);
} 

#endif
