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

#include "config.h"

#include <glib-object.h>

#include "libgimpcolor/gimpcolor.h"

#include "core-types.h"

#include "base/pixel-region.h"
#include "base/tile-manager.h"
#include "base/tile.h"

#include "paint-funcs/paint-funcs.h"

#include "gimp.h"
#include "gimpimage.h"
#include "gimpimage-projection.h"
#include "gimplayer.h"
#include "gimplayer-floating-sel.h"
#include "gimplayermask.h"
#include "gimplist.h"


/*  local function prototypes  */

static void     gimp_image_projection_validate_tile (TileManager     *tm,
                                                     Tile            *tile);
static void     gimp_image_construct_layers         (GimpImage      *gimage,
                                                     gint            x,
                                                     gint            y,
                                                     gint            w,
                                                     gint            h);
static void     gimp_image_construct_channels       (GimpImage      *gimage,
                                                     gint            x,
                                                     gint            y,
                                                     gint            w,
                                                     gint            h);
static void     gimp_image_initialize_projection    (GimpImage      *gimage,
                                                     gint            x,
                                                     gint            y,
                                                     gint            w,
                                                     gint            h);
static void     gimp_image_construct                (GimpImage      *gimage,
                                                     gint            x,
                                                     gint            y,
                                                     gint            w,
                                                     gint            h);

static void     project_intensity                   (GimpImage      *gimage,
                                                     GimpLayer      *layer,
                                                     PixelRegion    *src,
                                                     PixelRegion    *dest,
                                                     PixelRegion    *mask);
static void     project_intensity_alpha             (GimpImage      *gimage,
                                                     GimpLayer      *layer,
                                                     PixelRegion    *src,
                                                     PixelRegion    *dest,
                                                     PixelRegion    *mask);
static void     project_indexed                     (GimpImage      *gimage,
                                                     GimpLayer      *layer,
                                                     PixelRegion    *src,
                                                     PixelRegion    *dest);
static void     project_indexed_alpha               (GimpImage      *gimage,
                                                     GimpLayer      *layer,
                                                     PixelRegion    *src,
                                                     PixelRegion    *dest,
                                                     PixelRegion    *mask);
static void     project_channel                     (GimpImage      *gimage,
                                                     GimpChannel    *channel,
                                                     PixelRegion    *src,
                                                     PixelRegion    *src2);


/*  public functions  */

void
gimp_image_projection_allocate (GimpImage *gimage)
{
  GimpImageType proj_type  = 0;
  gint          proj_bytes = 0;

  g_return_if_fail (GIMP_IS_IMAGE (gimage));

  /*  Find the number of bytes required for the projection.
   *  This includes the intensity channels and an alpha channel
   *  if one doesn't exist.
   */
  switch (gimp_image_base_type (gimage))
    {
    case GIMP_RGB:
    case GIMP_INDEXED:
      proj_bytes = 4;
      proj_type  = GIMP_RGBA_IMAGE;
      break;

    case GIMP_GRAY:
      proj_bytes = 2;
      proj_type  = GIMP_GRAYA_IMAGE;
      break;

    default:
      g_assert_not_reached ();
    }

  if (gimage->projection)
    {
      if (proj_type      != gimage->proj_type                       ||
          proj_bytes     != gimage->proj_bytes                      ||
          gimage->width  != tile_manager_width (gimage->projection) ||
          gimage->height != tile_manager_height (gimage->projection))
        {
          gimp_image_projection_free (gimage);
        }
    }

  if (! gimage->projection)
    {
      gimage->proj_type  = proj_type;
      gimage->proj_bytes = proj_bytes;

      gimage->projection = tile_manager_new (gimage->width, gimage->height,
                                             gimage->proj_bytes);
      tile_manager_set_user_data (gimage->projection, gimage);
      tile_manager_set_validate_proc (gimage->projection,
                                      gimp_image_projection_validate_tile);
    }
}

