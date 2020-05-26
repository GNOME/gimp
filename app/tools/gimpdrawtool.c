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
 * along with this program.  If not, see <https://www.gnu.org/licenses/>.
 */

#include "config.h"

#include <string.h>

#include <gegl.h>
#include <gtk/gtk.h>

#include "libgimpmath/gimpmath.h"

#include "tools-types.h"

#include "core/gimpimage.h"
#include "core/gimppickable.h"

#include "display/gimpcanvas.h"
#include "display/gimpcanvasarc.h"
#include "display/gimpcanvasboundary.h"
#include "display/gimpcanvasgroup.h"
#include "display/gimpcanvasguide.h"
#include "display/gimpcanvashandle.h"
#include "display/gimpcanvasitem-utils.h"
#include "display/gimpcanvasline.h"
#include "display/gimpcanvaspen.h"
#include "display/gimpcanvaspolygon.h"
#include "display/gimpcanvasrectangle.h"
#include "display/gimpcanvassamplepoint.h"
#include "display/gimpcanvastextcursor.h"
#include "display/gimpcanvastransformpreview.h"
#include "display/gimpdisplay.h"
#include "display/gimpdisplayshell.h"
#include "display/gimpdisplayshell-items.h"
#include "display/gimpdisplayshell-transform.h"
#include "display/gimptoolwidget.h"

#include "gimpdrawtool.h"
#include "gimptoolcontrol.h"


#define USE_TIMEOUT
#define DRAW_FPS              120
#define DRAW_TIMEOUT          (1000 /* milliseconds */ / (2 * DRAW_FPS))
#define MINIMUM_DRAW_INTERVAL (G_TIME_SPAN_SECOND / DRAW_FPS)


static void          gimp_draw_tool_dispose       (GObject          *object);

static gboolean      gimp_draw_tool_has_display   (GimpTool         *tool,
                                                   GimpDisplay      *display);
static GimpDisplay * gimp_draw_tool_has_image     (GimpTool         *tool,
                                                   GimpImage        *image);
static void          gimp_draw_tool_control       (GimpTool         *tool,
                                                   GimpToolAction    action,
                                                   GimpDisplay      *display);
static gboolean      gimp_draw_tool_key_press     (GimpTool         *tool,
                                                   GdkEventKey      *kevent,
                                                   GimpDisplay      *display);
static gboolean      gimp_draw_tool_key_release   (GimpTool         *tool,
                                                   GdkEventKey      *kevent,
                                                   GimpDisplay      *display);
static void          gimp_draw_tool_modifier_key  (GimpTool         *tool,
                                                   GdkModifierType   key,
                                                   gboolean          press,
                                                   GdkModifierType   state,
                                                   GimpDisplay      *display);
static void          gimp_draw_tool_active_modifier_key
                                                  (GimpTool         *tool,
                                                   GdkModifierType   key,
                                                   gboolean          press,
                                                   GdkModifierType   state,
                                                   GimpDisplay      *display);
static void          gimp_draw_tool_oper_update   (GimpTool         *tool,
                                                   const GimpCoords *coords,
                                                   GdkModifierType   state,
                                                   gboolean          proximity,
                                                   GimpDisplay      *display);
static void          gimp_draw_tool_cursor_update (GimpTool         *tool,
                                                   const GimpCoords *coords,
                                                   GdkModifierType   state,
                                                   GimpDisplay      *display);

static void          gimp_draw_tool_widget_status (GimpToolWidget   *widget,
                                                   const gchar      *status,
                                                   GimpTool         *tool);
static void          gimp_draw_tool_widget_status_coords
                                                  (GimpToolWidget   *widget,
                                                   const gchar      *title,
                                                   gdouble           x,
                                                   const gchar      *separator,
                                                   gdouble           y,
                                                   const gchar      *help,
                                                   GimpTool         *tool);
static void          gimp_draw_tool_widget_message
                                                  (GimpToolWidget   *widget,
                                                   const gchar      *message,
                                                   GimpTool         *tool);
static void          gimp_draw_tool_widget_snap_offsets
                                                  (GimpToolWidget   *widget,
                                                   gint              offset_x,
                                                   gint              offset_y,
                                                   gint              width,
                                                   gint              height,
                                                   GimpTool         *tool);

