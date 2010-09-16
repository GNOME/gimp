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
#include "libgimpmath/gimpmath.h"
#include "libgimpwidgets/gimpwidgets.h"

#include "display-types.h"

#include "base/boundary.h"
#include "base/tile-manager.h"

#include "core/gimpcontext.h"
#include "core/gimpdrawable.h"
#include "core/gimpgrid.h"
#include "core/gimpguide.h"
#include "core/gimpimage.h"
#include "core/gimpimage-grid.h"
#include "core/gimpimage-guides.h"
#include "core/gimpimage-sample-points.h"
#include "core/gimpprojection.h"
#include "core/gimpsamplepoint.h"

#include "vectors/gimpstroke.h"
#include "vectors/gimpvectors.h"

#include "widgets/gimpcairo.h"
#include "widgets/gimpwidgets-utils.h"

#include "gimpcanvas.h"
#include "gimpdisplay.h"
#include "gimpdisplayshell.h"
#include "gimpdisplayshell-appearance.h"
#include "gimpdisplayshell-cursor.h"
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
  gint            level;
  gint            level_width;
  gint            level_height;

  g_return_if_fail (GIMP_IS_DISPLAY_SHELL (shell));

  image = gimp_display_get_image (shell->display);

  g_return_if_fail (GIMP_IS_IMAGE (image));

  proj = gimp_image_get_projection (image);

  gimp_display_shell_calculate_scale_x_and_y (shell, scale, &scale_x, &scale_y);

  level = gimp_projection_get_level (proj, scale_x, scale_y);

  tiles = gimp_projection_get_tiles_at_level (proj, level, NULL);

  level_width  = tile_manager_width (tiles);
  level_height = tile_manager_height (tiles);

  if (w) *w = PROJ_ROUND (level_width  * (scale_x * (1 << level)));
  if (h) *h = PROJ_ROUND (level_height * (scale_y * (1 << level)));
}

void
gimp_display_shell_draw_guide (GimpDisplayShell   *shell,
                               cairo_t            *cr,
                               GimpGuide          *guide,
                               gboolean            active)
{
  gint    position;
  gdouble dx1, dy1, dx2, dy2;
  gint    x1, y1, x2, y2;
  gint    x, y;

  g_return_if_fail (GIMP_IS_DISPLAY_SHELL (shell));
  g_return_if_fail (cr != NULL);
  g_return_if_fail (GIMP_IS_GUIDE (guide));

  position = gimp_guide_get_position (guide);
  if (position < 0)
    return;

  cairo_clip_extents (cr, &dx1, &dy1, &dx2, &dy2);

  x1 = floor (dx1);
  y1 = floor (dy1);
  x2 = ceil (dx2);
  y2 = ceil (dy2);

  gimp_display_shell_set_guide_style (shell, cr, active);

  switch (gimp_guide_get_orientation (guide))
    {
    case GIMP_ORIENTATION_HORIZONTAL:
      gimp_display_shell_transform_xy (shell, 0, position, &x, &y, FALSE);
      if (y >= y1 && y < y2)
        {
          cairo_move_to (cr, x1, y + 0.5);
          cairo_line_to (cr, x2, y + 0.5);
        }
      break;

    case GIMP_ORIENTATION_VERTICAL:
      gimp_display_shell_transform_xy (shell, position, 0, &x, &y, FALSE);
      if (x >= x1 && x < x2)
        {
          cairo_move_to (cr, x + 0.5, y1);
          cairo_line_to (cr, x + 0.5, y2);
        }
      break;

    case GIMP_ORIENTATION_UNKNOWN:
      return;
    }

  cairo_stroke (cr);
}

void
gimp_display_shell_draw_guides (GimpDisplayShell *shell,
                                cairo_t          *cr)
{
  GimpImage *image;

  g_return_if_fail (GIMP_IS_DISPLAY_SHELL (shell));
  g_return_if_fail (cr != NULL);

  image = gimp_display_get_image (shell->display);

  if (image && gimp_display_shell_get_show_guides (shell))
    {
      GList *list;

      for (list = gimp_image_get_guides (image);
           list;
           list = g_list_next (list))
        {
          gimp_display_shell_draw_guide (shell, cr, list->data, FALSE);
        }
    }
}

