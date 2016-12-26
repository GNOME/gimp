/* GIMP - The GNU Image Manipulation Program
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * Measure tool
 * Copyright (C) 1999-2003 Sven Neumann <sven@gimp.org>
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

#include <stdlib.h>

#include <gegl.h>
#include <gtk/gtk.h>
#include <gdk/gdkkeysyms.h>

#include "libgimpbase/gimpbase.h"
#include "libgimpmath/gimpmath.h"
#include "libgimpwidgets/gimpwidgets.h"

#include "tools-types.h"

#include "core/gimp-utils.h"
#include "core/gimpimage.h"
#include "core/gimpimage-guides.h"
#include "core/gimpimage-undo.h"
#include "core/gimpimage-undo-push.h"

#include "widgets/gimphelp-ids.h"
#include "widgets/gimpwidgets-utils.h"

#include "display/gimpcanvasgroup.h"
#include "display/gimpcanvashandle.h"
#include "display/gimpdisplay.h"
#include "display/gimpdisplayshell.h"
#include "display/gimpdisplayshell-appearance.h"
#include "display/gimptoolgui.h"

#include "gimpmeasureoptions.h"
#include "gimpmeasuretool.h"
#include "gimptoolcontrol.h"

#include "gimp-intl.h"


#define ARC_RADIUS 30


/*  local function prototypes  */

static void     gimp_measure_tool_control         (GimpTool              *tool,
                                                   GimpToolAction         action,
                                                   GimpDisplay           *display);
static void     gimp_measure_tool_button_press    (GimpTool              *tool,
                                                   const GimpCoords      *coords,
                                                   guint32                time,
                                                   GdkModifierType        state,
                                                   GimpButtonPressType    press_type,
                                                   GimpDisplay           *display);
static void     gimp_measure_tool_button_release  (GimpTool              *tool,
                                                   const GimpCoords      *coords,
                                                   guint32                time,
                                                   GdkModifierType        state,
                                                   GimpButtonReleaseType  release_type,
                                                   GimpDisplay           *display);
static void     gimp_measure_tool_motion          (GimpTool              *tool,
                                                   const GimpCoords      *coords,
                                                   guint32                time,
                                                   GdkModifierType        state,
                                                   GimpDisplay           *display);
static gboolean gimp_measure_tool_key_press       (GimpTool              *tool,
                                                   GdkEventKey           *kevent,
                                                   GimpDisplay           *display);
static void gimp_measure_tool_active_modifier_key (GimpTool              *tool,
                                                   GdkModifierType        key,
                                                   gboolean               press,
                                                   GdkModifierType        state,
                                                   GimpDisplay           *display);
static void     gimp_measure_tool_oper_update     (GimpTool              *tool,
                                                   const GimpCoords      *coords,
                                                   GdkModifierType        state,
                                                   gboolean               proximity,
                                                   GimpDisplay           *display);
static void     gimp_measure_tool_cursor_update   (GimpTool              *tool,
                                                   const GimpCoords      *coords,
                                                   GdkModifierType        state,
                                                   GimpDisplay           *display);

static void     gimp_measure_tool_draw            (GimpDrawTool          *draw_tool);

static gdouble  gimp_measure_tool_get_angle       (gint                   dx,
                                                   gint                   dy,
                                                   gdouble                xres,
                                                   gdouble                yres);

static GimpToolGui * gimp_measure_tool_dialog_new (GimpMeasureTool       *measure);
static void     gimp_measure_tool_dialog_update   (GimpMeasureTool       *measure,
                                                   GimpDisplay           *display);


G_DEFINE_TYPE (GimpMeasureTool, gimp_measure_tool, GIMP_TYPE_DRAW_TOOL)

#define parent_class gimp_measure_tool_parent_class


void
gimp_measure_tool_register (GimpToolRegisterCallback  callback,
                            gpointer                  data)
{
  (* callback) (GIMP_TYPE_MEASURE_TOOL,
                GIMP_TYPE_MEASURE_OPTIONS,
                gimp_measure_options_gui,
                0,
                "gimp-measure-tool",
                _("Measure"),
                _("Measure Tool: Measure distances and angles"),
                N_("_Measure"), "<shift>M",
                NULL, GIMP_HELP_TOOL_MEASURE,
                GIMP_STOCK_TOOL_MEASURE,
                data);
}

static void
gimp_measure_tool_class_init (GimpMeasureToolClass *klass)
{
  GimpToolClass     *tool_class      = GIMP_TOOL_CLASS (klass);
  GimpDrawToolClass *draw_tool_class = GIMP_DRAW_TOOL_CLASS (klass);

  tool_class->control             = gimp_measure_tool_control;
  tool_class->button_press        = gimp_measure_tool_button_press;
  tool_class->button_release      = gimp_measure_tool_button_release;
  tool_class->motion              = gimp_measure_tool_motion;
  tool_class->key_press           = gimp_measure_tool_key_press;
  tool_class->active_modifier_key = gimp_measure_tool_active_modifier_key;
  tool_class->oper_update         = gimp_measure_tool_oper_update;
  tool_class->cursor_update       = gimp_measure_tool_cursor_update;

  draw_tool_class->draw           = gimp_measure_tool_draw;
}

