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

#include "glib.h"
#include "tag.h"

/* forward declarations */
struct _Paint;


typedef struct _PixelRow PixelRow;

struct _PixelRow
{
  Tag        tag;
  guchar *   buffer;
  int        width;
};


void              pixelrow_init           (PixelRow *, Tag, guchar *buf, int numpixels);

guchar *          pixelrow_getdata        (PixelRow *, int);
struct _Paint *   pixelrow_convert_paint  (PixelRow *, struct _Paint *);

Tag               pixelrow_tag            (PixelRow *);
int               pixelrow_width          (PixelRow *);
guchar *          pixelrow_data           (PixelRow *);


#endif /* __PIXEL_ROW_H__ */
