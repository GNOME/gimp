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

#include "display-types.h"

#include "core/ligma.h"
#include "core/ligmaviewable.h"

#include "widgets/ligmahelp-ids.h"
#include "widgets/ligmaviewabledialog.h"

#include "ligmadisplay.h"
#include "ligmadisplayshell.h"
#include "ligmadisplayshell-scale.h"
#include "ligmadisplayshell-scale-dialog.h"

#include "ligma-intl.h"


#define SCALE_EPSILON      0.0001
#define SCALE_EQUALS(a,b) (fabs ((a) - (b)) < SCALE_EPSILON)


typedef struct
{
  LigmaDisplayShell *shell;
  LigmaZoomModel    *model;
  GtkAdjustment    *scale_adj;
  GtkAdjustment    *num_adj;
  GtkAdjustment    *denom_adj;
} ScaleDialogData;


/*  local function prototypes  */

static void  ligma_display_shell_scale_dialog_response (GtkWidget        *widget,
                                                       gint              response_id,
                                                       ScaleDialogData  *dialog);
static void  ligma_display_shell_scale_dialog_free     (ScaleDialogData  *dialog);

static void  update_zoom_values                       (GtkAdjustment    *adj,
                                                       ScaleDialogData  *dialog);



/*  public functions  */

/**
 * ligma_display_shell_scale_dialog:
 * @shell: the #LigmaDisplayShell
 *
 * Constructs and displays a dialog allowing the user to enter a
 * custom display scale.
 **/
