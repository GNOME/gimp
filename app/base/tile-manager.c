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

static void tile_manager_destroy_level (TileManager *tm,
					TileLevel *level);
static void tile_invalidate (Tile **tile_ptr, TileManager *tm, int tile_num);
static int tile_manager_get_tile_num (TileManager *tm,
				       int xpixel,
				       int ypixel,
				       int level);


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
    tile_manager_destroy_level (tm, &tm->levels[i]);

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
	tile_manager_destroy_level (tm, &tm->levels[i]);
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
		       int          level,
		       int          wantread,
		       int          wantwrite)
{
  int tile_num;

  tile_num = tile_manager_get_tile_num (tm, xpixel, ypixel, level);
  if (tile_num < 0)
    return NULL;

  return tile_manager_get (tm, tile_num, level, wantread, wantwrite);
}

Tile*
tile_manager_get (TileManager *tm,
		  int          tile_num,
		  int          level,
		  int          wantread,
		  int          wantwrite)
{
  TileLevel *tile_level;
  Tile **tiles;
  Tile **tile_ptr;
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
      tile_level->tiles = g_new (Tile*, ntiles);
      tiles = tile_level->tiles;

      nrows = tile_level->ntile_rows;
      ncols = tile_level->ntile_cols;

      right_tile = tile_level->width - ((ncols - 1) * TILE_WIDTH);
      bottom_tile = tile_level->height - ((nrows - 1) * TILE_HEIGHT);

      for (i = 0, k = 0; i < nrows; i++)
	{
	  for (j = 0; j < ncols; j++, k++)
	    {
	      tiles[k] = g_new (Tile, 1);
	      tile_init (tiles[k], tile_level->bpp);
	      tile_attach (tiles[k], tm, k);

	      if (j == (ncols - 1))
		tiles[k]->ewidth = right_tile;

	      if (i == (nrows - 1))
		tiles[k]->eheight = bottom_tile;
	    }
	}
    }

  tile_ptr = &tile_level->tiles[tile_num];

  if (wantread) 
    {
      TILE_MUTEX_LOCK (*tile_ptr);
      if (wantwrite) 
	{
	  if ((*tile_ptr)->share_count > 1) 
	    {
	      /* Copy-on-write required */
	      Tile *newtile = g_new (Tile, 1);
	      tile_init (newtile, (*tile_ptr)->bpp);
	      newtile->ewidth  = (*tile_ptr)->ewidth;
	      newtile->eheight = (*tile_ptr)->eheight;
	      newtile->valid   = (*tile_ptr)->valid;
	      if ((*tile_ptr)->data != NULL) 
		{
		  newtile->data    = g_new (guchar, tile_size (newtile));
		  memcpy (newtile->data, (*tile_ptr)->data, tile_size (newtile));
		}
	      tile_detach (*tile_ptr, tm, tile_num);
	      TILE_MUTEX_LOCK (newtile);
	      tile_attach (newtile, tm, tile_num);
	      *tile_ptr = newtile;
	    }

	  (*tile_ptr)->write_count++;
	  (*tile_ptr)->dirty = 1;
	}
      TILE_MUTEX_UNLOCK (*tile_ptr);
      tile_lock (*tile_ptr);
    }

  return *tile_ptr;
}

void
tile_manager_get_async (TileManager *tm,
                        int          xpixel,
                        int          ypixel,
                        int          level)
{
  Tile *tile_ptr;
  TileLevel *tile_level;
  int tile_num;

  tile_num = tile_manager_get_tile_num (tm, xpixel, ypixel, level);
  if (tile_num < 0)
    return;

  tile_level = &tm->levels[level];
  tile_ptr = tile_level->tiles[tile_num];

  tile_swap_in_async (tile_ptr);
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

  col = toplevel_tile->tlink->tile_num % tm->levels[0].ntile_cols;
  row = toplevel_tile->tlink->tile_num / tm->levels[0].ntile_cols;

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
	  tile_invalidate (&level->tiles[num], tm, num);
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
	    tile_invalidate (&tm->levels[i].tiles[j], tm, j);
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

  col = toplevel_tile->tlink->tile_num % tm->levels[0].ntile_cols;
  row = toplevel_tile->tlink->tile_num / tm->levels[0].ntile_cols;

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

  tile = tile_manager_get (tm, num, level, TRUE, TRUE);

  tile_lock (toplevel_tile);

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

  tile_release (tile, TRUE);
  tile_release (toplevel_tile, FALSE);
}


static void
tile_manager_destroy_level (TileManager *tm, TileLevel *level)
{
  int ntiles;
  int i;

  if (level->tiles)
    {
      ntiles = level->ntile_rows * level->ntile_cols;

      for (i = 0; i < ntiles; i++)
	{
	  TILE_MUTEX_LOCK (level->tiles[i]);
	  tile_detach (level->tiles[i], tm, i);
	}

      g_free (level->tiles);
    }
}


