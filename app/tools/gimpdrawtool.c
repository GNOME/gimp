/* GIMP - The GNU Image Manipulation Program
 * Copyright (C) 1995-2001 Spencer Kimball, Peter Mattis, and others
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

#include <string.h>

#include <gegl.h>
#include <gtk/gtk.h>

#include "libgimpmath/gimpmath.h"

#include "tools-types.h"

#include "base/boundary.h"

#include "core/gimpimage.h"

#include "vectors/gimpanchor.h"
#include "vectors/gimpstroke.h"
#include "vectors/gimpvectors.h"

#include "display/gimpcanvas.h"
#include "display/gimpdisplay.h"
#include "display/gimpdisplayshell.h"
#include "display/gimpdisplayshell-transform.h"

#include "gimpdrawtool.h"


#define DRAW_TIMEOUT 35 /* ~29 FPS */
#define USE_TIMEOUT  1


static gboolean      gimp_draw_tool_has_display  (GimpTool       *tool,
                                                  GimpDisplay    *display);
static GimpDisplay * gimp_draw_tool_has_image    (GimpTool       *tool,
                                                  GimpImage      *image);
static void          gimp_draw_tool_control      (GimpTool       *tool,
                                                  GimpToolAction  action,
                                                  GimpDisplay    *display);

static void          gimp_draw_tool_draw         (GimpDrawTool   *draw_tool);
static void          gimp_draw_tool_real_draw    (GimpDrawTool   *draw_tool);

static inline void   gimp_draw_tool_shift_to_north_west
                                                 (gdouble         x,
                                                  gdouble         y,
                                                  gint            handle_width,
                                                  gint            handle_height,
                                                  GtkAnchorType   anchor,
                                                  gdouble        *shifted_x,
                                                  gdouble        *shifted_y);
static inline void   gimp_draw_tool_shift_to_center
                                                 (gdouble         x,
                                                  gdouble         y,
                                                  gint            handle_width,
                                                  gint            handle_height,
                                                  GtkAnchorType   anchor,
                                                  gdouble        *shifted_x,
                                                  gdouble        *shifted_y);


G_DEFINE_TYPE (GimpDrawTool, gimp_draw_tool, GIMP_TYPE_TOOL)

#define parent_class gimp_draw_tool_parent_class


static void
gimp_draw_tool_class_init (GimpDrawToolClass *klass)
{
  GimpToolClass *tool_class = GIMP_TOOL_CLASS (klass);

  tool_class->has_display = gimp_draw_tool_has_display;
  tool_class->has_image   = gimp_draw_tool_has_image;
  tool_class->control     = gimp_draw_tool_control;

  klass->draw             = gimp_draw_tool_real_draw;
}

static void
gimp_draw_tool_init (GimpDrawTool *draw_tool)
{
  draw_tool->display      = NULL;

  draw_tool->paused_count = 0;
  draw_tool->is_drawn     = FALSE;
}

static gboolean
gimp_draw_tool_has_display (GimpTool    *tool,
                            GimpDisplay *display)
{
  GimpDrawTool *draw_tool = GIMP_DRAW_TOOL (tool);

  return (display == draw_tool->display ||
          GIMP_TOOL_CLASS (parent_class)->has_display (tool, display));
}

static GimpDisplay *
gimp_draw_tool_has_image (GimpTool  *tool,
                          GimpImage *image)
{
  GimpDrawTool *draw_tool = GIMP_DRAW_TOOL (tool);
  GimpDisplay  *display;

  display = GIMP_TOOL_CLASS (parent_class)->has_image (tool, image);

  if (! display && draw_tool->display)
    {
      if (image && gimp_display_get_image (draw_tool->display) == image)
        display = draw_tool->display;

      /*  NULL image means any display  */
      if (! image)
        display = draw_tool->display;
    }

  return display;
}

static void
gimp_draw_tool_control (GimpTool       *tool,
                        GimpToolAction  action,
                        GimpDisplay    *display)
{
  GimpDrawTool *draw_tool = GIMP_DRAW_TOOL (tool);

  switch (action)
    {
    case GIMP_TOOL_ACTION_PAUSE:
      gimp_draw_tool_pause (draw_tool);
      break;

    case GIMP_TOOL_ACTION_RESUME:
      gimp_draw_tool_resume (draw_tool);
      break;

    case GIMP_TOOL_ACTION_HALT:
      gimp_draw_tool_stop (draw_tool);
      break;
    }

  GIMP_TOOL_CLASS (parent_class)->control (tool, action, display);
}

static gboolean
gimp_draw_tool_draw_timeout (GimpDrawTool *draw_tool)
{
  draw_tool->draw_timeout = 0;

  gimp_draw_tool_draw (draw_tool);

  return FALSE;
}

static void
gimp_draw_tool_draw (GimpDrawTool *draw_tool)
{
  if (! draw_tool->draw_timeout    &&
      draw_tool->paused_count == 0 &&
      draw_tool->display)
    {
      GIMP_DRAW_TOOL_GET_CLASS (draw_tool)->draw (draw_tool);

      draw_tool->is_drawn = ! draw_tool->is_drawn;
    }
}

static void
gimp_draw_tool_real_draw (GimpDrawTool *draw_tool)
{
  /* the default implementation does nothing */
}

void
gimp_draw_tool_start (GimpDrawTool *draw_tool,
                      GimpDisplay  *display)
{
  g_return_if_fail (GIMP_IS_DRAW_TOOL (draw_tool));
  g_return_if_fail (GIMP_IS_DISPLAY (display));

  gimp_draw_tool_stop (draw_tool);

  draw_tool->display = display;

  gimp_draw_tool_draw (draw_tool);
}

void
gimp_draw_tool_stop (GimpDrawTool *draw_tool)
{
  g_return_if_fail (GIMP_IS_DRAW_TOOL (draw_tool));

  gimp_draw_tool_draw (draw_tool);

  if (draw_tool->draw_timeout)
    {
      g_source_remove (draw_tool->draw_timeout);
      draw_tool->draw_timeout = 0;
    }

  draw_tool->display = NULL;
}

gboolean
gimp_draw_tool_is_active (GimpDrawTool *draw_tool)
{
  g_return_val_if_fail (GIMP_IS_DRAW_TOOL (draw_tool), FALSE);

  return draw_tool->display != NULL;
}

void
gimp_draw_tool_pause (GimpDrawTool *draw_tool)
{
  g_return_if_fail (GIMP_IS_DRAW_TOOL (draw_tool));

  gimp_draw_tool_draw (draw_tool);

  draw_tool->paused_count++;

  if (draw_tool->draw_timeout)
    {
      g_source_remove (draw_tool->draw_timeout);
      draw_tool->draw_timeout = 0;
    }
}

void
gimp_draw_tool_resume (GimpDrawTool *draw_tool)
{
  g_return_if_fail (GIMP_IS_DRAW_TOOL (draw_tool));

  if (draw_tool->paused_count > 0)
    {
      draw_tool->paused_count--;

#ifdef USE_TIMEOUT
      if (! draw_tool->draw_timeout)
        draw_tool->draw_timeout =
          gdk_threads_add_timeout_full (G_PRIORITY_HIGH,
                                        DRAW_TIMEOUT,
                                        (GSourceFunc) gimp_draw_tool_draw_timeout,
                                        draw_tool, NULL);
#else
  gimp_draw_tool_draw (draw_tool);
#endif
    }
  else
    {
      g_warning ("%s: called with draw_tool->paused_count == 0", G_STRFUNC);
    }
}

gboolean
gimp_draw_tool_is_drawn (GimpDrawTool *draw_tool)
{
  g_return_val_if_fail (GIMP_IS_DRAW_TOOL (draw_tool), FALSE);

  return draw_tool->is_drawn;
}

/**
 * gimp_draw_tool_calc_distance:
 * @draw_tool: a #GimpDrawTool
 * @display:   a #GimpDisplay
 * @x1:        start point X in image coordinates
 * @y1:        start point Y in image coordinates
 * @x2:        end point X in image coordinates
 * @y2:        end point Y in image coordinates
 *
 * If you just need to compare distances, consider to use
 * gimp_draw_tool_calc_distance_square() instead.
 *
 * Returns: the distance between the given points in display coordinates
 **/
gdouble
gimp_draw_tool_calc_distance (GimpDrawTool *draw_tool,
                              GimpDisplay  *display,
                              gdouble       x1,
                              gdouble       y1,
                              gdouble       x2,
                              gdouble       y2)
{
  return sqrt (gimp_draw_tool_calc_distance_square (draw_tool, display,
                                                    x1, y1, x2, y2));
}

