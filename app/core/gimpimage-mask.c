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
#include "appenv.h"
#include "brushes.h"
#include "canvas.h"
#include "drawable.h"
#include "errors.h"
#include "floating_sel.h"
#include "gdisplay.h"
#include "gimage_mask.h"
#include "interface.h"
#include "layer.h"
#include "layers_dialog.h"
#include "paint_core_16.h"
#include "paint_funcs_area.h"
#include "palette.h"
#include "pixelarea.h"
#include "pixelrow.h"
#include "undo.h"

#include "layer_pvt.h"
#include "drawable_pvt.h"

#define FIXME
/*should find a better place for these--leftover from tiles*/
int tilesx, tilesy;

/*  feathering variables  */
double gimage_mask_feather_radius = 5;
int    gimage_mask_border_radius = 5;
int    gimage_mask_grow_pixels = 1;
int    gimage_mask_shrink_pixels = 1;
int    gimage_mask_stroking = FALSE;

/*  local functions  */
static void * gimage_mask_stroke_paint_func (PaintCore *, GimpDrawable *, int);

/*  functions  */
int
gimage_mask_boundary (gimage, segs_in, segs_out, num_segs_in, num_segs_out)
     GImage * gimage;
     BoundSeg **segs_in;
     BoundSeg **segs_out;
     int *num_segs_in;
     int *num_segs_out;
{
  Layer *layer;
  int x1, y1, x2, y2;

  if ((layer = gimage_floating_sel (gimage)))
    {
      /*  If there is a floating selection, then
       *  we need to do some slightly different boundaries.
       *  Instead of inside and outside boundaries being defined
       *  by the extents of the layer, the inside boundary (the one
       *  that actually marches and is black/white) is the boundary of
       *  the floating selection.  The outside boundary (doesn't move,
       *  is black/gray) is defined as the normal selection mask
       */

      /*  Find the selection mask boundary  */
      channel_boundary (gimage_get_mask (gimage),
			segs_in, segs_out,
			num_segs_in, num_segs_out,
			0, 0, 0, 0);

      /*  Find the floating selection boundary  */
      *segs_in = floating_sel_boundary (layer, num_segs_in);

      return TRUE;
    }
  /*  Otherwise, return the boundary...if a channel is active  */
  else if (drawable_channel (gimage_active_drawable (gimage)))
    {
      return channel_boundary (gimage_get_mask (gimage),
			       segs_in, segs_out,
			       num_segs_in, num_segs_out,
			       0, 0, gimage->width, gimage->height);
    }
  /*  if a layer is active, we return multiple boundaries based on the extents  */
  else if ((layer = gimage_get_active_layer (gimage)))
    {
      int off_x, off_y;
      drawable_offsets (GIMP_DRAWABLE(layer), &off_x, &off_y);
      x1 = BOUNDS (off_x, 0, gimage->width);
      y1 = BOUNDS (off_y, 0, gimage->height);
      x2 = BOUNDS (off_x + drawable_width (GIMP_DRAWABLE(layer)), 0, gimage->width);
      y2 = BOUNDS (off_y + drawable_height (GIMP_DRAWABLE(layer)), 0, gimage->height);

      return channel_boundary (gimage_get_mask (gimage),
			       segs_in, segs_out,
			       num_segs_in, num_segs_out,
			       x1, y1, x2, y2);
    }
  else
    {
      *segs_in = NULL;
      *segs_out = NULL;
      *num_segs_in = 0;
      *num_segs_out = 0;
      return FALSE;
    }
}


int
gimage_mask_bounds (gimage, x1, y1, x2, y2)
     GImage *gimage;
     int *x1, *y1, *x2, *y2;
{
  return channel_bounds (gimage_get_mask (gimage), x1, y1, x2, y2);
}


void
gimage_mask_invalidate (gimage)
     GImage *gimage;
{
  Layer *layer;
  Channel *mask;

  /*  Turn the current selection off  */
  gdisplays_selection_visibility (gimage->ID, SelectionOff);

  mask = gimage_get_mask (gimage);
  mask->boundary_known = FALSE;

  /*  If there is a floating selection, update it's area...
   *  we need to do this since this selection mask can act as an additional
   *  mask in the composition of the floating selection
   */
  layer = gimage_get_active_layer (gimage);
  if (layer && layer_is_floating_sel (layer))
    drawable_update (GIMP_DRAWABLE(layer), 0, 0, GIMP_DRAWABLE(layer)->width, GIMP_DRAWABLE(layer)->height);
}


int
gimage_mask_value (gimage, x, y)
     GImage * gimage;
     int x, y;
{
  return channel_value (gimage_get_mask (gimage), x, y);
}


