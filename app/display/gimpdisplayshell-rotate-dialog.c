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

#include "widgets/ligmadial.h"
#include "widgets/ligmahelp-ids.h"
#include "widgets/ligmaviewabledialog.h"

#include "ligmadisplay.h"
#include "ligmadisplayshell.h"
#include "ligmadisplayshell-rotate.h"
#include "ligmadisplayshell-rotate-dialog.h"

#include "ligma-intl.h"


#define RESPONSE_RESET 1


typedef struct
{
  LigmaDisplayShell *shell;
  GtkAdjustment    *rotate_adj;
  gdouble           old_angle;
} RotateDialogData;


/*  local function prototypes  */

static void  ligma_display_shell_rotate_dialog_response (GtkWidget         *widget,
                                                        gint               response_id,
                                                        RotateDialogData  *dialog);
static void      ligma_display_shell_rotate_dialog_free (RotateDialogData  *dialog);

static void      rotate_adjustment_changed             (GtkAdjustment     *adj,
                                                        RotateDialogData  *dialog);
static void      display_shell_rotated                 (LigmaDisplayShell  *shell,
                                                        RotateDialogData  *dialog);

static gboolean  deg_to_rad                            (GBinding          *binding,
                                                        const GValue      *from_value,
                                                        GValue            *to_value,
                                                        gpointer           user_data);
static gboolean  rad_to_deg                            (GBinding          *binding,
                                                        const GValue      *from_value,
                                                        GValue            *to_value,
                                                        gpointer           user_data);


/*  public functions  */

/**
 * ligma_display_shell_rotate_dialog:
 * @shell: the #LigmaDisplayShell
 *
 * Constructs and displays a dialog allowing the user to enter a
 * custom display rotate.
 **/
void
ligma_display_shell_rotate_dialog (LigmaDisplayShell *shell)
{
  RotateDialogData *data;
  LigmaImage        *image;
  GtkWidget        *toplevel;
  GtkWidget        *hbox;
  GtkWidget        *spin;
  GtkWidget        *dial;
  GtkWidget        *label;

  g_return_if_fail (LIGMA_IS_DISPLAY_SHELL (shell));

  if (shell->rotate_dialog)
    {
      gtk_window_present (GTK_WINDOW (shell->rotate_dialog));
      return;
    }

  image = ligma_display_get_image (shell->display);

  data = g_slice_new (RotateDialogData);

  data->shell     = shell;
  data->old_angle = shell->rotate_angle;

  shell->rotate_dialog =
    ligma_viewable_dialog_new (g_list_prepend (NULL, image),
                              ligma_get_user_context (shell->display->ligma),
                              _("Rotate View"), "display-rotate",
                              LIGMA_ICON_OBJECT_ROTATE_180,
                              _("Select Rotation Angle"),
                              GTK_WIDGET (shell),
                              ligma_standard_help_func,
                              LIGMA_HELP_VIEW_ROTATE_OTHER,

                              _("_Reset"),  RESPONSE_RESET,
                              _("_Cancel"), GTK_RESPONSE_CANCEL,
                              _("_OK"),     GTK_RESPONSE_OK,

                              NULL);

  ligma_dialog_set_alternative_button_order (GTK_DIALOG (shell->rotate_dialog),
                                           GTK_RESPONSE_OK,
                                           GTK_RESPONSE_CANCEL,
                                           -1);

  g_object_weak_ref (G_OBJECT (shell->rotate_dialog),
                     (GWeakNotify) ligma_display_shell_rotate_dialog_free, data);

  g_object_add_weak_pointer (G_OBJECT (shell->rotate_dialog),
                             (gpointer) &shell->rotate_dialog);

  toplevel = gtk_widget_get_toplevel (GTK_WIDGET (shell));

  gtk_window_set_transient_for (GTK_WINDOW (shell->rotate_dialog),
                                GTK_WINDOW (toplevel));
  gtk_window_set_destroy_with_parent (GTK_WINDOW (shell->rotate_dialog), TRUE);

  g_signal_connect (shell->rotate_dialog, "response",
                    G_CALLBACK (ligma_display_shell_rotate_dialog_response),
                    data);

  hbox = gtk_box_new (GTK_ORIENTATION_HORIZONTAL, 6);
  gtk_container_set_border_width (GTK_CONTAINER (hbox), 12);
  gtk_box_pack_start (GTK_BOX (gtk_dialog_get_content_area (GTK_DIALOG (shell->rotate_dialog))),
                      hbox, TRUE, TRUE, 0);
  gtk_widget_show (hbox);

  label = gtk_label_new (_("Angle:"));
  gtk_box_pack_start (GTK_BOX (hbox), label, FALSE, FALSE, 0);
  gtk_widget_show (label);

  data->rotate_adj = gtk_adjustment_new (shell->rotate_angle,
                                         0.0, 360.0, 1, 15, 0);
  spin = ligma_spin_button_new (data->rotate_adj, 1.0, 2);
  gtk_spin_button_set_numeric (GTK_SPIN_BUTTON (spin), TRUE);
  gtk_spin_button_set_wrap (GTK_SPIN_BUTTON (spin), TRUE);
  gtk_entry_set_activates_default (GTK_ENTRY (spin), TRUE);
  gtk_box_pack_start (GTK_BOX (hbox), spin, TRUE, TRUE, 0);
  gtk_widget_show (spin);

  label = gtk_label_new (_("degrees"));
  gtk_box_pack_start (GTK_BOX (hbox), label, FALSE, FALSE, 0);
  gtk_widget_show (label);

  dial = ligma_dial_new ();
  g_object_set (dial,
                "size",       32,
                "background", LIGMA_CIRCLE_BACKGROUND_PLAIN,
                "draw-beta",  FALSE,
                NULL);
  gtk_box_pack_start (GTK_BOX (hbox), dial, FALSE, FALSE, 0);
  gtk_widget_show (dial);

  g_object_bind_property_full (data->rotate_adj, "value",
                               dial,             "alpha",
                               G_BINDING_BIDIRECTIONAL |
                               G_BINDING_SYNC_CREATE,
                               deg_to_rad,
                               rad_to_deg,
                               NULL, NULL);

  g_signal_connect (data->rotate_adj, "value-changed",
                    G_CALLBACK (rotate_adjustment_changed),
                    data);
  g_signal_connect (shell, "rotated",
                    G_CALLBACK (display_shell_rotated),
                    data);

  gtk_widget_show (shell->rotate_dialog);
}

