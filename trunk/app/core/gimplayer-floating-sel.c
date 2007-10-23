/* GIMP - The GNU Image Manipulation Program
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

#include "libgimpmath/gimpmath.h"

#include "core-types.h"

#include "base/boundary.h"
#include "base/pixel-region.h"
#include "base/tile-manager.h"

#include "paint-funcs/paint-funcs.h"

#include "gimp.h"
#include "gimpimage.h"
#include "gimpimage-undo.h"
#include "gimpimage-undo-push.h"
#include "gimplayer.h"
#include "gimplayer-floating-sel.h"
#include "gimplayermask.h"

#include "gimp-intl.h"


void
floating_sel_attach (GimpLayer    *layer,
                     GimpDrawable *drawable)
{
  GimpImage *image;
  GimpLayer *floating_sel;

  g_return_if_fail (GIMP_IS_LAYER (layer));
  g_return_if_fail (GIMP_IS_DRAWABLE (drawable));
  g_return_if_fail (gimp_item_is_attached (GIMP_ITEM (drawable)));
  g_return_if_fail (drawable != GIMP_DRAWABLE (layer));
  g_return_if_fail (gimp_item_get_image (GIMP_ITEM (layer)) ==
                    gimp_item_get_image (GIMP_ITEM (drawable)));

  image = gimp_item_get_image (GIMP_ITEM (drawable));

  floating_sel = gimp_image_floating_sel (image);

  /*  If there is already a floating selection, anchor it  */
  if (floating_sel)
    {
      floating_sel_anchor (floating_sel);

      /*  if we were pasting to the old floating selection, paste now
       *  to the drawable
       */
      if (drawable == (GimpDrawable *) floating_sel)
        drawable = gimp_image_get_active_drawable (image);
    }

  /*  set the drawable and allocate a backing store  */
  gimp_layer_set_lock_alpha (layer, TRUE, FALSE);
  layer->fs.drawable      = drawable;
  layer->fs.backing_store = tile_manager_new (GIMP_ITEM (layer)->width,
                                              GIMP_ITEM (layer)->height,
                                              gimp_drawable_bytes (drawable));

  /*  add the layer to the image  */
  gimp_image_add_layer (image, layer, 0);

  /*  store the affected area from the drawable in the backing store  */
  floating_sel_rigor (layer, TRUE);
}

void
floating_sel_remove (GimpLayer *layer)
{
  GimpImage *image;

  g_return_if_fail (GIMP_IS_LAYER (layer));
  g_return_if_fail (gimp_layer_is_floating_sel (layer));

  image = gimp_item_get_image (GIMP_ITEM (layer->fs.drawable));

  gimp_image_undo_group_start (image, GIMP_UNDO_GROUP_FS_REMOVE,
                               _("Remove Floating Selection"));

  /*  store the affected area from the drawable in the backing store  */
  floating_sel_relax (layer, TRUE);

  /*  Invalidate the preview of the obscured drawable.  We do this here
   *  because it will not be done until the floating selection is removed,
   *  at which point the obscured drawable's preview will not be declared
   *  invalid.
   */
  gimp_viewable_invalidate_preview (GIMP_VIEWABLE (layer));

  /*  remove the layer from the image  */
  gimp_image_remove_layer (image, layer);

  gimp_image_undo_group_end (image);
}