void
ligma_display_shell_scale_dialog (LigmaDisplayShell *shell)
{
  ScaleDialogData *data;
  LigmaImage       *image;
  GtkWidget       *toplevel;
  GtkWidget       *hbox;
  GtkWidget       *grid;
  GtkWidget       *spin;
  GtkWidget       *label;
  gint             num, denom, row;

  g_return_if_fail (LIGMA_IS_DISPLAY_SHELL (shell));

  if (shell->scale_dialog)
    {
      gtk_window_present (GTK_WINDOW (shell->scale_dialog));
      return;
    }

  if (SCALE_EQUALS (shell->other_scale, 0.0))
    {
      /* other_scale not yet initialized */
      shell->other_scale = ligma_zoom_model_get_factor (shell->zoom);
    }

  image = ligma_display_get_image (shell->display);

  data = g_slice_new (ScaleDialogData);

  data->shell = shell;
  data->model = g_object_new (LIGMA_TYPE_ZOOM_MODEL,
                              "value", fabs (shell->other_scale),
                              NULL);

  shell->scale_dialog =
    ligma_viewable_dialog_new (g_list_prepend (NULL, image),
                              ligma_get_user_context (shell->display->ligma),
                              _("Zoom Ratio"), "display_scale",
                              "zoom-original",
                              _("Select Zoom Ratio"),
                              GTK_WIDGET (shell),
                              ligma_standard_help_func,
                              LIGMA_HELP_VIEW_ZOOM_OTHER,

                              _("_Cancel"), GTK_RESPONSE_CANCEL,
                              _("_OK"),     GTK_RESPONSE_OK,

                              NULL);

  ligma_dialog_set_alternative_button_order (GTK_DIALOG (shell->scale_dialog),
                                           GTK_RESPONSE_OK,
                                           GTK_RESPONSE_CANCEL,
                                           -1);

  g_object_weak_ref (G_OBJECT (shell->scale_dialog),
                     (GWeakNotify) ligma_display_shell_scale_dialog_free, data);
  g_object_weak_ref (G_OBJECT (shell->scale_dialog),
                     (GWeakNotify) g_object_unref, data->model);

  g_object_add_weak_pointer (G_OBJECT (shell->scale_dialog),
                             (gpointer) &shell->scale_dialog);

  toplevel = gtk_widget_get_toplevel (GTK_WIDGET (shell));

  gtk_window_set_transient_for (GTK_WINDOW (shell->scale_dialog),
                                GTK_WINDOW (toplevel));
  gtk_window_set_destroy_with_parent (GTK_WINDOW (shell->scale_dialog), TRUE);

  g_signal_connect (shell->scale_dialog, "response",
                    G_CALLBACK (ligma_display_shell_scale_dialog_response),
                    data);

  grid = gtk_grid_new ();
  gtk_container_set_border_width (GTK_CONTAINER (grid), 12);
  gtk_grid_set_column_spacing (GTK_GRID (grid), 6);
  gtk_grid_set_row_spacing (GTK_GRID (grid), 6);
  gtk_box_pack_start (GTK_BOX (gtk_dialog_get_content_area (GTK_DIALOG (shell->scale_dialog))),
                      grid, TRUE, TRUE, 0);
  gtk_widget_show (grid);

  row = 0;

  hbox = gtk_box_new (GTK_ORIENTATION_HORIZONTAL, 6);
  ligma_grid_attach_aligned (GTK_GRID (grid), 0, row++,
                            _("Zoom ratio:"), 0.0, 0.5,
                            hbox, 1);

  ligma_zoom_model_get_fraction (data->model, &num, &denom);

  data->num_adj = gtk_adjustment_new (num, 1, 256, 1, 8, 0);
  spin = ligma_spin_button_new (data->num_adj, 1.0, 0);
  gtk_spin_button_set_numeric (GTK_SPIN_BUTTON (spin), TRUE);
  gtk_entry_set_activates_default (GTK_ENTRY (spin), TRUE);
  gtk_box_pack_start (GTK_BOX (hbox), spin, TRUE, TRUE, 0);
  gtk_widget_show (spin);

  label = gtk_label_new (":");
  gtk_box_pack_start (GTK_BOX (hbox), label, FALSE, FALSE, 0);
  gtk_widget_show (label);

  data->denom_adj = gtk_adjustment_new (denom, 1, 256, 1, 8, 0);
  spin = ligma_spin_button_new (data->denom_adj, 1.0, 0);
  gtk_spin_button_set_numeric (GTK_SPIN_BUTTON (spin), TRUE);
  gtk_entry_set_activates_default (GTK_ENTRY (spin), TRUE);
  gtk_box_pack_start (GTK_BOX (hbox), spin, TRUE, TRUE, 0);
  gtk_widget_show (spin);

  hbox = gtk_box_new (GTK_ORIENTATION_HORIZONTAL, 6);
  ligma_grid_attach_aligned (GTK_GRID (grid), 0, row++,
                            _("Zoom:"), 0.0, 0.5,
                            hbox, 1);

  data->scale_adj = gtk_adjustment_new (fabs (shell->other_scale) * 100,
                                        100.0 / 256.0, 25600.0,
                                        10, 50, 0);
  spin = ligma_spin_button_new (data->scale_adj, 1.0, 2);
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
ligma_display_shell_scale_dialog_response (GtkWidget       *widget,
                                          gint             response_id,
                                          ScaleDialogData *dialog)
{
  if (response_id == GTK_RESPONSE_OK)
    {
      gdouble scale;

      scale = gtk_adjustment_get_value (dialog->scale_adj);

      ligma_display_shell_scale (dialog->shell,
                                LIGMA_ZOOM_TO,
                                scale / 100.0,
                                LIGMA_ZOOM_FOCUS_BEST_GUESS);
    }
  else
    {
      /*  need to emit "scaled" to get the menu updated  */
      ligma_display_shell_scaled (dialog->shell);
    }

  dialog->shell->other_scale = - fabs (dialog->shell->other_scale);

  gtk_widget_destroy (dialog->shell->scale_dialog);
}

static void
ligma_display_shell_scale_dialog_free (ScaleDialogData *dialog)
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

      ligma_zoom_model_zoom (dialog->model, LIGMA_ZOOM_TO, scale / 100.0);
      ligma_zoom_model_get_fraction (dialog->model, &num, &denom);

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
