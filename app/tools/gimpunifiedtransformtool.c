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

#include "widgets/gimphelp-ids.h"

#include "display/gimpdisplay.h"
#include "display/gimptoolgui.h"
#include "display/gimptooltransformgrid.h"

#include "gimptoolcontrol.h"
#include "gimptransformgridoptions.h"
#include "gimpunifiedtransformtool.h"

#include "gimp-intl.h"


/*  index into trans_info array  */
enum
{
  X0,
  Y0,
  X1,
  Y1,
  X2,
  Y2,
  X3,
  Y3,
  PIVOT_X,
  PIVOT_Y,
};


/*  local function prototypes  */

static gchar          * gimp_unified_transform_tool_get_undo_desc  (GimpTransformTool        *tr_tool);

static void             gimp_unified_transform_tool_prepare        (GimpTransformGridTool    *tg_tool);
static GimpToolWidget * gimp_unified_transform_tool_get_widget     (GimpTransformGridTool    *tg_tool);
static void             gimp_unified_transform_tool_update_widget  (GimpTransformGridTool    *tg_tool);
static void             gimp_unified_transform_tool_widget_changed (GimpTransformGridTool    *tg_tool);

static void             gimp_unified_transform_tool_recalc_points  (GimpGenericTransformTool *generic);


G_DEFINE_TYPE (GimpUnifiedTransformTool, gimp_unified_transform_tool,
               GIMP_TYPE_GENERIC_TRANSFORM_TOOL)

#define parent_class gimp_unified_transform_tool_parent_class


void
gimp_unified_transform_tool_register (GimpToolRegisterCallback  callback,
                                      gpointer                  data)
{
  (* callback) (GIMP_TYPE_UNIFIED_TRANSFORM_TOOL,
                GIMP_TYPE_TRANSFORM_GRID_OPTIONS,
                gimp_transform_grid_options_gui,
                GIMP_CONTEXT_PROP_MASK_BACKGROUND,
                "gimp-unified-transform-tool",
                _("Unified Transform"),
                _("Unified Transform Tool: "
                  "Transform the layer, selection or path"),
                N_("_Unified Transform"), "<shift>T",
                NULL, GIMP_HELP_TOOL_UNIFIED_TRANSFORM,
                GIMP_ICON_TOOL_UNIFIED_TRANSFORM,
                data);
}

static void
gimp_unified_transform_tool_class_init (GimpUnifiedTransformToolClass *klass)
{
  GimpTransformToolClass        *tr_class      = GIMP_TRANSFORM_TOOL_CLASS (klass);
  GimpTransformGridToolClass    *tg_class      = GIMP_TRANSFORM_GRID_TOOL_CLASS (klass);
  GimpGenericTransformToolClass *generic_class = GIMP_GENERIC_TRANSFORM_TOOL_CLASS (klass);

  tr_class->get_undo_desc      = gimp_unified_transform_tool_get_undo_desc;

  tg_class->prepare            = gimp_unified_transform_tool_prepare;
  tg_class->get_widget         = gimp_unified_transform_tool_get_widget;
  tg_class->update_widget      = gimp_unified_transform_tool_update_widget;
  tg_class->widget_changed     = gimp_unified_transform_tool_widget_changed;

  generic_class->recalc_points = gimp_unified_transform_tool_recalc_points;

  tr_class->progress_text      = _("Unified transform");
}

static void
gimp_unified_transform_tool_init (GimpUnifiedTransformTool *unified_tool)
{
}

static gchar *
gimp_unified_transform_tool_get_undo_desc (GimpTransformTool *tr_tool)
{
  return g_strdup (C_("undo-type", "Unified Transform"));
}

static void
gimp_unified_transform_tool_prepare (GimpTransformGridTool *tg_tool)
{
  GimpTransformTool *tr_tool = GIMP_TRANSFORM_TOOL (tg_tool);

  GIMP_TRANSFORM_GRID_TOOL_CLASS (parent_class)->prepare (tg_tool);

  tg_tool->trans_info[PIVOT_X] = (gdouble) (tr_tool->x1 + tr_tool->x2) / 2.0;
  tg_tool->trans_info[PIVOT_Y] = (gdouble) (tr_tool->y1 + tr_tool->y2) / 2.0;

  tg_tool->trans_info[X0] = (gdouble) tr_tool->x1;
  tg_tool->trans_info[Y0] = (gdouble) tr_tool->y1;
  tg_tool->trans_info[X1] = (gdouble) tr_tool->x2;
  tg_tool->trans_info[Y1] = (gdouble) tr_tool->y1;
  tg_tool->trans_info[X2] = (gdouble) tr_tool->x1;
  tg_tool->trans_info[Y2] = (gdouble) tr_tool->y2;
  tg_tool->trans_info[X3] = (gdouble) tr_tool->x2;
  tg_tool->trans_info[Y3] = (gdouble) tr_tool->y2;
}