int
gimage_mask_is_empty (gimage)
     GImage *gimage;
{
  /*  in order to allow stroking of selections, we need to pretend here
   *  that the selection mask is empty so that it doesn't mask the paint
   *  during the stroke operation.
   */
  if (gimage_mask_stroking)
    return TRUE;
  else
    return channel_is_empty (gimage_get_mask (gimage));
}


void
gimage_mask_translate (gimage, off_x, off_y)
     GImage *gimage;
     int off_x;
     int off_y;
{
  channel_translate (gimage_get_mask (gimage), off_x, off_y);
}


Canvas *
gimage_mask_extract (gimage, drawable, cut_gimage, keep_indexed)
     GImage * gimage;
     GimpDrawable *drawable;
     int cut_gimage;
     int keep_indexed;
{
  Canvas * tiles;
  Channel * sel_mask;
  PixelArea srcPR, destPR, maskPR;
  Tag d_tag, canvas_tag;
  int x1, y1, x2, y2;
  int off_x, off_y;
  int non_empty;
  COLOR16_NEW (bg_color, drawable_tag(drawable));
  COLOR16_INIT (bg_color);
  color16_background (&bg_color);

  if (!drawable) 
    return NULL;

  /*  If there are no bounds, then just extract the entire image
   *  This may not be the correct behavior, but after getting rid
   *  of floating selections, it's still tempting to "cut" or "copy"
   *  a small layer and expect it to work, even though there is no
   *  actual selection mask
   */
  non_empty = drawable_mask_bounds (drawable, &x1, &y1, &x2, &y2);
  if (non_empty && (!(x2 - x1) || !(y2 - y1)))
    {
      g_message ("Unable to cut/copy because the selected\nregion is empty.");
      return NULL;
    }

  d_tag = drawable_tag (drawable);
  canvas_tag = tag_set_alpha (d_tag, ALPHA_YES);
  
  if (tag_format (canvas_tag) == FORMAT_INDEXED)
    {
      if (!keep_indexed)
	canvas_tag = tag_set_format (canvas_tag , FORMAT_RGB); 
    }  

  /*  get the selection mask */
  if (non_empty)
    sel_mask = gimage_get_mask (gimage);
  else
    sel_mask = NULL;

  /*  If a cut was specified, and the selection mask is not empty, push an undo  */
  if (cut_gimage && non_empty)
    drawable_apply_image (drawable, x1, y1, x2, y2, NULL);

  drawable_offsets (drawable, &off_x, &off_y);

  /*  Allocate the temp buffer  */
  tiles = canvas_new (canvas_tag, (x2 - x1), (y2 - y1), STORAGE_FLAT);
     
  tilesx = x1 + off_x;
  tilesy = y1 + off_y;

  /* configure the pixel areas  */
  pixelarea_init (&srcPR, drawable_data (drawable), 
		x1, y1, (x2 - x1), (y2 - y1), cut_gimage);
  pixelarea_init (&destPR, tiles, 
		0, 0, (x2 - x1), (y2 - y1), TRUE);

  /*  If there is a selection, extract from it  */
  if (non_empty)
    {
      pixelarea_init (&maskPR, GIMP_DRAWABLE(sel_mask)->tiles, 
		(x1 + off_x), (y1 + off_y), (x2 - x1), (y2 - y1), FALSE);

      extract_from_area (&srcPR, &destPR, &maskPR, &bg_color, 
		drawable_cmap (drawable), cut_gimage);

      if (cut_gimage)
	{
	  /*  Clear the region  */
	  channel_clear (gimage_get_mask (gimage));

	  /*  Update the region  */
	  gdisplays_update_area (gimage->ID, tilesx, tilesy,
				 canvas_width (tiles), canvas_height (tiles));

	  /*  Invalidate the preview  */
	  drawable_invalidate_preview (drawable);
	}
    }
  /*  Otherwise, get the entire active layer  */
  else
    {
      /*  If the layer is indexed...we need to extract pixels  */
      if ( tag_format (d_tag)== FORMAT_INDEXED && !keep_indexed)
        extract_from_area (&srcPR, &destPR, &maskPR, &bg_color, 
		drawable_cmap (drawable), cut_gimage);
      else
	copy_area (&srcPR, &destPR); /* should work even if dest has alpha & src doesnt */
      
      /*  If we're cutting, remove either the layer (or floating selection),
       *  the layer mask, or the channel
       */
      if (cut_gimage && drawable_layer (drawable))
	{
	  if (layer_is_floating_sel (drawable_layer (drawable)))
	    floating_sel_remove (drawable_layer (drawable));
	  else
	    gimage_remove_layer (gimage, GIMP_LAYER (drawable));
	}
      else if (cut_gimage && drawable_layer_mask (drawable))
	{
	  gimage_remove_layer_mask (gimage, GIMP_LAYER_MASK(drawable)->layer, DISCARD);
	}
      else if (cut_gimage && drawable_channel (drawable))
	gimage_remove_channel (gimage, GIMP_CHANNEL(drawable));
    }

  return tiles;
}

