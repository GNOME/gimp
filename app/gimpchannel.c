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
#include <math.h>
#include "appenv.h"
#include "channel.h"
#include "drawable.h"
#include "errors.h"
#include "gimage_mask.h"
#include "layer.h"
#include "linked.h"
#include "paint_funcs.h"
#include "temp_buf.h"
#include "undo.h"


#define ROUND(x) ((int) (x + 0.5))

/*
 *  Static variables
 */
extern int global_drawable_ID;
static link_ptr channel_list = NULL;



/**************************/
/*  Function definitions  */

static void
channel_validate (TileManager *tm, Tile *tile, int level)
{
  /*  Set the contents of the tile to empty  */
  memset (tile->data, TRANSPARENT, tile->ewidth * tile->eheight);
}


void
channel_allocate (Channel *channel, int width, int height)
{
  channel->tiles = tile_manager_new (width, height, 1);
}


void
channel_deallocate (Channel *channel)
{
  if (channel->tiles)
    tile_manager_destroy (channel->tiles);
}


Channel *
channel_new (int gimage_ID, int width, int height, char *name, int opacity,
	     unsigned char *col)
{
  Channel * channel;
  int i;

  channel = (Channel *) g_malloc (sizeof (Channel));

  if (!name)
    name = "unnamed";

  channel->name = (char *) g_malloc (strlen (name) + 1);
  strcpy (channel->name, name);

  /*  set size information  */
  channel->width = width;
  channel->height = height;
  channel->bytes = 1;

  /*  allocate the memory for this channel  */
  channel_allocate (channel, width, height);
  channel->visible = 1;

  /*  set the channel color and opacity  */
  for (i = 0; i < 3; i++)
    channel->col[i] = col[i];
  channel->opacity = opacity;
  channel->show_masked = 1;
  channel->dirty = 0;

  /*  give this channel an ID  */
  channel->ID = global_drawable_ID++;
  channel->layer_ID = -1;
  channel->gimage_ID = gimage_ID;

  /*  add the new channel to the global list  */
  channel_list = append_to_list (channel_list, (void *) channel);

  /*  selection mask variables  */
  channel->empty = TRUE;
  channel->segs_in = NULL;
  channel->segs_out = NULL;
  channel->num_segs_in = 0;
  channel->num_segs_out = 0;
  channel->bounds_known = TRUE;
  channel->boundary_known = TRUE;
  channel->x1 = channel->y1 = 0;
  channel->x2 = width;
  channel->y2 = height;

  /*  preview variables  */
  channel->preview = NULL;
  channel->preview_valid = FALSE;

  return channel;
}


Channel *
channel_copy (Channel *channel)
{
  char * channel_name;
  Channel * new_channel;
  PixelRegion srcPR, destPR;

  /*  formulate the new channel name  */
  channel_name = (char *) g_malloc (strlen (channel->name) + 6);
  sprintf (channel_name, "%s copy", channel->name);

  /*  allocate a new channel object  */
  new_channel = channel_new (channel->gimage_ID, channel->width, channel->height, channel_name,
			     channel->opacity, channel->col);
  new_channel->visible = channel->visible;
  new_channel->show_masked = channel->show_masked;

  /*  copy the contents across channels  */
  pixel_region_init (&srcPR, channel->tiles, 0, 0, channel->width, channel->height, FALSE);
  pixel_region_init (&destPR, new_channel->tiles, 0, 0, channel->width, channel->height, TRUE);
  copy_region (&srcPR, &destPR);

  /*  free up the channel_name memory  */
  g_free (channel_name);

  return new_channel;
}


Channel *
channel_get_ID (int ID)
{
  link_ptr tmp = channel_list;
  Channel * channel;

  while (tmp)
    {
      channel = (Channel *) tmp->data;
      if (channel->ID == ID)
	return channel;

      tmp = next_item (tmp);
    }

  return NULL;
}


void
channel_delete (Channel *channel)
{
  /* free the segments?  */
  if (channel->segs_in)
    g_free (channel->segs_in);
  if (channel->segs_out)
    g_free (channel->segs_out);

  /*  remove this image from the global list  */
  channel_list = remove_from_list (channel_list, (void *) channel);

  /*  deallocate the channel mem  */
  channel_deallocate (channel);

  /*  free the channel name buffer  */
  g_free (channel->name);

  g_free (channel);
}


void
channel_apply_image (Channel *channel, int x1, int y1, int x2, int y2,
		     TileManager *tiles, int sparse)
{
  /*  Need to push an undo operation  */
  if (! tiles)
    undo_push_image (gimage_get_ID (channel->gimage_ID), channel->ID, x1, y1, x2, y2);
  else
    undo_push_image_mod (gimage_get_ID (channel->gimage_ID), channel->ID, x1, y1, x2, y2, tiles, sparse);
}


