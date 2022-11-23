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

#include "widgets/ligmahelp-ids.h"

#include "display/ligmadisplay.h"
#include "display/ligmadisplayshell.h"
#include "display/ligmadisplayshell-transform.h"
#include "display/ligmatoolgui.h"
#include "display/ligmatooltransformgrid.h"

#include "ligmaperspectivetool.h"
#include "ligmatoolcontrol.h"
#include "ligmatransformgridoptions.h"

#include "ligma-intl.h"


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
  Y3
};


/*  local function prototypes  */

static void             ligma_perspective_tool_matrix_to_info (LigmaTransformGridTool    *tg_tool,
                                                              const LigmaMatrix3        *transform);
static void             ligma_perspective_tool_prepare        (LigmaTransformGridTool    *tg_tool);
static void             ligma_perspective_tool_readjust       (LigmaTransformGridTool    *tg_tool);
static LigmaToolWidget * ligma_perspective_tool_get_widget     (LigmaTransformGridTool    *tg_tool);
static void             ligma_perspective_tool_update_widget  (LigmaTransformGridTool    *tg_tool);
static void             ligma_perspective_tool_widget_changed (LigmaTransformGridTool    *tg_tool);

static void             ligma_perspective_tool_info_to_points (LigmaGenericTransformTool *generic);


G_DEFINE_TYPE (LigmaPerspectiveTool, ligma_perspective_tool,
               LIGMA_TYPE_GENERIC_TRANSFORM_TOOL)

#define parent_class ligma_perspective_tool_parent_class


void
ligma_perspective_tool_register (LigmaToolRegisterCallback  callback,
                                gpointer                  data)
{
  (* callback) (LIGMA_TYPE_PERSPECTIVE_TOOL,
                LIGMA_TYPE_TRANSFORM_GRID_OPTIONS,
                ligma_transform_grid_options_gui,
                LIGMA_CONTEXT_PROP_MASK_BACKGROUND,
                "ligma-perspective-tool",
                _("Perspective"),
                _("Perspective Tool: "
                  "Change perspective of the layer, selection or path"),
                N_("_Perspective"), "<shift>P",
                NULL, LIGMA_HELP_TOOL_PERSPECTIVE,
                LIGMA_ICON_TOOL_PERSPECTIVE,
                data);
}

static void
ligma_perspective_tool_class_init (LigmaPerspectiveToolClass *klass)
{
  LigmaTransformToolClass        *tr_class      = LIGMA_TRANSFORM_TOOL_CLASS (klass);
  LigmaTransformGridToolClass    *tg_class      = LIGMA_TRANSFORM_GRID_TOOL_CLASS (klass);
  LigmaGenericTransformToolClass *generic_class = LIGMA_GENERIC_TRANSFORM_TOOL_CLASS (klass);

  tg_class->matrix_to_info      = ligma_perspective_tool_matrix_to_info;
  tg_class->prepare             = ligma_perspective_tool_prepare;
  tg_class->readjust            = ligma_perspective_tool_readjust;
  tg_class->get_widget          = ligma_perspective_tool_get_widget;
  tg_class->update_widget       = ligma_perspective_tool_update_widget;
  tg_class->widget_changed      = ligma_perspective_tool_widget_changed;

  generic_class->info_to_points = ligma_perspective_tool_info_to_points;

  tr_class->undo_desc           = C_("undo-type", "Perspective");
  tr_class->progress_text       = _("Perspective transformation");
}

static void
ligma_perspective_tool_init (LigmaPerspectiveTool *perspective_tool)
{
  LigmaTool *tool = LIGMA_TOOL (perspective_tool);

  ligma_tool_control_set_tool_cursor (tool->control,
                                     LIGMA_TOOL_CURSOR_PERSPECTIVE);
}

static void
ligma_perspective_tool_matrix_to_info (LigmaTransformGridTool *tg_tool,
                                      const LigmaMatrix3     *transform)
{
  LigmaTransformTool *tr_tool = LIGMA_TRANSFORM_TOOL (tg_tool);

  ligma_matrix3_transform_point (transform,
                                tr_tool->x1,
                                tr_tool->y1,
                                &tg_tool->trans_info[X0],
                                &tg_tool->trans_info[Y0]);
  ligma_matrix3_transform_point (transform,
                                tr_tool->x2,
                                tr_tool->y1,
                                &tg_tool->trans_info[X1],
                                &tg_tool->trans_info[Y1]);
  ligma_matrix3_transform_point (transform,
                                tr_tool->x1,
                                tr_tool->y2,
                                &tg_tool->trans_info[X2],
                                &tg_tool->trans_info[Y2]);
  ligma_matrix3_transform_point (transform,
                                tr_tool->x2,
                                tr_tool->y2,
                                &tg_tool->trans_info[X3],
                                &tg_tool->trans_info[Y3]);
}

static void
ligma_perspective_tool_prepare (LigmaTransformGridTool *tg_tool)
{
  LigmaTransformTool *tr_tool = LIGMA_TRANSFORM_TOOL (tg_tool);

  LIGMA_TRANSFORM_GRID_TOOL_CLASS (parent_class)->prepare (tg_tool);

  tg_tool->trans_info[X0] = (gdouble) tr_tool->x1;
  tg_tool->trans_info[Y0] = (gdouble) tr_tool->y1;
  tg_tool->trans_info[X1] = (gdouble) tr_tool->x2;
  tg_tool->trans_info[Y1] = (gdouble) tr_tool->y1;
  tg_tool->trans_info[X2] = (gdouble) tr_tool->x1;
  tg_tool->trans_info[Y2] = (gdouble) tr_tool->y2;
  tg_tool->trans_info[X3] = (gdouble) tr_tool->x2;
  tg_tool->trans_info[Y3] = (gdouble) tr_tool->y2;
}

