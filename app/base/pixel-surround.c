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
  TileManager *mgr;
  gint         bpp;
  gint         w;
  gint         h;
  guchar       bg[MAX_CHANNELS];
  Tile        *tile;
  guchar       buff[0];
};


PixelSurround *
pixel_surround_new (TileManager  *tm,
                    gint          w,
                    gint          h,
                    const guchar  bg[MAX_CHANNELS])
{
  PixelSurround *ps;
  gint           size;
  gint           i;

  g_return_val_if_fail (tm != NULL, NULL);

  size = w * h * tile_manager_bpp (tm);

  ps = g_malloc (sizeof (PixelSurround) + size);

  ps->mgr = tm;
  ps->bpp = tile_manager_bpp (tm);
  ps->w   = w;
  ps->h   = h;

  for (i = 0; i < MAX_CHANNELS; ++i)
    ps->bg[i] = bg[i];

  ps->tile = NULL;

  return ps;
}

#define PIXEL_COPY(dest, src, bpp) \
  switch (bpp)              \
  {                         \
  case 4: *dest++ = *src++; \
  case 3: *dest++ = *src++; \
  case 2: *dest++ = *src++; \
  case 1: *dest++ = *src++; \
  }

const guchar *
pixel_surround_lock (PixelSurround *ps,
                     gint           x,
                     gint           y,
                     gint          *rowstride)
{
  Tile   *tile;
  guchar *dest;
  gint    i;
  gint    j;

  /*  check that PixelSurround isn't already locked  */
  g_return_val_if_fail (ps->tile == NULL, NULL);

  tile = tile_manager_get_tile (ps->mgr, x, y, TRUE, FALSE);

  i = x % TILE_WIDTH;
  j = y % TILE_HEIGHT;

  /* does the tile cover the whole region? */
  if (tile &&
      i + ps->w < tile_ewidth (tile) &&
      j + ps->h < tile_eheight (tile))
    {
      ps->tile = tile;

      *rowstride = tile_ewidth (tile) * ps->bpp;

      return tile_data_pointer (tile, i, j);
    }

  if (tile)
    tile_release (tile, FALSE);

  /* copy pixels, one by one */
  dest = ps->buff;

  for (j = y; j < y + ps->h; ++j)
    {
      for (i = x; i < x + ps->w; ++i)
        {
          const guchar *src;

          tile = tile_manager_get_tile (ps->mgr, i, j, TRUE, FALSE);

          if (tile)
            {
              src = tile_data_pointer (tile, i % TILE_WIDTH, j % TILE_HEIGHT);

              PIXEL_COPY (dest, src, ps->bpp);

              tile_release (tile, FALSE);
            }
          else
            {
              src = ps->bg;

              PIXEL_COPY (dest, src, ps->bpp);
            }
        }
    }

  *rowstride = ps->w * ps->bpp;

  return ps->buff;
}

void
pixel_surround_release (PixelSurround *ps)
{
  if (ps->tile)
    {
      tile_release (ps->tile, FALSE);
      ps->tile = NULL;
    }
}

void
pixel_surround_destroy (PixelSurround *ps)
{
  /*  check that PixelSurround is not locked  */
  g_return_if_fail (ps->tile == NULL);

  g_free (ps);
}
