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
#include <stdlib.h>
#include <string.h>
#include <errno.h>

#include <glib-object.h>

#include "libgimpbase/gimpbase.h"

#include "core-types.h"

#include "gimpgradient.h"
#include "gimpgradient-load.h"

#include "gimp-intl.h"


GimpData *
gimp_gradient_load (const gchar  *filename,
                    gboolean      stingy_memory_use,
                    GError      **error)
{
  GimpGradient        *gradient;
  GimpGradientSegment *seg;
  GimpGradientSegment *prev;
  gint                 num_segments;
  gint                 i;
  gint                 type, color;
  FILE                *file;
  gchar                line[1024];

  g_return_val_if_fail (filename != NULL, NULL);
  g_return_val_if_fail (g_path_is_absolute (filename), NULL);
  g_return_val_if_fail (error == NULL || *error == NULL, NULL);

  file = fopen (filename, "rb");
  if (!file)
    {
      g_set_error (error, GIMP_DATA_ERROR, GIMP_DATA_ERROR_OPEN,
                   _("Could not open '%s' for reading: %s"),
                   gimp_filename_to_utf8 (filename), g_strerror (errno));
      return NULL;
    }

  fgets (line, 1024, file);
  if (strcmp (line, "GIMP Gradient\n") != 0)
    {
      g_set_error (error, GIMP_DATA_ERROR, GIMP_DATA_ERROR_READ,
                   _("Fatal parse error in gradient file '%s': "
                     "Not a GIMP gradient file."),
                   gimp_filename_to_utf8 (filename));
      fclose (file);
      return NULL;
    }

  gradient = g_object_new (GIMP_TYPE_GRADIENT, NULL);

  fgets (line, 1024, file);
  if (! strncmp (line, "Name: ", strlen ("Name: ")))
    {
      gchar *utf8;

      utf8 = gimp_any_to_utf8 (&line[strlen ("Name: ")], -1,
                               _("Invalid UTF-8 string in gradient file '%s'."),
                               gimp_filename_to_utf8 (filename));
      g_strstrip (utf8);

      gimp_object_set_name (GIMP_OBJECT (gradient), utf8);
      g_free (utf8);

      fgets (line, 1024, file);
    }
  else /* old gradient format */
    {
      gchar *basename;
      gchar *utf8;

      basename = g_path_get_basename (filename);

      utf8 = g_filename_to_utf8 (basename, -1, NULL, NULL, NULL);
      g_free (basename);

      gimp_object_set_name (GIMP_OBJECT (gradient), utf8);
      g_free (utf8);
    }

  num_segments = atoi (line);

  if (num_segments < 1)
    {
      g_set_error (error, GIMP_DATA_ERROR, GIMP_DATA_ERROR_READ,
                   _("Fatal parse error in gradient file '%s': "
                     "File is corrupt."),
                   gimp_filename_to_utf8 (filename));
      g_object_unref (gradient);
      fclose (file);
      return NULL;
    }

  prev = NULL;

  for (i = 0; i < num_segments; i++)
    {
      gchar *end;

      seg = gimp_gradient_segment_new ();

      seg->prev = prev;

      if (prev)
	prev->next = seg;
      else
	gradient->segments = seg;

      fgets (line, 1024, file);

      seg->left = g_ascii_strtod (line, &end);
      if (end && errno != ERANGE)
        seg->middle = g_ascii_strtod (end, &end);
      if (end && errno != ERANGE)
        seg->right = g_ascii_strtod (end, &end);

      if (end && errno != ERANGE)
        seg->left_color.r = g_ascii_strtod (end, &end);
      if (end && errno != ERANGE)
        seg->left_color.g = g_ascii_strtod (end, &end);
      if (end && errno != ERANGE)
        seg->left_color.b = g_ascii_strtod (end, &end);
      if (end && errno != ERANGE)
        seg->left_color.a = g_ascii_strtod (end, &end);

      if (end && errno != ERANGE)
        seg->right_color.r = g_ascii_strtod (end, &end);
      if (end && errno != ERANGE)
        seg->right_color.g = g_ascii_strtod (end, &end);
      if (end && errno != ERANGE)
        seg->right_color.b = g_ascii_strtod (end, &end);
      if (end && errno != ERANGE)
        seg->right_color.a = g_ascii_strtod (end, &end);

      if (errno != ERANGE &&
          sscanf (end, "%d %d", &type, &color) == 2)
        {
	  seg->type  = (GimpGradientSegmentType) type;
	  seg->color = (GimpGradientSegmentColor) color;
        }
      else
        {
	  g_message (_("Corrupt segment %d in gradient file '%s'."),
		     i, gimp_filename_to_utf8 (filename));
	}

      prev = seg;
    }

  fclose (file);

  return GIMP_DATA (gradient);
}
