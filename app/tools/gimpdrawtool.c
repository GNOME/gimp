/* LIGMA - The GNU Image Manipulation Program
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

#include "libligmamath/ligmamath.h"

#include "tools-types.h"

#include "core/ligmaimage.h"
#include "core/ligmapickable.h"

#include "display/ligmacanvas.h"
#include "display/ligmacanvasarc.h"
#include "display/ligmacanvasboundary.h"
#include "display/ligmacanvasgroup.h"
#include "display/ligmacanvasguide.h"
#include "display/ligmacanvashandle.h"
#include "display/ligmacanvasitem-utils.h"
#include "display/ligmacanvasline.h"
#include "display/ligmacanvaspen.h"
#include "display/ligmacanvaspolygon.h"
#include "display/ligmacanvasrectangle.h"
#include "display/ligmacanvassamplepoint.h"
#include "display/ligmacanvastextcursor.h"
#include "display/ligmacanvastransformpreview.h"
#include "display/ligmadisplay.h"
#include "display/ligmadisplayshell.h"
#include "display/ligmadisplayshell-items.h"
#include "display/ligmadisplayshell-transform.h"
#include "display/ligmatoolwidget.h"

#include "ligmadrawtool.h"
#include "ligmatoolcontrol.h"


#define USE_TIMEOUT
#define DRAW_FPS              120
#define DRAW_TIMEOUT          (1000 /* milliseconds */ / (2 * DRAW_FPS))
#define MINIMUM_DRAW_INTERVAL (G_TIME_SPAN_SECOND / DRAW_FPS)


static void            ligma_draw_tool_dispose       (GObject           *object);

static gboolean        ligma_draw_tool_has_display   (LigmaTool          *tool,
                                                     LigmaDisplay       *display);
static LigmaDisplay   * ligma_draw_tool_has_image     (LigmaTool          *tool,
                                                     LigmaImage         *image);
static void            ligma_draw_tool_control       (LigmaTool          *tool,
                                                     LigmaToolAction     action,
                                                     LigmaDisplay       *display);
static gboolean        ligma_draw_tool_key_press     (LigmaTool          *tool,
                                                     GdkEventKey       *kevent,
                                                     LigmaDisplay       *display);
static gboolean        ligma_draw_tool_key_release   (LigmaTool          *tool,
                                                     GdkEventKey       *kevent,
                                                     LigmaDisplay       *display);
static void            ligma_draw_tool_modifier_key  (LigmaTool          *tool,
                                                     GdkModifierType    key,
                                                     gboolean           press,
                                                     GdkModifierType    state,
                                                     LigmaDisplay       *display);
static void            ligma_draw_tool_active_modifier_key
                                                    (LigmaTool          *tool,
                                                     GdkModifierType    key,
                                                     gboolean           press,
                                                     GdkModifierType    state,
                                                     LigmaDisplay       *display);
static void            ligma_draw_tool_oper_update   (LigmaTool          *tool,
                                                     const LigmaCoords  *coords,
                                                     GdkModifierType    state,
                                                     gboolean           proximity,
                                                     LigmaDisplay       *display);
static void            ligma_draw_tool_cursor_update (LigmaTool          *tool,
                                                     const LigmaCoords  *coords,
                                                     GdkModifierType    state,
                                                     LigmaDisplay       *display);
static LigmaUIManager * ligma_draw_tool_get_popup     (LigmaTool          *tool,
                                                     const LigmaCoords  *coords,
                                                     GdkModifierType    state,
                                                     LigmaDisplay       *display,
                                                     const gchar      **ui_path);

static void            ligma_draw_tool_widget_status (LigmaToolWidget    *widget,
                                                     const gchar       *status,
                                                     LigmaTool          *tool);
static void            ligma_draw_tool_widget_status_coords
                                                    (LigmaToolWidget    *widget,
                                                     const gchar       *title,
                                                     gdouble            x,
                                                     const gchar       *separator,
                                                     gdouble            y,
                                                     const gchar       *help,
                                                     LigmaTool          *tool);
static void            ligma_draw_tool_widget_message
                                                    (LigmaToolWidget    *widget,
                                                     const gchar       *message,
                                                     LigmaTool          *tool);
static void            ligma_draw_tool_widget_snap_offsets
                                                    (LigmaToolWidget    *widget,
                                                     gint               offset_x,
                                                     gint               offset_y,
                                                     gint               width,
                                                     gint               height,
                                                     LigmaTool          *tool);

static void            ligma_draw_tool_draw          (LigmaDrawTool      *draw_tool);
static void            ligma_draw_tool_undraw        (LigmaDrawTool      *draw_tool);
static void            ligma_draw_tool_real_draw     (LigmaDrawTool      *draw_tool);


G_DEFINE_TYPE (LigmaDrawTool, ligma_draw_tool, LIGMA_TYPE_TOOL)

#define parent_class ligma_draw_tool_parent_class


