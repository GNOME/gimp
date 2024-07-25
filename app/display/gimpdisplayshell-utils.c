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
 * along with this program.  If not, see <https://www.gnu.org/licenses/>.
 */

#include "config.h"

#include <gegl.h>
#include <gtk/gtk.h>

#include "libgimpbase/gimpbase.h"
#include "libgimpmath/gimpmath.h"

#include "display-types.h"

#include "core/gimp-utils.h"
#include "core/gimpimage.h"
#include "core/gimpunit.h"

#include "gimpdisplay.h"
#include "gimpdisplayshell.h"
#include "gimpdisplayshell-utils.h"

#include "gimp-intl.h"

void
gimp_display_shell_get_constrained_line_params (GimpDisplayShell *shell,
                                                gdouble          *offset_angle,
                                                gdouble          *xres,
                                                gdouble          *yres)
{
  g_return_if_fail (GIMP_IS_DISPLAY_SHELL (shell));
  g_return_if_fail (offset_angle != NULL);
  g_return_if_fail (xres != NULL);
  g_return_if_fail (yres != NULL);

  if (shell->flip_horizontally ^ shell->flip_vertically)
    *offset_angle = +shell->rotate_angle;
  else
    *offset_angle = -shell->rotate_angle;

  *xres = 1.0;
  *yres = 1.0;

  if (! shell->dot_for_dot)
    {
      GimpImage *image = gimp_display_get_image (shell->display);

      if (image)
        gimp_image_get_resolution (image, xres, yres);
    }
}

void
gimp_display_shell_constrain_line (GimpDisplayShell *shell,
                                   gdouble           start_x,
                                   gdouble           start_y,
                                   gdouble          *end_x,
                                   gdouble          *end_y,
                                   gint              n_snap_lines)
{
  gdouble offset_angle;
  gdouble xres, yres;

  g_return_if_fail (GIMP_IS_DISPLAY_SHELL (shell));
  g_return_if_fail (end_x != NULL);
  g_return_if_fail (end_y != NULL);

  gimp_display_shell_get_constrained_line_params (shell,
                                                  &offset_angle,
                                                  &xres, &yres);

  gimp_constrain_line (start_x, start_y,
                       end_x,   end_y,
                       n_snap_lines,
                       offset_angle,
                       xres, yres);
}

gdouble
gimp_display_shell_constrain_angle (GimpDisplayShell *shell,
                                    gdouble           angle,
                                    gint              n_snap_lines)
{
  gdouble x, y;

  g_return_val_if_fail (GIMP_IS_DISPLAY_SHELL (shell), 0.0);

  x = cos (angle);
  y = sin (angle);

  gimp_display_shell_constrain_line (shell,
                                     0.0, 0.0,
                                     &x,  &y,
                                     n_snap_lines);

  return atan2 (y, x);
}

/**
 * gimp_display_shell_get_line_status:
 * @status:    initial status text.
 * @separator: separator text between the line information and @status.
 * @shell:     #GimpDisplayShell this status text will be displayed for.
 * @x1:        abscissa of first point.
 * @y1:        ordinate of first point.
 * @x2:        abscissa of second point.
 * @y2:        ordinate of second point.
 *
 * Utility function to prepend the status message with a distance and
 * angle value. Obviously this is only to be used for tools when it
 * makes sense, and in particular when there is a concept of line. For
 * instance when shift-clicking a painting tool or in the blend tool,
 * etc.
 * This utility prevents code duplication but also ensures a common
 * display for every tool where such a status is needed. It will take
 * into account the shell unit settings and will use the ideal digit
 * precision according to current image resolution.
 *
 * Returns: a newly allocated string containing the enhanced status.
 **/
gchar *
gimp_display_shell_get_line_status (GimpDisplayShell *shell,
                                    const gchar      *status,
                                    const gchar      *separator,
                                    gdouble           x1,
                                    gdouble           y1,
                                    gdouble           x2,
                                    gdouble           y2)
{
  GimpImage *image;
  gchar     *enhanced_status;
  gdouble    xres;
  gdouble    yres;
  gdouble    dx, dy, pixel_dist;
  gdouble    angle;

  image = gimp_display_get_image (shell->display);
  if (! image)
    {
      /* This makes no sense to add line information when no image is
       * attached to the display. */
      return g_strdup (status);
    }

  if (shell->unit == gimp_unit_pixel ())
    xres = yres = 1.0;
  else
    gimp_image_get_resolution (image, &xres, &yres);

  dx = x2 - x1;
  dy = y2 - y1;
  pixel_dist = sqrt (SQR (dx) + SQR (dy));

  if (dx)
    {
      angle = gimp_rad_to_deg (atan ((dy/yres) / (dx/xres)));
      if (dx > 0)
        {
          if (dy > 0)
            angle = 360.0 - angle;
          else if (dy < 0)
            angle = -angle;
        }
      else
        {
          angle = 180.0 - angle;
        }
    }
  else if (dy)
    {
      angle = dy > 0 ? 270.0 : 90.0;
    }
  else
    {
      angle = 0.0;
    }

  if (shell->unit == gimp_unit_pixel ())
    {
      enhanced_status = g_strdup_printf ("%.1f %s, %.2f\302\260%s%s",
                                         pixel_dist, _("pixels"), angle,
                                         separator, status);
    }
  else
    {
      gdouble inch_dist;
      gdouble unit_dist;
      gint    digits = 0;

      /* The distance in unit. */
      inch_dist = sqrt (SQR (dx / xres) + SQR (dy / yres));
      unit_dist = gimp_unit_get_factor (shell->unit) * inch_dist;

      /* The ideal digit precision for unit in current resolution. */
      if (inch_dist)
        digits = gimp_unit_get_scaled_digits (shell->unit,
                                              pixel_dist / inch_dist);

      enhanced_status = g_strdup_printf ("%.*f %s, %.2f\302\260%s%s",
                                         digits, unit_dist,
                                         gimp_unit_get_symbol (shell->unit),
                                         angle, separator, status);

    }

  return enhanced_status;
}
