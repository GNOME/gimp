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

#include <gegl.h>
#include <gtk/gtk.h>

#include "libgimpbase/gimpbase.h"
#include "libgimpcolor/gimpcolor.h"
#include "libgimpmath/gimpmath.h"
#include "libgimpwidgets/gimpwidgets.h"

#include "display-types.h"

#include "core/gimp-cairo.h"
#include "core/gimp-utils.h"
#include "core/gimpimage.h"

#include "gimpcanvas.h"
#include "gimpcanvas-style.h"
#include "gimpdisplay.h"
#include "gimpdisplayshell.h"
#include "gimpdisplayshell-draw.h"
#include "gimpdisplayshell-render.h"

#include "widgets/gimprender.h"


/*  public functions  */

void
gimp_display_shell_draw_selection_out (GimpDisplayShell *shell,
                                       cairo_t          *cr,
                                       GimpSegment      *segs,
                                       gint              n_segs)
{
  g_return_if_fail (GIMP_IS_DISPLAY_SHELL (shell));
  g_return_if_fail (cr != NULL);
  g_return_if_fail (segs != NULL && n_segs > 0);

  gimp_canvas_set_selection_out_style (shell->canvas, cr,
                                       shell->offset_x, shell->offset_y);

  gimp_cairo_segments (cr, segs, n_segs);
  cairo_stroke (cr);
}

void
gimp_display_shell_draw_selection_in (GimpDisplayShell   *shell,
                                      cairo_t            *cr,
                                      cairo_pattern_t    *mask,
                                      gint                index)
{
  g_return_if_fail (GIMP_IS_DISPLAY_SHELL (shell));
  g_return_if_fail (cr != NULL);
  g_return_if_fail (mask != NULL);

  gimp_canvas_set_selection_in_style (shell->canvas, cr, index,
                                      shell->offset_x, shell->offset_y);

  cairo_mask (cr, mask);
}

void
gimp_display_shell_draw_background (GimpDisplayShell *shell,
                                    cairo_t          *cr)
{
  GimpCanvas *canvas;

  g_return_if_fail (GIMP_IS_DISPLAY_SHELL (shell));
  g_return_if_fail (cr != NULL);

  canvas = GIMP_CANVAS (shell->canvas);

  if (canvas->padding_mode != GIMP_CANVAS_PADDING_MODE_DEFAULT)
    {
      GimpColorConfig *config = gimp_display_shell_get_color_config (shell);

      /* Padding color is color-managed to shell's monitor profile but not
       * soft-proofed.
       */
      gimp_cairo_set_source_color (cr, canvas->padding_color, config, FALSE,
                                   GTK_WIDGET (shell));
      cairo_paint (cr);
    }
}

void
gimp_display_shell_draw_checkerboard (GimpDisplayShell *shell,
                                      cairo_t          *cr)
{
  GimpImage *image;

  g_return_if_fail (GIMP_IS_DISPLAY_SHELL (shell));
  g_return_if_fail (cr != NULL);

  image = gimp_display_get_image (shell->display);

  if (G_UNLIKELY (! shell->checkerboard))
    {
      GimpCheckSize    check_size;
      const GeglColor *rgb1;
      const GeglColor *rgb2;

      g_object_get (shell->display->config,
                    "transparency-size", &check_size,
                    NULL);

      rgb1 = gimp_render_check_color1 ();
      rgb2 = gimp_render_check_color2 ();

      shell->checkerboard =
        gimp_cairo_checkerboard_create (cr, 1 << (check_size + 2), rgb1, rgb2);
    }

  cairo_translate (cr, - shell->offset_x, - shell->offset_y);

  if (gimp_image_get_component_visible (image, GIMP_CHANNEL_ALPHA))
    cairo_set_source (cr, shell->checkerboard);
  else
    cairo_set_source_rgb (cr, 0.0, 0.0, 0.0);

  cairo_paint (cr);
}

