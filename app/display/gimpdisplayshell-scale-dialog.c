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

#include "display-types.h"

#include "core/gimp.h"
#include "core/gimpviewable.h"

#include "widgets/gimphelp-ids.h"
#include "widgets/gimpviewabledialog.h"

#include "gimpdisplay.h"
#include "gimpdisplayshell.h"
#include "gimpdisplayshell-scale.h"
#include "gimpdisplayshell-scale-dialog.h"

#include "gimp-intl.h"


#define SCALE_EPSILON      0.0001
#define SCALE_EQUALS(a,b) (fabs ((a) - (b)) < SCALE_EPSILON)


typedef struct
{
  GimpDisplayShell *shell;
  GimpZoomModel    *model;
  GtkAdjustment    *scale_adj;
  GtkAdjustment    *num_adj;
  GtkAdjustment    *denom_adj;
} ScaleDialogData;


/*  local function prototypes  */

static void  gimp_display_shell_scale_dialog_response (GtkWidget        *widget,
                                                       gint              response_id,
                                                       ScaleDialogData  *dialog);
static void  gimp_display_shell_scale_dialog_free     (ScaleDialogData  *dialog);

static void  update_zoom_values                       (GtkAdjustment    *adj,
                                                       ScaleDialogData  *dialog);



/*  public functions  */

/**
 * gimp_display_shell_scale_dialog:
 * @shell: the #GimpDisplayShell
 *
 * Constructs and displays a dialog allowing the user to enter a
 * custom display scale.
 **/
void
gimp_display_shell_scale_dialog (GimpDisplayShell *shell)
{
  ScaleDialogData *data;
  GimpImage       *image;
  GtkWidget       *toplevel;
  GtkWidget       *hbox;
  GtkWidget       *table;
  GtkWidget       *spin;
  GtkWidget       *label;
  gint             num, denom, row;

  g_return_if_fail (GIMP_IS_DISPLAY_SHELL (shell));

  if (shell->scale_dialog)
    {
      gtk_window_present (GTK_WINDOW (shell->scale_dialog));
      return;
    }

  if (SCALE_EQUALS (shell->other_scale, 0.0))
    {
      /* other_scale not yet initialized */
      shell->other_scale = gimp_zoom_model_get_factor (shell->zoom);
    }

  image = gimp_display_get_image (shell->display);

  data = g_slice_new (ScaleDialogData);

  data->shell = shell;
  data->model = g_object_new (GIMP_TYPE_ZOOM_MODEL,
                              "value", fabs (shell->other_scale),
                              NULL);

  shell->scale_dialog =
    gimp_viewable_dialog_new (GIMP_VIEWABLE (image),
                              gimp_get_user_context (shell->display->gimp),
                              _("Zoom Ratio"), "display_scale",
                              "zoom-original",
                              _("Select Zoom Ratio"),
                              GTK_WIDGET (shell),
                              gimp_standard_help_func,
                              GIMP_HELP_VIEW_ZOOM_OTHER,

                              _("_Cancel"), GTK_RESPONSE_CANCEL,
                              _("_OK"),     GTK_RESPONSE_OK,

                              NULL);

  gtk_dialog_set_alternative_button_order (GTK_DIALOG (shell->scale_dialog),
                                           GTK_RESPONSE_OK,
                                           GTK_RESPONSE_CANCEL,
                                           -1);

  g_object_weak_ref (G_OBJECT (shell->scale_dialog),
                     (GWeakNotify) gimp_display_shell_scale_dialog_free, data);
  g_object_weak_ref (G_OBJECT (shell->scale_dialog),
                     (GWeakNotify) g_object_unref, data->model);

  g_object_add_weak_pointer (G_OBJECT (shell->scale_dialog),
                             (gpointer) &shell->scale_dialog);

  toplevel = gtk_widget_get_toplevel (GTK_WIDGET (shell));

  gtk_window_set_transient_for (GTK_WINDOW (shell->scale_dialog),
                                GTK_WINDOW (toplevel));
  gtk_window_set_destroy_with_parent (GTK_WINDOW (shell->scale_dialog), TRUE);

  g_signal_connect (shell->scale_dialog, "response",
                    G_CALLBACK (gimp_display_shell_scale_dialog_response),
                    data);

  table = gtk_table_new (2, 2, FALSE);
  gtk_container_set_border_width (GTK_CONTAINER (table), 12);
  gtk_table_set_col_spacings (GTK_TABLE (table), 6);
  gtk_table_set_row_spacings (GTK_TABLE (table), 6);
  gtk_box_pack_start (GTK_BOX (gtk_dialog_get_content_area (GTK_DIALOG (shell->scale_dialog))),
                      table, TRUE, TRUE, 0);
  gtk_widget_show (table);

  row = 0;

  hbox = gtk_box_new (GTK_ORIENTATION_HORIZONTAL, 6);
  gimp_table_attach_aligned (GTK_TABLE (table), 0, row++,
                             _("Zoom ratio:"), 0.0, 0.5,
                             hbox, 1, FALSE);

  gimp_zoom_model_get_fraction (data->model, &num, &denom);

  data->num_adj = gtk_adjustment_new (num, 1, 256, 1, 8, 0);
  spin = gtk_spin_button_new (data->num_adj, 1.0, 0);
  gtk_spin_button_set_numeric (GTK_SPIN_BUTTON (spin), TRUE);
  gtk_entry_set_activates_default (GTK_ENTRY (spin), TRUE);
  gtk_box_pack_start (GTK_BOX (hbox), spin, TRUE, TRUE, 0);
  gtk_widget_show (spin);

  label = gtk_label_new (":");
  gtk_box_pack_start (GTK_BOX (hbox), label, FALSE, FALSE, 0);
  gtk_widget_show (label);

  data->denom_adj = gtk_adjustment_new (denom, 1, 256, 1, 8, 0);
  spin = gtk_spin_button_new (data->denom_adj, 1.0, 0);
  gtk_spin_button_set_numeric (GTK_SPIN_BUTTON (spin), TRUE);
  gtk_entry_set_activates_default (GTK_ENTRY (spin), TRUE);
  gtk_box_pack_start (GTK_BOX (hbox), spin, TRUE, TRUE, 0);
  gtk_widget_show (spin);

  hbox = gtk_box_new (GTK_ORIENTATION_HORIZONTAL, 6);
  gimp_table_attach_aligned (GTK_TABLE (table), 0, row++,
                             _("Zoom:"), 0.0, 0.5,
                             hbox, 1, FALSE);

  data->scale_adj = gtk_adjustment_new (fabs (shell->other_scale) * 100,
                                        100.0 / 256.0, 25600.0,
                                        10, 50, 0);
  spin = gtk_spin_button_new (data->scale_adj, 1.0, 2);
  gtk_spin_button_set_numeric (GTK_SPIN_BUTTON (spin), TRUE);
  gtk_entry_set_activates_default (GTK_ENTRY (spin), TRUE);
  gtk_box_pack_start (GTK_BOX (hbox), spin, TRUE, TRUE, 0);
  gtk_widget_show (spin);

  label = gtk_label_new ("%");
  gtk_box_pack_start (GTK_BOX (hbox), label, FALSE, FALSE, 0);
  gtk_widget_show (label);

  g_signal_connect (data->scale_adj, "value-changed",
                    G_CALLBACK (update_zoom_values), data);
  g_signal_connect (data->num_adj, "value-changed",
                    G_CALLBACK (update_zoom_values), data);
  g_signal_connect (data->denom_adj, "value-changed",
                    G_CALLBACK (update_zoom_values), data);

  gtk_widget_show (shell->scale_dialog);
}

