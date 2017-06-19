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
#include "display/gimptoolgui.h"
#include "display/gimptooltransformgrid.h"

#include "gimpscaletool.h"
#include "gimptoolcontrol.h"
#include "gimptransformoptions.h"

#include "gimp-intl.h"


/*  index into trans_info array  */
enum
{
  X0,
  Y0,
  X1,
  Y1
};


/*  local function prototypes  */

static void             gimp_scale_tool_dialog         (GimpTransformTool *tr_tool);
static void             gimp_scale_tool_dialog_update  (GimpTransformTool *tr_tool);
static void             gimp_scale_tool_prepare        (GimpTransformTool *tr_tool);
static GimpToolWidget * gimp_scale_tool_get_widget     (GimpTransformTool *tr_tool);
static void             gimp_scale_tool_recalc_matrix  (GimpTransformTool *tr_tool,
                                                        GimpToolWidget    *widget);
static gchar          * gimp_scale_tool_get_undo_desc  (GimpTransformTool *tr_tool);

static void             gimp_scale_tool_widget_changed (GimpToolWidget    *widget,
                                                        GimpTransformTool *tr_tool);

static void             gimp_scale_tool_size_notify    (GtkWidget         *box,
                                                        GParamSpec        *pspec,
                                                        GimpTransformTool *tr_tool);


G_DEFINE_TYPE (GimpScaleTool, gimp_scale_tool, GIMP_TYPE_TRANSFORM_TOOL)

#define parent_class gimp_scale_tool_parent_class


void
gimp_scale_tool_register (GimpToolRegisterCallback  callback,
                          gpointer                  data)
{
  (* callback) (GIMP_TYPE_SCALE_TOOL,
                GIMP_TYPE_TRANSFORM_OPTIONS,
                gimp_transform_options_gui,
                GIMP_CONTEXT_PROP_MASK_BACKGROUND,
                "gimp-scale-tool",
                _("Scale"),
                _("Scale Tool: Scale the layer, selection or path"),
                N_("_Scale"), "<shift>T",
                NULL, GIMP_HELP_TOOL_SCALE,
                GIMP_ICON_TOOL_SCALE,
                data);
}

static void
gimp_scale_tool_class_init (GimpScaleToolClass *klass)
{
  GimpTransformToolClass *trans_class = GIMP_TRANSFORM_TOOL_CLASS (klass);

  trans_class->dialog          = gimp_scale_tool_dialog;
  trans_class->dialog_update   = gimp_scale_tool_dialog_update;
  trans_class->prepare         = gimp_scale_tool_prepare;
  trans_class->get_widget      = gimp_scale_tool_get_widget;
  trans_class->recalc_matrix   = gimp_scale_tool_recalc_matrix;
  trans_class->get_undo_desc   = gimp_scale_tool_get_undo_desc;

  trans_class->ok_button_label = _("_Scale");
}

static void
gimp_scale_tool_init (GimpScaleTool *scale_tool)
{
  GimpTool          *tool    = GIMP_TOOL (scale_tool);
  GimpTransformTool *tr_tool = GIMP_TRANSFORM_TOOL (scale_tool);

  gimp_tool_control_set_tool_cursor (tool->control, GIMP_TOOL_CURSOR_RESIZE);

  tr_tool->progress_text = _("Scaling");
}

static void
gimp_scale_tool_dialog (GimpTransformTool *tr_tool)
{
}

static void
gimp_scale_tool_dialog_update (GimpTransformTool *tr_tool)
{
  GimpTransformOptions *options = GIMP_TRANSFORM_TOOL_GET_OPTIONS (tr_tool);
  gint                  width;
  gint                  height;

  width  = ROUND (tr_tool->trans_info[X1] - tr_tool->trans_info[X0]);
  height = ROUND (tr_tool->trans_info[Y1] - tr_tool->trans_info[Y0]);

  g_object_set (GIMP_SCALE_TOOL (tr_tool)->box,
                "width",       width,
                "height",      height,
                "keep-aspect", options->constrain_scale,
                NULL);
}

