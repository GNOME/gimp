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

#include "libgimpbase/gimpbase.h"
#include "libgimpmath/gimpmath.h"
#include "libgimpwidgets/gimpwidgets.h"

#include "tools-types.h"

#include "core/gimp-transform-utils.h"
#include "core/gimpimage.h"
#include "core/gimpdrawable-transform.h"
#include "core/gimptoolinfo.h"
#include "core/gimpunit.h"

#include "widgets/gimphelp-ids.h"

#include "display/gimpdisplay.h"
#include "display/gimpdisplayshell.h"

#ifdef __GNUC__
#warning FIXME #include "dialogs/dialogs-types.h"
#endif
#include "dialogs/dialogs-types.h"
#include "dialogs/info-dialog.h"

#include "gimpscaletool.h"
#include "gimptoolcontrol.h"
#include "gimptransformoptions.h"

#include "gimp-intl.h"


/*  local function prototypes  */

static void   gimp_scale_tool_class_init     (GimpScaleToolClass *klass);
static void   gimp_scale_tool_init           (GimpScaleTool      *sc_tool);

static void   gimp_scale_tool_dialog         (GimpTransformTool  *tr_tool);
static void   gimp_scale_tool_dialog_update  (GimpTransformTool  *tr_tool);
static void   gimp_scale_tool_prepare        (GimpTransformTool  *tr_tool,
                                              GimpDisplay        *gdisp);
static void   gimp_scale_tool_motion         (GimpTransformTool  *tr_tool,
                                              GimpDisplay        *gdisp);
static void   gimp_scale_tool_recalc         (GimpTransformTool  *tr_tool,
                                              GimpDisplay        *gdisp);

static void   gimp_scale_tool_size_changed   (GtkWidget          *widget,
                                              GimpTransformTool  *tr_tool);
static void   gimp_scale_tool_unit_changed   (GtkWidget          *widget,
                                              GimpTransformTool  *tr_tool);
static void   gimp_scale_tool_aspect_changed (GtkWidget          *widget,
                                              GimpTransformTool  *tr_tool);


/*  private variables  */

static GimpTransformToolClass *parent_class = NULL;


/*  public functions  */

void
gimp_scale_tool_register (GimpToolRegisterCallback  callback,
                          gpointer                  data)
{
  (* callback) (GIMP_TYPE_SCALE_TOOL,
                GIMP_TYPE_TRANSFORM_OPTIONS,
                gimp_transform_options_gui,
                0,
                "gimp-scale-tool",
                _("Scale"),
                _("Scale the layer or selection"),
                N_("_Scale"), "<shift>T",
                NULL, GIMP_HELP_TOOL_SCALE,
                GIMP_STOCK_TOOL_SCALE,
                data);
}

GType
gimp_scale_tool_get_type (void)
{
  static GType tool_type = 0;

  if (! tool_type)
    {
      static const GTypeInfo tool_info =
      {
        sizeof (GimpScaleToolClass),
        (GBaseInitFunc) NULL,
        (GBaseFinalizeFunc) NULL,
        (GClassInitFunc) gimp_scale_tool_class_init,
        NULL,           /* class_finalize */
        NULL,           /* class_data     */
        sizeof (GimpScaleTool),
        0,              /* n_preallocs    */
        (GInstanceInitFunc) gimp_scale_tool_init,
      };

      tool_type = g_type_register_static (GIMP_TYPE_TRANSFORM_TOOL,
                                          "GimpScaleTool",
                                          &tool_info, 0);
    }

  return tool_type;
}


/*  private functions  */

static void
gimp_scale_tool_class_init (GimpScaleToolClass *klass)
{
  GimpTransformToolClass *trans_class = GIMP_TRANSFORM_TOOL_CLASS (klass);

  parent_class = g_type_class_peek_parent (klass);

  trans_class->dialog         = gimp_scale_tool_dialog;
  trans_class->dialog_update  = gimp_scale_tool_dialog_update;
  trans_class->prepare        = gimp_scale_tool_prepare;
  trans_class->motion         = gimp_scale_tool_motion;
  trans_class->recalc         = gimp_scale_tool_recalc;
}

