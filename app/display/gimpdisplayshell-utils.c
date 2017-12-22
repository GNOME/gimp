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

#include "core/gimp-utils.h"

#include "gimpdisplayshell.h"
#include "gimpdisplayshell-utils.h"


gdouble
gimp_display_shell_get_constrained_line_offset_angle (GimpDisplayShell *shell)
{
  g_return_val_if_fail (GIMP_IS_DISPLAY_SHELL (shell), 0.0);

  if (shell->flip_horizontally ^ shell->flip_vertically)
    return +shell->rotate_angle;
  else
    return -shell->rotate_angle;
}

void
gimp_display_shell_constrain_line (GimpDisplayShell *shell,
                                   gdouble           start_x,
                                   gdouble           start_y,
                                   gdouble          *end_x,
                                   gdouble          *end_y,
                                   gint              n_snap_lines)
{
  g_return_if_fail (GIMP_IS_DISPLAY_SHELL (shell));
  g_return_if_fail (end_x != NULL);
  g_return_if_fail (end_y != NULL);

  gimp_constrain_line (start_x, start_y,
                       end_x,   end_y,
                       n_snap_lines,
                       gimp_display_shell_get_constrained_line_offset_angle (shell));
}