static void
ligma_draw_tool_class_init (LigmaDrawToolClass *klass)
{
  GObjectClass  *object_class = G_OBJECT_CLASS (klass);
  LigmaToolClass *tool_class   = LIGMA_TOOL_CLASS (klass);

  object_class->dispose           = ligma_draw_tool_dispose;

  tool_class->has_display         = ligma_draw_tool_has_display;
  tool_class->has_image           = ligma_draw_tool_has_image;
  tool_class->control             = ligma_draw_tool_control;
  tool_class->key_press           = ligma_draw_tool_key_press;
  tool_class->key_release         = ligma_draw_tool_key_release;
  tool_class->modifier_key        = ligma_draw_tool_modifier_key;
  tool_class->active_modifier_key = ligma_draw_tool_active_modifier_key;
  tool_class->oper_update         = ligma_draw_tool_oper_update;
  tool_class->cursor_update       = ligma_draw_tool_cursor_update;
  tool_class->get_popup           = ligma_draw_tool_get_popup;

  klass->draw                     = ligma_draw_tool_real_draw;
}

static void
ligma_draw_tool_init (LigmaDrawTool *draw_tool)
{
  draw_tool->display      = NULL;
  draw_tool->paused_count = 0;
  draw_tool->preview      = NULL;
  draw_tool->item         = NULL;
}

static void
ligma_draw_tool_dispose (GObject *object)
{
  LigmaDrawTool *draw_tool = LIGMA_DRAW_TOOL (object);

  if (draw_tool->draw_timeout)
    {
      g_source_remove (draw_tool->draw_timeout);
      draw_tool->draw_timeout = 0;
    }

  ligma_draw_tool_set_widget (draw_tool, NULL);
  ligma_draw_tool_set_default_status (draw_tool, NULL);

  G_OBJECT_CLASS (parent_class)->dispose (object);
}

static gboolean
ligma_draw_tool_has_display (LigmaTool    *tool,
                            LigmaDisplay *display)
{
  LigmaDrawTool *draw_tool = LIGMA_DRAW_TOOL (tool);

  return (display == draw_tool->display ||
          LIGMA_TOOL_CLASS (parent_class)->has_display (tool, display));
}

static LigmaDisplay *
ligma_draw_tool_has_image (LigmaTool  *tool,
                          LigmaImage *image)
{
  LigmaDrawTool *draw_tool = LIGMA_DRAW_TOOL (tool);
  LigmaDisplay  *display;

  display = LIGMA_TOOL_CLASS (parent_class)->has_image (tool, image);

  if (! display && draw_tool->display)
    {
      if (image && ligma_display_get_image (draw_tool->display) == image)
        display = draw_tool->display;

      /*  NULL image means any display  */
      if (! image)
        display = draw_tool->display;
    }

  return display;
}

static void
ligma_draw_tool_control (LigmaTool       *tool,
                        LigmaToolAction  action,
                        LigmaDisplay    *display)
{
  LigmaDrawTool *draw_tool = LIGMA_DRAW_TOOL (tool);

  switch (action)
    {
    case LIGMA_TOOL_ACTION_PAUSE:
    case LIGMA_TOOL_ACTION_RESUME:
      break;

    case LIGMA_TOOL_ACTION_HALT:
      if (ligma_draw_tool_is_active (draw_tool))
        ligma_draw_tool_stop (draw_tool);
      ligma_draw_tool_set_widget (draw_tool, NULL);
      break;

    case LIGMA_TOOL_ACTION_COMMIT:
      break;
    }

  LIGMA_TOOL_CLASS (parent_class)->control (tool, action, display);
}

static gboolean
ligma_draw_tool_key_press (LigmaTool    *tool,
                          GdkEventKey *kevent,
                          LigmaDisplay *display)
{
  LigmaDrawTool *draw_tool = LIGMA_DRAW_TOOL (tool);

  if (draw_tool->widget && display == draw_tool->display)
    {
      return ligma_tool_widget_key_press (draw_tool->widget, kevent);
    }

  return LIGMA_TOOL_CLASS (parent_class)->key_press (tool, kevent, display);
}

static gboolean
ligma_draw_tool_key_release (LigmaTool    *tool,
                            GdkEventKey *kevent,
                            LigmaDisplay *display)
{
  LigmaDrawTool *draw_tool = LIGMA_DRAW_TOOL (tool);

  if (draw_tool->widget && display == draw_tool->display)
    {
      return ligma_tool_widget_key_release (draw_tool->widget, kevent);
    }

  return LIGMA_TOOL_CLASS (parent_class)->key_release (tool, kevent, display);
}

static void
ligma_draw_tool_modifier_key (LigmaTool        *tool,
                             GdkModifierType  key,
                             gboolean         press,
                             GdkModifierType  state,
                             LigmaDisplay     *display)
{
  LigmaDrawTool *draw_tool = LIGMA_DRAW_TOOL (tool);

  if (draw_tool->widget && display == draw_tool->display)
    {
      ligma_tool_widget_hover_modifier (draw_tool->widget, key, press, state);
    }

  LIGMA_TOOL_CLASS (parent_class)->modifier_key (tool, key, press, state,
                                                display);
}

static void
ligma_draw_tool_active_modifier_key (LigmaTool        *tool,
                                    GdkModifierType  key,
                                    gboolean         press,
                                    GdkModifierType  state,
                                    LigmaDisplay     *display)
{
  LigmaDrawTool *draw_tool = LIGMA_DRAW_TOOL (tool);

  if (draw_tool->widget && display == draw_tool->display)
    {
      ligma_tool_widget_motion_modifier (draw_tool->widget, key, press, state);
    }

  LIGMA_TOOL_CLASS (parent_class)->active_modifier_key (tool, key, press, state,
                                                       display);
}