/**
 * gimp_draw_tool_calc_distance_square:
 * @draw_tool: a #GimpDrawTool
 * @display:   a #GimpDisplay
 * @x1:        start point X in image coordinates
 * @y1:        start point Y in image coordinates
 * @x2:        end point X in image coordinates
 * @y2:        end point Y in image coordinates
 *
 * This function is more effective than gimp_draw_tool_calc_distance()
 * as it doesn't perform a sqrt(). Use this if you just need to compare
 * distances.
 *
 * Returns: the square of the distance between the given points in
            display coordinates
 **/
gdouble
gimp_draw_tool_calc_distance_square (GimpDrawTool *draw_tool,
                                     GimpDisplay  *display,
                                     gdouble       x1,
                                     gdouble       y1,
                                     gdouble       x2,
                                     gdouble       y2)
{
  GimpDisplayShell *shell;
  gdouble           tx1, ty1;
  gdouble           tx2, ty2;

  g_return_val_if_fail (GIMP_IS_DRAW_TOOL (draw_tool), 0.0);
  g_return_val_if_fail (GIMP_IS_DISPLAY (display), 0.0);

  shell = gimp_display_get_shell (display);

  gimp_display_shell_transform_xy_f (shell, x1, y1, &tx1, &ty1, FALSE);
  gimp_display_shell_transform_xy_f (shell, x2, y2, &tx2, &ty2, FALSE);

  return SQR (tx2 - tx1) + SQR (ty2 - ty1);
}

/**
 * gimp_draw_tool_in_radius:
 * @draw_tool: a #GimpDrawTool
 * @display:   a #GimpDisplay
 * @x1:        start point X in image coordinates
 * @y1:        start point Y in image coordinates
 * @x2:        end point X in image coordinates
 * @y2:        end point Y in image coordinates
 * @radius:    distance in screen coordinates, not image coordinates
 *
 * The points are in image space coordinates.
 *
 * Returns: %TRUE if the points are within radius of each other,
 *          %FALSE otherwise
 **/
gboolean
gimp_draw_tool_in_radius (GimpDrawTool *draw_tool,
                          GimpDisplay  *display,
                          gdouble       x1,
                          gdouble       y1,
                          gdouble       x2,
                          gdouble       y2,
                          gint          radius)
{
  return (gimp_draw_tool_calc_distance_square (draw_tool, display,
                                               x1, y1, x2, y2) < SQR (radius));
}

void
gimp_draw_tool_set_clip_rect (GimpDrawTool *draw_tool,
                              GdkRectangle *rect,
                              gboolean      use_offsets)
{
  GimpDisplayShell *shell;

  g_return_if_fail (GIMP_IS_DRAW_TOOL (draw_tool));

  shell = gimp_display_get_shell (draw_tool->display);

  if (rect)
    {
      GdkRectangle r;

      gimp_display_shell_transform_xy (shell,
                                       rect->x + rect->width,
                                       rect->y + rect->height,
                                       &r.width, &r.height,
                                       use_offsets);
      gimp_display_shell_transform_xy (shell,
                                       rect->x, rect->y,
                                       &r.x, &r.y,
                                       use_offsets);

      r.width  -= r.x;
      r.height -= r.y;

      gimp_canvas_set_clip_rect (GIMP_CANVAS (shell->canvas),
                                 GIMP_CANVAS_STYLE_XOR, &r);
    }
  else
    {
      gimp_canvas_set_clip_rect (GIMP_CANVAS (shell->canvas),
                                 GIMP_CANVAS_STYLE_XOR, NULL);
    }
}

/**
 * gimp_draw_tool_draw_line:
 * @draw_tool:   the #GimpDrawTool
 * @x1:          start point X in image coordinates
 * @y1:          start point Y in image coordinates
 * @x2:          end point X in image coordinates
 * @y2:          end point Y in image coordinates
 * @use_offsets: whether to use the image pixel offsets of the tool's display
 *
 * This function takes image space coordinates and transforms them to
 * screen window coordinates, then draws a line between the resulting
 * coordindates.
 **/
void
gimp_draw_tool_draw_line (GimpDrawTool *draw_tool,
                          gdouble       x1,
                          gdouble       y1,
                          gdouble       x2,
                          gdouble       y2,
                          gboolean      use_offsets)
{
  GimpDisplayShell *shell;
  gdouble           tx1, ty1;
  gdouble           tx2, ty2;

  g_return_if_fail (GIMP_IS_DRAW_TOOL (draw_tool));

  shell = gimp_display_get_shell (draw_tool->display);

  gimp_display_shell_transform_xy_f (shell,
                                     x1, y1,
                                     &tx1, &ty1,
                                     use_offsets);
  gimp_display_shell_transform_xy_f (shell,
                                     x2, y2,
                                     &tx2, &ty2,
                                     use_offsets);

  gimp_canvas_draw_line (GIMP_CANVAS (shell->canvas), GIMP_CANVAS_STYLE_XOR,
                         PROJ_ROUND (tx1), PROJ_ROUND (ty1),
                         PROJ_ROUND (tx2), PROJ_ROUND (ty2));
}

/**
 * gimp_draw_tool_draw_dashed_line:
 * @draw_tool:   the #GimpDrawTool
 * @x1:          start point X in image coordinates
 * @y1:          start point Y in image coordinates
 * @x2:          end point X in image coordinates
 * @y2:          end point Y in image coordinates
 * @use_offsets: whether to use the image pixel offsets of the tool's display
 *
 * This function takes image space coordinates and transforms them to
 * screen window coordinates, then draws a dashed line between the
 * resulting coordindates.
 **/
void
gimp_draw_tool_draw_dashed_line (GimpDrawTool *draw_tool,
                                 gdouble       x1,
                                 gdouble       y1,
                                 gdouble       x2,
                                 gdouble       y2,
                                 gboolean      use_offsets)
{
  GimpDisplayShell *shell;
  gdouble           tx1, ty1;
  gdouble           tx2, ty2;

  g_return_if_fail (GIMP_IS_DRAW_TOOL (draw_tool));

  shell = gimp_display_get_shell (draw_tool->display);

  gimp_display_shell_transform_xy_f (shell,
                                     x1, y1,
                                     &tx1, &ty1,
                                     use_offsets);
  gimp_display_shell_transform_xy_f (shell,
                                     x2, y2,
                                     &tx2, &ty2,
                                     use_offsets);

  gimp_canvas_draw_line (GIMP_CANVAS (shell->canvas),
                         GIMP_CANVAS_STYLE_XOR_DASHED,
                         PROJ_ROUND (tx1), PROJ_ROUND (ty1),
                         PROJ_ROUND (tx2), PROJ_ROUND (ty2));
}

/**
 * gimp_draw_tool_draw_guide:
 * @draw_tool:   the #GimpDrawTool
 * @orientation: the orientation of the guide line
 * @position:    the position of the guide line in image coordinates
 *
 * This function draws a guide line across the canvas.
 **/
void
gimp_draw_tool_draw_guide_line (GimpDrawTool        *draw_tool,
                                GimpOrientationType  orientation,
                                gint                 position)
{
  GimpDisplayShell *shell;
  gint              x1, y1, x2, y2;
  gint              x, y;

  g_return_if_fail (GIMP_IS_DRAW_TOOL (draw_tool));

  shell = gimp_display_get_shell (draw_tool->display);

  x1 = 0;
  y1 = 0;

  gdk_drawable_get_size (gtk_widget_get_window (shell->canvas), &x2, &y2);

  switch (orientation)
    {
    case GIMP_ORIENTATION_HORIZONTAL:
      gimp_display_shell_transform_xy (shell, 0, position, &x, &y, FALSE);
      y1 = y2 = y;
      break;

    case GIMP_ORIENTATION_VERTICAL:
      gimp_display_shell_transform_xy (shell, position, 0, &x, &y, FALSE);
      x1 = x2 = x;
      break;

    case GIMP_ORIENTATION_UNKNOWN:
      return;
    }

  gimp_canvas_draw_line (GIMP_CANVAS (shell->canvas), GIMP_CANVAS_STYLE_XOR,
                         x1, y1, x2, y2);
}

/**
 * gimp_draw_tool_draw_rectangle:
 * @draw_tool:   the #GimpDrawTool
 * @filled:      whether to fill the rectangle
 * @x:           horizontal image coordinate
 * @y:           vertical image coordinate
 * @width:       width in image coordinates
 * @height:      height in image coordinates
 * @use_offsets: whether to use the image pixel offsets of the tool's display
 *
 * This function takes image space coordinates and transforms them to
 * screen window coordinates, then draws the resulting rectangle.
 **/
