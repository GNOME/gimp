/* LIGMA - The GNU Image Manipulation Program
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * Measure tool
 * Copyright (C) 1999-2003 Sven Neumann <sven@ligma.org>
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

#include <gegl.h>
#include <gtk/gtk.h>
#include <gdk/gdkkeysyms.h>

#include "libligmabase/ligmabase.h"
#include "libligmamath/ligmamath.h"
#include "libligmawidgets/ligmawidgets.h"

#include "tools-types.h"

#include "core/ligmaimage.h"
#include "core/ligmaimage-guides.h"
#include "core/ligmaimage-undo.h"
#include "core/ligmaimage-undo-push.h"
#include "core/ligmaprogress.h"
#include "core/ligma-transform-utils.h"

#include "widgets/ligmahelp-ids.h"
#include "widgets/ligmawidgets-utils.h"

#include "display/ligmadisplay.h"
#include "display/ligmadisplayshell.h"
#include "display/ligmadisplayshell-appearance.h"
#include "display/ligmatoolcompass.h"
#include "display/ligmatoolgui.h"

#include "ligmameasureoptions.h"
#include "ligmameasuretool.h"
#include "ligmatoolcontrol.h"

#include "ligma-intl.h"


/*  local function prototypes  */

static void     ligma_measure_tool_control         (LigmaTool              *tool,
                                                   LigmaToolAction         action,
                                                   LigmaDisplay           *display);
static void     ligma_measure_tool_modifier_key    (LigmaTool              *tool,
                                                   GdkModifierType        key,
                                                   gboolean               press,
                                                   GdkModifierType        state,
                                                   LigmaDisplay           *display);
static void     ligma_measure_tool_button_press    (LigmaTool              *tool,
                                                   const LigmaCoords      *coords,
                                                   guint32                time,
                                                   GdkModifierType        state,
                                                   LigmaButtonPressType    press_type,
                                                   LigmaDisplay           *display);
static void     ligma_measure_tool_button_release  (LigmaTool              *tool,
                                                   const LigmaCoords      *coords,
                                                   guint32                time,
                                                   GdkModifierType        state,
                                                   LigmaButtonReleaseType  release_type,
                                                   LigmaDisplay           *display);
static void     ligma_measure_tool_motion          (LigmaTool              *tool,
                                                   const LigmaCoords      *coords,
                                                   guint32                time,
                                                   GdkModifierType        state,
                                                   LigmaDisplay           *display);

static void     ligma_measure_tool_recalc_matrix   (LigmaTransformTool     *tr_tool);
static gchar  * ligma_measure_tool_get_undo_desc   (LigmaTransformTool     *tr_tool);

static void     ligma_measure_tool_compass_changed (LigmaToolWidget        *widget,
                                                   LigmaMeasureTool       *measure);
static void     ligma_measure_tool_compass_response(LigmaToolWidget        *widget,
                                                   gint                   response_id,
                                                   LigmaMeasureTool       *measure);
static void     ligma_measure_tool_compass_status  (LigmaToolWidget        *widget,
                                                   const gchar           *status,
                                                   LigmaMeasureTool       *measure);
static void     ligma_measure_tool_compass_create_guides
                                                  (LigmaToolWidget        *widget,
                                                   gint                   x,
                                                   gint                   y,
                                                   gboolean               horizontal,
                                                   gboolean               vertical,
                                                   LigmaMeasureTool       *measure);

static void     ligma_measure_tool_start           (LigmaMeasureTool       *measure,
                                                   LigmaDisplay           *display,
                                                   const LigmaCoords      *coords);
static void     ligma_measure_tool_halt            (LigmaMeasureTool       *measure);

static LigmaToolGui * ligma_measure_tool_dialog_new (LigmaMeasureTool       *measure);
static void     ligma_measure_tool_dialog_update   (LigmaMeasureTool       *measure,
                                                   LigmaDisplay           *display);

