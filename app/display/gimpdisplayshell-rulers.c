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

#include "libgimpbase/gimpbase.h"
#include "libgimpmath/gimpmath.h"
#include "libgimpwidgets/gimpwidgets.h"

#include "display-types.h"

#include "core/gimpimage.h"

#include "gimpdisplay.h"
#include "gimpdisplayshell.h"
#include "gimpdisplayshell-rulers.h"
#include "gimpdisplayshell-scale.h"


/**
 * gimp_display_shell_rulers_update:
 * @shell:
 *
 **/
void
gimp_display_shell_rulers_update (GimpDisplayShell *shell)
{
  GimpImage *image;
  gint       image_width;
  gint       image_height;
  gdouble    offset_x     = 0.0;
  gdouble    offset_y     = 0.0;
  gdouble    scale_x      = 1.0;
  gdouble    scale_y      = 1.0;
  gdouble    resolution_x = 1.0;
  gdouble    resolution_y = 1.0;
  gdouble    horizontal_lower;
  gdouble    horizontal_upper;
  gdouble    horizontal_max_size;
  gdouble    vertical_lower;
  gdouble    vertical_upper;
  gdouble    vertical_max_size;

  if (! shell->display)
    return;

  image = gimp_display_get_image (shell->display);

  if (image)
    {
      gint    image_x, image_y;
      gdouble res_x, res_y;

      gimp_display_shell_scale_get_image_bounds (shell,
                                                 &image_x, &image_y,
                                                 &image_width, &image_height);

      gimp_display_shell_get_rotated_scale (shell, &scale_x, &scale_y);

      image_width  /= scale_x;
      image_height /= scale_y;

      offset_x = shell->offset_x - image_x;
      offset_y = shell->offset_y - image_y;

      gimp_image_get_resolution (image, &res_x, &res_y);

      if (shell->rotate_angle == 0.0 || res_x == res_y)
        {
          resolution_x = res_x;
          resolution_y = res_y;
        }
      else
        {
          gdouble cos_a = cos (G_PI * shell->rotate_angle / 180.0);
          gdouble sin_a = sin (G_PI * shell->rotate_angle / 180.0);

          if (shell->dot_for_dot)
            {
              resolution_x = 1.0 / sqrt (SQR (cos_a / res_x) +
                                         SQR (sin_a / res_y));
              resolution_y = 1.0 / sqrt (SQR (cos_a / res_y) +
                                         SQR (sin_a / res_x));
            }
          else
            {
              resolution_x = sqrt (SQR (res_x * cos_a) + SQR (res_y * sin_a));
              resolution_y = sqrt (SQR (res_y * cos_a) + SQR (res_x * sin_a));
            }
        }
    }
  else
    {
      image_width  = shell->disp_width;
      image_height = shell->disp_height;
    }

  /* Initialize values */

  horizontal_lower = 0;
  vertical_lower   = 0;

  if (image)
    {
      horizontal_upper    = gimp_pixels_to_units (shell->disp_width / scale_x,
                                                  shell->unit,
                                                  resolution_x);
      horizontal_max_size = gimp_pixels_to_units (MAX (image_width,
                                                       image_height),
                                                  shell->unit,
                                                  resolution_x);

      vertical_upper      = gimp_pixels_to_units (shell->disp_height / scale_y,
                                                  shell->unit,
                                                  resolution_y);
      vertical_max_size   = gimp_pixels_to_units (MAX (image_width,
                                                       image_height),
                                                  shell->unit,
                                                  resolution_y);
    }
  else
    {
      horizontal_upper    = image_width;
      horizontal_max_size = MAX (image_width, image_height);

      vertical_upper      = image_height;
      vertical_max_size   = MAX (image_width, image_height);
    }


  /* Adjust due to scrolling */

  if (image)
    {
      offset_x *= horizontal_upper / shell->disp_width;
      offset_y *= vertical_upper   / shell->disp_height;

      horizontal_lower += offset_x;
      horizontal_upper += offset_x;

      vertical_lower   += offset_y;
      vertical_upper   += offset_y;
    }

  /* Finally setup the actual rulers */

  gimp_ruler_set_range (GIMP_RULER (shell->hrule),
                        horizontal_lower,
                        horizontal_upper,
                        horizontal_max_size);

  gimp_ruler_set_unit  (GIMP_RULER (shell->hrule),
                        shell->unit);

  gimp_ruler_set_range (GIMP_RULER (shell->vrule),
                        vertical_lower,
                        vertical_upper,
                        vertical_max_size);

  gimp_ruler_set_unit  (GIMP_RULER (shell->vrule),
                        shell->unit);
}