void
gimp_draw_tool_draw_rectangle (GimpDrawTool *draw_tool,
                               gboolean      filled,
                               gdouble       x,
                               gdouble       y,
                               gdouble       width,
                               gdouble       height,
                               gboolean      use_offsets)
{
  GimpDisplayShell *shell;
  gdouble           tx1, ty1;
  gdouble           tx2, ty2;
  guint             w, h;

  g_return_if_fail (GIMP_IS_DRAW_TOOL (draw_tool));

  shell = gimp_display_get_shell (draw_tool->display);

  gimp_display_shell_transform_xy_f (shell,
                                     MIN (x, x + width), MIN (y, y + height),
                                     &tx1, &ty1,
                                     use_offsets);
  gimp_display_shell_transform_xy_f (shell,
                                     MAX (x, x + width), MAX (y, y + height),
                                     &tx2, &ty2,
                                     use_offsets);

  tx1 = CLAMP (tx1, -1, shell->disp_width  + 1);
  ty1 = CLAMP (ty1, -1, shell->disp_height + 1);
  tx2 = CLAMP (tx2, -1, shell->disp_width  + 1);
  ty2 = CLAMP (ty2, -1, shell->disp_height + 1);

  tx2 -= tx1;
  ty2 -= ty1;
  w = PROJ_ROUND (MAX (0.0, tx2));
  h = PROJ_ROUND (MAX (0.0, ty2));

  if (w > 0 && h > 0)
    {
      if (! filled)
        {
          w--;
          h--;
        }

      gimp_canvas_draw_rectangle (GIMP_CANVAS (shell->canvas),
                                  GIMP_CANVAS_STYLE_XOR,
                                  filled,
                                  PROJ_ROUND (tx1), PROJ_ROUND (ty1),
                                  w, h);
    }
}

void
gimp_draw_tool_draw_arc (GimpDrawTool *draw_tool,
                         gboolean      filled,
                         gdouble       x,
                         gdouble       y,
                         gdouble       width,
                         gdouble       height,
                         gint          angle1,
                         gint          angle2,
                         gboolean      use_offsets)
{
  GimpDisplayShell *shell;
  gdouble           tx1, ty1;
  gdouble           tx2, ty2;
  guint             w, h;

  g_return_if_fail (GIMP_IS_DRAW_TOOL (draw_tool));

  shell = gimp_display_get_shell (draw_tool->display);

  gimp_display_shell_transform_xy_f (shell,
                                     MIN (x, x + width), MIN (y, y + height),
                                     &tx1, &ty1,
                                     use_offsets);
  gimp_display_shell_transform_xy_f (shell,
                                     MAX (x, x + width), MAX (y, y + height),
                                     &tx2, &ty2,
                                     use_offsets);

  tx2 -= tx1;
  ty2 -= ty1;
  w = PROJ_ROUND (MAX (0.0, tx2));
  h = PROJ_ROUND (MAX (0.0, ty2));

  if (w > 0 && h > 0)
    {
      if (! filled)
        {
          w--;
          h--;
        }

      if (w != 1 && h != 1)
        {
          gimp_canvas_draw_arc (GIMP_CANVAS (shell->canvas),
                                GIMP_CANVAS_STYLE_XOR,
                                filled,
                                PROJ_ROUND (tx1), PROJ_ROUND (ty1),
                                w, h,
                                angle1, angle2);
        }
      else
        {
          /* work around the problem of an 1xN or Nx1 arc not being shown
           * properly
           */
          gimp_canvas_draw_rectangle (GIMP_CANVAS (shell->canvas),
                                      GIMP_CANVAS_STYLE_XOR,
                                      filled,
                                      PROJ_ROUND (tx1), PROJ_ROUND (ty1),
                                      w, h);
        }
    }
}

void
gimp_draw_tool_draw_rectangle_by_anchor (GimpDrawTool   *draw_tool,
                                         gboolean        filled,
                                         gdouble         x,
                                         gdouble         y,
                                         gint            width,
                                         gint            height,
                                         GtkAnchorType   anchor,
                                         gboolean        use_offsets)
{
  GimpDisplayShell *shell;
  gdouble           tx, ty;

  g_return_if_fail (GIMP_IS_DRAW_TOOL (draw_tool));

  shell = gimp_display_get_shell (draw_tool->display);

  gimp_display_shell_transform_xy_f (shell,
                                     x, y,
                                     &tx, &ty,
                                     use_offsets);

  gimp_draw_tool_shift_to_north_west (tx, ty,
                                      width, height,
                                      anchor,
                                      &tx, &ty);

  if (! filled)
    {
      width  -= 1;
      height -= 1;
    }

  gimp_canvas_draw_rectangle (GIMP_CANVAS (shell->canvas),
                              GIMP_CANVAS_STYLE_XOR,
                              filled,
                              PROJ_ROUND (tx), PROJ_ROUND (ty),
                              width, height);
}

void
gimp_draw_tool_draw_arc_by_anchor (GimpDrawTool  *draw_tool,
                                   gboolean       filled,
                                   gdouble        x,
                                   gdouble        y,
                                   gint           width,
                                   gint           height,
                                   gint           angle1,
                                   gint           angle2,
                                   GtkAnchorType  anchor,
                                   gboolean       use_offsets)
{
  GimpDisplayShell *shell;
  gdouble           tx, ty;

  g_return_if_fail (GIMP_IS_DRAW_TOOL (draw_tool));

  shell = gimp_display_get_shell (draw_tool->display);

  gimp_display_shell_transform_xy_f (shell,
                                     x, y,
                                     &tx, &ty,
                                     use_offsets);

  gimp_draw_tool_shift_to_north_west (tx, ty,
                                      width, height,
                                      anchor,
                                      &tx, &ty);

  if (! filled)
    {
      width  -= 1;
      height -= 1;
    }

  gimp_canvas_draw_arc (GIMP_CANVAS (shell->canvas), GIMP_CANVAS_STYLE_XOR,
                        filled,
                        PROJ_ROUND (tx), PROJ_ROUND (ty),
                        width, height,
                        angle1, angle2);
}

void
gimp_draw_tool_draw_cross_by_anchor (GimpDrawTool  *draw_tool,
                                     gdouble        x,
                                     gdouble        y,
                                     gint           width,
                                     gint           height,
                                     GtkAnchorType  anchor,
                                     gboolean       use_offsets)
{
  GimpDisplayShell *shell;
  gdouble           tx, ty;

  g_return_if_fail (GIMP_IS_DRAW_TOOL (draw_tool));

  shell = gimp_display_get_shell (draw_tool->display);

  gimp_display_shell_transform_xy_f (shell,
                                     x, y,
                                     &tx, &ty,
                                     use_offsets);

  gimp_draw_tool_shift_to_center (tx, ty,
                                  width, height,
                                  anchor,
                                  &tx, &ty);

  gimp_canvas_draw_line (GIMP_CANVAS (shell->canvas), GIMP_CANVAS_STYLE_XOR,
                         PROJ_ROUND (tx),
                         PROJ_ROUND (ty) - (height >> 1),
                         PROJ_ROUND (tx),
                         PROJ_ROUND (ty) + ((height + 1) >> 1));
  gimp_canvas_draw_line (GIMP_CANVAS (shell->canvas), GIMP_CANVAS_STYLE_XOR,
                         PROJ_ROUND (tx) - (width >> 1),
                         PROJ_ROUND (ty),
                         PROJ_ROUND (tx) + ((width + 1) >> 1),
                         PROJ_ROUND (ty));
}

