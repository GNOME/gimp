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

#include "libligmabase/ligmabase.h"
#include "libligmacolor/ligmacolor.h"
#include "libligmaconfig/ligmaconfig.h"

#include "core/core-types.h"

#include "core/ligma.h"
#include "core/ligmabrush.h"
#include "core/ligmabrush-load.h"
#include "core/ligmabrush-private.h"
#include "core/ligmadrawable.h"
#include "core/ligmaimage.h"
#include "core/ligmalayer-new.h"
#include "core/ligmaimage-resize.h"
#include "core/ligmaparamspecs.h"
#include "core/ligmatempbuf.h"

#include "pdb/ligmaprocedure.h"

#include "file-data-gbr.h"

#include "ligma-intl.h"


/*  local function prototypes  */

static LigmaImage * file_gbr_brush_to_image (Ligma         *ligma,
                                            LigmaBrush    *brush);
static LigmaBrush * file_gbr_image_to_brush (LigmaImage    *image,
                                            LigmaDrawable *drawable,
                                            const gchar  *name,
                                            gdouble       spacing);


/*  public functions  */

LigmaValueArray *
file_gbr_load_invoker (LigmaProcedure         *procedure,
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
      LigmaBrush *brush = ligma_brush_load_brush (context, file, input, error);

      if (brush)
        {
          image = file_gbr_brush_to_image (ligma, brush);
          g_object_unref (brush);
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
file_gbr_save_invoker (LigmaProcedure         *procedure,
                       Ligma                  *ligma,
                       LigmaContext           *context,
                       LigmaProgress          *progress,
                       const LigmaValueArray  *args,
                       GError               **error)
{
  LigmaValueArray *return_vals;
  LigmaImage      *image;
  LigmaDrawable   *drawable;
  LigmaBrush      *brush;
  const gchar    *name;
  GFile          *file;
  gint            spacing;
  gboolean        success;

  ligma_set_busy (ligma);

  image    = g_value_get_object (ligma_value_array_index (args, 1));
  drawable = g_value_get_object (ligma_value_array_index (args, 2));
  file     = g_value_get_object (ligma_value_array_index (args, 3));
  spacing  = g_value_get_int    (ligma_value_array_index (args, 4));
  name     = g_value_get_string (ligma_value_array_index (args, 5));

  brush = file_gbr_image_to_brush (image, drawable, name, spacing);

  ligma_data_set_file (LIGMA_DATA (brush), file, TRUE, TRUE);

  success = ligma_data_save (LIGMA_DATA (brush), error);

  g_object_unref (brush);

  return_vals = ligma_procedure_get_return_values (procedure, success,
                                                  error ? *error : NULL);

  ligma_unset_busy (ligma);

  return return_vals;
}

LigmaLayer *
file_gbr_brush_to_layer (LigmaImage *image,
                         LigmaBrush *brush)
{
  LigmaLayer    *layer;
  const Babl   *format;
  gboolean      alpha;
  gint          width;
  gint          height;
  gint          image_width;
  gint          image_height;
  LigmaTempBuf  *mask;
  LigmaTempBuf  *pixmap;
  GeglBuffer   *buffer;

  g_return_val_if_fail (LIGMA_IS_IMAGE (image), NULL);
  g_return_val_if_fail (LIGMA_IS_BRUSH (brush), NULL);

  mask   = ligma_brush_get_mask   (brush);
  pixmap = ligma_brush_get_pixmap (brush);

  if (pixmap)
    alpha = TRUE;
  else
    alpha = FALSE;

  width  = ligma_temp_buf_get_width  (mask);
  height = ligma_temp_buf_get_height (mask);

  image_width  = ligma_image_get_width  (image);
  image_height = ligma_image_get_height (image);

  if (width > image_width || height > image_height)
    {
      gint new_width  = MAX (image_width,  width);
      gint new_height = MAX (image_height, height);

      ligma_image_resize (image, ligma_get_user_context (image->ligma),
                         new_width, new_height,
                         (new_width  - image_width) / 2,
                         (new_height - image_height) / 2,
                         NULL);

      image_width  = new_width;
      image_height = new_height;
    }

  format = ligma_image_get_layer_format (image, alpha);

  layer = ligma_layer_new (image, width, height, format,
                          ligma_object_get_name (brush),
                          1.0, LIGMA_LAYER_MODE_NORMAL);

  ligma_item_set_offset (LIGMA_ITEM (layer),
                        (image_width  - width)  / 2,
                        (image_height - height) / 2);

  buffer = ligma_drawable_get_buffer (LIGMA_DRAWABLE (layer));

  if (pixmap)
    {
      guchar *pixmap_data;
      guchar *mask_data;
      guchar *p;
      guchar *m;
      gint    i;

      gegl_buffer_set (buffer, GEGL_RECTANGLE (0, 0, width, height), 0,
                       babl_format ("R'G'B' u8"),
                       ligma_temp_buf_get_data (pixmap), GEGL_AUTO_ROWSTRIDE);

      pixmap_data = gegl_buffer_linear_open (buffer, NULL, NULL, NULL);
      mask_data   = ligma_temp_buf_get_data (mask);

      for (i = 0, p = pixmap_data, m = mask_data;
           i < width * height;
           i++, p += 4, m += 1)
        {
          p[3] = *m;
        }

      gegl_buffer_linear_close (buffer, pixmap_data);
    }
  else
    {
      guchar *mask_data = ligma_temp_buf_get_data (mask);
      gint    i;

      for (i = 0; i < width * height; i++)
        mask_data[i] = 255 - mask_data[i];

      gegl_buffer_set (buffer, GEGL_RECTANGLE (0, 0, width, height), 0,
                       babl_format ("Y' u8"),
                       mask_data, GEGL_AUTO_ROWSTRIDE);
    }

  return layer;
}

LigmaBrush *
file_gbr_drawable_to_brush (LigmaDrawable        *drawable,
                            const GeglRectangle *rect,
                            const gchar         *name,
                            gdouble              spacing)
{
  LigmaBrush   *brush;
  GeglBuffer  *buffer;
  LigmaTempBuf *mask;
  LigmaTempBuf *pixmap = NULL;
  gint         width;
  gint         height;

  g_return_val_if_fail (LIGMA_IS_DRAWABLE (drawable), NULL);
  g_return_val_if_fail (rect != NULL, NULL);

  buffer = ligma_drawable_get_buffer (drawable);
  width  = rect->width;
  height = rect->height;

  brush = g_object_new (LIGMA_TYPE_BRUSH,
                        "name",      name,
                        "mime-type", "image/x-ligma-gbr",
                        "spacing",   spacing,
                        NULL);

  mask = ligma_temp_buf_new (width, height, babl_format ("Y u8"));

  if (ligma_drawable_is_gray (drawable))
    {
      guchar *m = ligma_temp_buf_get_data (mask);
      gint    i;

      if (ligma_drawable_has_alpha (drawable))
        {
          GeglBufferIterator *iter;
          LigmaRGB             white;

          ligma_rgba_set_uchar (&white, 255, 255, 255, 255);

          iter = gegl_buffer_iterator_new (buffer, rect, 0,
                                           babl_format ("Y'A u8"),
                                           GEGL_ACCESS_READ, GEGL_ABYSS_NONE,
                                           1);

          while (gegl_buffer_iterator_next (iter))
            {
              guint8 *data = (guint8 *) iter->items[0].data;
              gint    j;

              for (j = 0; j < iter->length; j++)
                {
                  LigmaRGB gray;
                  gint    x, y;
                  gint    dest;

                  ligma_rgba_set_uchar (&gray,
                                       data[0], data[0], data[0],
                                       data[1]);

                  ligma_rgb_composite (&gray, &white,
                                      LIGMA_RGB_COMPOSITE_BEHIND);

                  x = iter->items[0].roi.x + j % iter->items[0].roi.width;
                  y = iter->items[0].roi.y + j / iter->items[0].roi.width;

                  dest = y * width + x;

                  ligma_rgba_get_uchar (&gray, &m[dest], NULL, NULL, NULL);

                  data += 2;
                }
            }
        }
      else
        {
          gegl_buffer_get (buffer, rect, 1.0,
                           babl_format ("Y' u8"), m,
                           GEGL_AUTO_ROWSTRIDE, GEGL_ABYSS_NONE);
        }

      /*  invert  */
      for (i = 0; i < width * height; i++)
        m[i] = 255 - m[i];
    }
  else
    {
      pixmap = ligma_temp_buf_new (width, height, babl_format ("R'G'B' u8"));

      gegl_buffer_get (buffer, rect, 1.0,
                       babl_format ("R'G'B' u8"),
                       ligma_temp_buf_get_data (pixmap),
                       GEGL_AUTO_ROWSTRIDE, GEGL_ABYSS_NONE);

      gegl_buffer_get (buffer, rect, 1.0,
                       babl_format ("A u8"),
                       ligma_temp_buf_get_data (mask),
                       GEGL_AUTO_ROWSTRIDE, GEGL_ABYSS_NONE);
    }


  brush->priv->mask   = mask;
  brush->priv->pixmap = pixmap;

  return brush;
}


/*  private functions  */

static LigmaImage *
file_gbr_brush_to_image (Ligma      *ligma,
                         LigmaBrush *brush)
{
  LigmaImage         *image;
  LigmaLayer         *layer;
  LigmaImageBaseType  base_type;
  gint               width;
  gint               height;
  LigmaTempBuf       *mask   = ligma_brush_get_mask   (brush);
  LigmaTempBuf       *pixmap = ligma_brush_get_pixmap (brush);
  GString           *string;
  LigmaConfigWriter  *writer;
  LigmaParasite      *parasite;

  if (pixmap)
    base_type = LIGMA_RGB;
  else
    base_type = LIGMA_GRAY;

  width  = ligma_temp_buf_get_width  (mask);
  height = ligma_temp_buf_get_height (mask);

  image = ligma_image_new (ligma, width, height, base_type,
                          LIGMA_PRECISION_U8_NON_LINEAR);

  string = g_string_new (NULL);
  writer = ligma_config_writer_new_from_string (string);

  ligma_config_writer_open (writer, "spacing");
  ligma_config_writer_printf (writer, "%d", ligma_brush_get_spacing (brush));
  ligma_config_writer_close (writer);

  ligma_config_writer_linefeed (writer);

  ligma_config_writer_open (writer, "description");
  ligma_config_writer_string (writer, ligma_object_get_name (brush));
  ligma_config_writer_close (writer);

  ligma_config_writer_finish (writer, NULL, NULL);

  parasite = ligma_parasite_new ("LigmaProcedureConfig-file-gbr-save-last",
                                LIGMA_PARASITE_PERSISTENT,
                                string->len + 1, string->str);
  ligma_image_parasite_attach (image, parasite, FALSE);
  ligma_parasite_free (parasite);

  g_string_free (string, TRUE);

  layer = file_gbr_brush_to_layer (image, brush);
  ligma_image_add_layer (image, layer, NULL, 0, FALSE);

  return image;
}

static LigmaBrush *
file_gbr_image_to_brush (LigmaImage    *image,
                         LigmaDrawable *drawable,
                         const gchar  *name,
                         gdouble       spacing)
{
  gint width  = ligma_item_get_width  (LIGMA_ITEM (drawable));
  gint height = ligma_item_get_height (LIGMA_ITEM (drawable));

  return file_gbr_drawable_to_brush (drawable,
                                     GEGL_RECTANGLE (0, 0, width, height),
                                     name, spacing);
}
