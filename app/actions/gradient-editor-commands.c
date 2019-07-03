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

#include "actions-types.h"

#include "core/gimp.h"
#include "core/gimpcontext.h"
#include "core/gimpgradient.h"

#include "widgets/gimpgradienteditor.h"
#include "widgets/gimphelp-ids.h"
#include "widgets/gimpuimanager.h"
#include "widgets/gimpviewabledialog.h"

#include "gradient-editor-commands.h"

#include "gimp-intl.h"


/*  local function prototypes  */

static void   gradient_editor_split_uniform_response (GtkWidget           *widget,
                                                      gint                 response_id,
                                                      GimpGradientEditor  *editor);
static void   gradient_editor_replicate_response     (GtkWidget           *widget,
                                                      gint                 response_id,
                                                      GimpGradientEditor  *editor);


/*  public functions */

void
gradient_editor_left_color_cmd_callback (GimpAction *action,
                                         GVariant   *value,
                                         gpointer    data)
{
  GimpGradientEditor *editor = GIMP_GRADIENT_EDITOR (data);

  gimp_gradient_editor_edit_left_color (editor);
}

void
gradient_editor_left_color_type_cmd_callback (GimpAction *action,
                                              GVariant   *value,
                                              gpointer    data)
{
  GimpGradientEditor  *editor = GIMP_GRADIENT_EDITOR (data);
  GimpGradient        *gradient;
  GimpGradientSegment *left;
  GimpGradientColor    color_type;

  color_type = (GimpGradientColor) g_variant_get_int32 (value);

  gimp_gradient_editor_get_selection (editor, &gradient, &left, NULL);

  if (gradient        &&
      color_type >= 0 &&
      color_type !=
      gimp_gradient_segment_get_left_color_type (gradient, left))
    {
      GimpRGB color;

      gimp_gradient_segment_get_left_flat_color (gradient,
                                                 GIMP_DATA_EDITOR (editor)->context,
                                                 left, &color);

      gimp_data_freeze (GIMP_DATA (gradient));

      gimp_gradient_segment_set_left_color_type (gradient, left, color_type);

      if (color_type == GIMP_GRADIENT_COLOR_FIXED)
        gimp_gradient_segment_set_left_color (gradient, left, &color);

      gimp_data_thaw (GIMP_DATA (gradient));
    }
}

void
gradient_editor_load_left_cmd_callback (GimpAction *action,
                                        GVariant   *value,
                                        gpointer    data)
{
  GimpGradientEditor  *editor      = GIMP_GRADIENT_EDITOR (data);
  GimpDataEditor      *data_editor = GIMP_DATA_EDITOR (data);
  GimpGradient        *gradient;
  GimpGradientSegment *left;
  GimpGradientSegment *right;
  GimpGradientSegment *seg;
  GimpRGB              color;
  GimpGradientColor    color_type = GIMP_GRADIENT_COLOR_FIXED;
  gint                 index      = g_variant_get_int32 (value);

  gimp_gradient_editor_get_selection (editor, &gradient, &left, &right);

  switch (index)
    {
    case GRADIENT_EDITOR_COLOR_NEIGHBOR_ENDPOINT:
      if (left->prev != NULL)
        seg = left->prev;
      else
        seg = gimp_gradient_segment_get_last (left);

      color      = seg->right_color;
      color_type = seg->right_color_type;
      break;

    case GRADIENT_EDITOR_COLOR_OTHER_ENDPOINT:
      color      = right->right_color;
      color_type = right->right_color_type;
      break;

    case GRADIENT_EDITOR_COLOR_FOREGROUND:
      gimp_context_get_foreground (data_editor->context, &color);
      break;

    case GRADIENT_EDITOR_COLOR_BACKGROUND:
      gimp_context_get_background (data_editor->context, &color);
      break;

    default: /* Load a color */
      color = editor->saved_colors[index - GRADIENT_EDITOR_COLOR_FIRST_CUSTOM];
      break;
    }

  gimp_data_freeze (GIMP_DATA (gradient));

  gimp_gradient_segment_range_blend (gradient, left, right,
                                     &color,
                                     &right->right_color,
                                     TRUE, TRUE);
  gimp_gradient_segment_set_left_color_type (gradient, left, color_type);

  gimp_data_thaw (GIMP_DATA (gradient));
}