void
floating_sel_anchor (GimpLayer *layer)
{
  GimpImage    *image;
  GimpDrawable *drawable;

  g_return_if_fail (GIMP_IS_LAYER (layer));
  g_return_if_fail (gimp_layer_is_floating_sel (layer));

  image = gimp_item_get_image (GIMP_ITEM (layer));

  if (! gimp_layer_is_floating_sel (layer))
    {
      gimp_message (image->gimp, NULL, GIMP_MESSAGE_WARNING,
                    _("Cannot anchor this layer because "
                      "it is not a floating selection."));
      return;
    }

  /*  Start a floating selection anchoring undo  */
  gimp_image_undo_group_start (image, GIMP_UNDO_GROUP_FS_ANCHOR,
                               _("Anchor Floating Selection"));

  /* Invalidate the previews of the layer that will be composited
   * with the floating section.
   */
  gimp_viewable_invalidate_preview (GIMP_VIEWABLE (layer->fs.drawable));

  /*  Relax the floating selection  */
  floating_sel_relax (layer, TRUE);

  /*  Composite the floating selection contents  */
  floating_sel_composite (layer,
                          GIMP_ITEM (layer)->offset_x,
                          GIMP_ITEM (layer)->offset_y,
                          GIMP_ITEM (layer)->width,
                          GIMP_ITEM (layer)->height, TRUE);

  drawable = layer->fs.drawable;

  /*  remove the floating selection  */
  gimp_image_remove_layer (image, layer);

  /*  end the group undo  */
  gimp_image_undo_group_end (image);

  /*  invalidate the boundaries  */
  gimp_drawable_invalidate_boundary (GIMP_DRAWABLE (gimp_image_get_mask (image)));
}

void
floating_sel_activate_drawable (GimpLayer *layer)
{
  GimpImage *image;

  g_return_if_fail (GIMP_IS_LAYER (layer));
  g_return_if_fail (gimp_layer_is_floating_sel (layer));

  image = gimp_item_get_image (GIMP_ITEM (layer));

  /*  set the underlying drawable to active  */
  if (GIMP_IS_LAYER_MASK (layer->fs.drawable))
    {
      GimpLayerMask *mask = GIMP_LAYER_MASK (layer->fs.drawable);

      gimp_image_set_active_layer (image, gimp_layer_mask_get_layer (mask));
    }
  else if (GIMP_IS_CHANNEL (layer->fs.drawable))
    {
      gimp_image_set_active_channel (image, GIMP_CHANNEL (layer->fs.drawable));
    }
  else
    {
      gimp_image_set_active_layer (image, GIMP_LAYER (layer->fs.drawable));
    }
}

