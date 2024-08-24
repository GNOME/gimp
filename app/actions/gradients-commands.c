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

#include "libgimpwidgets/gimpwidgets.h"

#include "actions-types.h"

#include "core/gimp.h"
#include "core/gimpgradient-save.h"
#include "core/gimpcontext.h"

#include "widgets/gimpcontainereditor.h"
#include "widgets/gimpcontainerview.h"
#include "widgets/gimphelp-ids.h"
#include "widgets/gimpwidgets-utils.h"

#include "dialogs/dialogs.h"

#include "gradients-commands.h"

#include "gimp-intl.h"


/*  local function prototypes  */

static void   gradients_save_as_pov_ray_response (GtkWidget    *dialog,
                                                  gint          response_id,
                                                  GimpGradient *gradient);


/*  public functions  */

void
gradients_save_as_pov_ray_cmd_callback (GimpAction *action,
                                        GVariant   *value,
                                        gpointer    data)
{
  GimpContainerEditor *editor = GIMP_CONTAINER_EDITOR (data);
  GimpContext         *context;
  GimpGradient        *gradient;
  GtkWidget           *dialog;

  context  = gimp_container_view_get_context (editor->view);
  gradient = gimp_context_get_gradient (context);

  if (! gradient)
    return;

#define SAVE_AS_POV_DIALOG_KEY "gimp-save-as-pov-ray-dialog"

  dialog = dialogs_get_dialog (G_OBJECT (gradient), SAVE_AS_POV_DIALOG_KEY);

  if (! dialog)
    {
      gchar *title = g_strdup_printf (_("Save '%s' as POV-Ray"),
                                      gimp_object_get_name (gradient));

      dialog = gtk_file_chooser_dialog_new (title, NULL,
                                            GTK_FILE_CHOOSER_ACTION_SAVE,

                                            _("_Cancel"), GTK_RESPONSE_CANCEL,
                                            _("_Save"),   GTK_RESPONSE_OK,

                                            NULL);

      g_free (title);

      gtk_dialog_set_default_response (GTK_DIALOG (dialog), GTK_RESPONSE_OK);
      gimp_dialog_set_alternative_button_order (GTK_DIALOG (dialog),
                                                GTK_RESPONSE_OK,
                                                GTK_RESPONSE_CANCEL,
                                                -1);

      g_object_set_data (G_OBJECT (dialog), "gimp", context->gimp);

      gtk_window_set_screen (GTK_WINDOW (dialog),
                             gtk_widget_get_screen (GTK_WIDGET (editor)));
      gtk_window_set_role (GTK_WINDOW (dialog), "gimp-gradient-save-pov");
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

      gimp_help_connect (dialog, NULL, gimp_standard_help_func,
                         GIMP_HELP_GRADIENT_SAVE_AS_POV, NULL, NULL);

      dialogs_attach_dialog (G_OBJECT (gradient),
                             SAVE_AS_POV_DIALOG_KEY, dialog);
    }

  gtk_window_present (GTK_WINDOW (dialog));

#ifdef G_OS_WIN32
  gimp_window_set_title_bar_theme (context->gimp, dialog);
#endif
}


/*  private functions  */

static void
gradients_save_as_pov_ray_response (GtkWidget    *dialog,
                                    gint          response_id,
                                    GimpGradient *gradient)
{
  if (response_id == GTK_RESPONSE_OK)
    {
      GFile  *file  = gtk_file_chooser_get_file (GTK_FILE_CHOOSER (dialog));
      GError *error = NULL;

      if (! gimp_gradient_save_pov (gradient, file, &error))
        {
          gimp_message_literal (g_object_get_data (G_OBJECT (dialog), "gimp"),
                                G_OBJECT (dialog), GIMP_MESSAGE_ERROR,
                                error->message);
          g_clear_error (&error);
          g_object_unref (file);
          return;
        }

      g_object_unref (file);
    }

  gtk_widget_destroy (dialog);
}
