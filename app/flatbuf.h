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
#ifndef __FLATBUF_H__
#define __FLATBUF_H__

#include "glib.h"
#include "tag.h"


typedef struct _FlatBuf FlatBuf;


FlatBuf *      flatbuf_new            (Tag, int w, int h);
void           flatbuf_delete         (FlatBuf *);
FlatBuf *      flatbuf_clone          (FlatBuf *);

Tag            flatbuf_tag            (FlatBuf *);
Precision      flatbuf_precision      (FlatBuf *);
Format         flatbuf_format         (FlatBuf *);
Alpha          flatbuf_alpha          (FlatBuf *);

Precision      flatbuf_set_precision  (FlatBuf *, Precision);
Format         flatbuf_set_format     (FlatBuf *, Format);
Alpha          flatbuf_set_alpha      (FlatBuf *, Alpha);

guint          flatbuf_width          (FlatBuf *);
guint          flatbuf_height         (FlatBuf *);

guint          flatbuf_ref            (FlatBuf *, int x, int y);
void           flatbuf_unref          (FlatBuf *, int x, int y);
void           flatbuf_init           (FlatBuf *, FlatBuf *, int x, int y);
guchar *       flatbuf_data           (FlatBuf *, int x, int y);
guint          flatbuf_rowstride      (FlatBuf *, int x, int y);
guint          flatbuf_portion_width  (FlatBuf *, int x, int y);
guint          flatbuf_portion_height (FlatBuf *, int x, int y);



/* copy to and from TempBuf */
struct _temp_buf;
void           flatbuf_to_tb          (FlatBuf *, struct _temp_buf *);
void           flatbuf_from_tb        (FlatBuf *, struct _temp_buf *);

#endif /* __FLATBUF_H__ */
