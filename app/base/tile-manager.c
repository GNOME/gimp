/* GIMP - The GNU Image Manipulation Program
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
 * Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.
 */

#include "config.h"

#include <string.h>

#include <glib-object.h>

#include "base-types.h"

#include "tile.h"
#include "tile-private.h"
#include "tile-cache.h"
#include "tile-manager.h"
#include "tile-manager-private.h"
#include "tile-swap.h"


static inline gint
tile_manager_get_tile_num (TileManager *tm,
                           gint         xpixel,
                           gint         ypixel)
{
  if ((xpixel < 0) || (xpixel >= tm->width) ||
      (ypixel < 0) || (ypixel >= tm->height))
    return -1;

  return (ypixel / TILE_HEIGHT) * tm->ntile_cols + (xpixel / TILE_WIDTH);
}


TileManager *
tile_manager_new (gint width,
                  gint height,
                  gint bpp)
{
  TileManager *tm;

  g_return_val_if_fail (width > 0 && height > 0, NULL);
  g_return_val_if_fail (bpp > 0 && bpp <= 4, NULL);

  tm = g_slice_new0 (TileManager);

  tm->ref_count  = 1;
  tm->width      = width;
  tm->height     = height;
  tm->bpp        = bpp;
  tm->ntile_rows = (height + TILE_HEIGHT - 1) / TILE_HEIGHT;
  tm->ntile_cols = (width  + TILE_WIDTH  - 1) / TILE_WIDTH;
  tm->cached_num = -1;

  return tm;
}

TileManager *
tile_manager_ref (TileManager *tm)
{
  g_return_val_if_fail (tm != NULL, NULL);

  tm->ref_count++;

  return tm;
}

void
tile_manager_unref (TileManager *tm)
{
  g_return_if_fail (tm != NULL);

  tm->ref_count--;

  if (tm->ref_count < 1)
    {
      if (tm->cached_tile)
        tile_release (tm->cached_tile, FALSE);

      if (tm->tiles)
        {
          gint ntiles = tm->ntile_rows * tm->ntile_cols;
          gint i;

          for (i = 0; i < ntiles; i++)
            tile_detach (tm->tiles[i], tm, i);

          g_free (tm->tiles);
        }

      g_slice_free (TileManager, tm);
    }
}

void
tile_manager_set_validate_proc (TileManager      *tm,
                                TileValidateProc  proc)
{
  g_return_if_fail (tm != NULL);

  tm->validate_proc = proc;
}

Tile *
tile_manager_get_tile (TileManager *tm,
                       gint         xpixel,
                       gint         ypixel,
                       gint         wantread,
                       gint         wantwrite)
{
  g_return_val_if_fail (tm != NULL, NULL);

  return tile_manager_get (tm,
                           tile_manager_get_tile_num (tm, xpixel, ypixel),
                           wantread, wantwrite);
}

