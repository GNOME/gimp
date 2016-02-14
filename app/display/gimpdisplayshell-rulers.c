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

#include "libgimpbase/gimpbase.h"
#include "libgimpwidgets/gimpwidgets.h"

#include "display-types.h"

#include "core/gimpimage.h"

#include "gimpdisplay.h"
#include "gimpdisplayshell.h"
#include "gimpdisplayshell-rulers.h"


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
      image_width  = gimp_image_get_width  (image);
      image_height = gimp_image_get_height (image);

      gimp_image_get_resolution (image, &resolution_x, &resolution_y);
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
      horizontal_upper    = gimp_pixels_to_units (FUNSCALEX (shell,
                                                             shell->disp_width),
                                                  shell->unit,
                                                  resolution_x);
      horizontal_max_size = gimp_pixels_to_units (MAX (image_width,
                                                       image_height),
                                                  shell->unit,
                                                  resolution_x);

      vertical_upper      = gimp_pixels_to_units (FUNSCALEY (shell,
                                                             shell->disp_height),
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
      gdouble offset_x;
      gdouble offset_y;

      offset_x = gimp_pixels_to_units (FUNSCALEX (shell,
                                                  (gdouble) shell->offset_x),
                                       shell->unit,
                                       resolution_x);

      offset_y = gimp_pixels_to_units (FUNSCALEX (shell,
                                                  (gdouble) shell->offset_y),
                                       shell->unit,
                                       resolution_y);

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
