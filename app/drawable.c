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
#include <math.h>
#include "appenv.h"
#include "channel.h"
#include "drawable.h"
#include "errors.h"
#include "floating_sel.h"
#include "gimage.h"
#include "gimage_mask.h"
#include "gdisplay.h"
#include "layer.h"
#include "linked.h"
#include "palette.h"


/*
 *  Static variables
 */
int global_drawable_ID = 1;

/**************************/
/*  Function definitions  */

void
drawable_apply_image (drawable_id, x1, y1, x2, y2, tiles, sparse)
     int drawable_id;
     int x1, y1;
     int x2, y2;
     TileManager *tiles;
     int sparse;
{
  Channel *channel;
  Layer *layer;

  if ((channel = channel_get_ID (drawable_id)))
    channel_apply_image (channel, x1, y1, x2, y2, tiles, sparse);
  else if ((layer = layer_get_ID (drawable_id)))
    layer_apply_image (layer, x1, y1, x2, y2, tiles, sparse);
}


void
drawable_merge_shadow (drawable_id, undo)
     int drawable_id;
     int undo;
{
  GImage *gimage;
  PixelRegion shadowPR;
  int x1, y1, x2, y2;

  if (! (gimage = drawable_gimage (drawable_id)))
    return;

  /*  A useful optimization here is to limit the update to the
   *  extents of the selection mask, as it cannot extend beyond
   *  them.
   */
  drawable_mask_bounds (drawable_id, &x1, &y1, &x2, &y2);
  pixel_region_init (&shadowPR, gimage->shadow, x1, y1,
		     (x2 - x1), (y2 - y1), FALSE);
  gimage_apply_image (gimage, drawable_id, &shadowPR, undo, OPAQUE,
		      REPLACE_MODE, NULL, x1, y1);
}


void
drawable_fill (drawable_id, fill_type)
     int drawable_id;
     int fill_type;
{
  GImage *gimage;
  PixelRegion destPR;
  unsigned char c[MAX_CHANNELS];
  unsigned char i, r, g, b, a;

  if (! (gimage = drawable_gimage (drawable_id)))
    return;

  a = 255;

  /*  a gimage_fill affects the active layer  */
  switch (fill_type)
    {
    case BACKGROUND_FILL:
      palette_get_background (&r, &g, &b);
      a = 255;
      break;
    case WHITE_FILL:
      a = r = g = b = 255;
      break;
    case TRANSPARENT_FILL:
      r = g = b = a = 0;
      break;
    case NO_FILL:
      return;
      break;
    }

  switch (drawable_type (drawable_id))
    {
    case RGB_GIMAGE: case RGBA_GIMAGE:
      c[RED_PIX] = r;
      c[GREEN_PIX] = g;
      c[BLUE_PIX] = b;
      if (drawable_type (drawable_id) == RGBA_GIMAGE)
	c[ALPHA_PIX] = a;
      break;
    case GRAY_GIMAGE: case GRAYA_GIMAGE:
      c[GRAY_PIX] = r;
      if (drawable_type (drawable_id) == GRAYA_GIMAGE)
	c[ALPHA_G_PIX] = a;
      break;
    case INDEXED_GIMAGE: case INDEXEDA_GIMAGE:
      c[RED_PIX] = r;
      c[GREEN_PIX] = g;
      c[BLUE_PIX] = b;
      gimage_transform_color (gimage, drawable_id, c, &i, RGB);
      c[INDEXED_PIX] = i;
      if (drawable_type (drawable_id) == INDEXEDA_GIMAGE)
	  c[ALPHA_I_PIX] = a;
      break;
    default:
      warning ("Can't fill unknown image type.");
      break;
    }

  pixel_region_init (&destPR,
		     drawable_data (drawable_id),
		     0, 0,
		     drawable_width (drawable_id),
		     drawable_height (drawable_id),
		     TRUE);
  color_region (&destPR, c);

  /*  Update the filled area  */
  drawable_update (drawable_id, 0, 0,
		   drawable_width (drawable_id),
		   drawable_height (drawable_id));
}


