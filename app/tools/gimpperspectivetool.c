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
#include "core/gimpdrawable-transform.h"

#include "widgets/gimphelp-ids.h"

#include "display/gimpdisplay.h"

#include "gimpperspectivetool.h"
#include "gimptoolcontrol.h"
#include "gimptransformoptions.h"

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
  Y3
};


/*  local function prototypes  */

static void    gimp_perspective_tool_dialog        (GimpTransformTool *tr_tool);
static void    gimp_perspective_tool_dialog_update (GimpTransformTool *tr_tool);
static void    gimp_perspective_tool_prepare       (GimpTransformTool *tr_tool);
static void    gimp_perspective_tool_motion        (GimpTransformTool *tr_tool);
static void    gimp_perspective_tool_recalc_matrix (GimpTransformTool *tr_tool);
static gchar * gimp_perspective_tool_get_undo_desc (GimpTransformTool *tr_tool);


G_DEFINE_TYPE (GimpPerspectiveTool, gimp_perspective_tool,
               GIMP_TYPE_TRANSFORM_TOOL)


void
gimp_perspective_tool_register (GimpToolRegisterCallback  callback,
                                gpointer                  data)
{
  (* callback) (GIMP_TYPE_PERSPECTIVE_TOOL,
                GIMP_TYPE_TRANSFORM_OPTIONS,
                gimp_transform_options_gui,
                GIMP_CONTEXT_BACKGROUND_MASK,
                "gimp-perspective-tool",
                _("Perspective"),
                _("Perspective Tool: "
                  "Change perspective of the layer, selection or path"),
                N_("_Perspective"), "<shift>P",
                NULL, GIMP_HELP_TOOL_PERSPECTIVE,
                GIMP_STOCK_TOOL_PERSPECTIVE,
                data);
}

static void
gimp_perspective_tool_class_init (GimpPerspectiveToolClass *klass)
{
  GimpTransformToolClass *trans_class = GIMP_TRANSFORM_TOOL_CLASS (klass);

  trans_class->dialog        = gimp_perspective_tool_dialog;
  trans_class->dialog_update = gimp_perspective_tool_dialog_update;
  trans_class->prepare       = gimp_perspective_tool_prepare;
  trans_class->motion        = gimp_perspective_tool_motion;
  trans_class->recalc_matrix = gimp_perspective_tool_recalc_matrix;
  trans_class->get_undo_desc = gimp_perspective_tool_get_undo_desc;
}

static void
gimp_perspective_tool_init (GimpPerspectiveTool *perspective_tool)
{
  GimpTool          *tool    = GIMP_TOOL (perspective_tool);
  GimpTransformTool *tr_tool = GIMP_TRANSFORM_TOOL (perspective_tool);

  gimp_tool_control_set_tool_cursor (tool->control,
                                     GIMP_TOOL_CURSOR_PERSPECTIVE);

  tr_tool->progress_text = _("Perspective transformation");

  tr_tool->use_grid      = TRUE;
  tr_tool->use_handles   = TRUE;
  tr_tool->use_center    = TRUE;
}

static void
gimp_perspective_tool_dialog (GimpTransformTool *tr_tool)
{
  GimpPerspectiveTool *perspective = GIMP_PERSPECTIVE_TOOL (tr_tool);
  GtkWidget           *content_area;
  GtkWidget           *frame;
  GtkWidget           *table;
  gint                 x, y;

  content_area = gtk_dialog_get_content_area (GTK_DIALOG (tr_tool->dialog));

  frame = gimp_frame_new (_("Transformation Matrix"));
  gtk_container_set_border_width (GTK_CONTAINER (frame), 6);
  gtk_box_pack_start (GTK_BOX (content_area), frame, FALSE, FALSE, 0);
  gtk_widget_show (frame);

  table = gtk_table_new (3, 3, FALSE);
  gtk_table_set_row_spacings (GTK_TABLE (table), 2);
  gtk_table_set_col_spacings (GTK_TABLE (table), 2);
  gtk_container_add (GTK_CONTAINER (frame), table);
  gtk_widget_show (table);

  for (y = 0; y < 3; y++)
    for (x = 0; x < 3; x++)
      {
        GtkWidget *label = gtk_label_new (" ");

        gtk_misc_set_alignment (GTK_MISC (label), 1.0, 0.0);
        gtk_label_set_width_chars (GTK_LABEL (label), 12);
        gtk_table_attach (GTK_TABLE (table), label,
                          x, x + 1, y, y + 1, GTK_EXPAND, GTK_FILL, 0, 0);
        gtk_widget_show (label);

        perspective->label[y][x] = label;
      }
}

