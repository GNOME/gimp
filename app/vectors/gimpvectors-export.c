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

#include "gimpanchor.h"
#include "gimpstroke.h"
#include "gimpvectors.h"
#include "gimpvectors-export.h"

#include "gimp-intl.h"


static gchar * gimp_vectors_path_data (const GimpVectors *vectors);


gboolean
gimp_vectors_export (const GimpVectors  *vectors,
                     const gchar        *filename,
                     GError            **error)
{
  GimpImage *image;
  FILE      *file;
  gchar     *data;

  g_return_val_if_fail (GIMP_IS_VECTORS (vectors), FALSE);
  g_return_val_if_fail (filename != NULL, FALSE);
  g_return_val_if_fail (error == NULL || *error == NULL, FALSE);

  image = gimp_item_get_image (GIMP_ITEM (vectors));

  data = gimp_vectors_path_data (vectors);

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
  fprintf (file,
           "  <path d=\"%s\"\n"
           "        fill=\"none\" stroke=\"black\", stroke-width=\"1\" />\n",
           data);
  fprintf (file,
           "</svg>\n");

  if (fclose (file))
    {
      g_set_error (error, 0, 0,
                   _("Error while writing '%s': %s"),
                   filename, g_strerror (errno));
      return FALSE;
    }

  g_free (data);

  return TRUE;
}

static gchar *
gimp_vectors_path_data (const GimpVectors *vectors)
{
  GString *str;
  GList   *strokes;

  str = g_string_new (NULL);

  for (strokes = vectors->strokes; strokes; strokes = strokes->next)
    {
      GimpStroke *stroke = strokes->data;
      GList      *anchors;
      GimpAnchor *anchor;

      anchors = stroke->anchors;
      if (!anchors)
        continue;

      anchor = anchors->data;
      g_string_append_printf (str, "M%d,%d",
                              (gint) anchor->position.x,
                              (gint) anchor->position.y);

      for (anchors = anchors->next; anchors; anchors = anchors->next)
        {
          anchor = anchors->data;
          g_string_append_printf (str, " L%d,%d",
                                  (gint) anchor->position.x,
                                  (gint) anchor->position.y);
        }

      if (stroke->closed)
        g_string_append_printf (str, " Z");
    }

  return g_string_free (str, FALSE);
}
