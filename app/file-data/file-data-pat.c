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

#include <gdk-pixbuf/gdk-pixbuf.h>
#include <gegl.h>

#include "libgimpbase/gimpbase.h"
#include "libgimpconfig/gimpconfig.h"

#include "core/core-types.h"

#include "gegl/gimp-babl.h"

#include "core/gimp.h"
#include "core/gimpcontainer.h"
#include "core/gimpdrawable.h"
#include "core/gimpimage.h"
#include "core/gimpimage-new.h"
#include "core/gimpimage-resize.h"
#include "core/gimplayer-new.h"
#include "core/gimpparamspecs.h"
#include "core/gimppattern.h"
#include "core/gimppattern-load.h"
#include "core/gimppickable.h"
#include "core/gimptempbuf.h"

#include "pdb/gimpprocedure.h"

#include "file-data-pat.h"

#include "gimp-intl.h"


/*  local function prototypes  */

static GimpImage   * file_pat_pattern_to_image (Gimp          *gimp,
                                                GimpPattern   *pattern);
static GimpPattern * file_pat_image_to_pattern (GimpImage     *image,
                                                GimpContext   *context,
                                                gint           n_drawables,
                                                GimpDrawable **drawables,
                                                const gchar   *name);


/*  public functions  */

GimpValueArray *
file_pat_load_invoker (GimpProcedure         *procedure,
                       Gimp                  *gimp,
                       GimpContext           *context,
                       GimpProgress          *progress,
                       const GimpValueArray  *args,
                       GError               **error)
{
  GimpValueArray *return_vals;
  GimpImage      *image = NULL;
  GFile          *file;
  GInputStream   *input;
  GError         *my_error = NULL;

  gimp_set_busy (gimp);

  file = g_value_get_object (gimp_value_array_index (args, 1));

  input = G_INPUT_STREAM (g_file_read (file, NULL, &my_error));

  if (input)
    {
      GList *list = gimp_pattern_load (context, file, input, error);

      if (list)
        {
          GimpPattern *pattern = list->data;

          g_list_free (list);

          image = file_pat_pattern_to_image (gimp, pattern);
          g_object_unref (pattern);
        }

      g_object_unref (input);
    }
  else
    {
      g_propagate_prefixed_error (error, my_error,
                                  _("Could not open '%s' for reading: "),
                                  gimp_file_get_utf8_name (file));
    }

  return_vals = gimp_procedure_get_return_values (procedure, image != NULL,
                                                  error ? *error : NULL);

  if (image)
    g_value_set_object (gimp_value_array_index (return_vals, 1), image);

  gimp_unset_busy (gimp);

  return return_vals;
}

GimpValueArray *
file_pat_save_invoker (GimpProcedure         *procedure,
                       Gimp                  *gimp,
                       GimpContext           *context,
                       GimpProgress          *progress,
                       const GimpValueArray  *args,
                       GError               **error)
{
  GimpValueArray  *return_vals;
  GimpImage       *image;
  GimpPattern     *pattern;
  const gchar     *name;
  GFile           *file;
  GimpDrawable   **drawables;
  gint             n_drawables;
  gboolean         success;

  gimp_set_busy (gimp);

  image       = g_value_get_object (gimp_value_array_index (args, 1));
  n_drawables = g_value_get_int (gimp_value_array_index (args, 2));
  drawables   = (GimpDrawable **) gimp_value_get_object_array (gimp_value_array_index (args, 3));
  file        = g_value_get_object (gimp_value_array_index (args, 4));
  name        = g_value_get_string (gimp_value_array_index (args, 5));

  pattern = file_pat_image_to_pattern (image, context, n_drawables, drawables, name);

  gimp_data_set_file (GIMP_DATA (pattern), file, TRUE, TRUE);

  success = gimp_data_save (GIMP_DATA (pattern), error);

  g_object_unref (pattern);

  return_vals = gimp_procedure_get_return_values (procedure, success,
                                                  error ? *error : NULL);

  gimp_unset_busy (gimp);

  return return_vals;
}


/*  private functions  */