static void
gimp_measure_tool_init (GimpMeasureTool *measure)
{
  GimpTool *tool = GIMP_TOOL (measure);

  gimp_tool_control_set_handle_empty_image (tool->control, TRUE);
  gimp_tool_control_set_precision          (tool->control,
                                            GIMP_CURSOR_PRECISION_PIXEL_BORDER);
  gimp_tool_control_set_tool_cursor        (tool->control,
                                            GIMP_TOOL_CURSOR_MEASURE);

  measure->function    = CREATING;
  measure->point       = -1;
  measure->status_help = TRUE;
}

static void
gimp_measure_tool_control (GimpTool       *tool,
                           GimpToolAction  action,
                           GimpDisplay    *display)
{
  GimpMeasureTool *measure = GIMP_MEASURE_TOOL (tool);

  switch (action)
    {
    case GIMP_TOOL_ACTION_PAUSE:
    case GIMP_TOOL_ACTION_RESUME:
      break;

    case GIMP_TOOL_ACTION_HALT:
      if (measure->gui)
        g_object_unref (measure->gui);
      break;

    case GIMP_TOOL_ACTION_COMMIT:
      break;
    }

  GIMP_TOOL_CLASS (parent_class)->control (tool, action, display);
}

static void
gimp_measure_tool_button_press (GimpTool            *tool,
                                const GimpCoords    *coords,
                                guint32              time,
                                GdkModifierType      state,
                                GimpButtonPressType  press_type,
                                GimpDisplay         *display)
{
  GimpMeasureTool    *measure = GIMP_MEASURE_TOOL (tool);
  GimpMeasureOptions *options = GIMP_MEASURE_TOOL_GET_OPTIONS (tool);
  GimpDisplayShell   *shell   = gimp_display_get_shell (display);
  GimpImage          *image   = gimp_display_get_image (display);

  /*  if we are changing displays, pop the statusbar of the old one  */
  if (display != tool->display)
    {
      if (tool->display)
        gimp_tool_pop_status (tool, tool->display);
    }

  measure->function = CREATING;

  measure->mouse_x = coords->x;
  measure->mouse_y = coords->y;

  if (display == tool->display)
    {
      /*  if the cursor is in one of the handles, the new function
       *  will be moving or adding a new point or guide
       */
      if (measure->point != -1)
        {
          GdkModifierType extend_mask = gimp_get_extend_selection_mask ();
          GdkModifierType toggle_mask = gimp_get_toggle_behavior_mask ();

          if (state & (toggle_mask | GDK_MOD1_MASK))
            {
              gboolean create_hguide;
              gboolean create_vguide;

              create_hguide = ((state & toggle_mask) &&
                               (measure->y[measure->point] ==
                                CLAMP (measure->y[measure->point],
                                       0,
                                       gimp_image_get_height (image))));

              create_vguide = ((state & GDK_MOD1_MASK) &&
                               (measure->x[measure->point] ==
                                CLAMP (measure->x[measure->point],
                                       0,
                                       gimp_image_get_width (image))));

              if (create_hguide || create_vguide)
                {
                  if (create_hguide && create_vguide)
                    gimp_image_undo_group_start (image,
                                                 GIMP_UNDO_GROUP_GUIDE,
                                                 _("Add Guides"));

                  if (create_hguide)
                    gimp_image_add_hguide (image, measure->y[measure->point],
                                           TRUE);

                  if (create_vguide)
                    gimp_image_add_vguide (image, measure->x[measure->point],
                                           TRUE);

                  if (create_hguide && create_vguide)
                    gimp_image_undo_group_end (image);

                  gimp_image_flush (image);
                }

              measure->function = GUIDING;
            }
          else
            {
              if (state & extend_mask)
                measure->function = ADDING;
              else
                measure->function = MOVING;
            }
        }

      /*  adding to the middle point makes no sense  */
      if (measure->point      == 0      &&
          measure->function   == ADDING &&
          measure->num_points == 3)
        {
          measure->function = MOVING;
        }

      /*  if the function is still CREATING, we are outside the handles  */
      if (measure->function == CREATING)
        {
          if (measure->num_points > 1 && (state & GDK_MOD1_MASK))
            {
              measure->function = MOVING_ALL;

              measure->last_x = coords->x;
              measure->last_y = coords->y;
            }
        }
    }

  if (measure->function == CREATING)
    {
      if (gimp_draw_tool_is_active (GIMP_DRAW_TOOL (measure)))
        gimp_draw_tool_stop (GIMP_DRAW_TOOL (measure));

      measure->x[0] = measure->x[1] = measure->x[2] = 0.0;
      measure->y[0] = measure->y[1] = measure->y[2] = 0.0;

      /*  set the first point and go into ADDING mode  */
      measure->x[0]       = coords->x + 0.5;
      measure->y[0]       = coords->y + 0.5;
      measure->point      = 0;
      measure->num_points = 1;
      measure->function   = ADDING;

      /*  set the displaylay  */
      tool->display = display;

      gimp_tool_replace_status (tool, display, _("Drag to create a line"));

      gimp_draw_tool_start (GIMP_DRAW_TOOL (tool), display);
    }

  gimp_tool_control_activate (tool->control);

  /*  create the info window if necessary  */
  if (! measure->gui)
    {
      if (options->use_info_window ||
          ! gimp_display_shell_get_show_statusbar (shell))
        {
          measure->gui = gimp_measure_tool_dialog_new (measure);
          g_object_add_weak_pointer (G_OBJECT (measure->gui),
                                     (gpointer) &measure->gui);
        }
    }

  if (measure->gui)
    {
      gimp_tool_gui_set_shell (measure->gui, shell);
      gimp_tool_gui_set_viewable (measure->gui, GIMP_VIEWABLE (image));

      gimp_measure_tool_dialog_update (measure, display);
    }
}