static void
gimp_perspective_tool_dialog_update (GimpTransformTool *tr_tool)
{
  GimpPerspectiveTool *perspective = GIMP_PERSPECTIVE_TOOL (tr_tool);
  gint                 x, y;

  for (y = 0; y < 3; y++)
    for (x = 0; x < 3; x++)
      {
        gchar buf[32];

        g_snprintf (buf, sizeof (buf),
                    "%10.5f", tr_tool->transform.coeff[y][x]);

        gtk_label_set_text (GTK_LABEL (perspective->label[y][x]), buf);
      }
}

static void
gimp_perspective_tool_prepare (GimpTransformTool  *tr_tool)
{
  tr_tool->trans_info[X0] = (gdouble) tr_tool->x1;
  tr_tool->trans_info[Y0] = (gdouble) tr_tool->y1;
  tr_tool->trans_info[X1] = (gdouble) tr_tool->x2;
  tr_tool->trans_info[Y1] = (gdouble) tr_tool->y1;
  tr_tool->trans_info[X2] = (gdouble) tr_tool->x1;
  tr_tool->trans_info[Y2] = (gdouble) tr_tool->y2;
  tr_tool->trans_info[X3] = (gdouble) tr_tool->x2;
  tr_tool->trans_info[Y3] = (gdouble) tr_tool->y2;
}

static void
gimp_perspective_tool_motion (GimpTransformTool *transform_tool)
{
  gdouble diff_x, diff_y;

  diff_x = transform_tool->curx - transform_tool->lastx;
  diff_y = transform_tool->cury - transform_tool->lasty;

  switch (transform_tool->function)
    {
    case TRANSFORM_HANDLE_NW:
      transform_tool->trans_info[X0] += diff_x;
      transform_tool->trans_info[Y0] += diff_y;
      break;

    case TRANSFORM_HANDLE_NE:
      transform_tool->trans_info[X1] += diff_x;
      transform_tool->trans_info[Y1] += diff_y;
      break;

    case TRANSFORM_HANDLE_SW:
      transform_tool->trans_info[X2] += diff_x;
      transform_tool->trans_info[Y2] += diff_y;
      break;

    case TRANSFORM_HANDLE_SE:
      transform_tool->trans_info[X3] += diff_x;
      transform_tool->trans_info[Y3] += diff_y;
      break;

    case TRANSFORM_HANDLE_CENTER:
      transform_tool->trans_info[X0] += diff_x;
      transform_tool->trans_info[Y0] += diff_y;
      transform_tool->trans_info[X1] += diff_x;
      transform_tool->trans_info[Y1] += diff_y;
      transform_tool->trans_info[X2] += diff_x;
      transform_tool->trans_info[Y2] += diff_y;
      transform_tool->trans_info[X3] += diff_x;
      transform_tool->trans_info[Y3] += diff_y;
      break;

    default:
      break;
    }
}

static void
gimp_perspective_tool_recalc_matrix (GimpTransformTool *tr_tool)
{
  gimp_matrix3_identity (&tr_tool->transform);
  gimp_transform_matrix_perspective (&tr_tool->transform,
                                     tr_tool->x1,
                                     tr_tool->y1,
                                     tr_tool->x2 - tr_tool->x1,
                                     tr_tool->y2 - tr_tool->y1,
                                     tr_tool->trans_info[X0],
                                     tr_tool->trans_info[Y0],
                                     tr_tool->trans_info[X1],
                                     tr_tool->trans_info[Y1],
                                     tr_tool->trans_info[X2],
                                     tr_tool->trans_info[Y2],
                                     tr_tool->trans_info[X3],
                                     tr_tool->trans_info[Y3]);
}

static gchar *
gimp_perspective_tool_get_undo_desc (GimpTransformTool *tr_tool)
{
  return g_strdup (C_("undo-type", "Perspective"));
}
