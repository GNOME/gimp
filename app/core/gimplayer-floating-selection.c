/* GIMP - The GNU Image Manipulation Program
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <https://www.gnu.org/licenses/>.
 */

#include "config.h"

#include <gdk-pixbuf/gdk-pixbuf.h>
#include <gegl.h>

#include "libgimpbase/gimpbase.h"

#include "core-types.h"

#include "gimpboundary.h"
#include "gimpdrawable-filters.h"
#include "gimpdrawable-floating-selection.h"
#include "gimperror.h"
#include "gimpimage.h"
#include "gimpimage-undo.h"
#include "gimpimage-undo-push.h"
#include "gimplayer.h"
#include "gimplayer-floating-selection.h"
#include "gimplayermask.h"

#include "gimp-intl.h"


/* public functions  */

void
floating_sel_attach (GimpLayer    *layer,
                     GimpDrawable *drawable)
{
  GimpImage *image;
  GimpLayer *floating_sel;
  GimpLayer *parent   = NULL;
  gint       position = 0;

  g_return_if_fail (GIMP_IS_LAYER (layer));
  g_return_if_fail (GIMP_IS_DRAWABLE (drawable));
  g_return_if_fail (gimp_item_is_attached (GIMP_ITEM (drawable)));
  g_return_if_fail (drawable != GIMP_DRAWABLE (layer));
  g_return_if_fail (gimp_item_get_image (GIMP_ITEM (layer)) ==
                    gimp_item_get_image (GIMP_ITEM (drawable)));

  image = gimp_item_get_image (GIMP_ITEM (drawable));

  floating_sel = gimp_image_get_floating_selection (image);

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

  gimp_layer_set_lock_alpha (layer, TRUE, FALSE);

  gimp_layer_set_floating_sel_drawable (layer, drawable);

  /*  Floating selection layer placement, default to the top of the
   *  layers stack; parent and position are adapted according to the
   *  drawable associated with the floating selection.
   */

  if (GIMP_IS_LAYER_MASK (drawable))
    {
       GimpLayer *tmp = gimp_layer_mask_get_layer (GIMP_LAYER_MASK (drawable));

       parent   = GIMP_LAYER (gimp_item_get_parent (GIMP_ITEM (tmp)));
       position = gimp_item_get_index (GIMP_ITEM (tmp));
    }
  else if (GIMP_IS_LAYER (drawable))
    {
      parent   = GIMP_LAYER (gimp_item_get_parent (GIMP_ITEM (drawable)));
      position = gimp_item_get_index (GIMP_ITEM (drawable));
    }

  gimp_image_add_layer (image, layer, parent, position, TRUE);
}

void
floating_sel_anchor (GimpLayer *layer)
{
  GimpImage     *image;
  GimpDrawable  *drawable;
  GimpFilter    *filter = NULL;
  GeglRectangle  bounding_box;
  GeglRectangle  dr_bounding_box;
  gint           off_x, off_y;
  gint           dr_off_x, dr_off_y;

  g_return_if_fail (GIMP_IS_LAYER (layer));
  g_return_if_fail (gimp_layer_is_floating_sel (layer));

  /* Don't let gimp_image_remove_layer free the layer while we still need it */
  g_object_ref (layer);

  image = gimp_item_get_image (GIMP_ITEM (layer));

  gimp_image_undo_group_start (image, GIMP_UNDO_GROUP_FS_ANCHOR,
                               C_("undo-type", "Anchor Floating Selection"));

  drawable = gimp_layer_get_floating_sel_drawable (layer);

  gimp_item_get_offset (GIMP_ITEM (layer), &off_x, &off_y);
  gimp_item_get_offset (GIMP_ITEM (drawable), &dr_off_x, &dr_off_y);

  bounding_box    = gimp_drawable_get_bounding_box (GIMP_DRAWABLE (layer));
  dr_bounding_box = gimp_drawable_get_bounding_box (drawable);

  bounding_box.x    += off_x;
  bounding_box.y    += off_y;

  dr_bounding_box.x += dr_off_x;
  dr_bounding_box.y += dr_off_y;

  if (gimp_item_get_visible (GIMP_ITEM (layer)) &&
      gegl_rectangle_intersect (NULL, &bounding_box, &dr_bounding_box))
    {
      filter = gimp_drawable_get_floating_sel_filter (drawable);
    }

  if (filter)
    {
      gimp_drawable_merge_filter (drawable, filter, NULL, NULL,
                                  NULL, FALSE, FALSE, FALSE);
    }

  gimp_image_remove_layer (image, layer, TRUE, NULL);

  gimp_image_undo_group_end (image);

  /*  invalidate the boundaries  */
  gimp_drawable_invalidate_boundary (GIMP_DRAWABLE (gimp_image_get_mask (image)));

  g_object_unref (layer);
}

