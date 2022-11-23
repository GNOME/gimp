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

#include <cairo.h>
#include <gegl.h>
#include <gdk-pixbuf/gdk-pixbuf.h>

#include "libligmacolor/ligmacolor.h"

#include "core-types.h"

#include "ligma.h"
#include "ligmachannel.h"
#include "ligmaimage.h"
#include "ligmaimage-private.h"
#include "ligmaimage-quick-mask.h"
#include "ligmaimage-undo.h"
#include "ligmaimage-undo-push.h"
#include "ligmalayer.h"
#include "ligmalayer-floating-selection.h"

#include "ligma-intl.h"


#define CHANNEL_WAS_ACTIVE (0x2)


/*  public functions  */

void
ligma_image_set_quick_mask_state (LigmaImage *image,
                                 gboolean   active)
{
  LigmaImagePrivate *private;
  LigmaChannel      *selection;
  LigmaChannel      *mask;
  gboolean          channel_was_active;

  g_return_if_fail (LIGMA_IS_IMAGE (image));

  if (active == ligma_image_get_quick_mask_state (image))
    return;

  private = LIGMA_IMAGE_GET_PRIVATE (image);

  /*  Keep track of the state so that we can make the right drawable
   *  active again when deactiviting quick mask (see bug #134371).
   */
  if (private->quick_mask_state)
    channel_was_active = (private->quick_mask_state & CHANNEL_WAS_ACTIVE) != 0;
  else
    channel_was_active = (ligma_image_get_selected_channels (image) != NULL);

  /*  Set private->quick_mask_state early so we can return early when
   *  being called recursively.
   */
  private->quick_mask_state = (active
                               ? TRUE | (channel_was_active ?
                                         CHANNEL_WAS_ACTIVE : 0)
                               : FALSE);

  selection = ligma_image_get_mask (image);
  mask      = ligma_image_get_quick_mask (image);

  if (active)
    {
      if (! mask)
        {
          LigmaLayer *floating_sel;

          ligma_image_undo_group_start (image, LIGMA_UNDO_GROUP_IMAGE_QUICK_MASK,
                                       C_("undo-type", "Enable Quick Mask"));

          floating_sel = ligma_image_get_floating_selection (image);

          if (floating_sel)
            floating_sel_to_layer (floating_sel, NULL);

          mask = LIGMA_CHANNEL (ligma_item_duplicate (LIGMA_ITEM (selection),
                                                    LIGMA_TYPE_CHANNEL));

          if (! ligma_channel_is_empty (selection))
            ligma_channel_clear (selection, NULL, TRUE);

          ligma_channel_set_color (mask, &private->quick_mask_color, FALSE);
          ligma_item_rename (LIGMA_ITEM (mask), LIGMA_IMAGE_QUICK_MASK_NAME,
                            NULL);

          if (private->quick_mask_inverted)
            ligma_channel_invert (mask, FALSE);

          ligma_image_add_channel (image, mask, NULL, 0, TRUE);

          ligma_image_undo_group_end (image);
        }
    }
  else
    {
      if (mask)
        {
          LigmaLayer *floating_sel = ligma_image_get_floating_selection (image);

          ligma_image_undo_group_start (image, LIGMA_UNDO_GROUP_IMAGE_QUICK_MASK,
                                       C_("undo-type", "Disable Quick Mask"));

          if (private->quick_mask_inverted)
            ligma_channel_invert (mask, TRUE);

          if (floating_sel &&
              ligma_layer_get_floating_sel_drawable (floating_sel) == LIGMA_DRAWABLE (mask))
            floating_sel_anchor (floating_sel);

          ligma_item_to_selection (LIGMA_ITEM (mask),
                                  LIGMA_CHANNEL_OP_REPLACE,
                                  TRUE, FALSE, 0.0, 0.0);
          ligma_image_remove_channel (image, mask, TRUE, NULL);

          if (! channel_was_active)
            ligma_image_unset_selected_channels (image);

          ligma_image_undo_group_end (image);
        }
    }

  ligma_image_quick_mask_changed (image);
}

gboolean
ligma_image_get_quick_mask_state (LigmaImage *image)
{
  g_return_val_if_fail (LIGMA_IS_IMAGE (image), FALSE);

  return LIGMA_IMAGE_GET_PRIVATE (image)->quick_mask_state;
}

void
ligma_image_set_quick_mask_color (LigmaImage     *image,
                                 const LigmaRGB *color)
{
  LigmaChannel *quick_mask;

  g_return_if_fail (LIGMA_IS_IMAGE (image));
  g_return_if_fail (color != NULL);

  LIGMA_IMAGE_GET_PRIVATE (image)->quick_mask_color = *color;

  quick_mask = ligma_image_get_quick_mask (image);
  if (quick_mask)
    ligma_channel_set_color (quick_mask, color, TRUE);
}

void
ligma_image_get_quick_mask_color (LigmaImage *image,
                                 LigmaRGB   *color)
{
  g_return_if_fail (LIGMA_IS_IMAGE (image));
  g_return_if_fail (color != NULL);

  *color = LIGMA_IMAGE_GET_PRIVATE (image)->quick_mask_color;
}

LigmaChannel *
ligma_image_get_quick_mask (LigmaImage *image)
{
  g_return_val_if_fail (LIGMA_IS_IMAGE (image), NULL);

  return ligma_image_get_channel_by_name (image, LIGMA_IMAGE_QUICK_MASK_NAME);
}

void
ligma_image_quick_mask_invert (LigmaImage *image)
{
  LigmaImagePrivate *private;

  g_return_if_fail (LIGMA_IS_IMAGE (image));

  private = LIGMA_IMAGE_GET_PRIVATE (image);

  if (private->quick_mask_state)
    {
      LigmaChannel *quick_mask = ligma_image_get_quick_mask (image);

      if (quick_mask)
        ligma_channel_invert (quick_mask, TRUE);
    }

  private->quick_mask_inverted = ! private->quick_mask_inverted;
}

gboolean
ligma_image_get_quick_mask_inverted (LigmaImage *image)
{
  g_return_val_if_fail (LIGMA_IS_IMAGE (image), FALSE);

  return LIGMA_IMAGE_GET_PRIVATE (image)->quick_mask_inverted;
}
