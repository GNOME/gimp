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

#include <gegl.h>
#include <gtk/gtk.h>
#include <gdk/gdkkeysyms.h>

#include "libgimpbase/gimpbase.h"
#include "libgimpmath/gimpmath.h"
#include "libgimpwidgets/gimpwidgets.h"

#include "tools-types.h"

#include "core/gimpimage.h"
#include "core/gimpimage-guides.h"
#include "core/gimpimage-undo.h"
#include "core/gimpimage-undo-push.h"

#include "widgets/gimphelp-ids.h"

#include "display/gimpdisplay.h"
#include "display/gimpdisplayshell.h"
#include "display/gimpdisplayshell-appearance.h"
#include "display/gimptoolcompass.h"
#include "display/gimptoolgui.h"

#include "gimpmeasureoptions.h"
#include "gimpmeasuretool.h"
#include "gimptoolcontrol.h"

#include "gimp-intl.h"


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

static void     gimp_measure_tool_compass_changed (GimpToolWidget        *widget,
                                                   GimpMeasureTool       *measure);
static void     gimp_measure_tool_compass_response(GimpToolWidget        *widget,
                                                   gint                   response_id,
                                                   GimpMeasureTool       *measure);
static void     gimp_measure_tool_compass_status  (GimpToolWidget        *widget,
                                                   const gchar           *status,
                                                   GimpMeasureTool       *measure);
static void     gimp_measure_tool_compass_create_guides
                                                  (GimpToolWidget        *widget,
                                                   gint                   x,
                                                   gint                   y,
                                                   gboolean               horizontal,
                                                   gboolean               vertical,
                                                   GimpMeasureTool       *measure);

static void     gimp_measure_tool_start           (GimpMeasureTool       *measure,
                                                   GimpDisplay           *display,
                                                   const GimpCoords      *coords);
static void     gimp_measure_tool_halt            (GimpMeasureTool       *measure);

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
                GIMP_ICON_TOOL_MEASURE,
                data);
}

static void
gimp_measure_tool_class_init (GimpMeasureToolClass *klass)
{
  GimpToolClass *tool_class = GIMP_TOOL_CLASS (klass);

  tool_class->control        = gimp_measure_tool_control;
  tool_class->button_press   = gimp_measure_tool_button_press;
  tool_class->button_release = gimp_measure_tool_button_release;
  tool_class->motion         = gimp_measure_tool_motion;
}

static void
gimp_measure_tool_init (GimpMeasureTool *measure)
{
  GimpTool *tool = GIMP_TOOL (measure);

  gimp_tool_control_set_handle_empty_image (tool->control, TRUE);
  gimp_tool_control_set_active_modifiers   (tool->control,
                                            GIMP_TOOL_ACTIVE_MODIFIERS_SEPARATE);
  gimp_tool_control_set_precision          (tool->control,
                                            GIMP_CURSOR_PRECISION_PIXEL_BORDER);
  gimp_tool_control_set_cursor             (tool->control,
                                            GIMP_CURSOR_CROSSHAIR_SMALL);
  gimp_tool_control_set_tool_cursor        (tool->control,
                                            GIMP_TOOL_CURSOR_MEASURE);

  gimp_draw_tool_set_default_status (GIMP_DRAW_TOOL (tool),
                                     _("Click-Drag to create a line"));
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
      gimp_measure_tool_halt (measure);
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

  if (tool->display && display != tool->display)
    gimp_tool_control (tool, GIMP_TOOL_ACTION_HALT, tool->display);

  if (! measure->widget)
    {
      gimp_measure_tool_start (measure, display, coords);

      gimp_tool_widget_hover (measure->widget, coords, state, TRUE);
    }

  if (gimp_tool_widget_button_press (measure->widget, coords, time, state,
                                     press_type))
    {
      measure->grab_widget = measure->widget;
    }

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

  gimp_tool_control_activate (tool->control);
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

  gimp_tool_control_halt (tool->control);

  if (measure->grab_widget)
    {
      gimp_tool_widget_button_release (measure->grab_widget,
                                       coords, time, state, release_type);
      measure->grab_widget = NULL;
    }
}