static void
ligma_draw_tool_oper_update (LigmaTool         *tool,
                            const LigmaCoords *coords,
                            GdkModifierType   state,
                            gboolean          proximity,
                            LigmaDisplay      *display)
{
  LigmaDrawTool *draw_tool = LIGMA_DRAW_TOOL (tool);

  if (draw_tool->widget && display == draw_tool->display)
    {
      ligma_tool_widget_hover (draw_tool->widget, coords, state, proximity);
    }
  else if (proximity && draw_tool->default_status)
    {
      ligma_tool_replace_status (tool, display, "%s", draw_tool->default_status);
    }
  else if (! proximity)
    {
      ligma_tool_pop_status (tool, display);
    }
  else
    {
      LIGMA_TOOL_CLASS (parent_class)->oper_update (tool, coords, state,
                                                   proximity, display);
    }
}

static void
ligma_draw_tool_cursor_update (LigmaTool         *tool,
                              const LigmaCoords *coords,
                              GdkModifierType   state,
                              LigmaDisplay      *display)
{
  LigmaDrawTool *draw_tool = LIGMA_DRAW_TOOL (tool);

  if (draw_tool->widget && display == draw_tool->display)
    {
      LigmaCursorType     cursor;
      LigmaToolCursorType tool_cursor;
      LigmaCursorModifier modifier;

      cursor      = ligma_tool_control_get_cursor (tool->control);
      tool_cursor = ligma_tool_control_get_tool_cursor (tool->control);
      modifier    = ligma_tool_control_get_cursor_modifier (tool->control);

      if (ligma_tool_widget_get_cursor (draw_tool->widget, coords, state,
                                       &cursor, &tool_cursor, &modifier))
        {
          ligma_tool_set_cursor (tool, display,
                                cursor, tool_cursor, modifier);
          return;
        }
    }

  LIGMA_TOOL_CLASS (parent_class)->cursor_update (tool, coords, state,
                                                 display);
}

static LigmaUIManager *
ligma_draw_tool_get_popup (LigmaTool          *tool,
                          const LigmaCoords  *coords,
                          GdkModifierType    state,
                          LigmaDisplay       *display,
                          const gchar      **ui_path)
{
  LigmaDrawTool *draw_tool = LIGMA_DRAW_TOOL (tool);

  if (draw_tool->widget && display == draw_tool->display)
    {
      LigmaUIManager *ui_manager;

      ui_manager = ligma_tool_widget_get_popup (draw_tool->widget,
                                               coords, state,
                                               ui_path);

      if (ui_manager)
        return ui_manager;
    }

  return LIGMA_TOOL_CLASS (parent_class)->get_popup (tool,
                                                    coords, state, display,
                                                    ui_path);
}

static void
ligma_draw_tool_widget_status (LigmaToolWidget *widget,
                              const gchar    *status,
                              LigmaTool       *tool)
{
  LigmaDrawTool *draw_tool = LIGMA_DRAW_TOOL (tool);

  if (ligma_draw_tool_is_active (draw_tool))
    {
      if (status)
        ligma_tool_replace_status (tool, draw_tool->display, "%s", status);
      else
        ligma_tool_pop_status (tool, draw_tool->display);
    }
}

static void
ligma_draw_tool_widget_status_coords (LigmaToolWidget *widget,
                                     const gchar    *title,
                                     gdouble         x,
                                     const gchar    *separator,
                                     gdouble         y,
                                     const gchar    *help,
                                     LigmaTool       *tool)
{
  LigmaDrawTool *draw_tool = LIGMA_DRAW_TOOL (tool);

  if (ligma_draw_tool_is_active (draw_tool))
    {
      ligma_tool_pop_status (tool, draw_tool->display);
      ligma_tool_push_status_coords (tool, draw_tool->display,
                                    ligma_tool_control_get_precision (
                                      tool->control),
                                    title, x, separator, y, help);
    }
}

static void
ligma_draw_tool_widget_message (LigmaToolWidget *widget,
                               const gchar    *message,
                               LigmaTool       *tool)
{
  LigmaDrawTool *draw_tool = LIGMA_DRAW_TOOL (tool);

  if (ligma_draw_tool_is_active (draw_tool))
    ligma_tool_message_literal (tool, draw_tool->display, message);
}

static void
ligma_draw_tool_widget_snap_offsets (LigmaToolWidget   *widget,
                                    gint              offset_x,
                                    gint              offset_y,
                                    gint              width,
                                    gint              height,
                                    LigmaTool         *tool)
{
  ligma_tool_control_set_snap_offsets (tool->control,
                                      offset_x, offset_y,
                                      width, height);
}

#ifdef USE_TIMEOUT
static gboolean
ligma_draw_tool_draw_timeout (LigmaDrawTool *draw_tool)
{
  guint64 now = g_get_monotonic_time ();

  /* keep the timeout running if the last drawing just happened */
  if ((now - draw_tool->last_draw_time) <= MINIMUM_DRAW_INTERVAL)
    return TRUE;

  draw_tool->draw_timeout = 0;

  ligma_draw_tool_draw (draw_tool);

  return FALSE;
}
#endif

