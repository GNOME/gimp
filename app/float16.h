/* The GIMP -- an image manipulation program
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.
 */
#ifndef __FLOAT16_H__
#define __FLOAT16_H__

#include <glib.h>

#define ONE_FLOAT16 16256
#define HALF_FLOAT16 16128
#define ZERO_FLOAT16 0

extern guint16 f16_shorts_in[];
extern const gfloat *f16_float_in;
extern gfloat f16_float_out;
extern const guint16 *f16_shorts_out;

#if BYTE_ORDER == BIG_ENDIAN
#define FLT(x) (*f16_shorts_in = (x), *f16_float_in)
#define FLT16(x) (f16_float_out = (x), *f16_shorts_out) 
#elif BYTE_ORDER == LITTLE_ENDIAN
#define FLT(x) (f16_shorts_in[1] = (x), *f16_float_in)
#define FLT16(x) (f16_float_out = (x), f16_shorts_out[1]) 
#endif

#endif
