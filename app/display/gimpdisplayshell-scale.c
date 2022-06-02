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

#include "config/gimpguiconfig.h"

#include "core/gimp.h"
#include "core/gimpimage.h"

#include "gimpdisplay.h"
#include "gimpdisplayshell.h"
#include "gimpdisplayshell-expose.h"
#include "gimpdisplayshell-render.h"
#include "gimpdisplayshell-rotate.h"
#include "gimpdisplayshell-scale.h"
#include "gimpdisplayshell-scroll.h"
#include "gimpdisplayshell-transform.h"
#include "gimpimagewindow.h"


#define SCALE_TIMEOUT             2
#define SCALE_EPSILON             0.0001
#define ALMOST_CENTERED_THRESHOLD 2

#define SCALE_EQUALS(a,b) (fabs ((a) - (b)) < SCALE_EPSILON)


/*  local function prototypes  */

static void      gimp_display_shell_scale_get_screen_resolution
                                                         (GimpDisplayShell *shell,
                                                          gdouble          *xres,
                                                          gdouble          *yres);
static void      gimp_display_shell_scale_get_image_size_for_scale
                                                         (GimpDisplayShell *shell,
                                                          gdouble           scale,
                                                          gint             *w,
                                                          gint             *h);
static void      gimp_display_shell_calculate_scale_x_and_y
                                                         (GimpDisplayShell *shell,
                                                          gdouble           scale,
                                                          gdouble          *scale_x,
                                                          gdouble          *scale_y);

static void      gimp_display_shell_scale_to             (GimpDisplayShell *shell,
                                                          gdouble           scale,
                                                          gdouble           viewport_x,
                                                          gdouble           viewport_y);
static void      gimp_display_shell_scale_fit_or_fill    (GimpDisplayShell *shell,
                                                          gboolean          fill);

static gboolean  gimp_display_shell_scale_image_starts_to_fit
                                                         (GimpDisplayShell *shell,
                                                          gdouble           new_scale,
                                                          gdouble           current_scale,
                                                          gboolean         *horizontally,
                                                          gboolean         *vertically);
static gboolean  gimp_display_shell_scale_image_stops_to_fit
                                                         (GimpDisplayShell *shell,
                                                          gdouble           new_scale,
                                                          gdouble           current_scale,
                                                          gboolean         *horizontally,
                                                          gboolean         *vertically);

static gboolean  gimp_display_shell_scale_viewport_coord_almost_centered
                                                         (GimpDisplayShell *shell,
                                                          gint              x,
                                                          gint              y,
                                                          gboolean         *horizontally,
                                                          gboolean         *vertically);

static void      gimp_display_shell_scale_get_image_center_viewport
                                                         (GimpDisplayShell *shell,
                                                          gint             *image_center_x,
                                                          gint             *image_center_y);


static void      gimp_display_shell_scale_get_zoom_focus (GimpDisplayShell *shell,
                                                          gdouble           new_scale,
                                                          gdouble           current_scale,
                                                          gdouble          *x,
                                                          gdouble          *y,
                                                          GimpZoomFocus     zoom_focus);


/*  public functions  */

/**
 * gimp_display_shell_scale_revert:
 * @shell:     the #GimpDisplayShell
 *
 * Reverts the display to the previously used scale. If no previous
 * scale exist, then the call does nothing.
 *
 * Returns: %TRUE if the scale was reverted, otherwise %FALSE.
 **/
gboolean
gimp_display_shell_scale_revert (GimpDisplayShell *shell)
{
  g_return_val_if_fail (GIMP_IS_DISPLAY_SHELL (shell), FALSE);

  /* don't bother if no scale has been set */
  if (shell->last_scale < SCALE_EPSILON)
    return FALSE;

  shell->last_scale_time = 0;

  gimp_display_shell_scale_by_values (shell,
                                      shell->last_scale,
                                      shell->last_offset_x,
                                      shell->last_offset_y,
                                      FALSE);   /* don't resize the window */

  return TRUE;
}

/**
 * gimp_display_shell_scale_can_revert:
 * @shell: the #GimpDisplayShell
 *
 * Returns: %TRUE if a previous display scale exists, otherwise %FALSE.
 **/
gboolean
gimp_display_shell_scale_can_revert (GimpDisplayShell *shell)
{
  g_return_val_if_fail (GIMP_IS_DISPLAY_SHELL (shell), FALSE);

  return (shell->last_scale > SCALE_EPSILON);
}

/**
 * gimp_display_shell_scale_save_revert_values:
 * @shell:
 *
 * Handle the updating of the Revert Zoom variables.
 **/
void
gimp_display_shell_scale_save_revert_values (GimpDisplayShell *shell)
{
  guint now;

  g_return_if_fail (GIMP_IS_DISPLAY_SHELL (shell));

  now = time (NULL);

  if (now - shell->last_scale_time >= SCALE_TIMEOUT)
    {
      shell->last_scale    = gimp_zoom_model_get_factor (shell->zoom);
      shell->last_offset_x = shell->offset_x;
      shell->last_offset_y = shell->offset_y;
    }

  shell->last_scale_time = now;
}

/**
 * gimp_display_shell_scale_set_dot_for_dot:
 * @shell:        the #GimpDisplayShell
 * @dot_for_dot:  whether "Dot for Dot" should be enabled
 *
 * If @dot_for_dot is set to %TRUE then the "Dot for Dot" mode (where image and
 * screen pixels are of the same size) is activated. Dually, the mode is
 * disabled if @dot_for_dot is %FALSE.
 **/
