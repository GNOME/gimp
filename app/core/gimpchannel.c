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
#include "paint_funcs.h"
#include "temp_buf.h"
#include "undo.h"

#include "channel_pvt.h"
#include "tile.h"

/*
enum {
  LAST_SIGNAL
};
*/

static void gimp_channel_class_init (GimpChannelClass *klass);
static void gimp_channel_init       (GimpChannel      *channel);
static void gimp_channel_destroy    (GtkObject        *object);

/*
static gint channel_signals[LAST_SIGNAL] = { 0 };
*/

static GimpDrawableClass *parent_class = NULL;

GtkType
gimp_channel_get_type ()
{
  static GtkType channel_type = 0;

  if (!channel_type)
    {
      GtkTypeInfo channel_info =
      {
	"GimpChannel",
	sizeof (GimpChannel),
	sizeof (GimpChannelClass),
	(GtkClassInitFunc) gimp_channel_class_init,
	(GtkObjectInitFunc) gimp_channel_init,
        /* reserved_1 */ NULL,
	/* reserved_2 */ NULL,
	(GtkClassInitFunc) NULL,
      };

      channel_type = gtk_type_unique (gimp_drawable_get_type (), &channel_info);
    }

  return channel_type;
}

static void
gimp_channel_class_init (GimpChannelClass *class)
{
  GtkObjectClass *object_class;

  object_class = (GtkObjectClass*) class;
  parent_class = gtk_type_class (gimp_drawable_get_type ());

  /*
  gtk_object_class_add_signals (object_class, channel_signals, LAST_SIGNAL);
  */

  object_class->destroy = gimp_channel_destroy;
}

static void
gimp_channel_init (GimpChannel *channel)
{
}

#define ROUND(x) ((int) (x + 0.5))

/*
 *  Static variables
 */
extern int global_drawable_ID;

int channel_get_count = 0;


/**************************/
/*  Function definitions  */

static void
channel_validate (TileManager *tm, Tile *tile)
{
  /*  Set the contents of the tile to empty  */
  memset (tile_data_pointer (tile, 0, 0), 
	  TRANSPARENT_OPACITY, tile_size(tile));
}




Channel *
channel_new (GimpImage* gimage, int width, int height, char *name, int opacity,
	     unsigned char *col)
{
  Channel * channel;
  int i;

  channel = gtk_type_new (gimp_channel_get_type ());

  gimp_drawable_configure (GIMP_DRAWABLE(channel), 
			   gimage, width, height, GRAY_GIMAGE, name);

  /*  set the channel color and opacity  */
  for (i = 0; i < 3; i++)
    channel->col[i] = col[i];
  channel->opacity = opacity;
  channel->show_masked = 1;

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

  return channel;
}

Channel *
channel_ref (Channel *channel)
{
  gtk_object_ref  (GTK_OBJECT (channel));
  gtk_object_sink (GTK_OBJECT (channel));
  return channel;
}


void
channel_unref (Channel *channel)
{
  gtk_object_unref (GTK_OBJECT (channel));
}


Channel *
channel_copy (Channel *channel)
{
  char * channel_name;
  Channel * new_channel;
  PixelRegion srcPR, destPR;

  /*  formulate the new channel name  */
  channel_name = (char *) g_malloc (strlen (GIMP_DRAWABLE(channel)->name) + 6);
  sprintf (channel_name, "%s copy", GIMP_DRAWABLE(channel)->name);

  /*  allocate a new channel object  */
  new_channel = channel_new (GIMP_DRAWABLE(channel)->gimage, 
			     GIMP_DRAWABLE(channel)->width, 
			     GIMP_DRAWABLE(channel)->height, 
			     channel_name, channel->opacity, channel->col);
  GIMP_DRAWABLE(new_channel)->visible = GIMP_DRAWABLE(channel)->visible;
  new_channel->show_masked = channel->show_masked;

  /*  copy the contents across channels  */
  pixel_region_init (&srcPR, GIMP_DRAWABLE(channel)->tiles, 0, 0, 
		     GIMP_DRAWABLE(channel)->width, 
		     GIMP_DRAWABLE(channel)->height, FALSE);
  pixel_region_init (&destPR, GIMP_DRAWABLE(new_channel)->tiles, 0, 0, GIMP_DRAWABLE(channel)->width, GIMP_DRAWABLE(channel)->height, TRUE);
  copy_region (&srcPR, &destPR);

  /*  free up the channel_name memory  */
  g_free (channel_name);

  return new_channel;
}


