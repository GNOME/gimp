/* The GIMP -- an image manipulation program
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

#include <gtk/gtk.h>

#include "display-types.h"

#include "core/gimpgrid.h"
#include "core/gimpimage.h"
#include "core/gimpimage-guides.h"
#include "core/gimplist.h"

#include "vectors/gimpstroke.h"
#include "vectors/gimpvectors.h"

#include "widgets/gimpwidgets-utils.h"

#include "gimpcanvas.h"
#include "gimpdisplay.h"
#include "gimpdisplayshell.h"
#include "gimpdisplayshell-appearance.h"
#include "gimpdisplayshell-draw.h"
#include "gimpdisplayshell-render.h"
#include "gimpdisplayshell-transform.h"


/*  local function prototypes  */

static GdkGC * gimp_display_shell_grid_gc_new (GtkWidget *widget,
                                               GimpGrid  *grid);


/*  public functions  */

void
gimp_display_shell_draw_guide (GimpDisplayShell *shell,
                               GimpGuide        *guide,
                               gboolean          active)
{
  gint  x1, x2;
  gint  y1, y2;
  gint  x, y;
  gint  w, h;

  g_return_if_fail (GIMP_IS_DISPLAY_SHELL (shell));
  g_return_if_fail (guide != NULL);

  if (guide->position < 0)
    return;

  gimp_display_shell_transform_xy (shell, 0, 0, &x1, &y1, FALSE);
  gimp_display_shell_transform_xy (shell,
                                   shell->gdisp->gimage->width,
                                   shell->gdisp->gimage->height,
                                   &x2, &y2, FALSE);

  gdk_drawable_get_size (shell->canvas->window, &w, &h);

  if (x1 < 0) x1 = 0;
  if (y1 < 0) y1 = 0;
  if (x2 > w) x2 = w;
  if (y2 > h) y2 = h;

  switch (guide->orientation)
    {
    case GIMP_ORIENTATION_HORIZONTAL:
      gimp_display_shell_transform_xy (shell,
                                       0, guide->position, &x, &y, FALSE);
      y1 = y2 = y;
      break;

    case GIMP_ORIENTATION_VERTICAL:
      gimp_display_shell_transform_xy (shell,
                                       guide->position, 0, &x, &y, FALSE);
      x1 = x2 = x;
      break;

    case GIMP_ORIENTATION_UNKNOWN:
      return;
    }

  gimp_canvas_draw_line (GIMP_CANVAS (shell->canvas),
                         (active ?
                          GIMP_CANVAS_STYLE_GUIDE_ACTIVE :
                          GIMP_CANVAS_STYLE_GUIDE_NORMAL), x1, y1, x2, y2);
}

void
gimp_display_shell_draw_guides (GimpDisplayShell *shell)
{
  g_return_if_fail (GIMP_IS_DISPLAY_SHELL (shell));

  if (gimp_display_shell_get_show_guides (shell))
    {
      GList *list;

      for (list = shell->gdisp->gimage->guides;
           list;
           list = g_list_next (list))
	{
	  gimp_display_shell_draw_guide (shell,
                                         (GimpGuide *) list->data,
                                         FALSE);
	}
    }
}

