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
#include <gdk/gdkkeysyms.h>

#include "libligmamath/ligmamath.h"
#include "libligmawidgets/ligmawidgets.h"

#include "tools-types.h"

#include "core/ligma-transform-utils.h"
#include "core/ligmaimage.h"

#include "widgets/ligmahelp-ids.h"
#include "widgets/ligmapivotselector.h"

#include "display/ligmadisplay.h"
#include "display/ligmadisplayshell.h"
#include "display/ligmadisplayshell-transform.h"
#include "display/ligmatoolgui.h"
#include "display/ligmatoolrotategrid.h"

#include "ligmarotatetool.h"
#include "ligmatoolcontrol.h"
#include "ligmatransformgridoptions.h"

#include "ligma-intl.h"


/*  index into trans_info array  */
enum
{
  ANGLE,
  PIVOT_X,
  PIVOT_Y
};


#define SB_WIDTH 8
#define EPSILON  1e-6


/*  local function prototypes  */

static gboolean         ligma_rotate_tool_key_press      (LigmaTool              *tool,
                                                         GdkEventKey           *kevent,
                                                         LigmaDisplay           *display);

static gboolean         ligma_rotate_tool_info_to_matrix (LigmaTransformGridTool *tg_tool,
                                                         LigmaMatrix3           *transform);
static void             ligma_rotate_tool_matrix_to_info (LigmaTransformGridTool *tg_tool,
                                                         const LigmaMatrix3     *transform);
static gchar          * ligma_rotate_tool_get_undo_desc  (LigmaTransformGridTool *tg_tool);
static void             ligma_rotate_tool_dialog         (LigmaTransformGridTool *tg_tool);
static void             ligma_rotate_tool_dialog_update  (LigmaTransformGridTool *tg_tool);
static void             ligma_rotate_tool_prepare        (LigmaTransformGridTool *tg_tool);
static void             ligma_rotate_tool_readjust       (LigmaTransformGridTool *tg_tool);
static LigmaToolWidget * ligma_rotate_tool_get_widget     (LigmaTransformGridTool *tg_tool);
static void             ligma_rotate_tool_update_widget  (LigmaTransformGridTool *tg_tool);
static void             ligma_rotate_tool_widget_changed (LigmaTransformGridTool *tg_tool);

static void             rotate_angle_changed            (GtkAdjustment         *adj,
                                                         LigmaTransformGridTool *tg_tool);
static void             rotate_center_changed           (GtkWidget             *entry,
                                                         LigmaTransformGridTool *tg_tool);
static void             rotate_pivot_changed            (LigmaPivotSelector     *selector,
                                                         LigmaTransformGridTool *tg_tool);


G_DEFINE_TYPE (LigmaRotateTool, ligma_rotate_tool, LIGMA_TYPE_TRANSFORM_GRID_TOOL)

#define parent_class ligma_rotate_tool_parent_class


void
ligma_rotate_tool_register (LigmaToolRegisterCallback  callback,
                           gpointer                  data)
{
  (* callback) (LIGMA_TYPE_ROTATE_TOOL,
                LIGMA_TYPE_TRANSFORM_GRID_OPTIONS,
                ligma_transform_grid_options_gui,
                LIGMA_CONTEXT_PROP_MASK_BACKGROUND,
                "ligma-rotate-tool",
                _("Rotate"),
                _("Rotate Tool: Rotate the layer, selection or path"),
                N_("_Rotate"), "<shift>R",
                NULL, LIGMA_HELP_TOOL_ROTATE,
                LIGMA_ICON_TOOL_ROTATE,
                data);
}

static void
ligma_rotate_tool_class_init (LigmaRotateToolClass *klass)
{
  LigmaToolClass              *tool_class = LIGMA_TOOL_CLASS (klass);
  LigmaTransformToolClass     *tr_class   = LIGMA_TRANSFORM_TOOL_CLASS (klass);
  LigmaTransformGridToolClass *tg_class   = LIGMA_TRANSFORM_GRID_TOOL_CLASS (klass);

  tool_class->key_press     = ligma_rotate_tool_key_press;

  tg_class->info_to_matrix  = ligma_rotate_tool_info_to_matrix;
  tg_class->matrix_to_info  = ligma_rotate_tool_matrix_to_info;
  tg_class->get_undo_desc   = ligma_rotate_tool_get_undo_desc;
  tg_class->dialog          = ligma_rotate_tool_dialog;
  tg_class->dialog_update   = ligma_rotate_tool_dialog_update;
  tg_class->prepare         = ligma_rotate_tool_prepare;
  tg_class->readjust        = ligma_rotate_tool_readjust;
  tg_class->get_widget      = ligma_rotate_tool_get_widget;
  tg_class->update_widget   = ligma_rotate_tool_update_widget;
  tg_class->widget_changed  = ligma_rotate_tool_widget_changed;

  tr_class->undo_desc       = C_("undo-type", "Rotate");
  tr_class->progress_text   = _("Rotating");
  tg_class->ok_button_label = _("R_otate");
}