Channel *
channel_get_ID (int ID)
{
  GimpDrawable *drawable;
  drawable = drawable_get_ID (ID);
  if (drawable && GIMP_IS_CHANNEL (drawable)) 
    return GIMP_CHANNEL (drawable);
  else
    return NULL;
}


void
channel_delete (Channel *channel)
{
  gtk_object_unref (GTK_OBJECT (channel));
}

static void
gimp_channel_destroy (GtkObject *object)
{
  GimpChannel *channel;

  g_return_if_fail (object != NULL);
  g_return_if_fail (GIMP_IS_CHANNEL (object));

  channel = GIMP_CHANNEL (object);

  /* free the segments?  */
  if (channel->segs_in)
    g_free (channel->segs_in);
  if (channel->segs_out)
    g_free (channel->segs_out);

  if (GTK_OBJECT_CLASS (parent_class)->destroy)
    (*GTK_OBJECT_CLASS (parent_class)->destroy) (object);
  
}

void
channel_scale (Channel *channel, int new_width, int new_height)
{
  PixelRegion srcPR, destPR;
  TileManager *new_tiles;

  if (new_width == 0 || new_height == 0)
    return;

  /*  Update the old channel position  */
  drawable_update (GIMP_DRAWABLE(channel), 0, 0, GIMP_DRAWABLE(channel)->width, GIMP_DRAWABLE(channel)->height);

  /*  Configure the pixel regions  */
  pixel_region_init (&srcPR, GIMP_DRAWABLE(channel)->tiles, 0, 0, GIMP_DRAWABLE(channel)->width, GIMP_DRAWABLE(channel)->height, FALSE);

  /*  Allocate the new channel, configure dest region  */
  new_tiles = tile_manager_new (new_width, new_height, 1);
  pixel_region_init (&destPR, new_tiles, 0, 0, new_width, new_height, TRUE);

  /*  Add an alpha channel  */
  scale_region (&srcPR, &destPR);

  /*  Push the channel on the undo stack  */
  undo_push_channel_mod (GIMP_DRAWABLE(channel)->gimage, channel);

  /*  Configure the new channel  */
  GIMP_DRAWABLE(channel)->tiles = new_tiles;
  GIMP_DRAWABLE(channel)->width = new_width;
  GIMP_DRAWABLE(channel)->height = new_height;

  /*  bounds are now unknown  */
  channel->bounds_known = FALSE;

  /*  Update the new channel position  */
  drawable_update (GIMP_DRAWABLE(channel), 0, 0, GIMP_DRAWABLE(channel)->width, GIMP_DRAWABLE(channel)->height);
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
  x2 = BOUNDS ((offx + GIMP_DRAWABLE(channel)->width), 0, new_width);
  y2 = BOUNDS ((offy + GIMP_DRAWABLE(channel)->height), 0, new_height);
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
  drawable_update (GIMP_DRAWABLE(channel), 0, 0, GIMP_DRAWABLE(channel)->width, GIMP_DRAWABLE(channel)->height);

  /*  Configure the pixel regions  */
  pixel_region_init (&srcPR, GIMP_DRAWABLE(channel)->tiles, x1, y1, w, h, FALSE);

  /*  Determine whether the new channel needs to be initially cleared  */
  if ((new_width > GIMP_DRAWABLE(channel)->width) ||
      (new_height > GIMP_DRAWABLE(channel)->height) ||
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
  undo_push_channel_mod (GIMP_DRAWABLE(channel)->gimage, channel);

  /*  Configure the new channel  */
  GIMP_DRAWABLE(channel)->tiles = new_tiles;
  GIMP_DRAWABLE(channel)->width = new_width;
  GIMP_DRAWABLE(channel)->height = new_height;

  /*  bounds are now unknown  */
  channel->bounds_known = FALSE;

  /*  update the new channel area  */
  drawable_update (GIMP_DRAWABLE(channel), 0, 0, GIMP_DRAWABLE(channel)->width, GIMP_DRAWABLE(channel)->height);
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
  GIMP_DRAWABLE(channel)->visible = !GIMP_DRAWABLE(channel)->visible;

  return GIMP_DRAWABLE(channel)->visible;
}


TempBuf *
channel_preview (Channel *channel, int width, int height)
{
  MaskBuf * preview_buf;
  PixelRegion srcPR, destPR;
  int subsample;

  /*  The easy way  */
  if (GIMP_DRAWABLE(channel)->preview_valid &&
      GIMP_DRAWABLE(channel)->preview->width == width &&
      GIMP_DRAWABLE(channel)->preview->height == height)
    return GIMP_DRAWABLE(channel)->preview;
  /*  The hard way  */
  else
    {
      /*  calculate 'acceptable' subsample  */
      subsample = 1;
      if (width < 1) width = 1;
      if (height < 1) height = 1;
      while ((width * (subsample + 1) * 2 < GIMP_DRAWABLE(channel)->width) &&
	     (height * (subsample + 1) * 2 < GIMP_DRAWABLE(channel)->height))
	subsample = subsample + 1;

      pixel_region_init (&srcPR, GIMP_DRAWABLE(channel)->tiles, 0, 0, GIMP_DRAWABLE(channel)->width, GIMP_DRAWABLE(channel)->height, FALSE);

      preview_buf = mask_buf_new (width, height);
      destPR.bytes = 1;
      destPR.x = 0;
      destPR.y = 0;
      destPR.w = width;
      destPR.h = height;
      destPR.rowstride = width;
      destPR.data = mask_buf_data (preview_buf);

      subsample_region (&srcPR, &destPR, subsample);

      if (GIMP_DRAWABLE(channel)->preview_valid)
	mask_buf_free (GIMP_DRAWABLE(channel)->preview);

      GIMP_DRAWABLE(channel)->preview = preview_buf;
      GIMP_DRAWABLE(channel)->preview_valid = 1;

      return GIMP_DRAWABLE(channel)->preview;
    }
}


void
channel_invalidate_previews (GimpImage* gimage)
{
  GSList * tmp;
  Channel * channel;

  tmp = gimage->channels;

  while (tmp)
    {
      channel = (Channel *) tmp->data;
      drawable_invalidate_preview (GIMP_DRAWABLE(channel));
      tmp = g_slist_next (tmp);
    }
}


/****************************/
/* selection mask functions */


Channel *
channel_new_mask (GimpImage* gimage, int width, int height)
{
  unsigned char black[3] = {0, 0, 0};
  Channel *new_channel;

  /*  Create the new channel  */
  new_channel = channel_new (gimage, width, height, "Selection Mask", 127, black);

  /*  Set the validate procedure  */
  tile_manager_set_validate_proc (GIMP_DRAWABLE(new_channel)->tiles, channel_validate);

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
	  pixel_region_init (&bPR, GIMP_DRAWABLE(mask)->tiles, x3, y3, (x4 - x3), (y4 - y3), FALSE);
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
	      pixel_region_init (&bPR, GIMP_DRAWABLE(mask)->tiles, 0, 0, GIMP_DRAWABLE(mask)->width, GIMP_DRAWABLE(mask)->height, FALSE);
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
      if (x < 0 || x >= GIMP_DRAWABLE(mask)->width || y < 0 || y >= GIMP_DRAWABLE(mask)->height)
	return 0;
    }

  tile = tile_manager_get_tile (GIMP_DRAWABLE(mask)->tiles, x, y, TRUE, FALSE);
  val = *(unsigned char *)(tile_data_pointer (tile, x % TILE_WIDTH, y % TILE_HEIGHT));
  tile_release (tile, FALSE);

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
  *x1 = GIMP_DRAWABLE(mask)->width;
  *y1 = GIMP_DRAWABLE(mask)->height;
  *x2 = 0;
  *y2 = 0;

  pixel_region_init (&maskPR, GIMP_DRAWABLE(mask)->tiles, 0, 0, GIMP_DRAWABLE(mask)->width, GIMP_DRAWABLE(mask)->height, FALSE);
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

  *x2 = BOUNDS (*x2 + 1, 0, GIMP_DRAWABLE(mask)->width);
  *y2 = BOUNDS (*y2 + 1, 0, GIMP_DRAWABLE(mask)->height);

  if (*x1 == GIMP_DRAWABLE(mask)->width && *y1 == GIMP_DRAWABLE(mask)->height)
    {
      mask->empty = TRUE;
      mask->x1 = 0; mask->y1 = 0;
      mask->x2 = GIMP_DRAWABLE(mask)->width;
      mask->y2 = GIMP_DRAWABLE(mask)->height;
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

  pixel_region_init (&maskPR, GIMP_DRAWABLE(mask)->tiles, 0, 0, GIMP_DRAWABLE(mask)->width, GIMP_DRAWABLE(mask)->height, FALSE);
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
  mask->x2 = GIMP_DRAWABLE(mask)->width;
  mask->y2 = GIMP_DRAWABLE(mask)->height;

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
  if (x2 > GIMP_DRAWABLE(mask)->width) x2 = GIMP_DRAWABLE(mask)->width;
  if (x < 0) x = 0;
  if (x > GIMP_DRAWABLE(mask)->width) x = GIMP_DRAWABLE(mask)->width;
  width = x2 - x;
  if (!width) return;

  if (y < 0 || y > GIMP_DRAWABLE(mask)->height)
    return;

  pixel_region_init (&maskPR, GIMP_DRAWABLE(mask)->tiles, x, y, width, 1, TRUE);
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
  if (x2 > GIMP_DRAWABLE(mask)->width) x2 = GIMP_DRAWABLE(mask)->width;
  if (x < 0) x = 0;
  if (x > GIMP_DRAWABLE(mask)->width) x = GIMP_DRAWABLE(mask)->width;
  width = x2 - x;
  if (!width) return;

  if (y < 0 || y > GIMP_DRAWABLE(mask)->height)
    return;

  pixel_region_init (&maskPR, GIMP_DRAWABLE(mask)->tiles, x, y, width, 1, TRUE);
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
  if (x2 > GIMP_DRAWABLE(mask)->width) x2 = GIMP_DRAWABLE(mask)->width;
  if (x < 0) x = 0;
  if (x > GIMP_DRAWABLE(mask)->width) x = GIMP_DRAWABLE(mask)->width;
  width = x2 - x;
  if (!width) return;

  if (y < 0 || y > GIMP_DRAWABLE(mask)->height)
    return;

  pixel_region_init (&maskPR, GIMP_DRAWABLE(mask)->tiles, x, y, width, 1, TRUE);
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
      if (i >= 0 && i < GIMP_DRAWABLE(mask)->height)
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

  mask->x1 = BOUNDS (mask->x1, 0, GIMP_DRAWABLE(mask)->width);
  mask->y1 = BOUNDS (mask->y1, 0, GIMP_DRAWABLE(mask)->height);
  mask->x2 = BOUNDS (mask->x2, 0, GIMP_DRAWABLE(mask)->width);
  mask->y2 = BOUNDS (mask->y2, 0, GIMP_DRAWABLE(mask)->height);
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
      if (i >= 0 && i < GIMP_DRAWABLE(mask)->height)
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

  mask->x1 = BOUNDS (mask->x1, 0, GIMP_DRAWABLE(mask)->width);
  mask->y1 = BOUNDS (mask->y1, 0, GIMP_DRAWABLE(mask)->height);
  mask->x2 = BOUNDS (mask->x2, 0, GIMP_DRAWABLE(mask)->width);
  mask->y2 = BOUNDS (mask->y2, 0, GIMP_DRAWABLE(mask)->height);
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

  x1 = BOUNDS (off_x, 0, GIMP_DRAWABLE(mask)->width);
  y1 = BOUNDS (off_y, 0, GIMP_DRAWABLE(mask)->height);
  x2 = BOUNDS (off_x + GIMP_DRAWABLE(add_on)->width, 0, GIMP_DRAWABLE(mask)->width);
  y2 = BOUNDS (off_y + GIMP_DRAWABLE(add_on)->height, 0, GIMP_DRAWABLE(mask)->height);

  w = (x2 - x1);
  h = (y2 - y1);

  pixel_region_init (&srcPR, GIMP_DRAWABLE(add_on)->tiles, (x1 - off_x), (y1 - off_y), w, h, FALSE);
  pixel_region_init (&destPR, GIMP_DRAWABLE(mask)->tiles, x1, y1, w, h, TRUE);

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

  x1 = BOUNDS (off_x, 0, GIMP_DRAWABLE(output)->width);
  y1 = BOUNDS (off_y, 0, GIMP_DRAWABLE(output)->height);
  x2 = BOUNDS (off_x + GIMP_DRAWABLE(input)->width, 0, GIMP_DRAWABLE(output)->width);
  y2 = BOUNDS (off_y + GIMP_DRAWABLE(input)->height, 0, GIMP_DRAWABLE(output)->height);

  pixel_region_init (&srcPR, GIMP_DRAWABLE(input)->tiles, (x1 - off_x), (y1 - off_y), (x2 - x1), (y2 - y1), FALSE);
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
      pixel_region_init (&srcPR, GIMP_DRAWABLE(mask)->tiles, x1, y1, (x2 - x1), (y2 - y1), FALSE);
      pixel_region_init (&destPR, undo_tiles, 0, 0, (x2 - x1), (y2 - y1), TRUE);
      copy_region (&srcPR, &destPR);
    }
  else
    undo_tiles = NULL;

  mask_undo->tiles = undo_tiles;
  mask_undo->x = x1;
  mask_undo->y = y1;

  /* push the undo buffer onto the undo stack */
  gimage = GIMP_DRAWABLE(mask)->gimage;
  undo_push_mask (gimage, mask_undo);
  gimage_mask_invalidate (gimage);

  /*  invalidate the preview  */
  GIMP_DRAWABLE(mask)->preview_valid = 0;
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
      pixel_region_init (&maskPR, GIMP_DRAWABLE(mask)->tiles, mask->x1, mask->y1,
			 (mask->x2 - mask->x1), (mask->y2 - mask->y1), TRUE);
      color_region (&maskPR, &bg);
    }
  else
    {
      /*  clear the mask  */
      pixel_region_init (&maskPR, GIMP_DRAWABLE(mask)->tiles, 0, 0, GIMP_DRAWABLE(mask)->width, GIMP_DRAWABLE(mask)->height, TRUE);
      color_region (&maskPR, &bg);
    }

  /*  we know the bounds  */
  mask->bounds_known = TRUE;
  mask->empty = TRUE;
  mask->x1 = 0; mask->y1 = 0;
  mask->x2 = GIMP_DRAWABLE(mask)->width;
  mask->y2 = GIMP_DRAWABLE(mask)->height;
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

  pixel_region_init (&maskPR, GIMP_DRAWABLE(mask)->tiles, 0, 0, GIMP_DRAWABLE(mask)->width, GIMP_DRAWABLE(mask)->height, TRUE);
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

  pixel_region_init (&maskPR, GIMP_DRAWABLE(mask)->tiles, 0, 0, GIMP_DRAWABLE(mask)->width, GIMP_DRAWABLE(mask)->height, TRUE);
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
  pixel_region_init (&maskPR, GIMP_DRAWABLE(mask)->tiles, 0, 0, GIMP_DRAWABLE(mask)->width, GIMP_DRAWABLE(mask)->height, TRUE);
  color_region (&maskPR, &bg);

  /*  we know the bounds  */
  mask->bounds_known = TRUE;
  mask->empty = FALSE;
  mask->x1 = 0; mask->y1 = 0;
  mask->x2 = GIMP_DRAWABLE(mask)->width;
  mask->y2 = GIMP_DRAWABLE(mask)->height;
}


