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

#include <string.h>

#include <gegl.h>
#include <gtk/gtk.h>

#include "libgimpmath/gimpmath.h"
#include "libgimpwidgets/gimpwidgets.h"

#include "tools-types.h"

#include "core/gimp-transform-utils.h"
#include "core/gimpimage.h"

#include "widgets/gimphelp-ids.h"
#include "widgets/gimpsizebox.h"

#include "display/gimpdisplay.h"
#include "display/gimpdisplayshell.h"
#include "display/gimpdisplayshell-transform.h"
#include "display/gimptoolgui.h"
#include "display/gimptooltransformgrid.h"

#include "gimpscaletool.h"
#include "gimptoolcontrol.h"
#include "gimptransformgridoptions.h"

#include "gimp-intl.h"


#define EPSILON 1e-6


/*  index into trans_info array  */
enum
{
  X0,
  Y0,
  X1,
  Y1
};


/*  local function prototypes  */

static gboolean         gimp_scale_tool_info_to_matrix (GimpTransformGridTool *tg_tool,
                                                        GimpMatrix3           *transform);
static void             gimp_scale_tool_matrix_to_info (GimpTransformGridTool *tg_tool,
                                                        const GimpMatrix3     *transform);
static gchar          * gimp_scale_tool_get_undo_desc  (GimpTransformGridTool *tg_tool);
static void             gimp_scale_tool_dialog         (GimpTransformGridTool *tg_tool);
static void             gimp_scale_tool_dialog_update  (GimpTransformGridTool *tg_tool);
static void             gimp_scale_tool_prepare        (GimpTransformGridTool *tg_tool);
static void             gimp_scale_tool_readjust       (GimpTransformGridTool *tg_tool);
static GimpToolWidget * gimp_scale_tool_get_widget     (GimpTransformGridTool *tg_tool);
static void             gimp_scale_tool_update_widget  (GimpTransformGridTool *tg_tool);
static void             gimp_scale_tool_widget_changed (GimpTransformGridTool *tg_tool);

static void             gimp_scale_tool_size_notify    (GtkWidget             *box,
                                                        GParamSpec            *pspec,
                                                        GimpTransformGridTool *tg_tool);


G_DEFINE_TYPE (GimpScaleTool, gimp_scale_tool, GIMP_TYPE_TRANSFORM_GRID_TOOL)

#define parent_class gimp_scale_tool_parent_class


void
gimp_scale_tool_register (GimpToolRegisterCallback  callback,
                          gpointer                  data)
{
  (* callback) (GIMP_TYPE_SCALE_TOOL,
                GIMP_TYPE_TRANSFORM_GRID_OPTIONS,
                gimp_transform_grid_options_gui,
                GIMP_CONTEXT_PROP_MASK_BACKGROUND,
                "gimp-scale-tool",
                _("Scale"),
                _("Scale Tool: Scale the layer, selection or path"),
                N_("_Scale"), "<shift>S",
                NULL, GIMP_HELP_TOOL_SCALE,
                GIMP_ICON_TOOL_SCALE,
                data);
}

static void
gimp_scale_tool_class_init (GimpScaleToolClass *klass)
{
  GimpTransformToolClass     *tr_class = GIMP_TRANSFORM_TOOL_CLASS (klass);
  GimpTransformGridToolClass *tg_class = GIMP_TRANSFORM_GRID_TOOL_CLASS (klass);

  tg_class->info_to_matrix  = gimp_scale_tool_info_to_matrix;
  tg_class->matrix_to_info  = gimp_scale_tool_matrix_to_info;
  tg_class->get_undo_desc   = gimp_scale_tool_get_undo_desc;
  tg_class->dialog          = gimp_scale_tool_dialog;
  tg_class->dialog_update   = gimp_scale_tool_dialog_update;
  tg_class->prepare         = gimp_scale_tool_prepare;
  tg_class->readjust        = gimp_scale_tool_readjust;
  tg_class->get_widget      = gimp_scale_tool_get_widget;
  tg_class->update_widget   = gimp_scale_tool_update_widget;
  tg_class->widget_changed  = gimp_scale_tool_widget_changed;

  tr_class->undo_desc       = C_("undo-type", "Scale");
  tr_class->progress_text   = _("Scaling");
  tg_class->ok_button_label = _("_Scale");
}

