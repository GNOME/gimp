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

#include "core/gimpdrawable.h"
#include "core/gimpimage.h"

#include "vectors/gimpanchor.h"
#include "vectors/gimpstroke.h"
#include "vectors/gimpvectors.h"

#include "display/gimpcanvas.h"
#include "display/gimpcanvasarc.h"
#include "display/gimpcanvasboundary.h"
#include "display/gimpcanvascorner.h"
#include "display/gimpcanvasgroup.h"
#include "display/gimpcanvasguide.h"
#include "display/gimpcanvashandle.h"
#include "display/gimpcanvasitem-utils.h"
#include "display/gimpcanvasline.h"
#include "display/gimpcanvaspath.h"
#include "display/gimpcanvaspen.h"
#include "display/gimpcanvaspolygon.h"
#include "display/gimpcanvasrectangle.h"
#include "display/gimpcanvasrectangleguides.h"
#include "display/gimpcanvassamplepoint.h"
#include "display/gimpcanvastextcursor.h"
#include "display/gimpcanvastransformguides.h"
#include "display/gimpcanvastransformpreview.h"
#include "display/gimpdisplay.h"
#include "display/gimpdisplayshell.h"
#include "display/gimpdisplayshell-items.h"
#include "display/gimpdisplayshell-transform.h"

#include "gimpdrawtool.h"


#define DRAW_TIMEOUT              4
#define USE_TIMEOUT               1
#define MINIMUM_DRAW_INTERVAL 50000 /* 50000 microseconds == 20 fps */


static void          gimp_draw_tool_dispose      (GObject          *object);

static gboolean      gimp_draw_tool_has_display  (GimpTool         *tool,
                                                  GimpDisplay      *display);
static GimpDisplay * gimp_draw_tool_has_image    (GimpTool         *tool,
                                                  GimpImage        *image);
static void          gimp_draw_tool_control      (GimpTool         *tool,
                                                  GimpToolAction    action,
                                                  GimpDisplay      *display);

static void          gimp_draw_tool_draw         (GimpDrawTool     *draw_tool);
static void          gimp_draw_tool_undraw       (GimpDrawTool     *draw_tool);
static void          gimp_draw_tool_real_draw    (GimpDrawTool     *draw_tool);


G_DEFINE_TYPE (GimpDrawTool, gimp_draw_tool, GIMP_TYPE_TOOL)

#define parent_class gimp_draw_tool_parent_class


static void
gimp_draw_tool_class_init (GimpDrawToolClass *klass)
{
  GObjectClass  *object_class = G_OBJECT_CLASS (klass);
  GimpToolClass *tool_class   = GIMP_TOOL_CLASS (klass);

  object_class->dispose   = gimp_draw_tool_dispose;

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
  draw_tool->preview      = NULL;
  draw_tool->item         = NULL;
}

static void
gimp_draw_tool_dispose (GObject *object)
{
  GimpDrawTool *draw_tool = GIMP_DRAW_TOOL (object);

  if (draw_tool->draw_timeout)
    {
      g_source_remove (draw_tool->draw_timeout);
      draw_tool->draw_timeout = 0;
    }

  G_OBJECT_CLASS (parent_class)->dispose (object);
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
    case GIMP_TOOL_ACTION_RESUME:
      break;

    case GIMP_TOOL_ACTION_HALT:
      if (gimp_draw_tool_is_active (draw_tool))
        gimp_draw_tool_stop (draw_tool);
      break;

    case GIMP_TOOL_ACTION_COMMIT:
      break;
    }

  GIMP_TOOL_CLASS (parent_class)->control (tool, action, display);
}

#ifdef USE_TIMEOUT
static gboolean
gimp_draw_tool_draw_timeout (GimpDrawTool *draw_tool)
{
  guint64 now = g_get_monotonic_time ();

  /* keep the timeout running if the last drawing just happened */
  if ((now - draw_tool->last_draw_time) <= MINIMUM_DRAW_INTERVAL)
    return TRUE;

  draw_tool->draw_timeout = 0;

  gimp_draw_tool_draw (draw_tool);

  return FALSE;
}
#endif

