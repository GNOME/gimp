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

#include "canvas.h"
#include "flatbuf.h"
#include "tilebuf.h"
#include "trace.h"


struct _Canvas
{
  Storage    storage;
  AutoAlloc  autoalloc;
  TileBuf  * tile_data;
  FlatBuf  * flat_data;

  Tag   tag;
  int   bytes;
  int   width;
  int   height;
};



Canvas *
canvas_new (
            Tag tag,
            int w,
            int h,
            Storage storage
            )
{
  Canvas *c;

  c = (Canvas *) g_malloc (sizeof (Canvas));

  c->storage = storage;
  c->autoalloc = AUTOALLOC_ON;
  
  c->tag    = tag;
  c->bytes  = tag_bytes (tag);
  c->width  = w;
  c->height = h;

  switch (storage)
    {
    case STORAGE_FLAT:
      c->tile_data = NULL;
      c->flat_data = flatbuf_new (tag, w, h, c);
      break;
    case STORAGE_TILED:
      c->flat_data = NULL;
      c->tile_data = tilebuf_new (tag, w, h, c);
      break;
    default:
      g_free (c);
      return NULL;
    }

  return c;
}


void 
canvas_delete  (
                Canvas * c
                )
{
  if (c)
    {
      if (c->tile_data)
        tilebuf_delete (c->tile_data);
      if (c->flat_data)
        flatbuf_delete (c->flat_data);
      g_free (c);
    }
}


void
canvas_info (
             Canvas *c
             )
{
  if (c)
    {
      trace_begin ("Canvas 0x%x", c);
      trace_printf (c->autoalloc==AUTOALLOC_ON ? "AutoAlloc" : "No AutoAlloc" );
      switch (c->storage)
        {
        case STORAGE_TILED:
          tilebuf_info (c->tile_data);
          break;
        case STORAGE_FLAT:
          flatbuf_info (c->flat_data);
          break;
        default:
          trace_printf ("Unknown image data");
          break;
        }
      trace_end ();
    }
}


Tag 
canvas_tag  (
             Canvas * c
             )
{
  if (c)
    return c->tag;

  return tag_null ();
}


Precision 
canvas_precision  (
                   Canvas * c
                   )
{
  return ( c ? tag_precision (c->tag) : PRECISION_NONE);
}


Format 
canvas_format  (
                Canvas * c
                )
{
  return ( c ? tag_format (c->tag) : FORMAT_NONE);
}


Alpha 
canvas_alpha  (
               Canvas * c
               )
{
  return ( c ? tag_alpha (c->tag) : ALPHA_NONE);
}


Storage 
canvas_storage  (
                 Canvas * c
                 )
{
  return ( c ? c->storage : STORAGE_NONE);
}


AutoAlloc
canvas_autoalloc (
                  Canvas * c
                  )
{
  return ( c ? c->autoalloc : AUTOALLOC_NONE);
}


AutoAlloc
canvas_set_autoalloc (
                      Canvas * c,
                      AutoAlloc aa
                      )
{
  if (c)
    switch (aa)
      {
      case AUTOALLOC_OFF:
      case AUTOALLOC_ON:
        c->autoalloc = aa;

      default:
        break;
      }
  return canvas_autoalloc (c);
}


int 
canvas_width  (
               Canvas * c
               )
{
  return ( c ? c->width : 0);
}


int 
canvas_height  (
                Canvas * c
                )
{
  return ( c ? c->height : 0);
}


int 
canvas_bytes  (
               Canvas * c
               )
{
  return ( c ? c->bytes : 0);
}




/* debugging counters */
int ref_ro = 0;
int ref_rw = 0;
int ref_un = 0;

int ref_fa = 0;
int ref_uf = 0;

RefRC 
canvas_portion_ref  (
                     Canvas * c,
                     int x,
                     int y
                     )
{
  RefRC rc = REFRC_FAIL;
  
  if (c)
    {
      if (c->tile_data)
        rc = tilebuf_portion_ref (c->tile_data, x, y);
      else if (c->flat_data)
        rc = flatbuf_portion_ref (c->flat_data, x, y);
    }
  
  switch (rc)
    {
    case REFRC_OK:
      ref_ro++;
      break;
    case REFRC_FAIL:
    default:
      ref_fa++;
      break;
    }
  return rc;
}


RefRC 
canvas_portion_refrw  (
                       Canvas * c,
                       int x,
                       int y
                       )
{
  RefRC rc = REFRC_FAIL;
  
  if (c)
    {
      if (c->tile_data)
        rc = tilebuf_portion_refrw (c->tile_data, x, y);
      else if (c->flat_data)
        rc = flatbuf_portion_refrw (c->flat_data, x, y);
    }
  
  switch (rc)
    {
    case REFRC_OK:
      ref_rw++;
      break;
    case REFRC_FAIL:
    default:
      ref_fa++;
      break;
    }
  return rc;
}


