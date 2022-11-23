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

#include "libligmamath/ligmamath.h"
#include "libligmawidgets/ligmawidgets.h"

#include "tools-types.h"

#include "core/ligma-transform-utils.h"

#include "widgets/ligmahelp-ids.h"
#include "widgets/ligmawidgets-utils.h"

#include "display/ligmadisplay.h"
#include "display/ligmatoolgui.h"

#include "ligmagenerictransformtool.h"
#include "ligmatoolcontrol.h"
#include "ligmatransformgridoptions.h"

#include "ligma-intl.h"


/*  local function prototypes  */

static gboolean   ligma_generic_transform_tool_info_to_matrix (LigmaTransformGridTool *tg_tool,
                                                              LigmaMatrix3           *transform);
static void       ligma_generic_transform_tool_dialog         (LigmaTransformGridTool *tg_tool);
static void       ligma_generic_transform_tool_dialog_update  (LigmaTransformGridTool *tg_tool);
static void       ligma_generic_transform_tool_prepare        (LigmaTransformGridTool *tg_tool);


G_DEFINE_TYPE (LigmaGenericTransformTool, ligma_generic_transform_tool,
               LIGMA_TYPE_TRANSFORM_GRID_TOOL)

#define parent_class ligma_generic_transform_tool_parent_class


static void
ligma_generic_transform_tool_class_init (LigmaGenericTransformToolClass *klass)
{
  LigmaTransformGridToolClass *tg_class = LIGMA_TRANSFORM_GRID_TOOL_CLASS (klass);

  tg_class->info_to_matrix = ligma_generic_transform_tool_info_to_matrix;
  tg_class->dialog         = ligma_generic_transform_tool_dialog;
  tg_class->dialog_update  = ligma_generic_transform_tool_dialog_update;
  tg_class->prepare        = ligma_generic_transform_tool_prepare;
}

static void
ligma_generic_transform_tool_init (LigmaGenericTransformTool *unified_tool)
{
}

static gboolean
ligma_generic_transform_tool_info_to_matrix (LigmaTransformGridTool *tg_tool,
                                            LigmaMatrix3           *transform)
{
  LigmaGenericTransformTool *generic = LIGMA_GENERIC_TRANSFORM_TOOL (tg_tool);

  if (LIGMA_GENERIC_TRANSFORM_TOOL_GET_CLASS (generic)->info_to_points)
    LIGMA_GENERIC_TRANSFORM_TOOL_GET_CLASS (generic)->info_to_points (generic);

  ligma_matrix3_identity (transform);

  return ligma_transform_matrix_generic (transform,
                                        generic->input_points,
                                        generic->output_points);
}

static void
ligma_generic_transform_tool_dialog (LigmaTransformGridTool *tg_tool)
{
  LigmaGenericTransformTool *generic = LIGMA_GENERIC_TRANSFORM_TOOL (tg_tool);
  GtkWidget                *frame;
  GtkWidget                *vbox;
  GtkWidget                *grid;
  GtkWidget                *label;
  GtkSizeGroup             *size_group;
  gint                      x, y;

  frame = ligma_frame_new (_("Transform Matrix"));
  gtk_box_pack_start (GTK_BOX (ligma_tool_gui_get_vbox (tg_tool->gui)), frame,
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
          gtk_label_set_width_chars (GTK_LABEL (label), 8);
          ligma_label_set_attributes (GTK_LABEL (label),
                                     PANGO_ATTR_SCALE, PANGO_SCALE_SMALL,
                                     -1);
          gtk_grid_attach (GTK_GRID (grid), label, x, y, 1, 1);
          gtk_widget_show (label);
        }
    }

  label = generic->invalid_label = gtk_label_new (_("Invalid transform"));
  ligma_label_set_attributes (GTK_LABEL (label),
                             PANGO_ATTR_STYLE, PANGO_STYLE_ITALIC,
                             -1);
  gtk_size_group_add_widget (size_group, label);
  gtk_box_pack_start (GTK_BOX (vbox), label, TRUE, TRUE, 0);

  g_object_unref (size_group);
}

static void
ligma_generic_transform_tool_dialog_update (LigmaTransformGridTool *tg_tool)
{
  LigmaGenericTransformTool *generic = LIGMA_GENERIC_TRANSFORM_TOOL (tg_tool);
  LigmaMatrix3               transform;
  gboolean                  transform_valid;

  transform_valid = ligma_transform_grid_tool_info_to_matrix (tg_tool,
                                                             &transform);

  if (transform_valid)
    {
      gint x, y;

      gtk_widget_show (generic->matrix_grid);
      gtk_widget_hide (generic->invalid_label);

      for (y = 0; y < 3; y++)
        {
          for (x = 0; x < 3; x++)
            {
              gchar buf[32];

              g_snprintf (buf, sizeof (buf), "%.4f", transform.coeff[y][x]);

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
ligma_generic_transform_tool_prepare (LigmaTransformGridTool *tg_tool)
{
  LigmaTransformTool        *tr_tool = LIGMA_TRANSFORM_TOOL (tg_tool);
  LigmaGenericTransformTool *generic = LIGMA_GENERIC_TRANSFORM_TOOL (tg_tool);

  generic->input_points[0] = (LigmaVector2) {tr_tool->x1, tr_tool->y1};
  generic->input_points[1] = (LigmaVector2) {tr_tool->x2, tr_tool->y1};
  generic->input_points[2] = (LigmaVector2) {tr_tool->x1, tr_tool->y2};
  generic->input_points[3] = (LigmaVector2) {tr_tool->x2, tr_tool->y2};

  memcpy (generic->output_points, generic->input_points,
          sizeof (generic->input_points));
}
