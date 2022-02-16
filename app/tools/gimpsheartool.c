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
 * along with this program.  If not, see <https://www.gnu.org/licenses/>.
 */

#include "config.h"

#include <gegl.h>
#include <gtk/gtk.h>

#include "libgimpmath/gimpmath.h"
#include "libgimpwidgets/gimpwidgets.h"

#include "tools-types.h"

#include "core/gimp-transform-utils.h"

#include "widgets/gimphelp-ids.h"

#include "display/gimpdisplay.h"
#include "display/gimptoolgui.h"
#include "display/gimptoolsheargrid.h"

#include "gimpsheartool.h"
#include "gimptoolcontrol.h"
#include "gimptransformgridoptions.h"

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

static gboolean         gimp_shear_tool_info_to_matrix (GimpTransformGridTool *tg_tool,
                                                        GimpMatrix3           *transform);
static gchar          * gimp_shear_tool_get_undo_desc  (GimpTransformGridTool *tg_tool);
static void             gimp_shear_tool_dialog         (GimpTransformGridTool *tg_tool);
static void             gimp_shear_tool_dialog_update  (GimpTransformGridTool *tg_tool);
static void             gimp_shear_tool_prepare        (GimpTransformGridTool *tg_tool);
static GimpToolWidget * gimp_shear_tool_get_widget     (GimpTransformGridTool *tg_tool);
static void             gimp_shear_tool_update_widget  (GimpTransformGridTool *tg_tool);
static void             gimp_shear_tool_widget_changed (GimpTransformGridTool *tg_tool);

static void             shear_x_mag_changed            (GtkAdjustment         *adj,
                                                        GimpTransformGridTool *tg_tool);
static void             shear_y_mag_changed            (GtkAdjustment         *adj,
                                                        GimpTransformGridTool *tg_tool);


G_DEFINE_TYPE (GimpShearTool, gimp_shear_tool, GIMP_TYPE_TRANSFORM_GRID_TOOL)

#define parent_class gimp_shear_tool_parent_class


