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

#include <string.h>

#include <gtk/gtk.h>

#include "libgimpmath/gimpmath.h"
#include "libgimpwidgets/gimpwidgets.h"

#include "tools-types.h"

#include "core/gimp-transform-utils.h"
#include "core/gimpimage.h"
#include "core/gimpdrawable-transform.h"
#include "core/gimpunit.h"

#include "widgets/gimphelp-ids.h"
#include "widgets/gimpsizebox.h"

#include "display/gimpdisplay.h"
#include "display/gimpdisplayshell.h"

#include "gimpscaletool.h"
#include "gimptoolcontrol.h"
#include "gimptransformoptions.h"

#include "gimp-intl.h"


/*  local function prototypes  */

static void   gimp_scale_tool_dialog        (GimpTransformTool  *tr_tool);
static void   gimp_scale_tool_dialog_update (GimpTransformTool  *tr_tool);
static void   gimp_scale_tool_prepare       (GimpTransformTool  *tr_tool,
                                             GimpDisplay        *display);
static void   gimp_scale_tool_motion        (GimpTransformTool  *tr_tool,
                                             GimpDisplay        *display);
static void   gimp_scale_tool_recalc        (GimpTransformTool  *tr_tool,
                                             GimpDisplay        *display);

static void   gimp_scale_tool_size_notify   (GtkWidget          *box,
                                             GParamSpec         *pspec,
                                             GimpTransformTool  *tr_tool);


G_DEFINE_TYPE (GimpScaleTool, gimp_scale_tool, GIMP_TYPE_TRANSFORM_TOOL)

#define parent_class gimp_scale_tool_parent_class


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
                _("Scale Tool: Scale the layer, selection or path"),
                N_("_Scale"), "<shift>T",
                NULL, GIMP_HELP_TOOL_SCALE,
                GIMP_STOCK_TOOL_SCALE,
                data);
}

static void
gimp_scale_tool_class_init (GimpScaleToolClass *klass)
{
  GimpTransformToolClass *trans_class = GIMP_TRANSFORM_TOOL_CLASS (klass);

  trans_class->dialog        = gimp_scale_tool_dialog;
  trans_class->dialog_update = gimp_scale_tool_dialog_update;
  trans_class->prepare       = gimp_scale_tool_prepare;
  trans_class->motion        = gimp_scale_tool_motion;
  trans_class->recalc        = gimp_scale_tool_recalc;
}

static void
gimp_scale_tool_init (GimpScaleTool *scale_tool)
{
  GimpTool          *tool    = GIMP_TOOL (scale_tool);
  GimpTransformTool *tr_tool = GIMP_TRANSFORM_TOOL (scale_tool);

  gimp_tool_control_set_tool_cursor (tool->control, GIMP_TOOL_CURSOR_RESIZE);

  tr_tool->undo_desc       = Q_("command|Scale");
  tr_tool->progress_text   = _("Scaling");

  tr_tool->use_grid        = TRUE;
  tr_tool->use_handles     = TRUE;
  tr_tool->use_center      = TRUE;
  tr_tool->use_mid_handles = TRUE;
}

static void
gimp_scale_tool_dialog (GimpTransformTool *tr_tool)
{
}

static void
gimp_scale_tool_dialog_update (GimpTransformTool *tr_tool)
{
  GimpTransformOptions *options = GIMP_TRANSFORM_TOOL_GET_OPTIONS (tr_tool);

  gint width  = ROUND (tr_tool->trans_info[X1] - tr_tool->trans_info[X0]);
  gint height = ROUND (tr_tool->trans_info[Y1] - tr_tool->trans_info[Y0]);

  g_object_set (GIMP_SCALE_TOOL (tr_tool)->box,
                "width",       width,
                "height",      height,
                "keep-aspect", options->constrain,
                NULL);
}

static void
gimp_scale_tool_prepare (GimpTransformTool *tr_tool,
                         GimpDisplay       *display)
{
  GimpScaleTool        *scale   = GIMP_SCALE_TOOL (tr_tool);
  GimpTransformOptions *options = GIMP_TRANSFORM_TOOL_GET_OPTIONS (tr_tool);

  tr_tool->trans_info[X0] = (gdouble) tr_tool->x1;
  tr_tool->trans_info[Y0] = (gdouble) tr_tool->y1;
  tr_tool->trans_info[X1] = (gdouble) tr_tool->x2;
  tr_tool->trans_info[Y1] = (gdouble) tr_tool->y2;

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
                  "keep-aspect", options->constrain,
                  "unit",        GIMP_DISPLAY_SHELL (display->shell)->unit,
                  "xresolution", display->image->xresolution,
                  "yresolution", display->image->yresolution,
                  NULL);

  gtk_container_set_border_width (GTK_CONTAINER (scale->box), 6);
  gtk_box_pack_start (GTK_BOX (GTK_DIALOG (tr_tool->dialog)->vbox), scale->box,
                      FALSE, FALSE, 0);
  gtk_widget_show (scale->box);

  g_signal_connect (scale->box, "notify",
                    G_CALLBACK (gimp_scale_tool_size_notify),
                    tr_tool);
}

