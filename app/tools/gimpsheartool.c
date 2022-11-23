/* LIGMA - The GNU Image Manipulation Program
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

#include "libligmamath/ligmamath.h"
#include "libligmawidgets/ligmawidgets.h"

#include "tools-types.h"

#include "core/ligma-transform-utils.h"

#include "widgets/ligmahelp-ids.h"

#include "display/ligmadisplay.h"
#include "display/ligmatoolgui.h"
#include "display/ligmatoolsheargrid.h"

#include "ligmasheartool.h"
#include "ligmatoolcontrol.h"
#include "ligmatransformgridoptions.h"

#include "ligma-intl.h"


/*  index into trans_info array  */
enum
{
  ORIENTATION,
  SHEAR_X,
  SHEAR_Y
};


#define SB_WIDTH 10


/*  local function prototypes  */

static gboolean         ligma_shear_tool_info_to_matrix (LigmaTransformGridTool *tg_tool,
                                                        LigmaMatrix3           *transform);
static gchar          * ligma_shear_tool_get_undo_desc  (LigmaTransformGridTool *tg_tool);
static void             ligma_shear_tool_dialog         (LigmaTransformGridTool *tg_tool);
static void             ligma_shear_tool_dialog_update  (LigmaTransformGridTool *tg_tool);
static void             ligma_shear_tool_prepare        (LigmaTransformGridTool *tg_tool);
static LigmaToolWidget * ligma_shear_tool_get_widget     (LigmaTransformGridTool *tg_tool);
static void             ligma_shear_tool_update_widget  (LigmaTransformGridTool *tg_tool);
static void             ligma_shear_tool_widget_changed (LigmaTransformGridTool *tg_tool);

static void             shear_x_mag_changed            (GtkAdjustment         *adj,
                                                        LigmaTransformGridTool *tg_tool);
static void             shear_y_mag_changed            (GtkAdjustment         *adj,
                                                        LigmaTransformGridTool *tg_tool);


G_DEFINE_TYPE (LigmaShearTool, ligma_shear_tool, LIGMA_TYPE_TRANSFORM_GRID_TOOL)

#define parent_class ligma_shear_tool_parent_class


void
ligma_shear_tool_register (LigmaToolRegisterCallback  callback,
                          gpointer                  data)
{
  (* callback) (LIGMA_TYPE_SHEAR_TOOL,
                LIGMA_TYPE_TRANSFORM_GRID_OPTIONS,
                ligma_transform_grid_options_gui,
                0,
                "ligma-shear-tool",
                _("Shear"),
                _("Shear Tool: Shear the layer, selection or path"),
                N_("S_hear"), "<shift>H",
                NULL, LIGMA_HELP_TOOL_SHEAR,
                LIGMA_ICON_TOOL_SHEAR,
                data);
}

static void
ligma_shear_tool_class_init (LigmaShearToolClass *klass)
{
  LigmaTransformToolClass     *tr_class = LIGMA_TRANSFORM_TOOL_CLASS (klass);
  LigmaTransformGridToolClass *tg_class = LIGMA_TRANSFORM_GRID_TOOL_CLASS (klass);

  tg_class->info_to_matrix  = ligma_shear_tool_info_to_matrix;
  tg_class->get_undo_desc   = ligma_shear_tool_get_undo_desc;
  tg_class->dialog          = ligma_shear_tool_dialog;
  tg_class->dialog_update   = ligma_shear_tool_dialog_update;
  tg_class->prepare         = ligma_shear_tool_prepare;
  tg_class->get_widget      = ligma_shear_tool_get_widget;
  tg_class->update_widget   = ligma_shear_tool_update_widget;
  tg_class->widget_changed  = ligma_shear_tool_widget_changed;

  tr_class->progress_text   = C_("undo-type", "Shear");
  tr_class->progress_text   = _("Shearing");
  tg_class->ok_button_label = _("_Shear");
}

static void
ligma_shear_tool_init (LigmaShearTool *shear_tool)
{
  LigmaTool *tool = LIGMA_TOOL (shear_tool);

  ligma_tool_control_set_tool_cursor (tool->control, LIGMA_TOOL_CURSOR_SHEAR);
}