static void
ligma_rotate_tool_init (LigmaRotateTool *rotate_tool)
{
  LigmaTool *tool = LIGMA_TOOL (rotate_tool);

  ligma_tool_control_set_tool_cursor (tool->control, LIGMA_TOOL_CURSOR_ROTATE);
}

static gboolean
ligma_rotate_tool_key_press (LigmaTool    *tool,
                            GdkEventKey *kevent,
                            LigmaDisplay *display)
{
  LigmaDrawTool *draw_tool = LIGMA_DRAW_TOOL (tool);

  if (display == draw_tool->display)
    {
      LigmaRotateTool *rotate     = LIGMA_ROTATE_TOOL (tool);
      GtkSpinButton  *angle_spin = GTK_SPIN_BUTTON (rotate->angle_spin_button);

      switch (kevent->keyval)
        {
        case GDK_KEY_Up:
          gtk_spin_button_spin (angle_spin, GTK_SPIN_STEP_FORWARD, 0.0);
          return TRUE;

        case GDK_KEY_Down:
          gtk_spin_button_spin (angle_spin, GTK_SPIN_STEP_BACKWARD, 0.0);
          return TRUE;

        case GDK_KEY_Right:
          gtk_spin_button_spin (angle_spin, GTK_SPIN_PAGE_FORWARD, 0.0);
          return TRUE;

        case GDK_KEY_Left:
          gtk_spin_button_spin (angle_spin, GTK_SPIN_PAGE_BACKWARD, 0.0);
          return TRUE;

        default:
          break;
        }
    }

  return LIGMA_TOOL_CLASS (parent_class)->key_press (tool, kevent, display);
}

static gboolean
ligma_rotate_tool_info_to_matrix (LigmaTransformGridTool *tg_tool,
                                 LigmaMatrix3           *transform)
{
  ligma_matrix3_identity (transform);
  ligma_transform_matrix_rotate_center (transform,
                                       tg_tool->trans_info[PIVOT_X],
                                       tg_tool->trans_info[PIVOT_Y],
                                       tg_tool->trans_info[ANGLE]);

  return TRUE;
}

static void
ligma_rotate_tool_matrix_to_info (LigmaTransformGridTool *tg_tool,
                                 const LigmaMatrix3     *transform)
{
  gdouble c;
  gdouble s;
  gdouble x;
  gdouble y;
  gdouble q;

  c = transform->coeff[0][0];
  s = transform->coeff[1][0];
  x = transform->coeff[0][2];
  y = transform->coeff[1][2];

  tg_tool->trans_info[ANGLE] = atan2 (s, c);

  q = 2.0 * (1.0 - transform->coeff[0][0]);

  if (fabs (q) > EPSILON)
    {
      tg_tool->trans_info[PIVOT_X] = ((1.0 - c) * x - s * y) / q;
      tg_tool->trans_info[PIVOT_Y] = (s * x + (1.0 - c) * y) / q;
    }
  else
    {
      LigmaMatrix3 transfer;

      ligma_transform_grid_tool_info_to_matrix (tg_tool, &transfer);
      ligma_matrix3_invert (&transfer);
      ligma_matrix3_mult (transform, &transfer);

      ligma_matrix3_transform_point (&transfer,
                                    tg_tool->trans_info[PIVOT_X],
                                    tg_tool->trans_info[PIVOT_Y],
                                    &tg_tool->trans_info[PIVOT_X],
                                    &tg_tool->trans_info[PIVOT_Y]);
    }
}

static gchar *
ligma_rotate_tool_get_undo_desc (LigmaTransformGridTool *tg_tool)
{
  LigmaTransformTool *tr_tool = LIGMA_TRANSFORM_TOOL (tg_tool);
  gdouble            center_x;
  gdouble            center_y;

  center_x = (tr_tool->x1 + tr_tool->x2) / 2.0;
  center_y = (tr_tool->y1 + tr_tool->y2) / 2.0;

  if (fabs (tg_tool->trans_info[PIVOT_X] - center_x) <= EPSILON &&
      fabs (tg_tool->trans_info[PIVOT_Y] - center_y) <= EPSILON)
    {
      return g_strdup_printf (C_("undo-type",
                                 "Rotate by %-3.3g°"),
                              ligma_rad_to_deg (tg_tool->trans_info[ANGLE]));
    }
  else
    {
      return g_strdup_printf (C_("undo-type",
                                 "Rotate by %-3.3g° around (%g, %g)"),
                              ligma_rad_to_deg (tg_tool->trans_info[ANGLE]),
                              tg_tool->trans_info[PIVOT_X],
                              tg_tool->trans_info[PIVOT_Y]);
    }
}

