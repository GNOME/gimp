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


/* what sort of physical representation to use */
typedef struct _Canvas Canvas;

typedef enum _Storage Storage;
enum _Storage
{
  STORAGE_NONE   = 0,
  STORAGE_FLAT   = 1,
  STORAGE_TILED  = 2,
  STORAGE_SHM    = 3
};


/* should a ref of an unalloced portion automatically allocate the
   memory or simply fail */
typedef enum _AutoAlloc AutoAlloc;
enum _AutoAlloc
{
  AUTOALLOC_NONE = 0,
  AUTOALLOC_OFF  = 1,
  AUTOALLOC_ON   = 2
};


/* the result of referencing a portion.  a successful ref means the
   data pointer is non-null */
typedef enum _RefRC RefRC;
enum _RefRC
{
  REFRC_NONE   = 0,
  REFRC_OK     = 1,
  REFRC_FAIL   = 2
};


Canvas *       canvas_new            (Tag, int w, int h, Storage);
void           canvas_delete         (Canvas *);
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

/* allocate and/or swap in the backing store for this pixel */
RefRC          canvas_portion_refro     (Canvas *, int x, int y);
RefRC          canvas_portion_refrw     (Canvas *, int x, int y);
RefRC          canvas_portion_unref     (Canvas *, int x, int y);

/* initialize the backing store for this pixel */
typedef guint (*CanvasInitFunc) (Canvas *, int, int, void *);
void           canvas_portion_init_setup (Canvas *, CanvasInitFunc, void *);
guint          canvas_portion_init       (Canvas *, int x, int y);



/* FIXME FIXME FIXME */
int canvas_fixme_getx (Canvas *);
int canvas_fixme_gety (Canvas *);
int canvas_fixme_setx (Canvas *, int);
int canvas_fixme_sety (Canvas *, int);

#endif /* __CANVAS_H__ */