static void     ligma_measure_tool_straighten_button_clicked
                                                  (GtkWidget             *button,
                                                   LigmaMeasureTool       *measure);

G_DEFINE_TYPE (LigmaMeasureTool, ligma_measure_tool, LIGMA_TYPE_TRANSFORM_TOOL)

#define parent_class ligma_measure_tool_parent_class


void
ligma_measure_tool_register (LigmaToolRegisterCallback  callback,
                            gpointer                  data)
{
  (* callback) (LIGMA_TYPE_MEASURE_TOOL,
                LIGMA_TYPE_MEASURE_OPTIONS,
                ligma_measure_options_gui,
                0,
                "ligma-measure-tool",
                _("Measure"),
                _("Measure Tool: Measure distances and angles"),
                N_("_Measure"), "<shift>M",
                NULL, LIGMA_HELP_TOOL_MEASURE,
                LIGMA_ICON_TOOL_MEASURE,
                data);
}

static void
ligma_measure_tool_class_init (LigmaMeasureToolClass *klass)
{
  LigmaToolClass          *tool_class = LIGMA_TOOL_CLASS (klass);
  LigmaTransformToolClass *tr_class   = LIGMA_TRANSFORM_TOOL_CLASS (klass);

  tool_class->control        = ligma_measure_tool_control;
  tool_class->modifier_key   = ligma_measure_tool_modifier_key;
  tool_class->button_press   = ligma_measure_tool_button_press;
  tool_class->button_release = ligma_measure_tool_button_release;
  tool_class->motion         = ligma_measure_tool_motion;

  tr_class->recalc_matrix    = ligma_measure_tool_recalc_matrix;
  tr_class->get_undo_desc    = ligma_measure_tool_get_undo_desc;

  tr_class->undo_desc        = C_("undo-type", "Straighten");
  tr_class->progress_text    = _("Straightening");
}

static void
ligma_measure_tool_init (LigmaMeasureTool *measure)
{
  LigmaTool *tool = LIGMA_TOOL (measure);

  ligma_tool_control_set_handle_empty_image (tool->control, TRUE);
  ligma_tool_control_set_active_modifiers   (tool->control,
                                            LIGMA_TOOL_ACTIVE_MODIFIERS_SEPARATE);
  ligma_tool_control_set_precision          (tool->control,
                                            LIGMA_CURSOR_PRECISION_PIXEL_BORDER);
  ligma_tool_control_set_cursor             (tool->control,
                                            LIGMA_CURSOR_CROSSHAIR_SMALL);
  ligma_tool_control_set_tool_cursor        (tool->control,
                                            LIGMA_TOOL_CURSOR_MEASURE);

  ligma_draw_tool_set_default_status (LIGMA_DRAW_TOOL (tool),
                                     _("Click-Drag to create a line"));
}

static void
ligma_measure_tool_control (LigmaTool       *tool,
                           LigmaToolAction  action,
                           LigmaDisplay    *display)
{
  LigmaMeasureTool *measure = LIGMA_MEASURE_TOOL (tool);

  switch (action)
    {
    case LIGMA_TOOL_ACTION_PAUSE:
    case LIGMA_TOOL_ACTION_RESUME:
      break;

    case LIGMA_TOOL_ACTION_HALT:
      ligma_measure_tool_halt (measure);
      break;

    case LIGMA_TOOL_ACTION_COMMIT:
      break;
    }

  LIGMA_TOOL_CLASS (parent_class)->control (tool, action, display);
}

static void
ligma_measure_tool_modifier_key (LigmaTool        *tool,
                                GdkModifierType  key,
                                gboolean         press,
                                GdkModifierType  state,
                                LigmaDisplay     *display)
{
  LigmaMeasureOptions *options = LIGMA_MEASURE_TOOL_GET_OPTIONS (tool);

  if (key == ligma_get_toggle_behavior_mask ())
    {
      switch (options->orientation)
        {
        case LIGMA_COMPASS_ORIENTATION_HORIZONTAL:
          g_object_set (options,
                        "orientation", LIGMA_COMPASS_ORIENTATION_VERTICAL,
                        NULL);
          break;

        case LIGMA_COMPASS_ORIENTATION_VERTICAL:
          g_object_set (options,
                        "orientation", LIGMA_COMPASS_ORIENTATION_HORIZONTAL,
                        NULL);
          break;

        default:
          break;
        }
    }
}

