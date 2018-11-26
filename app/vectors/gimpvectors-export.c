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

#include "vectors-types.h"

#include "core/gimpimage.h"
#include "core/gimpitem.h"

#include "gimpanchor.h"
#include "gimpstroke.h"
#include "gimpbezierstroke.h"
#include "gimpvectors.h"
#include "gimpvectors-export.h"

#include "gimp-intl.h"


static GString * gimp_vectors_export            (GimpImage   *image,
                                                 GimpVectors *vectors);
static void      gimp_vectors_export_image_size (GimpImage   *image,
                                                 GString     *str);
static void      gimp_vectors_export_path       (GimpVectors *vectors,
                                                 GString     *str);
static gchar   * gimp_vectors_export_path_data  (GimpVectors *vectors);


/**
 * gimp_vectors_export_file:
 * @image: the #GimpImage from which to export vectors
 * @vectors: a #GimpVectors object or %NULL to export all vectors in @image
 * @file: the file to write
 * @error: return location for errors
 *
 * Exports one or more vectors to a SVG file.
 *
 * Return value: %TRUE on success,
 *               %FALSE if there was an error writing the file
 **/
gboolean
gimp_vectors_export_file (GimpImage    *image,
                          GimpVectors  *vectors,
                          GFile        *file,
                          GError      **error)
{
  GOutputStream *output;
  GString       *string;
  GError        *my_error = NULL;

  g_return_val_if_fail (GIMP_IS_IMAGE (image), FALSE);
  g_return_val_if_fail (vectors == NULL || GIMP_IS_VECTORS (vectors), FALSE);
  g_return_val_if_fail (G_IS_FILE (file), FALSE);
  g_return_val_if_fail (error == NULL || *error == NULL, FALSE);

  output = G_OUTPUT_STREAM (g_file_replace (file,
                                            NULL, FALSE, G_FILE_CREATE_NONE,
                                            NULL, error));
  if (! output)
    return FALSE;

  string = gimp_vectors_export (image, vectors);

  if (! g_output_stream_write_all (output, string->str, string->len,
                                   NULL, NULL, &my_error))
    {
      GCancellable *cancellable = g_cancellable_new ();

      g_set_error (error, my_error->domain, my_error->code,
                   _("Writing SVG file '%s' failed: %s"),
                   gimp_file_get_utf8_name (file), my_error->message);
      g_clear_error (&my_error);
      g_string_free (string, TRUE);

      /* Cancel the overwrite initiated by g_file_replace(). */
      g_cancellable_cancel (cancellable);
      g_output_stream_close (output, cancellable, NULL);
      g_object_unref (cancellable);
      g_object_unref (output);

      return FALSE;
    }

  g_string_free (string, TRUE);
  g_object_unref (output);

  return TRUE;
}

/**
 * gimp_vectors_export_string:
 * @image: the #GimpImage from which to export vectors
 * @vectors: a #GimpVectors object or %NULL to export all vectors in @image
 *
 * Exports one or more vectors to a SVG string.
 *
 * Return value: a %NUL-terminated string that holds a complete XML document
 **/
gchar *
gimp_vectors_export_string (GimpImage   *image,
                            GimpVectors *vectors)
{
  g_return_val_if_fail (GIMP_IS_IMAGE (image), NULL);
  g_return_val_if_fail (vectors == NULL || GIMP_IS_VECTORS (vectors), NULL);

  return g_string_free (gimp_vectors_export (image, vectors), FALSE);
}

static GString *
gimp_vectors_export (GimpImage   *image,
                     GimpVectors *vectors)
{
  GString *str = g_string_new (NULL);

  g_string_append_printf (str,
                          "<?xml version=\"1.0\" encoding=\"UTF-8\" standalone=\"no\"?>\n"
                          "<!DOCTYPE svg PUBLIC \"-//W3C//DTD SVG 20010904//EN\"\n"
                          "              \"http://www.w3.org/TR/2001/REC-SVG-20010904/DTD/svg10.dtd\">\n"
                          "\n"
                          "<svg xmlns=\"http://www.w3.org/2000/svg\"\n");

  g_string_append (str, "     ");
  gimp_vectors_export_image_size (image, str);
  g_string_append_c (str, '\n');

  g_string_append_printf (str,
                          "     viewBox=\"0 0 %d %d\">\n",
                          gimp_image_get_width  (image),
                          gimp_image_get_height (image));

  if (vectors)
    {
      gimp_vectors_export_path (vectors, str);
    }
  else
    {
      GList *list;

      for (list = gimp_image_get_vectors_iter (image);
           list;
           list = list->next)
        {
          gimp_vectors_export_path (GIMP_VECTORS (list->data), str);
        }
    }

  g_string_append (str, "</svg>\n");

  return str;
}

