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

#include <gtk/gtk.h>

#include "libgimpmath/gimpmath.h"

#include "apptypes.h"

#include "boundary.h"
#include "drawable.h"
#include "floating_sel.h"
#include "gdisplay.h"
#include "gimage_mask.h"
#include "gimpcontainer.h"
#include "gimpimage.h"
#include "gimplayer.h"
#include "gimplayermask.h"
#include "paint_funcs.h"
#include "pixel_region.h"
#include "tile_manager.h"
#include "undo.h"

#include "libgimp/gimpintl.h"


void
floating_sel_attach (GimpLayer    *layer,
		     GimpDrawable *drawable)
{
  GImage    *gimage;
  GimpLayer *floating_sel;

  if (! (gimage = gimp_drawable_gimage (drawable)))
    return;

  /*  If there is already a floating selection, anchor it  */
  if (gimage->floating_sel != NULL)
    {
      floating_sel = gimage->floating_sel;
      floating_sel_anchor (gimp_image_floating_sel (gimage));

      /*  if we were pasting to the old floating selection, paste now to the drawable  */
      if (drawable == GIMP_DRAWABLE (floating_sel))
	drawable = gimp_image_active_drawable (gimage);
    }

  /*  set the drawable and allocate a backing store  */
  layer->preserve_trans = TRUE;
  layer->fs.drawable = drawable;
  layer->fs.backing_store =
    tile_manager_new (GIMP_DRAWABLE (layer)->width,
		      GIMP_DRAWABLE (layer)->height,
		      gimp_drawable_bytes (drawable));

  /*  because setting the sensitivity in the layers_dialog lock call redraws the
   *  previews, we need to lock the dialogs before the floating sel is actually added.
   *  however, they won't lock unless we set the gimage's floating sel pointer
   */
  gimage->floating_sel = layer;

  /*  add the layer to the gimage  */
  gimp_image_add_layer (gimage, layer, 0);

  /*  store the affected area from the drawable in the backing store  */
  floating_sel_rigor (layer, TRUE);
}

void
floating_sel_remove (GimpLayer *layer)
{
  GImage *gimage;

  if (! (gimage = gimp_drawable_gimage (layer->fs.drawable)))
    return;

  /*  store the affected area from the drawable in the backing store  */
  floating_sel_relax (layer, TRUE);

  /*  invalidate the preview of the obscured drawable.  We do this here
   *  because it will not be done until the floating selection is removed,
   *  at which point the obscured drawable's preview will not be declared invalid
   */
  gimp_viewable_invalidate_preview (GIMP_VIEWABLE (layer));

  /*  remove the layer from the gimage  */
  gimp_image_remove_layer (gimage, layer);
}

void
floating_sel_anchor (GimpLayer *layer)
{
  GImage *gimage;

  if (! (gimage = gimp_drawable_gimage (GIMP_DRAWABLE (layer))))
    return;
  if (! gimp_layer_is_floating_sel (layer))
    {
      g_message (_("Cannot anchor this layer because\nit is not a floating selection."));
      return;
    }

  /*  Start a floating selection anchoring undo  */
  undo_push_group_start (gimage, FS_ANCHOR_UNDO);

  /* Invalidate the previews of the layer that will be composited
   * with the floating section.
   */
  gimp_viewable_invalidate_preview (GIMP_VIEWABLE (layer->fs.drawable));

  /*  Relax the floating selection  */
  floating_sel_relax (layer, TRUE);

  /*  Composite the floating selection contents  */
  floating_sel_composite (layer,
			  GIMP_DRAWABLE (layer)->offset_x,
			  GIMP_DRAWABLE (layer)->offset_y,
			  GIMP_DRAWABLE (layer)->width,
			  GIMP_DRAWABLE (layer)->height, TRUE);

  /*  remove the floating selection  */
  gimp_image_remove_layer (gimage, layer);

  /*  end the group undo  */
  undo_push_group_end (gimage);

  /*  invalidate the boundaries  */
  gimage_mask_invalidate (gimage);
}

void
floating_sel_reset (GimpLayer *layer)
{
  GImage *gimage;

  if (! (gimage = gimp_drawable_gimage (GIMP_DRAWABLE (layer))))
    return;

  /*  set the underlying drawable to active  */
  if (GIMP_IS_LAYER (layer->fs.drawable))
    {
      gimp_image_set_active_layer (gimage, GIMP_LAYER (layer->fs.drawable));
    }
  else if (GIMP_IS_LAYER_MASK (layer->fs.drawable))
    {
      gimp_image_set_active_layer (gimage,
				   GIMP_LAYER_MASK (layer->fs.drawable)->layer);
    }
  else if (GIMP_IS_CHANNEL (layer->fs.drawable))
    {
      gimp_image_set_active_channel (gimage, GIMP_CHANNEL (layer->fs.drawable));

      if (gimp_container_num_children (gimage->layers))
	{
	  gimp_image_set_active_layer (gimage,
				       (GimpLayer *) gimage->layer_stack->data);
	}
    }
}

