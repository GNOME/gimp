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

#include <sys/types.h>
#include <string.h>

#include "tilebuf.h"
#include "trace.h"


#define TILE16_WIDTH   64
#define TILE16_HEIGHT  64

typedef struct _Tile16 Tile16;



struct _TileBuf
{
  Tag        tag;
  int        width;
  int        height;

  Tile16   * tiles;
};

struct _Tile16
{
  short     valid;
  short     ref_count;
  guchar  * data;
};



static int  tile16_index       (TileBuf *, int, int);
static int  tile16_xoffset     (TileBuf *, int);
static int  tile16_yoffset     (TileBuf *, int);




TileBuf * 
tilebuf_new  (
              Tag tag,
              int w,
              int h
              )
{
  TileBuf *t;
  int n;
  
  t = (TileBuf *) g_malloc (sizeof (TileBuf));

  t->tag = tag;
  t->width = w;
  t->height = h;

  n = tile16_index (t, w - 1, h - 1) + 1;
  t->tiles = g_new (Tile16, n);
  while (n--)
    {
      t->tiles[n].valid = FALSE;
      t->tiles[n].ref_count = 0;
      t->tiles[n].data = NULL;
    }
  
  return t;
}


void 
tilebuf_delete  (
                 TileBuf * t
                 )
{
  if (t)
    {
      if (t->tiles)
        {
          int n = tile16_index (t, t->width - 1, t->height - 1) + 1;
          while (n--)
            if (t->tiles[n].data)
              g_free (t->tiles[n].data);
          g_free (t->tiles);
        }
      g_free (t);
    }
}


TileBuf * 
tilebuf_clone  (
                TileBuf * t
                )
{
  TileBuf *newt = NULL;

  if (t)
    {
      newt = (TileBuf *) g_malloc (sizeof (TileBuf));

      newt->tag = t->tag;
      newt->width = t->width;
      newt->height = t->height;

      g_warning ("finish writing tilebuf_clone()");
      /* tile16_clone (t, newt); */
    }
  
  return newt;
}


void
tilebuf_info (
              TileBuf * t
              )
{
  if (t)
    {
      trace_begin ("TileBuf 0x%x", t);
      trace_printf ("%d by %d tiled buffer", t->width, t->height);
      trace_printf ("%s %s %s",
                    tag_string_precision (tag_precision (tilebuf_tag (t))),
                    tag_string_format (tag_format (tilebuf_tag (t))),
                    tag_string_alpha (tag_alpha (tilebuf_tag (t))));
      if (t->tiles)
        {
          int i = tile16_index (t, t->width-1, t->height-1) + 1;
          while (i--)
            {
              if (t->tiles[i].valid)
                {
                  trace_printf ("  Tile %d, ref %d, data %x",
                                i, t->tiles[i].ref_count, t->tiles[i].data);
                }
            }
        }
      else
        {
          trace_printf ("  No tiles");
        }
      trace_end ();
    }
}


Tag 
tilebuf_tag  (
              TileBuf * t
              )
{
  if (t)
    return t->tag;
  return tag_null ();
}


Precision 
tilebuf_precision  (
                    TileBuf * t
                    )
{
  return tag_precision (tilebuf_tag (t));
}


Format 
tilebuf_format  (
                 TileBuf * t
                 )
{
  return tag_format (tilebuf_tag (t));
}


Alpha 
tilebuf_alpha  (
                TileBuf * t
                )
{
  return tag_alpha (tilebuf_tag (t));
}


Precision 
tilebuf_set_precision  (
                        TileBuf * t,
                        Precision x
                        )
{
  g_warning ("finish writing tilebuf_set_precision()");
  return tilebuf_precision (t);
}


Format 
tilebuf_set_format  (
                     TileBuf * t,
                     Format x
                     )
{
  g_warning ("finish writing tilebuf_set_format()");
  return tilebuf_format (t);
}


Alpha 
tilebuf_set_alpha  (
                    TileBuf * t,
                    Alpha x
                    )
{
  g_warning ("finish writing tilebuf_set_alpha()");
  return tilebuf_alpha (t);
}


guint 
tilebuf_width  (
                TileBuf * t
                )
{
  if (t)
    return t->width;
  return 0;
}


guint 
tilebuf_height  (
                 TileBuf * t
                 )
{
  if (t)
    return t->height;
  return 0;
}









