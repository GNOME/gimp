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

#include "libgimpbase/gimpbase.h"
#include "libgimpcolor/gimpcolor.h"

#include "core/core-types.h"

#include "core/gimp.h"
#include "core/gimpbrush.h"
#include "core/gimpbrush-load.h"
#include "core/gimpbrush-private.h"
#include "core/gimpdrawable.h"
#include "core/gimpimage.h"
#include "core/gimplayer-new.h"
#include "core/gimpimage-resize.h"
#include "core/gimpparamspecs.h"
#include "core/gimptempbuf.h"

#include "pdb/gimpprocedure.h"

#include "file-data-gbr.h"

#include "gimp-intl.h"


/*  local function prototypes  */

static GimpImage * file_gbr_brush_to_image (Gimp         *gimp,
                                            GimpBrush    *brush);
static GimpBrush * file_gbr_image_to_brush (GimpImage    *image,
                                            GimpDrawable *drawable,
                                            const gchar  *name,
                                            gdouble       spacing);


/*  public functions  */

GimpValueArray *
file_gbr_load_invoker (GimpProcedure         *procedure,
                       Gimp                  *gimp,
                       GimpContext           *context,
                       GimpProgress          *progress,
                       const GimpValueArray  *args,
                       GError               **error)
{
  GimpValueArray *return_vals;
  GimpImage      *image = NULL;
  const gchar    *uri;
  GFile          *file;
  GInputStream   *input;
  GError         *my_error = NULL;

  gimp_set_busy (gimp);

  uri  = g_value_get_string (gimp_value_array_index (args, 1));
  file = g_file_new_for_uri (uri);

  input = G_INPUT_STREAM (g_file_read (file, NULL, &my_error));

  if (input)
    {
      GimpBrush *brush = gimp_brush_load_brush (context, file, input, error);

      if (brush)
        {
          image = file_gbr_brush_to_image (gimp, brush);
          g_object_unref (brush);
        }

      g_object_unref (input);
    }
  else
    {
      g_propagate_prefixed_error (error, my_error,
                                  _("Could not open '%s' for reading: "),
                                  gimp_file_get_utf8_name (file));
    }

  g_object_unref (file);

  return_vals = gimp_procedure_get_return_values (procedure, image != NULL,
                                                  error ? *error : NULL);

  if (image)
    gimp_value_set_image (gimp_value_array_index (return_vals, 1), image);

  gimp_unset_busy (gimp);

  return return_vals;
}

GimpValueArray *
file_gbr_save_invoker (GimpProcedure         *procedure,
                       Gimp                  *gimp,
                       GimpContext           *context,
                       GimpProgress          *progress,
                       const GimpValueArray  *args,
                       GError               **error)
{
  GimpValueArray *return_vals;
  GimpImage      *image;
  GimpDrawable   *drawable;
  GimpBrush      *brush;
  const gchar    *uri;
  const gchar    *name;
  GFile          *file;
  gint            spacing;
  gboolean        success;

  gimp_set_busy (gimp);

  image    = gimp_value_get_image (gimp_value_array_index (args, 1), gimp);
  drawable = gimp_value_get_drawable (gimp_value_array_index (args, 2), gimp);
  uri      = g_value_get_string (gimp_value_array_index (args, 3));
  spacing  = g_value_get_int (gimp_value_array_index (args, 5));
  name     = g_value_get_string (gimp_value_array_index (args, 6));

  file = g_file_new_for_uri (uri);

  brush = file_gbr_image_to_brush (image, drawable, name, spacing);

  gimp_data_set_file (GIMP_DATA (brush), file, TRUE, TRUE);

  success = gimp_data_save (GIMP_DATA (brush), error);

  g_object_unref (brush);
  g_object_unref (file);

  return_vals = gimp_procedure_get_return_values (procedure, success,
                                                  error ? *error : NULL);

  gimp_unset_busy (gimp);

  return return_vals;
}

GimpLayer *
file_gbr_brush_to_layer (GimpImage *image,
                         GimpBrush *brush)
{
  GimpLayer    *layer;
  const Babl   *format;
  gboolean      alpha;
  gint          width;
  gint          height;
  gint          image_width;
  gint          image_height;
  GimpTempBuf  *mask;
  GimpTempBuf  *pixmap;
  GeglBuffer   *buffer;

  g_return_val_if_fail (GIMP_IS_IMAGE (image), NULL);
  g_return_val_if_fail (GIMP_IS_BRUSH (brush), NULL);

  mask   = gimp_brush_get_mask   (brush);
  pixmap = gimp_brush_get_pixmap (brush);

  if (pixmap)
    alpha = TRUE;
  else
    alpha = FALSE;

  width  = gimp_temp_buf_get_width  (mask);
  height = gimp_temp_buf_get_height (mask);

  image_width  = gimp_image_get_width  (image);
  image_height = gimp_image_get_height (image);

  if (width > image_width || height > image_height)
    {
      gint new_width  = MAX (image_width,  width);
      gint new_height = MAX (image_height, height);

      gimp_image_resize (image, gimp_get_user_context (image->gimp),
                         new_width, new_height,
                         (new_width  - image_width) / 2,
                         (new_height - image_height) / 2,
                         NULL);

      image_width  = new_width;
      image_height = new_height;
    }

  format = gimp_image_get_layer_format (image, alpha);

  layer = gimp_layer_new (image, width, height, format,
                          gimp_object_get_name (brush),
                          1.0, GIMP_LAYER_MODE_NORMAL);

  gimp_item_set_offset (GIMP_ITEM (layer),
                        (image_width  - width)  / 2,
                        (image_height - height) / 2);

  buffer = gimp_drawable_get_buffer (GIMP_DRAWABLE (layer));

  if (pixmap)
    {
      guchar *pixmap_data;
      guchar *mask_data;
      guchar *p;
      guchar *m;
      gint    i;

      gegl_buffer_set (buffer, GEGL_RECTANGLE (0, 0, width, height), 0,
                       babl_format ("R'G'B' u8"),
                       gimp_temp_buf_get_data (pixmap), GEGL_AUTO_ROWSTRIDE);

      pixmap_data = gegl_buffer_linear_open (buffer, NULL, NULL, NULL);
      mask_data   = gimp_temp_buf_get_data (mask);

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
      guchar *mask_data = gimp_temp_buf_get_data (mask);
      gint    i;

      for (i = 0; i < width * height; i++)
        mask_data[i] = 255 - mask_data[i];

      gegl_buffer_set (buffer, GEGL_RECTANGLE (0, 0, width, height), 0,
                       babl_format ("Y' u8"),
                       mask_data, GEGL_AUTO_ROWSTRIDE);
    }

  return layer;
}

