/* LIBGIMP - The GIMP Library
 * Copyright (C) 1995-1997 Peter Mattis and Spencer Kimball
 *
 * gimpdrawable.c
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Library General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the
 * Free Software Foundation, Inc., 59 Temple Place - Suite 330,
 * Boston, MA 02111-1307, USA.
 */

#include <glib.h>

#include "gimp.h"


#define TILE_WIDTH   _gimp_tile_width
#define TILE_HEIGHT  _gimp_tile_height


extern gint _gimp_tile_width;
extern gint _gimp_tile_height;


GimpDrawable *
gimp_drawable_get (gint32 drawable_ID)
{
  GimpDrawable *drawable;

  drawable = g_new (GimpDrawable, 1);
  drawable->id           = drawable_ID;
  drawable->width        = gimp_drawable_width  (drawable_ID);
  drawable->height       = gimp_drawable_height (drawable_ID);
  drawable->bpp          = gimp_drawable_bpp    (drawable_ID);
  drawable->ntile_rows   = (drawable->height + TILE_HEIGHT - 1) / TILE_HEIGHT;
  drawable->ntile_cols   = (drawable->width  + TILE_WIDTH  - 1) / TILE_WIDTH;
  drawable->tiles        = NULL;
  drawable->shadow_tiles = NULL;

  return drawable;
}

void
gimp_drawable_detach (GimpDrawable *drawable)
{
  if (drawable)
    {
      gimp_drawable_flush (drawable);
      if (drawable->tiles)
	g_free (drawable->tiles);
      if (drawable->shadow_tiles)
	g_free (drawable->shadow_tiles);
      g_free (drawable);
    }
}

void
gimp_drawable_flush (GimpDrawable *drawable)
{
  GimpTile *tiles;
  gint ntiles;
  gint i;

  if (drawable)
    {
      if (drawable->tiles)
	{
	  tiles = drawable->tiles;
	  ntiles = drawable->ntile_rows * drawable->ntile_cols;

	  for (i = 0; i < ntiles; i++)
	    if ((tiles[i].ref_count > 0) && tiles[i].dirty)
	      gimp_tile_flush (&tiles[i]);
	}

      if (drawable->shadow_tiles)
	{
	  tiles = drawable->shadow_tiles;
	  ntiles = drawable->ntile_rows * drawable->ntile_cols;

	  for (i = 0; i < ntiles; i++)
	    if ((tiles[i].ref_count > 0) && tiles[i].dirty)
	      gimp_tile_flush (&tiles[i]);
	}
    }
}

void
gimp_drawable_delete (GimpDrawable *drawable)
{
  if (drawable)
    {
      if (gimp_drawable_is_layer (drawable->id))
	gimp_layer_delete (drawable->id);
      else
	gimp_channel_delete (drawable->id);
    }
}


gchar *
gimp_drawable_name (gint32 drawable_ID)
{
  if (gimp_drawable_is_layer (drawable_ID))
    return gimp_layer_get_name (drawable_ID);

  return gimp_channel_get_name (drawable_ID);
}


gboolean
gimp_drawable_visible (gint32 drawable_ID)
{
  if (gimp_drawable_is_layer (drawable_ID))
    return gimp_layer_get_visible (drawable_ID);

  return gimp_channel_get_visible (drawable_ID);
}


void
gimp_drawable_set_name (gint32  drawable_ID,
			gchar  *name)
{
  if (gimp_drawable_is_layer (drawable_ID))
    gimp_layer_set_name (drawable_ID, name);
  else
    gimp_channel_set_name (drawable_ID, name);
}

void
gimp_drawable_set_visible (gint32 drawable_ID,
			   gint   visible)
{
  if (gimp_drawable_is_layer (drawable_ID))
    gimp_layer_set_visible (drawable_ID, visible);
  else
    gimp_channel_set_visible (drawable_ID, visible);
}

GimpTile *
gimp_drawable_get_tile (GimpDrawable *drawable,
			gboolean      shadow,
			gint          row,
			gint          col)
{
  GimpTile *tiles;
  guint right_tile;
  guint bottom_tile;
  gint  ntiles;
  gint  tile_num;
  gint  i, j, k;

  if (drawable)
    {
      if (shadow)
	tiles = drawable->shadow_tiles;
      else
	tiles = drawable->tiles;

      if (!tiles)
	{
	  ntiles = drawable->ntile_rows * drawable->ntile_cols;
	  tiles = g_new (GimpTile, ntiles);

	  right_tile = drawable->width - ((drawable->ntile_cols - 1) * TILE_WIDTH);
	  bottom_tile = drawable->height - ((drawable->ntile_rows - 1) * TILE_HEIGHT);

	  for (i = 0, k = 0; i < drawable->ntile_rows; i++)
	    {
	      for (j = 0; j < drawable->ntile_cols; j++, k++)
		{
		  tiles[k].bpp       = drawable->bpp;
		  tiles[k].tile_num  = k;
		  tiles[k].ref_count = 0;
		  tiles[k].dirty     = FALSE;
		  tiles[k].shadow    = shadow;
		  tiles[k].data      = NULL;
		  tiles[k].drawable  = drawable;

		  if (j == (drawable->ntile_cols - 1))
		    tiles[k].ewidth  = right_tile;
		  else
		    tiles[k].ewidth  = TILE_WIDTH;

		  if (i == (drawable->ntile_rows - 1))
		    tiles[k].eheight = bottom_tile;
		  else
		    tiles[k].eheight = TILE_HEIGHT;
		}
	    }

	  if (shadow)
	    drawable->shadow_tiles = tiles;
	  else
	    drawable->tiles = tiles;
	}

      tile_num = row * drawable->ntile_cols + col;
      return &tiles[tile_num];
    }

  return NULL;
}

GimpTile *
gimp_drawable_get_tile2 (GimpDrawable *drawable,
			 gint          shadow,
			 gint          x,
			 gint          y)
{
  gint row;
  gint col;

  col = x / TILE_WIDTH;
  row = y / TILE_HEIGHT;

  return gimp_drawable_get_tile (drawable, shadow, row, col);
}

guchar *
gimp_drawable_get_thumbnail_data (gint32  drawable_ID,
				  gint   *width,
				  gint   *height,
				  gint   *bpp)
{
  gint    ret_width;
  gint    ret_height;
  guchar *image_data;
  gint    data_size;

  _gimp_drawable_thumbnail (drawable_ID,
			    *width,
			    *height,
			    &ret_width,
			    &ret_height,
			    bpp,
			    &data_size,
			    &image_data);

  *width  = ret_width;
  *height = ret_height;

  return image_data;
}
