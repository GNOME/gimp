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

#include "canvas.h"
#include "flatbuf.h"
#include "tilebuf.h"

#include "temp_buf.h"  /* temporary */
#include "tile_manager.h"  /* temporary */


struct _Canvas
{
  Tiling     tiling;
  TileBuf  * tile_data;
  FlatBuf  * flat_data;
};



Canvas *
canvas_new (
            Tag tag,
            int w,
            int h,
            Tiling tiling
            )
{
  Canvas *c;

  c = (Canvas *) g_malloc (sizeof (Canvas));

  c->tiling = tiling;

  if (tiling == TILING_MAYBE)
    {
      if ((w * h * tag_bytes (tag)) < (1024 * 1024 * 4))
        tiling = TILING_NO;
      else
        tiling = TILING_YES;
    }
  
  switch (tiling)
    {
    case TILING_NO:
    case TILING_NEVER:
      c->tile_data = NULL;
      c->flat_data = flatbuf_new (tag, w, h);
      break;
    case TILING_YES:
    case TILING_ALWAYS:
      c->flat_data = NULL;
      c->tile_data = tilebuf_new (tag, w, h);
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


Canvas * 
canvas_clone  (
               Canvas * c
               )
{
  if (c)
    {
      Canvas *new_c;

      new_c = (Canvas *) g_malloc (sizeof (Canvas));

      new_c->tiling = c->tiling;
      if (c->tile_data)
        new_c->tile_data = tilebuf_clone (c->tile_data);
      if (c->flat_data)
        new_c->flat_data = flatbuf_clone (c->flat_data);
      
      return new_c;
    }
  return NULL;
}


Tag 
canvas_tag  (
             Canvas * c
             )
{
  if (c)
    if (c->tile_data)
      return tilebuf_tag (c->tile_data);
    else if (c->flat_data)
      return flatbuf_tag (c->flat_data);

  return tag_new (PRECISION_NONE, FORMAT_NONE, ALPHA_NONE);
}


Precision 
canvas_precision  (
                   Canvas * c
                   )
{
  return tag_precision (canvas_tag (c));
}


Format 
canvas_format  (
                Canvas * c
                )
{
  return tag_format (canvas_tag (c));
}


Alpha 
canvas_alpha  (
               Canvas * c
               )
{
  return tag_alpha (canvas_tag (c));
}


Tiling 
canvas_tiling  (
                Canvas * c
                )
{
  if (c)
    return c->tiling;
  return TILING_NONE;
}


Precision 
canvas_set_precision  (
                       Canvas * c,
                       Precision p
                       )
{
  /* WRITEME */
  return canvas_precision (c);
}


Format 
canvas_set_format  (
                    Canvas * c,
                    Format f
                    )
{
  /* WRITEME */
  return canvas_format (c);
}


Alpha 
canvas_set_alpha  (
                   Canvas * c,
                   Alpha a
                   )
{
  /* WRITEME */
  return canvas_alpha (c);
}


Tiling 
canvas_set_tiling  (
                    Canvas * c,
                    Tiling t
                    )
{
  /* WRITEME */
  return canvas_tiling (c);
}


int 
canvas_width  (
               Canvas * c
               )
{
  if (c)
    if (c->tile_data)
      return tilebuf_width (c->tile_data);
    else if (c->flat_data)
      return flatbuf_width (c->flat_data);

  return 0;
}


int 
canvas_height  (
                Canvas * c
                )
{
  if (c)
    if (c->tile_data)
      return tilebuf_height (c->tile_data);
    else if (c->flat_data)
      return flatbuf_height (c->flat_data);

  return 0;
}


guint
canvas_ref  (
             Canvas * c,
             int x,
             int y
             )
{
  if (c)
    if (c->tile_data)
      return tilebuf_ref (c->tile_data, x, y);
    else if (c->flat_data)
      return flatbuf_ref (c->flat_data, x, y);

  return FALSE;
}


void 
canvas_unref  (
               Canvas * c,
               int x,
               int y
               )
{
  if (c)
    if (c->tile_data)
      tilebuf_unref (c->tile_data, x, y);
    else if (c->flat_data)
      flatbuf_unref (c->flat_data, x, y);
}


void
canvas_init  (
              Canvas * c,
              Canvas * src,
              int x,
              int y
              )
{
  if (c && src)
    if ((c->tile_data) && (src->tile_data))
      tilebuf_init (c->tile_data, src->tile_data, x, y);
    else if ((c->flat_data) && (src->flat_data))
      flatbuf_init (c->flat_data, src->flat_data, x, y);
}


guchar * 
canvas_data  (
              Canvas * c,
              int x,
              int y
              )
{
  if (c)
    if (c->tile_data)
      return tilebuf_data (c->tile_data, x, y);
    else if (c->flat_data)
      return flatbuf_data (c->flat_data, x, y);

  return NULL;
}


int 
canvas_rowstride  (
                   Canvas * c,
                   int x,
                   int y
                   )
{
  if (c)
    if (c->tile_data)
      return tilebuf_rowstride (c->tile_data, x, y);
    else if (c->flat_data)
      return flatbuf_rowstride (c->flat_data, x, y);

  return 0;
}


int 
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


int 
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



/* temporary conversion routines */
Canvas *
canvas_from_tb (
                TempBuf *tb
                )
{
  Canvas *c;
  
  c = canvas_new (tag_by_bytes (tb->bytes),
                  tb->width,
                  tb->height,
                  TILING_NEVER);

  flatbuf_from_tb (c->flat_data, tb);

  return c;
}


TempBuf *
canvas_to_tb (
              Canvas *c
              )
{
  TempBuf *tb;

  tb = temp_buf_new (canvas_width (c),
                     canvas_height (c),
                     tag_bytes (canvas_tag (c)),
                     0, 0, NULL);
  
  flatbuf_to_tb (c->flat_data, tb);

  return tb;
}


Canvas *
canvas_from_tm (
                TileManager *tm
                )
{
  Canvas *c;

  c = canvas_new (tag_by_bytes (tm_bytes (tm)),
                  tm_width (tm),
                  tm_height (tm),
                  TILING_ALWAYS);

  tilebuf_from_tm (c->tile_data, tm);

  return c;
}


TileManager *
canvas_to_tm (
              Canvas *c
              )
{
  TileManager *tm;

  tm = tile_manager_new (canvas_width (c),
                         canvas_height (c),
                         tag_bytes (canvas_tag (c)));
  
  tilebuf_to_tm (c->tile_data, tm);

  return tm;
}
