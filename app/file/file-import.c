/* GIMP - The GNU Image Manipulation Program
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * file-import.c
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

#include <gdk-pixbuf/gdk-pixbuf.h>
#include <gegl.h>

#include "core/core-types.h"

#include "config/gimpcoreconfig.h"

#include "core/gimp.h"
#include "core/gimpcontext.h"
#include "core/gimpimage.h"
#include "core/gimpimage-color-profile.h"
#include "core/gimpimage-convert-precision.h"
#include "core/gimpimage-rotate.h"
#include "core/gimplayer.h"
#include "core/gimpprogress.h"

#include "text/gimptextlayer.h"

#include "file-import.h"


/*  public functions  */

void
file_import_image (GimpImage    *image,
                   GimpContext  *context,
                   GFile        *file,
                   gboolean      interactive,
                   GimpProgress *progress)
{
  GimpCoreConfig *config;

  g_return_if_fail (GIMP_IS_IMAGE (image));
  g_return_if_fail (GIMP_IS_CONTEXT (context));
  g_return_if_fail (G_IS_FILE (file));
  g_return_if_fail (progress == NULL || GIMP_IS_PROGRESS (progress));

  config = image->gimp->config;

  if (interactive && gimp_image_get_base_type (image) != GIMP_INDEXED)
    {
      if (config->import_promote_float)
        {
          GimpPrecision old_precision = gimp_image_get_precision (image);

          if (old_precision != GIMP_PRECISION_FLOAT_LINEAR)
            {
              gimp_image_convert_precision (image,
                                            GIMP_PRECISION_FLOAT_LINEAR,
                                            GEGL_DITHER_NONE,
                                            GEGL_DITHER_NONE,
                                            GEGL_DITHER_NONE,
                                            progress);

              if (config->import_promote_dither &&
                  old_precision == GIMP_PRECISION_U8_NON_LINEAR)
                {
                  gimp_image_convert_dither_u8 (image, progress);
                }
            }
        }

      if (config->import_add_alpha)
        {
          GList *layers = gimp_image_get_layer_list (image);
          GList *list;

          for (list = layers; list; list = g_list_next (list))
            {
              if (! gimp_viewable_get_children (list->data) &&
                  ! gimp_item_is_text_layer (list->data)    &&
                  ! gimp_drawable_has_alpha (list->data))
                {
                  gimp_layer_add_alpha (list->data);
                }
            }

          g_list_free (layers);
        }
    }

  gimp_image_import_color_profile (image, context, progress, interactive);
  gimp_image_import_rotation_metadata (image, context, progress, interactive);

  /* Remember the import source */
  gimp_image_set_imported_file (image, file);

  /* We shall treat this file as an Untitled file */
  gimp_image_set_file (image, NULL);
}
