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

#include "tile_manager.h" /* temporary */


#define TILE16_WIDTH 64
#define TILE16_HEIGHT 64

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



int tile16_ref_count = 0;


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


TileBuf * 
tilebuf_clone  (
                TileBuf * t
                )
{
  TileBuf *newt;

  newt = (TileBuf *) g_malloc (sizeof (TileBuf));

  newt->tag = t->tag;
  newt->width = t->width;
  newt->height = t->height;

  /* tile16_clone (t, newt); */
  
  return newt;
}


Tag 
tilebuf_tag  (
              TileBuf * t
              )
{
  return t->tag;
}


Precision 
tilebuf_precision  (
                    TileBuf * t
                    )
{
  return tag_precision (t->tag);
}


Format 
tilebuf_format  (
                 TileBuf * t
                 )
{
  return tag_format (t->tag);
}


Alpha 
tilebuf_alpha  (
                TileBuf * t
                )
{
  return tag_alpha (t->tag);
}


Precision 
tilebuf_set_precision  (
                        TileBuf * t,
                        Precision x
                        )
{
  return tilebuf_precision (t);
}


Format 
tilebuf_set_format  (
                     TileBuf * t,
                     Format x
                     )
{
  return tilebuf_format (t);
}


Alpha 
tilebuf_set_alpha  (
                    TileBuf * t,
                    Alpha x
                    )
{
  return tilebuf_alpha (t);
}


guint 
tilebuf_width  (
                TileBuf * t
                )
{
  return t->width;
}


guint 
tilebuf_height  (
                 TileBuf * t
                 )
{
  return t->height;
}


guint 
tilebuf_ref  (
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
      tile16_ref_count++;
      
      if ((tile->ref_count == 1) && (tile->data == NULL))
        {
          if (tile->valid == TRUE)
            {
              /* swap tile in */
            }
          else
            {
              /* alloc tile */
              int n =
                TILE16_WIDTH *
                TILE16_HEIGHT *
                tag_bytes (tilebuf_tag (t));
              tile->valid = TRUE;
              tile->data = g_new (guchar, n);
              memset (tile->data, 0, n);
              rc = TRUE;
            }
        }
    }
  return rc;
}


void 
tilebuf_unref  (
                TileBuf * t,
                int x,
                int y
                )
{
  int i = tile16_index(t, x, y);
  if (i >= 0)
    {
      Tile16 * tile = &t->tiles[i];
      
      if (tile->ref_count == 0)
        {
          tile16_ref_count--;
          tile->ref_count--;

          /* swap tile out */
        }
    }
}


void 
tilebuf_init  (
               TileBuf * t,
               TileBuf * src,
               int x,
               int y
               )
{
  int src_index = tile16_index(src, x, y);
  int dst_index = tile16_index(t, x, y);

  if ((src_index >= 0) && (dst_index >= 0))
    {
      Tile16 * st = &src->tiles[src_index];
      Tile16 * dt = &t->tiles[dst_index];

      if (dt->data && st->data)
        {
          int len =
            TILE16_WIDTH *
            TILE16_HEIGHT *
            tag_bytes (tilebuf_tag (t));
          memcpy (dt->data, st->data, len);
        }
    }
}




guchar * 
tilebuf_data  (
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


int 
tilebuf_rowstride  (
                    TileBuf * t,
                    int x,
                    int y
                    )
{
  return TILE16_WIDTH * tag_bytes (t->tag);
}


guint 
tilebuf_portion_width  (
                        TileBuf * t,
                        int x,
                        int y
                        )
{
  return TILE16_WIDTH - tile16_xoffset (t, x);
}


guint 
tilebuf_portion_height  (
                         TileBuf * t,
                         int x,
                         int y
                         )
{
  return TILE16_HEIGHT - tile16_yoffset (t, y);
}






static int
tile16_index (
              TileBuf * t,
              int x,
              int y
              )
{
  x = x / TILE16_WIDTH;
  y = y / TILE16_HEIGHT;
  return (y * (t->width + TILE16_WIDTH - 1) / TILE16_WIDTH + x);
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





/* temporary glue */
void 
tilebuf_to_tm  (
                TileBuf * t,
                TileManager * tm
                )
{
  int i;
  
  i = ( ( (t->width + TILE16_WIDTH - 1) / TILE16_WIDTH ) *
        ( (t->height + TILE16_HEIGHT - 1) / TILE16_HEIGHT ) );

  while (i--)
    {
      Tile * tile = tile_manager_get (tm, i, 0);

      if (t->tiles[i].valid == TRUE)
        {
          int n =
            TILE16_WIDTH *
            TILE16_HEIGHT *
            tag_bytes (tilebuf_tag (t));
          tile_ref (tile);
          memcpy (tile->data, t->tiles[i].data, n);
          tile_unref (tile, TRUE);
        }
    }
}


void 
tilebuf_from_tm  (
                  TileBuf * t,
                  TileManager * tm
                  )
{
  int i;
  
  i = ( ( (t->width + TILE16_WIDTH - 1) / TILE16_WIDTH ) *
        ( (t->height + TILE16_HEIGHT - 1) / TILE16_HEIGHT ) );
  
  while (i--)
    {
      Tile * tile = tile_manager_get (tm, i, 0);

      t->tiles[i].valid = FALSE;
      t->tiles[i].ref_count = 0;
      if (t->tiles[i].data)
        g_free (t->tiles[i].data);
      t->tiles[i].data = NULL;
      
      if (tile->valid)
        {
          int n =
            TILE16_WIDTH *
            TILE16_HEIGHT *
            tag_bytes (tilebuf_tag (t));
          t->tiles[i].valid = TRUE;
          t->tiles[i].data = g_new (guchar, n);
          tile_ref (tile);
          memcpy (t->tiles[i].data, tile->data, n);
          tile_unref (tile, FALSE);
        }
    }
}


