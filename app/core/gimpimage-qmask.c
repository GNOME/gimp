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

#include <stdlib.h>

#include <glib-object.h>

#include "libgimpcolor/gimpcolor.h"

#include "core-types.h"

#include "gimp.h"
#include "gimpchannel.h"
#include "gimpcontext.h"
#include "gimpimage.h"
#include "gimpimage-mask.h"
#include "gimpimage-qmask.h"
#include "gimpimage-undo.h"
#include "gimpimage-undo-push.h"
#include "gimplayer-floating-sel.h"

#include "gimp-intl.h"


/*  local function prototypes  */

static void   gimp_image_qmask_removed_callback (GObject   *qmask,
                                                 GimpImage *gimage);


/*  public functions  */

void
gimp_image_set_qmask_state (GimpImage *gimage,
                            gboolean   qmask_state)
{
  GimpChannel *mask;
  GimpRGB      color;

  g_return_if_fail (GIMP_IS_IMAGE (gimage));

  if (qmask_state == gimage->qmask_state)
    return;

  if (qmask_state)
    {
      /* Set the defaults */
      color = gimage->qmask_color;

      mask = gimp_image_get_channel_by_name (gimage, "Qmask");

      if (! mask)
        {
          gimp_image_undo_group_start (gimage, GIMP_UNDO_GROUP_IMAGE_QMASK,
                                       _("Enable QuickMask"));

          if (gimp_image_mask_is_empty (gimage)) /* if no selection */
            {
              GimpLayer *layer;

              if ((layer = gimp_image_floating_sel (gimage)))
                {
                  floating_sel_to_layer (layer);
                }

              mask = gimp_channel_new (gimage, 
                                       gimage->width, 
                                       gimage->height,
                                       "Qmask",
                                       &color);

              gimp_drawable_fill_by_type (GIMP_DRAWABLE (mask),
                                          gimp_get_current_context (gimage->gimp),
                                          GIMP_TRANSPARENT_FILL);
            }
          else /* if selection */
            {
              mask = GIMP_CHANNEL (gimp_item_duplicate (GIMP_ITEM (gimp_image_get_mask (gimage)),
                                                        G_TYPE_FROM_INSTANCE (gimp_image_get_mask (gimage)),
                                                        FALSE));
              gimp_image_mask_clear (gimage, NULL);  /* Clear the selection */

              gimp_channel_set_color (mask, &color, FALSE);
              gimp_item_rename (GIMP_ITEM (mask), "Qmask");
            }

          gimp_image_add_channel (gimage, mask, 0);

          if (gimage->qmask_inverted)
            gimp_channel_invert (mask, TRUE);

          gimp_image_undo_push_image_qmask (gimage, NULL);

          gimp_image_undo_group_end (gimage);

          /* connect to the removed signal, so the buttons get updated */
          g_signal_connect (mask, "removed", 
                            G_CALLBACK (gimp_image_qmask_removed_callback),
                            gimage);
        }
    }
  else
    {
      mask = gimp_image_get_channel_by_name (gimage, "Qmask");

      if (mask)
        { 
          gimp_image_undo_group_start (gimage, GIMP_UNDO_GROUP_IMAGE_QMASK,
                                       _("Disable QuickMask"));

          /*  push the undo here since removing the mask will
           *  call the qmask_removed_callback() which will set
           *  the qmask_state to FALSE
           */
          gimp_image_undo_push_image_qmask (gimage, NULL);

          if (gimage->qmask_inverted)
            gimp_channel_invert (mask, TRUE);

          gimp_image_mask_load (gimage, mask);
          gimp_image_remove_channel (gimage, mask);

          gimp_image_undo_group_end (gimage);
        }
    }

  gimage->qmask_state = qmask_state ? TRUE : FALSE;

  gimp_image_qmask_changed (gimage);
}

gboolean
gimp_image_get_qmask_state (const GimpImage *gimage)
{
  g_return_val_if_fail (GIMP_IS_IMAGE (gimage), FALSE);

  return gimage->qmask_state;
}

void
gimp_image_qmask_invert (GimpImage *gimage)
{
  g_return_if_fail (GIMP_IS_IMAGE (gimage));

  if (gimage->qmask_state)
    {
      GimpChannel *qmask;

      qmask = gimp_image_get_channel_by_name (gimage, "Qmask");

      if (qmask)
        {
          gimp_channel_invert (qmask, TRUE);

          gimp_drawable_update (GIMP_DRAWABLE (qmask),
                                0, 0,
                                GIMP_ITEM (qmask)->width,
                                GIMP_ITEM (qmask)->height);
        }
    }

  gimage->qmask_inverted = ! gimage->qmask_inverted;
}


/*  private functions  */

static void
gimp_image_qmask_removed_callback (GObject   *qmask,
                                   GimpImage *gimage)
{
  gimp_image_set_qmask_state (gimage, FALSE);
}