static void
ligma_draw_tool_draw (LigmaDrawTool *draw_tool)
{
  guint64 now = g_get_monotonic_time ();

  if (draw_tool->display &&
      draw_tool->paused_count == 0 &&
      (! draw_tool->draw_timeout ||
       (now - draw_tool->last_draw_time) > MINIMUM_DRAW_INTERVAL))
    {
      LigmaDisplayShell *shell = ligma_display_get_shell (draw_tool->display);

      if (draw_tool->draw_timeout)
        {
          g_source_remove (draw_tool->draw_timeout);
          draw_tool->draw_timeout = 0;
        }

      ligma_draw_tool_undraw (draw_tool);

      LIGMA_DRAW_TOOL_GET_CLASS (draw_tool)->draw (draw_tool);

      if (draw_tool->group_stack)
        {
          g_warning ("%s: draw_tool->group_stack not empty after calling "
                     "LigmaDrawTool::draw() of %s",
                     G_STRFUNC,
                     g_type_name (G_TYPE_FROM_INSTANCE (draw_tool)));

          while (draw_tool->group_stack)
            ligma_draw_tool_pop_group (draw_tool);
        }

      if (draw_tool->preview)
        ligma_display_shell_add_preview_item (shell, draw_tool->preview);

      if (draw_tool->item)
        ligma_display_shell_add_tool_item (shell, draw_tool->item);

      draw_tool->last_draw_time = g_get_monotonic_time ();

#if 0
      g_printerr ("drawing tool stuff took %f seconds\n",
                  (draw_tool->last_draw_time - now) / 1000000.0);
#endif
    }
}

static void
ligma_draw_tool_undraw (LigmaDrawTool *draw_tool)
{
  if (draw_tool->display)
    {
      LigmaDisplayShell *shell = ligma_display_get_shell (draw_tool->display);

      if (draw_tool->preview)
        {
          ligma_display_shell_remove_preview_item (shell, draw_tool->preview);
          g_clear_object (&draw_tool->preview);
        }

      if (draw_tool->item)
        {
          ligma_display_shell_remove_tool_item (shell, draw_tool->item);
          g_clear_object (&draw_tool->item);
        }
    }
}

static void
ligma_draw_tool_real_draw (LigmaDrawTool *draw_tool)
{
  if (draw_tool->widget)
    {
      LigmaCanvasItem *item = ligma_tool_widget_get_item (draw_tool->widget);

      ligma_draw_tool_add_item (draw_tool, item);
    }
}

void
ligma_draw_tool_start (LigmaDrawTool *draw_tool,
                      LigmaDisplay  *display)
{
  g_return_if_fail (LIGMA_IS_DRAW_TOOL (draw_tool));
  g_return_if_fail (LIGMA_IS_DISPLAY (display));
  g_return_if_fail (ligma_draw_tool_is_active (draw_tool) == FALSE);

  draw_tool->display = display;

  ligma_draw_tool_draw (draw_tool);
}

void
ligma_draw_tool_stop (LigmaDrawTool *draw_tool)
{
  g_return_if_fail (LIGMA_IS_DRAW_TOOL (draw_tool));
  g_return_if_fail (ligma_draw_tool_is_active (draw_tool) == TRUE);

  ligma_draw_tool_undraw (draw_tool);

  if (draw_tool->draw_timeout)
    {
      g_source_remove (draw_tool->draw_timeout);
      draw_tool->draw_timeout = 0;
    }

  draw_tool->last_draw_time = 0;

  draw_tool->display = NULL;
}

gboolean
ligma_draw_tool_is_active (LigmaDrawTool *draw_tool)
{
  g_return_val_if_fail (LIGMA_IS_DRAW_TOOL (draw_tool), FALSE);

  return draw_tool->display != NULL;
}

void
ligma_draw_tool_pause (LigmaDrawTool *draw_tool)
{
  g_return_if_fail (LIGMA_IS_DRAW_TOOL (draw_tool));

  draw_tool->paused_count++;

  if (draw_tool->draw_timeout)
    {
      g_source_remove (draw_tool->draw_timeout);
      draw_tool->draw_timeout = 0;
    }
}

void
ligma_draw_tool_resume (LigmaDrawTool *draw_tool)
{
  g_return_if_fail (LIGMA_IS_DRAW_TOOL (draw_tool));
  g_return_if_fail (draw_tool->paused_count > 0);

  draw_tool->paused_count--;

  if (draw_tool->paused_count == 0)
    {
#ifdef USE_TIMEOUT
      /* Don't install the timeout if the draw tool isn't active, so
       * suspend()/resume() can always be called, and have no side
       * effect on an inactive tool. See bug #687851.
       */
      if (ligma_draw_tool_is_active (draw_tool) && ! draw_tool->draw_timeout)
        {
          draw_tool->draw_timeout =
            gdk_threads_add_timeout_full (G_PRIORITY_HIGH_IDLE,
                                          DRAW_TIMEOUT,
                                          (GSourceFunc) ligma_draw_tool_draw_timeout,
                                          draw_tool, NULL);
        }
#endif

      /* call draw() anyway, it will do nothing if the timeout is
       * running, but will additionally check the drawing times to
       * ensure the minimum framerate
       */
      ligma_draw_tool_draw (draw_tool);
    }
}

/**
 * ligma_draw_tool_calc_distance:
 * @draw_tool: a #LigmaDrawTool
 * @display:   a #LigmaDisplay
 * @x1:        start point X in image coordinates
 * @y1:        start point Y in image coordinates
 * @x2:        end point X in image coordinates
 * @y2:        end point Y in image coordinates
 *
 * If you just need to compare distances, consider to use
 * ligma_draw_tool_calc_distance_square() instead.
 *
 * Returns: the distance between the given points in display coordinates
 **/
