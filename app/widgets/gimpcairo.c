/* GIMP - The GNU Image Manipulation Program
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * gimpcairo.c
 * Copyright (C) 2010  Michael Natterer <mitch@gimp.org>
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
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#include "config.h"

#include <gtk/gtk.h>

#include "libgimpcolor/gimpcolor.h"
#include "libgimpwidgets/gimpwidgets.h"

#include "widgets-types.h"

#include "gimpcairo.h"
#include "gimpcairo-wilber.h"


static cairo_user_data_key_t surface_data_key = { 0, };


cairo_pattern_t *
gimp_cairo_stipple_pattern_create (const GimpRGB *fg,
                                   const GimpRGB *bg,
                                   gint           index)
{
  cairo_surface_t *surface;
  cairo_pattern_t *pattern;
  guchar          *data;
  guchar          *d;
  guchar           fg_r, fg_g, fg_b, fg_a;
  guchar           bg_r, bg_g, bg_b, bg_a;
  gint             x, y;

  g_return_val_if_fail (fg != NULL, NULL);
  g_return_val_if_fail (bg != NULL, NULL);

  data = g_malloc (8 * 8 * 4);

  gimp_rgba_get_uchar (fg, &fg_r, &fg_g, &fg_b, &fg_a);
  gimp_rgba_get_uchar (bg, &bg_r, &bg_g, &bg_b, &bg_a);

  d = data;

  for (y = 0; y < 8; y++)
    {
      for (x = 0; x < 8; x++)
        {
          if ((x + y + index) % 8 >= 4)
            GIMP_CAIRO_ARGB32_SET_PIXEL (d, fg_r, fg_g, fg_b, fg_a);
          else
            GIMP_CAIRO_ARGB32_SET_PIXEL (d, bg_r, bg_g, bg_b, bg_a);

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

  return pattern;
}

void
gimp_cairo_add_arc (cairo_t *cr,
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
gimp_cairo_add_segments (cairo_t     *cr,
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

void
gimp_cairo_draw_toolbox_wilber (GtkWidget *widget,
                                cairo_t   *cr)
{
  GtkStyle     *style;
  GtkStateType  state;
  GtkAllocation allocation;
  gdouble       wilber_width;
  gdouble       wilber_height;
  gdouble       factor;

  g_return_if_fail (GTK_IS_WIDGET (widget));
  g_return_if_fail (cr != NULL);

  style = gtk_widget_get_style (widget);
  state = gtk_widget_get_state (widget);

  gtk_widget_get_allocation (widget, &allocation);

  gimp_cairo_wilber_get_size (cr, &wilber_width, &wilber_height);

  factor = allocation.width / wilber_width * 0.9;

  if (! gtk_widget_get_has_window (widget))
    cairo_translate (cr, allocation.x, allocation.y);

  cairo_scale (cr, factor, factor);

  gimp_cairo_wilber (cr,
                     (allocation.width  / factor - wilber_width)  / 2.0,
                     (allocation.height / factor - wilber_height) / 2.0);

  cairo_set_source_rgba (cr,
                         style->fg[state].red   / 65535.0,
                         style->fg[state].green / 65535.0,
                         style->fg[state].blue  / 65535.0,
                         0.10);
  cairo_fill (cr);
}

void
gimp_cairo_draw_drop_wilber (GtkWidget *widget,
                             cairo_t   *cr)
{
  GtkStyle     *style;
  GtkStateType  state;
  GtkAllocation allocation;
  gdouble       wilber_width;
  gdouble       wilber_height;
  gdouble       width;
  gdouble       height;
  gdouble       side;
  gdouble       factor;

  g_return_if_fail (GTK_IS_WIDGET (widget));
  g_return_if_fail (cr != NULL);

  style  = gtk_widget_get_style (widget);
  state  = gtk_widget_get_state (widget);

  gtk_widget_get_allocation (widget, &allocation);

  gimp_cairo_wilber_get_size (cr, &wilber_width, &wilber_height);

  wilber_width  /= 2;
  wilber_height /= 2;

  side = MIN (MIN (allocation.width, allocation.height),
              MAX (allocation.width, allocation.height) / 2);

  width  = MAX (wilber_width,  side);
  height = MAX (wilber_height, side);

  factor = MIN (width / wilber_width, height / wilber_height);

  if (! gtk_widget_get_has_window (widget))
    cairo_translate (cr, allocation.x, allocation.y);

  cairo_scale (cr, factor, factor);

  /*  magic factors depend on the image used, everything else is generic
   */
  gimp_cairo_wilber (cr,
                     - wilber_width * 0.6,
                     allocation.height / factor - wilber_height * 1.1);

  cairo_set_source_rgba (cr,
                         style->fg[state].red   / 65535.0,
                         style->fg[state].green / 65535.0,
                         style->fg[state].blue  / 65535.0,
                         0.15);
  cairo_fill (cr);
}
