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
 * Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include "appenv.h"
#include "drawable.h"
#include "errors.h"
#include "floating_sel.h"
#include "gdisplay.h"
#include "gimage.h"
#include "gimage_mask.h"
#include "interface.h"
#include "layer.h"
#include "layers_dialog.h"
#include "linked.h"
#include "paint_funcs.h"
#include "temp_buf.h"
#include "undo.h"

/*  static functions  */

static void transform_color     (GImage *, PixelRegion *,
				 PixelRegion *, int, int);
static void layer_preview_scale (int, unsigned char *, PixelRegion *,
				 PixelRegion *, int);

/*
 *  Static variables
 */
extern int global_drawable_ID;
static link_ptr layer_list = NULL;


/********************************/
/*  Local function definitions  */

static void
transform_color (gimage, layerPR, bufPR, drawable_id, type)
     GImage * gimage;
     PixelRegion * layerPR;
     PixelRegion * bufPR;
     int type;
{
  int i, h;
  unsigned char * s, * d;
  void * pr;

  for (pr = pixel_regions_register (2, layerPR, bufPR); pr != NULL; pr = pixel_regions_process (pr))
    {
      h = layerPR->h;
      s = bufPR->data;
      d = layerPR->data;

      while (h--)
	{
	  for (i = 0; i < layerPR->w; i++)
	    {
	      gimage_transform_color (gimage, drawable_id,
				      s + (i * bufPR->bytes),
				      d + (i * layerPR->bytes), type);
	      /*  copy alpha channel  */
	      d[(i + 1) * layerPR->bytes - 1] = s[(i + 1) * bufPR->bytes - 1];
	    }

	  s += bufPR->rowstride;
	  d += layerPR->rowstride;
	}
    }
}

/**************************/
/*  Function definitions  */

void
layer_allocate (layer, width, height, bpp)
     Layer * layer;
     int width;
     int height;
     int bpp;
{
  layer->tiles = tile_manager_new (width, height, bpp);
}


void
layer_deallocate (layer)
     Layer * layer;
{
  if (layer->tiles)
    tile_manager_destroy (layer->tiles);
}


Layer *
layer_new (gimage_ID, width, height, type, name, opacity, mode)
     int gimage_ID;
     int width, height;
     int type;
     char * name;
     int opacity;
     int mode;
{
  Layer * layer;

  if (width == 0 || height == 0) {
    warning ("Zero width or height layers not allowed.");
    return NULL;
  }

  layer = (Layer *) g_malloc (sizeof (Layer));

  if (!name)
    name = "unnamed";

  layer->name = (char *) g_malloc (strlen (name) + 1);
  strcpy (layer->name, name);

  /*  set size information  */
  layer->offset_x = 0;
  layer->offset_y = 0;
  layer->width = width;
  layer->height = height;
  layer->type = type;

  switch (type)
    {
    case RGB_GIMAGE:
      layer->bytes = 3; break;
    case GRAY_GIMAGE:
      layer->bytes = 1; break;
    case RGBA_GIMAGE:
      layer->bytes = 4; break;
    case GRAYA_GIMAGE:
      layer->bytes = 2; break;
    case INDEXED_GIMAGE:
      layer->bytes = 1; break;
    case INDEXEDA_GIMAGE:
      layer->bytes = 2; break;
    default:
      warning ("Layer type not supported.\n");
      break;
    }

  /*  allocate the memory for this layer  */
  layer_allocate (layer, width, height, layer->bytes);
  layer->visible = 1;
  layer->linked = 0;
  layer->preserve_trans = 0;

  /*  no layer mask is present at start  */
  layer->mask = NULL;
  layer->apply_mask = 0;
  layer->edit_mask = 0;
  layer->show_mask = 0;

  /*  mode and opacity  */
  layer->mode = mode;
  layer->opacity = opacity;
  layer->dirty = 0;

  /*  give this layer an ID  */
  layer->ID = global_drawable_ID++;
  layer->gimage_ID = gimage_ID;

  /*  preview variables  */
  layer->preview = NULL;
  layer->preview_valid = FALSE;

  /*  floating selection variables  */
  layer->fs.backing_store = NULL;
  layer->fs.drawable = -1;
  layer->fs.initial = TRUE;
  layer->fs.boundary_known = FALSE;
  layer->fs.segs = NULL;
  layer->fs.num_segs = 0;

  /*  add the new layer to the global list  */
  layer_list = append_to_list (layer_list, (void *) layer);

  return layer;
}