void
gimp_image_projection_free (GimpImage *gimage)
{
  g_return_if_fail (GIMP_IS_IMAGE (gimage));

  if (gimage->projection)
    {
      tile_manager_unref (gimage->projection);
      gimage->projection = NULL;
    }
}

TileManager *
gimp_image_projection (GimpImage *gimage)
{
  g_return_val_if_fail (GIMP_IS_IMAGE (gimage), NULL);

  if (gimage->projection == NULL                                ||
      tile_manager_width  (gimage->projection) != gimage->width ||
      tile_manager_height (gimage->projection) != gimage->height)
    {
      gimp_image_projection_allocate (gimage);
    }

  return gimage->projection;
}

GimpImageType
gimp_image_projection_type (const GimpImage *gimage)
{
  g_return_val_if_fail (GIMP_IS_IMAGE (gimage), -1);

  return gimage->proj_type;
}

gint
gimp_image_projection_bytes (const GimpImage *gimage)
{
  g_return_val_if_fail (GIMP_IS_IMAGE (gimage), -1);

  return gimage->proj_bytes;
}

gdouble
gimp_image_projection_opacity (const GimpImage *gimage)
{
  g_return_val_if_fail (GIMP_IS_IMAGE (gimage), GIMP_OPACITY_OPAQUE);

  return GIMP_OPACITY_OPAQUE;
}

guchar *
gimp_image_projection_get_color_at (GimpImage *gimage,
                                    gint       x,
                                    gint       y)
{
  Tile   *tile;
  guchar *src;
  guchar *dest;

  g_return_val_if_fail (GIMP_IS_IMAGE (gimage), NULL);

  if (x < 0 || y < 0 || x >= gimage->width || y >= gimage->height)
    return NULL;

  dest = g_new (guchar, 5);
  tile = tile_manager_get_tile (gimp_image_projection (gimage), x, y,
				TRUE, FALSE);
  src = tile_data_pointer (tile, x % TILE_WIDTH, y % TILE_HEIGHT);
  gimp_image_get_color (gimage, gimp_image_projection_type (gimage), src, dest);

  dest[4] = 0;
  tile_release (tile, FALSE);

  return dest;
}

void
gimp_image_invalidate (GimpImage *gimage,
		       gint       x,
		       gint       y,
		       gint       w,
		       gint       h,
		       gint       x1,
		       gint       y1,
		       gint       x2,
		       gint       y2)
{
  Tile        *tile;
  TileManager *tm;
  gint         i, j;
  gint         startx, starty;
  gint         endx, endy;
  gint         tilex, tiley;

  g_return_if_fail (GIMP_IS_IMAGE (gimage));

  tm = gimp_image_projection (gimage);

  startx = x;
  starty = y;
  endx   = x + w;
  endy   = y + h;

  /*  invalidate all tiles which are located outside of the displayed area
   *   all tiles inside the displayed area are constructed.
   */
  for (i = y; i < (y + h); i += (TILE_HEIGHT - (i % TILE_HEIGHT)))
    for (j = x; j < (x + w); j += (TILE_WIDTH - (j % TILE_WIDTH)))
      {
	tile = tile_manager_get_tile (tm, j, i, FALSE, FALSE);

        /*  check if the tile is outside the bounds  */
        if ((MIN ((j + tile_ewidth (tile)), x2) - MAX (j, x1)) <= 0)
          {
            tile_invalidate_tile (&tile, tm, j, i);

            if (j < x1)
              startx = MAX (startx,
                            (j - (j % TILE_WIDTH) + tile_ewidth (tile)));
            else
              endx = MIN (endx, j);
          }
        else if (MIN ((i + tile_eheight (tile)), y2) - MAX (i, y1) <= 0)
          {
            tile_invalidate_tile (&tile, tm, j, i);

            if (i < y1)
              starty = MAX (starty,
                            (i - (i % TILE_HEIGHT) + tile_eheight (tile)));
            else
              endy = MIN (endy, i);
          }
        else
          {
            /*  If the tile is not valid, make sure we get the entire tile
             *   in the construction extents
             */
            if (tile_is_valid (tile) == FALSE)
              {
                tilex = j - (j % TILE_WIDTH);
                tiley = i - (i % TILE_HEIGHT);

                startx = MIN (startx, tilex);
                endx   = MAX (endx, tilex + tile_ewidth (tile));
                starty = MIN (starty, tiley);
                endy   = MAX (endy, tiley + tile_eheight (tile));

                tile_mark_valid (tile); /* hmmmmmmm..... */
              }
          }
      }

  if ((endx - startx) > 0 && (endy - starty) > 0)
    gimp_image_construct (gimage,
			  startx, starty,
			  (endx - startx), (endy - starty));
}