static void
gimp_draw_tool_draw (GimpDrawTool *draw_tool)
{
  guint64 now = g_get_monotonic_time ();

  if (draw_tool->display &&
      draw_tool->paused_count == 0 &&
      (! draw_tool->draw_timeout ||
       (now - draw_tool->last_draw_time) > MINIMUM_DRAW_INTERVAL))
    {
      GimpDisplayShell *shell = gimp_display_get_shell (draw_tool->display);

      if (draw_tool->draw_timeout)
        {
          g_source_remove (draw_tool->draw_timeout);
          draw_tool->draw_timeout = 0;
        }

      gimp_draw_tool_undraw (draw_tool);

      GIMP_DRAW_TOOL_GET_CLASS (draw_tool)->draw (draw_tool);

      if (draw_tool->group_stack)
        {
          g_warning ("%s: draw_tool->group_stack not empty after calling "
                     "GimpDrawTool::draw() of %s",
                     G_STRFUNC,
                     g_type_name (G_TYPE_FROM_INSTANCE (draw_tool)));

          while (draw_tool->group_stack)
            gimp_draw_tool_pop_group (draw_tool);
        }

      if (draw_tool->preview)
        gimp_display_shell_add_preview_item (shell, draw_tool->preview);

      if (draw_tool->item)
        gimp_display_shell_add_tool_item (shell, draw_tool->item);

#if 0
      gimp_display_shell_flush (shell, TRUE);
#endif

      draw_tool->last_draw_time = g_get_monotonic_time ();

#if 0
      g_printerr ("drawing tool stuff took %f seconds\n",
                  (draw_tool->last_draw_time - now) / 1000000.0);
#endif
    }
}