static GimpImage *
file_pat_pattern_to_image (Gimp        *gimp,
                           GimpPattern *pattern)
{
  GimpImage         *image;
  GimpLayer         *layer;
  const Babl        *format;
  GimpImageBaseType  base_type;
  gboolean           alpha;
  gint               width;
  gint               height;
  GimpTempBuf       *mask   = gimp_pattern_get_mask (pattern);
  GeglBuffer        *buffer;
  GString           *string;
  GimpConfigWriter  *writer;
  GimpParasite      *parasite;

  format = gimp_temp_buf_get_format (mask);

  switch (babl_format_get_bytes_per_pixel (format))
    {
    case 1:
      base_type = GIMP_GRAY;
      alpha     = FALSE;
      break;

    case 2:
      base_type = GIMP_GRAY;
      alpha     = TRUE;
      break;

    case 3:
      base_type = GIMP_RGB;
      alpha     = FALSE;
      break;

    case 4:
      base_type = GIMP_RGB;
      alpha     = TRUE;
      break;

    default:
      g_return_val_if_reached (NULL);
    }

  width  = gimp_temp_buf_get_width  (mask);
  height = gimp_temp_buf_get_height (mask);

  image = gimp_image_new (gimp, width, height, base_type,
                          GIMP_PRECISION_U8_NON_LINEAR);

  string = g_string_new (NULL);
  writer = gimp_config_writer_new_from_string (string);

  gimp_config_writer_open (writer, "description");
  gimp_config_writer_string (writer, gimp_object_get_name (pattern));
  gimp_config_writer_close (writer);

  gimp_config_writer_finish (writer, NULL, NULL);

  parasite = gimp_parasite_new ("GimpProcedureConfig-file-pat-save-last",
                                GIMP_PARASITE_PERSISTENT,
                                string->len + 1, string->str);
  gimp_image_parasite_attach (image, parasite, FALSE);
  gimp_parasite_free (parasite);

  g_string_free (string, TRUE);

  format = gimp_image_get_layer_format (image, alpha);

  layer = gimp_layer_new (image, width, height, format,
                          gimp_object_get_name (pattern),
                          1.0, GIMP_LAYER_MODE_NORMAL);
  gimp_image_add_layer (image, layer, NULL, 0, FALSE);

  buffer = gimp_drawable_get_buffer (GIMP_DRAWABLE (layer));

  gegl_buffer_set (buffer, GEGL_RECTANGLE (0, 0, width, height), 0,
                   NULL,
                   gimp_temp_buf_get_data (mask), GEGL_AUTO_ROWSTRIDE);

  return image;
}

static GimpPattern *
file_pat_image_to_pattern (GimpImage     *image,
                           GimpContext   *context,
                           gint           n_drawables,
                           GimpDrawable **drawables,
                           const gchar   *name)
{
  GimpPattern *pattern;
  GimpImage   *subimage = NULL;
  const Babl  *format;
  gint         width;
  gint         height;

  g_return_val_if_fail (n_drawables > 0, NULL);
  g_return_val_if_fail (drawables != NULL, NULL);

  if (n_drawables > 1)
    {
      GList *drawable_list = NULL;

      for (gint i = 0; i < n_drawables; i++)
        drawable_list = g_list_prepend (drawable_list, drawables[i]);

      subimage = gimp_image_new_from_drawables (image->gimp, drawable_list, FALSE, FALSE);
      g_list_free (drawable_list);
      gimp_container_remove (image->gimp->images, GIMP_OBJECT (subimage));
      gimp_image_resize_to_layers (subimage, context,
                                   NULL, NULL, NULL, NULL, NULL);
      width  = gimp_image_get_width (subimage);
      height = gimp_image_get_width (subimage);

      gimp_pickable_flush (GIMP_PICKABLE (subimage));
    }
  else
    {
      width  = gimp_item_get_width  (GIMP_ITEM (drawables[0]));
      height = gimp_item_get_height (GIMP_ITEM (drawables[0]));
    }

  format = gimp_babl_format (gimp_drawable_is_gray (drawables[0]) ?
                             GIMP_GRAY : GIMP_RGB,
                             GIMP_PRECISION_U8_NON_LINEAR,
                             (subimage && gimp_image_has_alpha (subimage)) ||
                             gimp_drawable_has_alpha (drawables[0]),
                             NULL);

  pattern = g_object_new (GIMP_TYPE_PATTERN,
                          "name",      name,
                          "mime-type", "image/x-gimp-pat",
                          NULL);

  pattern->mask = gimp_temp_buf_new (width, height, format);

  gegl_buffer_get (subimage != NULL ? gimp_pickable_get_buffer (GIMP_PICKABLE (subimage)) :
                                      gimp_drawable_get_buffer (drawables[0]),
                   GEGL_RECTANGLE (0, 0, width, height), 1.0,
                   format, gimp_temp_buf_get_data (pattern->mask),
                   GEGL_AUTO_ROWSTRIDE, GEGL_ABYSS_NONE);

  g_clear_object (&subimage);

  return pattern;
}