static void
ligma_rotate_tool_dialog (LigmaTransformGridTool *tg_tool)
{
  LigmaRotateTool *rotate = LIGMA_ROTATE_TOOL (tg_tool);
  GtkWidget      *grid;
  GtkWidget      *button;
  GtkWidget      *scale;
  GtkAdjustment  *adj;

  grid = gtk_grid_new ();
  gtk_grid_set_row_spacing (GTK_GRID (grid), 2);
  gtk_grid_set_column_spacing (GTK_GRID (grid), 6);
  gtk_box_pack_start (GTK_BOX (ligma_tool_gui_get_vbox (tg_tool->gui)), grid,
                      FALSE, FALSE, 0);
  gtk_widget_show (grid);

  rotate->angle_adj = gtk_adjustment_new (0, -180, 180, 0.1, 15, 0);
  button = ligma_spin_button_new (rotate->angle_adj, 1.0, 2);
  gtk_spin_button_set_numeric (GTK_SPIN_BUTTON (button), TRUE);
  gtk_spin_button_set_wrap (GTK_SPIN_BUTTON (button), TRUE);
  gtk_entry_set_width_chars (GTK_ENTRY (button), SB_WIDTH);
  gtk_widget_set_halign (button, GTK_ALIGN_START);
  ligma_grid_attach_aligned (GTK_GRID (grid), 0, 0, _("_Angle:"),
                             0.0, 0.5, button, 1);
  rotate->angle_spin_button = button;

  g_signal_connect (rotate->angle_adj, "value-changed",
                    G_CALLBACK (rotate_angle_changed),
                    tg_tool);

  scale = gtk_scale_new (GTK_ORIENTATION_HORIZONTAL, rotate->angle_adj);
  gtk_scale_set_draw_value (GTK_SCALE (scale), FALSE);
  gtk_widget_set_hexpand (scale, TRUE);
  gtk_grid_attach (GTK_GRID (grid), scale, 1, 1, 2, 1);
  gtk_widget_show (scale);

  adj = gtk_adjustment_new (0, -1, 1, 1, 10, 0);
  button = ligma_spin_button_new (adj, 1.0, 2);
  gtk_spin_button_set_numeric (GTK_SPIN_BUTTON (button), TRUE);
  gtk_entry_set_width_chars (GTK_ENTRY (button), SB_WIDTH);
  gtk_widget_set_halign (button, GTK_ALIGN_START);
  ligma_grid_attach_aligned (GTK_GRID (grid), 0, 2, _("Center _X:"),
                            0.0, 0.5, button, 1);

  rotate->sizeentry = ligma_size_entry_new (1, LIGMA_UNIT_PIXEL, "%a",
                                           TRUE, TRUE, FALSE, SB_WIDTH,
                                           LIGMA_SIZE_ENTRY_UPDATE_SIZE);
  ligma_size_entry_add_field (LIGMA_SIZE_ENTRY (rotate->sizeentry),
                             GTK_SPIN_BUTTON (button), NULL);
  ligma_size_entry_set_pixel_digits (LIGMA_SIZE_ENTRY (rotate->sizeentry), 2);
  gtk_widget_set_halign (rotate->sizeentry, GTK_ALIGN_START);
  ligma_grid_attach_aligned (GTK_GRID (grid), 0, 3, _("Center _Y:"),
                            0.0, 0.5, rotate->sizeentry, 1);

  g_signal_connect (rotate->sizeentry, "value-changed",
                    G_CALLBACK (rotate_center_changed),
                    tg_tool);

  rotate->pivot_selector = ligma_pivot_selector_new (0.0, 0.0, 0.0, 0.0);
  gtk_grid_attach (GTK_GRID (grid), rotate->pivot_selector, 2, 2, 1, 2);
  gtk_widget_show (rotate->pivot_selector);

  g_signal_connect (rotate->pivot_selector, "changed",
                    G_CALLBACK (rotate_pivot_changed),
                    tg_tool);
}

