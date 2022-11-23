/* LIGMA - The GNU Image Manipulation Program
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * ligmaimage-convert-precision.c
 * Copyright (C) 2012 Michael Natterer <mitch@ligma.org>
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
#include <gdk-pixbuf/gdk-pixbuf.h>
#include <gegl.h>

#include "libligmabase/ligmabase.h"
#include "libligmacolor/ligmacolor.h"

#include "core-types.h"

#include "gegl/ligma-babl.h"
#include "gegl/ligma-gegl-loops.h"

#include "ligmachannel.h"
#include "ligmadrawable.h"
#include "ligmadrawable-operation.h"
#include "ligmaimage.h"
#include "ligmaimage-color-profile.h"
#include "ligmaimage-convert-precision.h"
#include "ligmaimage-undo.h"
#include "ligmaimage-undo-push.h"
#include "ligmaobjectqueue.h"
#include "ligmaprogress.h"

#include "text/ligmatextlayer.h"

#include "ligma-intl.h"


void
ligma_image_convert_precision (LigmaImage        *image,
                              LigmaPrecision     precision,
                              GeglDitherMethod  layer_dither_type,
                              GeglDitherMethod  text_layer_dither_type,
                              GeglDitherMethod  mask_dither_type,
                              LigmaProgress     *progress)
{
  LigmaColorProfile *old_profile;
  LigmaColorProfile *new_profile = NULL;
  const Babl       *old_format;
  const Babl       *new_format;
  LigmaObjectQueue  *queue;
  LigmaProgress     *sub_progress;
  GList            *layers;
  LigmaDrawable     *drawable;
  const gchar      *enum_desc;
  gchar            *undo_desc = NULL;

  g_return_if_fail (LIGMA_IS_IMAGE (image));
  g_return_if_fail (precision != ligma_image_get_precision (image));
  g_return_if_fail (ligma_babl_is_valid (ligma_image_get_base_type (image),
                                        precision));
  g_return_if_fail (progress == NULL || LIGMA_IS_PROGRESS (progress));

  ligma_enum_get_value (LIGMA_TYPE_PRECISION, precision,
                       NULL, NULL, &enum_desc, NULL);

  undo_desc = g_strdup_printf (C_("undo-type", "Convert Image to %s"),
                               enum_desc);

  if (progress)
    ligma_progress_start (progress, FALSE, "%s", undo_desc);

  queue        = ligma_object_queue_new (progress);
  sub_progress = LIGMA_PROGRESS (queue);

  layers = ligma_image_get_layer_list (image);
  ligma_object_queue_push_list (queue, layers);
  g_list_free (layers);

  ligma_object_queue_push (queue, ligma_image_get_mask (image));
  ligma_object_queue_push_container (queue, ligma_image_get_channels (image));

  g_object_freeze_notify (G_OBJECT (image));

  ligma_image_undo_group_start (image, LIGMA_UNDO_GROUP_IMAGE_CONVERT,
                               undo_desc);
  g_free (undo_desc);

  /*  Push the image precision to the stack  */
  ligma_image_undo_push_image_precision (image, NULL);

  old_profile = ligma_color_managed_get_color_profile (LIGMA_COLOR_MANAGED (image));
  old_format  = ligma_image_get_layer_format (image, FALSE);

  /*  Set the new precision  */
  g_object_set (image, "precision", precision, NULL);

  new_format = ligma_image_get_layer_format (image, FALSE);

  /* we use old_format and new_format just for looking at their
   * TRCs, new_format's space might be incorrect, don't use it
   * for anything else.
   */
  if (ligma_babl_format_get_trc (old_format) !=
      ligma_babl_format_get_trc (new_format))
    {
      LigmaImageBaseType base_type = ligma_image_get_base_type (image);
      LigmaTRCType       new_trc   = ligma_babl_trc (precision);

      /* if the image doesn't use the builtin profile, create a new
       * one, using the original profile's chromacities and
       * whitepoint, but a linear/sRGB-gamma TRC.
       */
      if (ligma_image_get_color_profile (image))
        {
          if (new_trc == LIGMA_TRC_LINEAR)
            {
              new_profile =
                ligma_color_profile_new_linear_from_color_profile (old_profile);
            }
          else
            {
              new_profile =
                ligma_color_profile_new_srgb_trc_from_color_profile (old_profile);
            }
        }

      /* we always need a profile for convert_type with changing TRC
       * on the same image, use the new precision's builtin profile if
       * the profile couldn't be converted or the image used the old
       * TRC's builtin profile.
       */
      if (! new_profile)
        {
          new_profile = ligma_babl_get_builtin_color_profile (base_type,
                                                             new_trc);
          g_object_ref (new_profile);
        }
    }

  while ((drawable = ligma_object_queue_pop (queue)))
    {
      if (drawable == LIGMA_DRAWABLE (ligma_image_get_mask (image)))
        {
          GeglBuffer *buffer;

          ligma_image_undo_push_mask_precision (image, NULL,
                                               LIGMA_CHANNEL (drawable));

          buffer = gegl_buffer_new (GEGL_RECTANGLE (0, 0,
                                                    ligma_image_get_width  (image),
                                                    ligma_image_get_height (image)),
                                    ligma_image_get_mask_format (image));

          ligma_gegl_buffer_copy (ligma_drawable_get_buffer (drawable), NULL,
                                 GEGL_ABYSS_NONE,
                                 buffer, NULL);

          ligma_drawable_set_buffer (drawable, FALSE, NULL, buffer);
          g_object_unref (buffer);

          ligma_progress_set_value (sub_progress, 1.0);
        }
      else
        {
          GeglDitherMethod dither_type;

          if (ligma_item_is_text_layer (LIGMA_ITEM (drawable)))
            dither_type = text_layer_dither_type;
          else
            dither_type = layer_dither_type;

          ligma_drawable_convert_type (drawable, image,
                                      ligma_drawable_get_base_type (drawable),
                                      precision,
                                      ligma_drawable_has_alpha (drawable),
                                      old_profile,
                                      new_profile,
                                      dither_type,
                                      mask_dither_type,
                                      TRUE, sub_progress);
        }
    }

  if (new_profile)
    {
      ligma_image_set_color_profile (image, new_profile, NULL);
      g_object_unref (new_profile);
    }
  else
    {
      ligma_color_managed_profile_changed (LIGMA_COLOR_MANAGED (image));
    }

  ligma_image_undo_group_end (image);

  ligma_image_precision_changed (image);
  g_object_thaw_notify (G_OBJECT (image));

  g_object_unref (queue);

  if (progress)
    ligma_progress_end (progress);
}

