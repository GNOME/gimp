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

#include "core-types.h"

#include "base/boundary.h"
#include "base/pixel-region.h"
#include "base/tile-manager.h"

#include "paint-funcs/paint-funcs.h"

#include "pdb/procedural_db.h"

#include "gimpchannel.h"
#include "gimpcontext.h"
#include "gimpimage.h"
#include "gimpimage-mask.h"
#include "gimplayer.h"
#include "gimplayermask.h"
#include "gimptoolinfo.h"

#include "floating_sel.h"
#include "undo.h"

#include "libgimp/gimpintl.h"


/*  local variables  */

static gboolean   gimp_image_mask_stroking = FALSE;


/*  public functions  */

gboolean
gimp_image_mask_boundary (GimpImage  *gimage,
                          BoundSeg  **segs_in,
                          BoundSeg  **segs_out,
                          gint       *num_segs_in,
                          gint       *num_segs_out)
{
  GimpDrawable *d;
  GimpLayer    *layer;
  gint          x1, y1;
  gint          x2, y2;

  g_return_val_if_fail (GIMP_IS_IMAGE (gimage), FALSE);
  g_return_val_if_fail (segs_in != NULL, FALSE);
  g_return_val_if_fail (segs_out != NULL, FALSE);
  g_return_val_if_fail (num_segs_in != NULL, FALSE);
  g_return_val_if_fail (num_segs_out != NULL, FALSE);

  if ((layer = gimp_image_floating_sel (gimage)))
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
      gimp_channel_boundary (gimp_image_get_mask (gimage),
			     segs_in, segs_out,
			     num_segs_in, num_segs_out,
			     0, 0, 0, 0);

      /*  Find the floating selection boundary  */
      *segs_in = floating_sel_boundary (layer, num_segs_in);

      return TRUE;
    }
  /*  Otherwise, return the boundary...if a channel is active  */
  else if ((d = gimp_image_active_drawable (gimage)) &&
	   GIMP_IS_CHANNEL (d))
    {
      return gimp_channel_boundary (gimp_image_get_mask (gimage),
				    segs_in, segs_out,
				    num_segs_in, num_segs_out,
				    0, 0, gimage->width, gimage->height);
    }
  /* if a layer is active, we return multiple boundaries based on the extents */
  else if ((layer = gimp_image_get_active_layer (gimage)))
    {
      gint off_x, off_y;

      gimp_drawable_offsets (GIMP_DRAWABLE(layer), &off_x, &off_y);
      x1 = CLAMP (off_x, 0, gimage->width);
      y1 = CLAMP (off_y, 0, gimage->height);
      x2 = CLAMP (off_x + gimp_drawable_width (GIMP_DRAWABLE(layer)), 0,
		  gimage->width);
      y2 = CLAMP (off_y + gimp_drawable_height (GIMP_DRAWABLE(layer)), 0,
		  gimage->height);

      return gimp_channel_boundary (gimp_image_get_mask (gimage),
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
gimp_image_mask_bounds (GimpImage *gimage,
                        gint      *x1,
                        gint      *y1,
                        gint      *x2,
                        gint      *y2)
{
  return gimp_channel_bounds (gimp_image_get_mask (gimage), x1, y1, x2, y2);
}


void
gimp_image_mask_invalidate (GimpImage *gimage)
{
  GimpLayer   *layer;
  GimpChannel *mask;

  /*  Turn the current selection off  */
  gimp_image_selection_control (gimage, GIMP_SELECTION_OFF);

  mask = gimp_image_get_mask (gimage);
  mask->boundary_known = FALSE;

  /*  If there is a floating selection, update it's area...
   *  we need to do this since this selection mask can act as an additional
   *  mask in the composition of the floating selection
   */
  layer = gimp_image_get_active_layer (gimage);

  if (layer && gimp_layer_is_floating_sel (layer))
    gimp_drawable_update (GIMP_DRAWABLE (layer),
			  0, 0,
			  GIMP_DRAWABLE (layer)->width,
			  GIMP_DRAWABLE (layer)->height);

  /* Issue the MASK_CHANGED signal here */
  gimp_image_mask_changed (gimage);
}


gint
gimp_image_mask_value (GimpImage *gimage,
                       gint       x,
                       gint       y)
{
  return gimp_channel_value (gimp_image_get_mask (gimage), x, y);
}


gboolean
gimp_image_mask_is_empty (GimpImage *gimage)
{
  /*  in order to allow stroking of selections, we need to pretend here
   *  that the selection mask is empty so that it doesn't mask the paint
   *  during the stroke operation.
   */
  if (gimp_image_mask_stroking)
    return TRUE;
  else
    return gimp_channel_is_empty (gimp_image_get_mask (gimage));
}


void
gimp_image_mask_translate (GimpImage *gimage,
                           gint       off_x,
                           gint       off_y)
{
  gimp_channel_translate (gimp_image_get_mask (gimage), off_x, off_y);
}


TileManager *
gimp_image_mask_extract (GimpImage    *gimage,
                         GimpDrawable *drawable,
                         gboolean      cut_gimage,
                         gboolean      keep_indexed,
                         gboolean      add_alpha)
{
  TileManager *tiles;
  GimpChannel *sel_mask;
  PixelRegion  srcPR, destPR, maskPR;
  guchar       bg[MAX_CHANNELS];
  gint         bytes, type;
  gint         x1, y1;
  gint         x2, y2;
  gint         off_x, off_y;
  gboolean     non_empty;

  if (!drawable) 
    return NULL;

  /*  If there are no bounds, then just extract the entire image
   *  This may not be the correct behavior, but after getting rid
   *  of floating selections, it's still tempting to "cut" or "copy"
   *  a small layer and expect it to work, even though there is no
   *  actual selection mask
   */
  non_empty = gimp_drawable_mask_bounds (drawable, &x1, &y1, &x2, &y2);
  if (non_empty && (!(x2 - x1) || !(y2 - y1)))
    {
      g_message (_("Unable to cut or copy because the\n"
		   "selected region is empty."));
      return NULL;
    }

  /*  How many bytes in the temp buffer?  */
  switch (gimp_drawable_type (drawable))
    {
    case GIMP_RGB_IMAGE: case GIMP_RGBA_IMAGE:
      bytes = add_alpha ? 4 : drawable->bytes;
      type = GIMP_RGB;
      break;
    case GIMP_GRAY_IMAGE: case GIMP_GRAYA_IMAGE:
      bytes = add_alpha ? 2 : drawable->bytes;
      type = GIMP_GRAY;
      break;
    case GIMP_INDEXED_IMAGE: case GIMP_INDEXEDA_IMAGE:
      if (keep_indexed)
	{
	  bytes = add_alpha ? 2 : drawable->bytes;
	  type = GIMP_GRAY;
	}
      else
	{
	  bytes = add_alpha ? 4 : drawable->bytes;
	  type = GIMP_INDEXED;
	}
      break;
    default:
      bytes = 3;
      type  = GIMP_RGB;
      break;
    }

  /*  get the selection mask */
  if (non_empty)
    sel_mask = gimp_image_get_mask (gimage);
  else
    sel_mask = NULL;

  gimp_image_get_background (gimage, drawable, bg);

  /*  If a cut was specified, and the selection mask is not empty,
   *  push an undo
   */
  if (cut_gimage && non_empty)
    gimp_drawable_apply_image (drawable, x1, y1, x2, y2, NULL, FALSE);

  gimp_drawable_offsets (drawable, &off_x, &off_y);

  /*  Allocate the temp buffer  */
  tiles = tile_manager_new ((x2 - x1), (y2 - y1), bytes);
  tile_manager_set_offsets (tiles, x1 + off_x, y1 + off_y);

  /* configure the pixel regions  */
  pixel_region_init (&srcPR, gimp_drawable_data (drawable),
		     x1, y1, (x2 - x1), (y2 - y1), cut_gimage);
  pixel_region_init (&destPR, tiles, 0, 0, (x2 - x1), (y2 - y1), TRUE);

  /*  If there is a selection, extract from it  */
  if (non_empty)
    {
      pixel_region_init (&maskPR, GIMP_DRAWABLE(sel_mask)->tiles,
			 (x1 + off_x), (y1 + off_y), (x2 - x1), (y2 - y1),
			 FALSE);

      extract_from_region (&srcPR, &destPR, &maskPR,
			   gimp_drawable_cmap (drawable),
			   bg, type,
			   gimp_drawable_has_alpha (drawable), cut_gimage);

      if (cut_gimage)
	{
	  /*  Clear the region  */
	  gimp_channel_clear (gimp_image_get_mask (gimage));

	  /*  Update the region  */
	  gimp_image_update (gimage, 
			     x1 + off_x, y1 + off_y,
			     (x2 - x1), (y2 - y1));

	  /*  Invalidate the preview  */
	  gimp_viewable_invalidate_preview (GIMP_VIEWABLE (drawable));
	}
    }
  /*  Otherwise, get the entire active layer  */
  else
    {
      /*  If the layer is indexed...we need to extract pixels  */
      if (type == GIMP_INDEXED && !keep_indexed)
	extract_from_region (&srcPR, &destPR, NULL,
			     gimp_drawable_cmap (drawable),
			     bg, type,
			     gimp_drawable_has_alpha (drawable), FALSE);
      /*  If the layer doesn't have an alpha channel, add one  */
      else if (bytes > srcPR.bytes)
	add_alpha_region (&srcPR, &destPR);
      /*  Otherwise, do a straight copy  */
      else
	copy_region (&srcPR, &destPR);

      /*  If we're cutting, remove either the layer (or floating selection),
       *  the layer mask, or the channel
       */
      if (cut_gimage)
	{
	  if (GIMP_IS_LAYER (drawable))
	    {
	      if (gimp_layer_is_floating_sel (GIMP_LAYER (drawable)))
		floating_sel_remove (GIMP_LAYER (drawable));
	      else
		gimp_image_remove_layer (gimage, GIMP_LAYER (drawable));
	    }
	  else if (GIMP_IS_LAYER_MASK (drawable))
	    {
	      gimp_layer_apply_mask (gimp_layer_mask_get_layer (GIMP_LAYER_MASK (drawable)),
				     DISCARD, TRUE);
	    }
	  else if (GIMP_IS_CHANNEL (drawable))
	    {
	      gimp_image_remove_channel (gimage, GIMP_CHANNEL (drawable));
	    }
	}
    }

  return tiles;
}

GimpLayer *
gimp_image_mask_float (GimpImage    *gimage,
                       GimpDrawable *drawable,
                       gint          off_x,    /* optional offset */
                       gint          off_y)
{
  GimpLayer   *layer;
  GimpChannel *mask = gimp_image_get_mask (gimage);
  TileManager *tiles;
  gboolean     non_empty;
  gint         x1, y1;
  gint         x2, y2;

  /*  Make sure there is a region to float...  */
  non_empty = gimp_drawable_mask_bounds ( (drawable), &x1, &y1, &x2, &y2);
  if (! non_empty || (x2 - x1) == 0 || (y2 - y1) == 0)
    {
      g_message (_("Cannot float selection: No selection made."));
      return NULL;
    }

  /*  Start an undo group  */
  undo_push_group_start (gimage, FLOAT_MASK_UNDO);

  /*  Cut the selected region  */
  tiles = gimp_image_mask_extract (gimage, drawable, TRUE, FALSE, TRUE);

  /*  Create a new layer from the buffer  */
  layer = gimp_layer_new_from_tiles (gimage,
				     tiles, 
				     _("Floating Selection"),
				     OPAQUE_OPACITY, GIMP_NORMAL_MODE);

  /*  Set the offsets  */
  tile_manager_get_offsets (tiles, &x1, &y1);
  GIMP_DRAWABLE (layer)->offset_x = x1 + off_x;
  GIMP_DRAWABLE (layer)->offset_y = y1 + off_y;

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
gimp_image_mask_clear (GimpImage *gimage)
{
  gimp_channel_clear (gimp_image_get_mask (gimage));
}


void
gimp_image_mask_undo (GimpImage *gimage)
{
  gimp_channel_push_undo (gimp_image_get_mask (gimage));
}


void
gimp_image_mask_invert (GimpImage *gimage)
{
  gimp_channel_invert (gimp_image_get_mask (gimage));
}


void
gimp_image_mask_sharpen (GimpImage *gimage)
{
  /*  No need to play with the selection visibility
   *  because sharpen will not change the outline
   */
  gimp_channel_sharpen (gimp_image_get_mask (gimage));
}


void
gimp_image_mask_all (GimpImage *gimage)
{
  gimp_channel_all (gimp_image_get_mask (gimage));
}


void
gimp_image_mask_none (GimpImage *gimage)
{
  gimp_channel_clear (gimp_image_get_mask (gimage));
}


void
gimp_image_mask_feather (GimpImage *gimage,
                         gdouble    feather_radius_x,
                         gdouble    feather_radius_y)
{
  gimp_channel_feather (gimp_image_get_mask (gimage),
			feather_radius_x,
			feather_radius_y,
                        TRUE /* push undo */);
}


void
gimp_image_mask_border (GimpImage *gimage,
                        gint       border_radius_x,
                        gint       border_radius_y)
{
  gimp_channel_border (gimp_image_get_mask (gimage),
		       border_radius_x,
		       border_radius_y);
}


void
gimp_image_mask_grow (GimpImage *gimage,
                      gint       grow_pixels_x,
                      gint       grow_pixels_y)
{
  gimp_channel_grow (gimp_image_get_mask (gimage),
		     grow_pixels_x,
		     grow_pixels_y);
}


void
gimp_image_mask_shrink (GimpImage *gimage,
                        gint       shrink_pixels_x,
                        gint       shrink_pixels_y,
                        gboolean   edge_lock)
{
  gimp_channel_shrink (gimp_image_get_mask (gimage),
		       shrink_pixels_x,
		       shrink_pixels_y,
		       edge_lock);
}


void
gimp_image_mask_layer_alpha (GimpImage *gimage,
                             GimpLayer *layer)
{
  /*  extract the layer's alpha channel  */
  if (gimp_drawable_has_alpha (GIMP_DRAWABLE (layer)))
    {
      /*  load the mask with the given layer's alpha channel  */
      gimp_channel_layer_alpha (gimp_image_get_mask (gimage), layer);
    }
  else
    {
      g_message (_("The active layer has no alpha channel\n"
		   "to convert to a selection."));
      return;
    }
}


void
gimp_image_mask_layer_mask (GimpImage *gimage,
                            GimpLayer *layer)
{
  /*  extract the layer's alpha channel  */
  if (gimp_layer_get_mask (layer))
    {
      /*  load the mask with the given layer's alpha channel  */
      gimp_channel_layer_mask (gimp_image_get_mask (gimage), layer);
    }
  else
    {
      g_message (_("The active layer has no mask\n"
		   "to convert to a selection."));
      return;
    }
}


void
gimp_image_mask_load (GimpImage   *gimage,
                      GimpChannel *channel)
{
  /*  Load the specified channel to the gimage mask  */
  gimp_channel_load (gimp_image_get_mask (gimage), (channel));
}


GimpChannel *
gimp_image_mask_save (GimpImage *gimage)
{
  GimpChannel *new_channel;

  new_channel = gimp_channel_copy (gimp_image_get_mask (gimage),
                                   G_TYPE_FROM_INSTANCE (gimp_image_get_mask (gimage)),
                                   TRUE);

  /*  saved selections are not visible by default  */
  gimp_drawable_set_visible (GIMP_DRAWABLE (new_channel), FALSE);

  gimp_image_add_channel (gimage, new_channel, -1);

  return new_channel;
}


gboolean
gimp_image_mask_stroke (GimpImage    *gimage,
                        GimpDrawable *drawable,
                        GimpContext  *context)
{
  BoundSeg    *bs_in;
  BoundSeg    *bs_out;
  gint         num_segs_in;
  gint         num_segs_out;
  BoundSeg    *stroke_segs;
  gint         num_strokes;
  gint         seg;
  gint         offx, offy;
  gint         i;
  gdouble     *stroke_points;
  gint         cpnt;
  Argument    *return_vals;
  gint         nreturn_vals;
  const gchar *pdb_string;

  g_return_val_if_fail (GIMP_IS_IMAGE (gimage), FALSE);
  g_return_val_if_fail (GIMP_IS_DRAWABLE (drawable), FALSE);
  g_return_val_if_fail (GIMP_IS_CONTEXT (context), FALSE);

  if (! gimp_image_mask_boundary (gimage, &bs_in, &bs_out,
                                  &num_segs_in, &num_segs_out))
    {
      g_message (_("No selection to stroke."));
      return FALSE;
    }

  stroke_segs = sort_boundary (bs_in, num_segs_in, &num_strokes);
  if (num_strokes == 0)
    return TRUE;

  pdb_string = gimp_context_get_tool (context)->pdb_string;

  /*  find the drawable offsets  */
  gimp_drawable_offsets (drawable, &offx, &offy);
  gimp_image_mask_stroking = TRUE;

  /*  Start an undo group  */
  undo_push_group_start (gimage, PAINT_CORE_UNDO);

  seg = 0;
  cpnt = 0;
  /* Largest array required (may be used in segments!) */
  stroke_points = g_malloc (sizeof (gdouble) * 2 * (num_segs_in + 4));

  /* we offset all coordinates by 0.5 to align the brush with the path */

  stroke_points[cpnt++] = (gdouble)(stroke_segs[0].x1 - offx + 0.5);
  stroke_points[cpnt++] = (gdouble)(stroke_segs[0].y1 - offy + 0.5);

  for (i = 0; i < num_strokes; i++)
    {
      while ((stroke_segs[seg].x1 != -1 ||
	      stroke_segs[seg].x2 != -1 ||
	      stroke_segs[seg].y1 != -1 ||
	      stroke_segs[seg].y2 != -1))
	{
	  stroke_points[cpnt++] = (gdouble)(stroke_segs[seg].x2 - offx + 0.5);
	  stroke_points[cpnt++] = (gdouble)(stroke_segs[seg].y2 - offy + 0.5);
	  seg ++;
	}

      /* Close the stroke points up */
      stroke_points[cpnt++] = stroke_points[0];
      stroke_points[cpnt++] = stroke_points[1];

      /* Stroke with the correct tool */
      return_vals =
	procedural_db_run_proc (gimage->gimp,
				pdb_string,
				&nreturn_vals,
				GIMP_PDB_DRAWABLE, gimp_drawable_get_ID (drawable),
				GIMP_PDB_INT32, (gint32) cpnt,
				GIMP_PDB_FLOATARRAY, stroke_points,
				GIMP_PDB_END);

      if (return_vals && return_vals[0].value.pdb_int != GIMP_PDB_SUCCESS)
	g_message (_("Paint operation failed."));

      procedural_db_destroy_args (return_vals, nreturn_vals);

      cpnt = 0;
      seg ++;
      stroke_points[cpnt++] = (gdouble)(stroke_segs[seg].x1 - offx + 0.5);
      stroke_points[cpnt++] = (gdouble)(stroke_segs[seg].y1 - offy + 0.5);
    }

  /*  cleanup  */
  gimp_image_mask_stroking = FALSE;
  g_free (stroke_points);
  g_free (stroke_segs);

  /*  End an undo group  */
  undo_push_group_end (gimage);

  return TRUE;
}