static void
gimp_scale_tool_init (GimpScaleTool *scale_tool)
{
  GimpTool          *tool    = GIMP_TOOL (scale_tool);
  GimpTransformTool *tr_tool = GIMP_TRANSFORM_TOOL (scale_tool);

  gimp_tool_control_set_tool_cursor (tool->control, GIMP_TOOL_CURSOR_RESIZE);

  tr_tool->shell_desc    = _("Scaling information");
  tr_tool->progress_text = _("Scaling...");
}

static void
gimp_scale_tool_dialog (GimpTransformTool *tr_tool)
{
  GimpScaleTool *scale = GIMP_SCALE_TOOL (tr_tool);
  GtkWidget     *spinbutton;

  info_dialog_add_label (tr_tool->info_dialog,
                         _("Original Width:"),
                         scale->orig_width_buf);
  info_dialog_add_label (tr_tool->info_dialog,
                         _("Height:"),
                         scale->orig_height_buf);

  spinbutton = info_dialog_add_spinbutton (tr_tool->info_dialog,
                                           _("Current width:"),
                                           NULL, -1, 1, 1, 10, 1, 1, 2,
                                           NULL, NULL);
  scale->sizeentry = info_dialog_add_sizeentry (tr_tool->info_dialog,
                                                _("Current height:"),
                                                scale->size_vals, 1,
                                                GIMP_UNIT_PIXEL, "%a",
                                                TRUE, TRUE, FALSE,
                                                GIMP_SIZE_ENTRY_UPDATE_SIZE,
                                                G_CALLBACK (gimp_scale_tool_size_changed),
                                                tr_tool);
  g_signal_connect (scale->sizeentry, "unit_changed",
                    G_CALLBACK (gimp_scale_tool_unit_changed),
                    tr_tool);

  gimp_size_entry_add_field (GIMP_SIZE_ENTRY (scale->sizeentry),
                             GTK_SPIN_BUTTON (spinbutton), NULL);

  info_dialog_add_label (tr_tool->info_dialog,
                         _("Scale ratio X:"),
                         scale->x_ratio_buf);
  info_dialog_add_label (tr_tool->info_dialog,
                         _("Scale ratio Y:"),
                         scale->y_ratio_buf);

  spinbutton = info_dialog_add_spinbutton (tr_tool->info_dialog,
                                           _("Aspect Ratio:"),
                                           &scale->aspect_ratio_val,
                                           0, 65536, 0.01, 0.1, 1, 0.5, 2,
                                           G_CALLBACK (gimp_scale_tool_aspect_changed),
                                           tr_tool);

  gtk_table_set_row_spacing (GTK_TABLE (tr_tool->info_dialog->info_table),
                             1, 4);
  gtk_table_set_row_spacing (GTK_TABLE (tr_tool->info_dialog->info_table),
                             2, 0);
}