static void
gimp_measure_tool_button_release (GimpTool              *tool,
                                  const GimpCoords      *coords,
                                  guint32                time,
                                  GdkModifierType        state,
                                  GimpButtonReleaseType  release_type,
                                  GimpDisplay           *display)
{
  GimpMeasureTool *measure = GIMP_MEASURE_TOOL (tool);

  measure->function = FINISHED;

  gimp_tool_control_halt (tool->control);
}

static void
gimp_measure_tool_motion (GimpTool         *tool,
                          const GimpCoords *coords,
                          guint32           time,
                          GdkModifierType   state,
                          GimpDisplay      *display)
{
  GimpMeasureTool *measure = GIMP_MEASURE_TOOL (tool);
  gint             dx, dy;
  gint             i;
  gint             tmp;

  gimp_draw_tool_pause (GIMP_DRAW_TOOL (measure));

  measure->mouse_x = coords->x;
  measure->mouse_y = coords->y;

  /*  A few comments here, because this routine looks quite weird at first ...
   *
   *  The goal is to keep point 0, called the start point, to be
   *  always the one in the middle or, if there are only two points,
   *  the one that is fixed.  The angle is then always measured at
   *  this point.
   */

  switch (measure->function)
    {
    case ADDING:
      switch (measure->point)
        {
        case 0:
          /*  we are adding to the start point  */
          break;

        case 1:
          /*  we are adding to the end point, make it the new start point  */
          tmp = measure->x[0];
          measure->x[0] = measure->x[1];
          measure->x[1] = tmp;

          tmp = measure->y[0];
          measure->y[0] = measure->y[1];
          measure->y[1] = tmp;
          break;

        case 2:
          /*  we are adding to the third point, make it the new start point  */
          measure->x[1] = measure->x[0];
          measure->y[1] = measure->y[0];
          measure->x[0] = measure->x[2];
          measure->y[0] = measure->y[2];
          break;

        default:
          break;
        }

      measure->num_points = MIN (measure->num_points + 1, 3);
      measure->point      = measure->num_points - 1;
      measure->function   = MOVING;
      /*  don't break here!  */

    case MOVING:
      /*  if we are moving the start point and only have two, make it
       *  the end point
       */
      if (measure->num_points == 2 && measure->point == 0)
        {
          tmp = measure->x[0];
          measure->x[0] = measure->x[1];
          measure->x[1] = tmp;

          tmp = measure->y[0];
          measure->y[0] = measure->y[1];
          measure->y[1] = tmp;

          measure->point = 1;
        }

      measure->x[measure->point] = ROUND (coords->x);
      measure->y[measure->point] = ROUND (coords->y);

      if (state & gimp_get_constrain_behavior_mask ())
        {
          gdouble  x = measure->x[measure->point];
          gdouble  y = measure->y[measure->point];

          gimp_constrain_line (measure->x[0], measure->y[0],
                               &x, &y,
                               GIMP_CONSTRAIN_LINE_15_DEGREES);

          measure->x[measure->point] = ROUND (x);
          measure->y[measure->point] = ROUND (y);
        }
      break;

    case MOVING_ALL:
      dx = ROUND (coords->x) - measure->last_x;
      dy = ROUND (coords->y) - measure->last_y;

      for (i = 0; i < measure->num_points; i++)
        {
          measure->x[i] += dx;
          measure->y[i] += dy;
        }

      measure->last_x = ROUND (coords->x);
      measure->last_y = ROUND (coords->y);
      break;

    default:
      break;
    }

  if (measure->function == MOVING)
    gimp_measure_tool_dialog_update (measure, display);

  gimp_draw_tool_resume (GIMP_DRAW_TOOL (measure));
}

static gboolean
gimp_measure_tool_key_press (GimpTool    *tool,
                             GdkEventKey *kevent,
                             GimpDisplay *display)
{
  if (display == tool->display)
    {
      switch (kevent->keyval)
        {
        case GDK_KEY_Escape:
          gimp_tool_control (tool, GIMP_TOOL_ACTION_HALT, display);
          return TRUE;

        default:
          break;
        }
    }

  return FALSE;
}