void
floating_sel_to_layer (GimpLayer *layer)
{
  FStoLayerUndo *fsu;
  gint           off_x, off_y;
  gint           width, height;

  GImage *gimage;

  if (! (gimage = gimp_drawable_gimage (GIMP_DRAWABLE (layer))))
    return;

  /*  Check if the floating layer belongs to a channel...  */
  if (GIMP_IS_CHANNEL (layer->fs.drawable))
    {
      g_message (_("Cannot create a new layer from the floating\n"
		 "selection because it belongs to a\n"
		 "layer mask or channel."));
      return;
    }

  /*  restore the contents of the drawable  */
  floating_sel_restore (layer,
			GIMP_DRAWABLE (layer)->offset_x,
			GIMP_DRAWABLE (layer)->offset_y,
			GIMP_DRAWABLE (layer)->width,
			GIMP_DRAWABLE (layer)->height);

  /*  determine whether the resulting layer needs an update  */
  gimp_drawable_offsets (layer->fs.drawable, &off_x, &off_y);
  width  = gimp_drawable_width (layer->fs.drawable);
  height = gimp_drawable_height (layer->fs.drawable);

  /*  update the fs drawable--this updates the gimage composite preview
   *  as well as the underlying drawable's
   */
  gimp_viewable_invalidate_preview (GIMP_VIEWABLE (layer));

  /*  allocate the undo structure  */
  fsu = g_new (FStoLayerUndo, 1);
  fsu->layer    = layer;
  fsu->drawable = layer->fs.drawable;

  undo_push_fs_to_layer (gimage, fsu);

  /*  clear the selection  */
  gimp_layer_invalidate_boundary (layer);

  /*  Set pointers  */
  layer->fs.drawable = NULL;
  gimage->floating_sel = NULL;
  GIMP_DRAWABLE (layer)->visible = TRUE;

  /*  if the floating selection exceeds the attached layer's extents,
      update the new layer  */

  /*  I don't think that the preview is ever valid as is, since the layer
      will be added on top of the others.  Revert this if I'm wrong.
      msw@gimp.org
  */

  drawable_update (GIMP_DRAWABLE (layer),
		   0, 0,
		   GIMP_DRAWABLE (layer)->width,
		   GIMP_DRAWABLE (layer)->height);
  
  /* This may be undesirable when invoked non-interactively... we'll see. */
  /*reinit_layer_idlerender (gimage, layer);*/
}

void
floating_sel_store (GimpLayer *layer,
		    gint       x,
		    gint       y,
		    gint       w,
		    gint       h)
{
  PixelRegion srcPR, destPR;
  gint        offx, offy;
  gint        x1, y1, x2, y2;

  /*  Check the backing store & make sure it has the correct dimensions  */
  if ((tile_manager_width (layer->fs.backing_store)  != gimp_drawable_width (GIMP_DRAWABLE(layer)))  ||
      (tile_manager_height (layer->fs.backing_store) != gimp_drawable_height (GIMP_DRAWABLE(layer))) ||
      (tile_manager_bpp (layer->fs.backing_store)    != gimp_drawable_bytes (layer->fs.drawable)))
    {
      /*  free the backing store and allocate anew  */
      tile_manager_destroy (layer->fs.backing_store);

      layer->fs.backing_store =
	tile_manager_new (GIMP_DRAWABLE (layer)->width,
			  GIMP_DRAWABLE (layer)->height,
			  gimp_drawable_bytes (layer->fs.drawable));
    }

  /*  What this function does is save the specified area of the
   *  drawable that this floating selection obscures.  We do this so
   *  that it will be possible to subsequently restore the drawable's area
   */
  gimp_drawable_offsets (layer->fs.drawable, &offx, &offy);

  /*  Find the minimum area we need to uncover -- in gimage space  */
  x1 = MAX (GIMP_DRAWABLE (layer)->offset_x, offx);
  y1 = MAX (GIMP_DRAWABLE (layer)->offset_y, offy);
  x2 = MIN (GIMP_DRAWABLE (layer)->offset_x + GIMP_DRAWABLE (layer)->width,
	    offx + gimp_drawable_width (layer->fs.drawable));
  y2 = MIN (GIMP_DRAWABLE (layer)->offset_y + GIMP_DRAWABLE (layer)->height,
	    offy + gimp_drawable_height (layer->fs.drawable));

  x1 = CLAMP (x, x1, x2);
  y1 = CLAMP (y, y1, y2);
  x2 = CLAMP (x + w, x1, x2);
  y2 = CLAMP (y + h, y1, y2);

  if ((x2 - x1) > 0 && (y2 - y1) > 0)
    {
      /*  Copy the area from the drawable to the backing store  */
      pixel_region_init (&srcPR, gimp_drawable_data (layer->fs.drawable),
			 (x1 - offx), (y1 - offy), (x2 - x1), (y2 - y1), FALSE);
      pixel_region_init (&destPR, layer->fs.backing_store,
			 (x1 - GIMP_DRAWABLE (layer)->offset_x),
			 (y1 - GIMP_DRAWABLE (layer)->offset_y),
			 (x2 - x1), (y2 - y1), TRUE);

      copy_region (&srcPR, &destPR);
    }
}