void
gradient_editor_save_left_cmd_callback (GimpAction *action,
                                        GVariant   *value,
                                        gpointer    data)
{
  GimpGradientEditor  *editor = GIMP_GRADIENT_EDITOR (data);
  GimpGradient        *gradient;
  GimpGradientSegment *left;
  gint                 index = g_variant_get_int32 (value);

  gimp_gradient_editor_get_selection (editor, &gradient, &left, NULL);

  gimp_gradient_segment_get_left_color (gradient, left,
                                        &editor->saved_colors[index]);
}

void
gradient_editor_right_color_cmd_callback (GimpAction *action,
                                          GVariant   *value,
                                          gpointer    data)
{
  GimpGradientEditor *editor = GIMP_GRADIENT_EDITOR (data);

  gimp_gradient_editor_edit_right_color (editor);
}

void
gradient_editor_right_color_type_cmd_callback (GimpAction *action,
                                               GVariant   *value,
                                               gpointer    data)
{
  GimpGradientEditor  *editor = GIMP_GRADIENT_EDITOR (data);
  GimpGradient        *gradient;
  GimpGradientSegment *right;
  GimpGradientColor    color_type;

  color_type = (GimpGradientColor) g_variant_get_int32 (value);

  gimp_gradient_editor_get_selection (editor, &gradient, NULL, &right);

  if (gradient        &&
      color_type >= 0 &&
      color_type !=
      gimp_gradient_segment_get_right_color_type (gradient, right))
    {
      GimpRGB color;

      gimp_gradient_segment_get_right_flat_color (gradient,
                                                  GIMP_DATA_EDITOR (editor)->context,
                                                  right, &color);

      gimp_data_freeze (GIMP_DATA (gradient));

      gimp_gradient_segment_set_right_color_type (gradient, right, color_type);

      if (color_type == GIMP_GRADIENT_COLOR_FIXED)
        gimp_gradient_segment_set_right_color (gradient, right, &color);

      gimp_data_thaw (GIMP_DATA (gradient));
    }
}

void
gradient_editor_load_right_cmd_callback (GimpAction *action,
                                         GVariant   *value,
                                         gpointer    data)
{
  GimpGradientEditor  *editor      = GIMP_GRADIENT_EDITOR (data);
  GimpDataEditor      *data_editor = GIMP_DATA_EDITOR (data);
  GimpGradient        *gradient;
  GimpGradientSegment *left;
  GimpGradientSegment *right;
  GimpGradientSegment *seg;
  GimpRGB              color;
  GimpGradientColor    color_type = GIMP_GRADIENT_COLOR_FIXED;
  gint                 index      = g_variant_get_int32 (value);

  gimp_gradient_editor_get_selection (editor, &gradient, &left, &right);

  switch (index)
    {
    case GRADIENT_EDITOR_COLOR_NEIGHBOR_ENDPOINT:
      if (right->next != NULL)
        seg = right->next;
      else
        seg = gimp_gradient_segment_get_first (right);

      color      = seg->left_color;
      color_type = seg->left_color_type;
      break;

    case GRADIENT_EDITOR_COLOR_OTHER_ENDPOINT:
      color      = left->left_color;
      color_type = left->left_color_type;
      break;

    case GRADIENT_EDITOR_COLOR_FOREGROUND:
      gimp_context_get_foreground (data_editor->context, &color);
      break;

    case GRADIENT_EDITOR_COLOR_BACKGROUND:
      gimp_context_get_background (data_editor->context, &color);
      break;

    default: /* Load a color */
      color = editor->saved_colors[index - GRADIENT_EDITOR_COLOR_FIRST_CUSTOM];
      break;
    }

  gimp_data_freeze (GIMP_DATA (gradient));

  gimp_gradient_segment_range_blend (gradient, left, right,
                                     &left->left_color,
                                     &color,
                                     TRUE, TRUE);
  gimp_gradient_segment_set_right_color_type (gradient, left, color_type);

  gimp_data_thaw (GIMP_DATA (gradient));
}

