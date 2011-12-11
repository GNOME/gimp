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

#include "base/tile-manager.h"

#include "core/gimpdrawable.h"
#include "core/gimpimage.h"
#include "core/gimpprojection.h"

#include "widgets/gimpcairo.h"

#include "gimpcanvas.h"
#include "gimpcanvaspath.h"
#include "gimpdisplay.h"
#include "gimpdisplayshell.h"
#include "gimpdisplayshell-draw.h"
#include "gimpdisplayshell-render.h"
#include "gimpdisplayshell-scale.h"
#include "gimpdisplayshell-scroll.h"
#include "gimpdisplayshell-style.h"
#include "gimpdisplayshell-transform.h"


/*  public functions  */

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
gimp_display_shell_draw_get_scaled_image_size (GimpDisplayShell *shell,
                                               gint             *w,
                                               gint             *h)
{
  g_return_if_fail (GIMP_IS_DISPLAY_SHELL (shell));

  gimp_display_shell_draw_get_scaled_image_size_for_scale (shell,
                                                           gimp_zoom_model_get_factor (shell->zoom),
                                                           w,
                                                           h);
}

/**
 * gimp_display_shell_draw_get_scaled_image_size_for_scale:
 * @shell:
 * @scale:
 * @w:
 * @h:
 *
 **/
void
gimp_display_shell_draw_get_scaled_image_size_for_scale (GimpDisplayShell *shell,
                                                         gdouble           scale,
                                                         gint             *w,
                                                         gint             *h)
{
  GimpImage      *image;
  GimpProjection *proj;
  TileManager    *tiles;
  gdouble         scale_x;
  gdouble         scale_y;

  g_return_if_fail (GIMP_IS_DISPLAY_SHELL (shell));

  image = gimp_display_get_image (shell->display);

  g_return_if_fail (GIMP_IS_IMAGE (image));

  proj = gimp_image_get_projection (image);

  gimp_display_shell_calculate_scale_x_and_y (shell, scale, &scale_x, &scale_y);

  tiles = gimp_projection_get_tiles_at_level (proj, 0, NULL);

  if (w) *w = scale_x * tile_manager_width (tiles);
  if (h) *h = scale_y * tile_manager_height (tiles);
}

void
gimp_display_shell_draw_selection_out (GimpDisplayShell *shell,
                                       cairo_t          *cr,
                                       GimpSegment      *segs,
                                       gint              n_segs)
{
  g_return_if_fail (GIMP_IS_DISPLAY_SHELL (shell));
  g_return_if_fail (cr != NULL);
  g_return_if_fail (segs != NULL && n_segs > 0);

  gimp_display_shell_set_selection_out_style (shell, cr);

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

  gimp_display_shell_set_selection_in_style (shell, cr, index);

  cairo_mask (cr, mask);
}

void
gimp_display_shell_draw_image (GimpDisplayShell *shell,
                               cairo_t          *cr,
                               gint              x,
                               gint              y,
                               gint              w,
                               gint              h)
{
  gint x2, y2;
  gint i, j;

  g_return_if_fail (GIMP_IS_DISPLAY_SHELL (shell));
  g_return_if_fail (gimp_display_get_image (shell->display));
  g_return_if_fail (cr != NULL);

  x2 = x + w;
  y2 = y + h;

  /*  display the image in RENDER_BUF_WIDTH x RENDER_BUF_HEIGHT
   *  sized chunks
   */
  for (i = y; i < y2; i += GIMP_DISPLAY_RENDER_BUF_HEIGHT)
    {
      for (j = x; j < x2; j += GIMP_DISPLAY_RENDER_BUF_WIDTH)
        {
          gint disp_xoffset, disp_yoffset;
          gint dx, dy;

          dx = MIN (x2 - j, GIMP_DISPLAY_RENDER_BUF_WIDTH);
          dy = MIN (y2 - i, GIMP_DISPLAY_RENDER_BUF_HEIGHT);

          gimp_display_shell_scroll_get_disp_offset (shell,
                                                     &disp_xoffset,
                                                     &disp_yoffset);

          gimp_display_shell_render (shell, cr,
                                     j - disp_xoffset,
                                     i - disp_yoffset,
                                     dx, dy);
        }
    }
}

static cairo_pattern_t *
gimp_display_shell_create_checkerboard (GimpDisplayShell *shell,
                                        cairo_t          *cr)
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

  return gimp_cairo_checkerboard_create (cr,
                                         1 << (check_size + 2), &light, &dark);
}

void
gimp_display_shell_draw_checkerboard (GimpDisplayShell *shell,
                                      cairo_t          *cr,
                                      gint              x,
                                      gint              y,
                                      gint              w,
                                      gint              h)
{
  g_return_if_fail (GIMP_IS_DISPLAY_SHELL (shell));
  g_return_if_fail (cr != NULL);

  if (G_UNLIKELY (! shell->checkerboard))
    shell->checkerboard = gimp_display_shell_create_checkerboard (shell, cr);

  cairo_rectangle (cr, x, y, w, h);
  cairo_clip (cr);

  cairo_translate (cr, - shell->offset_x, - shell->offset_y);
  cairo_set_source (cr, shell->checkerboard);
  cairo_paint (cr);
}
