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

#include <string.h>

#include <glib-object.h>

#include "base-types.h"

#include "pixel-region.h"
#include "pixel-surround.h"
#include "tile-manager.h"
#include "tile.h"


struct _PixelSurround
{
  TileManager *mgr;        /*  tile manager to access tiles from    */
  gint         bpp;        /*  bytes per pixel in tile manager      */
  gint         w;          /*  width of pixel surround area         */
  gint         h;          /*  height of pixel surround area        */
  Tile        *tile;       /*  locked tile (may be NULL)            */
  gint         tile_x;     /*  origin of locked tile                */
  gint         tile_y;     /*  origin of locked tile                */
  gint         tile_w;     /*  width of locked tile                 */
  gint         tile_h;     /*  height of locked tile                */
  gint         rowstride;  /*  rowstride of buffers                 */
  guchar      *bg;         /*  buffer filled with background color  */
  guchar      *buf;        /*  buffer used for combining tile data  */
};


/*  inlining this function gives a few percent speedup  */
static inline const guchar *
pixel_surround_get_data (PixelSurround *surround,
                         gint           x,
                         gint           y,
                         gint          *w,
                         gint          *h,
                         gint          *rowstride)
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
          /*  store offset and size of the locked tile  */
          surround->tile_x = x & ~(TILE_WIDTH - 1);
          surround->tile_y = y & ~(TILE_HEIGHT - 1);
          surround->tile_w = tile_ewidth (surround->tile);
          surround->tile_h = tile_eheight (surround->tile);
        }
    }

  if (surround->tile)
    {
      *w = surround->tile_x + surround->tile_w - x;
      *h = surround->tile_y + surround->tile_h - y;

      *rowstride = surround->tile_w * surround->bpp;

      return tile_data_pointer (surround->tile,
                                x % TILE_WIDTH, y % TILE_HEIGHT);
    }
  else
    {
      /*   return a pointer to a virtual background tile  */
      if (x < 0)
        *w = MIN (- x, surround->w);
      else
        *w = surround->w;

      if (y < 0)
        *h = MIN (- y, surround->h);
      else
        *h = surround->h;

      *rowstride = surround->rowstride;

      return surround->bg;
    }
}

/**
 * pixel_surround_new:
 * @tiles:  tile manager
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
  guchar        *dest;
  gint           pixels;

  g_return_val_if_fail (tiles != NULL, NULL);

  surround = g_new0 (PixelSurround, 1);

  surround->mgr       = tiles;
  surround->bpp       = tile_manager_bpp (tiles);
  surround->w         = width;
  surround->h         = height;
  surround->rowstride = width * surround->bpp;
  surround->bg        = g_new (guchar, surround->rowstride * height);
  surround->buf       = g_new (guchar, surround->rowstride * height);

  dest = surround->bg;
  pixels = width * height;

  while (pixels--)
    {
      gint i;

      for (i = 0; i < surround->bpp; i++)
        *dest++ = bg[i];
    }

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
  const guchar *src;
  gint          w, h;

  src = pixel_surround_get_data (surround, x, y, &w, &h, rowstride);

  if (w >= surround->w && h >= surround->h)
    {
      /*  return a pointer to the data if it covers the whole region  */
      return src;
    }
  else
    {
      /*  otherwise, copy region to our internal buffer  */
      guchar *dest = surround->buf;
      gint    inc  = surround->w;
      gint    i    = 0;
      gint    j    = 0;

      /*  These loops are somewhat twisted. The idea is to make as few
       *  calls to pixel_surround_get_data() as possible. Thus whenever we
       *  have source data, we copy all of it to the destination buffer.
       *  The inner loops that copy data are nested into outer loops that
       *  make sure that the destination area is completley filled.
       */

      /*  jump right into the loops since we already have source data  */
      goto start;

      while (i < surround->w)
        {
          dest = surround->buf + i * surround->bpp;

          for (j = 0; j < surround->h;)
            {
              gint rows;

              src = pixel_surround_get_data (surround,
                                             x + i, y + j, &w, &h, rowstride);

            start:

              w = MIN (w, surround->w - i);
              h = MIN (h, surround->h - j);

              rows = h;

              while (rows--)
                {
                  memcpy (dest, src, w * surround->bpp);

                  src += *rowstride;
                  dest += surround->rowstride;
                }

              j += h;
              inc = MIN (inc, w);
            }

          i += inc;
        }
    }

  *rowstride = surround->rowstride;

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

  g_free (surround->buf);
  g_free (surround->bg);
  g_free (surround);
}
