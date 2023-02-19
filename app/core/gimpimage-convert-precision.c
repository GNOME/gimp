/* GIMP - The GNU Image Manipulation Program
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * gimpimage-convert-precision.c
 * Copyright (C) 2012 Michael Natterer <mitch@gimp.org>
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

#include "libgimpbase/gimpbase.h"
#include "libgimpcolor/gimpcolor.h"

#include "core-types.h"

#include "gegl/gimp-babl.h"
#include "gegl/gimp-gegl-loops.h"

#include "gimpchannel.h"
#include "gimpdrawable.h"
#include "gimpdrawable-operation.h"
#include "gimpimage.h"
#include "gimpimage-color-profile.h"
#include "gimpimage-convert-precision.h"
#include "gimpimage-undo.h"
#include "gimpimage-undo-push.h"
#include "gimpobjectqueue.h"
#include "gimpprogress.h"

#include "text/gimptextlayer.h"

#include "gimp-intl.h"


void
gimp_image_convert_precision (GimpImage        *image,
                              GimpPrecision     precision,
                              GeglDitherMethod  layer_dither_type,
                              GeglDitherMethod  text_layer_dither_type,
                              GeglDitherMethod  mask_dither_type,
                              GimpProgress     *progress)
{
  GimpColorProfile *old_profile;
  GimpColorProfile *new_profile = NULL;
  const Babl       *old_format;
  const Babl       *new_format;
  GimpObjectQueue  *queue;
  GimpProgress     *sub_progress;
  GList            *layers;
  GimpDrawable     *drawable;
  const gchar      *enum_desc;
  gchar            *undo_desc = NULL;

  g_return_if_fail (GIMP_IS_IMAGE (image));
  g_return_if_fail (precision != gimp_image_get_precision (image));
  g_return_if_fail (gimp_babl_is_valid (gimp_image_get_base_type (image),
                                        precision));
  g_return_if_fail (progress == NULL || GIMP_IS_PROGRESS (progress));

  gimp_enum_get_value (GIMP_TYPE_PRECISION, precision,
                       NULL, NULL, &enum_desc, NULL);

  undo_desc = g_strdup_printf (C_("undo-type", "Convert Image to %s"),
                               enum_desc);

  if (progress)
    gimp_progress_start (progress, FALSE, "%s", undo_desc);

  queue        = gimp_object_queue_new (progress);
  sub_progress = GIMP_PROGRESS (queue);

  layers = gimp_image_get_layer_list (image);
  gimp_object_queue_push_list (queue, layers);
  g_list_free (layers);

  gimp_object_queue_push (queue, gimp_image_get_mask (image));
  gimp_object_queue_push_container (queue, gimp_image_get_channels (image));

  g_object_freeze_notify (G_OBJECT (image));

  gimp_image_undo_group_start (image, GIMP_UNDO_GROUP_IMAGE_CONVERT,
                               undo_desc);
  g_free (undo_desc);

  /*  Push the image precision to the stack  */
  gimp_image_undo_push_image_precision (image, NULL);

  old_profile = gimp_color_managed_get_color_profile (GIMP_COLOR_MANAGED (image));
  old_format  = gimp_image_get_layer_format (image, FALSE);

  gimp_image_set_converting (image, TRUE);

  /*  Set the new precision  */
  g_object_set (image, "precision", precision, NULL);

  new_format = gimp_image_get_layer_format (image, FALSE);

  /* we use old_format and new_format just for looking at their
   * TRCs, new_format's space might be incorrect, don't use it
   * for anything else.
   */
  if (gimp_babl_format_get_trc (old_format) !=
      gimp_babl_format_get_trc (new_format))
    {
      GimpImageBaseType base_type = gimp_image_get_base_type (image);
      GimpTRCType       new_trc   = gimp_babl_trc (precision);

      /* if the image doesn't use the builtin profile, create a new
       * one, using the original profile's chromacities and
       * whitepoint, but a linear/sRGB-gamma TRC.
       */
      if (gimp_image_get_color_profile (image))
        {
          if (new_trc == GIMP_TRC_LINEAR)
            {
              new_profile =
                gimp_color_profile_new_linear_from_color_profile (old_profile);
            }
          else
            {
              new_profile =
                gimp_color_profile_new_srgb_trc_from_color_profile (old_profile);
            }
        }

      /* we always need a profile for convert_type with changing TRC
       * on the same image, use the new precision's builtin profile if
       * the profile couldn't be converted or the image used the old
       * TRC's builtin profile.
       */
      if (! new_profile)
        {
          new_profile = gimp_babl_get_builtin_color_profile (base_type,
                                                             new_trc);
          g_object_ref (new_profile);
        }
    }

  while ((drawable = gimp_object_queue_pop (queue)))
    {
      if (drawable == GIMP_DRAWABLE (gimp_image_get_mask (image)))
        {
          GeglBuffer *buffer;

          gimp_image_undo_push_mask_precision (image, NULL,
                                               GIMP_CHANNEL (drawable));

          buffer = gegl_buffer_new (GEGL_RECTANGLE (0, 0,
                                                    gimp_image_get_width  (image),
                                                    gimp_image_get_height (image)),
                                    gimp_image_get_mask_format (image));

          gimp_gegl_buffer_copy (gimp_drawable_get_buffer (drawable), NULL,
                                 GEGL_ABYSS_NONE,
                                 buffer, NULL);

          gimp_drawable_set_buffer (drawable, FALSE, NULL, buffer);
          g_object_unref (buffer);

          gimp_progress_set_value (sub_progress, 1.0);
        }
      else
        {
          GeglDitherMethod dither_type;

          if (gimp_item_is_text_layer (GIMP_ITEM (drawable)))
            dither_type = text_layer_dither_type;
          else
            dither_type = layer_dither_type;

          gimp_drawable_convert_type (drawable, image,
                                      gimp_drawable_get_base_type (drawable),
                                      precision,
                                      gimp_drawable_has_alpha (drawable),
                                      old_profile,
                                      new_profile,
                                      dither_type,
                                      mask_dither_type,
                                      TRUE, sub_progress);
        }
    }

  if (new_profile)
    {
      gimp_image_set_color_profile (image, new_profile, NULL);
      g_object_unref (new_profile);
    }
  else
    {
      gimp_color_managed_profile_changed (GIMP_COLOR_MANAGED (image));
    }

  gimp_image_set_converting (image, FALSE);

  gimp_image_undo_group_end (image);

  gimp_image_precision_changed (image);
  g_object_thaw_notify (G_OBJECT (image));

  g_object_unref (queue);

  if (progress)
    gimp_progress_end (progress);
}