static void
gimp_scale_tool_dialog_update (GimpTransformTool *tr_tool)
{
  GimpTool             *tool  = GIMP_TOOL (tr_tool);
  GimpScaleTool        *scale = GIMP_SCALE_TOOL (tr_tool);
  GimpTransformOptions *options;
  Gimp                 *gimp;
  gdouble               ratio_x, ratio_y;
  gint                  x1, y1, x2, y2, x3, y3, x4, y4;
  GimpUnit              unit;
  gdouble               unit_factor;
  gchar                 format_buf[16];

  static GimpUnit       label_unit = GIMP_UNIT_PIXEL;

  options = GIMP_TRANSFORM_OPTIONS (tool->tool_info->tool_options);

  unit = gimp_size_entry_get_unit (GIMP_SIZE_ENTRY (scale->sizeentry));

  /*  Find original sizes  */
  x1 = tr_tool->x1;
  y1 = tr_tool->y1;
  x2 = tr_tool->x2;
  y2 = tr_tool->y2;

  if (unit != GIMP_UNIT_PERCENT)
    label_unit = unit;

  gimp = tool->tool_info->gimp;

  unit_factor = _gimp_unit_get_factor (gimp, label_unit);

  if (label_unit) /* unit != GIMP_UNIT_PIXEL */
    {
      g_snprintf (format_buf, sizeof (format_buf), "%%.%df %s",
                  _gimp_unit_get_digits (gimp, label_unit) + 1,
                  _gimp_unit_get_symbol (gimp, label_unit));
      g_snprintf (scale->orig_width_buf, MAX_INFO_BUF, format_buf,
                  (x2 - x1) * unit_factor / tool->gdisp->gimage->xresolution);
      g_snprintf (scale->orig_height_buf, MAX_INFO_BUF, format_buf,
                  (y2 - y1) * unit_factor / tool->gdisp->gimage->yresolution);
    }
  else /* unit == GIMP_UNIT_PIXEL */
    {
      g_snprintf (scale->orig_width_buf, MAX_INFO_BUF, "%d", x2 - x1);
      g_snprintf (scale->orig_height_buf, MAX_INFO_BUF, "%d", y2 - y1);
    }

  /*  Find current sizes  */
  x3 = (gint) tr_tool->trans_info[X0];
  y3 = (gint) tr_tool->trans_info[Y0];
  x4 = (gint) tr_tool->trans_info[X1];
  y4 = (gint) tr_tool->trans_info[Y1];

  scale->size_vals[0] = x4 - x3;
  scale->size_vals[1] = y4 - y3;

  ratio_x = ratio_y = 0.0;

  if (x2 - x1)
    ratio_x = (double) (x4 - x3) / (double) (x2 - x1);
  if (y2 - y1)
    ratio_y = (double) (y4 - y3) / (double) (y2 - y1);

  /* Detecting initial update, aspect_ratio reset */
  if (ratio_x == 1 && ratio_y == 1)
    scale->aspect_ratio_val = 0.0;

  /* Only when one or the two options are disabled, is necessary to
   * update the value Taking care of the initial update too
   */
  if (! options->constrain_1 ||
      ! options->constrain_2 ||
      scale->aspect_ratio_val == 0 )
    {
      scale->aspect_ratio_val =
        ((tr_tool->trans_info[X1] - tr_tool->trans_info[X0]) /
         (tr_tool->trans_info[Y1] - tr_tool->trans_info[Y0]));
    }

  g_snprintf (scale->x_ratio_buf, sizeof (scale->x_ratio_buf),
              "%0.2f", ratio_x);
  g_snprintf (scale->y_ratio_buf, sizeof (scale->y_ratio_buf),
              "%0.2f", ratio_y);

  info_dialog_update (tr_tool->info_dialog);
  info_dialog_show (tr_tool->info_dialog);
}

static void
gimp_scale_tool_prepare (GimpTransformTool *tr_tool,
                         GimpDisplay       *gdisp)
{
  GimpScaleTool *scale = GIMP_SCALE_TOOL (tr_tool);

  scale->size_vals[0] = tr_tool->x2 - tr_tool->x1;
  scale->size_vals[1] = tr_tool->y2 - tr_tool->y1;

  g_signal_handlers_block_by_func (scale->sizeentry,
                                   gimp_scale_tool_size_changed,
                                   tr_tool);
  g_signal_handlers_block_by_func (scale->sizeentry,
                                   gimp_scale_tool_unit_changed,
                                   tr_tool);

  gimp_size_entry_set_unit (GIMP_SIZE_ENTRY (scale->sizeentry),
                            GIMP_DISPLAY_SHELL (gdisp->shell)->unit);

  gimp_size_entry_set_resolution (GIMP_SIZE_ENTRY (scale->sizeentry), 0,
                                  gdisp->gimage->xresolution, FALSE);
  gimp_size_entry_set_resolution (GIMP_SIZE_ENTRY (scale->sizeentry), 1,
                                  gdisp->gimage->yresolution, FALSE);

  gimp_size_entry_set_refval_boundaries (GIMP_SIZE_ENTRY (scale->sizeentry), 0,
                                         GIMP_MIN_IMAGE_SIZE,
                                         GIMP_MAX_IMAGE_SIZE);
  gimp_size_entry_set_refval_boundaries (GIMP_SIZE_ENTRY (scale->sizeentry), 1,
                                         GIMP_MIN_IMAGE_SIZE,
                                         GIMP_MAX_IMAGE_SIZE);

  gimp_size_entry_set_size (GIMP_SIZE_ENTRY (scale->sizeentry), 0,
                            0, scale->size_vals[0]);
  gimp_size_entry_set_size (GIMP_SIZE_ENTRY (scale->sizeentry), 1,
                            0, scale->size_vals[1]);

  gimp_size_entry_set_refval (GIMP_SIZE_ENTRY (scale->sizeentry), 0,
                              scale->size_vals[0]);
  gimp_size_entry_set_refval (GIMP_SIZE_ENTRY (scale->sizeentry), 1,
                              scale->size_vals[1]);

  g_signal_handlers_unblock_by_func (scale->sizeentry,
                                     gimp_scale_tool_size_changed,
                                     tr_tool);
  g_signal_handlers_unblock_by_func (scale->sizeentry,
                                     gimp_scale_tool_unit_changed,
                                     tr_tool);

  tr_tool->trans_info[X0] = (gdouble) tr_tool->x1;
  tr_tool->trans_info[Y0] = (gdouble) tr_tool->y1;
  tr_tool->trans_info[X1] = (gdouble) tr_tool->x2;
  tr_tool->trans_info[Y1] = (gdouble) tr_tool->y2;
}

