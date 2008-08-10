/* GIMP - The GNU Image Manipulation Program
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.
 */

#include "config.h"

#include <stdlib.h>

#include <gtk/gtk.h>

#include "libgimpmath/gimpmath.h"

#include "display-types.h"

#include "config/gimpdisplayconfig.h"

#include "base/tile-manager.h"

#include "core/gimpimage.h"
#include "core/gimpprojection.h"

#include "gimpdisplay.h"
#include "gimpdisplay-foreach.h"
#include "gimpdisplayshell.h"
#include "gimpdisplayshell-private.h"
#include "gimpdisplayshell-scale.h"
#include "gimpdisplayshell-scroll.h"


#define OVERPAN_FACTOR 0.5


/**
 * gimp_display_shell_center_around_image_coordinate:
 * @shell:
 * @image_x:
 * @image_y:
 *
 * Center the viewport around the passed image coordinate
 *
 **/
void
gimp_display_shell_center_around_image_coordinate (GimpDisplayShell       *shell,
                                                   gdouble                 image_x,
                                                   gdouble                 image_y)
{
  gint scaled_image_x;
  gint scaled_image_y;
  gint offset_to_apply_x;
  gint offset_to_apply_y;

  scaled_image_x = RINT (image_x * shell->scale_x);
  scaled_image_y = RINT (image_y * shell->scale_y);

  offset_to_apply_x = scaled_image_x - shell->disp_width  / 2 - shell->offset_x;
  offset_to_apply_y = scaled_image_y - shell->disp_height / 2 - shell->offset_y;

  gimp_display_shell_scroll_private (shell,
                                     offset_to_apply_x,
                                     offset_to_apply_y);
}

void
gimp_display_shell_scroll_private (GimpDisplayShell *shell,
                                   gint              x_offset,
                                   gint              y_offset)
{
  gint old_x;
  gint old_y;

  g_return_if_fail (GIMP_IS_DISPLAY_SHELL (shell));

  old_x = shell->offset_x;
  old_y = shell->offset_y;

  shell->offset_x += x_offset;
  shell->offset_y += y_offset;

  gimp_display_shell_scroll_clamp_offsets (shell);

  /*  the actual changes in offset  */
  x_offset = (shell->offset_x - old_x);
  y_offset = (shell->offset_y - old_y);

  if (x_offset || y_offset)
    {
      /*  reset the old values so that the tool can accurately redraw  */
      shell->offset_x = old_x;
      shell->offset_y = old_y;

      gimp_display_shell_pause (shell);

      /*  set the offsets back to the new values  */
      shell->offset_x += x_offset;
      shell->offset_y += y_offset;

      gdk_window_scroll (shell->canvas->window, -x_offset, -y_offset);

      /*  Make sure expose events are processed before scrolling again  */
      gdk_window_process_updates (shell->canvas->window, FALSE);

      /*  Update scrollbars and rulers  */
      gimp_display_shell_scale_setup (shell);

      gimp_display_shell_resume (shell);

      gimp_display_shell_scrolled (shell);
    }
}

void
gimp_display_shell_scroll_clamp_offsets (GimpDisplayShell *shell)
{
  g_return_if_fail (GIMP_IS_DISPLAY_SHELL (shell));

  if (shell->display->image)
    {
      gint sw, sh;
      gint min_offset_x;
      gint max_offset_x;
      gint min_offset_y;
      gint max_offset_y;

      sw = SCALEX (shell, gimp_image_get_width  (shell->display->image));
      sh = SCALEY (shell, gimp_image_get_height (shell->display->image));

      if (shell->disp_width < sw)
        {
          min_offset_x = 0  - shell->disp_width * OVERPAN_FACTOR;
          max_offset_x = sw - shell->disp_width * (1.0 - OVERPAN_FACTOR);
        }
      else
        {
          gint overpan_amount;

          overpan_amount = shell->disp_width - sw * (1.0 - OVERPAN_FACTOR);

          min_offset_x = 0  - overpan_amount;
          max_offset_x = sw + overpan_amount - shell->disp_width;
        }

      if (shell->disp_height < sh)
        {
          min_offset_y = 0  - shell->disp_height * OVERPAN_FACTOR;
          max_offset_y = sh - shell->disp_height * (1.0 - OVERPAN_FACTOR);
        }
      else
        {
          gint overpan_amount;

          overpan_amount = shell->disp_height - sh * (1.0 - OVERPAN_FACTOR);

          min_offset_y = 0  - overpan_amount;
          max_offset_y = sh + overpan_amount - shell->disp_height;
        }


      /* Handle scrollbar stepper sensitiity */

      gtk_range_set_lower_stepper_sensitivity (GTK_RANGE (shell->hsb),
                                               min_offset_x < shell->offset_x ?
                                               GTK_SENSITIVITY_ON :
                                               GTK_SENSITIVITY_OFF);

      gtk_range_set_upper_stepper_sensitivity (GTK_RANGE (shell->hsb),
                                               max_offset_x > shell->offset_x ?
                                               GTK_SENSITIVITY_ON :
                                               GTK_SENSITIVITY_OFF);

      gtk_range_set_lower_stepper_sensitivity (GTK_RANGE (shell->vsb),
                                               min_offset_y < shell->offset_y ?
                                               GTK_SENSITIVITY_ON :
                                               GTK_SENSITIVITY_OFF);

      gtk_range_set_upper_stepper_sensitivity (GTK_RANGE (shell->vsb),
                                               max_offset_y > shell->offset_y ?
                                               GTK_SENSITIVITY_ON :
                                               GTK_SENSITIVITY_OFF);


      /* Clamp */

      shell->offset_x = CLAMP (shell->offset_x, min_offset_x, max_offset_x);
      shell->offset_y = CLAMP (shell->offset_y, min_offset_y, max_offset_y);
    }
  else
    {
      shell->offset_x = 0;
      shell->offset_y = 0;
    }
}