void
gimp_image_convert_dither_u8 (GimpImage    *image,
                              GimpProgress *progress)
{
  GeglNode *dither;

  g_return_if_fail (GIMP_IS_IMAGE (image));
  g_return_if_fail (progress == NULL || GIMP_IS_PROGRESS (progress));

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
      GimpObjectQueue *queue;
      GimpProgress    *sub_progress;
      GList           *layers;
      GList           *list;
      GimpDrawable    *drawable;

      if (progress)
        gimp_progress_start (progress, FALSE, "%s", _("Dithering"));

      queue        = gimp_object_queue_new (progress);
      sub_progress = GIMP_PROGRESS (queue);

      layers = gimp_image_get_layer_list (image);

      for (list = layers; list; list = g_list_next (list))
        {
          if (! gimp_viewable_get_children (list->data) &&
              ! gimp_item_is_text_layer (list->data))
            {
              gimp_object_queue_push (queue, list->data);
            }
        }

      g_list_free (layers);

      while ((drawable = gimp_object_queue_pop (queue)))
        {
          gimp_drawable_apply_operation (drawable, sub_progress,
                                         _("Dithering"),
                                         dither);
        }

      g_object_unref (queue);

      if (progress)
        gimp_progress_end (progress);

      g_object_unref (dither);
    }
}
