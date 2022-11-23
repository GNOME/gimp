/* LIGMA - The GNU Image Manipulation Program
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

#include "libligmamath/ligmamath.h"

#include "display-types.h"

#include "config/ligmadisplayconfig.h"

#include "core/ligmaimage.h"

#include "ligmacanvas.h"
#include "ligmadisplay.h"
#include "ligmadisplay-foreach.h"
#include "ligmadisplayshell.h"
#include "ligmadisplayshell-expose.h"
#include "ligmadisplayshell-render.h"
#include "ligmadisplayshell-rotate.h"
#include "ligmadisplayshell-rulers.h"
#include "ligmadisplayshell-scale.h"
#include "ligmadisplayshell-scroll.h"
#include "ligmadisplayshell-scrollbars.h"
#include "ligmadisplayshell-transform.h"


#define OVERPAN_FACTOR 0.5


/**
 * ligma_display_shell_scroll:
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
ligma_display_shell_scroll (LigmaDisplayShell *shell,
                           gint              x_offset,
                           gint              y_offset)
{
  gint old_x;
  gint old_y;

  g_return_if_fail (LIGMA_IS_DISPLAY_SHELL (shell));

  if (x_offset == 0 && y_offset == 0)
    return;

  old_x = shell->offset_x;
  old_y = shell->offset_y;

  /* freeze the active tool */
  ligma_display_shell_pause (shell);

  shell->offset_x += x_offset;
  shell->offset_y += y_offset;

  ligma_display_shell_scroll_clamp_and_update (shell);

  /*  the actual changes in offset  */
  x_offset = (shell->offset_x - old_x);
  y_offset = (shell->offset_y - old_y);

  if (x_offset || y_offset)
    {
      ligma_display_shell_scrolled (shell);

      ligma_overlay_box_scroll (LIGMA_OVERLAY_BOX (shell->canvas),
                               -x_offset, -y_offset);

      if (shell->render_cache)
        {
          cairo_surface_t       *surface;
          cairo_t               *cr;

          surface = cairo_surface_create_similar_image (
            shell->render_cache,
            CAIRO_FORMAT_ARGB32,
            shell->disp_width  * shell->render_scale,
            shell->disp_height * shell->render_scale);

          cr = cairo_create (surface);
          cairo_set_operator (cr, CAIRO_OPERATOR_SOURCE);
          cairo_set_source_surface (cr, shell->render_cache, 0, 0);
          cairo_paint (cr);
          cairo_destroy (cr);

          cr = cairo_create (shell->render_cache);
          cairo_set_operator (cr, CAIRO_OPERATOR_SOURCE);
          cairo_set_source_surface (cr, surface,
                                    -x_offset * shell->render_scale,
                                    -y_offset * shell->render_scale);
          cairo_paint (cr);
          cairo_destroy (cr);

          cairo_surface_destroy (surface);
        }

      if (shell->render_cache_valid)
        {
          cairo_rectangle_int_t rect;

          cairo_region_translate (shell->render_cache_valid,
                                  -x_offset, -y_offset);

          rect.x      = 0;
          rect.y      = 0;
          rect.width  = shell->disp_width;
          rect.height = shell->disp_height;

          cairo_region_intersect_rectangle (shell->render_cache_valid, &rect);
        }
    }

  /* re-enable the active tool */
  ligma_display_shell_resume (shell);
}

/**
 * ligma_display_shell_scroll_set_offsets:
 * @shell:
 * @offset_x:
 * @offset_y:
 *
 * This function scrolls the image in the shell's viewport. It redraws
 * the entire canvas.
 *
 * Use it for setting the scroll offset on freshly scaled images or
 * when the window is resized. For panning, use
 * ligma_display_shell_scroll().
 **/
void
ligma_display_shell_scroll_set_offset (LigmaDisplayShell *shell,
                                      gint              offset_x,
                                      gint              offset_y)
{
  g_return_if_fail (LIGMA_IS_DISPLAY_SHELL (shell));

  if (shell->offset_x == offset_x &&
      shell->offset_y == offset_y)
    return;

  ligma_display_shell_scale_save_revert_values (shell);

  /* freeze the active tool */
  ligma_display_shell_pause (shell);

  shell->offset_x = offset_x;
  shell->offset_y = offset_y;

  ligma_display_shell_scroll_clamp_and_update (shell);

  ligma_display_shell_scrolled (shell);

  ligma_display_shell_expose_full (shell);
  ligma_display_shell_render_invalidate_full (shell);

  /* re-enable the active tool */
  ligma_display_shell_resume (shell);
}

/**
 * ligma_display_shell_scroll_clamp_and_update:
 * @shell:
 *
 * Helper function for calling two functions that are commonly called
 * in pairs.
 **/
