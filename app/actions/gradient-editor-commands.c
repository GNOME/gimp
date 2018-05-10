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

#include "libgimpwidgets/gimpwidgets.h"

#include "actions-types.h"

#include "core/gimp.h"
#include "core/gimpcontext.h"
#include "core/gimpdatafactory.h"
#include "core/gimpgradient.h"

#include "widgets/gimpcolordialog.h"
#include "widgets/gimpdialogfactory.h"
#include "widgets/gimpgradienteditor.h"
#include "widgets/gimphelp-ids.h"
#include "widgets/gimpuimanager.h"
#include "widgets/gimpviewabledialog.h"

#include "gradient-editor-commands.h"

#include "gimp-intl.h"


/*  local function prototypes  */

static void   gradient_editor_left_color_update      (GimpColorDialog     *dialog,
                                                      const GimpRGB       *color,
                                                      GimpColorDialogState state,
                                                      GimpGradientEditor  *editor);
static void   gradient_editor_right_color_update     (GimpColorDialog     *dialog,
                                                      const GimpRGB       *color,
                                                      GimpColorDialogState state,
                                                      GimpGradientEditor  *editor);

static GimpGradientSegment *
              gradient_editor_save_selection         (GimpGradientEditor  *editor);
static void   gradient_editor_replace_selection      (GimpGradientEditor  *editor,
                                                      GimpGradientSegment *replace_seg);

static void   gradient_editor_split_uniform_response (GtkWidget           *widget,
                                                      gint                 response_id,
                                                      GimpGradientEditor  *editor);
static void   gradient_editor_replicate_response     (GtkWidget           *widget,
                                                      gint                 response_id,
                                                      GimpGradientEditor  *editor);


/*  public functions */

void
gradient_editor_left_color_cmd_callback (GtkAction *action,
                                         gpointer   data)
{
  GimpGradientEditor *editor = GIMP_GRADIENT_EDITOR (data);
  GimpGradient       *gradient;

  gradient = GIMP_GRADIENT (GIMP_DATA_EDITOR (editor)->data);

  editor->left_saved_dirty    = gimp_data_is_dirty (GIMP_DATA (gradient));
  editor->left_saved_segments = gradient_editor_save_selection (editor);

  editor->color_dialog =
    gimp_color_dialog_new (GIMP_VIEWABLE (gradient),
                           GIMP_DATA_EDITOR (editor)->context,
                           _("Left Endpoint Color"),
                           GIMP_ICON_GRADIENT,
                           _("Gradient Segment's Left Endpoint Color"),
                           GTK_WIDGET (editor),
                           gimp_dialog_factory_get_singleton (),
                           "gimp-gradient-editor-color-dialog",
                           &editor->control_sel_l->left_color,
                           TRUE, TRUE);

  g_signal_connect (editor->color_dialog, "destroy",
                    G_CALLBACK (gtk_widget_destroyed),
                    &editor->color_dialog);

  g_signal_connect (editor->color_dialog, "update",
                    G_CALLBACK (gradient_editor_left_color_update),
                    editor);

  gtk_widget_set_sensitive (GTK_WIDGET (editor), FALSE);
  gimp_ui_manager_update (gimp_editor_get_ui_manager (GIMP_EDITOR (editor)),
                          gimp_editor_get_popup_data (GIMP_EDITOR (editor)));

  gtk_window_present (GTK_WINDOW (editor->color_dialog));
}

void
gradient_editor_left_color_type_cmd_callback (GtkAction *action,
                                              GtkAction *current,
                                              gpointer   data)
{
  GimpGradientEditor *editor = GIMP_GRADIENT_EDITOR (data);
  GimpGradient       *gradient;
  GimpGradientColor   color_type;

  gradient = GIMP_GRADIENT (GIMP_DATA_EDITOR (editor)->data);

  color_type = gtk_radio_action_get_current_value (GTK_RADIO_ACTION (action));

  if (gradient        &&
      color_type >= 0 &&
      color_type !=
      gimp_gradient_segment_get_left_color_type (gradient,
                                                 editor->control_sel_l))
    {
      GimpRGB color;

      gimp_gradient_segment_get_left_flat_color (
        gradient, GIMP_DATA_EDITOR (editor)->context, editor->control_sel_l,
        &color);

      gimp_data_freeze (GIMP_DATA (gradient));

      gimp_gradient_segment_set_left_color_type (gradient,
                                                 editor->control_sel_l,
                                                 color_type);

      if (color_type == GIMP_GRADIENT_COLOR_FIXED)
        gimp_gradient_segment_set_left_color (gradient,
                                              editor->control_sel_l,
                                              &color);

      gimp_data_thaw (GIMP_DATA (gradient));
    }
}