static void
gimp_measure_tool_active_modifier_key (GimpTool        *tool,
                                       GdkModifierType  key,
                                       gboolean         press,
                                       GdkModifierType  state,
                                       GimpDisplay     *display)
{
  GimpMeasureTool *measure = GIMP_MEASURE_TOOL (tool);

  if (key == gimp_get_constrain_behavior_mask () &&
      measure->function == MOVING)
    {
      gdouble x, y;

      gimp_draw_tool_pause (GIMP_DRAW_TOOL (tool));

      x = measure->mouse_x;
      y = measure->mouse_y;

      if (press)
        gimp_constrain_line (measure->x[0], measure->y[0],
                             &x, &y,
                             GIMP_CONSTRAIN_LINE_15_DEGREES);

      measure->x[measure->point] = ROUND (x);
      measure->y[measure->point] = ROUND (y);

      gimp_measure_tool_dialog_update (measure, display);

      gimp_draw_tool_resume (GIMP_DRAW_TOOL (tool));
    }
}

static void
gimp_measure_tool_oper_update (GimpTool         *tool,
                               const GimpCoords *coords,
                               GdkModifierType   state,
                               gboolean          proximity,
                               GimpDisplay      *display)
{
  GimpMeasureTool *measure = GIMP_MEASURE_TOOL (tool);
  gchar           *status  = NULL;
  gint             i;

  if (tool->display == display)
    {
      gint point = -1;

      for (i = 0; i < measure->num_points; i++)
        {
          if (gimp_canvas_item_hit (measure->handles[i],
                                    coords->x, coords->y))
            {
              GdkModifierType extend_mask = gimp_get_extend_selection_mask ();
              GdkModifierType toggle_mask = gimp_get_toggle_behavior_mask ();

              point = i;

              if (state & toggle_mask)
                {
                  if (state & GDK_MOD1_MASK)
                    {
                      status = gimp_suggest_modifiers (_("Click to place "
                                                         "vertical and "
                                                         "horizontal guides"),
                                                       0,
                                                       NULL, NULL, NULL);
                    }
                  else
                    {
                      status = gimp_suggest_modifiers (_("Click to place a "
                                                         "horizontal guide"),
                                                       GDK_MOD1_MASK & ~state,
                                                       NULL, NULL, NULL);
                    }

                  gimp_tool_replace_status (tool, display, "%s", status);
                  g_free (status);
                  measure->status_help = TRUE;
                  break;
                }

              if (state & GDK_MOD1_MASK)
                {
                  status = gimp_suggest_modifiers (_("Click to place a "
                                                     "vertical guide"),
                                                   toggle_mask & ~state,
                                                   NULL, NULL, NULL);
                  gimp_tool_replace_status (tool, display, "%s", status);
                  g_free (status);
                  measure->status_help = TRUE;
                  break;
                }

              if ((state & extend_mask)
                  && ! ((i == 0) && (measure->num_points == 3)))
                {
                  status = gimp_suggest_modifiers (_("Click-Drag to add a "
                                                     "new point"),
                                                   (toggle_mask |
                                                    GDK_MOD1_MASK) & ~state,
                                                   NULL, NULL, NULL);
                }
              else
                {
                  if ((i == 0) && (measure->num_points == 3))
                    state |= extend_mask;
                  status = gimp_suggest_modifiers (_("Click-Drag to move this "
                                                     "point"),
                                                   (extend_mask |
                                                    toggle_mask |
                                                    GDK_MOD1_MASK) & ~state,
                                                   NULL, NULL, NULL);
                }

              gimp_tool_replace_status (tool, display, "%s", status);
              g_free (status);
              measure->status_help = TRUE;
              break;
            }
        }

      if (point == -1)
        {
          if ((measure->num_points > 1) && (state & GDK_MOD1_MASK))
            {
              gimp_tool_replace_status (tool, display, _("Click-Drag to move "
                                                         "all points"));
              measure->status_help = TRUE;
            }
          else if (measure->status_help)
            {
              if (measure->num_points > 1)
                {
                  /* replace status bar hint by distance and angle */
                  gimp_measure_tool_dialog_update (measure, display);
                }
              else
                {
                  gimp_tool_replace_status (tool, display, " ");
                }
            }
        }

      if (point != measure->point)
        {
          if (measure->point != -1 && measure->handles[measure->point])
            {
              gimp_canvas_item_set_highlight (measure->handles[measure->point],
                                              FALSE);
            }

          measure->point = point;

          if (measure->point != -1 && measure->handles[measure->point])
            {
              gimp_canvas_item_set_highlight (measure->handles[measure->point],
                                              TRUE);
            }
        }
    }
}