void
gimp_display_shell_draw_grid (GimpDisplayShell *shell,
                              cairo_t          *cr)
{
  GimpImage *image;

  g_return_if_fail (GIMP_IS_DISPLAY_SHELL (shell));
  g_return_if_fail (cr != NULL);

  image = gimp_display_get_image (shell->display);

  if (image && gimp_display_shell_get_show_grid (shell))
    {
      GimpGrid *grid;
      gdouble   x, y;
      gdouble   dx1, dy1, dx2, dy2;
      gint      x0, x1, x2, x3;
      gint      y0, y1, y2, y3;
      gint      x_real, y_real;
      gdouble   x_offset, y_offset;
      gint      width, height;

#define CROSSHAIR 2

      grid = gimp_image_get_grid (image);
      if (! grid)
        return;

      g_return_if_fail (grid->xspacing > 0 && grid->yspacing > 0);

      cairo_clip_extents (cr, &dx1, &dy1, &dx2, &dy2);

      x1 = floor (dx1);
      y1 = floor (dy1);
      x2 = ceil (dx2);
      y2 = ceil (dy2);

      width  = gimp_image_get_width  (image);
      height = gimp_image_get_height (image);

      x_offset = grid->xoffset;
      while (x_offset > 0)
        x_offset -= grid->xspacing;

      y_offset = grid->yoffset;
      while (y_offset > 0)
        y_offset -= grid->yspacing;

      gimp_display_shell_set_grid_style (shell, cr, grid);

      switch (grid->style)
        {
        case GIMP_GRID_DOTS:
          for (x = x_offset; x <= width; x += grid->xspacing)
            {
              if (x < 0)
                continue;

              gimp_display_shell_transform_xy (shell,
                                               x, 0, &x_real, &y_real,
                                               FALSE);

              if (x_real < x1 || x_real >= x2)
                continue;

              for (y = y_offset; y <= height; y += grid->yspacing)
                {
                  if (y < 0)
                    continue;

                  gimp_display_shell_transform_xy (shell,
                                                   x, y, &x_real, &y_real,
                                                   FALSE);

                  if (y_real >= y1 && y_real < y2)
                    {
                      cairo_move_to (cr, x_real,     y_real + 0.5);
                      cairo_line_to (cr, x_real + 1, y_real + 0.5);
                    }
                }
            }
          break;

        case GIMP_GRID_INTERSECTIONS:
          for (x = x_offset; x <= width; x += grid->xspacing)
            {
              if (x < 0)
                continue;

              gimp_display_shell_transform_xy (shell,
                                               x, 0, &x_real, &y_real,
                                               FALSE);

              if (x_real + CROSSHAIR < x1 || x_real - CROSSHAIR >= x2)
                continue;

              for (y = y_offset; y <= height; y += grid->yspacing)
                {
                  if (y < 0)
                    continue;

                  gimp_display_shell_transform_xy (shell,
                                                   x, y, &x_real, &y_real,
                                                   FALSE);

                  if (y_real + CROSSHAIR < y1 || y_real - CROSSHAIR >= y2)
                    continue;

                  if (x_real >= x1 && x_real < x2)
                    {
                      cairo_move_to (cr,
                                     x_real + 0.5,
                                     CLAMP (y_real - CROSSHAIR,
                                            y1, y2 - 1));
                      cairo_line_to (cr,
                                     x_real + 0.5,
                                     CLAMP (y_real + CROSSHAIR,
                                            y1, y2 - 1) + 1);
                    }

                  if (y_real >= y1 && y_real < y2)
                    {
                      cairo_move_to (cr,
                                     CLAMP (x_real - CROSSHAIR,
                                            x1, x2 - 1),
                                     y_real + 0.5);
                      cairo_line_to (cr,
                                     CLAMP (x_real + CROSSHAIR,
                                            x1, x2 - 1) + 1,
                                     y_real + 0.5);
                    }
                }
            }
          break;

        case GIMP_GRID_ON_OFF_DASH:
        case GIMP_GRID_DOUBLE_DASH:
        case GIMP_GRID_SOLID:
          gimp_display_shell_transform_xy (shell,
                                           0, 0, &x0, &y0,
                                           FALSE);
          gimp_display_shell_transform_xy (shell,
                                           width, height, &x3, &y3,
                                           FALSE);

          for (x = x_offset; x < width; x += grid->xspacing)
            {
              if (x < 0)
                continue;

              gimp_display_shell_transform_xy (shell,
                                               x, 0, &x_real, &y_real,
                                               FALSE);

              if (x_real >= x1 && x_real < x2)
                {
                  cairo_move_to (cr, x_real + 0.5, y0);
                  cairo_line_to (cr, x_real + 0.5, y3 + 1);
                }
            }

          for (y = y_offset; y < height; y += grid->yspacing)
            {
              if (y < 0)
                continue;

              gimp_display_shell_transform_xy (shell,
                                               0, y, &x_real, &y_real,
                                               FALSE);

              if (y_real >= y1 && y_real < y2)
                {
                  cairo_move_to (cr, x0,     y_real + 0.5);
                  cairo_line_to (cr, x3 + 1, y_real + 0.5);
                }
            }
          break;
        }

      cairo_stroke (cr);
    }
}

