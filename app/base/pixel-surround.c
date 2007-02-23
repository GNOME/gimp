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

const guchar *
pixel_surround_lock (PixelSurround *ps,
                     gint           x,
                     gint           y,
                     gint          *rowstride)
{
  gint          i, j;
  const guchar *k;
  guchar       *ptr;

  ps->tile = tile_manager_get_tile (ps->mgr, x, y, TRUE, FALSE);

  i = x % TILE_WIDTH;
  j = y % TILE_HEIGHT;

  /* does the tile cover the whole region? */
  if (ps->tile &&
      i + ps->w < tile_ewidth (ps->tile) &&
      j + ps->h < tile_eheight (ps->tile))
    {
      *rowstride = tile_ewidth (ps->tile) * ps->bpp;

      return tile_data_pointer (ps->tile, i, j);
    }

  /* nope, we will use our internal buffer */
  if (ps->tile)
    {
      tile_release (ps->tile, FALSE);
      ps->tile = NULL;
    }

  /* copy pixels, one by one */
  ptr = ps->buff;

  for (j = y; j < y + ps->h; ++j)
    {
      for (i = x; i < x + ps->w; ++i)
        {
          Tile *tile = tile_manager_get_tile (ps->mgr, i, j, TRUE, FALSE);

          if (tile)
            {
              const guchar *buff = tile_data_pointer (tile,
                                                      i % TILE_WIDTH,
                                                      j % TILE_HEIGHT);

              for (k = buff; k < buff + ps->bpp; ++k, ++ptr)
                *ptr = *k;

              tile_release (tile, FALSE);
            }
          else
            {
              for (k = ps->bg; k < ps->bg + ps->bpp; ++k, ++ptr)
                *ptr = *k;
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
  g_free (ps);
}