static void
gimp_measure_tool_motion (GimpTool         *tool,
                          const GimpCoords *coords,
                          guint32           time,
                          GdkModifierType   state,
                          GimpDisplay      *display)
{
  GimpMeasureTool *measure = GIMP_MEASURE_TOOL (tool);

  if (measure->grab_widget)
    {
      gimp_tool_widget_motion (measure->grab_widget, coords, time, state);
    }
}

static void
gimp_measure_tool_compass_changed (GimpToolWidget  *widget,
                                   GimpMeasureTool *measure)
{
  g_object_get (widget,
                "n-points", &measure->n_points,
                "x1",       &measure->x[0],
                "y1",       &measure->y[0],
                "x2",       &measure->x[1],
                "y2",       &measure->y[1],
                "x3",       &measure->x[2],
                "y3",       &measure->y[2],
                NULL);

  gimp_measure_tool_dialog_update (measure, GIMP_TOOL (measure)->display);
}

static void
gimp_measure_tool_compass_response (GimpToolWidget  *widget,
                                    gint             response_id,
                                    GimpMeasureTool *measure)
{
  GimpTool *tool = GIMP_TOOL (measure);

  if (response_id == GIMP_TOOL_WIDGET_RESPONSE_CANCEL)
    gimp_tool_control (tool, GIMP_TOOL_ACTION_HALT, tool->display);
}

static void
gimp_measure_tool_compass_status (GimpToolWidget  *widget,
                                  const gchar     *status,
                                  GimpMeasureTool *measure)
{
  GimpTool *tool = GIMP_TOOL (measure);

  if (! status)
    {
      /* replace status bar hint by distance and angle */
      gimp_measure_tool_dialog_update (measure, tool->display);
    }
}

static void
gimp_measure_tool_compass_create_guides (GimpToolWidget  *widget,
                                         gint             x,
                                         gint             y,
                                         gboolean         horizontal,
                                         gboolean         vertical,
                                         GimpMeasureTool *measure)
{
  GimpDisplay *display = GIMP_TOOL (measure)->display;
  GimpImage   *image   = gimp_display_get_image (display);

  if (x < 0 || x > gimp_image_get_width (image))
    vertical = FALSE;

  if (y < 0 || y > gimp_image_get_height (image))
    horizontal = FALSE;

  if (horizontal || vertical)
    {
      if (horizontal && vertical)
        gimp_image_undo_group_start (image,
                                     GIMP_UNDO_GROUP_GUIDE,
                                     _("Add Guides"));

      if (horizontal)
        gimp_image_add_hguide (image, y, TRUE);

      if (vertical)
        gimp_image_add_vguide (image, x, TRUE);

      if (horizontal && vertical)
        gimp_image_undo_group_end (image);

      gimp_image_flush (image);
    }
}

static void
gimp_measure_tool_start (GimpMeasureTool  *measure,
                         GimpDisplay      *display,
                         const GimpCoords *coords)
{
  GimpTool         *tool  = GIMP_TOOL (measure);
  GimpDisplayShell *shell = gimp_display_get_shell (display);

  measure->n_points = 1;
  measure->x[0]     = coords->x;
  measure->y[0]     = coords->y;
  measure->x[1]     = 0;
  measure->y[1]     = 0;
  measure->x[2]     = 0;
  measure->y[2]     = 0;

  measure->widget = gimp_tool_compass_new (shell,
                                           measure->n_points,
                                           measure->x[0],
                                           measure->y[0],
                                           measure->x[1],
                                           measure->y[1],
                                           measure->x[2],
                                           measure->y[2]);

  gimp_draw_tool_set_widget (GIMP_DRAW_TOOL (tool), measure->widget);

  g_signal_connect (measure->widget, "changed",
                    G_CALLBACK (gimp_measure_tool_compass_changed),
                    measure);
  g_signal_connect (measure->widget, "response",
                    G_CALLBACK (gimp_measure_tool_compass_response),
                    measure);
  g_signal_connect (measure->widget, "status",
                    G_CALLBACK (gimp_measure_tool_compass_status),
                    measure);
  g_signal_connect (measure->widget, "create-guides",
                    G_CALLBACK (gimp_measure_tool_compass_create_guides),
                    measure);

  tool->display = display;

  gimp_draw_tool_start (GIMP_DRAW_TOOL (measure), display);
}

