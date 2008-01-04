/* GIMP - The GNU Image Manipulation Program
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
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

#include <gtk/gtk.h>

#include "libgimpmath/gimpmath.h"
#include "libgimpwidgets/gimpwidgets.h"

#include "tools-types.h"

#include "core/gimp-transform-utils.h"
#include "core/gimpimage.h"

#include "widgets/gimphelp-ids.h"

#include "display/gimpdisplay.h"
#include "display/gimpdisplayshell.h"

#include "gimprotatetool.h"
#include "gimptoolcontrol.h"
#include "gimptransformoptions.h"

#include "gimp-intl.h"


/*  index into trans_info array  */
#define ANGLE        0
#define REAL_ANGLE   1
#define CENTER_X     2
#define CENTER_Y     3

#define FIFTEEN_DEG  (G_PI / 12.0)

#define SB_WIDTH     10


/*  local function prototypes  */

static void   gimp_rotate_tool_dialog        (GimpTransformTool   *tr_tool);
static void   gimp_rotate_tool_dialog_update (GimpTransformTool   *tr_tool);
static void   gimp_rotate_tool_prepare       (GimpTransformTool   *tr_tool,
                                              GimpDisplay         *display);
static void   gimp_rotate_tool_motion        (GimpTransformTool   *tr_tool,
                                              GimpDisplay         *display);
static void   gimp_rotate_tool_recalc        (GimpTransformTool   *tr_tool,
                                              GimpDisplay         *display);

static void   rotate_angle_changed           (GtkAdjustment       *adj,
                                              GimpTransformTool   *tr_tool);
static void   rotate_center_changed          (GtkWidget           *entry,
                                              GimpTransformTool   *tr_tool);


G_DEFINE_TYPE (GimpRotateTool, gimp_rotate_tool, GIMP_TYPE_TRANSFORM_TOOL)

#define parent_class gimp_rotate_tool_parent_class


void
gimp_rotate_tool_register (GimpToolRegisterCallback  callback,
                           gpointer                  data)
{
  (* callback) (GIMP_TYPE_ROTATE_TOOL,
                GIMP_TYPE_TRANSFORM_OPTIONS,
                gimp_transform_options_gui,
                0,
                "gimp-rotate-tool",
                _("Rotate"),
                _("Rotate Tool: Rotate the layer, selection or path"),
                N_("_Rotate"), "<shift>R",
                NULL, GIMP_HELP_TOOL_ROTATE,
                GIMP_STOCK_TOOL_ROTATE,
                data);
}

static void
gimp_rotate_tool_class_init (GimpRotateToolClass *klass)
{
  GimpTransformToolClass *trans_class = GIMP_TRANSFORM_TOOL_CLASS (klass);

  trans_class->dialog        = gimp_rotate_tool_dialog;
  trans_class->dialog_update = gimp_rotate_tool_dialog_update;
  trans_class->prepare       = gimp_rotate_tool_prepare;
  trans_class->motion        = gimp_rotate_tool_motion;
  trans_class->recalc        = gimp_rotate_tool_recalc;
}

static void
gimp_rotate_tool_init (GimpRotateTool *rotate_tool)
{
  GimpTool          *tool    = GIMP_TOOL (rotate_tool);
  GimpTransformTool *tr_tool = GIMP_TRANSFORM_TOOL (rotate_tool);

  gimp_tool_control_set_tool_cursor (tool->control, GIMP_TOOL_CURSOR_ROTATE);

  tr_tool->undo_desc     = Q_("command|Rotate");
  tr_tool->progress_text = _("Rotating");

  tr_tool->use_grid      = TRUE;
  tr_tool->use_center    = TRUE;
}

