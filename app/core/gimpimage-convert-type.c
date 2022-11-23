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
#include <gdk-pixbuf/gdk-pixbuf.h>
#include <gegl.h>

#include "libligmacolor/ligmacolor.h"

#include "core-types.h"

#include "gegl/ligma-babl.h"

#include "ligma.h"
#include "ligmadrawable.h"
#include "ligmaimage.h"
#include "ligmaimage-color-profile.h"
#include "ligmaimage-colormap.h"
#include "ligmaimage-convert-type.h"
#include "ligmaimage-undo.h"
#include "ligmaimage-undo-push.h"
#include "ligmaobjectqueue.h"
#include "ligmaprogress.h"

#include "ligma-intl.h"


gboolean
ligma_image_convert_type (LigmaImage          *image,
                         LigmaImageBaseType   new_type,
                         LigmaColorProfile   *dest_profile,
                         LigmaProgress       *progress,
                         GError            **error)
{
  LigmaColorProfile  *src_profile;
  LigmaImageBaseType  old_type;
  const Babl        *new_layer_format;
  LigmaObjectQueue   *queue;
  GList             *all_layers;
  LigmaDrawable      *drawable;
  const gchar       *undo_desc = NULL;

  g_return_val_if_fail (LIGMA_IS_IMAGE (image), FALSE);
  g_return_val_if_fail (new_type != ligma_image_get_base_type (image), FALSE);
  g_return_val_if_fail (new_type != LIGMA_INDEXED, FALSE);
  g_return_val_if_fail (ligma_babl_is_valid (new_type,
                                            ligma_image_get_precision (image)),
                        FALSE);
  g_return_val_if_fail (dest_profile == NULL || LIGMA_IS_COLOR_PROFILE (dest_profile),
                        FALSE);
  g_return_val_if_fail (progress == NULL || LIGMA_IS_PROGRESS (progress), FALSE);
  g_return_val_if_fail (error == NULL || *error == NULL, FALSE);

  new_layer_format = ligma_babl_format (new_type,
                                       ligma_image_get_precision (image),
                                       TRUE,
                                       ligma_image_get_layer_space (image));

  if (dest_profile &&
      ! ligma_image_validate_color_profile_by_format (new_layer_format,
                                                     dest_profile,
                                                     NULL, error))
    {
      return FALSE;
    }

  switch (new_type)
    {
    case LIGMA_RGB:
      undo_desc = C_("undo-type", "Convert Image to RGB");
      break;

    case LIGMA_GRAY:
      undo_desc = C_("undo-type", "Convert Image to Grayscale");
      break;

    default:
      g_return_val_if_reached (FALSE);
    }

  ligma_set_busy (image->ligma);

  queue    = ligma_object_queue_new (progress);
  progress = LIGMA_PROGRESS (queue);

  all_layers = ligma_image_get_layer_list (image);
  ligma_object_queue_push_list (queue, all_layers);
  g_list_free (all_layers);

  g_object_freeze_notify (G_OBJECT (image));

  ligma_image_undo_group_start (image, LIGMA_UNDO_GROUP_IMAGE_CONVERT,
                               undo_desc);

  src_profile = ligma_color_managed_get_color_profile (LIGMA_COLOR_MANAGED (image));

  /*  Push the image type to the stack  */
  ligma_image_undo_push_image_type (image, NULL);

  /*  Set the new base type  */
  old_type = ligma_image_get_base_type (image);

  g_object_set (image, "base-type", new_type, NULL);

  /*  When converting to/from GRAY, always convert to the new type's
   *  builtin profile as a fallback, we need one for convert_type on
   *  the same image
   */
  if (old_type == LIGMA_GRAY ||
      new_type == LIGMA_GRAY)
    {
      if (! dest_profile)
        dest_profile = ligma_image_get_builtin_color_profile (image);
    }

  while ((drawable = ligma_object_queue_pop (queue)))
    {
      ligma_drawable_convert_type (drawable, image,
                                  new_type,
                                  ligma_drawable_get_precision (drawable),
                                  ligma_drawable_has_alpha (drawable),
                                  src_profile,
                                  dest_profile,
                                  GEGL_DITHER_NONE, GEGL_DITHER_NONE,
                                  TRUE, progress);
    }

  if (old_type == LIGMA_INDEXED)
    ligma_image_unset_colormap (image, TRUE);

  /*  When converting to/from GRAY, set the new profile.
   */
  if (old_type == LIGMA_GRAY ||
      new_type == LIGMA_GRAY)
    {
      ligma_image_set_color_profile (image, dest_profile, NULL);
    }

  ligma_image_undo_group_end (image);

  ligma_image_mode_changed (image);
  g_object_thaw_notify (G_OBJECT (image));

  g_object_unref (queue);

  ligma_unset_busy (image->ligma);

  return TRUE;
}