static void
gimp_scale_tool_prepare (GimpTransformTool *tr_tool)
{
  GimpScaleTool        *scale   = GIMP_SCALE_TOOL (tr_tool);
  GimpTransformOptions *options = GIMP_TRANSFORM_TOOL_GET_OPTIONS (tr_tool);
  GimpDisplay          *display = GIMP_TOOL (tr_tool)->display;
  gdouble               xres;
  gdouble               yres;

  tr_tool->trans_info[X0] = (gdouble) tr_tool->x1;
  tr_tool->trans_info[Y0] = (gdouble) tr_tool->y1;
  tr_tool->trans_info[X1] = (gdouble) tr_tool->x2;
  tr_tool->trans_info[Y1] = (gdouble) tr_tool->y2;

  gimp_image_get_resolution (gimp_display_get_image (display),
                             &xres, &yres);

  if (scale->box)
    {
      g_signal_handlers_disconnect_by_func (scale->box,
                                            gimp_scale_tool_size_notify,
                                            tr_tool);
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

  gtk_box_pack_start (GTK_BOX (gimp_tool_gui_get_vbox (tr_tool->gui)),
                      scale->box, FALSE, FALSE, 0);
  gtk_widget_show (scale->box);

  g_signal_connect (scale->box, "notify",
                    G_CALLBACK (gimp_scale_tool_size_notify),
                    tr_tool);
}

static GimpToolWidget *
gimp_scale_tool_get_widget (GimpTransformTool *tr_tool)
{
  GimpTool             *tool    = GIMP_TOOL (tr_tool);
  GimpTransformOptions *options = GIMP_TRANSFORM_TOOL_GET_OPTIONS (tr_tool);
  GimpDisplayShell     *shell   = gimp_display_get_shell (tool->display);
  GimpToolWidget       *widget;

  widget = gimp_tool_transform_grid_new (shell,
                                         &tr_tool->transform,
                                         tr_tool->x1,
                                         tr_tool->y1,
                                         tr_tool->x2,
                                         tr_tool->y2);

  g_object_set (widget,
                "inside-function",         GIMP_TRANSFORM_FUNCTION_SCALE,
                "outside-function",        GIMP_TRANSFORM_FUNCTION_SCALE,
                "use-corner-handles",      TRUE,
                "use-side-handles",        TRUE,
                "use-center-handle",       TRUE,
                "constrain-move",          options->constrain_move,
                "constrain-scale",         options->constrain_scale,
                "constrain-rotate",        options->constrain_rotate,
                "constrain-shear",         options->constrain_shear,
                "constrain-perspective",   options->constrain_perspective,
                "frompivot-scale",         options->frompivot_scale,
                "frompivot-shear",         options->frompivot_shear,
                "frompivot-perspective",   options->frompivot_perspective,
                "cornersnap",              options->cornersnap,
                "fixedpivot",              options->fixedpivot,
                NULL);

  g_signal_connect (widget, "changed",
                    G_CALLBACK (gimp_scale_tool_widget_changed),
                    tr_tool);

  return widget;
}

static void
gimp_scale_tool_recalc_matrix (GimpTransformTool *tr_tool,
                               GimpToolWidget    *widget)
{
  gimp_matrix3_identity (&tr_tool->transform);
  gimp_transform_matrix_scale (&tr_tool->transform,
                               tr_tool->x1,
                               tr_tool->y1,
                               tr_tool->x2 - tr_tool->x1,
                               tr_tool->y2 - tr_tool->y1,
                               tr_tool->trans_info[X0],
                               tr_tool->trans_info[Y0],
                               tr_tool->trans_info[X1] - tr_tool->trans_info[X0],
                               tr_tool->trans_info[Y1] - tr_tool->trans_info[Y0]);

  if (widget)
    g_object_set (widget,
                  "transform", &tr_tool->transform,
                  "x1",        (gdouble) tr_tool->x1,
                  "y1",        (gdouble) tr_tool->y1,
                  "x2",        (gdouble) tr_tool->x2,
                  "y2",        (gdouble) tr_tool->y2,
                  NULL);
}

static gchar *
gimp_scale_tool_get_undo_desc (GimpTransformTool *tr_tool)
{
  gint width  = ROUND (tr_tool->trans_info[X1] - tr_tool->trans_info[X0]);
  gint height = ROUND (tr_tool->trans_info[Y1] - tr_tool->trans_info[Y0]);

  return g_strdup_printf (C_("undo-type", "Scale to %d x %d"),
                          width, height);
}

static void
gimp_scale_tool_widget_changed (GimpToolWidget    *widget,
                                GimpTransformTool *tr_tool)
{
  GimpMatrix3 *transform;

  g_object_get (widget,
                "transform", &transform,
                NULL);

  gimp_matrix3_transform_point (transform,
                                tr_tool->x1, tr_tool->y1,
                                &tr_tool->trans_info[X0],
                                &tr_tool->trans_info[Y0]);
  gimp_matrix3_transform_point (transform,
                                tr_tool->x2, tr_tool->y2,
                                &tr_tool->trans_info[X1],
                                &tr_tool->trans_info[Y1]);

  g_free (transform);

  gimp_transform_tool_recalc_matrix (tr_tool, NULL);
}

static void
gimp_scale_tool_size_notify (GtkWidget         *box,
                             GParamSpec        *pspec,
                             GimpTransformTool *tr_tool)
{
  GimpTransformOptions *options = GIMP_TRANSFORM_TOOL_GET_OPTIONS (tr_tool);

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

      old_width  = ROUND (tr_tool->trans_info[X1] - tr_tool->trans_info[X0]);
      old_height = ROUND (tr_tool->trans_info[Y1] - tr_tool->trans_info[Y0]);

      if ((width != old_width) || (height != old_height))
        {
          tr_tool->trans_info[X1] = tr_tool->trans_info[X0] + width;
          tr_tool->trans_info[Y1] = tr_tool->trans_info[Y0] + height;

          gimp_transform_tool_push_internal_undo (tr_tool);

          gimp_transform_tool_recalc_matrix (tr_tool, tr_tool->widget);
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