static void
ligma_display_shell_rotate_dialog_response (GtkWidget        *widget,
                                           gint              response_id,
                                           RotateDialogData *dialog)
{
  switch (response_id)
    {
    case RESPONSE_RESET:
      gtk_adjustment_set_value (dialog->rotate_adj, 0.0);
      break;

    case GTK_RESPONSE_CANCEL:
      gtk_adjustment_set_value (dialog->rotate_adj, dialog->old_angle);
      /* fall thru */

    default:
      gtk_widget_destroy (dialog->shell->rotate_dialog);
      break;
    }
}

static void
ligma_display_shell_rotate_dialog_free (RotateDialogData *dialog)
{
  g_signal_handlers_disconnect_by_func (dialog->shell,
                                        display_shell_rotated,
                                        dialog);

  g_slice_free (RotateDialogData, dialog);
}

static void
rotate_adjustment_changed (GtkAdjustment    *adj,
                           RotateDialogData *dialog)
{
  gdouble angle = gtk_adjustment_get_value (dialog->rotate_adj);

  g_signal_handlers_block_by_func (dialog->shell,
                                   display_shell_rotated,
                                   dialog);

  ligma_display_shell_rotate_to (dialog->shell, angle);

  g_signal_handlers_unblock_by_func (dialog->shell,
                                     display_shell_rotated,
                                     dialog);
}

static void
display_shell_rotated (LigmaDisplayShell  *shell,
                       RotateDialogData  *dialog)
{
  g_signal_handlers_block_by_func (dialog->rotate_adj,
                                   rotate_adjustment_changed,
                                   dialog);

  gtk_adjustment_set_value (dialog->rotate_adj, shell->rotate_angle);

  g_signal_handlers_unblock_by_func (dialog->rotate_adj,
                                     rotate_adjustment_changed,
                                     dialog);
}

static gboolean
deg_to_rad (GBinding     *binding,
            const GValue *from_value,
            GValue       *to_value,
            gpointer      user_data)
{
  gdouble value = g_value_get_double (from_value);

  value = 360.0 - value;

  value *= G_PI / 180.0;

  g_value_set_double (to_value, value);

  return TRUE;
}

static gboolean
rad_to_deg (GBinding     *binding,
            const GValue *from_value,
            GValue       *to_value,
            gpointer      user_data)
{
  gdouble value = g_value_get_double (from_value);

  value *= 180.0 / G_PI;

  value = 360.0 - value;

  g_value_set_double (to_value, value);

  return TRUE;
}
