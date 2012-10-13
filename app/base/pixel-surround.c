/* GIMP - The GNU Image Manipulation Program
 * Copyright (C) 1995-2001 Spencer Kimball, Peter Mattis, and others
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
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
  TileManager       *mgr;        /*  tile manager to access tiles from    */
  gint               xmax;       /*  largest x coordinate in tile manager */
  gint               ymax;       /*  largest y coordinate in tile manager */
  gint               bpp;        /*  bytes per pixel in tile manager      */
  gint               w;          /*  width of pixel surround area         */
  gint               h;          /*  height of pixel surround area        */
  Tile              *tile;       /*  locked tile (may be NULL)            */
  gint               tile_x;     /*  origin of locked tile                */
  gint               tile_y;     /*  origin of locked tile                */
  gint               tile_w;     /*  width of locked tile                 */
  gint               tile_h;     /*  height of locked tile                */
  gint               rowstride;  /*  rowstride of buffers                 */
  guchar            *bg;         /*  buffer filled with background color  */
  guchar            *buf;        /*  buffer used for combining tile data  */
  PixelSurroundMode  mode;
};

static const guchar * pixel_surround_get_data (PixelSurround *surround,
                                               gint           x,
                                               gint           y,
                                               gint          *w,
                                               gint          *h,
                                               gint          *rowstride);


/**
 * pixel_surround_new:
 * @tiles:  tile manager
 * @width:  width of surround region
 * @height: height of surround region
 * @mode:   how to deal with pixels that are not covered by the tile manager
 *
 * PixelSurround provides you a contiguous read-only view of the area
 * surrounding a pixel. It is an efficient pixel access strategy for
 * interpolation algorithms.
 *
 * Return value: a new #PixelSurround.
 */
PixelSurround *
pixel_surround_new (TileManager       *tiles,
                    gint               width,
                    gint               height,
                    PixelSurroundMode  mode)
{
  PixelSurround *surround;

  g_return_val_if_fail (tiles != NULL, NULL);
  g_return_val_if_fail (width < TILE_WIDTH, NULL);
  g_return_val_if_fail (height < TILE_WIDTH, NULL);

  surround = g_slice_new0 (PixelSurround);

  surround->mgr       = tiles;
  surround->xmax      = tile_manager_width (surround->mgr) - 1;
  surround->ymax      = tile_manager_height (surround->mgr) - 1;
  surround->bpp       = tile_manager_bpp (tiles);
  surround->w         = width;
  surround->h         = height;
  surround->rowstride = width * surround->bpp;
  surround->bg        = g_new0 (guchar, surround->rowstride * height);
  surround->buf       = g_new (guchar, surround->rowstride * height);
  surround->mode      = mode;

  return surround;
}

/**
 * pixel_surround_set_bg:
 * @surround: a #PixelSurround
 * @bg:       background color
 *
 * This sets the color that the #PixelSurround uses when in
 * %PIXEL_SURROUND_BACKGROUND mode for pixels that are not covered by
 * the tile manager.
 */
void
pixel_surround_set_bg (PixelSurround *surround,
                       const guchar  *bg)
{
  guchar *dest   = surround->bg;
  gint    pixels = surround->w * surround->h;

  while (pixels--)
    {
      gint i;

      for (i = 0; i < surround->bpp; i++)
        *dest++ = bg[i];
    }
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

  g_slice_free (PixelSurround, surround);
}


enum
{
  LEFT   = 1 << 0,
  RIGHT  = 1 << 1,
  TOP    = 1 << 2,
  BOTTOM = 1 << 3
};

static void
pixel_surround_fill_row (PixelSurround *surround,
                         const guchar  *src,
                         gint           w)
{
  guchar *dest  = surround->bg;
  gint    bytes = MIN (w, surround->w) * surround->bpp;
  gint    rows  = surround->h;

  while (rows--)
    {
      memcpy (dest, src, bytes);
      dest += surround->rowstride;
    }
}

static void
pixel_surround_fill_col (PixelSurround *surround,
                         const guchar  *src,
                         gint           rowstride,
                         gint           h)
{
  guchar *dest = surround->bg;
  gint    cols = surround->w;
  gint    rows = MIN (h, surround->h);

  while (cols--)
    {
      const guchar *s = src;
      guchar       *d = dest;
      gint          r = rows;

      while (r--)
        {
          memcpy (d, s, surround->bpp);

          s += rowstride;
          d += surround->rowstride;
        }

      dest += surround->bpp;
    }
}

static const guchar *
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

      return tile_data_pointer (surround->tile, x, y);
    }

  if (x < 0)
    *w = MIN (- x, surround->w);
  else
    *w = surround->w;

  if (y < 0)
    *h = MIN (- y, surround->h);
  else
    *h = surround->h;

  *rowstride = surround->rowstride;

  if (surround->mode == PIXEL_SURROUND_SMEAR)
    {
      const guchar *edata;
      gint          ex = x;
      gint          ey = y;
      gint          ew, eh;
      gint          estride;
      gint          ecode = 0;

      if (ex < 0)
        {
          ex = 0;
          ecode |= LEFT;
        }
      else if (ex > surround->xmax)
        {
          ex = surround->xmax;
          ecode |= RIGHT;
        }

      if (ey < 0)
        {
          ey = 0;
          ecode |= TOP;
        }
      else if (ey > surround->ymax)
        {
          ey = surround->ymax;
          ecode |= BOTTOM;
        }

      /*  call ourselves with corrected coordinates  */
      edata = pixel_surround_get_data (surround, ex, ey, &ew, &eh, &estride);

      /*  fill the virtual background tile  */
      switch (ecode)
        {
        case (TOP | LEFT):
        case (TOP | RIGHT):
        case (BOTTOM | LEFT):
        case (BOTTOM | RIGHT):
          pixel_surround_set_bg (surround, edata);
          break;

        case (TOP):
        case (BOTTOM):
          pixel_surround_fill_row (surround, edata, ew);
          *w = MIN (*w, ew);
          break;

        case (LEFT):
        case (RIGHT):
          pixel_surround_fill_col (surround, edata, estride, eh);
          *h = MIN (*h, eh);
          break;
        }
    }

  /*   return a pointer to the virtual background tile  */
  return surround->bg;
}