void
gimp_display_shell_scale_set_dot_for_dot (GimpDisplayShell *shell,
                                          gboolean          dot_for_dot)
{
  g_return_if_fail (GIMP_IS_DISPLAY_SHELL (shell));

  if (dot_for_dot != shell->dot_for_dot)
    {
      GimpDisplayConfig *config = shell->display->config;
      gboolean           resize_window;

      /* Resize windows only in multi-window mode */
      resize_window = (config->resize_windows_on_zoom &&
                       ! GIMP_GUI_CONFIG (config)->single_window_mode);

      /* freeze the active tool */
      gimp_display_shell_pause (shell);

      shell->dot_for_dot = dot_for_dot;

      gimp_display_shell_scale_update (shell);

      gimp_display_shell_scale_resize (shell, resize_window, FALSE);

      /* re-enable the active tool */
      gimp_display_shell_resume (shell);
    }
}

/**
 * gimp_display_shell_scale_get_image_size:
 * @shell:
 * @w:
 * @h:
 *
 * Gets the size of the rendered image after it has been scaled.
 *
 **/
void
gimp_display_shell_scale_get_image_size (GimpDisplayShell *shell,
                                         gint             *w,
                                         gint             *h)
{
  g_return_if_fail (GIMP_IS_DISPLAY_SHELL (shell));

  gimp_display_shell_scale_get_image_size_for_scale (shell,
                                                     gimp_zoom_model_get_factor (shell->zoom),
                                                     w, h);
}

/**
 * gimp_display_shell_scale_get_image_bounds:
 * @shell:
 * @x:
 * @y:
 * @w:
 * @h:
 *
 * Gets the screen-space boudning box of the image, after it has
 * been transformed (i.e., scaled, rotated, and scrolled).
 **/
void
gimp_display_shell_scale_get_image_bounds (GimpDisplayShell *shell,
                                           gint             *x,
                                           gint             *y,
                                           gint             *w,
                                           gint             *h)
{
  GimpImage *image;
  gdouble    x1, y1;
  gdouble    x2, y2;

  g_return_if_fail (GIMP_IS_DISPLAY_SHELL (shell));

  image = gimp_display_get_image (shell->display);

  gimp_display_shell_transform_bounds (shell,
                                       0, 0,
                                       gimp_image_get_width (image),
                                       gimp_image_get_height (image),
                                       &x1, &y1,
                                       &x2, &y2);

  x1 = ceil (x1);
  y1 = ceil (y1);
  x2 = floor (x2);
  y2 = floor (y2);

  if (x) *x = x1 + shell->offset_x;
  if (y) *y = y1 + shell->offset_y;
  if (w) *w = x2 - x1;
  if (h) *h = y2 - y1;
}

/**
 * gimp_display_shell_scale_get_image_unrotated_bounds:
 * @shell:
 * @x:
 * @y:
 * @w:
 * @h:
 *
 * Gets the screen-space boudning box of the image, after it has
 * been scaled and scrolled, but before it has been rotated.
 **/
void
gimp_display_shell_scale_get_image_unrotated_bounds (GimpDisplayShell *shell,
                                                     gint             *x,
                                                     gint             *y,
                                                     gint             *w,
                                                     gint             *h)
{
  GimpImage *image;

  g_return_if_fail (GIMP_IS_DISPLAY_SHELL (shell));

  image = gimp_display_get_image (shell->display);

  if (x) *x = -shell->offset_x;
  if (y) *y = -shell->offset_y;
  if (w) *w = floor (gimp_image_get_width  (image) * shell->scale_x);
  if (h) *h = floor (gimp_image_get_height (image) * shell->scale_y);
}

/**
 * gimp_display_shell_scale_get_image_bounding_box:
 * @shell:
 * @x:
 * @y:
 * @w:
 * @h:
 *
 * Gets the screen-space boudning box of the image content, after it has
 * been transformed (i.e., scaled, rotated, and scrolled).
 **/
void
gimp_display_shell_scale_get_image_bounding_box (GimpDisplayShell *shell,
                                                 gint             *x,
                                                 gint             *y,
                                                 gint             *w,
                                                 gint             *h)
{
  GeglRectangle bounding_box;
  gdouble       x1, y1;
  gdouble       x2, y2;

  g_return_if_fail (GIMP_IS_DISPLAY_SHELL (shell));

  bounding_box = gimp_display_shell_get_bounding_box (shell);

  gimp_display_shell_transform_bounds (shell,
                                       bounding_box.x,
                                       bounding_box.y,
                                       bounding_box.x + bounding_box.width,
                                       bounding_box.y + bounding_box.height,
                                       &x1, &y1,
                                       &x2, &y2);

  if (! shell->show_all)
    {
      x1 = ceil  (x1);
      y1 = ceil  (y1);
      x2 = floor (x2);
      y2 = floor (y2);
    }
  else
    {
      x1 = floor (x1);
      y1 = floor (y1);
      x2 = ceil  (x2);
      y2 = ceil  (y2);
    }

  if (x) *x = x1 + shell->offset_x;
  if (y) *y = y1 + shell->offset_y;
  if (w) *w = x2 - x1;
  if (h) *h = y2 - y1;
}

/**
 * gimp_display_shell_scale_get_image_unrotated_bounding_box:
 * @shell:
 * @x:
 * @y:
 * @w:
 * @h:
 *
 * Gets the screen-space boudning box of the image content, after it has
 * been scaled and scrolled, but before it has been rotated.
 **/