static void
ligma_rotate_tool_dialog_update (LigmaTransformGridTool *tg_tool)
{
  LigmaRotateTool *rotate = LIGMA_ROTATE_TOOL (tg_tool);

  gtk_adjustment_set_value (rotate->angle_adj,
                            ligma_rad_to_deg (tg_tool->trans_info[ANGLE]));

  g_signal_handlers_block_by_func (rotate->sizeentry,
                                   rotate_center_changed,
                                   tg_tool);

  ligma_size_entry_set_refval (LIGMA_SIZE_ENTRY (rotate->sizeentry), 0,
                              tg_tool->trans_info[PIVOT_X]);
  ligma_size_entry_set_refval (LIGMA_SIZE_ENTRY (rotate->sizeentry), 1,
                              tg_tool->trans_info[PIVOT_Y]);

  g_signal_handlers_unblock_by_func (rotate->sizeentry,
                                     rotate_center_changed,
                                     tg_tool);

  g_signal_handlers_block_by_func (rotate->pivot_selector,
                                   rotate_pivot_changed,
                                   tg_tool);

  ligma_pivot_selector_set_position (
    LIGMA_PIVOT_SELECTOR (rotate->pivot_selector),
    tg_tool->trans_info[PIVOT_X],
    tg_tool->trans_info[PIVOT_Y]);

  g_signal_handlers_unblock_by_func (rotate->pivot_selector,
                                     rotate_pivot_changed,
                                     tg_tool);
}

static void
ligma_rotate_tool_prepare (LigmaTransformGridTool *tg_tool)
{
  LigmaTransformTool *tr_tool = LIGMA_TRANSFORM_TOOL (tg_tool);
  LigmaRotateTool    *rotate  = LIGMA_ROTATE_TOOL (tg_tool);
  LigmaDisplay       *display = LIGMA_TOOL (tg_tool)->display;
  LigmaImage         *image   = ligma_display_get_image (display);
  gdouble            xres;
  gdouble            yres;

  tg_tool->trans_info[ANGLE]   = 0.0;
  tg_tool->trans_info[PIVOT_X] = (gdouble) (tr_tool->x1 + tr_tool->x2) / 2.0;
  tg_tool->trans_info[PIVOT_Y] = (gdouble) (tr_tool->y1 + tr_tool->y2) / 2.0;

  ligma_image_get_resolution (image, &xres, &yres);

  g_signal_handlers_block_by_func (rotate->sizeentry,
                                   rotate_center_changed,
                                   tg_tool);

  ligma_size_entry_set_unit (LIGMA_SIZE_ENTRY (rotate->sizeentry),
                            ligma_display_get_shell (display)->unit);

  ligma_size_entry_set_resolution (LIGMA_SIZE_ENTRY (rotate->sizeentry), 0,
                                  xres, FALSE);
  ligma_size_entry_set_resolution (LIGMA_SIZE_ENTRY (rotate->sizeentry), 1,
                                  yres, FALSE);

  ligma_size_entry_set_refval_boundaries (LIGMA_SIZE_ENTRY (rotate->sizeentry), 0,
                                         -65536,
                                         65536 +
                                         ligma_image_get_width (image));
  ligma_size_entry_set_refval_boundaries (LIGMA_SIZE_ENTRY (rotate->sizeentry), 1,
                                         -65536,
                                         65536 +
                                         ligma_image_get_height (image));

  ligma_size_entry_set_size (LIGMA_SIZE_ENTRY (rotate->sizeentry), 0,
                            tr_tool->x1, tr_tool->x2);
  ligma_size_entry_set_size (LIGMA_SIZE_ENTRY (rotate->sizeentry), 1,
                            tr_tool->y1, tr_tool->y2);

  g_signal_handlers_unblock_by_func (rotate->sizeentry,
                                     rotate_center_changed,
                                     tg_tool);

  ligma_pivot_selector_set_bounds (LIGMA_PIVOT_SELECTOR (rotate->pivot_selector),
                                  tr_tool->x1, tr_tool->y1,
                                  tr_tool->x2, tr_tool->y2);
}

static void
ligma_rotate_tool_readjust (LigmaTransformGridTool *tg_tool)
{
  LigmaTool         *tool  = LIGMA_TOOL (tg_tool);
  LigmaDisplayShell *shell = ligma_display_get_shell (tool->display);

  tg_tool->trans_info[ANGLE] = -ligma_deg_to_rad (shell->rotate_angle);

  if (tg_tool->trans_info[ANGLE] <= -G_PI)
    tg_tool->trans_info[ANGLE] += 2.0 * G_PI;

  ligma_display_shell_untransform_xy_f (shell,
                                       shell->disp_width  / 2.0,
                                       shell->disp_height / 2.0,
                                       &tg_tool->trans_info[PIVOT_X],
                                       &tg_tool->trans_info[PIVOT_Y]);
}