Layer *
layer_copy (layer, add_alpha)
     Layer * layer;
     int add_alpha;
{
  char * layer_name;
  Layer * new_layer;
  int new_type;
  PixelRegion srcPR, destPR;

  /*  formulate the new layer name  */
  layer_name = (char *) g_malloc (strlen (layer->name) + 6);
  sprintf (layer_name, "%s copy", layer->name);

  /*  when copying a layer, the copy ALWAYS has an alpha channel  */
  if (add_alpha)
    {
      switch (layer->type)
	{
	case RGB_GIMAGE:
	  new_type = RGBA_GIMAGE;
	  break;
	case GRAY_GIMAGE:
	  new_type = GRAYA_GIMAGE;
	  break;
	case INDEXED_GIMAGE:
	  new_type = INDEXEDA_GIMAGE;
	  break;
	default:
	  new_type = layer->type;
	  break;
	}
    }
  else
    new_type = layer->type;

  /*  allocate a new layer object  */
  new_layer = layer_new (layer->gimage_ID, layer->width, layer->height,
			 new_type, layer_name, layer->opacity, layer->mode);
  if (!new_layer) {
    warning("layer_copy: could not allocate new layer");
    goto cleanup;
  }

  new_layer->offset_x = layer->offset_x;
  new_layer->offset_y = layer->offset_y;
  new_layer->visible = layer->visible;
  new_layer->linked = layer->linked;
  new_layer->preserve_trans = layer->preserve_trans;

  /*  copy the contents across layers  */
  if (new_type == layer->type)
    {
      pixel_region_init (&srcPR, layer->tiles, 0, 0, layer->width, layer->height, FALSE);
      pixel_region_init (&destPR, new_layer->tiles, 0, 0, layer->width, layer->height, TRUE);
      copy_region (&srcPR, &destPR);
    }
  else
    {
      pixel_region_init (&srcPR, layer->tiles, 0, 0, layer->width, layer->height, FALSE);
      pixel_region_init (&destPR, new_layer->tiles, 0, 0, layer->width, layer->height, TRUE);
      add_alpha_region (&srcPR, &destPR);
    }

  /*  duplicate the layer mask if necessary  */
  if (layer->mask)
    {
      new_layer->mask = channel_copy (layer->mask);
      new_layer->apply_mask = layer->apply_mask;
      new_layer->edit_mask = layer->edit_mask;
      new_layer->show_mask = layer->show_mask;
    }

 cleanup:
  /*  free up the layer_name memory  */
  g_free (layer_name);

  return new_layer;
}


Layer *
layer_from_tiles (gimage_ptr, drawable_id, tiles, name, opacity, mode)
     void *gimage_ptr;
     int drawable_id;
     TileManager *tiles;
     char *name;
     int opacity;
     int mode;
{
  GImage * gimage;
  Layer * new_layer;
  int layer_type;
  PixelRegion layerPR, bufPR;

  /*  Function copies buffer to a layer
   *  taking into consideration the possibility of transforming
   *  the contents to meet the requirements of the target image type
   */

  /*  If no tile manager, return NULL  */
  if (!tiles)
    return NULL;

  gimage = (GImage *) gimage_ptr;

  layer_type = drawable_type_with_alpha (drawable_id);

  /*  Create the new layer  */
  new_layer = layer_new (0, tiles->levels[0].width, tiles->levels[0].height,
			 layer_type, name, opacity, mode);

  if (!new_layer) {
    warning("layer_from_tiles: could not allocate new layer");
    return NULL;
  }

  /*  Configure the pixel regions  */
  pixel_region_init (&layerPR, new_layer->tiles, 0, 0, new_layer->width, new_layer->height, TRUE);
  pixel_region_init (&bufPR, tiles, 0, 0, new_layer->width, new_layer->height, FALSE);

  if ((tiles->levels[0].bpp == 4 && new_layer->type == RGBA_GIMAGE) ||
      (tiles->levels[0].bpp == 2 && new_layer->type == GRAYA_GIMAGE))
    /*  If we want a layer the same type as the buffer  */
    copy_region (&bufPR, &layerPR);
  else
    /*  Transform the contents of the buf to the new_layer  */
    transform_color (gimage, &layerPR, &bufPR, new_layer->ID,
		     (tiles->levels[0].bpp == 4) ? RGB : GRAY);

  return new_layer;
}