gdouble
ligma_draw_tool_calc_distance (LigmaDrawTool *draw_tool,
                              LigmaDisplay  *display,
                              gdouble       x1,
                              gdouble       y1,
                              gdouble       x2,
                              gdouble       y2)
{
  return sqrt (ligma_draw_tool_calc_distance_square (draw_tool, display,
                                                    x1, y1, x2, y2));
}

/**
 * ligma_draw_tool_calc_distance_square:
 * @draw_tool: a #LigmaDrawTool
 * @display:   a #LigmaDisplay
 * @x1:        start point X in image coordinates
 * @y1:        start point Y in image coordinates
 * @x2:        end point X in image coordinates
 * @y2:        end point Y in image coordinates
 *
 * This function is more effective than ligma_draw_tool_calc_distance()
 * as it doesn't perform a sqrt(). Use this if you just need to compare
 * distances.
 *
 * Returns: the square of the distance between the given points in
 *          display coordinates
 **/
gdouble
ligma_draw_tool_calc_distance_square (LigmaDrawTool *draw_tool,
                                     LigmaDisplay  *display,
                                     gdouble       x1,
                                     gdouble       y1,
                                     gdouble       x2,
                                     gdouble       y2)
{
  LigmaDisplayShell *shell;
  gdouble           tx1, ty1;
  gdouble           tx2, ty2;

  g_return_val_if_fail (LIGMA_IS_DRAW_TOOL (draw_tool), 0.0);
  g_return_val_if_fail (LIGMA_IS_DISPLAY (display), 0.0);

  shell = ligma_display_get_shell (display);

  ligma_display_shell_transform_xy_f (shell, x1, y1, &tx1, &ty1);
  ligma_display_shell_transform_xy_f (shell, x2, y2, &tx2, &ty2);

  return SQR (tx2 - tx1) + SQR (ty2 - ty1);
}

void
ligma_draw_tool_set_widget (LigmaDrawTool   *draw_tool,
                           LigmaToolWidget *widget)
{
  g_return_if_fail (LIGMA_IS_DRAW_TOOL (draw_tool));
  g_return_if_fail (widget == NULL || LIGMA_IS_TOOL_WIDGET (widget));

  if (widget == draw_tool->widget)
    return;

  if (draw_tool->widget)
    {
      ligma_tool_widget_set_focus (draw_tool->widget, FALSE);

      g_signal_handlers_disconnect_by_func (draw_tool->widget,
                                            ligma_draw_tool_widget_status,
                                            draw_tool);
      g_signal_handlers_disconnect_by_func (draw_tool->widget,
                                            ligma_draw_tool_widget_status_coords,
                                            draw_tool);
      g_signal_handlers_disconnect_by_func (draw_tool->widget,
                                            ligma_draw_tool_widget_message,
                                            draw_tool);
      g_signal_handlers_disconnect_by_func (draw_tool->widget,
                                            ligma_draw_tool_widget_snap_offsets,
                                            draw_tool);

      if (ligma_draw_tool_is_active (draw_tool))
        {
          LigmaCanvasItem *item = ligma_tool_widget_get_item (draw_tool->widget);

          ligma_draw_tool_remove_item (draw_tool, item);
        }

      g_object_unref (draw_tool->widget);
    }

  draw_tool->widget = widget;

  if (draw_tool->widget)
    {
      g_object_ref (draw_tool->widget);

      if (ligma_draw_tool_is_active (draw_tool))
        {
          LigmaCanvasItem *item = ligma_tool_widget_get_item (draw_tool->widget);

          ligma_draw_tool_add_item (draw_tool, item);
        }

      g_signal_connect (draw_tool->widget, "status",
                        G_CALLBACK (ligma_draw_tool_widget_status),
                        draw_tool);
      g_signal_connect (draw_tool->widget, "status-coords",
                        G_CALLBACK (ligma_draw_tool_widget_status_coords),
                        draw_tool);
      g_signal_connect (draw_tool->widget, "message",
                        G_CALLBACK (ligma_draw_tool_widget_message),
                        draw_tool);
      g_signal_connect (draw_tool->widget, "snap-offsets",
                        G_CALLBACK (ligma_draw_tool_widget_snap_offsets),
                        draw_tool);

      ligma_tool_widget_set_focus (draw_tool->widget, TRUE);
    }
}

void
ligma_draw_tool_set_default_status (LigmaDrawTool *draw_tool,
                                   const gchar  *status)
{
  g_return_if_fail (LIGMA_IS_DRAW_TOOL (draw_tool));

  if (draw_tool->default_status)
    g_free (draw_tool->default_status);

  draw_tool->default_status = g_strdup (status);
}

void
ligma_draw_tool_add_preview (LigmaDrawTool   *draw_tool,
                            LigmaCanvasItem *item)
{
  g_return_if_fail (LIGMA_IS_DRAW_TOOL (draw_tool));
  g_return_if_fail (LIGMA_IS_CANVAS_ITEM (item));

  if (! draw_tool->preview)
    draw_tool->preview =
      ligma_canvas_group_new (ligma_display_get_shell (draw_tool->display));

  ligma_canvas_group_add_item (LIGMA_CANVAS_GROUP (draw_tool->preview), item);
}