static void
ligma_measure_tool_button_press (LigmaTool            *tool,
                                const LigmaCoords    *coords,
                                guint32              time,
                                GdkModifierType      state,
                                LigmaButtonPressType  press_type,
                                LigmaDisplay         *display)
{
  LigmaMeasureTool    *measure = LIGMA_MEASURE_TOOL (tool);
  LigmaMeasureOptions *options = LIGMA_MEASURE_TOOL_GET_OPTIONS (tool);
  LigmaDisplayShell   *shell   = ligma_display_get_shell (display);
  LigmaImage          *image   = ligma_display_get_image (display);

  if (tool->display && display != tool->display)
    ligma_tool_control (tool, LIGMA_TOOL_ACTION_HALT, tool->display);

  if (! measure->widget)
    {
      measure->supress_guides = TRUE;

      ligma_measure_tool_start (measure, display, coords);

      ligma_tool_widget_hover (measure->widget, coords, state, TRUE);
    }

  if (ligma_tool_widget_button_press (measure->widget, coords, time, state,
                                     press_type))
    {
      measure->grab_widget = measure->widget;
    }

  /*  create the info window if necessary  */
  if (! measure->gui)
    {
      if (options->use_info_window ||
          ! ligma_display_shell_get_show_statusbar (shell))
        {
          measure->gui = ligma_measure_tool_dialog_new (measure);
          g_object_add_weak_pointer (G_OBJECT (measure->gui),
                                     (gpointer) &measure->gui);
        }
    }

  if (measure->gui)
    {
      ligma_tool_gui_set_shell (measure->gui, shell);
      ligma_tool_gui_set_viewable (measure->gui, LIGMA_VIEWABLE (image));

      ligma_measure_tool_dialog_update (measure, display);
    }

  ligma_tool_control_activate (tool->control);
}

static void
ligma_measure_tool_button_release (LigmaTool              *tool,
                                  const LigmaCoords      *coords,
                                  guint32                time,
                                  GdkModifierType        state,
                                  LigmaButtonReleaseType  release_type,
                                  LigmaDisplay           *display)
{
  LigmaMeasureTool *measure = LIGMA_MEASURE_TOOL (tool);

  ligma_tool_control_halt (tool->control);

  if (measure->grab_widget)
    {
      ligma_tool_widget_button_release (measure->grab_widget,
                                       coords, time, state, release_type);
      measure->grab_widget = NULL;
    }

  measure->supress_guides = FALSE;
}

static void
ligma_measure_tool_motion (LigmaTool         *tool,
                          const LigmaCoords *coords,
                          guint32           time,
                          GdkModifierType   state,
                          LigmaDisplay      *display)
{
  LigmaMeasureTool *measure = LIGMA_MEASURE_TOOL (tool);

  if (measure->grab_widget)
    {
      ligma_tool_widget_motion (measure->grab_widget, coords, time, state);
    }
}

static void
ligma_measure_tool_recalc_matrix (LigmaTransformTool *tr_tool)
{
  LigmaMeasureTool *measure = LIGMA_MEASURE_TOOL (tr_tool);
  gdouble          angle;

  if (measure->n_points < 2)
    {
      tr_tool->transform_valid = FALSE;

      return;
    }

  g_object_get (measure->widget,
                "pixel-angle", &angle,
                NULL);

  ligma_matrix3_identity (&tr_tool->transform);
  ligma_transform_matrix_rotate_center (&tr_tool->transform,
                                       measure->x[0], measure->y[0],
                                       angle);

  tr_tool->transform_valid = TRUE;
}

