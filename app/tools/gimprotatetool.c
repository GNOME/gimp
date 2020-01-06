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
#include <gdk/gdkkeysyms.h>

#include "libgimpmath/gimpmath.h"
#include "libgimpwidgets/gimpwidgets.h"

#include "tools-types.h"

#include "core/gimp-transform-utils.h"
#include "core/gimpimage.h"

#include "widgets/gimphelp-ids.h"
#include "widgets/gimppivotselector.h"

#include "display/gimpdisplay.h"
#include "display/gimpdisplayshell.h"
#include "display/gimpdisplayshell-transform.h"
#include "display/gimptoolgui.h"
#include "display/gimptoolrotategrid.h"

#include "gimprotatetool.h"
#include "gimptoolcontrol.h"
#include "gimptransformgridoptions.h"

#include "gimp-intl.h"


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

static gboolean         gimp_rotate_tool_key_press      (GimpTool              *tool,
                                                         GdkEventKey           *kevent,
                                                         GimpDisplay           *display);

static gboolean         gimp_rotate_tool_info_to_matrix (GimpTransformGridTool *tg_tool,
                                                         GimpMatrix3           *transform);
static void             gimp_rotate_tool_matrix_to_info (GimpTransformGridTool *tg_tool,
                                                         const GimpMatrix3     *transform);
static gchar          * gimp_rotate_tool_get_undo_desc  (GimpTransformGridTool *tg_tool);
static void             gimp_rotate_tool_dialog         (GimpTransformGridTool *tg_tool);
static void             gimp_rotate_tool_dialog_update  (GimpTransformGridTool *tg_tool);
static void             gimp_rotate_tool_prepare        (GimpTransformGridTool *tg_tool);
static void             gimp_rotate_tool_readjust       (GimpTransformGridTool *tg_tool);
static GimpToolWidget * gimp_rotate_tool_get_widget     (GimpTransformGridTool *tg_tool);
static void             gimp_rotate_tool_update_widget  (GimpTransformGridTool *tg_tool);
static void             gimp_rotate_tool_widget_changed (GimpTransformGridTool *tg_tool);

static void             rotate_angle_changed            (GtkAdjustment         *adj,
                                                         GimpTransformGridTool *tg_tool);
static void             rotate_center_changed           (GtkWidget             *entry,
                                                         GimpTransformGridTool *tg_tool);
static void             rotate_pivot_changed            (GimpPivotSelector     *selector,
                                                         GimpTransformGridTool *tg_tool);


G_DEFINE_TYPE (GimpRotateTool, gimp_rotate_tool, GIMP_TYPE_TRANSFORM_GRID_TOOL)

#define parent_class gimp_rotate_tool_parent_class


void
gimp_rotate_tool_register (GimpToolRegisterCallback  callback,
                           gpointer                  data)
{
  (* callback) (GIMP_TYPE_ROTATE_TOOL,
                GIMP_TYPE_TRANSFORM_GRID_OPTIONS,
                gimp_transform_grid_options_gui,
                GIMP_CONTEXT_PROP_MASK_BACKGROUND,
                "gimp-rotate-tool",
                _("Rotate"),
                _("Rotate Tool: Rotate the layer, selection or path"),
                N_("_Rotate"), "<shift>R",
                NULL, GIMP_HELP_TOOL_ROTATE,
                GIMP_ICON_TOOL_ROTATE,
                data);
}

static void
gimp_rotate_tool_class_init (GimpRotateToolClass *klass)
{
  GimpToolClass              *tool_class = GIMP_TOOL_CLASS (klass);
  GimpTransformToolClass     *tr_class   = GIMP_TRANSFORM_TOOL_CLASS (klass);
  GimpTransformGridToolClass *tg_class   = GIMP_TRANSFORM_GRID_TOOL_CLASS (klass);

  tool_class->key_press     = gimp_rotate_tool_key_press;

  tg_class->info_to_matrix  = gimp_rotate_tool_info_to_matrix;
  tg_class->matrix_to_info  = gimp_rotate_tool_matrix_to_info;
  tg_class->get_undo_desc   = gimp_rotate_tool_get_undo_desc;
  tg_class->dialog          = gimp_rotate_tool_dialog;
  tg_class->dialog_update   = gimp_rotate_tool_dialog_update;
  tg_class->prepare         = gimp_rotate_tool_prepare;
  tg_class->readjust        = gimp_rotate_tool_readjust;
  tg_class->get_widget      = gimp_rotate_tool_get_widget;
  tg_class->update_widget   = gimp_rotate_tool_update_widget;
  tg_class->widget_changed  = gimp_rotate_tool_widget_changed;

  tr_class->undo_desc       = C_("undo-type", "Rotate");
  tr_class->progress_text   = _("Rotating");
  tg_class->ok_button_label = _("R_otate");
}