void
gimp_draw_tool_draw_handle (GimpDrawTool   *draw_tool,
                            GimpHandleType  type,
                            gdouble         x,
                            gdouble         y,
                            gint            width,
                            gint            height,
                            GtkAnchorType   anchor,
                            gboolean        use_offsets)
{
  g_return_if_fail (GIMP_IS_DRAW_TOOL (draw_tool));

  switch (type)
    {
    case GIMP_HANDLE_SQUARE:
      gimp_draw_tool_draw_rectangle_by_anchor (draw_tool,
                                               FALSE,
                                               x, y,
                                               width,
                                               height,
                                               anchor,
                                               use_offsets);
      break;

    case GIMP_HANDLE_FILLED_SQUARE:
      gimp_draw_tool_draw_rectangle_by_anchor (draw_tool,
                                               TRUE,
                                               x, y,
                                               width,
                                               height,
                                               anchor,
                                               use_offsets);
      break;

    case GIMP_HANDLE_CIRCLE:
      gimp_draw_tool_draw_arc_by_anchor (draw_tool,
                                         FALSE,
                                         x, y,
                                         width,
                                         height,
                                         0, 360 * 64,
                                         anchor,
                                         use_offsets);
      break;

    case GIMP_HANDLE_FILLED_CIRCLE:
      gimp_draw_tool_draw_arc_by_anchor (draw_tool,
                                         TRUE,
                                         x, y,
                                         width,
                                         height,
                                         0, 360 * 64,
                                         anchor,
                                         use_offsets);
      break;

    case GIMP_HANDLE_CROSS:
      gimp_draw_tool_draw_cross_by_anchor (draw_tool,
                                           x, y,
                                           width,
                                           height,
                                           anchor,
                                           use_offsets);
      break;

    default:
      g_warning ("%s: invalid handle type %d", G_STRFUNC, type);
      break;
    }
}

/**
 * gimp_draw_tool_draw_corner:
 * @draw_tool:   the #GimpDrawTool
 * @highlight:
 * @put_outside: whether to put the handles on the outside of the rectangle
 * @x1:
 * @y1:
 * @x2:
 * @y2:
 * @width:       corner width
 * @height:      corner height
 * @anchor:      which corner to draw
 * @use_offsets: whether to use the image pixel offsets of the tool's display
 *
 * This function takes image space coordinates and transforms them to
 * screen window coordinates. It draws a corner into an already drawn
 * rectangle outline, taking care of not drawing over an already drawn line.
 **/
