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


  int      valid;
  int      ref_count;
  void *   data;
};



FlatBuf *
flatbuf_new (
             Tag  tag,
             int  w,
             int  h
             )
{
  FlatBuf *f;
  
  f = (FlatBuf *) g_malloc (sizeof (FlatBuf));

  f->tag = tag;
  f->width = w;
  f->height = h;

  f->valid = FALSE;
  f->ref_count = 0;
  f->data = NULL;

  return f;
}


void
flatbuf_delete (
                FlatBuf * f
                )
{
  if (f)
    {
      if (f->data)
        g_free (f->data);

      g_free (f);
    }
}


FlatBuf *
flatbuf_clone (
               FlatBuf * f
               )
{
  FlatBuf *newf = NULL;

  if (f)
    {
      newf = (FlatBuf *) g_malloc (sizeof (FlatBuf));

      newf->tag = f->tag;
      newf->width = f->width;
      newf->height = f->height;

      newf->valid = FALSE;
      newf->ref_count = 0;
      newf->data = NULL;

      if (f->valid == TRUE)
        {
          flatbuf_ref (newf, 0, 0);
          flatbuf_ref (f, 0, 0);
          memcpy (newf->data, f->data, f->width * f->height * tag_bytes(f->tag));
          flatbuf_unref (f, 0, 0);
          flatbuf_unref (newf, 0, 0);
        }
    }
  
  return newf;
}


void
flatbuf_info (
              FlatBuf * f
              )
{
  if (f)
    {
      trace_begin ("Flatbuf 0x%x", f);
      trace_printf ("%d by %d flat buffer", f->width, f->height);
      trace_printf ("%s %s %s",
                    tag_string_precision (tag_precision (flatbuf_tag (f))),
                    tag_string_format (tag_format (flatbuf_tag (f))),
                    tag_string_alpha (tag_alpha (flatbuf_tag (f))));
      trace_printf ("ref %d for 0x%x", f->ref_count, f->data);
      trace_end ();
    }
}


Tag
flatbuf_tag (
             FlatBuf * f
             )
{
  if (f)
    return f->tag;
  return tag_null ();
}


Precision
flatbuf_precision (
                   FlatBuf * f
                   )
{
  return tag_precision (flatbuf_tag (f));
}


Format
flatbuf_format (
                FlatBuf * f
                )
{
  return tag_format (flatbuf_tag (f));
}


Alpha
flatbuf_alpha (
               FlatBuf * f
               )
{
  return tag_alpha (flatbuf_tag (f));
}


Precision
flatbuf_set_precision (
                       FlatBuf * f,
                       Precision x
                       )
{
  g_warning ("finish writing flatbuf_set_precision()");
  return flatbuf_precision (f);
}


Format
flatbuf_set_format (
                    FlatBuf * f,
                    Format    x
                    )
{
  g_warning ("finish writing flatbuf_set_format()");
  return flatbuf_format (f);
}


Alpha
flatbuf_set_alpha (
                   FlatBuf * f,
                   Alpha     x
                   )
{
  g_warning ("finish writing flatbuf_set_alpha()");
  return flatbuf_alpha (f);
}


guint 
flatbuf_width  (
                FlatBuf * f
                )
{
  if (f)
    return f->width;
  return 0;
}


guint 
flatbuf_height  (
                 FlatBuf * f
                 )
{
  if (f)
    return f->height;
  return 0;
}


guint
flatbuf_ref  (
              FlatBuf * f,
              int x,
              int y
              )
{
  guint rc = FALSE;
  
  if (f)
    {
      f->ref_count++;
      if ((f->ref_count == 1) && (f->data == NULL))
        {
          if (f->valid == TRUE)
            {
              /* swap buffer in */
            }
          else
            {
              /* alloc buffer */
              int n = tag_bytes (flatbuf_tag (f)) * f->width * f->height;
              f->valid = TRUE;
              f->data = g_malloc (n);
              memset (f->data, 0, n);
              rc = TRUE;
            }
        }
    }
  return rc;
}


void 
flatbuf_unref  (
                FlatBuf * f,
                int x,
                int y
                )
{
  if (f)
    {
      f->ref_count--;
      if (f->ref_count == 0)
        {
          /* swap out */
        }
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
  if (f
      && src
      && (f->width == src->width)
      && (f->height == src->height)
      && tag_equal (f->tag, src->tag))
    {
      flatbuf_ref (f, 0, 0);
      flatbuf_ref (src, 0, 0);
      memcpy (f->data, src->data, f->width * f->height * tag_bytes (f->tag));
      flatbuf_unref (src, 0, 0);
      flatbuf_unref (f, 0, 0);
    }
}


guchar *
flatbuf_data (
              FlatBuf * f,
              int x,
              int y
              )
{
  if (f && (x < f->width) && (y < f->height))
    return (guchar*)f->data + ((y * f->width) + x) * tag_bytes (f->tag);
  return NULL;
}


guint 
flatbuf_rowstride  (
                    FlatBuf * f,
                    int x,
                    int y
                    )
{
  if (f && (x < f->width) && (y < f->height))
    return f->width * tag_bytes (f->tag);
  return 0;
}


guint 
flatbuf_portion_width  (
                        FlatBuf * f,
                        int x,
                        int y
                        )
{
  if (f && (x < f->width) && (y < f->height))
    return f->width - x;
  return 0;
}


guint 
flatbuf_portion_height  (
                         FlatBuf * f,
                         int x,
                         int y
                         )
{
  if (f && (x < f->width) && (y < f->height))
    return f->height - y;
  return 0;
}










#include "temp_buf.h" /* temporary */

void
flatbuf_to_tb (
               FlatBuf * fb,
               TempBuf * tb
               )
{
  if (fb && tb)
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
}


void
flatbuf_from_tb (
                 FlatBuf * fb,
                 TempBuf * tb
                 )
{
  if (fb && tb)
    {
      fb->tag       = tag_by_bytes (tb->bytes);
      fb->width     = tb->width;
      fb->height    = tb->height;
      fb->valid     = FALSE;
      fb->ref_count = 0;
      fb->data      = NULL;
      {
        flatbuf_ref (fb, 0, 0);
        memcpy (fb->data, tb->data, tb->bytes * tb->width * tb->height);
        flatbuf_unref (fb, 0, 0);
      }
    }
}