void
gimp_image_invalidate_without_render (GimpImage *gimage,
				      gint       x,
				      gint       y,
				      gint       w,
				      gint       h,
				      gint       x1,
				      gint       y1,
				      gint       x2,
				      gint       y2)
{
  Tile        *tile;
  TileManager *tm;
  gint         i, j;

  g_return_if_fail (GIMP_IS_IMAGE (gimage));

  tm = gimp_image_projection (gimage);

  /*  invalidate all tiles which are located outside of the displayed area
   *   all tiles inside the displayed area are constructed.
   */
  for (i = y; i < (y + h); i += (TILE_HEIGHT - (i % TILE_HEIGHT)))
    for (j = x; j < (x + w); j += (TILE_WIDTH - (j % TILE_WIDTH)))
      {
	tile = tile_manager_get_tile (tm, j, i, FALSE, FALSE);

        /*  check if the tile is outside the bounds  */
        if ((MIN ((j + tile_ewidth(tile)), x2) - MAX (j, x1)) <= 0)
          {
            tile_invalidate_tile (&tile, tm, j, i);
          }
        else if (MIN ((i + tile_eheight(tile)), y2) - MAX (i, y1) <= 0)
          {
            tile_invalidate_tile (&tile, tm, j, i);
          }
      }
}


/*  private functions  */

static void
gimp_image_projection_validate_tile (TileManager *tm,
                                     Tile        *tile)
{
  GimpImage *gimage;
  gint       x, y;
  gint       w, h;

  /*  Get the gimage from the tilemanager  */
  gimage = (GimpImage *) tile_manager_get_user_data (tm);

  gimp_set_busy_until_idle (gimage->gimp);

  /*  Find the coordinates of this tile  */
  tile_manager_get_tile_coordinates (tm, tile, &x, &y);
  w = tile_ewidth  (tile);
  h = tile_eheight (tile);

  gimp_image_construct (gimage, x, y, w, h);
}

