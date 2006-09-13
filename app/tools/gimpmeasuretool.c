/* The GIMP -- an image manipulation program
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * Measure tool
 * Copyright (C) 1999-2003 Sven Neumann <sven@gimp.org>
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

#include <stdlib.h>

#include <gtk/gtk.h>
#include <gdk/gdkkeysyms.h>

#include "libgimpmath/gimpmath.h"
#include "libgimpwidgets/gimpwidgets.h"

#include "tools-types.h"

#include "core/gimpimage.h"
#include "core/gimpimage-guides.h"
#include "core/gimpimage-undo.h"
#include "core/gimpimage-undo-push.h"
#include "core/gimpunit.h"

#include "widgets/gimphelp-ids.h"
#include "widgets/gimptooldialog.h"
#include "widgets/gimpviewabledialog.h"

#include "display/gimpdisplay.h"
#include "display/gimpdisplayshell.h"

#include "gimpmeasureoptions.h"
#include "gimpmeasuretool.h"
#include "gimptoolcontrol.h"
#include "tools-utils.h"

#include "gimp-intl.h"


/*  definitions  */
#define  TARGET         8
#define  ARC_RADIUS     30


/*  local function prototypes  */

static void     gimp_measure_tool_control         (GimpTool        *tool,
                                                   GimpToolAction   action,
                                                   GimpDisplay     *display);
static void     gimp_measure_tool_button_press    (GimpTool        *tool,
                                                   GimpCoords      *coords,
                                                   guint32          time,
                                                   GdkModifierType  state,
                                                   GimpDisplay     *display);
static void     gimp_measure_tool_button_release  (GimpTool        *tool,
                                                   GimpCoords      *coords,
                                                   guint32          time,
                                                   GdkModifierType  state,
                                                   GimpDisplay     *display);
static void     gimp_measure_tool_motion          (GimpTool        *tool,
                                                   GimpCoords      *coords,
                                                   guint32          time,
                                                   GdkModifierType  state,
                                                   GimpDisplay     *display);
static gboolean gimp_measure_tool_key_press       (GimpTool        *tool,
                                                   GdkEventKey     *kevent,
                                                   GimpDisplay     *display);
static void gimp_measure_tool_active_modifier_key (GimpTool        *tool,
                                                   GdkModifierType  key,
                                                   gboolean         press,
                                                   GdkModifierType  state,
                                                   GimpDisplay     *display);
static void     gimp_measure_tool_cursor_update   (GimpTool        *tool,
                                                   GimpCoords      *coords,
                                                   GdkModifierType  state,
                                                   GimpDisplay     *display);

static void     gimp_measure_tool_draw            (GimpDrawTool    *draw_tool);

static void     gimp_measure_tool_halt            (GimpMeasureTool *measure);

static gdouble     gimp_measure_tool_get_angle     (gint             dx,
                                                    gint             dy,
                                                    gdouble          xres,
                                                    gdouble          yres);

static GtkWidget * gimp_measure_tool_dialog_new    (GimpMeasureTool *measure);
static void        gimp_measure_tool_dialog_update (GimpMeasureTool *measure,
                                                    GimpDisplay     *display);


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
                _("Measure distances and angles"),
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
  tool_class->cursor_update       = gimp_measure_tool_cursor_update;

  draw_tool_class->draw           = gimp_measure_tool_draw;
}

static void
gimp_measure_tool_init (GimpMeasureTool *measure_tool)
{
  GimpTool *tool = GIMP_TOOL (measure_tool);

  gimp_tool_control_set_handle_empty_image (tool->control, TRUE);
  gimp_tool_control_set_tool_cursor        (tool->control,
                                            GIMP_TOOL_CURSOR_MEASURE);
}

static void
gimp_measure_tool_control (GimpTool       *tool,
                           GimpToolAction  action,
                           GimpDisplay    *display)
{
  switch (action)
    {
    case GIMP_TOOL_ACTION_PAUSE:
    case GIMP_TOOL_ACTION_RESUME:
      break;

    case GIMP_TOOL_ACTION_HALT:
      gimp_measure_tool_halt (GIMP_MEASURE_TOOL (tool));
      break;
    }

  GIMP_TOOL_CLASS (parent_class)->control (tool, action, display);
}