static void
gimp_scale_tool_init (GimpScaleTool *scale_tool)
{
  GimpTool *tool  = GIMP_TOOL (scale_tool);

  gimp_tool_control_set_tool_cursor (tool->control, GIMP_TOOL_CURSOR_RESIZE);
}

static gboolean
gimp_scale_tool_info_to_matrix  (GimpTransformGridTool *tg_tool,
                                 GimpMatrix3           *transform)
{
  GimpTransformTool *tr_tool = GIMP_TRANSFORM_TOOL (tg_tool);

  gimp_matrix3_identity (transform);
  gimp_transform_matrix_scale (transform,
                               tr_tool->x1,
                               tr_tool->y1,
                               tr_tool->x2 - tr_tool->x1,
                               tr_tool->y2 - tr_tool->y1,
                               tg_tool->trans_info[X0],
                               tg_tool->trans_info[Y0],
                               tg_tool->trans_info[X1] - tg_tool->trans_info[X0],
                               tg_tool->trans_info[Y1] - tg_tool->trans_info[Y0]);

  return TRUE;
}

static void
gimp_scale_tool_matrix_to_info  (GimpTransformGridTool *tg_tool,
                                 const GimpMatrix3     *transform)
{
  GimpTransformTool *tr_tool = GIMP_TRANSFORM_TOOL (tg_tool);
  gdouble            x;
  gdouble            y;
  gdouble            w;
  gdouble            h;

  x = transform->coeff[0][2];
  y = transform->coeff[1][2];
  w = transform->coeff[0][0];
  h = transform->coeff[1][1];

  tg_tool->trans_info[X0] = x + w * tr_tool->x1;
  tg_tool->trans_info[Y0] = y + h * tr_tool->y1;
  tg_tool->trans_info[X1] = tg_tool->trans_info[X0] +
                            w * (tr_tool->x2 - tr_tool->x1);
  tg_tool->trans_info[Y1] = tg_tool->trans_info[Y0] +
                            h * (tr_tool->y2 - tr_tool->y1);
}

static gchar *
gimp_scale_tool_get_undo_desc (GimpTransformGridTool *tg_tool)
{
  gint width;
  gint height;

  width  = ROUND (tg_tool->trans_info[X1] - tg_tool->trans_info[X0]);
  height = ROUND (tg_tool->trans_info[Y1] - tg_tool->trans_info[Y0]);

  return g_strdup_printf (C_("undo-type", "Scale to %d x %d"),
                          width, height);
}

static void
gimp_scale_tool_dialog (GimpTransformGridTool *tg_tool)
{
}

static void
gimp_scale_tool_dialog_update (GimpTransformGridTool *tg_tool)
{
  GimpTransformGridOptions *options = GIMP_TRANSFORM_GRID_TOOL_GET_OPTIONS (tg_tool);
  gint                      width;
  gint                      height;

  width  = ROUND (tg_tool->trans_info[X1] - tg_tool->trans_info[X0]);
  height = ROUND (tg_tool->trans_info[Y1] - tg_tool->trans_info[Y0]);

  g_object_set (GIMP_SCALE_TOOL (tg_tool)->box,
                "width",       width,
                "height",      height,
                "keep-aspect", options->constrain_scale,
                NULL);
}

