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
#include <gdk-pixbuf/gdk-pixbuf.h>
#include <gegl.h>

#include "libgimpcolor/gimpcolor.h"

#include "core-types.h"

#include "gegl/gimp-babl.h"

#include "gimp.h"
#include "gimpdrawable.h"
#include "gimpimage.h"
#include "gimpimage-color-profile.h"
#include "gimpimage-colormap.h"
#include "gimpimage-convert-type.h"
#include "gimpimage-undo.h"
#include "gimpimage-undo-push.h"
#include "gimpobjectqueue.h"
#include "gimpprogress.h"

#include "gimp-intl.h"


gboolean
gimp_image_convert_type (GimpImage          *image,
                         GimpImageBaseType   new_type,
                         GimpColorProfile   *dest_profile,
                         GimpProgress       *progress,
                         GError            **error)
{
  GimpImageBaseType  old_type;
  const Babl        *new_layer_format;
  GimpObjectQueue   *queue;
  GList             *all_layers;
  GimpDrawable      *drawable;
  const gchar       *undo_desc = NULL;

  g_return_val_if_fail (GIMP_IS_IMAGE (image), FALSE);
  g_return_val_if_fail (new_type != gimp_image_get_base_type (image), FALSE);
  g_return_val_if_fail (new_type != GIMP_INDEXED, FALSE);
  g_return_val_if_fail (gimp_babl_is_valid (new_type,
                                            gimp_image_get_precision (image)),
                        FALSE);
  g_return_val_if_fail (dest_profile == NULL || GIMP_IS_COLOR_PROFILE (dest_profile),
                        FALSE);
  g_return_val_if_fail (progress == NULL || GIMP_IS_PROGRESS (progress), FALSE);
  g_return_val_if_fail (error == NULL || *error == NULL, FALSE);

  new_layer_format = gimp_babl_format (new_type,
                                       gimp_image_get_precision (image),
                                       TRUE);

  if (dest_profile &&
      ! gimp_image_validate_color_profile_by_format (new_layer_format,
                                                     dest_profile,
                                                     NULL, error))
    {
      return FALSE;
    }

  switch (new_type)
    {
    case GIMP_RGB:
      undo_desc = C_("undo-type", "Convert Image to RGB");
      break;

    case GIMP_GRAY:
      undo_desc = C_("undo-type", "Convert Image to Grayscale");
      break;

    default:
      g_return_val_if_reached (FALSE);
    }

  gimp_set_busy (image->gimp);

  queue    = gimp_object_queue_new (progress);
  progress = GIMP_PROGRESS (queue);

  all_layers = gimp_image_get_layer_list (image);
  gimp_object_queue_push_list (queue, all_layers);
  g_list_free (all_layers);

  g_object_freeze_notify (G_OBJECT (image));

  gimp_image_undo_group_start (image, GIMP_UNDO_GROUP_IMAGE_CONVERT,
                               undo_desc);

  /*  Push the image type to the stack  */
  gimp_image_undo_push_image_type (image, NULL);

  /*  Set the new base type  */
  old_type = gimp_image_get_base_type (image);

  g_object_set (image, "base-type", new_type, NULL);

  /*  When converting to/from GRAY, convert to the new type's builtin
   *  profile if none was passed.
   */
  if (old_type == GIMP_GRAY ||
      new_type == GIMP_GRAY)
    {
      if (! dest_profile && gimp_image_get_color_profile (image))
        dest_profile = gimp_image_get_builtin_color_profile (image);
    }

  while ((drawable = gimp_object_queue_pop (queue)))
    {
      gimp_drawable_convert_type (drawable, image,
                                  new_type,
                                  gimp_drawable_get_precision (drawable),
                                  gimp_drawable_has_alpha (drawable),
                                  dest_profile,
                                  GEGL_DITHER_NONE, GEGL_DITHER_NONE,
                                  TRUE, progress);
    }

  if (old_type == GIMP_INDEXED)
    gimp_image_unset_colormap (image, TRUE);

  /*  When converting to/from GRAY, set the new profile.
   */
  if (old_type == GIMP_GRAY ||
      new_type == GIMP_GRAY)
    {
      if (gimp_image_get_color_profile (image))
        gimp_image_set_color_profile (image, dest_profile, NULL);
      else
        gimp_color_managed_profile_changed (GIMP_COLOR_MANAGED (image));
    }

  gimp_image_undo_group_end (image);

  gimp_image_mode_changed (image);
  g_object_thaw_notify (G_OBJECT (image));

  g_object_unref (queue);

  gimp_unset_busy (image->gimp);

  return TRUE;
}
