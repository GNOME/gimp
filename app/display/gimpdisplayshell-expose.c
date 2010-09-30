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

#include <gegl.h>
#include <gtk/gtk.h>

#include "libgimpmath/gimpmath.h"

#include "display-types.h"

#include "vectors/gimpvectors.h"

#include "gimpcanvasitem.h"
#include "gimpdisplayshell.h"
#include "gimpdisplayshell-expose.h"
#include "gimpdisplayshell-transform.h"


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

static void
gimp_display_shell_expose_region (GimpDisplayShell *shell,
                                  gdouble           x1,
                                  gdouble           y1,
                                  gdouble           x2,
                                  gdouble           y2,
                                  gint              border)
{
  const gint x = floor (x1);
  const gint y = floor (y1);
  const gint w = ceil (x2) - x;
  const gint h = ceil (y2) - y;

  gimp_display_shell_expose_area (shell,
                                  x - border,
                                  y - border,
                                  w + 2 * border,
                                  h + 2 * border);
}

void
gimp_display_shell_expose_item (GimpDisplayShell *shell,
                                GimpCanvasItem   *item)
{
  GdkRegion *region;

  g_return_if_fail (GIMP_IS_DISPLAY_SHELL (shell));
  g_return_if_fail (GIMP_IS_CANVAS_ITEM (item));

  region = gimp_canvas_item_get_extents (item, shell);

  if (region)
    {
      gdk_window_invalidate_region (gtk_widget_get_window (shell->canvas),
                                    region, TRUE);
      gdk_region_destroy (region);
    }
}

void
gimp_display_shell_expose_vectors (GimpDisplayShell *shell,
                                   GimpVectors      *vectors)
{
  gdouble x1, y1;
  gdouble x2, y2;

  g_return_if_fail (GIMP_IS_DISPLAY_SHELL (shell));
  g_return_if_fail (vectors != NULL);

  if (gimp_vectors_bounds (vectors, &x1, &y1, &x2, &y2))
    {
      gimp_display_shell_transform_xy_f (shell, x1, y1, &x1, &y1);
      gimp_display_shell_transform_xy_f (shell, x2, y2, &x2, &y2);

      gimp_display_shell_expose_region (shell, x1, y1, x2, y2, 2);
    }
}

void
gimp_display_shell_expose_full (GimpDisplayShell *shell)
{
  g_return_if_fail (GIMP_IS_DISPLAY_SHELL (shell));

  gtk_widget_queue_draw (shell->canvas);
}
