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
#include "appenv.h"
#include "channels_dialog.h"
#include "drawable.h"
#include "errors.h"
#include "floating_sel.h"
#include "gimage.h"
#include "gimage_mask.h"
#include "interface.h"
#include "paint_funcs.h"
#include "undo.h"


void
floating_sel_attach (Layer *layer,
		     int    drawable_id)
{
  GImage *gimage;
  int floating_sel;

  if (! (gimage = drawable_gimage (drawable_id)))
    return;

  /*  If there is already a floating selection, anchor it  */
  if (gimage->floating_sel != -1)
    {
      floating_sel = gimage->floating_sel;
      floating_sel_anchor (gimage_floating_sel (gimage));

      /*  if we were pasting to the old floating selection, paste now to the drawable  */
      if (drawable_id == floating_sel)
	drawable_id = gimage_active_drawable (gimage);
    }

  /*  set the drawable and allocate a backing store  */
  layer->preserve_trans = TRUE;
  layer->fs.drawable = drawable_id;
  layer->fs.backing_store = tile_manager_new (layer->width, layer->height, drawable_bytes (drawable_id));

  /*  because setting the sensitivity in the layers_dialog lock call redraws the
   *  previews, we need to lock the dialogs before the floating sel is actually added.
   *  however, they won't lock unless we set the gimage's floating sel pointer
   */
  gimage->floating_sel = layer->ID;

  /*  add the layer to the gimage  */
  gimage_add_layer (gimage, layer, 0);

  /*  store the affected area from the drawable in the backing store  */
  floating_sel_rigor (layer, TRUE);
}

void
floating_sel_remove (Layer *layer)
{
  GImage *gimage;

  if (! (gimage = drawable_gimage (layer->fs.drawable)))
    return;

  /*  store the affected area from the drawable in the backing store  */
  floating_sel_relax (layer, TRUE);

  /*  invalidate the preview of the obscured drawable.  We do this here
   *  because it will not be done until the floating selection is removed,
   *  at which point the obscured drawable's preview will not be declared invalid
   */
  drawable_invalidate_preview (layer->ID);

  /*  remove the layer from the gimage  */
  gimage_remove_layer (gimage, layer->ID);
}

void
floating_sel_anchor (Layer *layer)
{
  GImage *gimage;

  if (! (gimage = drawable_gimage (layer->ID)))
    return;
  if (! layer_is_floating_sel (layer))
    {
      message_box ("Cannot anchor this layer because\nit is not a floating selection.",
		   NULL, NULL);
      return;
    }

  /*  Start a floating selection anchoring undo  */
  undo_push_group_start (gimage, FS_ANCHOR_UNDO);

  /*  Relax the floating selection  */
  floating_sel_relax (layer, TRUE);

  /*  Composite the floating selection contents  */
  floating_sel_composite (layer, layer->offset_x, layer->offset_y,
			  layer->width, layer->height, TRUE);

  /*  remove the floating selection  */
  gimage_remove_layer (gimage, layer->ID);

  /*  end the group undo  */
  undo_push_group_end (gimage);

  /*  invalidate the boundaries  */
  gimage_mask_invalidate (gimage);
}

void
floating_sel_reset (layer)
     Layer *layer;
{
  GImage *gimage;
  Channel *lm;

  if (! (gimage = drawable_gimage (layer->ID)))
    return;

  /*  set the underlying drawable to active  */
  if (drawable_layer (layer->fs.drawable))
    gimage_set_active_layer (gimage, layer->fs.drawable);
  else if ((lm = drawable_layer_mask (layer->fs.drawable)))
    gimage_set_active_layer (gimage, lm->layer_ID);
  else if (drawable_channel (layer->fs.drawable))
    {
      gimage_set_active_channel (gimage, layer->fs.drawable);
      if (gimage->layers)
	gimage->active_layer = ((Layer *) gimage->layer_stack->data)->ID;
      else
	gimage->active_layer = -1;
    }
}

