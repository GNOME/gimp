/* LIGMA - The GNU Image Manipulation Program
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

#include "libligmabase/ligmabase.h"

#include "core-types.h"

#include "ligmaboundary.h"
#include "ligmadrawable-filters.h"
#include "ligmadrawable-floating-selection.h"
#include "ligmaerror.h"
#include "ligmaimage.h"
#include "ligmaimage-undo.h"
#include "ligmaimage-undo-push.h"
#include "ligmalayer.h"
#include "ligmalayer-floating-selection.h"
#include "ligmalayermask.h"

#include "ligma-intl.h"


/* public functions  */

void
floating_sel_attach (LigmaLayer    *layer,
                     LigmaDrawable *drawable)
{
  LigmaImage *image;
  LigmaLayer *floating_sel;
  LigmaLayer *parent   = NULL;
  gint       position = 0;

  g_return_if_fail (LIGMA_IS_LAYER (layer));
  g_return_if_fail (LIGMA_IS_DRAWABLE (drawable));
  g_return_if_fail (ligma_item_is_attached (LIGMA_ITEM (drawable)));
  g_return_if_fail (drawable != LIGMA_DRAWABLE (layer));
  g_return_if_fail (ligma_item_get_image (LIGMA_ITEM (layer)) ==
                    ligma_item_get_image (LIGMA_ITEM (drawable)));

  image = ligma_item_get_image (LIGMA_ITEM (drawable));

  floating_sel = ligma_image_get_floating_selection (image);

  /*  If there is already a floating selection, anchor it  */
  if (floating_sel)
    {
      floating_sel_anchor (floating_sel);

      /*  if we were pasting to the old floating selection, paste now
       *  to the drawable
       */
      if (drawable == (LigmaDrawable *) floating_sel)
        {
          GList *drawables = ligma_image_get_selected_drawables (image);

          g_return_if_fail (g_list_length (drawables) == 1);
          drawable = drawables->data;
          g_list_free (drawables);
        }
    }

  ligma_layer_set_lock_alpha (layer, TRUE, FALSE);

  ligma_layer_set_floating_sel_drawable (layer, drawable);

  /*  Floating selection layer placement, default to the top of the
   *  layers stack; parent and position are adapted according to the
   *  drawable associated with the floating selection.
   */

  if (LIGMA_IS_LAYER_MASK (drawable))
    {
       LigmaLayer *tmp = ligma_layer_mask_get_layer (LIGMA_LAYER_MASK (drawable));

       parent   = LIGMA_LAYER (ligma_item_get_parent (LIGMA_ITEM (tmp)));
       position = ligma_item_get_index (LIGMA_ITEM (tmp));
    }
  else if (LIGMA_IS_LAYER (drawable))
    {
      parent   = LIGMA_LAYER (ligma_item_get_parent (LIGMA_ITEM (drawable)));
      position = ligma_item_get_index (LIGMA_ITEM (drawable));
    }

  ligma_image_add_layer (image, layer, parent, position, TRUE);
}

void
floating_sel_anchor (LigmaLayer *layer)
{
  LigmaImage     *image;
  LigmaDrawable  *drawable;
  LigmaFilter    *filter = NULL;
  GeglRectangle  bounding_box;
  GeglRectangle  dr_bounding_box;
  gint           off_x, off_y;
  gint           dr_off_x, dr_off_y;

  g_return_if_fail (LIGMA_IS_LAYER (layer));
  g_return_if_fail (ligma_layer_is_floating_sel (layer));

  /* Don't let ligma_image_remove_layer free the layer while we still need it */
  g_object_ref (layer);

  image = ligma_item_get_image (LIGMA_ITEM (layer));

  ligma_image_undo_group_start (image, LIGMA_UNDO_GROUP_FS_ANCHOR,
                               C_("undo-type", "Anchor Floating Selection"));

  drawable = ligma_layer_get_floating_sel_drawable (layer);

  ligma_item_get_offset (LIGMA_ITEM (layer), &off_x, &off_y);
  ligma_item_get_offset (LIGMA_ITEM (drawable), &dr_off_x, &dr_off_y);

  bounding_box    = ligma_drawable_get_bounding_box (LIGMA_DRAWABLE (layer));
  dr_bounding_box = ligma_drawable_get_bounding_box (drawable);

  bounding_box.x    += off_x;
  bounding_box.y    += off_y;

  dr_bounding_box.x += dr_off_x;
  dr_bounding_box.y += dr_off_y;

  if (ligma_item_get_visible (LIGMA_ITEM (layer)) &&
      gegl_rectangle_intersect (NULL, &bounding_box, &dr_bounding_box))
    {
      filter = ligma_drawable_get_floating_sel_filter (drawable);
    }

  if (filter)
    {
      ligma_drawable_merge_filter (drawable, filter, NULL, NULL,
                                  NULL, FALSE, FALSE, FALSE);
    }

  ligma_image_remove_layer (image, layer, TRUE, NULL);

  ligma_image_undo_group_end (image);

  /*  invalidate the boundaries  */
  ligma_drawable_invalidate_boundary (LIGMA_DRAWABLE (ligma_image_get_mask (image)));

  g_object_unref (layer);
}