void
gimp_display_shell_scale_get_image_unrotated_bounding_box (GimpDisplayShell *shell,
                                                           gint             *x,
                                                           gint             *y,
                                                           gint             *w,
                                                           gint             *h)
{
  GeglRectangle bounding_box;
  gdouble       x1, y1;
  gdouble       x2, y2;

  g_return_if_fail (GIMP_IS_DISPLAY_SHELL (shell));

  bounding_box = gimp_display_shell_get_bounding_box (shell);

  x1 = bounding_box.x * shell->scale_x -
       shell->offset_x;
  y1 = bounding_box.y * shell->scale_y -
       shell->offset_y;

  x2 = (bounding_box.x + bounding_box.width)  * shell->scale_x -
       shell->offset_x;
  y2 = (bounding_box.y + bounding_box.height) * shell->scale_y -
       shell->offset_y;

  if (! shell->show_all)
    {
      x1 = ceil  (x1);
      y1 = ceil  (y1);
      x2 = floor (x2);
      y2 = floor (y2);
    }
  else
    {
      x1 = floor (x1);
      y1 = floor (y1);
      x2 = ceil  (x2);
      y2 = ceil  (y2);
    }

  if (x) *x = x1;
  if (y) *y = y1;
  if (w) *w = x2 - x1;
  if (h) *h = y2 - y1;
}

/**
 * gimp_display_shell_scale_image_is_within_viewport:
 * @shell:
 *
 * Returns: %TRUE if the (scaled) image is smaller than and within the
 *          viewport.
 **/
gboolean
gimp_display_shell_scale_image_is_within_viewport (GimpDisplayShell *shell,
                                                   gboolean         *horizontally,
                                                   gboolean         *vertically)
{
  gboolean horizontally_dummy, vertically_dummy;

  g_return_val_if_fail (GIMP_IS_DISPLAY_SHELL (shell), FALSE);

  if (! horizontally) horizontally = &horizontally_dummy;
  if (! vertically)   vertically   = &vertically_dummy;

  if (! gimp_display_shell_get_infinite_canvas (shell))
    {
      gint sx, sy;
      gint sw, sh;

      gimp_display_shell_scale_get_image_bounding_box (shell,
                                                       &sx, &sy, &sw, &sh);

      sx -= shell->offset_x;
      sy -= shell->offset_y;

      *horizontally = sx >= 0 && sx + sw <= shell->disp_width;
      *vertically   = sy >= 0 && sy + sh <= shell->disp_height;
    }
  else
    {
      *horizontally = FALSE;
      *vertically   = FALSE;
    }

  return *vertically && *horizontally;
}

/* We used to calculate the scale factor in the SCALEFACTOR_X() and
 * SCALEFACTOR_Y() macros. But since these are rather frequently
 * called and the values rarely change, we now store them in the
 * shell and call this function whenever they need to be recalculated.
 */
void
gimp_display_shell_scale_update (GimpDisplayShell *shell)
{
  GimpImage *image;

  g_return_if_fail (GIMP_IS_DISPLAY_SHELL (shell));

  image = gimp_display_get_image (shell->display);

  if (image)
    {
      gimp_display_shell_calculate_scale_x_and_y (shell,
                                                  gimp_zoom_model_get_factor (shell->zoom),
                                                  &shell->scale_x,
                                                  &shell->scale_y);
    }
  else
    {
      shell->scale_x = 1.0;
      shell->scale_y = 1.0;
    }
}

/**
 * gimp_display_shell_scale:
 * @shell:     the #GimpDisplayShell
 * @zoom_type: whether to zoom in, out or to a specific scale
 * @scale:     ignored unless @zoom_type == %GIMP_ZOOM_TO
 *
 * This function figures out the context of the zoom and behaves
 * appropriately thereafter.
 *
 **/
void
gimp_display_shell_scale (GimpDisplayShell *shell,
                          GimpZoomType      zoom_type,
                          gdouble           new_scale,
                          GimpZoomFocus     zoom_focus)
{
  GimpDisplayConfig *config;
  gdouble            current_scale;
  gdouble            delta = 0.0;
  gboolean           resize_window;

  g_return_if_fail (GIMP_IS_DISPLAY_SHELL (shell));
  g_return_if_fail (shell->canvas != NULL);

  current_scale = gimp_zoom_model_get_factor (shell->zoom);

  if (zoom_type == GIMP_ZOOM_SMOOTH)
    delta = -new_scale;

  if (zoom_type == GIMP_ZOOM_PINCH)
    delta = new_scale;

  if (zoom_type != GIMP_ZOOM_TO)
    new_scale = gimp_zoom_model_zoom_step (zoom_type, current_scale, delta);

  if (SCALE_EQUALS (new_scale, current_scale))
    return;

  config = shell->display->config;

  /* Resize windows only in multi-window mode */
  resize_window = (config->resize_windows_on_zoom &&
                   ! GIMP_GUI_CONFIG (config)->single_window_mode);

  if (resize_window)
    {
      /* If the window is resized on zoom, simply do the zoom and get
       * things rolling
       */
      gimp_zoom_model_zoom (shell->zoom, GIMP_ZOOM_TO, new_scale);

      gimp_display_shell_scale_resize (shell, TRUE, FALSE);

      /* XXX The @zoom_focus policy is clearly not working in this code
       * path. This should be fixed.
       */
    }
  else
    {
      gdouble  x, y;
      gint     image_center_x;
      gint     image_center_y;

      gimp_display_shell_scale_get_zoom_focus (shell,
                                               new_scale,
                                               current_scale,
                                               &x,
                                               &y,
                                               zoom_focus);
      gimp_display_shell_scale_get_image_center_viewport (shell,
                                                          &image_center_x,
                                                          &image_center_y);

      gimp_display_shell_scale_to (shell, new_scale, x, y);

      /* skip centering magic if pointer focus was requested */
      if (zoom_focus != GIMP_ZOOM_FOCUS_POINTER)
        {
          gboolean starts_fitting_horiz;
          gboolean starts_fitting_vert;
          gboolean zoom_focus_almost_centered_horiz;
          gboolean zoom_focus_almost_centered_vert;
          gboolean image_center_almost_centered_horiz;
          gboolean image_center_almost_centered_vert;

          /* If an image axis started to fit due to zooming out or if
           * the focus point is as good as in the center, center on
           * that axis
           */
          gimp_display_shell_scale_image_starts_to_fit (shell,
                                                        new_scale,
                                                        current_scale,
                                                        &starts_fitting_horiz,
                                                        &starts_fitting_vert);

          gimp_display_shell_scale_viewport_coord_almost_centered (shell,
                                                                   x,
                                                                   y,
                                                                   &zoom_focus_almost_centered_horiz,
                                                                   &zoom_focus_almost_centered_vert);
          gimp_display_shell_scale_viewport_coord_almost_centered (shell,
                                                                   image_center_x,
                                                                   image_center_y,
                                                                   &image_center_almost_centered_horiz,
                                                                   &image_center_almost_centered_vert);

          gimp_display_shell_scroll_center_image (shell,
                                                  starts_fitting_horiz ||
                                                  (zoom_focus_almost_centered_horiz &&
                                                   image_center_almost_centered_horiz),
                                                  starts_fitting_vert ||
                                                  (zoom_focus_almost_centered_vert &&
                                                   image_center_almost_centered_vert));
        }
    }
}

