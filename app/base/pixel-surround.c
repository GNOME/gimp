/* GIMP - The GNU Image Manipulation Program
 * Copyright (C) 1995-2001 Spencer Kimball, Peter Mattis, and others
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
 * Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.
 */

#include "config.h"

#include <glib-object.h>

#include "base-types.h"

#include "pixel-region.h"
#include "pixel-surround.h"
#include "tile-manager.h"
#include "tile.h"


struct _PixelSurround
{
  TileManager *mgr;               /*  tile manager to access tiles from   */
  gint         bpp;               /*  bytes per pixel in tile manager     */
  gint         w;                 /*  width of pixel surround area        */
  gint         h;                 /*  height of pixel surround area       */
  guchar       bg[MAX_CHANNELS];  /*  color to use for uncovered regions  */
  Tile        *tile;              /*  locked tile (may be NULL)           */
  gint         tile_x;            /*  origin of locked tile               */
  gint         tile_y;            /*  origin of locked tile               */
  gint         tile_w;            /*  width of locked tile                */
  gint         tile_h;            /*  height of locked tile               */
  guchar       buf[0];
};


/*  inlining this function gives a few percent speedup  */
static inline gboolean
pixel_surround_get_tile (PixelSurround *surround,
                         gint           x,
                         gint           y)
{
  /*  do we still have a tile lock that we can use?  */
  if (surround->tile)
    {
      if (x < surround->tile_x || x >= surround->tile_x + surround->tile_w ||
          y < surround->tile_y || y >= surround->tile_y + surround->tile_h)
        {
          tile_release (surround->tile, FALSE);
          surround->tile = NULL;
        }
    }

  /*  if not, try to get one for the target pixel  */
  if (! surround->tile)
    {
      surround->tile = tile_manager_get_tile (surround->mgr, x, y, TRUE, FALSE);

      if (surround->tile)
        {
          surround->tile_x = x & ~(TILE_WIDTH - 1);
          surround->tile_y = y & ~(TILE_HEIGHT - 1);
          surround->tile_w = tile_ewidth (surround->tile);
          surround->tile_h = tile_eheight (surround->tile);
        }
    }

  return (surround->tile != NULL);
}

/**
 * pixel_surround_new:
 * @tm:     tile manager
 * @width:  width of surround region
 * @height: height of surround region
 * @bg:     color to use for pixels that are not covered by the tile manager
 *
 * Return value: a new #PixelSurround.
 */
PixelSurround *
pixel_surround_new (TileManager  *tiles,
                    gint          width,
                    gint          height,
                    const guchar  bg[MAX_CHANNELS])
{
  PixelSurround *surround;
  gint           i;

  g_return_val_if_fail (tiles != NULL, NULL);

  surround = g_malloc (sizeof (PixelSurround) +
                       width * height * tile_manager_bpp (tiles));

  surround->mgr  = tiles;
  surround->bpp  = tile_manager_bpp (tiles);
  surround->w    = width;
  surround->h    = height;

  for (i = 0; i < surround->bpp; ++i)
    surround->bg[i] = bg[i];

  surround->tile = NULL;

  return surround;
}

/**
 * pixel_surround_lock:
 * @surround:  a #PixelSurround
 * @x:         X coordinate of upper left corner
 * @y:         Y coordinate of upper left corner
 * @rowstride: return location for rowstride
 *
 * Gives access to a region of pixels. The upper left corner is
 * specified by the @x and @y parameters. The size of the region
 * is determined by the dimensions given when creating the @surround.
 *
 * When you don't need to read from the pixels any longer, you should
 * unlock the @surround using pixel_surround_unlock(). If you need a
 * different region, just call pixel_surround_lock() again.
 *
 * Return value: pointer to pixel data (read-only)
 */
const guchar *
pixel_surround_lock (PixelSurround *surround,
                     gint           x,
                     gint           y,
                     gint          *rowstride)
{
  guchar *dest;
  gint    i = x % TILE_WIDTH;
  gint    j = y % TILE_HEIGHT;

  /*  return a pointer to the tile data if the tile covers the whole region  */
  if (pixel_surround_get_tile (surround, x, y) &&
      i + surround->w <= surround->tile_w      &&
      j + surround->h <= surround->tile_h)
    {
      *rowstride = surround->tile_w * surround->bpp;

      return tile_data_pointer (surround->tile, i, j);
    }

  /*  otherwise, copy region to our internal buffer  */
  dest = surround->buf;

  for (j = y; j < y + surround->h; j++)
    {
      for (i = x; i < x + surround->w; i++)
        {
          const guchar *src;

          if (pixel_surround_get_tile (surround, i, j))
            {
              src = tile_data_pointer (surround->tile,
                                       i % TILE_WIDTH, j % TILE_HEIGHT);
            }
          else
            {
              src = surround->bg;
            }

          switch (surround->bpp)
            {
            case 4: *dest++ = *src++;
            case 3: *dest++ = *src++;
            case 2: *dest++ = *src++;
            case 1: *dest++ = *src++;
            }
        }
    }

  *rowstride = surround->w * surround->bpp;

  return surround->buf;
}

/**
 * pixel_surround_release:
 * @surround: #PixelSurround
 *
 * Unlocks pixels locked by @surround. See pixel_surround_lock().
 */
void
pixel_surround_release (PixelSurround *surround)
{
  if (surround->tile)
    {
      tile_release (surround->tile, FALSE);
      surround->tile = NULL;
    }
}

/**
 * pixel_surround_destroy:
 * @surround: #PixelSurround
 *
 * Unlocks pixels and frees any resources allocated for @surround. You
 * must not use @surround any longer after calling this function.
 */
void
pixel_surround_destroy (PixelSurround *surround)
{
  g_return_if_fail (surround != NULL);

  pixel_surround_release (surround);
  g_free (surround);
}
