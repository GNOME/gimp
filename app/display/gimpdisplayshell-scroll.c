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

#include <stdlib.h>
#include <math.h>

#include <gegl.h>
#include <gtk/gtk.h>

#include "libgimpmath/gimpmath.h"

#include "display-types.h"

#include "config/gimpdisplayconfig.h"

#include "core/gimpimage.h"

#include "gimpcanvas.h"
#include "gimpdisplay.h"
#include "gimpdisplay-foreach.h"
#include "gimpdisplayshell.h"
#include "gimpdisplayshell-expose.h"
#include "gimpdisplayshell-rotate.h"
#include "gimpdisplayshell-rulers.h"
#include "gimpdisplayshell-scale.h"
#include "gimpdisplayshell-scroll.h"
#include "gimpdisplayshell-scrollbars.h"
#include "gimpdisplayshell-transform.h"


#define OVERPAN_FACTOR 0.5


/**
 * gimp_display_shell_scroll:
 * @shell:
 * @x_offset:
 * @y_offset:
 *
 * This function scrolls the image in the shell's viewport. It does
 * actual scrolling of the pixels, so only the newly scrolled-in parts
 * are freshly redrawn.
 *
 * Use it for incremental actual panning.
 **/
void
gimp_display_shell_scroll (GimpDisplayShell *shell,
                           gint              x_offset,
                           gint              y_offset)
{
  gint old_x;
  gint old_y;

  g_return_if_fail (GIMP_IS_DISPLAY_SHELL (shell));

  if (x_offset == 0 && y_offset == 0)
    return;

  old_x = shell->offset_x;
  old_y = shell->offset_y;

  /* freeze the active tool */
  gimp_display_shell_pause (shell);

  shell->offset_x += x_offset;
  shell->offset_y += y_offset;

  gimp_display_shell_scroll_clamp_and_update (shell);

  /*  the actual changes in offset  */
  x_offset = (shell->offset_x - old_x);
  y_offset = (shell->offset_y - old_y);

  if (x_offset || y_offset)
    {
      gimp_display_shell_scrolled (shell);

      gimp_overlay_box_scroll (GIMP_OVERLAY_BOX (shell->canvas),
                               -x_offset, -y_offset);

    }

  /* re-enable the active tool */
  gimp_display_shell_resume (shell);
}

/**
 * gimp_display_shell_scroll_set_offsets:
 * @shell:
 * @offset_x:
 * @offset_y:
 *
 * This function scrolls the image in the shell's viewport. It redraws
 * the entire canvas.
 *
 * Use it for setting the scroll offset on freshly scaled images or
 * when the window is resized. For panning, use
 * gimp_display_shell_scroll().
 **/
void
gimp_display_shell_scroll_set_offset (GimpDisplayShell *shell,
                                      gint              offset_x,
                                      gint              offset_y)
{
  g_return_if_fail (GIMP_IS_DISPLAY_SHELL (shell));

  if (shell->offset_x == offset_x &&
      shell->offset_y == offset_y)
    return;

  gimp_display_shell_scale_save_revert_values (shell);

  /* freeze the active tool */
  gimp_display_shell_pause (shell);

  shell->offset_x = offset_x;
  shell->offset_y = offset_y;

  gimp_display_shell_scroll_clamp_and_update (shell);

  gimp_display_shell_scrolled (shell);

  gimp_display_shell_expose_full (shell);

  /* re-enable the active tool */
  gimp_display_shell_resume (shell);
}

/**
 * gimp_display_shell_scroll_clamp_and_update:
 * @shell:
 *
 * Helper function for calling two functions that are commonly called
 * in pairs.
 **/