void
gimp_shear_tool_register (GimpToolRegisterCallback  callback,
                          gpointer                  data)
{
  (* callback) (GIMP_TYPE_SHEAR_TOOL,
                GIMP_TYPE_TRANSFORM_GRID_OPTIONS,
                gimp_transform_grid_options_gui,
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
  GimpTransformToolClass     *tr_class = GIMP_TRANSFORM_TOOL_CLASS (klass);
  GimpTransformGridToolClass *tg_class = GIMP_TRANSFORM_GRID_TOOL_CLASS (klass);

  tg_class->info_to_matrix  = gimp_shear_tool_info_to_matrix;
  tg_class->get_undo_desc   = gimp_shear_tool_get_undo_desc;
  tg_class->dialog          = gimp_shear_tool_dialog;
  tg_class->dialog_update   = gimp_shear_tool_dialog_update;
  tg_class->prepare         = gimp_shear_tool_prepare;
  tg_class->get_widget      = gimp_shear_tool_get_widget;
  tg_class->update_widget   = gimp_shear_tool_update_widget;
  tg_class->widget_changed  = gimp_shear_tool_widget_changed;

  tr_class->progress_text   = C_("undo-type", "Shear");
  tr_class->progress_text   = _("Shearing");
  tg_class->ok_button_label = _("_Shear");
}

static void
gimp_shear_tool_init (GimpShearTool *shear_tool)
{
  GimpTool *tool = GIMP_TOOL (shear_tool);

  gimp_tool_control_set_tool_cursor (tool->control, GIMP_TOOL_CURSOR_SHEAR);
}

static gboolean
gimp_shear_tool_info_to_matrix (GimpTransformGridTool *tg_tool,
                                GimpMatrix3           *transform)
{
  GimpTransformTool *tr_tool = GIMP_TRANSFORM_TOOL (tg_tool);
  gdouble            amount;

  if (tg_tool->trans_info[SHEAR_X] == 0.0 &&
      tg_tool->trans_info[SHEAR_Y] == 0.0)
    {
      tg_tool->trans_info[ORIENTATION] = GIMP_ORIENTATION_UNKNOWN;
    }

  if (tg_tool->trans_info[ORIENTATION] == GIMP_ORIENTATION_HORIZONTAL)
    amount = tg_tool->trans_info[SHEAR_X];
  else
    amount = tg_tool->trans_info[SHEAR_Y];

  gimp_matrix3_identity (transform);
  gimp_transform_matrix_shear (transform,
                               tr_tool->x1,
                               tr_tool->y1,
                               tr_tool->x2 - tr_tool->x1,
                               tr_tool->y2 - tr_tool->y1,
                               tg_tool->trans_info[ORIENTATION],
                               amount);

  return TRUE;
}

static gchar *
gimp_shear_tool_get_undo_desc (GimpTransformGridTool *tg_tool)
{
  gdouble x = tg_tool->trans_info[SHEAR_X];
  gdouble y = tg_tool->trans_info[SHEAR_Y];

  switch ((gint) tg_tool->trans_info[ORIENTATION])
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
gimp_shear_tool_dialog (GimpTransformGridTool *tg_tool)
{
  GimpShearTool *shear = GIMP_SHEAR_TOOL (tg_tool);
  GtkWidget     *vbox;
  GtkWidget     *scale;

  vbox = gtk_box_new (GTK_ORIENTATION_VERTICAL, 2);
  gtk_box_pack_start (GTK_BOX (gimp_tool_gui_get_vbox (tg_tool->gui)), vbox,
                      FALSE, FALSE, 0);
  gtk_widget_show (vbox);

  shear->x_adj = gtk_adjustment_new (0, -65536, 65536, 1, 10, 0);
  scale = gimp_spin_scale_new (shear->x_adj, _("Shear magnitude _X"), 0);
  gimp_spin_scale_set_scale_limits (GIMP_SPIN_SCALE (scale), -1000, 1000);
  gtk_box_pack_start (GTK_BOX (vbox), scale, FALSE, FALSE, 0);
  gtk_widget_show (scale);

  g_signal_connect (shear->x_adj, "value-changed",
                    G_CALLBACK (shear_x_mag_changed),
                    tg_tool);

  shear->y_adj = gtk_adjustment_new (0, -65536, 65536, 1, 10, 0);
  scale = gimp_spin_scale_new (shear->y_adj, _("Shear magnitude _Y"), 0);
  gimp_spin_scale_set_scale_limits (GIMP_SPIN_SCALE (scale), -1000, 1000);
  gtk_box_pack_start (GTK_BOX (vbox), scale, FALSE, FALSE, 0);
  gtk_widget_show (scale);

  g_signal_connect (shear->y_adj, "value-changed",
                    G_CALLBACK (shear_y_mag_changed),
                    tg_tool);
}

static void
gimp_shear_tool_dialog_update (GimpTransformGridTool *tg_tool)
{
  GimpShearTool *shear = GIMP_SHEAR_TOOL (tg_tool);

  gtk_adjustment_set_value (shear->x_adj, tg_tool->trans_info[SHEAR_X]);
  gtk_adjustment_set_value (shear->y_adj, tg_tool->trans_info[SHEAR_Y]);
}

static void
gimp_shear_tool_prepare (GimpTransformGridTool *tg_tool)
{
  tg_tool->trans_info[ORIENTATION] = GIMP_ORIENTATION_UNKNOWN;
  tg_tool->trans_info[SHEAR_X]     = 0.0;
  tg_tool->trans_info[SHEAR_Y]     = 0.0;
}

static GimpToolWidget *
gimp_shear_tool_get_widget (GimpTransformGridTool *tg_tool)
{
  GimpTool          *tool    = GIMP_TOOL (tg_tool);
  GimpTransformTool *tr_tool = GIMP_TRANSFORM_TOOL (tg_tool);
  GimpDisplayShell  *shell   = gimp_display_get_shell (tool->display);
  GimpToolWidget    *widget;

  widget = gimp_tool_shear_grid_new (shell,
                                     tr_tool->x1,
                                     tr_tool->y1,
                                     tr_tool->x2,
                                     tr_tool->y2,
                                     tg_tool->trans_info[ORIENTATION],
                                     tg_tool->trans_info[SHEAR_X],
                                     tg_tool->trans_info[SHEAR_Y]);

  g_object_set (widget,
                "inside-function",  GIMP_TRANSFORM_FUNCTION_SHEAR,
                "outside-function", GIMP_TRANSFORM_FUNCTION_SHEAR,
                "frompivot-shear",  TRUE,
                NULL);

  return widget;
}

static void
gimp_shear_tool_update_widget (GimpTransformGridTool *tg_tool)
{
  GimpTransformTool *tr_tool = GIMP_TRANSFORM_TOOL (tg_tool);

  GIMP_TRANSFORM_GRID_TOOL_CLASS (parent_class)->update_widget (tg_tool);

  g_object_set (tg_tool->widget,
                "x1",          (gdouble) tr_tool->x1,
                "y1",          (gdouble) tr_tool->y1,
                "x2",          (gdouble) tr_tool->x2,
                "y2",          (gdouble) tr_tool->y2,
                "orientation", (gint) tg_tool->trans_info[ORIENTATION],
                "shear-x",     tg_tool->trans_info[SHEAR_X],
                "shear-y",     tg_tool->trans_info[SHEAR_Y],
                NULL);
}

static void
gimp_shear_tool_widget_changed (GimpTransformGridTool *tg_tool)
{
  GimpOrientationType orientation;

  g_object_get (tg_tool->widget,
                "orientation", &orientation,
                "shear-x",     &tg_tool->trans_info[SHEAR_X],
                "shear-y",     &tg_tool->trans_info[SHEAR_Y],
                NULL);

  tg_tool->trans_info[ORIENTATION] = orientation;

  GIMP_TRANSFORM_GRID_TOOL_CLASS (parent_class)->widget_changed (tg_tool);
}

static void
shear_x_mag_changed (GtkAdjustment         *adj,
                     GimpTransformGridTool *tg_tool)
{
  gdouble value = gtk_adjustment_get_value (adj);

  if (value != tg_tool->trans_info[SHEAR_X])
    {
      GimpTool          *tool    = GIMP_TOOL (tg_tool);
      GimpTransformTool *tr_tool = GIMP_TRANSFORM_TOOL (tg_tool);

      tg_tool->trans_info[ORIENTATION] = GIMP_ORIENTATION_HORIZONTAL;

      tg_tool->trans_info[SHEAR_X] = value;
      tg_tool->trans_info[SHEAR_Y] = 0.0;  /* can only shear in one axis */

      gimp_transform_grid_tool_push_internal_undo (tg_tool, TRUE);

      gimp_transform_tool_recalc_matrix (tr_tool, tool->display);
    }
}

static void
shear_y_mag_changed (GtkAdjustment         *adj,
                     GimpTransformGridTool *tg_tool)
{
  gdouble value = gtk_adjustment_get_value (adj);

  if (value != tg_tool->trans_info[SHEAR_Y])
    {
      GimpTool          *tool    = GIMP_TOOL (tg_tool);
      GimpTransformTool *tr_tool = GIMP_TRANSFORM_TOOL (tg_tool);

      tg_tool->trans_info[ORIENTATION] = GIMP_ORIENTATION_VERTICAL;

      tg_tool->trans_info[SHEAR_Y] = value;
      tg_tool->trans_info[SHEAR_X] = 0.0;  /* can only shear in one axis */

      gimp_transform_grid_tool_push_internal_undo (tg_tool, TRUE);

      gimp_transform_tool_recalc_matrix (tr_tool, tool->display);
    }
}