void
gradient_editor_save_right_cmd_callback (GimpAction *action,
                                         GVariant   *value,
                                         gpointer    data)
{
  GimpGradientEditor  *editor = GIMP_GRADIENT_EDITOR (data);
  GimpGradient        *gradient;
  GimpGradientSegment *right;
  gint                 index = g_variant_get_int32 (value);

  gimp_gradient_editor_get_selection (editor, &gradient, NULL, &right);

  gimp_gradient_segment_get_right_color (gradient, right,
                                         &editor->saved_colors[index]);
}

void
gradient_editor_blending_func_cmd_callback (GimpAction *action,
                                            GVariant   *value,
                                            gpointer    data)
{
  GimpGradientEditor      *editor = GIMP_GRADIENT_EDITOR (data);
  GimpGradient            *gradient;
  GimpGradientSegment     *left;
  GimpGradientSegment     *right;
  GEnumClass              *enum_class = NULL;
  GimpGradientSegmentType  type;

  type = (GimpGradientSegmentType) g_variant_get_int32 (value);

  gimp_gradient_editor_get_selection (editor, &gradient, &left, &right);

  enum_class = g_type_class_ref (GIMP_TYPE_GRADIENT_SEGMENT_TYPE);

  if (gradient && g_enum_get_value (enum_class, type))
    {
      gimp_gradient_segment_range_set_blending_function (gradient,
                                                         left, right,
                                                         type);
    }

  g_type_class_unref (enum_class);
}

void
gradient_editor_coloring_type_cmd_callback (GimpAction *action,
                                            GVariant   *value,
                                            gpointer    data)
{
  GimpGradientEditor       *editor = GIMP_GRADIENT_EDITOR (data);
  GimpGradient             *gradient;
  GimpGradientSegment      *left;
  GimpGradientSegment      *right;
  GEnumClass               *enum_class = NULL;
  GimpGradientSegmentColor  color;

  color = (GimpGradientSegmentColor) g_variant_get_int32 (value);

  gimp_gradient_editor_get_selection (editor, &gradient, &left, &right);

  enum_class = g_type_class_ref (GIMP_TYPE_GRADIENT_SEGMENT_COLOR);

  if (gradient && g_enum_get_value (enum_class, color))
    {
      gimp_gradient_segment_range_set_coloring_type (gradient,
                                                     left, right,
                                                     color);
    }

  g_type_class_unref (enum_class);
}

void
gradient_editor_flip_cmd_callback (GimpAction *action,
                                   GVariant   *value,
                                   gpointer    data)
{
  GimpGradientEditor  *editor = GIMP_GRADIENT_EDITOR (data);
  GimpGradient        *gradient;
  GimpGradientSegment *left;
  GimpGradientSegment *right;

  gimp_gradient_editor_get_selection (editor, &gradient, &left, &right);

  gimp_gradient_segment_range_flip (gradient,
                                    left, right,
                                    &left, &right);

  gimp_gradient_editor_set_selection (editor, left, right);
}