static void
gimp_scale_tool_prepare (GimpTransformGridTool *tg_tool)
{
  GimpScaleTool            *scale   = GIMP_SCALE_TOOL (tg_tool);
  GimpTransformTool        *tr_tool = GIMP_TRANSFORM_TOOL (tg_tool);
  GimpTransformGridOptions *options = GIMP_TRANSFORM_GRID_TOOL_GET_OPTIONS (tg_tool);
  GimpDisplay              *display = GIMP_TOOL (tg_tool)->display;
  gdouble                   xres;
  gdouble                   yres;

  tg_tool->trans_info[X0] = (gdouble) tr_tool->x1;
  tg_tool->trans_info[Y0] = (gdouble) tr_tool->y1;
  tg_tool->trans_info[X1] = (gdouble) tr_tool->x2;
  tg_tool->trans_info[Y1] = (gdouble) tr_tool->y2;

  gimp_image_get_resolution (gimp_display_get_image (display),
                             &xres, &yres);

  if (scale->box)
    {
      g_signal_handlers_disconnect_by_func (scale->box,
                                            gimp_scale_tool_size_notify,
                                            tg_tool);
      gtk_widget_destroy (scale->box);
    }

  /*  Need to create a new GimpSizeBox widget because the initial
   *  width and height is what counts as 100%.
   */
  scale->box =
    g_object_new (GIMP_TYPE_SIZE_BOX,
                  "width",       tr_tool->x2 - tr_tool->x1,
                  "height",      tr_tool->y2 - tr_tool->y1,
                  "keep-aspect", options->constrain_scale,
                  "unit",        gimp_display_get_shell (display)->unit,
                  "xresolution", xres,
                  "yresolution", yres,
                  NULL);

  gtk_box_pack_start (GTK_BOX (gimp_tool_gui_get_vbox (tg_tool->gui)),
                      scale->box, FALSE, FALSE, 0);
  gtk_widget_show (scale->box);

  g_signal_connect (scale->box, "notify",
                    G_CALLBACK (gimp_scale_tool_size_notify),
                    tg_tool);
}

static void
gimp_scale_tool_readjust (GimpTransformGridTool *tg_tool)
{
  GimpTool         *tool  = GIMP_TOOL (tg_tool);
  GimpDisplayShell *shell = gimp_display_get_shell (tool->display);
  gdouble           x;
  gdouble           y;
  gdouble           r;

  x = shell->disp_width  / 2.0;
  y = shell->disp_height / 2.0;
  r = MAX (MIN (x, y) / G_SQRT2 -
           GIMP_TOOL_TRANSFORM_GRID_MAX_HANDLE_SIZE / 2.0,
           GIMP_TOOL_TRANSFORM_GRID_MAX_HANDLE_SIZE / 2.0);

  gimp_display_shell_untransform_xy_f (shell,
                                       x, y,
                                       &x, &y);

  tg_tool->trans_info[X0] = RINT (x - FUNSCALEX (shell, r));
  tg_tool->trans_info[Y0] = RINT (y - FUNSCALEY (shell, r));
  tg_tool->trans_info[X1] = RINT (x + FUNSCALEX (shell, r));
  tg_tool->trans_info[Y1] = RINT (y + FUNSCALEY (shell, r));
}

static GimpToolWidget *
gimp_scale_tool_get_widget (GimpTransformGridTool *tg_tool)
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
                "inside-function",    GIMP_TRANSFORM_FUNCTION_SCALE,
                "outside-function",   GIMP_TRANSFORM_FUNCTION_SCALE,
                "use-corner-handles", TRUE,
                "use-side-handles",   TRUE,
                "use-center-handle",  TRUE,
                NULL);

  return widget;
}

static void
gimp_scale_tool_update_widget (GimpTransformGridTool *tg_tool)
{
  GimpTransformTool *tr_tool = GIMP_TRANSFORM_TOOL (tg_tool);

  GIMP_TRANSFORM_GRID_TOOL_CLASS (parent_class)->update_widget (tg_tool);

  g_object_set (
    tg_tool->widget,
    "x1", (gdouble) tr_tool->x1,
    "y1", (gdouble) tr_tool->y1,
    "x2", (gdouble) tr_tool->x2,
    "y2", (gdouble) tr_tool->y2,
    NULL);
}

