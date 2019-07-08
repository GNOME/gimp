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

#include <cairo.h>
#include <gegl.h>
#include <gdk-pixbuf/gdk-pixbuf.h>

#include "libgimpcolor/gimpcolor.h"

#include "core-types.h"

#include "gimp.h"
#include "gimpchannel.h"
#include "gimpimage.h"
#include "gimpimage-private.h"
#include "gimpimage-quick-mask.h"
#include "gimpimage-undo.h"
#include "gimpimage-undo-push.h"
#include "gimplayer.h"
#include "gimplayer-floating-selection.h"

#include "gimp-intl.h"


#define CHANNEL_WAS_ACTIVE (0x2)


/*  public functions  */

void
gimp_image_set_quick_mask_state (GimpImage *image,
                                 gboolean   active)
{
  GimpImagePrivate *private;
  GimpChannel      *selection;
  GimpChannel      *mask;
  gboolean          channel_was_active;

  g_return_if_fail (GIMP_IS_IMAGE (image));

  if (active == gimp_image_get_quick_mask_state (image))
    return;

  private = GIMP_IMAGE_GET_PRIVATE (image);

  /*  Keep track of the state so that we can make the right drawable
   *  active again when deactiviting quick mask (see bug #134371).
   */
  if (private->quick_mask_state)
    channel_was_active = (private->quick_mask_state & CHANNEL_WAS_ACTIVE) != 0;
  else
    channel_was_active = gimp_image_get_active_channel (image) != NULL;

  /*  Set private->quick_mask_state early so we can return early when
   *  being called recursively.
   */
  private->quick_mask_state = (active
                               ? TRUE | (channel_was_active ?
                                         CHANNEL_WAS_ACTIVE : 0)
                               : FALSE);

  selection = gimp_image_get_mask (image);
  mask      = gimp_image_get_quick_mask (image);

  if (active)
    {
      if (! mask)
        {
          GimpLayer *floating_sel;

          gimp_image_undo_group_start (image, GIMP_UNDO_GROUP_IMAGE_QUICK_MASK,
                                       C_("undo-type", "Enable Quick Mask"));

          floating_sel = gimp_image_get_floating_selection (image);

          if (floating_sel)
            floating_sel_to_layer (floating_sel, NULL);

          mask = GIMP_CHANNEL (gimp_item_duplicate (GIMP_ITEM (selection),
                                                    GIMP_TYPE_CHANNEL));

          if (! gimp_channel_is_empty (selection))
            gimp_channel_clear (selection, NULL, TRUE);

          gimp_channel_set_color (mask, &private->quick_mask_color, FALSE);
          gimp_item_rename (GIMP_ITEM (mask), GIMP_IMAGE_QUICK_MASK_NAME,
                            NULL);

          if (private->quick_mask_inverted)
            gimp_channel_invert (mask, FALSE);

          gimp_image_add_channel (image, mask, NULL, 0, TRUE);

          gimp_image_undo_group_end (image);
        }
    }
  else
    {
      if (mask)
        {
          GimpLayer *floating_sel = gimp_image_get_floating_selection (image);

          gimp_image_undo_group_start (image, GIMP_UNDO_GROUP_IMAGE_QUICK_MASK,
                                       C_("undo-type", "Disable Quick Mask"));

          if (private->quick_mask_inverted)
            gimp_channel_invert (mask, TRUE);

          if (floating_sel &&
              gimp_layer_get_floating_sel_drawable (floating_sel) == GIMP_DRAWABLE (mask))
            floating_sel_anchor (floating_sel);

          gimp_item_to_selection (GIMP_ITEM (mask),
                                  GIMP_CHANNEL_OP_REPLACE,
                                  TRUE, FALSE, 0.0, 0.0);
          gimp_image_remove_channel (image, mask, TRUE, NULL);

          if (! channel_was_active)
            gimp_image_unset_active_channel (image);

          gimp_image_undo_group_end (image);
        }
    }

  gimp_image_quick_mask_changed (image);
}

gboolean
gimp_image_get_quick_mask_state (GimpImage *image)
{
  g_return_val_if_fail (GIMP_IS_IMAGE (image), FALSE);

  return GIMP_IMAGE_GET_PRIVATE (image)->quick_mask_state;
}

void
gimp_image_set_quick_mask_color (GimpImage     *image,
                                 const GimpRGB *color)
{
  GimpChannel *quick_mask;

  g_return_if_fail (GIMP_IS_IMAGE (image));
  g_return_if_fail (color != NULL);

  GIMP_IMAGE_GET_PRIVATE (image)->quick_mask_color = *color;

  quick_mask = gimp_image_get_quick_mask (image);
  if (quick_mask)
    gimp_channel_set_color (quick_mask, color, TRUE);
}

void
gimp_image_get_quick_mask_color (GimpImage *image,
                                 GimpRGB   *color)
{
  g_return_if_fail (GIMP_IS_IMAGE (image));
  g_return_if_fail (color != NULL);

  *color = GIMP_IMAGE_GET_PRIVATE (image)->quick_mask_color;
}

GimpChannel *
gimp_image_get_quick_mask (GimpImage *image)
{
  g_return_val_if_fail (GIMP_IS_IMAGE (image), NULL);

  return gimp_image_get_channel_by_name (image, GIMP_IMAGE_QUICK_MASK_NAME);
}

void
gimp_image_quick_mask_invert (GimpImage *image)
{
  GimpImagePrivate *private;

  g_return_if_fail (GIMP_IS_IMAGE (image));

  private = GIMP_IMAGE_GET_PRIVATE (image);

  if (private->quick_mask_state)
    {
      GimpChannel *quick_mask = gimp_image_get_quick_mask (image);

      if (quick_mask)
        gimp_channel_invert (quick_mask, TRUE);
    }

  private->quick_mask_inverted = ! private->quick_mask_inverted;
}

gboolean
gimp_image_get_quick_mask_inverted (GimpImage *image)
{
  g_return_val_if_fail (GIMP_IS_IMAGE (image), FALSE);

  return GIMP_IMAGE_GET_PRIVATE (image)->quick_mask_inverted;
}
