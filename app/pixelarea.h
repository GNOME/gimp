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
#ifndef __PIXEL_AREA_H__
#define __PIXEL_AREA_H__

#include "glib.h"
#include "tag.h"


/* Forward declarations */
struct _Canvas;
struct _PixelRow;
struct _Paint;


typedef struct _PixelArea PixelArea;

struct _PixelArea
{
  /* the image we're iterating over */
  struct _Canvas * canvas;
  
  /* the current area */
  int   x, y;
  int   w, h;

  /* the initial region start */
  int   startx;
  int   starty;

  /*  will this area be dirtied?  */
  int   dirty;
};



/*  PixelArea functions  */
void              pixelarea_init          (PixelArea *, struct _Canvas *,
                                           int x, int y, int w, int h,
                                           int will_dirty);

void              pixelarea_getdata       (PixelArea *, struct _PixelRow *, int);
struct _Paint *   pixelarea_convert_paint (PixelArea *, struct _Paint *);

Tag               pixelarea_tag           (PixelArea *);
int               pixelarea_width         (PixelArea *);
int               pixelarea_height        (PixelArea *);
guchar *          pixelarea_data          (PixelArea *);
int               pixelarea_rowstride     (PixelArea *);


/* pixel area iterators */
void *            pixelarea_register      (int, ...);
void *            pixelarea_process       (void *);
void              pixelarea_process_stop  (void *);

#endif /* __PIXELAREA_H__ */
