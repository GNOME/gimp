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
#include "canvas.h"
#include "channel.h"
#include "drawable.h"
#include "errors.h"
#include "gimage_mask.h"
#include "layer.h"
#include "paint_funcs_area.h"
#include "palette.h"
#include "pixelarea.h"
#include "pixelrow.h"
#include "undo.h"

#include "channel_pvt.h"

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

guint
gimp_channel_get_type ()
{
  static guint channel_type = 0;

  if (!channel_type)
    {
      GtkTypeInfo channel_info =
      {
	"GimpChannel",
	sizeof (GimpChannel),
	sizeof (GimpChannelClass),
	(GtkClassInitFunc) gimp_channel_class_init,
	(GtkObjectInitFunc) gimp_channel_init,
	(GtkArgSetFunc) NULL,
	(GtkArgGetFunc) NULL,
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

#if 0
static void
channel_validate (TileManager *tm, Tile *tile, int level)
{
  /*  Set the contents of the tile to empty  */
  memset (tile->data, TRANSPARENT_OPACITY, tile->ewidth * tile->eheight);
}
#endif



Channel *
channel_new (int gimage_ID, int width, int height, char *name, int opacity,
	     unsigned char *col)
{
  Tag t = tag_new (PRECISION_U8, FORMAT_GRAY, ALPHA_NONE);
  return channel_new_tag (gimage_ID, width, height, t, name, opacity, col);
}

Channel *
channel_new_tag (int gimage_ID, int width, int height, Tag tag, char *name, int opacity,
	     unsigned char *col)
{
  Channel * channel;
  int i;

  channel = gtk_type_new (gimp_channel_get_type ());

  gimp_drawable_configure_tag (GIMP_DRAWABLE(channel), 
			   gimage_ID, width, height, tag, STORAGE_TILED, name);

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
  PixelArea src_area, dest_area;

  /*  formulate the new channel name  */
  channel_name = (char *) g_malloc (strlen (GIMP_DRAWABLE(channel)->name) + 6);
  sprintf (channel_name, "%s copy", GIMP_DRAWABLE(channel)->name);

  /*  allocate a new channel object  */
  new_channel = channel_new_tag (GIMP_DRAWABLE(channel)->gimage_ID, 
                                 GIMP_DRAWABLE(channel)->width, 
                                 GIMP_DRAWABLE(channel)->height,
                                 drawable_tag (GIMP_DRAWABLE(channel)), 
                                 channel_name, channel->opacity, channel->col);
  GIMP_DRAWABLE(new_channel)->visible = GIMP_DRAWABLE(channel)->visible;
  new_channel->show_masked = channel->show_masked;

  /*  copy the contents across channels  */
  pixelarea_init (&src_area, GIMP_DRAWABLE(channel)->canvas,
                  0, 0, 
                  GIMP_DRAWABLE(channel)->width, GIMP_DRAWABLE(channel)->height,
                  FALSE);
  pixelarea_init (&dest_area, GIMP_DRAWABLE(new_channel)->canvas, 
                  0, 0,
                  GIMP_DRAWABLE(channel)->width, GIMP_DRAWABLE(channel)->height,
                  TRUE);
  copy_area (&src_area, &dest_area);

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
  PixelArea src_area, dest_area;
  Canvas *new_canvas;
  if (new_width == 0 || new_height == 0)
    return;

  /*  Update the old channel position  */
  drawable_update (GIMP_DRAWABLE(channel), 0, 0, GIMP_DRAWABLE(channel)->width, GIMP_DRAWABLE(channel)->height);

  /*  Configure the pixel areas  */
  pixelarea_init (&src_area, GIMP_DRAWABLE(channel)->canvas, 
	0, 0, GIMP_DRAWABLE(channel)->width, GIMP_DRAWABLE(channel)->height, FALSE);

  /*  Allocate the new channel, configure dest region  */
  new_canvas = canvas_new (drawable_tag (GIMP_DRAWABLE(channel)), 
		new_width, new_height, STORAGE_TILED);
  pixelarea_init (&dest_area, new_canvas, 
		0, 0, new_width, new_height, TRUE);

  /*  Scale the area  */
  scale_area (&src_area, &dest_area);

  /*  Push the channel on the undo stack  */
  undo_push_channel_mod (gimage_get_ID (GIMP_DRAWABLE(channel)->gimage_ID), channel);

  /*  Configure the new channel  */
  GIMP_DRAWABLE(channel)->canvas = new_canvas;
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
  PixelArea src_area, dest_area;
  Canvas *new_canvas;
  int clear;
  int w, h;
  int x1, y1, x2, y2;
  COLOR16_NEW (bg_color, drawable_tag(GIMP_DRAWABLE(channel)) );
  COLOR16_INIT (bg_color);
  color16_black (&bg_color);

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
  pixelarea_init (&src_area, GIMP_DRAWABLE(channel)->canvas, 
	x1, y1, w, h, FALSE);

  /*  Determine whether the new channel needs to be initially cleared  */
  if ((new_width > GIMP_DRAWABLE(channel)->width) ||
      (new_height > GIMP_DRAWABLE(channel)->height) ||
      (x2 || y2))
    clear = TRUE;
  else
    clear = FALSE;

  /*  Allocate the new channel, configure dest region  */
  new_canvas = canvas_new (drawable_tag (GIMP_DRAWABLE(channel)), 
	new_width, new_height, STORAGE_TILED);

  /*  Set to black (empty--for selections)  */
  if (clear)
    {
      pixelarea_init (&dest_area, new_canvas, 
	0, 0, new_width, new_height, TRUE);
      color_area (&dest_area, &bg_color);
    }

  /*  copy from the old to the new  */
  pixelarea_init (&dest_area, new_canvas, x2, y2, w, h, TRUE);
  if (w && h)
    copy_area (&src_area, &dest_area);

  /*  Push the channel on the undo stack  */
  undo_push_channel_mod (gimage_get_ID (GIMP_DRAWABLE(channel)->gimage_ID), channel);

  /*  Configure the new channel  */
  GIMP_DRAWABLE(channel)->canvas = new_canvas;
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
#if 1
  printf ("channel_preview not implemented\n");
  return NULL;
#else
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
#endif
}


void
channel_invalidate_previews (int gimage_id)
{
  GSList * tmp;
  Channel * channel;
  GImage * gimage;

  if (! (gimage = gimage_get_ID (gimage_id)))
    return;

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
channel_new_mask (int gimage_ID, int width, int height)
{
  unsigned char black[3] = {0, 0, 0};
  Channel *new_channel;

  /*  Create the new channel  */
  new_channel = channel_new (gimage_ID, width, height, "Selection Mask", 127, black);

  /*  Set the validate procedure  */
  /*tile_manager_set_validate_proc (GIMP_DRAWABLE(new_channel)->tiles, channel_validate);*/

  return new_channel;
}

Channel *
channel_new_mask_tag (int gimage_ID, int width, int height, Tag tag)
{
  unsigned char black[3] = {0, 0, 0};
  Channel *new_channel;

  /*  Create the new channel  */
  new_channel = channel_new_tag (gimage_ID, width, height, tag, "Selection Mask", 127, black);

  /*  Set the validate procedure  */
  /*tile_manager_set_validate_proc (GIMP_DRAWABLE(new_channel)->tiles, channel_validate);*/

  return new_channel;
}


int
channel_boundary (Channel *mask, BoundSeg **segs_in, BoundSeg **segs_out,
		  int *num_segs_in, int *num_segs_out,
		  int x1, int y1, int x2, int y2)
{
  int x3, y3, x4, y4;
  PixelArea b_area;

  if (! mask->boundary_known)
    {
      /* free the out of date boundary segments */
      if (mask->segs_in)
	g_free (mask->segs_in);
      if (mask->segs_out)
	g_free (mask->segs_out);
      
      if (channel_bounds (mask, &x3, &y3, &x4, &y4))
        {
          pixelarea_init (&b_area, GIMP_DRAWABLE(mask)->canvas, 
                          x3, y3, (x4 - x3), (y4 - y3), FALSE);
	  mask->segs_out = find_mask_boundary (&b_area, &mask->num_segs_out,
					       IgnoreBounds,
					       x1, y1,
					       x2, y2);
	  x1 = MAXIMUM (x1, x3);
	  y1 = MAXIMUM (y1, y3);
	  x2 = MINIMUM (x2, x4);
	  y2 = MINIMUM (y2, y4);

	  if (x2 > x1 && y2 > y1)
	    {
	      pixelarea_init (&b_area, GIMP_DRAWABLE(mask)->canvas, 
		0, 0, GIMP_DRAWABLE(mask)->width, GIMP_DRAWABLE(mask)->height, FALSE);
	      mask->segs_in =  find_mask_boundary (&b_area, &mask->num_segs_in,
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
  Canvas *canvas = GIMP_DRAWABLE(mask)->canvas;
  int val;
  guchar *data;

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
  canvas_portion_refro (canvas, x, y);
  data = canvas_portion_data (canvas, x, y);
#define FIXME
  /* this is 8 bit only */
  val = *data;
  canvas_portion_unref (canvas, x, y);
  return val;
}


int
channel_bounds (Channel *mask, int *x1, int *y1, int *x2, int *y2)
{
  PixelArea maskPR;
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

  pixelarea_init (&maskPR, GIMP_DRAWABLE(mask)->canvas, 0, 0, 
		GIMP_DRAWABLE(mask)->width, GIMP_DRAWABLE(mask)->height, FALSE);
  for (pr = pixelarea_register (1, &maskPR); 
	pr != NULL; 
	pr = pixelarea_process (pr))
    {
      data = pixelarea_data (&maskPR);
      ex = pixelarea_x (&maskPR) + pixelarea_width (&maskPR);
      ey = pixelarea_y (&maskPR) + pixelarea_height (&maskPR);

      for (y = pixelarea_y (&maskPR); y < ey; y++)
	{
	  found = FALSE;
	  for (x = pixelarea_x (&maskPR); x < ex; x++, data++)
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
  PixelArea maskPR;
  unsigned char * data;
  int x, y;
  void * pr;

  if (mask->bounds_known)
    return mask->empty;

  pixelarea_init (&maskPR, GIMP_DRAWABLE(mask)->canvas, 
		0, 0, GIMP_DRAWABLE(mask)->width, GIMP_DRAWABLE(mask)->height, FALSE);
  for (pr = pixelarea_register (1, &maskPR); 
	pr != NULL; 
	pr = pixelarea_process (pr))
    {
      /*  check if any pixel in the mask is non-zero  */
      data = pixelarea_data (&maskPR);
      for (y = 0; y < pixelarea_height (&maskPR); y++)
	for (x = 0; x < pixelarea_width (&maskPR); x++)
	  if (*data++)
	    {
	      pixelarea_process_stop (pr);
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
  PixelArea maskPR;
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

  pixelarea_init (&maskPR, GIMP_DRAWABLE(mask)->canvas, x, y, width, 1, TRUE);
  for (pr = pixelarea_register (1, &maskPR); 
	pr != NULL; 
	pr = pixelarea_process (pr))
    {
      data = pixelarea_data (&maskPR);
      width = pixelarea_width (&maskPR);
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
  PixelArea maskPR;
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

  pixelarea_init (&maskPR, GIMP_DRAWABLE(mask)->canvas, x, y, width, 1, TRUE);
  for (pr = pixelarea_register (1, &maskPR); 
	pr != NULL; 
	pr = pixelarea_process (pr))
    {
      data = pixelarea_data (&maskPR);
      width = pixelarea_width (&maskPR);
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
  PixelArea maskPR;
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

  pixelarea_init (&maskPR, GIMP_DRAWABLE(mask)->canvas,
	x, y, width, 1, TRUE);
  for (pr = pixelarea_register (1, &maskPR); 
        pr != NULL; 
	pr = pixelarea_process (pr))
    {
      data = pixelarea_data (&maskPR);
      width = pixelarea_width (&maskPR);
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
  PixelArea srcPR, destPR;
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

  pixelarea_init (&srcPR, GIMP_DRAWABLE(add_on)->canvas,
			(x1 - off_x), (y1 - off_y), w, h, FALSE);
  pixelarea_init (&destPR, GIMP_DRAWABLE(mask)->canvas, 
			x1, y1, w, h, TRUE);

  for (pr = pixelarea_register (2, &srcPR, &destPR); 
	pr != NULL; 
	pr = pixelarea_process (pr))
    {
      src = pixelarea_data (&srcPR);
      dest = pixelarea_data (&destPR);

      for (y = 0; y < pixelarea_height (&srcPR); y++)
	{
	  for (x = 0; x < pixelarea_width (&srcPR); x++)
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
	  src += pixelarea_rowstride (&srcPR);
	  dest += pixelarea_rowstride (&destPR);
	}
    }

  mask->bounds_known = FALSE;
}


void
channel_feather (Channel *input, Channel *output, double radius,
		 int op, int off_x, int off_y)
{
  int x1, y1, x2, y2;
  PixelArea srcPR;

  x1 = BOUNDS (off_x, 0, GIMP_DRAWABLE(output)->width);
  y1 = BOUNDS (off_y, 0, GIMP_DRAWABLE(output)->height);
  x2 = BOUNDS (off_x + GIMP_DRAWABLE(input)->width, 0, GIMP_DRAWABLE(output)->width);
  y2 = BOUNDS (off_y + GIMP_DRAWABLE(input)->height, 0, GIMP_DRAWABLE(output)->height);

  pixelarea_init (&srcPR, GIMP_DRAWABLE(input)->canvas, 
		(x1 - off_x), (y1 - off_y), (x2 - x1), (y2 - y1), FALSE);
  gaussian_blur_area (&srcPR, radius);

  if (input != output) 
    channel_combine_mask(output, input, op, 0, 0);

  output->bounds_known = FALSE;
}


void
channel_push_undo (Channel *mask)
{
  int x1, y1, x2, y2;
  MaskUndo *mask_undo;
  Canvas *undo_canvas;
  PixelArea srcPR, destPR;
  GImage *gimage;

  mask_undo = (MaskUndo *) g_malloc (sizeof (MaskUndo));
  if (channel_bounds (mask, &x1, &y1, &x2, &y2))
    {
      undo_canvas = canvas_new (drawable_tag(GIMP_DRAWABLE(mask)),
			x2 - x1, y2 - y1, STORAGE_FLAT);
      pixelarea_init (&srcPR, GIMP_DRAWABLE(mask)->canvas,
		x1, y1, (x2 - x1), (y2 - y1), FALSE);
      pixelarea_init (&destPR, undo_canvas, 
		0, 0, (x2 - x1), (y2 - y1), TRUE);
      copy_area (&srcPR, &destPR);
    }
  else
    undo_canvas = NULL;

  mask_undo->tiles = undo_canvas;
  mask_undo->x = x1;
  mask_undo->y = y1;

  /* push the undo buffer onto the undo stack */
  gimage = gimage_get_ID (GIMP_DRAWABLE(mask)->gimage_ID);
  undo_push_mask (gimage, mask_undo);
  gimage_mask_invalidate (gimage);

  /*  invalidate the preview  */
  GIMP_DRAWABLE(mask)->preview_valid = 0;
}


void
channel_clear (Channel *mask)
{
  PixelArea mask_area;
  COLOR16_NEW (bg_color, drawable_tag(GIMP_DRAWABLE(mask)) );
  COLOR16_INIT (bg_color);
  color16_black (&bg_color);

  /*  push the current channel onto the undo stack  */
  channel_push_undo (mask);

  if (mask->bounds_known && !mask->empty)
    {
      pixelarea_init (&mask_area, GIMP_DRAWABLE(mask)->canvas, mask->x1, mask->y1,
			 (mask->x2 - mask->x1), (mask->y2 - mask->y1), TRUE);
      color_area (&mask_area, &bg_color);
    }
  else
    {
      /*  clear the mask  */
      pixelarea_init (&mask_area, GIMP_DRAWABLE(mask)->canvas, 
	0, 0, GIMP_DRAWABLE(mask)->width, GIMP_DRAWABLE(mask)->height, TRUE);
      color_area (&mask_area, &bg_color);
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
  PixelArea maskPR;
  unsigned char *data;
  int size;
  void * pr;

  /*  push the current channel onto the undo stack  */
  channel_push_undo (mask);

  pixelarea_init (&maskPR, GIMP_DRAWABLE(mask)->canvas, 0, 0, 
		GIMP_DRAWABLE(mask)->width, GIMP_DRAWABLE(mask)->height, TRUE);
  for (pr = pixelarea_register (1, &maskPR); 
	pr != NULL; 
	pr = pixelarea_process (pr))
    {
      /*  subtract each pixel in the mask from 255  */
      data = pixelarea_data (&maskPR);
      size = pixelarea_width (&maskPR) * pixelarea_height (&maskPR);
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
  PixelArea maskPR;
  unsigned char *data;
  int size;
  void * pr;

  /*  push the current channel onto the undo stack  */
  channel_push_undo (mask);

  pixelarea_init (&maskPR, GIMP_DRAWABLE(mask)->canvas, 
			0, 0, GIMP_DRAWABLE(mask)->width, GIMP_DRAWABLE(mask)->height, TRUE);
  for (pr = pixelarea_register (1, &maskPR); 
	pr != NULL; 
	pr = pixelarea_process (pr))
    {
      /*  if a pixel in the mask has a non-zero value, make it 255  */
      data = pixelarea_data (&maskPR);
      size = pixelarea_width (&maskPR) * pixelarea_height (&maskPR);
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
  PixelArea maskPR;
  COLOR16_NEW (bg_color, drawable_tag(GIMP_DRAWABLE(mask)) );
  COLOR16_INIT (bg_color);
  color16_white (&bg_color);

  /*  push the current channel onto the undo stack  */
  channel_push_undo (mask);

  /*  clear the mask  */
  pixelarea_init (&maskPR, GIMP_DRAWABLE(mask)->canvas, 
			0, 0, GIMP_DRAWABLE(mask)->width, GIMP_DRAWABLE(mask)->height, TRUE);
  color_area (&maskPR, &bg_color);

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
  PixelArea bPR;
  int x1, y1, x2, y2;
  BoundSeg * bs;
  int num_segs;
  COLOR16_NEW (bg_color, drawable_tag(GIMP_DRAWABLE(mask)) );
  COLOR16_INIT (bg_color);
  color16_black (&bg_color);

  if (! channel_bounds (mask, &x1, &y1, &x2, &y2))
    return;

  /*  push the current channel onto the undo stack  */
  channel_push_undo (mask);

  pixelarea_init (&bPR, GIMP_DRAWABLE(mask)->canvas, 
		x1, y1, (x2 - x1), (y2 - y1), FALSE);
  bs =  find_mask_boundary (&bPR, &num_segs, WithinBounds, x1, y1, x2, y2);

  /*  clear the channel  */
  if (mask->bounds_known && !mask->empty)
    {
      pixelarea_init (&bPR, GIMP_DRAWABLE(mask)->canvas,
			 mask->x1, mask->y1,
			 (mask->x2 - mask->x1), (mask->y2 - mask->y1), TRUE);
      color_area (&bPR, &bg_color);
    }
  else
    {
      /*  clear the mask  */
      pixelarea_init (&bPR, GIMP_DRAWABLE(mask)->canvas, 
		0, 0, GIMP_DRAWABLE(mask)->width, GIMP_DRAWABLE(mask)->height, TRUE);
      color_area (&bPR, &bg_color);
    }

  /*  calculate a border of specified radius based on the boundary segments  */
  pixelarea_init (&bPR, GIMP_DRAWABLE(mask)->canvas, 
	0, 0, GIMP_DRAWABLE(mask)->width, GIMP_DRAWABLE(mask)->height, TRUE);
  border_area (&bPR, bs, num_segs, radius);

  g_free (bs);

  mask->bounds_known = FALSE;
}


void
channel_grow (Channel *mask, int steps)
{
  PixelArea bPR;

  if (channel_is_empty (mask))
    return;

  /*  push the current channel onto the undo stack  */
  channel_push_undo (mask);

  /*  need full extents for grow  */
  pixelarea_init (&bPR, GIMP_DRAWABLE(mask)->canvas, 
	0, 0, GIMP_DRAWABLE(mask)->width, GIMP_DRAWABLE(mask)->height, TRUE);

  while (steps--)
    thin_area (&bPR, GROW_REGION);

  mask->bounds_known = FALSE;
}


void
channel_shrink (Channel *mask, int steps)
{
  PixelArea bPR;
  int x1, y1, x2, y2;

  if (! channel_bounds (mask, &x1, &y1, &x2, &y2))
    return;

  /*  push the current channel onto the undo stack  */
  channel_push_undo (mask);

  pixelarea_init (&bPR, GIMP_DRAWABLE(mask)->canvas, 
		x1, y1, (x2 - x1), (y2 - y1), TRUE);

  while (steps--)
    thin_area (&bPR, SHRINK_REGION);

  mask->bounds_known = FALSE;
}


void
channel_translate (Channel *mask, int off_x, int off_y)
{
  int width, height;
  Channel *tmp_mask;
  PixelArea srcPR, destPR;
  int x1, y1, x2, y2;
  COLOR16_NEW (empty_color, drawable_tag(GIMP_DRAWABLE(mask)));
  COLOR16_INIT (empty_color);
  color16_black (&empty_color);

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
      tmp_mask = channel_new_mask_tag (GIMP_DRAWABLE(mask)->gimage_ID, 
					width, height, drawable_tag (GIMP_DRAWABLE(mask)));

      pixelarea_init (&srcPR, GIMP_DRAWABLE(mask)->canvas, 
	x1 - off_x, y1 - off_y, width, height, FALSE);
      pixelarea_init (&destPR, GIMP_DRAWABLE(tmp_mask)->canvas, 
	0, 0, width, height, TRUE);
      copy_area (&srcPR, &destPR);
    }

  /*  clear the mask  */
  pixelarea_init (&srcPR, GIMP_DRAWABLE(mask)->canvas, 
	0, 0, GIMP_DRAWABLE(mask)->width, GIMP_DRAWABLE(mask)->height, TRUE);
  color_area (&srcPR, &empty_color);

  if (width != 0 && height != 0)
    {
      /*  copy the temp mask back to the mask  */
      pixelarea_init (&srcPR, GIMP_DRAWABLE(tmp_mask)->canvas, 
		0, 0, width, height, FALSE);
      pixelarea_init (&destPR, GIMP_DRAWABLE(mask)->canvas, 
		x1, y1, width, height, TRUE);
      copy_area (&srcPR, &destPR);

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
  PixelArea srcPR, destPR;
  int x1, y1, x2, y2;
  COLOR16_NEW (empty_color, drawable_tag(GIMP_DRAWABLE(mask)));
  COLOR16_INIT (empty_color);
  color16_black (&empty_color);

  /*  push the current mask onto the undo stack  */
  channel_push_undo (mask);

  /*  clear the mask  */
  pixelarea_init (&destPR, GIMP_DRAWABLE(mask)->canvas, 
	0, 0, GIMP_DRAWABLE(mask)->width, GIMP_DRAWABLE(mask)->height, TRUE);
  color_area (&destPR, &empty_color);

  x1 = BOUNDS (GIMP_DRAWABLE(layer)->offset_x, 0, GIMP_DRAWABLE(mask)->width);
  y1 = BOUNDS (GIMP_DRAWABLE(layer)->offset_y, 0, GIMP_DRAWABLE(mask)->height);
  x2 = BOUNDS (GIMP_DRAWABLE(layer)->offset_x + GIMP_DRAWABLE(layer)->width, 0, GIMP_DRAWABLE(mask)->width);
  y2 = BOUNDS (GIMP_DRAWABLE(layer)->offset_y + GIMP_DRAWABLE(layer)->height, 0, GIMP_DRAWABLE(mask)->height);

  pixelarea_init (&srcPR, GIMP_DRAWABLE(layer)->canvas,
		     (x1 - GIMP_DRAWABLE(layer)->offset_x), (y1 - GIMP_DRAWABLE(layer)->offset_y),
		     (x2 - x1), (y2 - y1), FALSE);
  pixelarea_init (&destPR, GIMP_DRAWABLE(mask)->canvas,
			x1, y1, (x2 - x1), (y2 - y1), TRUE);
  extract_alpha_area(&srcPR, NULL, &destPR);

  mask->bounds_known = FALSE;
}


void
channel_load (Channel *mask, Channel *channel)
{
  PixelArea srcPR, destPR;



  /*  push the current mask onto the undo stack  */
  channel_push_undo (mask);

  /*  copy the channel to the mask  */
  pixelarea_init (&srcPR, GIMP_DRAWABLE(channel)->canvas,
		0, 0, GIMP_DRAWABLE(channel)->width, GIMP_DRAWABLE(channel)->height, FALSE);
  pixelarea_init (&destPR, GIMP_DRAWABLE(mask)->canvas, 
			0, 0, GIMP_DRAWABLE(channel)->width, GIMP_DRAWABLE(channel)->height, TRUE);
  copy_area (&srcPR, &destPR);

  mask->bounds_known = FALSE;
}

void
channel_invalidate_bounds (Channel *channel)
{
  channel->bounds_known = FALSE;
}