static void
gimp_image_construct_layers (GimpImage *gimage,
			     gint       x,
			     gint       y,
			     gint       w,
			     gint       h)
{
  GimpLayer   *layer;
  gint         x1, y1, x2, y2;
  PixelRegion  src1PR, src2PR, maskPR;
  PixelRegion * mask;
  GList       *list;
  GList       *reverse_list;
  gint         off_x;
  gint         off_y;

  /*  composite the floating selection if it exists  */
  if ((layer = gimp_image_floating_sel (gimage)))
    floating_sel_composite (layer, x, y, w, h, FALSE);


  reverse_list = NULL;

  for (list = GIMP_LIST (gimage->layers)->list;
       list;
       list = g_list_next (list))
    {
      layer = (GimpLayer *) list->data;

      /*  only add layers that are visible and not floating selections
       *  to the list
       */
      if (! gimp_layer_is_floating_sel (layer) &&
	  gimp_item_get_visible (GIMP_ITEM (layer)))
	{
	  reverse_list = g_list_prepend (reverse_list, layer);
	}
    }

  for (list = reverse_list; list; list = g_list_next (list))
    {
      layer = (GimpLayer *) list->data;

      gimp_item_offsets (GIMP_ITEM (layer), &off_x, &off_y);

      x1 = CLAMP (off_x, x, x + w);
      y1 = CLAMP (off_y, y, y + h);
      x2 = CLAMP (off_x + gimp_item_width  (GIMP_ITEM (layer)), x, x + w);
      y2 = CLAMP (off_y + gimp_item_height (GIMP_ITEM (layer)), y, y + h);

      /* configure the pixel regions  */
      pixel_region_init (&src1PR, gimp_image_projection (gimage),
			 x1, y1, (x2 - x1), (y2 - y1),
			 TRUE);

      /*  If we're showing the layer mask instead of the layer...  */
      if (layer->mask && layer->mask->show_mask)
	{
	  pixel_region_init (&src2PR,
			     gimp_drawable_data (GIMP_DRAWABLE (layer->mask)),
			     (x1 - off_x), (y1 - off_y),
			     (x2 - x1), (y2 - y1), FALSE);

	  copy_gray_to_region (&src2PR, &src1PR);
	}
      /*  Otherwise, normal  */
      else
	{
	  pixel_region_init (&src2PR,
			     gimp_drawable_data (GIMP_DRAWABLE (layer)),
			     (x1 - off_x), (y1 - off_y),
			     (x2 - x1), (y2 - y1), FALSE);

	  if (layer->mask && layer->mask->apply_mask)
	    {
	      pixel_region_init (&maskPR,
				 gimp_drawable_data (GIMP_DRAWABLE (layer->mask)),
				 (x1 - off_x), (y1 - off_y),
				 (x2 - x1), (y2 - y1), FALSE);
	      mask = &maskPR;
	    }
	  else
	    mask = NULL;

	  /*  Based on the type of the layer, project the layer onto the
	   *  projection image...
	   */
	  switch (gimp_drawable_type (GIMP_DRAWABLE (layer)))
	    {
	    case GIMP_RGB_IMAGE: case GIMP_GRAY_IMAGE:
	      /* no mask possible */
	      project_intensity (gimage, layer, &src2PR, &src1PR, mask);
	      break;

	    case GIMP_RGBA_IMAGE: case GIMP_GRAYA_IMAGE:
	      project_intensity_alpha (gimage, layer, &src2PR, &src1PR, mask);
	      break;

	    case GIMP_INDEXED_IMAGE:
	      /* no mask possible */
	      project_indexed (gimage, layer, &src2PR, &src1PR);
	      break;

	    case GIMP_INDEXEDA_IMAGE:
	      project_indexed_alpha (gimage, layer, &src2PR, &src1PR, mask);
	      break;

	    default:
	      break;
	    }
	}

      gimage->construct_flag = TRUE;  /*  something was projected  */
    }

  g_list_free (reverse_list);
}

static void
gimp_image_construct_channels (GimpImage *gimage,
			       gint       x,
			       gint       y,
			       gint       w,
			       gint       h)
{
  GimpChannel *channel;
  PixelRegion  src1PR;
  PixelRegion  src2PR;
  GList       *list;
  GList       *reverse_list = NULL;

  /*  reverse the channel list  */
  for (list = GIMP_LIST (gimage->channels)->list;
       list;
       list = g_list_next (list))
    {
      reverse_list = g_list_prepend (reverse_list, list->data);
    }

  for (list = reverse_list; list; list = g_list_next (list))
    {
      channel = (GimpChannel *) list->data;

      if (gimp_item_get_visible (GIMP_ITEM (channel)))
	{
	  /* configure the pixel regions  */
	  pixel_region_init (&src1PR,
			     gimp_image_projection (gimage),
			     x, y, w, h,
			     TRUE);
	  pixel_region_init (&src2PR,
			     gimp_drawable_data (GIMP_DRAWABLE (channel)),
			     x, y, w, h,
			     FALSE);

	  project_channel (gimage, channel, &src1PR, &src2PR);

	  gimage->construct_flag = TRUE;
	}
    }

  g_list_free (reverse_list);
}