void
channel_border (Channel *mask, int radius)
{
  PixelRegion bPR;
  int x1, y1, x2, y2;

  if (radius < 0)
    return;
  if (! channel_bounds (mask, &x1, &y1, &x2, &y2))
    return;
  if (x1 - radius < 0)
    x1 = 0;
  else
    x1 -= radius;
  if (x2 + radius > GIMP_DRAWABLE(mask)->width)
    x2 = GIMP_DRAWABLE(mask)->width;
  else
    x2 += radius;

  if (y1 - radius < 0)
    y1 = 0;
  else
    y1 -= radius;
  if (y2 + radius > GIMP_DRAWABLE(mask)->height)
    y2 = GIMP_DRAWABLE(mask)->height;
  else
    y2 += radius;
  /*  push the current channel onto the undo stack  */
  channel_push_undo (mask);

  pixel_region_init (&bPR, GIMP_DRAWABLE(mask)->tiles, x1, y1,
		     (x2-x1), (y2-y1), TRUE);

  border_region(&bPR, radius);

  mask->bounds_known = FALSE;
}


void
channel_grow (Channel *mask, int radius)
{
  PixelRegion bPR;
  int x1, y1, x2, y2;

  if (radius < 0)
  {
    channel_shrink(mask, -radius);
    return;
  }

  if (! channel_bounds (mask, &x1, &y1, &x2, &y2))
    return;

  if (channel_is_empty (mask))
    return;

  if (x1 - radius > 0)
    x1 = x1 - radius;
  else
    x1 = 0;
  if (y1 - radius > 0)
    y1 = y1 - radius;
  else
    y1 = 0;
  if (x2 + radius< GIMP_DRAWABLE(mask)->width)
    x2 = x2 + radius;
  else
    x2 = GIMP_DRAWABLE(mask)->width;
  if (y2 + radius< GIMP_DRAWABLE(mask)->height)
    y2 = y2 + radius;
  else
    y2 = GIMP_DRAWABLE(mask)->height;

  /*  push the current channel onto the undo stack  */
  channel_push_undo (mask);

  /*  need full extents for grow, not! */
  pixel_region_init (&bPR, GIMP_DRAWABLE(mask)->tiles, x1, y1, (x2 - x1),
		     (y2 - y1), TRUE);

  fatten_region(&bPR, radius);
  mask->bounds_known = FALSE;
}