void
ligma_draw_tool_remove_preview (LigmaDrawTool   *draw_tool,
                               LigmaCanvasItem *item)
{
  g_return_if_fail (LIGMA_IS_DRAW_TOOL (draw_tool));
  g_return_if_fail (LIGMA_IS_CANVAS_ITEM (item));
  g_return_if_fail (draw_tool->preview != NULL);

  ligma_canvas_group_remove_item (LIGMA_CANVAS_GROUP (draw_tool->preview), item);
}

void
ligma_draw_tool_add_item (LigmaDrawTool   *draw_tool,
                         LigmaCanvasItem *item)
{
  LigmaCanvasGroup *group;

  g_return_if_fail (LIGMA_IS_DRAW_TOOL (draw_tool));
  g_return_if_fail (LIGMA_IS_CANVAS_ITEM (item));

  if (! draw_tool->item)
    draw_tool->item =
      ligma_canvas_group_new (ligma_display_get_shell (draw_tool->display));

  group = LIGMA_CANVAS_GROUP (draw_tool->item);

  if (draw_tool->group_stack)
    group = draw_tool->group_stack->data;

  ligma_canvas_group_add_item (group, item);
}

void
ligma_draw_tool_remove_item (LigmaDrawTool   *draw_tool,
                            LigmaCanvasItem *item)
{
  g_return_if_fail (LIGMA_IS_DRAW_TOOL (draw_tool));
  g_return_if_fail (LIGMA_IS_CANVAS_ITEM (item));
  g_return_if_fail (draw_tool->item != NULL);

  ligma_canvas_group_remove_item (LIGMA_CANVAS_GROUP (draw_tool->item), item);
}

LigmaCanvasGroup *
ligma_draw_tool_add_stroke_group (LigmaDrawTool *draw_tool)
{
  LigmaCanvasItem *item;

  g_return_val_if_fail (LIGMA_IS_DRAW_TOOL (draw_tool), NULL);

  item = ligma_canvas_group_new (ligma_display_get_shell (draw_tool->display));
  ligma_canvas_group_set_group_stroking (LIGMA_CANVAS_GROUP (item), TRUE);

  ligma_draw_tool_add_item (draw_tool, item);
  g_object_unref (item);

  return LIGMA_CANVAS_GROUP (item);
}

LigmaCanvasGroup *
ligma_draw_tool_add_fill_group (LigmaDrawTool *draw_tool)
{
  LigmaCanvasItem *item;

  g_return_val_if_fail (LIGMA_IS_DRAW_TOOL (draw_tool), NULL);

  item = ligma_canvas_group_new (ligma_display_get_shell (draw_tool->display));
  ligma_canvas_group_set_group_filling (LIGMA_CANVAS_GROUP (item), TRUE);

  ligma_draw_tool_add_item (draw_tool, item);
  g_object_unref (item);

  return LIGMA_CANVAS_GROUP (item);
}

void
ligma_draw_tool_push_group (LigmaDrawTool    *draw_tool,
                           LigmaCanvasGroup *group)
{
  g_return_if_fail (LIGMA_IS_DRAW_TOOL (draw_tool));
  g_return_if_fail (LIGMA_IS_CANVAS_GROUP (group));

  draw_tool->group_stack = g_list_prepend (draw_tool->group_stack, group);
}

void
ligma_draw_tool_pop_group (LigmaDrawTool *draw_tool)
{
  g_return_if_fail (LIGMA_IS_DRAW_TOOL (draw_tool));
  g_return_if_fail (draw_tool->group_stack != NULL);

  draw_tool->group_stack = g_list_remove (draw_tool->group_stack,
                                          draw_tool->group_stack->data);
}

/**
 * ligma_draw_tool_add_line:
 * @draw_tool:   the #LigmaDrawTool
 * @x1:          start point X in image coordinates
 * @y1:          start point Y in image coordinates
 * @x2:          end point X in image coordinates
 * @y2:          end point Y in image coordinates
 *
 * This function takes image space coordinates and transforms them to
 * screen window coordinates, then draws a line between the resulting
 * coordinates.
 **/
LigmaCanvasItem *
ligma_draw_tool_add_line (LigmaDrawTool *draw_tool,
                         gdouble       x1,
                         gdouble       y1,
                         gdouble       x2,
                         gdouble       y2)
{
  LigmaCanvasItem *item;

  g_return_val_if_fail (LIGMA_IS_DRAW_TOOL (draw_tool), NULL);

  item = ligma_canvas_line_new (ligma_display_get_shell (draw_tool->display),
                               x1, y1, x2, y2);

  ligma_draw_tool_add_item (draw_tool, item);
  g_object_unref (item);

  return item;
}

/**
 * ligma_draw_tool_add_guide:
 * @draw_tool:   the #LigmaDrawTool
 * @orientation: the orientation of the guide line
 * @position:    the position of the guide line in image coordinates
 *
 * This function draws a guide line across the canvas.
 **/
LigmaCanvasItem *
ligma_draw_tool_add_guide (LigmaDrawTool        *draw_tool,
                          LigmaOrientationType  orientation,
                          gint                 position,
                          LigmaGuideStyle       style)
{
  LigmaCanvasItem *item;

  g_return_val_if_fail (LIGMA_IS_DRAW_TOOL (draw_tool), NULL);

  item = ligma_canvas_guide_new (ligma_display_get_shell (draw_tool->display),
                                orientation, position,
                                style);

  ligma_draw_tool_add_item (draw_tool, item);
  g_object_unref (item);

  return item;
}