void
floating_sel_to_layer (Layer *layer)
{
  FStoLayerUndo *fsu;
  int off_x, off_y;
  int width, height;
  int update = FALSE;

  GImage *gimage;

  if (! (gimage = drawable_gimage (layer->ID)))
    return;

  /*  Check if the floating layer belongs to a channel...  */
  if (drawable_channel (layer->fs.drawable) ||
      drawable_layer_mask (layer->fs.drawable))
    {
      message_box ("Cannot create a new layer from the floating\n"
		   "selection because it belongs to a\n"
		   "layer mask or channel.", NULL, NULL);
      return;
    }

  /*  restore the contents of the drawable  */
  floating_sel_restore (layer, layer->offset_x, layer->offset_y, layer->width, layer->height);

  /*  determine whether the resulting layer needs an update  */
  drawable_offsets (layer->fs.drawable, &off_x, &off_y);
  width = drawable_width (layer->fs.drawable);
  height = drawable_height (layer->fs.drawable);
  update = (update || ((layer->offset_x < off_x) ||
		       (layer->offset_y < off_y) ||
		       (layer->offset_x + layer->width > off_x + width) ||
		       (layer->offset_y + layer->height > off_y + height)) ? TRUE : FALSE);

  /*  update the fs drawable--this updates the gimage composite preview
   *  as well as the underlying drawable's
   */
  drawable_invalidate_preview (layer->ID);

  /*  allocate the undo structure  */
  fsu = (FStoLayerUndo *) g_malloc (sizeof (FStoLayerUndo));
  fsu->layer = layer;
  fsu->drawable = layer->fs.drawable;

  undo_push_fs_to_layer (gimage, fsu);

  /*  clear the selection  */
  layer_invalidate_boundary (layer);

  /*  Set pointers  */
  layer->fs.drawable = -1;
  gimage->floating_sel = -1;

  /*  if the floating selection exceeds the attached layer's extents, update the new layer  */
  if (update)
    drawable_update (layer->ID, 0, 0, layer->width, layer->height);
  /*  otherwise, just update the preview, because the image is valid as is  */
  else
    drawable_invalidate_preview (layer->ID);
}

void
floating_sel_store (Layer *layer,
		    int    x,
		    int    y,
		    int    w,
		    int    h)
{
  PixelRegion srcPR, destPR;
  int offx, offy;
  int x1, y1, x2, y2;

  /*  Check the backing store & make sure it has the correct dimensions  */
  if (layer->fs.backing_store->levels[0].width != layer->width ||
      layer->fs.backing_store->levels[0].height != layer->height ||
      layer->fs.backing_store->levels[0].bpp != drawable_bytes (layer->fs.drawable))
    {
      /*  free the backing store and allocate anew  */
      tile_manager_destroy (layer->fs.backing_store);

      layer->fs.backing_store = tile_manager_new (layer->width, layer->height,
						  drawable_bytes (layer->fs.drawable));
    }

  /*  What this function does is save the specified area of the
   *  drawable that this floating selection obscures.  We do this so
   *  that it will be possible to subsequently restore the drawable's area
   */
  drawable_offsets (layer->fs.drawable, &offx, &offy);

  /*  Find the minimum area we need to uncover -- in gimage space  */
  x1 = MAXIMUM (layer->offset_x, offx);
  y1 = MAXIMUM (layer->offset_y, offy);
  x2 = MINIMUM (layer->offset_x + layer->width, offx + drawable_width (layer->fs.drawable));
  y2 = MINIMUM (layer->offset_y + layer->height, offy + drawable_height (layer->fs.drawable));

  x1 = BOUNDS (x, x1, x2);
  y1 = BOUNDS (y, y1, y2);
  x2 = BOUNDS (x + w, x1, x2);
  y2 = BOUNDS (y + h, y1, y2);

  if ((x2 - x1) > 0 && (y2 - y1) > 0)
    {
      /*  Copy the area from the drawable to the backing store  */
      pixel_region_init (&srcPR, drawable_data (layer->fs.drawable),
			 (x1 - offx), (y1 - offy), (x2 - x1), (y2 - y1), FALSE);
      pixel_region_init (&destPR, layer->fs.backing_store,
			 (x1 - layer->offset_x), (y1 - layer->offset_y), (x2 - x1), (y2 - y1), TRUE);

      copy_region (&srcPR, &destPR);
    }
}

void
floating_sel_restore (Layer *layer,
		      int    x,
		      int    y,
		      int    w,
		      int    h)
{
  PixelRegion srcPR, destPR;
  int offx, offy;
  int x1, y1, x2, y2;

  /*  What this function does is "uncover" the specified area in the
   *  drawable that this floating selection obscures.  We do this so
   *  that either the floating selection can be removed or it can be
   *  translated
   */

  /*  Find the minimum area we need to uncover -- in gimage space  */
  drawable_offsets (layer->fs.drawable, &offx, &offy);
  x1 = MAXIMUM (layer->offset_x, offx);
  y1 = MAXIMUM (layer->offset_y, offy);
  x2 = MINIMUM (layer->offset_x + layer->width, offx + drawable_width (layer->fs.drawable));
  y2 = MINIMUM (layer->offset_y + layer->height, offy + drawable_height (layer->fs.drawable));

  x1 = BOUNDS (x, x1, x2);
  y1 = BOUNDS (y, y1, y2);
  x2 = BOUNDS (x + w, x1, x2);
  y2 = BOUNDS (y + h, y1, y2);

  if ((x2 - x1) > 0 && (y2 - y1) > 0)
    {
      /*  Copy the area from the backing store to the drawable  */
      pixel_region_init (&srcPR, layer->fs.backing_store,
			 (x1 - layer->offset_x), (y1 - layer->offset_y), (x2 - x1), (y2 - y1), FALSE);
      pixel_region_init (&destPR, drawable_data (layer->fs.drawable),
			 (x1 - offx), (y1 - offy), (x2 - x1), (y2 - y1), TRUE);

      copy_region (&srcPR, &destPR);
    }
}