void
channel_scale (Channel *channel, int new_width, int new_height)
{
  PixelRegion srcPR, destPR;
  TileManager *new_tiles;

  if (new_width == 0 || new_height == 0)
    return;

  /*  Update the old channel position  */
  drawable_update (channel->ID, 0, 0, channel->width, channel->height);

  /*  Configure the pixel regions  */
  pixel_region_init (&srcPR, channel->tiles, 0, 0, channel->width, channel->height, FALSE);

  /*  Allocate the new channel, configure dest region  */
  new_tiles = tile_manager_new (new_width, new_height, 1);
  pixel_region_init (&destPR, new_tiles, 0, 0, new_width, new_height, TRUE);

  /*  Add an alpha channel  */
  scale_region (&srcPR, &destPR);

  /*  Push the channel on the undo stack  */
  undo_push_channel_mod (gimage_get_ID (channel->gimage_ID), channel);

  /*  Configure the new channel  */
  channel->tiles = new_tiles;
  channel->width = new_width;
  channel->height = new_height;

  /*  bounds are now unknown  */
  channel->bounds_known = FALSE;

  /*  Update the new channel position  */
  drawable_update (channel->ID, 0, 0, channel->width, channel->height);
}


void
channel_resize (Channel *channel, int new_width, int new_height,
		int offx, int offy)
{
  PixelRegion srcPR, destPR;
  TileManager *new_tiles;
  unsigned char bg = 0;
  int clear;
  int w, h;
  int x1, y1, x2, y2;

  if (!new_width || !new_height)
    return;

  x1 = BOUNDS (offx, 0, new_width);
  y1 = BOUNDS (offy, 0, new_height);
  x2 = BOUNDS ((offx + channel->width), 0, new_width);
  y2 = BOUNDS ((offy + channel->height), 0, new_height);
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

  /*  Update the old channel position  */
  drawable_update (channel->ID, 0, 0, channel->width, channel->height);

  /*  Configure the pixel regions  */
  pixel_region_init (&srcPR, channel->tiles, x1, y1, w, h, FALSE);

  /*  Determine whether the new channel needs to be initially cleared  */
  if ((new_width > channel->width) ||
      (new_height > channel->height) ||
      (x2 || y2))
    clear = TRUE;
  else
    clear = FALSE;

  /*  Allocate the new channel, configure dest region  */
  new_tiles = tile_manager_new (new_width, new_height, 1);

  /*  Set to black (empty--for selections)  */
  if (clear)
    {
      pixel_region_init (&destPR, new_tiles, 0, 0, new_width, new_height, TRUE);
      color_region (&destPR, &bg);
    }

  /*  copy from the old to the new  */
  pixel_region_init (&destPR, new_tiles, x2, y2, w, h, TRUE);
  if (w && h)
    copy_region (&srcPR, &destPR);

  /*  Push the channel on the undo stack  */
  undo_push_channel_mod (gimage_get_ID (channel->gimage_ID), channel);

  /*  Configure the new channel  */
  channel->tiles = new_tiles;
  channel->width = new_width;
  channel->height = new_height;

  /*  bounds are now unknown  */
  channel->bounds_known = FALSE;

  /*  update the new channel area  */
  drawable_update (channel->ID, 0, 0, channel->width, channel->height);
}


/********************/
/* access functions */

unsigned char *
channel_data (Channel *channel)
{
  return NULL;
}


int
channel_toggle_visibility (Channel *channel)
{
  channel->visible = !channel->visible;

  return channel->visible;
}


TempBuf *
channel_preview (Channel *channel, int width, int height)
{
  MaskBuf * preview_buf;
  PixelRegion srcPR, destPR;
  int subsample;

  /*  The easy way  */
  if (channel->preview_valid &&
      channel->preview->width == width &&
      channel->preview->height == height)
    return channel->preview;
  /*  The hard way  */
  else
    {
      /*  calculate 'acceptable' subsample  */
      subsample = 1;
      if (width < 1) width = 1;
      if (height < 1) height = 1;
      while ((width * (subsample + 1) * 2 < channel->width) &&
	     (height * (subsample + 1) * 2 < channel->height))
	subsample = subsample + 1;

      pixel_region_init (&srcPR, channel->tiles, 0, 0, channel->width, channel->height, FALSE);

      preview_buf = mask_buf_new (width, height);
      destPR.bytes = 1;
      destPR.x = 0;
      destPR.y = 0;
      destPR.w = width;
      destPR.h = height;
      destPR.rowstride = width;
      destPR.data = mask_buf_data (preview_buf);

      subsample_region (&srcPR, &destPR, subsample);

      if (channel->preview_valid)
	mask_buf_free (channel->preview);

      channel->preview = preview_buf;
      channel->preview_valid = 1;

      return channel->preview;
    }
}


void
channel_invalidate_previews (int gimage_id)
{
  link_ptr tmp = channel_list;
  Channel * channel;

  while (tmp)
    {
      channel = (Channel *) tmp->data;
      if (gimage_id == -1 || (channel->gimage_ID == gimage_id))
	drawable_invalidate_preview (channel->ID);
      tmp = next_item (tmp);
    }
}


