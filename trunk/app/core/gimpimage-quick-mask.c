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

#include "libgimpcolor/gimpcolor.h"

#include "core-types.h"

#include "gimp.h"
#include "gimpchannel.h"
#include "gimpimage.h"
#include "gimpimage-quick-mask.h"
#include "gimpimage-undo.h"
#include "gimpimage-undo-push.h"
#include "gimplayer.h"
#include "gimplayer-floating-sel.h"
#include "gimpselection.h"

#include "gimp-intl.h"


#define CHANNEL_WAS_ACTIVE (0x2)


/*  public functions  */

void
gimp_image_set_quick_mask_state (GimpImage *image,
                                 gboolean   active)
{
  GimpChannel *selection;
  GimpChannel *mask;
  gboolean     channel_was_active;

  g_return_if_fail (GIMP_IS_IMAGE (image));

  if (active == gimp_image_get_quick_mask_state (image))
    return;

  /*  Keep track of the state so that we can make the right drawable
   *  active again when deactiviting quick mask (see bug #134371).
   */
  if (image->quick_mask_state)
    channel_was_active = (image->quick_mask_state & CHANNEL_WAS_ACTIVE) != 0;
  else
    channel_was_active = gimp_image_get_active_channel (image) != NULL;

  /*  Set image->quick_mask_state early so we can return early when
   *  being called recursively.
   */
  image->quick_mask_state = (active
                             ? TRUE | (channel_was_active ?
                                       CHANNEL_WAS_ACTIVE : 0)
                             : FALSE);

  selection = gimp_image_get_mask (image);
  mask      = gimp_image_get_quick_mask (image);

  if (active)
    {
      if (! mask)
        {
          gimp_image_undo_group_start (image, GIMP_UNDO_GROUP_IMAGE_QUICK_MASK,
                                       _("Enable Quick Mask"));

          if (gimp_channel_is_empty (selection))
            {
              /* if no selection */

              GimpLayer *floating_sel = gimp_image_floating_sel (image);

              if (floating_sel)
                floating_sel_to_layer (floating_sel);

              mask = gimp_channel_new (image,
                                       image->width,
                                       image->height,
                                       GIMP_IMAGE_QUICK_MASK_NAME,
                                       &image->quick_mask_color);

              /* Clear the mask */
              gimp_channel_clear (mask, NULL, FALSE);
            }
          else
            {
              /* if selection */

              mask = GIMP_CHANNEL (gimp_item_duplicate (GIMP_ITEM (selection),
                                                        GIMP_TYPE_CHANNEL,
                                                        FALSE));

              /* Clear the selection */
              gimp_channel_clear (selection, NULL, TRUE);

              gimp_channel_set_color (mask, &image->quick_mask_color, FALSE);
              gimp_item_rename (GIMP_ITEM (mask), GIMP_IMAGE_QUICK_MASK_NAME);
            }

          if (image->quick_mask_inverted)
            gimp_channel_invert (mask, FALSE);

          gimp_image_add_channel (image, mask, 0);

          gimp_image_undo_group_end (image);
        }
    }
  else
    {
      if (mask)
        {
          GimpLayer *floating_sel = gimp_image_floating_sel (image);

          gimp_image_undo_group_start (image, GIMP_UNDO_GROUP_IMAGE_QUICK_MASK,
                                       _("Disable Quick Mask"));

          if (image->quick_mask_inverted)
            gimp_channel_invert (mask, TRUE);

          if (floating_sel && floating_sel->fs.drawable == GIMP_DRAWABLE (mask))
            floating_sel_anchor (floating_sel);

          gimp_selection_load (gimp_image_get_mask (image), mask);
          gimp_image_remove_channel (image, mask);

          if (! channel_was_active)
            gimp_image_unset_active_channel (image);

          gimp_image_undo_group_end (image);
        }
    }

  gimp_image_quick_mask_changed (image);
}

gboolean
gimp_image_get_quick_mask_state (const GimpImage *image)
{
  g_return_val_if_fail (GIMP_IS_IMAGE (image), FALSE);

  return (image->quick_mask_state != 0);
}

void
gimp_image_set_quick_mask_color (GimpImage     *image,
                                 const GimpRGB *color)
{
  GimpChannel *quick_mask;

  g_return_if_fail (GIMP_IS_IMAGE (image));
  g_return_if_fail (color != NULL);

  image->quick_mask_color = *color;

  quick_mask = gimp_image_get_quick_mask (image);
  if (quick_mask)
    gimp_channel_set_color (quick_mask, color, TRUE);
}

void
gimp_image_get_quick_mask_color (const GimpImage *image,
                                 GimpRGB         *color)
{
  g_return_if_fail (GIMP_IS_IMAGE (image));
  g_return_if_fail (color != NULL);

  *color = image->quick_mask_color;
}

GimpChannel *
gimp_image_get_quick_mask (const GimpImage *image)
{
  g_return_val_if_fail (GIMP_IS_IMAGE (image), NULL);

  return gimp_image_get_channel_by_name (image, GIMP_IMAGE_QUICK_MASK_NAME);
}

void
gimp_image_quick_mask_invert (GimpImage *image)
{
  g_return_if_fail (GIMP_IS_IMAGE (image));

  if (image->quick_mask_state)
    {
      GimpChannel *quick_mask = gimp_image_get_quick_mask (image);

      if (quick_mask)
        gimp_channel_invert (quick_mask, TRUE);
    }

  image->quick_mask_inverted = ! image->quick_mask_inverted;
}