static void
gimp_rotate_tool_dialog (GimpTransformTool *tr_tool)
{
  GimpRotateTool *rotate = GIMP_ROTATE_TOOL (tr_tool);
  GtkWidget      *table;
  GtkWidget      *button;
  GtkWidget      *scale;
  GtkObject      *adj;

  table = gtk_table_new (4, 2, FALSE);
  gtk_container_set_border_width (GTK_CONTAINER (table), 6);
  gtk_table_set_row_spacings (GTK_TABLE (table), 2);
  gtk_table_set_col_spacings (GTK_TABLE (table), 6);
  gtk_table_set_row_spacing (GTK_TABLE (table), 1, 6);
  gtk_box_pack_start (GTK_BOX (GTK_DIALOG (tr_tool->dialog)->vbox), table,
                      FALSE, FALSE, 0);
  gtk_widget_show (table);

  button = gimp_spin_button_new (&rotate->angle_adj,
                                 0, -180, 180, 0.1, 15, 0, 2, 2);
  gtk_spin_button_set_wrap (GTK_SPIN_BUTTON (button), TRUE);
  gtk_entry_set_width_chars (GTK_ENTRY (button), SB_WIDTH);
  gimp_table_attach_aligned (GTK_TABLE (table), 0, 0, _("_Angle:"),
                             0.0, 0.5, button, 1, TRUE);

  g_signal_connect (rotate->angle_adj, "value-changed",
                    G_CALLBACK (rotate_angle_changed),
                    tr_tool);

  scale = gtk_hscale_new (GTK_ADJUSTMENT (rotate->angle_adj));
  gtk_scale_set_draw_value (GTK_SCALE (scale), FALSE);
  gtk_table_attach (GTK_TABLE (table), scale, 1, 2, 1, 2,
                    GTK_EXPAND | GTK_FILL, GTK_SHRINK, 0, 0);
  gtk_widget_show (scale);

  button = gimp_spin_button_new (&adj, 0, -1, 1, 1, 10, 1, 1, 2);
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
                    tr_tool);
}

static void
gimp_rotate_tool_dialog_update (GimpTransformTool *tr_tool)
{
  GimpRotateTool *rotate = GIMP_ROTATE_TOOL (tr_tool);

  gtk_adjustment_set_value (GTK_ADJUSTMENT (rotate->angle_adj),
                            gimp_rad_to_deg (tr_tool->trans_info[ANGLE]));

  g_signal_handlers_block_by_func (rotate->sizeentry,
                                   rotate_center_changed,
                                   tr_tool);

  gimp_size_entry_set_refval (GIMP_SIZE_ENTRY (rotate->sizeentry), 0,
                              tr_tool->trans_info[CENTER_X]);
  gimp_size_entry_set_refval (GIMP_SIZE_ENTRY (rotate->sizeentry), 1,
                              tr_tool->trans_info[CENTER_Y]);

  g_signal_handlers_unblock_by_func (rotate->sizeentry,
                                     rotate_center_changed,
                                     tr_tool);
}

static void
gimp_rotate_tool_prepare (GimpTransformTool *tr_tool,
                          GimpDisplay       *display)
{
  GimpRotateTool *rotate = GIMP_ROTATE_TOOL (tr_tool);

  tr_tool->trans_info[ANGLE]      = 0.0;
  tr_tool->trans_info[REAL_ANGLE] = 0.0;
  tr_tool->trans_info[CENTER_X]   = tr_tool->cx;
  tr_tool->trans_info[CENTER_Y]   = tr_tool->cy;

  g_signal_handlers_block_by_func (rotate->sizeentry,
                                   rotate_center_changed,
                                   tr_tool);

  gimp_size_entry_set_unit (GIMP_SIZE_ENTRY (rotate->sizeentry),
                            GIMP_DISPLAY_SHELL (display->shell)->unit);

  gimp_size_entry_set_resolution (GIMP_SIZE_ENTRY (rotate->sizeentry), 0,
                                  display->image->xresolution, FALSE);
  gimp_size_entry_set_resolution (GIMP_SIZE_ENTRY (rotate->sizeentry), 1,
                                  display->image->yresolution, FALSE);

  gimp_size_entry_set_refval_boundaries (GIMP_SIZE_ENTRY (rotate->sizeentry), 0,
                                         -65536,
                                         65536 + display->image->width);
  gimp_size_entry_set_refval_boundaries (GIMP_SIZE_ENTRY (rotate->sizeentry), 1,
                                         -65536,
                                         65536 + display->image->height);

  gimp_size_entry_set_size (GIMP_SIZE_ENTRY (rotate->sizeentry), 0,
                            tr_tool->x1, tr_tool->x2);
  gimp_size_entry_set_size (GIMP_SIZE_ENTRY (rotate->sizeentry), 1,
                            tr_tool->y1, tr_tool->y2);

  g_signal_handlers_unblock_by_func (rotate->sizeentry,
                                     rotate_center_changed,
                                     tr_tool);
}