/****************************/
/* selection mask functions */


Channel *
channel_new_mask (int gimage_ID, int width, int height)
{
  unsigned char black[3] = {0, 0, 0};
  Channel *new_channel;

  /*  Create the new channel  */
  new_channel = channel_new (gimage_ID, width, height, "Selection Mask", 127, black);

  /*  Set the validate procedure  */
  tile_manager_set_validate_proc (new_channel->tiles, channel_validate);

  return new_channel;
}


int
channel_boundary (Channel *mask, BoundSeg **segs_in, BoundSeg **segs_out,
		  int *num_segs_in, int *num_segs_out,
		  int x1, int y1, int x2, int y2)
{
  int x3, y3, x4, y4;
  PixelRegion bPR;

  if (! mask->boundary_known)
    {
      /* free the out of date boundary segments */
      if (mask->segs_in)
	g_free (mask->segs_in);
      if (mask->segs_out)
	g_free (mask->segs_out);

      if (channel_bounds (mask, &x3, &y3, &x4, &y4))
	{
	  pixel_region_init (&bPR, mask->tiles, x3, y3, (x4 - x3), (y4 - y3), FALSE);
	  mask->segs_out = find_mask_boundary (&bPR, &mask->num_segs_out,
					       IgnoreBounds,
					       x1, y1,
					       x2, y2);
	  x1 = MAXIMUM (x1, x3);
	  y1 = MAXIMUM (y1, y3);
	  x2 = MINIMUM (x2, x4);
	  y2 = MINIMUM (y2, y4);

	  if (x2 > x1 && y2 > y1)
	    {
	      pixel_region_init (&bPR, mask->tiles, 0, 0, mask->width, mask->height, FALSE);
	      mask->segs_in =  find_mask_boundary (&bPR, &mask->num_segs_in,
						   WithinBounds,
						   x1, y1,
						   x2, y2);
	    }
	  else
	    {
	      mask->segs_in = NULL;
	      mask->num_segs_in = 0;
	    }
	}
      else
	{
	  mask->segs_in = NULL;
	  mask->segs_out = NULL;
	  mask->num_segs_in = 0;
	  mask->num_segs_out = 0;
	}
      mask->boundary_known = TRUE;
    }

  *segs_in = mask->segs_in;
  *segs_out = mask->segs_out;
  *num_segs_in = mask->num_segs_in;
  *num_segs_out = mask->num_segs_out;

  return TRUE;
}


int
channel_value (Channel *mask, int x, int y)
{
  Tile *tile;
  int val;

  /*  Some checks to cut back on unnecessary work  */
  if (mask->bounds_known)
    {
      if (mask->empty)
	return 0;
      else if (x < mask->x1 || x >= mask->x2 || y < mask->y1 || y >= mask->y2)
	return 0;
    }
  else
    {
      if (x < 0 || x >= mask->width || y < 0 || y >= mask->height)
	return 0;
    }

  tile = tile_manager_get_tile (mask->tiles, x, y, 0);
  tile_ref (tile);
  val = tile->data[(y % TILE_HEIGHT) * TILE_WIDTH + (x % TILE_WIDTH)];
  tile_unref (tile, FALSE);

  return val;
}


int
channel_bounds (Channel *mask, int *x1, int *y1, int *x2, int *y2)
{
  PixelRegion maskPR;
  unsigned char * data;
  int x, y;
  int ex, ey;
  int found;
  void *pr;

  /*  if the mask's bounds have already been reliably calculated...  */
  if (mask->bounds_known)
    {
      *x1 = mask->x1;
      *y1 = mask->y1;
      *x2 = mask->x2;
      *y2 = mask->y2;

      return (mask->empty) ? FALSE : TRUE;
    }

  /*  go through and calculate the bounds  */
  *x1 = mask->width;
  *y1 = mask->height;
  *x2 = 0;
  *y2 = 0;

  pixel_region_init (&maskPR, mask->tiles, 0, 0, mask->width, mask->height, FALSE);
  for (pr = pixel_regions_register (1, &maskPR); pr != NULL; pr = pixel_regions_process (pr))
    {
      data = maskPR.data;
      ex = maskPR.x + maskPR.w;
      ey = maskPR.y + maskPR.h;

      for (y = maskPR.y; y < ey; y++)
	{
	  found = FALSE;
	  for (x = maskPR.x; x < ex; x++, data++)
	    if (*data)
	      {
		if (x < *x1)
		  *x1 = x;
		if (x > *x2)
		  *x2 = x;
		found = TRUE;
	      }
	  if (found)
	    {
	      if (y < *y1)
		*y1 = y;
	      if (y > *y2)
		*y2 = y;
	    }
	}
    }

  *x2 = BOUNDS (*x2 + 1, 0, mask->width);
  *y2 = BOUNDS (*y2 + 1, 0, mask->height);

  if (*x1 == mask->width && *y1 == mask->height)
    {
      mask->empty = TRUE;
      mask->x1 = 0; mask->y1 = 0;
      mask->x2 = mask->width;
      mask->y2 = mask->height;
    }
  else
    {
      mask->empty = FALSE;
      mask->x1 = *x1;
      mask->y1 = *y1;
      mask->x2 = *x2;
      mask->y2 = *y2;
    }
  mask->bounds_known = TRUE;

  return (mask->empty) ? FALSE : TRUE;
}