void
drawable_update (drawable_id, x, y, w, h)
     int drawable_id;
     int x, y;
     int w, h;
{
  GImage *gimage;
  int offset_x, offset_y;

  if (! (gimage = drawable_gimage (drawable_id)))
    return;

  drawable_offsets (drawable_id, &offset_x, &offset_y);
  x += offset_x;
  y += offset_y;
  gdisplays_update_area (gimage->ID, x, y, w, h);

  /*  invalidate the preview  */
  drawable_invalidate_preview (drawable_id);
}


int
drawable_mask_bounds (drawable_id, x1, y1, x2, y2)
     int drawable_id;
     int *x1, *y1;
     int *x2, *y2;
{
  GImage *gimage;
  int off_x, off_y;

  if (! (gimage = drawable_gimage (drawable_id)))
    return FALSE;

  if (gimage_mask_bounds (gimage, x1, y1, x2, y2))
    {
      drawable_offsets (drawable_id, &off_x, &off_y);
      *x1 = BOUNDS (*x1 - off_x, 0, drawable_width (drawable_id));
      *y1 = BOUNDS (*y1 - off_y, 0, drawable_height (drawable_id));
      *x2 = BOUNDS (*x2 - off_x, 0, drawable_width (drawable_id));
      *y2 = BOUNDS (*y2 - off_y, 0, drawable_height (drawable_id));
      return TRUE;
    }
  else
    {
      *x2 = drawable_width (drawable_id);
      *y2 = drawable_height (drawable_id);
      return FALSE;
    }
}


void
drawable_invalidate_preview (drawable_id)
     int drawable_id;
{
  Channel *channel;
  Layer *layer;
  GImage *gimage;

  if ((channel = channel_get_ID (drawable_id)))
    channel->preview_valid = FALSE;
  else if ((layer = layer_get_ID (drawable_id)))
    {
      if (layer_is_floating_sel (layer))
	floating_sel_invalidate (layer);
      else
	layer->preview_valid = FALSE;
    }

  gimage = drawable_gimage (drawable_id);
  if (gimage)
    {
      gimage->comp_preview_valid[0] = FALSE;
      gimage->comp_preview_valid[1] = FALSE;
      gimage->comp_preview_valid[2] = FALSE;
    }
}


int
drawable_dirty (int drawable_id)
{
  Channel *channel;
  Layer *layer;

  if ((channel = channel_get_ID (drawable_id)))
    return channel->dirty = (channel->dirty < 0) ? 2 : channel->dirty + 1;
  else if ((layer = layer_get_ID (drawable_id)))
    return layer->dirty = (layer->dirty < 0) ? 2 : layer->dirty + 1;
  else
    return 0;
}


int
drawable_clean (int drawable_id)
{
  Channel *channel;
  Layer *layer;

  if ((channel = channel_get_ID (drawable_id)))
    return channel->dirty = (channel->dirty <= 0) ? 0 : channel->dirty - 1;
  else if ((layer = layer_get_ID (drawable_id)))
    return layer->dirty = (layer->dirty <= 0) ? 0 : layer->dirty - 1;
  else
    return 0;
}


GImage *
drawable_gimage (int drawable_id)
{
  Channel *channel;
  Layer *layer;

  if ((channel = channel_get_ID (drawable_id)))
    return gimage_get_ID (channel->gimage_ID);
  else if ((layer = layer_get_ID (drawable_id)))
    return gimage_get_ID (layer->gimage_ID);
  else
    return NULL;
}


int
drawable_type (int drawable_id)
{
  Channel *channel;
  Layer *layer;

  if ((channel = channel_get_ID (drawable_id)))
    return GRAY_GIMAGE;
  else if ((layer = layer_get_ID (drawable_id)))
    return layer->type;
  else
    return -1;
}


int
drawable_has_alpha (int drawable_id)
{
  Channel *channel;
  Layer *layer;

  if ((channel = channel_get_ID (drawable_id)))
    return FALSE;
  else if ((layer = layer_get_ID (drawable_id)))
    return layer_has_alpha (layer);
  else
    return FALSE;
}


int
drawable_type_with_alpha (int drawable_id)
{
  int type = drawable_type (drawable_id);
  int has_alpha = drawable_has_alpha (drawable_id);

  if (has_alpha)
    return type;
  else
    switch (type)
      {
      case RGB_GIMAGE:
	return RGBA_GIMAGE; break;
      case GRAY_GIMAGE:
	return GRAYA_GIMAGE; break;
      case INDEXED_GIMAGE:
	return INDEXEDA_GIMAGE; break;
      }
  return 0;
}