void
gimp_draw_tool_draw_corner (GimpDrawTool   *draw_tool,
                            gboolean        highlight,
                            gboolean        put_outside,
                            gdouble         x1,
                            gdouble         y1,
                            gdouble         x2,
                            gdouble         y2,
                            gint            width,
                            gint            height,
                            GtkAnchorType   anchor,
                            gboolean        use_offsets)
{
  GimpDisplayShell *shell;
  GimpCanvas       *canvas;
  gint              tx1, ty1;
  gint              tx2, ty2;
  gint              tw,  th;
  gint              top_and_bottom_handle_x_offset;
  gint              left_and_right_handle_y_offset;

  g_return_if_fail (GIMP_IS_DRAW_TOOL (draw_tool));

  shell  = gimp_display_get_shell (draw_tool->display);
  canvas = GIMP_CANVAS (shell->canvas);

  gimp_display_shell_transform_xy (shell, x1, y1, &tx1, &ty1, use_offsets);
  gimp_display_shell_transform_xy (shell, x2, y2, &tx2, &ty2, use_offsets);

  tw = tx2 - tx1;
  th = ty2 - ty1;

  if ((! put_outside && (tw <= width || th <= height)) ||
      width <= 2 || height <= 2)
    return;

  top_and_bottom_handle_x_offset = (tw - width)  / 2;
  left_and_right_handle_y_offset = (th - height) / 2;

  switch (anchor)
    {
    case GTK_ANCHOR_CENTER:
      break;

    case GTK_ANCHOR_NORTH_WEST:
      if (put_outside)
        {
          gimp_canvas_draw_line (canvas, GIMP_CANVAS_STYLE_XOR,
                                 tx1 - 1,         ty1,
                                 tx1 - width + 1, ty1);
          gimp_canvas_draw_line (canvas, GIMP_CANVAS_STYLE_XOR,
                                 tx1 - width + 1, ty1,
                                 tx1 - width + 1, ty1 - height + 1);
          gimp_canvas_draw_line (canvas, GIMP_CANVAS_STYLE_XOR,
                                 tx1 - width + 1, ty1 - height + 1,
                                 tx1,             ty1 - height + 1);
          gimp_canvas_draw_line (canvas, GIMP_CANVAS_STYLE_XOR,
                                 tx1,             ty1 - height + 1,
                                 tx1,             ty1);
        }
      else
        {
          gimp_canvas_draw_line (canvas, GIMP_CANVAS_STYLE_XOR,
                                 tx1 + 1,         ty1 + height - 1,
                                 tx1 + width - 1, ty1 + height - 1);
          gimp_canvas_draw_line (canvas, GIMP_CANVAS_STYLE_XOR,
                                 tx1 + width - 1, ty1 + 1,
                                 tx1 + width - 1, ty1 + height);
        }
      break;

    case GTK_ANCHOR_NORTH_EAST:
      if (put_outside)
        {
          gimp_canvas_draw_line (canvas, GIMP_CANVAS_STYLE_XOR,
                                 tx2,             ty1,
                                 tx2 + width - 2, ty1);
          gimp_canvas_draw_line (canvas, GIMP_CANVAS_STYLE_XOR,
                                 tx2 + width - 2, ty1,
                                 tx2 + width - 2, ty1 - height + 1);
          gimp_canvas_draw_line (canvas, GIMP_CANVAS_STYLE_XOR,
                                 tx2 + width - 2, ty1 - height + 1,
                                 tx2 - 1,         ty1 - height + 1);
          gimp_canvas_draw_line (canvas, GIMP_CANVAS_STYLE_XOR,
                                 tx2 - 1,         ty1 - height + 1,
                                 tx2 - 1,         ty1);
        }
      else
        {
          gimp_canvas_draw_line (canvas, GIMP_CANVAS_STYLE_XOR,
                                 tx2 - 2,         ty1 + height - 1,
                                 tx2 - width,     ty1 + height - 1);
          gimp_canvas_draw_line (canvas, GIMP_CANVAS_STYLE_XOR,
                                 tx2 - width,     ty1 + 1,
                                 tx2 - width,     ty1 + height);
        }
      break;

    case GTK_ANCHOR_SOUTH_WEST:
      if (put_outside)
        {
          gimp_canvas_draw_line (canvas, GIMP_CANVAS_STYLE_XOR,
                                 tx1 - 1,         ty2 - 1,
                                 tx1 - width + 1, ty2 - 1);
          gimp_canvas_draw_line (canvas, GIMP_CANVAS_STYLE_XOR,
                                 tx1 - width + 1, ty2 - 1,
                                 tx1 - width + 1, ty2 + height - 2);
          gimp_canvas_draw_line (canvas, GIMP_CANVAS_STYLE_XOR,
                                 tx1 - width + 1, ty2 + height - 2,
                                 tx1,             ty2 + height - 2);
          gimp_canvas_draw_line (canvas, GIMP_CANVAS_STYLE_XOR,
                                 tx1,             ty2 + height - 2,
                                 tx1,             ty2 - 1);
        }
      else
        {
          gimp_canvas_draw_line (canvas, GIMP_CANVAS_STYLE_XOR,
                                 tx1 + 1,         ty2 - height,
                                 tx1 + width - 1, ty2 - height);
          gimp_canvas_draw_line (canvas, GIMP_CANVAS_STYLE_XOR,
                                 tx1 + width - 1, ty2 - height,
                                 tx1 + width - 1, ty2 - 1);
        }
      break;

    case GTK_ANCHOR_SOUTH_EAST:
      if (put_outside)
        {
          gimp_canvas_draw_line (canvas, GIMP_CANVAS_STYLE_XOR,
                                 tx2,             ty2 - 1,
                                 tx2 + width - 2, ty2 - 1);
          gimp_canvas_draw_line (canvas, GIMP_CANVAS_STYLE_XOR,
                                 tx2 + width - 2, ty2 - 1,
                                 tx2 + width - 2, ty2 + height - 2);
          gimp_canvas_draw_line (canvas, GIMP_CANVAS_STYLE_XOR,
                                 tx2 + width - 2, ty2 + height - 2,
                                 tx2 - 1,         ty2 + height - 2);
          gimp_canvas_draw_line (canvas, GIMP_CANVAS_STYLE_XOR,
                                 tx2 - 1,         ty2 + height - 2,
                                 tx2 - 1,         ty2 - 1);
        }
      else
        {
          gimp_canvas_draw_line (canvas, GIMP_CANVAS_STYLE_XOR,
                                 tx2 - 2,         ty2 - height,
                                 tx2 - width,     ty2 - height);
          gimp_canvas_draw_line (canvas, GIMP_CANVAS_STYLE_XOR,
                                 tx2 - width,     ty2 - height,
                                 tx2 - width,     ty2 - 1);
        }
      break;

    case GTK_ANCHOR_NORTH:
      if (put_outside)
        {
          gimp_canvas_draw_line (canvas, GIMP_CANVAS_STYLE_XOR,
                                 tx1,             ty1 - 1,
                                 tx1,             ty1 - height + 1);
          gimp_canvas_draw_line (canvas, GIMP_CANVAS_STYLE_XOR,
                                 tx1,             ty1 - height + 1,
                                 tx2 - 1,         ty1 - height + 1);
          gimp_canvas_draw_line (canvas, GIMP_CANVAS_STYLE_XOR,
                                 tx2 - 1,         ty1 - height + 1,
                                 tx2 - 1,         ty1);
        }
      else
        {
          gimp_canvas_draw_line (canvas, GIMP_CANVAS_STYLE_XOR,
                                 tx1 + 1, ty1 + height - 1,
                                 tx2 - 1, ty1 + height - 1);
        }
      break;

    case GTK_ANCHOR_SOUTH:
      if (put_outside)
        {
          gimp_canvas_draw_line (canvas, GIMP_CANVAS_STYLE_XOR,
                                 tx1,             ty2,
                                 tx1,             ty2 + height - 2);
          gimp_canvas_draw_line (canvas, GIMP_CANVAS_STYLE_XOR,
                                 tx1,             ty2 + height - 2,
                                 tx2 - 1,         ty2 + height - 2);
          gimp_canvas_draw_line (canvas, GIMP_CANVAS_STYLE_XOR,
                                 tx2 - 1,         ty2 + height - 2,
                                 tx2 - 1,         ty2 - 1);
        }
      else
        {
          gimp_canvas_draw_line (canvas, GIMP_CANVAS_STYLE_XOR,
                                 tx1 + 1, ty2 - height,
                                 tx2 - 1, ty2 - height);
        }
      break;

    case GTK_ANCHOR_WEST:
      if (put_outside)
        {
          gimp_canvas_draw_line (canvas, GIMP_CANVAS_STYLE_XOR,
                                 tx1 - 1,         ty1,
                                 tx1 - width + 1, ty1);
          gimp_canvas_draw_line (canvas, GIMP_CANVAS_STYLE_XOR,
                                 tx1 - width + 1, ty1,
                                 tx1 - width + 1, ty2 - 1);
          gimp_canvas_draw_line (canvas, GIMP_CANVAS_STYLE_XOR,
                                 tx1 - width + 1, ty2 - 1,
                                 tx1,             ty2 - 1);
        }
      else
        {
          gimp_canvas_draw_line (canvas, GIMP_CANVAS_STYLE_XOR,
                                 tx1 + width - 1, ty1 + 1,
                                 tx1 + width - 1, ty2 - 1);
        }
      break;

    case GTK_ANCHOR_EAST:
      if (put_outside)
        {
          gimp_canvas_draw_line (canvas, GIMP_CANVAS_STYLE_XOR,
                                 tx2,             ty1,
                                 tx2 + width - 2, ty1);
          gimp_canvas_draw_line (canvas, GIMP_CANVAS_STYLE_XOR,
                                 tx2 + width - 2, ty1,
                                 tx2 + width - 2, ty2 - 1);
          gimp_canvas_draw_line (canvas, GIMP_CANVAS_STYLE_XOR,
                                 tx2 + width - 2, ty2 - 1,
                                 tx2 - 1,         ty2 - 1);
        }
      else
        {
          gimp_canvas_draw_line (canvas, GIMP_CANVAS_STYLE_XOR,
                                 tx2 - width, ty1 + 1,
                                 tx2 - width, ty2 - 1);
        }
      break;
    }

  if (! highlight)
    return;

  switch (anchor)
    {
    case GTK_ANCHOR_NORTH_WEST:
      if (put_outside)
        {
          gimp_canvas_draw_rectangle (canvas, GIMP_CANVAS_STYLE_XOR, FALSE,
                                      tx1 - width + 2, ty1 - height + 2,
                                      width - 3,       height - 3);
        }
      else
        {
          gimp_canvas_draw_rectangle (canvas, GIMP_CANVAS_STYLE_XOR, FALSE,
                                      tx1 + 1,         ty1 + 1,
                                      width - 3,       height - 3);
        }
      break;

    case GTK_ANCHOR_NORTH_EAST:
      if (put_outside)
        {
          gimp_canvas_draw_rectangle (canvas, GIMP_CANVAS_STYLE_XOR, FALSE,
                                      tx2,             ty1 - height + 2,
                                      width - 3,       height - 3);
        }
      else
        {
          gimp_canvas_draw_rectangle (canvas, GIMP_CANVAS_STYLE_XOR, FALSE,
                                      tx2 - width + 1, ty1 + 1,
                                      width - 3,       height - 3);
        }
      break;

    case GTK_ANCHOR_SOUTH_WEST:
      if (put_outside)
        {
          gimp_canvas_draw_rectangle (canvas, GIMP_CANVAS_STYLE_XOR, FALSE,
                                      tx1 - width + 2, ty2,
                                      width - 3,       height - 3);
        }
      else
        {
          gimp_canvas_draw_rectangle (canvas, GIMP_CANVAS_STYLE_XOR, FALSE,
                                      tx1 + 1,         ty2 - height + 1,
                                      width - 3,       height - 3);
        }
      break;

    case GTK_ANCHOR_SOUTH_EAST:
      if (put_outside)
        {
          gimp_canvas_draw_rectangle (canvas, GIMP_CANVAS_STYLE_XOR, FALSE,
                                      tx2,             ty2,
                                      width - 3,       height - 3);
        }
      else
        {
          gimp_canvas_draw_rectangle (canvas, GIMP_CANVAS_STYLE_XOR, FALSE,
                                      tx2 - width + 1, ty2 - height + 1,
                                      width - 3,       height - 3);
        }
      break;

    case GTK_ANCHOR_NORTH:
      if (put_outside)
        {
          gimp_canvas_draw_rectangle (canvas, GIMP_CANVAS_STYLE_XOR, FALSE,
                                      tx1 + top_and_bottom_handle_x_offset + 1,
                                      ty1 - height + 2,
                                      width - 3, height - 3);
        }
      else
        {
          gimp_canvas_draw_rectangle (canvas, GIMP_CANVAS_STYLE_XOR, FALSE,
                                      tx1 + top_and_bottom_handle_x_offset,
                                      ty1 + 1,
                                      width, height - 3);
          gimp_canvas_draw_line (canvas, GIMP_CANVAS_STYLE_XOR,
                                 tx1 + top_and_bottom_handle_x_offset + 1,
                                 ty1 + 2,
                                 tx1 + top_and_bottom_handle_x_offset + 1,
                                 ty1 + height - 2);
          gimp_canvas_draw_line (canvas, GIMP_CANVAS_STYLE_XOR,
                                 tx1 + top_and_bottom_handle_x_offset + width - 1,
                                 ty1 + 2,
                                 tx1 + top_and_bottom_handle_x_offset + width - 1,
                                 ty1 + height - 2);
        }
      break;

    case GTK_ANCHOR_SOUTH:
      if (put_outside)
        {
          gimp_canvas_draw_rectangle (canvas, GIMP_CANVAS_STYLE_XOR, FALSE,
                                      tx1 + top_and_bottom_handle_x_offset + 1,
                                      ty2,
                                      width - 3, height - 3);
        }
      else
        {
          gimp_canvas_draw_rectangle (canvas, GIMP_CANVAS_STYLE_XOR, FALSE,
                                      tx1 + top_and_bottom_handle_x_offset,
                                      ty2 - height + 1,
                                      width, height - 3);
          gimp_canvas_draw_line (canvas, GIMP_CANVAS_STYLE_XOR,
                                 tx1 + top_and_bottom_handle_x_offset + 1,
                                 ty2 - 3,
                                 tx1 + top_and_bottom_handle_x_offset + 1,
                                 ty2 - height + 1);
          gimp_canvas_draw_line (canvas, GIMP_CANVAS_STYLE_XOR,
                                 tx1 + top_and_bottom_handle_x_offset + width - 1,
                                 ty2 - 3,
                                 tx1 + top_and_bottom_handle_x_offset + width - 1,
                                 ty2 - height + 1);
        }
      break;

    case GTK_ANCHOR_WEST:
      if (put_outside)
        {
          gimp_canvas_draw_rectangle (canvas, GIMP_CANVAS_STYLE_XOR, FALSE,
                                      tx1 - width + 2,
                                      ty1 + left_and_right_handle_y_offset + 1,
                                      width - 3, height - 3);
        }
      else
        {
          gimp_canvas_draw_rectangle (canvas, GIMP_CANVAS_STYLE_XOR, FALSE,
                                      tx1 + 1,
                                      ty1 + left_and_right_handle_y_offset,
                                      width - 3, height);
          gimp_canvas_draw_line (canvas, GIMP_CANVAS_STYLE_XOR,
                                 tx1 + 2,
                                 ty1 + left_and_right_handle_y_offset + 1,
                                 tx1 + width - 2,
                                 ty1 + left_and_right_handle_y_offset + 1);
          gimp_canvas_draw_line (canvas, GIMP_CANVAS_STYLE_XOR,
                                 tx1 + 2,
                                 ty1 + left_and_right_handle_y_offset + height - 1,
                                 tx1 + width - 2,
                                 ty1 + left_and_right_handle_y_offset + height - 1);
        }
      break;

    case GTK_ANCHOR_EAST:
      if (put_outside)
        {
          gimp_canvas_draw_rectangle (canvas, GIMP_CANVAS_STYLE_XOR, FALSE,
                                      tx2,
                                      ty1 + left_and_right_handle_y_offset + 1,
                                      width - 3, height - 3);
        }
      else
        {
          gimp_canvas_draw_rectangle (canvas, GIMP_CANVAS_STYLE_XOR, FALSE,
                                      tx2 - width + 1,
                                      ty1 + left_and_right_handle_y_offset,
                                      width - 3, height);
          gimp_canvas_draw_line (canvas, GIMP_CANVAS_STYLE_XOR,
                                 tx2 - 3,
                                 ty1 + left_and_right_handle_y_offset + 1,
                                 tx2 - width + 1,
                                 ty1 + left_and_right_handle_y_offset + 1);
          gimp_canvas_draw_line (canvas, GIMP_CANVAS_STYLE_XOR,
                                 tx2 - 3,
                                 ty1 + left_and_right_handle_y_offset + height - 1,
                                 tx2 - width + 1,
                                 ty1 + left_and_right_handle_y_offset + height - 1);
        }
      break;

    default:
      break;
    }
}


