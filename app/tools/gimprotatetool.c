/* The GIMP -- an image manipulation program
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
#include "core/gimpdrawable-transform.h"
#include "core/gimptoolinfo.h"

#include "widgets/gimphelp-ids.h"

#include "display/gimpdisplay.h"
#include "display/gimpdisplayshell.h"

#ifdef __GNUC__
#warning FIXME #include "dialogs/dialogs-types.h"
#endif
#include "dialogs/dialogs-types.h"
#include "dialogs/info-dialog.h"

#include "gimprotatetool.h"
#include "gimptoolcontrol.h"
#include "gimptransformoptions.h"

#include "gimp-intl.h"


/*  index into trans_info array  */
#define ANGLE        0
#define REAL_ANGLE   1
#define CENTER_X     2
#define CENTER_Y     3

#define EPSILON      0.018  /*  ~ 1 degree  */
#define FIFTEEN_DEG  (G_PI / 12.0)


/*  local function prototypes  */

static void   gimp_rotate_tool_class_init    (GimpRotateToolClass *klass);
static void   gimp_rotate_tool_init          (GimpRotateTool      *rotate_tool);

static void   gimp_rotate_tool_dialog        (GimpTransformTool   *tr_tool);
static void   gimp_rotate_tool_dialog_update (GimpTransformTool   *tr_tool);
static void   gimp_rotate_tool_prepare       (GimpTransformTool   *tr_tool,
                                              GimpDisplay         *gdisp);
static void   gimp_rotate_tool_motion        (GimpTransformTool   *tr_tool,
                                              GimpDisplay         *gdisp);
static void   gimp_rotate_tool_recalc        (GimpTransformTool   *tr_tool,
                                              GimpDisplay         *gdisp);

static void   rotate_angle_changed           (GtkWidget           *entry,
                                              GimpTransformTool   *tr_tool);
static void   rotate_center_changed          (GtkWidget           *entry,
                                              GimpTransformTool   *tr_tool);


/*  private variables  */

static GimpTransformToolClass *parent_class = NULL;


/*  public functions  */

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
                _("Rotate the layer or selection"),
                N_("_Rotate"), "<shift>R",
                NULL, GIMP_HELP_TOOL_ROTATE,
                GIMP_STOCK_TOOL_ROTATE,
                data);
}

GType
gimp_rotate_tool_get_type (void)
{
  static GType tool_type = 0;

  if (! tool_type)
    {
      static const GTypeInfo tool_info =
      {
        sizeof (GimpRotateToolClass),
        (GBaseInitFunc) NULL,
        (GBaseFinalizeFunc) NULL,
        (GClassInitFunc) gimp_rotate_tool_class_init,
        NULL,           /* class_finalize */
        NULL,           /* class_data     */
        sizeof (GimpRotateTool),
        0,              /* n_preallocs    */
        (GInstanceInitFunc) gimp_rotate_tool_init,
      };

      tool_type = g_type_register_static (GIMP_TYPE_TRANSFORM_TOOL,
                                          "GimpRotateTool",
                                          &tool_info, 0);
    }

  return tool_type;
}


/*  private functions  */

static void
gimp_rotate_tool_class_init (GimpRotateToolClass *klass)
{
  GimpTransformToolClass *trans_class = GIMP_TRANSFORM_TOOL_CLASS (klass);

  parent_class = g_type_class_peek_parent (klass);

  trans_class->dialog         = gimp_rotate_tool_dialog;
  trans_class->dialog_update  = gimp_rotate_tool_dialog_update;
  trans_class->prepare        = gimp_rotate_tool_prepare;
  trans_class->motion         = gimp_rotate_tool_motion;
  trans_class->recalc         = gimp_rotate_tool_recalc;
}

static void
gimp_rotate_tool_init (GimpRotateTool *rotate_tool)
{
  GimpTool          *tool    = GIMP_TOOL (rotate_tool);
  GimpTransformTool *tr_tool = GIMP_TRANSFORM_TOOL (rotate_tool);

  gimp_tool_control_set_tool_cursor (tool->control, GIMP_TOOL_CURSOR_ROTATE);

  tr_tool->shell_desc    = _("Rotation Information");
  tr_tool->progress_text = _("Rotating...");
}