void
gimp_display_shell_draw_grid (GimpDisplayShell *shell)
{
  g_return_if_fail (GIMP_IS_DISPLAY_SHELL (shell));

  if (gimp_display_shell_get_show_grid (shell))
    {
      GimpGrid   *grid;
      GimpCanvas *canvas;
      GdkGC      *gc;
      gint        x1, x2;
      gint        y1, y2;
      gint        x, y;
      gint        x_real, y_real;
      gint        width, height;
      const gint  length = 2;

      grid = GIMP_GRID (shell->gdisp->gimage->grid);

      if (grid == NULL)
        return;

      gimp_display_shell_transform_xy (shell, 0, 0, &x1, &y1, FALSE);
      gimp_display_shell_transform_xy (shell,
                                       shell->gdisp->gimage->width,
                                       shell->gdisp->gimage->height,
                                       &x2, &y2, FALSE);

      width  = shell->gdisp->gimage->width;
      height = shell->gdisp->gimage->height;

      canvas = GIMP_CANVAS (shell->canvas);

      gc = gimp_display_shell_grid_gc_new (shell->canvas, grid);
      gimp_canvas_set_custom_gc (canvas, gc);
      g_object_unref (gc);

      switch (grid->style)
        {
        case GIMP_GRID_DOTS:
          for (x = grid->xoffset; x <= width; x += grid->xspacing)
            {
              for (y = grid->yoffset; y <= height; y += grid->yspacing)
                {
                  gimp_display_shell_transform_xy (shell,
                                                   x, y, &x_real, &y_real,
                                                   FALSE);

                  if (x_real >= x1 && x_real < x2 &&
                      y_real >= y1 && y_real < y2)
                    {
                      gimp_canvas_draw_point (GIMP_CANVAS (shell->canvas),
                                              GIMP_CANVAS_STYLE_CUSTOM,
                                              x_real, y_real);
                    }
                }
            }
          break;

        case GIMP_GRID_INTERSECTIONS:
          for (x = grid->xoffset; x <= width; x += grid->xspacing)
            {
              for (y = grid->yoffset; y <= height; y += grid->yspacing)
                {
                  gimp_display_shell_transform_xy (shell,
                                                   x, y, &x_real, &y_real,
                                                   FALSE);

                  if (x_real >= x1 && x_real < x2)
                    gimp_canvas_draw_line (canvas, GIMP_CANVAS_STYLE_CUSTOM,
                                           x_real,
                                           CLAMP (y_real - length, y1, y2 - 1),
                                           x_real,
                                           CLAMP (y_real + length, y1, y2 - 1));
                  if (y_real >= y1 && y_real < y2)
                    gimp_canvas_draw_line (canvas, GIMP_CANVAS_STYLE_CUSTOM,
                                           CLAMP (x_real - length, x1, x2 - 1),
                                           y_real,
                                           CLAMP (x_real + length, x1, x2 - 1),
                                           y_real);
                }
            }
          break;

        case GIMP_GRID_ON_OFF_DASH:
        case GIMP_GRID_DOUBLE_DASH:
        case GIMP_GRID_SOLID:
          for (x = grid->xoffset; x < width; x += grid->xspacing)
            {
              gimp_display_shell_transform_xy (shell,
                                               x, 0, &x_real, &y_real,
                                               FALSE);

              if (x_real > x1)
                gimp_canvas_draw_line (canvas, GIMP_CANVAS_STYLE_CUSTOM,
                                       x_real, y1, x_real, y2 - 1);
            }

          for (y = grid->yoffset; y < height; y += grid->yspacing)
            {
              gimp_display_shell_transform_xy (shell,
                                               0, y, &x_real, &y_real,
                                               FALSE);

              if (y_real > y1)
                gimp_canvas_draw_line (canvas, GIMP_CANVAS_STYLE_CUSTOM,
                                       x1, y_real, x2 - 1, y_real);
            }
          break;
        }

      gimp_canvas_set_custom_gc (canvas, NULL);
    }
}

void
gimp_display_shell_draw_vector (GimpDisplayShell *shell,
                                GimpVectors      *vectors)
{
  GimpStroke *stroke = NULL;

  g_return_if_fail (GIMP_IS_DISPLAY_SHELL (shell));
  g_return_if_fail (GIMP_IS_VECTORS (vectors));

  while ((stroke = gimp_vectors_stroke_get_next (vectors, stroke)))
    {
      GArray   *coords;
      gboolean  closed;

      coords = gimp_stroke_interpolate (stroke, 1.0, &closed);

      if (coords && coords->len)
        {
          GimpCoords *coord;
          GdkPoint   *gdk_coords;
          gint        i;
          gdouble     sx, sy;

          gdk_coords = g_new (GdkPoint, coords->len);

          for (i = 0; i < coords->len; i++)
            {
              coord = &g_array_index (coords, GimpCoords, i);

              gimp_display_shell_transform_xy_f (shell,
                                                 coord->x, coord->y,
                                                 &sx, &sy,
                                                 FALSE);
              gdk_coords[i].x = ROUND (sx);
              gdk_coords[i].y = ROUND (sy);
            }

          gimp_canvas_draw_lines (GIMP_CANVAS (shell->canvas),
                                  GIMP_CANVAS_STYLE_XOR,
                                  gdk_coords, coords->len);

          g_free (gdk_coords);
        }

      if (coords)
        g_array_free (coords, TRUE);
    }
}

