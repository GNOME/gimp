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
#if LIBRSVG_CHECK_VERSION(2, 46, 0)
  RsvgRectangle      target_rect;
#endif

  gchar             *input;
  gchar             *output;
  gint               dim;
  gint               retval = 0;

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

#if LIBRSVG_CHECK_VERSION(2, 46, 0)
  target_rect.x = target_rect.y = 0;
  target_rect.width = target_rect.height = dim;

  if (! rsvg_handle_render_document (handle, cr, &target_rect, NULL))
#else
  if (! rsvg_handle_render_cairo (handle, cr))
#endif
    {
      g_fprintf (stderr,
                 "Error: failed to render '%s'\n",
                 input);
      retval = 1;
    }

  if (retval == 0 &&
      cairo_surface_write_to_png (surface, output) != CAIRO_STATUS_SUCCESS)
    {
      g_fprintf (stderr,
                 "Error: failed to write '%s'\n",
                 output);
      retval = 1;
    }

  cairo_surface_destroy (surface);
  cairo_destroy (cr);
  g_object_unref (handle);

  return retval;
}