static void
gimp_measure_tool_halt (GimpMeasureTool *measure)
{
  GimpTool *tool = GIMP_TOOL (measure);

  if (tool->display)
    gimp_tool_pop_status (tool, tool->display);

  if (gimp_draw_tool_is_active (GIMP_DRAW_TOOL (measure)))
    gimp_draw_tool_stop (GIMP_DRAW_TOOL (measure));

  gimp_draw_tool_set_widget (GIMP_DRAW_TOOL (tool), NULL);
  g_clear_object (&measure->widget);

  g_clear_object (&measure->gui);

  tool->display = NULL;
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
  gdouble           inch_distance;
  gdouble           theta1, theta2;
  gdouble           pixel_angle;
  gdouble           unit_angle;
  gdouble           xres;
  gdouble           yres;
  gchar             format[128];
  gint              unit_distance_digits = 0;
  gint              unit_width_digits;
  gint              unit_height_digits;

  /*  calculate distance and angle  */
  ax = measure->x[1] - measure->x[0];
  ay = measure->y[1] - measure->y[0];

  if (measure->n_points == 3)
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
  inch_distance = sqrt (SQR ((gdouble) (ax - bx) / xres) +
                        SQR ((gdouble) (ay - by) / yres));
  unit_distance  = gimp_unit_get_factor (shell->unit) * inch_distance;

  if (measure->n_points != 3)
    bx = ax > 0 ? 1 : -1;

  theta1 = gimp_measure_tool_get_angle (ax, ay, 1.0, 1.0);
  theta2 = gimp_measure_tool_get_angle (bx, by, 1.0, 1.0);

  pixel_angle = fabs (theta1 - theta2);
  if (pixel_angle > 180.0)
    pixel_angle = fabs (360.0 - pixel_angle);

  theta1 = gimp_measure_tool_get_angle (ax, ay, xres, yres);
  theta2 = gimp_measure_tool_get_angle (bx, by, xres, yres);

  unit_angle = fabs (theta1 - theta2);
  if (unit_angle > 180.0)
    unit_angle = fabs (360.0 - unit_angle);

  /* Compute minimum digits to display accurate values, so that
   * every pixel shows a different value in unit.
   */
  if (inch_distance)
    unit_distance_digits = gimp_unit_get_scaled_digits (shell->unit,
                                                        pixel_distance /
                                                        inch_distance);
  unit_width_digits  = gimp_unit_get_scaled_digits (shell->unit, xres);
  unit_height_digits = gimp_unit_get_scaled_digits (shell->unit, yres);

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
                  unit_distance_digits,
                  gimp_unit_get_plural (shell->unit),
                  unit_width_digits,
                  unit_height_digits);

      gimp_tool_replace_status (GIMP_TOOL (measure), display, format,
                                unit_distance, unit_angle,
                                unit_width, unit_height);
    }

  if (measure->gui)
    {
      gchar buf[128];

      /* Distance */
      g_snprintf (buf, sizeof (buf), "%.1f", pixel_distance);
      gtk_label_set_text (GTK_LABEL (measure->distance_label[0]), buf);

      if (shell->unit != GIMP_UNIT_PIXEL)
        {
          g_snprintf (format, sizeof (format), "%%.%df",
                      unit_distance_digits);
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
          g_snprintf (format, sizeof (format), "%%.%df",
                      unit_width_digits);
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
          g_snprintf (format, sizeof (format), "%%.%df",
                      unit_height_digits);
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
                           gimp_widget_get_monitor (GTK_WIDGET (shell)),
                           TRUE,

                           _("_Close"), GTK_RESPONSE_CLOSE,

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