/**
 * gimp_display_shell_scale_to_rectangle:
 * @shell:         the #GimpDisplayShell
 * @zoom_type:     whether to zoom in or out
 * @x:             retangle's x in image coordinates
 * @y:             retangle's y in image coordinates
 * @width:         retangle's width in image coordinates
 * @height:        retangle's height in image coordinates
 * @resize_window: whether the display window should be resized
 *
 * Scales and scrolls to a specific image rectangle
 **/
void
gimp_display_shell_scale_to_rectangle (GimpDisplayShell *shell,
                                       GimpZoomType      zoom_type,
                                       gdouble           x,
                                       gdouble           y,
                                       gdouble           width,
                                       gdouble           height,
                                       gboolean          resize_window)
{
  gdouble current_scale;
  gdouble new_scale;
  gdouble factor   = 1.0;
  gint    offset_x = 0;
  gint    offset_y = 0;

  g_return_if_fail (GIMP_IS_DISPLAY_SHELL (shell));

  gimp_display_shell_transform_bounds (shell,
                                       x, y,
                                       x + width, y + height,
                                       &x, &y,
                                       &width, &height);

  /* Convert scrolled (x1, y1, x2, y2) to unscrolled (x, y, width, height). */
  width  -= x;
  height -= y;
  x      += shell->offset_x;
  y      += shell->offset_y;

  width  = MAX (1.0, width);
  height = MAX (1.0, height);

  current_scale = gimp_zoom_model_get_factor (shell->zoom);

  switch (zoom_type)
    {
    case GIMP_ZOOM_IN:
      factor = MIN ((shell->disp_width  / width),
                    (shell->disp_height / height));
      break;

    case GIMP_ZOOM_OUT:
      factor = MAX ((width  / shell->disp_width),
                    (height / shell->disp_height));
      break;

    default:
      g_return_if_reached ();
      break;
    }

  new_scale = current_scale * factor;

  switch (zoom_type)
    {
    case GIMP_ZOOM_IN:
      /*  move the center of the rectangle to the center of the
       *  viewport:
       *
       *  new_offset = center of rectangle in new scale screen coords
       *               including offset
       *               -
       *               center of viewport in screen coords without
       *               offset
       */
      offset_x = RINT (factor * (x + width  / 2.0) - (shell->disp_width  / 2));
      offset_y = RINT (factor * (y + height / 2.0) - (shell->disp_height / 2));
      break;

    case GIMP_ZOOM_OUT:
      /*  move the center of the viewport to the center of the
       *  rectangle:
       *
       *  new_offset = center of viewport in new scale screen coords
       *               including offset
       *               -
       *               center of rectangle in screen coords without
       *               offset
       */
      offset_x = RINT (factor * (shell->offset_x + shell->disp_width  / 2) -
                       ((x + width  / 2.0) - shell->offset_x));

      offset_y = RINT (factor * (shell->offset_y + shell->disp_height / 2) -
                       ((y + height / 2.0) - shell->offset_y));
      break;

    default:
      break;
    }

  if (new_scale != current_scale   ||
      offset_x  != shell->offset_x ||
      offset_y  != shell->offset_y)
    {
      gimp_display_shell_scale_by_values (shell,
                                          new_scale,
                                          offset_x, offset_y,
                                          resize_window);
    }
}

/**
 * gimp_display_shell_scale_fit_in:
 * @shell: the #GimpDisplayShell
 *
 * Sets the scale such that the entire image precisely fits in the
 * display area.
 **/
void
gimp_display_shell_scale_fit_in (GimpDisplayShell *shell)
{
  g_return_if_fail (GIMP_IS_DISPLAY_SHELL (shell));

  gimp_display_shell_scale_fit_or_fill (shell,
                                        /* fill = */ FALSE);
 }

/**
 * gimp_display_shell_scale_fill:
 * @shell: the #GimpDisplayShell
 *
 * Sets the scale such that the entire display area is precisely
 * filled by the image.
 **/
void
gimp_display_shell_scale_fill (GimpDisplayShell *shell)
{
  g_return_if_fail (GIMP_IS_DISPLAY_SHELL (shell));

  gimp_display_shell_scale_fit_or_fill (shell,
                                        /* fill = */ TRUE);
}

/**
 * gimp_display_shell_scale_by_values:
 * @shell:         the #GimpDisplayShell
 * @scale:         the new scale
 * @offset_x:      the new X offset
 * @offset_y:      the new Y offset
 * @resize_window: whether the display window should be resized
 *
 * Directly sets the image scale and image offsets used by the display. If
 * @resize_window is %TRUE then the display window is resized to better
 * accommodate the image, see gimp_display_shell_shrink_wrap().
 **/