void
gimp_display_shell_draw_pen (GimpDisplayShell  *shell,
                             cairo_t           *cr,
                             const GimpVector2 *points,
                             gint               n_points,
                             GimpContext       *context,
                             GimpActiveColor    color,
                             gint               width)
{
  gint i;
  gint x, y;

  g_return_if_fail (GIMP_IS_DISPLAY_SHELL (shell));
  g_return_if_fail (cr != NULL);
  g_return_if_fail (GIMP_IS_CONTEXT (context));
  g_return_if_fail (n_points == 0 || points != NULL);

  if (n_points == 0)
    return;

  gimp_display_shell_set_pen_style (shell, cr, context, color, width);
  cairo_translate (cr, 0.5, 0.5);

  gimp_display_shell_transform_xy (shell,
                                   points[0].x, points[0].y,
                                   &x, &y, FALSE);

  cairo_move_to (cr, x, y);

  for (i = 1; i < n_points; i++)
    {
      gimp_display_shell_transform_xy (shell,
                                       points[i].x, points[i].y,
                                       &x, &y, FALSE);

      cairo_line_to (cr, x, y);
    }

  if (i == 1)
    cairo_line_to (cr, x, y);

  cairo_stroke (cr);
}

void
gimp_display_shell_draw_sample_point (GimpDisplayShell   *shell,
                                      cairo_t            *cr,
                                      GimpSamplePoint    *sample_point,
                                      gboolean            active)
{
  GimpImage   *image;
  gdouble      dx1, dy1, dx2, dy2;
  gint         x1, x2, y1, y2;
  gint         sx1, sx2, sy1, sy2;
  gdouble      x, y;
  PangoLayout *layout;

  g_return_if_fail (GIMP_IS_DISPLAY_SHELL (shell));
  g_return_if_fail (cr != NULL);
  g_return_if_fail (sample_point != NULL);

  if (sample_point->x < 0)
    return;

  image = gimp_display_get_image (shell->display);

  cairo_clip_extents (cr, &dx1, &dy1, &dx2, &dy2);

  x1 = floor (dx1);
  y1 = floor (dy1);
  x2 = ceil (dx2);
  y2 = ceil (dy2);

  gimp_display_shell_transform_xy_f (shell,
                                     sample_point->x + 0.5,
                                     sample_point->y + 0.5,
                                     &x, &y, FALSE);

  sx1 = floor (x - GIMP_SAMPLE_POINT_DRAW_SIZE);
  sx2 = ceil  (x + GIMP_SAMPLE_POINT_DRAW_SIZE);
  sy1 = floor (y - GIMP_SAMPLE_POINT_DRAW_SIZE);
  sy2 = ceil  (y + GIMP_SAMPLE_POINT_DRAW_SIZE);

  if (sx1 > x2 ||
      sx2 < x1 ||
      sy1 > y2 ||
      sy2 < y1)
    return;

  gimp_display_shell_set_sample_point_style (shell, cr, active);

#define HALF_SIZE (GIMP_SAMPLE_POINT_DRAW_SIZE / 2)

  cairo_move_to (cr, x + 0.5, sy1);
  cairo_line_to (cr, x + 0.5, sy1 + HALF_SIZE);

  cairo_move_to (cr, x + 0.5, sy2);
  cairo_line_to (cr, x + 0.5, sy2 - HALF_SIZE);

  cairo_move_to (cr, sx1,             y + 0.5);
  cairo_line_to (cr, sx1 + HALF_SIZE, y + 0.5);

  cairo_move_to (cr, sx2,             y + 0.5);
  cairo_line_to (cr, sx2 - HALF_SIZE, y + 0.5);

  cairo_arc_negative (cr, x + 0.5, y + 0.5, HALF_SIZE, 0.0, 0.5 * G_PI);

  cairo_stroke (cr);

  layout =
    gimp_canvas_get_layout (GIMP_CANVAS (shell->canvas),
                            "%d",
                            g_list_index (gimp_image_get_sample_points (image),
                                          sample_point) + 1);

  cairo_move_to (cr, x + 2, y + 2);
  pango_cairo_show_layout (cr, layout);

  cairo_fill (cr);
}