static void
gimp_rotate_tool_init (GimpRotateTool *rotate_tool)
{
  GimpTool *tool = GIMP_TOOL (rotate_tool);

  gimp_tool_control_set_tool_cursor (tool->control, GIMP_TOOL_CURSOR_ROTATE);
}

static gboolean
gimp_rotate_tool_key_press (GimpTool    *tool,
                            GdkEventKey *kevent,
                            GimpDisplay *display)
{
  GimpDrawTool *draw_tool = GIMP_DRAW_TOOL (tool);

  if (display == draw_tool->display)
    {
      GimpRotateTool *rotate     = GIMP_ROTATE_TOOL (tool);
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

  return GIMP_TOOL_CLASS (parent_class)->key_press (tool, kevent, display);
}

static gboolean
gimp_rotate_tool_info_to_matrix (GimpTransformGridTool *tg_tool,
                                 GimpMatrix3           *transform)
{
  gimp_matrix3_identity (transform);
  gimp_transform_matrix_rotate_center (transform,
                                       tg_tool->trans_info[PIVOT_X],
                                       tg_tool->trans_info[PIVOT_Y],
                                       tg_tool->trans_info[ANGLE]);

  return TRUE;
}

static void
gimp_rotate_tool_matrix_to_info (GimpTransformGridTool *tg_tool,
                                 const GimpMatrix3     *transform)
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
      GimpMatrix3 transfer;

      gimp_transform_grid_tool_info_to_matrix (tg_tool, &transfer);
      gimp_matrix3_invert (&transfer);
      gimp_matrix3_mult (transform, &transfer);

      gimp_matrix3_transform_point (&transfer,
                                    tg_tool->trans_info[PIVOT_X],
                                    tg_tool->trans_info[PIVOT_Y],
                                    &tg_tool->trans_info[PIVOT_X],
                                    &tg_tool->trans_info[PIVOT_Y]);
    }
}

static gchar *
gimp_rotate_tool_get_undo_desc (GimpTransformGridTool *tg_tool)
{
  GimpTransformTool *tr_tool = GIMP_TRANSFORM_TOOL (tg_tool);
  gdouble            center_x;
  gdouble            center_y;

  center_x = (tr_tool->x1 + tr_tool->x2) / 2.0;
  center_y = (tr_tool->y1 + tr_tool->y2) / 2.0;

  if (fabs (tg_tool->trans_info[PIVOT_X] - center_x) <= EPSILON &&
      fabs (tg_tool->trans_info[PIVOT_Y] - center_y) <= EPSILON)
    {
      return g_strdup_printf (C_("undo-type",
                                 "Rotate by %-3.3g°"),
                              gimp_rad_to_deg (tg_tool->trans_info[ANGLE]));
    }
  else
    {
      return g_strdup_printf (C_("undo-type",
                                 "Rotate by %-3.3g° around (%g, %g)"),
                              gimp_rad_to_deg (tg_tool->trans_info[ANGLE]),
                              tg_tool->trans_info[PIVOT_X],
                              tg_tool->trans_info[PIVOT_Y]);
    }
}

