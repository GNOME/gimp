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
#include "gimplayer-floating-sel.h"

#include "undo.h"

#include "libgimp/gimpintl.h"


/*  local function prototypes  */

static void   gimp_image_qmask_removed_callback (GObject   *qmask,
                                                 GimpImage *gimage);


/*  public functions  */

void
gimp_image_set_qmask_state (GimpImage *gimage,
                            gboolean   qmask_state)
{
  g_return_if_fail (GIMP_IS_IMAGE (gimage));

  if (qmask_state != gimage->qmask_state)
    {
      GimpChannel *mask;
      GimpRGB      color;

      if (qmask_state)
        {
          /* Set the defaults */
          color = gimage->qmask_color;

          mask = gimp_image_get_channel_by_name (gimage, "Qmask");

          if (! mask)
            {
              undo_push_group_start (gimage, IMAGE_QMASK_UNDO_GROUP);

              if (gimp_image_mask_is_empty (gimage))
                {
                  GimpLayer *layer;

                  /* if no selection */

                  if ((layer = gimp_image_floating_sel (gimage)))
                    {
                      floating_sel_to_layer (layer);
                    }

                  mask = gimp_channel_new (gimage, 
                                           gimage->width, 
                                           gimage->height,
                                           "Qmask",
                                           &color);
                  gimp_image_add_channel (gimage, mask, 0);

                  gimp_drawable_fill_by_type (GIMP_DRAWABLE (mask),
                                              gimp_get_current_context (gimage->gimp),
                                              GIMP_TRANSPARENT_FILL);
                }
              else /* if selection */
                {
                  mask = gimp_channel_copy (gimp_image_get_mask (gimage),
                                            G_TYPE_FROM_INSTANCE (gimp_image_get_mask (gimage)),
                                            TRUE);
                  gimp_image_mask_none (gimage);  /* Clear the selection */

                  gimp_image_add_channel (gimage, mask, 0);
                  gimp_channel_set_color (mask, &color);
                  gimp_object_set_name (GIMP_OBJECT (mask), "Qmask");
                }

              if (gimage->qmask_inverted)
                gimp_channel_invert (mask);

              undo_push_image_qmask (gimage);

              undo_push_group_end (gimage);

              /* connect to the removed signal, so the buttons get updated */
              g_signal_connect (G_OBJECT (mask), "removed", 
                                G_CALLBACK (gimp_image_qmask_removed_callback),
                                gimage);
            }
        }
      else
        {
          mask = gimp_image_get_channel_by_name (gimage, "Qmask");

          if (mask)
            { 
              undo_push_group_start (gimage, IMAGE_QMASK_UNDO_GROUP);

              /*  push the undo here since removing the mask will
               *  call the qmask_removed_callback() which will set
               *  the qmask_state to FALSE
               */
              undo_push_image_qmask (gimage);

              if (gimage->qmask_inverted)
                gimp_channel_invert (mask);

              gimp_image_mask_load (gimage, mask);
              gimp_image_remove_channel (gimage, mask);

              undo_push_group_end (gimage);
            }
        }

      gimage->qmask_state = qmask_state ? TRUE : FALSE;

      gimp_image_qmask_changed (gimage);
    }
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
          gimp_channel_invert (qmask);

          gimp_drawable_update (GIMP_DRAWABLE (qmask),
                                0, 0,
                                GIMP_DRAWABLE (qmask)->width,
                                GIMP_DRAWABLE (qmask)->height);
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
