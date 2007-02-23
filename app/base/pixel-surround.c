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
  Tile        *tile;
  TileManager *mgr;
  guchar      *buff;
  gint         buff_size;
  gint         bpp;
  gint         w;
  gint         h;
  guchar       bg[MAX_CHANNELS];
};


PixelSurround *
pixel_surround_new (TileManager  *tm,
                    gint          w,
                    gint          h,
                    const guchar  bg[MAX_CHANNELS])
{
  PixelSurround *ps;
  gint           i;

  g_return_val_if_fail (tm != NULL, NULL);

  ps = g_new0 (PixelSurround, 1);

  ps->mgr       = tm;
  ps->bpp       = tile_manager_bpp (tm);
  ps->w         = w;
  ps->h         = h;
  ps->buff_size = w * h * ps->bpp;
  ps->buff      = g_new (guchar, ps->buff_size);

  for (i = 0; i < MAX_CHANNELS; ++i)
    ps->bg[i] = bg[i];

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

  /* do we have the whole region? */
  if (ps->tile &&
      (i < (tile_ewidth (ps->tile) - ps->w)) &&
      (j < (tile_eheight (ps->tile) - ps->h)))
    {
      *rowstride = tile_ewidth (ps->tile) * ps->bpp;

      return tile_data_pointer (ps->tile, i, j);
    }

  /* nope, do this the hard way (for now) */
  if (ps->tile)
    {
      tile_release (ps->tile, FALSE);
      ps->tile = NULL;
    }

  /* copy pixels, one by one */
  /* no, this is not the best way, but it's much better than before */
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
  g_return_if_fail (ps != NULL);

  if (ps->buff)
    g_free (ps->buff);

  g_free (ps);
}
