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

#include <stdlib.h>

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
#include "gimpdisplayshell-scale.h"
#include "gimpdisplayshell-scroll.h"


#define OVERPAN_FACTOR      0.5
#define MINIMUM_STEP_AMOUNT 1.0


typedef struct
{
  GimpDisplayShell *shell;
  gboolean          vertically;
  gboolean          horizontally;
} SizeAllocateCallbackData;


/**
 * gimp_display_shell_scroll_center_image_coordinate:
 * @shell:
 * @image_x:
 * @image_y:
 *
 * Center the viewport around the passed image coordinate
 *
 **/
void
gimp_display_shell_scroll_center_image_coordinate (GimpDisplayShell *shell,
                                                   gdouble           image_x,
                                                   gdouble           image_y)
{
  gint scaled_image_x;
  gint scaled_image_y;
  gint offset_to_apply_x;
  gint offset_to_apply_y;

  scaled_image_x = RINT (image_x * shell->scale_x);
  scaled_image_y = RINT (image_y * shell->scale_y);

  offset_to_apply_x = scaled_image_x - shell->disp_width  / 2 - shell->offset_x;
  offset_to_apply_y = scaled_image_y - shell->disp_height / 2 - shell->offset_y;

  gimp_display_shell_scroll (shell,
                             offset_to_apply_x,
                             offset_to_apply_y);
}

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

      gimp_display_shell_rotate_update_transform (shell);

      gimp_overlay_box_scroll (GIMP_OVERLAY_BOX (shell->canvas),
                               -x_offset, -y_offset);

      /*  Update scrollbars and rulers  */
      gimp_display_shell_scale_update_scrollbars (shell);
      gimp_display_shell_scale_update_rulers (shell);

      gimp_display_shell_resume (shell);

      gimp_display_shell_scrolled (shell);
    }
}