static gchar *
ligma_measure_tool_get_undo_desc (LigmaTransformTool *tr_tool)
{
  LigmaMeasureTool        *measure = LIGMA_MEASURE_TOOL (tr_tool);
  LigmaCompassOrientation  orientation;
  gdouble                 angle;

  g_object_get (measure->widget,
                "effective-orientation", &orientation,
                "pixel-angle",           &angle,
                NULL);

  angle = ligma_rad_to_deg (fabs (angle));

  switch (orientation)
    {
    case LIGMA_COMPASS_ORIENTATION_AUTO:
      return g_strdup_printf (C_("undo-type",
                                 "Straighten by %-3.3g°"),
                              angle);

    case LIGMA_COMPASS_ORIENTATION_HORIZONTAL:
      return g_strdup_printf (C_("undo-type",
                                 "Straighten Horizontally by %-3.3g°"),
                              angle);

    case LIGMA_COMPASS_ORIENTATION_VERTICAL:
      return g_strdup_printf (C_("undo-type",
                                 "Straighten Vertically by %-3.3g°"),
                              angle);
    }

  g_return_val_if_reached (NULL);
}

static void
ligma_measure_tool_compass_changed (LigmaToolWidget  *widget,
                                   LigmaMeasureTool *measure)
{
  LigmaMeasureOptions *options = LIGMA_MEASURE_TOOL_GET_OPTIONS (measure);

  g_object_get (widget,
                "n-points", &measure->n_points,
                "x1",       &measure->x[0],
                "y1",       &measure->y[0],
                "x2",       &measure->x[1],
                "y2",       &measure->y[1],
                "x3",       &measure->x[2],
                "y3",       &measure->y[2],
                NULL);

  gtk_widget_set_sensitive (options->straighten_button, measure->n_points >= 2);
  ligma_measure_tool_dialog_update (measure, LIGMA_TOOL (measure)->display);
}

static void
ligma_measure_tool_compass_response (LigmaToolWidget  *widget,
                                    gint             response_id,
                                    LigmaMeasureTool *measure)
{
  LigmaTool *tool = LIGMA_TOOL (measure);

  if (response_id == LIGMA_TOOL_WIDGET_RESPONSE_CANCEL)
    ligma_tool_control (tool, LIGMA_TOOL_ACTION_HALT, tool->display);
}

static void
ligma_measure_tool_compass_status (LigmaToolWidget  *widget,
                                  const gchar     *status,
                                  LigmaMeasureTool *measure)
{
  LigmaTool *tool = LIGMA_TOOL (measure);

  if (! status)
    {
      /* replace status bar hint by distance and angle */
      ligma_measure_tool_dialog_update (measure, tool->display);
    }
}

static void
ligma_measure_tool_compass_create_guides (LigmaToolWidget  *widget,
                                         gint             x,
                                         gint             y,
                                         gboolean         horizontal,
                                         gboolean         vertical,
                                         LigmaMeasureTool *measure)
{
  LigmaDisplay *display = LIGMA_TOOL (measure)->display;
  LigmaImage   *image   = ligma_display_get_image (display);

  if (measure->supress_guides)
    return;

  if (x < 0 || x > ligma_image_get_width (image))
    vertical = FALSE;

  if (y < 0 || y > ligma_image_get_height (image))
    horizontal = FALSE;

  if (horizontal || vertical)
    {
      if (horizontal && vertical)
        ligma_image_undo_group_start (image,
                                     LIGMA_UNDO_GROUP_GUIDE,
                                     _("Add Guides"));

      if (horizontal)
        ligma_image_add_hguide (image, y, TRUE);

      if (vertical)
        ligma_image_add_vguide (image, x, TRUE);

      if (horizontal && vertical)
        ligma_image_undo_group_end (image);

      ligma_image_flush (image);
    }
}