void
gradient_editor_load_left_cmd_callback (GtkAction *action,
                                        gint       value,
                                        gpointer   data)
{
  GimpGradientEditor  *editor      = GIMP_GRADIENT_EDITOR (data);
  GimpDataEditor      *data_editor = GIMP_DATA_EDITOR (data);
  GimpGradient        *gradient;
  GimpGradientSegment *seg;
  GimpRGB              color;
  GimpGradientColor    color_type = GIMP_GRADIENT_COLOR_FIXED;

  gradient = GIMP_GRADIENT (data_editor->data);

  switch (value)
    {
    case GRADIENT_EDITOR_COLOR_NEIGHBOR_ENDPOINT:
      if (editor->control_sel_l->prev != NULL)
        seg = editor->control_sel_l->prev;
      else
        seg = gimp_gradient_segment_get_last (editor->control_sel_l);

      color      = seg->right_color;
      color_type = seg->right_color_type;
      break;

    case GRADIENT_EDITOR_COLOR_OTHER_ENDPOINT:
      color      = editor->control_sel_r->right_color;
      color_type = editor->control_sel_l->right_color_type;
      break;

    case GRADIENT_EDITOR_COLOR_FOREGROUND:
      gimp_context_get_foreground (data_editor->context, &color);
      break;

    case GRADIENT_EDITOR_COLOR_BACKGROUND:
      gimp_context_get_background (data_editor->context, &color);
      break;

    default: /* Load a color */
      color = editor->saved_colors[value - GRADIENT_EDITOR_COLOR_FIRST_CUSTOM];
      break;
    }

  gimp_data_freeze (GIMP_DATA (gradient));

  gimp_gradient_segment_range_blend (gradient,
                                     editor->control_sel_l,
                                     editor->control_sel_r,
                                     &color,
                                     &editor->control_sel_r->right_color,
                                     TRUE, TRUE);
  gimp_gradient_segment_set_left_color_type (gradient,
                                             editor->control_sel_l,
                                             color_type);

  gimp_data_thaw (GIMP_DATA (gradient));
}

void
gradient_editor_save_left_cmd_callback (GtkAction *action,
                                        gint       value,
                                        gpointer   data)
{
  GimpGradientEditor *editor = GIMP_GRADIENT_EDITOR (data);
  GimpGradient       *gradient;

  gradient = GIMP_GRADIENT (GIMP_DATA_EDITOR (editor)->data);

  gimp_gradient_segment_get_left_color (gradient, editor->control_sel_l,
                                        &editor->saved_colors[value]);
}

void
gradient_editor_right_color_cmd_callback (GtkAction *action,
                                          gpointer   data)
{
  GimpGradientEditor *editor = GIMP_GRADIENT_EDITOR (data);
  GimpGradient       *gradient;

  gradient = GIMP_GRADIENT (GIMP_DATA_EDITOR (editor)->data);

  editor->right_saved_dirty    = gimp_data_is_dirty (GIMP_DATA (gradient));
  editor->right_saved_segments = gradient_editor_save_selection (editor);

  editor->color_dialog =
    gimp_color_dialog_new (GIMP_VIEWABLE (gradient),
                           GIMP_DATA_EDITOR (editor)->context,
                           _("Right Endpoint Color"),
                           GIMP_ICON_GRADIENT,
                           _("Gradient Segment's Right Endpoint Color"),
                           GTK_WIDGET (editor),
                           gimp_dialog_factory_get_singleton (),
                           "gimp-gradient-editor-color-dialog",
                           &editor->control_sel_l->right_color,
                           TRUE, TRUE);

  g_signal_connect (editor->color_dialog, "destroy",
                    G_CALLBACK (gtk_widget_destroyed),
                    &editor->color_dialog);

  g_signal_connect (editor->color_dialog, "update",
                    G_CALLBACK (gradient_editor_right_color_update),
                    editor);

  gtk_widget_set_sensitive (GTK_WIDGET (editor), FALSE);
  gimp_ui_manager_update (gimp_editor_get_ui_manager (GIMP_EDITOR (editor)),
                          gimp_editor_get_popup_data (GIMP_EDITOR (editor)));

  gtk_window_present (GTK_WINDOW (editor->color_dialog));
}