void
channel_shrink (Channel *mask, int radius)
{
  PixelRegion bPR;
  int x1, y1, x2, y2;

  if (radius < 0)
  {
    channel_shrink(mask, -radius);
    return;
  }
  if (! channel_bounds (mask, &x1, &y1, &x2, &y2))
    return;

  if (channel_is_empty (mask))
    return;

  /*  push the current channel onto the undo stack  */
  channel_push_undo (mask);

  if (x1 > 0)
    x1--;
  if (y1 > 0)
    y1--;
  if (x2 < GIMP_DRAWABLE(mask)->width)
    x2++;
  if (y2 < GIMP_DRAWABLE(mask)->height)
    y2++;
  
  pixel_region_init (&bPR, GIMP_DRAWABLE(mask)->tiles, x1, y1, (x2 - x1),
		     (y2 - y1), TRUE);

  thin_region (&bPR, radius);

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
  x1 = BOUNDS ((x1 + off_x), 0, GIMP_DRAWABLE(mask)->width);
  y1 = BOUNDS ((y1 + off_y), 0, GIMP_DRAWABLE(mask)->height);
  x2 = BOUNDS ((x2 + off_x), 0, GIMP_DRAWABLE(mask)->width);
  y2 = BOUNDS ((y2 + off_y), 0, GIMP_DRAWABLE(mask)->height);

  width = x2 - x1;
  height = y2 - y1;

  /*  make sure width and height are non-zero  */
  if (width != 0 && height != 0)
    {
      /*  copy the portion of the mask we will keep to a
       *  temporary buffer
       */
      tmp_mask = channel_new_mask (GIMP_DRAWABLE(mask)->gimage, width, height);

      pixel_region_init (&srcPR, GIMP_DRAWABLE(mask)->tiles, x1 - off_x, y1 - off_y, width, height, FALSE);
      pixel_region_init (&destPR, GIMP_DRAWABLE(tmp_mask)->tiles, 0, 0, width, height, TRUE);
      copy_region (&srcPR, &destPR);
    }

  /*  clear the mask  */
  pixel_region_init (&srcPR, GIMP_DRAWABLE(mask)->tiles, 0, 0, GIMP_DRAWABLE(mask)->width, GIMP_DRAWABLE(mask)->height, TRUE);
  color_region (&srcPR, &empty);

  if (width != 0 && height != 0)
    {
      /*  copy the temp mask back to the mask  */
      pixel_region_init (&srcPR, GIMP_DRAWABLE(tmp_mask)->tiles, 0, 0, width, height, FALSE);
      pixel_region_init (&destPR, GIMP_DRAWABLE(mask)->tiles, x1, y1, width, height, TRUE);
      copy_region (&srcPR, &destPR);

      /*  free the temporary mask  */
      channel_delete (tmp_mask);
    }

  /*  calculate new bounds  */
  if (width == 0 || height == 0)
    {
      mask->empty = TRUE;
      mask->x1 = 0; mask->y1 = 0;
      mask->x2 = GIMP_DRAWABLE(mask)->width;
      mask->y2 = GIMP_DRAWABLE(mask)->height;
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
channel_layer_alpha (Channel *mask, Layer *layer)
{
  PixelRegion srcPR, destPR;
  unsigned char empty = 0;
  int x1, y1, x2, y2;

  /*  push the current mask onto the undo stack  */
  channel_push_undo (mask);

  /*  clear the mask  */
  pixel_region_init (&destPR, GIMP_DRAWABLE(mask)->tiles, 0, 0, GIMP_DRAWABLE(mask)->width, GIMP_DRAWABLE(mask)->height, TRUE);
  color_region (&destPR, &empty);

  x1 = BOUNDS (GIMP_DRAWABLE(layer)->offset_x, 0, GIMP_DRAWABLE(mask)->width);
  y1 = BOUNDS (GIMP_DRAWABLE(layer)->offset_y, 0, GIMP_DRAWABLE(mask)->height);
  x2 = BOUNDS (GIMP_DRAWABLE(layer)->offset_x + GIMP_DRAWABLE(layer)->width, 0, GIMP_DRAWABLE(mask)->width);
  y2 = BOUNDS (GIMP_DRAWABLE(layer)->offset_y + GIMP_DRAWABLE(layer)->height, 0, GIMP_DRAWABLE(mask)->height);

  pixel_region_init (&srcPR, GIMP_DRAWABLE(layer)->tiles,
		     (x1 - GIMP_DRAWABLE(layer)->offset_x), (y1 - GIMP_DRAWABLE(layer)->offset_y),
		     (x2 - x1), (y2 - y1), FALSE);
  pixel_region_init (&destPR, GIMP_DRAWABLE(mask)->tiles, x1, y1, (x2 - x1), (y2 - y1), TRUE);
  extract_alpha_region (&srcPR, NULL, &destPR);

  mask->bounds_known = FALSE;
}




void
channel_load (Channel *mask, Channel *channel)
{
  PixelRegion srcPR, destPR;

  /*  push the current mask onto the undo stack  */
  channel_push_undo (mask);

  /*  copy the channel to the mask  */
  pixel_region_init (&srcPR, GIMP_DRAWABLE(channel)->tiles, 0, 0, GIMP_DRAWABLE(channel)->width, GIMP_DRAWABLE(channel)->height, FALSE);
  pixel_region_init (&destPR, GIMP_DRAWABLE(mask)->tiles, 0, 0, GIMP_DRAWABLE(channel)->width, GIMP_DRAWABLE(channel)->height, TRUE);
  copy_region (&srcPR, &destPR);

  mask->bounds_known = FALSE;
}

void
channel_invalidate_bounds (Channel *channel)
{
  channel->bounds_known = FALSE;
}