void
ligma_display_shell_scroll_clamp_and_update (LigmaDisplayShell *shell)
{
  LigmaImage *image;

  g_return_if_fail (LIGMA_IS_DISPLAY_SHELL (shell));

  image = ligma_display_get_image (shell->display);

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

          ligma_display_shell_rotate_update_transform (shell);

          ligma_display_shell_scale_get_image_bounds (shell,
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

              ligma_display_shell_rotate_update_transform (shell);
            }

          /* Set scrollbar stepper sensitiity */

          ligma_display_shell_scrollbars_update_steppers (shell,
                                                         min_offset_x,
                                                         max_offset_x,
                                                         min_offset_y,
                                                         max_offset_y);
        }
      else
        {
          /* Set scrollbar stepper sensitiity */

          ligma_display_shell_scrollbars_update_steppers (shell,
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

  ligma_display_shell_scrollbars_update (shell);
  ligma_display_shell_rulers_update (shell);
}

/**
 * ligma_display_shell_scroll_unoverscrollify:
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
ligma_display_shell_scroll_unoverscrollify (LigmaDisplayShell *shell,
                                           gint              in_offset_x,
                                           gint              in_offset_y,
                                           gint             *out_offset_x,
                                           gint             *out_offset_y)
{
  gint sw, sh;
  gint out_offset_x_dummy, out_offset_y_dummy;

  g_return_if_fail (LIGMA_IS_DISPLAY_SHELL (shell));

  if (! out_offset_x) out_offset_x = &out_offset_x_dummy;
  if (! out_offset_y) out_offset_y = &out_offset_y_dummy;

  *out_offset_x = in_offset_x;
  *out_offset_y = in_offset_y;

  if (! shell->show_all)
    {
      ligma_display_shell_scale_get_image_size (shell, &sw, &sh);

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
 * ligma_display_shell_scroll_center_image_xy:
 * @shell:
 * @image_x:
 * @image_y:
 *
 * Center the viewport around the passed image coordinate
 **/
void
ligma_display_shell_scroll_center_image_xy (LigmaDisplayShell *shell,
                                           gdouble           image_x,
                                           gdouble           image_y)
{
  gint viewport_x;
  gint viewport_y;

  ligma_display_shell_transform_xy (shell,
                                   image_x, image_y,
                                   &viewport_x, &viewport_y);

  ligma_display_shell_scroll (shell,
                             viewport_x - shell->disp_width  / 2,
                             viewport_y - shell->disp_height / 2);
}

/**
 * ligma_display_shell_scroll_center_image:
 * @shell:
 * @horizontally:
 * @vertically:
 *
 * Centers the image in the display shell on the desired axes.
 **/
void
ligma_display_shell_scroll_center_image (LigmaDisplayShell *shell,
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

  g_return_if_fail (LIGMA_IS_DISPLAY_SHELL (shell));

  if (! shell->display                          ||
      ! ligma_display_get_image (shell->display) ||
      (! vertically && ! horizontally))
    return;

  ligma_display_shell_scale_get_image_bounds (shell,
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

  ligma_display_shell_scroll (shell, offset_x, offset_y);
}

/**
 * ligma_display_shell_scroll_center_image:
 * @shell:
 * @horizontally:
 * @vertically:
 *
 * Centers the image content in the display shell on the desired axes.
 **/
void
ligma_display_shell_scroll_center_content (LigmaDisplayShell *shell,
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

  g_return_if_fail (LIGMA_IS_DISPLAY_SHELL (shell));

  if (! shell->display                          ||
      ! ligma_display_get_image (shell->display) ||
      (! vertically && ! horizontally))
    return;

  if (! ligma_display_shell_get_infinite_canvas (shell))
    {
      ligma_display_shell_scale_get_image_bounds (shell,
                                                 &content_x,
                                                 &content_y,
                                                 &content_width,
                                                 &content_height);
    }
  else
    {
      ligma_display_shell_scale_get_image_bounding_box (shell,
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

  ligma_display_shell_scroll (shell, offset_x, offset_y);
}

/**
 * ligma_display_shell_scroll_get_scaled_viewport:
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
ligma_display_shell_scroll_get_scaled_viewport (LigmaDisplayShell *shell,
                                               gint             *x,
                                               gint             *y,
                                               gint             *w,
                                               gint             *h)
{
  g_return_if_fail (LIGMA_IS_DISPLAY_SHELL (shell));

  *x = shell->offset_x;
  *y = shell->offset_y;
  *w = shell->disp_width;
  *h = shell->disp_height;
}

/**
 * ligma_display_shell_scroll_get_viewport:
 * @shell:
 * @x:
 * @y:
 * @w:
 * @h:
 *
 * Gets the viewport in image coordinates.
 **/
void
ligma_display_shell_scroll_get_viewport (LigmaDisplayShell *shell,
                                        gdouble          *x,
                                        gdouble          *y,
                                        gdouble          *w,
                                        gdouble          *h)
{
  g_return_if_fail (LIGMA_IS_DISPLAY_SHELL (shell));

  *x = shell->offset_x    / shell->scale_x;
  *y = shell->offset_y    / shell->scale_y;
  *w = shell->disp_width  / shell->scale_x;
  *h = shell->disp_height / shell->scale_y;
}
