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

typedef enum _Storage Storage;
enum _Storage
{
  STORAGE_NONE   = 0,
  STORAGE_FLAT   = 1,
  STORAGE_TILED  = 2,
};


Canvas *       canvas_new            (Tag, int w, int h, Storage);
void           canvas_delete         (Canvas *);
Canvas *       canvas_clone          (Canvas *);
void           canvas_info           (Canvas *);

Tag            canvas_tag            (Canvas *);
Precision      canvas_precision      (Canvas *);
Format         canvas_format         (Canvas *);
Alpha          canvas_alpha          (Canvas *);
Storage        canvas_storage        (Canvas *);

int            canvas_autoalloc      (Canvas *);
int            canvas_set_autoalloc  (Canvas *, int);

int            canvas_width          (Canvas *);
int            canvas_height         (Canvas *);


/* a portion is a rectangular area of a Canvas with identical
   properties.  at the moment, the only defined property is the
   backing store.  ie: a portion is a section of Canvas that all fits
   on a single tile.  in the future, other properties like
   'initialized' may be added */

guint          canvas_portion_ref       (Canvas *, int x, int y);
void           canvas_portion_unref     (Canvas *, int x, int y);

guint          canvas_portion_width     (Canvas *, int x, int y);
guint          canvas_portion_height    (Canvas *, int x, int y);
guint          canvas_portion_top       (Canvas *, int x, int y);
guint          canvas_portion_left      (Canvas *, int x, int y);

guchar *       canvas_portion_data      (Canvas *, int x, int y);
guint          canvas_portion_rowstride (Canvas *, int x, int y);

guint          canvas_portion_alloced   (Canvas *, int x, int y);
guint          canvas_portion_alloc     (Canvas *, int x, int y);
guint          canvas_portion_unalloc   (Canvas *, int x, int y);



/* temporary evil */
void canvas_init (Canvas *, Canvas *, int x, int y);

#endif /* __CANVAS_H__ */