guint 
tilebuf_portion_ref  (
                      TileBuf * t,
                      int x,
                      int y
                      )
{
  guint rc = FALSE;

  int i = tile16_index(t, x, y);
  if (i >= 0)
    {
      Tile16 * tile = &t->tiles[i];
      
      tile->ref_count++;
      
      if (tile->data == NULL)
        {
          if (tile->valid == TRUE)
            {
              /* swap tile in */
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
tilebuf_portion_unref  (
                        TileBuf * t,
                        int x,
                        int y
                        )
{
  int i = tile16_index(t, x, y);
  if (i >= 0)
    {
      Tile16 * tile = &t->tiles[i];

      tile->ref_count--;
      if ((tile->ref_count == 0) &&
          (tile->valid == TRUE))
        {
          /* swap tile out */
        }
    }
}


guint 
tilebuf_portion_width  (
                        TileBuf * t,
                        int x,
                        int y
                        )
{
  int i = tile16_index(t, x, y);
  if (i >= 0)
    {
      int offset = tile16_xoffset (t, x);
      int base = x - offset;
      if (base+TILE16_WIDTH > t->width)
        return tile16_xoffset (t, t->width) - offset;
      else
        return TILE16_WIDTH - offset;
    }
  return 0;
}


guint 
tilebuf_portion_height  (
                         TileBuf * t,
                         int x,
                         int y
                         )
{
  int i = tile16_index(t, x, y);
  if (i >= 0)
    {
      int offset = tile16_yoffset (t, y);
      int base = y - offset;
      if (base+TILE16_HEIGHT > t->height)
        return tile16_yoffset (t, t->height) - offset;
      else
        return TILE16_HEIGHT - offset;
    }
  return 0;
}


guint 
tilebuf_portion_top  (
                      TileBuf * t,
                      int x,
                      int y
                      )
{
  int i = tile16_index(t, x, y);
  if (i >= 0)
    {
      return y - tile16_yoffset (t, y);
    }
  return 0;
}


guint 
tilebuf_portion_left  (
                       TileBuf * t,
                       int x,
                       int y
                       )
{
  int i = tile16_index(t, x, y);
  if (i >= 0)
    {
      return x - tile16_xoffset (t, x);
    }
  return 0;
}


guchar * 
tilebuf_portion_data  (
                       TileBuf * t,
                       int x,
                       int y
                       )
{
  int i = tile16_index(t, x, y);
  if (i >= 0)
    {
      Tile16 * tile = &t->tiles[i];
      if (tile->data)
        {
          return (tile->data + (tag_bytes (tilebuf_tag (t)) *
                                ((tile16_yoffset (t, y) * TILE16_WIDTH) +
                                 tile16_xoffset (t, x))));
        }
    }
  return NULL;
}


guint 
tilebuf_portion_rowstride  (
                            TileBuf * t,
                            int x,
                            int y
                            )
{
  int i = tile16_index(t, x, y);
  if (i >= 0)
    return TILE16_WIDTH * tag_bytes (tilebuf_tag (t));
  return 0;
}


guint
tilebuf_portion_alloced (
                         TileBuf * t,
                         int x,
                         int y
                         )
{
  int i = tile16_index(t, x, y);
  if (i >= 0)
    {
      Tile16 * tile = &t->tiles[i];
      return tile->valid;
    }
  return FALSE;
}


guint 
tilebuf_portion_alloc  (
                        TileBuf * t,
                        int x,
                        int y
                        )
{
  int i = tile16_index(t, x, y);
  if (i >= 0)
    {
      Tile16 * tile = &t->tiles[i];
      if (tile->valid == FALSE)
        {
          int n = TILE16_WIDTH * TILE16_HEIGHT * tag_bytes (tilebuf_tag (t));
          tile->data = g_malloc (n);
          if (tile->data)
            {
              tile->valid = TRUE;
              memset (tile->data, 0, n);
              return TRUE;
            }
        }
    }
  return FALSE;
}


guint 
tilebuf_portion_unalloc  (
                          TileBuf * t,
                          int x,
                          int y
                          )
{
  int i = tile16_index(t, x, y);
  if (i >= 0)
    {
      Tile16 * tile = &t->tiles[i];
      if ((tile->valid == TRUE) && (tile->ref_count == 0))
        {
          g_free (tile->data);
          tile->data = NULL;
          tile->valid = FALSE;
          return TRUE;
        }
    }
  return FALSE;
}






static int
tile16_index (
              TileBuf * t,
              int x,
              int y
              )
{
  if (t)
    {
      if ((x < t->width) && (y < t->height))    
        {
          x = x / TILE16_WIDTH;
          y = y / TILE16_HEIGHT;
          return (y * ((t->width + TILE16_WIDTH - 1) / TILE16_WIDTH) + x);
        }
    }
  return -1;
}


static int
tile16_xoffset (
                TileBuf * t,
                int x
                )
{
  return x % TILE16_WIDTH;
}


static int
tile16_yoffset (
                TileBuf * t,
                int y
                )
{
  return y % TILE16_HEIGHT;
}





/* temporary evil */
void 
tilebuf_init  (
               TileBuf * t,
               TileBuf * src,
               int x,
               int y
               )
{
  if (t && src
      && (t->width == src->width)
      && (t->height == src->height)
      && (x < t->width)
      && (y < t->height)
      && tag_equal (t->tag, src->tag))
    {
      tilebuf_portion_ref (t, x, y);
      tilebuf_portion_ref (src, x, y);
      {
        Tile16 * st = &src->tiles[tile16_index(src, x, y)];
        Tile16 * dt = &t->tiles[tile16_index(t, x, y)];
        int len = TILE16_WIDTH * TILE16_HEIGHT * tag_bytes (tilebuf_tag (t));
        memcpy (dt->data, st->data, len);
      }
      tilebuf_portion_unref (src, x, y);
      tilebuf_portion_unref (t, x, y);
    }
}