void
gimp_display_shell_scroll_set_offset (GimpDisplayShell *shell,
                                      gint              offset_x,
                                      gint              offset_y)
{
  g_return_if_fail (GIMP_IS_DISPLAY_SHELL (shell));

  if (shell->offset_x == offset_x &&
      shell->offset_y == offset_y)
    return;

  gimp_display_shell_scale_handle_zoom_revert (shell);

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

void
gimp_display_shell_scroll_clamp_offsets (GimpDisplayShell *shell)
{
  GimpImage *image;

  g_return_if_fail (GIMP_IS_DISPLAY_SHELL (shell));

  image = gimp_display_get_image (shell->display);

  if (image)
    {
      gint sw, sh;
      gint min_offset_x;
      gint max_offset_x;
      gint min_offset_y;
      gint max_offset_y;

      sw = SCALEX (shell, gimp_image_get_width  (image));
      sh = SCALEY (shell, gimp_image_get_height (image));

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
 * gimp_display_shell_scroll_clamp_and_update:
 * @shell:
 *
 * Helper function for calling two functions that are commonly called
 * in pairs.
 **/
void
gimp_display_shell_scroll_clamp_and_update (GimpDisplayShell *shell)
{
  gimp_display_shell_scroll_clamp_offsets (shell);

  gimp_display_shell_scale_update_scrollbars (shell);
  gimp_display_shell_scale_update_rulers (shell);
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
 *
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

  if (! shell->display                          ||
      ! gimp_display_get_image (shell->display) ||
      (! vertically && ! horizontally))
    return;

  target_offset_x = shell->offset_x;
  target_offset_y = shell->offset_y;

  gimp_display_shell_scale_get_image_size (shell, &sw, &sh);

  if (horizontally)
    {
      target_offset_x = (sw - shell->disp_width) / 2;
    }

  if (vertically)
    {
      target_offset_y = (sh - shell->disp_height) / 2;
    }

  gimp_display_shell_scroll_set_offset (shell,
                                        target_offset_x,
                                        target_offset_y);
}

static void
gimp_display_shell_scroll_center_image_callback (GtkWidget                *canvas,
                                                 GtkAllocation            *allocation,
                                                 SizeAllocateCallbackData *data)
{
  gimp_display_shell_scroll_center_image (data->shell,
                                          data->horizontally,
                                          data->vertically);

  g_signal_handlers_disconnect_by_func (canvas,
                                        gimp_display_shell_scroll_center_image_callback,
                                        data);

  g_slice_free (SizeAllocateCallbackData, data);
}

/**
 * gimp_display_shell_scroll_center_image_on_next_size_allocate:
 * @shell:
 *
 * Centers the image in the display as soon as the canvas has got its
 * new size.
 *
 * Only call this if you are sure the canvas size will change.
 * (Otherwise the signal connection and centering will lurk until the
 * canvas size is changed e.g. by toggling the rulers.)
 *
 **/
void
gimp_display_shell_scroll_center_image_on_next_size_allocate (GimpDisplayShell *shell,
                                                              gboolean          horizontally,
                                                              gboolean          vertically)
{
  SizeAllocateCallbackData *data;

  g_return_if_fail (GIMP_IS_DISPLAY_SHELL (shell));

  data = g_slice_new (SizeAllocateCallbackData);

  if (data)
    {
      data->shell        = shell;
      data->horizontally = horizontally;
      data->vertically   = vertically;

      g_signal_connect (shell->canvas, "size-allocate",
                        G_CALLBACK (gimp_display_shell_scroll_center_image_callback),
                        data);
    }

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
 * the image
 *
 **/
void
gimp_display_shell_scroll_get_scaled_viewport (const GimpDisplayShell *shell,
                                               gint                   *x,
                                               gint                   *y,
                                               gint                   *w,
                                               gint                   *h)
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
 * Gets the viewport in image coordinates
 *
 **/
void
gimp_display_shell_scroll_get_viewport (const GimpDisplayShell *shell,
                                        gdouble                *x,
                                        gdouble                *y,
                                        gdouble                *w,
                                        gdouble                *h)
{
  g_return_if_fail (GIMP_IS_DISPLAY_SHELL (shell));

  *x = shell->offset_x    / shell->scale_x;
  *y = shell->offset_y    / shell->scale_y;
  *w = shell->disp_width  / shell->scale_x;
  *h = shell->disp_height / shell->scale_y;
}

/**
 * gimp_display_shell_scroll_setup_hscrollbar:
 * @shell:
 * @value:
 *
 * Setup the limits of the horizontal scrollbar
 *
 **/
void
gimp_display_shell_scroll_setup_hscrollbar (GimpDisplayShell *shell,
                                            gdouble           value)
{
  gint    sw;
  gdouble lower;
  gdouble upper;

  g_return_if_fail (GIMP_IS_DISPLAY_SHELL (shell));

  if (! shell->display ||
      ! gimp_display_get_image (shell->display))
    return;

  gimp_display_shell_scale_get_image_size (shell, &sw, NULL);

  if (shell->disp_width < sw)
    {
      lower = MIN (value, 0);
      upper = MAX (value + shell->disp_width, sw);
    }
  else
    {
      lower = MIN (value, -(shell->disp_width - sw) / 2);
      upper = MAX (value + shell->disp_width,
                   sw + (shell->disp_width - sw) / 2);
    }

  g_object_set (shell->hsbdata,
                "lower",          lower,
                "upper",          upper,
                "step-increment", (gdouble) MAX (shell->scale_x,
                                                 MINIMUM_STEP_AMOUNT),
                NULL);
}

/**
 * gimp_display_shell_scroll_setup_vscrollbar:
 * @shell:
 * @value:
 *
 * Setup the limits of the vertical scrollbar
 *
 **/
void
gimp_display_shell_scroll_setup_vscrollbar (GimpDisplayShell *shell,
                                            gdouble           value)
{
  gint    sh;
  gdouble lower;
  gdouble upper;

  g_return_if_fail (GIMP_IS_DISPLAY_SHELL (shell));

  if (! shell->display ||
      ! gimp_display_get_image (shell->display))
    return;

  gimp_display_shell_scale_get_image_size (shell, NULL, &sh);

  if (shell->disp_height < sh)
    {
      lower = MIN (value, 0);
      upper = MAX (value + shell->disp_height, sh);
    }
  else
    {
      lower = MIN (value, -(shell->disp_height - sh) / 2);
      upper = MAX (value + shell->disp_height,
                   sh + (shell->disp_height - sh) / 2);
    }

  g_object_set (shell->vsbdata,
                "lower",          lower,
                "upper",          upper,
                "step-increment", (gdouble) MAX (shell->scale_y,
                                                 MINIMUM_STEP_AMOUNT),
                NULL);
}