void
gradient_editor_replicate_cmd_callback (GimpAction *action,
                                        GVariant   *value,
                                        gpointer    data)
{
  GimpGradientEditor  *editor      = GIMP_GRADIENT_EDITOR (data);
  GimpDataEditor      *data_editor = GIMP_DATA_EDITOR (data);
  GimpGradient        *gradient;
  GimpGradientSegment *left;
  GimpGradientSegment *right;
  GtkWidget           *dialog;
  GtkWidget           *vbox;
  GtkWidget           *label;
  GtkWidget           *scale;
  GtkAdjustment       *scale_data;
  const gchar         *title;
  const gchar         *desc;

  gimp_gradient_editor_get_selection (editor, &gradient, &left, &right);

  if (left == right)
    {
      title = _("Replicate Segment");
      desc  = _("Replicate Gradient Segment");
    }
  else
    {
      title = _("Replicate Selection");
      desc  = _("Replicate Gradient Selection");
    }

  dialog = gimp_viewable_dialog_new (GIMP_VIEWABLE (gradient),
                                     data_editor->context,
                                     title,
                                     "gimp-gradient-segment-replicate",
                                     GIMP_ICON_GRADIENT, desc,
                                     GTK_WIDGET (editor),
                                     gimp_standard_help_func,
                                     GIMP_HELP_GRADIENT_EDITOR_REPLICATE,

                                     _("_Cancel"),    GTK_RESPONSE_CANCEL,
                                     _("_Replicate"), GTK_RESPONSE_OK,

                                     NULL);

  gtk_dialog_set_alternative_button_order (GTK_DIALOG (dialog),
                                           GTK_RESPONSE_OK,
                                           GTK_RESPONSE_CANCEL,
                                           -1);

  g_signal_connect (dialog, "response",
                    G_CALLBACK (gradient_editor_replicate_response),
                    editor);

  vbox = gtk_box_new (GTK_ORIENTATION_VERTICAL, 12);
  gtk_container_set_border_width (GTK_CONTAINER (vbox), 12);
  gtk_box_pack_start (GTK_BOX (gtk_dialog_get_content_area (GTK_DIALOG (dialog))),
                      vbox, TRUE, TRUE, 0);
  gtk_widget_show (vbox);

  /*  Instructions  */
  if (left == right)
    label = gtk_label_new (_("Select the number of times\n"
                             "to replicate the selected segment."));
  else
    label = gtk_label_new (_("Select the number of times\n"
                             "to replicate the selection."));

  gtk_box_pack_start (GTK_BOX (vbox), label, FALSE, FALSE, 0);
  gtk_widget_show (label);

  /*  Scale  */
  scale_data = GTK_ADJUSTMENT (gtk_adjustment_new (2.0, 2.0, 21.0, 1.0, 1.0, 1.0));

  scale = gtk_scale_new (GTK_ORIENTATION_HORIZONTAL, scale_data);
  gtk_scale_set_digits (GTK_SCALE (scale), 0);
  gtk_scale_set_value_pos (GTK_SCALE (scale), GTK_POS_TOP);
  gtk_box_pack_start (GTK_BOX (vbox), scale, FALSE, TRUE, 4);
  gtk_widget_show (scale);

  g_object_set_data (G_OBJECT (dialog), "adjustment", scale_data);

  gtk_widget_set_sensitive (GTK_WIDGET (editor), FALSE);
  gimp_ui_manager_update (gimp_editor_get_ui_manager (GIMP_EDITOR (editor)),
                          gimp_editor_get_popup_data (GIMP_EDITOR (editor)));

  gtk_widget_show (dialog);
}

void
gradient_editor_split_midpoint_cmd_callback (GimpAction *action,
                                             GVariant   *value,
                                             gpointer    data)
{
  GimpGradientEditor  *editor      = GIMP_GRADIENT_EDITOR (data);
  GimpDataEditor      *data_editor = GIMP_DATA_EDITOR (data);
  GimpGradient        *gradient;
  GimpGradientSegment *left;
  GimpGradientSegment *right;

  gimp_gradient_editor_get_selection (editor, &gradient, &left, &right);

  gimp_gradient_segment_range_split_midpoint (gradient,
                                              data_editor->context,
                                              left, right,
                                              editor->blend_color_space,
                                              &left, &right);

  gimp_gradient_editor_set_selection (editor, left, right);
}

