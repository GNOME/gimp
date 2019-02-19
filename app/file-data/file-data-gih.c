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
#include "libgimpbase/gimpparasiteio.h"
#include "libgimpcolor/gimpcolor.h"

#include "core/core-types.h"

#include "core/gimp.h"
#include "core/gimpbrushpipe.h"
#include "core/gimpbrushpipe-load.h"
#include "core/gimpbrush-private.h"
#include "core/gimpdrawable.h"
#include "core/gimpimage.h"
#include "core/gimplayer-new.h"
#include "core/gimpparamspecs.h"
#include "core/gimptempbuf.h"

#include "pdb/gimpprocedure.h"

#include "file-data-gbr.h"
#include "file-data-gih.h"

#include "gimp-intl.h"


/*  local function prototypes  */

static GimpImage     * file_gih_pipe_to_image (Gimp          *gimp,
                                               GimpBrushPipe *pipe);
static GimpBrushPipe * file_gih_image_to_pipe (GimpImage     *image,
                                               GimpDrawable  *drawable,
                                               const gchar   *name,
                                               gdouble        spacing);


/*  public functions  */

GimpValueArray *
file_gih_load_invoker (GimpProcedure         *procedure,
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
      GList *list = gimp_brush_pipe_load (context, file, input, error);

      if (list)
        {
          GimpBrushPipe *pipe = list->data;

          g_list_free (list);

          image = file_gih_pipe_to_image (gimp, pipe);
          g_object_unref (pipe);
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
file_gih_save_invoker (GimpProcedure         *procedure,
                       Gimp                  *gimp,
                       GimpContext           *context,
                       GimpProgress          *progress,
                       const GimpValueArray  *args,
                       GError               **error)
{
  GimpValueArray *return_vals;
  GimpImage      *image;
  GimpDrawable   *drawable;
  GimpBrushPipe  *pipe;
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

  pipe = file_gih_image_to_pipe (image, drawable, name, spacing);

  gimp_data_set_file (GIMP_DATA (pipe), file, TRUE, TRUE);

  success = gimp_data_save (GIMP_DATA (pipe), error);

  g_object_unref (pipe);
  g_object_unref (file);

  return_vals = gimp_procedure_get_return_values (procedure, success,
                                                  error ? *error : NULL);

  gimp_unset_busy (gimp);

  return return_vals;
}


/*  private functions  */

static GimpImage *
file_gih_pipe_to_image (Gimp          *gimp,
                        GimpBrushPipe *pipe)
{
  GimpImage         *image;
  const gchar       *name;
  GimpImageBaseType  base_type;
  GimpParasite      *parasite;
  gint               i;

  if (gimp_brush_get_pixmap (pipe->current))
    base_type = GIMP_RGB;
  else
    base_type = GIMP_GRAY;

  name = gimp_object_get_name (pipe);

  image = gimp_image_new (gimp, 1, 1, base_type,
                          GIMP_PRECISION_U8_GAMMA);

  parasite = gimp_parasite_new ("gimp-brush-pipe-name",
                                GIMP_PARASITE_PERSISTENT,
                                strlen (name) + 1, name);
  gimp_image_parasite_attach (image, parasite);
  gimp_parasite_free (parasite);

  for (i = 0; i < pipe->n_brushes; i++)
    {
      GimpLayer *layer;

      layer = file_gbr_brush_to_layer (image, pipe->brushes[i]);
      gimp_image_add_layer (image, layer, NULL, i, FALSE);
    }

  if (pipe->params)
    {
      GimpPixPipeParams  params;
      gchar             *paramstring;

      /* Since we do not (yet) load the pipe as described in the
       * header, but use one layer per brush, we have to alter the
       * paramstring before attaching it as a parasite.
       *
       * (this comment copied over from file-gih, whatever "as
       * described in the header" means) -- mitch
       */

      gimp_pixpipe_params_init (&params);
      gimp_pixpipe_params_parse (pipe->params, &params);

      params.cellwidth  = gimp_image_get_width  (image);
      params.cellheight = gimp_image_get_height (image);
      params.cols       = 1;
      params.rows       = 1;

      paramstring = gimp_pixpipe_params_build (&params);
      if (paramstring)
        {
          parasite = gimp_parasite_new ("gimp-brush-pipe-parameters",
                                        GIMP_PARASITE_PERSISTENT,
                                        strlen (paramstring) + 1,
                                        paramstring);
          gimp_image_parasite_attach (image, parasite);
          gimp_parasite_free (parasite);
          g_free (paramstring);
        }
    }

  return image;
}

static GimpBrushPipe *
file_gih_image_to_pipe (GimpImage    *image,
                        GimpDrawable *drawable,
                        const gchar  *name,
                        gdouble       spacing)
{
#if 0
  GimpBrush   *brush;
  GeglBuffer  *buffer;
  GimpTempBuf *mask;
  GimpTempBuf *pixmap = NULL;
  gint         width;
  gint         height;

  buffer = gimp_drawable_get_buffer (drawable);
  width  = gimp_item_get_width  (GIMP_ITEM (drawable));
  height = gimp_item_get_height (GIMP_ITEM (drawable));

  brush = g_object_new (GIMP_TYPE_BRUSH,
                        "name",      name,
                        "mime-type", "image/x-gimp-gih",
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

          iter = gegl_buffer_iterator_new (buffer, NULL, 0,
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
          gegl_buffer_get (buffer, GEGL_RECTANGLE (0, 0, width, height), 1.0,
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

      gegl_buffer_get (buffer, GEGL_RECTANGLE (0, 0, width, height), 1.0,
                       babl_format ("R'G'B' u8"),
                       gimp_temp_buf_get_data (pixmap),
                       GEGL_AUTO_ROWSTRIDE, GEGL_ABYSS_NONE);

      gegl_buffer_get (buffer, GEGL_RECTANGLE (0, 0, width, height), 1.0,
                       babl_format ("A u8"),
                       gimp_temp_buf_get_data (mask),
                       GEGL_AUTO_ROWSTRIDE, GEGL_ABYSS_NONE);
    }


  brush->priv->mask   = mask;
  brush->priv->pixmap = pixmap;

  return brush;
#endif

  return NULL;
}
