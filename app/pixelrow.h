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
 * Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 */
#ifndef __PIXEL_ROW_H__
#define __PIXEL_ROW_H__

#include "tag.h"

/* a structure that attaches a length and data encoding format to a
   buffer of raw data */
typedef struct _PixelRow PixelRow;

struct _PixelRow
{
  Tag        tag;
  guchar *   buffer;
  int        width;
};

/* if you define DEBUG_PIXELROW, then actual functions which check
   inputs will be used.  otherwise, the macros below will be used */

#ifdef DEBUG_PIXELROW

void     pixelrow_init   (PixelRow *, Tag, guchar *, int);
Tag      pixelrow_tag    (PixelRow *);
int      pixelrow_width  (PixelRow *);
guchar * pixelrow_data   (PixelRow *);

#else

#define pixelrow_init(p,t,b,w) (p)->tag=(t);(p)->buffer=(b);(p)->width=(w)
#define pixelrow_tag(p)        (p)->tag
#define pixelrow_width(p)      (p)->width
#define pixelrow_data(p)       (p)->buffer

#endif

#endif /* __PIXEL_ROW_H__ */
