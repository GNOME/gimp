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

#include "libgimp/gimpintl.h"

static int tile_manager_get_tile_num (TileManager *tm,
				       int xpixel,
				       int ypixel);


TileManager*
tile_manager_new (int toplevel_width,
		  int toplevel_height,
		  int bpp)
{
  TileManager *tm;
  int width, height;

  tm = g_new (TileManager, 1);

  tm->user_data = NULL;
  tm->validate_proc = NULL;

  width = toplevel_width;
  height = toplevel_height;

  tm->width = width;
  tm->height = height;
  tm->bpp = bpp;
  tm->ntile_rows = (height + TILE_HEIGHT - 1) / TILE_HEIGHT;
  tm->ntile_cols = (width + TILE_WIDTH - 1) / TILE_WIDTH;
  tm->tiles = NULL;
  
  return tm;
}

void
tile_manager_destroy (TileManager *tm)
{
  int ntiles;
  int i;

  if (tm->tiles)
    {
      ntiles = tm->ntile_rows * tm->ntile_cols;

      for (i = 0; i < ntiles; i++)
	{
	  TILE_MUTEX_LOCK (tm->tiles[i]);
	  tile_detach (tm->tiles[i], tm, i);
	}

      g_free (tm->tiles);
    }

  g_free (tm);
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
		       int          wantread,
		       int          wantwrite)
{
  int tile_num;

  tile_num = tile_manager_get_tile_num (tm, xpixel, ypixel);
  if (tile_num < 0)
    return NULL;

  return tile_manager_get (tm, tile_num, wantread, wantwrite);
}

Tile*
tile_manager_get (TileManager *tm,
		  int          tile_num,
		  int          wantread,
		  int          wantwrite)
{
  Tile **tiles;
  Tile **tile_ptr;
  int ntiles;
  int nrows, ncols;
  int right_tile;
  int bottom_tile;
  int i, j, k;

  ntiles = tm->ntile_rows * tm->ntile_cols;

  if ((tile_num < 0) || (tile_num >= ntiles))
    return NULL;

  if (!tm->tiles)
    {
      tm->tiles = g_new (Tile*, ntiles);
      tiles = tm->tiles;

      nrows = tm->ntile_rows;
      ncols = tm->ntile_cols;

      right_tile = tm->width - ((ncols - 1) * TILE_WIDTH);
      bottom_tile = tm->height - ((nrows - 1) * TILE_HEIGHT);

      for (i = 0, k = 0; i < nrows; i++)
	{
	  for (j = 0; j < ncols; j++, k++)
	    {
	      tiles[k] = g_new (Tile, 1);
	      tile_init (tiles[k], tm->bpp);
	      tile_attach (tiles[k], tm, k);

	      if (j == (ncols - 1))
		tiles[k]->ewidth = right_tile;

	      if (i == (nrows - 1))
		tiles[k]->eheight = bottom_tile;
	    }
	}
    }

  tile_ptr = &tm->tiles[tile_num];

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
                        int          ypixel)
{
  Tile *tile_ptr;
  int tile_num;

  tile_num = tile_manager_get_tile_num (tm, xpixel, ypixel);
  if (tile_num < 0)
    return;

  tile_ptr = tm->tiles[tile_num];

  tile_swap_in_async (tile_ptr);
}

void
tile_manager_validate (TileManager *tm,
		       Tile        *tile)
{
  tile->valid = TRUE;

  if (tm->validate_proc)
    (* tm->validate_proc) (tm, tile);
}

void
tile_manager_invalidate_tiles (TileManager *tm,
			       Tile        *toplevel_tile)
{
  double x, y;
  int row, col;
  int num;

  col = toplevel_tile->tlink->tile_num % tm->ntile_cols;
  row = toplevel_tile->tlink->tile_num / tm->ntile_cols;

  x = (col * TILE_WIDTH + toplevel_tile->ewidth / 2.0) / (double) tm->width;
  y = (row * TILE_HEIGHT + toplevel_tile->eheight / 2.0) / (double) tm->height;

  if (tm->tiles)
    {
      col = x * tm->width / TILE_WIDTH;
      row = y * tm->height / TILE_HEIGHT;
      num = row * tm->ntile_cols + col;
      tile_invalidate (&tm->tiles[num], tm, num);
    }
}