static void
gimp_measure_tool_cursor_update (GimpTool         *tool,
                                 const GimpCoords *coords,
                                 GdkModifierType   state,
                                 GimpDisplay      *display)
{
  GimpMeasureTool   *measure  = GIMP_MEASURE_TOOL (tool);
  GimpCursorType     cursor   = GIMP_CURSOR_CROSSHAIR_SMALL;
  GimpCursorModifier modifier = GIMP_CURSOR_MODIFIER_NONE;

  if (tool->display == display)
    {
      if (measure->point != -1)
        {
          GdkModifierType extend_mask = gimp_get_extend_selection_mask ();
          GdkModifierType toggle_mask = gimp_get_toggle_behavior_mask ();

          if (state & toggle_mask)
            {
              if (state & GDK_MOD1_MASK)
                cursor = GIMP_CURSOR_CORNER_BOTTOM_RIGHT;
              else
                cursor = GIMP_CURSOR_SIDE_BOTTOM;
            }
          else if (state & GDK_MOD1_MASK)
            {
              cursor = GIMP_CURSOR_SIDE_RIGHT;
            }
          else if ((state & extend_mask) &&
                   ! ((measure->point == 0) &&
                      (measure->num_points == 3)))
            {
              modifier = GIMP_CURSOR_MODIFIER_PLUS;
            }
          else
            {
              modifier = GIMP_CURSOR_MODIFIER_MOVE;
            }
        }
      else
        {
          if ((measure->num_points > 1) && (state & GDK_MOD1_MASK))
            {
              modifier = GIMP_CURSOR_MODIFIER_MOVE;
            }
        }
    }

  gimp_tool_control_set_cursor          (tool->control, cursor);
  gimp_tool_control_set_cursor_modifier (tool->control, modifier);

  GIMP_TOOL_CLASS (parent_class)->cursor_update (tool, coords, state, display);
}

static void
gimp_measure_tool_draw (GimpDrawTool *draw_tool)
{
  GimpMeasureTool *measure = GIMP_MEASURE_TOOL (draw_tool);
  GimpTool        *tool    = GIMP_TOOL (draw_tool);
  GimpCanvasGroup *stroke_group;
  gint             i;
  gint             draw_arc = 0;

  for (i = 0; i < 3; i++)
    measure->handles[i] = 0;

  stroke_group = gimp_draw_tool_add_stroke_group (draw_tool);

  for (i = 0; i < measure->num_points; i++)
    {
      if (i == 0 && measure->num_points == 3)
        {
          measure->handles[i] =
            gimp_draw_tool_add_handle (draw_tool,
                                       GIMP_HANDLE_CIRCLE,
                                       measure->x[i],
                                       measure->y[i],
                                       GIMP_TOOL_HANDLE_SIZE_CROSS,
                                       GIMP_TOOL_HANDLE_SIZE_CROSS,
                                       GIMP_HANDLE_ANCHOR_CENTER);
        }
      else
        {
          measure->handles[i] =
            gimp_draw_tool_add_handle (draw_tool,
                                       GIMP_HANDLE_CROSS,
                                       measure->x[i],
                                       measure->y[i],
                                       GIMP_TOOL_HANDLE_SIZE_CROSS,
                                       GIMP_TOOL_HANDLE_SIZE_CROSS,
                                       GIMP_HANDLE_ANCHOR_CENTER);
        }

      if (i > 0)
        {
          gimp_draw_tool_push_group (draw_tool, stroke_group);

          gimp_draw_tool_add_line (draw_tool,
                                   measure->x[0],
                                   measure->y[0],
                                   measure->x[i],
                                   measure->y[i]);

          gimp_draw_tool_pop_group (draw_tool);

          /*  only draw the arc if the lines are long enough  */
          if (gimp_draw_tool_calc_distance (draw_tool, tool->display,
                                            measure->x[0],
                                            measure->y[0],
                                            measure->x[i],
                                            measure->y[i]) > ARC_RADIUS)
            {
              draw_arc++;
            }
        }
    }

  if (measure->point != -1 && measure->handles[measure->point])
    {
      gimp_canvas_item_set_highlight (measure->handles[measure->point],
                                      TRUE);
    }

  if (measure->num_points > 1 && draw_arc == measure->num_points - 1)
    {
      gdouble angle1 = measure->angle2 / 180.0 * G_PI;
      gdouble angle2 = (measure->angle1 - measure->angle2) / 180.0 * G_PI;

      if (angle2 > G_PI)
        angle2 -= 2.0 * G_PI;

      if (angle2 < -G_PI)
        angle2 += 2.0 * G_PI;

      if (angle2 != 0.0)
        {
          GimpCanvasItem *item;

          gimp_draw_tool_push_group (draw_tool, stroke_group);

          item = gimp_draw_tool_add_handle (draw_tool,
                                            GIMP_HANDLE_CIRCLE,
                                            measure->x[0],
                                            measure->y[0],
                                            ARC_RADIUS * 2 + 1,
                                            ARC_RADIUS * 2 + 1,
                                            GIMP_HANDLE_ANCHOR_CENTER);

          gimp_canvas_handle_set_angles (item, angle1, angle2);

          if (measure->num_points == 2)
            {
              GimpDisplayShell *shell;
              gdouble           target;
              gdouble           arc_radius;

              shell = gimp_display_get_shell (tool->display);

              target     = FUNSCALEX (shell, (GIMP_TOOL_HANDLE_SIZE_CROSS >> 1));
              arc_radius = FUNSCALEX (shell, ARC_RADIUS);

              gimp_draw_tool_add_line (draw_tool,
                                       measure->x[0],
                                       measure->y[0],
                                       (measure->x[1] >= measure->x[0] ?
                                        measure->x[0] + arc_radius + target :
                                        measure->x[0] - arc_radius - target),
                                       measure->y[0]);
            }

          gimp_draw_tool_pop_group (draw_tool);
        }
    }
}