static void          gimp_draw_tool_draw          (GimpDrawTool     *draw_tool);
static void          gimp_draw_tool_undraw        (GimpDrawTool     *draw_tool);
static void          gimp_draw_tool_real_draw     (GimpDrawTool     *draw_tool);


G_DEFINE_TYPE (GimpDrawTool, gimp_draw_tool, GIMP_TYPE_TOOL)

#define parent_class gimp_draw_tool_parent_class


static void
gimp_draw_tool_class_init (GimpDrawToolClass *klass)
{
  GObjectClass  *object_class = G_OBJECT_CLASS (klass);
  GimpToolClass *tool_class   = GIMP_TOOL_CLASS (klass);

  object_class->dispose           = gimp_draw_tool_dispose;

  tool_class->has_display         = gimp_draw_tool_has_display;
  tool_class->has_image           = gimp_draw_tool_has_image;
  tool_class->control             = gimp_draw_tool_control;
  tool_class->key_press           = gimp_draw_tool_key_press;
  tool_class->key_release         = gimp_draw_tool_key_release;
  tool_class->modifier_key        = gimp_draw_tool_modifier_key;
  tool_class->active_modifier_key = gimp_draw_tool_active_modifier_key;
  tool_class->oper_update         = gimp_draw_tool_oper_update;
  tool_class->cursor_update       = gimp_draw_tool_cursor_update;

  klass->draw                     = gimp_draw_tool_real_draw;
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

  gimp_draw_tool_set_widget (draw_tool, NULL);
  gimp_draw_tool_set_default_status (draw_tool, NULL);

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
      gimp_draw_tool_set_widget (draw_tool, NULL);
      break;

    case GIMP_TOOL_ACTION_COMMIT:
      break;
    }

  GIMP_TOOL_CLASS (parent_class)->control (tool, action, display);
}

static gboolean
gimp_draw_tool_key_press (GimpTool    *tool,
                          GdkEventKey *kevent,
                          GimpDisplay *display)
{
  GimpDrawTool *draw_tool = GIMP_DRAW_TOOL (tool);

  if (draw_tool->widget && display == draw_tool->display)
    {
      return gimp_tool_widget_key_press (draw_tool->widget, kevent);
    }

  return GIMP_TOOL_CLASS (parent_class)->key_press (tool, kevent, display);
}

static gboolean
gimp_draw_tool_key_release (GimpTool    *tool,
                            GdkEventKey *kevent,
                            GimpDisplay *display)
{
  GimpDrawTool *draw_tool = GIMP_DRAW_TOOL (tool);

  if (draw_tool->widget && display == draw_tool->display)
    {
      return gimp_tool_widget_key_release (draw_tool->widget, kevent);
    }

  return GIMP_TOOL_CLASS (parent_class)->key_release (tool, kevent, display);
}

static void
gimp_draw_tool_modifier_key (GimpTool        *tool,
                             GdkModifierType  key,
                             gboolean         press,
                             GdkModifierType  state,
                             GimpDisplay     *display)
{
  GimpDrawTool *draw_tool = GIMP_DRAW_TOOL (tool);

  if (draw_tool->widget && display == draw_tool->display)
    {
      gimp_tool_widget_hover_modifier (draw_tool->widget, key, press, state);
    }

  GIMP_TOOL_CLASS (parent_class)->modifier_key (tool, key, press, state,
                                                display);
}

static void
gimp_draw_tool_active_modifier_key (GimpTool        *tool,
                                    GdkModifierType  key,
                                    gboolean         press,
                                    GdkModifierType  state,
                                    GimpDisplay     *display)
{
  GimpDrawTool *draw_tool = GIMP_DRAW_TOOL (tool);

  if (draw_tool->widget && display == draw_tool->display)
    {
      gimp_tool_widget_motion_modifier (draw_tool->widget, key, press, state);
    }

  GIMP_TOOL_CLASS (parent_class)->active_modifier_key (tool, key, press, state,
                                                       display);
}

