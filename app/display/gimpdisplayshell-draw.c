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
#include "libgimpcolor/gimpcolor.h"
#include "libgimpmath/gimpmath.h"
#include "libgimpwidgets/gimpwidgets.h"

#include "display-types.h"

#include "core/gimp-cairo.h"
#include "core/gimp-utils.h"

#include "gimpcanvas.h"
#include "gimpcanvas-style.h"
#include "gimpcanvaspath.h"
#include "gimpdisplay.h"
#include "gimpdisplayshell.h"
#include "gimpdisplayshell-draw.h"
#include "gimpdisplayshell-render.h"
#include "gimpdisplayshell-scale.h"
#include "gimpdisplayshell-transform.h"
#include "gimpdisplayxfer.h"


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

  gimp_cairo_add_segments (cr, segs, n_segs);
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
  GdkWindow       *window;
  cairo_pattern_t *bg_pattern;

  g_return_if_fail (GIMP_IS_DISPLAY_SHELL (shell));
  g_return_if_fail (cr != NULL);

  window     = gtk_widget_get_window (shell->canvas);
  bg_pattern = gdk_window_get_background_pattern (window);

  cairo_set_source (cr, bg_pattern);
  cairo_paint (cr);
}

void
gimp_display_shell_draw_checkerboard (GimpDisplayShell *shell,
                                      cairo_t          *cr)
{
  g_return_if_fail (GIMP_IS_DISPLAY_SHELL (shell));
  g_return_if_fail (cr != NULL);

  if (G_UNLIKELY (! shell->checkerboard))
    {
      GimpCheckSize  check_size;
      GimpCheckType  check_type;
      guchar         check_light;
      guchar         check_dark;
      GimpRGB        light;
      GimpRGB        dark;

      g_object_get (shell->display->config,
                    "transparency-size", &check_size,
                    "transparency-type", &check_type,
                    NULL);

      gimp_checks_get_shades (check_type, &check_light, &check_dark);
      gimp_rgb_set_uchar (&light, check_light, check_light, check_light);
      gimp_rgb_set_uchar (&dark,  check_dark,  check_dark,  check_dark);

      shell->checkerboard =
        gimp_cairo_checkerboard_create (cr,
                                        1 << (check_size + 2), &light, &dark);
    }

  cairo_translate (cr, - shell->offset_x, - shell->offset_y);
  cairo_set_source (cr, shell->checkerboard);
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
  gint x1, y1, x2, y2;
  gint i, j;
  gint chunk_width;
  gint chunk_height;

  g_return_if_fail (GIMP_IS_DISPLAY_SHELL (shell));
  g_return_if_fail (gimp_display_get_image (shell->display));
  g_return_if_fail (cr != NULL);

  if (shell->rotate_untransform)
    {
      gdouble tx1, ty1;
      gdouble tx2, ty2;
      gint    image_width;
      gint    image_height;

      gimp_display_shell_unrotate_bounds (shell,
                                          x, y, x + w, y + h,
                                          &tx1, &ty1, &tx2, &ty2);

      x1 = floor (tx1 - 0.5);
      y1 = floor (ty1 - 0.5);
      x2 = ceil (tx2 + 0.5);
      y2 = ceil (ty2 + 0.5);

      gimp_display_shell_scale_get_image_size (shell,
                                               &image_width, &image_height);

      x1 = CLAMP (x1, -shell->offset_x, -shell->offset_x + image_width);
      y1 = CLAMP (y1, -shell->offset_y, -shell->offset_y + image_height);
      x2 = CLAMP (x2, -shell->offset_x, -shell->offset_x + image_width);
      y2 = CLAMP (y2, -shell->offset_y, -shell->offset_y + image_height);

      if (!(x2 > x1) || !(y2 > y1))
        return;
    }
  else
    {
      x1 = x;
      y1 = y;
      x2 = x + w;
      y2 = y + h;
    }

  /*  display the image in RENDER_BUF_WIDTH x RENDER_BUF_HEIGHT
   *  sized chunks
   */
  chunk_width  = GIMP_DISPLAY_RENDER_BUF_WIDTH;
  chunk_height = GIMP_DISPLAY_RENDER_BUF_HEIGHT;

  if ((shell->scale_x / shell->scale_y) > 2.0)
    {
      while ((chunk_width / chunk_height) < (shell->scale_x / shell->scale_y))
        chunk_height /= 2;
    }
  else if ((shell->scale_y / shell->scale_x) > 2.0)
    {
      while ((chunk_height / chunk_width) < (shell->scale_y / shell->scale_x))
        chunk_width /= 2;
    }

  for (i = y1; i < y2; i += chunk_height)
    {
      for (j = x1; j < x2; j += chunk_width)
        {
          gint dx, dy;

          dx = MIN (x2 - j, chunk_width);
          dy = MIN (y2 - i, chunk_height);

          gimp_display_shell_render (shell, cr, j, i, dx, dy);
        }
    }
}
