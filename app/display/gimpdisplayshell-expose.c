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

#include "core/gimpguide.h"
#include "core/gimpsamplepoint.h"

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

void
gimp_display_shell_expose_guide (GimpDisplayShell *shell,
                                 GimpGuide        *guide)
{
  gint position;
  gint x, y;

  g_return_if_fail (GIMP_IS_DISPLAY_SHELL (shell));
  g_return_if_fail (GIMP_IS_GUIDE (guide));

  position = gimp_guide_get_position (guide);

  if (position < 0)
    return;

  gimp_display_shell_transform_xy (shell,
                                   position, position,
                                   &x, &y,
                                   FALSE);

  switch (gimp_guide_get_orientation (guide))
    {
    case GIMP_ORIENTATION_HORIZONTAL:
      gimp_display_shell_expose_area (shell, 0, y, shell->disp_width, 1);
      break;

    case GIMP_ORIENTATION_VERTICAL:
      gimp_display_shell_expose_area (shell, x, 0, 1, shell->disp_height);
      break;

    default:
      break;
    }
}

void
gimp_display_shell_expose_sample_point (GimpDisplayShell *shell,
                                        GimpSamplePoint  *sample_point)
{
  gdouble x, y;
  gint    x1, y1, x2, y2;

  g_return_if_fail (GIMP_IS_DISPLAY_SHELL (shell));
  g_return_if_fail (sample_point != NULL);

  if (sample_point->x < 0)
    return;

  gimp_display_shell_transform_xy_f (shell,
                                     sample_point->x + 0.5,
                                     sample_point->y + 0.5,
                                     &x, &y,
                                     FALSE);

  x1 = MAX (0, floor (x - GIMP_SAMPLE_POINT_DRAW_SIZE));
  y1 = MAX (0, floor (y - GIMP_SAMPLE_POINT_DRAW_SIZE));
  x2 = MIN (shell->disp_width,  ceil (x + GIMP_SAMPLE_POINT_DRAW_SIZE));
  y2 = MIN (shell->disp_height, ceil (y + GIMP_SAMPLE_POINT_DRAW_SIZE));

  /* HACK: add 3 instead of 1 so the number gets cleared too */
  gimp_display_shell_expose_area (shell, x1, y1, x2 - x1 + 3, y2 - y1 + 3);
}

void
gimp_display_shell_expose_vectors (GimpDisplayShell *shell,
                                   GimpVectors      *vectors)
{
  g_return_if_fail (GIMP_IS_DISPLAY_SHELL (shell));
  g_return_if_fail (vectors != NULL);

  // FIXME
  gimp_display_shell_expose_full (shell);
}

void
gimp_display_shell_expose_full (GimpDisplayShell *shell)
{
  g_return_if_fail (GIMP_IS_DISPLAY_SHELL (shell));

  gtk_widget_queue_draw (shell->canvas);
}