void
gimp_display_shell_scroll_clamp_and_update (GimpDisplayShell *shell)
{
  GimpImage *image;

  g_return_if_fail (GIMP_IS_DISPLAY_SHELL (shell));

  image = gimp_display_get_image (shell->display);

  if (image)
    {
      if (! shell->show_all)
        {
          gint bounds_x;
          gint bounds_y;
          gint bounds_width;
          gint bounds_height;
          gint min_offset_x;
          gint max_offset_x;
          gint min_offset_y;
          gint max_offset_y;
          gint offset_x;
          gint offset_y;

          gimp_display_shell_rotate_update_transform (shell);

          gimp_display_shell_scale_get_image_bounds (shell,
                                                     &bounds_x,
                                                     &bounds_y,
                                                     &bounds_width,
                                                     &bounds_height);

          if (shell->disp_width < bounds_width)
            {
              min_offset_x = bounds_x -
                             shell->disp_width * OVERPAN_FACTOR;
              max_offset_x = bounds_x + bounds_width -
                             shell->disp_width * (1.0 - OVERPAN_FACTOR);
            }
          else
            {
              gint overpan_amount;

              overpan_amount = shell->disp_width -
                               bounds_width * (1.0 - OVERPAN_FACTOR);

              min_offset_x = bounds_x -
                             overpan_amount;
              max_offset_x = bounds_x + bounds_width - shell->disp_width +
                             overpan_amount;
            }

          if (shell->disp_height < bounds_height)
            {
              min_offset_y = bounds_y -
                             shell->disp_height * OVERPAN_FACTOR;
              max_offset_y = bounds_y + bounds_height -
                             shell->disp_height * (1.0 - OVERPAN_FACTOR);
            }
          else
            {
              gint overpan_amount;

              overpan_amount = shell->disp_height -
                               bounds_height * (1.0 - OVERPAN_FACTOR);

              min_offset_y = bounds_y -
                             overpan_amount;
              max_offset_y = bounds_y + bounds_height +
                             overpan_amount - shell->disp_height;
            }

          /* Clamp */

          offset_x = CLAMP (shell->offset_x, min_offset_x, max_offset_x);
          offset_y = CLAMP (shell->offset_y, min_offset_y, max_offset_y);

          if (offset_x != shell->offset_x || offset_y != shell->offset_y)
            {
              shell->offset_x = offset_x;
              shell->offset_y = offset_y;

              gimp_display_shell_rotate_update_transform (shell);
            }

          /* Set scrollbar stepper sensitiity */

          gimp_display_shell_scrollbars_update_steppers (shell,
                                                         min_offset_x,
                                                         max_offset_x,
                                                         min_offset_y,
                                                         max_offset_y);
        }
      else
        {
          /* Set scrollbar stepper sensitiity */

          gimp_display_shell_scrollbars_update_steppers (shell,
                                                         G_MININT,
                                                         G_MAXINT,
                                                         G_MININT,
                                                         G_MAXINT);
        }
    }
  else
    {
      shell->offset_x = 0;
      shell->offset_y = 0;
    }

  gimp_display_shell_scrollbars_update (shell);
  gimp_display_shell_rulers_update (shell);
}

/**
 * gimp_display_shell_scroll_unoverscrollify:
 * @shell:
 * @in_offset_x:
 * @in_offset_y:
 * @out_offset_x:
 * @out_offset_y:
 *
 * Takes a scroll offset and returns the offset that will not result
 * in a scroll beyond the image border. If the image is already
 * overscrolled, the return value is 0 for that given axis.
 **/
void
gimp_display_shell_scroll_unoverscrollify (GimpDisplayShell *shell,
                                           gint              in_offset_x,
                                           gint              in_offset_y,
                                           gint             *out_offset_x,
                                           gint             *out_offset_y)
{
  gint sw, sh;
  gint out_offset_x_dummy, out_offset_y_dummy;

  g_return_if_fail (GIMP_IS_DISPLAY_SHELL (shell));

  if (! out_offset_x) out_offset_x = &out_offset_x_dummy;
  if (! out_offset_y) out_offset_y = &out_offset_y_dummy;

  *out_offset_x = in_offset_x;
  *out_offset_y = in_offset_y;

  if (! shell->show_all)
    {
      gimp_display_shell_scale_get_image_size (shell, &sw, &sh);

      if (in_offset_x < 0)
        {
          *out_offset_x = MAX (in_offset_x,
                               MIN (0, 0 - shell->offset_x));
        }
      else if (in_offset_x > 0)
        {
          gint min_offset = sw - shell->disp_width;

          *out_offset_x = MIN (in_offset_x,
                               MAX (0, min_offset - shell->offset_x));
        }

      if (in_offset_y < 0)
        {
          *out_offset_y = MAX (in_offset_y,
                               MIN (0, 0 - shell->offset_y));
        }
      else if (in_offset_y > 0)
        {
          gint min_offset = sh - shell->disp_height;

          *out_offset_y = MIN (in_offset_y,
                               MAX (0, min_offset - shell->offset_y));
        }
    }
}

/**
 * gimp_display_shell_scroll_center_image_xy:
 * @shell:
 * @image_x:
 * @image_y:
 *
 * Center the viewport around the passed image coordinate
 **/
void
gimp_display_shell_scroll_center_image_xy (GimpDisplayShell *shell,
                                           gdouble           image_x,
                                           gdouble           image_y)
{
  gint viewport_x;
  gint viewport_y;

  gimp_display_shell_transform_xy (shell,
                                   image_x, image_y,
                                   &viewport_x, &viewport_y);

  gimp_display_shell_scroll (shell,
                             viewport_x - shell->disp_width  / 2,
                             viewport_y - shell->disp_height / 2);
}

