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
#ifndef __TILE_BUF_H__
#define __TILE_BUF_H__

#include "tag.h"


typedef struct _TileBuf  TileBuf;


TileBuf *        tilebuf_new            (Tag, int w, int h);
void             tilebuf_delete         (TileBuf *);
TileBuf *        tilebuf_clone          (TileBuf *);
void             tilebuf_info           (TileBuf *);

Tag              tilebuf_tag            (TileBuf *);
Precision        tilebuf_precision      (TileBuf *);
Format           tilebuf_format         (TileBuf *);
Alpha            tilebuf_alpha          (TileBuf *);

guint            tilebuf_width          (TileBuf *);
guint            tilebuf_height         (TileBuf *);





guint            tilebuf_portion_ref       (TileBuf *, int x, int y);
void             tilebuf_portion_unref     (TileBuf *, int x, int y);

guint            tilebuf_portion_width     (TileBuf *, int x, int y);
guint            tilebuf_portion_height    (TileBuf *, int x, int y);
guint            tilebuf_portion_top       (TileBuf *, int x, int y);
guint            tilebuf_portion_left      (TileBuf *, int x, int y);

guchar *         tilebuf_portion_data      (TileBuf *, int x, int y);
guint            tilebuf_portion_rowstride (TileBuf *, int x, int y);

guint            tilebuf_portion_alloced   (TileBuf *, int x, int y);
guint            tilebuf_portion_alloc     (TileBuf *, int x, int y);
guint            tilebuf_portion_unalloc   (TileBuf *, int x, int y);



/* temporary evil */
void tilebuf_init (TileBuf *, TileBuf *, int x, int y);

#endif /* __TILE_BUF_H__ */