void
floating_sel_restore (GimpLayer *layer,
		      gint       x,
		      gint       y,
		      gint       w,
		      gint       h)
{
  PixelRegion srcPR, destPR;
  gint        offx, offy;
  gint        x1, y1, x2, y2;

  /*  What this function does is "uncover" the specified area in the
   *  drawable that this floating selection obscures.  We do this so
   *  that either the floating selection can be removed or it can be
   *  translated
   */

  /*  Find the minimum area we need to uncover -- in gimage space  */
  gimp_drawable_offsets (layer->fs.drawable, &offx, &offy);
  x1 = MAX (GIMP_DRAWABLE (layer)->offset_x, offx);
  y1 = MAX (GIMP_DRAWABLE (layer)->offset_y, offy);
  x2 = MIN (GIMP_DRAWABLE (layer)->offset_x + GIMP_DRAWABLE (layer)->width,
	    offx + gimp_drawable_width (layer->fs.drawable));
  y2 = MIN (GIMP_DRAWABLE(layer)->offset_y + GIMP_DRAWABLE (layer)->height,
	    offy + gimp_drawable_height (layer->fs.drawable));

  x1 = CLAMP (x, x1, x2);
  y1 = CLAMP (y, y1, y2);
  x2 = CLAMP (x + w, x1, x2);
  y2 = CLAMP (y + h, y1, y2);

  if ((x2 - x1) > 0 && (y2 - y1) > 0)
    {
      /*  Copy the area from the backing store to the drawable  */
      pixel_region_init (&srcPR, layer->fs.backing_store,
			 (x1 - GIMP_DRAWABLE (layer)->offset_x),
			 (y1 - GIMP_DRAWABLE (layer)->offset_y),
			 (x2 - x1), (y2 - y1), FALSE);
      pixel_region_init (&destPR, gimp_drawable_data (layer->fs.drawable),
			 (x1 - offx), (y1 - offy), (x2 - x1), (y2 - y1), TRUE);

      copy_region (&srcPR, &destPR);
    }
}

void
floating_sel_rigor (GimpLayer *layer,
		    gboolean   undo)
{
  GImage *gimage = GIMP_DRAWABLE(layer)->gimage;

  /*  store the affected area from the drawable in the backing store  */
  floating_sel_store (layer,
		      GIMP_DRAWABLE (layer)->offset_x,
		      GIMP_DRAWABLE (layer)->offset_y,
		      GIMP_DRAWABLE (layer)->width,
		      GIMP_DRAWABLE (layer)->height);
  layer->fs.initial = TRUE;

  if (undo)
    undo_push_fs_rigor (gimage, GIMP_DRAWABLE (layer)->ID);
}

void
floating_sel_relax (GimpLayer *layer,
		    gboolean   undo)
{
  GImage *gimage = GIMP_DRAWABLE(layer)->gimage;

  /*  restore the contents of drawable the floating layer is attached to  */
  if (layer->fs.initial == FALSE)
    floating_sel_restore (layer,
			  GIMP_DRAWABLE (layer)->offset_x,
			  GIMP_DRAWABLE (layer)->offset_y,
			  GIMP_DRAWABLE (layer)->width,
			  GIMP_DRAWABLE (layer)->height);
  layer->fs.initial = TRUE;

  if (undo)
    undo_push_fs_relax (gimage, GIMP_DRAWABLE (layer)->ID);
}