Channel *
layer_add_mask (layer, mask_id)
     Layer * layer;
     int mask_id;
{
  Channel *mask;

  if (layer->mask)
    return NULL;
  if ((mask = channel_get_ID (mask_id)) == NULL)
    return NULL;

  layer->mask = mask;
  mask->layer_ID = layer->ID;

  /*  Set the application mode in the layer to "apply"  */
  layer->apply_mask = 1;
  layer->edit_mask = 1;
  layer->show_mask = 0;

  drawable_update (layer->ID, 0, 0, layer->width, layer->height);

  return layer->mask;
}


Channel *
layer_create_mask (layer, add_mask_type)
     Layer * layer;
     AddMaskType add_mask_type;
{
  PixelRegion maskPR, layerPR;
  Channel *mask;
  char * mask_name;
  unsigned char black[3] = {0, 0, 0};
  unsigned char white_mask = OPAQUE;
  unsigned char black_mask = TRANSPARENT;

  mask_name = (char *) g_malloc (strlen (layer->name) + strlen ("mask") + 2);
  sprintf (mask_name, "%s mask", layer->name);

  /*  Create the layer mask  */
  mask = channel_new (layer->gimage_ID, layer->width, layer->height,
		      mask_name, OPAQUE, black);

  pixel_region_init (&maskPR, mask->tiles, 0, 0, mask->width, mask->height, TRUE);

  switch (add_mask_type)
    {
    case WhiteMask:
      color_region (&maskPR, &white_mask);
      break;
    case BlackMask:
      color_region (&maskPR, &black_mask);
      break;
    case AlphaMask:
      /*  Extract the layer's alpha channel  */
      if (layer_has_alpha (layer))
	{
	  pixel_region_init (&layerPR, layer->tiles, 0, 0, layer->width, layer->height, FALSE);
	  extract_alpha_region (&layerPR, NULL, &maskPR);
	}
      break;
    }

  g_free (mask_name);
  return mask;
}


Layer *
layer_get_ID (ID)
     int ID;
{
  link_ptr tmp = layer_list;
  Layer * layer;

  while (tmp)
    {
      layer = (Layer *) tmp->data;
      if (layer->ID == ID)
	return layer;

      tmp = next_item (tmp);
    }

  return NULL;
}


void
layer_delete (layer)
     Layer * layer;
{
  /*  remove this image from the global list  */
  layer_list = remove_from_list (layer_list, (void *) layer);

  /*  free the shared memory  */
  layer_deallocate (layer);

  /*  if a layer mask exists, free it  */
  if (layer->mask)
    channel_delete (layer->mask);

  /*  free the layer name buffer  */
  g_free (layer->name);

  /*  free the layer preview if it exists  */
  if (layer->preview)
    temp_buf_free (layer->preview);

  /*  free the layer boundary if it exists  */
  if (layer->fs.segs)
    g_free (layer->fs.segs);

  /*  free the floating selection if it exists  */
  if (layer_is_floating_sel (layer))
    {
      tile_manager_destroy (layer->fs.backing_store);
    }

  g_free (layer);
}


void
layer_apply_mask (layer, mode)
     Layer * layer;
     int mode;
{
  PixelRegion srcPR, maskPR;

  if (!layer->mask)
    return;

  /*  this operation can only be done to layers with an alpha channel  */
  if (! layer_has_alpha (layer))
    return;

  /*  Need to save the mask here for undo  */

  if (mode == APPLY)
    {
      /*  Put this apply mask operation on the undo stack
       */
      layer_apply_image (layer, 0, 0, layer->width, layer->height, NULL, FALSE);

      /*  Combine the current layer's alpha channel and the mask  */
      pixel_region_init (&srcPR, layer->tiles, 0, 0, layer->width, layer->height, TRUE);
      pixel_region_init (&maskPR, layer->mask->tiles, 0, 0, layer->width, layer->height, FALSE);

      apply_mask_to_region (&srcPR, &maskPR, OPAQUE);
      layer->preview_valid = FALSE;

      layer->mask = NULL;
      layer->apply_mask = 0;
      layer->edit_mask = 0;
      layer->show_mask = 0;
    }
  else if (mode == DISCARD)
    {
      layer->mask = NULL;
      layer->apply_mask = 0;
      layer->edit_mask = 0;
      layer->show_mask = 0;
    }
}


