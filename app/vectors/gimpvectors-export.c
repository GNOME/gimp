/* The GIMP -- an image manipulation program
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

#include <stdio.h>
#include <errno.h>

#include <glib-object.h>

#include "vectors-types.h"

#include "core/gimpimage.h"
#include "core/gimpitem.h"
#include "core/gimplist.h"

#include "gimpanchor.h"
#include "gimpstroke.h"
#include "gimpbezierstroke.h"
#include "gimpvectors.h"
#include "gimpvectors-export.h"

#include "gimp-intl.h"


static void    gimp_vectors_export_path (const GimpVectors *vectors,
                                         FILE              *file);
static gchar * gimp_vectors_path_data   (const GimpVectors *vectors);


/**
 * gimp_vectors_export:
 * @image: the #GimpImage from which to export vectors
 * @vectors: a #GimpVectors object or %NULL to export all vectors in @image
 * @filename: the name of the file to write
 * @error: return location for errors
 *
 * Exports one or more vectors to a SVG file.
 *
 * Return value: %TRUE on success, %FALSE if an error occured
 **/
gboolean
gimp_vectors_export (const GimpImage    *image,
                     const GimpVectors  *vectors,
                     const gchar        *filename,
                     GError            **error)
{
  FILE  *file;

  g_return_val_if_fail (GIMP_IS_IMAGE (image), FALSE);
  g_return_val_if_fail (vectors == NULL || GIMP_IS_VECTORS (vectors), FALSE);
  g_return_val_if_fail (filename != NULL, FALSE);
  g_return_val_if_fail (error == NULL || *error == NULL, FALSE);

  file = fopen (filename, "w");
  if (!file)
    {
      g_set_error (error, 0, 0,
                   _("Failed to open file: '%s': %s"),
                   filename, g_strerror (errno));
      return FALSE;
    }

  fprintf (file,
           "<?xml version=\"1.0\" standalone=\"no\"?>\n"
           "<!DOCTYPE svg PUBLIC \"-//W3C//DTD SVG 20010904//EN\"\n"
           "\"http://www.w3.org/TR/2001/REC-SVG-20010904/DTD/svg10.dtd\">\n");
  fprintf (file,
           "<svg viewBox=\"0 0 %d %d\"\n"
           "     xmlns=\"http://www.w3.org/2000/svg\">\n",
           image->width, image->height);

  if (vectors)
    {
      gimp_vectors_export_path (vectors, file);
    }
  else
    {
      GList *list;

      for (list = GIMP_LIST (image->vectors)->list; list; list = list->next)
        gimp_vectors_export_path (GIMP_VECTORS (list->data), file);
    }

  fprintf (file,
           "</svg>\n");

  if (fclose (file))
    {
      g_set_error (error, 0, 0,
                   _("Error while writing '%s': %s"),
                   filename, g_strerror (errno));
      return FALSE;
    }

  return TRUE;
}

static void
gimp_vectors_export_path (const GimpVectors *vectors,
                          FILE              *file)
{
  gchar *data = gimp_vectors_path_data (vectors);

  fprintf (file,
           "  <path fill=\"none\" stroke=\"black\" stroke-width=\"1\"\n"
           "        d=\"%s\"/>\n",
           data);
  g_free (data);
}

static gchar *
gimp_vectors_path_data (const GimpVectors *vectors)
{
  GString  *str;
  GList    *strokes;
  gboolean  first_stroke = TRUE;
  gchar     x_string[G_ASCII_DTOSTR_BUF_SIZE];
  gchar     y_string[G_ASCII_DTOSTR_BUF_SIZE];

  str = g_string_new (NULL);

  for (strokes = vectors->strokes; strokes; strokes = strokes->next)
    {
      GimpStroke *stroke = strokes->data;
      GArray     *control_points;
      GimpAnchor *anchor;
      gboolean    closed;
      gint        i;

      control_points = gimp_stroke_control_points_get (stroke, &closed);

      if (! first_stroke)
        g_string_append_printf (str, "\n           ");

      first_stroke = FALSE;

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
              g_string_append_printf (str, "\n           C");
            }

          for (i=2; i < (control_points->len + (closed ? 2 : - 1)); i++)
            {
              anchor = &g_array_index (control_points, GimpAnchor,
                                       i % control_points->len);
              g_ascii_formatd (x_string, G_ASCII_DTOSTR_BUF_SIZE,
                               "%.2f", anchor->position.x);
              g_ascii_formatd (y_string, G_ASCII_DTOSTR_BUF_SIZE,
                               "%.2f", anchor->position.y);
              g_string_append_printf (str, " %s,%s", x_string, y_string);

              if (i % 3 == 1)
                  g_string_append_printf (str, "\n          ");
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
              g_string_append_printf (str, "\n           L");
            }

          for (i=1; i < control_points->len; i++)
            {
              anchor = &g_array_index (control_points, GimpAnchor, i);
              g_ascii_formatd (x_string, G_ASCII_DTOSTR_BUF_SIZE,
                               "%.2f", anchor->position.x);
              g_ascii_formatd (y_string, G_ASCII_DTOSTR_BUF_SIZE,
                               "%.2f", anchor->position.y);
              g_string_append_printf (str, " %s,%s", x_string, y_string);

              if (i % 3 == 1)
                  g_string_append_printf (str, "\n          ");
            }

          if (closed && control_points->len > 1)
              g_string_append_printf (str, " Z");
        }

      g_array_free (control_points, TRUE);
    }

  return g_string_free (str, FALSE);
}