static void
gimp_scale_tool_motion (GimpTransformTool *tr_tool,
                        GimpDisplay       *display)
{
  GimpTransformOptions *options = GIMP_TRANSFORM_TOOL_GET_OPTIONS (tr_tool);
  gdouble              *x1;
  gdouble              *y1;
  gdouble              *x2;
  gdouble              *y2;
  gint                  dir_x;
  gint                  dir_y;
  gdouble               diff_x = tr_tool->curx - tr_tool->lastx;
  gdouble               diff_y = tr_tool->cury - tr_tool->lasty;

  switch (tr_tool->function)
    {
    case TRANSFORM_HANDLE_N:
      diff_x = 0; /* and fall through */
    case TRANSFORM_HANDLE_NW:
      x1 = &tr_tool->trans_info[X0];
      y1 = &tr_tool->trans_info[Y0];
      x2 = &tr_tool->trans_info[X1];
      y2 = &tr_tool->trans_info[Y1];
      dir_x = dir_y = 1;
      break;

    case TRANSFORM_HANDLE_E:
      diff_y = 0; /* and fall through */
    case TRANSFORM_HANDLE_NE:
      x1 = &tr_tool->trans_info[X1];
      y1 = &tr_tool->trans_info[Y0];
      x2 = &tr_tool->trans_info[X0];
      y2 = &tr_tool->trans_info[Y1];
      dir_x = -1;
      dir_y = 1;
      break;

    case TRANSFORM_HANDLE_W:
      diff_y = 0; /* and fall through */
    case TRANSFORM_HANDLE_SW:
      x1 = &tr_tool->trans_info[X0];
      y1 = &tr_tool->trans_info[Y1];
      x2 = &tr_tool->trans_info[X1];
      y2 = &tr_tool->trans_info[Y0];
      dir_x = 1;
      dir_y = -1;
      break;

    case TRANSFORM_HANDLE_S:
      diff_x = 0; /* and fall through */
    case TRANSFORM_HANDLE_SE:
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

  *x1 += diff_x;
  *y1 += diff_y;

  /*  if control is being held, constrain the aspect ratio  */
  if (options->constrain)
    {
      /*  FIXME: improve this  */
      gdouble h = tr_tool->trans_info[Y1] - tr_tool->trans_info[Y0];

      switch (tr_tool->function)
        {
        case TRANSFORM_HANDLE_NW:
        case TRANSFORM_HANDLE_SW:
          tr_tool->trans_info[X0] =
            tr_tool->trans_info[X1] - tr_tool->aspect * h;
          break;

        case TRANSFORM_HANDLE_NE:
        case TRANSFORM_HANDLE_SE:
          tr_tool->trans_info[X1] =
            tr_tool->trans_info[X0] + tr_tool->aspect * h;
          break;

        default:
          break;
        }
    }

  if (dir_x > 0)
    {
      if (*x1 >= *x2)
        *x1 = *x2 - 1;
    }
  else
    {
      if (*x1 <= *x2)
        *x1 = *x2 + 1;
    }

  if (dir_y > 0)
    {
      if (*y1 >= *y2)
        *y1 = *y2 - 1;
    }
  else
    {
      if (*y1 <= *y2)
        *y1 = *y2 + 1;
    }
}

static void
gimp_scale_tool_recalc (GimpTransformTool *tr_tool,
                        GimpDisplay       *display)
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
          gimp_draw_tool_pause (GIMP_DRAW_TOOL (tr_tool));

          tr_tool->trans_info[X1] = tr_tool->trans_info[X0] + width;
          tr_tool->trans_info[Y1] = tr_tool->trans_info[Y0] + height;

          gimp_transform_tool_recalc (tr_tool, GIMP_TOOL (tr_tool)->display);

          gimp_transform_tool_expose_preview (tr_tool);

          gimp_draw_tool_resume (GIMP_DRAW_TOOL (tr_tool));
        }
    }
  else if (! strcmp (pspec->name, "keep-aspect"))
    {
      gboolean constrain;

      g_object_get (box,
                    "keep-aspect", &constrain,
                    NULL);

      if (constrain != options->constrain)
        {
          gint width;
          gint height;

          g_object_get (box,
                        "width",  &width,
                        "height", &height,
                        NULL);

          /*  Take the aspect ratio from the size box when the user
           *  activates the constraint by pressing the chain button.
           */
          tr_tool->aspect = (gdouble) width / (gdouble) height;

          g_object_set (options,
                        "constrain", constrain,
                        NULL);
        }
    }
}
