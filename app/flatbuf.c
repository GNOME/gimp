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
  Canvas * canvas;

  int      valid;
  int      ref_count;
  void *   data;

  int      bytes;
};



FlatBuf *
flatbuf_new (
             Tag  tag,
             int  w,
             int  h,
             Canvas * c
             )
{
  FlatBuf *f;
  
  f = (FlatBuf *) g_malloc (sizeof (FlatBuf));

  f->tag = tag;
  f->width = w;
  f->height = h;
  f->canvas = c;
  
  f->valid = FALSE;
  f->ref_count = 0;
  f->data = NULL;

  f->bytes = tag_bytes (tag);
  
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









RefRC 
flatbuf_portion_refro  (
                        FlatBuf * f,
                        int x,
                        int y
                        )
{
  RefRC rc = REFRC_FAIL;
  
  if (f && (x < f->width) && (y < f->height))
    {
      if (f->valid == FALSE)
        if (canvas_autoalloc (f->canvas) == AUTOALLOC_ON)
          (void) flatbuf_portion_alloc (f, x, y);
      
      if (f->valid == TRUE)
        {
          f->ref_count++;
          rc = REFRC_OK;
        }
    }

  return rc;
}


RefRC 
flatbuf_portion_refrw  (
                        FlatBuf * f,
                        int x,
                        int y
                        )
{
  RefRC rc = REFRC_FAIL;
  
  if (f && (x < f->width) && (y < f->height))
    {
      if (f->valid == FALSE)
        if (canvas_autoalloc (f->canvas) == AUTOALLOC_ON)
          (void) flatbuf_portion_alloc (f, x, y);
      
      if (f->valid == TRUE)
        {
          f->ref_count++;
          rc = REFRC_OK;
        }
    }

  return rc;
}


RefRC 
flatbuf_portion_unref  (
                        FlatBuf * f,
                        int x,
                        int y
                        )
{
  RefRC rc = REFRC_FAIL;

  if (f && (x < f->width) && (y < f->height))
    {
      if (f->ref_count > 0)
        f->ref_count--;
      else
        g_warning ("flatbuf unreffing with ref_count==0");

      rc = REFRC_OK;
    }
  
  return rc;
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
flatbuf_portion_y  (
                    FlatBuf * f,
                    int x,
                    int y
                    )
{
  /* always 0 */
  return 0;
}


guint 
flatbuf_portion_x  (
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
        return (guchar*)f->data + ((y * f->width) + x) * f->bytes;
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
    return f->width * f->bytes;
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
      if (f->valid == TRUE)
        {
          return TRUE;
        }
      else
        {
          int n = f->bytes * f->width * f->height;
          f->data = g_malloc (n);
          if (f->data)
            {
              memset (f->data, 0, n);
              f->valid = TRUE;
              if (canvas_portion_init (f->canvas, x, y) != TRUE)
                g_warning ("flatbuf failed to init portion...");
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