static void
gimp_rotate_tool_dialog (GimpTransformGridTool *tg_tool)
{
  GimpRotateTool *rotate = GIMP_ROTATE_TOOL (tg_tool);
  GtkWidget      *table;
  GtkWidget      *button;
  GtkWidget      *scale;
  GtkAdjustment  *adj;

  table = gtk_table_new (4, 3, FALSE);
  gtk_table_set_row_spacings (GTK_TABLE (table), 2);
  gtk_table_set_col_spacings (GTK_TABLE (table), 6);
  gtk_table_set_row_spacing (GTK_TABLE (table), 1, 6);
  gtk_box_pack_start (GTK_BOX (gimp_tool_gui_get_vbox (tg_tool->gui)), table,
                      FALSE, FALSE, 0);
  gtk_widget_show (table);

  rotate->angle_adj = (GtkAdjustment *)
    gtk_adjustment_new (0, -180, 180, 0.1, 15, 0);
  button = gimp_spin_button_new (rotate->angle_adj, 1.0, 2);
  gtk_spin_button_set_numeric (GTK_SPIN_BUTTON (button), TRUE);
  gtk_spin_button_set_wrap (GTK_SPIN_BUTTON (button), TRUE);
  gtk_entry_set_width_chars (GTK_ENTRY (button), SB_WIDTH);
  gimp_table_attach_aligned (GTK_TABLE (table), 0, 0, _("_Angle:"),
                             0.0, 0.5, button, 1, TRUE);
  rotate->angle_spin_button = button;

  g_signal_connect (rotate->angle_adj, "value-changed",
                    G_CALLBACK (rotate_angle_changed),
                    tg_tool);

  scale = gtk_scale_new (GTK_ORIENTATION_HORIZONTAL, rotate->angle_adj);
  gtk_scale_set_draw_value (GTK_SCALE (scale), FALSE);
  gtk_table_attach (GTK_TABLE (table), scale, 1, 3, 1, 2,
                    GTK_EXPAND | GTK_FILL, GTK_SHRINK, 0, 0);
  gtk_widget_show (scale);

  adj = (GtkAdjustment *) gtk_adjustment_new (0, -1, 1, 1, 10, 0);
  button = gimp_spin_button_new (adj, 1.0, 2);
  gtk_spin_button_set_numeric (GTK_SPIN_BUTTON (button), TRUE);
  gtk_entry_set_width_chars (GTK_ENTRY (button), SB_WIDTH);
  gimp_table_attach_aligned (GTK_TABLE (table), 0, 2, _("Center _X:"),
                             0.0, 0.5, button, 1, TRUE);

  rotate->sizeentry = gimp_size_entry_new (1, GIMP_UNIT_PIXEL, "%a",
                                           TRUE, TRUE, FALSE, SB_WIDTH,
                                           GIMP_SIZE_ENTRY_UPDATE_SIZE);
  gimp_size_entry_add_field (GIMP_SIZE_ENTRY (rotate->sizeentry),
                             GTK_SPIN_BUTTON (button), NULL);
  gimp_size_entry_set_pixel_digits (GIMP_SIZE_ENTRY (rotate->sizeentry), 2);
  gimp_table_attach_aligned (GTK_TABLE (table), 0, 3, _("Center _Y:"),
                             0.0, 0.5, rotate->sizeentry, 1, TRUE);

  g_signal_connect (rotate->sizeentry, "value-changed",
                    G_CALLBACK (rotate_center_changed),
                    tg_tool);

  rotate->pivot_selector = gimp_pivot_selector_new (0.0, 0.0, 0.0, 0.0);
  gtk_table_attach (GTK_TABLE (table), rotate->pivot_selector, 2, 3, 2, 4,
                    GTK_SHRINK, GTK_SHRINK, 0, 0);
  gtk_widget_show (rotate->pivot_selector);

  g_signal_connect (rotate->pivot_selector, "changed",
                    G_CALLBACK (rotate_pivot_changed),
                    tg_tool);
}

static void
gimp_rotate_tool_dialog_update (GimpTransformGridTool *tg_tool)
{
  GimpRotateTool *rotate = GIMP_ROTATE_TOOL (tg_tool);

  gtk_adjustment_set_value (rotate->angle_adj,
                            gimp_rad_to_deg (tg_tool->trans_info[ANGLE]));

  g_signal_handlers_block_by_func (rotate->sizeentry,
                                   rotate_center_changed,
                                   tg_tool);

  gimp_size_entry_set_refval (GIMP_SIZE_ENTRY (rotate->sizeentry), 0,
                              tg_tool->trans_info[PIVOT_X]);
  gimp_size_entry_set_refval (GIMP_SIZE_ENTRY (rotate->sizeentry), 1,
                              tg_tool->trans_info[PIVOT_Y]);

  g_signal_handlers_unblock_by_func (rotate->sizeentry,
                                     rotate_center_changed,
                                     tg_tool);

  g_signal_handlers_block_by_func (rotate->pivot_selector,
                                   rotate_pivot_changed,
                                   tg_tool);

  gimp_pivot_selector_set_position (
    GIMP_PIVOT_SELECTOR (rotate->pivot_selector),
    tg_tool->trans_info[PIVOT_X],
    tg_tool->trans_info[PIVOT_Y]);

  g_signal_handlers_unblock_by_func (rotate->pivot_selector,
                                     rotate_pivot_changed,
                                     tg_tool);
}