/**
 * gimp_display_shell_scroll_center_image:
 * @shell:
 * @horizontally:
 * @vertically:
 *
 * Centers the image in the display shell on the desired axes.
 *
 **/
void
gimp_display_shell_scroll_center_image (GimpDisplayShell *shell,
                                        gboolean          horizontally,
                                        gboolean          vertically)
{
  gint sw, sh;
  gint target_offset_x, target_offset_y;

  g_return_if_fail (GIMP_IS_DISPLAY_SHELL (shell));

  if (! shell->display)
    return;

  target_offset_x = shell->offset_x;
  target_offset_y = shell->offset_y;

  gimp_display_shell_get_scaled_image_size (shell, &sw, &sh);

  if (horizontally)
    {
      target_offset_x = (sw - shell->disp_width) / 2;
    }

  if (vertically)
    {
      target_offset_y = (sh - shell->disp_height) / 2;
    }

  /* Note that we can't use gimp_display_shell_scroll_private() here
   * because that would expose the image twice, causing unwanted
   * flicker.
   */
  gimp_display_shell_scale_by_values (shell, gimp_zoom_model_get_factor (shell->zoom),
                                      target_offset_x, target_offset_y,
                                      FALSE);
}

static void
gimp_display_shell_scroll_center_image_callback (GimpDisplayShell *shell,
                                                 GtkAllocation    *allocation,
                                                 GtkWidget        *canvas)
{
  gimp_display_shell_scroll_center_image (shell, TRUE, TRUE);

  g_signal_handlers_disconnect_by_func (canvas,
                                        gimp_display_shell_scroll_center_image_callback,
                                        shell);
}

/**
 * gimp_display_shell_scroll_center_image_on_next_size_allocate:
 * @shell:
 *
 * Centers the image in the display as soon as the canvas has got its
 * new size
 *
 **/
void
gimp_display_shell_scroll_center_image_on_next_size_allocate (GimpDisplayShell *shell)
{
  g_return_if_fail (GIMP_IS_DISPLAY_SHELL (shell));

  g_signal_connect_swapped (shell->canvas, "size-allocate",
                            G_CALLBACK (gimp_display_shell_scroll_center_image_callback),
                            shell);
}

/**
 * gimp_display_shell_get_scaled_viewport:
 * @shell:
 * @x:
 * @y:
 * @w:
 * @h:
 *
 * Gets the viewport in screen coordinates, with origin at (0, 0) in
 * the image
 *
 **/
void
gimp_display_shell_get_scaled_viewport (const GimpDisplayShell *shell,
                                        gint                   *x,
                                        gint                   *y,
                                        gint                   *w,
                                        gint                   *h)
{
  gint scaled_viewport_offset_x;
  gint scaled_viewport_offset_y;

  g_return_if_fail (GIMP_IS_DISPLAY_SHELL (shell));

  gimp_display_shell_get_scaled_viewport_offset (shell,
                                                 &scaled_viewport_offset_x,
                                                 &scaled_viewport_offset_y);
  if (x) *x = -scaled_viewport_offset_x;
  if (y) *y = -scaled_viewport_offset_y;
  if (w) *w =  shell->disp_width;
  if (h) *h =  shell->disp_height;
}

/**
 * gimp_display_shell_get_viewport:
 * @shell:
 * @x:
 * @y:
 * @w:
 * @h:
 *
 * Gets the viewport in image coordinates
 *
 **/
void
gimp_display_shell_get_viewport (const GimpDisplayShell *shell,
                                 gdouble                *x,
                                 gdouble                *y,
                                 gdouble                *w,
                                 gdouble                *h)
{
  g_return_if_fail (GIMP_IS_DISPLAY_SHELL (shell));

  if (x) *x = shell->offset_x    / shell->scale_x;
  if (y) *y = shell->offset_y    / shell->scale_y;
  if (w) *w = shell->disp_width  / shell->scale_x;
  if (h) *h = shell->disp_height / shell->scale_y;
}

/**
 * gimp_display_shell_get_scaled_viewport_offset:
 * @shell:
 * @x:
 * @y:
 *
 * Gets the scaled image offset in viewport coordinates
 *
 **/