static void
ligma_perspective_tool_readjust (LigmaTransformGridTool *tg_tool)
{
  LigmaTool         *tool  = LIGMA_TOOL (tg_tool);
  LigmaDisplayShell *shell = ligma_display_get_shell (tool->display);
  gdouble           x;
  gdouble           y;
  gdouble           r;

  x = shell->disp_width  / 2.0;
  y = shell->disp_height / 2.0;
  r = MAX (MIN (x, y) / G_SQRT2 -
           LIGMA_TOOL_TRANSFORM_GRID_MAX_HANDLE_SIZE / 2.0,
           LIGMA_TOOL_TRANSFORM_GRID_MAX_HANDLE_SIZE / 2.0);

  ligma_display_shell_untransform_xy_f (shell,
                                       x - r, y - r,
                                       &tg_tool->trans_info[X0],
                                       &tg_tool->trans_info[Y0]);
  ligma_display_shell_untransform_xy_f (shell,
                                       x + r, y - r,
                                       &tg_tool->trans_info[X1],
                                       &tg_tool->trans_info[Y1]);
  ligma_display_shell_untransform_xy_f (shell,
                                       x - r, y + r,
                                       &tg_tool->trans_info[X2],
                                       &tg_tool->trans_info[Y2]);
  ligma_display_shell_untransform_xy_f (shell,
                                       x + r, y + r,
                                       &tg_tool->trans_info[X3],
                                       &tg_tool->trans_info[Y3]);
}

static LigmaToolWidget *
ligma_perspective_tool_get_widget (LigmaTransformGridTool *tg_tool)
{
  LigmaTool          *tool    = LIGMA_TOOL (tg_tool);
  LigmaTransformTool *tr_tool = LIGMA_TRANSFORM_TOOL (tg_tool);
  LigmaDisplayShell  *shell   = ligma_display_get_shell (tool->display);
  LigmaToolWidget    *widget;

  widget = ligma_tool_transform_grid_new (shell,
                                         &tr_tool->transform,
                                         tr_tool->x1,
                                         tr_tool->y1,
                                         tr_tool->x2,
                                         tr_tool->y2);

  g_object_set (widget,
                "inside-function",         LIGMA_TRANSFORM_FUNCTION_PERSPECTIVE,
                "outside-function",        LIGMA_TRANSFORM_FUNCTION_PERSPECTIVE,
                "use-perspective-handles", TRUE,
                "use-center-handle",       TRUE,
                NULL);

  return widget;
}

static void
ligma_perspective_tool_update_widget (LigmaTransformGridTool *tg_tool)
{
  LigmaTransformTool *tr_tool = LIGMA_TRANSFORM_TOOL (tg_tool);

  LIGMA_TRANSFORM_GRID_TOOL_CLASS (parent_class)->update_widget (tg_tool);

  g_object_set (tg_tool->widget,
                "x1", (gdouble) tr_tool->x1,
                "y1", (gdouble) tr_tool->y1,
                "x2", (gdouble) tr_tool->x2,
                "y2", (gdouble) tr_tool->y2,
                NULL);
}

static void
ligma_perspective_tool_widget_changed (LigmaTransformGridTool *tg_tool)
{
  LigmaTransformTool *tr_tool = LIGMA_TRANSFORM_TOOL (tg_tool);
  LigmaMatrix3       *transform;

  g_object_get (tg_tool->widget,
                "transform", &transform,
                NULL);

  ligma_matrix3_transform_point (transform,
                                tr_tool->x1, tr_tool->y1,
                                &tg_tool->trans_info[X0],
                                &tg_tool->trans_info[Y0]);
  ligma_matrix3_transform_point (transform,
                                tr_tool->x2, tr_tool->y1,
                                &tg_tool->trans_info[X1],
                                &tg_tool->trans_info[Y1]);
  ligma_matrix3_transform_point (transform,
                                tr_tool->x1, tr_tool->y2,
                                &tg_tool->trans_info[X2],
                                &tg_tool->trans_info[Y2]);
  ligma_matrix3_transform_point (transform,
                                tr_tool->x2, tr_tool->y2,
                                &tg_tool->trans_info[X3],
                                &tg_tool->trans_info[Y3]);

  g_free (transform);

  LIGMA_TRANSFORM_GRID_TOOL_CLASS (parent_class)->widget_changed (tg_tool);
}

static void
ligma_perspective_tool_info_to_points (LigmaGenericTransformTool *generic)
{
  LigmaTransformTool     *tr_tool = LIGMA_TRANSFORM_TOOL (generic);
  LigmaTransformGridTool *tg_tool = LIGMA_TRANSFORM_GRID_TOOL (generic);

  generic->input_points[0]  = (LigmaVector2) {tr_tool->x1, tr_tool->y1};
  generic->input_points[1]  = (LigmaVector2) {tr_tool->x2, tr_tool->y1};
  generic->input_points[2]  = (LigmaVector2) {tr_tool->x1, tr_tool->y2};
  generic->input_points[3]  = (LigmaVector2) {tr_tool->x2, tr_tool->y2};

  generic->output_points[0] = (LigmaVector2) {tg_tool->trans_info[X0],
                                             tg_tool->trans_info[Y0]};
  generic->output_points[1] = (LigmaVector2) {tg_tool->trans_info[X1],
                                             tg_tool->trans_info[Y1]};
  generic->output_points[2] = (LigmaVector2) {tg_tool->trans_info[X2],
                                             tg_tool->trans_info[Y2]};
  generic->output_points[3] = (LigmaVector2) {tg_tool->trans_info[X3],
                                             tg_tool->trans_info[Y3]};
}