void
gradient_editor_split_uniformly_cmd_callback (GimpAction *action,
                                              GVariant   *value,
                                              gpointer    data)
{
  GimpGradientEditor  *editor      = GIMP_GRADIENT_EDITOR (data);
  GimpDataEditor      *data_editor = GIMP_DATA_EDITOR (data);
  GimpGradient        *gradient;
  GimpGradientSegment *left;
  GimpGradientSegment *right;
  GtkWidget           *dialog;
  GtkWidget           *vbox;
  GtkWidget           *label;
  GtkWidget           *scale;
  GtkAdjustment       *scale_data;
  const gchar         *title;
  const gchar         *desc;

  gimp_gradient_editor_get_selection (editor, &gradient, &left, &right);

  if (left == right)
    {
      title = _("Split Segment Uniformly");
      desc  = _("Split Gradient Segment Uniformly");
    }
  else
    {
      title = _("Split Segments Uniformly");
      desc  = _("Split Gradient Segments Uniformly");
    }

  dialog = gimp_viewable_dialog_new (GIMP_VIEWABLE (gradient),
                                     data_editor->context,
                                     title,
                                     "gimp-gradient-segment-split-uniformly",
                                     GIMP_ICON_GRADIENT, desc,
                                     GTK_WIDGET (editor),
                                     gimp_standard_help_func,
                                     GIMP_HELP_GRADIENT_EDITOR_SPLIT_UNIFORM,

                                     _("_Cancel"), GTK_RESPONSE_CANCEL,
                                     _("_Split"),  GTK_RESPONSE_OK,

                                     NULL);

  gtk_dialog_set_alternative_button_order (GTK_DIALOG (dialog),
                                           GTK_RESPONSE_OK,
                                           GTK_RESPONSE_CANCEL,
                                           -1);

  g_signal_connect (dialog, "response",
                    G_CALLBACK (gradient_editor_split_uniform_response),
                    editor);

  /*  The main vbox  */
  vbox = gtk_box_new (GTK_ORIENTATION_VERTICAL, 12);
  gtk_container_set_border_width (GTK_CONTAINER (vbox), 12);
  gtk_box_pack_start (GTK_BOX (gtk_dialog_get_content_area (GTK_DIALOG (dialog))),
                      vbox, TRUE, TRUE, 0);
  gtk_widget_show (vbox);

  /*  Instructions  */
  if (left == right)
    label = gtk_label_new (_("Select the number of uniform parts\n"
                             "in which to split the selected segment."));
  else
    label = gtk_label_new (_("Select the number of uniform parts\n"
                             "in which to split the segments in the selection."));

  gtk_box_pack_start (GTK_BOX (vbox), label, FALSE, FALSE, 0);
  gtk_widget_show (label);

  /*  Scale  */
  scale_data = GTK_ADJUSTMENT (gtk_adjustment_new (2.0, 2.0, 21.0, 1.0, 1.0, 1.0));

  scale = gtk_scale_new (GTK_ORIENTATION_HORIZONTAL, scale_data);
  gtk_scale_set_digits (GTK_SCALE (scale), 0);
  gtk_scale_set_value_pos (GTK_SCALE (scale), GTK_POS_TOP);
  gtk_box_pack_start (GTK_BOX (vbox), scale, FALSE, FALSE, 4);
  gtk_widget_show (scale);

  g_object_set_data (G_OBJECT (dialog), "adjustment", scale_data);

  gtk_widget_set_sensitive (GTK_WIDGET (editor), FALSE);
  gimp_ui_manager_update (gimp_editor_get_ui_manager (GIMP_EDITOR (editor)),
                          gimp_editor_get_popup_data (GIMP_EDITOR (editor)));

  gtk_widget_show (dialog);
}

void
gradient_editor_delete_cmd_callback (GimpAction *action,
                                     GVariant   *value,
                                     gpointer    data)
{
  GimpGradientEditor  *editor = GIMP_GRADIENT_EDITOR (data);
  GimpGradient        *gradient;
  GimpGradientSegment *left;
  GimpGradientSegment *right;

  gimp_gradient_editor_get_selection (editor, &gradient, &left, &right);

  gimp_gradient_segment_range_delete (gradient,
                                      left, right,
                                      &left, &right);

  gimp_gradient_editor_set_selection (editor, left, right);
}

void
gradient_editor_recenter_cmd_callback (GimpAction *action,
                                       GVariant   *value,
                                       gpointer    data)
{
  GimpGradientEditor  *editor = GIMP_GRADIENT_EDITOR (data);
  GimpGradient        *gradient;
  GimpGradientSegment *left;
  GimpGradientSegment *right;

  gimp_gradient_editor_get_selection (editor, &gradient, &left, &right);

  gimp_gradient_segment_range_recenter_handles (gradient, left, right);
}

void
gradient_editor_redistribute_cmd_callback (GimpAction *action,
                                           GVariant   *value,
                                           gpointer    data)
{
  GimpGradientEditor  *editor = GIMP_GRADIENT_EDITOR (data);
  GimpGradient        *gradient;
  GimpGradientSegment *left;
  GimpGradientSegment *right;

  gimp_gradient_editor_get_selection (editor, &gradient, &left, &right);

  gimp_gradient_segment_range_redistribute_handles (gradient, left, right);
}