void
gimp_display_shell_draw_image (GimpDisplayShell *shell,
                               cairo_t          *cr,
                               gint              x,
                               gint              y,
                               gint              w,
                               gint              h)
{
  gdouble chunk_width;
  gdouble chunk_height;
  gdouble scale = 1.0;
  gint    n_rows;
  gint    n_cols;
  gint    r, c;

  g_return_if_fail (GIMP_IS_DISPLAY_SHELL (shell));
  g_return_if_fail (gimp_display_get_image (shell->display));
  g_return_if_fail (cr != NULL);

  /*  display the image in RENDER_BUF_WIDTH x RENDER_BUF_HEIGHT
   *  maximally-sized image-space chunks.  adjust the screen-space
   *  chunk size as necessary, to accommodate for the display
   *  transform and window scale factor.
   */
  chunk_width  = shell->render_buf_width;
  chunk_height = shell->render_buf_height;

  /* multiply the image scale-factor by the window scale-factor, and divide
   * the cairo scale-factor by the same amount (further down), so that we make
   * full use of the screen resolution, even on hidpi displays.
   */
  scale *= shell->render_scale;

  scale *= MAX (shell->scale_x, shell->scale_y);

  if (scale != shell->scale_x)
    chunk_width  = (chunk_width  - 1.0) * (shell->scale_x / scale);
  if (scale != shell->scale_y)
    chunk_height = (chunk_height - 1.0) * (shell->scale_y / scale);

  if (shell->rotate_untransform)
    {
      gdouble a = shell->rotate_angle * G_PI / 180.0;

      chunk_width = chunk_height = (MIN (chunk_width, chunk_height) - 1.0) /
                                   (fabs (sin (a)) + fabs (cos (a)));
    }

  /* divide the painted area to evenly-sized chunks */
  n_rows = ceil (h / floor (chunk_height));
  n_cols = ceil (w / floor (chunk_width));

  for (r = 0; r < n_rows; r++)
    {
      gint y1 = y + (2 *  r      * h + n_rows) / (2 * n_rows);
      gint y2 = y + (2 * (r + 1) * h + n_rows) / (2 * n_rows);

      for (c = 0; c < n_cols; c++)
        {
          gint x1 = x + (2 *  c      * w + n_cols) / (2 * n_cols);
          gint x2 = x + (2 * (c + 1) * w + n_cols) / (2 * n_cols);

          cairo_save (cr);

          /* clip to chunk bounds, in screen space */
          cairo_rectangle (cr, x1, y1, x2 - x1, y2 - y1);
          cairo_clip (cr);

          if (! gimp_display_shell_render_is_valid (shell,
                                                    x1, y1, x2 - x1, y2 - y1))
            {
              /* render image to the render cache */
              gimp_display_shell_render (shell, cr,
                                         x1, y1, x2 - x1, y2 - y1,
                                         scale);

              gimp_display_shell_render_validate_area (shell,
                                                       x1, y1, x2 - x1, y2 - y1);
            }

          /* divide the cairo scale-factor by the window scale-factor, since
           * the render cache uses device pixels.  see comment further up.
           */
          cairo_scale (cr,
                       1.0 / shell->render_scale,
                       1.0 / shell->render_scale);

          /* render from the render cache to screen */
          cairo_set_source_surface (cr, shell->render_cache, 0, 0);
          cairo_paint (cr);

          cairo_restore (cr);

          /* if the GIMP_BRICK_WALL environment variable is defined,
           * show chunk bounds
           */
          {
            static gint brick_wall = -1;

            if (brick_wall < 0)
              brick_wall = (g_getenv ("GIMP_BRICK_WALL") != NULL);

            if (brick_wall)
              {
                cairo_set_source_rgb (cr, 0.0, 0.0, 0.0);
                cairo_rectangle (cr, x1, y1, x2 - x1, y2 - y1);
                cairo_stroke (cr);
              }
          }
        }
    }
}