/**
 * gimp_display_shell_scroll_center_image:
 * @shell:
 * @horizontally:
 * @vertically:
 *
 * Centers the image in the display shell on the desired axes.
 **/
void
gimp_display_shell_scroll_center_image (GimpDisplayShell *shell,
                                        gboolean          horizontally,
                                        gboolean          vertically)
{
  gint image_x;
  gint image_y;
  gint image_width;
  gint image_height;
  gint center_x;
  gint center_y;
  gint offset_x = 0;
  gint offset_y = 0;

  g_return_if_fail (GIMP_IS_DISPLAY_SHELL (shell));

  if (! shell->display                          ||
      ! gimp_display_get_image (shell->display) ||
      (! vertically && ! horizontally))
    return;

  gimp_display_shell_scale_get_image_bounds (shell,
                                             &image_x, &image_y,
                                             &image_width, &image_height);

  if (shell->disp_width > image_width)
    {
      image_x     -= (shell->disp_width - image_width) / 2;
      image_width  = shell->disp_width;
    }

  if (shell->disp_height > image_height)
    {
      image_y      -= (shell->disp_height - image_height) / 2;
      image_height  = shell->disp_height;
    }

  center_x = image_x + image_width  / 2;
  center_y = image_y + image_height / 2;

  if (horizontally)
    offset_x = center_x - shell->disp_width / 2 - shell->offset_x;

  if (vertically)
    offset_y = center_y - shell->disp_height / 2 - shell->offset_y;

  gimp_display_shell_scroll (shell, offset_x, offset_y);
}

/**
 * gimp_display_shell_scroll_center_image:
 * @shell:
 * @horizontally:
 * @vertically:
 *
 * Centers the image content in the display shell on the desired axes.
 **/
void
gimp_display_shell_scroll_center_content (GimpDisplayShell *shell,
                                          gboolean          horizontally,
                                          gboolean          vertically)
{
  gint content_x;
  gint content_y;
  gint content_width;
  gint content_height;
  gint center_x;
  gint center_y;
  gint offset_x = 0;
  gint offset_y = 0;

  g_return_if_fail (GIMP_IS_DISPLAY_SHELL (shell));

  if (! shell->display                          ||
      ! gimp_display_get_image (shell->display) ||
      (! vertically && ! horizontally))
    return;

  if (! gimp_display_shell_get_infinite_canvas (shell))
    {
      gimp_display_shell_scale_get_image_bounds (shell,
                                                 &content_x,
                                                 &content_y,
                                                 &content_width,
                                                 &content_height);
    }
  else
    {
      gimp_display_shell_scale_get_image_bounding_box (shell,
                                                       &content_x,
                                                       &content_y,
                                                       &content_width,
                                                       &content_height);
    }

  if (shell->disp_width > content_width)
    {
      content_x     -= (shell->disp_width - content_width) / 2;
      content_width  = shell->disp_width;
    }

  if (shell->disp_height > content_height)
    {
      content_y      -= (shell->disp_height - content_height) / 2;
      content_height  = shell->disp_height;
    }

  center_x = content_x + content_width  / 2;
  center_y = content_y + content_height / 2;

  if (horizontally)
    offset_x = center_x - shell->disp_width / 2 - shell->offset_x;

  if (vertically)
    offset_y = center_y - shell->disp_height / 2 - shell->offset_y;

  gimp_display_shell_scroll (shell, offset_x, offset_y);
}

/**
 * gimp_display_shell_scroll_get_scaled_viewport:
 * @shell:
 * @x:
 * @y:
 * @w:
 * @h:
 *
 * Gets the viewport in screen coordinates, with origin at (0, 0) in
 * the image.
 **/
void
gimp_display_shell_scroll_get_scaled_viewport (GimpDisplayShell *shell,
                                               gint             *x,
                                               gint             *y,
                                               gint             *w,
                                               gint             *h)
{
  g_return_if_fail (GIMP_IS_DISPLAY_SHELL (shell));

  *x = shell->offset_x;
  *y = shell->offset_y;
  *w = shell->disp_width;
  *h = shell->disp_height;
}

/**
 * gimp_display_shell_scroll_get_viewport:
 * @shell:
 * @x:
 * @y:
 * @w:
 * @h:
 *
 * Gets the viewport in image coordinates.
 **/
void
gimp_display_shell_scroll_get_viewport (GimpDisplayShell *shell,
                                        gdouble          *x,
                                        gdouble          *y,
                                        gdouble          *w,
                                        gdouble          *h)
{
  g_return_if_fail (GIMP_IS_DISPLAY_SHELL (shell));

  *x = shell->offset_x    / shell->scale_x;
  *y = shell->offset_y    / shell->scale_y;
  *w = shell->disp_width  / shell->scale_x;
  *h = shell->disp_height / shell->scale_y;
}