void
layer_translate (layer, off_x, off_y)
     Layer * layer;
     int off_x, off_y;
{
  /*  the undo call goes here  */
  undo_push_layer_displace (gimage_get_ID (layer->gimage_ID), layer->ID);

  /*  update the affected region  */
  drawable_update (layer->ID, 0, 0, layer->width, layer->height);

  /*  invalidate the selection boundary because of a layer modification  */
  layer_invalidate_boundary (layer);

  /*  update the layer offsets  */
  layer->offset_x += off_x;
  layer->offset_y += off_y;

  /*  update the affected region  */
  drawable_update (layer->ID, 0, 0, layer->width, layer->height);

  /*  invalidate the mask preview  */
  if (layer->mask)
    drawable_invalidate_preview (layer->mask->ID);
}


void
layer_apply_image (layer, x1, y1, x2, y2, tiles, sparse)
     Layer * layer;
     int x1, y1, x2, y2;
     TileManager * tiles;
     int sparse;
{
  if (! tiles)
    /*  Need to push an undo operation  */
    undo_push_image (gimage_get_ID (layer->gimage_ID), layer->ID, x1, y1, x2, y2);
  else
    undo_push_image_mod (gimage_get_ID (layer->gimage_ID), layer->ID, x1, y1, x2, y2, tiles, sparse);
}


void
layer_add_alpha (layer)
     Layer *layer;
{
  PixelRegion srcPR, destPR;
  TileManager *new_tiles;
  int type;

  /*  Don't bother if the layer already has alpha  */
  switch (layer->type)
    {
    case RGB_GIMAGE:
      type = RGBA_GIMAGE;
      break;
    case GRAY_GIMAGE:
      type = GRAYA_GIMAGE;
      break;
    case INDEXED_GIMAGE:
      type = INDEXEDA_GIMAGE;
      break;
    case RGBA_GIMAGE:
    case GRAYA_GIMAGE:
    case INDEXEDA_GIMAGE:
    default:
      return;
      break;
    }

  /*  Configure the pixel regions  */
  pixel_region_init (&srcPR, layer->tiles, 0, 0, layer->width, layer->height, FALSE);

  /*  Allocate the new layer, configure dest region  */
  new_tiles = tile_manager_new (layer->width, layer->height, (layer->bytes + 1));
  pixel_region_init (&destPR, new_tiles, 0, 0, layer->width, layer->height, TRUE);

  /*  Add an alpha channel  */
  add_alpha_region (&srcPR, &destPR);

  /*  Push the layer on the undo stack  */
  undo_push_layer_mod (gimage_get_ID (layer->gimage_ID), layer);

  /*  Configure the new layer  */
  layer->tiles = new_tiles;
  layer->type = type;
  layer->bytes = layer->bytes + 1;

  /*  update gdisplay titles to reflect the possibility of
   *  this layer being the only layer in the gimage
   */
  gdisplays_update_title (layer->gimage_ID);
}