static LigmaToolWidget *
ligma_rotate_tool_get_widget (LigmaTransformGridTool *tg_tool)
{
  LigmaTool          *tool    = LIGMA_TOOL (tg_tool);
  LigmaTransformTool *tr_tool = LIGMA_TRANSFORM_TOOL (tg_tool);
  LigmaDisplayShell  *shell   = ligma_display_get_shell (tool->display);
  LigmaToolWidget    *widget;

  widget = ligma_tool_rotate_grid_new (shell,
                                      tr_tool->x1,
                                      tr_tool->y1,
                                      tr_tool->x2,
                                      tr_tool->y2,
                                      tg_tool->trans_info[PIVOT_X],
                                      tg_tool->trans_info[PIVOT_Y],
                                      tg_tool->trans_info[ANGLE]);

  g_object_set (widget,
                "inside-function",  LIGMA_TRANSFORM_FUNCTION_ROTATE,
                "outside-function", LIGMA_TRANSFORM_FUNCTION_ROTATE,
                "use-pivot-handle", TRUE,
                NULL);

  return widget;
}

static void
ligma_rotate_tool_update_widget (LigmaTransformGridTool *tg_tool)
{
  LIGMA_TRANSFORM_GRID_TOOL_CLASS (parent_class)->update_widget (tg_tool);

  g_object_set (tg_tool->widget,
                "angle",   tg_tool->trans_info[ANGLE],
                "pivot-x", tg_tool->trans_info[PIVOT_X],
                "pivot-y", tg_tool->trans_info[PIVOT_Y],
                NULL);
}

static void
ligma_rotate_tool_widget_changed (LigmaTransformGridTool *tg_tool)
{
  g_object_get (tg_tool->widget,
                "angle",   &tg_tool->trans_info[ANGLE],
                "pivot-x", &tg_tool->trans_info[PIVOT_X],
                "pivot-y", &tg_tool->trans_info[PIVOT_Y],
                NULL);

  LIGMA_TRANSFORM_GRID_TOOL_CLASS (parent_class)->widget_changed (tg_tool);
}

static void
rotate_angle_changed (GtkAdjustment         *adj,
                      LigmaTransformGridTool *tg_tool)
{
  gdouble value = ligma_deg_to_rad (gtk_adjustment_get_value (adj));

  if (fabs (value - tg_tool->trans_info[ANGLE]) > EPSILON)
    {
      LigmaTool          *tool    = LIGMA_TOOL (tg_tool);
      LigmaTransformTool *tr_tool = LIGMA_TRANSFORM_TOOL (tg_tool);

      tg_tool->trans_info[ANGLE] = value;

      ligma_transform_grid_tool_push_internal_undo (tg_tool, TRUE);

      ligma_transform_tool_recalc_matrix (tr_tool, tool->display);
    }
}

static void
rotate_center_changed (GtkWidget             *widget,
                       LigmaTransformGridTool *tg_tool)
{
  gdouble px = ligma_size_entry_get_refval (LIGMA_SIZE_ENTRY (widget), 0);
  gdouble py = ligma_size_entry_get_refval (LIGMA_SIZE_ENTRY (widget), 1);

  if ((px != tg_tool->trans_info[PIVOT_X]) ||
      (py != tg_tool->trans_info[PIVOT_Y]))
    {
      LigmaTool          *tool    = LIGMA_TOOL (tg_tool);
      LigmaTransformTool *tr_tool = LIGMA_TRANSFORM_TOOL (tg_tool);

      tg_tool->trans_info[PIVOT_X] = px;
      tg_tool->trans_info[PIVOT_Y] = py;

      ligma_transform_grid_tool_push_internal_undo (tg_tool, TRUE);

      ligma_transform_tool_recalc_matrix (tr_tool, tool->display);
    }
}

static void
rotate_pivot_changed (LigmaPivotSelector     *selector,
                      LigmaTransformGridTool *tg_tool)
{
  LigmaTool          *tool    = LIGMA_TOOL (tg_tool);
  LigmaTransformTool *tr_tool = LIGMA_TRANSFORM_TOOL (tg_tool);

  ligma_pivot_selector_get_position (selector,
                                    &tg_tool->trans_info[PIVOT_X],
                                    &tg_tool->trans_info[PIVOT_Y]);

  ligma_transform_grid_tool_push_internal_undo (tg_tool, TRUE);

  ligma_transform_tool_recalc_matrix (tr_tool, tool->display);
}
