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
 * Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.
 */

#include "tile_cache.h"
#include "tile_manager.h"
#include "tile_swap.h"

#include "tile_manager_pvt.h"
#include "tile_pvt.h"			/* ick. */

static void tile_manager_destroy_level (TileLevel *level);


TileManager*
tile_manager_new (int toplevel_width,
		  int toplevel_height,
		  int bpp)
{
  TileManager *tm;
  int tmp1, tmp2;
  int width, height;
  int i;

  tm = g_new (TileManager, 1);

  tmp1 = tile_manager_calc_levels (toplevel_width, TILE_WIDTH);
  tmp2 = tile_manager_calc_levels (toplevel_height, TILE_HEIGHT);

  tm->nlevels = MAX (tmp1, tmp2);
  tm->levels = g_new (TileLevel, tm->nlevels);
  tm->user_data = NULL;
  tm->validate_proc = NULL;

  width = toplevel_width;
  height = toplevel_height;

  for (i = 0; i < tm->nlevels; i++)
    {
      tm->levels[i].width = width;
      tm->levels[i].height = height;
      tm->levels[i].bpp = bpp;
      tm->levels[i].ntile_rows = (height + TILE_HEIGHT - 1) / TILE_HEIGHT;
      tm->levels[i].ntile_cols = (width + TILE_WIDTH - 1) / TILE_WIDTH;
      tm->levels[i].tiles = NULL;

      width /= 2;
      height /= 2;
    }

  return tm;
}

void
tile_manager_destroy (TileManager *tm)
{
  int i;

  for (i = 0; i < tm->nlevels; i++)
    tile_manager_destroy_level (&tm->levels[i]);

  g_free (tm->levels);
  g_free (tm);
}

int
tile_manager_calc_levels (int size,
			  int tile_size)
{
  int levels;

  levels = 1;
  while (size > tile_size)
    {
      size /= 2;
      levels += 1;
    }

  return levels;
}

void
tile_manager_set_nlevels (TileManager *tm,
			  int          nlevels)
{
  TileLevel *levels;
  int width, height;
  int i;

  if ((nlevels < 1) || (nlevels == tm->nlevels))
    return;

  levels = g_new (TileLevel, nlevels);

  if (nlevels > tm->nlevels)
    {
      for (i = 0; i < tm->nlevels; i++)
	levels[i] = tm->levels[i];

      width = tm->levels[tm->nlevels - 1].width;
      height = tm->levels[tm->nlevels - 1].height;

      for (; i < nlevels; i++)
	{
	  levels[i].width = width;
	  levels[i].height = height;
	  levels[i].bpp = tm->levels[0].bpp;
	  levels[i].ntile_rows = (height + TILE_HEIGHT - 1) / TILE_HEIGHT;
	  levels[i].ntile_cols = (width + TILE_WIDTH - 1) / TILE_WIDTH;
	  levels[i].tiles = NULL;

	  width /= 2;
	  height /= 2;

	  if (width < 1)
	    width = 1;
	  if (height < 1)
	    height = 1;
	}
    }
  else
    {
      for (i = 0; i < nlevels; i++)
	levels[i] = tm->levels[i];

      for (; i < tm->nlevels; i++)
	tile_manager_destroy_level (&tm->levels[i]);
    }

  g_free (tm->levels);

  tm->nlevels = nlevels;
  tm->levels = levels;
}

void
tile_manager_set_validate_proc (TileManager      *tm,
				TileValidateProc  proc)
{
  tm->validate_proc = proc;
}

Tile*
tile_manager_get_tile (TileManager *tm,
		       int          xpixel,
		       int          ypixel,
		       int          level)
{
  TileLevel *tile_level;
  int tile_row;
  int tile_col;
  int tile_num;

  if ((level < 0) || (level >= tm->nlevels))
    return NULL;

  tile_level = &tm->levels[level];
  if ((xpixel < 0) || (xpixel >= tile_level->width) ||
      (ypixel < 0) || (ypixel >= tile_level->height))
    return NULL;

  tile_row = ypixel / TILE_HEIGHT;
  tile_col = xpixel / TILE_WIDTH;
  tile_num = tile_row * tile_level->ntile_cols + tile_col;

  return tile_manager_get (tm, tile_num, level);
}

Tile*
tile_manager_get (TileManager *tm,
		  int          tile_num,
		  int          level)
{
  TileLevel *tile_level;
  Tile *tiles;
  int ntiles;
  int nrows, ncols;
  int right_tile;
  int bottom_tile;
  int i, j, k;

  if ((level < 0) || (level >= tm->nlevels))
    return NULL;

  tile_level = &tm->levels[level];
  ntiles = tile_level->ntile_rows * tile_level->ntile_cols;

  if ((tile_num < 0) || (tile_num >= ntiles))
    return NULL;

  if (!tile_level->tiles)
    {
      tile_level->tiles = g_new (Tile, ntiles);
      tiles = tile_level->tiles;

      nrows = tile_level->ntile_rows;
      ncols = tile_level->ntile_cols;

      right_tile = tile_level->width - ((ncols - 1) * TILE_WIDTH);
      bottom_tile = tile_level->height - ((nrows - 1) * TILE_HEIGHT);

      for (i = 0, k = 0; i < nrows; i++)
	{
	  for (j = 0; j < ncols; j++, k++)
	    {
	      tile_init (&tiles[k], tile_level->bpp);
	      tiles[k].tile_num = k;
	      tiles[k].tm = tm;

	      if (j == (ncols - 1))
		tiles[k].ewidth = right_tile;

	      if (i == (nrows - 1))
		tiles[k].eheight = bottom_tile;
	    }
	}
    }

  return &tile_level->tiles[tile_num];
}