/**
 * ligma_draw_tool_add_crosshair:
 * @draw_tool:  the #LigmaDrawTool
 * @position_x: the position of the vertical guide line in image coordinates
 * @position_y: the position of the horizontal guide line in image coordinates
 *
 * This function draws two crossing guide lines across the canvas.
 **/
LigmaCanvasItem *
ligma_draw_tool_add_crosshair (LigmaDrawTool *draw_tool,
                              gint          position_x,
                              gint          position_y)
{
  LigmaCanvasGroup *group;

  group = ligma_draw_tool_add_stroke_group (draw_tool);

  ligma_draw_tool_push_group (draw_tool, group);
  ligma_draw_tool_add_guide (draw_tool,
                            LIGMA_ORIENTATION_VERTICAL, position_x,
                            LIGMA_GUIDE_STYLE_NONE);
  ligma_draw_tool_add_guide (draw_tool,
                            LIGMA_ORIENTATION_HORIZONTAL, position_y,
                            LIGMA_GUIDE_STYLE_NONE);
  ligma_draw_tool_pop_group (draw_tool);

  return LIGMA_CANVAS_ITEM (group);
}

/**
 * ligma_draw_tool_add_sample_point:
 * @draw_tool: the #LigmaDrawTool
 * @x:         X position of the sample point
 * @y:         Y position of the sample point
 * @index:     Index of the sample point
 *
 * This function draws a sample point
 **/
LigmaCanvasItem *
ligma_draw_tool_add_sample_point (LigmaDrawTool *draw_tool,
                                 gint          x,
                                 gint          y,
                                 gint          index)
{
  LigmaCanvasItem *item;

  g_return_val_if_fail (LIGMA_IS_DRAW_TOOL (draw_tool), NULL);

  item = ligma_canvas_sample_point_new (ligma_display_get_shell (draw_tool->display),
                                       x, y, index, TRUE);

  ligma_draw_tool_add_item (draw_tool, item);
  g_object_unref (item);

  return item;
}

/**
 * ligma_draw_tool_add_rectangle:
 * @draw_tool:   the #LigmaDrawTool
 * @filled:      whether to fill the rectangle
 * @x:           horizontal image coordinate
 * @y:           vertical image coordinate
 * @width:       width in image coordinates
 * @height:      height in image coordinates
 *
 * This function takes image space coordinates and transforms them to
 * screen window coordinates, then draws the resulting rectangle.
 **/
LigmaCanvasItem *
ligma_draw_tool_add_rectangle (LigmaDrawTool *draw_tool,
                              gboolean      filled,
                              gdouble       x,
                              gdouble       y,
                              gdouble       width,
                              gdouble       height)
{
  LigmaCanvasItem *item;

  g_return_val_if_fail (LIGMA_IS_DRAW_TOOL (draw_tool), NULL);

  item = ligma_canvas_rectangle_new (ligma_display_get_shell (draw_tool->display),
                                    x, y, width, height, filled);

  ligma_draw_tool_add_item (draw_tool, item);
  g_object_unref (item);

  return item;
}

LigmaCanvasItem *
ligma_draw_tool_add_arc (LigmaDrawTool *draw_tool,
                        gboolean      filled,
                        gdouble       x,
                        gdouble       y,
                        gdouble       width,
                        gdouble       height,
                        gdouble       start_angle,
                        gdouble       slice_angle)
{
  LigmaCanvasItem *item;

  g_return_val_if_fail (LIGMA_IS_DRAW_TOOL (draw_tool), NULL);

  item = ligma_canvas_arc_new (ligma_display_get_shell (draw_tool->display),
                              x + width  / 2.0,
                              y + height / 2.0,
                              width  / 2.0,
                              height / 2.0,
                              start_angle,
                              slice_angle,
                              filled);

  ligma_draw_tool_add_item (draw_tool, item);
  g_object_unref (item);

  return item;
}

LigmaCanvasItem *
ligma_draw_tool_add_handle (LigmaDrawTool     *draw_tool,
                           LigmaHandleType    type,
                           gdouble           x,
                           gdouble           y,
                           gint              width,
                           gint              height,
                           LigmaHandleAnchor  anchor)
{
  LigmaCanvasItem *item;

  g_return_val_if_fail (LIGMA_IS_DRAW_TOOL (draw_tool), NULL);

  item = ligma_canvas_handle_new (ligma_display_get_shell (draw_tool->display),
                                 type, anchor, x, y, width, height);

  ligma_draw_tool_add_item (draw_tool, item);
  g_object_unref (item);

  return item;
}

LigmaCanvasItem *
ligma_draw_tool_add_lines (LigmaDrawTool      *draw_tool,
                          const LigmaVector2 *points,
                          gint               n_points,
                          LigmaMatrix3       *transform,
                          gboolean           filled)
{
  LigmaCanvasItem *item;

  g_return_val_if_fail (LIGMA_IS_DRAW_TOOL (draw_tool), NULL);

  if (points == NULL || n_points < 2)
    return NULL;

  item = ligma_canvas_polygon_new (ligma_display_get_shell (draw_tool->display),
                                  points, n_points, transform, filled);

  ligma_draw_tool_add_item (draw_tool, item);
  g_object_unref (item);

  return item;
}

