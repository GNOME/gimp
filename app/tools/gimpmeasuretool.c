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
static void     gimp_measure_tool_cursor_update   (GimpTool        *tool,
                                                   GimpCoords      *coords,
                                                   GdkModifierType  state,
                                                   GimpDisplay     *display);

static void     gimp_measure_tool_draw            (GimpDrawTool    *draw_tool);

static void     gimp_measure_tool_halt            (GimpMeasureTool *mtool);

static gdouble     gimp_measure_tool_get_angle     (gint             dx,
                                                    gint             dy,
                                                    gdouble          xres,
                                                    gdouble          yres);

static GtkWidget * gimp_measure_tool_dialog_new    (GimpMeasureTool *mtool);
static void        gimp_measure_tool_dialog_update (GimpMeasureTool *mtool,
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

  tool_class->control        = gimp_measure_tool_control;
  tool_class->button_press   = gimp_measure_tool_button_press;
  tool_class->button_release = gimp_measure_tool_button_release;
  tool_class->motion         = gimp_measure_tool_motion;
  tool_class->key_press      = gimp_measure_tool_key_press;
  tool_class->cursor_update  = gimp_measure_tool_cursor_update;

  draw_tool_class->draw      = gimp_measure_tool_draw;
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
  GimpMeasureTool    *mtool   = GIMP_MEASURE_TOOL (tool);
  GimpMeasureOptions *options = GIMP_MEASURE_TOOL_GET_OPTIONS (tool);
  GimpDisplayShell   *shell;
  gint                i;

  shell = GIMP_DISPLAY_SHELL (display->shell);

  /*  if we are changing displays, pop the statusbar of the old one  */
  if (gimp_tool_control_is_active (tool->control) && display != tool->display)
    {
      gimp_tool_pop_status (tool, display);
    }

  mtool->function = CREATING;

  if (gimp_tool_control_is_active (tool->control) && display == tool->display)
    {
      /*  if the cursor is in one of the handles,
       *  the new function will be moving or adding a new point or guide
       */
      for (i = 0; i < mtool->num_points; i++)
        {
          if (gimp_draw_tool_on_handle (GIMP_DRAW_TOOL (tool), display,
                                        coords->x,
                                        coords->y,
                                        GIMP_HANDLE_CIRCLE,
                                        mtool->x[i],
                                        mtool->y[i],
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
                                   (mtool->y[i] ==
                                    CLAMP (mtool->y[i],
                                           0,
                                           display->image->height)));

                  create_vguide = ((state & GDK_MOD1_MASK) &&
                                   (mtool->x[i] ==
                                    CLAMP (mtool->x[i],
                                           0,
                                           display->image->width)));

                  if (create_hguide && create_vguide)
                    gimp_image_undo_group_start (display->image,
                                                 GIMP_UNDO_GROUP_IMAGE_GUIDE,
                                                 _("Add Guides"));

                  if (create_hguide)
                    {
                      guide = gimp_image_add_hguide (display->image,
                                                     mtool->y[i],
                                                     TRUE);
                      gimp_image_update_guide (display->image, guide);
                    }

                  if (create_vguide)
                    {
                      guide = gimp_image_add_vguide (display->image,
                                                     mtool->x[i],
                                                     TRUE);
                      gimp_image_update_guide (display->image, guide);
                    }

                  if (create_hguide && create_vguide)
                    gimp_image_undo_group_end (display->image);

                  if (create_hguide || create_vguide)
                    gimp_image_flush (display->image);

                  mtool->function = GUIDING;
                  break;
                }

              mtool->function = (state & GDK_SHIFT_MASK) ? ADDING : MOVING;
              mtool->point = i;
              break;
            }
        }

      /*  adding to the middle point makes no sense  */
      if (i == 0 &&
          mtool->function == ADDING &&
          mtool->num_points == 3)
        {
          mtool->function = MOVING;
        }

      /*  if the function is still CREATING, we are outside the handles  */
      if (mtool->function == CREATING)
        {
          if (mtool->num_points > 1 && (state & GDK_MOD1_MASK))
            {
              mtool->function = MOVING_ALL;

              mtool->last_x = coords->x;
              mtool->last_y = coords->y;
            }
        }
    }

  if (mtool->function == CREATING)
    {
      if (gimp_tool_control_is_active (tool->control))
        {
          gimp_draw_tool_stop (GIMP_DRAW_TOOL (mtool));

          mtool->x[0] = mtool->x[1] = mtool->x[2] = 0.0;
          mtool->y[0] = mtool->y[1] = mtool->y[2] = 0.0;

          gimp_measure_tool_dialog_update (mtool, display);
        }

      /*  set the first point and go into ADDING mode  */
      mtool->x[0]       = coords->x + 0.5;
      mtool->y[0]       = coords->y + 0.5;
      mtool->point      = 0;
      mtool->num_points = 1;
      mtool->function   = ADDING;

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
  if (! mtool->dialog && (options->use_info_window ||
                          ! GTK_WIDGET_VISIBLE (shell->statusbar)))
    {
      mtool->dialog = gimp_measure_tool_dialog_new (mtool);
      g_object_add_weak_pointer (G_OBJECT (mtool->dialog),
                                 (gpointer) &mtool->dialog);
    }

  if (mtool->dialog)
    gimp_viewable_dialog_set_viewable (GIMP_VIEWABLE_DIALOG (mtool->dialog),
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
  GimpMeasureTool *measure_tool = GIMP_MEASURE_TOOL (tool);

  measure_tool->function = FINISHED;
}

static void
gimp_measure_tool_motion (GimpTool        *tool,
                          GimpCoords      *coords,
                          guint32          time,
                          GdkModifierType  state,
                          GimpDisplay     *display)
{
  GimpMeasureTool    *mtool = GIMP_MEASURE_TOOL (tool);
  gint                dx, dy;
  gint                i;
  gint                tmp;

  gimp_draw_tool_pause (GIMP_DRAW_TOOL (mtool));

  /*
   *  A few comments here, because this routine looks quite weird at first ...
   *
   *  The goal is to keep point 0, called the start point, to be
   *  always the one in the middle or, if there are only two points,
   *  the one that is fixed.  The angle is then always measured at
   *  this point.
   */

  switch (mtool->function)
    {
    case ADDING:
      switch (mtool->point)
        {
        case 0:  /*  we are adding to the start point  */
          break;
        case 1:  /*  we are adding to the end point,
                     make it the new start point  */
          tmp = mtool->x[0];
          mtool->x[0] = mtool->x[1];
          mtool->x[1] = tmp;
          tmp = mtool->y[0];
          mtool->y[0] = mtool->y[1];
          mtool->y[1] = tmp;
          break;
        case 2:  /*  we are adding to the third point,
                     make it the new start point  */
          mtool->x[1] = mtool->x[0];
          mtool->y[1] = mtool->y[0];
          mtool->x[0] = mtool->x[2];
          mtool->y[0] = mtool->y[2];
          break;
        default:
          break;
        }
      mtool->num_points = MIN (mtool->num_points + 1, 3);
      mtool->point = mtool->num_points - 1;
      mtool->function = MOVING;
      /*  no, don't break here!  */

    case MOVING:
      /*  if we are moving the start point and only have two,
          make it the end point  */
      if (mtool->num_points == 2 && mtool->point == 0)
        {
          tmp = mtool->x[0];
          mtool->x[0] = mtool->x[1];
          mtool->x[1] = tmp;
          tmp = mtool->y[0];
          mtool->y[0] = mtool->y[1];
          mtool->y[1] = tmp;
          mtool->point = 1;
        }
      i = mtool->point;

      mtool->x[i] = ROUND (coords->x);
      mtool->y[i] = ROUND (coords->y);

      if (state & GDK_CONTROL_MASK)
        {
          gdouble  x = mtool->x[i];
          gdouble  y = mtool->y[i];

          gimp_tool_motion_constrain (mtool->x[0], mtool->y[0], &x, &y);

          mtool->x[i] = ROUND (x);
          mtool->y[i] = ROUND (y);
        }
      break;

    case MOVING_ALL:
      dx = ROUND (coords->x) - mtool->last_x;
      dy = ROUND (coords->y) - mtool->last_y;

      for (i = 0; i < mtool->num_points; i++)
        {
          mtool->x[i] += dx;
          mtool->y[i] += dy;
        }

      mtool->last_x = ROUND (coords->x);
      mtool->last_y = ROUND (coords->y);
      break;

    default:
      break;
    }

  if (mtool->function == MOVING)
    gimp_measure_tool_dialog_update (mtool, display);

  gimp_draw_tool_resume (GIMP_DRAW_TOOL (mtool));
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
gimp_measure_tool_cursor_update (GimpTool        *tool,
                                 GimpCoords      *coords,
                                 GdkModifierType  state,
                                 GimpDisplay     *display)
{
  GimpMeasureTool   *mtool     = GIMP_MEASURE_TOOL (tool);
  gboolean           in_handle = FALSE;
  GimpCursorType     cursor    = GIMP_CURSOR_CROSSHAIR_SMALL;
  GimpCursorModifier modifier  = GIMP_CURSOR_MODIFIER_NONE;
  gint               i;

  if (gimp_tool_control_is_active (tool->control) && tool->display == display)
    {
      for (i = 0; i < mtool->num_points; i++)
        {
          if (gimp_draw_tool_on_handle (GIMP_DRAW_TOOL (tool), display,
                                        coords->x,
                                        coords->y,
                                        GIMP_HANDLE_CIRCLE,
                                        mtool->x[i],
                                        mtool->y[i],
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

              if (i == 0 && mtool->num_points == 3 &&
                  modifier == GIMP_CURSOR_MODIFIER_PLUS)
                modifier = GIMP_CURSOR_MODIFIER_MOVE;
              break;
            }
        }

      if (! in_handle && mtool->num_points > 1 && state & GDK_MOD1_MASK)
        modifier = GIMP_CURSOR_MODIFIER_MOVE;
    }

  gimp_tool_control_set_cursor          (tool->control, cursor);
  gimp_tool_control_set_cursor_modifier (tool->control, modifier);

  GIMP_TOOL_CLASS (parent_class)->cursor_update (tool, coords, state, display);
}

static void
gimp_measure_tool_draw (GimpDrawTool *draw_tool)
{
  GimpMeasureTool *mtool = GIMP_MEASURE_TOOL (draw_tool);
  GimpTool        *tool  = GIMP_TOOL (draw_tool);
  gint             i;
  gint             angle1, angle2;
  gint             draw_arc = 0;

  for (i = 0; i < mtool->num_points; i++)
    {
      if (i == 0 && mtool->num_points == 3)
        {
          gimp_draw_tool_draw_handle (draw_tool,
                                      GIMP_HANDLE_CIRCLE,
                                      mtool->x[i],
                                      mtool->y[i],
                                      TARGET,
                                      TARGET,
                                      GTK_ANCHOR_CENTER,
                                      FALSE);
        }
      else
        {
          gimp_draw_tool_draw_handle (draw_tool,
                                      GIMP_HANDLE_CROSS,
                                      mtool->x[i],
                                      mtool->y[i],
                                      TARGET * 2,
                                      TARGET * 2,
                                      GTK_ANCHOR_CENTER,
                                      FALSE);
        }

      if (i > 0)
        {
          gimp_draw_tool_draw_line (draw_tool,
                                    mtool->x[0],
                                    mtool->y[0],
                                    mtool->x[i],
                                    mtool->y[i],
                                    FALSE);

          /*  only draw the arc if the lines are long enough  */
          if (gimp_draw_tool_calc_distance (draw_tool, tool->display,
                                            mtool->x[0],
                                            mtool->y[0],
                                            mtool->x[i],
                                            mtool->y[i]) > ARC_RADIUS)
            {
              draw_arc++;
            }
        }
    }

  if (mtool->num_points > 1 && draw_arc == mtool->num_points - 1)
    {
      angle1 = mtool->angle2 * 64.0;
      angle2 = (mtool->angle1 - mtool->angle2) * 64.0;

      if (angle2 > 11520)
          angle2 -= 23040;
      if (angle2 < -11520)
          angle2 += 23040;

      if (angle2 != 0)
        {
          gimp_draw_tool_draw_arc_by_anchor (draw_tool,
                                             FALSE,
                                             mtool->x[0],
                                             mtool->y[0],
                                             ARC_RADIUS,
                                             ARC_RADIUS,
                                             angle1, angle2,
                                             GTK_ANCHOR_CENTER,
                                             FALSE);

          if (mtool->num_points == 2)
            {
              GimpDisplayShell *shell;
              gdouble           target;
              gdouble           arc_radius;

              shell = GIMP_DISPLAY_SHELL (tool->display->shell);

              target     = FUNSCALEX (shell, (TARGET >> 1));
              arc_radius = FUNSCALEX (shell, ARC_RADIUS);

              gimp_draw_tool_draw_line (draw_tool,
                                        mtool->x[0],
                                        mtool->y[0],
                                        (mtool->x[1] >= mtool->x[0] ?
                                         mtool->x[0] + arc_radius + target :
                                         mtool->x[0] - arc_radius - target),
                                        mtool->y[0],
                                        FALSE);
            }
        }
    }
}

static void
gimp_measure_tool_halt (GimpMeasureTool *mtool)
{
  GimpTool *tool = GIMP_TOOL (mtool);

  if (mtool->dialog)
    gtk_widget_destroy (mtool->dialog);

  gimp_tool_pop_status (tool, tool->display);

  if (gimp_draw_tool_is_active (GIMP_DRAW_TOOL (mtool)))
    gimp_draw_tool_stop (GIMP_DRAW_TOOL (mtool));

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
gimp_measure_tool_dialog_update (GimpMeasureTool *mtool,
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
  ax = mtool->x[1] - mtool->x[0];
  ay = mtool->y[1] - mtool->y[0];

  if (mtool->num_points == 3)
    {
      bx = mtool->x[2] - mtool->x[0];
      by = mtool->y[2] - mtool->y[0];
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

  if (mtool->num_points != 3)
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

  mtool->angle1 = theta1;
  mtool->angle2 = theta2;

  unit_angle = fabs (theta1 - theta2);
  if (unit_angle > 180.0)
    unit_angle = fabs (360.0 - unit_angle);

  if (shell->unit == GIMP_UNIT_PIXEL)
    {
      g_snprintf (buf, sizeof (buf), "%.1f %s, %.2f \302\260 (%d × %d)",
                  pixel_distance, _("pixels"), pixel_angle,
                  pixel_width, pixel_height);
    }
  else
    {
      g_snprintf (format, sizeof (format),
                  "%%.%df %s, %%.2f \302\260 (%%.%df × %%.%df)",
                  _gimp_unit_get_digits (image->gimp, shell->unit),
                  _gimp_unit_get_plural (image->gimp, shell->unit),
                  _gimp_unit_get_digits (image->gimp, shell->unit),
                  _gimp_unit_get_digits (image->gimp, shell->unit));

      g_snprintf (buf, sizeof (buf), format, unit_distance, unit_angle,
                  unit_width, unit_height);
    }

  gimp_tool_replace_status (GIMP_TOOL (mtool), display, buf);

  if (mtool->dialog)
    {
      g_snprintf (format, sizeof (format), "%%.%df",
                  _gimp_unit_get_digits (image->gimp, shell->unit));
      /* Distance */
      g_snprintf (buf, sizeof (buf), "%.1f", pixel_distance);
      gtk_label_set_text (GTK_LABEL (mtool->distance_label[0]), buf);

      if (shell->unit != GIMP_UNIT_PIXEL)
        {
          g_snprintf (buf, sizeof (buf), format, unit_distance);
          gtk_label_set_text (GTK_LABEL (mtool->distance_label[1]), buf);

          gtk_label_set_text (GTK_LABEL (mtool->unit_label[0]),
                              _gimp_unit_get_plural (image->gimp, shell->unit));
        }
      else
        {
          gtk_label_set_text (GTK_LABEL (mtool->distance_label[1]), " ");
          gtk_label_set_text (GTK_LABEL (mtool->unit_label[0]),  " ");
        }

      /* Angle */
      g_snprintf (buf, sizeof (buf), "%.2f", pixel_angle);
      gtk_label_set_text (GTK_LABEL (mtool->angle_label[0]), buf);

      if (fabs (unit_angle - pixel_angle) > 0.01)
        {
          g_snprintf (buf, sizeof (buf), "%.2f", unit_angle);
          gtk_label_set_text (GTK_LABEL (mtool->angle_label[1]), buf);

          gtk_label_set_text (GTK_LABEL (mtool->unit_label[1]), "\302\260");
        }
      else
        {
          gtk_label_set_text (GTK_LABEL (mtool->angle_label[1]), " ");
          gtk_label_set_text (GTK_LABEL (mtool->unit_label[1]),  " ");
        }

      /* Width */
      g_snprintf (buf, sizeof (buf), "%d", pixel_width);
      gtk_label_set_text (GTK_LABEL (mtool->width_label[0]), buf);

      if (shell->unit != GIMP_UNIT_PIXEL)
        {
          g_snprintf (buf, sizeof (buf), format, unit_width);
          gtk_label_set_text (GTK_LABEL (mtool->width_label[1]), buf);

          gtk_label_set_text (GTK_LABEL (mtool->unit_label[2]),
                              _gimp_unit_get_plural (image->gimp, shell->unit));
        }
      else
        {
          gtk_label_set_text (GTK_LABEL (mtool->width_label[1]), " ");
          gtk_label_set_text (GTK_LABEL (mtool->unit_label[2]), " ");
        }

      g_snprintf (buf, sizeof (buf), "%d", pixel_height);
      gtk_label_set_text (GTK_LABEL (mtool->height_label[0]), buf);

      /* Height */
      if (shell->unit != GIMP_UNIT_PIXEL)
        {
          g_snprintf (buf, sizeof (buf), format, unit_height);
          gtk_label_set_text (GTK_LABEL (mtool->height_label[1]), buf);

          gtk_label_set_text (GTK_LABEL (mtool->unit_label[3]),
                              _gimp_unit_get_plural (image->gimp, shell->unit));
        }
      else
        {
          gtk_label_set_text (GTK_LABEL (mtool->height_label[1]), " ");
          gtk_label_set_text (GTK_LABEL (mtool->unit_label[3]), " ");
        }

      if (GTK_WIDGET_VISIBLE (mtool->dialog))
        gdk_window_show (mtool->dialog->window);
      else
        gtk_widget_show (mtool->dialog);
    }
}

static GtkWidget *
gimp_measure_tool_dialog_new (GimpMeasureTool *mtool)
{
  GimpTool  *tool = GIMP_TOOL (mtool);
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

  mtool->distance_label[0] = label = gtk_label_new ("0.0");
  gtk_misc_set_alignment (GTK_MISC (label), 1.0, 0.5);
  gtk_table_attach_defaults (GTK_TABLE (table), label, 1, 2, 0, 1);
  gtk_widget_show (label);

  label = gtk_label_new (_("pixels"));
  gtk_misc_set_alignment (GTK_MISC (label), 0.0, 0.5);
  gtk_table_attach_defaults (GTK_TABLE (table), label, 2, 3, 0, 1);
  gtk_widget_show (label);

  mtool->distance_label[1] = label = gtk_label_new ("0.0");
  gtk_misc_set_alignment (GTK_MISC (label), 1.0, 0.5);
  gtk_table_attach_defaults (GTK_TABLE (table), label, 3, 4, 0, 1);
  gtk_widget_show (label);

  mtool->unit_label[0] = label = gtk_label_new (" ");
  gtk_misc_set_alignment (GTK_MISC (label), 0.0, 0.5);
  gtk_table_attach_defaults (GTK_TABLE (table), label, 4, 5, 0, 1);
  gtk_widget_show (label);


  label = gtk_label_new (_("Angle:"));
  gtk_misc_set_alignment (GTK_MISC (label), 0.0, 0.5);
  gtk_table_attach_defaults (GTK_TABLE (table), label, 0, 1, 1, 2);
  gtk_widget_show (label);

  mtool->angle_label[0] = label = gtk_label_new ("0.0");
  gtk_misc_set_alignment (GTK_MISC (label), 1.0, 0.5);
  gtk_table_attach_defaults (GTK_TABLE (table), label, 1, 2, 1, 2);
  gtk_widget_show (label);

  label = gtk_label_new ("\302\260");
  gtk_misc_set_alignment (GTK_MISC (label), 0.0, 0.5);
  gtk_table_attach_defaults (GTK_TABLE (table), label, 2, 3, 1, 2);
  gtk_widget_show (label);

  mtool->angle_label[1] = label = gtk_label_new (" ");
  gtk_misc_set_alignment (GTK_MISC (label), 1.0, 0.5);
  gtk_table_attach_defaults (GTK_TABLE (table), label, 3, 4, 1, 2);
  gtk_widget_show (label);

  mtool->unit_label[1] = label = gtk_label_new (" ");
  gtk_misc_set_alignment (GTK_MISC (label), 0.0, 0.5);
  gtk_table_attach_defaults (GTK_TABLE (table), label, 4, 5, 1, 2);
  gtk_widget_show (label);


  label = gtk_label_new (_("Width:"));
  gtk_misc_set_alignment (GTK_MISC (label), 0.0, 0.5);
  gtk_table_attach_defaults (GTK_TABLE (table), label, 0, 1, 2, 3);
  gtk_widget_show (label);

  mtool->width_label[0] = label = gtk_label_new ("0.0");
  gtk_misc_set_alignment (GTK_MISC (label), 1.0, 0.5);
  gtk_table_attach_defaults (GTK_TABLE (table), label, 1, 2, 2, 3);
  gtk_widget_show (label);

  label = gtk_label_new (_("pixels"));
  gtk_misc_set_alignment (GTK_MISC (label), 0.0, 0.5);
  gtk_table_attach_defaults (GTK_TABLE (table), label, 2, 3, 2, 3);
  gtk_widget_show (label);

  mtool->width_label[1] = label = gtk_label_new ("0.0");
  gtk_misc_set_alignment (GTK_MISC (label), 1.0, 0.5);
  gtk_table_attach_defaults (GTK_TABLE (table), label, 3, 4, 2, 3);
  gtk_widget_show (label);

  mtool->unit_label[2] = label = gtk_label_new (" ");
  gtk_misc_set_alignment (GTK_MISC (label), 0.0, 0.5);
  gtk_table_attach_defaults (GTK_TABLE (table), label, 4, 5, 2, 3);
  gtk_widget_show (label);


  label = gtk_label_new (_("Height:"));
  gtk_misc_set_alignment (GTK_MISC (label), 0.0, 0.5);
  gtk_table_attach_defaults (GTK_TABLE (table), label, 0, 1, 3, 4);
  gtk_widget_show (label);

  mtool->height_label[0] = label = gtk_label_new ("0.0");
  gtk_misc_set_alignment (GTK_MISC (label), 1.0, 0.5);
  gtk_table_attach_defaults (GTK_TABLE (table), label, 1, 2, 3, 4);
  gtk_widget_show (label);

  label = gtk_label_new (_("pixels"));
  gtk_misc_set_alignment (GTK_MISC (label), 0.0, 0.5);
  gtk_table_attach_defaults (GTK_TABLE (table), label, 2, 3, 3, 4);
  gtk_widget_show (label);

  mtool->height_label[1] = label = gtk_label_new ("0.0");
  gtk_misc_set_alignment (GTK_MISC (label), 1.0, 0.5);
  gtk_table_attach_defaults (GTK_TABLE (table), label, 3, 4, 3, 4);
  gtk_widget_show (label);

  mtool->unit_label[3] = label = gtk_label_new (" ");
  gtk_misc_set_alignment (GTK_MISC (label), 0.0, 0.5);
  gtk_table_attach_defaults (GTK_TABLE (table), label, 4, 5, 3, 4);
  gtk_widget_show (label);

  return dialog;
}