void
gimp_draw_tool_draw_lines (GimpDrawTool      *draw_tool,
                           const GimpVector2 *points,
                           gint               n_points,
                           gboolean           filled,
                           gboolean           use_offsets)
{
  GimpDisplayShell *shell;
  GdkPoint         *coords;

  g_return_if_fail (GIMP_IS_DRAW_TOOL (draw_tool));

  if (points == NULL || n_points == 0)
    return;

  shell = gimp_display_get_shell (draw_tool->display);

  coords = g_new (GdkPoint, n_points);

  gimp_display_shell_transform_points (shell,
                                       points, coords, n_points, use_offsets);

  if (filled)
    {
      gimp_canvas_draw_polygon (GIMP_CANVAS (shell->canvas),
                                GIMP_CANVAS_STYLE_XOR,
                                TRUE, coords, n_points);
    }
  else
    {
      gimp_canvas_draw_lines (GIMP_CANVAS (shell->canvas),
                              GIMP_CANVAS_STYLE_XOR,
                              coords, n_points);
    }

  g_free (coords);
}

void
gimp_draw_tool_draw_strokes (GimpDrawTool     *draw_tool,
                             const GimpCoords *points,
                             gint              n_points,
                             gboolean          filled,
                             gboolean          use_offsets)
{
  GimpDisplayShell *shell;
  GdkPoint         *coords;

  g_return_if_fail (GIMP_IS_DRAW_TOOL (draw_tool));

  if (n_points == 0)
    return;

  shell = gimp_display_get_shell (draw_tool->display);

  coords = g_new (GdkPoint, n_points);

  gimp_display_shell_transform_coords (shell,
                                       points, coords, n_points, use_offsets);

  if (filled)
    {
      gimp_canvas_draw_polygon (GIMP_CANVAS (shell->canvas),
                                GIMP_CANVAS_STYLE_XOR,
                                TRUE, coords, n_points);
    }
  else
    {
      gimp_canvas_draw_lines (GIMP_CANVAS (shell->canvas),
                              GIMP_CANVAS_STYLE_XOR,
                              coords, n_points);
    }

  g_free (coords);
}

/**
 * gimp_draw_tool_draw_boundary:
 * @draw_tool:    a #GimpDrawTool
 * @bound_segs:   the sorted brush outline
 * @n_bound_segs: the number of segments in @bound_segs
 * @offset_x:     x offset
 * @offset_y:     y offset
 * @use_offsets:  whether to use offsets
 *
 * Draw the boundary of the brush that @draw_tool uses. The boundary
 * should be sorted with sort_boundary(), and @n_bound_segs should
 * include the sentinel segments inserted by sort_boundary() that
 * indicate the end of connected segment sequences (groups) .
 */
void
gimp_draw_tool_draw_boundary (GimpDrawTool   *draw_tool,
                              const BoundSeg *bound_segs,
                              gint            n_bound_segs,
                              gdouble         offset_x,
                              gdouble         offset_y,
                              gboolean        use_offsets)
{
  GimpDisplayShell *shell;
  GdkPoint         *gdk_points;
  gint              n_gdk_points;
  gint              xmax, ymax;
  gdouble           x, y;
  gint              i;

  g_return_if_fail (GIMP_IS_DRAW_TOOL (draw_tool));
  g_return_if_fail (n_bound_segs > 0);
  g_return_if_fail (bound_segs != NULL);

  shell = gimp_display_get_shell (draw_tool->display);

  gdk_points = g_new0 (GdkPoint, n_bound_segs + 1);
  n_gdk_points = 0;

  xmax = shell->disp_width  + 1;
  ymax = shell->disp_height + 1;

  /* The sorted boundary has sentinel segments inserted at the end of
   * each group.
   */
  for (i = 0; i < n_bound_segs; i++)
    {
      if (bound_segs[i].x1 == -1 &&
          bound_segs[i].y1 == -1 &&
          bound_segs[i].x2 == -1 &&
          bound_segs[i].y2 == -1)
        {
          /* Group ends */
          gimp_canvas_draw_lines (GIMP_CANVAS (shell->canvas),
                                  GIMP_CANVAS_STYLE_XOR_DOTTED,
                                  gdk_points, n_gdk_points);
          n_gdk_points = 0;
          continue;
        }

      if (n_gdk_points == 0)
        {
          gimp_display_shell_transform_xy_f (shell,
                                             bound_segs[i].x1 + offset_x,
                                             bound_segs[i].y1 + offset_y,
                                             &x, &y,
                                             use_offsets);

          gdk_points[0].x = PROJ_ROUND (CLAMP (x, -1, xmax));
          gdk_points[0].y = PROJ_ROUND (CLAMP (y, -1, ymax));

          /*  If this segment is a closing segment && the segments lie inside
           *  the region, OR if this is an opening segment and the segments
           *  lie outside the region...
           *  we need to transform it by one display pixel
           */
          if (! bound_segs[i].open)
            {
              /*  If it is vertical  */
              if (bound_segs[i].x1 == bound_segs[i].x2)
                {
                  gdk_points[0].x -= 1;
                }
              else
                {
                  gdk_points[0].y -= 1;
                }
            }

          n_gdk_points++;
        }

      g_assert (n_gdk_points < n_bound_segs + 1);

      gimp_display_shell_transform_xy_f (shell,
                                         bound_segs[i].x2 + offset_x,
                                         bound_segs[i].y2 + offset_y,
                                         &x, &y,
                                         use_offsets);

      gdk_points[n_gdk_points].x = PROJ_ROUND (CLAMP (x, -1, xmax));
      gdk_points[n_gdk_points].y = PROJ_ROUND (CLAMP (y, -1, ymax));

      /*  If this segment is a closing segment && the segments lie inside
       *  the region, OR if this is an opening segment and the segments
       *  lie outside the region...
       *  we need to transform it by one display pixel
       */
      if (! bound_segs[i].open)
        {
          /*  If it is vertical  */
          if (bound_segs[i].x1 == bound_segs[i].x2)
            {
              gdk_points[n_gdk_points    ].x -= 1;
              gdk_points[n_gdk_points - 1].x -= 1;
            }
          else
            {
              gdk_points[n_gdk_points    ].y -= 1;
              gdk_points[n_gdk_points - 1].y -= 1;
            }
        }

      n_gdk_points++;
    }

  g_free (gdk_points);
}

