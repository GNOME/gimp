/* GIMP - The GNU Image Manipulation Program
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.
 */

#include "config.h"

#include <errno.h>
#include <string.h>

#include <glib-object.h>
#include <glib/gstdio.h>

#include "libgimpbase/gimpbase.h"

#include "vectors-types.h"

#include "core/gimpimage.h"
#include "core/gimpitem.h"
#include "core/gimplist.h"
#include "core/gimpunit.h"

#include "gimpanchor.h"
#include "gimpstroke.h"
#include "gimpbezierstroke.h"
#include "gimpvectors.h"
#include "gimpvectors-export.h"

#include "gimp-intl.h"


static GString * gimp_vectors_export            (const GimpImage   *image,
                                                 const GimpVectors *vectors);
static void      gimp_vectors_export_image_size (const GimpImage   *image,
                                                 GString           *str);
static void      gimp_vectors_export_path       (const GimpVectors *vectors,
                                                 GString           *str);
static gchar   * gimp_vectors_export_path_data  (const GimpVectors *vectors);


/**
 * gimp_vectors_export_file:
 * @image: the #GimpImage from which to export vectors
 * @vectors: a #GimpVectors object or %NULL to export all vectors in @image
 * @filename: the name of the file to write
 * @error: return location for errors
 *
 * Exports one or more vectors to a SVG file.
 *
 * Return value: %TRUE on success,
 *               %FALSE if there was an error writing the file
 **/
gboolean
gimp_vectors_export_file (const GimpImage    *image,
                          const GimpVectors  *vectors,
                          const gchar        *filename,
                          GError            **error)
{
  FILE    *file;
  GString *str;

  g_return_val_if_fail (GIMP_IS_IMAGE (image), FALSE);
  g_return_val_if_fail (vectors == NULL || GIMP_IS_VECTORS (vectors), FALSE);
  g_return_val_if_fail (filename != NULL, FALSE);
  g_return_val_if_fail (error == NULL || *error == NULL, FALSE);

  file = g_fopen (filename, "w");
  if (!file)
    {
      g_set_error (error, 0, 0, _("Could not open '%s' for writing: %s"),
                   gimp_filename_to_utf8 (filename), g_strerror (errno));
      return FALSE;
    }

  str = gimp_vectors_export (image, vectors);

  fprintf (file, "%s", str->str);

  g_string_free (str, TRUE);

  if (fclose (file))
    {
      g_set_error (error, 0, 0, _("Error while writing '%s': %s"),
                   gimp_filename_to_utf8 (filename), g_strerror (errno));
      return FALSE;
    }

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
gimp_vectors_export_string (const GimpImage    *image,
                            const GimpVectors  *vectors)
{
  g_return_val_if_fail (GIMP_IS_IMAGE (image), NULL);
  g_return_val_if_fail (vectors == NULL || GIMP_IS_VECTORS (vectors), NULL);

  return g_string_free (gimp_vectors_export (image, vectors), FALSE);
}

static GString *
gimp_vectors_export (const GimpImage   *image,
                     const GimpVectors *vectors)
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
                          image->width, image->height);

  if (vectors)
    {
      gimp_vectors_export_path (vectors, str);
    }
  else
    {
      GList *list;

      for (list = GIMP_LIST (image->vectors)->list; list; list = list->next)
        gimp_vectors_export_path (GIMP_VECTORS (list->data), str);
    }

  g_string_append (str, "</svg>\n");

  return str;
}

static void
gimp_vectors_export_image_size (const GimpImage *image,
                                GString         *str)
{
  GimpUnit     unit;
  const gchar *abbrev;
  gchar        wbuf[G_ASCII_DTOSTR_BUF_SIZE];
  gchar        hbuf[G_ASCII_DTOSTR_BUF_SIZE];
  gdouble      w, h;

  w = (gdouble) image->width  / image->xresolution;
  h = (gdouble) image->height / image->yresolution;

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

  g_ascii_formatd (wbuf, sizeof (wbuf),
                   "%g", w * _gimp_unit_get_factor (image->gimp, unit));
  g_ascii_formatd (hbuf, sizeof (hbuf),
                   "%g", h * _gimp_unit_get_factor (image->gimp, unit));

  g_string_append_printf (str,
                          "width=\"%s%s\" height=\"%s%s\"",
                          wbuf, abbrev, hbuf, abbrev);
}

static void
gimp_vectors_export_path (const GimpVectors *vectors,
                          GString           *str)
{
  const gchar *name = gimp_object_get_name (GIMP_OBJECT (vectors));
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
gimp_vectors_export_path_data (const GimpVectors *vectors)
{
  GString  *str;
  GList    *strokes;
  gchar     x_string[G_ASCII_DTOSTR_BUF_SIZE];
  gchar     y_string[G_ASCII_DTOSTR_BUF_SIZE];
  gboolean  closed = FALSE;

  str = g_string_new (NULL);

  for (strokes = vectors->strokes; strokes; strokes = strokes->next)
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