Layer *
gimage_mask_float (gimage, drawable, off_x, off_y)
     GImage * gimage;
     GimpDrawable* drawable;
     int off_x, off_y;  /*  optional offset  */
{
  Layer *layer;
  Channel *mask = gimage_get_mask (gimage);
  Canvas* tiles;
  int non_empty;
  int x1, y1, x2, y2;

  /*  Make sure there is a region to float...  */
  non_empty = drawable_mask_bounds ( (drawable), &x1, &y1, &x2, &y2);
  if (! non_empty || (x2 - x1) == 0 || (y2 - y1) == 0)
    {
      g_message ("Float Selection: No selection to float.");
      return NULL;
    }

  /*  Start an undo group  */
  undo_push_group_start (gimage, FLOAT_MASK_UNDO);

  /*  Cut the selected region  */
  tiles = gimage_mask_extract (gimage, drawable, TRUE, FALSE);

  /*  Create a new layer from the buffer  */
  layer = layer_from_tiles (gimage, drawable, tiles, "Floated Layer", OPAQUE_OPACITY, NORMAL);

  /*  Set the offsets  */
  GIMP_DRAWABLE(layer)->offset_x = tilesx + off_x;
  GIMP_DRAWABLE(layer)->offset_y = tilesy + off_y;

  /*  Free the temp buffer  */
  canvas_delete (tiles);

  /*  Add the floating layer to the gimage  */
  floating_sel_attach (layer, drawable);

  /*  End an undo group  */
  undo_push_group_end (gimage);

  /*  invalidate the gimage's boundary variables  */
  mask->boundary_known = FALSE;

  return layer;
}


void
gimage_mask_clear (gimage)
     GImage * gimage;
{
  channel_clear (gimage_get_mask (gimage));
}


void
gimage_mask_undo (gimage)
     GImage *gimage;
{
  channel_push_undo (gimage_get_mask (gimage));
}


void
gimage_mask_invert (gimage)
     GImage * gimage;
{
  channel_invert (gimage_get_mask (gimage));
}


void
gimage_mask_sharpen (gimage)
     GImage * gimage;
{
  /*  No need to play with the selection visibility
   *  because sharpen will not change the outline
   */
  channel_sharpen (gimage_get_mask (gimage));
}


void
gimage_mask_all (gimage)
     GImage * gimage;
{
  channel_all (gimage_get_mask (gimage));
}


void
gimage_mask_none (gimage)
     GImage * gimage;
{
  channel_clear (gimage_get_mask (gimage));
}


void
gimage_mask_feather (gimage, feather_radius)
     GImage * gimage;
     double feather_radius;
{
  gimage_mask_feather_radius = feather_radius;

  /*  push the current mask onto the undo stack--need to do this here because
   *  channel_feather doesn't do it
   */
  channel_push_undo (gimage_get_mask (gimage));

  /*  feather the region  */
  channel_feather (gimage_get_mask (gimage),
		   gimage_get_mask (gimage),
		   gimage_mask_feather_radius, REPLACE, 0, 0);
}


void
gimage_mask_border (gimage, border_radius)
     GImage * gimage;
     int border_radius;
{
  gimage_mask_border_radius = border_radius;

  /*  feather the region  */
  channel_border (gimage_get_mask (gimage),
		  gimage_mask_border_radius);
}


void
gimage_mask_grow (gimage, grow_pixels)
     GImage * gimage;
     int grow_pixels;
{
  gimage_mask_grow_pixels = grow_pixels;

  /*  feather the region  */
  channel_grow (gimage_get_mask (gimage),
		gimage_mask_grow_pixels);
}


void
gimage_mask_shrink (gimage, shrink_pixels)
     GImage * gimage;
     int shrink_pixels;
{
  gimage_mask_shrink_pixels = shrink_pixels;
  /*  feather the region  */
  channel_shrink (gimage_get_mask (gimage),
		  gimage_mask_shrink_pixels);
}