void
layer_scale (layer, new_width, new_height, local_origin)
     Layer *layer;
     int new_width, new_height;
     int local_origin;
{
  PixelRegion srcPR, destPR;
  TileManager *new_tiles;

  if (new_width == 0 || new_height == 0)
    return;

  /*  If there is a layer mask, make sure it gets scaled also  */
  if (layer->mask)
    channel_scale (layer->mask, new_width, new_height);

  /*  Update the old layer position  */
  drawable_update (layer->ID, 0, 0, layer->width, layer->height);

  /*  Configure the pixel regions  */
  pixel_region_init (&srcPR, layer->tiles, 0, 0, layer->width, layer->height, FALSE);

  /*  Allocate the new layer, configure dest region  */
  new_tiles = tile_manager_new (new_width, new_height, layer->bytes);
  pixel_region_init (&destPR, new_tiles, 0, 0, new_width, new_height, TRUE);

  /*  Scale the layer -
   *   If the layer is of type INDEXED, then we don't use pixel-value
   *   resampling because that doesn't necessarily make sense for INDEXED
   *   images.
   */
  if ((layer->type == INDEXED_GIMAGE) || (layer->type == INDEXEDA_GIMAGE))
    scale_region_no_resample (&srcPR, &destPR);
  else
    scale_region (&srcPR, &destPR);

  /*  Push the layer on the undo stack  */
  undo_push_layer_mod (gimage_get_ID (layer->gimage_ID), layer);

  /*  Configure the new layer  */
  if (local_origin)
    {
      int cx, cy;

      cx = layer->offset_x + layer->width / 2;
      cy = layer->offset_y + layer->height / 2;

      layer->offset_x = cx - (new_width / 2);
      layer->offset_y = cy - (new_height / 2);
    }
  else
    {
      double xrat, yrat;

      xrat = (double) new_width / (double) layer->width;
      yrat = (double) new_height / (double) layer->height;

      layer->offset_x = (int) (xrat * layer->offset_x);
      layer->offset_y = (int) (yrat * layer->offset_y);
    }
  layer->tiles = new_tiles;
  layer->width = new_width;
  layer->height = new_height;

  /*  Update the new layer position  */
  drawable_update (layer->ID, 0, 0, layer->width, layer->height);
}


void
layer_resize (layer, new_width, new_height, offx, offy)
     Layer *layer;
     int new_width, new_height;
     int offx, offy;
{
  PixelRegion srcPR, destPR;
  TileManager *new_tiles;
  int w, h;
  int x1, y1, x2, y2;

  if (!new_width || !new_height)
    return;

  /*  If there is a layer mask, make sure it gets resized also  */
  if (layer->mask)
    channel_resize (layer->mask, new_width, new_height, offx, offy);

  x1 = BOUNDS (offx, 0, new_width);
  y1 = BOUNDS (offy, 0, new_height);
  x2 = BOUNDS ((offx + layer->width), 0, new_width);
  y2 = BOUNDS ((offy + layer->height), 0, new_height);
  w = x2 - x1;
  h = y2 - y1;

  if (offx > 0)
    {
      x1 = 0;
      x2 = offx;
    }
  else
    {
      x1 = -offx;
      x2 = 0;
    }

  if (offy > 0)
    {
      y1 = 0;
      y2 = offy;
    }
  else
    {
      y1 = -offy;
      y2 = 0;
    }

  /*  Update the old layer position  */
  drawable_update (layer->ID, 0, 0, layer->width, layer->height);

  /*  Configure the pixel regions  */
  pixel_region_init (&srcPR, layer->tiles, x1, y1, w, h, FALSE);

  /*  Allocate the new layer, configure dest region  */
  new_tiles = tile_manager_new (new_width, new_height, layer->bytes);
  pixel_region_init (&destPR, new_tiles, 0, 0, new_width, new_height, TRUE);

  /*  fill with the fill color  */
  if (layer_has_alpha (layer))
    {
      /*  Set to transparent and black  */
      unsigned char bg[4] = {0, 0, 0, 0};
      color_region (&destPR, bg);
    }
  else
    {
      unsigned char bg[3];
      gimage_get_background (gimage_get_ID (layer->gimage_ID), layer->ID, bg);
      color_region (&destPR, bg);
    }
  pixel_region_init (&destPR, new_tiles, x2, y2, w, h, TRUE);

  /*  copy from the old to the new  */
  if (w && h)
    copy_region (&srcPR, &destPR);

  /*  Push the layer on the undo stack  */
  undo_push_layer_mod (gimage_get_ID (layer->gimage_ID), layer);

  /*  Configure the new layer  */
  layer->tiles = new_tiles;
  layer->offset_x = x1 + layer->offset_x - x2;
  layer->offset_y = y1 + layer->offset_y - y2;
  layer->width = new_width;
  layer->height = new_height;

  /*  update the new layer area  */
  drawable_update (layer->ID, 0, 0, layer->width, layer->height);
}