void
gradient_editor_right_color_type_cmd_callback (GtkAction *action,
                                               GtkAction *current,
                                               gpointer   data)
{
  GimpGradientEditor *editor = GIMP_GRADIENT_EDITOR (data);
  GimpGradient       *gradient;
  GimpGradientColor   color_type;

  gradient = GIMP_GRADIENT (GIMP_DATA_EDITOR (editor)->data);

  color_type = gtk_radio_action_get_current_value (GTK_RADIO_ACTION (action));

  if (gradient        &&
      color_type >= 0 &&
      color_type !=
      gimp_gradient_segment_get_right_color_type (gradient,
                                                  editor->control_sel_r))
    {
      GimpRGB color;

      gimp_gradient_segment_get_right_flat_color (
        gradient, GIMP_DATA_EDITOR (editor)->context, editor->control_sel_r,
        &color);

      gimp_data_freeze (GIMP_DATA (gradient));

      gimp_gradient_segment_set_right_color_type (gradient,
                                                  editor->control_sel_r,
                                                  color_type);

      if (color_type == GIMP_GRADIENT_COLOR_FIXED)
        gimp_gradient_segment_set_right_color (gradient,
                                               editor->control_sel_r,
                                               &color);

      gimp_data_thaw (GIMP_DATA (gradient));
    }
}

void
gradient_editor_load_right_cmd_callback (GtkAction *action,
                                         gint       value,
                                         gpointer   data)
{
  GimpGradientEditor  *editor      = GIMP_GRADIENT_EDITOR (data);
  GimpDataEditor      *data_editor = GIMP_DATA_EDITOR (data);
  GimpGradient        *gradient;
  GimpGradientSegment *seg;
  GimpRGB              color;
  GimpGradientColor    color_type = GIMP_GRADIENT_COLOR_FIXED;

  gradient = GIMP_GRADIENT (data_editor->data);

  switch (value)
    {
    case GRADIENT_EDITOR_COLOR_NEIGHBOR_ENDPOINT:
      if (editor->control_sel_r->next != NULL)
        seg = editor->control_sel_r->next;
      else
        seg = gimp_gradient_segment_get_first (editor->control_sel_r);

      color      = seg->left_color;
      color_type = seg->left_color_type;
      break;

    case GRADIENT_EDITOR_COLOR_OTHER_ENDPOINT:
      color      = editor->control_sel_l->left_color;
      color_type = editor->control_sel_l->left_color_type;
      break;

    case GRADIENT_EDITOR_COLOR_FOREGROUND:
      gimp_context_get_foreground (data_editor->context, &color);
      break;

    case GRADIENT_EDITOR_COLOR_BACKGROUND:
      gimp_context_get_background (data_editor->context, &color);
      break;

    default: /* Load a color */
      color = editor->saved_colors[value - GRADIENT_EDITOR_COLOR_FIRST_CUSTOM];
      break;
    }

  gimp_data_freeze (GIMP_DATA (gradient));

  gimp_gradient_segment_range_blend (gradient,
                                     editor->control_sel_l,
                                     editor->control_sel_r,
                                     &editor->control_sel_l->left_color,
                                     &color,
                                     TRUE, TRUE);
  gimp_gradient_segment_set_right_color_type (gradient,
                                              editor->control_sel_l,
                                              color_type);

  gimp_data_thaw (GIMP_DATA (gradient));
}