void
floating_sel_to_layer (GimpLayer *layer)
{
  GimpItem  *item;
  GimpImage *image;

  g_return_if_fail (GIMP_IS_LAYER (layer));
  g_return_if_fail (gimp_layer_is_floating_sel (layer));

  item = GIMP_ITEM (layer);

  if (! (image = gimp_item_get_image (item)))
    return;

  /*  Check if the floating layer belongs to a channel...  */
  if (GIMP_IS_CHANNEL (layer->fs.drawable))
    {
      gimp_message (image->gimp, NULL, GIMP_MESSAGE_WARNING,
                    _("Cannot create a new layer from the floating selection "
                      "because it belongs to a layer mask or channel."));
      return;
    }

  gimp_image_undo_group_start (image, GIMP_UNDO_GROUP_FS_TO_LAYER,
                               _("Floating Selection to Layer"));

  /*  restore the contents of the drawable  */
  floating_sel_restore (layer,
                        item->offset_x,
                        item->offset_y,
                        item->width,
                        item->height);

  gimp_image_undo_push_fs_to_layer (image, NULL, layer);

  /*  clear the selection  */
  gimp_drawable_invalidate_boundary (GIMP_DRAWABLE (layer));

  /*  Set pointers  */
  layer->fs.drawable   = NULL;
  image->floating_sel = NULL;

  gimp_item_set_visible (GIMP_ITEM (layer), TRUE, TRUE);
  gimp_layer_set_lock_alpha (layer, FALSE, TRUE);

  gimp_image_undo_group_end (image);

  gimp_object_name_changed (GIMP_OBJECT (layer));

  gimp_drawable_update (GIMP_DRAWABLE (layer),
                        0, 0,
                        GIMP_ITEM (layer)->width,
                        GIMP_ITEM (layer)->height);

  gimp_image_floating_selection_changed (image);
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

  g_return_if_fail (GIMP_IS_LAYER (layer));
  g_return_if_fail (gimp_layer_is_floating_sel (layer));

  /*  Check the backing store & make sure it has the correct dimensions  */
  if ((tile_manager_width  (layer->fs.backing_store) !=
       gimp_item_width (GIMP_ITEM(layer)))  ||
      (tile_manager_height (layer->fs.backing_store) !=
       gimp_item_height (GIMP_ITEM(layer))) ||
      (tile_manager_bpp    (layer->fs.backing_store) !=
       gimp_drawable_bytes (layer->fs.drawable)))
    {
      /*  free the backing store and allocate anew  */
      tile_manager_unref (layer->fs.backing_store);

      layer->fs.backing_store =
        tile_manager_new (GIMP_ITEM (layer)->width,
                          GIMP_ITEM (layer)->height,
                          gimp_drawable_bytes (layer->fs.drawable));
    }

  /*  What this function does is save the specified area of the
   *  drawable that this floating selection obscures.  We do this so
   *  that it will be possible to subsequently restore the drawable's area
   */
  gimp_item_offsets (GIMP_ITEM (layer->fs.drawable), &offx, &offy);

  /*  Find the minimum area we need to uncover -- in image space  */
  x1 = MAX (GIMP_ITEM (layer)->offset_x, offx);
  y1 = MAX (GIMP_ITEM (layer)->offset_y, offy);
  x2 = MIN (GIMP_ITEM (layer)->offset_x + GIMP_ITEM (layer)->width,
            offx + gimp_item_width (GIMP_ITEM (layer->fs.drawable)));
  y2 = MIN (GIMP_ITEM (layer)->offset_y + GIMP_ITEM (layer)->height,
            offy + gimp_item_height (GIMP_ITEM (layer->fs.drawable)));

  x1 = CLAMP (x, x1, x2);
  y1 = CLAMP (y, y1, y2);
  x2 = CLAMP (x + w, x1, x2);
  y2 = CLAMP (y + h, y1, y2);

  if ((x2 - x1) > 0 && (y2 - y1) > 0)
    {
      /*  Copy the area from the drawable to the backing store  */
      pixel_region_init (&srcPR, gimp_drawable_get_tiles (layer->fs.drawable),
                         (x1 - offx), (y1 - offy), (x2 - x1), (y2 - y1), FALSE);
      pixel_region_init (&destPR, layer->fs.backing_store,
                         (x1 - GIMP_ITEM (layer)->offset_x),
                         (y1 - GIMP_ITEM (layer)->offset_y),
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

  g_return_if_fail (GIMP_IS_LAYER (layer));
  g_return_if_fail (gimp_layer_is_floating_sel (layer));

  /*  What this function does is "uncover" the specified area in the
   *  drawable that this floating selection obscures.  We do this so
   *  that either the floating selection can be removed or it can be
   *  translated
   */

  /*  Find the minimum area we need to uncover -- in image space  */
  gimp_item_offsets (GIMP_ITEM (layer->fs.drawable), &offx, &offy);

  x1 = MAX (GIMP_ITEM (layer)->offset_x, offx);
  y1 = MAX (GIMP_ITEM (layer)->offset_y, offy);
  x2 = MIN (GIMP_ITEM (layer)->offset_x + GIMP_ITEM (layer)->width,
            offx + gimp_item_width  (GIMP_ITEM (layer->fs.drawable)));
  y2 = MIN (GIMP_ITEM(layer)->offset_y + GIMP_ITEM (layer)->height,
            offy + gimp_item_height (GIMP_ITEM (layer->fs.drawable)));

  x1 = CLAMP (x, x1, x2);
  y1 = CLAMP (y, y1, y2);
  x2 = CLAMP (x + w, x1, x2);
  y2 = CLAMP (y + h, y1, y2);

  if ((x2 - x1) > 0 && (y2 - y1) > 0)
    {
      /*  Copy the area from the backing store to the drawable  */
      pixel_region_init (&srcPR, layer->fs.backing_store,
                         (x1 - GIMP_ITEM (layer)->offset_x),
                         (y1 - GIMP_ITEM (layer)->offset_y),
                         (x2 - x1), (y2 - y1), FALSE);
      pixel_region_init (&destPR, gimp_drawable_get_tiles (layer->fs.drawable),
                         (x1 - offx), (y1 - offy), (x2 - x1), (y2 - y1), TRUE);

      copy_region (&srcPR, &destPR);
    }
}

void
floating_sel_rigor (GimpLayer *layer,
                    gboolean   push_undo)
{
  g_return_if_fail (GIMP_IS_LAYER (layer));
  g_return_if_fail (gimp_layer_is_floating_sel (layer));

  /*  store the affected area from the drawable in the backing store  */
  floating_sel_store (layer,
                      GIMP_ITEM (layer)->offset_x,
                      GIMP_ITEM (layer)->offset_y,
                      GIMP_ITEM (layer)->width,
                      GIMP_ITEM (layer)->height);
  layer->fs.initial = TRUE;

  if (push_undo)
    gimp_image_undo_push_fs_rigor (gimp_item_get_image (GIMP_ITEM (layer)),
                                   NULL,
                                   layer);
}

void
floating_sel_relax (GimpLayer *layer,
                    gboolean   push_undo)
{
  g_return_if_fail (GIMP_IS_LAYER (layer));
  g_return_if_fail (gimp_layer_is_floating_sel (layer));

  /*  restore the contents of drawable the floating layer is attached to  */
  if (layer->fs.initial == FALSE)
    floating_sel_restore (layer,
                          GIMP_ITEM (layer)->offset_x,
                          GIMP_ITEM (layer)->offset_y,
                          GIMP_ITEM (layer)->width,
                          GIMP_ITEM (layer)->height);
  layer->fs.initial = TRUE;

  if (push_undo)
    gimp_image_undo_push_fs_relax (gimp_item_get_image (GIMP_ITEM (layer)),
                                   NULL,
                                   layer);
}

void
floating_sel_composite (GimpLayer *layer,
                        gint       x,
                        gint       y,
                        gint       w,
                        gint       h,
                        gboolean   push_undo)
{
  GimpImage *image;

  g_return_if_fail (GIMP_IS_LAYER (layer));
  g_return_if_fail (gimp_layer_is_floating_sel (layer));

  if (! (image = gimp_item_get_image (GIMP_ITEM (layer))))
    return;

  /*  What this function does is composite the specified area of the
   *  drawble with the floating selection.  We do this when the image
   *  is constructed, before any other composition takes place.
   */

  /*  If this isn't the first composite,
   *  restore the image underneath
   */
  if (! layer->fs.initial)
    floating_sel_restore (layer, x, y, w, h);
  else if (gimp_item_get_visible (GIMP_ITEM (layer)))
    layer->fs.initial = FALSE;

  /*  First restore what's behind the image if necessary,
   *  then check for visibility
   */
  if (gimp_item_get_visible (GIMP_ITEM (layer)))
    {
      gint offx, offy;
      gint x1, y1, x2, y2;

      /*  Find the minimum area we need to composite -- in image space  */
      gimp_item_offsets (GIMP_ITEM (layer->fs.drawable), &offx, &offy);

      x1 = MAX (GIMP_ITEM (layer)->offset_x, offx);
      y1 = MAX (GIMP_ITEM (layer)->offset_y, offy);
      x2 = MIN (GIMP_ITEM (layer)->offset_x +
                GIMP_ITEM (layer)->width,
                offx + gimp_item_width  (GIMP_ITEM (layer->fs.drawable)));
      y2 = MIN (GIMP_ITEM (layer)->offset_y +
                GIMP_ITEM (layer)->height,
                offy + gimp_item_height (GIMP_ITEM (layer->fs.drawable)));

      x1 = CLAMP (x, x1, x2);
      y1 = CLAMP (y, y1, y2);
      x2 = CLAMP (x + w, x1, x2);
      y2 = CLAMP (y + h, y1, y2);

      if ((x2 - x1) > 0 && (y2 - y1) > 0)
        {
          PixelRegion  fsPR;
          GimpLayer   *d_layer = NULL;
          gboolean     lock_alpha;

          /*  composite the area from the layer to the drawable  */
          pixel_region_init (&fsPR, GIMP_DRAWABLE (layer)->tiles,
                             (x1 - GIMP_ITEM (layer)->offset_x),
                             (y1 - GIMP_ITEM (layer)->offset_y),
                             (x2 - x1), (y2 - y1), FALSE);

          /*  a kludge here to prevent the case of the drawable
           *  underneath having lock alpha on, and disallowing
           *  the composited floating selection from being shown
           */
          if (GIMP_IS_LAYER (layer->fs.drawable))
            {
              d_layer = GIMP_LAYER (layer->fs.drawable);
              if ((lock_alpha = gimp_layer_get_lock_alpha (d_layer)))
                gimp_layer_set_lock_alpha (d_layer, FALSE, FALSE);
            }
          else
            lock_alpha = FALSE;

          /*  apply the fs with the undo specified by the value
           *  passed to this function
           */
          gimp_drawable_apply_region (layer->fs.drawable, &fsPR,
                                      push_undo, NULL,
                                      layer->opacity,
                                      layer->mode,
                                      NULL,
                                      (x1 - offx), (y1 - offy));

          /*  restore lock alpha  */
          if (lock_alpha)
            gimp_layer_set_lock_alpha (d_layer, TRUE, FALSE);
        }
    }
}

const BoundSeg *
floating_sel_boundary (GimpLayer *layer,
                       gint      *n_segs)
{
  PixelRegion bPR;
  gint        i;

  g_return_val_if_fail (GIMP_IS_LAYER (layer), NULL);
  g_return_val_if_fail (gimp_layer_is_floating_sel (layer), NULL);
  g_return_val_if_fail (n_segs != NULL, NULL);

  if (layer->fs.boundary_known == FALSE)
    {
      gint width, height;
      gint off_x, off_y;

      width  = gimp_item_width  (GIMP_ITEM (layer));
      height = gimp_item_height (GIMP_ITEM (layer));
      gimp_item_offsets (GIMP_ITEM (layer), &off_x, &off_y);

      if (layer->fs.segs)
        g_free (layer->fs.segs);

      if (gimp_drawable_has_alpha (GIMP_DRAWABLE (layer)))
        {
          /*  find the segments  */
          pixel_region_init (&bPR,
                             gimp_drawable_get_tiles (GIMP_DRAWABLE (layer)),
                             0, 0, width, height, FALSE);
          layer->fs.segs = boundary_find (&bPR, BOUNDARY_WITHIN_BOUNDS,
                                          0, 0, width, height,
                                          BOUNDARY_HALF_WAY,
                                          &layer->fs.num_segs);

          /*  offset the segments  */
          for (i = 0; i < layer->fs.num_segs; i++)
            {
              layer->fs.segs[i].x1 += off_x;
              layer->fs.segs[i].y1 += off_y;
              layer->fs.segs[i].x2 += off_x;
              layer->fs.segs[i].y2 += off_y;
            }
        }
      else
        {
          layer->fs.num_segs = 4;
          layer->fs.segs     = g_new0 (BoundSeg, 4);

          /* top */
          layer->fs.segs[0].x1 = off_x;
          layer->fs.segs[0].y1 = off_y;
          layer->fs.segs[0].x2 = off_x + width;
          layer->fs.segs[0].y2 = off_y;

          /* left */
          layer->fs.segs[1].x1 = off_x;
          layer->fs.segs[1].y1 = off_y;
          layer->fs.segs[1].x2 = off_x;
          layer->fs.segs[1].y2 = off_y + height;

          /* right */
          layer->fs.segs[2].x1 = off_x + width;
          layer->fs.segs[2].y1 = off_y;
          layer->fs.segs[2].x2 = off_x + width;
          layer->fs.segs[2].y2 = off_y + height;

          /* bottom */
          layer->fs.segs[3].x1 = off_x;
          layer->fs.segs[3].y1 = off_y + height;
          layer->fs.segs[3].x2 = off_x + width;
          layer->fs.segs[3].y2 = off_y + height;
        }

      layer->fs.boundary_known = TRUE;
    }

  *n_segs = layer->fs.num_segs;

  return layer->fs.segs;
}

void
floating_sel_invalidate (GimpLayer *layer)
{
  g_return_if_fail (GIMP_IS_LAYER (layer));
  g_return_if_fail (gimp_layer_is_floating_sel (layer));

  /*  Invalidate the attached-to drawable's preview  */
  gimp_viewable_invalidate_preview (GIMP_VIEWABLE (layer->fs.drawable));

  /*  Invalidate the boundary  */
  layer->fs.boundary_known = FALSE;
}