LigmaCanvasItem *
ligma_draw_tool_add_strokes (LigmaDrawTool     *draw_tool,
                            const LigmaCoords *points,
                            gint              n_points,
                            LigmaMatrix3      *transform,
                            gboolean          filled)
{
  LigmaCanvasItem *item;

  g_return_val_if_fail (LIGMA_IS_DRAW_TOOL (draw_tool), NULL);

  if (points == NULL || n_points < 2)
    return NULL;

  item = ligma_canvas_polygon_new_from_coords (ligma_display_get_shell (draw_tool->display),
                                              points, n_points, transform, filled);

  ligma_draw_tool_add_item (draw_tool, item);
  g_object_unref (item);

  return item;
}

LigmaCanvasItem *
ligma_draw_tool_add_pen (LigmaDrawTool      *draw_tool,
                        const LigmaVector2 *points,
                        gint               n_points,
                        LigmaContext       *context,
                        LigmaActiveColor    color,
                        gint               width)
{
  LigmaCanvasItem *item;

  g_return_val_if_fail (LIGMA_IS_DRAW_TOOL (draw_tool), NULL);

  if (points == NULL || n_points < 2)
    return NULL;

  item = ligma_canvas_pen_new (ligma_display_get_shell (draw_tool->display),
                              points, n_points, context, color, width);

  ligma_draw_tool_add_item (draw_tool, item);
  g_object_unref (item);

  return item;
}

/**
 * ligma_draw_tool_add_boundary:
 * @draw_tool:    a #LigmaDrawTool
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
LigmaCanvasItem *
ligma_draw_tool_add_boundary (LigmaDrawTool       *draw_tool,
                             const LigmaBoundSeg *bound_segs,
                             gint                n_bound_segs,
                             LigmaMatrix3        *transform,
                             gdouble             offset_x,
                             gdouble             offset_y)
{
  LigmaCanvasItem *item;

  g_return_val_if_fail (LIGMA_IS_DRAW_TOOL (draw_tool), NULL);
  g_return_val_if_fail (n_bound_segs > 0, NULL);
  g_return_val_if_fail (bound_segs != NULL, NULL);

  item = ligma_canvas_boundary_new (ligma_display_get_shell (draw_tool->display),
                                   bound_segs, n_bound_segs,
                                   transform,
                                   offset_x, offset_y);

  ligma_draw_tool_add_item (draw_tool, item);
  g_object_unref (item);

  return item;
}

LigmaCanvasItem *
ligma_draw_tool_add_text_cursor (LigmaDrawTool     *draw_tool,
                                PangoRectangle   *cursor,
                                gboolean          overwrite,
                                LigmaTextDirection direction)
{
  LigmaCanvasItem *item;

  g_return_val_if_fail (LIGMA_IS_DRAW_TOOL (draw_tool), NULL);

  item = ligma_canvas_text_cursor_new (ligma_display_get_shell (draw_tool->display),
                                      cursor, overwrite, direction);

  ligma_draw_tool_add_item (draw_tool, item);
  g_object_unref (item);

  return item;
}

LigmaCanvasItem *
ligma_draw_tool_add_transform_preview (LigmaDrawTool      *draw_tool,
                                      LigmaPickable      *pickable,
                                      const LigmaMatrix3 *transform,
                                      gdouble            x1,
                                      gdouble            y1,
                                      gdouble            x2,
                                      gdouble            y2)
{
  LigmaCanvasItem *item;

  g_return_val_if_fail (LIGMA_IS_DRAW_TOOL (draw_tool), NULL);
  g_return_val_if_fail (LIGMA_IS_PICKABLE (pickable), NULL);
  g_return_val_if_fail (transform != NULL, NULL);

  item = ligma_canvas_transform_preview_new (ligma_display_get_shell (draw_tool->display),
                                            pickable, transform,
                                            x1, y1, x2, y2);

  ligma_draw_tool_add_preview (draw_tool, item);
  g_object_unref (item);

  return item;
}

gboolean
ligma_draw_tool_on_handle (LigmaDrawTool     *draw_tool,
                          LigmaDisplay      *display,
                          gdouble           x,
                          gdouble           y,
                          LigmaHandleType    type,
                          gdouble           handle_x,
                          gdouble           handle_y,
                          gint              width,
                          gint              height,
                          LigmaHandleAnchor  anchor)
{
  LigmaDisplayShell *shell;
  gdouble           tx, ty;
  gdouble           handle_tx, handle_ty;

  g_return_val_if_fail (LIGMA_IS_DRAW_TOOL (draw_tool), FALSE);
  g_return_val_if_fail (LIGMA_IS_DISPLAY (display), FALSE);

  shell = ligma_display_get_shell (display);

  ligma_display_shell_zoom_xy_f (shell,
                                x, y,
                                &tx, &ty);
  ligma_display_shell_zoom_xy_f (shell,
                                handle_x, handle_y,
                                &handle_tx, &handle_ty);

  switch (type)
    {
    case LIGMA_HANDLE_SQUARE:
    case LIGMA_HANDLE_FILLED_SQUARE:
    case LIGMA_HANDLE_CROSS:
    case LIGMA_HANDLE_CROSSHAIR:
      ligma_canvas_item_shift_to_north_west (anchor,
                                            handle_tx, handle_ty,
                                            width, height,
                                            &handle_tx, &handle_ty);

      return (tx == CLAMP (tx, handle_tx, handle_tx + width) &&
              ty == CLAMP (ty, handle_ty, handle_ty + height));

    case LIGMA_HANDLE_CIRCLE:
    case LIGMA_HANDLE_FILLED_CIRCLE:
      ligma_canvas_item_shift_to_center (anchor,
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