GimpBrush *
file_gbr_drawable_to_brush (GimpDrawable        *drawable,
                            const GeglRectangle *rect,
                            const gchar         *name,
                            gdouble              spacing)
{
  GimpBrush   *brush;
  GeglBuffer  *buffer;
  GimpTempBuf *mask;
  GimpTempBuf *pixmap = NULL;
  gint         width;
  gint         height;

  g_return_val_if_fail (GIMP_IS_DRAWABLE (drawable), NULL);
  g_return_val_if_fail (rect != NULL, NULL);

  buffer = gimp_drawable_get_buffer (drawable);
  width  = rect->width;
  height = rect->height;

  brush = g_object_new (GIMP_TYPE_BRUSH,
                        "name",      name,
                        "mime-type", "image/x-gimp-gbr",
                        "spacing",   spacing,
                        NULL);

  mask = gimp_temp_buf_new (width, height, babl_format ("Y u8"));

  if (gimp_drawable_is_gray (drawable))
    {
      guchar *m = gimp_temp_buf_get_data (mask);
      gint    i;

      if (gimp_drawable_has_alpha (drawable))
        {
          GeglBufferIterator *iter;
          GimpRGB             white;

          gimp_rgba_set_uchar (&white, 255, 255, 255, 255);

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
                  GimpRGB gray;
                  gint    x, y;
                  gint    dest;

                  gimp_rgba_set_uchar (&gray,
                                       data[0], data[0], data[0],
                                       data[1]);

                  gimp_rgb_composite (&gray, &white,
                                      GIMP_RGB_COMPOSITE_BEHIND);

                  x = iter->items[0].roi.x + j % iter->items[0].roi.width;
                  y = iter->items[0].roi.y + j / iter->items[0].roi.width;

                  dest = y * width + x;

                  gimp_rgba_get_uchar (&gray, &m[dest], NULL, NULL, NULL);

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
      pixmap = gimp_temp_buf_new (width, height, babl_format ("R'G'B' u8"));

      gegl_buffer_get (buffer, rect, 1.0,
                       babl_format ("R'G'B' u8"),
                       gimp_temp_buf_get_data (pixmap),
                       GEGL_AUTO_ROWSTRIDE, GEGL_ABYSS_NONE);

      gegl_buffer_get (buffer, rect, 1.0,
                       babl_format ("A u8"),
                       gimp_temp_buf_get_data (mask),
                       GEGL_AUTO_ROWSTRIDE, GEGL_ABYSS_NONE);
    }


  brush->priv->mask   = mask;
  brush->priv->pixmap = pixmap;

  return brush;
}


/*  private functions  */

static GimpImage *
file_gbr_brush_to_image (Gimp      *gimp,
                         GimpBrush *brush)
{
  GimpImage         *image;
  GimpLayer         *layer;
  const gchar       *name;
  GimpImageBaseType  base_type;
  gint               width;
  gint               height;
  GimpTempBuf       *mask   = gimp_brush_get_mask   (brush);
  GimpTempBuf       *pixmap = gimp_brush_get_pixmap (brush);
  GimpParasite      *parasite;

  if (pixmap)
    base_type = GIMP_RGB;
  else
    base_type = GIMP_GRAY;

  name   = gimp_object_get_name (brush);
  width  = gimp_temp_buf_get_width  (mask);
  height = gimp_temp_buf_get_height (mask);

  image = gimp_image_new (gimp, width, height, base_type,
                          GIMP_PRECISION_U8_GAMMA);

  parasite = gimp_parasite_new ("gimp-brush-name",
                                GIMP_PARASITE_PERSISTENT,
                                strlen (name) + 1, name);
  gimp_image_parasite_attach (image, parasite, FALSE);
  gimp_parasite_free (parasite);

  layer = file_gbr_brush_to_layer (image, brush);
  gimp_image_add_layer (image, layer, NULL, 0, FALSE);

  return image;
}

static GimpBrush *
file_gbr_image_to_brush (GimpImage    *image,
                         GimpDrawable *drawable,
                         const gchar  *name,
                         gdouble       spacing)
{
  gint width  = gimp_item_get_width  (GIMP_ITEM (drawable));
  gint height = gimp_item_get_height (GIMP_ITEM (drawable));

  return file_gbr_drawable_to_brush (drawable,
                                     GEGL_RECTANGLE (0, 0, width, height),
                                     name, spacing);
}