static void
tile_invalidate (Tile **tile_ptr, TileManager *tm, int tile_num)
{
  Tile *tile = *tile_ptr;

  TILE_MUTEX_LOCK (tile);

  if (!tile->valid) 
    goto leave;

  if (tile->share_count > 1) 
    {
      /* This tile is shared.  Replace it with a new, invalid tile. */
      Tile *newtile = g_new (Tile, 1);
      tile_init (newtile, tile->bpp);
      newtile->ewidth  = tile->ewidth;
      newtile->eheight = tile->eheight;
      tile_detach (tile, tm, tile_num);
      TILE_MUTEX_LOCK (newtile);
      tile_attach (newtile, tm, tile_num);
      tile = *tile_ptr = newtile;
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

leave:
  TILE_MUTEX_UNLOCK (tile);
}


void
tile_manager_map_tile (TileManager *tm,
		       int          xpixel,
		       int          ypixel,
		       int          level,
		       Tile        *srctile)
{
  TileLevel *tile_level;
  int tile_row;
  int tile_col;
  int tile_num;

  /*  printf("#");fflush(stdout); */

  if ((level < 0) || (level >= tm->nlevels))
    {
      g_warning ("tile_manager_map_tile: level out of range.");
      return;
    }

  tile_level = &tm->levels[level];

  if ((xpixel < 0) || (xpixel >= tile_level->width) ||
      (ypixel < 0) || (ypixel >= tile_level->height))
    {
      g_warning ("tile_manager_map_tile: tile co-ord out of range.");
      return;
    }

  tile_row = ypixel / TILE_HEIGHT;
  tile_col = xpixel / TILE_WIDTH;
  tile_num = tile_row * tile_level->ntile_cols + tile_col;

  tile_manager_map (tm, tile_num, level, srctile);
}

void
tile_manager_map (TileManager *tm,
		  int          tile_num,
		  int          level,
		  Tile        *srctile)
{
  TileLevel *tile_level;
  Tile **tiles;
  Tile **tile_ptr;
  int ntiles;
  int nrows, ncols;
  int right_tile;
  int bottom_tile;
  int i, j, k;

  /*  printf("@");fflush(stdout);*/

  if ((level < 0) || (level >= tm->nlevels))
    {
      g_warning ("tile_manager_map: level out of range.");
      return;
    }

  tile_level = &tm->levels[level];
  ntiles = tile_level->ntile_rows * tile_level->ntile_cols;

  if ((tile_num < 0) || (tile_num >= ntiles))
    {
      g_warning ("tile_manager_map: tile out of range.");
      return;
    }

  if (!tile_level->tiles)
    {
      /*      g_warning ("tile_manager_map: empty tile level - init'ing.");*/

      tile_level->tiles = g_new (Tile*, ntiles);
      tiles = tile_level->tiles;

      nrows = tile_level->ntile_rows;
      ncols = tile_level->ntile_cols;

      right_tile = tile_level->width - ((ncols - 1) * TILE_WIDTH);
      bottom_tile = tile_level->height - ((nrows - 1) * TILE_HEIGHT);

      for (i = 0, k = 0; i < nrows; i++)
	{
	  for (j = 0; j < ncols; j++, k++)
	    {
	      /*	      printf(",");fflush(stdout);*/

	      tiles[k] = g_new (Tile, 1);
	      tile_init (tiles[k], tile_level->bpp);
	      tile_attach (tiles[k], tm, k);

	      if (j == (ncols - 1))
		tiles[k]->ewidth = right_tile;

	      if (i == (nrows - 1))
		tiles[k]->eheight = bottom_tile;
	    }
	}

      /*      g_warning ("tile_manager_map: empty tile level - done.");*/
    }

  tile_ptr = &tile_level->tiles[tile_num];

  /*  printf(")");fflush(stdout);*/

  TILE_MUTEX_LOCK (*tile_ptr);
  tile_detach (*tile_ptr, tm, tile_num);

  /*  printf(">");fflush(stdout);*/

  TILE_MUTEX_LOCK (srctile);

  /*  printf(" [src:%p tm:%p tn:%d] ", srctile, tm, tile_num); fflush(stdout);*/

  tile_attach (srctile, tm, tile_num);
  *tile_ptr = srctile;
  TILE_MUTEX_UNLOCK (srctile);

  /*  printf("}");fflush(stdout);*/
}

static int
tile_manager_get_tile_num (TileManager *tm,
		           int xpixel,
		           int ypixel,
		           int level)
{
  TileLevel *tile_level;
  int tile_row;
  int tile_col;
  int tile_num;

  if ((level < 0) || (level >= tm->nlevels))
    return -1;

  tile_level = &tm->levels[level];
  if ((xpixel < 0) || (xpixel >= tile_level->width) ||
      (ypixel < 0) || (ypixel >= tile_level->height))
    return -1;

  tile_row = ypixel / TILE_HEIGHT;
  tile_col = xpixel / TILE_WIDTH;
  tile_num = tile_row * tile_level->ntile_cols + tile_col;

  return tile_num;
}

void 
tile_manager_set_user_data (TileManager *tm,
			    void        *user_data)
{
  tm->user_data = user_data;
}

void *
tile_manager_get_user_data (TileManager *tm)
{
  return tm->user_data;
}

int 
tile_manager_level_width  (TileManager *tm, int level) 
{
  return tm->levels[level].width;
}

int 
tile_manager_level_height (TileManager *tm, int level)
{
  return tm->levels[level].height;
}

int 
tile_manager_level_bpp    (TileManager *tm, int level)
{
  return tm->levels[level].bpp;
}

void
tile_manager_get_tile_coordinates (TileManager *tm, Tile *tile, int *x, int *y)
{
  TileLink *tl;

  for (tl = tile->tlink; tl; tl = tl->next) 
    {
      if (tl->tm == tm) break;
    }

  if (tl == NULL) 
    {
      g_warning ("tile_manager_get_tile_coordinates: tile not attached to manager");
      return;
    }

  *x = TILE_WIDTH * (tl->tile_num % tm->levels[0].ntile_cols);
  *y = TILE_HEIGHT * (tl->tile_num / tm->levels[0].ntile_cols);
}

  

			    