Tile *
tile_manager_get (TileManager *tm,
                  gint         tile_num,
                  gint         wantread,
                  gint         wantwrite)
{
  Tile **tiles;
  Tile **tile_ptr;
  gint   ntiles;
  gint   nrows, ncols;
  gint   right_tile;
  gint   bottom_tile;
  gint   i, j, k;

  g_return_val_if_fail (tm != NULL, NULL);

  ntiles = tm->ntile_rows * tm->ntile_cols;

  if ((tile_num < 0) || (tile_num >= ntiles))
    return NULL;

  if (! tm->tiles)
    {
      tm->tiles = g_new (Tile *, ntiles);
      tiles = tm->tiles;

      nrows = tm->ntile_rows;
      ncols = tm->ntile_cols;

      right_tile  = tm->width  - ((ncols - 1) * TILE_WIDTH);
      bottom_tile = tm->height - ((nrows - 1) * TILE_HEIGHT);

      for (i = 0, k = 0; i < nrows; i++)
        {
          for (j = 0; j < ncols; j++, k++)
            {
              Tile *new = tile_new (tm->bpp);

              tile_attach (new, tm, k);

              if (j == (ncols - 1))
                new->ewidth = right_tile;

              if (i == (nrows - 1))
                new->eheight = bottom_tile;

              new->size = new->ewidth * new->eheight * new->bpp;

              tiles[k] = new;
            }
        }
    }

  tile_ptr = &tm->tiles[tile_num];

  if (G_UNLIKELY (wantwrite && !wantread))
    g_warning ("WRITE-ONLY TILE... UNTESTED!");

#ifdef DEBUG_TILE_MANAGER
  if (G_UNLIKELY ((*tile_ptr)->share_count && (*tile_ptr)->write_count))
    g_printerr (">> MEEPITY %d,%d <<\n",
                (*tile_ptr)->share_count, (*tile_ptr)->write_count);
#endif

  if (wantread)
    {
      if (wantwrite)
        {
          if ((*tile_ptr)->share_count > 1)
            {
              /* Copy-on-write required */
              Tile *new = tile_new ((*tile_ptr)->bpp);

              new->ewidth  = (*tile_ptr)->ewidth;
              new->eheight = (*tile_ptr)->eheight;
              new->valid   = (*tile_ptr)->valid;

              new->size    = new->ewidth * new->eheight * new->bpp;
              new->data    = g_new (guchar, new->size);

              if ((*tile_ptr)->rowhint)
                new->rowhint = g_memdup ((*tile_ptr)->rowhint,
                                             new->eheight *
                                             sizeof (TileRowHint));

              if ((*tile_ptr)->data)
                {
                  memcpy (new->data, (*tile_ptr)->data, new->size);
                }
              else
                {
                  tile_lock (*tile_ptr);
                  memcpy (new->data, (*tile_ptr)->data, new->size);
                  tile_release (*tile_ptr, FALSE);
                }

              tile_detach (*tile_ptr, tm, tile_num);

              tile_attach (new, tm, tile_num);
              *tile_ptr = new;
            }

          (*tile_ptr)->write_count++;
          (*tile_ptr)->dirty = TRUE;
        }
#ifdef DEBUG_TILE_MANAGER
      else
        {
          if (G_UNLIKELY ((*tile_ptr)->write_count))
            g_printerr ("STINK! r/o on r/w tile (%d)\n",
                        (*tile_ptr)->write_count);
        }
#endif

      tile_lock (*tile_ptr);
    }

  return *tile_ptr;
}

void
tile_manager_validate (TileManager *tm,
                       Tile        *tile)
{
  g_return_if_fail (tm != NULL);
  g_return_if_fail (tile != NULL);

  tile->valid = TRUE;

  if (tm->validate_proc)
    (* tm->validate_proc) (tm, tile);

#ifdef DEBUG_TILE_MANAGER
  g_printerr ("%c", tm->user_data ? 'V' : 'v');
#endif
}

void
tile_manager_invalidate_tiles (TileManager *tm,
                               Tile        *toplevel_tile)
{
  gdouble x, y;
  gint    row, col;

  g_return_if_fail (tm != NULL);
  g_return_if_fail (toplevel_tile != NULL);

  col = toplevel_tile->tlink->tile_num % tm->ntile_cols;
  row = toplevel_tile->tlink->tile_num / tm->ntile_cols;

  x = (col * TILE_WIDTH  + toplevel_tile->ewidth  / 2.0) / (gdouble) tm->width;
  y = (row * TILE_HEIGHT + toplevel_tile->eheight / 2.0) / (gdouble) tm->height;

  if (tm->tiles)
    {
      gint num;

      col = x * tm->width / TILE_WIDTH;
      row = y * tm->height / TILE_HEIGHT;

      num = row * tm->ntile_cols + col;

      tile_invalidate (&tm->tiles[num], tm, num);
    }
}

void
tile_invalidate_tile (Tile        **tile_ptr,
                      TileManager  *tm,
                      gint          xpixel,
                      gint          ypixel)
{
  gint num;

  g_return_if_fail (tile_ptr != NULL);
  g_return_if_fail (tm != NULL);

  num = tile_manager_get_tile_num (tm, xpixel, ypixel);
  if (num < 0)
    return;

  tile_invalidate (tile_ptr, tm, num);
}