void
gimp_display_shell_scale_by_values (GimpDisplayShell *shell,
                                    gdouble           scale,
                                    gint              offset_x,
                                    gint              offset_y,
                                    gboolean          resize_window)
{
  g_return_if_fail (GIMP_IS_DISPLAY_SHELL (shell));

  /*  Abort early if the values are all setup already. We don't
   *  want to inadvertently resize the window (bug #164281).
   */
  if (SCALE_EQUALS (gimp_zoom_model_get_factor (shell->zoom), scale) &&
      shell->offset_x == offset_x &&
      shell->offset_y == offset_y)
    return;

  gimp_display_shell_scale_save_revert_values (shell);

  /* freeze the active tool */
  gimp_display_shell_pause (shell);

  gimp_zoom_model_zoom (shell->zoom, GIMP_ZOOM_TO, scale);

  shell->offset_x = offset_x;
  shell->offset_y = offset_y;

  gimp_display_shell_rotate_update_transform (shell);

  gimp_display_shell_scale_resize (shell, resize_window, FALSE);

  /* re-enable the active tool */
  gimp_display_shell_resume (shell);
}

void
gimp_display_shell_scale_drag (GimpDisplayShell *shell,
                               gdouble           start_x,
                               gdouble           start_y,
                               gdouble           delta_x,
                               gdouble           delta_y)
{
  gdouble scale;

  g_return_if_fail (GIMP_IS_DISPLAY_SHELL (shell));

  scale = gimp_zoom_model_get_factor (shell->zoom);

  if (delta_y != 0.0)
    {
      GimpDisplayConfig *config = shell->display->config;
      gdouble            speed  = config->drag_zoom_speed * 0.01;

      gimp_display_shell_push_zoom_focus_pointer_pos (shell, start_x, start_y);

      if (config->drag_zoom_mode == PROP_DRAG_ZOOM_MODE_DISTANCE)
        {
          gimp_display_shell_scale (shell,
                                    GIMP_ZOOM_TO,
                                    scale * exp (0.005 * speed * delta_y),
                                    GIMP_ZOOM_FOCUS_POINTER);
        }
      else if (delta_y > 0.0) /* drag_zoom_mode == PROP_DRAG_ZOOM_MODE_DURATION */
        {
          gimp_display_shell_scale (shell,
                                    GIMP_ZOOM_TO,
                                    scale * (1 + 0.1 * speed),
                                    GIMP_ZOOM_FOCUS_POINTER);
        }
      else /* delta_y < 0.0 */
        {
          gimp_display_shell_scale (shell,
                                    GIMP_ZOOM_TO,
                                    scale * (1 - 0.1 * speed),
                                    GIMP_ZOOM_FOCUS_POINTER);
        }


      if (shell->zoom_focus_point)
        {
          /* In case we hit one of the cases when the focus pointer
           * position was unused.
           */
          g_slice_free (GdkPoint, shell->zoom_focus_point);
          shell->zoom_focus_point = NULL;
        }
    }
}

/**
 * gimp_display_shell_scale_shrink_wrap:
 * @shell: the #GimpDisplayShell
 *
 * Convenience function with the same functionality as
 * gimp_display_shell_scale_resize(@shell, TRUE, grow_only).
 **/
void
gimp_display_shell_scale_shrink_wrap (GimpDisplayShell *shell,
                                      gboolean          grow_only)
{
  g_return_if_fail (GIMP_IS_DISPLAY_SHELL (shell));

  gimp_display_shell_scale_resize (shell, TRUE, grow_only);
}

/**
 * gimp_display_shell_scale_resize:
 * @shell:          the #GimpDisplayShell
 * @resize_window:  whether the display window should be resized
 * @grow_only:      whether shrinking of the window is allowed or not
 *
 * Function commonly called after a change in display scale to make the changes
 * visible to the user. If @resize_window is %TRUE then the display window is
 * resized to accommodate the display image as per
 * gimp_display_shell_shrink_wrap().
 **/
void
gimp_display_shell_scale_resize (GimpDisplayShell *shell,
                                 gboolean          resize_window,
                                 gboolean          grow_only)
{
  g_return_if_fail (GIMP_IS_DISPLAY_SHELL (shell));

  /* freeze the active tool */
  gimp_display_shell_pause (shell);

  if (resize_window)
    {
      GimpImageWindow *window = gimp_display_shell_get_window (shell);

      if (window && gimp_image_window_get_active_shell (window) == shell)
        {
          gimp_image_window_shrink_wrap (window, grow_only);
        }
    }

  gimp_display_shell_scroll_clamp_and_update (shell);
  gimp_display_shell_scaled (shell);

  gimp_display_shell_expose_full (shell);
  gimp_display_shell_render_invalidate_full (shell);

  /* re-enable the active tool */
  gimp_display_shell_resume (shell);
}

