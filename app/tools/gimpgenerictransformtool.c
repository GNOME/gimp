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

#include <gegl.h>
#include <gtk/gtk.h>

#include "libgimpmath/gimpmath.h"
#include "libgimpwidgets/gimpwidgets.h"

#include "tools-types.h"

#include "core/gimp-transform-utils.h"

#include "widgets/gimphelp-ids.h"
#include "widgets/gimpwidgets-utils.h"

#include "display/gimpdisplay.h"
#include "display/gimptoolgui.h"

#include "gimpgenerictransformtool.h"
#include "gimptoolcontrol.h"
#include "gimptransformoptions.h"

#include "gimp-intl.h"


/*  local function prototypes  */

static void   gimp_generic_transform_tool_dialog        (GimpTransformTool *tr_tool);
static void   gimp_generic_transform_tool_dialog_update (GimpTransformTool *tr_tool);
static void   gimp_generic_transform_tool_prepare       (GimpTransformTool *tr_tool);
static void   gimp_generic_transform_tool_recalc_matrix (GimpTransformTool *tr_tool,
                                                         GimpToolWidget    *widget);


G_DEFINE_TYPE (GimpGenericTransformTool, gimp_generic_transform_tool,
               GIMP_TYPE_TRANSFORM_TOOL)


static void
gimp_generic_transform_tool_class_init (GimpGenericTransformToolClass *klass)
{
  GimpTransformToolClass *trans_class = GIMP_TRANSFORM_TOOL_CLASS (klass);

  trans_class->dialog        = gimp_generic_transform_tool_dialog;
  trans_class->dialog_update = gimp_generic_transform_tool_dialog_update;
  trans_class->prepare       = gimp_generic_transform_tool_prepare;
  trans_class->recalc_matrix = gimp_generic_transform_tool_recalc_matrix;
}

static void
gimp_generic_transform_tool_init (GimpGenericTransformTool *unified_tool)
{
}

static void
gimp_generic_transform_tool_dialog (GimpTransformTool *tr_tool)
{
  GimpGenericTransformTool *generic = GIMP_GENERIC_TRANSFORM_TOOL (tr_tool);
  GtkWidget                *frame;
  GtkWidget                *vbox;
  GtkWidget                *grid;
  GtkWidget                *label;
  GtkSizeGroup             *size_group;
  gint                      x, y;

  frame = gimp_frame_new (_("Transform Matrix"));
  gtk_box_pack_start (GTK_BOX (gimp_tool_gui_get_vbox (tr_tool->gui)), frame,
                      FALSE, FALSE, 0);
  gtk_widget_show (frame);

  vbox = gtk_box_new (GTK_ORIENTATION_VERTICAL, 0);
  gtk_container_add (GTK_CONTAINER (frame), vbox);
  gtk_widget_show (vbox);

  size_group = gtk_size_group_new (GTK_SIZE_GROUP_BOTH);

  grid = generic->matrix_grid = gtk_grid_new ();
  gtk_grid_set_row_spacing (GTK_GRID (grid), 2);
  gtk_grid_set_column_spacing (GTK_GRID (grid), 2);
  gtk_box_pack_start (GTK_BOX (vbox), grid, TRUE, TRUE, 0);
  gtk_size_group_add_widget (size_group, grid);
  gtk_widget_show (grid);

  for (y = 0; y < 3; y++)
    {
      for (x = 0; x < 3; x++)
        {
          label = generic->matrix_labels[y][x] = gtk_label_new (" ");
          gtk_label_set_xalign (GTK_LABEL (label), 1.0);
          gtk_label_set_width_chars (GTK_LABEL (label), 12);
          gtk_grid_attach (GTK_GRID (grid), label, x, y, 1, 1);
          gtk_widget_show (label);
        }
    }

  label = generic->invalid_label = gtk_label_new (_("Invalid transform"));
  gimp_label_set_attributes (GTK_LABEL (label),
                             PANGO_ATTR_STYLE, PANGO_STYLE_ITALIC,
                             -1);
  gtk_size_group_add_widget (size_group, label);
  gtk_box_pack_start (GTK_BOX (vbox), label, TRUE, TRUE, 0);

  g_object_unref (size_group);
}

static void
gimp_generic_transform_tool_dialog_update (GimpTransformTool *tr_tool)
{
  GimpGenericTransformTool *generic = GIMP_GENERIC_TRANSFORM_TOOL (tr_tool);

  if (tr_tool->transform_valid)
    {
      gint x, y;

      gtk_widget_show (generic->matrix_grid);
      gtk_widget_hide (generic->invalid_label);

      for (y = 0; y < 3; y++)
        {
          for (x = 0; x < 3; x++)
            {
              gchar buf[32];

              g_snprintf (buf, sizeof (buf),
                          "%10.5f", tr_tool->transform.coeff[y][x]);

              gtk_label_set_text (GTK_LABEL (generic->matrix_labels[y][x]), buf);
            }
        }
    }
  else
    {
      gtk_widget_show (generic->invalid_label);
      gtk_widget_hide (generic->matrix_grid);
    }
}

static void
gimp_generic_transform_tool_prepare (GimpTransformTool *tr_tool)
{
  GimpGenericTransformTool *generic = GIMP_GENERIC_TRANSFORM_TOOL (tr_tool);

  generic->input_points[0] = (GimpVector2) {tr_tool->x1, tr_tool->y1};
  generic->input_points[1] = (GimpVector2) {tr_tool->x2, tr_tool->y1};
  generic->input_points[2] = (GimpVector2) {tr_tool->x1, tr_tool->y2};
  generic->input_points[3] = (GimpVector2) {tr_tool->x2, tr_tool->y2};

  memcpy (generic->output_points, generic->input_points,
          sizeof (generic->input_points));
}

static void
gimp_generic_transform_tool_recalc_matrix (GimpTransformTool *tr_tool,
                                           GimpToolWidget    *widget)
{
  GimpGenericTransformTool *generic = GIMP_GENERIC_TRANSFORM_TOOL (tr_tool);

  if (GIMP_GENERIC_TRANSFORM_TOOL_GET_CLASS (generic)->recalc_points)
    {
      GIMP_GENERIC_TRANSFORM_TOOL_GET_CLASS (generic)->recalc_points (generic,
                                                                      widget);
    }

  gimp_matrix3_identity (&tr_tool->transform);
  tr_tool->transform_valid =
    gimp_transform_matrix_generic (&tr_tool->transform,
                                   generic->input_points,
                                   generic->output_points);
}