static void
gimp_rotate_tool_dialog (GimpTransformTool *tr_tool)
{
  GimpRotateTool *rotate = GIMP_ROTATE_TOOL (tr_tool);
  GtkWidget      *widget;
  GtkWidget      *spinbutton2;

  widget = info_dialog_add_spinbutton (tr_tool->info_dialog, _("Angle:"),
                                       &rotate->angle_val,
                                       -180, 180, 1, 15, 1, 1, 2,
                                       G_CALLBACK (rotate_angle_changed),
                                       tr_tool);
  gtk_spin_button_set_wrap (GTK_SPIN_BUTTON (widget), TRUE);

  /*  this looks strange (-180, 181), but it works  */
  widget = info_dialog_add_scale (tr_tool->info_dialog, "",
                                  &rotate->angle_val,
                                  -180, 181, 0.01, 0.1, 1, -1,
                                  G_CALLBACK (rotate_angle_changed),
                                  tr_tool);
  gtk_widget_set_size_request (widget, 180, -1);

  spinbutton2 = info_dialog_add_spinbutton (tr_tool->info_dialog,
                                            _("Center X:"),
                                            NULL,
                                            -1, 1, 1, 10, 1, 1, 2,
                                            NULL, NULL);
  rotate->sizeentry = info_dialog_add_sizeentry (tr_tool->info_dialog,
                                                 _("Center Y:"),
                                                 rotate->center_vals, 1,
                                                 GIMP_UNIT_PIXEL, "%a",
                                                 TRUE, TRUE, FALSE,
                                                 GIMP_SIZE_ENTRY_UPDATE_SIZE,
                                                 G_CALLBACK (rotate_center_changed),
                                                 tr_tool);

  gimp_size_entry_add_field (GIMP_SIZE_ENTRY (rotate->sizeentry),
                             GTK_SPIN_BUTTON (spinbutton2), NULL);

  gtk_table_set_row_spacing (GTK_TABLE (tr_tool->info_dialog->info_table),
                             1, 6);
  gtk_table_set_row_spacing (GTK_TABLE (tr_tool->info_dialog->info_table),
                             2, 0);
}

static void
gimp_rotate_tool_dialog_update (GimpTransformTool *tr_tool)
{
  GimpRotateTool *rotate = GIMP_ROTATE_TOOL (tr_tool);

  rotate->angle_val      = gimp_rad_to_deg (tr_tool->trans_info[ANGLE]);
  rotate->center_vals[0] = tr_tool->trans_info[CENTER_X];
  rotate->center_vals[1] = tr_tool->trans_info[CENTER_Y];

  info_dialog_update (tr_tool->info_dialog);
  info_dialog_show (tr_tool->info_dialog);
}