void
tile_manager_validate (TileManager *tm,
		       Tile        *tile)
{
  tile->valid = TRUE;

  if (tm->validate_proc)
    (* tm->validate_proc) (tm, tile, -1);
}

void
tile_manager_invalidate_tiles (TileManager *tm,
			       Tile        *toplevel_tile)
{
  TileLevel *level;
  double x, y;
  int row, col;
  int num;
  int i;

  col = toplevel_tile->tile_num % tm->levels[0].ntile_cols;
  row = toplevel_tile->tile_num / tm->levels[0].ntile_cols;

  x = (col * TILE_WIDTH + toplevel_tile->ewidth / 2.0) / (double) tm->levels[0].width;
  y = (row * TILE_HEIGHT + toplevel_tile->eheight / 2.0) / (double) tm->levels[0].height;

  for (i = 1; i < tm->nlevels; i++)
    {
      level = &tm->levels[i];
      if (level->tiles)
	{
	  col = x * level->width / TILE_WIDTH;
	  row = y * level->height / TILE_HEIGHT;
	  num = row * level->ntile_cols + col;
	  tile_invalidate (&level->tiles[num]);
	}
    }
}

void
tile_manager_invalidate_sublevels (TileManager *tm)
{
  int ntiles;
  int i, j;

  for (i = 1; i < tm->nlevels; i++)
    {
      if (tm->levels[i].tiles)
	{
	  ntiles = tm->levels[i].ntile_rows * tm->levels[i].ntile_cols;
	  for (j = 0; j < ntiles; j++)
	    tile_invalidate (&tm->levels[i].tiles[j]);
	}
    }
}

void
tile_manager_update_tile (TileManager *tm,
			  Tile        *toplevel_tile,
			  int          level)
{
  TileLevel *tile_level;
  Tile *tile;
  guchar *src, *dest;
  double x, y;
  int srcx, srcy;
  int tilex, tiley;
  int tilew, tileh;
  int bpp;
  int row, col;
  int num;
  int i, j, k;

  if ((level < 1) || (level >= tm->nlevels))
    return;

  col = toplevel_tile->tile_num % tm->levels[0].ntile_cols;
  row = toplevel_tile->tile_num / tm->levels[0].ntile_cols;

  x = (col * TILE_WIDTH + toplevel_tile->ewidth / 2.0) / (double) tm->levels[0].width;
  y = (row * TILE_HEIGHT + toplevel_tile->eheight / 2.0) / (double) tm->levels[0].height;

  tilex = ((col * TILE_WIDTH) >> level) % 64;
  tiley = ((row * TILE_HEIGHT) >> level) % 64;

  if (level > 6)
    {
      if (((col % (level - 6)) != 0) ||
	  ((row % (level - 6)) != 0))
	return;

      tilew = 1;
      tileh = 1;
    }
  else
    {
      tilew = (toplevel_tile->ewidth) >> level;
      tileh = (toplevel_tile->eheight) >> level;
    }

  tile_level = &tm->levels[level];
  col = (x * tile_level->width) / TILE_WIDTH;
  row = (y * tile_level->height) / TILE_HEIGHT;
  num = row * tile_level->ntile_cols + col;

  tile = tile_manager_get (tm, num, level);

  tile_ref2 (tile, TRUE);
  tile_ref2 (toplevel_tile, FALSE);

  tilew += tilex;
  tileh += tiley;

  bpp = tile->bpp;

  for (i = tiley; i < tileh; i++)
    {
      srcx = tilex << level;
      srcy = tiley << level;

      src = toplevel_tile->data + (srcy * toplevel_tile->ewidth + srcx) * bpp;
      dest = tile->data + (tiley * tile->ewidth + tilex) * bpp;

      for (j = tilex; j < tilew; j++)
	{
	  for (k = 0; k < bpp; k++)
	    dest[k] = src[k];

	  dest += bpp;
	  src += (bpp << level);
	}
    }

  tile_unref (tile, TRUE);
  tile_unref (toplevel_tile, FALSE);
}


static void
tile_manager_destroy_level (TileLevel *level)
{
  int ntiles;
  int i;

  if (level->tiles)
    {
      ntiles = level->ntile_rows * level->ntile_cols;

      for (i = 0; i < ntiles; i++)
	tile_invalidate (&level->tiles[i]);

      g_free (level->tiles);
    }
}