static void
gimp_scale_tool_widget_changed (GimpTransformGridTool *tg_tool)
{
  GimpTool          *tool    = GIMP_TOOL (tg_tool);
  GimpTransformTool *tr_tool = GIMP_TRANSFORM_TOOL (tg_tool);
  GimpMatrix3       *transform;
  gdouble            x0, y0;
  gdouble            x1, y1;
  gint               width, height;

  g_object_get (tg_tool->widget,
                "transform", &transform,
                NULL);

  gimp_matrix3_transform_point (transform,
                                tr_tool->x1, tr_tool->y1,
                                &x0,         &y0);
  gimp_matrix3_transform_point (transform,
                                tr_tool->x2, tr_tool->y2,
                                &x1,         &y1);

  g_free (transform);

  width  = ROUND (x1 - x0);
  height = ROUND (y1 - y0);

  if (width > 0)
    {
      tg_tool->trans_info[X0] = x0;
      tg_tool->trans_info[X1] = x1;
    }
  else if (fabs (x0 - tg_tool->trans_info[X0]) < EPSILON)
    {
      tg_tool->trans_info[X1] = tg_tool->trans_info[X0] + 1.0;
    }
  else if (fabs (x1 - tg_tool->trans_info[X1]) < EPSILON)
    {
      tg_tool->trans_info[X0] = tg_tool->trans_info[X1] - 1.0;
    }
  else
    {
      tg_tool->trans_info[X0] = (x0 + x1) / 2.0 - 0.5;
      tg_tool->trans_info[X1] = (x0 + x1) / 2.0 + 0.5;
    }

  if (height > 0)
    {
      tg_tool->trans_info[Y0] = y0;
      tg_tool->trans_info[Y1] = y1;
    }
  else if (fabs (y0 - tg_tool->trans_info[Y0]) < EPSILON)
    {
      tg_tool->trans_info[Y1] = tg_tool->trans_info[Y0] + 1.0;
    }
  else if (fabs (y1 - tg_tool->trans_info[Y1]) < EPSILON)
    {
      tg_tool->trans_info[Y0] = tg_tool->trans_info[Y1] - 1.0;
    }
  else
    {
      tg_tool->trans_info[Y0] = (y0 + y1) / 2.0 - 0.5;
      tg_tool->trans_info[Y1] = (y0 + y1) / 2.0 + 0.5;
    }

  if (width <= 0 || height <= 0)
    gimp_transform_tool_recalc_matrix (tr_tool, tool->display);

  GIMP_TRANSFORM_GRID_TOOL_CLASS (parent_class)->widget_changed (tg_tool);
}

static void
gimp_scale_tool_size_notify (GtkWidget             *box,
                             GParamSpec            *pspec,
                             GimpTransformGridTool *tg_tool)
{
  GimpTransformGridOptions *options = GIMP_TRANSFORM_GRID_TOOL_GET_OPTIONS (tg_tool);

  if (! strcmp (pspec->name, "width") ||
      ! strcmp (pspec->name, "height"))
    {
      gint width;
      gint height;
      gint old_width;
      gint old_height;

      g_object_get (box,
                    "width",  &width,
                    "height", &height,
                    NULL);

      old_width  = ROUND (tg_tool->trans_info[X1] - tg_tool->trans_info[X0]);
      old_height = ROUND (tg_tool->trans_info[Y1] - tg_tool->trans_info[Y0]);

      if ((width != old_width) || (height != old_height))
        {
          GimpTool          *tool    = GIMP_TOOL (tg_tool);
          GimpTransformTool *tr_tool = GIMP_TRANSFORM_TOOL (tg_tool);

          if (options->frompivot_scale)
            {
              gdouble center_x;
              gdouble center_y;

              center_x = (tg_tool->trans_info[X0] +
                          tg_tool->trans_info[X1]) / 2.0;
              center_y = (tg_tool->trans_info[Y0] +
                          tg_tool->trans_info[Y1]) / 2.0;

              tg_tool->trans_info[X0] = center_x - width  / 2.0;
              tg_tool->trans_info[Y0] = center_y - height / 2.0;
              tg_tool->trans_info[X1] = center_x + width  / 2.0;
              tg_tool->trans_info[Y1] = center_y + height / 2.0;
            }
          else
            {
              tg_tool->trans_info[X1] = tg_tool->trans_info[X0] + width;
              tg_tool->trans_info[Y1] = tg_tool->trans_info[Y0] + height;
            }

          gimp_transform_grid_tool_push_internal_undo (tg_tool, TRUE);

          gimp_transform_tool_recalc_matrix (tr_tool, tool->display);
        }
    }
  else if (! strcmp (pspec->name, "keep-aspect"))
    {
      gboolean constrain;

      g_object_get (box,
                    "keep-aspect", &constrain,
                    NULL);

      if (constrain != options->constrain_scale)
        {
          gint width;
          gint height;

          g_object_get (box,
                        "width",  &width,
                        "height", &height,
                        NULL);

          g_object_set (options,
                        "constrain-scale", constrain,
                        NULL);
        }
    }
}