void
tile_invalidate (Tile        **tile_ptr,
                 TileManager  *tm,
                 gint          tile_num)
{
  Tile *tile = *tile_ptr;

  g_return_if_fail (tile_ptr != NULL);
  g_return_if_fail (tm != NULL);

  if (! tile->valid)
    return;

  if (tile_num == tm->cached_num)
    {
      tile_release (tm->cached_tile, FALSE);

      tm->cached_tile = NULL;
      tm->cached_num  = -1;
    }

  if (G_UNLIKELY (tile->share_count > 1))
    {
      /* This tile is shared.  Replace it with a new, invalid tile. */
      Tile *new = tile_new (tile->bpp);

      g_print ("invalidating shared tile (executing buggy code!!!)\n");

      new->ewidth  = tile->ewidth;
      new->eheight = tile->eheight;
      new->size    = tile->size;

      tile_detach (tile, tm, tile_num);

      tile_attach (new, tm, tile_num);
      tile = *tile_ptr = new;
    }

  if (tile->listhead)
    tile_cache_flush (tile);

  tile->valid = FALSE;

  if (tile->data)
    {
      g_free (tile->data);
      tile->data = NULL;
    }

  if (tile->swap_offset != -1)
    {
      /* If the tile is on disk, then delete its
       *  presence there.
       */
      tile_swap_delete (tile);
    }
}

void
tile_manager_map_tile (TileManager *tm,
                       gint         xpixel,
                       gint         ypixel,
                       Tile        *srctile)
{
  gint num;

  g_return_if_fail (tm != NULL);
  g_return_if_fail (srctile != NULL);

  num = tile_manager_get_tile_num (tm, xpixel, ypixel);

  if (G_UNLIKELY (num < 0))
    {
      g_warning ("%s: tile coordinates out of range.", G_GNUC_FUNCTION);
      return;
    }

  tile_manager_map (tm, num, srctile);
}

void
tile_manager_map (TileManager *tm,
                  gint         tile_num,
                  Tile        *srctile)
{
  Tile **tiles;
  Tile **tile_ptr;
  gint   ntiles;
  gint   nrows, ncols;
  gint   right_tile;
  gint   bottom_tile;
  gint   i, j, k;

  g_return_if_fail (tm != NULL);
  g_return_if_fail (srctile != NULL);

  ntiles = tm->ntile_rows * tm->ntile_cols;

  if (G_UNLIKELY ((tile_num < 0) || (tile_num >= ntiles)))
    {
      g_warning ("%s: tile out of range", G_GNUC_FUNCTION);
      return;
    }

  if (G_UNLIKELY (! tm->tiles))
    {
      g_warning ("%s: empty tile level - initializing", G_GNUC_FUNCTION);

      tm->tiles = g_new (Tile *, ntiles);
      tiles = tm->tiles;

      nrows = tm->ntile_rows;
      ncols = tm->ntile_cols;

      right_tile  = tm->width  - ((ncols - 1) * TILE_WIDTH);
      bottom_tile = tm->height - ((nrows - 1) * TILE_HEIGHT);

      for (i = 0, k = 0; i < nrows; i++)
        {
          for (j = 0; j < ncols; j++, k++)
            {
              Tile *new = tile_new (tm->bpp);

#ifdef DEBUG_TILE_MANAGER
              g_printerr (",");
#endif
              tile_attach (new, tm, k);

              if (j == (ncols - 1))
                new->ewidth = right_tile;

              if (i == (nrows - 1))
                new->eheight = bottom_tile;

              new->size = new->ewidth * new->eheight * new->bpp;

              tiles[k] = new;
            }
        }
    }

  tile_ptr = &tm->tiles[tile_num];

#ifdef DEBUG_TILE_MANAGER
  g_printerr (")");
#endif

  if (G_UNLIKELY (! srctile->valid))
    g_warning("%s: srctile not validated yet!  please report", G_GNUC_FUNCTION);

  if (G_UNLIKELY ((*tile_ptr)->ewidth  != srctile->ewidth  ||
                  (*tile_ptr)->eheight != srctile->eheight ||
                  (*tile_ptr)->bpp     != srctile->bpp))
    {
      g_warning ("%s: nonconformant map (%p -> %p)",
                 G_GNUC_FUNCTION, srctile, *tile_ptr);
    }

  tile_detach (*tile_ptr, tm, tile_num);

#ifdef DEBUG_TILE_MANAGER
  g_printerr (">");
#endif

#ifdef DEBUG_TILE_MANAGER
  g_printerr (" [src:%p tm:%p tn:%d] ", srctile, tm, tile_num);
#endif

  tile_attach (srctile, tm, tile_num);
  *tile_ptr = srctile;

#ifdef DEBUG_TILE_MANAGER
  g_printerr ("}\n");
#endif
}