int
drawable_color (drawable_id)
     int drawable_id;
{
  if (drawable_type (drawable_id) == RGBA_GIMAGE ||
      drawable_type (drawable_id) == RGB_GIMAGE)
    return 1;
  else
    return 0;
}


int
drawable_gray (drawable_id)
     int drawable_id;
{
  if (drawable_type (drawable_id) == GRAYA_GIMAGE ||
      drawable_type (drawable_id) == GRAY_GIMAGE)
    return 1;
  else
    return 0;
}


int
drawable_indexed (drawable_id)
     int drawable_id;
{
  if (drawable_type (drawable_id) == INDEXEDA_GIMAGE ||
      drawable_type (drawable_id) == INDEXED_GIMAGE)
    return 1;
  else
    return 0;
}


TileManager *
drawable_data (int drawable_id)
{
  Channel *channel;
  Layer *layer;

  if ((channel = channel_get_ID (drawable_id)))
    return channel->tiles;
  else if ((layer = layer_get_ID (drawable_id)))
    return layer->tiles;
  else
    return NULL;
}


TileManager *
drawable_shadow (int drawable_id)
{
  GImage *gimage;
  Channel *channel;
  Layer *layer;

  if (! (gimage = drawable_gimage (drawable_id)))
    return NULL;

  if ((channel = channel_get_ID (drawable_id)))
    {
      return gimage_shadow (gimage, channel->width, channel->height, 1);
    }
  else if ((layer = layer_get_ID (drawable_id)))
    {
      return gimage_shadow (gimage, layer->width, layer->height, layer->bytes);
    }
  else
    return NULL;
}


int
drawable_bytes (int drawable_id)
{
  Channel *channel;
  Layer *layer;

  if ((channel = channel_get_ID (drawable_id)))
    return channel->bytes;
  else if ((layer = layer_get_ID (drawable_id)))
    return layer->bytes;
  else
    return -1;
}


int
drawable_width (int drawable_id)
{
  Channel *channel;
  Layer *layer;

  if ((channel = channel_get_ID (drawable_id)))
    return channel->width;
  else if ((layer = layer_get_ID (drawable_id)))
    return layer->width;
  else
    return -1;
}


int
drawable_height (int drawable_id)
{
  Channel *channel;
  Layer *layer;

  if ((channel = channel_get_ID (drawable_id)))
    return channel->height;
  else if ((layer = layer_get_ID (drawable_id)))
    return layer->height;
  else
    return -1;
}


void
drawable_offsets (drawable_id, off_x, off_y)
     int drawable_id;
     int *off_x;
     int *off_y;
{
  Channel *channel;
  Layer *layer;

  /*  return the active layer offsets  */
  if ((layer = layer_get_ID (drawable_id)))
    {
      *off_x = layer->offset_x;
      *off_y = layer->offset_y;
      return;
    }
  /*  The case of a layer mask  */
  else if ((channel = channel_get_ID (drawable_id)))
    {
      if (channel->layer_ID != -1)
	if ((layer = layer_get_ID (channel->layer_ID)))
	  {
	    *off_x = layer->offset_x;
	    *off_y = layer->offset_y;
	    return;
	  }
    }

  *off_x = 0;
  *off_y = 0;
}


unsigned char *
drawable_cmap (drawable_id)
     int drawable_id;
{
  GImage *gimage;

  if ((gimage = drawable_gimage (drawable_id)))
    return gimage->cmap;
  else
    return NULL;
}


Layer *
drawable_layer (drawable_id)
     int drawable_id;
{
  Layer *layer;

  if ((layer = layer_get_ID (drawable_id)))
    if (!layer->mask || (layer->mask && !layer->edit_mask))
      return layer;

  return NULL;
}


Channel *
drawable_layer_mask (drawable_id)
     int drawable_id;
{
  Channel *channel;

  if ((channel = channel_get_ID (drawable_id)))
    if (channel->layer_ID != -1)
      return channel;

  return NULL;
}


Channel *
drawable_channel    (drawable_id)
     int drawable_id;
{
  Channel *channel;
  if ((channel = channel_get_ID (drawable_id)))
    return channel;
  else
    return NULL;
}
