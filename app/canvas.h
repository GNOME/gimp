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
#ifndef __CANVAS_H__
#define __CANVAS_H__

#include "tag.h"


typedef struct _Canvas Canvas;

typedef enum _Tiling Tiling;
enum _Tiling
{
  TILING_NONE   = 0,
  TILING_NEVER  = 1,
  TILING_NO     = 2,
  TILING_MAYBE  = 3,
  TILING_YES    = 4,
  TILING_ALWAYS = 5
};


Canvas *       canvas_new            (Tag, int w, int h, Tiling);
void           canvas_delete         (Canvas *);
Canvas *       canvas_clone          (Canvas *);

Tag            canvas_tag            (Canvas *);
Precision      canvas_precision      (Canvas *);
Format         canvas_format         (Canvas *);
Alpha          canvas_alpha          (Canvas *);
Tiling         canvas_tiling         (Canvas *);

Precision      canvas_set_precision  (Canvas *, Precision);
Format         canvas_set_format     (Canvas *, Format);
Alpha          canvas_set_alpha      (Canvas *, Alpha);
Tiling         canvas_set_tiling     (Canvas *, Tiling);

int            canvas_width          (Canvas *);
int            canvas_height         (Canvas *);

guint          canvas_ref            (Canvas *, int x, int y);
void           canvas_unref          (Canvas *, int x, int y);
void           canvas_init           (Canvas *, Canvas *, int x, int y);
guchar *       canvas_data           (Canvas *, int x, int y);
int            canvas_rowstride      (Canvas *, int x, int y);
int            canvas_portion_width  (Canvas *, int x, int y);
int            canvas_portion_height (Canvas *, int x, int y);




/* temporary conversions */
struct _temp_buf;
struct _TileManager;
Canvas *               canvas_from_tb (struct _temp_buf *);
Canvas *               canvas_from_tm (struct _TileManager *);
struct _temp_buf *     canvas_to_tb   (Canvas *);
struct _TileManager *  canvas_to_tm   (Canvas *);


#endif /* __CANVAS_H__ */
