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
#include "gimptransformgridoptions.h"

#include "gimp-intl.h"


/*  local function prototypes  */

static gboolean   gimp_generic_transform_tool_info_to_matrix (GimpTransformGridTool *tg_tool,
                                                              GimpMatrix3           *transform);
static void       gimp_generic_transform_tool_dialog         (GimpTransformGridTool *tg_tool);
static void       gimp_generic_transform_tool_dialog_update  (GimpTransformGridTool *tg_tool);
static void       gimp_generic_transform_tool_prepare        (GimpTransformGridTool *tg_tool);


G_DEFINE_TYPE (GimpGenericTransformTool, gimp_generic_transform_tool,
               GIMP_TYPE_TRANSFORM_GRID_TOOL)

#define parent_class gimp_generic_transform_tool_parent_class


static void
gimp_generic_transform_tool_class_init (GimpGenericTransformToolClass *klass)
{
  GimpTransformGridToolClass *tg_class = GIMP_TRANSFORM_GRID_TOOL_CLASS (klass);

  tg_class->info_to_matrix = gimp_generic_transform_tool_info_to_matrix;
  tg_class->dialog         = gimp_generic_transform_tool_dialog;
  tg_class->dialog_update  = gimp_generic_transform_tool_dialog_update;
  tg_class->prepare        = gimp_generic_transform_tool_prepare;
}

static void
gimp_generic_transform_tool_init (GimpGenericTransformTool *unified_tool)
{
}

static gboolean
gimp_generic_transform_tool_info_to_matrix (GimpTransformGridTool *tg_tool,
                                            GimpMatrix3           *transform)
{
  GimpGenericTransformTool *generic = GIMP_GENERIC_TRANSFORM_TOOL (tg_tool);

  if (GIMP_GENERIC_TRANSFORM_TOOL_GET_CLASS (generic)->info_to_points)
    GIMP_GENERIC_TRANSFORM_TOOL_GET_CLASS (generic)->info_to_points (generic);

  gimp_matrix3_identity (transform);

  return gimp_transform_matrix_generic (transform,
                                        generic->input_points,
                                        generic->output_points);
}

static void
gimp_generic_transform_tool_dialog (GimpTransformGridTool *tg_tool)
{
  GimpGenericTransformTool *generic = GIMP_GENERIC_TRANSFORM_TOOL (tg_tool);
  GtkWidget                *frame;
  GtkWidget                *vbox;
  GtkWidget                *label;
  GtkSizeGroup             *size_group;

  frame = gimp_frame_new (_("Transform Matrix"));
  gtk_box_pack_start (GTK_BOX (gimp_tool_gui_get_vbox (tg_tool->gui)), frame,
                      FALSE, FALSE, 0);
  gtk_widget_show (frame);

  vbox = gtk_box_new (GTK_ORIENTATION_VERTICAL, 0);
  gtk_container_add (GTK_CONTAINER (frame), vbox);
  gtk_widget_show (vbox);

  size_group = gtk_size_group_new (GTK_SIZE_GROUP_BOTH);

  label = generic->matrix_label = gtk_label_new (" ");
  gtk_size_group_add_widget (size_group, label);
  gtk_box_pack_start (GTK_BOX (vbox), label, TRUE, TRUE, 0);
  gimp_label_set_attributes (GTK_LABEL (label), PANGO_ATTR_SCALE, PANGO_SCALE_SMALL, -1);
  gtk_label_set_selectable (GTK_LABEL (label), TRUE);
  gtk_widget_show (label);

  label = generic->invalid_label = gtk_label_new (_("Invalid transform"));
  gimp_label_set_attributes (GTK_LABEL (label),
                             PANGO_ATTR_STYLE, PANGO_STYLE_ITALIC,
                             -1);
  gtk_size_group_add_widget (size_group, label);
  gtk_box_pack_start (GTK_BOX (vbox), label, TRUE, TRUE, 0);

  g_object_unref (size_group);
}

static void
gimp_generic_transform_tool_dialog_update (GimpTransformGridTool *tg_tool)
{
  GimpGenericTransformTool *generic = GIMP_GENERIC_TRANSFORM_TOOL (tg_tool);
  GimpMatrix3               transform;
  gboolean                  transform_valid;

  transform_valid = gimp_transform_grid_tool_info_to_matrix (tg_tool,
                                                             &transform);

  if (transform_valid)
    {
      gchar buf[256];
      gtk_widget_show (generic->matrix_label);
      gtk_widget_hide (generic->invalid_label);

      g_snprintf (buf, sizeof (buf), "<tt>% 11.4f\t% 11.4f\t% 11.4f\n% 11.4f\t% 11.4f"
                  "\t% 11.4f\n% 11.4f\t% 11.4f\t% 11.4f</tt>",
                  transform.coeff[0][0], transform.coeff[0][1], transform.coeff[0][2],
                  transform.coeff[1][0], transform.coeff[1][1], transform.coeff[1][2],
                  transform.coeff[2][0], transform.coeff[2][1], transform.coeff[2][2]);
      gtk_label_set_markup (GTK_LABEL (generic->matrix_label), buf);
    }
  else
    {
      gtk_widget_show (generic->invalid_label);
      gtk_widget_hide (generic->matrix_label);
    }
}

static void
gimp_generic_transform_tool_prepare (GimpTransformGridTool *tg_tool)
{
  GimpTransformTool        *tr_tool = GIMP_TRANSFORM_TOOL (tg_tool);
  GimpGenericTransformTool *generic = GIMP_GENERIC_TRANSFORM_TOOL (tg_tool);

  generic->input_points[0] = (GimpVector2) {tr_tool->x1, tr_tool->y1};
  generic->input_points[1] = (GimpVector2) {tr_tool->x2, tr_tool->y1};
  generic->input_points[2] = (GimpVector2) {tr_tool->x1, tr_tool->y2};
  generic->input_points[3] = (GimpVector2) {tr_tool->x2, tr_tool->y2};

  memcpy (generic->output_points, generic->input_points,
          sizeof (generic->input_points));
}