static void
gimp_rotate_tool_prepare (GimpTransformGridTool *tg_tool)
{
  GimpTransformTool *tr_tool = GIMP_TRANSFORM_TOOL (tg_tool);
  GimpRotateTool    *rotate  = GIMP_ROTATE_TOOL (tg_tool);
  GimpDisplay       *display = GIMP_TOOL (tg_tool)->display;
  GimpImage         *image   = gimp_display_get_image (display);
  gdouble            xres;
  gdouble            yres;

  tg_tool->trans_info[ANGLE]   = 0.0;
  tg_tool->trans_info[PIVOT_X] = (gdouble) (tr_tool->x1 + tr_tool->x2) / 2.0;
  tg_tool->trans_info[PIVOT_Y] = (gdouble) (tr_tool->y1 + tr_tool->y2) / 2.0;

  gimp_image_get_resolution (image, &xres, &yres);

  g_signal_handlers_block_by_func (rotate->sizeentry,
                                   rotate_center_changed,
                                   tg_tool);

  gimp_size_entry_set_unit (GIMP_SIZE_ENTRY (rotate->sizeentry),
                            gimp_display_get_shell (display)->unit);

  gimp_size_entry_set_resolution (GIMP_SIZE_ENTRY (rotate->sizeentry), 0,
                                  xres, FALSE);
  gimp_size_entry_set_resolution (GIMP_SIZE_ENTRY (rotate->sizeentry), 1,
                                  yres, FALSE);

  gimp_size_entry_set_refval_boundaries (GIMP_SIZE_ENTRY (rotate->sizeentry), 0,
                                         -65536,
                                         65536 +
                                         gimp_image_get_width (image));
  gimp_size_entry_set_refval_boundaries (GIMP_SIZE_ENTRY (rotate->sizeentry), 1,
                                         -65536,
                                         65536 +
                                         gimp_image_get_height (image));

  gimp_size_entry_set_size (GIMP_SIZE_ENTRY (rotate->sizeentry), 0,
                            tr_tool->x1, tr_tool->x2);
  gimp_size_entry_set_size (GIMP_SIZE_ENTRY (rotate->sizeentry), 1,
                            tr_tool->y1, tr_tool->y2);

  g_signal_handlers_unblock_by_func (rotate->sizeentry,
                                     rotate_center_changed,
                                     tg_tool);

  gimp_pivot_selector_set_bounds (GIMP_PIVOT_SELECTOR (rotate->pivot_selector),
                                  tr_tool->x1, tr_tool->y1,
                                  tr_tool->x2, tr_tool->y2);
}

static void
gimp_rotate_tool_readjust (GimpTransformGridTool *tg_tool)
{
  GimpTool         *tool  = GIMP_TOOL (tg_tool);
  GimpDisplayShell *shell = gimp_display_get_shell (tool->display);

  tg_tool->trans_info[ANGLE] = -gimp_deg_to_rad (shell->rotate_angle);

  if (tg_tool->trans_info[ANGLE] <= -G_PI)
    tg_tool->trans_info[ANGLE] += 2.0 * G_PI;

  gimp_display_shell_untransform_xy_f (shell,
                                       shell->disp_width  / 2.0,
                                       shell->disp_height / 2.0,
                                       &tg_tool->trans_info[PIVOT_X],
                                       &tg_tool->trans_info[PIVOT_Y]);
}