static gdouble
gimp_measure_tool_get_angle (gint    dx,
                             gint    dy,
                             gdouble xres,
                             gdouble yres)
{
  gdouble angle;

  if (dx)
    angle = gimp_rad_to_deg (atan (((gdouble) (dy) / yres) /
                                   ((gdouble) (dx) / xres)));
  else if (dy)
    angle = dy > 0 ? 270.0 : 90.0;
  else
    angle = 180.0;

  if (dx > 0)
    {
      if (dy > 0)
        angle = 360.0 - angle;
      else
        angle = -angle;
    }
  else
    {
      angle = 180.0 - angle;
    }

  return angle;
}

static void
gimp_measure_tool_dialog_update (GimpMeasureTool *measure,
                                 GimpDisplay     *display)
{
  GimpDisplayShell *shell = gimp_display_get_shell (display);
  GimpImage        *image = gimp_display_get_image (display);
  gint              ax, ay;
  gint              bx, by;
  gint              pixel_width;
  gint              pixel_height;
  gdouble           unit_width;
  gdouble           unit_height;
  gdouble           pixel_distance;
  gdouble           unit_distance;
  gdouble           theta1, theta2;
  gdouble           pixel_angle;
  gdouble           unit_angle;
  gdouble           xres;
  gdouble           yres;
  gchar             format[128];

  /*  calculate distance and angle  */
  ax = measure->x[1] - measure->x[0];
  ay = measure->y[1] - measure->y[0];

  if (measure->num_points == 3)
    {
      bx = measure->x[2] - measure->x[0];
      by = measure->y[2] - measure->y[0];
    }
  else
    {
      bx = 0;
      by = 0;
    }

  pixel_width  = ABS (ax - bx);
  pixel_height = ABS (ay - by);

  gimp_image_get_resolution (image, &xres, &yres);

  unit_width  = gimp_pixels_to_units (pixel_width,  shell->unit, xres);
  unit_height = gimp_pixels_to_units (pixel_height, shell->unit, yres);

  pixel_distance = sqrt (SQR (ax - bx) + SQR (ay - by));
  unit_distance  = (gimp_unit_get_factor (shell->unit) *
                    sqrt (SQR ((gdouble) (ax - bx) / xres) +
                          SQR ((gdouble) (ay - by) / yres)));

  if (measure->num_points != 3)
    bx = ax > 0 ? 1 : -1;

  theta1 = gimp_measure_tool_get_angle (ax, ay, 1.0, 1.0);
  theta2 = gimp_measure_tool_get_angle (bx, by, 1.0, 1.0);

  pixel_angle = fabs (theta1 - theta2);
  if (pixel_angle > 180.0)
    pixel_angle = fabs (360.0 - pixel_angle);

  theta1 = gimp_measure_tool_get_angle (ax, ay, xres, yres);
  theta2 = gimp_measure_tool_get_angle (bx, by, xres, yres);

  measure->angle1 = theta1;
  measure->angle2 = theta2;

  unit_angle = fabs (theta1 - theta2);
  if (unit_angle > 180.0)
    unit_angle = fabs (360.0 - unit_angle);

  if (shell->unit == GIMP_UNIT_PIXEL)
    {
      gimp_tool_replace_status (GIMP_TOOL (measure), display,
                                "%.1f %s, %.2f\302\260 (%d × %d)",
                                pixel_distance, _("pixels"), pixel_angle,
                                pixel_width, pixel_height);
    }
  else
    {
      g_snprintf (format, sizeof (format),
                  "%%.%df %s, %%.2f\302\260 (%%.%df × %%.%df)",
                  gimp_unit_get_digits (shell->unit),
                  gimp_unit_get_plural (shell->unit),
                  gimp_unit_get_digits (shell->unit),
                  gimp_unit_get_digits (shell->unit));

      gimp_tool_replace_status (GIMP_TOOL (measure), display, format,
                                unit_distance, unit_angle,
                                unit_width, unit_height);
    }
  measure->status_help = FALSE;

  if (measure->gui)
    {
      gchar buf[128];

      g_snprintf (format, sizeof (format), "%%.%df",
                  gimp_unit_get_digits (shell->unit));

      /* Distance */
      g_snprintf (buf, sizeof (buf), "%.1f", pixel_distance);
      gtk_label_set_text (GTK_LABEL (measure->distance_label[0]), buf);

      if (shell->unit != GIMP_UNIT_PIXEL)
        {
          g_snprintf (buf, sizeof (buf), format, unit_distance);
          gtk_label_set_text (GTK_LABEL (measure->distance_label[1]), buf);

          gtk_label_set_text (GTK_LABEL (measure->unit_label[0]),
                              gimp_unit_get_plural (shell->unit));
        }
      else
        {
          gtk_label_set_text (GTK_LABEL (measure->distance_label[1]), NULL);
          gtk_label_set_text (GTK_LABEL (measure->unit_label[0]), NULL);
        }

      /* Angle */
      g_snprintf (buf, sizeof (buf), "%.2f", pixel_angle);
      gtk_label_set_text (GTK_LABEL (measure->angle_label[0]), buf);

      if (fabs (unit_angle - pixel_angle) > 0.01)
        {
          g_snprintf (buf, sizeof (buf), "%.2f", unit_angle);
          gtk_label_set_text (GTK_LABEL (measure->angle_label[1]), buf);

          gtk_label_set_text (GTK_LABEL (measure->unit_label[1]), "\302\260");
        }
      else
        {
          gtk_label_set_text (GTK_LABEL (measure->angle_label[1]), NULL);
          gtk_label_set_text (GTK_LABEL (measure->unit_label[1]), NULL);
        }

      /* Width */
      g_snprintf (buf, sizeof (buf), "%d", pixel_width);
      gtk_label_set_text (GTK_LABEL (measure->width_label[0]), buf);

      if (shell->unit != GIMP_UNIT_PIXEL)
        {
          g_snprintf (buf, sizeof (buf), format, unit_width);
          gtk_label_set_text (GTK_LABEL (measure->width_label[1]), buf);

          gtk_label_set_text (GTK_LABEL (measure->unit_label[2]),
                              gimp_unit_get_plural (shell->unit));
        }
      else
        {
          gtk_label_set_text (GTK_LABEL (measure->width_label[1]), NULL);
          gtk_label_set_text (GTK_LABEL (measure->unit_label[2]), NULL);
        }

      g_snprintf (buf, sizeof (buf), "%d", pixel_height);
      gtk_label_set_text (GTK_LABEL (measure->height_label[0]), buf);

      /* Height */
      if (shell->unit != GIMP_UNIT_PIXEL)
        {
          g_snprintf (buf, sizeof (buf), format, unit_height);
          gtk_label_set_text (GTK_LABEL (measure->height_label[1]), buf);

          gtk_label_set_text (GTK_LABEL (measure->unit_label[3]),
                              gimp_unit_get_plural (shell->unit));
        }
      else
        {
          gtk_label_set_text (GTK_LABEL (measure->height_label[1]), NULL);
          gtk_label_set_text (GTK_LABEL (measure->unit_label[3]), NULL);
        }

      gimp_tool_gui_show (measure->gui);
    }
}