static GimpToolWidget *
gimp_unified_transform_tool_get_widget (GimpTransformGridTool *tg_tool)
{
  GimpTool          *tool    = GIMP_TOOL (tg_tool);
  GimpTransformTool *tr_tool = GIMP_TRANSFORM_TOOL (tg_tool);
  GimpDisplayShell  *shell   = gimp_display_get_shell (tool->display);
  GimpToolWidget    *widget;

  widget = gimp_tool_transform_grid_new (shell,
                                         &tr_tool->transform,
                                         tr_tool->x1,
                                         tr_tool->y1,
                                         tr_tool->x2,
                                         tr_tool->y2);

  g_object_set (widget,
                "pivot-x",                 (tr_tool->x1 + tr_tool->x2) / 2.0,
                "pivot-y",                 (tr_tool->y1 + tr_tool->y2) / 2.0,
                "inside-function",         GIMP_TRANSFORM_FUNCTION_MOVE,
                "outside-function",        GIMP_TRANSFORM_FUNCTION_ROTATE,
                "use-corner-handles",      TRUE,
                "use-perspective-handles", TRUE,
                "use-side-handles",        TRUE,
                "use-shear-handles",       TRUE,
                "use-pivot-handle",        TRUE,
                NULL);

  return widget;
}

static void
gimp_unified_transform_tool_update_widget (GimpTransformGridTool *tg_tool)
{
  GimpTransformTool *tr_tool = GIMP_TRANSFORM_TOOL (tg_tool);

  g_object_set (tg_tool->widget,
                "transform", &tr_tool->transform,
                "x1",        (gdouble) tr_tool->x1,
                "y1",        (gdouble) tr_tool->y1,
                "x2",        (gdouble) tr_tool->x2,
                "y2",        (gdouble) tr_tool->y2,
                "pivot-x",   tg_tool->trans_info[PIVOT_X],
                "pivot-y",   tg_tool->trans_info[PIVOT_Y],
                NULL);
}

static void
gimp_unified_transform_tool_widget_changed (GimpTransformGridTool *tg_tool)
{
  GimpTransformTool *tr_tool = GIMP_TRANSFORM_TOOL (tg_tool);
  GimpMatrix3       *transform;

  g_object_get (tg_tool->widget,
                "transform", &transform,
                "pivot-x",   &tg_tool->trans_info[PIVOT_X],
                "pivot-y",   &tg_tool->trans_info[PIVOT_Y],
                NULL);

  gimp_matrix3_transform_point (transform,
                                tr_tool->x1, tr_tool->y1,
                                &tg_tool->trans_info[X0],
                                &tg_tool->trans_info[Y0]);
  gimp_matrix3_transform_point (transform,
                                tr_tool->x2, tr_tool->y1,
                                &tg_tool->trans_info[X1],
                                &tg_tool->trans_info[Y1]);
  gimp_matrix3_transform_point (transform,
                                tr_tool->x1, tr_tool->y2,
                                &tg_tool->trans_info[X2],
                                &tg_tool->trans_info[Y2]);
  gimp_matrix3_transform_point (transform,
                                tr_tool->x2, tr_tool->y2,
                                &tg_tool->trans_info[X3],
                                &tg_tool->trans_info[Y3]);

  g_free (transform);

  GIMP_TRANSFORM_GRID_TOOL_CLASS (parent_class)->widget_changed (tg_tool);
}

static void
gimp_unified_transform_tool_recalc_points (GimpGenericTransformTool *generic)
{
  GimpTransformTool     *tr_tool = GIMP_TRANSFORM_TOOL (generic);
  GimpTransformGridTool *tg_tool = GIMP_TRANSFORM_GRID_TOOL (generic);

  generic->input_points[0]  = (GimpVector2) {tr_tool->x1, tr_tool->y1};
  generic->input_points[1]  = (GimpVector2) {tr_tool->x2, tr_tool->y1};
  generic->input_points[2]  = (GimpVector2) {tr_tool->x1, tr_tool->y2};
  generic->input_points[3]  = (GimpVector2) {tr_tool->x2, tr_tool->y2};

  generic->output_points[0] = (GimpVector2) {tg_tool->trans_info[X0],
                                             tg_tool->trans_info[Y0]};
  generic->output_points[1] = (GimpVector2) {tg_tool->trans_info[X1],
                                             tg_tool->trans_info[Y1]};
  generic->output_points[2] = (GimpVector2) {tg_tool->trans_info[X2],
                                             tg_tool->trans_info[Y2]};
  generic->output_points[3] = (GimpVector2) {tg_tool->trans_info[X3],
                                             tg_tool->trans_info[Y3]};
}