static void
gimp_draw_tool_undraw (GimpDrawTool *draw_tool)
{
  if (draw_tool->display)
    {
      GimpDisplayShell *shell = gimp_display_get_shell (draw_tool->display);

      if (draw_tool->preview)
        {
          gimp_display_shell_remove_preview_item (shell, draw_tool->preview);
          g_object_unref (draw_tool->preview);
          draw_tool->preview = NULL;
        }

      if (draw_tool->item)
        {
          gimp_display_shell_remove_tool_item (shell, draw_tool->item);
          g_object_unref (draw_tool->item);
          draw_tool->item = NULL;
        }
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
  g_return_if_fail (gimp_draw_tool_is_active (draw_tool) == FALSE);

  draw_tool->display = display;

  gimp_draw_tool_draw (draw_tool);
}

void
gimp_draw_tool_stop (GimpDrawTool *draw_tool)
{
  g_return_if_fail (GIMP_IS_DRAW_TOOL (draw_tool));
  g_return_if_fail (gimp_draw_tool_is_active (draw_tool) == TRUE);

  gimp_draw_tool_undraw (draw_tool);

  if (draw_tool->draw_timeout)
    {
      g_source_remove (draw_tool->draw_timeout);
      draw_tool->draw_timeout = 0;
    }

  draw_tool->last_draw_time = 0;

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
  g_return_if_fail (draw_tool->paused_count > 0);

  draw_tool->paused_count--;

  if (draw_tool->paused_count == 0)
    {
#ifdef USE_TIMEOUT
      /* Don't install the timeout if the draw tool isn't active, so
       * suspend()/resume() can always be called, and have no side
       * effect on an inactive tool. See bug #687851.
       */
      if (gimp_draw_tool_is_active (draw_tool) && ! draw_tool->draw_timeout)
        {
          draw_tool->draw_timeout =
            gdk_threads_add_timeout_full (G_PRIORITY_HIGH_IDLE,
                                          DRAW_TIMEOUT,
                                          (GSourceFunc) gimp_draw_tool_draw_timeout,
                                          draw_tool, NULL);
        }
#endif

      /* call draw() anyway, it will do nothing if the timeout is
       * running, but will additionally check the drawing times to
       * ensure the minimum framerate
       */
      gimp_draw_tool_draw (draw_tool);
    }
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
 *          display coordinates
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

  gimp_display_shell_transform_xy_f (shell, x1, y1, &tx1, &ty1);
  gimp_display_shell_transform_xy_f (shell, x2, y2, &tx2, &ty2);

  return SQR (tx2 - tx1) + SQR (ty2 - ty1);
}

void
gimp_draw_tool_add_preview (GimpDrawTool   *draw_tool,
                            GimpCanvasItem *item)
{
  g_return_if_fail (GIMP_IS_DRAW_TOOL (draw_tool));
  g_return_if_fail (GIMP_IS_CANVAS_ITEM (item));

  if (! draw_tool->preview)
    draw_tool->preview =
      gimp_canvas_group_new (gimp_display_get_shell (draw_tool->display));

  gimp_canvas_group_add_item (GIMP_CANVAS_GROUP (draw_tool->preview), item);
}

void
gimp_draw_tool_remove_preview (GimpDrawTool   *draw_tool,
                               GimpCanvasItem *item)
{
  g_return_if_fail (GIMP_IS_DRAW_TOOL (draw_tool));
  g_return_if_fail (GIMP_IS_CANVAS_ITEM (item));
  g_return_if_fail (draw_tool->preview != NULL);

  gimp_canvas_group_remove_item (GIMP_CANVAS_GROUP (draw_tool->preview), item);
}

void
gimp_draw_tool_add_item (GimpDrawTool   *draw_tool,
                         GimpCanvasItem *item)
{
  GimpCanvasGroup *group;

  g_return_if_fail (GIMP_IS_DRAW_TOOL (draw_tool));
  g_return_if_fail (GIMP_IS_CANVAS_ITEM (item));

  if (! draw_tool->item)
    draw_tool->item =
      gimp_canvas_group_new (gimp_display_get_shell (draw_tool->display));

  group = GIMP_CANVAS_GROUP (draw_tool->item);

  if (draw_tool->group_stack)
    group = draw_tool->group_stack->data;

  gimp_canvas_group_add_item (group, item);
}

void
gimp_draw_tool_remove_item (GimpDrawTool   *draw_tool,
                            GimpCanvasItem *item)
{
  g_return_if_fail (GIMP_IS_DRAW_TOOL (draw_tool));
  g_return_if_fail (GIMP_IS_CANVAS_ITEM (item));
  g_return_if_fail (draw_tool->item != NULL);

  gimp_canvas_group_remove_item (GIMP_CANVAS_GROUP (draw_tool->item), item);
}

GimpCanvasGroup *
gimp_draw_tool_add_stroke_group (GimpDrawTool *draw_tool)
{
  GimpCanvasItem *item;

  g_return_val_if_fail (GIMP_IS_DRAW_TOOL (draw_tool), NULL);

  item = gimp_canvas_group_new (gimp_display_get_shell (draw_tool->display));
  gimp_canvas_group_set_group_stroking (GIMP_CANVAS_GROUP (item), TRUE);

  gimp_draw_tool_add_item (draw_tool, item);
  g_object_unref (item);

  return GIMP_CANVAS_GROUP (item);
}

GimpCanvasGroup *
gimp_draw_tool_add_fill_group (GimpDrawTool *draw_tool)
{
  GimpCanvasItem *item;

  g_return_val_if_fail (GIMP_IS_DRAW_TOOL (draw_tool), NULL);

  item = gimp_canvas_group_new (gimp_display_get_shell (draw_tool->display));
  gimp_canvas_group_set_group_filling (GIMP_CANVAS_GROUP (item), TRUE);

  gimp_draw_tool_add_item (draw_tool, item);
  g_object_unref (item);

  return GIMP_CANVAS_GROUP (item);
}

void
gimp_draw_tool_push_group (GimpDrawTool    *draw_tool,
                           GimpCanvasGroup *group)
{
  g_return_if_fail (GIMP_IS_DRAW_TOOL (draw_tool));
  g_return_if_fail (GIMP_IS_CANVAS_GROUP (group));

  draw_tool->group_stack = g_list_prepend (draw_tool->group_stack, group);
}

void
gimp_draw_tool_pop_group (GimpDrawTool *draw_tool)
{
  g_return_if_fail (GIMP_IS_DRAW_TOOL (draw_tool));
  g_return_if_fail (draw_tool->group_stack != NULL);

  draw_tool->group_stack = g_list_remove (draw_tool->group_stack,
                                          draw_tool->group_stack->data);
}

/**
 * gimp_draw_tool_add_line:
 * @draw_tool:   the #GimpDrawTool
 * @x1:          start point X in image coordinates
 * @y1:          start point Y in image coordinates
 * @x2:          end point X in image coordinates
 * @y2:          end point Y in image coordinates
 *
 * This function takes image space coordinates and transforms them to
 * screen window coordinates, then draws a line between the resulting
 * coordindates.
 **/
GimpCanvasItem *
gimp_draw_tool_add_line (GimpDrawTool *draw_tool,
                         gdouble       x1,
                         gdouble       y1,
                         gdouble       x2,
                         gdouble       y2)
{
  GimpCanvasItem *item;

  g_return_val_if_fail (GIMP_IS_DRAW_TOOL (draw_tool), NULL);

  item = gimp_canvas_line_new (gimp_display_get_shell (draw_tool->display),
                               x1, y1, x2, y2);

  gimp_draw_tool_add_item (draw_tool, item);
  g_object_unref (item);

  return item;
}

/**
 * gimp_draw_tool_add_guide:
 * @draw_tool:   the #GimpDrawTool
 * @orientation: the orientation of the guide line
 * @position:    the position of the guide line in image coordinates
 *
 * This function draws a guide line across the canvas.
 **/
GimpCanvasItem *
gimp_draw_tool_add_guide (GimpDrawTool        *draw_tool,
                          GimpOrientationType  orientation,
                          gint                 position,
                          GimpGuideStyle       style)
{
  GimpCanvasItem *item;

  g_return_val_if_fail (GIMP_IS_DRAW_TOOL (draw_tool), NULL);

  item = gimp_canvas_guide_new (gimp_display_get_shell (draw_tool->display),
                                orientation, position,
                                style);

  gimp_draw_tool_add_item (draw_tool, item);
  g_object_unref (item);

  return item;
}

/**
 * gimp_draw_tool_add_crosshair:
 * @draw_tool:  the #GimpDrawTool
 * @position_x: the position of the vertical guide line in image coordinates
 * @position_y: the position of the horizontal guide line in image coordinates
 *
 * This function draws two crossing guide lines across the canvas.
 **/
GimpCanvasItem *
gimp_draw_tool_add_crosshair (GimpDrawTool *draw_tool,
                              gint          position_x,
                              gint          position_y)
{
  GimpCanvasGroup *group;

  group = gimp_draw_tool_add_stroke_group (draw_tool);

  gimp_draw_tool_push_group (draw_tool, group);
  gimp_draw_tool_add_guide (draw_tool,
                            GIMP_ORIENTATION_VERTICAL, position_x,
                            GIMP_GUIDE_STYLE_NONE);
  gimp_draw_tool_add_guide (draw_tool,
                            GIMP_ORIENTATION_HORIZONTAL, position_y,
                            GIMP_GUIDE_STYLE_NONE);
  gimp_draw_tool_pop_group (draw_tool);

  return GIMP_CANVAS_ITEM (group);
}

/**
 * gimp_draw_tool_add_sample_point:
 * @draw_tool: the #GimpDrawTool
 * @x:         X position of the sample point
 * @y:         Y position of the sample point
 * @index:     Index of the sample point
 *
 * This function draws a sample point
 **/
GimpCanvasItem *
gimp_draw_tool_add_sample_point (GimpDrawTool *draw_tool,
                                 gint          x,
                                 gint          y,
                                 gint          index)
{
  GimpCanvasItem *item;

  g_return_val_if_fail (GIMP_IS_DRAW_TOOL (draw_tool), NULL);

  item = gimp_canvas_sample_point_new (gimp_display_get_shell (draw_tool->display),
                                       x, y, index, TRUE);

  gimp_draw_tool_add_item (draw_tool, item);
  g_object_unref (item);

  return item;
}

/**
 * gimp_draw_tool_add_rectangle:
 * @draw_tool:   the #GimpDrawTool
 * @filled:      whether to fill the rectangle
 * @x:           horizontal image coordinate
 * @y:           vertical image coordinate
 * @width:       width in image coordinates
 * @height:      height in image coordinates
 *
 * This function takes image space coordinates and transforms them to
 * screen window coordinates, then draws the resulting rectangle.
 **/
GimpCanvasItem *
gimp_draw_tool_add_rectangle (GimpDrawTool *draw_tool,
                              gboolean      filled,
                              gdouble       x,
                              gdouble       y,
                              gdouble       width,
                              gdouble       height)
{
  GimpCanvasItem *item;

  g_return_val_if_fail (GIMP_IS_DRAW_TOOL (draw_tool), NULL);

  item = gimp_canvas_rectangle_new (gimp_display_get_shell (draw_tool->display),
                                    x, y, width, height, filled);

  gimp_draw_tool_add_item (draw_tool, item);
  g_object_unref (item);

  return item;
}

GimpCanvasItem *
gimp_draw_tool_add_rectangle_guides (GimpDrawTool   *draw_tool,
                                     GimpGuidesType  type,
                                     gdouble         x,
                                     gdouble         y,
                                     gdouble         width,
                                     gdouble         height)
{
  GimpCanvasItem *item;

  g_return_val_if_fail (GIMP_IS_DRAW_TOOL (draw_tool), NULL);

  item = gimp_canvas_rectangle_guides_new (gimp_display_get_shell (draw_tool->display),
                                           x, y, width, height, type, 4);

  gimp_draw_tool_add_item (draw_tool, item);
  g_object_unref (item);

  return item;
}

GimpCanvasItem *
gimp_draw_tool_add_arc (GimpDrawTool *draw_tool,
                        gboolean      filled,
                        gdouble       x,
                        gdouble       y,
                        gdouble       width,
                        gdouble       height,
                        gdouble       start_angle,
                        gdouble       slice_angle)
{
  GimpCanvasItem *item;

  g_return_val_if_fail (GIMP_IS_DRAW_TOOL (draw_tool), NULL);

  item = gimp_canvas_arc_new (gimp_display_get_shell (draw_tool->display),
                              x + width  / 2.0,
                              y + height / 2.0,
                              width  / 2.0,
                              height / 2.0,
                              start_angle,
                              slice_angle,
                              filled);

  gimp_draw_tool_add_item (draw_tool, item);
  g_object_unref (item);

  return item;
}

GimpCanvasItem *
gimp_draw_tool_add_handle (GimpDrawTool     *draw_tool,
                           GimpHandleType    type,
                           gdouble           x,
                           gdouble           y,
                           gint              width,
                           gint              height,
                           GimpHandleAnchor  anchor)
{
  GimpCanvasItem *item;

  g_return_val_if_fail (GIMP_IS_DRAW_TOOL (draw_tool), NULL);

  item = gimp_canvas_handle_new (gimp_display_get_shell (draw_tool->display),
                                 type, anchor, x, y, width, height);

  gimp_draw_tool_add_item (draw_tool, item);
  g_object_unref (item);

  return item;
}

/**
 * gimp_draw_tool_add_corner:
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
 *
 * This function takes image space coordinates and transforms them to
 * screen window coordinates. It draws a corner into an already drawn
 * rectangle outline, taking care of not drawing over an already drawn line.
 **/
GimpCanvasItem *
gimp_draw_tool_add_corner (GimpDrawTool     *draw_tool,
                           gboolean          highlight,
                           gboolean          put_outside,
                           gdouble           x1,
                           gdouble           y1,
                           gdouble           x2,
                           gdouble           y2,
                           gint              width,
                           gint              height,
                           GimpHandleAnchor  anchor)
{
  GimpCanvasItem *item;

  g_return_val_if_fail (GIMP_IS_DRAW_TOOL (draw_tool), NULL);

  item = gimp_canvas_corner_new (gimp_display_get_shell (draw_tool->display),
                                 x1, y1, x2 - x1, y2 - y1,
                                 anchor, width, height, put_outside);
  gimp_canvas_item_set_highlight (item, highlight);

  gimp_draw_tool_add_item (draw_tool, item);
  g_object_unref (item);

  return item;
}

GimpCanvasItem *
gimp_draw_tool_add_lines (GimpDrawTool      *draw_tool,
                          const GimpVector2 *points,
                          gint               n_points,
                          GimpMatrix3       *transform,
                          gboolean           filled)
{
  GimpCanvasItem *item;

  g_return_val_if_fail (GIMP_IS_DRAW_TOOL (draw_tool), NULL);

  if (points == NULL || n_points < 2)
    return NULL;

  item = gimp_canvas_polygon_new (gimp_display_get_shell (draw_tool->display),
                                  points, n_points, transform, filled);

  gimp_draw_tool_add_item (draw_tool, item);
  g_object_unref (item);

  return item;
}

GimpCanvasItem *
gimp_draw_tool_add_strokes (GimpDrawTool     *draw_tool,
                            const GimpCoords *points,
                            gint              n_points,
                            GimpMatrix3      *transform,
                            gboolean          filled)
{
  GimpCanvasItem *item;

  g_return_val_if_fail (GIMP_IS_DRAW_TOOL (draw_tool), NULL);

  if (points == NULL || n_points < 2)
    return NULL;

  item = gimp_canvas_polygon_new_from_coords (gimp_display_get_shell (draw_tool->display),
                                              points, n_points, transform, filled);

  gimp_draw_tool_add_item (draw_tool, item);
  g_object_unref (item);

  return item;
}

GimpCanvasItem *
gimp_draw_tool_add_path (GimpDrawTool         *draw_tool,
                         const GimpBezierDesc *desc,
                         gdouble               x,
                         gdouble               y)
{
  GimpCanvasItem *item;

  g_return_val_if_fail (GIMP_IS_DRAW_TOOL (draw_tool), NULL);
  g_return_val_if_fail (desc != NULL, NULL);

  item = gimp_canvas_path_new (gimp_display_get_shell (draw_tool->display),
                               desc, x, y, FALSE, GIMP_PATH_STYLE_DEFAULT);

  gimp_draw_tool_add_item (draw_tool, item);
  g_object_unref (item);

  return item;
}

GimpCanvasItem *
gimp_draw_tool_add_pen (GimpDrawTool      *draw_tool,
                        const GimpVector2 *points,
                        gint               n_points,
                        GimpContext       *context,
                        GimpActiveColor    color,
                        gint               width)
{
  GimpCanvasItem *item;

  g_return_val_if_fail (GIMP_IS_DRAW_TOOL (draw_tool), NULL);

  if (points == NULL || n_points < 2)
    return NULL;

  item = gimp_canvas_pen_new (gimp_display_get_shell (draw_tool->display),
                              points, n_points, context, color, width);

  gimp_draw_tool_add_item (draw_tool, item);
  g_object_unref (item);

  return item;
}

/**
 * gimp_draw_tool_add_boundary:
 * @draw_tool:    a #GimpDrawTool
 * @bound_segs:   the sorted brush outline
 * @n_bound_segs: the number of segments in @bound_segs
 * @matrix:       transform matrix for the boundary
 * @offset_x:     x offset
 * @offset_y:     y offset
 *
 * Draw the boundary of the brush that @draw_tool uses. The boundary
 * should be sorted with sort_boundary(), and @n_bound_segs should
 * include the sentinel segments inserted by sort_boundary() that
 * indicate the end of connected segment sequences (groups) .
 */
GimpCanvasItem *
gimp_draw_tool_add_boundary (GimpDrawTool       *draw_tool,
                             const GimpBoundSeg *bound_segs,
                             gint                n_bound_segs,
                             GimpMatrix3        *transform,
                             gdouble             offset_x,
                             gdouble             offset_y)
{
  GimpCanvasItem *item;

  g_return_val_if_fail (GIMP_IS_DRAW_TOOL (draw_tool), NULL);
  g_return_val_if_fail (n_bound_segs > 0, NULL);
  g_return_val_if_fail (bound_segs != NULL, NULL);

  item = gimp_canvas_boundary_new (gimp_display_get_shell (draw_tool->display),
                                   bound_segs, n_bound_segs,
                                   transform,
                                   offset_x, offset_y);

  gimp_draw_tool_add_item (draw_tool, item);
  g_object_unref (item);

  return item;
}

GimpCanvasItem *
gimp_draw_tool_add_text_cursor (GimpDrawTool   *draw_tool,
                                PangoRectangle *cursor,
                                gboolean        overwrite)
{
  GimpCanvasItem *item;

  g_return_val_if_fail (GIMP_IS_DRAW_TOOL (draw_tool), NULL);

  item = gimp_canvas_text_cursor_new (gimp_display_get_shell (draw_tool->display),
                                      cursor, overwrite);

  gimp_draw_tool_add_item (draw_tool, item);
  g_object_unref (item);

  return item;
}

GimpCanvasItem *
gimp_draw_tool_add_transform_preview (GimpDrawTool      *draw_tool,
                                      GimpDrawable      *drawable,
                                      const GimpMatrix3 *transform,
                                      gdouble            x1,
                                      gdouble            y1,
                                      gdouble            x2,
                                      gdouble            y2,
                                      gboolean           perspective)
{
  GimpCanvasItem *item;

  g_return_val_if_fail (GIMP_IS_DRAW_TOOL (draw_tool), NULL);
  g_return_val_if_fail (GIMP_IS_DRAWABLE (drawable), NULL);
  g_return_val_if_fail (transform != NULL, NULL);

  item = gimp_canvas_transform_preview_new (gimp_display_get_shell (draw_tool->display),
                                            drawable, transform,
                                            x1, y1, x2, y2,
                                            perspective);

  gimp_draw_tool_add_preview (draw_tool, item);
  g_object_unref (item);

  return item;
}

gboolean
gimp_draw_tool_on_handle (GimpDrawTool     *draw_tool,
                          GimpDisplay      *display,
                          gdouble           x,
                          gdouble           y,
                          GimpHandleType    type,
                          gdouble           handle_x,
                          gdouble           handle_y,
                          gint              width,
                          gint              height,
                          GimpHandleAnchor  anchor)
{
  GimpDisplayShell *shell;
  gdouble           tx, ty;
  gdouble           handle_tx, handle_ty;

  g_return_val_if_fail (GIMP_IS_DRAW_TOOL (draw_tool), FALSE);
  g_return_val_if_fail (GIMP_IS_DISPLAY (display), FALSE);

  shell = gimp_display_get_shell (display);

  gimp_display_shell_zoom_xy_f (shell,
                                x, y,
                                &tx, &ty);
  gimp_display_shell_zoom_xy_f (shell,
                                handle_x, handle_y,
                                &handle_tx, &handle_ty);

  switch (type)
    {
    case GIMP_HANDLE_SQUARE:
    case GIMP_HANDLE_FILLED_SQUARE:
    case GIMP_HANDLE_CROSS:
    case GIMP_HANDLE_CROSSHAIR:
      gimp_canvas_item_shift_to_north_west (anchor,
                                            handle_tx, handle_ty,
                                            width, height,
                                            &handle_tx, &handle_ty);

      return (tx == CLAMP (tx, handle_tx, handle_tx + width) &&
              ty == CLAMP (ty, handle_ty, handle_ty + height));

    case GIMP_HANDLE_CIRCLE:
    case GIMP_HANDLE_FILLED_CIRCLE:
      gimp_canvas_item_shift_to_center (anchor,
                                        handle_tx, handle_ty,
                                        width, height,
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
      GList *anchor_list;
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
                                GIMP_HANDLE_ANCHOR_CENTER) &&
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
                                               GIMP_HANDLE_ANCHOR_CENTER))
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
                                     GIMP_HANDLE_ANCHOR_CENTER))
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
  if (ret_segment_end)   *ret_segment_end   = NULL;
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
                                GIMP_HANDLE_ANCHOR_CENTER))
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