static void
gimp_scale_tool_motion (GimpTransformTool *tr_tool,
                        GimpDisplay       *gdisp)
{
  GimpTransformOptions *options;
  gdouble              *x1;
  gdouble              *y1;
  gdouble              *x2;
  gdouble              *y2;
  gdouble               mag;
  gdouble               dot;
  gint                  dir_x, dir_y;
  gdouble               diff_x, diff_y;

  options = GIMP_TRANSFORM_OPTIONS (GIMP_TOOL (tr_tool)->tool_info->tool_options);

  diff_x = tr_tool->curx - tr_tool->lastx;
  diff_y = tr_tool->cury - tr_tool->lasty;

  switch (tr_tool->function)
    {
    case TRANSFORM_HANDLE_1:
      x1 = &tr_tool->trans_info[X0];
      y1 = &tr_tool->trans_info[Y0];
      x2 = &tr_tool->trans_info[X1];
      y2 = &tr_tool->trans_info[Y1];
      dir_x = dir_y = 1;
      break;

    case TRANSFORM_HANDLE_2:
      x1 = &tr_tool->trans_info[X1];
      y1 = &tr_tool->trans_info[Y0];
      x2 = &tr_tool->trans_info[X0];
      y2 = &tr_tool->trans_info[Y1];
      dir_x = -1;
      dir_y = 1;
      break;

    case TRANSFORM_HANDLE_3:
      x1 = &tr_tool->trans_info[X0];
      y1 = &tr_tool->trans_info[Y1];
      x2 = &tr_tool->trans_info[X1];
      y2 = &tr_tool->trans_info[Y0];
      dir_x = 1;
      dir_y = -1;
      break;

    case TRANSFORM_HANDLE_4:
      x1 = &tr_tool->trans_info[X1];
      y1 = &tr_tool->trans_info[Y1];
      x2 = &tr_tool->trans_info[X0];
      y2 = &tr_tool->trans_info[Y0];
      dir_x = dir_y = -1;
      break;

    case TRANSFORM_HANDLE_CENTER:
      tr_tool->trans_info[X0] += diff_x;
      tr_tool->trans_info[Y0] += diff_y;
      tr_tool->trans_info[X1] += diff_x;
      tr_tool->trans_info[Y1] += diff_y;
      tr_tool->trans_info[X2] += diff_x;
      tr_tool->trans_info[Y2] += diff_y;
      tr_tool->trans_info[X3] += diff_x;
      tr_tool->trans_info[Y3] += diff_y;

      return;

    default:
      return;
    }

  /*  if just the mod1 key is down, affect only the height  */
  if (options->constrain_2 && ! options->constrain_1)
    {
      diff_x = 0;
    }
  /*  if just the control key is down, affect only the width  */
  else if (options->constrain_1 && ! options->constrain_2)
    {
      diff_y = 0;
    }
  /*  if control and mod1 are both down, constrain the aspect ratio  */
  else if (options->constrain_1 && options->constrain_2)
    {
      mag = hypot ((gdouble) (tr_tool->x2 - tr_tool->x1),
                   (gdouble) (tr_tool->y2 - tr_tool->y1));

      dot = (dir_x * diff_x * (tr_tool->x2 - tr_tool->x1) +
             dir_y * diff_y * (tr_tool->y2 - tr_tool->y1));

      if (mag > 0.0)
        {
          diff_x = dir_x * (tr_tool->x2 - tr_tool->x1) * dot / (mag * mag);
          diff_y = dir_y * (tr_tool->y2 - tr_tool->y1) * dot / (mag * mag);
        }
      else
        {
          diff_x = diff_y = 0;
        }
    }

  *x1 += diff_x;
  *y1 += diff_y;

  if (dir_x > 0)
    {
      if (*x1 >= *x2) *x1 = *x2 - 1;
    }
  else
    {
      if (*x1 <= *x2) *x1 = *x2 + 1;
    }

  if (dir_y > 0)
    {
      if (*y1 >= *y2) *y1 = *y2 - 1;
    }
  else
    {
      if (*y1 <= *y2) *y1 = *y2 + 1;
    }
}

