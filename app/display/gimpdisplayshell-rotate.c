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

#include <math.h>

#include <gegl.h>
#include <gtk/gtk.h>

#include "libgimpmath/gimpmath.h"

#include "display-types.h"

#include "gimpdisplay.h"
#include "gimpdisplayshell.h"
#include "gimpdisplayshell-expose.h"
#include "gimpdisplayshell-rotate.h"
#include "gimpdisplayshell-scale.h"
#include "gimpdisplayshell-scroll.h"
#include "gimpdisplayshell-transform.h"


#define ANGLE_EPSILON 1e-3


/*  local function prototypes  */

static void   gimp_display_shell_save_viewport_center    (GimpDisplayShell *shell,
                                                          gdouble          *x,
                                                          gdouble          *y);
static void   gimp_display_shell_restore_viewport_center (GimpDisplayShell *shell,
                                                          gdouble           x,
                                                          gdouble           y);


/*  public functions  */

void
gimp_display_shell_flip (GimpDisplayShell *shell,
                         gboolean          flip_horizontally,
                         gboolean          flip_vertically)
{
  g_return_if_fail (GIMP_IS_DISPLAY_SHELL (shell));

  flip_horizontally = flip_horizontally ? TRUE : FALSE;
  flip_vertically   = flip_vertically   ? TRUE : FALSE;

  if (flip_horizontally != shell->flip_horizontally ||
      flip_vertically   != shell->flip_vertically)
    {
      gdouble cx, cy;

      /* Maintain the current center of the viewport. */
      gimp_display_shell_save_viewport_center (shell, &cx, &cy);

      /* Adjust the rotation angle so that the image gets reflected across the
       * horizontal, and/or vertical, axes in screen space, regardless of the
       * current rotation.
       */
      if (flip_horizontally == shell->flip_horizontally ||
          flip_vertically   == shell->flip_vertically)
        {
          if (shell->rotate_angle != 0.0)
            shell->rotate_angle = 360.0 - shell->rotate_angle;
        }

      shell->flip_horizontally = flip_horizontally;
      shell->flip_vertically   = flip_vertically;

      gimp_display_shell_rotated (shell);

      gimp_display_shell_restore_viewport_center (shell, cx, cy);

      gimp_display_shell_expose_full (shell);
    }
}

void
gimp_display_shell_rotate (GimpDisplayShell *shell,
                           gdouble           delta)
{
  g_return_if_fail (GIMP_IS_DISPLAY_SHELL (shell));

  gimp_display_shell_rotate_to (shell, shell->rotate_angle + delta);
}

void
gimp_display_shell_rotate_to (GimpDisplayShell *shell,
                              gdouble           value)
{
  gdouble cx, cy;

  g_return_if_fail (GIMP_IS_DISPLAY_SHELL (shell));

  /* Maintain the current center of the viewport. */
  gimp_display_shell_save_viewport_center (shell, &cx, &cy);

  /* Make sure the angle is within the range [0, 360). */
  value = fmod (value, 360.0);
  if (value < 0.0)
    value += 360.0;

  shell->rotate_angle = value;

  gimp_display_shell_scroll_clamp_and_update (shell);

  gimp_display_shell_rotated (shell);

  gimp_display_shell_restore_viewport_center (shell, cx, cy);

  gimp_display_shell_expose_full (shell);
}

void
gimp_display_shell_rotate_drag (GimpDisplayShell *shell,
                                gdouble           last_x,
                                gdouble           last_y,
                                gdouble           cur_x,
                                gdouble           cur_y,
                                gboolean          constrain)
{
  gdouble pivot_x, pivot_y;
  gdouble src_x,   src_y,   src_angle;
  gdouble dest_x,  dest_y,  dest_angle;
  gdouble                   delta_angle;

  g_return_if_fail (GIMP_IS_DISPLAY_SHELL (shell));

  /* Rotate the image around the center of the viewport. */
  pivot_x     = shell->disp_width  / 2.0;
  pivot_y     = shell->disp_height / 2.0;

  src_x       = last_x - pivot_x;
  src_y       = last_y - pivot_y;
  src_angle   = atan2 (src_y, src_x);

  dest_x      = cur_x - pivot_x;
  dest_y      = cur_y - pivot_y;
  dest_angle  = atan2 (dest_y, dest_x);

  delta_angle = dest_angle - src_angle;

  shell->rotate_drag_angle += 180.0 * delta_angle / G_PI;

  gimp_display_shell_rotate_to (shell,
                                constrain ?
                                RINT (shell->rotate_drag_angle / 15.0) * 15.0 :
                                shell->rotate_drag_angle);
}

void
gimp_display_shell_rotate_update_transform (GimpDisplayShell *shell)
{
  g_return_if_fail (GIMP_IS_DISPLAY_SHELL (shell));

  g_clear_pointer (&shell->rotate_transform,   g_free);
  g_clear_pointer (&shell->rotate_untransform, g_free);

  if (fabs (shell->rotate_angle)         < ANGLE_EPSILON ||
      fabs (360.0 - shell->rotate_angle) < ANGLE_EPSILON)
    shell->rotate_angle = 0.0;

  if ((shell->rotate_angle != 0.0 ||
       shell->flip_horizontally   ||
       shell->flip_vertically) &&
      gimp_display_get_image (shell->display))
    {
      gint    image_width, image_height;
      gdouble cx, cy;

      shell->rotate_transform   = g_new (cairo_matrix_t, 1);
      shell->rotate_untransform = g_new (cairo_matrix_t, 1);

      gimp_display_shell_scale_get_image_size (shell,
                                               &image_width, &image_height);

      cx = -shell->offset_x + image_width  / 2;
      cy = -shell->offset_y + image_height / 2;

      cairo_matrix_init_translate (shell->rotate_transform, cx, cy);

      if (shell->rotate_angle != 0.0)
        cairo_matrix_rotate (shell->rotate_transform,
                             shell->rotate_angle / 180.0 * G_PI);

      if (shell->flip_horizontally)
        cairo_matrix_scale (shell->rotate_transform, -1.0, 1.0);

      if (shell->flip_vertically)
        cairo_matrix_scale (shell->rotate_transform, 1.0, -1.0);

      cairo_matrix_translate (shell->rotate_transform, -cx, -cy);

      *shell->rotate_untransform = *shell->rotate_transform;
      cairo_matrix_invert (shell->rotate_untransform);
    }
}


/*  private functions  */

static void
gimp_display_shell_save_viewport_center (GimpDisplayShell *shell,
                                         gdouble          *x,
                                         gdouble          *y)
{
  gimp_display_shell_unrotate_xy_f (shell,
                                    shell->disp_width  / 2,
                                    shell->disp_height / 2,
                                    x, y);
}

static void
gimp_display_shell_restore_viewport_center (GimpDisplayShell *shell,
                                            gdouble           x,
                                            gdouble           y)
{
  gimp_display_shell_rotate_xy_f (shell, x, y, &x, &y);

  x += shell->offset_x - shell->disp_width  / 2;
  y += shell->offset_y - shell->disp_height / 2;

  gimp_display_shell_scroll_set_offset (shell, RINT (x), RINT (y));
}