void
gimp_display_shell_set_initial_scale (GimpDisplayShell *shell,
                                      gdouble           scale,
                                      gint             *display_width,
                                      gint             *display_height)
{
  GimpImage    *image;
  GdkRectangle  workarea;
  gint          image_width;
  gint          image_height;
  gint          monitor_width;
  gint          monitor_height;
  gint          shell_width;
  gint          shell_height;

  g_return_if_fail (GIMP_IS_DISPLAY_SHELL (shell));

  image = gimp_display_get_image (shell->display);

  gdk_monitor_get_workarea (shell->initial_monitor, &workarea);

  image_width  = gimp_image_get_width  (image);
  image_height = gimp_image_get_height (image);

  monitor_width  = workarea.width  * 0.75;
  monitor_height = workarea.height * 0.75;

  /* We need to zoom before we use SCALE[XY] */
  gimp_zoom_model_zoom (shell->zoom, GIMP_ZOOM_TO, scale);

  shell_width  = SCALEX (shell, image_width);
  shell_height = SCALEY (shell, image_height);

  if (shell->display->config->initial_zoom_to_fit)
    {
      /*  Limit to the size of the monitor...  */
      if (shell_width > monitor_width || shell_height > monitor_height)
        {
          gdouble new_scale;
          gdouble current = gimp_zoom_model_get_factor (shell->zoom);

          new_scale = current * MIN (((gdouble) monitor_height) / shell_height,
                                     ((gdouble) monitor_width)  / shell_width);

          new_scale = gimp_zoom_model_zoom_step (GIMP_ZOOM_OUT, new_scale, 0.0);

          /*  Since zooming out might skip a zoom step we zoom in
           *  again and test if we are small enough.
           */
          gimp_zoom_model_zoom (shell->zoom, GIMP_ZOOM_TO,
                                gimp_zoom_model_zoom_step (GIMP_ZOOM_IN,
                                                           new_scale, 0.0));

          if (SCALEX (shell, image_width)  > monitor_width ||
              SCALEY (shell, image_height) > monitor_height)
            gimp_zoom_model_zoom (shell->zoom, GIMP_ZOOM_TO, new_scale);

          shell_width  = SCALEX (shell, image_width);
          shell_height = SCALEY (shell, image_height);
        }
    }
  else
    {
      /*  Set up size like above, but do not zoom to fit. Useful when
       *  working on large images.
       */
      shell_width  = MIN (shell_width,  monitor_width);
      shell_height = MIN (shell_height, monitor_height);
    }

  if (display_width)  *display_width  = shell_width;
  if (display_height) *display_height = shell_height;
}

/**
 * gimp_display_shell_get_rotated_scale:
 * @shell:   the #GimpDisplayShell
 * @scale_x: horizontal scale output
 * @scale_y: vertical scale output
 *
 * Returns the screen space horizontal and vertical scaling
 * factors, taking rotation into account.
 **/
void
gimp_display_shell_get_rotated_scale (GimpDisplayShell *shell,
                                      gdouble          *scale_x,
                                      gdouble          *scale_y)
{
  g_return_if_fail (GIMP_IS_DISPLAY_SHELL (shell));

  if (shell->rotate_angle == 0.0 || shell->scale_x == shell->scale_y)
    {
      if (scale_x) *scale_x = shell->scale_x;
      if (scale_y) *scale_y = shell->scale_y;
    }
  else
    {
      gdouble a     = G_PI * shell->rotate_angle / 180.0;
      gdouble cos_a = cos (a);
      gdouble sin_a = sin (a);

      if (scale_x) *scale_x = 1.0 / sqrt (SQR (cos_a / shell->scale_x) +
                                          SQR (sin_a / shell->scale_y));

      if (scale_y) *scale_y = 1.0 / sqrt (SQR (cos_a / shell->scale_y) +
                                          SQR (sin_a / shell->scale_x));
    }
}

/**
 * gimp_display_shell_push_zoom_focus_pointer_pos:
 * @shell:
 * @x:
 * @y:
 *
 * When the zoom focus mechanism asks for the pointer the next time,
 * use @x and @y.
 *
 * It was primarily created for unit testing (see commit 7e3898da093).
 * Therefore it should not be used by our code. When it is, we should
 * make sure that @shell->zoom_focus_point has been properly used and
 * freed, if we don't want it to leak.
 * Just calling gimp_display_shell_scale() is not enough as there are
 * currently some code paths where the values is not used.
 **/
void
gimp_display_shell_push_zoom_focus_pointer_pos (GimpDisplayShell *shell,
                                                gint              x,
                                                gint              y)
{
  GdkPoint *point = g_slice_new (GdkPoint);
  point->x = x;
  point->y = y;

  g_slice_free (GdkPoint, shell->zoom_focus_point);
  shell->zoom_focus_point = point;
}


/*  private functions   */

static void
gimp_display_shell_scale_get_screen_resolution (GimpDisplayShell *shell,
                                                gdouble          *xres,
                                                gdouble          *yres)
{
  gdouble x, y;

  if (shell->dot_for_dot)
    {
      gimp_image_get_resolution (gimp_display_get_image (shell->display),
                                 &x, &y);
    }
  else
    {
      x = shell->monitor_xres;
      y = shell->monitor_yres;
    }

  if (xres) *xres = x;
  if (yres) *yres = y;
}

/**
 * gimp_display_shell_scale_get_image_size_for_scale:
 * @shell:
 * @scale:
 * @w:
 * @h:
 *
 **/
static void
gimp_display_shell_scale_get_image_size_for_scale (GimpDisplayShell *shell,
                                                   gdouble           scale,
                                                   gint             *w,
                                                   gint             *h)
{
  GimpImage *image = gimp_display_get_image (shell->display);
  gdouble    scale_x;
  gdouble    scale_y;

  gimp_display_shell_calculate_scale_x_and_y (shell, scale, &scale_x, &scale_y);

  if (w) *w = scale_x * gimp_image_get_width  (image);
  if (h) *h = scale_y * gimp_image_get_height (image);
}

/**
 * gimp_display_shell_calculate_scale_x_and_y:
 * @shell:
 * @scale:
 * @scale_x:
 * @scale_y:
 *
 **/
static void
gimp_display_shell_calculate_scale_x_and_y (GimpDisplayShell *shell,
                                            gdouble           scale,
                                            gdouble          *scale_x,
                                            gdouble          *scale_y)
{
  GimpImage *image = gimp_display_get_image (shell->display);
  gdouble    xres;
  gdouble    yres;
  gdouble    screen_xres;
  gdouble    screen_yres;

  gimp_image_get_resolution (image, &xres, &yres);
  gimp_display_shell_scale_get_screen_resolution (shell,
                                                  &screen_xres, &screen_yres);

  if (scale_x) *scale_x = scale * screen_xres / xres;
  if (scale_y) *scale_y = scale * screen_yres / yres;
}

