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

#include "tag.h"


/* Forward declarations */
struct _Canvas;
struct _PixelRow;


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
void              pixelarea_resize        (PixelArea *,
                                           int x, int y, int w, int h,
                                           int will_dirty);
void              pixelarea_getdata       (PixelArea *, struct _PixelRow *, int);
Tag               pixelarea_tag           (PixelArea *);
int               pixelarea_width         (PixelArea *);
int               pixelarea_height        (PixelArea *);
int               pixelarea_x             (PixelArea *);
int               pixelarea_y             (PixelArea *);
guchar *          pixelarea_data          (PixelArea *);
int               pixelarea_rowstride     (PixelArea *);
guint             pixelarea_ref           (PixelArea *);
guint             pixelarea_unref         (PixelArea *);


/* pixel area iterators */
void *            pixelarea_register       (int, ...);
void *            pixelarea_register_noref (int, ...);
void *            pixelarea_process        (void *);
void              pixelarea_process_stop   (void *);


/* these belong elsewhere */
void              pixelarea_copy_row      (PixelArea *, struct _PixelRow *,
                                           int, int, int, int);
void              pixelarea_copy_col      (PixelArea *, struct _PixelRow *,
                                           int, int, int, int);
void              pixelarea_write_row     (PixelArea *, struct _PixelRow *,
                                           int, int, int);
void              pixelarea_write_col     (PixelArea *, struct _PixelRow *,
                                           int, int, int);


#endif /* __PIXELAREA_H__ */