static void
gimp_scale_tool_recalc (GimpTransformTool *tr_tool,
                        GimpDisplay       *gdisp)
{
  gimp_transform_matrix_scale (tr_tool->x1,
                               tr_tool->y1,
                               tr_tool->x2 - tr_tool->x1,
                               tr_tool->y2 - tr_tool->y1,
                               tr_tool->trans_info[X0],
                               tr_tool->trans_info[Y0],
                               tr_tool->trans_info[X1] - tr_tool->trans_info[X0],
                               tr_tool->trans_info[Y1] - tr_tool->trans_info[Y0],
                               &tr_tool->transform);
}

static void
gimp_scale_tool_size_changed (GtkWidget         *widget,
                              GimpTransformTool *tr_tool)
{
  GimpTool             *tool  = GIMP_TOOL (tr_tool);
  GimpScaleTool        *scale = GIMP_SCALE_TOOL (tr_tool);
  GimpTransformOptions *options;
  gint                  width;
  gint                  height;

  options = GIMP_TRANSFORM_OPTIONS (tool->tool_info->tool_options);

  width  = RINT (gimp_size_entry_get_refval (GIMP_SIZE_ENTRY (widget), 0));
  height = RINT (gimp_size_entry_get_refval (GIMP_SIZE_ENTRY (widget), 1));

  if ((width  != (tr_tool->trans_info[X1] -
                  tr_tool->trans_info[X0])) ||
      (height != (tr_tool->trans_info[Y1] -
                  tr_tool->trans_info[Y0])))
    {
      gimp_draw_tool_pause (GIMP_DRAW_TOOL (tr_tool));

      if (options->constrain_1 && options->constrain_2)
        {
          gdouble ratio = scale->aspect_ratio_val;

          /* Calculating height and width taking into account the aspect ratio*/
          if (width != (tr_tool->trans_info[X1] - tr_tool->trans_info[X0]))
            height = width / ratio;
          else
            width = height * ratio;
        }

      tr_tool->trans_info[X1] = tr_tool->trans_info[X0] + width;
      tr_tool->trans_info[Y1] = tr_tool->trans_info[Y0] + height;

      gimp_transform_tool_recalc (tr_tool, GIMP_TOOL (tr_tool)->gdisp);

      gimp_transform_tool_expose_preview (tr_tool);

      gimp_draw_tool_resume (GIMP_DRAW_TOOL (tr_tool));
    }
}

static void
gimp_scale_tool_unit_changed (GtkWidget         *widget,
                              GimpTransformTool *tr_tool)
{
  gimp_scale_tool_dialog_update (tr_tool);
}

static void
gimp_scale_tool_aspect_changed (GtkWidget         *widget,
                                GimpTransformTool *tr_tool)
{
  GimpScaleTool *scale = GIMP_SCALE_TOOL (tr_tool);

  scale->aspect_ratio_val = GTK_ADJUSTMENT (widget)->value;

  gimp_draw_tool_pause (GIMP_DRAW_TOOL (tr_tool));

  tr_tool->trans_info[Y1] =
    ((gdouble) (tr_tool->trans_info[X1] - tr_tool->trans_info[X0]) /
     scale->aspect_ratio_val) +
    tr_tool->trans_info[Y0];

  gimp_transform_tool_recalc (tr_tool, GIMP_TOOL (tr_tool)->gdisp);

  gimp_draw_tool_resume (GIMP_DRAW_TOOL (tr_tool));
}