/**
 * gimp_display_shell_scale_to:
 * @shell:
 * @scale:
 * @viewport_x:
 * @viewport_y:
 *
 * Zooms. The display offsets are adjusted so that the point specified
 * by @x and @y doesn't change it's position on screen.
 **/
static void
gimp_display_shell_scale_to (GimpDisplayShell *shell,
                             gdouble           scale,
                             gdouble           viewport_x,
                             gdouble           viewport_y)
{
  gdouble image_x, image_y;
  gdouble new_viewport_x, new_viewport_y;

  g_return_if_fail (GIMP_IS_DISPLAY_SHELL (shell));

  if (! shell->display)
    return;

  /* freeze the active tool */
  gimp_display_shell_pause (shell);

  gimp_display_shell_untransform_xy_f (shell,
                                       viewport_x,
                                       viewport_y,
                                       &image_x,
                                       &image_y);

  /* Note that we never come here if we need to resize_windows_on_zoom
   */
  gimp_display_shell_scale_by_values (shell,
                                      scale,
                                      shell->offset_x,
                                      shell->offset_y,
                                      FALSE);

  gimp_display_shell_transform_xy_f (shell,
                                     image_x,
                                     image_y,
                                     &new_viewport_x,
                                     &new_viewport_y);

  gimp_display_shell_scroll (shell,
                             new_viewport_x - viewport_x,
                             new_viewport_y - viewport_y);

  /* re-enable the active tool */
  gimp_display_shell_resume (shell);
}

/**
 * gimp_display_shell_scale_fit_or_fill:
 * @shell: the #GimpDisplayShell
 * @fill:  whether to scale the image to fill the viewport,
 *         or fit inside the viewport
 *
 * A common implementation for gimp_display_shell_scale_{fit_in,fill}().
 **/
static void
gimp_display_shell_scale_fit_or_fill (GimpDisplayShell *shell,
                                      gboolean          fill)
{
  GeglRectangle bounding_box;
  gdouble       image_x;
  gdouble       image_y;
  gdouble       image_width;
  gdouble       image_height;
  gdouble       current_scale;
  gdouble       zoom_factor;

  if (! gimp_display_shell_get_infinite_canvas (shell))
    {
      GimpImage *image = gimp_display_get_image (shell->display);

      bounding_box.x      = 0;
      bounding_box.y      = 0;
      bounding_box.width  = gimp_image_get_width  (image);
      bounding_box.height = gimp_image_get_height (image);
    }
  else
    {
      bounding_box = gimp_display_shell_get_bounding_box (shell);
    }

  gimp_display_shell_transform_bounds (shell,
                                       bounding_box.x,
                                       bounding_box.y,
                                       bounding_box.x + bounding_box.width,
                                       bounding_box.y + bounding_box.height,
                                       &image_x,
                                       &image_y,
                                       &image_width,
                                       &image_height);

  image_width  -= image_x;
  image_height -= image_y;

  current_scale = gimp_zoom_model_get_factor (shell->zoom);

  if (fill)
    {
      zoom_factor = MAX (shell->disp_width  / image_width,
                         shell->disp_height / image_height);
    }
  else
    {
      zoom_factor = MIN (shell->disp_width  / image_width,
                         shell->disp_height / image_height);
    }

  gimp_display_shell_scale (shell,
                            GIMP_ZOOM_TO,
                            zoom_factor * current_scale,
                            GIMP_ZOOM_FOCUS_BEST_GUESS);

  gimp_display_shell_scroll_center_content (shell, TRUE, TRUE);
}

static gboolean
gimp_display_shell_scale_image_starts_to_fit (GimpDisplayShell *shell,
                                              gdouble           new_scale,
                                              gdouble           current_scale,
                                              gboolean         *horizontally,
                                              gboolean         *vertically)
{
  gboolean vertically_dummy;
  gboolean horizontally_dummy;

  if (! vertically)   vertically   = &vertically_dummy;
  if (! horizontally) horizontally = &horizontally_dummy;

  /* The image can only start to fit if we zoom out */
  if (new_scale > current_scale ||
      gimp_display_shell_get_infinite_canvas (shell))
    {
      *vertically   = FALSE;
      *horizontally = FALSE;
    }
  else
    {
      gint current_scale_width;
      gint current_scale_height;
      gint new_scale_width;
      gint new_scale_height;

      gimp_display_shell_scale_get_image_size_for_scale (shell,
                                                         current_scale,
                                                         &current_scale_width,
                                                         &current_scale_height);

      gimp_display_shell_scale_get_image_size_for_scale (shell,
                                                         new_scale,
                                                         &new_scale_width,
                                                         &new_scale_height);

      *vertically   = (current_scale_width  >  shell->disp_width &&
                       new_scale_width      <= shell->disp_width);
      *horizontally = (current_scale_height >  shell->disp_height &&
                       new_scale_height     <= shell->disp_height);
    }

  return *vertically && *horizontally;
}

static gboolean
gimp_display_shell_scale_image_stops_to_fit (GimpDisplayShell *shell,
                                             gdouble           new_scale,
                                             gdouble           current_scale,
                                             gboolean         *horizontally,
                                             gboolean         *vertically)
{
  return gimp_display_shell_scale_image_starts_to_fit (shell,
                                                       current_scale,
                                                       new_scale,
                                                       horizontally,
                                                       vertically);
}

/**
 * gimp_display_shell_scale_viewport_coord_almost_centered:
 * @shell:
 * @x:
 * @y:
 * @horizontally:
 * @vertically:
 *
 **/