static void
gimp_rotate_tool_prepare (GimpTransformTool *tr_tool,
                          GimpDisplay       *gdisp)
{
  GimpRotateTool *rotate = GIMP_ROTATE_TOOL (tr_tool);

  rotate->angle_val      = 0.0;
  rotate->center_vals[0] = tr_tool->cx;
  rotate->center_vals[1] = tr_tool->cy;

  g_signal_handlers_block_by_func (rotate->sizeentry,
                                   rotate_center_changed,
                                   tr_tool);

  gimp_size_entry_set_unit (GIMP_SIZE_ENTRY (rotate->sizeentry),
                            GIMP_DISPLAY_SHELL (gdisp->shell)->unit);

  gimp_size_entry_set_resolution (GIMP_SIZE_ENTRY (rotate->sizeentry), 0,
                                  gdisp->gimage->xresolution, FALSE);
  gimp_size_entry_set_resolution (GIMP_SIZE_ENTRY (rotate->sizeentry), 1,
                                  gdisp->gimage->yresolution, FALSE);

  gimp_size_entry_set_refval_boundaries (GIMP_SIZE_ENTRY (rotate->sizeentry), 0,
                                         -65536,
                                         65536 + gdisp->gimage->width);
  gimp_size_entry_set_refval_boundaries (GIMP_SIZE_ENTRY (rotate->sizeentry), 1,
                                         -65536,
                                         65536 + gdisp->gimage->height);

  gimp_size_entry_set_size (GIMP_SIZE_ENTRY (rotate->sizeentry), 0,
                            tr_tool->x1, tr_tool->x2);
  gimp_size_entry_set_size (GIMP_SIZE_ENTRY (rotate->sizeentry), 1,
                            tr_tool->y1, tr_tool->y2);

  gimp_size_entry_set_refval (GIMP_SIZE_ENTRY (rotate->sizeentry), 0,
                              rotate->center_vals[0]);
  gimp_size_entry_set_refval (GIMP_SIZE_ENTRY (rotate->sizeentry), 1,
                              rotate->center_vals[1]);

  g_signal_handlers_unblock_by_func (rotate->sizeentry,
                                     rotate_center_changed,
                                     tr_tool);

  tr_tool->trans_info[ANGLE]      = rotate->angle_val;
  tr_tool->trans_info[REAL_ANGLE] = rotate->angle_val;
  tr_tool->trans_info[CENTER_X]   = rotate->center_vals[0];
  tr_tool->trans_info[CENTER_Y]   = rotate->center_vals[1];
}

static void
gimp_rotate_tool_motion (GimpTransformTool *tr_tool,
                         GimpDisplay       *gdisp)
{
  GimpTransformOptions *options;
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

  options =
    GIMP_TRANSFORM_OPTIONS (GIMP_TOOL (tr_tool)->tool_info->tool_options);

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

  /*  limit the angle to between 0 and 360 degrees  */
  if (tr_tool->trans_info[REAL_ANGLE] < - G_PI)
    tr_tool->trans_info[REAL_ANGLE] =
      2.0 * G_PI - tr_tool->trans_info[REAL_ANGLE];
  else if (tr_tool->trans_info[REAL_ANGLE] > G_PI)
    tr_tool->trans_info[REAL_ANGLE] =
      tr_tool->trans_info[REAL_ANGLE] - 2.0 * G_PI;

  /*  constrain the angle to 15-degree multiples if ctrl is held down  */
  if (options->constrain_1)
    {
      tr_tool->trans_info[ANGLE] =
        FIFTEEN_DEG * (int) ((tr_tool->trans_info[REAL_ANGLE] +
                              FIFTEEN_DEG / 2.0) /
                             FIFTEEN_DEG);
    }
  else
    {
      tr_tool->trans_info[ANGLE] = tr_tool->trans_info[REAL_ANGLE];
    }
}

static void
gimp_rotate_tool_recalc (GimpTransformTool *tr_tool,
                         GimpDisplay       *gdisp)
{
  tr_tool->cx = tr_tool->trans_info[CENTER_X];
  tr_tool->cy = tr_tool->trans_info[CENTER_Y];

  gimp_transform_matrix_rotate_center (tr_tool->cx,
                                       tr_tool->cy,
                                       tr_tool->trans_info[ANGLE],
                                       &tr_tool->transform);
}

static void
rotate_angle_changed (GtkWidget         *widget,
                      GimpTransformTool *tr_tool)
{
  gdouble value = gimp_deg_to_rad (GTK_ADJUSTMENT (widget)->value);

#define ANGLE_EPSILON 0.0001

  if (ABS (value - tr_tool->trans_info[ANGLE]) > ANGLE_EPSILON)
    {
      gimp_draw_tool_pause (GIMP_DRAW_TOOL (tr_tool));

      tr_tool->trans_info[ANGLE] = value;

      gimp_transform_tool_recalc (tr_tool, GIMP_TOOL (tr_tool)->gdisp);

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

      gimp_transform_tool_recalc (tr_tool, GIMP_TOOL (tr_tool)->gdisp);

      gimp_transform_tool_expose_preview (tr_tool);

      gimp_draw_tool_resume (GIMP_DRAW_TOOL (tr_tool));
    }
}
