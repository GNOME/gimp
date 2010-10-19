/* GIMP - The GNU Image Manipulation Program
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
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

#include "libgimpmath/gimpmath.h"
#include "libgimpwidgets/gimpwidgets.h"

#include "tools-types.h"

#include "core/gimp-transform-utils.h"

#include "widgets/gimphelp-ids.h"
#include "widgets/gimpspinscale.h"

#include "display/gimpdisplay.h"
#include "display/gimptoolgui.h"
#include "display/gimptoolsheargrid.h"

#include "gimpsheartool.h"
#include "gimptoolcontrol.h"
#include "gimptransformoptions.h"

#include "gimp-intl.h"


/*  index into trans_info array  */
enum
{
  ORIENTATION,
  SHEAR_X,
  SHEAR_Y
};


#define SB_WIDTH 10


/*  local function prototypes  */

static void             gimp_shear_tool_dialog         (GimpTransformTool *tr_tool);
static void             gimp_shear_tool_dialog_update  (GimpTransformTool *tr_tool);

static void             gimp_shear_tool_prepare        (GimpTransformTool *tr_tool);
static GimpToolWidget * gimp_shear_tool_get_widget     (GimpTransformTool *tr_tool);
static void             gimp_shear_tool_recalc_matrix  (GimpTransformTool *tr_tool,
                                                        GimpToolWidget    *widget);
static gchar          * gimp_shear_tool_get_undo_desc  (GimpTransformTool *tr_tool);

static void             gimp_shear_tool_widget_changed (GimpToolWidget    *widget,
                                                        GimpTransformTool *tr_tool);

static void             shear_x_mag_changed            (GtkAdjustment     *adj,
                                                        GimpTransformTool *tr_tool);
static void             shear_y_mag_changed            (GtkAdjustment     *adj,
                                                        GimpTransformTool *tr_tool);


G_DEFINE_TYPE (GimpShearTool, gimp_shear_tool, GIMP_TYPE_TRANSFORM_TOOL)


void
gimp_shear_tool_register (GimpToolRegisterCallback  callback,
                          gpointer                  data)
{
  (* callback) (GIMP_TYPE_SHEAR_TOOL,
                GIMP_TYPE_TRANSFORM_OPTIONS,
                gimp_transform_options_gui,
                0,
                "gimp-shear-tool",
                _("Shear"),
                _("Shear Tool: Shear the layer, selection or path"),
                N_("S_hear"), "<shift>H",
                NULL, GIMP_HELP_TOOL_SHEAR,
                GIMP_ICON_TOOL_SHEAR,
                data);
}

static void
gimp_shear_tool_class_init (GimpShearToolClass *klass)
{
  GimpTransformToolClass *trans_class = GIMP_TRANSFORM_TOOL_CLASS (klass);

  trans_class->dialog          = gimp_shear_tool_dialog;
  trans_class->dialog_update   = gimp_shear_tool_dialog_update;
  trans_class->prepare         = gimp_shear_tool_prepare;
  trans_class->get_widget      = gimp_shear_tool_get_widget;
  trans_class->recalc_matrix   = gimp_shear_tool_recalc_matrix;
  trans_class->get_undo_desc   = gimp_shear_tool_get_undo_desc;

  trans_class->ok_button_label = _("_Shear");
}

static void
gimp_shear_tool_init (GimpShearTool *shear_tool)
{
  GimpTool          *tool    = GIMP_TOOL (shear_tool);
  GimpTransformTool *tr_tool = GIMP_TRANSFORM_TOOL (shear_tool);

  gimp_tool_control_set_tool_cursor (tool->control, GIMP_TOOL_CURSOR_SHEAR);

  tr_tool->progress_text = _("Shearing");
}