static void
ligma_measure_tool_start (LigmaMeasureTool  *measure,
                         LigmaDisplay      *display,
                         const LigmaCoords *coords)
{
  LigmaTool           *tool    = LIGMA_TOOL (measure);
  LigmaDisplayShell   *shell   = ligma_display_get_shell (display);
  LigmaMeasureOptions *options = LIGMA_MEASURE_TOOL_GET_OPTIONS (tool);

  measure->n_points = 1;
  measure->x[0]     = coords->x;
  measure->y[0]     = coords->y;
  measure->x[1]     = 0;
  measure->y[1]     = 0;
  measure->x[2]     = 0;
  measure->y[2]     = 0;

  measure->widget = ligma_tool_compass_new (shell,
                                           options->orientation,
                                           measure->n_points,
                                           measure->x[0],
                                           measure->y[0],
                                           measure->x[1],
                                           measure->y[1],
                                           measure->x[2],
                                           measure->y[2]);

  ligma_draw_tool_set_widget (LIGMA_DRAW_TOOL (tool), measure->widget);

  g_object_bind_property (options,         "orientation",
                          measure->widget, "orientation",
                          G_BINDING_DEFAULT);

  g_signal_connect (measure->widget, "changed",
                    G_CALLBACK (ligma_measure_tool_compass_changed),
                    measure);
  g_signal_connect (measure->widget, "response",
                    G_CALLBACK (ligma_measure_tool_compass_response),
                    measure);
  g_signal_connect (measure->widget, "status",
                    G_CALLBACK (ligma_measure_tool_compass_status),
                    measure);
  g_signal_connect (measure->widget, "create-guides",
                    G_CALLBACK (ligma_measure_tool_compass_create_guides),
                    measure);
  g_signal_connect (options->straighten_button, "clicked",
                    G_CALLBACK (ligma_measure_tool_straighten_button_clicked),
                    measure);

  tool->display = display;

  ligma_draw_tool_start (LIGMA_DRAW_TOOL (measure), display);
}

static void
ligma_measure_tool_halt (LigmaMeasureTool *measure)
{
  LigmaMeasureOptions *options = LIGMA_MEASURE_TOOL_GET_OPTIONS (measure);
  LigmaTool           *tool    = LIGMA_TOOL (measure);

  if (options->straighten_button)
    {
      gtk_widget_set_sensitive (options->straighten_button, FALSE);

      g_signal_handlers_disconnect_by_func (
        options->straighten_button,
        G_CALLBACK (ligma_measure_tool_straighten_button_clicked),
        measure);
    }

  if (tool->display)
    ligma_tool_pop_status (tool, tool->display);

  if (ligma_draw_tool_is_active (LIGMA_DRAW_TOOL (measure)))
    ligma_draw_tool_stop (LIGMA_DRAW_TOOL (measure));

  ligma_draw_tool_set_widget (LIGMA_DRAW_TOOL (tool), NULL);
  g_clear_object (&measure->widget);

  g_clear_object (&measure->gui);

  tool->display = NULL;
}

