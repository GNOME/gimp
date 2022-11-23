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

#include <gdk-pixbuf/gdk-pixbuf.h>
#include <gegl.h>

#include "libligmabase/ligmabase.h"
#include "libligmaconfig/ligmaconfig.h"

#include "core/core-types.h"

#include "gegl/ligma-babl.h"

#include "core/ligma.h"
#include "core/ligmacontainer.h"
#include "core/ligmadrawable.h"
#include "core/ligmaimage.h"
#include "core/ligmaimage-new.h"
#include "core/ligmaimage-resize.h"
#include "core/ligmalayer-new.h"
#include "core/ligmaparamspecs.h"
#include "core/ligmapattern.h"
#include "core/ligmapattern-load.h"
#include "core/ligmapickable.h"
#include "core/ligmatempbuf.h"

#include "pdb/ligmaprocedure.h"

#include "file-data-pat.h"

#include "ligma-intl.h"


/*  local function prototypes  */

static LigmaImage   * file_pat_pattern_to_image (Ligma          *ligma,
                                                LigmaPattern   *pattern);
static LigmaPattern * file_pat_image_to_pattern (LigmaImage     *image,
                                                LigmaContext   *context,
                                                gint           n_drawables,
                                                LigmaDrawable **drawables,
                                                const gchar   *name);


/*  public functions  */

LigmaValueArray *
file_pat_load_invoker (LigmaProcedure         *procedure,
                       Ligma                  *ligma,
                       LigmaContext           *context,
                       LigmaProgress          *progress,
                       const LigmaValueArray  *args,
                       GError               **error)
{
  LigmaValueArray *return_vals;
  LigmaImage      *image = NULL;
  GFile          *file;
  GInputStream   *input;
  GError         *my_error = NULL;

  ligma_set_busy (ligma);

  file = g_value_get_object (ligma_value_array_index (args, 1));

  input = G_INPUT_STREAM (g_file_read (file, NULL, &my_error));

  if (input)
    {
      GList *list = ligma_pattern_load (context, file, input, error);

      if (list)
        {
          LigmaPattern *pattern = list->data;

          g_list_free (list);

          image = file_pat_pattern_to_image (ligma, pattern);
          g_object_unref (pattern);
        }

      g_object_unref (input);
    }
  else
    {
      g_propagate_prefixed_error (error, my_error,
                                  _("Could not open '%s' for reading: "),
                                  ligma_file_get_utf8_name (file));
    }

  return_vals = ligma_procedure_get_return_values (procedure, image != NULL,
                                                  error ? *error : NULL);

  if (image)
    g_value_set_object (ligma_value_array_index (return_vals, 1), image);

  ligma_unset_busy (ligma);

  return return_vals;
}

LigmaValueArray *
file_pat_save_invoker (LigmaProcedure         *procedure,
                       Ligma                  *ligma,
                       LigmaContext           *context,
                       LigmaProgress          *progress,
                       const LigmaValueArray  *args,
                       GError               **error)
{
  LigmaValueArray  *return_vals;
  LigmaImage       *image;
  LigmaPattern     *pattern;
  const gchar     *name;
  GFile           *file;
  LigmaDrawable   **drawables;
  gint             n_drawables;
  gboolean         success;

  ligma_set_busy (ligma);

  image       = g_value_get_object (ligma_value_array_index (args, 1));
  n_drawables = g_value_get_int (ligma_value_array_index (args, 2));
  drawables   = (LigmaDrawable **) ligma_value_get_object_array (ligma_value_array_index (args, 3));
  file        = g_value_get_object (ligma_value_array_index (args, 4));
  name        = g_value_get_string (ligma_value_array_index (args, 5));

  pattern = file_pat_image_to_pattern (image, context, n_drawables, drawables, name);

  ligma_data_set_file (LIGMA_DATA (pattern), file, TRUE, TRUE);

  success = ligma_data_save (LIGMA_DATA (pattern), error);

  g_object_unref (pattern);

  return_vals = ligma_procedure_get_return_values (procedure, success,
                                                  error ? *error : NULL);

  ligma_unset_busy (ligma);

  return return_vals;
}


/*  private functions  */