static gboolean
ligma_shear_tool_info_to_matrix (LigmaTransformGridTool *tg_tool,
                                LigmaMatrix3           *transform)
{
  LigmaTransformTool *tr_tool = LIGMA_TRANSFORM_TOOL (tg_tool);
  gdouble            amount;

  if (tg_tool->trans_info[SHEAR_X] == 0.0 &&
      tg_tool->trans_info[SHEAR_Y] == 0.0)
    {
      tg_tool->trans_info[ORIENTATION] = LIGMA_ORIENTATION_UNKNOWN;
    }

  if (tg_tool->trans_info[ORIENTATION] == LIGMA_ORIENTATION_HORIZONTAL)
    amount = tg_tool->trans_info[SHEAR_X];
  else
    amount = tg_tool->trans_info[SHEAR_Y];

  ligma_matrix3_identity (transform);
  ligma_transform_matrix_shear (transform,
                               tr_tool->x1,
                               tr_tool->y1,
                               tr_tool->x2 - tr_tool->x1,
                               tr_tool->y2 - tr_tool->y1,
                               tg_tool->trans_info[ORIENTATION],
                               amount);

  return TRUE;
}

static gchar *
ligma_shear_tool_get_undo_desc (LigmaTransformGridTool *tg_tool)
{
  gdouble x = tg_tool->trans_info[SHEAR_X];
  gdouble y = tg_tool->trans_info[SHEAR_Y];

  switch ((gint) tg_tool->trans_info[ORIENTATION])
    {
    case LIGMA_ORIENTATION_HORIZONTAL:
      return g_strdup_printf (C_("undo-type", "Shear horizontally by %-3.3g"),
                              x);

    case LIGMA_ORIENTATION_VERTICAL:
      return g_strdup_printf (C_("undo-type", "Shear vertically by %-3.3g"),
                              y);

    default:
      /* e.g. user entered numbers but no notification callback */
      return g_strdup_printf (C_("undo-type", "Shear horizontally by %-3.3g, vertically by %-3.3g"),
                              x, y);
    }
}

static void
ligma_shear_tool_dialog (LigmaTransformGridTool *tg_tool)
{
  LigmaShearTool *shear = LIGMA_SHEAR_TOOL (tg_tool);
  GtkWidget     *vbox;
  GtkWidget     *scale;

  vbox = gtk_box_new (GTK_ORIENTATION_VERTICAL, 2);
  gtk_box_pack_start (GTK_BOX (ligma_tool_gui_get_vbox (tg_tool->gui)), vbox,
                      FALSE, FALSE, 0);
  gtk_widget_show (vbox);

  shear->x_adj = gtk_adjustment_new (0, -65536, 65536, 1, 10, 0);
  scale = ligma_spin_scale_new (shear->x_adj, _("Shear magnitude _X"), 0);
  ligma_spin_scale_set_scale_limits (LIGMA_SPIN_SCALE (scale), -1000, 1000);
  gtk_box_pack_start (GTK_BOX (vbox), scale, FALSE, FALSE, 0);
  gtk_widget_show (scale);

  g_signal_connect (shear->x_adj, "value-changed",
                    G_CALLBACK (shear_x_mag_changed),
                    tg_tool);

  shear->y_adj = gtk_adjustment_new (0, -65536, 65536, 1, 10, 0);
  scale = ligma_spin_scale_new (shear->y_adj, _("Shear magnitude _Y"), 0);
  ligma_spin_scale_set_scale_limits (LIGMA_SPIN_SCALE (scale), -1000, 1000);
  gtk_box_pack_start (GTK_BOX (vbox), scale, FALSE, FALSE, 0);
  gtk_widget_show (scale);

  g_signal_connect (shear->y_adj, "value-changed",
                    G_CALLBACK (shear_y_mag_changed),
                    tg_tool);
}

static void
ligma_shear_tool_dialog_update (LigmaTransformGridTool *tg_tool)
{
  LigmaShearTool *shear = LIGMA_SHEAR_TOOL (tg_tool);

  gtk_adjustment_set_value (shear->x_adj, tg_tool->trans_info[SHEAR_X]);
  gtk_adjustment_set_value (shear->y_adj, tg_tool->trans_info[SHEAR_Y]);
}