void
gimp_display_shell_draw_vectors (GimpDisplayShell *shell)
{
  g_return_if_fail (GIMP_IS_DISPLAY_SHELL (shell));

  if (TRUE /* gimp_display_shell_get_show_vectors (shell) */)
    {
      GList *list;

      for (list = GIMP_LIST (shell->gdisp->gimage->vectors)->list;
           list;
           list = list->next)
	{
          GimpVectors *vectors = list->data;

          if (gimp_item_get_visible (GIMP_ITEM (vectors)))
            gimp_display_shell_draw_vector (shell, vectors);
	}
    }
}

void
gimp_display_shell_draw_cursor (GimpDisplayShell *shell)
{
  g_return_if_fail (GIMP_IS_DISPLAY_SHELL (shell));

  if (shell->have_cursor)
    gimp_canvas_draw_cursor (GIMP_CANVAS (shell->canvas),
                             shell->cursor_x, shell->cursor_y);
}

void
gimp_display_shell_draw_area (GimpDisplayShell *shell,
                              gint              x,
                              gint              y,
                              gint              w,
                              gint              h)
{
  gint  sx, sy;
  gint  x1, y1;
  gint  x2, y2;
  gint  dx, dy;
  gint  i, j;

  g_return_if_fail (GIMP_IS_DISPLAY_SHELL (shell));

  sx = SCALEX (shell, shell->gdisp->gimage->width);
  sy = SCALEY (shell, shell->gdisp->gimage->height);

  /*  Bounds checks  */
  x1 = CLAMP (x,     0, shell->disp_width);
  y1 = CLAMP (y,     0, shell->disp_height);
  x2 = CLAMP (x + w, 0, shell->disp_width);
  y2 = CLAMP (y + h, 0, shell->disp_height);

  x1 = MAX (x1, shell->disp_xoffset);
  y1 = MAX (y1, shell->disp_yoffset);
  x2 = MIN (x2, shell->disp_xoffset + sx);
  y2 = MIN (y2, shell->disp_yoffset + sy);

  /*  display the image in RENDER_BUF_WIDTH x RENDER_BUF_HEIGHT sized chunks */
  for (i = y1; i < y2; i += GIMP_DISPLAY_SHELL_RENDER_BUF_HEIGHT)
    {
      for (j = x1; j < x2; j += GIMP_DISPLAY_SHELL_RENDER_BUF_WIDTH)
        {
          dx = MIN (x2 - j, GIMP_DISPLAY_SHELL_RENDER_BUF_WIDTH);
          dy = MIN (y2 - i, GIMP_DISPLAY_SHELL_RENDER_BUF_HEIGHT);

          gimp_display_shell_render (shell,
                                     j - shell->disp_xoffset,
                                     i - shell->disp_yoffset,
                                     dx, dy);

#ifdef STRESS_TEST
          /* Invalidate the projection just after we render it! */
          gimp_image_invalidate_without_render (shell->gdisp->gimage,
                                                j - shell->disp_xoffset,
                                                i - shell->disp_yoffset,
                                                dx, dy,
                                                0, 0, 0, 0);
#endif
        }
    }
}


/*  private functions  */

static GdkGC *
gimp_display_shell_grid_gc_new (GtkWidget *widget,
                                GimpGrid  *grid)
{
  GdkGC       *gc;
  GdkGCValues  values;
  GdkColor     fg, bg;

  switch (grid->style)
    {
    case GIMP_GRID_ON_OFF_DASH:
      values.line_style = GDK_LINE_ON_OFF_DASH;
      break;

    case GIMP_GRID_DOUBLE_DASH:
      values.line_style = GDK_LINE_DOUBLE_DASH;
      break;

    case GIMP_GRID_DOTS:
    case GIMP_GRID_INTERSECTIONS:
    case GIMP_GRID_SOLID:
      values.line_style = GDK_LINE_SOLID;
      break;
    }

  values.join_style = GDK_JOIN_MITER;

  gc = gdk_gc_new_with_values (widget->window,
                               &values, GDK_GC_LINE_STYLE | GDK_GC_JOIN_STYLE);

  gimp_rgb_get_gdk_color (&grid->fgcolor, &fg);
  gimp_rgb_get_gdk_color (&grid->bgcolor, &bg);

  gdk_gc_set_rgb_fg_color (gc, &fg);
  gdk_gc_set_rgb_bg_color (gc, &bg);

  return gc;
}