void
ligma_image_convert_dither_u8 (LigmaImage    *image,
                              LigmaProgress *progress)
{
  GeglNode *dither;

  g_return_if_fail (LIGMA_IS_IMAGE (image));
  g_return_if_fail (progress == NULL || LIGMA_IS_PROGRESS (progress));

  dither = gegl_node_new_child (NULL,
                                "operation", "gegl:noise-rgb",
                                "red",       1.0 / 256.0,
                                "green",     1.0 / 256.0,
                                "blue",      1.0 / 256.0,
                                "linear",    FALSE,
                                "gaussian",  FALSE,
                                NULL);

  if (dither)
    {
      LigmaObjectQueue *queue;
      LigmaProgress    *sub_progress;
      GList           *layers;
      GList           *list;
      LigmaDrawable    *drawable;

      if (progress)
        ligma_progress_start (progress, FALSE, "%s", _("Dithering"));

      queue        = ligma_object_queue_new (progress);
      sub_progress = LIGMA_PROGRESS (queue);

      layers = ligma_image_get_layer_list (image);

      for (list = layers; list; list = g_list_next (list))
        {
          if (! ligma_viewable_get_children (list->data) &&
              ! ligma_item_is_text_layer (list->data))
            {
              ligma_object_queue_push (queue, list->data);
            }
        }

      g_list_free (layers);

      while ((drawable = ligma_object_queue_pop (queue)))
        {
          ligma_drawable_apply_operation (drawable, sub_progress,
                                         _("Dithering"),
                                         dither);
        }

      g_object_unref (queue);

      if (progress)
        ligma_progress_end (progress);

      g_object_unref (dither);
    }
}