void
tile_invalidate_tile (Tile **tile_ptr, TileManager *tm, 
		      int xpixel, int ypixel)
{
  int tile_num;

  tile_num = tile_manager_get_tile_num (tm, xpixel, ypixel);
  if (tile_num < 0) return;
  
  tile_invalidate (tile_ptr, tm, tile_num);
}


void
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
		       Tile        *srctile)
{
  int tile_row;
  int tile_col;
  int tile_num;

  if ((xpixel < 0) || (xpixel >= tm->width) ||
      (ypixel < 0) || (ypixel >= tm->height))
    {
      g_warning (_("tile_manager_map_tile: tile co-ord out of range."));
      return;
    }

  tile_row = ypixel / TILE_HEIGHT;
  tile_col = xpixel / TILE_WIDTH;
  tile_num = tile_row * tm->ntile_cols + tile_col;

  tile_manager_map (tm, tile_num, srctile);
}

void
tile_manager_map (TileManager *tm,
		  int          tile_num,
		  Tile        *srctile)
{
  Tile **tiles;
  Tile **tile_ptr;
  int ntiles;
  int nrows, ncols;
  int right_tile;
  int bottom_tile;
  int i, j, k;

  ntiles = tm->ntile_rows * tm->ntile_cols;

  if ((tile_num < 0) || (tile_num >= ntiles))
    {
      g_warning (_("tile_manager_map: tile out of range."));
      return;
    }

  if (!tm->tiles)
    {
      g_warning ("tile_manager_map: empty tile level - init'ing.");

      tm->tiles = g_new (Tile*, ntiles);
      tiles = tm->tiles;

      nrows = tm->ntile_rows;
      ncols = tm->ntile_cols;

      right_tile = tm->width - ((ncols - 1) * TILE_WIDTH);
      bottom_tile = tm->height - ((nrows - 1) * TILE_HEIGHT);

      for (i = 0, k = 0; i < nrows; i++)
	{
	  for (j = 0; j < ncols; j++, k++)
	    {
	      /*	      printf(",");fflush(stdout);*/

	      tiles[k] = g_new (Tile, 1);
	      tile_init (tiles[k], tm->bpp);
	      tile_attach (tiles[k], tm, k);

	      if (j == (ncols - 1))
		tiles[k]->ewidth = right_tile;

	      if (i == (nrows - 1))
		tiles[k]->eheight = bottom_tile;
	    }
	}

      /*      g_warning ("tile_manager_map: empty tile level - done.");*/
    }

  tile_ptr = &tm->tiles[tile_num];

  /*  printf(")");fflush(stdout);*/

  TILE_MUTEX_LOCK (*tile_ptr);
  if ((*tile_ptr)->ewidth  != srctile->ewidth ||
      (*tile_ptr)->eheight != srctile->eheight ||
      (*tile_ptr)->bpp     != srctile->bpp) {
    g_warning (_("tile_manager_map: nonconformant map (%p -> %p)"),
	       srctile, *tile_ptr);
  }
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
		           int ypixel)
{
  int tile_row;
  int tile_col;
  int tile_num;

  if ((xpixel < 0) || (xpixel >= tm->width) ||
      (ypixel < 0) || (ypixel >= tm->height))
    return -1;

  tile_row = ypixel / TILE_HEIGHT;
  tile_col = xpixel / TILE_WIDTH;
  tile_num = tile_row * tm->ntile_cols + tile_col;

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
tile_manager_level_width  (TileManager *tm) 
{
  return tm->width;
}

int 
tile_manager_level_height (TileManager *tm)
{
  return tm->height;
}

int 
tile_manager_level_bpp    (TileManager *tm)
{
  return tm->bpp;
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
      g_warning (_("tile_manager_get_tile_coordinates: tile not attached to manager"));
      return;
    }

  *x = TILE_WIDTH * (tl->tile_num % tm->ntile_cols);
  *y = TILE_HEIGHT * (tl->tile_num / tm->ntile_cols);
}

  
void
tile_manager_map_over_tile (TileManager *tm, Tile *tile, Tile *srctile)
{
  TileLink *tl;

  for (tl = tile->tlink; tl; tl = tl->next) 
    {
      if (tl->tm == tm) break;
    }

  if (tl == NULL) 
    {
      g_warning (_("tile_manager_map_over_tile: tile not attached to manager"));
      return;
    }

  tile_manager_map (tm, tl->tile_num, srctile);
}

  

			    