static GimpToolWidget *
gimp_rotate_tool_get_widget (GimpTransformGridTool *tg_tool)
{
  GimpTool          *tool    = GIMP_TOOL (tg_tool);
  GimpTransformTool *tr_tool = GIMP_TRANSFORM_TOOL (tg_tool);
  GimpDisplayShell  *shell   = gimp_display_get_shell (tool->display);
  GimpToolWidget    *widget;

  widget = gimp_tool_rotate_grid_new (shell,
                                      tr_tool->x1,
                                      tr_tool->y1,
                                      tr_tool->x2,
                                      tr_tool->y2,
                                      tg_tool->trans_info[PIVOT_X],
                                      tg_tool->trans_info[PIVOT_Y],
                                      tg_tool->trans_info[ANGLE]);

  g_object_set (widget,
                "inside-function",  GIMP_TRANSFORM_FUNCTION_ROTATE,
                "outside-function", GIMP_TRANSFORM_FUNCTION_ROTATE,
                "use-pivot-handle", TRUE,
                NULL);

  return widget;
}

static void
gimp_rotate_tool_update_widget (GimpTransformGridTool *tg_tool)
{
  GIMP_TRANSFORM_GRID_TOOL_CLASS (parent_class)->update_widget (tg_tool);

  g_object_set (tg_tool->widget,
                "angle",   tg_tool->trans_info[ANGLE],
                "pivot-x", tg_tool->trans_info[PIVOT_X],
                "pivot-y", tg_tool->trans_info[PIVOT_Y],
                NULL);
}

static void
gimp_rotate_tool_widget_changed (GimpTransformGridTool *tg_tool)
{
  g_object_get (tg_tool->widget,
                "angle",   &tg_tool->trans_info[ANGLE],
                "pivot-x", &tg_tool->trans_info[PIVOT_X],
                "pivot-y", &tg_tool->trans_info[PIVOT_Y],
                NULL);

  GIMP_TRANSFORM_GRID_TOOL_CLASS (parent_class)->widget_changed (tg_tool);
}

static void
rotate_angle_changed (GtkAdjustment         *adj,
                      GimpTransformGridTool *tg_tool)
{
  gdouble value = gimp_deg_to_rad (gtk_adjustment_get_value (adj));

  if (fabs (value - tg_tool->trans_info[ANGLE]) > EPSILON)
    {
      GimpTool          *tool    = GIMP_TOOL (tg_tool);
      GimpTransformTool *tr_tool = GIMP_TRANSFORM_TOOL (tg_tool);

      tg_tool->trans_info[ANGLE] = value;

      gimp_transform_grid_tool_push_internal_undo (tg_tool, TRUE);

      gimp_transform_tool_recalc_matrix (tr_tool, tool->display);
    }
}

static void
rotate_center_changed (GtkWidget             *widget,
                       GimpTransformGridTool *tg_tool)
{
  gdouble px = gimp_size_entry_get_refval (GIMP_SIZE_ENTRY (widget), 0);
  gdouble py = gimp_size_entry_get_refval (GIMP_SIZE_ENTRY (widget), 1);

  if ((px != tg_tool->trans_info[PIVOT_X]) ||
      (py != tg_tool->trans_info[PIVOT_Y]))
    {
      GimpTool          *tool    = GIMP_TOOL (tg_tool);
      GimpTransformTool *tr_tool = GIMP_TRANSFORM_TOOL (tg_tool);

      tg_tool->trans_info[PIVOT_X] = px;
      tg_tool->trans_info[PIVOT_Y] = py;

      gimp_transform_grid_tool_push_internal_undo (tg_tool, TRUE);

      gimp_transform_tool_recalc_matrix (tr_tool, tool->display);
    }
}

static void
rotate_pivot_changed (GimpPivotSelector     *selector,
                      GimpTransformGridTool *tg_tool)
{
  GimpTool          *tool    = GIMP_TOOL (tg_tool);
  GimpTransformTool *tr_tool = GIMP_TRANSFORM_TOOL (tg_tool);

  gimp_pivot_selector_get_position (selector,
                                    &tg_tool->trans_info[PIVOT_X],
                                    &tg_tool->trans_info[PIVOT_Y]);

  gimp_transform_grid_tool_push_internal_undo (tg_tool, TRUE);

  gimp_transform_tool_recalc_matrix (tr_tool, tool->display);
}