gboolean
floating_sel_to_layer (LigmaLayer  *layer,
                       GError    **error)
{
  LigmaItem  *item;
  LigmaImage *image;

  g_return_val_if_fail (LIGMA_IS_LAYER (layer), FALSE);
  g_return_val_if_fail (ligma_layer_is_floating_sel (layer), FALSE);
  g_return_val_if_fail (error == NULL || *error == NULL, FALSE);

  item  = LIGMA_ITEM (layer);
  image = ligma_item_get_image (item);

  /*  Check if the floating layer belongs to a channel  */
  if (LIGMA_IS_CHANNEL (ligma_layer_get_floating_sel_drawable (layer)))
    {
      g_set_error_literal (error, LIGMA_ERROR, LIGMA_FAILED,
                           _("Cannot create a new layer from the floating "
                             "selection because it belongs to a layer mask "
                             "or channel."));
      return FALSE;
    }

  ligma_image_undo_group_start (image, LIGMA_UNDO_GROUP_FS_TO_LAYER,
                               C_("undo-type", "Floating Selection to Layer"));

  ligma_image_undo_push_fs_to_layer (image, NULL, layer);

  ligma_drawable_detach_floating_sel (ligma_layer_get_floating_sel_drawable (layer));
  ligma_layer_set_floating_sel_drawable (layer, NULL);

  ligma_item_set_visible (item, TRUE, TRUE);
  ligma_layer_set_lock_alpha (layer, FALSE, TRUE);

  ligma_image_undo_group_end (image);

  /* When the floating selection is converted to/from a normal layer
   * it does something resembling a name change, so emit the
   * "name-changed" signal
   */
  ligma_object_name_changed (LIGMA_OBJECT (layer));

  ligma_drawable_update (LIGMA_DRAWABLE (layer),
                        0, 0,
                        ligma_item_get_width  (item),
                        ligma_item_get_height (item));

  return TRUE;
}

void
floating_sel_activate_drawable (LigmaLayer *layer)
{
  LigmaImage    *image;
  LigmaDrawable *drawable;
  GList        *selected_drawables;

  g_return_if_fail (LIGMA_IS_LAYER (layer));
  g_return_if_fail (ligma_layer_is_floating_sel (layer));

  image = ligma_item_get_image (LIGMA_ITEM (layer));

  drawable = ligma_layer_get_floating_sel_drawable (layer);

  /*  set the underlying drawable to active  */
  if (LIGMA_IS_LAYER_MASK (drawable))
    {
      LigmaLayerMask *mask = LIGMA_LAYER_MASK (drawable);

      selected_drawables = g_list_prepend (NULL, ligma_layer_mask_get_layer (mask));
      ligma_image_set_selected_layers (image, selected_drawables);
    }
  else if (LIGMA_IS_CHANNEL (drawable))
    {
      selected_drawables = g_list_prepend (NULL, drawable);
      ligma_image_set_selected_channels (image, selected_drawables);
    }
  else
    {
      selected_drawables = g_list_prepend (NULL, drawable);
      ligma_image_set_selected_layers (image, selected_drawables);
    }

  g_list_free (selected_drawables);
}

const LigmaBoundSeg *
floating_sel_boundary (LigmaLayer *layer,
                       gint      *n_segs)
{
  g_return_val_if_fail (LIGMA_IS_LAYER (layer), NULL);
  g_return_val_if_fail (ligma_layer_is_floating_sel (layer), NULL);
  g_return_val_if_fail (n_segs != NULL, NULL);

  if (layer->fs.boundary_known == FALSE)
    {
      gint width, height;
      gint off_x, off_y;

      width  = ligma_item_get_width  (LIGMA_ITEM (layer));
      height = ligma_item_get_height (LIGMA_ITEM (layer));
      ligma_item_get_offset (LIGMA_ITEM (layer), &off_x, &off_y);

      if (layer->fs.segs)
        g_free (layer->fs.segs);

      if (ligma_drawable_has_alpha (LIGMA_DRAWABLE (layer)))
        {
          GeglBuffer *buffer;
          gint        i;

          /*  find the segments  */
          buffer = ligma_drawable_get_buffer (LIGMA_DRAWABLE (layer));

          layer->fs.segs = ligma_boundary_find (buffer, NULL,
                                               babl_format ("A float"),
                                               LIGMA_BOUNDARY_WITHIN_BOUNDS,
                                               0, 0, width, height,
                                               LIGMA_BOUNDARY_HALF_WAY,
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
          layer->fs.segs     = g_new0 (LigmaBoundSeg, 4);

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
floating_sel_invalidate (LigmaLayer *layer)
{
  g_return_if_fail (LIGMA_IS_LAYER (layer));
  g_return_if_fail (ligma_layer_is_floating_sel (layer));

  /*  Invalidate the attached-to drawable's preview  */
  ligma_viewable_invalidate_preview (LIGMA_VIEWABLE (ligma_layer_get_floating_sel_drawable (layer)));

  /*  Invalidate the boundary  */
  layer->fs.boundary_known = FALSE;
}