int
channel_is_empty (Channel *mask)
{
  PixelRegion maskPR;
  unsigned char * data;
  int x, y;
  void * pr;

  if (mask->bounds_known)
    return mask->empty;

  pixel_region_init (&maskPR, mask->tiles, 0, 0, mask->width, mask->height, FALSE);
  for (pr = pixel_regions_register (1, &maskPR); pr != NULL; pr = pixel_regions_process (pr))
    {
      /*  check if any pixel in the mask is non-zero  */
      data = maskPR.data;
      for (y = 0; y < maskPR.h; y++)
	for (x = 0; x < maskPR.w; x++)
	  if (*data++)
	    {
	      pixel_regions_process_stop (pr);
	      return FALSE;
	    }
    }

  /*  The mask is empty, meaning we can set the bounds as known  */
  if (mask->segs_in)
    g_free (mask->segs_in);
  if (mask->segs_out)
    g_free (mask->segs_out);

  mask->empty = TRUE;
  mask->segs_in = NULL;
  mask->segs_out = NULL;
  mask->num_segs_in = 0;
  mask->num_segs_out = 0;
  mask->bounds_known = TRUE;
  mask->boundary_known = TRUE;
  mask->x1 = 0;  mask->y1 = 0;
  mask->x2 = mask->width;
  mask->y2 = mask->height;

  return TRUE;
}


void
channel_add_segment (Channel *mask, int x, int y, int width, int value)
{
  PixelRegion maskPR;
  unsigned char *data;
  int val;
  int x2;
  void * pr;

  /*  check horizontal extents...  */
  x2 = x + width;
  if (x2 < 0) x2 = 0;
  if (x2 > mask->width) x2 = mask->width;
  if (x < 0) x = 0;
  if (x > mask->width) x = mask->width;
  width = x2 - x;
  if (!width) return;

  if (y < 0 || y > mask->height)
    return;

  pixel_region_init (&maskPR, mask->tiles, x, y, width, 1, TRUE);
  for (pr = pixel_regions_register (1, &maskPR); pr != NULL; pr = pixel_regions_process (pr))
    {
      data = maskPR.data;
      width = maskPR.w;
      while (width--)
	{
	  val = *data + value;
	  if (val > 255)
	    val = 255;
	  *data++ = val;
	}
    }
}


void
channel_sub_segment (Channel *mask, int x, int y, int width, int value)
{
  PixelRegion maskPR;
  unsigned char *data;
  int val;
  int x2;
  void * pr;

  /*  check horizontal extents...  */
  x2 = x + width;
  if (x2 < 0) x2 = 0;
  if (x2 > mask->width) x2 = mask->width;
  if (x < 0) x = 0;
  if (x > mask->width) x = mask->width;
  width = x2 - x;
  if (!width) return;

  if (y < 0 || y > mask->height)
    return;

  pixel_region_init (&maskPR, mask->tiles, x, y, width, 1, TRUE);
  for (pr = pixel_regions_register (1, &maskPR); pr != NULL; pr = pixel_regions_process (pr))
    {
      data = maskPR.data;
      width = maskPR.w;
      while (width--)
	{
	  val = *data - value;
	  if (val < 0)
	    val = 0;
	  *data++ = val;
	}
    }
}


void
channel_inter_segment (Channel *mask, int x, int y, int width, int value)
{
  PixelRegion maskPR;
  unsigned char *data;
  int val;
  int x2;
  void * pr;

  /*  check horizontal extents...  */
  x2 = x + width;
  if (x2 < 0) x2 = 0;
  if (x2 > mask->width) x2 = mask->width;
  if (x < 0) x = 0;
  if (x > mask->width) x = mask->width;
  width = x2 - x;
  if (!width) return;

  if (y < 0 || y > mask->height)
    return;

  pixel_region_init (&maskPR, mask->tiles, x, y, width, 1, TRUE);
  for (pr = pixel_regions_register (1, &maskPR); pr != NULL; pr = pixel_regions_process (pr))
    {
      data = maskPR.data;
      width = maskPR.w;
      while (width--)
	{
	  val = MINIMUM(*data, value);
	  *data++ = val;
	}
    }
}