void
gimp_display_shell_get_scaled_viewport_offset (const GimpDisplayShell *shell,
                                               gint                   *x,
                                               gint                   *y)
{
  g_return_if_fail (GIMP_IS_DISPLAY_SHELL (shell));

  if (x) *x = -shell->offset_x;
  if (y) *y = -shell->offset_y;
}

/**
 * gimp_display_shell_get_scaled_image_size:
 * @shell:
 * @w:
 * @h:
 *
 * Gets the size of the rendered image after it has been scaled.
 *
 **/
void
gimp_display_shell_get_scaled_image_size (const GimpDisplayShell *shell,
                                          gint                   *w,
                                          gint                   *h)
{
  GimpProjection *proj;
  TileManager    *tiles;
  gint            level;
  gint            level_width;
  gint            level_height;

  g_return_if_fail (GIMP_IS_DISPLAY_SHELL (shell));
  g_return_if_fail (GIMP_IS_IMAGE (shell->display->image));

  proj = gimp_image_get_projection (shell->display->image);

  level = gimp_projection_get_level (proj, shell->scale_x, shell->scale_y);

  tiles = gimp_projection_get_tiles_at_level (proj, level, NULL);

  level_width  = tile_manager_width (tiles);
  level_height = tile_manager_height (tiles);

  if (w) *w = PROJ_ROUND (level_width  * (shell->scale_x * (1 << level)));
  if (h) *h = PROJ_ROUND (level_height * (shell->scale_y * (1 << level)));
}

/**
 * gimp_display_shell_get_disp_offset:
 * @shell:
 * @disp_xoffset:
 * @disp_yoffset:
 *
 * In viewport coordinates, get the offset of where to start rendering
 * the scaled image.
 *
 **/
void
gimp_display_shell_get_disp_offset (const GimpDisplayShell *shell,
                                    gint                   *disp_xoffset,
                                    gint                   *disp_yoffset)
{
  g_return_if_fail (GIMP_IS_DISPLAY_SHELL (shell));

  if (disp_xoffset)
    {
      if (shell->offset_x < 0)
        {
          *disp_xoffset = -shell->offset_x;
        }
      else
        {
          *disp_xoffset = 0;
        }
    }

  if (disp_yoffset)
    {
      if (shell->offset_y < 0)
        {
          *disp_yoffset = -shell->offset_y;
        }
      else
        {
          *disp_yoffset = 0;
        }
    }
}

/**
 * gimp_display_shell_get_render_start_offset:
 * @shell:
 * @offset_x:
 * @offset_y:
 *
 * Get the offset into the scaled image that we should start render
 * from
 *
 **/
void
gimp_display_shell_get_render_start_offset (const GimpDisplayShell *shell,
                                            gint                   *offset_x,
                                            gint                   *offset_y)
{
  g_return_if_fail (GIMP_IS_DISPLAY_SHELL (shell));

  if (offset_x) *offset_x = MAX (0, shell->offset_x);
  if (offset_y) *offset_y = MAX (0, shell->offset_y);
}

/**
 * gimp_display_shell_setup_hscrollbar_with_value:
 * @shell:
 * @value:
 *
 * Setup the limits of the horizontal scrollbar
 *
 **/
void
gimp_display_shell_setup_hscrollbar_with_value (GimpDisplayShell *shell,
                                                gdouble           value)
{
  gint sw;

  g_return_if_fail (GIMP_IS_DISPLAY_SHELL (shell));

  if (! shell->display ||
      ! shell->display->image)
    return;

  gimp_display_shell_get_scaled_image_size (shell, &sw, NULL);

  if (shell->disp_width < sw)
    {
      shell->hsbdata->lower = MIN (value,
                                   0);

      shell->hsbdata->upper = MAX (value + shell->disp_width,
                                   sw);
    }
  else
    {
      shell->hsbdata->lower = MIN (value,
                                   -(shell->disp_width - sw) / 2);

      shell->hsbdata->upper = MAX (value + shell->disp_width,
                                   sw + (shell->disp_width - sw) / 2);
    }

  shell->hsbdata->step_increment = MAX (shell->scale_x, 1.0);
}

/**
 * gimp_display_shell_setup_vscrollbar_with_value:
 * @shell:
 * @value:
 *
 * Setup the limits of the vertical scrollbar
 *
 **/
void
gimp_display_shell_setup_vscrollbar_with_value (GimpDisplayShell *shell,
                                                gdouble           value)
{
  gint sh;

  g_return_if_fail (GIMP_IS_DISPLAY_SHELL (shell));

  if (! shell->display ||
      ! shell->display->image)
    return;

  gimp_display_shell_get_scaled_image_size (shell, NULL, &sh);

  if (shell->disp_height < sh)
    {
      shell->vsbdata->lower = MIN (value,
                                   0);

      shell->vsbdata->upper = MAX (value + shell->disp_height,
                                   sh);
    }
  else
    {
      shell->vsbdata->lower = MIN (value,
                                   -(shell->disp_height - sh) / 2);

      shell->vsbdata->upper = MAX (value + shell->disp_height,
                                   sh + (shell->disp_height - sh) / 2);
    }

  shell->vsbdata->step_increment = MAX (shell->scale_y, 1.0);
}
