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

#include "appenv.h"
#include "drawable.h"
#include "floating_sel.h"
#include "gdisplay.h"
#include "gimage_mask.h"
#include "gimprc.h"
#include "layer.h"
#include "paint_core.h"
#include "paint_options.h"
#include "undo.h"

#include "channel_pvt.h"
#include "tile_manager_pvt.h"

#include "libgimp/gimpintl.h"


/*  local variables  */
static int gimage_mask_stroking = FALSE;

/*  functions  */
gboolean
gimage_mask_boundary (GImage    *gimage,
		      BoundSeg **segs_in,
		      BoundSeg **segs_out,
		      gint      *num_segs_in,
		      gint      *num_segs_out)
{
  GimpDrawable *d;
  Layer *layer;
  gint x1, y1;
  gint x2, y2;

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
  else if ((d = gimage_active_drawable (gimage)) &&
	   GIMP_IS_CHANNEL (d))
    {
      return channel_boundary (gimage_get_mask (gimage),
			       segs_in, segs_out,
			       num_segs_in, num_segs_out,
			       0, 0, gimage->width, gimage->height);
    }
  /* if a layer is active, we return multiple boundaries based on the extents */
  else if ((layer = gimage_get_active_layer (gimage)))
    {
      gint off_x, off_y;

      drawable_offsets (GIMP_DRAWABLE(layer), &off_x, &off_y);
      x1 = CLAMP (off_x, 0, gimage->width);
      y1 = CLAMP (off_y, 0, gimage->height);
      x2 = CLAMP (off_x + drawable_width (GIMP_DRAWABLE(layer)), 0,
		  gimage->width);
      y2 = CLAMP (off_y + drawable_height (GIMP_DRAWABLE(layer)), 0,
		  gimage->height);

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


gboolean
gimage_mask_bounds (GImage *gimage,
		    gint   *x1,
		    gint   *y1,
		    gint   *x2,
		    gint   *y2)
{
  return channel_bounds (gimage_get_mask (gimage), x1, y1, x2, y2);
}


void
gimage_mask_invalidate (GImage *gimage)
{
  Layer *layer;
  Channel *mask;

  /*  Turn the current selection off  */
  gdisplays_selection_visibility (gimage, SelectionOff);

  mask = gimage_get_mask (gimage);
  mask->boundary_known = FALSE;

  /*  If there is a floating selection, update it's area...
   *  we need to do this since this selection mask can act as an additional
   *  mask in the composition of the floating selection
   */
  layer = gimage_get_active_layer (gimage);
  if (layer && layer_is_floating_sel (layer))
    drawable_update (GIMP_DRAWABLE(layer), 0, 0,
		     GIMP_DRAWABLE(layer)->width, GIMP_DRAWABLE(layer)->height);
}


gint
gimage_mask_value (GImage *gimage,
		   int     x,
		   int     y)
{
  return channel_value (gimage_get_mask (gimage), x, y);
}


gboolean
gimage_mask_is_empty (GImage *gimage)
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
gimage_mask_translate (GImage *gimage,
		       gint    off_x,
		       gint    off_y)
{
  channel_translate (gimage_get_mask (gimage), off_x, off_y);
}


TileManager *
gimage_mask_extract (GImage       *gimage,
		     GimpDrawable *drawable,
		     gboolean      cut_gimage,
		     gboolean      keep_indexed,
		     gboolean      add_alpha)
{
  TileManager * tiles;
  Channel * sel_mask;
  PixelRegion srcPR, destPR, maskPR;
  guchar bg[MAX_CHANNELS];
  gint bytes, type;
  gint x1, y1;
  gint x2, y2;
  gint off_x, off_y;
  gboolean non_empty;

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
      g_message (_("Unable to cut/copy because the selected\n"
		   "region is empty."));
      return NULL;
    }

  /*  How many bytes in the temp buffer?  */
  switch (drawable_type (drawable))
    {
    case RGB_GIMAGE: case RGBA_GIMAGE:
      bytes = add_alpha ? 4 : drawable->bytes;
      type = RGB;
      break;
    case GRAY_GIMAGE: case GRAYA_GIMAGE:
      bytes = add_alpha ? 2 : drawable->bytes;
      type = GRAY;
      break;
    case INDEXED_GIMAGE: case INDEXEDA_GIMAGE:
      if (keep_indexed)
	{
	  bytes = add_alpha ? 2 : drawable->bytes;
	  type = GRAY;
	}
      else
	{
	  bytes = add_alpha ? 4 : drawable->bytes;
	  type = INDEXED;
	}
      break;
    default:
      bytes = 3;
      type  = RGB;
      break;
    }

  /*  get the selection mask */
  if (non_empty)
    sel_mask = gimage_get_mask (gimage);
  else
    sel_mask = NULL;

  gimage_get_background (gimage, drawable, bg);

  /*  If a cut was specified, and the selection mask is not empty,
   *  push an undo
   */
  if (cut_gimage && non_empty)
    drawable_apply_image (drawable, x1, y1, x2, y2, NULL, FALSE);

  drawable_offsets (drawable, &off_x, &off_y);

  /*  Allocate the temp buffer  */
  tiles = tile_manager_new ((x2 - x1), (y2 - y1), bytes);
  tiles->x = x1 + off_x;
  tiles->y = y1 + off_y;

  /* configure the pixel regions  */
  pixel_region_init (&srcPR, drawable_data (drawable),
		     x1, y1, (x2 - x1), (y2 - y1), cut_gimage);
  pixel_region_init (&destPR, tiles, 0, 0, (x2 - x1), (y2 - y1), TRUE);

  /*  If there is a selection, extract from it  */
  if (non_empty)
    {
      pixel_region_init (&maskPR, GIMP_DRAWABLE(sel_mask)->tiles,
			 (x1 + off_x), (y1 + off_y), (x2 - x1), (y2 - y1),
			 FALSE);

      extract_from_region (&srcPR, &destPR, &maskPR, drawable_cmap (drawable),
			   bg, type, drawable_has_alpha (drawable), cut_gimage);

      if (cut_gimage)
	{
	  /*  Clear the region  */
	  channel_clear (gimage_get_mask (gimage));

	  /*  Update the region  */
	  gdisplays_update_area (gimage, tiles->x, tiles->y,
				 tiles->width, tiles->height);

	  /*  Invalidate the preview  */
	  drawable_invalidate_preview (drawable);
	}
    }
  /*  Otherwise, get the entire active layer  */
  else
    {
      /*  If the layer is indexed...we need to extract pixels  */
      if (type == INDEXED && !keep_indexed)
	extract_from_region (&srcPR, &destPR, NULL, drawable_cmap (drawable),
			     bg, type, drawable_has_alpha (drawable), FALSE);
      /*  If the layer doesn't have an alpha channel, add one  */
      else if (bytes > srcPR.bytes)
	add_alpha_region (&srcPR, &destPR);
      /*  Otherwise, do a straight copy  */
      else
	copy_region (&srcPR, &destPR);

      /*  If we're cutting, remove either the layer (or floating selection),
       *  the layer mask, or the channel
       */
      if (cut_gimage && GIMP_IS_LAYER (drawable))
	{
	  if (layer_is_floating_sel (GIMP_LAYER (drawable)))
	    floating_sel_remove (GIMP_LAYER (drawable));
	  else
	    gimage_remove_layer (gimage, GIMP_LAYER (drawable));
	}
      else if (cut_gimage && GIMP_IS_LAYER_MASK (drawable))
	{
	  gimage_remove_layer_mask (gimage, 
				    layer_mask_get_layer (GIMP_LAYER_MASK (drawable)),
				    DISCARD);
	}
      else if (cut_gimage && GIMP_IS_CHANNEL (drawable))
	gimage_remove_channel (gimage, GIMP_CHANNEL (drawable));
    }

  return tiles;
}


Layer *
gimage_mask_float (GImage       *gimage,
		   GimpDrawable *drawable,
		   gint          off_x,    /* optional offset */
		   gint          off_y)
{
  Layer *layer;
  Channel *mask = gimage_get_mask (gimage);
  TileManager* tiles;
  gboolean non_empty;
  gint x1, y1;
  gint x2, y2;

  /*  Make sure there is a region to float...  */
  non_empty = drawable_mask_bounds ( (drawable), &x1, &y1, &x2, &y2);
  if (! non_empty || (x2 - x1) == 0 || (y2 - y1) == 0)
    {
      g_message (_("Float Selection: No selection to float."));
      return NULL;
    }

  /*  Start an undo group  */
  undo_push_group_start (gimage, FLOAT_MASK_UNDO);

  /*  Cut the selected region  */
  tiles = gimage_mask_extract (gimage, drawable, TRUE, FALSE, TRUE);

  /*  Create a new layer from the buffer  */
  layer = layer_new_from_tiles (gimage, gimp_drawable_type_with_alpha(drawable), tiles, 
				_("Floated Layer"), OPAQUE_OPACITY, NORMAL_MODE);

  /*  Set the offsets  */
  GIMP_DRAWABLE(layer)->offset_x = tiles->x + off_x;
  GIMP_DRAWABLE(layer)->offset_y = tiles->y + off_y;

  /*  Free the temp buffer  */
  tile_manager_destroy (tiles);

  /*  Add the floating layer to the gimage  */
  floating_sel_attach (layer, drawable);

  /*  End an undo group  */
  undo_push_group_end (gimage);

  /*  invalidate the gimage's boundary variables  */
  mask->boundary_known = FALSE;

  return layer;
}


void
gimage_mask_clear (GImage *gimage)
{
  channel_clear (gimage_get_mask (gimage));
}


void
gimage_mask_undo (GImage *gimage)
{
  channel_push_undo (gimage_get_mask (gimage));
}


void
gimage_mask_invert (GImage *gimage)
{
  channel_invert (gimage_get_mask (gimage));
}


void
gimage_mask_sharpen (GImage *gimage)
{
  /*  No need to play with the selection visibility
   *  because sharpen will not change the outline
   */
  channel_sharpen (gimage_get_mask (gimage));
}


void
gimage_mask_all (GImage *gimage)
{
  channel_all (gimage_get_mask (gimage));
}


void
gimage_mask_none (GImage *gimage)
{
  channel_clear (gimage_get_mask (gimage));
}


void
gimage_mask_feather (GImage  *gimage,
		     gdouble  feather_radius_x,
		     gdouble  feather_radius_y)
{
  /*  push the current mask onto the undo stack--need to do this here because
   *  channel_feather doesn't do it
   */
  channel_push_undo (gimage_get_mask (gimage));

  /*  feather the region  */
  channel_feather (gimage_get_mask (gimage),
		   gimage_get_mask (gimage),
		   feather_radius_x,
		   feather_radius_y,
		   REPLACE, 0, 0);
}


void
gimage_mask_border (GImage *gimage,
		    gint    border_radius_x,
		    gint    border_radius_y)
{
  /*  feather the region  */
  channel_border (gimage_get_mask (gimage),
		  border_radius_x,
		  border_radius_y);
}


void
gimage_mask_grow (GImage *gimage,
		  int     grow_pixels_x,
		  int     grow_pixels_y)
{
  /*  feather the region  */
  channel_grow (gimage_get_mask (gimage),
		grow_pixels_x,
		grow_pixels_y);
}


void
gimage_mask_shrink (GImage   *gimage,
		    gint      shrink_pixels_x,
		    gint      shrink_pixels_y,
		    gboolean  edge_lock)
{
  /*  feather the region  */
  channel_shrink (gimage_get_mask (gimage),
		  shrink_pixels_x,
		  shrink_pixels_y,
		  edge_lock);
}


void
gimage_mask_layer_alpha (GImage *gimage,
			 Layer  *layer)
{
  /*  extract the layer's alpha channel  */
  if (drawable_has_alpha (GIMP_DRAWABLE (layer)))
    {
      /*  load the mask with the given layer's alpha channel  */
      channel_layer_alpha (gimage_get_mask (gimage), layer);
    }
  else
    {
      g_message (_("The active layer has no alpha channel\n"
		   "to convert to a selection."));
      return;
    }
}


void
gimage_mask_layer_mask (GImage *gimage,
			Layer  *layer)
{
  /*  extract the layer's alpha channel  */
  if (layer_get_mask (layer))
    {
      /*  load the mask with the given layer's alpha channel  */
      channel_layer_mask (gimage_get_mask (gimage), layer);
    }
  else
    {
      g_message (_("The active layer has no mask\n"
		   "to convert to a selection."));
      return;
    }
}


void
gimage_mask_load (GImage  *gimage,
		  Channel *channel)
{
  /*  Load the specified channel to the gimage mask  */
  channel_load (gimage_get_mask (gimage), (channel));
}


Channel *
gimage_mask_save (GImage *gimage)
{
  Channel *new_channel;

  new_channel = channel_copy (gimage_get_mask (gimage));

  /*  saved selections are not visible by default  */
  GIMP_DRAWABLE(new_channel)->visible = FALSE;
  gimage_add_channel (gimage, new_channel, -1);

  return new_channel;
}


gboolean
gimage_mask_stroke (GImage       *gimage,
		    GimpDrawable *drawable)
{
  BoundSeg *bs_in;
  BoundSeg *bs_out;
  gint num_segs_in;
  gint num_segs_out;
  BoundSeg *stroke_segs;
  gint num_strokes;
  gint seg;
  gint offx, offy;
  gint i;
  gdouble *stroke_points;
  gint cpnt;
  Argument *return_vals;
  gint nreturn_vals;

  if (! gimage_mask_boundary (gimage, &bs_in, &bs_out,
			      &num_segs_in, &num_segs_out))
    {
      g_message (_("No selection to stroke!"));
      return FALSE;
    }

  stroke_segs = sort_boundary (bs_in, num_segs_in, &num_strokes);
  if (num_strokes == 0)
    return TRUE;

  /*  find the drawable offsets  */
  drawable_offsets (drawable, &offx, &offy);
  gimage_mask_stroking = TRUE;

  /*  Start an undo group  */
  undo_push_group_start (gimage, PAINT_CORE_UNDO);

  seg = 0;
  cpnt = 0;
  /* Largest array required (may be used in segments!) */
  stroke_points = g_malloc (sizeof (gdouble) * 2 * (num_segs_in + 4));

  stroke_points[cpnt++] = (gdouble)(stroke_segs[0].x1 - offx);
  stroke_points[cpnt++] = (gdouble)(stroke_segs[0].y1 - offy);

  for (i = 0; i < num_strokes; i++)
    {
      while ((stroke_segs[seg].x1 != -1 ||
	      stroke_segs[seg].x2 != -1 ||
	      stroke_segs[seg].y1 != -1 ||
	      stroke_segs[seg].y2 != -1))
	{
	  stroke_points[cpnt++] = (gdouble)(stroke_segs[seg].x2 - offx);
	  stroke_points[cpnt++] = (gdouble)(stroke_segs[seg].y2 - offy);
	  seg ++;
	}

      /* Close the stroke poitns up */
      stroke_points[cpnt++] = stroke_points[0];
      stroke_points[cpnt++] = stroke_points[1];

      /* Stroke with the correct tool */
      return_vals = procedural_db_run_proc (tool_active_PDB_string(),
					    &nreturn_vals,
					    PDB_DRAWABLE, drawable_ID (drawable),
					    PDB_INT32, (gint32) cpnt,
					    PDB_FLOATARRAY, stroke_points,
					    PDB_END);
      
      if (return_vals && return_vals[0].value.pdb_int == PDB_SUCCESS)
	{
	  /* Not required */
	  /*gdisplays_flush ();*/
	}
      else
	g_message (_("Paintbrush operation failed."));
      
      procedural_db_destroy_args (return_vals, nreturn_vals);
      
      cpnt = 0;
      seg ++;
      stroke_points[cpnt++] = (gdouble)(stroke_segs[seg].x1 - offx);
      stroke_points[cpnt++] = (gdouble)(stroke_segs[seg].y1 - offy);
    }

  /*  cleanup  */
  gimage_mask_stroking = FALSE;
  g_free (stroke_points);
  g_free (stroke_segs);

  /*  End an undo group  */
  undo_push_group_end (gimage);

  return TRUE;
}