static void
gimp_draw_tool_oper_update (GimpTool         *tool,
                            const GimpCoords *coords,
                            GdkModifierType   state,
                            gboolean          proximity,
                            GimpDisplay      *display)
{
  GimpDrawTool *draw_tool = GIMP_DRAW_TOOL (tool);

  if (draw_tool->widget && display == draw_tool->display)
    {
      gimp_tool_widget_hover (draw_tool->widget, coords, state, proximity);
    }
  else if (proximity && draw_tool->default_status)
    {
      gimp_tool_replace_status (tool, display, "%s", draw_tool->default_status);
    }
  else if (! proximity)
    {
      gimp_tool_pop_status (tool, display);
    }
  else
    {
      GIMP_TOOL_CLASS (parent_class)->oper_update (tool, coords, state,
                                                   proximity, display);
    }
}

static void
gimp_draw_tool_cursor_update (GimpTool         *tool,
                              const GimpCoords *coords,
                              GdkModifierType   state,
                              GimpDisplay      *display)
{
  GimpDrawTool *draw_tool = GIMP_DRAW_TOOL (tool);

  if (draw_tool->widget && display == draw_tool->display)
    {
      GimpCursorType     cursor;
      GimpToolCursorType tool_cursor;
      GimpCursorModifier modifier;

      cursor      = gimp_tool_control_get_cursor (tool->control);
      tool_cursor = gimp_tool_control_get_tool_cursor (tool->control);
      modifier    = gimp_tool_control_get_cursor_modifier (tool->control);

      if (gimp_tool_widget_get_cursor (draw_tool->widget, coords, state,
                                       &cursor, &tool_cursor, &modifier))
        {
          gimp_tool_set_cursor (tool, display,
                                cursor, tool_cursor, modifier);
          return;
        }
    }

  GIMP_TOOL_CLASS (parent_class)->cursor_update (tool, coords, state,
                                                 display);
}

static void
gimp_draw_tool_widget_status (GimpToolWidget *widget,
                              const gchar    *status,
                              GimpTool       *tool)
{
  GimpDrawTool *draw_tool = GIMP_DRAW_TOOL (tool);

  if (gimp_draw_tool_is_active (draw_tool))
    {
      if (status)
        gimp_tool_replace_status (tool, draw_tool->display, "%s", status);
      else
        gimp_tool_pop_status (tool, draw_tool->display);
    }
}

static void
gimp_draw_tool_widget_status_coords (GimpToolWidget *widget,
                                     const gchar    *title,
                                     gdouble         x,
                                     const gchar    *separator,
                                     gdouble         y,
                                     const gchar    *help,
                                     GimpTool       *tool)
{
  GimpDrawTool *draw_tool = GIMP_DRAW_TOOL (tool);

  if (gimp_draw_tool_is_active (draw_tool))
    {
      gimp_tool_pop_status (tool, draw_tool->display);
      gimp_tool_push_status_coords (tool, draw_tool->display,
                                    gimp_tool_control_get_precision (
                                      tool->control),
                                    title, x, separator, y, help);
    }
}

static void
gimp_draw_tool_widget_message (GimpToolWidget *widget,
                               const gchar    *message,
                               GimpTool       *tool)
{
  GimpDrawTool *draw_tool = GIMP_DRAW_TOOL (tool);

  if (gimp_draw_tool_is_active (draw_tool))
    gimp_tool_message_literal (tool, draw_tool->display, message);
}

static void
gimp_draw_tool_widget_snap_offsets (GimpToolWidget   *widget,
                                    gint              offset_x,
                                    gint              offset_y,
                                    gint              width,
                                    gint              height,
                                    GimpTool         *tool)
{
  gimp_tool_control_set_snap_offsets (tool->control,
                                      offset_x, offset_y,
                                      width, height);
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
          g_clear_object (&draw_tool->preview);
        }

      if (draw_tool->item)
        {
          gimp_display_shell_remove_tool_item (shell, draw_tool->item);
          g_clear_object (&draw_tool->item);
        }
    }
}