void
gradient_editor_save_right_cmd_callback (GtkAction *action,
                                         gint       value,
                                         gpointer   data)
{
  GimpGradientEditor *editor = GIMP_GRADIENT_EDITOR (data);
  GimpGradient       *gradient;

  gradient = GIMP_GRADIENT (GIMP_DATA_EDITOR (editor)->data);

  gimp_gradient_segment_get_right_color (gradient, editor->control_sel_r,
                                         &editor->saved_colors[value]);
}

void
gradient_editor_blending_func_cmd_callback (GtkAction *action,
                                            GtkAction *current,
                                            gpointer   data)
{
  GimpGradientEditor      *editor = GIMP_GRADIENT_EDITOR (data);
  GimpGradient            *gradient;
  GEnumClass              *enum_class = NULL;
  GimpGradientSegmentType  type;

  gradient = GIMP_GRADIENT (GIMP_DATA_EDITOR (editor)->data);

  type = gtk_radio_action_get_current_value (GTK_RADIO_ACTION (action));

  enum_class = g_type_class_ref (GIMP_TYPE_GRADIENT_SEGMENT_TYPE);

  if (gradient && g_enum_get_value (enum_class, type))
    {
      gimp_gradient_segment_range_set_blending_function (gradient,
                                                         editor->control_sel_l,
                                                         editor->control_sel_r,
                                                         type);
    }

  g_type_class_unref (enum_class);
}

void
gradient_editor_coloring_type_cmd_callback (GtkAction *action,
                                            GtkAction *current,
                                            gpointer   data)
{
  GimpGradientEditor       *editor = GIMP_GRADIENT_EDITOR (data);
  GimpGradient             *gradient;
  GEnumClass               *enum_class = NULL;
  GimpGradientSegmentColor  color;

  gradient = GIMP_GRADIENT (GIMP_DATA_EDITOR (editor)->data);

  color = gtk_radio_action_get_current_value (GTK_RADIO_ACTION (action));

  enum_class = g_type_class_ref (GIMP_TYPE_GRADIENT_SEGMENT_COLOR);

  if (gradient && g_enum_get_value (enum_class, color))
    {
      gimp_gradient_segment_range_set_coloring_type (gradient,
                                                     editor->control_sel_l,
                                                     editor->control_sel_r,
                                                     color);
    }

  g_type_class_unref (enum_class);
}

void
gradient_editor_flip_cmd_callback (GtkAction *action,
                                   gpointer   data)
{
  GimpGradientEditor *editor = GIMP_GRADIENT_EDITOR (data);
  GimpGradient       *gradient;

  gradient = GIMP_GRADIENT (GIMP_DATA_EDITOR (editor)->data);

  gimp_gradient_segment_range_flip (gradient,
                                    editor->control_sel_l,
                                    editor->control_sel_r,
                                    &editor->control_sel_l,
                                    &editor->control_sel_r);
}