BoundSeg *
layer_boundary (layer, num_segs)
     Layer *layer;
     int *num_segs;
{
  BoundSeg *new_segs;

  /*  Create the four boundary segments that encompass this
   *  layer's boundary.
   */
  new_segs = (BoundSeg *) g_malloc (sizeof (BoundSeg) * 4);
  *num_segs = 4;

  /*  if the layer is a floating selection  */
  if (layer_is_floating_sel (layer))
    {
      /*  if the owner drawable is a channel, just return nothing  */
      if (drawable_channel (layer->fs.drawable))
	{
	  *num_segs = 0;
	  return NULL;
	}
      /*  otherwise, set the layer to the owner drawable  */
      else
	layer = layer_get_ID (layer->fs.drawable);
    }

  new_segs[0].x1 = layer->offset_x;
  new_segs[0].y1 = layer->offset_y;
  new_segs[0].x2 = layer->offset_x;
  new_segs[0].y2 = layer->offset_y + layer->height;
  new_segs[0].open = 1;

  new_segs[1].x1 = layer->offset_x;
  new_segs[1].y1 = layer->offset_y;
  new_segs[1].x2 = layer->offset_x + layer->width;
  new_segs[1].y2 = layer->offset_y;
  new_segs[1].open = 1;

  new_segs[2].x1 = layer->offset_x + layer->width;
  new_segs[2].y1 = layer->offset_y;
  new_segs[2].x2 = layer->offset_x + layer->width;
  new_segs[2].y2 = layer->offset_y + layer->height;
  new_segs[2].open = 0;

  new_segs[3].x1 = layer->offset_x;
  new_segs[3].y1 = layer->offset_y + layer->height;
  new_segs[3].x2 = layer->offset_x + layer->width;
  new_segs[3].y2 = layer->offset_y + layer->height;
  new_segs[3].open = 0;

  return new_segs;
}


void
layer_invalidate_boundary (layer)
     Layer *layer;
{
  GImage *gimage;
  Channel *mask;

  /*  first get the selection mask channel  */
  if (! (gimage = gimage_get_ID (layer->gimage_ID)))
    return;

  /*  Turn the current selection off  */
  gdisplays_selection_visibility (gimage->ID, SelectionOff);

  mask = gimage_get_mask (gimage);

  /*  Only bother with the bounds if there is a selection  */
  if (! channel_is_empty (mask))
    {
      mask->bounds_known = FALSE;
      mask->boundary_known = FALSE;
    }

  /*  clear the affected region surrounding the layer  */
  gdisplays_selection_visibility (layer->gimage_ID, SelectionLayerOff);
}


int
layer_pick_correlate (layer, x, y)
     Layer *layer;
     int x, y;
{
  Tile *tile;
  Tile *mask_tile;
  int val;

  /*  Is the point inside the layer?
   *  First transform the point to layer coordinates...
   */
  x -= layer->offset_x;
  y -= layer->offset_y;
  if (x >= 0 && x < layer->width &&
      y >= 0 && y < layer->height &&
      layer->visible)
    {
      /*  If the point is inside, and the layer has no
       *  alpha channel, success!
       */
      if (! layer_has_alpha (layer))
	return TRUE;

      /*  Otherwise, determine if the alpha value at
       *  the given point is non-zero
       */
      tile = tile_manager_get_tile (layer->tiles, x, y, 0);
      tile_ref (tile);

      val = tile->data[tile->bpp * (tile->ewidth * (y % TILE_HEIGHT) + (x % TILE_WIDTH) + 1) - 1];
      if (layer->mask)
	{
	  mask_tile = tile_manager_get_tile (layer->mask->tiles, x, y, 0);
	  tile_ref (mask_tile);
	  val = (val * mask_tile->data[mask_tile->ewidth * (y % TILE_HEIGHT) + (x % TILE_WIDTH)]) / 255;
	}

      tile_unref (tile, FALSE);

      if (val > 63)
	return TRUE;
    }

  return FALSE;
}


/********************/
/* access functions */

unsigned char *
layer_data (layer)
     Layer * layer;
{
  return NULL;
}


Channel *
layer_mask (layer)
     Layer * layer;
{
  return layer->mask;
}


int
layer_has_alpha (layer)
     Layer * layer;
{
  if (layer->type == RGBA_GIMAGE ||
      layer->type == GRAYA_GIMAGE ||
      layer->type == INDEXEDA_GIMAGE)
    return 1;
  else
    return 0;
}