gboolean
floating_sel_to_layer (GimpLayer  *layer,
                       GError    **error)
{
  GimpItem  *item;
  GimpImage *image;

  g_return_val_if_fail (GIMP_IS_LAYER (layer), FALSE);
  g_return_val_if_fail (gimp_layer_is_floating_sel (layer), FALSE);
  g_return_val_if_fail (error == NULL || *error == NULL, FALSE);

  item  = GIMP_ITEM (layer);
  image = gimp_item_get_image (item);

  /*  Check if the floating layer belongs to a channel  */
  if (GIMP_IS_CHANNEL (gimp_layer_get_floating_sel_drawable (layer)))
    {
      g_set_error_literal (error, GIMP_ERROR, GIMP_FAILED,
                           _("Cannot create a new layer from the floating "
                             "selection because it belongs to a layer mask "
                             "or channel."));
      return FALSE;
    }

  gimp_image_undo_group_start (image, GIMP_UNDO_GROUP_FS_TO_LAYER,
                               C_("undo-type", "Floating Selection to Layer"));

  gimp_image_undo_push_fs_to_layer (image, NULL, layer);

  gimp_drawable_detach_floating_sel (gimp_layer_get_floating_sel_drawable (layer));
  gimp_layer_set_floating_sel_drawable (layer, NULL);

  gimp_item_set_visible (item, TRUE, TRUE);
  gimp_layer_set_lock_alpha (layer, FALSE, TRUE);

  gimp_image_undo_group_end (image);

  /* When the floating selection is converted to/from a normal layer
   * it does something resembling a name change, so emit the
   * "name-changed" signal
   */
  gimp_object_name_changed (GIMP_OBJECT (layer));

  gimp_drawable_update (GIMP_DRAWABLE (layer),
                        0, 0,
                        gimp_item_get_width  (item),
                        gimp_item_get_height (item));

  return TRUE;
}

void
floating_sel_activate_drawable (GimpLayer *layer)
{
  GimpImage    *image;
  GimpDrawable *drawable;

  g_return_if_fail (GIMP_IS_LAYER (layer));
  g_return_if_fail (gimp_layer_is_floating_sel (layer));

  image = gimp_item_get_image (GIMP_ITEM (layer));

  drawable = gimp_layer_get_floating_sel_drawable (layer);

  /*  set the underlying drawable to active  */
  if (GIMP_IS_LAYER_MASK (drawable))
    {
      GimpLayerMask *mask = GIMP_LAYER_MASK (drawable);

      gimp_image_set_active_layer (image, gimp_layer_mask_get_layer (mask));
    }
  else if (GIMP_IS_CHANNEL (drawable))
    {
      gimp_image_set_active_channel (image, GIMP_CHANNEL (drawable));
    }
  else
    {
      gimp_image_set_active_layer (image, GIMP_LAYER (drawable));
    }
}

const GimpBoundSeg *
floating_sel_boundary (GimpLayer *layer,
                       gint      *n_segs)
{
  g_return_val_if_fail (GIMP_IS_LAYER (layer), NULL);
  g_return_val_if_fail (gimp_layer_is_floating_sel (layer), NULL);
  g_return_val_if_fail (n_segs != NULL, NULL);

  if (layer->fs.boundary_known == FALSE)
    {
      gint width, height;
      gint off_x, off_y;

      width  = gimp_item_get_width  (GIMP_ITEM (layer));
      height = gimp_item_get_height (GIMP_ITEM (layer));
      gimp_item_get_offset (GIMP_ITEM (layer), &off_x, &off_y);

      if (layer->fs.segs)
        g_free (layer->fs.segs);

      if (gimp_drawable_has_alpha (GIMP_DRAWABLE (layer)))
        {
          GeglBuffer *buffer;
          gint        i;

          /*  find the segments  */
          buffer = gimp_drawable_get_buffer (GIMP_DRAWABLE (layer));

          layer->fs.segs = gimp_boundary_find (buffer, NULL,
                                               babl_format ("A float"),
                                               GIMP_BOUNDARY_WITHIN_BOUNDS,
                                               0, 0, width, height,
                                               GIMP_BOUNDARY_HALF_WAY,
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
          layer->fs.segs     = g_new0 (GimpBoundSeg, 4);

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
  gimp_viewable_invalidate_preview (GIMP_VIEWABLE (gimp_layer_get_floating_sel_drawable (layer)));

  /*  Invalidate the boundary  */
  layer->fs.boundary_known = FALSE;
}