static void
gimp_shear_tool_dialog (GimpTransformTool *tr_tool)
{
  GimpShearTool *shear = GIMP_SHEAR_TOOL (tr_tool);
  GtkWidget     *vbox;
  GtkWidget     *scale;

  vbox = gtk_box_new (GTK_ORIENTATION_VERTICAL, 2);
  gtk_box_pack_start (GTK_BOX (gimp_tool_gui_get_vbox (tr_tool->gui)), vbox,
                      FALSE, FALSE, 0);
  gtk_widget_show (vbox);

  shear->x_adj = gtk_adjustment_new (0, -65536, 65536, 1, 10, 0);
  scale = gimp_spin_scale_new (shear->x_adj, _("Shear magnitude _X"), 0);
  gimp_spin_scale_set_scale_limits (GIMP_SPIN_SCALE (scale), -1000, 1000);
  gtk_box_pack_start (GTK_BOX (vbox), scale, FALSE, FALSE, 0);
  gtk_widget_show (scale);

  g_signal_connect (shear->x_adj, "value-changed",
                    G_CALLBACK (shear_x_mag_changed),
                    tr_tool);

  shear->y_adj = gtk_adjustment_new (0, -65536, 65536, 1, 10, 0);
  scale = gimp_spin_scale_new (shear->y_adj, _("Shear magnitude _Y"), 0);
  gimp_spin_scale_set_scale_limits (GIMP_SPIN_SCALE (scale), -1000, 1000);
  gtk_box_pack_start (GTK_BOX (vbox), scale, FALSE, FALSE, 0);
  gtk_widget_show (scale);

  g_signal_connect (shear->y_adj, "value-changed",
                    G_CALLBACK (shear_y_mag_changed),
                    tr_tool);
}

static void
gimp_shear_tool_dialog_update (GimpTransformTool *tr_tool)
{
  GimpShearTool *shear = GIMP_SHEAR_TOOL (tr_tool);

  gtk_adjustment_set_value (shear->x_adj, tr_tool->trans_info[SHEAR_X]);
  gtk_adjustment_set_value (shear->y_adj, tr_tool->trans_info[SHEAR_Y]);
}

static void
gimp_shear_tool_prepare (GimpTransformTool *tr_tool)
{
  tr_tool->trans_info[ORIENTATION] = GIMP_ORIENTATION_UNKNOWN;
  tr_tool->trans_info[SHEAR_X]      = 0.0;
  tr_tool->trans_info[SHEAR_Y]      = 0.0;
}

static GimpToolWidget *
gimp_shear_tool_get_widget (GimpTransformTool *tr_tool)
{
  GimpTool         *tool  = GIMP_TOOL (tr_tool);
  GimpDisplayShell *shell = gimp_display_get_shell (tool->display);
  GimpToolWidget   *widget;

  widget = gimp_tool_shear_grid_new (shell,
                                     tr_tool->x1,
                                     tr_tool->y1,
                                     tr_tool->x2,
                                     tr_tool->y2,
                                     tr_tool->trans_info[ORIENTATION],
                                     tr_tool->trans_info[SHEAR_X],
                                     tr_tool->trans_info[SHEAR_Y]);

  g_object_set (widget,
                "inside-function",  GIMP_TRANSFORM_FUNCTION_SHEAR,
                "outside-function", GIMP_TRANSFORM_FUNCTION_SHEAR,
                "frompivot-shear",  TRUE,
                NULL);

  g_signal_connect (widget, "changed",
                    G_CALLBACK (gimp_shear_tool_widget_changed),
                    tr_tool);

  return widget;
}