static GimpToolGui *
gimp_measure_tool_dialog_new (GimpMeasureTool *measure)
{
  GimpTool         *tool = GIMP_TOOL (measure);
  GimpDisplayShell *shell;
  GimpToolGui      *gui;
  GtkWidget        *table;
  GtkWidget        *label;

  g_return_val_if_fail (tool->display != NULL, NULL);

  shell = gimp_display_get_shell (tool->display);

  gui = gimp_tool_gui_new (tool->tool_info,
                           NULL,
                           _("Measure Distances and Angles"),
                           NULL, NULL,
                           gtk_widget_get_screen (GTK_WIDGET (shell)),
                           gimp_widget_get_monitor (GTK_WIDGET (shell)),
                           TRUE,

                           GTK_STOCK_CLOSE, GTK_RESPONSE_CLOSE,

                           NULL);

  gimp_tool_gui_set_auto_overlay (gui, TRUE);
  gimp_tool_gui_set_focus_on_map (gui, FALSE);

  g_signal_connect (gui, "response",
                    G_CALLBACK (g_object_unref),
                    NULL);

  table = gtk_table_new (4, 5, TRUE);
  gtk_table_set_col_spacings (GTK_TABLE (table), 6);
  gtk_table_set_row_spacings (GTK_TABLE (table), 6);
  gtk_box_pack_start (GTK_BOX (gimp_tool_gui_get_vbox (gui)), table,
                      TRUE, TRUE, 0);
  gtk_widget_show (table);


  label = gtk_label_new (_("Distance:"));
  gtk_label_set_xalign (GTK_LABEL (label), 0.0);
  gtk_table_attach_defaults (GTK_TABLE (table), label, 0, 1, 0, 1);
  gtk_widget_show (label);

  measure->distance_label[0] = label = gtk_label_new ("0.0");
  gtk_label_set_selectable (GTK_LABEL (label), TRUE);
  gtk_label_set_xalign (GTK_LABEL (label), 1.0);
  gtk_table_attach_defaults (GTK_TABLE (table), label, 1, 2, 0, 1);
  gtk_widget_show (label);

  label = gtk_label_new (_("pixels"));
  gtk_label_set_xalign (GTK_LABEL (label), 0.0);
  gtk_table_attach_defaults (GTK_TABLE (table), label, 2, 3, 0, 1);
  gtk_widget_show (label);

  measure->distance_label[1] = label = gtk_label_new ("0.0");
  gtk_label_set_selectable (GTK_LABEL (label), TRUE);
  gtk_label_set_xalign (GTK_LABEL (label), 1.0);
  gtk_table_attach_defaults (GTK_TABLE (table), label, 3, 4, 0, 1);
  gtk_widget_show (label);

  measure->unit_label[0] = label = gtk_label_new (NULL);
  gtk_label_set_xalign (GTK_LABEL (label), 0.0);
  gtk_table_attach_defaults (GTK_TABLE (table), label, 4, 5, 0, 1);
  gtk_widget_show (label);


  label = gtk_label_new (_("Angle:"));
  gtk_label_set_xalign (GTK_LABEL (label), 0.0);
  gtk_table_attach_defaults (GTK_TABLE (table), label, 0, 1, 1, 2);
  gtk_widget_show (label);

  measure->angle_label[0] = label = gtk_label_new ("0.0");
  gtk_label_set_selectable (GTK_LABEL (label), TRUE);
  gtk_label_set_xalign (GTK_LABEL (label), 1.0);
  gtk_table_attach_defaults (GTK_TABLE (table), label, 1, 2, 1, 2);
  gtk_widget_show (label);

  label = gtk_label_new ("\302\260");
  gtk_label_set_xalign (GTK_LABEL (label), 0.0);
  gtk_table_attach_defaults (GTK_TABLE (table), label, 2, 3, 1, 2);
  gtk_widget_show (label);

  measure->angle_label[1] = label = gtk_label_new (NULL);
  gtk_label_set_selectable (GTK_LABEL (label), TRUE);
  gtk_label_set_xalign (GTK_LABEL (label), 1.0);
  gtk_table_attach_defaults (GTK_TABLE (table), label, 3, 4, 1, 2);
  gtk_widget_show (label);

  measure->unit_label[1] = label = gtk_label_new (NULL);
  gtk_label_set_xalign (GTK_LABEL (label), 0.0);
  gtk_table_attach_defaults (GTK_TABLE (table), label, 4, 5, 1, 2);
  gtk_widget_show (label);


  label = gtk_label_new (_("Width:"));
  gtk_label_set_xalign (GTK_LABEL (label), 0.0);
  gtk_table_attach_defaults (GTK_TABLE (table), label, 0, 1, 2, 3);
  gtk_widget_show (label);

  measure->width_label[0] = label = gtk_label_new ("0.0");
  gtk_label_set_selectable (GTK_LABEL (label), TRUE);
  gtk_label_set_xalign (GTK_LABEL (label), 1.0);
  gtk_table_attach_defaults (GTK_TABLE (table), label, 1, 2, 2, 3);
  gtk_widget_show (label);

  label = gtk_label_new (_("pixels"));
  gtk_label_set_xalign (GTK_LABEL (label), 0.0);
  gtk_table_attach_defaults (GTK_TABLE (table), label, 2, 3, 2, 3);
  gtk_widget_show (label);

  measure->width_label[1] = label = gtk_label_new ("0.0");
  gtk_label_set_selectable (GTK_LABEL (label), TRUE);
  gtk_label_set_xalign (GTK_LABEL (label), 1.0);
  gtk_table_attach_defaults (GTK_TABLE (table), label, 3, 4, 2, 3);
  gtk_widget_show (label);

  measure->unit_label[2] = label = gtk_label_new (NULL);
  gtk_label_set_xalign (GTK_LABEL (label), 0.0);
  gtk_table_attach_defaults (GTK_TABLE (table), label, 4, 5, 2, 3);
  gtk_widget_show (label);


  label = gtk_label_new (_("Height:"));
  gtk_label_set_xalign (GTK_LABEL (label), 0.0);
  gtk_table_attach_defaults (GTK_TABLE (table), label, 0, 1, 3, 4);
  gtk_widget_show (label);

  measure->height_label[0] = label = gtk_label_new ("0.0");
  gtk_label_set_selectable (GTK_LABEL (label), TRUE);
  gtk_label_set_xalign (GTK_LABEL (label), 1.0);
  gtk_table_attach_defaults (GTK_TABLE (table), label, 1, 2, 3, 4);
  gtk_widget_show (label);

  label = gtk_label_new (_("pixels"));
  gtk_label_set_xalign (GTK_LABEL (label), 0.0);
  gtk_table_attach_defaults (GTK_TABLE (table), label, 2, 3, 3, 4);
  gtk_widget_show (label);

  measure->height_label[1] = label = gtk_label_new ("0.0");
  gtk_label_set_selectable (GTK_LABEL (label), TRUE);
  gtk_label_set_xalign (GTK_LABEL (label), 1.0);
  gtk_table_attach_defaults (GTK_TABLE (table), label, 3, 4, 3, 4);
  gtk_widget_show (label);

  measure->unit_label[3] = label = gtk_label_new (NULL);
  gtk_label_set_xalign (GTK_LABEL (label), 0.0);
  gtk_table_attach_defaults (GTK_TABLE (table), label, 4, 5, 3, 4);
  gtk_widget_show (label);

  return gui;
}
