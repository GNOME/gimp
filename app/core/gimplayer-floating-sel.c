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
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#include "config.h"

#include <gegl.h>

#include "libgimpbase/gimpbase.h"

#include "core-types.h"

#include "base/boundary.h"
#include "base/pixel-region.h"

#include "gimperror.h"
#include "gimpimage.h"
#include "gimpimage-undo.h"
#include "gimpimage-undo-push.h"
#include "gimplayer.h"
#include "gimplayer-floating-sel.h"
#include "gimplayermask.h"

#include "gimp-intl.h"


/*  local function prototypes  */

static void   floating_sel_composite (GimpLayer *layer);


/* public functions  */

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

  gimp_image_add_layer (image, layer, NULL, 0, TRUE);
}

void
floating_sel_anchor (GimpLayer *layer)
{
  GimpImage *image;

  g_return_if_fail (GIMP_IS_LAYER (layer));
  g_return_if_fail (gimp_layer_is_floating_sel (layer));

  image = gimp_item_get_image (GIMP_ITEM (layer));

  gimp_image_undo_group_start (image, GIMP_UNDO_GROUP_FS_ANCHOR,
                               C_("undo-type", "Anchor Floating Selection"));

  /*  Composite the floating selection contents  */
  floating_sel_composite (layer);

  gimp_image_remove_layer (image, layer, TRUE, NULL);

  gimp_image_undo_group_end (image);

  /*  invalidate the boundaries  */
  gimp_drawable_invalidate_boundary (GIMP_DRAWABLE (gimp_image_get_mask (image)));
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

      width  = gimp_item_get_width  (GIMP_ITEM (layer));
      height = gimp_item_get_height (GIMP_ITEM (layer));
      gimp_item_get_offset (GIMP_ITEM (layer), &off_x, &off_y);

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
  gimp_viewable_invalidate_preview (GIMP_VIEWABLE (gimp_layer_get_floating_sel_drawable (layer)));

  /*  Invalidate the boundary  */
  layer->fs.boundary_known = FALSE;
}


/*  private functions  */

static void
floating_sel_composite (GimpLayer *layer)
{
  GimpDrawable *drawable;
  gint          off_x, off_y;
  gint          dr_off_x, dr_off_y;
  gint          combine_x, combine_y;
  gint          combine_width, combine_height;

  g_return_if_fail (GIMP_IS_LAYER (layer));
  g_return_if_fail (gimp_layer_is_floating_sel (layer));

  drawable = gimp_layer_get_floating_sel_drawable (layer);

  gimp_item_get_offset (GIMP_ITEM (layer), &off_x, &off_y);
  gimp_item_get_offset (GIMP_ITEM (drawable), &dr_off_x, &dr_off_y);

  if (gimp_item_get_visible (GIMP_ITEM (layer)) &&
      gimp_rectangle_intersect (off_x, off_y,
                                gimp_item_get_width  (GIMP_ITEM (layer)),
                                gimp_item_get_height (GIMP_ITEM (layer)),
                                dr_off_x, dr_off_y,
                                gimp_item_get_width  (GIMP_ITEM (drawable)),
                                gimp_item_get_height (GIMP_ITEM (drawable)),
                                &combine_x, &combine_y,
                                &combine_width, &combine_height))
    {
      PixelRegion fsPR;
      gboolean    lock_alpha = FALSE;

      /*  composite the area from the layer to the drawable  */
      pixel_region_init (&fsPR,
                         gimp_drawable_get_tiles (GIMP_DRAWABLE (layer)),
                         combine_x - off_x,
                         combine_y - off_y,
                         combine_width, combine_height,
                         FALSE);

      /*  a kludge here to prevent the case of the drawable
       *  underneath having lock alpha on, and disallowing
       *  the composited floating selection from being shown
       */
      if (GIMP_IS_LAYER (drawable))
        {
          lock_alpha = gimp_layer_get_lock_alpha (GIMP_LAYER (drawable));

          if (lock_alpha)
            gimp_layer_set_lock_alpha (GIMP_LAYER (drawable), FALSE, FALSE);
        }

      gimp_drawable_apply_region (drawable, &fsPR,
                                  TRUE, NULL,
                                  gimp_layer_get_opacity (layer),
                                  gimp_layer_get_mode (layer),
                                  NULL, NULL,
                                  combine_x - dr_off_x,
                                  combine_y - dr_off_y);

      /*  restore lock alpha  */
      if (lock_alpha)
        gimp_layer_set_lock_alpha (GIMP_LAYER (drawable), TRUE, FALSE);
    }
}
