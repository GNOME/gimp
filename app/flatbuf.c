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

#include <string.h>

#include "flatbuf.h"


struct _FlatBuf
{
  Tag      tag;
  int      width;
  int      height;
  void *   data;

  int      refcount;
  int      swapped;
  char *   filename;
};



FlatBuf *
flatbuf_new (
             Tag  tag,
             int  w,
             int  h
             )
{
  FlatBuf *f;
  guint   bytes;
  
  bytes = tag_bytes (tag);
  if (bytes == 0)
    return NULL;
  
  f = (FlatBuf *) g_malloc (sizeof (FlatBuf));

  f->tag = tag;
  f->width = w;
  f->height = h;
  f->data = g_malloc (w * h * bytes);
  memset (f->data, 0, w * h * bytes);

  /* for now, ref counts don;t mean anything, but could be used to
     control swapping */
  f->refcount = 1;
  f->swapped = FALSE;
  f->filename = NULL;

  return f;
}


void
flatbuf_delete (
                FlatBuf * f
                )
{
  if (f->data)
    g_free (f->data);

  if (f->swapped)
    /* flatbuf_swap_free (f) */ ;

  g_free (f);
}


FlatBuf *
flatbuf_clone (
               FlatBuf * f
               )
{
  FlatBuf *newf;

  newf = (FlatBuf *) g_malloc (sizeof (FlatBuf));

  newf->tag = f->tag;
  newf->width = f->width;
  newf->height = f->height;

  newf->data = g_malloc (f->width * f->height * tag_bytes(f->tag));
  flatbuf_ref (f, 0, 0);
  memcpy (newf->data, f->data, f->width * f->height * tag_bytes(f->tag));
  flatbuf_unref (f, 0, 0);

  newf->refcount = 1;
  newf->swapped = FALSE;
  newf->filename = NULL;
  
  return newf;
}


Tag
flatbuf_tag (
             FlatBuf * f
             )
{
  return f->tag;
}


Precision
flatbuf_precision (
                   FlatBuf * f
                   )
{
  return tag_precision (f->tag);
}


Format
flatbuf_format (
                FlatBuf * f
                )
{
  return tag_format (f->tag);
}


Alpha
flatbuf_alpha (
               FlatBuf * f
               )
{
  return tag_alpha (f->tag);
}


Precision
flatbuf_set_precision (
                       FlatBuf * f,
                       Precision x
                       )
{
  /* WRITEME */
  return flatbuf_precision (f);
}


Format
flatbuf_set_format (
                    FlatBuf * f,
                    Format    x
                    )
{
  /* WRITEME */
  return flatbuf_format (f);
}


Alpha
flatbuf_set_alpha (
                   FlatBuf * f,
                   Alpha     x
                   )
{
  /* WRITEME */
  return flatbuf_alpha (f);
}


guint 
flatbuf_width  (
                FlatBuf * f
                )
{
  return f->width;
}


guint 
flatbuf_height  (
                 FlatBuf * f
                 )
{
  return f->height;
}


guint
flatbuf_ref  (
              FlatBuf * f,
              int x,
              int y
              )
{
  f->refcount++;
  if (f->refcount == 1)
    {
      /* swap in */
    }
  return FALSE;
}


void 
flatbuf_unref  (
                FlatBuf * f,
                int x,
                int y
                )
{
  f->refcount--;
  if (f->refcount == 0)
    {
      /* swap out */
    }
}


void 
flatbuf_init  (
               FlatBuf * f,
               FlatBuf * src,
               int x,
               int y
               )
{
  /* the only time new store is allocated is in flatbuf_new(), and it
     is init'ed there, so we have nothing to do */
}


guchar *
flatbuf_data (
              FlatBuf * f,
              int x,
              int y
              )
{
  if ((x >= f->width) ||
      (y >= f->height))
    {
      return NULL;
    }
  return (guchar*)f->data + ((y * f->width) + x) * tag_bytes (f->tag);
}


guint 
flatbuf_rowstride  (
                    FlatBuf * f,
                    int x,
                    int y
                    )
{
  if ((x >= f->width) ||
      (y >= f->height))
    {
      return 0;
    }
  return f->width * tag_bytes (f->tag);
}


guint 
flatbuf_portion_width  (
                        FlatBuf * f,
                        int x,
                        int y
                        )
{
  if ((f == NULL) ||
      (x >= f->width) ||
      (y >= f->height))
    {
      return 0;
    }
  return f->width - x;
}


guint 
flatbuf_portion_height  (
                         FlatBuf * f,
                         int x,
                         int y
                         )
{
  if ((f == NULL) ||
      (x >= f->width) ||
      (y >= f->height))
    {
      return 0;
    }
  return f->height - y;
}










#include "temp_buf.h" /* temporary */

void
flatbuf_to_tb (
               FlatBuf * fb,
               TempBuf * tb
               )
{
  tb->bytes     = tag_bytes (fb->tag);
  tb->width     = fb->width;
  tb->height    = fb->height;
  tb->x         = 0;
  tb->y         = 0;
  tb->swapped   = FALSE;
  tb->filename  = NULL;
  tb->data      = g_malloc (tb->bytes * tb->width * tb->height);
  memcpy (tb->data, fb->data, tb->bytes * tb->width * tb->height);
}


void
flatbuf_from_tb (
                 FlatBuf * fb,
                 TempBuf * tb
                 )
{
  fb->tag       = tag_by_bytes (tb->bytes);
  fb->width     = tb->width;
  fb->height    = tb->height;
  fb->refcount  = 1;
  fb->swapped   = FALSE;
  fb->filename  = NULL;
  fb->data      = g_malloc (tb->bytes * tb->width * tb->height);
  memcpy (fb->data, tb->data, tb->bytes * tb->width * tb->height);
}