void
channel_combine_rect (Channel *mask, int op, int x, int y, int w, int h)
{
  int i;

  for (i = y; i < y + h; i++)
    {
      if (i >= 0 && i < mask->height)
	switch (op)
	  {
	  case ADD: case REPLACE:
	    channel_add_segment (mask, x, i, w, 255);
	    break;
	  case SUB:
	    channel_sub_segment (mask, x, i, w, 255);
	    break;
	  case INTERSECT:
	    channel_inter_segment (mask, x, i, w, 255);
	    break;
	  }
    }

  /*  Determine new boundary  */
  if (mask->bounds_known && (op == ADD) && !mask->empty)
    {
      if (x < mask->x1)
	mask->x1 = x;
      if (y < mask->y1)
	mask->y1 = y;
      if ((x + w) > mask->x2)
	mask->x2 = (x + w);
      if ((y + h) > mask->y2)
	mask->y2 = (y + h);
    }
  else if (op == REPLACE || mask->empty)
    {
      mask->empty = FALSE;
      mask->x1 = x;
      mask->y1 = y;
      mask->x2 = x + w;
      mask->y2 = y + h;
    }
  else
    mask->bounds_known = FALSE;

  mask->x1 = BOUNDS (mask->x1, 0, mask->width);
  mask->y1 = BOUNDS (mask->y1, 0, mask->height);
  mask->x2 = BOUNDS (mask->x2, 0, mask->width);
  mask->y2 = BOUNDS (mask->y2, 0, mask->height);
}


void
channel_combine_ellipse (Channel *mask, int op, int x, int y, int w, int h,
			 int aa /*  antialias selection?  */)
{
  int i, j;
  int x0, x1, x2;
  int val, last;
  float a_sqr, b_sqr, aob_sqr;
  float w_sqr, h_sqr;
  float y_sqr;
  float t0, t1;
  float r;
  float cx, cy;
  float rad;
  float dist;


  if (!w || !h)
    return;

  a_sqr = (w * w / 4.0);
  b_sqr = (h * h / 4.0);
  aob_sqr = a_sqr / b_sqr;

  cx = x + w / 2.0;
  cy = y + h / 2.0;

  for (i = y; i < (y + h); i++)
    {
      if (i >= 0 && i < mask->height)
	{
	  /*  Non-antialiased code  */
	  if (!aa)
	    {
	      y_sqr = (i + 0.5 - cy) * (i + 0.5 - cy);
	      rad = sqrt (a_sqr - a_sqr * y_sqr / (double) b_sqr);
	      x1 = ROUND (cx - rad);
	      x2 = ROUND (cx + rad);

	      switch (op)
		{
		case ADD: case REPLACE:
		  channel_add_segment (mask, x1, i, (x2 - x1), 255);
		  break;
		case SUB :
		  channel_sub_segment (mask, x1, i, (x2 - x1), 255);
		  break;
		case INTERSECT:
		  channel_inter_segment (mask, x1, i, (x2 - x1), 255);
		  break;
		}
	    }
	  /*  antialiasing  */
	  else
	    {
	      x0 = x;
	      last = 0;
	      h_sqr = (i + 0.5 - cy) * (i + 0.5 - cy);
	      for (j = x; j < (x + w); j++)
		{
		  w_sqr = (j + 0.5 - cx) * (j + 0.5 - cx);

		  if (h_sqr != 0)
		    {
		      t0 = w_sqr / h_sqr;
		      t1 = a_sqr / (t0 + aob_sqr);
		      r = sqrt (t1 + t0 * t1);
		      rad = sqrt (w_sqr + h_sqr);
		      dist = rad - r;
		    }
		  else
		    dist = -1.0;

		  if (dist < -0.5)
		    val = 255;
		  else if (dist < 0.5)
		    val = (int) (255 * (1 - (dist + 0.5)));
		  else
		    val = 0;

		  if (last != val && last)
		    {
		      switch (op)
			{
			case ADD: case REPLACE:
			  channel_add_segment (mask, x0, i, j - x0, last);
			  break;
			case SUB:
			  channel_sub_segment (mask, x0, i, j - x0, last);
			  break;
			case INTERSECT:
			  channel_inter_segment (mask, x0, i, j - x0, last);
			}
		    }

		  if (last != val)
		    {
		      x0 = j;
		      last = val;
		    }
		}

	      if (last)
		{
		  if (op == ADD || op == REPLACE)
		    channel_add_segment (mask, x0, i, j - x0, last);
		  else if (op == SUB)
		    channel_sub_segment (mask, x0, i, j - x0, last);
		  else if (op == INTERSECT)
		    channel_inter_segment (mask, x0, i, j - x0, last);
		}
	    }

	}
    }

  /*  Determine new boundary  */
  if (mask->bounds_known && (op == ADD) && !mask->empty)
    {
      if (x < mask->x1)
	mask->x1 = x;
      if (y < mask->y1)
	mask->y1 = y;
      if ((x + w) > mask->x2)
	mask->x2 = (x + w);
      if ((y + h) > mask->y2)
	mask->y2 = (y + h);
    }
  else if (op == REPLACE || mask->empty)
    {
      mask->empty = FALSE;
      mask->x1 = x;
      mask->y1 = y;
      mask->x2 = x + w;
      mask->y2 = y + h;
    }
  else
    mask->bounds_known = FALSE;

  mask->x1 = BOUNDS (mask->x1, 0, mask->width);
  mask->y1 = BOUNDS (mask->y1, 0, mask->height);
  mask->x2 = BOUNDS (mask->x2, 0, mask->width);
  mask->y2 = BOUNDS (mask->y2, 0, mask->height);
}