static void
gimp_measure_tool_button_press (GimpTool        *tool,
                                GimpCoords      *coords,
                                guint32          time,
                                GdkModifierType  state,
                                GimpDisplay     *display)
{
  GimpMeasureTool    *measure = GIMP_MEASURE_TOOL (tool);
  GimpMeasureOptions *options = GIMP_MEASURE_TOOL_GET_OPTIONS (tool);
  GimpDisplayShell   *shell;
  gint                i;

  shell = GIMP_DISPLAY_SHELL (display->shell);

  /*  if we are changing displays, pop the statusbar of the old one  */
  if (gimp_tool_control_is_active (tool->control) && display != tool->display)
    {
      gimp_tool_pop_status (tool, display);
    }

  measure->function = CREATING;

  measure->mouse_x = coords->x;
  measure->mouse_y = coords->y;

  if (gimp_tool_control_is_active (tool->control) && display == tool->display)
    {
      /*  if the cursor is in one of the handles,
       *  the new function will be moving or adding a new point or guide
       */
      for (i = 0; i < measure->num_points; i++)
        {
          if (gimp_draw_tool_on_handle (GIMP_DRAW_TOOL (tool), display,
                                        coords->x,
                                        coords->y,
                                        GIMP_HANDLE_CIRCLE,
                                        measure->x[i],
                                        measure->y[i],
                                        TARGET, TARGET,
                                        GTK_ANCHOR_CENTER,
                                        FALSE))
            {
              if (state & (GDK_CONTROL_MASK | GDK_MOD1_MASK))
                {
                  GimpGuide *guide;
                  gboolean   create_hguide;
                  gboolean   create_vguide;

                  create_hguide = ((state & GDK_CONTROL_MASK) &&
                                   (measure->y[i] ==
                                    CLAMP (measure->y[i],
                                           0,
                                           display->image->height)));

                  create_vguide = ((state & GDK_MOD1_MASK) &&
                                   (measure->x[i] ==
                                    CLAMP (measure->x[i],
                                           0,
                                           display->image->width)));

                  if (create_hguide && create_vguide)
                    gimp_image_undo_group_start (display->image,
                                                 GIMP_UNDO_GROUP_IMAGE_GUIDE,
                                                 _("Add Guides"));

                  if (create_hguide)
                    {
                      guide = gimp_image_add_hguide (display->image,
                                                     measure->y[i],
                                                     TRUE);
                      gimp_image_update_guide (display->image, guide);
                    }

                  if (create_vguide)
                    {
                      guide = gimp_image_add_vguide (display->image,
                                                     measure->x[i],
                                                     TRUE);
                      gimp_image_update_guide (display->image, guide);
                    }

                  if (create_hguide && create_vguide)
                    gimp_image_undo_group_end (display->image);

                  if (create_hguide || create_vguide)
                    gimp_image_flush (display->image);

                  measure->function = GUIDING;
                  break;
                }

              measure->function = (state & GDK_SHIFT_MASK) ? ADDING : MOVING;
              measure->point = i;
              break;
            }
        }

      /*  adding to the middle point makes no sense  */
      if (i == 0 &&
          measure->function == ADDING &&
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
      if (gimp_tool_control_is_active (tool->control))
        {
          gimp_draw_tool_stop (GIMP_DRAW_TOOL (measure));

          measure->x[0] = measure->x[1] = measure->x[2] = 0.0;
          measure->y[0] = measure->y[1] = measure->y[2] = 0.0;

          gimp_measure_tool_dialog_update (measure, display);
        }

      /*  set the first point and go into ADDING mode  */
      measure->x[0]       = coords->x + 0.5;
      measure->y[0]       = coords->y + 0.5;
      measure->point      = 0;
      measure->num_points = 1;
      measure->function   = ADDING;

      /*  set the displaylay  */
      tool->display = display;

      if (gimp_tool_control_is_active (tool->control))
        {
          gimp_tool_replace_status (tool, display, " ");
        }
      else
        {
          gimp_tool_control_activate (tool->control);
        }

      gimp_draw_tool_start (GIMP_DRAW_TOOL (tool), display);
    }

  /*  create the info window if necessary  */
  if (! measure->dialog && (options->use_info_window ||
                            ! GTK_WIDGET_VISIBLE (shell->statusbar)))
    {
      measure->dialog = gimp_measure_tool_dialog_new (measure);
      g_object_add_weak_pointer (G_OBJECT (measure->dialog),
                                 (gpointer) &measure->dialog);
    }

  if (measure->dialog)
    gimp_viewable_dialog_set_viewable (GIMP_VIEWABLE_DIALOG (measure->dialog),
                                       GIMP_VIEWABLE (tool->display->image),
                                       GIMP_CONTEXT (options));
}

static void
gimp_measure_tool_button_release (GimpTool        *tool,
                                  GimpCoords      *coords,
                                  guint32          time,
                                  GdkModifierType  state,
                                  GimpDisplay     *display)
{
  GimpMeasureTool *measure = GIMP_MEASURE_TOOL (tool);

  measure->function = FINISHED;
}

static void
gimp_measure_tool_motion (GimpTool        *tool,
                          GimpCoords      *coords,
                          guint32          time,
                          GdkModifierType  state,
                          GimpDisplay     *display)
{
  GimpMeasureTool *measure = GIMP_MEASURE_TOOL (tool);
  gint             dx, dy;
  gint             i;
  gint             tmp;

  gimp_draw_tool_pause (GIMP_DRAW_TOOL (measure));

  measure->mouse_x = coords->x;
  measure->mouse_y = coords->y;

  /*
   *  A few comments here, because this routine looks quite weird at first ...
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
        case 0:  /*  we are adding to the start point  */
          break;
        case 1:  /*  we are adding to the end point,
                     make it the new start point  */
          tmp = measure->x[0];
          measure->x[0] = measure->x[1];
          measure->x[1] = tmp;
          tmp = measure->y[0];
          measure->y[0] = measure->y[1];
          measure->y[1] = tmp;
          break;
        case 2:  /*  we are adding to the third point,
                     make it the new start point  */
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
      /*  no, don't break here!  */

    case MOVING:
      /*  if we are moving the start point and only have two,
          make it the end point  */
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

      i = measure->point;

      measure->x[i] = ROUND (coords->x);
      measure->y[i] = ROUND (coords->y);

      if (state & GDK_CONTROL_MASK)
        {
          gdouble  x = measure->x[i];
          gdouble  y = measure->y[i];

          gimp_tool_motion_constrain (measure->x[0], measure->y[0], &x, &y);

          measure->x[i] = ROUND (x);
          measure->y[i] = ROUND (y);
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
        case GDK_Escape:
          gimp_measure_tool_halt (GIMP_MEASURE_TOOL (tool));
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

  if (key == GDK_CONTROL_MASK && measure->function == MOVING)
    {
      gdouble x, y;

      gimp_draw_tool_pause (GIMP_DRAW_TOOL (tool));

      x = measure->mouse_x;
      y = measure->mouse_y;

      if (press)
        gimp_tool_motion_constrain (measure->x[0], measure->y[0], &x, &y);

      measure->x[measure->point] = ROUND (x);
      measure->y[measure->point] = ROUND (y);

      gimp_measure_tool_dialog_update (measure, display);

      gimp_draw_tool_resume (GIMP_DRAW_TOOL (tool));
    }
}

static void
gimp_measure_tool_cursor_update (GimpTool        *tool,
                                 GimpCoords      *coords,
                                 GdkModifierType  state,
                                 GimpDisplay     *display)
{
  GimpMeasureTool   *measure   = GIMP_MEASURE_TOOL (tool);
  gboolean           in_handle = FALSE;
  GimpCursorType     cursor    = GIMP_CURSOR_CROSSHAIR_SMALL;
  GimpCursorModifier modifier  = GIMP_CURSOR_MODIFIER_NONE;
  gint               i;

  if (gimp_tool_control_is_active (tool->control) && tool->display == display)
    {
      for (i = 0; i < measure->num_points; i++)
        {
          if (gimp_draw_tool_on_handle (GIMP_DRAW_TOOL (tool), display,
                                        coords->x,
                                        coords->y,
                                        GIMP_HANDLE_CIRCLE,
                                        measure->x[i],
                                        measure->y[i],
                                        TARGET, TARGET,
                                        GTK_ANCHOR_CENTER,
                                        FALSE))
            {
              in_handle = TRUE;

              if (state & GDK_CONTROL_MASK)
                {
                  if (state & GDK_MOD1_MASK)
                    cursor = GIMP_CURSOR_CORNER_BOTTOM_RIGHT;
                  else
                    cursor = GIMP_CURSOR_SIDE_BOTTOM;
                  break;
                }

              if (state & GDK_MOD1_MASK)
                {
                  cursor = GIMP_CURSOR_SIDE_RIGHT;
                  break;
                }

              if (state & GDK_SHIFT_MASK)
                modifier = GIMP_CURSOR_MODIFIER_PLUS;
              else
                modifier = GIMP_CURSOR_MODIFIER_MOVE;

              if (i == 0 && measure->num_points == 3 &&
                  modifier == GIMP_CURSOR_MODIFIER_PLUS)
                modifier = GIMP_CURSOR_MODIFIER_MOVE;
              break;
            }
        }

      if (! in_handle && measure->num_points > 1 && state & GDK_MOD1_MASK)
        modifier = GIMP_CURSOR_MODIFIER_MOVE;
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
  gint             i;
  gint             angle1, angle2;
  gint             draw_arc = 0;

  for (i = 0; i < measure->num_points; i++)
    {
      if (i == 0 && measure->num_points == 3)
        {
          gimp_draw_tool_draw_handle (draw_tool,
                                      GIMP_HANDLE_CIRCLE,
                                      measure->x[i],
                                      measure->y[i],
                                      TARGET,
                                      TARGET,
                                      GTK_ANCHOR_CENTER,
                                      FALSE);
        }
      else
        {
          gimp_draw_tool_draw_handle (draw_tool,
                                      GIMP_HANDLE_CROSS,
                                      measure->x[i],
                                      measure->y[i],
                                      TARGET * 2,
                                      TARGET * 2,
                                      GTK_ANCHOR_CENTER,
                                      FALSE);
        }

      if (i > 0)
        {
          gimp_draw_tool_draw_line (draw_tool,
                                    measure->x[0],
                                    measure->y[0],
                                    measure->x[i],
                                    measure->y[i],
                                    FALSE);

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

  if (measure->num_points > 1 && draw_arc == measure->num_points - 1)
    {
      angle1 = measure->angle2 * 64.0;
      angle2 = (measure->angle1 - measure->angle2) * 64.0;

      if (angle2 > 11520)
          angle2 -= 23040;
      if (angle2 < -11520)
          angle2 += 23040;

      if (angle2 != 0)
        {
          gimp_draw_tool_draw_arc_by_anchor (draw_tool,
                                             FALSE,
                                             measure->x[0],
                                             measure->y[0],
                                             ARC_RADIUS,
                                             ARC_RADIUS,
                                             angle1, angle2,
                                             GTK_ANCHOR_CENTER,
                                             FALSE);

          if (measure->num_points == 2)
            {
              GimpDisplayShell *shell;
              gdouble           target;
              gdouble           arc_radius;

              shell = GIMP_DISPLAY_SHELL (tool->display->shell);

              target     = FUNSCALEX (shell, (TARGET >> 1));
              arc_radius = FUNSCALEX (shell, ARC_RADIUS);

              gimp_draw_tool_draw_line (draw_tool,
                                        measure->x[0],
                                        measure->y[0],
                                        (measure->x[1] >= measure->x[0] ?
                                         measure->x[0] + arc_radius + target :
                                         measure->x[0] - arc_radius - target),
                                        measure->y[0],
                                        FALSE);
            }
        }
    }
}

static void
gimp_measure_tool_halt (GimpMeasureTool *measure)
{
  GimpTool *tool = GIMP_TOOL (measure);

  if (measure->dialog)
    gtk_widget_destroy (measure->dialog);

  gimp_tool_pop_status (tool, tool->display);

  if (gimp_draw_tool_is_active (GIMP_DRAW_TOOL (measure)))
    gimp_draw_tool_stop (GIMP_DRAW_TOOL (measure));

  if (gimp_tool_control_is_active (tool->control))
    gimp_tool_control_halt (tool->control);
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
  GimpDisplayShell *shell = GIMP_DISPLAY_SHELL (display->shell);
  GimpImage        *image = display->image;
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
  gchar             format[128];
  gchar             buf[128];

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

  unit_width  = (_gimp_unit_get_factor (image->gimp, shell->unit) *
                 pixel_width / image->xresolution);
  unit_height = (_gimp_unit_get_factor (image->gimp, shell->unit) *
                 pixel_height / image->yresolution);

  pixel_distance = sqrt (SQR (ax - bx) + SQR (ay - by));
  unit_distance  = (_gimp_unit_get_factor (image->gimp, shell->unit) *
                    sqrt (SQR ((gdouble)(ax - bx) / image->xresolution) +
                          SQR ((gdouble)(ay - by) / image->yresolution)));

  if (measure->num_points != 3)
    bx = ax > 0 ? 1 : -1;

  theta1 = gimp_measure_tool_get_angle (ax, ay, 1.0, 1.0);
  theta2 = gimp_measure_tool_get_angle (bx, by, 1.0, 1.0);

  pixel_angle = fabs (theta1 - theta2);
  if (pixel_angle > 180.0)
    pixel_angle = fabs (360.0 - pixel_angle);

  theta1 = gimp_measure_tool_get_angle (ax, ay,
                                        image->xresolution, image->yresolution);
  theta2 = gimp_measure_tool_get_angle (bx, by,
                                        image->xresolution, image->yresolution);

  measure->angle1 = theta1;
  measure->angle2 = theta2;

  unit_angle = fabs (theta1 - theta2);
  if (unit_angle > 180.0)
    unit_angle = fabs (360.0 - unit_angle);

  if (shell->unit == GIMP_UNIT_PIXEL)
    {
      g_snprintf (buf, sizeof (buf), "%.1f %s, %.2f\302\260 (%d × %d)",
                  pixel_distance, _("pixels"), pixel_angle,
                  pixel_width, pixel_height);
    }
  else
    {
      g_snprintf (format, sizeof (format),
                  "%%.%df %s, %%.2f\302\260 (%%.%df × %%.%df)",
                  _gimp_unit_get_digits (image->gimp, shell->unit),
                  _gimp_unit_get_plural (image->gimp, shell->unit),
                  _gimp_unit_get_digits (image->gimp, shell->unit),
                  _gimp_unit_get_digits (image->gimp, shell->unit));

      g_snprintf (buf, sizeof (buf), format, unit_distance, unit_angle,
                  unit_width, unit_height);
    }

  gimp_tool_replace_status (GIMP_TOOL (measure), display, buf);

  if (measure->dialog)
    {
      g_snprintf (format, sizeof (format), "%%.%df",
                  _gimp_unit_get_digits (image->gimp, shell->unit));
      /* Distance */
      g_snprintf (buf, sizeof (buf), "%.1f", pixel_distance);
      gtk_label_set_text (GTK_LABEL (measure->distance_label[0]), buf);

      if (shell->unit != GIMP_UNIT_PIXEL)
        {
          g_snprintf (buf, sizeof (buf), format, unit_distance);
          gtk_label_set_text (GTK_LABEL (measure->distance_label[1]), buf);

          gtk_label_set_text (GTK_LABEL (measure->unit_label[0]),
                              _gimp_unit_get_plural (image->gimp, shell->unit));
        }
      else
        {
          gtk_label_set_text (GTK_LABEL (measure->distance_label[1]), " ");
          gtk_label_set_text (GTK_LABEL (measure->unit_label[0]),  " ");
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
          gtk_label_set_text (GTK_LABEL (measure->angle_label[1]), " ");
          gtk_label_set_text (GTK_LABEL (measure->unit_label[1]),  " ");
        }

      /* Width */
      g_snprintf (buf, sizeof (buf), "%d", pixel_width);
      gtk_label_set_text (GTK_LABEL (measure->width_label[0]), buf);

      if (shell->unit != GIMP_UNIT_PIXEL)
        {
          g_snprintf (buf, sizeof (buf), format, unit_width);
          gtk_label_set_text (GTK_LABEL (measure->width_label[1]), buf);

          gtk_label_set_text (GTK_LABEL (measure->unit_label[2]),
                              _gimp_unit_get_plural (image->gimp, shell->unit));
        }
      else
        {
          gtk_label_set_text (GTK_LABEL (measure->width_label[1]), " ");
          gtk_label_set_text (GTK_LABEL (measure->unit_label[2]), " ");
        }

      g_snprintf (buf, sizeof (buf), "%d", pixel_height);
      gtk_label_set_text (GTK_LABEL (measure->height_label[0]), buf);

      /* Height */
      if (shell->unit != GIMP_UNIT_PIXEL)
        {
          g_snprintf (buf, sizeof (buf), format, unit_height);
          gtk_label_set_text (GTK_LABEL (measure->height_label[1]), buf);

          gtk_label_set_text (GTK_LABEL (measure->unit_label[3]),
                              _gimp_unit_get_plural (image->gimp, shell->unit));
        }
      else
        {
          gtk_label_set_text (GTK_LABEL (measure->height_label[1]), " ");
          gtk_label_set_text (GTK_LABEL (measure->unit_label[3]), " ");
        }

      if (GTK_WIDGET_VISIBLE (measure->dialog))
        gdk_window_show (measure->dialog->window);
      else
        gtk_widget_show (measure->dialog);
    }
}

static GtkWidget *
gimp_measure_tool_dialog_new (GimpMeasureTool *measure)
{
  GimpTool  *tool = GIMP_TOOL (measure);
  GtkWidget *dialog;
  GtkWidget *table;
  GtkWidget *label;

  dialog = gimp_tool_dialog_new (tool->tool_info,
                                 NULL /* tool->display->shell */,
                                 _("Measure Distances and Angles"),

                                 GTK_STOCK_CLOSE, GTK_RESPONSE_CLOSE,

                                 NULL);

  gtk_window_set_focus_on_map (GTK_WINDOW (dialog), FALSE);

  g_signal_connect (dialog, "response",
                    G_CALLBACK (gtk_widget_destroy),
                    NULL);

  table = gtk_table_new (4, 5, TRUE);
  gtk_table_set_col_spacings (GTK_TABLE (table), 6);
  gtk_table_set_row_spacings (GTK_TABLE (table), 6);
  gtk_container_set_border_width (GTK_CONTAINER (table), 6);
  gtk_container_add (GTK_CONTAINER (GTK_DIALOG (dialog)->vbox), table);
  gtk_widget_show (table);


  label = gtk_label_new (_("Distance:"));
  gtk_misc_set_alignment (GTK_MISC (label), 0.0, 0.5);
  gtk_table_attach_defaults (GTK_TABLE (table), label, 0, 1, 0, 1);
  gtk_widget_show (label);

  measure->distance_label[0] = label = gtk_label_new ("0.0");
  gtk_misc_set_alignment (GTK_MISC (label), 1.0, 0.5);
  gtk_table_attach_defaults (GTK_TABLE (table), label, 1, 2, 0, 1);
  gtk_widget_show (label);

  label = gtk_label_new (_("pixels"));
  gtk_misc_set_alignment (GTK_MISC (label), 0.0, 0.5);
  gtk_table_attach_defaults (GTK_TABLE (table), label, 2, 3, 0, 1);
  gtk_widget_show (label);

  measure->distance_label[1] = label = gtk_label_new ("0.0");
  gtk_misc_set_alignment (GTK_MISC (label), 1.0, 0.5);
  gtk_table_attach_defaults (GTK_TABLE (table), label, 3, 4, 0, 1);
  gtk_widget_show (label);

  measure->unit_label[0] = label = gtk_label_new (" ");
  gtk_misc_set_alignment (GTK_MISC (label), 0.0, 0.5);
  gtk_table_attach_defaults (GTK_TABLE (table), label, 4, 5, 0, 1);
  gtk_widget_show (label);


  label = gtk_label_new (_("Angle:"));
  gtk_misc_set_alignment (GTK_MISC (label), 0.0, 0.5);
  gtk_table_attach_defaults (GTK_TABLE (table), label, 0, 1, 1, 2);
  gtk_widget_show (label);

  measure->angle_label[0] = label = gtk_label_new ("0.0");
  gtk_misc_set_alignment (GTK_MISC (label), 1.0, 0.5);
  gtk_table_attach_defaults (GTK_TABLE (table), label, 1, 2, 1, 2);
  gtk_widget_show (label);

  label = gtk_label_new ("\302\260");
  gtk_misc_set_alignment (GTK_MISC (label), 0.0, 0.5);
  gtk_table_attach_defaults (GTK_TABLE (table), label, 2, 3, 1, 2);
  gtk_widget_show (label);

  measure->angle_label[1] = label = gtk_label_new (" ");
  gtk_misc_set_alignment (GTK_MISC (label), 1.0, 0.5);
  gtk_table_attach_defaults (GTK_TABLE (table), label, 3, 4, 1, 2);
  gtk_widget_show (label);

  measure->unit_label[1] = label = gtk_label_new (" ");
  gtk_misc_set_alignment (GTK_MISC (label), 0.0, 0.5);
  gtk_table_attach_defaults (GTK_TABLE (table), label, 4, 5, 1, 2);
  gtk_widget_show (label);


  label = gtk_label_new (_("Width:"));
  gtk_misc_set_alignment (GTK_MISC (label), 0.0, 0.5);
  gtk_table_attach_defaults (GTK_TABLE (table), label, 0, 1, 2, 3);
  gtk_widget_show (label);

  measure->width_label[0] = label = gtk_label_new ("0.0");
  gtk_misc_set_alignment (GTK_MISC (label), 1.0, 0.5);
  gtk_table_attach_defaults (GTK_TABLE (table), label, 1, 2, 2, 3);
  gtk_widget_show (label);

  label = gtk_label_new (_("pixels"));
  gtk_misc_set_alignment (GTK_MISC (label), 0.0, 0.5);
  gtk_table_attach_defaults (GTK_TABLE (table), label, 2, 3, 2, 3);
  gtk_widget_show (label);

  measure->width_label[1] = label = gtk_label_new ("0.0");
  gtk_misc_set_alignment (GTK_MISC (label), 1.0, 0.5);
  gtk_table_attach_defaults (GTK_TABLE (table), label, 3, 4, 2, 3);
  gtk_widget_show (label);

  measure->unit_label[2] = label = gtk_label_new (" ");
  gtk_misc_set_alignment (GTK_MISC (label), 0.0, 0.5);
  gtk_table_attach_defaults (GTK_TABLE (table), label, 4, 5, 2, 3);
  gtk_widget_show (label);


  label = gtk_label_new (_("Height:"));
  gtk_misc_set_alignment (GTK_MISC (label), 0.0, 0.5);
  gtk_table_attach_defaults (GTK_TABLE (table), label, 0, 1, 3, 4);
  gtk_widget_show (label);

  measure->height_label[0] = label = gtk_label_new ("0.0");
  gtk_misc_set_alignment (GTK_MISC (label), 1.0, 0.5);
  gtk_table_attach_defaults (GTK_TABLE (table), label, 1, 2, 3, 4);
  gtk_widget_show (label);

  label = gtk_label_new (_("pixels"));
  gtk_misc_set_alignment (GTK_MISC (label), 0.0, 0.5);
  gtk_table_attach_defaults (GTK_TABLE (table), label, 2, 3, 3, 4);
  gtk_widget_show (label);

  measure->height_label[1] = label = gtk_label_new ("0.0");
  gtk_misc_set_alignment (GTK_MISC (label), 1.0, 0.5);
  gtk_table_attach_defaults (GTK_TABLE (table), label, 3, 4, 3, 4);
  gtk_widget_show (label);

  measure->unit_label[3] = label = gtk_label_new (" ");
  gtk_misc_set_alignment (GTK_MISC (label), 0.0, 0.5);
  gtk_table_attach_defaults (GTK_TABLE (table), label, 4, 5, 3, 4);
  gtk_widget_show (label);

  return dialog;
}