void
tile_manager_set_user_data (TileManager *tm,
                            gpointer     user_data)
{
  g_return_if_fail (tm != NULL);

  tm->user_data = user_data;
}

gpointer
tile_manager_get_user_data (const TileManager *tm)
{
  g_return_val_if_fail (tm != NULL, NULL);

  return tm->user_data;
}

gint
tile_manager_width  (const TileManager *tm)
{
  g_return_val_if_fail (tm != NULL, 0);

  return tm->width;
}

gint
tile_manager_height (const TileManager *tm)
{
  g_return_val_if_fail (tm != NULL, 0);

  return tm->height;
}

gint
tile_manager_bpp (const TileManager *tm)
{
  g_return_val_if_fail (tm != NULL, 0);

  return tm->bpp;
}

void
tile_manager_get_offsets (const TileManager *tm,
                          gint              *x,
                          gint              *y)
{
  g_return_if_fail (tm != NULL);
  g_return_if_fail (x != NULL && y != NULL);

  *x = tm->x;
  *y = tm->y;
}

void
tile_manager_set_offsets (TileManager *tm,
                          gint         x,
                          gint         y)
{
  g_return_if_fail (tm != NULL);

  tm->x = x;
  tm->y = y;
}

gint64
tile_manager_get_memsize (const TileManager *tm,
                          gboolean           sparse)
{
  gint64 memsize;

  g_return_val_if_fail (tm != NULL, 0);

  /*  the tile manager itself  */
  memsize = sizeof (TileManager);

  /*  the array of tiles  */
  memsize += (gint64) tm->ntile_rows * tm->ntile_cols * (sizeof (Tile) +
                                                         sizeof (gpointer));

  /*  the memory allocated for the tiles   */
  if (sparse)
    {
      if (tm->tiles)
        {
          Tile   **tiles = tm->tiles;
          gint64   size  = TILE_WIDTH * TILE_HEIGHT * tm->bpp;
          gint     i, j;

          for (i = 0; i < tm->ntile_rows; i++)
            for (j = 0; j < tm->ntile_cols; j++, tiles++)
              {
                if (tile_is_valid (*tiles))
                  memsize += size;
              }
        }
    }
  else
    {
      memsize += (gint64) tm->width * tm->height * tm->bpp;
    }

  return memsize;
}

void
tile_manager_get_tile_coordinates (TileManager *tm,
                                   Tile        *tile,
                                   gint        *x,
                                   gint        *y)
{
  TileLink *tl;

  g_return_if_fail (tm != NULL);
  g_return_if_fail (x != NULL && y != NULL);

  for (tl = tile->tlink; tl; tl = tl->next)
    {
      if (tl->tm == tm)
        break;
    }

  if (G_UNLIKELY (tl == NULL))
    {
      g_warning ("%s: tile not attached to manager", G_GNUC_FUNCTION);
      return;
    }

  *x = TILE_WIDTH * (tl->tile_num % tm->ntile_cols);
  *y = TILE_HEIGHT * (tl->tile_num / tm->ntile_cols);
}

void
tile_manager_map_over_tile (TileManager *tm,
                            Tile        *tile,
                            Tile        *srctile)
{
  TileLink *tl;

  g_return_if_fail (tm != NULL);
  g_return_if_fail (tile != NULL);
  g_return_if_fail (srctile != NULL);

  for (tl = tile->tlink; tl; tl = tl->next)
    {
      if (tl->tm == tm)
        break;
    }

  if (G_UNLIKELY (tl == NULL))
    {
      g_warning ("%s: tile not attached to manager", G_GNUC_FUNCTION);
      return;
    }

  tile_manager_map (tm, tl->tile_num, srctile);
}

