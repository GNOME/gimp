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
#include "trace.h"


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
      flatbuf_portion_unalloc (f, 0, 0);
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

      g_warning ("finish writing flatbuf_clone()");
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
flatbuf_portion_ref  (
                      FlatBuf * f,
                      int x,
                      int y
                      )
{
  guint rc = FALSE;
  
  if (f && (x < f->width) && (y < f->height))
    {
      f->ref_count++;
      if (f->data == NULL)
        {
          if (f->valid == TRUE)
            {
              /* swap buffer in */
            }
          else
            {
              /* user action required to alloc and init data buffer */
              rc = TRUE;
            }
        }
    }

  return rc;
}


void 
flatbuf_portion_unref  (
                        FlatBuf * f,
                        int x,
                        int y
                        )
{
  if (f && (x < f->width) && (y < f->height))
    {
      f->ref_count--;
      if ((f->ref_count == 0) &&
          (f->valid == TRUE))
        {
          /* swap buffer out */
        }
    }
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


guint 
flatbuf_portion_top  (
                      FlatBuf * f,
                      int x,
                      int y
                      )
{
  /* always 0 */
  return 0;
}


guint 
flatbuf_portion_left  (
                       FlatBuf * f,
                       int x,
                       int y
                       )
{
  /* always 0 */
  return 0;
}


guchar * 
flatbuf_portion_data  (
                       FlatBuf * f,
                       int x,
                       int y
                       )
{
  if (f && (x < f->width) && (y < f->height))
    {
      if (f->data)
        return (guchar*)f->data + ((y * f->width) + x) * tag_bytes (f->tag);
    }
  return NULL;
}


guint 
flatbuf_portion_rowstride  (
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
flatbuf_portion_alloced (
                         FlatBuf * f,
                         int x,
                         int y
                         )
{
  if (f && (x < f->width) && (y < f->height))
    return f->valid;
  return FALSE;
}


guint 
flatbuf_portion_alloc  (
                        FlatBuf * f,
                        int x,
                        int y
                        )
{
  if (f && (x < f->width) && (y < f->height))
    {
      if (f->valid == FALSE)
        {
          int n = tag_bytes (flatbuf_tag (f)) * f->width * f->height;
          f->data = g_malloc (n);
          if (f->data)
            {
              f->valid = TRUE;
              memset (f->data, 0, n);
              return TRUE;
            }
        }
    }
  return FALSE;
}


guint 
flatbuf_portion_unalloc  (
                          FlatBuf * f,
                          int x,
                          int y
                          )
{
  if (f && (x < f->width) && (y < f->height))
    {
      if ((f->valid == TRUE) && (f->ref_count == 0))
        {
          g_free (f->data);
          f->data = NULL;
          f->valid = FALSE;
          return TRUE;
        }
    }
  return FALSE;
}





/* temporary evil */
void 
flatbuf_init  (
               FlatBuf * f,
               FlatBuf * src,
               int x,
               int y
               )
{
  if (f && src
      && (f->width == src->width)
      && (f->height == src->height)
      && (x < f->width)
      && (y < f->height)
      && tag_equal (f->tag, src->tag))
    {
      flatbuf_portion_ref (f, 0, 0);
      flatbuf_portion_ref (src, 0, 0);
      memcpy (f->data, src->data, f->width * f->height * tag_bytes (f->tag));
      flatbuf_portion_unref (src, 0, 0);
      flatbuf_portion_unref (f, 0, 0);
    }
}

