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
#ifndef __SHMBUF_H__
#define __SHMBUF_H__

#include "tag.h"
#include "canvas.h"

struct _ProcRecord;


typedef struct _ShmBuf ShmBuf;


ShmBuf *       shmbuf_new            (Tag, int w, int h, Canvas *);
void           shmbuf_delete         (ShmBuf *);
void           shmbuf_info           (ShmBuf *);

Tag            shmbuf_tag            (ShmBuf *);
Precision      shmbuf_precision      (ShmBuf *);
Format         shmbuf_format         (ShmBuf *);
Alpha          shmbuf_alpha          (ShmBuf *);

guint          shmbuf_width          (ShmBuf *);
guint          shmbuf_height         (ShmBuf *);




guint          shmbuf_portion_x         (ShmBuf *, int x, int y);
guint          shmbuf_portion_y         (ShmBuf *, int x, int y);
guint          shmbuf_portion_width     (ShmBuf *, int x, int y);
guint          shmbuf_portion_height    (ShmBuf *, int x, int y);

guchar *       shmbuf_portion_data      (ShmBuf *, int x, int y);
guint          shmbuf_portion_rowstride (ShmBuf *, int x, int y);

guint          shmbuf_portion_alloced   (ShmBuf *, int x, int y);
guint          shmbuf_portion_alloc     (ShmBuf *, int x, int y);
guint          shmbuf_portion_unalloc   (ShmBuf *, int x, int y);

RefRC          shmbuf_portion_refro       (ShmBuf *, int x, int y);
RefRC          shmbuf_portion_refrw     (ShmBuf *, int x, int y);
RefRC          shmbuf_portion_unref     (ShmBuf *, int x, int y);


extern struct _ProcRecord shmseg_new_proc;
extern struct _ProcRecord shmseg_delete_proc;

extern struct _ProcRecord shmseg_attach_proc;
extern struct _ProcRecord shmseg_detach_proc;
extern struct _ProcRecord shmseg_status_proc;

extern struct _ProcRecord shmbuf_new_proc;
extern struct _ProcRecord shmbuf_delete_proc;

#endif /* __SHMBUF_H__ */