static void
ligma_shear_tool_prepare (LigmaTransformGridTool *tg_tool)
{
  tg_tool->trans_info[ORIENTATION] = LIGMA_ORIENTATION_UNKNOWN;
  tg_tool->trans_info[SHEAR_X]     = 0.0;
  tg_tool->trans_info[SHEAR_Y]     = 0.0;
}

static LigmaToolWidget *
ligma_shear_tool_get_widget (LigmaTransformGridTool *tg_tool)
{
  LigmaTool          *tool    = LIGMA_TOOL (tg_tool);
  LigmaTransformTool *tr_tool = LIGMA_TRANSFORM_TOOL (tg_tool);
  LigmaDisplayShell  *shell   = ligma_display_get_shell (tool->display);
  LigmaToolWidget    *widget;

  widget = ligma_tool_shear_grid_new (shell,
                                     tr_tool->x1,
                                     tr_tool->y1,
                                     tr_tool->x2,
                                     tr_tool->y2,
                                     tg_tool->trans_info[ORIENTATION],
                                     tg_tool->trans_info[SHEAR_X],
                                     tg_tool->trans_info[SHEAR_Y]);

  g_object_set (widget,
                "inside-function",  LIGMA_TRANSFORM_FUNCTION_SHEAR,
                "outside-function", LIGMA_TRANSFORM_FUNCTION_SHEAR,
                "frompivot-shear",  TRUE,
                NULL);

  return widget;
}

static void
ligma_shear_tool_update_widget (LigmaTransformGridTool *tg_tool)
{
  LigmaTransformTool *tr_tool = LIGMA_TRANSFORM_TOOL (tg_tool);

  LIGMA_TRANSFORM_GRID_TOOL_CLASS (parent_class)->update_widget (tg_tool);

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
ligma_shear_tool_widget_changed (LigmaTransformGridTool *tg_tool)
{
  LigmaOrientationType orientation;

  g_object_get (tg_tool->widget,
                "orientation", &orientation,
                "shear-x",     &tg_tool->trans_info[SHEAR_X],
                "shear-y",     &tg_tool->trans_info[SHEAR_Y],
                NULL);

  tg_tool->trans_info[ORIENTATION] = orientation;

  LIGMA_TRANSFORM_GRID_TOOL_CLASS (parent_class)->widget_changed (tg_tool);
}

static void
shear_x_mag_changed (GtkAdjustment         *adj,
                     LigmaTransformGridTool *tg_tool)
{
  gdouble value = gtk_adjustment_get_value (adj);

  if (value != tg_tool->trans_info[SHEAR_X])
    {
      LigmaTool          *tool    = LIGMA_TOOL (tg_tool);
      LigmaTransformTool *tr_tool = LIGMA_TRANSFORM_TOOL (tg_tool);

      tg_tool->trans_info[ORIENTATION] = LIGMA_ORIENTATION_HORIZONTAL;

      tg_tool->trans_info[SHEAR_X] = value;
      tg_tool->trans_info[SHEAR_Y] = 0.0;  /* can only shear in one axis */

      ligma_transform_grid_tool_push_internal_undo (tg_tool, TRUE);

      ligma_transform_tool_recalc_matrix (tr_tool, tool->display);
    }
}

static void
shear_y_mag_changed (GtkAdjustment         *adj,
                     LigmaTransformGridTool *tg_tool)
{
  gdouble value = gtk_adjustment_get_value (adj);

  if (value != tg_tool->trans_info[SHEAR_Y])
    {
      LigmaTool          *tool    = LIGMA_TOOL (tg_tool);
      LigmaTransformTool *tr_tool = LIGMA_TRANSFORM_TOOL (tg_tool);

      tg_tool->trans_info[ORIENTATION] = LIGMA_ORIENTATION_VERTICAL;

      tg_tool->trans_info[SHEAR_Y] = value;
      tg_tool->trans_info[SHEAR_X] = 0.0;  /* can only shear in one axis */

      ligma_transform_grid_tool_push_internal_undo (tg_tool, TRUE);

      ligma_transform_tool_recalc_matrix (tr_tool, tool->display);
    }
}