void
floating_sel_rigor (Layer *layer,
		    int    undo)
{
  GImage *gimage;

  if ((gimage = gimage_get_ID (layer->gimage_ID)) == NULL)
    return;

  /*  store the affected area from the drawable in the backing store  */
  floating_sel_store (layer, layer->offset_x, layer->offset_y, layer->width, layer->height);
  layer->fs.initial = TRUE;

  if (undo)
    undo_push_fs_rigor (gimage, layer->ID);
}

void
floating_sel_relax (Layer *layer,
		    int    undo)
{
  GImage *gimage;

  if ((gimage = gimage_get_ID (layer->gimage_ID)) == NULL)
    return;

  /*  restore the contents of drawable the floating layer is attached to  */
  if (layer->fs.initial == FALSE)
    floating_sel_restore (layer, layer->offset_x, layer->offset_y, layer->width, layer->height);
  layer->fs.initial = TRUE;

  if (undo)
    undo_push_fs_relax (gimage, layer->ID);
}

void
floating_sel_composite (Layer *layer,
			int    x,
			int    y,
			int    w,
			int    h,
			int    undo)
{
  PixelRegion fsPR;
  GImage *gimage;
  Layer *d_layer;
  int preserve_trans;
  int active[MAX_CHANNELS];
  int offx, offy;
  int x1, y1, x2, y2;
  int i;

  d_layer = NULL;

  if (! (gimage = drawable_gimage (layer->ID)))
    return;

  /*  What this function does is composite the specified area of the
   *  drawble with the floating selection.  We do this when the gimage
   *  is constructed, before any other composition takes place.
   */

  /*  If this isn't the first composite, restore the image underneath  */
  if (! layer->fs.initial)
    floating_sel_restore (layer, x, y, w, h);
  else if (layer->visible)
    layer->fs.initial = FALSE;

  /*  First restore what's behind the image if necessary, then check for visibility  */
  if (layer->visible)
    {
      /*  Find the minimum area we need to composite -- in gimage space  */
      drawable_offsets (layer->fs.drawable, &offx, &offy);
      x1 = MAXIMUM (layer->offset_x, offx);
      y1 = MAXIMUM (layer->offset_y, offy);
      x2 = MINIMUM (layer->offset_x + layer->width, offx + drawable_width (layer->fs.drawable));
      y2 = MINIMUM (layer->offset_y + layer->height, offy + drawable_height (layer->fs.drawable));

      x1 = BOUNDS (x, x1, x2);
      y1 = BOUNDS (y, y1, y2);
      x2 = BOUNDS (x + w, x1, x2);
      y2 = BOUNDS (y + h, y1, y2);

      if ((x2 - x1) > 0 && (y2 - y1) > 0)
	{
	  /*  composite the area from the layer to the drawable  */
	  pixel_region_init (&fsPR, layer->tiles,
			     (x1 - layer->offset_x), (y1 - layer->offset_y),
			     (x2 - x1), (y2 - y1), FALSE);

	  /*  a kludge here to prevent the case of the drawable
	   *  underneath having preserve transparency on, and disallowing
	   *  the composited floating selection from being shown
	   */
	  if (drawable_layer (layer->fs.drawable))
	    {
	      d_layer = layer_get_ID (layer->fs.drawable);
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
	  gimage_apply_image (gimage, layer->fs.drawable, &fsPR,
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
floating_sel_boundary (Layer *layer,
		       int   *num_segs)
{
  PixelRegion bPR;
  int i;

  if (layer->fs.boundary_known == FALSE)
    {
      if (layer->fs.segs)
	g_free (layer->fs.segs);

      /*  find the segments  */
      pixel_region_init (&bPR, layer->tiles, 0, 0, layer->width, layer->height, FALSE);
      layer->fs.segs = find_mask_boundary (&bPR, &layer->fs.num_segs,
					   WithinBounds, 0, 0,
					   layer->width, layer->height);

      /*  offset the segments  */
      for (i = 0; i < layer->fs.num_segs; i++)
	{
	  layer->fs.segs[i].x1 += layer->offset_x;
	  layer->fs.segs[i].y1 += layer->offset_y;
	  layer->fs.segs[i].x2 += layer->offset_x;
	  layer->fs.segs[i].y2 += layer->offset_y;
	}

      layer->fs.boundary_known = TRUE;
    }

  *num_segs = layer->fs.num_segs;
  return layer->fs.segs;
}

void
floating_sel_invalidate (Layer *layer)
{
  /*  Invalidate the attached-to drawable's preview  */
  drawable_invalidate_preview (layer->fs.drawable);

  /*  Invalidate the boundary  */
  layer->fs.boundary_known = FALSE;
}