void
channel_combine_mask (Channel *mask, Channel *add_on, int op,
		      int off_x, int off_y)
{
  PixelRegion srcPR, destPR;
  unsigned char *src;
  unsigned char *dest;
  int val;
  int x1, y1, x2, y2;
  int x, y;
  int w, h;
  void * pr;

  x1 = BOUNDS (off_x, 0, mask->width);
  y1 = BOUNDS (off_y, 0, mask->height);
  x2 = BOUNDS (off_x + add_on->width, 0, mask->width);
  y2 = BOUNDS (off_y + add_on->height, 0, mask->height);

  w = (x2 - x1);
  h = (y2 - y1);

  pixel_region_init (&srcPR, add_on->tiles, (x1 - off_x), (y1 - off_y), w, h, FALSE);
  pixel_region_init (&destPR, mask->tiles, x1, y1, w, h, TRUE);

  for (pr = pixel_regions_register (2, &srcPR, &destPR); pr != NULL; pr = pixel_regions_process (pr))
    {
      src = srcPR.data;
      dest = destPR.data;

      for (y = 0; y < srcPR.h; y++)
	{
	  for (x = 0; x < srcPR.w; x++)
	    {
	      switch (op)
		{
		case ADD: case REPLACE:
		  val = dest[x] + src[x];
		  if (val > 255) val = 255;
		  break;
		case SUB:
		  val = dest[x] - src[x];
		  if (val < 0) val = 0;
		  break;
		case INTERSECT:
		  val = MINIMUM(dest[x], src[x]);
		  break;
		default:
		  val = 0;
		  break;
		}
	      dest[x] = val;
	    }
	  src += srcPR.rowstride;
	  dest += destPR.rowstride;
	}
    }

  mask->bounds_known = FALSE;
}


void
channel_feather (Channel *input, Channel *output, double radius,
		 int op, int off_x, int off_y)
{
  int x1, y1, x2, y2;
  PixelRegion srcPR;

  x1 = BOUNDS (off_x, 0, output->width);
  y1 = BOUNDS (off_y, 0, output->height);
  x2 = BOUNDS (off_x + input->width, 0, output->width);
  y2 = BOUNDS (off_y + input->height, 0, output->height);

  pixel_region_init (&srcPR, input->tiles, (x1 - off_x), (y1 - off_y), (x2 - x1), (y2 - y1), FALSE);
  gaussian_blur_region (&srcPR, radius);

  if (input != output) 
    channel_combine_mask(output, input, op, 0, 0);

  output->bounds_known = FALSE;
}


void
channel_push_undo (Channel *mask)
{
  int x1, y1, x2, y2;
  MaskUndo *mask_undo;
  TileManager *undo_tiles;
  PixelRegion srcPR, destPR;
  GImage *gimage;

  mask_undo = (MaskUndo *) g_malloc (sizeof (MaskUndo));
  if (channel_bounds (mask, &x1, &y1, &x2, &y2))
    {
      undo_tiles = tile_manager_new ((x2 - x1), (y2 - y1), 1);
      pixel_region_init (&srcPR, mask->tiles, x1, y1, (x2 - x1), (y2 - y1), FALSE);
      pixel_region_init (&destPR, undo_tiles, 0, 0, (x2 - x1), (y2 - y1), TRUE);
      copy_region (&srcPR, &destPR);
    }
  else
    undo_tiles = NULL;

  mask_undo->tiles = undo_tiles;
  mask_undo->x = x1;
  mask_undo->y = y1;

  /* push the undo buffer onto the undo stack */
  gimage = gimage_get_ID (mask->gimage_ID);
  undo_push_mask (gimage, mask_undo);
  gimage_mask_invalidate (gimage);

  /*  invalidate the preview  */
  mask->preview_valid = 0;
}


void
channel_clear (Channel *mask)
{
  PixelRegion maskPR;
  unsigned char bg = 0;

  /*  push the current channel onto the undo stack  */
  channel_push_undo (mask);

  if (mask->bounds_known && !mask->empty)
    {
      pixel_region_init (&maskPR, mask->tiles, mask->x1, mask->y1,
			 (mask->x2 - mask->x1), (mask->y2 - mask->y1), TRUE);
      color_region (&maskPR, &bg);
    }
  else
    {
      /*  clear the mask  */
      pixel_region_init (&maskPR, mask->tiles, 0, 0, mask->width, mask->height, TRUE);
      color_region (&maskPR, &bg);
    }

  /*  we know the bounds  */
  mask->bounds_known = TRUE;
  mask->empty = TRUE;
  mask->x1 = 0; mask->y1 = 0;
  mask->x2 = mask->width;
  mask->y2 = mask->height;
}