int
layer_is_floating_sel (layer)
     Layer *layer;
{
  if (layer->fs.drawable != -1)
    return 1;
  else
    return 0;
}

TempBuf *
layer_preview (layer, w, h)
     Layer *layer;
     int w, h;
{
  GImage *gimage;
  TempBuf *preview_buf;
  PixelRegion srcPR, destPR;
  int type;
  int bytes;
  int subsample;

  type  = 0;
  bytes = 0;

  /*  The easy way  */
  if (layer->preview_valid &&
      layer->preview->width == w &&
      layer->preview->height == h)
    return layer->preview;
  /*  The hard way  */
  else
    {
      gimage = gimage_get_ID (layer->gimage_ID);
      switch (layer->type)
	{
	case RGB_GIMAGE: case RGBA_GIMAGE:
	  type = 0;
	  bytes = layer->bytes;
	  break;
	case GRAY_GIMAGE: case GRAYA_GIMAGE:
	  type = 1;
	  bytes = layer->bytes;
	  break;
	case INDEXED_GIMAGE: case INDEXEDA_GIMAGE:
	  type = 2;
	  bytes = (layer->type == INDEXED_GIMAGE) ? 3 : 4;
	  break;
	}

      /*  calculate 'acceptable' subsample  */
      subsample = 1;
      while ((w * (subsample + 1) * 2 < layer->width) &&
	     (h * (subsample + 1) * 2 < layer->height))
	subsample = subsample + 1;

      pixel_region_init (&srcPR, layer->tiles, 0, 0, layer->width, layer->height, FALSE);

      preview_buf = temp_buf_new (w, h, bytes, 0, 0, NULL);
      destPR.bytes = preview_buf->bytes;
      destPR.w = w;
      destPR.h = h;
      destPR.rowstride = w * destPR.bytes;
      destPR.data = temp_buf_data (preview_buf);

      layer_preview_scale (type, gimage->cmap, &srcPR, &destPR, subsample);

      if (layer->preview)
	temp_buf_free (layer->preview);

      layer->preview = preview_buf;
      layer->preview_valid = TRUE;

      return layer->preview;
    }
}


TempBuf *
layer_mask_preview (layer, w, h)
     Layer *layer;
     int w, h;
{
  TempBuf *preview_buf;
  Channel *mask;
  PixelRegion srcPR, destPR;
  int subsample;

  mask = layer->mask;
  if (!mask)
    return NULL;

  /*  The easy way  */
  if (mask->preview_valid &&
      mask->preview->width == w &&
      mask->preview->height == h)
    return mask->preview;
  /*  The hard way  */
  else
    {
      /*  calculate 'acceptable' subsample  */
      subsample = 1;
      while ((w * (subsample + 1) * 2 < layer->width) &&
	     (h * (subsample + 1) * 2 < layer->height))
	subsample = subsample + 1;

      pixel_region_init (&srcPR, mask->tiles, 0, 0, mask->width, mask->height, FALSE);

      preview_buf = temp_buf_new (w, h, 1, 0, 0, NULL);
      destPR.bytes = preview_buf->bytes;
      destPR.w = w;
      destPR.h = h;
      destPR.rowstride = w * destPR.bytes;
      destPR.data = temp_buf_data (preview_buf);

      layer_preview_scale (1 /* GRAY */, NULL, &srcPR, &destPR, subsample);

      if (mask->preview)
	temp_buf_free (mask->preview);

      mask->preview = preview_buf;
      mask->preview_valid = TRUE;

      return mask->preview;
    }
}


void
layer_invalidate_previews (gimage_id)
     int gimage_id;
{
  link_ptr tmp = layer_list;
  Layer * layer;

  while (tmp)
    {
      layer = (Layer *) tmp->data;
      if (gimage_id == -1 || (layer->gimage_ID == gimage_id))
	drawable_invalidate_preview (layer->ID);
      tmp = next_item (tmp);
    }
}