void
gimp_draw_tool_draw_text_cursor (GimpDrawTool   *draw_tool,
                                 PangoRectangle *cursor,
                                 gboolean        overwrite,
                                 gboolean        use_offsets)
{
  GimpDisplayShell *shell;
  gdouble           tx1, ty1;
  gdouble           tx2, ty2;

  g_return_if_fail (GIMP_IS_DRAW_TOOL (draw_tool));

  shell = gimp_display_get_shell (draw_tool->display);

  gimp_display_shell_transform_xy_f (shell,
                                     cursor->x, cursor->y,
                                     &tx1, &ty1,
                                     use_offsets);

  if (overwrite)
    {
      gint x, y;
      gint width, height;

      gimp_display_shell_transform_xy_f (shell,
                                         cursor->x + cursor->width,
                                         cursor->y + cursor->height,
                                         &tx2, &ty2,
                                         use_offsets);

      x      = PROJ_ROUND (tx1);
      y      = PROJ_ROUND (ty1);
      width  = PROJ_ROUND (tx2 - tx1);
      height = PROJ_ROUND (ty2 - ty1);

      gimp_canvas_draw_rectangle (GIMP_CANVAS (shell->canvas),
                                  GIMP_CANVAS_STYLE_XOR, FALSE,
                                  x, y,
                                  width, height);
      gimp_canvas_draw_rectangle (GIMP_CANVAS (shell->canvas),
                                  GIMP_CANVAS_STYLE_XOR, FALSE,
                                  x + 1, y + 1,
                                  width - 2, height - 2);
    }
  else
    {
      gimp_display_shell_transform_xy_f (shell,
                                         cursor->x,
                                         cursor->y + cursor->height,
                                         &tx2, &ty2,
                                         use_offsets);

      /*  vertical line  */
      gimp_canvas_draw_line (GIMP_CANVAS (shell->canvas),
                             GIMP_CANVAS_STYLE_XOR,
                             PROJ_ROUND (tx1), PROJ_ROUND (ty1) + 2,
                             PROJ_ROUND (tx2), PROJ_ROUND (ty2) - 2);
      gimp_canvas_draw_line (GIMP_CANVAS (shell->canvas),
                             GIMP_CANVAS_STYLE_XOR,
                             PROJ_ROUND (tx1) - 1, PROJ_ROUND (ty1) + 2,
                             PROJ_ROUND (tx2) - 1, PROJ_ROUND (ty2) - 2);

      /*  top serif  */
      gimp_canvas_draw_line (GIMP_CANVAS (shell->canvas),
                             GIMP_CANVAS_STYLE_XOR,
                             PROJ_ROUND (tx1) - 3, PROJ_ROUND (ty1),
                             PROJ_ROUND (tx1) + 3, PROJ_ROUND (ty1));
      gimp_canvas_draw_line (GIMP_CANVAS (shell->canvas),
                             GIMP_CANVAS_STYLE_XOR,
                             PROJ_ROUND (tx1) - 3, PROJ_ROUND (ty1) + 1,
                             PROJ_ROUND (tx1) + 3, PROJ_ROUND (ty1) + 1);

      /*  bottom serif  */
      gimp_canvas_draw_line (GIMP_CANVAS (shell->canvas),
                             GIMP_CANVAS_STYLE_XOR,
                             PROJ_ROUND (tx2) - 3, PROJ_ROUND (ty2) - 1,
                             PROJ_ROUND (tx2) + 3, PROJ_ROUND (ty2) - 1);
      gimp_canvas_draw_line (GIMP_CANVAS (shell->canvas),
                             GIMP_CANVAS_STYLE_XOR,
                             PROJ_ROUND (tx2) - 3, PROJ_ROUND (ty2) - 2,
                             PROJ_ROUND (tx2) + 3, PROJ_ROUND (ty2) - 2);
    }
}

gboolean
gimp_draw_tool_on_handle (GimpDrawTool   *draw_tool,
                          GimpDisplay    *display,
                          gdouble         x,
                          gdouble         y,
                          GimpHandleType  type,
                          gdouble         handle_x,
                          gdouble         handle_y,
                          gint            width,
                          gint            height,
                          GtkAnchorType   anchor,
                          gboolean        use_offsets)
{
  GimpDisplayShell *shell;
  gdouble           tx, ty;
  gdouble           handle_tx, handle_ty;

  g_return_val_if_fail (GIMP_IS_DRAW_TOOL (draw_tool), FALSE);
  g_return_val_if_fail (GIMP_IS_DISPLAY (display), FALSE);

  shell = gimp_display_get_shell (display);

  gimp_display_shell_transform_xy_f (shell,
                                     x, y,
                                     &tx, &ty,
                                     use_offsets);
  gimp_display_shell_transform_xy_f (shell,
                                     handle_x, handle_y,
                                     &handle_tx, &handle_ty,
                                     use_offsets);

  switch (type)
    {
    case GIMP_HANDLE_SQUARE:
    case GIMP_HANDLE_FILLED_SQUARE:
    case GIMP_HANDLE_CROSS:
      gimp_draw_tool_shift_to_north_west (handle_tx, handle_ty,
                                          width, height,
                                          anchor,
                                          &handle_tx, &handle_ty);

      return (tx == CLAMP (tx, handle_tx, handle_tx + width) &&
              ty == CLAMP (ty, handle_ty, handle_ty + height));

    case GIMP_HANDLE_CIRCLE:
    case GIMP_HANDLE_FILLED_CIRCLE:
      gimp_draw_tool_shift_to_center (handle_tx, handle_ty,
                                      width, height,
                                      anchor,
                                      &handle_tx, &handle_ty);

      /* FIXME */
      if (width != height)
        width = (width + height) / 2;

      width /= 2;

      return ((SQR (handle_tx - tx) + SQR (handle_ty - ty)) < SQR (width));

    default:
      g_warning ("%s: invalid handle type %d", G_STRFUNC, type);
      break;
    }

  return FALSE;
}

gboolean
gimp_draw_tool_on_vectors_handle (GimpDrawTool      *draw_tool,
                                  GimpDisplay       *display,
                                  GimpVectors       *vectors,
                                  const GimpCoords  *coord,
                                  gint               width,
                                  gint               height,
                                  GimpAnchorType     preferred,
                                  gboolean           exclusive,
                                  GimpAnchor       **ret_anchor,
                                  GimpStroke       **ret_stroke)
{
  GimpStroke *stroke       = NULL;
  GimpStroke *pref_stroke  = NULL;
  GimpAnchor *anchor       = NULL;
  GimpAnchor *pref_anchor  = NULL;
  gdouble     dx, dy;
  gdouble     pref_mindist = -1;
  gdouble     mindist      = -1;

  g_return_val_if_fail (GIMP_IS_DRAW_TOOL (draw_tool), FALSE);
  g_return_val_if_fail (GIMP_IS_DISPLAY (display), FALSE);
  g_return_val_if_fail (GIMP_IS_VECTORS (vectors), FALSE);
  g_return_val_if_fail (coord != NULL, FALSE);

  if (ret_anchor) *ret_anchor = NULL;
  if (ret_stroke) *ret_stroke = NULL;

  while ((stroke = gimp_vectors_stroke_get_next (vectors, stroke)))
    {
      GList *anchor_list = gimp_stroke_get_draw_anchors (stroke);
      GList *list;

      anchor_list = g_list_concat (gimp_stroke_get_draw_anchors (stroke),
                                   gimp_stroke_get_draw_controls (stroke));

      for (list = anchor_list; list; list = g_list_next (list))
        {
          dx = coord->x - GIMP_ANCHOR (list->data)->position.x;
          dy = coord->y - GIMP_ANCHOR (list->data)->position.y;

          if (mindist < 0 || mindist > dx * dx + dy * dy)
            {
              mindist = dx * dx + dy * dy;
              anchor = GIMP_ANCHOR (list->data);

              if (ret_stroke)
                *ret_stroke = stroke;
            }

          if ((pref_mindist < 0 || pref_mindist > dx * dx + dy * dy) &&
              GIMP_ANCHOR (list->data)->type == preferred)
            {
              pref_mindist = dx * dx + dy * dy;
              pref_anchor = GIMP_ANCHOR (list->data);
              pref_stroke = stroke;
            }
        }

      g_list_free (anchor_list);
    }

  /* If the data passed into ret_anchor is a preferred anchor, return it. */
  if (ret_anchor && *ret_anchor &&
      gimp_draw_tool_on_handle (draw_tool, display,
                                coord->x,
                                coord->y,
                                GIMP_HANDLE_CIRCLE,
                                (*ret_anchor)->position.x,
                                (*ret_anchor)->position.y,
                                width, height,
                                GTK_ANCHOR_CENTER,
                                FALSE) &&
      (*ret_anchor)->type == preferred)
    {
      if (ret_stroke) *ret_stroke = pref_stroke;

      return TRUE;
    }

  if (pref_anchor && gimp_draw_tool_on_handle (draw_tool, display,
                                               coord->x,
                                               coord->y,
                                               GIMP_HANDLE_CIRCLE,
                                               pref_anchor->position.x,
                                               pref_anchor->position.y,
                                               width, height,
                                               GTK_ANCHOR_CENTER,
                                               FALSE))
    {
      if (ret_anchor) *ret_anchor = pref_anchor;
      if (ret_stroke) *ret_stroke = pref_stroke;

      return TRUE;
    }
  else if (!exclusive && anchor &&
           gimp_draw_tool_on_handle (draw_tool, display,
                                     coord->x,
                                     coord->y,
                                     GIMP_HANDLE_CIRCLE,
                                     anchor->position.x,
                                     anchor->position.y,
                                     width, height,
                                     GTK_ANCHOR_CENTER,
                                     FALSE))
    {
      if (ret_anchor)
        *ret_anchor = anchor;

      /* *ret_stroke already set correctly. */
      return TRUE;
    }

  if (ret_anchor)
    *ret_anchor = NULL;
  if (ret_stroke)
    *ret_stroke = NULL;

  return FALSE;
}

