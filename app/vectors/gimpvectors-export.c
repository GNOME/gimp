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

#include "vectors-types.h"

#include "core/ligmaimage.h"
#include "core/ligmaitem.h"

#include "ligmaanchor.h"
#include "ligmastroke.h"
#include "ligmabezierstroke.h"
#include "ligmavectors.h"
#include "ligmavectors-export.h"

#include "ligma-intl.h"


static GString * ligma_vectors_export            (LigmaImage   *image,
                                                 GList       *vectors);
static void      ligma_vectors_export_image_size (LigmaImage   *image,
                                                 GString     *str);
static void      ligma_vectors_export_path       (LigmaVectors *vectors,
                                                 GString     *str);
static gchar   * ligma_vectors_export_path_data  (LigmaVectors *vectors);


/**
 * ligma_vectors_export_file:
 * @image: the #LigmaImage from which to export vectors
 * @vectors: a #GList of #LigmaVectors objects or %NULL to export all vectors in @image
 * @file: the file to write
 * @error: return location for errors
 *
 * Exports one or more vectors to a SVG file.
 *
 * Returns: %TRUE on success,
 *               %FALSE if there was an error writing the file
 **/
gboolean
ligma_vectors_export_file (LigmaImage    *image,
                          GList        *vectors,
                          GFile        *file,
                          GError      **error)
{
  GOutputStream *output;
  GString       *string;
  GError        *my_error = NULL;

  g_return_val_if_fail (LIGMA_IS_IMAGE (image), FALSE);
  g_return_val_if_fail (G_IS_FILE (file), FALSE);
  g_return_val_if_fail (error == NULL || *error == NULL, FALSE);

  output = G_OUTPUT_STREAM (g_file_replace (file,
                                            NULL, FALSE, G_FILE_CREATE_NONE,
                                            NULL, error));
  if (! output)
    return FALSE;

  string = ligma_vectors_export (image, vectors);

  if (! g_output_stream_write_all (output, string->str, string->len,
                                   NULL, NULL, &my_error))
    {
      GCancellable *cancellable = g_cancellable_new ();

      g_set_error (error, my_error->domain, my_error->code,
                   _("Writing SVG file '%s' failed: %s"),
                   ligma_file_get_utf8_name (file), my_error->message);
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
 * ligma_vectors_export_string:
 * @image: the #LigmaImage from which to export vectors
 * @vectors: a #LigmaVectors object or %NULL to export all vectors in @image
 *
 * Exports one or more vectors to a SVG string.
 *
 * Returns: a %NUL-terminated string that holds a complete XML document
 **/
gchar *
ligma_vectors_export_string (LigmaImage *image,
                            GList     *vectors)
{
  g_return_val_if_fail (LIGMA_IS_IMAGE (image), NULL);

  return g_string_free (ligma_vectors_export (image, vectors), FALSE);
}

static GString *
ligma_vectors_export (LigmaImage *image,
                     GList     *vectors)
{
  GString *str = g_string_new (NULL);
  GList   *list;

  g_string_append_printf (str,
                          "<?xml version=\"1.0\" encoding=\"UTF-8\" standalone=\"no\"?>\n"
                          "<!DOCTYPE svg PUBLIC \"-//W3C//DTD SVG 20010904//EN\"\n"
                          "              \"http://www.w3.org/TR/2001/REC-SVG-20010904/DTD/svg10.dtd\">\n"
                          "\n"
                          "<svg xmlns=\"http://www.w3.org/2000/svg\"\n");

  g_string_append (str, "     ");
  ligma_vectors_export_image_size (image, str);
  g_string_append_c (str, '\n');

  g_string_append_printf (str,
                          "     viewBox=\"0 0 %d %d\">\n",
                          ligma_image_get_width  (image),
                          ligma_image_get_height (image));

  if (! vectors)
    vectors = ligma_image_get_vectors_iter (image);

  for (list = vectors; list; list = list->next)
    ligma_vectors_export_path (LIGMA_VECTORS (list->data), str);

  g_string_append (str, "</svg>\n");

  return str;
}

static void
ligma_vectors_export_image_size (LigmaImage *image,
                                GString   *str)
{
  LigmaUnit     unit;
  const gchar *abbrev;
  gchar        wbuf[G_ASCII_DTOSTR_BUF_SIZE];
  gchar        hbuf[G_ASCII_DTOSTR_BUF_SIZE];
  gdouble      xres;
  gdouble      yres;
  gdouble      w, h;

  ligma_image_get_resolution (image, &xres, &yres);

  w = (gdouble) ligma_image_get_width  (image) / xres;
  h = (gdouble) ligma_image_get_height (image) / yres;

  /*  FIXME: should probably use the display unit here  */
  unit = ligma_image_get_unit (image);
  switch (unit)
    {
    case LIGMA_UNIT_INCH:  abbrev = "in";  break;
    case LIGMA_UNIT_MM:    abbrev = "mm";  break;
    case LIGMA_UNIT_POINT: abbrev = "pt";  break;
    case LIGMA_UNIT_PICA:  abbrev = "pc";  break;
    default:              abbrev = "cm";
      unit = LIGMA_UNIT_MM;
      w /= 10.0;
      h /= 10.0;
      break;
    }

  g_ascii_formatd (wbuf, sizeof (wbuf), "%g", w * ligma_unit_get_factor (unit));
  g_ascii_formatd (hbuf, sizeof (hbuf), "%g", h * ligma_unit_get_factor (unit));

  g_string_append_printf (str,
                          "width=\"%s%s\" height=\"%s%s\"",
                          wbuf, abbrev, hbuf, abbrev);
}

static void
ligma_vectors_export_path (LigmaVectors *vectors,
                          GString     *str)
{
  const gchar *name = ligma_object_get_name (vectors);
  gchar       *data = ligma_vectors_export_path_data (vectors);
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
ligma_vectors_export_path_data (LigmaVectors *vectors)
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
      LigmaStroke *stroke = strokes->data;
      GArray     *control_points;
      LigmaAnchor *anchor;
      gint        i;

      if (closed)
        g_string_append_printf (str, NEWLINE);

      control_points = ligma_stroke_control_points_get (stroke, &closed);

      if (LIGMA_IS_BEZIER_STROKE (stroke))
        {
          if (control_points->len >= 3)
            {
              anchor = &g_array_index (control_points, LigmaAnchor, 1);
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

              anchor = &g_array_index (control_points, LigmaAnchor,
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
              anchor = &g_array_index (control_points, LigmaAnchor, 0);
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

              anchor = &g_array_index (control_points, LigmaAnchor, i);
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