static void
gimp_rotate_tool_motion (GimpTransformTool *tr_tool,
                         GimpDisplay       *display)
{
  GimpTransformOptions *options = GIMP_TRANSFORM_TOOL_GET_OPTIONS (tr_tool);
  gdouble               angle1, angle2, angle;
  gdouble               cx, cy;
  gdouble               x1, y1, x2, y2;

  if (tr_tool->function == TRANSFORM_HANDLE_CENTER)
    {
      tr_tool->trans_info[CENTER_X] = tr_tool->curx;
      tr_tool->trans_info[CENTER_Y] = tr_tool->cury;
      tr_tool->cx                   = tr_tool->curx;
      tr_tool->cy                   = tr_tool->cury;

      return;
    }

  cx = tr_tool->trans_info[CENTER_X];
  cy = tr_tool->trans_info[CENTER_Y];

  x1 = tr_tool->curx  - cx;
  x2 = tr_tool->lastx - cx;
  y1 = cy - tr_tool->cury;
  y2 = cy - tr_tool->lasty;

  /*  find the first angle  */
  angle1 = atan2 (y1, x1);

  /*  find the angle  */
  angle2 = atan2 (y2, x2);

  angle = angle2 - angle1;

  if (angle > G_PI || angle < -G_PI)
    angle = angle2 - ((angle1 < 0) ? 2.0 * G_PI + angle1 : angle1 - 2.0 * G_PI);

  /*  increment the transform tool's angle  */
  tr_tool->trans_info[REAL_ANGLE] += angle;

  /*  limit the angle to between -180 and 180 degrees  */
  if (tr_tool->trans_info[REAL_ANGLE] < - G_PI)
    {
      tr_tool->trans_info[REAL_ANGLE] =
        2.0 * G_PI + tr_tool->trans_info[REAL_ANGLE];
    }
  else if (tr_tool->trans_info[REAL_ANGLE] > G_PI)
    {
      tr_tool->trans_info[REAL_ANGLE] =
        tr_tool->trans_info[REAL_ANGLE] - 2.0 * G_PI;
    }

  /*  constrain the angle to 15-degree multiples if ctrl is held down  */
  if (options->constrain)
    {
      tr_tool->trans_info[ANGLE] =
        FIFTEEN_DEG * (int) ((tr_tool->trans_info[REAL_ANGLE] +
                              FIFTEEN_DEG / 2.0) / FIFTEEN_DEG);
    }
  else
    {
      tr_tool->trans_info[ANGLE] = tr_tool->trans_info[REAL_ANGLE];
    }
}

static void
gimp_rotate_tool_recalc (GimpTransformTool *tr_tool,
                         GimpDisplay       *display)
{
  tr_tool->cx = tr_tool->trans_info[CENTER_X];
  tr_tool->cy = tr_tool->trans_info[CENTER_Y];

  gimp_matrix3_identity (&tr_tool->transform);
  gimp_transform_matrix_rotate_center (&tr_tool->transform,
                                       tr_tool->cx,
                                       tr_tool->cy,
                                       tr_tool->trans_info[ANGLE]);
}

static void
rotate_angle_changed (GtkAdjustment     *adj,
                      GimpTransformTool *tr_tool)
{
  gdouble value = gimp_deg_to_rad (gtk_adjustment_get_value (adj));

#define ANGLE_EPSILON 0.0001

  if (ABS (value - tr_tool->trans_info[ANGLE]) > ANGLE_EPSILON)
    {
      gimp_draw_tool_pause (GIMP_DRAW_TOOL (tr_tool));

      tr_tool->trans_info[REAL_ANGLE] = tr_tool->trans_info[ANGLE] = value;

      gimp_transform_tool_recalc (tr_tool, GIMP_TOOL (tr_tool)->display);

      gimp_transform_tool_expose_preview (tr_tool);

      gimp_draw_tool_resume (GIMP_DRAW_TOOL (tr_tool));
    }

#undef ANGLE_EPSILON
}

static void
rotate_center_changed (GtkWidget         *widget,
                       GimpTransformTool *tr_tool)
{
  gdouble cx = gimp_size_entry_get_refval (GIMP_SIZE_ENTRY (widget), 0);
  gdouble cy = gimp_size_entry_get_refval (GIMP_SIZE_ENTRY (widget), 1);

  if ((cx != tr_tool->trans_info[CENTER_X]) ||
      (cy != tr_tool->trans_info[CENTER_Y]))
    {
      gimp_draw_tool_pause (GIMP_DRAW_TOOL (tr_tool));

      tr_tool->trans_info[CENTER_X] = cx;
      tr_tool->trans_info[CENTER_Y] = cy;
      tr_tool->cx = cx;
      tr_tool->cy = cy;

      gimp_transform_tool_recalc (tr_tool, GIMP_TOOL (tr_tool)->display);

      gimp_transform_tool_expose_preview (tr_tool);

      gimp_draw_tool_resume (GIMP_DRAW_TOOL (tr_tool));
    }
}