gboolean
gimp_draw_tool_on_vectors_curve (GimpDrawTool      *draw_tool,
                                 GimpDisplay       *display,
                                 GimpVectors       *vectors,
                                 const GimpCoords  *coord,
                                 gint               width,
                                 gint               height,
                                 GimpCoords        *ret_coords,
                                 gdouble           *ret_pos,
                                 GimpAnchor       **ret_segment_start,
                                 GimpAnchor       **ret_segment_end,
                                 GimpStroke       **ret_stroke)
{
  GimpStroke *stroke = NULL;
  GimpAnchor *segment_start;
  GimpAnchor *segment_end;
  GimpCoords  min_coords = GIMP_COORDS_DEFAULT_VALUES;
  GimpCoords  cur_coords;
  gdouble     min_dist, cur_dist, cur_pos;

  g_return_val_if_fail (GIMP_IS_DRAW_TOOL (draw_tool), FALSE);
  g_return_val_if_fail (GIMP_IS_DISPLAY (display), FALSE);
  g_return_val_if_fail (GIMP_IS_VECTORS (vectors), FALSE);
  g_return_val_if_fail (coord != NULL, FALSE);

  if (ret_coords)        *ret_coords        = *coord;
  if (ret_pos)           *ret_pos           = -1.0;
  if (ret_segment_start) *ret_segment_start = NULL;
  if (ret_segment_start) *ret_segment_end   = NULL;
  if (ret_stroke)        *ret_stroke        = NULL;

  min_dist = -1.0;

  while ((stroke = gimp_vectors_stroke_get_next (vectors, stroke)))
    {
      cur_dist = gimp_stroke_nearest_point_get (stroke, coord, 1.0,
                                                &cur_coords,
                                                &segment_start,
                                                &segment_end,
                                                &cur_pos);

      if (cur_dist >= 0 && (min_dist < 0 || cur_dist < min_dist))
        {
          min_dist   = cur_dist;
          min_coords = cur_coords;

          if (ret_coords)        *ret_coords        = cur_coords;
          if (ret_pos)           *ret_pos           = cur_pos;
          if (ret_segment_start) *ret_segment_start = segment_start;
          if (ret_segment_end)   *ret_segment_end   = segment_end;
          if (ret_stroke)        *ret_stroke        = stroke;
        }
    }

  if (min_dist >= 0 &&
      gimp_draw_tool_on_handle (draw_tool, display,
                                coord->x,
                                coord->y,
                                GIMP_HANDLE_CIRCLE,
                                min_coords.x,
                                min_coords.y,
                                width, height,
                                GTK_ANCHOR_CENTER,
                                FALSE))
    {
      return TRUE;
    }

  return FALSE;
}

gboolean
gimp_draw_tool_on_vectors (GimpDrawTool      *draw_tool,
                           GimpDisplay       *display,
                           const GimpCoords  *coords,
                           gint               width,
                           gint               height,
                           GimpCoords        *ret_coords,
                           gdouble           *ret_pos,
                           GimpAnchor       **ret_segment_start,
                           GimpAnchor       **ret_segment_end,
                           GimpStroke       **ret_stroke,
                           GimpVectors      **ret_vectors)
{
  GList *all_vectors;
  GList *list;

  if (ret_coords)        *ret_coords         = *coords;
  if (ret_pos)           *ret_pos            = -1.0;
  if (ret_segment_start) *ret_segment_start  = NULL;
  if (ret_segment_end)   *ret_segment_end    = NULL;
  if (ret_stroke)        *ret_stroke         = NULL;
  if (ret_vectors)       *ret_vectors        = NULL;

  all_vectors = gimp_image_get_vectors_list (gimp_display_get_image (display));

  for (list = all_vectors; list; list = g_list_next (list))
    {
      GimpVectors *vectors = list->data;

      if (! gimp_item_get_visible (GIMP_ITEM (vectors)))
        continue;

      if (gimp_draw_tool_on_vectors_curve (draw_tool,
                                           display,
                                           vectors, coords,
                                           width, height,
                                           ret_coords,
                                           ret_pos,
                                           ret_segment_start,
                                           ret_segment_end,
                                           ret_stroke))
        {
          if (ret_vectors)
            *ret_vectors = vectors;

          g_list_free (all_vectors);

          return TRUE;
        }
    }

  g_list_free (all_vectors);

  return FALSE;
}


/*  private functions  */

static inline void
gimp_draw_tool_shift_to_north_west (gdouble        x,
                                    gdouble        y,
                                    gint           handle_width,
                                    gint           handle_height,
                                    GtkAnchorType  anchor,
                                    gdouble       *shifted_x,
                                    gdouble       *shifted_y)
{
  switch (anchor)
    {
    case GTK_ANCHOR_CENTER:
      x -= (handle_width >> 1);
      y -= (handle_height >> 1);
      break;

    case GTK_ANCHOR_NORTH:
      x -= (handle_width >> 1);
      break;

    case GTK_ANCHOR_NORTH_WEST:
      /*  nothing, this is the default  */
      break;

    case GTK_ANCHOR_NORTH_EAST:
      x -= handle_width;
      break;

    case GTK_ANCHOR_SOUTH:
      x -= (handle_width >> 1);
      y -= handle_height;
      break;

    case GTK_ANCHOR_SOUTH_WEST:
      y -= handle_height;
      break;

    case GTK_ANCHOR_SOUTH_EAST:
      x -= handle_width;
      y -= handle_height;
      break;

    case GTK_ANCHOR_WEST:
      y -= (handle_height >> 1);
      break;

    case GTK_ANCHOR_EAST:
      x -= handle_width;
      y -= (handle_height >> 1);
      break;

    default:
      break;
    }

  if (shifted_x)
    *shifted_x = x;

  if (shifted_y)
    *shifted_y = y;
}

static inline void
gimp_draw_tool_shift_to_center (gdouble        x,
                                gdouble        y,
                                gint           handle_width,
                                gint           handle_height,
                                GtkAnchorType  anchor,
                                gdouble       *shifted_x,
                                gdouble       *shifted_y)
{
  switch (anchor)
    {
    case GTK_ANCHOR_CENTER:
      /*  nothing, this is the default  */
      break;

    case GTK_ANCHOR_NORTH:
      y += (handle_height >> 1);
      break;

    case GTK_ANCHOR_NORTH_WEST:
      x += (handle_width >> 1);
      y += (handle_height >> 1);
      break;

    case GTK_ANCHOR_NORTH_EAST:
      x -= (handle_width >> 1);
      y += (handle_height >> 1);
      break;

    case GTK_ANCHOR_SOUTH:
      y -= (handle_height >> 1);
      break;

    case GTK_ANCHOR_SOUTH_WEST:
      x += (handle_width >> 1);
      y -= (handle_height >> 1);
      break;

    case GTK_ANCHOR_SOUTH_EAST:
      x -= (handle_width >> 1);
      y -= (handle_height >> 1);
      break;

    case GTK_ANCHOR_WEST:
      x += (handle_width >> 1);
      break;

    case GTK_ANCHOR_EAST:
      x -= (handle_width >> 1);
      break;

    default:
      break;
    }

  if (shifted_x)
    *shifted_x = x;

  if (shifted_y)
    *shifted_y = y;
}