void
channel_invert (Channel *mask)
{
  PixelRegion maskPR;
  unsigned char *data;
  int size;
  void * pr;

  /*  push the current channel onto the undo stack  */
  channel_push_undo (mask);

  pixel_region_init (&maskPR, mask->tiles, 0, 0, mask->width, mask->height, TRUE);
  for (pr = pixel_regions_register (1, &maskPR); pr != NULL; pr = pixel_regions_process (pr))
    {
      /*  subtract each pixel in the mask from 255  */
      data = maskPR.data;
      size = maskPR.w * maskPR.h;
      while (size --)
	{
	  *data = 255 - *data;
	  data++;
	}
    }

  mask->bounds_known = FALSE;
}


void
channel_sharpen (Channel *mask)
{
  PixelRegion maskPR;
  unsigned char *data;
  int size;
  void * pr;

  /*  push the current channel onto the undo stack  */
  channel_push_undo (mask);

  pixel_region_init (&maskPR, mask->tiles, 0, 0, mask->width, mask->height, TRUE);
  for (pr = pixel_regions_register (1, &maskPR); pr != NULL; pr = pixel_regions_process (pr))
    {
      /*  if a pixel in the mask has a non-zero value, make it 255  */
      data = maskPR.data;
      size = maskPR.w * maskPR.h;
      while (size--)
	{
	  if (*data > HALF_WAY)
	    *data++ = 255;
	  else
	    *data++ = 0;
	}
    }

  mask->bounds_known = FALSE;
}


void
channel_all (Channel *mask)
{
  PixelRegion maskPR;
  unsigned char bg = 255;

  /*  push the current channel onto the undo stack  */
  channel_push_undo (mask);

  /*  clear the mask  */
  pixel_region_init (&maskPR, mask->tiles, 0, 0, mask->width, mask->height, TRUE);
  color_region (&maskPR, &bg);

  /*  we know the bounds  */
  mask->bounds_known = TRUE;
  mask->empty = FALSE;
  mask->x1 = 0; mask->y1 = 0;
  mask->x2 = mask->width;
  mask->y2 = mask->height;
}


void
channel_border (Channel *mask, int radius)
{
  PixelRegion bPR;
  int x1, y1, x2, y2;
  BoundSeg * bs;
  unsigned char bg = 0;
  int num_segs;

  if (! channel_bounds (mask, &x1, &y1, &x2, &y2))
    return;

  /*  push the current channel onto the undo stack  */
  channel_push_undo (mask);

  pixel_region_init (&bPR, mask->tiles, x1, y1, (x2 - x1), (y2 - y1), FALSE);
  bs =  find_mask_boundary (&bPR, &num_segs, WithinBounds, x1, y1, x2, y2);

  /*  clear the channel  */
  if (mask->bounds_known && !mask->empty)
    {
      pixel_region_init (&bPR, mask->tiles, mask->x1, mask->y1,
			 (mask->x2 - mask->x1), (mask->y2 - mask->y1), TRUE);
      color_region (&bPR, &bg);
    }
  else
    {
      /*  clear the mask  */
      pixel_region_init (&bPR, mask->tiles, 0, 0, mask->width, mask->height, TRUE);
      color_region (&bPR, &bg);
    }

  /*  calculate a border of specified radius based on the boundary segments  */
  pixel_region_init (&bPR, mask->tiles, 0, 0, mask->width, mask->height, TRUE);
  border_region (&bPR, bs, num_segs, radius);

  g_free (bs);

  mask->bounds_known = FALSE;
}


void
channel_grow (Channel *mask, int steps)
{
  PixelRegion bPR;

  if (channel_is_empty (mask))
    return;

  /*  push the current channel onto the undo stack  */
  channel_push_undo (mask);

  /*  need full extents for grow  */
  pixel_region_init (&bPR, mask->tiles, 0, 0, mask->width, mask->height, TRUE);

  while (steps--)
    thin_region (&bPR, GROW_REGION);

  mask->bounds_known = FALSE;
}


void
channel_shrink (Channel *mask, int steps)
{
  PixelRegion bPR;
  int x1, y1, x2, y2;

  if (! channel_bounds (mask, &x1, &y1, &x2, &y2))
    return;

  /*  push the current channel onto the undo stack  */
  channel_push_undo (mask);

  pixel_region_init (&bPR, mask->tiles, x1, y1, (x2 - x1), (y2 - y1), TRUE);

  while (steps--)
    thin_region (&bPR, SHRINK_REGION);

  mask->bounds_known = FALSE;
}