void
gradient_editor_blend_color_cmd_callback (GimpAction *action,
                                          GVariant   *value,
                                          gpointer    data)
{
  GimpGradientEditor  *editor = GIMP_GRADIENT_EDITOR (data);
  GimpGradient        *gradient;
  GimpGradientSegment *left;
  GimpGradientSegment *right;

  gimp_gradient_editor_get_selection (editor, &gradient, &left, &right);

  gimp_gradient_segment_range_blend (gradient, left, right,
                                     &left->left_color,
                                     &right->right_color,
                                     TRUE, FALSE);
}

void
gradient_editor_blend_opacity_cmd_callback (GimpAction *action,
                                            GVariant   *value,
                                            gpointer    data)
{
  GimpGradientEditor  *editor = GIMP_GRADIENT_EDITOR (data);
  GimpGradient        *gradient;
  GimpGradientSegment *left;
  GimpGradientSegment *right;

  gimp_gradient_editor_get_selection (editor, &gradient, &left, &right);

  gimp_gradient_segment_range_blend (gradient, left, right,
                                     &left->left_color,
                                     &right->right_color,
                                     FALSE, TRUE);
}

void
gradient_editor_zoom_cmd_callback (GimpAction *action,
                                   GVariant   *value,
                                   gpointer    data)
{
  GimpGradientEditor *editor    = GIMP_GRADIENT_EDITOR (data);
  GimpZoomType        zoom_type = (GimpZoomType) g_variant_get_int32 (value);

  gimp_gradient_editor_zoom (editor, zoom_type);
}


/*  private functions  */

static void
gradient_editor_split_uniform_response (GtkWidget          *widget,
                                        gint                response_id,
                                        GimpGradientEditor *editor)
{
  GtkAdjustment *adjustment;
  gint           split_parts;

  adjustment = g_object_get_data (G_OBJECT (widget), "adjustment");

  split_parts = RINT (gtk_adjustment_get_value (adjustment));

  gtk_widget_destroy (widget);
  gtk_widget_set_sensitive (GTK_WIDGET (editor), TRUE);
  gimp_ui_manager_update (gimp_editor_get_ui_manager (GIMP_EDITOR (editor)),
                          gimp_editor_get_popup_data (GIMP_EDITOR (editor)));

  if (response_id == GTK_RESPONSE_OK)
    {
      GimpDataEditor      *data_editor = GIMP_DATA_EDITOR (editor);
      GimpGradient        *gradient;
      GimpGradientSegment *left;
      GimpGradientSegment *right;

      gimp_gradient_editor_get_selection (editor, &gradient, &left, &right);

      gimp_gradient_segment_range_split_uniform (gradient,
                                                 data_editor->context,
                                                 left, right,
                                                 split_parts,
                                                 editor->blend_color_space,
                                                 &left, &right);

      gimp_gradient_editor_set_selection (editor, left, right);
    }
}

static void
gradient_editor_replicate_response (GtkWidget          *widget,
                                    gint                response_id,
                                    GimpGradientEditor *editor)
{
  GtkAdjustment *adjustment;
  gint           replicate_times;

  adjustment = g_object_get_data (G_OBJECT (widget), "adjustment");

  replicate_times = RINT (gtk_adjustment_get_value (adjustment));

  gtk_widget_destroy (widget);
  gtk_widget_set_sensitive (GTK_WIDGET (editor), TRUE);
  gimp_ui_manager_update (gimp_editor_get_ui_manager (GIMP_EDITOR (editor)),
                          gimp_editor_get_popup_data (GIMP_EDITOR (editor)));

  if (response_id == GTK_RESPONSE_OK)
    {
      GimpGradient        *gradient;
      GimpGradientSegment *left;
      GimpGradientSegment *right;

      gimp_gradient_editor_get_selection (editor, &gradient, &left, &right);

      gimp_gradient_segment_range_replicate (gradient,
                                             left, right,
                                             replicate_times,
                                             &left, &right);

      gimp_gradient_editor_set_selection (editor, left, right);
    }
}