static void
gimp_display_shell_scale_dialog_response (GtkWidget       *widget,
                                          gint             response_id,
                                          ScaleDialogData *dialog)
{
  if (response_id == GTK_RESPONSE_OK)
    {
      gdouble scale;

      scale = gtk_adjustment_get_value (dialog->scale_adj);

      gimp_display_shell_scale (dialog->shell,
                                GIMP_ZOOM_TO,
                                scale / 100.0,
                                GIMP_ZOOM_FOCUS_BEST_GUESS);
    }
  else
    {
      /*  need to emit "scaled" to get the menu updated  */
      gimp_display_shell_scaled (dialog->shell);
    }

  dialog->shell->other_scale = - fabs (dialog->shell->other_scale);

  gtk_widget_destroy (dialog->shell->scale_dialog);
}

static void
gimp_display_shell_scale_dialog_free (ScaleDialogData *dialog)
{
  g_slice_free (ScaleDialogData, dialog);
}

static void
update_zoom_values (GtkAdjustment   *adj,
                    ScaleDialogData *dialog)
{
  gint    num, denom;
  gdouble scale;

  g_signal_handlers_block_by_func (dialog->scale_adj,
                                   update_zoom_values,
                                   dialog);
  g_signal_handlers_block_by_func (dialog->num_adj,
                                   update_zoom_values,
                                   dialog);
  g_signal_handlers_block_by_func (dialog->denom_adj,
                                   update_zoom_values,
                                   dialog);

  if (adj == dialog->scale_adj)
    {
      scale = gtk_adjustment_get_value (dialog->scale_adj);

      gimp_zoom_model_zoom (dialog->model, GIMP_ZOOM_TO, scale / 100.0);
      gimp_zoom_model_get_fraction (dialog->model, &num, &denom);

      gtk_adjustment_set_value (dialog->num_adj, num);
      gtk_adjustment_set_value (dialog->denom_adj, denom);
    }
  else /* fraction adjustments */
    {
      scale = (gtk_adjustment_get_value (dialog->num_adj) /
               gtk_adjustment_get_value (dialog->denom_adj));

      gtk_adjustment_set_value (dialog->scale_adj, scale * 100);
    }

  g_signal_handlers_unblock_by_func (dialog->scale_adj,
                                     update_zoom_values,
                                     dialog);
  g_signal_handlers_unblock_by_func (dialog->num_adj,
                                     update_zoom_values,
                                     dialog);
  g_signal_handlers_unblock_by_func (dialog->denom_adj,
                                     update_zoom_values,
                                     dialog);
}