static void
ligma_measure_tool_dialog_update (LigmaMeasureTool *measure,
                                 LigmaDisplay     *display)
{
  LigmaDisplayShell *shell = ligma_display_get_shell (display);
  LigmaImage        *image = ligma_display_get_image (display);
  gint              ax, ay;
  gint              bx, by;
  gint              pixel_width;
  gint              pixel_height;
  gdouble           unit_width;
  gdouble           unit_height;
  gdouble           pixel_distance;
  gdouble           unit_distance;
  gdouble           inch_distance;
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

  ligma_image_get_resolution (image, &xres, &yres);

  unit_width  = ligma_pixels_to_units (pixel_width,  shell->unit, xres);
  unit_height = ligma_pixels_to_units (pixel_height, shell->unit, yres);

  pixel_distance = sqrt (SQR (ax - bx) + SQR (ay - by));
  inch_distance = sqrt (SQR ((gdouble) (ax - bx) / xres) +
                        SQR ((gdouble) (ay - by) / yres));
  unit_distance  = ligma_unit_get_factor (shell->unit) * inch_distance;

  g_object_get (measure->widget,
                "pixel-angle", &pixel_angle,
                "unit-angle",  &unit_angle,
                NULL);

  pixel_angle = fabs (pixel_angle * 180.0 / G_PI);
  unit_angle  = fabs (unit_angle  * 180.0 / G_PI);

  /* Compute minimum digits to display accurate values, so that
   * every pixel shows a different value in unit.
   */
  if (inch_distance)
    unit_distance_digits = ligma_unit_get_scaled_digits (shell->unit,
                                                        pixel_distance /
                                                        inch_distance);
  unit_width_digits  = ligma_unit_get_scaled_digits (shell->unit, xres);
  unit_height_digits = ligma_unit_get_scaled_digits (shell->unit, yres);

  if (shell->unit == LIGMA_UNIT_PIXEL)
    {
      ligma_tool_replace_status (LIGMA_TOOL (measure), display,
                                "%.1f %s, %.2f\302\260 (%d × %d)",
                                pixel_distance, _("pixels"), pixel_angle,
                                pixel_width, pixel_height);
    }
  else
    {
      g_snprintf (format, sizeof (format),
                  "%%.%df %s, %%.2f\302\260 (%%.%df × %%.%df)",
                  unit_distance_digits,
                  ligma_unit_get_plural (shell->unit),
                  unit_width_digits,
                  unit_height_digits);

      ligma_tool_replace_status (LIGMA_TOOL (measure), display, format,
                                unit_distance, unit_angle,
                                unit_width, unit_height);
    }

  if (measure->gui)
    {
      gchar buf[128];

      /* Distance */
      g_snprintf (buf, sizeof (buf), "%.1f", pixel_distance);
      gtk_label_set_text (GTK_LABEL (measure->distance_label[0]), buf);

      if (shell->unit != LIGMA_UNIT_PIXEL)
        {
          g_snprintf (format, sizeof (format), "%%.%df",
                      unit_distance_digits);
          g_snprintf (buf, sizeof (buf), format, unit_distance);
          gtk_label_set_text (GTK_LABEL (measure->distance_label[1]), buf);

          gtk_label_set_text (GTK_LABEL (measure->unit_label[0]),
                              ligma_unit_get_plural (shell->unit));
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

      if (shell->unit != LIGMA_UNIT_PIXEL)
        {
          g_snprintf (format, sizeof (format), "%%.%df",
                      unit_width_digits);
          g_snprintf (buf, sizeof (buf), format, unit_width);
          gtk_label_set_text (GTK_LABEL (measure->width_label[1]), buf);

          gtk_label_set_text (GTK_LABEL (measure->unit_label[2]),
                              ligma_unit_get_plural (shell->unit));
        }
      else
        {
          gtk_label_set_text (GTK_LABEL (measure->width_label[1]), NULL);
          gtk_label_set_text (GTK_LABEL (measure->unit_label[2]), NULL);
        }

      g_snprintf (buf, sizeof (buf), "%d", pixel_height);
      gtk_label_set_text (GTK_LABEL (measure->height_label[0]), buf);

      /* Height */
      if (shell->unit != LIGMA_UNIT_PIXEL)
        {
          g_snprintf (format, sizeof (format), "%%.%df",
                      unit_height_digits);
          g_snprintf (buf, sizeof (buf), format, unit_height);
          gtk_label_set_text (GTK_LABEL (measure->height_label[1]), buf);

          gtk_label_set_text (GTK_LABEL (measure->unit_label[3]),
                              ligma_unit_get_plural (shell->unit));
        }
      else
        {
          gtk_label_set_text (GTK_LABEL (measure->height_label[1]), NULL);
          gtk_label_set_text (GTK_LABEL (measure->unit_label[3]), NULL);
        }

      ligma_tool_gui_show (measure->gui);
    }
}

static LigmaToolGui *
ligma_measure_tool_dialog_new (LigmaMeasureTool *measure)
{
  LigmaTool         *tool = LIGMA_TOOL (measure);
  LigmaDisplayShell *shell;
  LigmaToolGui      *gui;
  GtkWidget        *grid;
  GtkWidget        *label;

  g_return_val_if_fail (tool->display != NULL, NULL);

  shell = ligma_display_get_shell (tool->display);

  gui = ligma_tool_gui_new (tool->tool_info,
                           NULL,
                           _("Measure Distances and Angles"),
                           NULL, NULL,
                           ligma_widget_get_monitor (GTK_WIDGET (shell)),
                           TRUE,

                           _("_Close"), GTK_RESPONSE_CLOSE,

                           NULL);

  ligma_tool_gui_set_auto_overlay (gui, TRUE);
  ligma_tool_gui_set_focus_on_map (gui, FALSE);

  g_signal_connect (gui, "response",
                    G_CALLBACK (g_object_unref),
                    NULL);

  grid = gtk_grid_new ();
  gtk_grid_set_column_spacing (GTK_GRID (grid), 6);
  gtk_grid_set_row_spacing (GTK_GRID (grid), 6);
  gtk_box_pack_start (GTK_BOX (ligma_tool_gui_get_vbox (gui)), grid,
                      FALSE, FALSE, 0);
  gtk_widget_show (grid);


  label = gtk_label_new (_("Distance:"));
  gtk_label_set_xalign (GTK_LABEL (label), 0.0);
  gtk_grid_attach (GTK_GRID (grid), label, 0, 0, 1, 1);
  gtk_widget_show (label);

  measure->distance_label[0] = label = gtk_label_new ("0.0");
  gtk_label_set_selectable (GTK_LABEL (label), TRUE);
  gtk_label_set_xalign (GTK_LABEL (label), 1.0);
  gtk_grid_attach (GTK_GRID (grid), label, 1, 0, 1, 1);
  gtk_widget_show (label);

  label = gtk_label_new (_("pixels"));
  gtk_label_set_xalign (GTK_LABEL (label), 0.0);
  gtk_grid_attach (GTK_GRID (grid), label, 2, 0, 1, 1);
  gtk_widget_show (label);

  measure->distance_label[1] = label = gtk_label_new ("0.0");
  gtk_label_set_selectable (GTK_LABEL (label), TRUE);
  gtk_label_set_xalign (GTK_LABEL (label), 1.0);
  gtk_grid_attach (GTK_GRID (grid), label, 3, 0, 1, 1);
  gtk_widget_show (label);

  measure->unit_label[0] = label = gtk_label_new (NULL);
  gtk_label_set_xalign (GTK_LABEL (label), 0.0);
  gtk_grid_attach (GTK_GRID (grid), label, 4, 0, 1, 1);
  gtk_widget_show (label);


  label = gtk_label_new (_("Angle:"));
  gtk_label_set_xalign (GTK_LABEL (label), 0.0);
  gtk_grid_attach (GTK_GRID (grid), label, 0, 1, 1, 1);
  gtk_widget_show (label);

  measure->angle_label[0] = label = gtk_label_new ("0.0");
  gtk_label_set_selectable (GTK_LABEL (label), TRUE);
  gtk_label_set_xalign (GTK_LABEL (label), 1.0);
  gtk_grid_attach (GTK_GRID (grid), label, 1, 1, 1, 1);
  gtk_widget_show (label);

  label = gtk_label_new ("\302\260");
  gtk_label_set_xalign (GTK_LABEL (label), 0.0);
  gtk_grid_attach (GTK_GRID (grid), label, 2, 1, 1, 1);
  gtk_widget_show (label);

  measure->angle_label[1] = label = gtk_label_new (NULL);
  gtk_label_set_selectable (GTK_LABEL (label), TRUE);
  gtk_label_set_xalign (GTK_LABEL (label), 1.0);
  gtk_grid_attach (GTK_GRID (grid), label, 3, 1, 1, 1);
  gtk_widget_show (label);

  measure->unit_label[1] = label = gtk_label_new (NULL);
  gtk_label_set_xalign (GTK_LABEL (label), 0.0);
  gtk_grid_attach (GTK_GRID (grid), label, 4, 1, 1, 1);
  gtk_widget_show (label);


  label = gtk_label_new (_("Width:"));
  gtk_label_set_xalign (GTK_LABEL (label), 0.0);
  gtk_grid_attach (GTK_GRID (grid), label, 0, 2, 1, 1);
  gtk_widget_show (label);

  measure->width_label[0] = label = gtk_label_new ("0.0");
  gtk_label_set_selectable (GTK_LABEL (label), TRUE);
  gtk_label_set_xalign (GTK_LABEL (label), 1.0);
  gtk_grid_attach (GTK_GRID (grid), label, 1, 2, 1, 1);
  gtk_widget_show (label);

  label = gtk_label_new (_("pixels"));
  gtk_label_set_xalign (GTK_LABEL (label), 0.0);
  gtk_grid_attach (GTK_GRID (grid), label, 2, 2, 1, 1);
  gtk_widget_show (label);

  measure->width_label[1] = label = gtk_label_new ("0.0");
  gtk_label_set_selectable (GTK_LABEL (label), TRUE);
  gtk_label_set_xalign (GTK_LABEL (label), 1.0);
  gtk_grid_attach (GTK_GRID (grid), label, 3, 2, 1, 1);
  gtk_widget_show (label);

  measure->unit_label[2] = label = gtk_label_new (NULL);
  gtk_label_set_xalign (GTK_LABEL (label), 0.0);
  gtk_grid_attach (GTK_GRID (grid), label, 4, 2, 1, 1);
  gtk_widget_show (label);


  label = gtk_label_new (_("Height:"));
  gtk_label_set_xalign (GTK_LABEL (label), 0.0);
  gtk_grid_attach (GTK_GRID (grid), label, 0, 3, 1, 1);
  gtk_widget_show (label);

  measure->height_label[0] = label = gtk_label_new ("0.0");
  gtk_label_set_selectable (GTK_LABEL (label), TRUE);
  gtk_label_set_xalign (GTK_LABEL (label), 1.0);
  gtk_grid_attach (GTK_GRID (grid), label, 1, 3, 1, 1);
  gtk_widget_show (label);

  label = gtk_label_new (_("pixels"));
  gtk_label_set_xalign (GTK_LABEL (label), 0.0);
  gtk_grid_attach (GTK_GRID (grid), label, 2, 3, 1, 1);
  gtk_widget_show (label);

  measure->height_label[1] = label = gtk_label_new ("0.0");
  gtk_label_set_selectable (GTK_LABEL (label), TRUE);
  gtk_label_set_xalign (GTK_LABEL (label), 1.0);
  gtk_grid_attach (GTK_GRID (grid), label, 3, 3, 1, 1);
  gtk_widget_show (label);

  measure->unit_label[3] = label = gtk_label_new (NULL);
  gtk_label_set_xalign (GTK_LABEL (label), 0.0);
  gtk_grid_attach (GTK_GRID (grid), label, 4, 3, 1, 1);
  gtk_widget_show (label);

  return gui;
}

static void
ligma_measure_tool_straighten_button_clicked (GtkWidget       *button,
                                             LigmaMeasureTool *measure)
{
  LigmaTool          *tool    = LIGMA_TOOL (measure);
  LigmaTransformTool *tr_tool = LIGMA_TRANSFORM_TOOL (measure);

  if (ligma_transform_tool_transform (tr_tool, tool->display))
    ligma_tool_control (tool, LIGMA_TOOL_ACTION_HALT, tool->display);
}