void
gimage_mask_layer_alpha (gimage, layer)
     GImage *gimage;
     Layer *layer;
{
  /*  extract the layer's alpha channel  */
  if (drawable_has_alpha (GIMP_DRAWABLE (layer)))
    {
      /*  load the mask with the given layer's alpha channel  */
      channel_layer_alpha (gimage_get_mask (gimage), layer);
    }
  else
    {
      g_message ("The active layer has no alpha channel\nto convert to a selection.");
      return;
    }
}


void
gimage_mask_layer_mask (gimage, layer)
     GImage *gimage;
     Layer *layer;
{
  /*  extract the layer's alpha channel  */
  if (layer->mask)
    {
      /*  load the mask with the given layer's alpha channel  */
      channel_layer_mask (gimage_get_mask (gimage), layer);
    }
  else
    {
      g_message ("The active layer has no mask\nto convert to a selection.");
      return;
    }
}


void
gimage_mask_load (gimage, channel)
     GImage *gimage;
     Channel *channel;
{
  /*  Load the specified channel to the gimage mask  */
  channel_load (gimage_get_mask (gimage), (channel));
}


Channel *
gimage_mask_save (gimage)
     GImage *gimage;
{
  Channel *new_channel;

  new_channel = channel_copy (gimage_get_mask (gimage));

  /*  saved selections are not visible by default  */
  GIMP_DRAWABLE(new_channel)->visible = FALSE;
  gimage_add_channel (gimage, new_channel, -1);

  return new_channel;
}


int
gimage_mask_stroke (gimage, drawable)
     GImage *gimage;
     GimpDrawable *drawable;
{
  BoundSeg *bs_in;
  BoundSeg *bs_out;
  int num_segs_in;
  int num_segs_out;
  BoundSeg *stroke_segs;
  int num_strokes;
  int seg;
  int offx, offy;
  int i;

  if (! gimage_mask_boundary (gimage, &bs_in, &bs_out, &num_segs_in, &num_segs_out))
    {
      g_message ("No selection to stroke!");
      return FALSE;
    }

  stroke_segs = sort_boundary (bs_in, num_segs_in, &num_strokes);
  if (num_strokes == 0)
    return TRUE;

  /*  find the drawable offsets  */
  drawable_offsets (drawable, &offx, &offy);

  /*  init the paint core  */
  if (! paint_core_init (&non_gui_paint_core, drawable,
			 (stroke_segs[0].x1 - offx),
			 (stroke_segs[0].y1 - offy)))
    return FALSE;

  /*  set the paint core's paint func  */
  non_gui_paint_core.paint_func = gimage_mask_stroke_paint_func;
  gimage_mask_stroking = TRUE;

  non_gui_paint_core.startx = non_gui_paint_core.lastx = (stroke_segs[0].x1 - offx);
  non_gui_paint_core.starty = non_gui_paint_core.lasty = (stroke_segs[0].y1 - offy);

  seg = 0;
  for (i = 0; i < num_strokes; i++)
    {
      while (stroke_segs[seg].x2 != -1)
	{
	  non_gui_paint_core.curx = (stroke_segs[seg].x2 - offx);
	  non_gui_paint_core.cury = (stroke_segs[seg].y2 - offy);

	  paint_core_interpolate (&non_gui_paint_core, drawable);

	  non_gui_paint_core.lastx = non_gui_paint_core.curx;
	  non_gui_paint_core.lasty = non_gui_paint_core.cury;

	  seg ++;
	}

      seg ++;
      non_gui_paint_core.startx = non_gui_paint_core.lastx = (stroke_segs[seg].x1 - offx);
      non_gui_paint_core.starty = non_gui_paint_core.lasty = (stroke_segs[seg].y1 - offy);
    }

  /*  finish the painting  */
  paint_core_finish (&non_gui_paint_core, drawable, -1);

  /*  cleanup  */
  gimage_mask_stroking = FALSE;
  paint_core_cleanup ();
  g_free (stroke_segs);

  return TRUE;
}

static void *
gimage_mask_stroke_paint_func (paint_core, drawable, state)
     PaintCore *paint_core;
     GimpDrawable *drawable;
     int state;
{
  Canvas * painthit;
  PixelArea a;
      
  /* Get the working canvas */
  painthit = paint_core_16_area (paint_core, drawable);
  pixelarea_init (&a, painthit, 0, 0, 0, 0, TRUE);

  /* construct the paint hit */
  {
    COLOR16_NEW (paint, canvas_tag (painthit));
    
    COLOR16_INIT (paint);
    color16_foreground (&paint);
    color_area (&a, &paint);
  }
      
  /* apply it to the image */
  paint_core_16_area_paste (paint_core, drawable,
                            1.0,
                            (gfloat) get_brush_opacity (),
                            SOFT,
                            CONSTANT,
                            get_brush_paint_mode ());

  return NULL;
}
