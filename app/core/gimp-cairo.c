/* GIMP - The GNU Image Manipulation Program
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * gimp-cairo.c
 * Copyright (C) 2010-2012  Michael Natterer <mitch@gimp.org>
 *
 * Some code here is based on code from librsvg that was originally
 * written by Raph Levien <raph@artofcode.com> for Gill.
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
#include <gegl.h>
#include <gdk-pixbuf/gdk-pixbuf.h>

#include "libgimpmath/gimpmath.h"
#include "libgimpcolor/gimpcolor.h"

#include "core-types.h"

#include "gimp-cairo.h"


#define REV (2.0 * G_PI)


static cairo_user_data_key_t surface_data_key = { 0, };


cairo_pattern_t *
gimp_cairo_pattern_create_stipple (GeglColor  *fg,
                                   GeglColor  *bg,
                                   gint        index,
                                   gdouble     offset_x,
                                   gdouble     offset_y,
                                   const Babl *render_space)
{
  cairo_surface_t *surface;
  cairo_pattern_t *pattern;
  guchar          *data;
  guchar          *d;
  guchar           fg_rgba[4];
  guchar           bg_rgba[4];
  gint             x, y;

  g_return_val_if_fail (GEGL_IS_COLOR (fg), NULL);
  g_return_val_if_fail (GEGL_IS_COLOR (bg), NULL);

  data = g_malloc (8 * 8 * 4);

  gegl_color_get_pixel (fg, babl_format_with_space ("R'G'B'A u8", render_space), fg_rgba);
  gegl_color_get_pixel (bg, babl_format_with_space ("R'G'B'A u8", render_space), bg_rgba);

  d = data;

  for (y = 0; y < 8; y++)
    {
      for (x = 0; x < 8; x++)
        {
          if ((x + y + index) % 8 >= 4)
            GIMP_CAIRO_ARGB32_SET_PIXEL (d, fg_rgba[0], fg_rgba[1], fg_rgba[2], fg_rgba[3]);
          else
            GIMP_CAIRO_ARGB32_SET_PIXEL (d, bg_rgba[0], bg_rgba[1], bg_rgba[2], bg_rgba[3]);

          d += 4;
        }
    }

  surface = cairo_image_surface_create_for_data (data,
                                                 CAIRO_FORMAT_ARGB32,
                                                 8, 8, 8 * 4);
  cairo_surface_set_user_data (surface, &surface_data_key,
                               data, (cairo_destroy_func_t) g_free);

  pattern = cairo_pattern_create_for_surface (surface);
  cairo_pattern_set_extend (pattern, CAIRO_EXTEND_REPEAT);

  cairo_surface_destroy (surface);

  if (offset_x != 0.0 || offset_y != 0.0)
    {
      cairo_matrix_t matrix;

      cairo_matrix_init_translate (&matrix,
                                   fmod (offset_x, 8),
                                   fmod (offset_y, 8));
      cairo_pattern_set_matrix (pattern, &matrix);
    }

  return pattern;
}

void
gimp_cairo_arc (cairo_t *cr,
                gdouble  center_x,
                gdouble  center_y,
                gdouble  radius,
                gdouble  start_angle,
                gdouble  slice_angle)
{
  g_return_if_fail (cr != NULL);

  if (slice_angle >= 0)
    {
      cairo_arc_negative (cr, center_x, center_y, radius,
                          - start_angle,
                          - start_angle - slice_angle);
    }
  else
    {
      cairo_arc (cr, center_x, center_y, radius,
                 - start_angle,
                 - start_angle - slice_angle);
    }
}

void
gimp_cairo_rounded_rectangle (cairo_t *cr,
                              gdouble  x,
                              gdouble  y,
                              gdouble  width,
                              gdouble  height,
                              gdouble  corner_radius)
{
  g_return_if_fail (cr != NULL);

  if (width < 0.0)
    {
      x     += width;
      width  = -width;
    }

  if (height < 0.0)
    {
      y      += height;
      height  = -height;
    }

  corner_radius = CLAMP (corner_radius, 0.0, MIN (width, height) / 2.0);

  if (corner_radius == 0.0)
    {
      cairo_rectangle (cr, x, y, width, height);

      return;
    }

  cairo_new_sub_path (cr);

  cairo_arc     (cr,
                 x + corner_radius, y + corner_radius,
                 corner_radius,
                 0.50 * REV, 0.75 * REV);
  cairo_line_to (cr,
                 x + width - corner_radius, y);

  cairo_arc     (cr,
                 x + width - corner_radius, y + corner_radius,
                 corner_radius,
                 0.75 * REV, 1.00 * REV);
  cairo_line_to (cr,
                 x + width, y + height - corner_radius);

  cairo_arc     (cr,
                 x + width - corner_radius, y + height - corner_radius,
                 corner_radius,
                 0.00 * REV, 0.25 * REV);
  cairo_line_to (cr,
                 x + corner_radius, y + height);

  cairo_arc     (cr,
                 x + corner_radius, y + height - corner_radius,
                 corner_radius,
                 0.25 * REV, 0.50 * REV);
  cairo_line_to (cr,
                 x, y + corner_radius);

  cairo_close_path (cr);
}

void
gimp_cairo_segments (cairo_t     *cr,
                     GimpSegment *segs,
                     gint         n_segs)
{
  gint i;

  g_return_if_fail (cr != NULL);
  g_return_if_fail (segs != NULL && n_segs > 0);

  for (i = 0; i < n_segs; i++)
    {
      if (segs[i].x1 == segs[i].x2)
        {
          cairo_move_to (cr, segs[i].x1 + 0.5, segs[i].y1 + 0.5);
          cairo_line_to (cr, segs[i].x2 + 0.5, segs[i].y2 - 0.5);
        }
      else
        {
          cairo_move_to (cr, segs[i].x1 + 0.5, segs[i].y1 + 0.5);
          cairo_line_to (cr, segs[i].x2 - 0.5, segs[i].y2 + 0.5);
        }
    }
}