static void
gimp_image_initialize_projection (GimpImage *gimage,
				  gint       x,
				  gint       y,
				  gint       w,
				  gint       h)
{

  GList       *list;
  gint         coverage = 0;
  PixelRegion  PR;
  guchar       clear[4] = { 0, 0, 0, 0 };

  /*  this function determines whether a visible layer
   *  provides complete coverage over the image.  If not,
   *  the projection is initialized to transparent
   */

  for (list = GIMP_LIST (gimage->layers)->list;
       list;
       list = g_list_next (list))
    {
      GimpItem *item;
      gint      off_x, off_y;

      item = (GimpItem *) list->data;

      gimp_item_offsets (item, &off_x, &off_y);

      if (gimp_item_get_visible (GIMP_ITEM (item))         &&
	  ! gimp_drawable_has_alpha (GIMP_DRAWABLE (item)) &&
	  (off_x <= x)                                     &&
	  (off_y <= y)                                     &&
	  (off_x + gimp_item_width  (item) >= x + w)       &&
	  (off_y + gimp_item_height (item) >= y + h))
	{
	  coverage = 1;
	  break;
	}
    }

  if (!coverage)
    {
      pixel_region_init (&PR, gimp_image_projection (gimage),
			 x, y, w, h, TRUE);
      color_region (&PR, clear);
    }
}

static void
gimp_image_construct (GimpImage *gimage,
		      gint       x,
		      gint       y,
		      gint       w,
		      gint       h)
{
  g_return_if_fail (GIMP_IS_IMAGE (gimage));

#if 0
  gint xoff;
  gint yoff;

  /*  set the construct flag, used to determine if anything
   *  has been written to the gimage raw image yet.
   */
  gimage->construct_flag = FALSE;

  if (gimage->layers)
    {
      gimp_item_offsets (GIMP_ITEM (gimage->layers->data), &xoff, &yoff);
    }

  if ((gimage->layers) &&                         /* There's a layer.      */
      (! g_slist_next (gimage->layers)) &&        /* It's the only layer.  */
      (gimp_drawable_has_alpha (GIMP_DRAWABLE (gimage->layers->data))) &&
                                                  /* It's !flat.           */
      (gimp_item_get_visible (GIMP_ITEM (gimage->layers->data))) &&
                                                  /* It's visible.         */
      (gimp_item_width  (GIMP_ITEM (gimage->layers->data)) ==
       gimage->width) &&
      (gimp_item_height (GIMP_ITEM (gimage->layers->data)) ==
       gimage->height) &&                         /* Covers all.           */
      (!gimp_drawable_is_indexed (GIMP_DRAWABLE (gimage->layers->data))) &&
                                                  /* Not indexed.          */
      (((GimpLayer *)(gimage->layers->data))->opacity == GIMP_OPACITY_OPAQUE)
                                                  /* Opaque                */
      )
    {
      gint xoff;
      gint yoff;

      gimp_item_offsets (GIMP_ITEM (gimage->layers->data), &xoff, &yoff);

      if ((xoff==0) && (yoff==0)) /* Starts at 0,0         */
	{
	  PixelRegion srcPR, destPR;
	  gpointer    pr;

	  g_warning("Can use cow-projection hack.  Yay!");

	  pixel_region_init (&srcPR, gimp_drawable_data
			     (GIMP_DRAWABLE (gimage->layers->data)),
			     x, y, w,h, FALSE);
	  pixel_region_init (&destPR,
			     gimp_image_projection (gimage),
			     x, y, w,h, TRUE);

	  for (pr = pixel_regions_register (2, &srcPR, &destPR);
	       pr != NULL;
	       pr = pixel_regions_process (pr))
	    {
	      tile_manager_map_over_tile (destPR.tiles,
					  destPR.curtile, srcPR.curtile);
	    }

	  gimage->construct_flag = TRUE;
	  gimp_image_construct_channels (gimage, x, y, w, h);

	  return;
	}
    }
#else
  gimage->construct_flag = FALSE;
#endif

  /*  First, determine if the projection image needs to be
   *  initialized--this is the case when there are no visible
   *  layers that cover the entire canvas--either because layers
   *  are offset or only a floating selection is visible
   */
  gimp_image_initialize_projection (gimage, x, y, w, h);

  /*  call functions which process the list of layers and
   *  the list of channels
   */
  gimp_image_construct_layers (gimage, x, y, w, h);
  gimp_image_construct_channels (gimage, x, y, w, h);
}

