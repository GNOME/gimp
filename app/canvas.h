
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

typedef enum _AutoAlloc AutoAlloc;
enum _AutoAlloc
{
  AUTOALLOC_NONE = 0,
  AUTOALLOC_OFF  = 1,
  AUTOALLOC_ON   = 2
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
AutoAlloc      canvas_autoalloc      (Canvas *);
AutoAlloc      canvas_set_autoalloc  (Canvas *, AutoAlloc);

int            canvas_width          (Canvas *);
int            canvas_height         (Canvas *);
int            canvas_bytes          (Canvas *);


/* a portion is a rectangular area of a Canvas that resides on a
   single underlying chunk of memory (eg: a tile) */

/* allocate and/or swap in the backing store for this pixel */
guint          canvas_portion_ref       (Canvas *, int x, int y);
void           canvas_portion_unref     (Canvas *, int x, int y);

/* return the TOP LEFT coordinate of the tile this pixel lies on */
guint          canvas_portion_x         (Canvas *, int x, int y);
guint          canvas_portion_y         (Canvas *, int x, int y);

/* return the maximum width and height of the rectangle that has the
   specified TOP LEFT pixel AND is contained on a single tile */
guint          canvas_portion_width     (Canvas *, int x, int y);
guint          canvas_portion_height    (Canvas *, int x, int y);

/* get the data pointer and rowstride for the rectangle with the
   specified TOP LEFT corner */
guchar *       canvas_portion_data      (Canvas *, int x, int y);
guint          canvas_portion_rowstride (Canvas *, int x, int y);

/* check if a pixel needs to have memory allocated, alloc and dealloc
   the memory */
guint          canvas_portion_alloced   (Canvas *, int x, int y);
guint          canvas_portion_alloc     (Canvas *, int x, int y);
guint          canvas_portion_unalloc   (Canvas *, int x, int y);


#endif /* __CANVAS_H__ */