static void
gimp_vectors_export_image_size (GimpImage *image,
                                GString   *str)
{
  GimpUnit     unit;
  const gchar *abbrev;
  gchar        wbuf[G_ASCII_DTOSTR_BUF_SIZE];
  gchar        hbuf[G_ASCII_DTOSTR_BUF_SIZE];
  gdouble      xres;
  gdouble      yres;
  gdouble      w, h;

  gimp_image_get_resolution (image, &xres, &yres);

  w = (gdouble) gimp_image_get_width  (image) / xres;
  h = (gdouble) gimp_image_get_height (image) / yres;

  /*  FIXME: should probably use the display unit here  */
  unit = gimp_image_get_unit (image);
  switch (unit)
    {
    case GIMP_UNIT_INCH:  abbrev = "in";  break;
    case GIMP_UNIT_MM:    abbrev = "mm";  break;
    case GIMP_UNIT_POINT: abbrev = "pt";  break;
    case GIMP_UNIT_PICA:  abbrev = "pc";  break;
    default:              abbrev = "cm";
      unit = GIMP_UNIT_MM;
      w /= 10.0;
      h /= 10.0;
      break;
    }

  g_ascii_formatd (wbuf, sizeof (wbuf), "%g", w * gimp_unit_get_factor (unit));
  g_ascii_formatd (hbuf, sizeof (hbuf), "%g", h * gimp_unit_get_factor (unit));

  g_string_append_printf (str,
                          "width=\"%s%s\" height=\"%s%s\"",
                          wbuf, abbrev, hbuf, abbrev);
}

static void
gimp_vectors_export_path (GimpVectors *vectors,
                          GString     *str)
{
  const gchar *name = gimp_object_get_name (vectors);
  gchar       *data = gimp_vectors_export_path_data (vectors);
  gchar       *esc_name;

  esc_name = g_markup_escape_text (name, strlen (name));

  g_string_append_printf (str,
                          "  <path id=\"%s\"\n"
                          "        fill=\"none\" stroke=\"black\" stroke-width=\"1\"\n"
                          "        d=\"%s\" />\n",
                          esc_name, data);

  g_free (esc_name);
  g_free (data);
}


#define NEWLINE "\n           "

static gchar *
gimp_vectors_export_path_data (GimpVectors *vectors)
{
  GString  *str;
  GList    *strokes;
  gchar     x_string[G_ASCII_DTOSTR_BUF_SIZE];
  gchar     y_string[G_ASCII_DTOSTR_BUF_SIZE];
  gboolean  closed = FALSE;

  str = g_string_new (NULL);

  for (strokes = vectors->strokes->head;
       strokes;
       strokes = strokes->next)
    {
      GimpStroke *stroke = strokes->data;
      GArray     *control_points;
      GimpAnchor *anchor;
      gint        i;

      if (closed)
        g_string_append_printf (str, NEWLINE);

      control_points = gimp_stroke_control_points_get (stroke, &closed);

      if (GIMP_IS_BEZIER_STROKE (stroke))
        {
          if (control_points->len >= 3)
            {
              anchor = &g_array_index (control_points, GimpAnchor, 1);
              g_ascii_formatd (x_string, G_ASCII_DTOSTR_BUF_SIZE,
                               "%.2f", anchor->position.x);
              g_ascii_formatd (y_string, G_ASCII_DTOSTR_BUF_SIZE,
                               "%.2f", anchor->position.y);
              g_string_append_printf (str, "M %s,%s", x_string, y_string);
            }

          if (control_points->len > 3)
            {
              g_string_append_printf (str, NEWLINE "C");
            }

          for (i = 2; i < (control_points->len + (closed ? 2 : - 1)); i++)
            {
              if (i > 2 && i % 3 == 2)
                g_string_append_printf (str, NEWLINE " ");

              anchor = &g_array_index (control_points, GimpAnchor,
                                       i % control_points->len);
              g_ascii_formatd (x_string, G_ASCII_DTOSTR_BUF_SIZE,
                               "%.2f", anchor->position.x);
              g_ascii_formatd (y_string, G_ASCII_DTOSTR_BUF_SIZE,
                               "%.2f", anchor->position.y);
              g_string_append_printf (str, " %s,%s", x_string, y_string);
            }

          if (closed && control_points->len > 3)
            g_string_append_printf (str, " Z");
        }
      else
        {
          g_printerr ("Unknown stroke type\n");

          if (control_points->len >= 1)
            {
              anchor = &g_array_index (control_points, GimpAnchor, 0);
              g_ascii_formatd (x_string, G_ASCII_DTOSTR_BUF_SIZE,
                               ".2f", anchor->position.x);
              g_ascii_formatd (y_string, G_ASCII_DTOSTR_BUF_SIZE,
                               ".2f", anchor->position.y);
              g_string_append_printf (str, "M %s,%s", x_string, y_string);
            }

          if (control_points->len > 1)
            {
              g_string_append_printf (str, NEWLINE "L");
            }

          for (i = 1; i < control_points->len; i++)
            {
              if (i > 1 && i % 3 == 1)
                g_string_append_printf (str, NEWLINE " ");

              anchor = &g_array_index (control_points, GimpAnchor, i);
              g_ascii_formatd (x_string, G_ASCII_DTOSTR_BUF_SIZE,
                               "%.2f", anchor->position.x);
              g_ascii_formatd (y_string, G_ASCII_DTOSTR_BUF_SIZE,
                               "%.2f", anchor->position.y);
              g_string_append_printf (str, " %s,%s", x_string, y_string);
            }

          if (closed && control_points->len > 1)
            g_string_append_printf (str, " Z");
        }

      g_array_free (control_points, TRUE);
    }

  return g_strchomp (g_string_free (str, FALSE));
}