static void
gimp_draw_tool_real_draw (GimpDrawTool *draw_tool)
{
  if (draw_tool->widget)
    {
      GimpCanvasItem *item = gimp_tool_widget_get_item (draw_tool->widget);

      gimp_draw_tool_add_item (draw_tool, item);
    }
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
gimp_draw_tool_set_widget (GimpDrawTool   *draw_tool,
                           GimpToolWidget *widget)
{
  g_return_if_fail (GIMP_IS_DRAW_TOOL (draw_tool));
  g_return_if_fail (widget == NULL || GIMP_IS_TOOL_WIDGET (widget));

  if (widget == draw_tool->widget)
    return;

  if (draw_tool->widget)
    {
      gimp_tool_widget_set_focus (draw_tool->widget, FALSE);

      g_signal_handlers_disconnect_by_func (draw_tool->widget,
                                            gimp_draw_tool_widget_status,
                                            draw_tool);
      g_signal_handlers_disconnect_by_func (draw_tool->widget,
                                            gimp_draw_tool_widget_status_coords,
                                            draw_tool);
      g_signal_handlers_disconnect_by_func (draw_tool->widget,
                                            gimp_draw_tool_widget_message,
                                            draw_tool);
      g_signal_handlers_disconnect_by_func (draw_tool->widget,
                                            gimp_draw_tool_widget_snap_offsets,
                                            draw_tool);

      if (gimp_draw_tool_is_active (draw_tool))
        {
          GimpCanvasItem *item = gimp_tool_widget_get_item (draw_tool->widget);

          gimp_draw_tool_remove_item (draw_tool, item);
        }

      g_object_unref (draw_tool->widget);
    }

  draw_tool->widget = widget;

  if (draw_tool->widget)
    {
      g_object_ref (draw_tool->widget);

      if (gimp_draw_tool_is_active (draw_tool))
        {
          GimpCanvasItem *item = gimp_tool_widget_get_item (draw_tool->widget);

          gimp_draw_tool_add_item (draw_tool, item);
        }

      g_signal_connect (draw_tool->widget, "status",
                        G_CALLBACK (gimp_draw_tool_widget_status),
                        draw_tool);
      g_signal_connect (draw_tool->widget, "status-coords",
                        G_CALLBACK (gimp_draw_tool_widget_status_coords),
                        draw_tool);
      g_signal_connect (draw_tool->widget, "message",
                        G_CALLBACK (gimp_draw_tool_widget_message),
                        draw_tool);
      g_signal_connect (draw_tool->widget, "snap-offsets",
                        G_CALLBACK (gimp_draw_tool_widget_snap_offsets),
                        draw_tool);

      gimp_tool_widget_set_focus (draw_tool->widget, TRUE);
    }
}

void
gimp_draw_tool_set_default_status (GimpDrawTool *draw_tool,
                                   const gchar  *status)
{
  g_return_if_fail (GIMP_IS_DRAW_TOOL (draw_tool));

  if (draw_tool->default_status)
    g_free (draw_tool->default_status);

  draw_tool->default_status = g_strdup (status);
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
gimp_draw_tool_add_text_cursor (GimpDrawTool     *draw_tool,
                                PangoRectangle   *cursor,
                                gboolean          overwrite,
                                GimpTextDirection direction)
{
  GimpCanvasItem *item;

  g_return_val_if_fail (GIMP_IS_DRAW_TOOL (draw_tool), NULL);

  item = gimp_canvas_text_cursor_new (gimp_display_get_shell (draw_tool->display),
                                      cursor, overwrite, direction);

  gimp_draw_tool_add_item (draw_tool, item);
  g_object_unref (item);

  return item;
}

GimpCanvasItem *
gimp_draw_tool_add_transform_preview (GimpDrawTool      *draw_tool,
                                      GimpPickable      *pickable,
                                      const GimpMatrix3 *transform,
                                      gdouble            x1,
                                      gdouble            y1,
                                      gdouble            x2,
                                      gdouble            y2)
{
  GimpCanvasItem *item;

  g_return_val_if_fail (GIMP_IS_DRAW_TOOL (draw_tool), NULL);
  g_return_val_if_fail (GIMP_IS_PICKABLE (pickable), NULL);
  g_return_val_if_fail (transform != NULL, NULL);

  item = gimp_canvas_transform_preview_new (gimp_display_get_shell (draw_tool->display),
                                            pickable, transform,
                                            x1, y1, x2, y2);

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
