/* compute-svg-viewbox.c
 * Copyright (C) 2016 Jehan
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

#include <librsvg/rsvg.h>
#include <stdio.h>
#include <stdlib.h>

int main (int argc, char **argv)
{
  RsvgHandle    *handle;
  RsvgRectangle  viewport = { 0.0, 0.0, 16.0, 16.0 };
  RsvgRectangle  out_ink_rect;
  RsvgRectangle  out_logical_rect;

  gchar         *endptr;
  gchar         *path;
  gchar         *id;
  gint           prev_x;
  gint           prev_y;

  if (argc != 5)
    {
      fprintf (stderr, "Usage: compute-svg-viewbox path object_id x y\n");
      return 1;
    }
  prev_x = strtol (argv[3], &endptr, 10);
  if (endptr == argv[3])
    {
      fprintf (stderr, "Error: the third parameter must be an integer\n");
      return 1;
    }
  prev_y = strtol (argv[4], &endptr, 10);
  if (endptr == argv[4])
    {
      fprintf (stderr, "Error: the fourth parameter must be an integer\n");
      return 1;
    }

  path   = argv[1];
  handle = rsvg_handle_new_from_file (path, NULL);
  if (! handle)
    {
      fprintf (stderr, "Error: wrong path \"%s\".\n", path);
      return 1;
    }

  id = g_strdup_printf ("#%s", argv[2]);
  if (! rsvg_handle_has_sub (handle, id))
    {
      fprintf (stderr, "Error: the id \"%s\" does not exist.\n", id);
      g_object_unref (handle);
      g_free (id);
      return 1;
    }

  rsvg_handle_get_geometry_for_layer (handle, id, &viewport, &out_ink_rect, &out_logical_rect, NULL);

  if (out_ink_rect.width != out_ink_rect.height)
    {
      /* Right now, we are constraining all objects into square objects. */
      fprintf (stderr, "WARNING: object \"%s\" has unexpected size %fx%f [pos: (%f, %f)].\n",
               id, out_ink_rect.width, out_ink_rect.height,
               out_ink_rect.x, out_ink_rect.y);
    }
  printf ("viewBox=\"%f %f %f %f\"",
          out_ink_rect.x + prev_x, out_ink_rect.y + prev_y,
          out_ink_rect.width, out_ink_rect.height);

  g_object_unref (handle);
  g_free (id);
  return 0;
}