static LigmaImage *
file_pat_pattern_to_image (Ligma        *ligma,
                           LigmaPattern *pattern)
{
  LigmaImage         *image;
  LigmaLayer         *layer;
  const Babl        *format;
  LigmaImageBaseType  base_type;
  gboolean           alpha;
  gint               width;
  gint               height;
  LigmaTempBuf       *mask   = ligma_pattern_get_mask (pattern);
  GeglBuffer        *buffer;
  GString           *string;
  LigmaConfigWriter  *writer;
  LigmaParasite      *parasite;

  format = ligma_temp_buf_get_format (mask);

  switch (babl_format_get_bytes_per_pixel (format))
    {
    case 1:
      base_type = LIGMA_GRAY;
      alpha     = FALSE;
      break;

    case 2:
      base_type = LIGMA_GRAY;
      alpha     = TRUE;
      break;

    case 3:
      base_type = LIGMA_RGB;
      alpha     = FALSE;
      break;

    case 4:
      base_type = LIGMA_RGB;
      alpha     = TRUE;
      break;

    default:
      g_return_val_if_reached (NULL);
    }

  width  = ligma_temp_buf_get_width  (mask);
  height = ligma_temp_buf_get_height (mask);

  image = ligma_image_new (ligma, width, height, base_type,
                          LIGMA_PRECISION_U8_NON_LINEAR);

  string = g_string_new (NULL);
  writer = ligma_config_writer_new_from_string (string);

  ligma_config_writer_open (writer, "description");
  ligma_config_writer_string (writer, ligma_object_get_name (pattern));
  ligma_config_writer_close (writer);

  ligma_config_writer_finish (writer, NULL, NULL);

  parasite = ligma_parasite_new ("LigmaProcedureConfig-file-pat-save-last",
                                LIGMA_PARASITE_PERSISTENT,
                                string->len + 1, string->str);
  ligma_image_parasite_attach (image, parasite, FALSE);
  ligma_parasite_free (parasite);

  g_string_free (string, TRUE);

  format = ligma_image_get_layer_format (image, alpha);

  layer = ligma_layer_new (image, width, height, format,
                          ligma_object_get_name (pattern),
                          1.0, LIGMA_LAYER_MODE_NORMAL);
  ligma_image_add_layer (image, layer, NULL, 0, FALSE);

  buffer = ligma_drawable_get_buffer (LIGMA_DRAWABLE (layer));

  gegl_buffer_set (buffer, GEGL_RECTANGLE (0, 0, width, height), 0,
                   NULL,
                   ligma_temp_buf_get_data (mask), GEGL_AUTO_ROWSTRIDE);

  return image;
}

static LigmaPattern *
file_pat_image_to_pattern (LigmaImage     *image,
                           LigmaContext   *context,
                           gint           n_drawables,
                           LigmaDrawable **drawables,
                           const gchar   *name)
{
  LigmaPattern *pattern;
  LigmaImage   *subimage = NULL;
  const Babl  *format;
  gint         width;
  gint         height;

  g_return_val_if_fail (n_drawables > 0, NULL);

  if (n_drawables > 1)
    {
      GList *drawable_list = NULL;

      for (gint i = 0; i < n_drawables; i++)
        drawable_list = g_list_prepend (drawable_list, drawables[i]);

      subimage = ligma_image_new_from_drawables (image->ligma, drawable_list, FALSE, FALSE);
      g_list_free (drawable_list);
      ligma_container_remove (image->ligma->images, LIGMA_OBJECT (subimage));
      ligma_image_resize_to_layers (subimage, context,
                                   NULL, NULL, NULL, NULL, NULL);
      width  = ligma_image_get_width (subimage);
      height = ligma_image_get_width (subimage);

      ligma_pickable_flush (LIGMA_PICKABLE (subimage));
    }
  else
    {
      width  = ligma_item_get_width  (LIGMA_ITEM (drawables[0]));
      height = ligma_item_get_height (LIGMA_ITEM (drawables[0]));
    }

  format = ligma_babl_format (ligma_drawable_is_gray (drawables[0]) ?
                             LIGMA_GRAY : LIGMA_RGB,
                             LIGMA_PRECISION_U8_NON_LINEAR,
                             (subimage && ligma_image_has_alpha (subimage)) ||
                             ligma_drawable_has_alpha (drawables[0]),
                             NULL);

  pattern = g_object_new (LIGMA_TYPE_PATTERN,
                          "name",      name,
                          "mime-type", "image/x-ligma-pat",
                          NULL);

  pattern->mask = ligma_temp_buf_new (width, height, format);

  gegl_buffer_get (subimage != NULL ? ligma_pickable_get_buffer (LIGMA_PICKABLE (subimage)) :
                                      ligma_drawable_get_buffer (drawables[0]),
                   GEGL_RECTANGLE (0, 0, width, height), 1.0,
                   format, ligma_temp_buf_get_data (pattern->mask),
                   GEGL_AUTO_ROWSTRIDE, GEGL_ABYSS_NONE);

  g_clear_object (&subimage);

  return pattern;
}