static void
gimp_shear_tool_recalc_matrix (GimpTransformTool *tr_tool,
                               GimpToolWidget    *widget)
{
  gdouble amount;

  if (tr_tool->trans_info[SHEAR_X] == 0.0 &&
      tr_tool->trans_info[SHEAR_Y] == 0.0)
    {
      tr_tool->trans_info[ORIENTATION] = GIMP_ORIENTATION_UNKNOWN;
    }

  if (tr_tool->trans_info[ORIENTATION] == GIMP_ORIENTATION_HORIZONTAL)
    amount = tr_tool->trans_info[SHEAR_X];
  else
    amount = tr_tool->trans_info[SHEAR_Y];

  gimp_matrix3_identity (&tr_tool->transform);
  gimp_transform_matrix_shear (&tr_tool->transform,
                               tr_tool->x1,
                               tr_tool->y1,
                               tr_tool->x2 - tr_tool->x1,
                               tr_tool->y2 - tr_tool->y1,
                               tr_tool->trans_info[ORIENTATION],
                               amount);

  if (widget)
    g_object_set (widget,
                  "transform",   &tr_tool->transform,
                  "x1",          (gdouble) tr_tool->x1,
                  "y1",          (gdouble) tr_tool->y1,
                  "x2",          (gdouble) tr_tool->x2,
                  "y2",          (gdouble) tr_tool->y2,
                  "orientation", (gint) tr_tool->trans_info[ORIENTATION],
                  "shear-x",     tr_tool->trans_info[SHEAR_X],
                  "shear-y",     tr_tool->trans_info[SHEAR_Y],
                  NULL);
}

static gchar *
gimp_shear_tool_get_undo_desc (GimpTransformTool *tr_tool)
{
  gdouble x = tr_tool->trans_info[SHEAR_X];
  gdouble y = tr_tool->trans_info[SHEAR_Y];

  switch ((gint) tr_tool->trans_info[ORIENTATION])
    {
    case GIMP_ORIENTATION_HORIZONTAL:
      return g_strdup_printf (C_("undo-type", "Shear horizontally by %-3.3g"),
                              x);

    case GIMP_ORIENTATION_VERTICAL:
      return g_strdup_printf (C_("undo-type", "Shear vertically by %-3.3g"),
                              y);

    default:
      /* e.g. user entered numbers but no notification callback */
      return g_strdup_printf (C_("undo-type", "Shear horizontally by %-3.3g, vertically by %-3.3g"),
                              x, y);
    }
}

static void
gimp_shear_tool_widget_changed (GimpToolWidget    *widget,
                                GimpTransformTool *tr_tool)
{
  GimpOrientationType orientation;

  g_object_get (widget,
                "orientation", &orientation,
                "shear-x",     &tr_tool->trans_info[SHEAR_X],
                "shear-y",     &tr_tool->trans_info[SHEAR_Y],
                NULL);

  tr_tool->trans_info[ORIENTATION] = orientation;

  gimp_transform_tool_recalc_matrix (tr_tool, NULL);
}

static void
shear_x_mag_changed (GtkAdjustment     *adj,
                     GimpTransformTool *tr_tool)
{
  gdouble value = gtk_adjustment_get_value (adj);

  if (value != tr_tool->trans_info[SHEAR_X])
    {
      tr_tool->trans_info[ORIENTATION] = GIMP_ORIENTATION_HORIZONTAL;

      tr_tool->trans_info[SHEAR_X] = value;
      tr_tool->trans_info[SHEAR_Y] = 0.0;  /* can only shear in one axis */

      gimp_transform_tool_push_internal_undo (tr_tool);

      gimp_transform_tool_recalc_matrix (tr_tool, tr_tool->widget);
    }
}

static void
shear_y_mag_changed (GtkAdjustment     *adj,
                     GimpTransformTool *tr_tool)
{
  gdouble value = gtk_adjustment_get_value (adj);

  if (value != tr_tool->trans_info[SHEAR_Y])
    {
      tr_tool->trans_info[ORIENTATION] = GIMP_ORIENTATION_VERTICAL;

      tr_tool->trans_info[SHEAR_Y] = value;
      tr_tool->trans_info[SHEAR_X] = 0.0;  /* can only shear in one axis */

      gimp_transform_tool_push_internal_undo (tr_tool);

      gimp_transform_tool_recalc_matrix (tr_tool, tr_tool->widget);
    }
}