void
read_pixel_data (TileManager *tm,
                 gint         x1,
                 gint         y1,
                 gint         x2,
                 gint         y2,
                 guchar      *buffer,
                 guint        stride)
{
  guint x, y;

  for (y = y1; y <= y2; y += TILE_HEIGHT - (y % TILE_HEIGHT))
    for (x = x1; x <= x2; x += TILE_WIDTH - (x % TILE_WIDTH))
      {
        Tile         *t = tile_manager_get_tile (tm, x, y, TRUE, FALSE);
        const guchar *s = tile_data_pointer (t,
                                             x % TILE_WIDTH, y % TILE_HEIGHT);
        guchar       *d = buffer + stride * (y - y1) + tm->bpp * (x - x1);
        guint         rows, cols;
        guint         srcstride;

        rows = tile_eheight (t) - y % TILE_HEIGHT;
        if (rows > (y2 - y + 1))
          rows = y2 - y + 1;

        cols = tile_ewidth (t) - x % TILE_WIDTH;
        if (cols > (x2 - x + 1))
          cols = x2 - x + 1;

        srcstride = tile_ewidth (t) * tile_bpp (t);

        while (rows--)
          {
            memcpy (d, s, cols * tm->bpp);

            s += srcstride;
            d += stride;
          }

        tile_release (t, FALSE);
      }
}

void
write_pixel_data (TileManager  *tm,
                  gint          x1,
                  gint          y1,
                  gint          x2,
                  gint          y2,
                  const guchar *buffer,
                  guint         stride)
{
  guint x, y;

  for (y = y1; y <= y2; y += TILE_HEIGHT - (y % TILE_HEIGHT))
    for (x = x1; x <= x2; x += TILE_WIDTH - (x % TILE_WIDTH))
      {
        Tile         *t = tile_manager_get_tile (tm, x, y, TRUE, TRUE);
        const guchar *s = buffer + stride * (y - y1) + tm->bpp * (x - x1);
        guchar       *d = tile_data_pointer (t,
                                             x % TILE_WIDTH, y % TILE_HEIGHT);
        guint         rows, cols;
        guint         dststride;

        rows = tile_eheight (t) - y % TILE_HEIGHT;
        if (rows > (y2 - y + 1))
          rows = y2 - y + 1;

        cols = tile_ewidth (t) - x % TILE_WIDTH;
        if (cols > (x2 - x + 1))
          cols = x2 - x + 1;

        dststride = tile_ewidth (t) * tile_bpp (t);

        while (rows--)
          {
            memcpy (d, s, cols * tm->bpp);

            s += stride;
            d += dststride;
          }

        tile_release (t, TRUE);
      }
}

void
read_pixel_data_1 (TileManager *tm,
                   gint         x,
                   gint         y,
                   guchar      *buffer)
{
  if (x >= 0 && y >= 0 && x < tm->width && y < tm->height)
    {
      gint num = tile_manager_get_tile_num (tm, x, y);

      if (num != tm->cached_num)    /* must fetch a new tile */
        {
           if (tm->cached_tile)
             tile_release (tm->cached_tile, FALSE);

           tm->cached_num  = num;
           tm->cached_tile = tile_manager_get (tm, num, TRUE, FALSE);
        }

      if (tm->cached_tile)
        {
          const guchar *src = tile_data_pointer (tm->cached_tile,
                                                 x % TILE_WIDTH,
                                                 y % TILE_HEIGHT);

           switch (tm->bpp)
             {
             case 4:
               *buffer++ = *src++;
             case 3:
               *buffer++ = *src++;
             case 2:
               *buffer++ = *src++;
             case 1:
               *buffer++ = *src++;
             }
        }
    }
}

void
write_pixel_data_1 (TileManager  *tm,
                    gint          x,
                    gint          y,
                    const guchar *buffer)
{
  Tile   *tile = tile_manager_get_tile (tm, x, y, TRUE, TRUE);
  guchar *dest = tile_data_pointer (tile, x % TILE_WIDTH, y % TILE_HEIGHT);

  switch (tm->bpp)
    {
    case 4:
      *dest++ = *buffer++;
    case 3:
      *dest++ = *buffer++;
    case 2:
      *dest++ = *buffer++;
    case 1:
      *dest++ = *buffer++;
    }

  tile_release (tile, TRUE);
}
