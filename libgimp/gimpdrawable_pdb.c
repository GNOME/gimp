/* LIBGIMP - The GIMP Library                                                   
 * Copyright (C) 1995-1997 Peter Mattis and Spencer Kimball                
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
#include <string.h>

#include "gimp.h"


#define TILE_WIDTH   _gimp_tile_width
#define TILE_HEIGHT  _gimp_tile_height


extern gint _gimp_tile_width;
extern gint _gimp_tile_height;


GDrawable*
gimp_drawable_get (gint32 drawable_ID)
{
  GDrawable *drawable;

  drawable = g_new (GDrawable, 1);
  drawable->id = drawable_ID;
  drawable->width = gimp_drawable_width (drawable_ID);
  drawable->height = gimp_drawable_height (drawable_ID);
  drawable->bpp = gimp_drawable_bpp (drawable_ID);
  drawable->ntile_rows = (drawable->height + TILE_HEIGHT - 1) / TILE_HEIGHT;
  drawable->ntile_cols = (drawable->width + TILE_WIDTH - 1) / TILE_WIDTH;
  drawable->tiles = NULL;
  drawable->shadow_tiles = NULL;

  return drawable;
}

void
gimp_drawable_detach (GDrawable *drawable)
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
gimp_drawable_flush (GDrawable *drawable)
{
  GTile *tiles;
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
gimp_drawable_delete (GDrawable *drawable)
{
  if (drawable)
    {
      if (gimp_drawable_is_layer (drawable->id))
	gimp_layer_delete (drawable->id);
      else
	gimp_channel_delete (drawable->id);
    }
}

void
gimp_drawable_update (gint32 drawable_ID,
		      gint   x,
		      gint   y,
		      guint  width,
		      guint  height)
{
  GParam *return_vals;
  gint nreturn_vals;

  return_vals = gimp_run_procedure ("gimp_drawable_update",
				    &nreturn_vals,
				    PARAM_DRAWABLE, drawable_ID,
				    PARAM_INT32, x,
				    PARAM_INT32, y,
				    PARAM_INT32, width,
				    PARAM_INT32, height,
				    PARAM_END);

  gimp_destroy_params (return_vals, nreturn_vals);
}

void
gimp_drawable_merge_shadow (gint32 drawable_ID,
			    gint   undoable)
{
  GParam *return_vals;
  gint nreturn_vals;

  return_vals = gimp_run_procedure ("gimp_drawable_merge_shadow",
				    &nreturn_vals,
				    PARAM_DRAWABLE, drawable_ID,
				    PARAM_INT32, undoable,
				    PARAM_END);

  gimp_destroy_params (return_vals, nreturn_vals);
}

gint32
gimp_drawable_image_id (gint32 drawable_ID)
{
  GParam *return_vals;
  gint nreturn_vals;
  gint32 image_ID;

  return_vals = gimp_run_procedure ("gimp_drawable_image",
				    &nreturn_vals,
				    PARAM_DRAWABLE, drawable_ID,
				    PARAM_END);

  image_ID = -1;
  if (return_vals[0].data.d_int32 == STATUS_SUCCESS)
    image_ID = return_vals[1].data.d_image;

  gimp_destroy_params (return_vals, nreturn_vals);

  return image_ID;
}

gchar*
gimp_drawable_name (gint32 drawable_ID)
{
  if (gimp_drawable_is_layer (drawable_ID))
    return gimp_layer_get_name (drawable_ID);
  return gimp_channel_get_name (drawable_ID);
}

guint
gimp_drawable_width (gint32 drawable_ID)
{
  GParam *return_vals;
  gint nreturn_vals;
  guint width;

  return_vals = gimp_run_procedure ("gimp_drawable_width",
				    &nreturn_vals,
				    PARAM_DRAWABLE, drawable_ID,
				    PARAM_END);

  width = 0;
  if (return_vals[0].data.d_int32 == STATUS_SUCCESS)
    width = return_vals[1].data.d_int32;

  gimp_destroy_params (return_vals, nreturn_vals);

  return width;
}

guint
gimp_drawable_height (gint32 drawable_ID)
{
  GParam *return_vals;
  gint nreturn_vals;
  guint height;

  return_vals = gimp_run_procedure ("gimp_drawable_height",
				    &nreturn_vals,
				    PARAM_DRAWABLE, drawable_ID,
				    PARAM_END);

  height = 0;
  if (return_vals[0].data.d_int32 == STATUS_SUCCESS)
    height = return_vals[1].data.d_int32;

  gimp_destroy_params (return_vals, nreturn_vals);

  return height;
}

guint
gimp_drawable_bpp (gint32 drawable_ID)
{
  GParam *return_vals;
  gint nreturn_vals;
  guint bpp;

  return_vals = gimp_run_procedure ("gimp_drawable_bytes",
				    &nreturn_vals,
				    PARAM_DRAWABLE, drawable_ID,
				    PARAM_END);

  bpp = 0;
  if (return_vals[0].data.d_int32 == STATUS_SUCCESS)
    bpp = return_vals[1].data.d_int32;

  gimp_destroy_params (return_vals, nreturn_vals);

  return bpp;
}

GDrawableType
gimp_drawable_type (gint32 drawable_ID)
{
  GParam *return_vals;
  gint nreturn_vals;
  gint result;

  return_vals = gimp_run_procedure ("gimp_drawable_type",
				    &nreturn_vals,
				    PARAM_DRAWABLE, drawable_ID,
				    PARAM_END);

  result = -1;
  if (return_vals[0].data.d_int32 == STATUS_SUCCESS)
    result = return_vals[1].data.d_int32;

  gimp_destroy_params (return_vals, nreturn_vals);

  return result;
}

gboolean
gimp_drawable_visible (gint32 drawable_ID)
{
  if (gimp_drawable_is_layer (drawable_ID))
    return gimp_layer_get_visible (drawable_ID);

  return gimp_channel_get_visible (drawable_ID);
}

gboolean
gimp_drawable_is_channel (gint32 drawable_ID)
{
  GParam   *return_vals;
  gint      nreturn_vals;
  gboolean  result;

  return_vals = gimp_run_procedure ("gimp_drawable_is_channel",
				    &nreturn_vals,
				    PARAM_DRAWABLE, drawable_ID,
				    PARAM_END);

  result = FALSE;
  if (return_vals[0].data.d_status == STATUS_SUCCESS)
    result = return_vals[1].data.d_int32;

  gimp_destroy_params (return_vals, nreturn_vals);

  return result;
}

gboolean
gimp_drawable_is_rgb (gint32 drawable_ID)
{
  GParam   *return_vals;
  gint      nreturn_vals;
  gboolean  result;

  return_vals = gimp_run_procedure ("gimp_drawable_is_rgb",
				    &nreturn_vals,
				    PARAM_DRAWABLE, drawable_ID,
				    PARAM_END);

  result = FALSE;
  if (return_vals[0].data.d_status == STATUS_SUCCESS)
    result = return_vals[1].data.d_int32;

  gimp_destroy_params (return_vals, nreturn_vals);

  return result;
}

gboolean
gimp_drawable_is_gray (gint32 drawable_ID)
{
  GParam   *return_vals;
  gint      nreturn_vals;
  gboolean  result;

  return_vals = gimp_run_procedure ("gimp_drawable_is_gray",
				    &nreturn_vals,
				    PARAM_DRAWABLE, drawable_ID,
				    PARAM_END);

  result = FALSE;
  if (return_vals[0].data.d_status == STATUS_SUCCESS)
    result = return_vals[1].data.d_int32;

  gimp_destroy_params (return_vals, nreturn_vals);

  return result;
}

gboolean
gimp_drawable_has_alpha (gint32 drawable_ID)
{
  GParam   *return_vals;
  gint      nreturn_vals;
  gboolean  result;

  return_vals = gimp_run_procedure ("gimp_drawable_has_alpha",
				    &nreturn_vals,
				    PARAM_DRAWABLE, drawable_ID,
				    PARAM_END);

  result = FALSE;
  if (return_vals[0].data.d_status == STATUS_SUCCESS)
    result = return_vals[1].data.d_int32;

  gimp_destroy_params (return_vals, nreturn_vals);

  return result;
}

gboolean
gimp_drawable_is_indexed (gint32 drawable_ID)
{
  GParam   *return_vals;
  gint      nreturn_vals;
  gboolean  result;

  return_vals = gimp_run_procedure ("gimp_drawable_is_indexed",
				    &nreturn_vals,
				    PARAM_DRAWABLE, drawable_ID,
				    PARAM_END);

  result = FALSE;
  if (return_vals[0].data.d_status == STATUS_SUCCESS)
    result = return_vals[1].data.d_int32;

  gimp_destroy_params (return_vals, nreturn_vals);

  return result;
}

gboolean
gimp_drawable_is_layer (gint32 drawable_ID)
{
  GParam   *return_vals;
  gint      nreturn_vals;
  gboolean  result;

  return_vals = gimp_run_procedure ("gimp_drawable_is_layer",
				    &nreturn_vals,
				    PARAM_DRAWABLE, drawable_ID,
				    PARAM_END);

  result = FALSE;
  if (return_vals[0].data.d_status == STATUS_SUCCESS)
    result = return_vals[1].data.d_int32;

  gimp_destroy_params (return_vals, nreturn_vals);

  return result;
}

gboolean
gimp_drawable_is_layer_mask (gint32 drawable_ID)
{
  GParam   *return_vals;
  gint      nreturn_vals;
  gboolean  result;

  return_vals = gimp_run_procedure ("gimp_drawable_is_layer_mask",
				    &nreturn_vals,
				    PARAM_DRAWABLE, drawable_ID,
				    PARAM_END);

  result = FALSE;
  if (return_vals[0].data.d_status == STATUS_SUCCESS)
    result = return_vals[1].data.d_int32;

  gimp_destroy_params (return_vals, nreturn_vals);

  return result;
}

gboolean
gimp_drawable_mask_bounds (gint32  drawable_ID,
			   gint   *x1,
			   gint   *y1,
			   gint   *x2,
			   gint   *y2)
{
  GParam   *return_vals;
  gint      nreturn_vals;
  gboolean  result;

  return_vals = gimp_run_procedure ("gimp_drawable_mask_bounds",
				    &nreturn_vals,
				    PARAM_DRAWABLE, drawable_ID,
				    PARAM_END);

  result = FALSE;
  if (return_vals[0].data.d_status == STATUS_SUCCESS)
    {
      result = return_vals[1].data.d_int32;
      *x1 = return_vals[2].data.d_int32;
      *y1 = return_vals[3].data.d_int32;
      *x2 = return_vals[4].data.d_int32;
      *y2 = return_vals[5].data.d_int32;
    }

  gimp_destroy_params (return_vals, nreturn_vals);

  return result;
}

void
gimp_drawable_offsets (gint32  drawable_ID,
		       gint   *offset_x,
		       gint   *offset_y)
{
  GParam *return_vals;
  gint nreturn_vals;

  return_vals = gimp_run_procedure ("gimp_drawable_offsets",
				    &nreturn_vals,
				    PARAM_DRAWABLE, drawable_ID,
				    PARAM_END);

  if (return_vals[0].data.d_status == STATUS_SUCCESS)
    {
      *offset_x = return_vals[1].data.d_int32;
      *offset_y = return_vals[2].data.d_int32;
    }

  gimp_destroy_params (return_vals, nreturn_vals);
}

void
gimp_drawable_fill (gint32       drawable_ID,
		    GimpFillType fill_type)
{
  GParam *return_vals;
  gint nreturn_vals;

  return_vals = gimp_run_procedure ("gimp_drawable_fill",
				    &nreturn_vals,
				    PARAM_DRAWABLE, drawable_ID,
				    PARAM_INT32, fill_type,
				    PARAM_END);

  gimp_destroy_params (return_vals, nreturn_vals);
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

GTile*
gimp_drawable_get_tile (GDrawable *drawable,
			gint      shadow,
			gint      row,
			gint      col)
{
  GTile *tiles;
  guint right_tile;
  guint bottom_tile;
  gint ntiles;
  gint tile_num;
  gint i, j, k;

  if (drawable)
    {
      if (shadow)
	tiles = drawable->shadow_tiles;
      else
	tiles = drawable->tiles;

      if (!tiles)
	{
	  ntiles = drawable->ntile_rows * drawable->ntile_cols;
	  tiles = g_new (GTile, ntiles);

	  right_tile = drawable->width - ((drawable->ntile_cols - 1) * TILE_WIDTH);
	  bottom_tile = drawable->height - ((drawable->ntile_rows - 1) * TILE_HEIGHT);

	  for (i = 0, k = 0; i < drawable->ntile_rows; i++)
	    {
	      for (j = 0; j < drawable->ntile_cols; j++, k++)
		{
		  tiles[k].bpp = drawable->bpp;
		  tiles[k].tile_num = k;
		  tiles[k].ref_count = 0;
		  tiles[k].dirty = FALSE;
		  tiles[k].shadow = shadow;
		  tiles[k].data = NULL;
		  tiles[k].drawable = drawable;

		  if (j == (drawable->ntile_cols - 1))
		    tiles[k].ewidth = right_tile;
		  else
		    tiles[k].ewidth = TILE_WIDTH;

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

GTile*
gimp_drawable_get_tile2 (GDrawable *drawable,
			 gint      shadow,
			 gint      x,
			 gint      y)
{
  gint row, col;

  col = x / TILE_WIDTH;
  row = y / TILE_HEIGHT;

  return gimp_drawable_get_tile (drawable, shadow, row, col);
}

GimpParasite *
gimp_drawable_parasite_find (gint32       drawable_ID,
			     const gchar *name)

{
  GParam *return_vals;
  gint nreturn_vals;
  GimpParasite *parasite;
  return_vals = gimp_run_procedure ("gimp_drawable_parasite_find",
				    &nreturn_vals,
				    PARAM_DRAWABLE, drawable_ID,
				    PARAM_STRING, name,
				    PARAM_END);

  if (return_vals[0].data.d_status == STATUS_SUCCESS)
    {
      parasite = gimp_parasite_copy (&return_vals[1].data.d_parasite);
    }
  else
    parasite = NULL;

  gimp_destroy_params (return_vals, nreturn_vals);
  
  return parasite;
}

void
gimp_drawable_parasite_attach (gint32              drawable_ID,
			       const GimpParasite *parasite)
{
  GParam *return_vals;
  gint nreturn_vals;

  return_vals = gimp_run_procedure ("gimp_drawable_parasite_attach",
				    &nreturn_vals,
				    PARAM_DRAWABLE, drawable_ID,
				    PARAM_PARASITE, parasite,
				    PARAM_END);

  gimp_destroy_params (return_vals, nreturn_vals);
}


void
gimp_drawable_attach_new_parasite (gint32          drawable, 
				   const gchar    *name, 
				   gint            flags,
				   gint            size, 
				   const gpointer  data)
{
  GParam *return_vals;
  gint nreturn_vals;
  GimpParasite *p = gimp_parasite_new (name, flags, size, data);

  return_vals = gimp_run_procedure ("gimp_drawable_parasite_attach",
				    &nreturn_vals,
				    PARAM_DRAWABLE, drawable,
				    PARAM_PARASITE, p,
				    PARAM_END);

  gimp_parasite_free(p);
  gimp_destroy_params (return_vals, nreturn_vals);
}

void
gimp_drawable_parasite_detach (gint32       drawable_ID,
			       const gchar *name)
{
  GParam *return_vals;
  gint nreturn_vals;

  return_vals = gimp_run_procedure ("gimp_drawable_parasite_detach",
				    &nreturn_vals,
				    PARAM_DRAWABLE, drawable_ID,
				    PARAM_STRING, name,
				    PARAM_END);


  gimp_destroy_params (return_vals, nreturn_vals);
}

guchar *
gimp_drawable_get_thumbnail_data (gint32 drawable_ID,
				  gint  *width,
				  gint  *height,
				  gint  *bytes)
{
  GParam  *return_vals;
  gint     nreturn_vals;
  guchar  *drawable_data = NULL;

  return_vals = gimp_run_procedure ("gimp_drawable_thumbnail",
				    &nreturn_vals,
				    PARAM_DRAWABLE, drawable_ID,
				    PARAM_INT32, *width,
				    PARAM_INT32, *height,
				    PARAM_END);

  if (return_vals[0].data.d_status == STATUS_SUCCESS)
    {
      *width = return_vals[1].data.d_int32;
      *height = return_vals[2].data.d_int32;
      *bytes = return_vals[3].data.d_int32;
      drawable_data = g_new (guchar, return_vals[4].data.d_int32);
      g_memmove (drawable_data, return_vals[5].data.d_int32array, return_vals[4].data.d_int32);
    }

  gimp_destroy_params (return_vals, nreturn_vals);

  return drawable_data;
}


