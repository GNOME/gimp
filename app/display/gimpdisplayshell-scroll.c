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

#include "core/gimpimage.h"

#include "gimpdisplay.h"
#include "gimpdisplay-foreach.h"
#include "gimpdisplayshell.h"
#include "gimpdisplayshell-private.h"
#include "gimpdisplayshell-scale.h"
#include "gimpdisplayshell-scroll.h"


/**
 * gimp_display_shell_scroll:
 * @shell:
 * @x_offset_into_image: In image coordinates.
 * @y_offset_into_image:
 *
 * When the viewport is smaller than the image, offset the viewport to
 * the specified amount into the image.
 *
 * TODO: Behave in a sane way when zoomed out.
 *
 **/
void gimp_display_shell_scroll (GimpDisplayShell *shell,
                                gdouble           x_offset_into_image,
                                gdouble           y_offset_into_image)
{
  gint x_offset;
  gint y_offset;

  x_offset = RINT (x_offset_into_image * shell->scale_x - shell->offset_x);
  y_offset = RINT (y_offset_into_image * shell->scale_y - shell->offset_y);

  gimp_display_shell_scroll_private (shell, x_offset, y_offset);
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

      sw = SCALEX (shell, gimp_image_get_width  (shell->display->image));
      sh = SCALEY (shell, gimp_image_get_height (shell->display->image));

      shell->offset_x = CLAMP (shell->offset_x, 0,
                               MAX (sw - shell->disp_width, 0));

      shell->offset_y = CLAMP (shell->offset_y, 0,
                               MAX (sh - shell->disp_height, 0));
    }
  else
    {
      shell->offset_x = 0;
      shell->offset_y = 0;
    }
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
gimp_display_shell_get_scaled_viewport (GimpDisplayShell *shell,
                                        gint             *x,
                                        gint             *y,
                                        gint             *w,
                                        gint             *h)
{
  if (x) *x = -shell->disp_xoffset + shell->offset_x;
  if (y) *y = -shell->disp_yoffset + shell->offset_y;
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
 * TODO: Handle when the viewport is zoomed out
 *
 **/
void
gimp_display_shell_get_viewport (GimpDisplayShell *shell,
                                 gdouble          *x,
                                 gdouble          *y,
                                 gdouble          *w,
                                 gdouble          *h)
{
  if (x) *x = shell->offset_x    / shell->scale_x;
  if (y) *y = shell->offset_y    / shell->scale_y;
  if (w) *w = shell->disp_width  / shell->scale_x;
  if (h) *h = shell->disp_height / shell->scale_y;
}

/**
 * gimp_display_shell_get_scaled_image_viewport_offset:
 * @shell:
 * @x:
 * @y:
 *
 * Gets the scaled image offset in viewport coordinates
 *
 **/
void
gimp_display_shell_get_scaled_image_viewport_offset (GimpDisplayShell *shell,
                                                     gint             *x,
                                                     gint             *y)
{
  if (x) *x = shell->disp_xoffset - shell->offset_x;
  if (y) *y = shell->disp_yoffset - shell->offset_y;
}