void
gimp_display_shell_draw_sample_points (GimpDisplayShell *shell,
                                       cairo_t          *cr)
{
  GimpImage *image;

  g_return_if_fail (GIMP_IS_DISPLAY_SHELL (shell));
  g_return_if_fail (cr != NULL);

  image = gimp_display_get_image (shell->display);

  if (image && gimp_display_shell_get_show_sample_points (shell))
    {
      GList *list;

      for (list = gimp_image_get_sample_points (image);
           list;
           list = g_list_next (list))
        {
          gimp_display_shell_draw_sample_point (shell, cr, list->data, FALSE);
        }
    }
}

void
gimp_display_shell_draw_layer_boundary (GimpDisplayShell *shell,
                                        cairo_t          *cr,
                                        GimpDrawable     *drawable,
                                        GdkSegment       *segs,
                                        gint              n_segs)
{
  g_return_if_fail (GIMP_IS_DISPLAY_SHELL (shell));
  g_return_if_fail (cr != NULL);
  g_return_if_fail (GIMP_IS_DRAWABLE (drawable));
  g_return_if_fail (segs != NULL && n_segs == 4);

  gimp_display_shell_set_layer_style (shell, cr, drawable);

  gimp_cairo_add_segments (cr, segs, n_segs);
  cairo_stroke (cr);
}