static void
layer_preview_scale (type, cmap, srcPR, destPR, subsample)
     int type;
     unsigned char *cmap;
     PixelRegion *srcPR;
     PixelRegion *destPR;
     int subsample;
{
#define EPSILON 0.000001
  unsigned char * src, * s;
  unsigned char * dest, * d;
  double * row, * r;
  int destwidth;
  int src_row, src_col;
  int bytes, b;
  int width, height;
  int orig_width, orig_height;
  double x_rat, y_rat;
  double x_cum, y_cum;
  double x_last, y_last;
  double * x_frac, y_frac, tot_frac;
  int i, j;
  int frac;
  int advance_dest;
  unsigned char rgb[MAX_CHANNELS];

  orig_width = srcPR->w / subsample;
  orig_height = srcPR->h / subsample;
  width = destPR->w;
  height = destPR->h;

  /*  Some calculations...  */
  bytes = destPR->bytes;
  destwidth = destPR->rowstride;

  /*  the data pointers...  */
  src = (unsigned char *) g_malloc (orig_width * bytes);
  dest = destPR->data;

  /*  find the ratios of old x to new x and old y to new y  */
  x_rat = (double) orig_width / (double) width;
  y_rat = (double) orig_height / (double) height;

  /*  allocate an array to help with the calculations  */
  row    = (double *) g_malloc (sizeof (double) * width * bytes);
  x_frac = (double *) g_malloc (sizeof (double) * (width + orig_width));

  /*  initialize the pre-calculated pixel fraction array  */
  src_col = 0;
  x_cum = (double) src_col;
  x_last = x_cum;

  for (i = 0; i < width + orig_width; i++)
    {
      if (x_cum + x_rat <= (src_col + 1 + EPSILON))
	{
	  x_cum += x_rat;
	  x_frac[i] = x_cum - x_last;
	}
      else
	{
	  src_col ++;
	  x_frac[i] = src_col - x_last;
	}
      x_last += x_frac[i];
    }

  /*  clear the "row" array  */
  memset (row, 0, sizeof (double) * width * bytes);

  /*  counters...  */
  src_row = 0;
  y_cum = (double) src_row;
  y_last = y_cum;

  pixel_region_get_row (srcPR, 0, src_row * subsample, orig_width * subsample, src, subsample);

  /*  Scale the selected region  */
  for (i = 0; i < height; )
    {
      src_col = 0;
      x_cum = (double) src_col;

      /* determine the fraction of the src pixel we are using for y */
      if (y_cum + y_rat <= (src_row + 1 + EPSILON))
	{
	  y_cum += y_rat;
	  y_frac = y_cum - y_last;
	  advance_dest = TRUE;
	}
      else
	{
	  src_row ++;
	  y_frac = src_row - y_last;
	  advance_dest = FALSE;
	}

      y_last += y_frac;

      s = src;
      r = row;

      frac = 0;
      j = width;

      while (j)
	{
	  tot_frac = x_frac[frac++] * y_frac;

	  /*  If indexed, transform the color to RGB  */
	  if (type == 2)
	    {
	      map_to_color (2, cmap, s, rgb);

	      r[RED_PIX] += rgb[RED_PIX] * tot_frac;
	      r[GREEN_PIX] += rgb[GREEN_PIX] * tot_frac;
	      r[BLUE_PIX] += rgb[BLUE_PIX] * tot_frac;
	      if (bytes == 4)
		r[ALPHA_PIX] += s[ALPHA_I_PIX] * tot_frac;
	    }
	  else
	    for (b = 0; b < bytes; b++)
	      r[b] += s[b] * tot_frac;

	  /*  increment the destination  */
	  if (x_cum + x_rat <= (src_col + 1 + EPSILON))
	    {
	      r += bytes;
	      x_cum += x_rat;
	      j--;
	    }

	  /* increment the source */
	  else
	    {
	      s += srcPR->bytes;
	      src_col++;
	    }
	}

      if (advance_dest)
	{
	  tot_frac = 1.0 / (x_rat * y_rat);

	  /*  copy "row" to "dest"  */
	  d = dest;
	  r = row;

	  j = width;
	  while (j--)
	    {
	      b = bytes;
	      while (b--)
		*d++ = (unsigned char) (*r++ * tot_frac);
	    }

	  dest += destwidth;

	  /*  clear the "row" array  */
	  memset (row, 0, sizeof (double) * destwidth);

	  i++;
	}
      else
	pixel_region_get_row (srcPR, 0, src_row * subsample, orig_width * subsample, src, subsample);
    }

  /*  free up temporary arrays  */
  g_free (row);
  g_free (x_frac);
  g_free (src);
}
