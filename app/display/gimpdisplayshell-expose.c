/* GIMP - The GNU Image Manipulation Program
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
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#include "config.h"

#include <gtk/gtk.h>

#include "display-types.h"

#include "gimpdisplayshell.h"
#include "gimpdisplayshell-expose.h"


void
gimp_display_shell_expose_area (GimpDisplayShell *shell,
                                gint              x,
                                gint              y,
                                gint              w,
                                gint              h)
{
  g_return_if_fail (GIMP_IS_DISPLAY_SHELL (shell));

  gtk_widget_queue_draw_area (shell->canvas, x, y, w, h);
}

void
gimp_display_shell_expose_region (GimpDisplayShell *shell,
                                  cairo_region_t   *region)
{
  GdkRegion *gdk_region;
  gint       n_rectangles;
  gint       i;

  g_return_if_fail (GIMP_IS_DISPLAY_SHELL (shell));
  g_return_if_fail (region != NULL);

  if (! gtk_widget_get_realized (shell->canvas))
    return;

  gdk_region = gdk_region_new ();
  n_rectangles = cairo_region_num_rectangles (region);

  for (i = 0; i < n_rectangles; i++)
    {
      cairo_rectangle_int_t rectangle;

      cairo_region_get_rectangle (region, i, &rectangle);

      gdk_region_union_with_rect (gdk_region, (GdkRectangle *) &rectangle);
    }

  gdk_window_invalidate_region (gtk_widget_get_window (shell->canvas),
                                gdk_region, TRUE);
  gdk_region_destroy (gdk_region);
}

void
gimp_display_shell_expose_full (GimpDisplayShell *shell)
{
  g_return_if_fail (GIMP_IS_DISPLAY_SHELL (shell));

  gtk_widget_queue_draw (shell->canvas);
}