RefRC 
canvas_portion_unref  (
                       Canvas * c,
                       int x,
                       int y
                       )
{
  RefRC rc = REFRC_FAIL;
  
  if (c)
    {
      if (c->tile_data)
        rc = tilebuf_portion_unref (c->tile_data, x, y);
      else if (c->flat_data)
        rc = flatbuf_portion_unref (c->flat_data, x, y);
    }
  
  switch (rc)
    {
    case REFRC_OK:
      ref_un++;
      break;
    case REFRC_FAIL:
    default:
      ref_uf++;
      break;
    }

  return rc;
}


guint 
canvas_portion_width  (
                       Canvas * c,
                       int x,
                       int y
                       )
{
  if (c)
    if (c->tile_data)
      return tilebuf_portion_width (c->tile_data, x, y);
    else if (c->flat_data)
      return flatbuf_portion_width (c->flat_data, x, y);

  return 0;
}


guint 
canvas_portion_height  (
                        Canvas * c,
                        int x,
                        int y
                        )
{
  if (c)
    if (c->tile_data)
      return tilebuf_portion_height (c->tile_data, x, y);
    else if (c->flat_data)
      return flatbuf_portion_height (c->flat_data, x, y);

  return 0;
}


guint 
canvas_portion_y  (
                   Canvas * c,
                   int x,
                   int y
                   )
{
  if (c)
    if (c->tile_data)
      return tilebuf_portion_y (c->tile_data, x, y);
    else if (c->flat_data)
      return flatbuf_portion_y (c->flat_data, x, y);

  return 0;
}


guint 
canvas_portion_x  (
                   Canvas * c,
                   int x,
                   int y
                   )
{
  if (c)
    if (c->tile_data)
      return tilebuf_portion_x (c->tile_data, x, y);
    else if (c->flat_data)
      return flatbuf_portion_x (c->flat_data, x, y);

  return 0;
}


guchar * 
canvas_portion_data  (
                      Canvas * c,
                      int x,
                      int y
                      )
{
  if (c)
    if (c->tile_data)
      return tilebuf_portion_data (c->tile_data, x, y);
    else if (c->flat_data)
      return flatbuf_portion_data (c->flat_data, x, y);

  return NULL;
}


guint 
canvas_portion_rowstride  (
                           Canvas * c,
                           int x,
                           int y
                           )
{
  if (c)
    if (c->tile_data)
      return tilebuf_portion_rowstride (c->tile_data, x, y);
    else if (c->flat_data)
      return flatbuf_portion_rowstride (c->flat_data, x, y);

  return 0;
}


guint 
canvas_portion_alloced  (
                         Canvas * c,
                         int x,
                         int y
                         )
{
  if (c)
    if (c->tile_data)
      return tilebuf_portion_alloced (c->tile_data, x, y);
    else if (c->flat_data)
      return flatbuf_portion_alloced (c->flat_data, x, y);

  return FALSE;
}


guint 
canvas_portion_alloc  (
                       Canvas * c,
                       int x,
                       int y
                       )
{
  if (c)
    if (c->tile_data)
      return tilebuf_portion_alloc (c->tile_data, x, y);
    else if (c->flat_data)
      return flatbuf_portion_alloc (c->flat_data, x, y);

  return FALSE;
}


guint 
canvas_portion_unalloc  (
                         Canvas * c,
                         int x,
                         int y
                         )
{
  if (c)
    if (c->tile_data)
      return tilebuf_portion_unalloc (c->tile_data, x, y);
    else if (c->flat_data)
      return flatbuf_portion_unalloc (c->flat_data, x, y);

  return FALSE;
}


/*
   FIXME:

   this doesn;t work right for tiles.  edge tiles only get the
   "on-image" portion inited.  the part that lies off the image isn;t
   inited because the potion_width and height ignore it.

   the proper fix for this is to split portion out of canvas and deal
   only with tiles at the canvas level.  for now we just memset the
   chunk to zero in portion_alloc */

guint 
canvas_portion_init  (
                      Canvas * c,
                      int x,
                      int y
                      )
{
  if (c)
    {
      guint xx = canvas_portion_x (c, x, y);
      guint yy = canvas_portion_y (c, x, y);
      guint ww = canvas_portion_width (c, xx, yy);
      guint hh = canvas_portion_height (c, xx, yy);
      guint nn = ww * hh * canvas_bytes (c);
      guchar * dd = canvas_portion_data (c, xx, yy);

      if (dd != NULL)
        {
          memset (dd, 0, nn);
          return TRUE;
        }
    }
  return FALSE;
}
