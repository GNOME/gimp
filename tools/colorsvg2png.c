/* colorsvg2png.c
 * Copyright (C) 2018 Jehan
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

#include <glib/gprintf.h>
#include <librsvg/rsvg.h>

int
main (int argc, char **argv)
{
  GError            *error = NULL;
  RsvgHandle        *handle;
  cairo_surface_t   *surface;
  cairo_t           *cr;
  RsvgDimensionData  original_dim;

  gchar             *input;
  gchar             *output;
  gint               dim;

  if (argc != 4)
    {
      g_fprintf (stderr, "Usage: colorsvg2png svg-image png-output size\n");
      return 1;
    }

  input  = argv[1];
  output = argv[2];
  dim    = (gint) g_ascii_strtoull (argv[3], NULL, 10);
  if (dim < 1)
    {
      g_fprintf (stderr, "Usage: invalid dimension %d\n", dim);
      return 1;
    }

  handle = rsvg_handle_new_from_file (input, &error);
  if (! handle)
    {
      g_fprintf (stderr,
                 "Error: failed to load '%s' as SVG: %s\n",
                 input, error->message);
      g_error_free (error);
      return 1;
    }

  surface = cairo_image_surface_create (CAIRO_FORMAT_ARGB32, dim, dim);
  cr = cairo_create (surface);
  rsvg_handle_get_dimensions (handle, &original_dim);
  cairo_surface_destroy (surface);
  cairo_scale (cr,
               (gdouble) dim / (gdouble) original_dim.width,
               (gdouble) dim / (gdouble) original_dim.height);

  if (! rsvg_handle_render_cairo (handle, cr))
    {
      g_fprintf (stderr,
                 "Error: failed to render '%s'\n",
                 input);
      return 1;
    }

  if (cairo_surface_write_to_png (surface, output) != CAIRO_STATUS_SUCCESS)
    {
      g_fprintf (stderr,
                 "Error: failed to write '%s'\n",
                 output);
      return 1;
    }
  cairo_destroy (cr);

  return 0;
}