void
floating_sel_composite (GimpLayer *layer,
			gint       x,
			gint       y,
			gint       w,
			gint       h,
			gboolean   undo)
{
  PixelRegion  fsPR;
  GImage      *gimage;
  GimpLayer   *d_layer;
  gint         preserve_trans;
  gint         active[MAX_CHANNELS];
  gint         offx, offy;
  gint         x1, y1, x2, y2;
  gint         i;

  d_layer = NULL;

  if (! (gimage = gimp_drawable_gimage (GIMP_DRAWABLE (layer))))
    return;

  /*  What this function does is composite the specified area of the
   *  drawble with the floating selection.  We do this when the gimage
   *  is constructed, before any other composition takes place.
   */

  /*  If this isn't the first composite, restore the image underneath  */
  if (! layer->fs.initial)
    floating_sel_restore (layer, x, y, w, h);
  else if (GIMP_DRAWABLE(layer)->visible)
    layer->fs.initial = FALSE;

  /*  First restore what's behind the image if necessary, then check for visibility  */
  if (GIMP_DRAWABLE(layer)->visible)
    {
      /*  Find the minimum area we need to composite -- in gimage space  */
      gimp_drawable_offsets (layer->fs.drawable, &offx, &offy);
      x1 = MAX (GIMP_DRAWABLE (layer)->offset_x, offx);
      y1 = MAX (GIMP_DRAWABLE (layer)->offset_y, offy);
      x2 = MIN (GIMP_DRAWABLE (layer)->offset_x +
		GIMP_DRAWABLE (layer)->width,
		offx + gimp_drawable_width (layer->fs.drawable));
      y2 = MIN (GIMP_DRAWABLE (layer)->offset_y +
		GIMP_DRAWABLE (layer)->height,
		offy + gimp_drawable_height (layer->fs.drawable));

      x1 = CLAMP (x, x1, x2);
      y1 = CLAMP (y, y1, y2);
      x2 = CLAMP (x + w, x1, x2);
      y2 = CLAMP (y + h, y1, y2);

      if ((x2 - x1) > 0 && (y2 - y1) > 0)
	{
	  /*  composite the area from the layer to the drawable  */
	  pixel_region_init (&fsPR, GIMP_DRAWABLE (layer)->tiles,
			     (x1 - GIMP_DRAWABLE (layer)->offset_x),
			     (y1 - GIMP_DRAWABLE (layer)->offset_y),
			     (x2 - x1), (y2 - y1), FALSE);

	  /*  a kludge here to prevent the case of the drawable
	   *  underneath having preserve transparency on, and disallowing
	   *  the composited floating selection from being shown
	   */
	  if (GIMP_IS_LAYER (layer->fs.drawable))
	    {
	      d_layer = GIMP_LAYER (layer->fs.drawable);
	      if ((preserve_trans = d_layer->preserve_trans))
		d_layer->preserve_trans = FALSE;
	    }
	  else
	    preserve_trans = FALSE;

	  /*  We need to set all gimage channels to active to make sure that
	   *  nothing strange happens while applying the floating selection.
	   *  It wouldn't make sense for the floating selection to be affected
	   *  by the active gimage channels.
	   */
	  for (i = 0; i < MAX_CHANNELS; i++)
	    {
	      active[i] = gimage->active[i];
	      gimage->active[i] = 1;
	    }

	  /*  apply the fs with the undo specified by the value
	   *  passed to this function
	   */
	  gimp_image_apply_image (gimage, layer->fs.drawable, &fsPR,
				  undo, layer->opacity, layer->mode,
				  NULL,
				  (x1 - offx), (y1 - offy));

	  /*  restore preserve transparency  */
	  if (preserve_trans)
	    d_layer->preserve_trans = TRUE;

	  /*  restore gimage active channels  */
	  for (i = 0; i < MAX_CHANNELS; i++)
	    gimage->active[i] = active[i];
	}
    }
}

BoundSeg *
floating_sel_boundary (GimpLayer *layer,
		       gint      *n_segs)
{
  PixelRegion bPR;
  gint        i;

  if (layer->fs.boundary_known == FALSE)
    {
      if (layer->fs.segs)
	g_free (layer->fs.segs);

      /*  find the segments  */
      pixel_region_init (&bPR, GIMP_DRAWABLE (layer)->tiles,
			 0, 0,
			 GIMP_DRAWABLE (layer)->width,
			 GIMP_DRAWABLE (layer)->height, FALSE);
      layer->fs.segs = find_mask_boundary (&bPR, &layer->fs.num_segs,
					   WithinBounds, 0, 0,
					   GIMP_DRAWABLE (layer)->width,
					   GIMP_DRAWABLE (layer)->height);

      /*  offset the segments  */
      for (i = 0; i < layer->fs.num_segs; i++)
	{
	  layer->fs.segs[i].x1 += GIMP_DRAWABLE (layer)->offset_x;
	  layer->fs.segs[i].y1 += GIMP_DRAWABLE (layer)->offset_y;
	  layer->fs.segs[i].x2 += GIMP_DRAWABLE (layer)->offset_x;
	  layer->fs.segs[i].y2 += GIMP_DRAWABLE (layer)->offset_y;
	}

      layer->fs.boundary_known = TRUE;
    }

  *n_segs = layer->fs.num_segs;

  return layer->fs.segs;
}

void
floating_sel_invalidate (GimpLayer *layer)
{
  /*  Invalidate the attached-to drawable's preview  */
  gimp_viewable_invalidate_preview (GIMP_VIEWABLE (layer->fs.drawable));

  /*  Invalidate the boundary  */
  layer->fs.boundary_known = FALSE;
}