void
channel_translate (Channel *mask, int off_x, int off_y)
{
  int width, height;
  Channel *tmp_mask;
  PixelRegion srcPR, destPR;
  unsigned char empty = 0;
  int x1, y1, x2, y2;

  tmp_mask = NULL;

  /*  push the current channel onto the undo stack  */
  channel_push_undo (mask);

  channel_bounds (mask, &x1, &y1, &x2, &y2);
  x1 = BOUNDS ((x1 + off_x), 0, mask->width);
  y1 = BOUNDS ((y1 + off_y), 0, mask->height);
  x2 = BOUNDS ((x2 + off_x), 0, mask->width);
  y2 = BOUNDS ((y2 + off_y), 0, mask->height);

  width = x2 - x1;
  height = y2 - y1;

  /*  make sure width and height are non-zero  */
  if (width != 0 && height != 0)
    {
      /*  copy the portion of the mask we will keep to a
       *  temporary buffer
       */
      tmp_mask = channel_new_mask (mask->gimage_ID, width, height);

      pixel_region_init (&srcPR, mask->tiles, x1 - off_x, y1 - off_y, width, height, FALSE);
      pixel_region_init (&destPR, tmp_mask->tiles, 0, 0, width, height, TRUE);
      copy_region (&srcPR, &destPR);
    }

  /*  clear the mask  */
  pixel_region_init (&srcPR, mask->tiles, 0, 0, mask->width, mask->height, TRUE);
  color_region (&srcPR, &empty);

  if (width != 0 && height != 0)
    {
      /*  copy the temp mask back to the mask  */
      pixel_region_init (&srcPR, tmp_mask->tiles, 0, 0, width, height, FALSE);
      pixel_region_init (&destPR, mask->tiles, x1, y1, width, height, TRUE);
      copy_region (&srcPR, &destPR);

      /*  free the temporary mask  */
      channel_delete (tmp_mask);
    }

  /*  calculate new bounds  */
  if (width == 0 || height == 0)
    {
      mask->empty = TRUE;
      mask->x1 = 0; mask->y1 = 0;
      mask->x2 = mask->width;
      mask->y2 = mask->height;
    }
  else
    {
      mask->x1 = x1;
      mask->y1 = y1;
      mask->x2 = x2;
      mask->y2 = y2;
    }
}


void
channel_layer_alpha (Channel *mask, int layer_id)
{
  PixelRegion srcPR, destPR;
  Layer *layer;
  unsigned char empty = 0;
  int x1, y1, x2, y2;

  /*  push the current mask onto the undo stack  */
  channel_push_undo (mask);

  /*  clear the mask  */
  pixel_region_init (&destPR, mask->tiles, 0, 0, mask->width, mask->height, TRUE);
  color_region (&destPR, &empty);

  layer = layer_get_ID (layer_id);
  x1 = BOUNDS (layer->offset_x, 0, mask->width);
  y1 = BOUNDS (layer->offset_y, 0, mask->height);
  x2 = BOUNDS (layer->offset_x + layer->width, 0, mask->width);
  y2 = BOUNDS (layer->offset_y + layer->height, 0, mask->height);

  pixel_region_init (&srcPR, layer->tiles,
		     (x1 - layer->offset_x), (y1 - layer->offset_y),
		     (x2 - x1), (y2 - y1), FALSE);
  pixel_region_init (&destPR, mask->tiles, x1, y1, (x2 - x1), (y2 - y1), TRUE);
  extract_alpha_region (&srcPR, NULL, &destPR);

  mask->bounds_known = FALSE;
}


void
channel_layer_mask (Channel *mask, int layer_id)
{
  PixelRegion srcPR, destPR;
  Layer *layer;
  unsigned char empty = 0;
  int x1, y1, x2, y2;

  /*  push the current mask onto the undo stack  */
  channel_push_undo (mask);

  /*  clear the mask  */
  pixel_region_init (&destPR, mask->tiles, 0, 0, mask->width, mask->height, TRUE);
  color_region (&destPR, &empty);

  layer = layer_get_ID (layer_id);
  x1 = BOUNDS (layer->offset_x, 0, mask->width);
  y1 = BOUNDS (layer->offset_y, 0, mask->height);
  x2 = BOUNDS (layer->offset_x + layer->width, 0, mask->width);
  y2 = BOUNDS (layer->offset_y + layer->height, 0, mask->height);

  pixel_region_init (&srcPR, layer->mask->tiles,
		     (x1 - layer->offset_x), (y1 - layer->offset_y),
		     (x2 - x1), (y2 - y1), FALSE);
  pixel_region_init (&destPR, mask->tiles, x1, y1, (x2 - x1), (y2 - y1), TRUE);
  copy_region (&srcPR, &destPR);

  mask->bounds_known = FALSE;
}


void
channel_load (Channel *mask, Channel *channel)
{
  PixelRegion srcPR, destPR;

  /*  push the current mask onto the undo stack  */
  channel_push_undo (mask);

  /*  copy the channel to the mask  */
  pixel_region_init (&srcPR, channel->tiles, 0, 0, channel->width, channel->height, FALSE);
  pixel_region_init (&destPR, mask->tiles, 0, 0, channel->width, channel->height, TRUE);
  copy_region (&srcPR, &destPR);

  mask->bounds_known = FALSE;
}