static void
project_intensity (GimpImage   *gimage,
		   GimpLayer   *layer,
		   PixelRegion *src,
		   PixelRegion *dest,
		   PixelRegion *mask)
{
  if (! gimage->construct_flag)
    initial_region (src, dest, mask, NULL,
                    layer->opacity * 255.999,
                    layer->mode,
                    gimage->visible,
                    INITIAL_INTENSITY);
  else
    combine_regions (dest, src, dest, mask, NULL,
                     layer->opacity * 255.999,
                     layer->mode,
                     gimage->visible,
                     COMBINE_INTEN_A_INTEN);
}

static void
project_intensity_alpha (GimpImage   *gimage,
			 GimpLayer   *layer,
			 PixelRegion *src,
			 PixelRegion *dest,
			 PixelRegion *mask)
{
  if (! gimage->construct_flag)
    initial_region (src, dest, mask, NULL,
                    layer->opacity * 255.999,
                    layer->mode,
                    gimage->visible,
                    INITIAL_INTENSITY_ALPHA);
  else
    combine_regions (dest, src, dest, mask, NULL,
                     layer->opacity * 255.999,
                     layer->mode,
                     gimage->visible,
                     COMBINE_INTEN_A_INTEN_A);
}

static void
project_indexed (GimpImage   *gimage,
		 GimpLayer   *layer,
		 PixelRegion *src,
		 PixelRegion *dest)
{
  if (! gimage->construct_flag)
    initial_region (src, dest, NULL, gimage->cmap,
                    layer->opacity * 255.999,
                    layer->mode,
                    gimage->visible,
                    INITIAL_INDEXED);
  else
    g_warning ("%s: unable to project indexed image.", G_GNUC_PRETTY_FUNCTION);
}

static void
project_indexed_alpha (GimpImage   *gimage,
		       GimpLayer   *layer,
		       PixelRegion *src,
		       PixelRegion *dest,
		       PixelRegion *mask)
{
  if (! gimage->construct_flag)
    initial_region (src, dest, mask, gimage->cmap,
                    layer->opacity * 255.999,
                    layer->mode,
                    gimage->visible,
                    INITIAL_INDEXED_ALPHA);
  else
    combine_regions (dest, src, dest, mask, gimage->cmap,
                     layer->opacity * 255.999,
                     layer->mode,
                     gimage->visible,
                     COMBINE_INTEN_A_INDEXED_A);
}

static void
project_channel (GimpImage   *gimage,
		 GimpChannel *channel,
		 PixelRegion *src,
		 PixelRegion *src2)
{
  guchar  col[3];
  guchar  opacity;
  gint    type;

  gimp_rgba_get_uchar (&channel->color,
		       &col[0], &col[1], &col[2], &opacity);

  if (! gimage->construct_flag)
    {
      type = (channel->show_masked) ?
	INITIAL_CHANNEL_MASK : INITIAL_CHANNEL_SELECTION;

      initial_region (src2, src, NULL, col,
                      opacity,
                      GIMP_NORMAL_MODE,
                      NULL,
                      type);
    }
  else
    {
      type = (channel->show_masked) ?
	COMBINE_INTEN_A_CHANNEL_MASK : COMBINE_INTEN_A_CHANNEL_SELECTION;

      combine_regions (src, src2, src, NULL, col,
                       opacity,
                       GIMP_NORMAL_MODE,
                       NULL,
                       type);
    }
}