void
gradient_editor_replicate_cmd_callback (GtkAction *action,
                                        gpointer   data)
{
  GimpGradientEditor *editor      = GIMP_GRADIENT_EDITOR (data);
  GimpDataEditor     *data_editor = GIMP_DATA_EDITOR (data);
  GtkWidget          *dialog;
  GtkWidget          *vbox;
  GtkWidget          *label;
  GtkWidget          *scale;
  GtkAdjustment      *scale_data;
  const gchar        *title;
  const gchar        *desc;

  if (editor->control_sel_l == editor->control_sel_r)
    {
      title = _("Replicate Segment");
      desc  = _("Replicate Gradient Segment");
    }
  else
    {
      title = _("Replicate Selection");
      desc  = _("Replicate Gradient Selection");
    }

  dialog =
    gimp_viewable_dialog_new (GIMP_VIEWABLE (data_editor->data),
                              data_editor->context,
                              title, "gimp-gradient-segment-replicate",
                              GIMP_ICON_GRADIENT, desc,
                              GTK_WIDGET (editor),
                              gimp_standard_help_func,
                              GIMP_HELP_GRADIENT_EDITOR_REPLICATE,

                              _("_Cancel"),    GTK_RESPONSE_CANCEL,
                              _("_Replicate"), GTK_RESPONSE_OK,

                              NULL);

  gimp_dialog_set_alternative_button_order (GTK_DIALOG (dialog),
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
  if (editor->control_sel_l == editor->control_sel_r)
    label = gtk_label_new (_("Select the number of times\n"
                             "to replicate the selected segment."));
  else
    label = gtk_label_new (_("Select the number of times\n"
                             "to replicate the selection."));

  gtk_box_pack_start (GTK_BOX (vbox), label, FALSE, FALSE, 0);
  gtk_widget_show (label);

  /*  Scale  */
  editor->replicate_times = 2;
  scale_data  = GTK_ADJUSTMENT (gtk_adjustment_new (2.0, 2.0, 21.0, 1.0, 1.0, 1.0));

  scale = gtk_scale_new (GTK_ORIENTATION_HORIZONTAL, scale_data);
  gtk_scale_set_digits (GTK_SCALE (scale), 0);
  gtk_scale_set_value_pos (GTK_SCALE (scale), GTK_POS_TOP);
  gtk_box_pack_start (GTK_BOX (vbox), scale, FALSE, TRUE, 4);
  gtk_widget_show (scale);

  g_signal_connect (scale_data, "value-changed",
                    G_CALLBACK (gimp_int_adjustment_update),
                    &editor->replicate_times);

  gtk_widget_set_sensitive (GTK_WIDGET (editor), FALSE);
  gimp_ui_manager_update (gimp_editor_get_ui_manager (GIMP_EDITOR (editor)),
                          gimp_editor_get_popup_data (GIMP_EDITOR (editor)));

  gtk_widget_show (dialog);
}

void
gradient_editor_split_midpoint_cmd_callback (GtkAction *action,
                                             gpointer   data)
{
  GimpGradientEditor *editor      = GIMP_GRADIENT_EDITOR (data);
  GimpDataEditor     *data_editor = GIMP_DATA_EDITOR (data);
  GimpGradient       *gradient    = GIMP_GRADIENT (data_editor->data);

  gimp_gradient_segment_range_split_midpoint (gradient,
                                              data_editor->context,
                                              editor->control_sel_l,
                                              editor->control_sel_r,
                                              editor->blend_color_space,
                                              &editor->control_sel_l,
                                              &editor->control_sel_r);
}

void
gradient_editor_split_uniformly_cmd_callback (GtkAction *action,
                                              gpointer   data)
{
  GimpGradientEditor *editor      = GIMP_GRADIENT_EDITOR (data);
  GimpDataEditor     *data_editor = GIMP_DATA_EDITOR (data);
  GtkWidget          *dialog;
  GtkWidget          *vbox;
  GtkWidget          *label;
  GtkWidget          *scale;
  GtkAdjustment      *scale_data;
  const gchar        *title;
  const gchar        *desc;

  if (editor->control_sel_l == editor->control_sel_r)
    {
      title = _("Split Segment Uniformly");
      desc  = _("Split Gradient Segment Uniformly");
    }
  else
    {
      title = _("Split Segments Uniformly");
      desc  = _("Split Gradient Segments Uniformly");
    }

  dialog =
    gimp_viewable_dialog_new (GIMP_VIEWABLE (data_editor->data),
                              data_editor->context,
                              title, "gimp-gradient-segment-split-uniformly",
                              GIMP_ICON_GRADIENT, desc,
                              GTK_WIDGET (editor),
                              gimp_standard_help_func,
                              GIMP_HELP_GRADIENT_EDITOR_SPLIT_UNIFORM,

                              _("_Cancel"), GTK_RESPONSE_CANCEL,
                              _("_Split"),  GTK_RESPONSE_OK,

                              NULL);

  gimp_dialog_set_alternative_button_order (GTK_DIALOG (dialog),
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
  if (editor->control_sel_l == editor->control_sel_r)
    label = gtk_label_new (_("Select the number of uniform parts\n"
                             "in which to split the selected segment."));
  else
    label = gtk_label_new (_("Select the number of uniform parts\n"
                             "in which to split the segments in the selection."));

  gtk_box_pack_start (GTK_BOX (vbox), label, FALSE, FALSE, 0);
  gtk_widget_show (label);

  /*  Scale  */
  editor->split_parts = 2;
  scale_data = GTK_ADJUSTMENT (gtk_adjustment_new (2.0, 2.0, 21.0, 1.0, 1.0, 1.0));

  scale = gtk_scale_new (GTK_ORIENTATION_HORIZONTAL, scale_data);
  gtk_scale_set_digits (GTK_SCALE (scale), 0);
  gtk_scale_set_value_pos (GTK_SCALE (scale), GTK_POS_TOP);
  gtk_box_pack_start (GTK_BOX (vbox), scale, FALSE, FALSE, 4);
  gtk_widget_show (scale);

  g_signal_connect (scale_data, "value-changed",
                    G_CALLBACK (gimp_int_adjustment_update),
                    &editor->split_parts);

  gtk_widget_set_sensitive (GTK_WIDGET (editor), FALSE);
  gimp_ui_manager_update (gimp_editor_get_ui_manager (GIMP_EDITOR (editor)),
                          gimp_editor_get_popup_data (GIMP_EDITOR (editor)));

  gtk_widget_show (dialog);
}

void
gradient_editor_delete_cmd_callback (GtkAction *action,
                                     gpointer   data)
{
  GimpGradientEditor *editor = GIMP_GRADIENT_EDITOR (data);
  GimpGradient       *gradient;

  gradient = GIMP_GRADIENT (GIMP_DATA_EDITOR (editor)->data);

  gimp_gradient_segment_range_delete (gradient,
                                      editor->control_sel_l,
                                      editor->control_sel_r,
                                      &editor->control_sel_l,
                                      &editor->control_sel_r);
}

void
gradient_editor_recenter_cmd_callback (GtkAction *action,
                                       gpointer   data)
{
  GimpGradientEditor *editor = GIMP_GRADIENT_EDITOR (data);
  GimpGradient       *gradient;

  gradient = GIMP_GRADIENT (GIMP_DATA_EDITOR (editor)->data);

  gimp_gradient_segment_range_recenter_handles (gradient,
                                                editor->control_sel_l,
                                                editor->control_sel_r);
}

void
gradient_editor_redistribute_cmd_callback (GtkAction *action,
                                           gpointer   data)
{
  GimpGradientEditor *editor = GIMP_GRADIENT_EDITOR (data);
  GimpGradient       *gradient;

  gradient = GIMP_GRADIENT (GIMP_DATA_EDITOR (editor)->data);

  gimp_gradient_segment_range_redistribute_handles (gradient,
                                                    editor->control_sel_l,
                                                    editor->control_sel_r);
}

void
gradient_editor_blend_color_cmd_callback (GtkAction *action,
                                          gpointer   data)
{
  GimpGradientEditor *editor = GIMP_GRADIENT_EDITOR (data);
  GimpGradient       *gradient;

  gradient = GIMP_GRADIENT (GIMP_DATA_EDITOR (editor)->data);

  gimp_gradient_segment_range_blend (gradient,
                                     editor->control_sel_l,
                                     editor->control_sel_r,
                                     &editor->control_sel_l->left_color,
                                     &editor->control_sel_r->right_color,
                                     TRUE, FALSE);
}

void
gradient_editor_blend_opacity_cmd_callback (GtkAction *action,
                                            gpointer   data)
{
  GimpGradientEditor *editor = GIMP_GRADIENT_EDITOR (data);
  GimpGradient       *gradient;

  gradient = GIMP_GRADIENT (GIMP_DATA_EDITOR (editor)->data);

  gimp_gradient_segment_range_blend (gradient,
                                     editor->control_sel_l,
                                     editor->control_sel_r,
                                     &editor->control_sel_l->left_color,
                                     &editor->control_sel_r->right_color,
                                     FALSE, TRUE);
}

void
gradient_editor_zoom_cmd_callback (GtkAction *action,
                                   gint       value,
                                   gpointer   data)
{
  GimpGradientEditor *editor = GIMP_GRADIENT_EDITOR (data);

  gimp_gradient_editor_zoom (editor, (GimpZoomType) value);
}


/*  private functions  */

static void
gradient_editor_left_color_update (GimpColorDialog      *dialog,
                                   const GimpRGB        *color,
                                   GimpColorDialogState  state,
                                   GimpGradientEditor   *editor)
{
  GimpGradient *gradient = GIMP_GRADIENT (GIMP_DATA_EDITOR (editor)->data);

  switch (state)
    {
    case GIMP_COLOR_DIALOG_UPDATE:
      gimp_gradient_segment_range_blend (gradient,
                                         editor->control_sel_l,
                                         editor->control_sel_r,
                                         color,
                                         &editor->control_sel_r->right_color,
                                         TRUE, TRUE);
      break;

    case GIMP_COLOR_DIALOG_OK:
      gimp_gradient_segment_range_blend (gradient,
                                         editor->control_sel_l,
                                         editor->control_sel_r,
                                         color,
                                         &editor->control_sel_r->right_color,
                                         TRUE, TRUE);
      gimp_gradient_segments_free (editor->left_saved_segments);
      gtk_widget_destroy (editor->color_dialog);
      editor->color_dialog = NULL;
      gtk_widget_set_sensitive (GTK_WIDGET (editor), TRUE);
      gimp_ui_manager_update (gimp_editor_get_ui_manager (GIMP_EDITOR (editor)),
                              gimp_editor_get_popup_data (GIMP_EDITOR (editor)));
      break;

    case GIMP_COLOR_DIALOG_CANCEL:
      gradient_editor_replace_selection (editor, editor->left_saved_segments);
      if (! editor->left_saved_dirty)
        gimp_data_clean (GIMP_DATA (gradient));
      gimp_viewable_invalidate_preview (GIMP_VIEWABLE (gradient));
      gtk_widget_destroy (editor->color_dialog);
      editor->color_dialog = NULL;
      gtk_widget_set_sensitive (GTK_WIDGET (editor), TRUE);
      gimp_ui_manager_update (gimp_editor_get_ui_manager (GIMP_EDITOR (editor)),
                              gimp_editor_get_popup_data (GIMP_EDITOR (editor)));
      break;
    }
}

static void
gradient_editor_right_color_update (GimpColorDialog      *dialog,
                                    const GimpRGB        *color,
                                    GimpColorDialogState  state,
                                    GimpGradientEditor   *editor)
{
  GimpGradient *gradient = GIMP_GRADIENT (GIMP_DATA_EDITOR (editor)->data);

  switch (state)
    {
    case GIMP_COLOR_DIALOG_UPDATE:
      gimp_gradient_segment_range_blend (gradient,
                                         editor->control_sel_l,
                                         editor->control_sel_r,
                                         &editor->control_sel_l->left_color,
                                         color,
                                         TRUE, TRUE);
      break;

    case GIMP_COLOR_DIALOG_OK:
      gimp_gradient_segment_range_blend (gradient,
                                         editor->control_sel_l,
                                         editor->control_sel_r,
                                         &editor->control_sel_l->left_color,
                                         color,
                                         TRUE, TRUE);
      gimp_gradient_segments_free (editor->right_saved_segments);
      gtk_widget_destroy (editor->color_dialog);
      editor->color_dialog = NULL;
      gtk_widget_set_sensitive (GTK_WIDGET (editor), TRUE);
      gimp_ui_manager_update (gimp_editor_get_ui_manager (GIMP_EDITOR (editor)),
                              gimp_editor_get_popup_data (GIMP_EDITOR (editor)));
      break;

    case GIMP_COLOR_DIALOG_CANCEL:
      gradient_editor_replace_selection (editor, editor->right_saved_segments);
      if (! editor->right_saved_dirty)
        gimp_data_clean (GIMP_DATA (gradient));
      gimp_viewable_invalidate_preview (GIMP_VIEWABLE (gradient));
      gtk_widget_destroy (editor->color_dialog);
      editor->color_dialog = NULL;
      gtk_widget_set_sensitive (GTK_WIDGET (editor), TRUE);
      gimp_ui_manager_update (gimp_editor_get_ui_manager (GIMP_EDITOR (editor)),
                              gimp_editor_get_popup_data (GIMP_EDITOR (editor)));
      break;
    }
}

static GimpGradientSegment *
gradient_editor_save_selection (GimpGradientEditor *editor)
{
  GimpGradientSegment *seg, *prev, *tmp;
  GimpGradientSegment *oseg, *oaseg;

  prev = NULL;
  oseg = editor->control_sel_l;
  tmp  = NULL;

  do
    {
      seg = gimp_gradient_segment_new ();

      *seg = *oseg; /* Copy everything */

      if (prev == NULL)
        tmp = seg; /* Remember first segment */
      else
        prev->next = seg;

      seg->prev = prev;
      seg->next = NULL;

      prev  = seg;
      oaseg = oseg;
      oseg  = oseg->next;
    }
  while (oaseg != editor->control_sel_r);

  return tmp;
}

static void
gradient_editor_replace_selection (GimpGradientEditor  *editor,
                                   GimpGradientSegment *replace_seg)
{
  GimpGradient        *gradient;
  GimpGradientSegment *lseg, *rseg;
  GimpGradientSegment *replace_last;

  gradient = GIMP_GRADIENT (GIMP_DATA_EDITOR (editor)->data);

  /* Remember left and right segments */

  lseg = editor->control_sel_l->prev;
  rseg = editor->control_sel_r->next;

  replace_last = gimp_gradient_segment_get_last (replace_seg);

  /* Free old selection */

  editor->control_sel_r->next = NULL;

  gimp_gradient_segments_free (editor->control_sel_l);

  /* Link in new segments */

  if (lseg)
    lseg->next = replace_seg;
  else
    gradient->segments = replace_seg;

  replace_seg->prev = lseg;

  if (rseg)
    rseg->prev = replace_last;

  replace_last->next = rseg;

  editor->control_sel_l = replace_seg;
  editor->control_sel_r = replace_last;
}

static void
gradient_editor_split_uniform_response (GtkWidget          *widget,
                                        gint                response_id,
                                        GimpGradientEditor *editor)
{
  gtk_widget_destroy (widget);
  gtk_widget_set_sensitive (GTK_WIDGET (editor), TRUE);
  gimp_ui_manager_update (gimp_editor_get_ui_manager (GIMP_EDITOR (editor)),
                          gimp_editor_get_popup_data (GIMP_EDITOR (editor)));

  if (response_id == GTK_RESPONSE_OK)
    {
      GimpDataEditor *data_editor = GIMP_DATA_EDITOR (editor);
      GimpGradient   *gradient    = GIMP_GRADIENT (data_editor->data);

      gimp_gradient_segment_range_split_uniform (gradient,
                                                 data_editor->context,
                                                 editor->control_sel_l,
                                                 editor->control_sel_r,
                                                 editor->split_parts,
                                                 editor->blend_color_space,
                                                 &editor->control_sel_l,
                                                 &editor->control_sel_r);
    }
}

static void
gradient_editor_replicate_response (GtkWidget          *widget,
                                    gint                response_id,
                                    GimpGradientEditor *editor)
{
  gtk_widget_destroy (widget);
  gtk_widget_set_sensitive (GTK_WIDGET (editor), TRUE);
  gimp_ui_manager_update (gimp_editor_get_ui_manager (GIMP_EDITOR (editor)),
                          gimp_editor_get_popup_data (GIMP_EDITOR (editor)));

  if (response_id == GTK_RESPONSE_OK)
    {
      GimpGradient *gradient = GIMP_GRADIENT (GIMP_DATA_EDITOR (editor)->data);

      gimp_gradient_segment_range_replicate (gradient,
                                             editor->control_sel_l,
                                             editor->control_sel_r,
                                             editor->replicate_times,
                                             &editor->control_sel_l,
                                             &editor->control_sel_r);
    }
}
