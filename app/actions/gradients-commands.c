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

#include "libligmawidgets/ligmawidgets.h"

#include "actions-types.h"

#include "core/ligma.h"
#include "core/ligmagradient-save.h"
#include "core/ligmacontext.h"

#include "widgets/ligmacontainereditor.h"
#include "widgets/ligmacontainerview.h"
#include "widgets/ligmahelp-ids.h"

#include "dialogs/dialogs.h"

#include "gradients-commands.h"

#include "ligma-intl.h"


/*  local function prototypes  */

static void   gradients_save_as_pov_ray_response (GtkWidget    *dialog,
                                                  gint          response_id,
                                                  LigmaGradient *gradient);


/*  public functions  */

void
gradients_save_as_pov_ray_cmd_callback (LigmaAction *action,
                                        GVariant   *value,
                                        gpointer    data)
{
  LigmaContainerEditor *editor = LIGMA_CONTAINER_EDITOR (data);
  LigmaContext         *context;
  LigmaGradient        *gradient;
  GtkWidget           *dialog;

  context  = ligma_container_view_get_context (editor->view);
  gradient = ligma_context_get_gradient (context);

  if (! gradient)
    return;

#define SAVE_AS_POV_DIALOG_KEY "ligma-save-as-pov-ray-dialog"

  dialog = dialogs_get_dialog (G_OBJECT (gradient), SAVE_AS_POV_DIALOG_KEY);

  if (! dialog)
    {
      gchar *title = g_strdup_printf (_("Save '%s' as POV-Ray"),
                                      ligma_object_get_name (gradient));

      dialog = gtk_file_chooser_dialog_new (title, NULL,
                                            GTK_FILE_CHOOSER_ACTION_SAVE,

                                            _("_Cancel"), GTK_RESPONSE_CANCEL,
                                            _("_Save"),   GTK_RESPONSE_OK,

                                            NULL);

      g_free (title);

      gtk_dialog_set_default_response (GTK_DIALOG (dialog), GTK_RESPONSE_OK);
      ligma_dialog_set_alternative_button_order (GTK_DIALOG (dialog),
                                               GTK_RESPONSE_OK,
                                               GTK_RESPONSE_CANCEL,
                                               -1);

      g_object_set_data (G_OBJECT (dialog), "ligma", context->ligma);

      gtk_window_set_screen (GTK_WINDOW (dialog),
                             gtk_widget_get_screen (GTK_WIDGET (editor)));
      gtk_window_set_role (GTK_WINDOW (dialog), "ligma-gradient-save-pov");
      gtk_window_set_position (GTK_WINDOW (dialog), GTK_WIN_POS_MOUSE);

      gtk_file_chooser_set_do_overwrite_confirmation (GTK_FILE_CHOOSER (dialog),
                                                      TRUE);

      g_signal_connect (dialog, "response",
                        G_CALLBACK (gradients_save_as_pov_ray_response),
                        gradient);
      g_signal_connect (dialog, "delete-event",
                        G_CALLBACK (gtk_true),
                        NULL);

      g_signal_connect_object (dialog, "destroy",
                               G_CALLBACK (g_object_unref),
                               g_object_ref (gradient),
                               G_CONNECT_SWAPPED);

      ligma_help_connect (dialog, ligma_standard_help_func,
                         LIGMA_HELP_GRADIENT_SAVE_AS_POV, NULL, NULL);

      dialogs_attach_dialog (G_OBJECT (gradient),
                             SAVE_AS_POV_DIALOG_KEY, dialog);
    }

  gtk_window_present (GTK_WINDOW (dialog));
}


/*  private functions  */

static void
gradients_save_as_pov_ray_response (GtkWidget    *dialog,
                                    gint          response_id,
                                    LigmaGradient *gradient)
{
  if (response_id == GTK_RESPONSE_OK)
    {
      GFile  *file  = gtk_file_chooser_get_file (GTK_FILE_CHOOSER (dialog));
      GError *error = NULL;

      if (! ligma_gradient_save_pov (gradient, file, &error))
        {
          ligma_message_literal (g_object_get_data (G_OBJECT (dialog), "ligma"),
                                G_OBJECT (dialog), LIGMA_MESSAGE_ERROR,
                                error->message);
          g_clear_error (&error);
          g_object_unref (file);
          return;
        }

      g_object_unref (file);
    }

  gtk_widget_destroy (dialog);
}