static gboolean
gimp_display_shell_scale_viewport_coord_almost_centered (GimpDisplayShell *shell,
                                                         gint              x,
                                                         gint              y,
                                                         gboolean         *horizontally,
                                                         gboolean         *vertically)
{
  gboolean local_horizontally = FALSE;
  gboolean local_vertically   = FALSE;
  gint     center_x           = shell->disp_width  / 2;
  gint     center_y           = shell->disp_height / 2;

  if (! gimp_display_shell_get_infinite_canvas (shell))
    {
      local_horizontally = (x > center_x - ALMOST_CENTERED_THRESHOLD &&
                            x < center_x + ALMOST_CENTERED_THRESHOLD);

      local_vertically   = (y > center_y - ALMOST_CENTERED_THRESHOLD &&
                            y < center_y + ALMOST_CENTERED_THRESHOLD);
    }

  if (horizontally) *horizontally = local_horizontally;
  if (vertically)   *vertically   = local_vertically;

  return local_horizontally && local_vertically;
}

static void
gimp_display_shell_scale_get_image_center_viewport (GimpDisplayShell *shell,
                                                    gint             *image_center_x,
                                                    gint             *image_center_y)
{
  gint sw, sh;

  gimp_display_shell_scale_get_image_size (shell, &sw, &sh);

  if (image_center_x) *image_center_x = -shell->offset_x + sw / 2;
  if (image_center_y) *image_center_y = -shell->offset_y + sh / 2;
}

/**
 * gimp_display_shell_scale_get_zoom_focus:
 * @shell:
 * @new_scale:
 * @x:
 * @y:
 *
 * Calculates the viewport coordinate to focus on when zooming
 * independently for each axis.
 **/
static void
gimp_display_shell_scale_get_zoom_focus (GimpDisplayShell *shell,
                                         gdouble           new_scale,
                                         gdouble           current_scale,
                                         gdouble          *x,
                                         gdouble          *y,
                                         GimpZoomFocus     zoom_focus)
{
  GtkWidget *window = GTK_WIDGET (gimp_display_shell_get_window (shell));
  GdkEvent  *event;
  gint       image_center_x;
  gint       image_center_y;
  gint       other_x;
  gint       other_y;

  /* Calculate stops-to-fit focus point */
  gimp_display_shell_scale_get_image_center_viewport (shell,
                                                      &image_center_x,
                                                      &image_center_y);

  /* Calculate other focus point, default is the canvas center */
  other_x = shell->disp_width  / 2;
  other_y = shell->disp_height / 2;

  /*  Center on the mouse position instead of the display center if
   *  one of the following conditions are fulfilled and pointer is
   *  within the canvas:
   *
   *   (1) there's no current event (the action was triggered by an
   *       input controller)
   *   (2) the event originates from the canvas (a scroll event)
   *   (3) the event originates from the window (a key press event)
   *
   *  Basically the only situation where we don't want to center on
   *  mouse position is if the action is being called from a menu.
   */
  event = gtk_get_current_event ();

  if (! event ||
      gtk_get_event_widget (event) == shell->canvas ||
      gtk_get_event_widget (event) == window)
    {
      gint canvas_pointer_x;
      gint canvas_pointer_y;

      if (shell->zoom_focus_point)
        {
          canvas_pointer_x = shell->zoom_focus_point->x;
          canvas_pointer_y = shell->zoom_focus_point->y;

          g_slice_free (GdkPoint, shell->zoom_focus_point);
          shell->zoom_focus_point = NULL;
        }
      else
        {
          GdkDisplay *display = gtk_widget_get_display (shell->canvas);
          GdkSeat    *seat    = gdk_display_get_default_seat (display);

          gdk_window_get_device_position (gtk_widget_get_window (shell->canvas),
                                          gdk_seat_get_pointer (seat),
                                          &canvas_pointer_x,
                                          &canvas_pointer_y,
                                          NULL);
        }

      if (canvas_pointer_x >= 0 &&
          canvas_pointer_y >= 0 &&
          canvas_pointer_x <  shell->disp_width &&
          canvas_pointer_y <  shell->disp_height)
        {
          other_x = canvas_pointer_x;
          other_y = canvas_pointer_y;
        }
    }

  if (zoom_focus == GIMP_ZOOM_FOCUS_RETAIN_CENTERING_ELSE_BEST_GUESS)
    {
      if (gimp_display_shell_scale_viewport_coord_almost_centered (shell,
                                                                   image_center_x,
                                                                   image_center_y,
                                                                   NULL,
                                                                   NULL))
        {
          zoom_focus = GIMP_ZOOM_FOCUS_IMAGE_CENTER;
        }
      else
        {
          zoom_focus = GIMP_ZOOM_FOCUS_BEST_GUESS;
        }
    }

  switch (zoom_focus)
    {
    case GIMP_ZOOM_FOCUS_POINTER:
      *x = other_x;
      *y = other_y;
      break;

    case GIMP_ZOOM_FOCUS_IMAGE_CENTER:
      *x = image_center_x;
      *y = image_center_y;
      break;

    case GIMP_ZOOM_FOCUS_BEST_GUESS:
    default:
      {
        gboolean within_horizontally, within_vertically;
        gboolean stops_horizontally, stops_vertically;

        gimp_display_shell_scale_image_is_within_viewport (shell,
                                                           &within_horizontally,
                                                           &within_vertically);

        gimp_display_shell_scale_image_stops_to_fit (shell,
                                                     new_scale,
                                                     current_scale,
                                                     &stops_horizontally,
                                                     &stops_vertically);

        *x = within_horizontally && ! stops_horizontally ? image_center_x : other_x;
        *y = within_vertically   && ! stops_vertically   ? image_center_y : other_y;
      }
      break;
    }
}