void
gimp_display_shell_draw_selection_out (GimpDisplayShell *shell,
                                       cairo_t          *cr,
                                       GdkSegment       *segs,
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

static void
gimp_display_shell_draw_one_vectors (GimpDisplayShell *shell,
                                     cairo_t          *cr,
                                     GimpVectors      *vectors,
                                     gboolean          active,
                                     gdouble           width)
{
  GimpStroke *stroke = NULL;

  while ((stroke = gimp_vectors_stroke_get_next (vectors, stroke)))
    {
      const GimpBezierDesc *desc = gimp_vectors_get_bezier (vectors);

      if (desc)
        {
          cairo_append_path (cr, (cairo_path_t *) desc);

          /* FIXME: need better styles */
          if (active)
            cairo_set_source_rgb (cr, 0.3, 0.7, 1.0);
          else
            cairo_set_source_rgb (cr, 1.0, 1.0, 1.0);

          cairo_set_line_width (cr, 1.6 * width);
          cairo_stroke_preserve (cr);

          cairo_set_source_rgb (cr, 0.0, 0.0, 0.0);
          cairo_set_line_width (cr, width);
          cairo_stroke (cr);
        }
    }
}

void
gimp_display_shell_draw_vectors (GimpDisplayShell *shell,
                                 cairo_t          *cr)
{
  GimpImage *image;

  g_return_if_fail (GIMP_IS_DISPLAY_SHELL (shell));

  image = gimp_display_get_image (shell->display);

  if (image && TRUE /* gimp_display_shell_get_show_vectors (shell) */)
    {
      GList       *all_vectors = gimp_image_get_vectors_list (image);
      GimpVectors *active      = gimp_image_get_active_vectors (image);
      const GList *list;
      gdouble      xscale;
      gdouble      yscale;
      gdouble      width;

      if (! all_vectors)
        return;

      cairo_translate (cr, - shell->offset_x, - shell->offset_y);
      cairo_scale (cr, shell->scale_x, shell->scale_y);

      /* determine a reasonable line width */
      xscale = yscale = 2.0;
      cairo_device_to_user_distance (cr, &xscale, &yscale);
      width = MAX (xscale, yscale);
      width = MIN (width, 2.0);

      for (list = all_vectors; list; list = list->next)
        {
          GimpVectors *vectors = list->data;

          if (! gimp_item_get_visible (GIMP_ITEM (vectors)))
            continue;

          gimp_display_shell_draw_one_vectors (shell, cr,
                                               vectors,
                                               (vectors == active), width);
        }

      g_list_free (all_vectors);
    }
}

void
gimp_display_shell_draw_cursor (GimpDisplayShell *shell,
                                cairo_t          *cr)
{
  g_return_if_fail (GIMP_IS_DISPLAY_SHELL (shell));
  g_return_if_fail (cr != NULL);

  if (shell->have_cursor)
    {
      gimp_display_shell_set_cursor_style (shell, cr);
      cairo_translate (cr, 0.5, 0.5);

      cairo_set_source_rgb (cr, 1.0, 1.0, 1.0);

      cairo_move_to (cr,
                     shell->cursor_x - GIMP_CURSOR_SIZE, shell->cursor_y - 1);
      cairo_line_to (cr,
                     shell->cursor_x + GIMP_CURSOR_SIZE, shell->cursor_y - 1);

      cairo_move_to (cr,
                     shell->cursor_x - GIMP_CURSOR_SIZE, shell->cursor_y + 1);
      cairo_line_to (cr,
                     shell->cursor_x + GIMP_CURSOR_SIZE, shell->cursor_y + 1);

      cairo_move_to (cr,
                     shell->cursor_x - 1, shell->cursor_y - GIMP_CURSOR_SIZE);
      cairo_line_to (cr,
                     shell->cursor_x - 1, shell->cursor_y + GIMP_CURSOR_SIZE);

      cairo_move_to (cr,
                     shell->cursor_x + 1, shell->cursor_y - GIMP_CURSOR_SIZE);
      cairo_line_to (cr,
                     shell->cursor_x + 1, shell->cursor_y + GIMP_CURSOR_SIZE);

      cairo_stroke (cr);

      cairo_set_source_rgb (cr, 0.0, 0.0, 0.0);

      cairo_move_to (cr, shell->cursor_x - GIMP_CURSOR_SIZE, shell->cursor_y);
      cairo_line_to (cr, shell->cursor_x + GIMP_CURSOR_SIZE, shell->cursor_y);

      cairo_move_to (cr, shell->cursor_x, shell->cursor_y - GIMP_CURSOR_SIZE);
      cairo_line_to (cr, shell->cursor_x, shell->cursor_y + GIMP_CURSOR_SIZE);

      cairo_stroke (cr);
    }
}

void
gimp_display_shell_draw_area (GimpDisplayShell *shell,
                              cairo_t          *cr,
                              gint              x,
                              gint              y,
                              gint              w,
                              gint              h)
{
  GdkRectangle  rect;
  gint          x2, y2;
  gint          i, j;

  g_return_if_fail (GIMP_IS_DISPLAY_SHELL (shell));
  g_return_if_fail (gimp_display_get_image (shell->display));
  g_return_if_fail (cr != NULL);

  x2 = x + w;
  y2 = y + h;

  if (shell->highlight)
    {
      rect.x      = ceil  (shell->scale_x * shell->highlight->x);
      rect.y      = ceil  (shell->scale_y * shell->highlight->y);
      rect.width  = floor (shell->scale_x * shell->highlight->width);
      rect.height = floor (shell->scale_y * shell->highlight->height);
    }

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
                                     dx, dy,
                                     shell->highlight ? &rect : NULL);
        }
    }
}
